package main

import (
	"context"
	"encoding/json"
	"fmt"
	"io"
	"log"
	"math/rand"
	"net/http"
	"os"
	"path/filepath"
	"strconv"
	"strings"
	"sync"
	"text/template"
	"time"

	"github.com/google/uuid"
	"github.com/gorilla/websocket"
)

// nightLenInMinutes is the baseline night length in in-game minutes (see winTimeGameSeconds).
const nightLenInMinutes = 1

// gameSecondsPerTick is in-game seconds advanced each sim tick; keep equal to the real tick
// interval in seconds so wall clock matches the night clock (night ends after nightLenInMinutes
// of in-game time).
const gameSecondsPerTick = 5

// powerDrainPerClosedDoorPerTick is how much shared power drops per sim tick per closed door.
// (Previously this used gameSecondsPerTick, which made a single door cost 5/tick.)
const powerDrainPerClosedDoorPerTick = 1

// maxNightGuest is the highest night selectable without a logged-in WebSocket (?token= JWT).
const maxNightGuest int16 = 2

var upgrader = websocket.Upgrader{
	ReadBufferSize:  1024,
	WriteBufferSize: 1024,
	CheckOrigin:     func(r *http.Request) bool { return true },
}

type UserWsConnection struct {
	LobbyID string
	// Remote is r.RemoteAddr for this connection (string); used in stateForClient.
	Remote  string
	Conn    *websocket.Conn
	WriteMu sync.Mutex
	// NightNum from WebSocket URL ?night= (1–7); host value is copied into the lobby on start.
	NightNum int16
	// UserID from optional ?token= JWT (0 = guest); used to persist night wins in SQLite.
	UserID int64
}

type Room struct {
	EntityID   int16 `json:"entityId"`
	IsObscured bool  `json:"isObscured"`
}

type GameRoomState struct {
	// Time is cumulative in-game seconds for this night (advances gameSecondsPerTick per sim tick).
	Time            int16      `json:"time"`
	Started         bool       `json:"started"`
	Host            string     `json:"host"`
	Accepted        bool       `json:"accepted"`
	Acceptor        string     `json:"acceptor"`
	Power           int        `json:"power"`
	MusicBoxWind    int        `json:"musicBoxWind"` // 0–100; P2 winds on right panel
	LHSDoorDown     bool       `json:"lhsDoorDown"`
	RHSDoorDown     bool       `json:"rhsDoorDown"`
	P2MaskDown      bool       `json:"p2MaskDown"`
	P2OfficeDanger  int        `json:"p2OfficeDanger"`
	P1Lost          bool       `json:"-"`
	P2Lost          bool       `json:"p2Lost"`
	NightNum        int16      `json:"nightNum"`
	Rooms           []Room     `json:"rooms"`
	HostIsPlayerOne bool       `json:"-"`
	Sim             *GameState `json:"-"`

	// Per-step queues (no hold protocol). Drained in applyChargeEconomyTick.
	p1PowerQueued int
	p2PowerQueued int // P2: tap J -> p2qp
	p2MusicQueued int // P2: tap L -> p2qm (each unit +4 wind max, then global decay)
}

// SimEntityWire is a single animatronic position in the live sim (for clients).
type SimEntityWire struct {
	EntityID  int16  `json:"entityId"`
	Name      string `json:"name"`
	RoomAlias string `json:"roomAlias"`
}

// gameStateWire is the JSON sent to a single client (includes per-client isPlayerOne).
type gameStateWire struct {
	GameRoomState
	LobbyID     string          `json:"lobbyId"`
	IsPlayerOne bool            `json:"isPlayerOne"`
	P2InLobby   bool            `json:"p2InLobby"` // second player joined this lobby
	SimEntities []SimEntityWire `json:"simEntities,omitempty"`
}

func stateForClient(room *GameRoomState, remoteAddr, lobbyID string) gameStateWire {
	w := gameStateWire{
		GameRoomState: *room,
		LobbyID:       lobbyID,
		P2InLobby:     room.Accepted && room.Acceptor != "",
	}
	switch remoteAddr {
	case room.Host:
		w.IsPlayerOne = room.HostIsPlayerOne
	case room.Acceptor:
		w.IsPlayerOne = !room.HostIsPlayerOne
	default:
		w.IsPlayerOne = false
	}
	if room.Sim != nil {
		w.SimEntities = room.Sim.SnapshotSimEntities()
	}
	return w
}

type Message struct {
	Type string      `json:"type"`
	Data interface{} `json:"data"`
}

// clampNightProgressionLoggedIn lowers requested night until night 1 or previous night is in wins table.
func clampNightProgressionLoggedIn(userID int64, requested int16) int16 {
	n := clampNight(requested)
	for n >= 2 {
		ok, err := hasUserWonNight(userID, int(n)-1)
		if err != nil {
			log.Printf("hasUserWonNight user=%d: %v", userID, err)
			return 1
		}
		if ok {
			break
		}
		n--
	}
	return n
}

func clampNight(n int16) int16 {
	if n < 1 {
		return 1
	}
	if n > 7 {
		return 7
	}
	return n
}

// parseNightQuery reads ?night= from the WebSocket handshake (1–7, default 1).
func parseNightQuery(r *http.Request) int16 {
	s := strings.TrimSpace(r.URL.Query().Get("night"))
	if s == "" {
		return 1
	}
	v, err := strconv.Atoi(s)
	if err != nil || v < 1 {
		return 1
	}
	if v > 7 {
		return 7
	}
	return int16(v)
}

// parseWSUserID returns JWT user id from ?token= on the WebSocket URL, or 0 if missing/invalid.
func parseWSUserID(r *http.Request) int64 {
	tok := strings.TrimSpace(r.URL.Query().Get("token"))
	if tok == "" {
		return 0
	}
	claims, err := parseTokenString(tok)
	if err != nil {
		return 0
	}
	return claims.UserID
}

// winTimeGameSeconds returns in-game seconds survived to win (same unit as room.Time).
// room.Time is cumulative in-game seconds; when it reaches this value, handleNightEnd runs.
// Higher nights end sooner (demo curve); maps use NightNum in CreateInitialMap / CreateP2Map.
func winTimeGameSeconds(night int16) int {
	n := int(clampNight(night))
	base := nightLenInMinutes * 60
	// Shorter nights for higher night numbers (tunable with sim difficulty).
	penalty := (n - 1) * (base / 7)
	out := base - penalty
	if out < base/4 {
		return base / 4
	}
	return out
}

var connectedUsersSet = make(map[string]*UserWsConnection)
var lobbySet = make(map[string]*GameRoomState)

func writeJSONToUser(u *UserWsConnection, t string, data interface{}) {
	if u == nil || u.Conn == nil {
		return
	}
	u.WriteMu.Lock()
	defer u.WriteMu.Unlock()
	_ = u.Conn.WriteJSON(Message{Type: t, Data: data})
}

// broadcastStateToLobby pushes sim/night time only to WebSockets in that lobby
// (after a sim step).
func broadcastStateToLobby(lobbyID string) {
	room, ok := lobbySet[lobbyID]
	if !ok || !room.Started {
		return
	}
	for _, u := range connectedUsersSet {
		if u == nil || u.LobbyID != lobbyID || u.Conn == nil {
			continue
		}
		writeJSONToUser(u, "state", stateForClient(room, u.Remote, lobbyID))
	}
}

func handleNightEnd(lobbyID string) {
	room, ok := lobbySet[lobbyID]
	winNight := int(1)
	if ok && room != nil {
		winNight = int(clampNight(room.NightNum))
	}

	var batch []*UserWsConnection
	for _, u := range connectedUsersSet {
		if u == nil || u.LobbyID != lobbyID || u.Conn == nil {
			continue
		}
		batch = append(batch, u)
	}
	for _, u := range batch {
		u.WriteMu.Lock()
		_ = u.Conn.WriteJSON(Message{Type: "status", Data: "win"})
		_ = u.Conn.Close()
		u.Conn = nil
		u.WriteMu.Unlock()
		if u.UserID > 0 {
			if err := recordWin(u.UserID, winNight); err != nil {
				log.Printf("lobby %s: record win night %d for user %d: %v", lobbyID, winNight, u.UserID, err)
			}
		}
	}
	cleanupLobby(lobbyID)
}

func handleLobbyLose(lobbyID string, killer string) {
	var batch []*UserWsConnection
	for _, u := range connectedUsersSet {
		if u == nil || u.LobbyID != lobbyID || u.Conn == nil {
			continue
		}
		batch = append(batch, u)
	}
	for _, u := range batch {
		u.WriteMu.Lock()
		status := "lose"
		if killer != "" {
			status = "lose:" + killer
		}
		_ = u.Conn.WriteJSON(Message{Type: "status", Data: status})
		_ = u.Conn.Close()
		u.Conn = nil
		u.WriteMu.Unlock()
	}
	cleanupLobby(lobbyID)
}

func handlePlayerTwoLose(lobbyID string, killer string) {
	room, ok := lobbySet[lobbyID]
	if !ok || room == nil {
		return
	}
	// Hard invariant: when server believes P2 mask is down, P2 cannot die.
	if room.P2MaskDown {
		log.Printf("lobby %s: suppressed P2 lose (killer=%s) because mask is down", lobbyID, killer)
		return
	}
	room.P2Lost = true
	for _, u := range connectedUsersSet {
		if u == nil || u.LobbyID != lobbyID || u.Conn == nil {
			continue
		}
		if !isPlayerTwoUser(room, u.Remote) {
			continue
		}
		u.WriteMu.Lock()
		status := "lose"
		if killer != "" {
			status = "lose:" + killer
		}
		_ = u.Conn.WriteJSON(Message{Type: "status", Data: status})
		_ = u.Conn.Close()
		u.Conn = nil
		u.WriteMu.Unlock()
		// Keep the lobby alive for P1; this slot can reconnect/rejoin later.
		u.LobbyID = ""
	}
	if room.P1Lost && room.P2Lost {
		cleanupLobby(lobbyID)
	}
}

func handlePlayerOneLose(lobbyID string, killer string) {
	room, ok := lobbySet[lobbyID]
	if !ok || room == nil {
		return
	}
	room.P1Lost = true
	for _, u := range connectedUsersSet {
		if u == nil || u.LobbyID != lobbyID || u.Conn == nil {
			continue
		}
		if !isPlayerOneUser(room, u.Remote) {
			continue
		}
		u.WriteMu.Lock()
		status := "lose"
		if killer != "" {
			status = "lose:" + killer
		}
		_ = u.Conn.WriteJSON(Message{Type: "status", Data: status})
		_ = u.Conn.Close()
		u.Conn = nil
		u.WriteMu.Unlock()
		// Keep the lobby alive for P2; this slot can reconnect/rejoin later.
		u.LobbyID = ""
	}
	if room.P1Lost && room.P2Lost {
		cleanupLobby(lobbyID)
	}
}

func isPlayerOneUser(room *GameRoomState, remote string) bool {
	if room == nil {
		return false
	}
	if remote == room.Host {
		return room.HostIsPlayerOne
	}
	if remote == room.Acceptor {
		return !room.HostIsPlayerOne
	}
	return false
}

func isPlayerTwoUser(room *GameRoomState, remote string) bool {
	if room == nil {
		return false
	}
	if remote == room.Host {
		return !room.HostIsPlayerOne
	}
	if remote == room.Acceptor {
		return room.HostIsPlayerOne
	}
	return false
}

// applyChargeEconomyTick runs generator + music box once per sim tick; gameSeconds is how many
// in-game seconds this tick represents (music wind decays by that much).
func applyChargeEconomyTick(room *GameRoomState, gameSeconds int) {
	if room == nil || gameSeconds <= 0 {
		return
	}
	if room.p1PowerQueued > 0 {
		room.Power += room.p1PowerQueued
		room.p1PowerQueued = 0
	}

	if !room.P2Lost && !room.P2MaskDown {
		if room.p2PowerQueued > 0 {
			room.Power += room.p2PowerQueued
			room.p2PowerQueued = 0
		}
		n := room.p2MusicQueued
		room.p2MusicQueued = 0
		for i := 0; i < n; i++ {
			if room.MusicBoxWind < 100 {
				room.MusicBoxWind += 4
			}
		}
	} else {
		room.p2PowerQueued = 0
		room.p2MusicQueued = 0
	}
	if room.MusicBoxWind > 0 {
		d := gameSeconds
		if d > room.MusicBoxWind {
			d = room.MusicBoxWind
		}
		room.MusicBoxWind -= d
	}
	if room.Power > 100 {
		room.Power = 100
	}
}

// advanceLobbySimOneTick advances room.Time, economy, and sim by one tick (gameSecondsPerTick
// in-game seconds). Used by the periodic ticker and by manual "step" messages.
func advanceLobbySimOneTick(lobbyID string) {
	room, exists := lobbySet[lobbyID]
	if !exists || room == nil || !room.Started || room.Sim == nil {
		return
	}

	room.Time += int16(gameSecondsPerTick)
	if room.LHSDoorDown {
		room.Power -= powerDrainPerClosedDoorPerTick
	}
	if room.RHSDoorDown {
		room.Power -= powerDrainPerClosedDoorPerTick
	}
	applyChargeEconomyTick(room, gameSecondsPerTick)
	if room.Power <= 0 {
		handleLobbyLose(lobbyID, "power_out")
		return
	}
	room.Sim.Step(lobbyID)
	if room.Sim.AnyEntityInRoom("player_one_office") {
		killer, _ := room.Sim.FirstEntityNameInRoom("player_one_office")
		handlePlayerOneLose(lobbyID, killer)
	}
	if room.Sim.AnyEntityInRoom("player_two_office") {
		if room.P2MaskDown {
			room.P2OfficeDanger = 0
			room.Sim.MoveEntitiesBackFromRoom("player_two_office")
		} else {
			room.P2OfficeDanger++
			if room.P2OfficeDanger >= 2 {
				killer, _ := room.Sim.FirstEntityNameInRoom("player_two_office")
				handlePlayerTwoLose(lobbyID, killer)
				room.P2OfficeDanger = 0
				room.Sim.MoveEntitiesBackFromRoom("player_two_office")
			}
		}
	} else {
		room.P2OfficeDanger = 0
	}
	broadcastStateToLobby(lobbyID)

	if int(room.Time) >= winTimeGameSeconds(room.NightNum) {
		handleNightEnd(lobbyID)
	}
}

// runSimStepTicker runs one full sim tick for every started lobby on a fixed interval.
func runSimStepTicker() {
	ticker := time.NewTicker(gameSecondsPerTick * time.Second)
	defer ticker.Stop()
	for range ticker.C {
		ids := make([]string, 0, len(lobbySet))
		for id := range lobbySet {
			ids = append(ids, id)
		}
		for _, lobbyID := range ids {
			advanceLobbySimOneTick(lobbyID)
		}
	}
}

func main() {
	rand.Seed(time.Now().UnixNano())
	go runSimStepTicker()

	if err := initDB("app.db"); err != nil {
		log.Fatal(err)
	}
	defer db.Close()

	authTmpl := template.Must(template.ParseFiles("auth.html"))

	if b, err := os.ReadFile("play.html"); err != nil {
		log.Printf("warning: play.html not read (%v); / will be empty", err)
	} else {
		playPageHTML = string(b)
	}

	wasmRoot := strings.TrimSpace(os.Getenv("WASMDIST"))
	if wasmRoot == "" {
		wasmRoot = "wasmdist"
	}
	if _, err := os.Stat(filepath.Join(wasmRoot, "game.html")); err != nil {
		alt := filepath.Join("server", "wasmdist")
		if _, err2 := os.Stat(filepath.Join(alt, "game.html")); err2 == nil {
			wasmRoot = alt
		} else {
			wd, _ := os.Getwd()
			log.Fatalf("wasmdist/game.html not found (looked in %q and %q). cwd=%q — run: cd client_wasm && ./run.sh",
				wasmRoot, alt, wd)
		}
	}
	absW, _ := filepath.Abs(wasmRoot)
	log.Printf("serving /game/* from %s", absW)

	http.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "text/html; charset=utf-8")
		_, _ = io.WriteString(w, playPageHTML)
	})

	http.Handle("/game/", http.StripPrefix("/game/", http.FileServer(http.Dir(wasmRoot))))

	http.HandleFunc("/auth", func(w http.ResponseWriter, r *http.Request) {
		authTmpl.Execute(w, nil)
	})

	http.HandleFunc("/api/register", registerHandler)
	http.HandleFunc("/api/login", loginHandler)
	http.HandleFunc("/api/me", meHandler)

	http.HandleFunc("/echo", func(w http.ResponseWriter, r *http.Request) {
		conn, err := upgrader.Upgrade(w, r, nil)
		if err != nil {
			return
		}
		defer conn.Close()

		addr := r.RemoteAddr
		if _, exists := connectedUsersSet[addr]; !exists {
			connectedUsersSet[addr] = &UserWsConnection{Remote: addr, NightNum: 1}
		}
		user := connectedUsersSet[addr]
		user.Conn = conn
		user.NightNum = parseNightQuery(r)
		user.UserID = parseWSUserID(r)
		if user.UserID == 0 && user.NightNum > maxNightGuest {
			user.NightNum = maxNightGuest
		} else if user.UserID > 0 {
			user.NightNum = clampNightProgressionLoggedIn(user.UserID, user.NightNum)
		}

		ctx, cancel := context.WithCancel(context.Background())
		defer cancel()

		// Keepalive only when this socket is not in a lobby. Game state is pushed
		// from runSimStepTicker (lobby-scoped) after each sim step.
		go func() {
			ping := time.NewTicker(30 * time.Second)
			defer ping.Stop()
			for {
				select {
				case <-ping.C:
					u := connectedUsersSet[addr]
					if u == nil || u.Conn == nil {
						return
					}
					_, inLobby := lobbySet[u.LobbyID]
					if inLobby {
						continue
					}
					writeJSONToUser(u, "ping", "ping")
				case <-ctx.Done():
					return
				}
			}
		}()

		for {
			_, msg, err := conn.ReadMessage()
			if err != nil {
				handleDisconnect(addr)
				return
			}
			fmt.Printf("%s", msg)
			var msgJson struct {
				Type    string `json:"type"`
				Content string `json:"content"`
			}

			if err := json.Unmarshal(msg, &msgJson); err != nil {
				writeJSONToUser(connectedUsersSet[addr], "error", "invalid-json")
				continue
			}

			switch msgJson.Type {

			case "join":
				handleJoin(conn, r, msgJson.Content)

			case "chat":
				if msgJson.Content == "invite" {
					handleInvite(conn, r)
				}
			case "check":
				handleCheckCamera(conn, r, msgJson.Content)
			case "status":
				fmt.Printf("Status: %v", msgJson)
				if msgJson.Content == "start" {
					fmt.Println("Starting game")
					handleStartGame(conn, r)
				}
			case "step":
				handleStepGame(conn, r)
			case "action":
				handleActionGame(conn, r, msgJson.Content)

			default:
				writeJSONToUser(connectedUsersSet[addr], "error", "unknown-type")
			}
		}
	})

	fmt.Println("Running Server on http://localhost:6789")
	http.ListenAndServe(":6789", nil)
}

func newGameRoomState(host string) GameRoomState {
	return GameRoomState{
		Time:           0,
		Started:        false,
		Host:           host,
		Accepted:       false,
		Acceptor:       "",
		Power:          30,
		MusicBoxWind:   0,
		LHSDoorDown:    false,
		RHSDoorDown:    false,
		P2MaskDown:     false,
		P2OfficeDanger: 0,
		Rooms: []Room{
			{
				EntityID:   0,
				IsObscured: false,
			},
		},
		NightNum: 1,
		Sim:      nil,
	}
}

func handleCheckCamera(conn *websocket.Conn, r *http.Request, content string) {
	user := connectedUsersSet[r.RemoteAddr]
	room, ok := lobbySet[user.LobbyID]
	if !ok || room.Sim == nil {
		writeJSONToUser(user, "error", "game-not-started")
		return
	}
	alias := strings.TrimSpace(content)
	if alias == "" {
		writeJSONToUser(user, "error", "bad-room-alias")
		return
	}
	node := room.Sim.FindNodeByAlias(alias)
	if node == nil {
		writeJSONToUser(user, "error", "unknown-room")
		return
	}
	type entWire struct {
		EntityID int16  `json:"entityId"`
		Name     string `json:"name"`
	}
	list := make([]entWire, 0, len(node.Entities))
	for _, e := range node.Entities {
		list = append(list, entWire{EntityID: e.Id, Name: e.Name})
	}
	writeJSONToUser(user, "checkCamera", map[string]interface{}{
		"roomAlias": alias,
		"entities":  list,
	})
}

func handleInvite(conn *websocket.Conn, r *http.Request) {
	user := connectedUsersSet[r.RemoteAddr]

	if user.LobbyID != "" {
		room := lobbySet[user.LobbyID]
		if room.Host == r.RemoteAddr {
			writeJSONToUser(user, "invite", user.LobbyID)
			return
		}
		writeJSONToUser(user, "error", "not-host")
		return
	}

	id := uuid.New().String()
	game := newGameRoomState(r.RemoteAddr)

	lobbySet[id] = &game
	user.LobbyID = id

	writeJSONToUser(user, "invite", id)
}

func handleJoin(conn *websocket.Conn, r *http.Request, lobbyID string) {
	room, exists := lobbySet[lobbyID]
	if !exists {
		writeJSONToUser(connectedUsersSet[r.RemoteAddr], "error", "lobby-not-found")
		return
	}

	if room.Accepted || room.Host == r.RemoteAddr {
		writeJSONToUser(connectedUsersSet[r.RemoteAddr], "error", "lobby-full")
		return
	}

	user := connectedUsersSet[r.RemoteAddr]
	user.LobbyID = lobbyID

	room.Accepted = true
	room.Acceptor = r.RemoteAddr

	writeJSONToUser(user, "joined", lobbyID)
}

func handleStartGame(conn *websocket.Conn, r *http.Request) {
	user := connectedUsersSet[r.RemoteAddr]
	room, exists := lobbySet[user.LobbyID]

	if !exists {
		writeJSONToUser(user, "error", "no-lobby")
		return
	}

	if room.Host != r.RemoteAddr {
		writeJSONToUser(user, "error", "not-host")
		return
	}

	wantNight := clampNight(user.NightNum)
	if wantNight > maxNightGuest && user.UserID <= 0 {
		writeJSONToUser(user, "error", "Log in at /auth (game needs your JWT on the WebSocket) to play nights 3–7.")
		return
	}
	if user.UserID > 0 && wantNight >= 2 {
		prevOK, err := hasUserWonNight(user.UserID, int(wantNight)-1)
		if err != nil {
			writeJSONToUser(user, "error", "Could not verify night progression; try again.")
			return
		}
		if !prevOK {
			writeJSONToUser(user, "error", "Win the previous night while logged in (saved in your profile) before starting this night.")
			return
		}
	}

	room.HostIsPlayerOne = rand.Intn(2) == 1
	room.Started = true
	room.Power = 30
	room.MusicBoxWind = 100
	room.LHSDoorDown = false
	room.RHSDoorDown = false
	room.P2MaskDown = false
	room.P2OfficeDanger = 0
	room.P1Lost = false
	room.P2Lost = false
	room.p1PowerQueued = 0
	room.p2PowerQueued = 0
	room.p2MusicQueued = 0
	room.NightNum = wantNight
	room.Sim = NewGameState(room.NightNum)
	room.Sim.SpawnEntities()
	log.Printf("lobby %s: SpawnEntities done, sim ready night=%d winAtGameSec=%d", user.LobbyID, room.NightNum, winTimeGameSeconds(room.NightNum))
	lid := user.LobbyID
	for _, u := range connectedUsersSet {
		if u == nil || u.LobbyID != lid || u.Conn == nil {
			continue
		}
		writeJSONToUser(u, "state", stateForClient(room, u.Remote, lid))
	}
}

func handleStepGame(conn *websocket.Conn, r *http.Request) {
	user := connectedUsersSet[r.RemoteAddr]
	if user == nil {
		return
	}
	room, exists := lobbySet[user.LobbyID]
	if !exists {
		writeJSONToUser(user, "error", "no-lobby")
		return
	}
	if !room.Started || room.Sim == nil {
		writeJSONToUser(user, "error", "game-not-started")
		return
	}
	advanceLobbySimOneTick(user.LobbyID)
}

func handleActionGame(conn *websocket.Conn, r *http.Request, content string) {
	user := connectedUsersSet[r.RemoteAddr]
	if user == nil {
		return
	}
	room, exists := lobbySet[user.LobbyID]
	if !exists || !room.Started || room.Sim == nil {
		return
	}
	parts := strings.Split(content, ":")
	if len(parts) < 2 {
		return
	}
	switch parts[0] {
	case "p1q":
		if len(parts) != 2 {
			return
		}
		if !isPlayerOneUser(room, r.RemoteAddr) {
			return
		}
		n := 1
		if x, err := strconv.Atoi(parts[1]); err == nil && x > 0 {
			n = x
			if n > 32 {
				n = 32
			}
		}
		room.p1PowerQueued += n
		if room.p1PowerQueued > 500 {
			room.p1PowerQueued = 500
		}
		return
	case "p2qp":
		if len(parts) != 2 {
			return
		}
		if !isPlayerTwoUser(room, r.RemoteAddr) {
			return
		}
		n := 1
		if x, err := strconv.Atoi(parts[1]); err == nil && x > 0 {
			n = x
			if n > 32 {
				n = 32
			}
		}
		if room.P2MaskDown || room.P2Lost {
			return
		}
		room.p2PowerQueued += n
		if room.p2PowerQueued > 500 {
			room.p2PowerQueued = 500
		}
		return
	case "p2qm":
		if len(parts) != 2 {
			return
		}
		if !isPlayerTwoUser(room, r.RemoteAddr) {
			return
		}
		n := 1
		if x, err := strconv.Atoi(parts[1]); err == nil && x > 0 {
			n = x
			if n > 32 {
				n = 32
			}
		}
		if room.P2MaskDown || room.P2Lost {
			return
		}
		room.p2MusicQueued += n
		if room.p2MusicQueued > 200 {
			room.p2MusicQueued = 200
		}
		return
	case "door":
		if len(parts) != 3 {
			return
		}
		if !isPlayerOneUser(room, r.RemoteAddr) {
			return
		}
		side := parts[1]
		closed := parts[2] == "closed"
		switch side {
		case "lhs":
			room.LHSDoorDown = closed
			if room.Sim.LHSDoor != nil {
				room.Sim.LHSDoor.IsBlocked = closed
			}
		case "rhs":
			room.RHSDoorDown = closed
			if room.Sim.RHSDoor != nil {
				room.Sim.RHSDoor.IsBlocked = closed
			}
		}
	case "mask":
		if len(parts) != 3 {
			return
		}
		if !isPlayerTwoUser(room, r.RemoteAddr) {
			log.Printf("lobby %s: ignored mask action from non-P2 remote=%s content=%q", user.LobbyID, r.RemoteAddr, content)
			return
		}
		if parts[1] != "p2" {
			return
		}
		room.P2MaskDown = parts[2] == "down"
		if room.P2MaskDown {
			room.p2PowerQueued = 0
			room.p2MusicQueued = 0
		}
		log.Printf("lobby %s: p2 mask set to %v by remote=%s", user.LobbyID, room.P2MaskDown, r.RemoteAddr)
		// If mask goes down after an entry race, immediately deflect office occupants.
		if room.P2MaskDown && room.Sim.AnyEntityInRoom("player_two_office") {
			room.P2OfficeDanger = 0
			room.Sim.MoveEntitiesBackFromRoom("player_two_office")
		}
	}
	broadcastStateToLobby(user.LobbyID)
}

func cleanupLobby(lobbyID string) {
	delete(lobbySet, lobbyID)

	for _, user := range connectedUsersSet {
		if user.LobbyID == lobbyID {
			user.LobbyID = ""
		}
	}
}

// handleDisconnect runs when a WebSocket read fails or the client closes the connection.
// It notifies other players in the same lobby, then removes the lobby so idle games do not
// accumulate on the server.
func handleDisconnect(remote string) {
	user := connectedUsersSet[remote]
	if user == nil {
		return
	}
	lobbyID := user.LobbyID
	if lobbyID == "" {
		user.Conn = nil
		return
	}
	if _, ok := lobbySet[lobbyID]; !ok {
		user.LobbyID = ""
		user.Conn = nil
		return
	}

	for _, u := range connectedUsersSet {
		if u == nil || u.LobbyID != lobbyID || u.Remote == remote || u.Conn == nil {
			continue
		}
		writeJSONToUser(u, "status", "peer-disconnected")
		u.WriteMu.Lock()
		_ = u.Conn.Close()
		u.Conn = nil
		u.WriteMu.Unlock()
	}

	cleanupLobby(lobbyID)
	user.Conn = nil
}

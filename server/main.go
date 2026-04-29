package main

import (
	"context"
	"encoding/json"
	"fmt"
	"log"
	"math/rand"
	"net/http"
	"strings"
	"sync"
	"text/template"
	"time"

	"github.com/google/uuid"
	"github.com/gorilla/websocket"
)

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
}

type Room struct {
	EntityID   int16 `json:"entityId"`
	IsObscured bool  `json:"isObscured"`
}

type GameRoomState struct {
	Time            int16      `json:"time"`
	Started         bool       `json:"started"`
	Host            string     `json:"host"`
	Accepted        bool       `json:"accepted"`
	Acceptor        string     `json:"acceptor"`
	Power           int        `json:"power"`
	LHSDoorDown     bool       `json:"lhsDoorDown"`
	RHSDoorDown     bool       `json:"rhsDoorDown"`
	Rooms           []Room     `json:"rooms"`
	HostIsPlayerOne bool       `json:"-"`
	Sim             *GameState `json:"-"`
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
	SimEntities []SimEntityWire `json:"simEntities,omitempty"`
}

func stateForClient(room *GameRoomState, remoteAddr, lobbyID string) gameStateWire {
	w := gameStateWire{GameRoomState: *room, LobbyID: lobbyID}
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

const NIGHT_IN_MINUTES = 99

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

// runSimStepTicker advances night time and the sim for each started lobby
// on a single shared interval (one tick every 10s; each lobby has its own room).
func runSimStepTicker() {
	ticker := time.NewTicker(10 * time.Second)
	defer ticker.Stop()
	for range ticker.C {
		var toEnd []string
		for lobbyID, room := range lobbySet {
			if room == nil || !room.Started || room.Sim == nil {
				continue
			}
			room.Time++
			if false {
				room.Sim.Step(lobbyID)
			}
			broadcastStateToLobby(lobbyID)
			if int(room.Time) > NIGHT_IN_MINUTES*60 {
				toEnd = append(toEnd, lobbyID)
			}
		}
		for _, id := range toEnd {
			handleNightEnd(id)
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

	tmpl := template.Must(template.ParseFiles("index.html"))
	authTmpl := template.Must(template.ParseFiles("auth.html"))

	http.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
		tmpl.Execute(w, nil)
	})

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
			connectedUsersSet[addr] = &UserWsConnection{Remote: addr}
		}
		user := connectedUsersSet[addr]
		user.Conn = conn

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
		Time:        0,
		Started:     false,
		Host:        host,
		Accepted:    false,
		Acceptor:    "",
		Power:       30,
		LHSDoorDown: false,
		RHSDoorDown: false,
		Rooms: []Room{
			{
				EntityID:   0,
				IsObscured: false,
			},
		},
		Sim: nil,
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

	room.HostIsPlayerOne = rand.Intn(2) == 1
	room.Started = true
	room.Power = 30
	room.LHSDoorDown = false
	room.RHSDoorDown = false
	room.Sim = NewGameState()
	room.Sim.SpawnEntities()
	log.Printf("lobby %s: SpawnEntities done, sim ready", user.LobbyID)
	writeJSONToUser(user, "state", stateForClient(room, r.RemoteAddr, user.LobbyID))
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

	room.Time++
	if room.LHSDoorDown {
		room.Power--
	}
	if room.RHSDoorDown {
		room.Power--
	}
	if room.Power <= 0 {
		handleLobbyLose(user.LobbyID, "power_out")
		return
	}
	room.Sim.Step(user.LobbyID)
	if room.Sim.AnyEntityInRoom("player_one_office") {
		killer, _ := room.Sim.FirstEntityNameInRoom("player_one_office")
		handleLobbyLose(user.LobbyID, killer)
		return
	}
	broadcastStateToLobby(user.LobbyID)

	if int(room.Time) > NIGHT_IN_MINUTES*60 {
		handleNightEnd(user.LobbyID)
	}
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
	if !isPlayerOneUser(room, r.RemoteAddr) {
		return
	}

	parts := strings.Split(content, ":")
	if len(parts) != 3 || parts[0] != "door" {
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
}

func cleanupLobby(lobbyID string) {
	delete(lobbySet, lobbyID)

	for _, user := range connectedUsersSet {
		if user.LobbyID == lobbyID {
			user.LobbyID = ""
		}
	}
}

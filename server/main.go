package main

import (
	"context"
	"encoding/json"
	"fmt"
	"log"
	"math/rand"
	"net/http"
	"strings"
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
	LobbyID string `json:"lobbyId"`
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
	Rooms           []Room     `json:"rooms"`
	HostIsPlayerOne bool       `json:"-"`
	Sim             *GameState `json:"-"`
}

// gameStateWire is the JSON sent to a single client (includes per-client isPlayerOne).
type gameStateWire struct {
	GameRoomState
	IsPlayerOne bool `json:"isPlayerOne"`
}

func stateForClient(room *GameRoomState, remoteAddr string) gameStateWire {
	w := gameStateWire{GameRoomState: *room}
	switch remoteAddr {
	case room.Host:
		w.IsPlayerOne = room.HostIsPlayerOne
	case room.Acceptor:
		w.IsPlayerOne = !room.HostIsPlayerOne
	default:
		w.IsPlayerOne = false
	}
	return w
}

type Message struct {
	Type string      `json:"type"`
	Data interface{} `json:"data"`
}

const NIGHT_IN_MINUTES = 1

var connectedUsersSet = make(map[string]*UserWsConnection)
var lobbySet = make(map[string]*GameRoomState)

func writeJSON(conn *websocket.Conn, t string, data interface{}) {
	_ = conn.WriteJSON(Message{
		Type: t,
		Data: data,
	})
}

// runSimStepTicker advances night time and the entity sim once per second for
// every started lobby (not tied to which client owns the WebSocket).
func runSimStepTicker() {
	ticker := time.NewTicker(time.Second)
	defer ticker.Stop()
	var inter = 0
	for range ticker.C {
		for lobbyID, room := range lobbySet {
			if room == nil || !room.Started || room.Sim == nil {
				continue
			}
			room.Time++
			fmt.Printf(lobbyID)
			if inter < 1 {
				// room.Sim.Step(lobbyID)
				inter++
			}
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

		if _, exists := connectedUsersSet[r.RemoteAddr]; !exists {
			connectedUsersSet[r.RemoteAddr] = &UserWsConnection{LobbyID: ""}
		}

		ctx, cancel := context.WithCancel(context.Background())
		defer cancel()

		go func(ctx context.Context) {
			ticker := time.NewTicker(time.Second)
			defer ticker.Stop()

			for {
				select {
				case <-ticker.C:
					user := connectedUsersSet[r.RemoteAddr]
					room, exists := lobbySet[user.LobbyID]

					if exists {
						if room.Started {
							writeJSON(conn, "state", stateForClient(room, r.RemoteAddr))
						}

						if room.Time > NIGHT_IN_MINUTES*60 {
							writeJSON(conn, "status", "win")
							cleanupLobby(user.LobbyID)
							conn.Close()
							return
						}
					} else {
						writeJSON(conn, "ping", "ping")
					}

				case <-ctx.Done():
					return
				}
			}
		}(ctx)

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
				writeJSON(conn, "error", "invalid-json")
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

			default:
				writeJSON(conn, "error", "unknown-type")
			}
		}
	})

	fmt.Println("Running Server on http://localhost:6789")
	http.ListenAndServe(":6789", nil)
}

func newGameRoomState(host string) GameRoomState {
	return GameRoomState{
		Time:     0,
		Started:  false,
		Host:     host,
		Accepted: false,
		Acceptor: "",
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
		writeJSON(conn, "error", "game-not-started")
		return
	}
	alias := strings.TrimSpace(content)
	if alias == "" {
		writeJSON(conn, "error", "bad-room-alias")
		return
	}
	node := room.Sim.FindNodeByAlias(alias)
	if node == nil {
		writeJSON(conn, "error", "unknown-room")
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
	writeJSON(conn, "checkCamera", map[string]interface{}{
		"roomAlias": alias,
		"entities":  list,
	})
}

func handleInvite(conn *websocket.Conn, r *http.Request) {
	user := connectedUsersSet[r.RemoteAddr]

	if user.LobbyID != "" {
		room := lobbySet[user.LobbyID]
		if room.Host == r.RemoteAddr {
			writeJSON(conn, "invite", user.LobbyID)
			return
		}
		writeJSON(conn, "error", "not-host")
		return
	}

	id := uuid.New().String()
	game := newGameRoomState(r.RemoteAddr)

	lobbySet[id] = &game
	user.LobbyID = id

	writeJSON(conn, "invite", id)
}

func handleJoin(conn *websocket.Conn, r *http.Request, lobbyID string) {
	room, exists := lobbySet[lobbyID]
	if !exists {
		writeJSON(conn, "error", "lobby-not-found")
		return
	}

	if room.Accepted || room.Host == r.RemoteAddr {
		writeJSON(conn, "error", "lobby-full")
		return
	}

	user := connectedUsersSet[r.RemoteAddr]
	user.LobbyID = lobbyID

	room.Accepted = true
	room.Acceptor = r.RemoteAddr

	writeJSON(conn, "joined", lobbyID)
}

func handleStartGame(conn *websocket.Conn, r *http.Request) {
	user := connectedUsersSet[r.RemoteAddr]
	room, exists := lobbySet[user.LobbyID]

	if !exists {
		writeJSON(conn, "error", "no-lobby")
		return
	}

	if room.Host != r.RemoteAddr {
		writeJSON(conn, "error", "not-host")
		return
	}

	room.HostIsPlayerOne = rand.Intn(2) == 1
	room.Started = true
	room.Sim = NewGameState()
	room.Sim.SpawnEntities()
	log.Printf("lobby %s: SpawnEntities done, sim ready", user.LobbyID)
	writeJSON(conn, "state", stateForClient(room, r.RemoteAddr))
}

func cleanupLobby(lobbyID string) {
	delete(lobbySet, lobbyID)

	for _, user := range connectedUsersSet {
		if user.LobbyID == lobbyID {
			user.LobbyID = ""
		}
	}
}

package main

import (
	"fmt"
	"log"
)

// --- Entity and Graph Definitions ---
type Entity struct {
	Id                 int16
	Name               string
	AggressionMultiple int16
	Position           int16
	TimeInRoom         int16
	PreferredNextIndex int16
}

type GameGraphNode struct {
	RoomID    int16
	AliasName string
	Entities  []Entity
	NextNodes []*GameGraphNode
	PrevNode  *GameGraphNode
	IsBlocked bool
}

func (g *GameGraphNode) AddNext(nodes ...*GameGraphNode) {
	for _, n := range nodes {
		if n == nil {
			continue
		}
		g.NextNodes = append(g.NextNodes, n)
		n.PrevNode = g
	}
}

// --- Entity Tracker ---
type EntityTracker struct {
	EntityPositions map[int16]*GameGraphNode
	EntityPaths     map[int16][]*GameGraphNode
	ChokeCooldowns  map[string]int
}

func NewEntityTracker() *EntityTracker {
	return &EntityTracker{
		EntityPositions: make(map[int16]*GameGraphNode),
		EntityPaths:     make(map[int16][]*GameGraphNode),
		ChokeCooldowns:  make(map[string]int),
	}
}

func (et *EntityTracker) AdvanceTickCooldowns() {
	for alias, ticks := range et.ChokeCooldowns {
		if ticks <= 1 {
			delete(et.ChokeCooldowns, alias)
			continue
		}
		et.ChokeCooldowns[alias] = ticks - 1
	}
}

func (et *EntityTracker) AddEntity(e Entity, node *GameGraphNode) {
	et.EntityPositions[e.Id] = node
	et.EntityPaths[e.Id] = []*GameGraphNode{node}
	node.Entities = append(node.Entities, e)
}

func (et *EntityTracker) MoveEntity(e Entity) {
	curr := et.EntityPositions[e.Id]
	if curr == nil || len(curr.NextNodes) == 0 {
		return
	}
	// Door nodes are the pre-office choke points; when blocked, do not allow
	// passing through to office even if the next node itself is unblocked.
	if curr.IsBlocked &&
		(curr.AliasName == "lhs_door" || curr.AliasName == "rhs_door") {
		if isSingleOccupancyChoke(curr.AliasName) {
			et.ChokeCooldowns[curr.AliasName] = 1
		}
		// Closed office door means a full retreat, not a one-node shuffle.
		et.ResetToStart(e)
		return
	}

	var next *GameGraphNode
	if int(e.PreferredNextIndex) < len(curr.NextNodes) {
		next = curr.NextNodes[e.PreferredNextIndex]
	} else {
		next = curr.NextNodes[0]
	}

	if next.IsBlocked {
		et.ResetToStart(e)
		return
	}
	if isSingleOccupancyChoke(next.AliasName) {
		if len(next.Entities) > 0 {
			return
		}
		if et.ChokeCooldowns[next.AliasName] > 0 {
			return
		}
	}

	if isSingleOccupancyChoke(curr.AliasName) && curr != next {
		et.ChokeCooldowns[curr.AliasName] = 1
	}
	curr.Entities = removeEntity(curr.Entities, e.Id)
	next.Entities = append(next.Entities, e)
	et.EntityPositions[e.Id] = next
	et.EntityPaths[e.Id] = append(et.EntityPaths[e.Id], next)
}

func (et *EntityTracker) ResetToStart(e Entity) {
	for len(et.EntityPaths[e.Id]) > 1 {
		et.MoveBack(e)
	}
}

func (et *EntityTracker) MoveBack(e Entity) {
	path := et.EntityPaths[e.Id]
	if len(path) <= 1 {
		return
	}

	curr := path[len(path)-1]
	prev := path[len(path)-2]

	curr.Entities = removeEntity(curr.Entities, e.Id)
	prev.Entities = append(prev.Entities, e)
	et.EntityPositions[e.Id] = prev
	et.EntityPaths[e.Id] = path[:len(path)-1]
}

func removeEntity(entities []Entity, id int16) []Entity {
	for i, e := range entities {
		if e.Id == id {
			return append(entities[:i], entities[i+1:]...)
		}
	}
	return entities
}

func isSingleOccupancyChoke(alias string) bool {
	switch alias {
	case "lhs_door", "rhs_door", "hallway_close_before_office", "lhs_vent", "rhs_vent":
		return true
	default:
		return false
	}
}

// --- Game Graph Creation ---
func CreateInitialMap(NightNum int16) *GameGraphNode {
	p1_office := &GameGraphNode{RoomID: 100, AliasName: "player_one_office"}

	lhs_door := &GameGraphNode{RoomID: 5, AliasName: "lhs_door", IsBlocked: false}
	lhs_closet := &GameGraphNode{RoomID: 4, AliasName: "lhs_closet"}
	lhs_hall := &GameGraphNode{RoomID: 3, AliasName: "lhs_hall"}

	rhs_door := &GameGraphNode{RoomID: 15, AliasName: "rhs_door", IsBlocked: false}
	rhs_closet := &GameGraphNode{RoomID: 14, AliasName: "rhs_closet"}
	rhs_hall := &GameGraphNode{RoomID: 13, AliasName: "rhs_hall"}

	party_room := &GameGraphNode{RoomID: 2, AliasName: "party_room"}
	repair := &GameGraphNode{RoomID: 6, AliasName: "repair"}
	bathrooms := &GameGraphNode{RoomID: 7, AliasName: "bathrooms"}
	stage := &GameGraphNode{RoomID: 1, AliasName: "stage"}

	lhs_door.AddNext(p1_office)
	lhs_closet.AddNext(lhs_door)
	lhs_hall.AddNext(lhs_closet)

	rhs_door.AddNext(p1_office)
	rhs_closet.AddNext(rhs_door)
	rhs_hall.AddNext(rhs_closet)

	party_room.AddNext(lhs_hall, rhs_hall, repair, bathrooms)
	repair.AddNext(lhs_hall)
	bathrooms.AddNext(rhs_hall)
	stage.AddNext(party_room)

	return stage
}

func CreateP2Map(NightNum int16) *GameGraphNode {
	p2_office := &GameGraphNode{RoomID: 100, AliasName: "player_two_office"}

	lhs_vent := &GameGraphNode{RoomID: 100, AliasName: "lhs_vent"}
	lhs_party := &GameGraphNode{RoomID: 100, AliasName: "lhs_party"}
	lhs_party_two := &GameGraphNode{RoomID: 100, AliasName: "lhs_party_two"}
	lhs_repair := &GameGraphNode{RoomID: 100, AliasName: "lhs_repair"}
	lhs_hall := &GameGraphNode{RoomID: 100, AliasName: "p2_lhs_hall"}

	rhs_vent := &GameGraphNode{RoomID: 100, AliasName: "rhs_vent"}
	rhs_party_one := &GameGraphNode{RoomID: 100, AliasName: "rhs_party_one"}
	rhs_party_two := &GameGraphNode{RoomID: 100, AliasName: "rhs_party_two"}
	rhs_hall := &GameGraphNode{RoomID: 100, AliasName: "p2_rhs_hall"}

	middle_door := &GameGraphNode{RoomID: 100, AliasName: "center_door"}
	middle_hall_two := &GameGraphNode{RoomID: 100, AliasName: "middle_hall_two"}
	middle_hall_one := &GameGraphNode{RoomID: 100, AliasName: "middle_hall_one"}
	hallway_far_before_office := &GameGraphNode{RoomID: 100, AliasName: "hallway_far_before_office"}
	hallway_close_before_office := &GameGraphNode{RoomID: 100, AliasName: "hallway_close_before_office"}

	// Toy routes:
	// toy_bonnie: toy_stage -> rhs_party_one -> rhs_party_two -> p2_rhs_hall -> hall_close -> office
	// toy_chica:  toy_stage -> lhs_party -> lhs_party_two -> p2_lhs_hall -> hall_close -> office
	// toy_freddy: toy_stage -> hall_far -> hall_close -> office
	// toy_foxy:   toy_stage -> middle_hall_two(circus) -> rhs_party_two -> hall_close -> office
	rhs_party_one.AddNext(rhs_party_two)
	rhs_party_two.AddNext(rhs_hall)
	rhs_hall.AddNext(hallway_close_before_office)
	lhs_party.AddNext(lhs_party_two)
	lhs_party_two.AddNext(lhs_hall)
	lhs_hall.AddNext(hallway_close_before_office)
	middle_hall_two.AddNext(rhs_party_two)
	// Keep these rooms reachable for checks/cameras, then merge into hall_far.
	middle_hall_one.AddNext(middle_door)
	middle_door.AddNext(hallway_far_before_office)
	rhs_vent.AddNext(hallway_far_before_office)
	lhs_vent.AddNext(hallway_far_before_office)
	hallway_far_before_office.AddNext(hallway_close_before_office)
	hallway_close_before_office.AddNext(p2_office)

	mangle_room := &GameGraphNode{RoomID: 100, AliasName: "p2_mangle_room"}
	toy_stage := &GameGraphNode{RoomID: 100, AliasName: "toy_stage"}
	toy_stage.AddNext(rhs_party_one, lhs_party, hallway_far_before_office, middle_hall_two)
	mangle_room.AddNext(middle_hall_one, lhs_repair)
	lhs_repair.AddNext(lhs_party)

	facade_stage := &GameGraphNode{RoomID: 100, AliasName: "facade"}
	facade_stage.AddNext(toy_stage, mangle_room)

	return facade_stage
}

// --- Helpers ---
func FindNodeByAlias(start *GameGraphNode, alias string) *GameGraphNode {
	visited := make(map[*GameGraphNode]bool)
	var dfs func(node *GameGraphNode) *GameGraphNode
	dfs = func(node *GameGraphNode) *GameGraphNode {
		if node == nil || visited[node] {
			return nil
		}
		visited[node] = true
		if node.AliasName == alias {
			return node
		}
		for _, next := range node.NextNodes {
			if found := dfs(next); found != nil {
				return found
			}
		}
		return nil
	}
	return dfs(start)
}

// --- Game State Struct ---
type GameState struct {
	Time     int16
	P1Graph  *GameGraphNode
	P2Graph  *GameGraphNode
	Tracker  *EntityTracker
	LHSDoor  *GameGraphNode
	RHSDoor  *GameGraphNode
	P2Office *GameGraphNode
}

func (gs *GameState) SpawnEntities() {
	// Standard entities in P1 graph
	freddy := Entity{Id: 1, Name: "freddy", PreferredNextIndex: 1}
	// Bonnie path: stage -> party_room -> repair -> lhs_hall -> lhs_closet
	// At party_room, index 2 is repair.
	bonnie := Entity{Id: 2, Name: "bonnie", PreferredNextIndex: 2}
	// Chica path: stage -> party_room -> rhs_hall -> rhs_closet
	// At party_room, index 1 is rhs_hall.
	chica := Entity{Id: 3, Name: "chica", PreferredNextIndex: 1}
	foxy := Entity{Id: 4, Name: "foxy", PreferredNextIndex: 0}

	gs.Tracker.AddEntity(freddy, gs.P1Graph) // stage
	gs.Tracker.AddEntity(bonnie, gs.P1Graph)
	gs.Tracker.AddEntity(chica, gs.P1Graph)
	gs.Tracker.AddEntity(foxy, gs.P1Graph.NextNodes[0]) // party_room

	// Toy entities in P2 graph at toy_stage
	toy_freddy := Entity{Id: 11, Name: "toy_freddy", PreferredNextIndex: 2}
	toy_bonnie := Entity{Id: 12, Name: "toy_bonnie", PreferredNextIndex: 0}
	toy_chica := Entity{Id: 13, Name: "toy_chica", PreferredNextIndex: 1}
	toy_foxy := Entity{Id: 14, Name: "toy_foxy", PreferredNextIndex: 3}

	toy_stage := FindNodeByAlias(gs.P2Graph, "toy_stage")
	gs.Tracker.AddEntity(toy_freddy, toy_stage)
	gs.Tracker.AddEntity(toy_bonnie, toy_stage)
	gs.Tracker.AddEntity(toy_chica, toy_stage)
	gs.Tracker.AddEntity(toy_foxy, toy_stage)
}
func NewGameState() *GameState {
	p1 := CreateInitialMap(1)
	p2 := CreateP2Map(1)

	// Linking maps
	p1_lhs := FindNodeByAlias(p1, "lhs_door")
	p1_office := FindNodeByAlias(p1, "player_one_office")
	p2_office := FindNodeByAlias(p2, "player_two_office")
	// Cross-map entry can vary as P2 routes are tuned; choose first reachable fallback.
	p2_entry := FindNodeByAlias(p2, "rhs_vent")
	if p2_entry == nil {
		p2_entry = FindNodeByAlias(p2, "hallway_far_before_office")
	}
	if p2_entry == nil {
		p2_entry = FindNodeByAlias(p2, "toy_stage")
	}
	if p1_office != nil {
		p1_office.AddNext(p2_entry)
	}
	if p2_office != nil {
		p2_office.AddNext(p1_lhs)
	}

	tracker := NewEntityTracker()

	return &GameState{
		P1Graph:  p1,
		P2Graph:  p2,
		Tracker:  tracker,
		LHSDoor:  p1_lhs,
		RHSDoor:  FindNodeByAlias(p1, "rhs_door"),
		P2Office: p2_office,
	}
}

func (gs *GameState) Step(lobbyID string) {
	log.Printf("[lobby %s] sim Step t=%d — positions before move:", lobbyID, gs.Time)
	for id, node := range gs.Tracker.EntityPositions {
		log.Printf("[lobby %s]   entity %d at %s", lobbyID, id, node.AliasName)
	}
	gs.Time++
	gs.Tracker.AdvanceTickCooldowns()
	entityIDs := make([]int16, 0, len(gs.Tracker.EntityPositions))
	for id := range gs.Tracker.EntityPositions {
		entityIDs = append(entityIDs, id)
	}
	for _, id := range entityIDs {
		node := gs.Tracker.EntityPositions[id]
		if node == nil {
			continue
		}
		var current Entity
		found := false
		for _, entity := range node.Entities {
			if entity.Id == id {
				current = entity
				found = true
				break
			}
		}
		if found {
			gs.Tracker.MoveEntity(current)
		}
	}
	log.Printf("[lobby %s] sim Step — positions after move:", lobbyID)
	for id, node := range gs.Tracker.EntityPositions {
		log.Printf("[lobby %s]   entity %d at %s", lobbyID, id, node.AliasName)
	}
	for id := range gs.Tracker.EntityPositions {
		gs.CheckEntityInPlayerRoom(id, lobbyID)
	}
}

// nameForEntityID returns the sim animatronic key used by clients (tronic map + mesh).
// Used when a node's Entities slice is missing the id (should be rare); keeps wire names stable.
func nameForEntityID(id int16) string {
	switch id {
	case 1:
		return "freddy"
	case 2:
		return "bonnie"
	case 3:
		return "chica"
	case 4:
		return "foxy"
	case 11:
		return "toy_freddy"
	case 12:
		return "toy_bonnie"
	case 13:
		return "toy_chica"
	case 14:
		return "toy_foxy"
	default:
		return ""
	}
}

// SnapshotSimEntities builds the list of animatronics and their room aliases for clients.
func (gs *GameState) SnapshotSimEntities() []SimEntityWire {
	if gs == nil || gs.Tracker == nil {
		return nil
	}
	out := make([]SimEntityWire, 0, len(gs.Tracker.EntityPositions))
	for id, node := range gs.Tracker.EntityPositions {
		name := ""
		for _, e := range node.Entities {
			if e.Id == id {
				name = e.Name
				break
			}
		}
		if name == "" {
			name = nameForEntityID(id)
		}
		out = append(out, SimEntityWire{
			EntityID:  id,
			Name:      name,
			RoomAlias: node.AliasName,
		})
	}
	return out
}

func (gs *GameState) FindNodeByAlias(alias string) *GameGraphNode {
	// Search P1 graph first
	if node := FindNodeByAlias(gs.P1Graph, alias); node != nil {
		return node
	}
	// Then search P2 graph
	return FindNodeByAlias(gs.P2Graph, alias)
}

// CheckEntityInPlayerRoom reports whether the entity is in a camera-visible room
// (not either player office). found is false if the id is not in the tracker.
// When lobbyID is non-empty and the entity is in an office, logs that the sim has
// the entity in-room but security feeds cannot render them there.
func (gs *GameState) CheckEntityInPlayerRoom(entityID int16, lobbyID string) (inPlayerRoom bool, roomAlias string, found bool) {
	if gs == nil || gs.Tracker == nil {
		return false, "", false
	}
	node := gs.Tracker.EntityPositions[entityID]
	if node == nil {
		return false, "", false
	}
	roomAlias = node.AliasName
	switch node.AliasName {
	case "player_one_office", "player_two_office":
		if lobbyID != "" {
			var name string
			for _, e := range node.Entities {
				if e.Id == entityID {
					name = e.Name
					break
				}
			}
			if name != "" {
				log.Printf("[lobby %s] entity %d (%s) in %s — in sim room but not on security cameras", lobbyID, entityID, name, roomAlias)
			} else {
				log.Printf("[lobby %s] entity %d in %s — in sim room but not on security cameras", lobbyID, entityID, roomAlias)
			}
		}
		return false, roomAlias, true
	default:
		return true, roomAlias, true
	}
}

func (gs *GameState) AnyEntityInRoom(alias string) bool {
	if gs == nil || gs.Tracker == nil {
		return false
	}
	for _, node := range gs.Tracker.EntityPositions {
		if node != nil && node.AliasName == alias {
			return true
		}
	}
	return false
}

func (gs *GameState) FirstEntityNameInRoom(alias string) (string, bool) {
	if gs == nil || gs.Tracker == nil {
		return "", false
	}
	for id, node := range gs.Tracker.EntityPositions {
		if node == nil || node.AliasName != alias {
			continue
		}
		for _, e := range node.Entities {
			if e.Id == id && e.Name != "" {
				return e.Name, true
			}
		}
		fallback := nameForEntityID(id)
		if fallback != "" {
			return fallback, true
		}
	}
	return "", false
}

func main1() {
	var game = NewGameState()
	game.SpawnEntities()
	game.Step("dev")
	game.Step("dev")
	check := game.FindNodeByAlias("lhs_closet")
	fmt.Printf("Looking at %v", check.Entities)
}

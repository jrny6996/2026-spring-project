package main

import (
	"encoding/json"
	"log"
	"sync"
)

type Vec3 struct {
	X float32 `json:"x"`
	Y float32 `json:"y"`
	Z float32 `json:"z"`
}

type TronicTweak struct {
	Offset      Vec3    `json:"offset"`
	RotationDeg float32 `json:"rotationDeg"`
	Scale       float32 `json:"scale"`
}

type TransformModifier struct {
	Offset Vec3    `json:"offset"`
	Scale  float32 `json:"scale"`
}

type CameraPose struct {
	EyePos    Vec3    `json:"eyePos"`
	EyeTarget Vec3    `json:"eyeTarget"`
	Fovy      float32 `json:"fovy"`
}

type SceneLayout struct {
	// Legacy key kept for migration from older layout rows.
	TronicOffsets  map[string]Vec3              `json:"tronicOffsets,omitempty"`
	TronicTweaks   map[string]TronicTweak       `json:"tronicTweaks"`
	TronicRooms    map[string]string            `json:"tronicRooms,omitempty"`
	CameraPoses    map[string]CameraPose        `json:"cameraPoses"`
	RoomModifiers  map[string]TransformModifier `json:"roomModifiers"`
	GlobalModifier TransformModifier            `json:"globalModifier"`
}

var (
	sceneLayoutMu sync.RWMutex
	sceneLayout   = SceneLayout{
		TronicOffsets:  map[string]Vec3{},
		TronicTweaks:   map[string]TronicTweak{},
		TronicRooms:    map[string]string{},
		CameraPoses:    map[string]CameraPose{},
		RoomModifiers:  map[string]TransformModifier{},
		GlobalModifier: TransformModifier{Offset: Vec3{}, Scale: 1.0},
	}
)

func sceneLayoutSnapshot() SceneLayout {
	sceneLayoutMu.RLock()
	defer sceneLayoutMu.RUnlock()
	out := SceneLayout{
		TronicOffsets:  make(map[string]Vec3, len(sceneLayout.TronicOffsets)),
		TronicTweaks:   make(map[string]TronicTweak, len(sceneLayout.TronicTweaks)),
		TronicRooms:    make(map[string]string, len(sceneLayout.TronicRooms)),
		CameraPoses:    make(map[string]CameraPose, len(sceneLayout.CameraPoses)),
		RoomModifiers:  make(map[string]TransformModifier, len(sceneLayout.RoomModifiers)),
		GlobalModifier: sceneLayout.GlobalModifier,
	}
	for k, v := range sceneLayout.TronicOffsets {
		out.TronicOffsets[k] = v
	}
	for k, v := range sceneLayout.TronicTweaks {
		out.TronicTweaks[k] = v
	}
	for k, v := range sceneLayout.TronicRooms {
		out.TronicRooms[k] = v
	}
	for k, v := range sceneLayout.CameraPoses {
		out.CameraPoses[k] = v
	}
	for k, v := range sceneLayout.RoomModifiers {
		out.RoomModifiers[k] = v
	}
	return out
}

func loadSceneLayoutFromDB() {
	raw, err := loadConfigValue("scene_layout")
	if err != nil || raw == "" {
		return
	}
	var parsed SceneLayout
	if err := json.Unmarshal([]byte(raw), &parsed); err != nil {
		log.Printf("scene_layout decode: %v", err)
		return
	}
	if parsed.TronicOffsets == nil {
		parsed.TronicOffsets = map[string]Vec3{}
	}
	if parsed.TronicTweaks == nil {
		parsed.TronicTweaks = map[string]TronicTweak{}
	}
	if parsed.TronicRooms == nil {
		parsed.TronicRooms = map[string]string{}
	}
	// Migrate legacy offset-only rows to tweak rows.
	for k, v := range parsed.TronicOffsets {
		if _, ok := parsed.TronicTweaks[k]; ok {
			continue
		}
		parsed.TronicTweaks[k] = TronicTweak{
			Offset:      v,
			RotationDeg: 0.0,
			Scale:       1.0,
		}
	}
	if parsed.CameraPoses == nil {
		parsed.CameraPoses = map[string]CameraPose{}
	}
	if parsed.RoomModifiers == nil {
		parsed.RoomModifiers = map[string]TransformModifier{}
	}
	if parsed.GlobalModifier.Scale <= 0.0 {
		parsed.GlobalModifier.Scale = 1.0
	}
	sceneLayoutMu.Lock()
	sceneLayout = parsed
	sceneLayoutMu.Unlock()
}

func persistSceneLayout() {
	snap := sceneLayoutSnapshot()
	b, err := json.Marshal(snap)
	if err != nil {
		log.Printf("scene_layout encode: %v", err)
		return
	}
	if err := saveConfigValue("scene_layout", string(b)); err != nil {
		log.Printf("scene_layout save: %v", err)
	}
}

func upsertTronicLayoutTweak(tronicKey string, t TronicTweak) {
	if tronicKey == "" {
		return
	}
	if t.Scale <= 0.0 {
		t.Scale = 1.0
	}
	sceneLayoutMu.Lock()
	sceneLayout.TronicTweaks[tronicKey] = t
	sceneLayout.TronicOffsets[tronicKey] = t.Offset
	sceneLayoutMu.Unlock()
	persistSceneLayout()
}

func upsertCameraPose(roomAlias string, pose CameraPose) {
	if roomAlias == "" {
		return
	}
	sceneLayoutMu.Lock()
	sceneLayout.CameraPoses[roomAlias] = pose
	sceneLayoutMu.Unlock()
	persistSceneLayout()
}

func upsertRoomModifier(roomAlias string, m TransformModifier) {
	if roomAlias == "" {
		return
	}
	if m.Scale <= 0.0 {
		m.Scale = 1.0
	}
	sceneLayoutMu.Lock()
	sceneLayout.RoomModifiers[roomAlias] = m
	sceneLayoutMu.Unlock()
	persistSceneLayout()
}

func upsertGlobalModifier(m TransformModifier) {
	if m.Scale <= 0.0 {
		m.Scale = 1.0
	}
	sceneLayoutMu.Lock()
	sceneLayout.GlobalModifier = m
	sceneLayoutMu.Unlock()
	persistSceneLayout()
}

func upsertTronicRoom(tronicKey, roomAlias string) {
	if tronicKey == "" || roomAlias == "" {
		return
	}
	sceneLayoutMu.Lock()
	sceneLayout.TronicRooms[tronicKey] = roomAlias
	sceneLayoutMu.Unlock()
	persistSceneLayout()
}

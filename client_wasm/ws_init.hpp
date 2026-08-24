#pragma once

#include <cstdio>
#include <string>

#include <emscripten/emscripten.h>
#include <emscripten/websocket.h>

#include "GameState.hpp"
#include "include/nlohmann/json.hpp"

struct WsContext {
  EMSCRIPTEN_WEBSOCKET_T socket;
  GameState* state;
};
namespace ws {
using nlohmann::json;

inline void send_json_message(EMSCRIPTEN_WEBSOCKET_T socket,
                              const std::string& type,
                              const std::string& content) {
  json msg = {
      {"type", type},
      {"content", content},
  };
  const std::string serialized = msg.dump();
  emscripten_websocket_send_utf8_text(socket, serialized.c_str());
}

inline void send_invite(EMSCRIPTEN_WEBSOCKET_T socket) {
  // return;
  send_json_message(socket, "chat", "invite");
}

inline void send_start(EMSCRIPTEN_WEBSOCKET_T socket) {
  // Server checks only content == "start"
  send_json_message(socket, "status", "start");
}

inline void send_join(EMSCRIPTEN_WEBSOCKET_T socket,
                      const std::string& lobbyId) {
  // Server requires type == "join" and content == lobbyId
  send_json_message(socket, "join", lobbyId);
}

/// Ask server which entities are in the sim room for this camera feed.
/// content must match GameGraphNode.AliasName (see SecurityCamera.sim_room_alias).
/// Response: checkCamera with data.roomAlias and data.entities[{entityId,name},...].
inline void send_check_camera(EMSCRIPTEN_WEBSOCKET_T socket,
                              const std::string& sim_room_alias) {
  send_json_message(socket, "check", sim_room_alias);
}

/// Ask server to advance one manual simulation step (for paused sim loop).
inline void send_step(EMSCRIPTEN_WEBSOCKET_T socket) {
  send_json_message(socket, "step", "tick");
}

/// Update first-floor office door state on the server.
/// side: "lhs" | "rhs", state: "open" | "closed"
inline void send_door_state(EMSCRIPTEN_WEBSOCKET_T socket, const char* side,
                            bool closed) {
  const std::string content =
      std::string("door:") + side + ":" + (closed ? "closed" : "open");
  send_json_message(socket, "action", content);
}

/// Update player-two mask state on server.
inline void send_mask_state(EMSCRIPTEN_WEBSOCKET_T socket, bool down) {
  const std::string content =
      std::string("mask:p2:") + (down ? "down" : "up");
  send_json_message(socket, "action", content);
}

/// P1: each press queues power units; server adds them on the next game step (T).
/// units clamped client-side; server caps backlog.
inline void send_p1_power_queued(EMSCRIPTEN_WEBSOCKET_T socket, int units) {
  if (units < 1)
    units = 1;
  if (units > 32)
    units = 32;
  char buf[24];
  std::snprintf(buf, sizeof(buf), "p1q:%d", units);
  send_json_message(socket, "action", std::string(buf));
}

/// P2: each tap queues units; drained on server game step (T). Same contract as P1 p1q.
inline void send_p2_power_queued(EMSCRIPTEN_WEBSOCKET_T socket, int units) {
  if (units < 1)
    units = 1;
  if (units > 32)
    units = 32;
  char buf[24];
  std::snprintf(buf, sizeof(buf), "p2qp:%d", units);
  send_json_message(socket, "action", std::string(buf));
}

inline void send_p2_music_queued(EMSCRIPTEN_WEBSOCKET_T socket, int units) {
  if (units < 1)
    units = 1;
  if (units > 32)
    units = 32;
  char buf[24];
  std::snprintf(buf, sizeof(buf), "p2qm:%d", units);
  send_json_message(socket, "action", std::string(buf));
}

inline void send_debug_pause_toggle(EMSCRIPTEN_WEBSOCKET_T socket) {
  send_json_message(socket, "action", "pause:toggle");
}

inline void send_debug_tronic_offset(EMSCRIPTEN_WEBSOCKET_T socket,
                                     const std::string& tronic_key, float x,
                                     float y, float z, float rotation_deg,
                                     float scale) {
  char buf[220];
  std::snprintf(buf, sizeof(buf), "dbgtronic:%s:%.4f:%.4f:%.4f:%.4f:%.4f",
                tronic_key.c_str(), static_cast<double>(x),
                static_cast<double>(y), static_cast<double>(z),
                static_cast<double>(rotation_deg), static_cast<double>(scale));
  send_json_message(socket, "action", std::string(buf));
}

inline void send_debug_camera_pose(EMSCRIPTEN_WEBSOCKET_T socket,
                                   const std::string& room_alias, float ex,
                                   float ey, float ez, float tx, float ty,
                                   float tz, float fovy) {
  char buf[256];
  std::snprintf(buf, sizeof(buf),
                "dbgcam:%s:%.4f:%.4f:%.4f:%.4f:%.4f:%.4f:%.4f",
                room_alias.c_str(), static_cast<double>(ex),
                static_cast<double>(ey), static_cast<double>(ez),
                static_cast<double>(tx), static_cast<double>(ty),
                static_cast<double>(tz), static_cast<double>(fovy));
  send_json_message(socket, "action", std::string(buf));
}

inline void send_debug_room_modifier(EMSCRIPTEN_WEBSOCKET_T socket,
                                     const std::string& room_alias, float x,
                                     float y, float z, float scale) {
  char buf[220];
  std::snprintf(buf, sizeof(buf), "dbgroommod:%s:%.4f:%.4f:%.4f:%.4f",
                room_alias.c_str(), static_cast<double>(x),
                static_cast<double>(y), static_cast<double>(z),
                static_cast<double>(scale));
  send_json_message(socket, "action", std::string(buf));
}

inline void send_debug_global_modifier(EMSCRIPTEN_WEBSOCKET_T socket, float x,
                                       float y, float z, float scale) {
  char buf[180];
  std::snprintf(buf, sizeof(buf), "dbgglobal:%.4f:%.4f:%.4f:%.4f",
                static_cast<double>(x), static_cast<double>(y),
                static_cast<double>(z), static_cast<double>(scale));
  send_json_message(socket, "action", std::string(buf));
}

inline void send_debug_move_tronic_to_room(EMSCRIPTEN_WEBSOCKET_T socket,
                                           const std::string& tronic_key,
                                           const std::string& room_alias) {
  send_json_message(socket, "action",
                    std::string("dbgmoveroom:") + tronic_key + ":" + room_alias);
}

inline void send_debug_save_tronic_room(EMSCRIPTEN_WEBSOCKET_T socket,
                                        const std::string& tronic_key,
                                        const std::string& room_alias) {
  send_json_message(socket, "action",
                    std::string("dbgsaveroom:") + tronic_key + ":" + room_alias);
}

inline bool try_parse_json(const char* data, size_t len, json& out) {
  try {
    out = json::parse(std::string(data, len));
    return true;
  } catch (std::exception& e) {
    printf("Error %s", e.what());
    return false;
  }
}

inline EM_BOOL on_open(int, const EmscriptenWebSocketOpenEvent*,
                       void* userData) {
  WsContext* ctx = static_cast<WsContext*>(userData);

  printf("WebSocket connected!\n");

  return EM_TRUE;
}
inline EM_BOOL on_message(int, const EmscriptenWebSocketMessageEvent* e,
                          void* userData) {

  WsContext* ctx = static_cast<WsContext*>(userData);
  GameState* state = ctx->state;

  if (e->isText) {
    nlohmann::json parsed;
    const char* text = (const char*)e->data;
    printf("\n text: %s \n", text);
    if (ws::try_parse_json(text, (size_t)e->numBytes, parsed)) {
      std::cout << parsed << "\n";
      if (parsed.contains("type") && parsed["type"] == "invite") {
        state->lobbyId = parsed.value("data", std::string(""));
        printf("Lobby ID updated: %s\n", state->lobbyId.c_str());
        state->lobby_created = true;
        state->is_lobby_host = true;
        state->menu_error.clear();
        state->menu_creating_lobby = false;
      }
      if (parsed.contains("type") && parsed["type"] == "joined") {
        state->lobbyId = parsed.value("data", std::string(""));
        printf("Joined lobby: %s\n", state->lobbyId.c_str());
        state->lobby_created = true;
        state->is_lobby_host = false;
        state->menu_error.clear();
        state->menu_creating_lobby = false;
      }
      if (parsed.contains("type") && parsed["type"] == "status") {
        const std::string status = parsed.value("data", std::string());
        if (status.rfind("lose", 0) == 0) {
          std::string killer;
          if (status.size() > 5)
            killer = status.substr(5);
          if (!killer.empty()) {
            printf("You lost to: %s\n", killer.c_str());
            state->check_camera_status = "You lose - killed by " + killer;
          } else {
            state->check_camera_status = "You lose";
          }
          state->gameStarted = false;
        } else if (status == "win") {
          state->check_camera_status = "You win";
          state->gameStarted = false;
        } else if (status == "peer-disconnected") {
          state->check_camera_status = "Other player disconnected";
          state->gameStarted = false;
          state->lobbyId.clear();
          state->lobby_created = false;
          state->has_player_slot = false;
        }
      }
      if (parsed.contains("type") && parsed["type"] == "error") {
        std::string detail;
        if (parsed.contains("data") && parsed["data"].is_string())
          detail = parsed["data"].get<std::string>();
        if (state->check_camera_in_flight) {
          state->check_camera_entities.clear();
          state->check_camera_status =
              detail.empty() ? "Camera check failed"
                             : ("Camera check failed: " + detail);
          state->check_camera_in_flight = false;
          state->check_camera_restore_feed =
              state->check_camera_suspend_feed_for_request;
          state->check_camera_suspend_feed_for_request = false;
        } else if (!detail.empty()) {
          state->menu_error = detail;
          state->menu_creating_lobby = false;
        }
      }
      if (parsed.contains("type") && parsed["type"] == "checkCamera" &&
          parsed.contains("data") && parsed["data"].is_object()) {
        auto& d = parsed["data"];
        const std::string alias = d.value("roomAlias", std::string());
        state->check_camera_last_room_alias = alias;
        state->check_camera_entities.clear();
        if (d.contains("entities") && d["entities"].is_array()) {
          for (const auto& item : d["entities"]) {
            if (!item.is_object())
              continue;
            CheckCameraEntityRow row;
            if (item.contains("entityId") && item["entityId"].is_number()) {
              if (item["entityId"].is_number_integer())
                row.id = item["entityId"].get<int>();
              else
                row.id = static_cast<int>(item["entityId"].get<double>());
            }
            row.name = item.value("name", std::string());
            state->check_camera_entities.push_back(std::move(row));
          }
        }
        state->check_camera_status =
            alias.empty()
                ? "Camera room scan"
                : (alias + " — " +
                   std::to_string(state->check_camera_entities.size()) + " ent.");
        state->check_camera_in_flight = false;
        state->check_camera_restore_feed =
            state->check_camera_suspend_feed_for_request;
        state->check_camera_suspend_feed_for_request = false;
      }
      if (parsed["type"] == "state" && parsed.contains("data") &&
          parsed["data"].is_object()) {
        auto& data = parsed["data"];
        bool apply_state = true;
        if (data.contains("lobbyId") && data["lobbyId"].is_string()) {
          const std::string wire_lid = data["lobbyId"].get<std::string>();
          if (!state->lobbyId.empty() && wire_lid != state->lobbyId)
            apply_state = false;
        }
        if (apply_state) {
          if (data.contains("started"))
            state->gameStarted = data.value("started", false);
          if (!state->gameStarted)
            state->sim_entities.clear();
          if (data.contains("time"))
            state->gameTime = data.value("time", 0);
          if (data.contains("nightNum")) {
            int nn = data.value("nightNum", 1);
            if (nn < 1)
              nn = 1;
            if (nn > 7)
              nn = 7;
            state->night_num = nn;
          }
          if (data.contains("isPlayerOne")) {
            state->has_player_slot = true;
            state->is_player_one = data.value("isPlayerOne", false);
          }
          if (data.contains("p2MaskDown")) {
            state->p2_mask_down = data.value("p2MaskDown", false);
          }
          if (data.contains("paused")) {
            state->server_paused = data.value("paused", false);
          }
          if (data.contains("power"))
            state->power = data.value("power", 30);
          if (data.contains("musicBoxWind"))
            state->music_box_wind = data.value("musicBoxWind", 0);
          // Always refresh from wire (default false if key omitted) so stale flags
          // never block P1 after lobby/game changes.
          state->p2_in_lobby = data.value("p2InLobby", false);
          state->p2_lost = data.value("p2Lost", false);
          if (state->gameStarted && data.contains("simEntities") &&
              data["simEntities"].is_array()) {
            state->sim_entities.clear();
            for (const auto& item : data["simEntities"]) {
              SimEntityRow row;
              if (item.contains("entityId") && item["entityId"].is_number()) {
                if (item["entityId"].is_number_integer())
                  row.id = item["entityId"].get<int>();
                else
                  row.id = static_cast<int>(item["entityId"].get<double>());
              }
              row.name = item.value("name", std::string());
              row.room_alias = item.value("roomAlias", std::string());
              state->sim_entities.push_back(std::move(row));
            }
          }
          if (data.contains("layout") && data["layout"].is_object()) {
            auto& layout = data["layout"];
            const std::string layout_sig = layout.dump();
            if (layout_sig != state->layout_wire_cache) {
              state->layout_wire_cache = layout_sig;
              state->layout_tronic_offsets.clear();
              state->layout_camera_rows.clear();
              if (layout.contains("tronicOffsets") &&
                  layout["tronicOffsets"].is_object()) {
                for (auto it = layout["tronicOffsets"].begin();
                     it != layout["tronicOffsets"].end(); ++it) {
                  if (!it.value().is_object())
                    continue;
                  LayoutOffsetRow row;
                  row.key = it.key();
                  row.x = it.value().value("x", 0.0f);
                  row.y = it.value().value("y", 0.0f);
                  row.z = it.value().value("z", 0.0f);
                  row.rotation_deg = 0.0f;
                  row.scale = 1.0f;
                  state->layout_tronic_offsets.push_back(std::move(row));
                }
              }
              if (layout.contains("tronicTweaks") &&
                  layout["tronicTweaks"].is_object()) {
                state->layout_tronic_offsets.clear();
                for (auto it = layout["tronicTweaks"].begin();
                     it != layout["tronicTweaks"].end(); ++it) {
                  if (!it.value().is_object())
                    continue;
                  auto& t = it.value();
                  if (!t.contains("offset") || !t["offset"].is_object())
                    continue;
                  LayoutOffsetRow row;
                  row.key = it.key();
                  row.x = t["offset"].value("x", 0.0f);
                  row.y = t["offset"].value("y", 0.0f);
                  row.z = t["offset"].value("z", 0.0f);
                  row.rotation_deg = t.value("rotationDeg", 0.0f);
                  row.scale = t.value("scale", 1.0f);
                  if (row.scale <= 0.0f)
                    row.scale = 1.0f;
                  state->layout_tronic_offsets.push_back(std::move(row));
                }
              }
              if (layout.contains("cameraPoses") &&
                  layout["cameraPoses"].is_object()) {
                for (auto it = layout["cameraPoses"].begin();
                     it != layout["cameraPoses"].end(); ++it) {
                  if (!it.value().is_object())
                    continue;
                  auto& c = it.value();
                  if (!c.contains("eyePos") || !c["eyePos"].is_object() ||
                      !c.contains("eyeTarget") || !c["eyeTarget"].is_object()) {
                    continue;
                  }
                  LayoutCameraRow row;
                  row.key = it.key();
                  row.ex = c["eyePos"].value("x", 0.0f);
                  row.ey = c["eyePos"].value("y", 0.0f);
                  row.ez = c["eyePos"].value("z", 0.0f);
                  row.tx = c["eyeTarget"].value("x", 0.0f);
                  row.ty = c["eyeTarget"].value("y", 0.0f);
                  row.tz = c["eyeTarget"].value("z", 0.0f);
                  row.fovy = c.value("fovy", 55.0f);
                  state->layout_camera_rows.push_back(std::move(row));
                }
              }
              state->layout_room_modifiers.clear();
              if (layout.contains("roomModifiers") &&
                  layout["roomModifiers"].is_object()) {
                for (auto it = layout["roomModifiers"].begin();
                     it != layout["roomModifiers"].end(); ++it) {
                  if (!it.value().is_object())
                    continue;
                  auto& m = it.value();
                  if (!m.contains("offset") || !m["offset"].is_object())
                    continue;
                  LayoutRoomModifierRow row;
                  row.key = it.key();
                  row.x = m["offset"].value("x", 0.0f);
                  row.y = m["offset"].value("y", 0.0f);
                  row.z = m["offset"].value("z", 0.0f);
                  row.scale = m.value("scale", 1.0f);
                  if (row.scale <= 0.0f)
                    row.scale = 1.0f;
                  state->layout_room_modifiers.push_back(std::move(row));
                }
              }
              state->layout_global_modifier = {};
              if (layout.contains("globalModifier") &&
                  layout["globalModifier"].is_object()) {
                auto& g = layout["globalModifier"];
                if (g.contains("offset") && g["offset"].is_object()) {
                  state->layout_global_modifier.x =
                      g["offset"].value("x", 0.0f);
                  state->layout_global_modifier.y =
                      g["offset"].value("y", 0.0f);
                  state->layout_global_modifier.z =
                      g["offset"].value("z", 0.0f);
                }
                state->layout_global_modifier.scale = g.value("scale", 1.0f);
                if (state->layout_global_modifier.scale <= 0.0f)
                  state->layout_global_modifier.scale = 1.0f;
              } else {
                state->layout_global_modifier.scale = 1.0f;
              }
              state->layout_revision++;
            }
          }
          state->printState();
        }
      }
    }
  }
  return EM_TRUE;
}

inline EM_BOOL on_error(int /*eventType*/,
                        const EmscriptenWebSocketErrorEvent* /*e*/,
                        void* /*userData*/) {
  printf("WebSocket error occurred.\n");
  return EM_TRUE;
}

inline EM_BOOL on_close(int /*eventType*/,
                        const EmscriptenWebSocketCloseEvent* e,
                        void* /*userData*/) {
  printf("WebSocket closed: code=%d reason=%s\n", e->code, e->reason);
  return EM_TRUE;
}

inline EMSCRIPTEN_WEBSOCKET_T init(const char* url, WsContext* ctx) {
  EmscriptenWebSocketCreateAttributes attr;
  emscripten_websocket_init_create_attributes(&attr);
  attr.url = url;
  attr.protocols = NULL;
  attr.createOnMainThread = EM_TRUE;

  ctx->socket = emscripten_websocket_new(&attr);
  if (ctx->socket <= 0)
    return ctx->socket;

  emscripten_websocket_set_onopen_callback(ctx->socket, ctx, on_open);
  emscripten_websocket_set_onmessage_callback(ctx->socket, ctx, on_message);
  emscripten_websocket_set_onerror_callback(ctx->socket, ctx, on_error);
  emscripten_websocket_set_onclose_callback(ctx->socket, ctx, on_close);

  return ctx->socket;
}
}  // namespace ws

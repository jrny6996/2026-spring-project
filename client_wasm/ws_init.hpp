#pragma once

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
          if (data.contains("isPlayerOne")) {
            state->has_player_slot = true;
            state->is_player_one = data.value("isPlayerOne", false);
          }
          if (state->gameStarted && data.contains("simEntities") &&
              data["simEntities"].is_array()) {
            state->sim_entities.clear();
            for (const auto& item : data["simEntities"]) {
              SimEntityRow row;
              row.id = item.value("entityId", 0);
              row.name = item.value("name", std::string());
              row.room_alias = item.value("roomAlias", std::string());
              state->sim_entities.push_back(std::move(row));
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

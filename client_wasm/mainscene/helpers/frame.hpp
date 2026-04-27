#pragma once
#include "camera_control.hpp"
#include "pbr_lights.hpp"
#include "../../GameState.hpp"
#include "../../camera_nav.hpp"
#include "../../ws_init.hpp"
#include "rooms/create_tronic_positions.hpp"
#include "raylib.h"
#include <emscripten/websocket.h>
#include <map>
#include <string>

namespace mainscene {

inline void process_check_camera_restore(GameState& state,
                                         CameraNavState& camera_nav) {
  if (state.check_camera_restore_feed) {
    camera_nav.active_feed = state.check_camera_saved_active_feed;
    state.check_camera_restore_feed = false;
  }
}

inline void process_camera_panel_toggle(CameraNavState& camera_nav,
                                        bool is_player_one) {
  if (IsKeyPressed(KEY_B))
    camera_nav.TogglePanel(is_player_one);
}

inline void try_send_check_camera_room(GameState& state,
                                       CameraNavState& camera_nav,
                                       EMSCRIPTEN_WEBSOCKET_T& socket) {
  if (camera_nav.active_feed >= 0 && IsKeyPressed(KEY_C) &&
      !state.check_camera_in_flight) {
    int n_cams = 0;
    const SecurityCamera* cams =
        CameraMaps::MapForPlayer(state.is_player_one, &n_cams);
    int idx = camera_nav.active_feed;
    if (idx >= 0 && idx < n_cams && cams[idx].sim_room_alias != nullptr) {
      state.check_camera_entities.clear();
      state.check_camera_status = "Checking cameras…";
      state.check_camera_saved_active_feed = camera_nav.active_feed;
      camera_nav.active_feed = -1;
      state.check_camera_in_flight = true;
      ws::send_check_camera(socket, cams[idx].sim_room_alias);
    }
  }
}

inline void clamp_and_apply_pbr_for_security_feed(
    GameState& state, CameraNavState& camera_nav, PbrLightGPU* lights,
    int pbr_light_count) {
  if (camera_nav.active_feed >= 0) {
    int n = 0;
    const SecurityCamera* map =
        CameraMaps::MapForPlayer(state.is_player_one, &n);
    if (camera_nav.active_feed >= n)
      camera_nav.active_feed = n > 0 ? n - 1 : 0;
    const PbrLightId active_light = map[camera_nav.active_feed].spot_light;
    for (int i = 0; i < CameraMaps::kInitSpotOrderCount; i++) {
      const SecurityCamera& sc = CameraMaps::kInitSpotOrder[i];
      set_pbr_light_enabled(lights, pbr_light_count, static_cast<int>(sc.spot_light),
                            sc.spot_light == active_light);
    }
    set_pbr_light_enabled(
        lights, pbr_light_count,
        static_cast<int>(PbrLightId::FirstFloorHallKeyLHS), false);
    set_pbr_light_enabled(
        lights, pbr_light_count,
        static_cast<int>(PbrLightId::FirstFloorHallKeyRHS), false);
  } else {
    if (state.is_player_one) {
      for (int i = 0; i < CameraMaps::kInitSpotOrderCount; i++) {
        const SecurityCamera& sc = CameraMaps::kInitSpotOrder[i];
        set_pbr_light_enabled(lights, pbr_light_count,
                              static_cast<int>(sc.spot_light), false);
      }
      set_pbr_light_enabled(
          lights, pbr_light_count,
          static_cast<int>(PbrLightId::FirstFloorHallKeyLHS), IsKeyDown(KEY_A));
      set_pbr_light_enabled(
          lights, pbr_light_count,
          static_cast<int>(PbrLightId::FirstFloorHallKeyRHS), IsKeyDown(KEY_D));
    } else {
      set_pbr_light_enabled(
          lights, pbr_light_count,
          static_cast<int>(PbrLightId::FirstFloorHallKeyLHS), false);
      set_pbr_light_enabled(
          lights, pbr_light_count,
          static_cast<int>(PbrLightId::FirstFloorHallKeyRHS), false);
      set_pbr_light_enabled(
          lights, pbr_light_count,
          static_cast<int>(PbrLightId::SecondFloorHallSpot), IsKeyDown(KEY_W));
      set_pbr_light_enabled(
          lights, pbr_light_count, static_cast<int>(PbrLightId::SecondFloorLHS),
          IsKeyDown(KEY_A));
      set_pbr_light_enabled(
          lights, pbr_light_count, static_cast<int>(PbrLightId::SecondFloorRHS),
          IsKeyDown(KEY_D));
    }
  }
}

inline void apply_player_two_default_campos(bool is_player_one,
                                            bool is_freeroam, Vector3& camPos) {
  if (!is_player_one && !is_freeroam) {
    camPos = {0.0f, 52.5f, 0.0f};
  }
}

inline void apply_security_feed_or_office_view(
    Camera& camera, CameraNavState& camera_nav, bool is_player_one,
    Vector3& camPos, float& yaw, float& pitch, bool is_freeroam) {
  if (camera_nav.active_feed >= 0) {
    int n = 0;
    const SecurityCamera* map = CameraMaps::MapForPlayer(is_player_one, &n);
    if (camera_nav.active_feed < n)
      CameraMaps::ApplySecurityCameraView(
          map[camera_nav.active_feed], static_cast<Camera3D&>(camera));
  } else {
    update_office_camera(camera, camPos, yaw, pitch, is_freeroam);
  }
}

inline const TronicPosition* tronic_pos_for_room(const TronicPositionMap* tpm,
                                                 const std::string& room_key) {
  if (!tpm || room_key.empty())
    return nullptr;
  auto it = tpm->pos_map.find(room_key);
  if (it == tpm->pos_map.end())
    return nullptr;
  return &it->second;
}

inline void draw_main_scene_3d(CameraNavState& camera_nav, bool is_player_one,
                              const GameState& state, const Model& freddy,
                              const Vector3& freddy_backup_pos,
                              const std::map<std::string, TronicPositionMap>&
                                  tronic_by_entity,
                              const Model& map, const Model& p_map) {
  const Vector3 rotationAxis = {0.0f, 1.0f, 0.0f};
  const Vector3 anim_scale = {50.0f, 50.0f, 50.0f};

  DrawModel(map, (Vector3){0.0f, 2.5f, -2.5f}, 1.0f, WHITE);
  DrawModel(p_map, (Vector3){1.0f, 50.0f, -13.0f}, 1.0f, WHITE);

  const bool on_feed = camera_nav.active_feed >= 0;
  if (!on_feed || !state.gameStarted) {
    DrawModelEx(freddy, freddy_backup_pos, rotationAxis, 0.0f, anim_scale,
                WHITE);
    return;
  }

  int n_cams = 0;
  const SecurityCamera* cams =
      CameraMaps::MapForPlayer(is_player_one, &n_cams);
  if (camera_nav.active_feed >= n_cams || n_cams == 0) {
    DrawModelEx(freddy, freddy_backup_pos, rotationAxis, 0.0f, anim_scale,
                WHITE);
    return;
  }

  const SecurityCamera& sc = cams[camera_nav.active_feed];
  const char* sim_alias = sc.sim_room_alias;
  if (!sim_alias || sim_alias[0] == '\0') {
    DrawModelEx(freddy, freddy_backup_pos, rotationAxis, 0.0f, anim_scale,
                WHITE);
    return;
  }

  const std::string room_key = tronic_room_key(is_player_one, sim_alias);

  bool any_in_room = false;
  for (const auto& ent : state.sim_entities) {
    if (ent.room_alias == sim_alias) {
      any_in_room = true;
      break;
    }
  }
  if (!any_in_room) {
    return;
  }

  for (const auto& ent : state.sim_entities) {
    if (ent.room_alias != sim_alias)
      continue;
    const TronicPosition* tp = nullptr;
    auto named = tronic_by_entity.find(ent.name);
    if (named != tronic_by_entity.end())
      tp = tronic_pos_for_room(&named->second, room_key);
    if (!tp) {
      auto fb = tronic_by_entity.find("freddy");
      if (fb != tronic_by_entity.end())
        tp = tronic_pos_for_room(&fb->second, room_key);
    }
    Vector3 pos = freddy_backup_pos;
    Vector3 ax = rotationAxis;
    float ang = 0.0f;
    if (tp) {
      pos = tp->position;
      ax = tp->rotationAxis;
      ang = tp->rotationAngle;
    }
    pos.x += static_cast<float>((ent.id % 5) * 2 - 4) * 0.35f;
    DrawModelEx(freddy, pos, ax, ang, anim_scale, WHITE);
  }
}

inline void draw_main_scene_2d(Camera& camera, CameraNavState& camera_nav,
                                 const GameState& state) {
  camera_nav.DrawPanel(state.is_player_one);
  std::string x_text = std::to_string(camera.position.x);
  std::string y_text = std::to_string(camera.position.y);
  std::string z_text = std::to_string(camera.position.z);
  DrawText(x_text.c_str(), 10, 10, 16, WHITE);
  DrawText(y_text.c_str(), 10, 28, 16, WHITE);
  DrawText(z_text.c_str(), 10, 46, 16, WHITE);
  if (state.has_player_slot) {
    const char* label = state.is_player_one ? "Player 1" : "Player 2";
    DrawText(label, 10, 64, 16, WHITE);
  }
  if (!state.check_camera_status.empty()) {
    const int hud_y = GetScreenHeight() - 48;
    Color hud_color = YELLOW;
    if (!state.check_camera_in_flight) {
      if (state.check_camera_status.rfind("Camera check failed", 0) == 0)
        hud_color = RED;
      else if (state.check_camera_entities.empty())
        hud_color = ORANGE;
      else
        hud_color = GREEN;
    }
    DrawText(state.check_camera_status.c_str(), 10, hud_y, 20, hud_color);
    int line_y = hud_y - 22;
    for (const auto& ent : state.check_camera_entities) {
      if (line_y < 120)
        break;
      std::string line =
          std::to_string(ent.id) + std::string("  ") + ent.name;
      DrawText(line.c_str(), 10, line_y, 16, RAYWHITE);
      line_y -= 18;
    }
  }
  DrawText("Cam feed: C = entities in this cam room (after start)", 10, 100,
           16, Fade(LIGHTGRAY, 0.85f));
}

}  // namespace mainscene

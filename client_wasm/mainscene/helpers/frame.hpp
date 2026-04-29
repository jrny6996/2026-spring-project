#pragma once
#include "camera_control.hpp"
#include "pbr_lights.hpp"
#include "../../GameState.hpp"
#include "../../camera_nav.hpp"
#include "../../ws_init.hpp"
#include "rooms/create_tronic_positions.hpp"
#include "raylib.h"
#include <emscripten/websocket.h>
#include <cstddef>
#include <cstdio>
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
  if (IsKeyPressed(KEY_S))
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
      state.check_camera_suspend_feed_for_request = true;
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
  if (it == tpm->pos_map.end()) {
    return nullptr;
  }
  return &it->second;
}

inline Vector3 tronic_draw_scale_for(
    const std::map<std::string, TronicPositionMap>& tronic_by_entity,
    const std::string& tkey, const Vector3& fallback_scale) {
  auto it = tronic_by_entity.find(tkey);
  if (it != tronic_by_entity.end())
    return it->second.scale;
  auto fb = tronic_by_entity.find("freddy");
  if (fb != tronic_by_entity.end())
    return fb->second.scale;
  return fallback_scale;
}

inline std::map<std::string, int> tronic_room_occupancy(
    const std::vector<SimEntityRow>& sim_entities, const char* only_room_alias) {
  std::map<std::string, int> counts;
  for (const auto& ent : sim_entities) {
    if (tronic_key_from_sim_entity(ent.id, ent.name).empty())
      continue;
    if (only_room_alias) {
      if (ent.room_alias != only_room_alias)
        continue;
    } else {
      if (ent.room_alias.empty())
        continue;
    }
    counts[ent.room_alias]++;
  }
  return counts;
}

/// Resolves world position / rotation for one sim animatronic (same rules as draw).
/// Returns false when there is no layout entry and backups are disabled.
inline bool resolve_tronic_draw_transform(
    const SimEntityRow& ent,
    const std::map<std::string, TronicPositionMap>& tronic_by_entity,
    const Vector3& default_pos, const Vector3& default_axis,
    bool use_backup_when_pos_missing, Vector3* out_pos, Vector3* out_axis,
    float* out_angle) {
  const std::string tkey = tronic_key_from_sim_entity(ent.id, ent.name);
  if (tkey.empty())
    return false;
  const std::string ent_room =
      tronic_3d_pos_key(ent.id, ent.room_alias.c_str());
  const TronicPosition* tp = nullptr;
  auto named = tronic_by_entity.find(tkey);
  if (named != tronic_by_entity.end())
    tp = tronic_pos_for_room(&named->second, ent_room);
  if (!tp) {
    auto fb = tronic_by_entity.find("freddy");
    if (fb != tronic_by_entity.end())
      tp = tronic_pos_for_room(&fb->second, ent_room);
  }
  if (!tp && !use_backup_when_pos_missing)
    return false;
  if (tp) {
    *out_pos = tp->position;
    *out_axis = tp->rotationAxis;
    *out_angle = tp->rotationAngle;
  } else {
    *out_pos = default_pos;
    *out_axis = default_axis;
    *out_angle = 0.0f;
  }
  return true;
}

inline bool should_apply_character_offset(const SimEntityRow& ent) {
  if (ent.id >= 1 && ent.id <= 4)
    return ent.room_alias == "stage";
  if (ent.id >= 11 && ent.id <= 14)
    return ent.room_alias == "toy_stage";
  return false;
}

/// `only_room_alias`: if non-null, only draw entities in that sim room (security
/// feed). If null, draw every entity that has a mapped 3D position (world / office
/// view). `use_backup_when_pos_missing` matches the feed: still draw at backup pos
/// when the room is not in the tronic table.
inline std::size_t draw_tronic_sim_entities(
    const std::vector<SimEntityRow>& sim_entities,
    const std::map<std::string, TronicPositionMap>& tronic_by_entity,
    Model* const* animatronic_models, std::size_t animatronic_model_count,
    const Vector3& default_pos,
    const Vector3& rotation_axis, const Vector3& anim_scale,
    const char* only_room_alias, bool use_backup_when_pos_missing) {
  if (animatronic_model_count < 2)
    return 0;
  const Model& default_mesh = *animatronic_models[1];
  const std::map<std::string, int> occupancy =
      tronic_room_occupancy(sim_entities, only_room_alias);
  std::size_t drawn = 0;
  for (const auto& ent : sim_entities) {
    const std::string tkey = tronic_key_from_sim_entity(ent.id, ent.name);
    if (tkey.empty())
      continue;
    if (only_room_alias) {
      if (ent.room_alias != only_room_alias)
        continue;
    } else {
      if (ent.room_alias.empty())
        continue;
    }
    Vector3 pos{};
    Vector3 ax{};
    float ang = 0.0f;
    if (!resolve_tronic_draw_transform(ent, tronic_by_entity, default_pos,
                                       rotation_axis, use_backup_when_pos_missing,
                                       &pos, &ax, &ang))
      continue;

    auto named = tronic_by_entity.find(tkey);
    const Model* mesh = &default_mesh;
    Vector3 draw_scale = anim_scale;
    Vector3 off{0.0f, 0.0f, 0.0f};
    if (named != tronic_by_entity.end()) {
      mesh = &named->second.model;
      draw_scale = named->second.scale;
      off = named->second.character_offset;
    } else {
      auto fb = tronic_by_entity.find("freddy");
      if (fb != tronic_by_entity.end()) {
        mesh = &fb->second.model;
        draw_scale = fb->second.scale;
        off = fb->second.character_offset;
      }
    }
    if (should_apply_character_offset(ent)) {
      pos.x += off.x;
      pos.y += off.y;
      pos.z += off.z;
    }
    auto occ_it = occupancy.find(ent.room_alias);
    if (occ_it != occupancy.end() && occ_it->second > 1) {
      const float spread = 0.42f;
      pos.x += static_cast<float>((ent.id % 5) - 2) * spread;
    }

    DrawModelEx(*mesh, pos, ax, ang, draw_scale, WHITE);
    ++drawn;
  }
  return drawn;
}

/// Same entity set and `resolve_tronic_draw_transform` rules as the F3 coord HUD
/// (backup layout, includes empty `room_alias`). Use so on-screen models match HUD.
inline std::size_t draw_tronic_sim_entities_matching_debug_hud(
    const std::vector<SimEntityRow>& sim_entities,
    const std::map<std::string, TronicPositionMap>& tronic_by_entity,
    Model* const* animatronic_models, std::size_t animatronic_model_count,
    const Vector3& default_pos,
    const Vector3& rotation_axis, const Vector3& anim_scale) {
  if (animatronic_model_count < 2)
    return 0;
  const Model& default_mesh = *animatronic_models[1];
  const std::map<std::string, int> occupancy =
      tronic_room_occupancy(sim_entities, nullptr);
  std::size_t drawn = 0;
  for (const auto& ent : sim_entities) {
    const std::string tkey = tronic_key_from_sim_entity(ent.id, ent.name);
    if (tkey.empty())
      continue;
    if (tkey == "freddy" || tkey == "toy_freddy")
      continue;
    Vector3 pos{};
    Vector3 ax{};
    float ang = 0.0f;
    if (!resolve_tronic_draw_transform(ent, tronic_by_entity, default_pos,
                                       rotation_axis,
                                       /*use_backup_when_pos_missing=*/true,
                                       &pos, &ax, &ang))
      continue;
    auto named = tronic_by_entity.find(tkey);
    const Model* mesh = &default_mesh;
    Vector3 draw_scale = tronic_draw_scale_for(tronic_by_entity, tkey, anim_scale);
    Vector3 off{0.0f, 0.0f, 0.0f};
    if (named != tronic_by_entity.end()) {
      mesh = &named->second.model;
      draw_scale = named->second.scale;
      off = named->second.character_offset;
    } else {
      auto fb = tronic_by_entity.find("freddy");
      if (fb != tronic_by_entity.end()) {
        mesh = &fb->second.model;
        draw_scale = fb->second.scale;
        off = fb->second.character_offset;
      }
    }
    if (should_apply_character_offset(ent)) {
      pos.x += off.x;
      pos.y += off.y;
      pos.z += off.z;
    }
    auto occ_it = occupancy.find(ent.room_alias);
    if (occ_it != occupancy.end() && occ_it->second > 1) {
      const float spread = 0.42f;
      pos.x += static_cast<float>((ent.id % 5) - 2) * spread;
    }
    DrawModelEx(*mesh, pos, ax, ang, draw_scale, WHITE);
    ++drawn;
  }
  return drawn;
}

inline void draw_main_scene_3d(
    CameraNavState& camera_nav, bool is_player_one, const GameState& state,
    Model* const* animatronic_models, std::size_t animatronic_model_count,
    const Vector3& default_pos,
    const std::map<std::string, TronicPositionMap>& tronic_by_entity,
    const Model& map, const Model& p_map, bool debug_tronic_coords) {
  const Vector3 rotationAxis = {0.0f, 1.0f, 0.0f};
  const Vector3 anim_scale = tronic_draw_scale_for(tronic_by_entity, "freddy",
                                                   {0.05f, 0.05f, 0.05f});

  if (animatronic_model_count < 2)
    return;
  const Model& default_mesh = *animatronic_models[1];
  if(state.is_player_one) DrawModel(map, (Vector3){0.0f, 2.5f, -2.5f}, 1.0f, WHITE);
  else DrawModel(p_map, (Vector3){1.0f, 50.0f, -13.0f}, 1.0f, WHITE);

  const bool on_feed = camera_nav.active_feed >= 0;
  if (!on_feed || !state.gameStarted) {
    if (state.gameStarted && !state.sim_entities.empty()) {
      std::size_t drawn = 0;
      if (debug_tronic_coords) {
        drawn = draw_tronic_sim_entities_matching_debug_hud(
            state.sim_entities, tronic_by_entity, animatronic_models,
            animatronic_model_count,
            default_pos, rotationAxis, anim_scale);
      } else {
        drawn = draw_tronic_sim_entities(state.sim_entities, tronic_by_entity,
                                         animatronic_models,
                                         animatronic_model_count, default_pos,
                                         rotationAxis, anim_scale, nullptr,
                                         false);
      }
      if (drawn == 0) {
        DrawModelEx(default_mesh, default_pos, rotationAxis, 0.0f, anim_scale,
                    WHITE);
      }
    } else {
      DrawModelEx(default_mesh, default_pos, rotationAxis, 0.0f, anim_scale,
                  WHITE);

                  
    }
    return;
  }

  int n_cams = 0;
  const SecurityCamera* cams =
      CameraMaps::MapForPlayer(is_player_one, &n_cams);
  if (camera_nav.active_feed >= n_cams || n_cams == 0) {
    DrawModelEx(default_mesh, default_pos, rotationAxis, 0.0f, anim_scale,
                WHITE);
    return;
  }

  const SecurityCamera& sc = cams[camera_nav.active_feed];
  const char* sim_alias = sc.sim_room_alias;
  if (!sim_alias || sim_alias[0] == '\0') {
    DrawModelEx(default_mesh, default_pos, rotationAxis, 0.0f, anim_scale,
                WHITE);
    return;
  }

  if (debug_tronic_coords && !state.sim_entities.empty()) {
    if (draw_tronic_sim_entities_matching_debug_hud(
            state.sim_entities, tronic_by_entity, animatronic_models,
            animatronic_model_count,
            default_pos, rotationAxis, anim_scale) == 0) {
      DrawModelEx(default_mesh, default_pos, rotationAxis, 0.0f, anim_scale,
                  WHITE);
    }
    return;
  }

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

  draw_tronic_sim_entities(state.sim_entities, tronic_by_entity,
                            animatronic_models, animatronic_model_count,
                            default_pos, rotationAxis,
                            anim_scale, sim_alias, true);
}

inline void draw_tronic_coords_debug_hud(
    const GameState& state,
    const std::map<std::string, TronicPositionMap>& tronic_by_entity,
    const Vector3& default_pos) {
  if (!state.gameStarted || state.sim_entities.empty())
    return;
  const Vector3 default_axis = {0.0f, 1.0f, 0.0f};
  int y = 140;
  DrawText("Tronic world coords:", 10, y, 14, LIME);
  y += 18;
  char line[256];
  const std::map<std::string, int> occupancy =
      tronic_room_occupancy(state.sim_entities, nullptr);
  for (const auto& ent : state.sim_entities) {
    const std::string tkey = tronic_key_from_sim_entity(ent.id, ent.name);
    if (tkey.empty())
      continue;
    Vector3 pos{};
    Vector3 ax{};
    float ang = 0.0f;
    if (!resolve_tronic_draw_transform(ent, tronic_by_entity, default_pos,
                                       default_axis,
                                       /*use_backup_when_pos_missing=*/true,
                                       &pos, &ax, &ang))
      continue;
    Vector3 off{0.0f, 0.0f, 0.0f};
    auto named = tronic_by_entity.find(tkey);
    if (named != tronic_by_entity.end())
      off = named->second.character_offset;
    else {
      auto fb = tronic_by_entity.find("freddy");
      if (fb != tronic_by_entity.end())
        off = fb->second.character_offset;
    }
    if (should_apply_character_offset(ent)) {
      pos.x += off.x;
      pos.y += off.y;
      pos.z += off.z;
    }
    auto occ_it = occupancy.find(ent.room_alias);
    if (occ_it != occupancy.end() && occ_it->second > 1) {
      const float spread = 0.42f;
      pos.x += static_cast<float>((ent.id % 5) - 2) * spread;
    }
    std::snprintf(line, sizeof(line),
                  "%s  [%s]  x=%.2f y=%.2f z=%.2f", ent.name.c_str(),
                  ent.room_alias.c_str(), static_cast<double>(pos.x),
                  static_cast<double>(pos.y), static_cast<double>(pos.z));
    DrawText(line, 10, y, 14, RAYWHITE);
    y += 16;
    if (y > GetScreenHeight() - 80)
      break;
  }
}

inline void draw_main_scene_2d(Camera& camera, CameraNavState& camera_nav,
                               const GameState& state,
                               bool show_tronic_coords_debug,
                               const std::map<std::string, TronicPositionMap>&
                                   tronic_by_entity,
                               const Vector3& tronic_default_pos) {
  camera_nav.DrawPanel(state.is_player_one);
  char coord_text[64];
  std::snprintf(coord_text, sizeof(coord_text), "x: %.2f",
                static_cast<double>(camera.position.x));
  DrawText(coord_text, 10, 10, 16, WHITE);
  std::snprintf(coord_text, sizeof(coord_text), "y: %.2f",
                static_cast<double>(camera.position.y));
  DrawText(coord_text, 10, 28, 16, WHITE);
  std::snprintf(coord_text, sizeof(coord_text), "z: %.2f",
                static_cast<double>(camera.position.z));
  DrawText(coord_text, 10, 46, 16, WHITE);
  if (state.has_player_slot) {
    const char* label = state.is_player_one ? "Player 1" : "Player 2";
    DrawText(label, 10, 64, 16, WHITE);
  }
  if (!state.is_player_one) {
    const char* mask_label = state.p2_mask_down ? "Mask: DOWN" : "Mask: UP";
    Color mask_color = state.p2_mask_down ? GREEN : ORANGE;
    DrawText(mask_label, 10, 82, 16, mask_color);
  }
  if (show_tronic_coords_debug) {
    draw_tronic_coords_debug_hud(state, tronic_by_entity, tronic_default_pos);
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
  DrawText("T = manual server sim step (after start)", 10, 118, 14,
           Fade(LIGHTGRAY, 0.75f));
  DrawText("F3 = toggle tronic x,y,z (after start)", 10, 134, 14,
           Fade(LIGHTGRAY, 0.75f));
}

}  // namespace mainscene

#pragma once
#include <array>
#include <cmath>
#include <cstdio>
#include <map>
#include <string>
#include <vector>
#include "GameState.hpp"
#include "Scene.hpp"
#include "audio_runtime.hpp"
#include "camera_nav.hpp"
#include "dev_flags.hpp"
#include "mainscene/helpers/camera_control.hpp"
#include "mainscene/helpers/frame.hpp"
#include "mainscene/helpers/pbr_lights.hpp"
#include "mainscene/helpers/rooms/create_tronic_positions.hpp"
#include "pbr_light_ids.hpp"
#include "raylib.h"
#include "ws_init.hpp"
Vector3 camPos = {0.0f, 5.0f, 0.0f};
float yaw = PI;
float pitch = 0.0f;

class MainScene : public Scene {
 private:
  struct TronicLayoutTweak {
    Vector3 offset;
    float rotation_deg = 0.0f;
    float scale = 1.0f;
  };
  struct TransformModifier {
    Vector3 offset;
    float scale = 1.0f;
  };
  struct CameraPoseOverride {
    Vector3 eye_pos;
    Vector3 eye_target;
    float fovy = 55.0f;
  };
  Texture2D texture;
  Model mask;
  Model map;
  Model p_map;
  Model freddy;
  Model bonnie;
  Model chica;
  Model foxy;
  Model toy_freddy;
  Model toy_bonnie;
  Model toy_chica;
  Model toy_foxy;
  Model door;
  Vector3 freddyInitialPos;
  Camera& camera;
  Shader map_shader;
  CameraNavState camera_nav_;
  bool is_freeroam = false;
  std::array<Model*, 4> animatronic_models_{};
  mainscene::PbrLightGPU pbr_lights_[mainscene::kMaxPbrLights];
  int pbr_light_count_ = 0;
  std::map<std::string, TronicPositionMap> tronic_maps_;
  std::map<std::string, TronicLayoutTweak> applied_tronic_tweaks_;
  std::map<std::string, TransformModifier> room_modifiers_;
  TransformModifier global_modifier_{{0.0f, 0.0f, 0.0f}, 1.0f};
  std::map<std::string, CameraPoseOverride> camera_pose_overrides_;
  int applied_layout_revision_ = -1;
  int debug_tronic_selection_ = 0;
  bool debug_tronic_coords_ = false;
  bool debug_fullbright_ = false;
  bool debug_transform_menu_open_ = true;
  int debug_transform_field_ = 0;
  int debug_room_selection_ = 0;
  bool debug_layout_dirty_ = false;
  std::vector<std::string> debug_room_choices_;
  bool left_door_closed_ = false;
  bool right_door_closed_ = false;
  float left_door_y_ = 6.5f;
  float right_door_y_ = 6.5f;
  /// P2: E in office arms tasks; then left-mouse hold on L/R half queues power/music.
  bool p2_e_tasks_armed_ = false;
  bool p2_task_overlay_open_ = false;
  float p2_mouse_hold_power_acc_ = 0.0f;
  float p2_mouse_hold_music_acc_ = 0.0f;
  struct DeferredModelLoad {
    const char* path;
    Model* target;
    bool apply_pbr;
  };
  std::vector<DeferredModelLoad> deferred_model_loads_;
  std::map<std::string, Vector3> tronic_target_scales_;
  size_t deferred_model_cursor_ = 0;
  float deferred_model_next_load_time_ = 0.0f;
  bool scene_models_ready_ = false;
  Model proxy_anim_;
  Model proxy_map_;
  Model proxy_mask_;
  Model proxy_door_;

  static constexpr float kDoorYDown = 2.5f;
  static constexpr float kDoorYUp = 6.5f;
  static constexpr float kDoorMoveSpeed = 8.0f;
  static constexpr float kProxyScaleMin = 0.34f;
  static constexpr const char* kFreddyPath =
      "assets/replacements/fnaf_plus_freddy_v1.glb";
  static constexpr const char* kBonniePath = "assets/replacements/bonnie.glb";
  static constexpr const char* kChicaPath =
      "assets/replacements/shattered_chica.glb";
  static constexpr const char* kFoxyPath =
      "assets/replacements/phantom_foxy.glb";
  static constexpr const char* kToyFreddyPath =
      "assets/replacements/toy-freddy.glb";
  static constexpr const char* kToyBonniePath =
      "assets/replacements/vintage_toy_bonnie.glb";
  static constexpr const char* kToyChicaPath =
      "assets/replacements/vhs_tape_chica.glb";
  static constexpr const char* kToyFoxyPath =
      "assets/replacements/mangle_-_fnaf_ar_special_delivery.glb";
  static constexpr const char* kMap1Path = "assets/fnaf_1_hw_map.glb";
  static constexpr const char* kMap2Path = "assets/fnaf_2_hw_map_updated.glb";
  static constexpr const char* kPbrVsPath = "assets/shaders/glsl100/pbr.vs";
  static constexpr const char* kPbrFsPath = "assets/shaders/glsl100/pbr.fs";
  static constexpr const char* kDoorPath = "assets/door.glb";
  static constexpr const char* kMaskPath = "assets/freddy_mask.glb";
  static constexpr const char* kDebugTronicKeys[8] = {
      "freddy", "bonnie", "chica", "foxy",
      "toy_freddy", "toy_bonnie", "toy_chica", "toy_foxy"};

  static Model make_proxy_model(float w, float h, float d, Color tint) {
    Model m = LoadModelFromMesh(GenMeshCube(w, h, d));
    if (m.materialCount > 0) {
      m.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = tint;
    }
    return m;
  }

  static Vector3 scaled_vector(Vector3 v, float s) {
    return {v.x * s, v.y * s, v.z * s};
  }

  void apply_proxy_tronic_scales(float t) {
    for (auto& it : tronic_maps_) {
      auto target = tronic_target_scales_.find(it.first);
      if (target == tronic_target_scales_.end())
        continue;
      it.second.scale = scaled_vector(target->second, t);
    }
  }

  void setup_proxies() {
    proxy_anim_ = make_proxy_model(1.0f, 2.0f, 1.0f, (Color){110, 130, 180, 255});
    proxy_map_ = make_proxy_model(70.0f, 10.0f, 70.0f, (Color){80, 80, 96, 255});
    proxy_mask_ = make_proxy_model(0.8f, 0.8f, 0.6f, (Color){150, 120, 80, 255});
    proxy_door_ = make_proxy_model(1.4f, 4.0f, 0.2f, (Color){90, 90, 90, 255});

    freddy = proxy_anim_;
    bonnie = proxy_anim_;
    chica = proxy_anim_;
    foxy = proxy_anim_;
    toy_freddy = proxy_anim_;
    toy_bonnie = proxy_anim_;
    toy_chica = proxy_anim_;
    toy_foxy = proxy_anim_;
    map = proxy_map_;
    p_map = proxy_map_;
    door = proxy_door_;
    mask = proxy_mask_;
  }

  void setup_deferred_model_loads() {
    deferred_model_loads_ = {
        {kMap1Path, &map, true},
        {kMap2Path, &p_map, true},
        {kFreddyPath, &freddy, true},
        {kBonniePath, &bonnie, true},
        {kChicaPath, &chica, true},
        {kFoxyPath, &foxy, true},
        {kToyFreddyPath, &toy_freddy, true},
        {kToyBonniePath, &toy_bonnie, true},
        {kToyChicaPath, &toy_chica, true},
        {kToyFoxyPath, &toy_foxy, true},
        {kDoorPath, &door, true},
        {kMaskPath, &mask, true},
    };
  }

  void stream_next_model_if_due() {
    if (scene_models_ready_)
      return;
    if (deferred_model_cursor_ >= deferred_model_loads_.size()) {
      scene_models_ready_ = true;
      apply_proxy_tronic_scales(1.0f);
      return;
    }
    const float now = static_cast<float>(GetTime());
    if (now < deferred_model_next_load_time_)
      return;

    DeferredModelLoad& next = deferred_model_loads_[deferred_model_cursor_];
    Model loaded = LoadModel(next.path);
    if (loaded.meshCount > 0) {
      *(next.target) = loaded;
      if (next.apply_pbr)
        mainscene::assign_pbr_to_model(map_shader, *(next.target));
      if (next.target == &map) {
        for (int i = 0; i < map.materialCount; i++) {
          if (map.materials[i].maps[MATERIAL_MAP_DIFFUSE].texture.id == 0) {
            printf("Missing texture on material %d\n", i);
          }
        }
      }
    } else {
      printf("Failed to load model: %s\n", next.path);
    }

    deferred_model_cursor_++;
    const float progress = deferred_model_loads_.empty()
                               ? 1.0f
                               : static_cast<float>(deferred_model_cursor_) /
                                     static_cast<float>(deferred_model_loads_.size());
    apply_proxy_tronic_scales(kProxyScaleMin + (1.0f - kProxyScaleMin) * progress);
    deferred_model_next_load_time_ = now + 0.10f;
    if (deferred_model_cursor_ >= deferred_model_loads_.size()) {
      scene_models_ready_ = true;
      apply_proxy_tronic_scales(1.0f);
    }
  }

  void apply_layout_from_state(const GameState& state) {
    if (state.layout_revision == applied_layout_revision_)
      return;
    applied_layout_revision_ = state.layout_revision;

    std::map<std::string, TronicLayoutTweak> next_tweaks;
    for (const auto& row : state.layout_tronic_offsets) {
      TronicLayoutTweak t{};
      t.offset = {row.x, row.y, row.z};
      t.rotation_deg = row.rotation_deg;
      t.scale = row.scale <= 0.0f ? 1.0f : row.scale;
      next_tweaks[row.key] = t;
    }

    for (const char* k : kDebugTronicKeys) {
      auto prev = applied_tronic_tweaks_.find(k);
      auto next = next_tweaks.find(k);
      TronicLayoutTweak prev_t{{0.0f, 0.0f, 0.0f}, 0.0f, 1.0f};
      if (prev != applied_tronic_tweaks_.end())
        prev_t = prev->second;
      TronicLayoutTweak next_t{{0.0f, 0.0f, 0.0f}, 0.0f, 1.0f};
      if (next != next_tweaks.end())
        next_t = next->second;
      Vector3 prev_v = prev_t.offset;
      Vector3 next_v = next_t.offset;
      Vector3 d = {next_v.x - prev_v.x, next_v.y - prev_v.y, next_v.z - prev_v.z};
      add_tronic_layout_offset(tronic_maps_, k, d);
      auto tm = tronic_maps_.find(k);
      if (tm != tronic_maps_.end()) {
        tm->second.layout_rotation_deg = next_t.rotation_deg;
        tm->second.layout_scale_mul = next_t.scale;
      }
    }
    applied_tronic_tweaks_ = next_tweaks;

    camera_pose_overrides_.clear();
    for (const auto& row : state.layout_camera_rows) {
      camera_pose_overrides_[row.key] = {
          {row.ex, row.ey, row.ez},
          {row.tx, row.ty, row.tz},
          row.fovy,
      };
    }

    room_modifiers_.clear();
    for (const auto& row : state.layout_room_modifiers) {
      room_modifiers_[row.key] = {{row.x, row.y, row.z},
                                  row.scale <= 0.0f ? 1.0f : row.scale};
    }
    global_modifier_.offset = {state.layout_global_modifier.x,
                               state.layout_global_modifier.y,
                               state.layout_global_modifier.z};
    global_modifier_.scale =
        state.layout_global_modifier.scale <= 0.0f
            ? 1.0f
            : state.layout_global_modifier.scale;
  }

  std::map<std::string, mainscene::LayoutRoomModifier> room_mods_for_draw() const {
    std::map<std::string, mainscene::LayoutRoomModifier> out;
    for (const auto& kv : room_modifiers_) {
      out[kv.first] = {kv.second.offset, kv.second.scale <= 0.0f ? 1.0f : kv.second.scale};
    }
    return out;
  }

  mainscene::LayoutGlobalModifier global_mod_for_draw() const {
    mainscene::LayoutGlobalModifier g;
    g.offset = global_modifier_.offset;
    g.scale = global_modifier_.scale <= 0.0f ? 1.0f : global_modifier_.scale;
    return g;
  }

  void refresh_debug_room_choices(const GameState& state) {
    std::map<std::string, bool> seen;
    std::vector<std::string> next;
    for (const auto& ent : state.sim_entities) {
      if (ent.room_alias.empty())
        continue;
      if (seen[ent.room_alias])
        continue;
      seen[ent.room_alias] = true;
      next.push_back(ent.room_alias);
    }
    if (next.empty())
      return;
    debug_room_choices_ = next;
    if (debug_room_selection_ < 0)
      debug_room_selection_ = 0;
    if (debug_room_selection_ >= static_cast<int>(debug_room_choices_.size()))
      debug_room_selection_ = static_cast<int>(debug_room_choices_.size()) - 1;
  }

  void apply_camera_view_with_overrides(const GameState& state,
                                        const bool freeroam_active) {
    if (camera_nav_.active_feed >= 0) {
      int n = 0;
      const SecurityCamera* map = CameraMaps::MapForPlayer(state.is_player_one, &n);
      if (camera_nav_.active_feed >= n)
        camera_nav_.active_feed = n > 0 ? n - 1 : 0;
      if (camera_nav_.active_feed >= 0 && camera_nav_.active_feed < n) {
        const SecurityCamera& sc = map[camera_nav_.active_feed];
        auto it = camera_pose_overrides_.find(sc.sim_room_alias);
        if (it != camera_pose_overrides_.end()) {
          this->camera.position = it->second.eye_pos;
          this->camera.target = it->second.eye_target;
          this->camera.up = {0.0f, 1.0f, 0.0f};
          this->camera.fovy = it->second.fovy;
          this->camera.projection = CAMERA_PERSPECTIVE;
          return;
        }
      }
      CameraMaps::ApplySecurityCameraView(map[camera_nav_.active_feed],
                                          static_cast<Camera3D&>(this->camera));
      return;
    }
    mainscene::update_office_camera(this->camera, camPos, yaw, pitch,
                                    freeroam_active);
  }

  void handle_debug_layout_controls(const GameState& state,
                                    EMSCRIPTEN_WEBSOCKET_T& socket) {
    refresh_debug_room_choices(state);
    if (IsKeyPressed(KEY_M))
      debug_transform_menu_open_ = !debug_transform_menu_open_;
    if (!debug_transform_menu_open_)
      return;

    if (IsKeyPressed(KEY_ONE))
      debug_tronic_selection_ = 0;
    if (IsKeyPressed(KEY_TWO))
      debug_tronic_selection_ = 1;
    if (IsKeyPressed(KEY_THREE))
      debug_tronic_selection_ = 2;
    if (IsKeyPressed(KEY_FOUR))
      debug_tronic_selection_ = 3;
    if (IsKeyPressed(KEY_FIVE))
      debug_tronic_selection_ = 4;
    if (IsKeyPressed(KEY_SIX))
      debug_tronic_selection_ = 5;
    if (IsKeyPressed(KEY_SEVEN))
      debug_tronic_selection_ = 6;
    if (IsKeyPressed(KEY_EIGHT))
      debug_tronic_selection_ = 7;
    if (debug_tronic_selection_ < 0)
      debug_tronic_selection_ = 0;
    if (debug_tronic_selection_ > 7)
      debug_tronic_selection_ = 7;

    if (IsKeyPressed(KEY_UP))
      debug_transform_field_ = (debug_transform_field_ + 4) % 5;
    if (IsKeyPressed(KEY_DOWN))
      debug_transform_field_ = (debug_transform_field_ + 1) % 5;
    if (IsKeyPressed(KEY_TAB))
      debug_transform_field_ = (debug_transform_field_ + 1) % 5;

    if (!debug_room_choices_.empty()) {
      if (IsKeyPressed(KEY_LEFT_BRACKET)) {
        debug_room_selection_ =
            (debug_room_selection_ + static_cast<int>(debug_room_choices_.size()) - 1) %
            static_cast<int>(debug_room_choices_.size());
        debug_layout_dirty_ = true;
      }
      if (IsKeyPressed(KEY_RIGHT_BRACKET)) {
        debug_room_selection_ =
            (debug_room_selection_ + 1) % static_cast<int>(debug_room_choices_.size());
        debug_layout_dirty_ = true;
      }
    } else {
      debug_room_selection_ = 0;
    }

    if (!state.server_paused) {
      return;
    }

    const char* tronic_key = kDebugTronicKeys[debug_tronic_selection_];
    TronicLayoutTweak curr{{0.0f, 0.0f, 0.0f}, 0.0f, 1.0f};
    auto it = applied_tronic_tweaks_.find(tronic_key);
    if (it != applied_tronic_tweaks_.end())
      curr = it->second;

    const bool adjust_dec = IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_COMMA);
    const bool adjust_inc = IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_PERIOD);
    if (adjust_dec || adjust_inc) {
      const bool fine = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
      const float dir = adjust_inc ? 1.0f : -1.0f;
      float step = 0.10f;
      if (debug_transform_field_ == 3)
        step = fine ? 0.25f : 2.0f;
      else if (debug_transform_field_ == 4)
        step = fine ? 0.01f : 0.05f;
      else
        step = fine ? 0.01f : 0.10f;

      const float delta = dir * step;
      TronicLayoutTweak next = curr;
      if (debug_transform_field_ == 0)
        next.offset.x += delta;
      else if (debug_transform_field_ == 1)
        next.offset.y += delta;
      else if (debug_transform_field_ == 2)
        next.offset.z += delta;
      else if (debug_transform_field_ == 3)
        next.rotation_deg += delta;
      else if (debug_transform_field_ == 4)
        next.scale = fmaxf(0.10f, next.scale + delta);

      Vector3 d = {next.offset.x - curr.offset.x, next.offset.y - curr.offset.y,
                   next.offset.z - curr.offset.z};
      add_tronic_layout_offset(tronic_maps_, tronic_key, d);
      auto tm = tronic_maps_.find(tronic_key);
      if (tm != tronic_maps_.end()) {
        tm->second.layout_rotation_deg = next.rotation_deg;
        tm->second.layout_scale_mul = next.scale;
      }
      applied_tronic_tweaks_[tronic_key] = next;
      debug_layout_dirty_ = true;
    }

    if (IsKeyPressed(KEY_ENTER)) {
      TronicLayoutTweak save_t = curr;
      auto save_it = applied_tronic_tweaks_.find(tronic_key);
      if (save_it != applied_tronic_tweaks_.end())
        save_t = save_it->second;
      ws::send_debug_tronic_offset(socket, tronic_key, save_t.offset.x,
                                   save_t.offset.y, save_t.offset.z,
                                   save_t.rotation_deg, save_t.scale);
      if (!debug_room_choices_.empty()) {
        const std::string room_key = debug_room_choices_[debug_room_selection_];
        ws::send_debug_move_tronic_to_room(socket, tronic_key, room_key);
        ws::send_debug_save_tronic_room(socket, tronic_key, room_key);
      }
      debug_layout_dirty_ = false;
    }
  }

  void draw_debug_transform_menu(const GameState& state) {
    if (!(is_dev && debug_transform_menu_open_))
      return;
    const int w = 560;
    const int h = 250;
    const int x = GetScreenWidth() - w - 14;
    const int y = GetScreenHeight() - h - 14;
    DrawRectangle(x, y, w, h, Fade((Color){9, 14, 22, 255}, 0.94f));
    DrawRectangleLines(x, y, w, h, Fade(SKYBLUE, 0.62f));
    DrawText("Layout Editor (Raylib)", x + 14, y + 10, 20, SKYBLUE);

    const char* tronic_key = kDebugTronicKeys[debug_tronic_selection_];
    TronicLayoutTweak t{{0.0f, 0.0f, 0.0f}, 0.0f, 1.0f};
    auto it = applied_tronic_tweaks_.find(tronic_key);
    if (it != applied_tronic_tweaks_.end())
      t = it->second;

    const char* room_name = "(none)";
    if (!debug_room_choices_.empty() &&
        debug_room_selection_ >= 0 &&
        debug_room_selection_ < static_cast<int>(debug_room_choices_.size())) {
      room_name = debug_room_choices_[debug_room_selection_].c_str();
    }

    char line[300];
    std::snprintf(line, sizeof(line), "Tronic %d/8: %s (1-8 select)",
                  debug_tronic_selection_ + 1, tronic_key);
    DrawText(line, x + 14, y + 38, 16, Fade(RAYWHITE, 0.95f));

    std::snprintf(line, sizeof(line), "Room: %s ([ and ] select)",
                  room_name);
    DrawText(line, x + 14, y + 58, 15, Fade(LIGHTGRAY, 0.95f));

    const Color lock_color = state.server_paused ? Fade(GREEN, 0.95f) : ORANGE;
    const char* lock_text = state.server_paused
                                ? "Edit status: unlocked (paused)"
                                : "Edit status: locked (press P to pause)";
    DrawText(lock_text, x + 14, y + 78, 15, lock_color);
    if (state.server_paused) {
      DrawText(debug_layout_dirty_ ? "DB save pending (Enter)"
                                   : "DB state synced",
               x + 320, y + 78, 15,
               debug_layout_dirty_ ? ORANGE : Fade(GREEN, 0.9f));
    }

    const char* names[5] = {"pos.x", "pos.y", "pos.z", "rotation", "scale"};
    const float values[5] = {t.offset.x, t.offset.y, t.offset.z, t.rotation_deg,
                             t.scale};
    for (int i = 0; i < 5; ++i) {
      std::snprintf(line, sizeof(line), "%-12s %8.3f", names[i], values[i]);
      DrawText(line, x + 24, y + 108 + i * 22, 18,
               i == debug_transform_field_ ? ORANGE : RAYWHITE);
    }

    DrawText("Up/Down/TAB field | Left/Right adjust | Enter save to DB | [ ] room",
             x + 14, y + h - 26, 14, Fade(LIGHTGRAY, 0.88f));
  }

  void enable_pbr() {

    mainscene::setup_pbr_shader_locs(map_shader);
    mainscene::setup_pbr_shader_uniform_defaults(map_shader);
    mainscene::init_all_scene_pbr_lights(map_shader, pbr_lights_,
                                         pbr_light_count_);
    mainscene::assign_pbr_to_model(map_shader, map);
    mainscene::assign_pbr_to_model(map_shader, p_map);
    mainscene::assign_pbr_to_model(map_shader, freddy);
    mainscene::assign_pbr_to_model(map_shader, bonnie);
    mainscene::assign_pbr_to_model(map_shader, chica);
    mainscene::assign_pbr_to_model(map_shader, foxy);
    mainscene::assign_pbr_to_model(map_shader, toy_freddy);
    mainscene::assign_pbr_to_model(map_shader, toy_bonnie);
    mainscene::assign_pbr_to_model(map_shader, toy_chica);
    mainscene::assign_pbr_to_model(map_shader, toy_foxy);
    mainscene::assign_pbr_to_model(map_shader, door);
    mainscene::assign_pbr_to_model(map_shader, mask);
  }

  void set_all_pbr_lights(bool enabled) {
    for (int i = 0; i < pbr_light_count_; ++i) {
      mainscene::set_pbr_light_enabled(pbr_lights_, pbr_light_count_, i, enabled);
    }
  }

  void apply_fullbright_shader(bool enabled) {
    if (enabled) {
      mainscene::set_pbr_ambient(map_shader, {1.0f, 1.0f, 1.0f}, 1.25f);
    } else {
      mainscene::set_pbr_ambient(map_shader, {0.0f, 0.0f, 0.0f}, -0.5f);
    }
  }

  bool debug_modifier_down() const {
    return IsKeyDown(KEY_LEFT_SUPER) || IsKeyDown(KEY_RIGHT_SUPER) ||
           IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
  }

  bool debug_freeroam_hotkey_pressed() const {
    // Browser/canvas key handling for Meta can vary; accept cmd/ctrl+` and plain `.
    if (!IsKeyPressed(KEY_GRAVE))
      return false;
    return true;
  }

  bool debug_coords_hotkey_pressed() const {
    return IsKeyPressed(KEY_GRAVE) && IsKeyDown(KEY_LEFT_SHIFT);
  }
  void update_doors(bool allow_input) {
    if (allow_input && IsKeyPressed(KEY_Q))
      left_door_closed_ = !left_door_closed_;
    if (allow_input && IsKeyPressed(KEY_E))
      right_door_closed_ = !right_door_closed_;

    const float dt = GetFrameTime();
    const float step = kDoorMoveSpeed * dt;

    const float left_target = left_door_closed_ ? kDoorYDown : kDoorYUp;
    const float right_target = right_door_closed_ ? kDoorYDown : kDoorYUp;

    if (left_door_y_ < left_target) {
      left_door_y_ = fminf(left_door_y_ + step, left_target);
    } else if (left_door_y_ > left_target) {
      left_door_y_ = fmaxf(left_door_y_ - step, left_target);
    }

    if (right_door_y_ < right_target) {
      right_door_y_ = fminf(right_door_y_ + step, right_target);
    } else if (right_door_y_ > right_target) {
      right_door_y_ = fmaxf(right_door_y_ - step, right_target);
    }
  }

  void draw_doors() {
    DrawModel(door, {-4.0f, left_door_y_, -2.5f}, 1.0f, WHITE);
    DrawModel(door, {0.0f, right_door_y_, -2.5f}, 1.0f, WHITE);
  }
  /// First-person mask held in front of P2; uses global camPos/yaw from office cam.
  void draw_mask() {
    constexpr float kHoldDist = 0.1f;
    constexpr float kEyeOffsetY = -0.70f;
    constexpr float kScale = 0.14f;
    Vector3 forward = {-sinf(yaw), 0.0f, -cosf(yaw)};
    Vector3 pos = {camPos.x + forward.x * kHoldDist, camPos.y + kEyeOffsetY,
                   camPos.z + forward.z * kHoldDist};
    const Vector3 axis = {0.0f, 1.0f, 0.0f};
    float rot_deg = (yaw * RAD2DEG) - 180.0f;
    DrawModelEx(mask, pos, axis, rot_deg, {kScale, kScale, kScale}, WHITE);
  }

 public:
  MainScene(Camera& cam) : Scene(cam), camera(cam) {
    SetMouseCursor(MOUSE_CURSOR_DEFAULT);
    setup_proxies();
    setup_deferred_model_loads();

    this->freddyInitialPos.x = -1.0f;
    this->freddyInitialPos.y = 4.5f;
    this->freddyInitialPos.z = -42.0f;

    this->camera.position = camPos;
    // this->camera.target = this->freddyInitialPos;

    this->map_shader = LoadShader(kPbrVsPath, kPbrFsPath);

    enable_pbr();
    const TronicRosterSpec tronic_roster{
        {this->freddy, {0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 1.0f}},
        {this->bonnie, {0.0045f, 0.0045f, 0.0045f}, {0.0f, 2.0f, 0.0f}},
        {this->chica, {0.068f, 0.068f, 0.068f}, {2.15f, 0.0f, 0.35f}},
        {this->foxy, {0.2f, 0.2f, 0.2f}, {0.0f, 0.0f, 0.0f}},
        {this->toy_freddy, {0.45f, 0.45f, 0.45f}, {0.0f, 0.0f, 0.0f}},
        {this->toy_bonnie, {0.25f, 0.25f, 0.25f}, {0.0f, 2.0f, 0.0f}},
        {this->toy_chica, {0.85f, 0.85f, 0.85f}, {0.0f, 0.0f, 0.0f}},
        {this->toy_foxy, {1.8f, 1.8f, 1.8f},{0.0f, 0.0f, 0.0f}},
    };
    this->tronic_maps_ = create_tronic_positions(tronic_roster);
    for (const auto& kv : tronic_maps_) {
      tronic_target_scales_[kv.first] = kv.second.scale;
    }
    apply_proxy_tronic_scales(kProxyScaleMin);
    animatronic_models_ = {&this->freddy, &this->bonnie, &this->chica,
                           &this->foxy};
    add_tronic_layout_offset(tronic_maps_, "toy_bonnie", {0.0f, 2.0f, 0.0f});

    this->camera.fovy = 50.0f;
    this->camera.target.x = -1.0f;
    this->camera.target.z = 10.0f;
  }

  bool assets_ready() const { return scene_models_ready_; }

  float assets_progress() const {
    if (deferred_model_loads_.empty())
      return 1.0f;
    if (scene_models_ready_)
      return 1.0f;
    return static_cast<float>(deferred_model_cursor_) /
           static_cast<float>(deferred_model_loads_.size());
  }

  void prime_assets_step() {
    // Keep menu/frame responsiveness: load at most one deferred asset each frame.
    stream_next_model_if_due();
  }

  bool set_pbr_light_enabled(int index, bool enabled) {
    return mainscene::set_pbr_light_enabled(pbr_lights_, pbr_light_count_,
                                            index, enabled);
  }

  bool pbr_light_enabled(PbrLightId id) const {
    return pbr_light_enabled(static_cast<int>(id));
  }

  bool pbr_light_enabled(int index) const {
    return mainscene::pbr_light_enabled(pbr_lights_, pbr_light_count_, index);
  }

  int pbr_light_count() const { return pbr_light_count_; }

  bool set_pbr_light_enabled(PbrLightId id, bool enabled) {
    return set_pbr_light_enabled(static_cast<int>(id), enabled);
  }

  void update(Scene*& curr_scene, GameState& state,
              EMSCRIPTEN_WEBSOCKET_T& socket) override {
    (void)curr_scene;
    apply_layout_from_state(state);
    mainscene::process_check_camera_restore(state, camera_nav_);
    const bool p2_cam_blocked = !state.is_player_one && state.p2_mask_down;
    if (!p2_cam_blocked) {
      mainscene::process_camera_panel_toggle(camera_nav_, state.is_player_one);
      mainscene::try_send_check_camera_room(state, camera_nav_, socket);
    } else {
      camera_nav_.panel_open = false;
      camera_nav_.active_feed = -1;
    }
    if constexpr (is_dev) {
      if (debug_coords_hotkey_pressed()) {
        debug_tronic_coords_ = !debug_tronic_coords_;
      } else if (debug_freeroam_hotkey_pressed()) {
        is_freeroam = !is_freeroam;
        debug_fullbright_ = is_freeroam;
        apply_fullbright_shader(debug_fullbright_);
        if (debug_fullbright_)
          set_all_pbr_lights(true);
      }
      if (state.gameStarted && IsKeyPressed(KEY_T))
        ws::send_step(socket);
      if (state.gameStarted && IsKeyPressed(KEY_P))
        ws::send_debug_pause_toggle(socket);
      handle_debug_layout_controls(state, socket);
    }
    if (state.gameStarted && !state.is_player_one && IsKeyPressed(KEY_Q)) {
      state.p2_mask_down = !state.p2_mask_down;
      ws::send_mask_state(socket, state.p2_mask_down);
      if (state.p2_mask_down) {
        p2_e_tasks_armed_ = false;
        p2_task_overlay_open_ = false;
        p2_mouse_hold_power_acc_ = 0.0f;
        p2_mouse_hold_music_acc_ = 0.0f;
        camera_nav_.panel_open = false;
        camera_nav_.active_feed = -1;
      }
    }

    if (state.gameStarted) {
      if (state.is_player_one && IsKeyPressed(KEY_SPACE)) {
        ws::send_p1_power_queued(socket, 1);
      }
      if (!state.is_player_one && !state.p2_mask_down) {
        constexpr float kHoldSendInterval = 0.12f;
        const bool office_view =
            camera_nav_.active_feed < 0 && !camera_nav_.panel_open;
        // E toggles task mode (office only); mouse only queues while armed.
        if (IsKeyPressed(KEY_E) && office_view)
          p2_e_tasks_armed_ = !p2_e_tasks_armed_;

        p2_task_overlay_open_ = p2_e_tasks_armed_ && office_view;

        if (p2_e_tasks_armed_ && office_view &&
            IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
          const int mx = GetMouseX();
          const int sw = GetScreenWidth();
          const int mid = sw / 2;
          const float dt = GetFrameTime();
          if (mx < mid) {
            p2_mouse_hold_music_acc_ = 0.0f;
            p2_mouse_hold_power_acc_ += dt;
            while (p2_mouse_hold_power_acc_ >= kHoldSendInterval) {
              p2_mouse_hold_power_acc_ -= kHoldSendInterval;
              ws::send_p2_power_queued(socket, 1);
            }
          } else {
            p2_mouse_hold_power_acc_ = 0.0f;
            p2_mouse_hold_music_acc_ += dt;
            while (p2_mouse_hold_music_acc_ >= kHoldSendInterval) {
              p2_mouse_hold_music_acc_ -= kHoldSendInterval;
              ws::send_p2_music_queued(socket, 1);
            }
          }
        } else {
          p2_mouse_hold_power_acc_ = 0.0f;
          p2_mouse_hold_music_acc_ = 0.0f;
        }
        if constexpr (is_dev) {
          if (!state.server_paused && IsKeyPressed(KEY_J))
            ws::send_p2_power_queued(socket, 1);
          if (!state.server_paused && IsKeyPressed(KEY_L))
            ws::send_p2_music_queued(socket, 1);
        }
      }
    } else {
      p2_e_tasks_armed_ = false;
      p2_task_overlay_open_ = false;
      p2_mouse_hold_power_acc_ = 0.0f;
      p2_mouse_hold_music_acc_ = 0.0f;
    }
    if (state.gameStarted && state.is_player_one) {
      const bool prev_left = left_door_closed_;
      const bool prev_right = right_door_closed_;
      update_doors(true);
      if (prev_left != left_door_closed_) {
        audio_runtime::play_door_pound();
        ws::send_door_state(socket, "lhs", left_door_closed_);
      }
      if (prev_right != right_door_closed_) {
        audio_runtime::play_door_pound();
        ws::send_door_state(socket, "rhs", right_door_closed_);
      }
    } else {
      update_doors(false);
    }

    if (is_dev && debug_fullbright_) {
      apply_fullbright_shader(true);
      set_all_pbr_lights(true);
    } else {
      apply_fullbright_shader(false);
      mainscene::clamp_and_apply_pbr_for_security_feed(
          state, camera_nav_, pbr_lights_, pbr_light_count_);
    }

    const bool freeroam_active = is_dev && is_freeroam;
    mainscene::apply_player_two_default_campos(state.is_player_one,
                                               freeroam_active, camPos);

    if (freeroam_active && !(is_dev && state.server_paused && debug_transform_menu_open_)) {
      mainscene::apply_free_cam_move(camPos, yaw, freeroam_active);
    }

    ClearBackground(BLACK);

    apply_camera_view_with_overrides(state, freeroam_active);

    mainscene::sync_pbr_shader_frame(map_shader, camera, pbr_lights_,
                                     pbr_light_count_, camPos);
    const auto room_mods = room_mods_for_draw();
    const auto global_mod = global_mod_for_draw();
    BeginDrawing();
    BeginMode3D(this->camera);
    mainscene::draw_main_scene_3d(
        this->camera_nav_, state.is_player_one, state,
        animatronic_models_.data(), animatronic_models_.size(),
        this->freddyInitialPos, this->tronic_maps_, this->map, this->p_map,
        is_dev && debug_tronic_coords_, &room_mods, &global_mod);
    if (state.is_player_one) {
      this->draw_doors();
    } else if (state.p2_mask_down) {
      this->draw_mask();
    }
    EndMode3D();
    mainscene::draw_main_scene_2d(this->camera, camera_nav_, state,
                                  is_dev && debug_tronic_coords_,
                                  this->tronic_maps_, this->freddyInitialPos,
                                  freeroam_active, p2_task_overlay_open_,
                                  &room_mods, &global_mod);
    if constexpr (is_dev) {
      const char* pause_txt =
          state.server_paused ? "PAUSED (P to resume)" : "RUNNING (P to pause)";
      DrawText(pause_txt, 10, GetScreenHeight() - 44, 18,
               state.server_paused ? ORANGE : Fade(LIGHTGRAY, 0.85f));
      DrawText("Debug layout editor is in-raylib (M to toggle)",
               10, GetScreenHeight() - 24, 14, Fade(SKYBLUE, 0.90f));
      draw_debug_transform_menu(state);
    }
    EndDrawing();
  }

  void listen() override {
    (void)0;
  }
};

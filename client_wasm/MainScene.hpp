#pragma once
#include <array>
#include <cmath>
#include <cstdio>
#include <map>
#include <string>
#include <vector>
#include "GameState.hpp"
#include "Scene.hpp"
#include "asset_preload.hpp"
#include "camera_nav.hpp"
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
  bool debug_tronic_coords_ = false;
  bool left_door_closed_ = false;
  bool right_door_closed_ = false;
  float left_door_y_ = 6.5f;
  float right_door_y_ = 6.5f;
  bool charge_sent_p1_ = false;
  bool charge_sent_p2_power_ = false;
  bool charge_sent_p2_music_ = false;
  /// P2 full-screen generator/music UI (E); off by default so the office is visible.
  bool p2_task_overlay_open_ = false;

  static constexpr float kDoorYDown = 2.5f;
  static constexpr float kDoorYUp = 6.5f;
  static constexpr float kDoorMoveSpeed = 8.0f;

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
    constexpr float kHoldDist = 2.0f;
    constexpr float kEyeOffsetY = -0.25f;
    constexpr float kScale = 0.14f;
    Vector3 forward = {-sinf(yaw), 0.0f, -cosf(yaw)};
    Vector3 pos = {camPos.x + forward.x * kHoldDist, camPos.y + kEyeOffsetY,
                   camPos.z + forward.z * kHoldDist};
    const Vector3 axis = {0.0f, 1.0f, 0.0f};
    float rot_deg = yaw * RAD2DEG;
    DrawModelEx(mask, pos, axis, rot_deg, {kScale, kScale, kScale}, WHITE);
  }

 public:
  MainScene(Camera& cam) : Scene(cam), camera(cam) {
    SetMouseCursor(MOUSE_CURSOR_DEFAULT);

    static constexpr const char* kFreddyPath =
        "assets/freddy_fazbear_from_fnaf_help_wanted.glb";
    static constexpr const char* kBonniePath =
        "assets/fnaf_1_bonnie_by_thudner.glb";
    static constexpr const char* kChicaPath = "assets/chica.glb";
    static constexpr const char* kFoxyPath = "assets/rynfox_fnaf_1_foxy_v6.glb";
    static constexpr const char* kToyFreddyPath =
        "assets/toy_freddy_v2_fixed.glb";
    static constexpr const char* kToyBonniePath = "assets/toy_bonnie.glb";
    static constexpr const char* kToyChicaPath = "assets/toy_chica.glb";
    static constexpr const char* kToyFoxyPath = "assets/fnaf_mangle.glb";
    static constexpr const char* kMap1Path = "assets/fnaf_1_hw_map.glb";
    static constexpr const char* kMap2Path = "assets/fnaf_2_hw_map_updated.glb";
    static constexpr const char* kPbrVsPath = "assets/shaders/glsl100/pbr.vs";
    static constexpr const char* kPbrFsPath = "assets/shaders/glsl100/pbr.fs";
    static constexpr const char* kDoorPath = "assets/door.glb";
    static constexpr const char* kMaskPath = "assets/mask.glb";
    SceneAssetPreloader preloader;
    preloader.PreloadAll(
        {kFreddyPath, kBonniePath, kChicaPath, kFoxyPath, kToyFreddyPath,
         kToyBonniePath, kToyChicaPath, kToyFoxyPath, kMap1Path, kMap2Path},
        {kPbrVsPath, kPbrFsPath});
    preloader.BeginServe();

    this->freddy = LoadModel(kFreddyPath);
    this->bonnie = LoadModel(kBonniePath);
    this->chica = LoadModel(kChicaPath);
    this->foxy = LoadModel(kFoxyPath);
    this->toy_freddy = LoadModel(kToyFreddyPath);
    this->toy_bonnie = LoadModel(kToyBonniePath);
    this->toy_chica = LoadModel(kToyChicaPath);
    this->toy_foxy = LoadModel(kToyFoxyPath);
    this->map = LoadModel(kMap1Path);
    this->p_map = LoadModel(kMap2Path);
    this->door = LoadModel(kDoorPath);
    this->mask = LoadModel(kMaskPath);
    for (int i = 0; i < this->map.materialCount; i++) {
      if (this->map.materials[i].maps[MATERIAL_MAP_DIFFUSE].texture.id == 0) {
        printf("Missing texture on material %d\n", i);
      }
    }

    float distanceInFront = 6.0f;

    this->freddyInitialPos.x = -1.0f;
    this->freddyInitialPos.y = 4.5f;
    this->freddyInitialPos.z = -42.0f;

    this->camera.position = camPos;
    // this->camera.target = this->freddyInitialPos;

    this->map_shader = LoadShader(kPbrVsPath, kPbrFsPath);

    preloader.EndServe();

    enable_pbr();
    const TronicRosterSpec tronic_roster{
        {this->freddy, {50.0f, 50.0f, 50.0f}, {1.0f, 0.0f, 1.0f}},
        {this->bonnie, {0.045f, 0.045f, 0.045f}, {-2.15f, 0.0f, 0.35f}},
        {this->chica, {0.068f, 0.068f, 0.068f}, {2.15f, 0.0f, 0.35f}},
        {this->foxy, {0.048f, 0.048f, 0.048f}, {0.0f, 0.0f, -0.95f}},
        {this->toy_freddy, {0.015f, 0.015f, 0.015f}, {0.0f, 0.0f, 0.35f}},
        {this->toy_bonnie, {2.015f, 2.015f, 2.015f}, {0.15f, 0.0f, 0.35f}},
        {this->toy_chica, {1.5f, 1.5f, 1.5f}, {0.15f, 0.0f, 0.35f}},
        {this->toy_foxy, {1.2f, 1.2f, 1.2f}, {0.0f, 0.0f, 0.6f}},
    };
    this->tronic_maps_ = create_tronic_positions(tronic_roster);
    animatronic_models_ = {&this->freddy, &this->bonnie, &this->chica,
                           &this->foxy};
    this->camera.fovy = 50.0f;
    this->camera.target.x = -1.0f;
    this->camera.target.z = 10.0f;
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
    mainscene::process_check_camera_restore(state, camera_nav_);
    const bool p2_cam_blocked = !state.is_player_one && state.p2_mask_down;
    if (!p2_cam_blocked) {
      mainscene::process_camera_panel_toggle(camera_nav_, state.is_player_one);
      mainscene::try_send_check_camera_room(state, camera_nav_, socket);
    } else {
      camera_nav_.panel_open = false;
      camera_nav_.active_feed = -1;
    }
    if (IsKeyPressed(KEY_L))
      debug_tronic_coords_ = !debug_tronic_coords_;
    if (state.gameStarted && IsKeyPressed(KEY_T))
      ws::send_step(socket);
    if (state.gameStarted && !state.is_player_one && IsKeyPressed(KEY_E)) {
      p2_task_overlay_open_ = !p2_task_overlay_open_;
    }
    if (state.gameStarted && !state.is_player_one && IsKeyPressed(KEY_Q)) {
      state.p2_mask_down = !state.p2_mask_down;
      ws::send_mask_state(socket, state.p2_mask_down);
      if (state.p2_mask_down) {
        p2_task_overlay_open_ = false;
        camera_nav_.panel_open = false;
        camera_nav_.active_feed = -1;
        if (charge_sent_p2_power_) {
          ws::send_charge_hold(socket, "p2power", false);
          charge_sent_p2_power_ = false;
        }
        if (charge_sent_p2_music_) {
          ws::send_charge_hold(socket, "p2music", false);
          charge_sent_p2_music_ = false;
        }
      }
    }

    if (state.gameStarted) {
      if (state.is_player_one) {
        const bool want_charge =
            IsKeyDown(KEY_SPACE) && (!state.p2_in_lobby || state.p2_lost);
        if (want_charge != charge_sent_p1_) {
          ws::send_charge_hold(socket, "p1", want_charge);
          charge_sent_p1_ = want_charge;
        }
      } else {
        if (state.p2_mask_down) {
          if (charge_sent_p2_power_) {
            ws::send_charge_hold(socket, "p2power", false);
            charge_sent_p2_power_ = false;
          }
          if (charge_sent_p2_music_) {
            ws::send_charge_hold(socket, "p2music", false);
            charge_sent_p2_music_ = false;
          }
        } else {
          const bool p2p = IsKeyDown(KEY_J);
          const bool p2m = IsKeyDown(KEY_L);
          if (p2p != charge_sent_p2_power_) {
            ws::send_charge_hold(socket, "p2power", p2p);
            charge_sent_p2_power_ = p2p;
          }
          if (p2m != charge_sent_p2_music_) {
            ws::send_charge_hold(socket, "p2music", p2m);
            charge_sent_p2_music_ = p2m;
          }
        }
      }
    } else {
      p2_task_overlay_open_ = false;
      if (charge_sent_p1_) {
        ws::send_charge_hold(socket, "p1", false);
        charge_sent_p1_ = false;
      }
      if (charge_sent_p2_power_) {
        ws::send_charge_hold(socket, "p2power", false);
        charge_sent_p2_power_ = false;
      }
      if (charge_sent_p2_music_) {
        ws::send_charge_hold(socket, "p2music", false);
        charge_sent_p2_music_ = false;
      }
    }
    if (state.gameStarted && state.is_player_one) {
      const bool prev_left = left_door_closed_;
      const bool prev_right = right_door_closed_;
      update_doors(true);
      if (prev_left != left_door_closed_) {
        ws::send_door_state(socket, "lhs", left_door_closed_);
      }
      if (prev_right != right_door_closed_) {
        ws::send_door_state(socket, "rhs", right_door_closed_);
      }
    } else {
      update_doors(false);
    }

    mainscene::clamp_and_apply_pbr_for_security_feed(
        state, camera_nav_, pbr_lights_, pbr_light_count_);

    mainscene::apply_player_two_default_campos(state.is_player_one, is_freeroam,
                                               camPos);

    if (is_freeroam) {
      mainscene::apply_free_cam_move(camPos, yaw, is_freeroam);
    }

    ClearBackground(BLACK);

    mainscene::apply_security_feed_or_office_view(this->camera, camera_nav_,
                                                  state.is_player_one, camPos,
                                                  yaw, pitch, is_freeroam);

    mainscene::sync_pbr_shader_frame(map_shader, camera, pbr_lights_,
                                     pbr_light_count_, camPos);
    BeginDrawing();
    BeginMode3D(this->camera);
    mainscene::draw_main_scene_3d(this->camera_nav_, state.is_player_one, state,
                                  animatronic_models_.data(),
                                  animatronic_models_.size(),
                                  this->freddyInitialPos, this->tronic_maps_,
                                  this->map, this->p_map, debug_tronic_coords_);
    if (state.is_player_one) {
      this->draw_doors();
    } else if (state.p2_mask_down) {
      this->draw_mask();
    }
    EndMode3D();
    mainscene::draw_main_scene_2d(this->camera, camera_nav_, state,
                                  debug_tronic_coords_, this->tronic_maps_,
                                  this->freddyInitialPos, this->is_freeroam,
                                  p2_task_overlay_open_);
    EndDrawing();
  }

  void listen() override {
    mainscene::on_freeroam_shortcut(camPos, yaw, pitch, is_freeroam);
  }
};

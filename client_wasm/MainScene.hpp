#pragma once
#include <cmath>
#include <cstdio>
#include <array>
#include <string>
#include "GameState.hpp"
#include "Scene.hpp"
#include "asset_preload.hpp"
#include "camera_nav.hpp"
#include "pbr_light_ids.hpp"
#include "raylib.h"
#include <map>
#include <string>
#include "mainscene/helpers/camera_control.hpp"
#include "mainscene/helpers/frame.hpp"
#include "mainscene/helpers/pbr_lights.hpp"
#include "mainscene/helpers/rooms/create_tronic_positions.hpp"
#include "ws_init.hpp"
#include <vector>
Vector3 camPos = {0.0f, 5.0f, 0.0f};
float yaw = PI;
float pitch = 0.0f;

class MainScene : public Scene {
 private:
  Texture2D texture;
  Model map;
  Model p_map;
  Model freddy;
  Model bonnie;
  Model chica;
  Model foxy;
  Model door;
  Vector3 freddyInitialPos;
  Camera& camera;
  Shader map_shader;
  CameraNavState camera_nav_;
  bool is_freeroam = false;
  std::array<Model*, 4> animatronic_models_{};
  mainscene::PbrLightGPU
      pbr_lights_[mainscene::kMaxPbrLights];
  int pbr_light_count_ = 0;
  std::map<std::string, TronicPositionMap> tronic_maps_;
  bool debug_tronic_coords_ = false;

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
    mainscene::assign_pbr_to_model(map_shader, door);
  }
  void draw_doors(){
     DrawModel(door,{0.0f, 2.5f, -2.5f}, 1.0f, WHITE);
          DrawModel(door,{0.0f, 2.5f, 2.5f}, 1.0f, WHITE);

  }

 public:
  MainScene(Camera& cam) : Scene(cam), camera(cam) {
    SetMouseCursor(MOUSE_CURSOR_DEFAULT);

    static constexpr const char* kFreddyPath =
        "assets/freddy_fazbear_from_fnaf_help_wanted.glb";
    static constexpr const char* kBonniePath =
        "assets/fnaf_1_bonnie_by_thudner.glb";
    static constexpr const char* kChicaPath = "assets/chica.glb";
    static constexpr const char* kFoxyPath =
        "assets/rynfox_fnaf_1_foxy_v6.glb";
    static constexpr const char* kMap1Path = "assets/fnaf_1_hw_map.glb";
    static constexpr const char* kMap2Path = "assets/fnaf_2_hw_map_updated.glb";
    static constexpr const char* kPbrVsPath = "assets/shaders/glsl100/pbr.vs";
    static constexpr const char* kPbrFsPath = "assets/shaders/glsl100/pbr.fs";
    static constexpr const char* kDoorPath = "assets/door.glb";

    SceneAssetPreloader preloader;
    preloader.PreloadAll({kFreddyPath, kBonniePath, kChicaPath, kFoxyPath,
                          kMap1Path, kMap2Path},
                         {kPbrVsPath, kPbrFsPath});
    preloader.BeginServe();

    this->freddy = LoadModel(kFreddyPath);
    this->bonnie = LoadModel(kBonniePath);
    this->chica = LoadModel(kChicaPath);
    this->foxy = LoadModel(kFoxyPath);
    this->map = LoadModel(kMap1Path);
    this->p_map = LoadModel(kMap2Path);
    this->door = LoadModel(kDoorPath);
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
    };
    this->tronic_maps_ = create_tronic_positions(tronic_roster);
    animatronic_models_ = {&this->freddy, &this->bonnie, &this->chica,
                           &this->foxy};
    this->camera.fovy = 50.0f;
    this->camera.target.x = -1.0f;
    this->camera.target.z = 10.0f;
  }

  bool set_pbr_light_enabled(int index, bool enabled) {
    return mainscene::set_pbr_light_enabled(
        pbr_lights_, pbr_light_count_, index, enabled);
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
    mainscene::process_camera_panel_toggle(camera_nav_, state.is_player_one);
    mainscene::try_send_check_camera_room(state, camera_nav_, socket);
    if (IsKeyPressed(KEY_L))
      debug_tronic_coords_ = !debug_tronic_coords_;

    mainscene::clamp_and_apply_pbr_for_security_feed(
        state, camera_nav_, pbr_lights_, pbr_light_count_);

    mainscene::apply_player_two_default_campos(state.is_player_one, is_freeroam,
                                                camPos);

    if (is_freeroam) {
      mainscene::apply_free_cam_move(camPos, yaw, is_freeroam);
    }

    ClearBackground(BLACK);

    mainscene::apply_security_feed_or_office_view(
        this->camera, camera_nav_, state.is_player_one, camPos, yaw, pitch,
        is_freeroam);

    mainscene::sync_pbr_shader_frame(map_shader, camera, pbr_lights_,
                                     pbr_light_count_, camPos);
    BeginDrawing();
    BeginMode3D(this->camera);
    mainscene::draw_main_scene_3d(
        this->camera_nav_, state.is_player_one, state,
        animatronic_models_.data(), animatronic_models_.size(),
        this->freddyInitialPos, this->tronic_maps_, this->map, this->p_map,
        debug_tronic_coords_);


    this->draw_doors();
    EndMode3D();
    mainscene::draw_main_scene_2d(this->camera, camera_nav_, state,
                                    debug_tronic_coords_, this->tronic_maps_,
                                    this->freddyInitialPos);
    EndDrawing();
  }

  void listen() override {
    mainscene::on_freeroam_shortcut(camPos, yaw, pitch, is_freeroam);
  }
};

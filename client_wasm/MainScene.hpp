#pragma once
#include <cmath>
#include <string>
#include "GameState.hpp"
#include "asset_preload.hpp"
#include "Scene.hpp"
#include "camera_nav.hpp"
#include "pbr_light_ids.hpp"
#include "raylib.h"
#include "ws_init.hpp"
Vector3 camPos = {0.0f, 5.0f, 0.0f};
float yaw = PI;
float pitch = 0.0f;

class MainScene : public Scene {
 private:
  Texture2D texture;
  Model map;
  Model p_map;
  Model freddy;
  Vector3 freddyInitialPos;
  Camera& camera;
  Shader map_shader;
  CameraNavState camera_nav_;
  bool is_freeroam = false;
  static constexpr float kFreeCamMoveSpeed = 12.0f;
  static constexpr int kMaxPbrLights = 32;
  static constexpr int kPbrLightPoint = 1;
  static constexpr int kPbrLightSpot = 2;
  // Player follow light (feeds PBR lights[]).
  static constexpr int kPlayerPbrLightIndex =
      static_cast<int>(PbrLightId::PlayerFollow);
  static constexpr float kPlayerLightIntensity = 2.5f;

  struct PbrLightGPU {
    int enabled = 1;
    int type = 1;
    Vector3 position{};
    Vector3 target{};
    float color[4]{1, 1, 1, 1};
    float intensity = 1.0f;
    float spot_inner_cos = 1.0f;
    float spot_outer_cos = 0.0f;
    int enabledLoc = -1;
    int typeLoc = -1;
    int positionLoc = -1;
    int targetLoc = -1;
    int colorLoc = -1;
    int intensityLoc = -1;
    int spotInnerCosLoc = -1;
    int spotOuterCosLoc = -1;
  };

  PbrLightGPU pbr_lights_[kMaxPbrLights];
  int pbr_light_count_ = 0;

  void push_pbr_light(const PbrLightGPU& L) {
    SetShaderValue(map_shader, L.enabledLoc, &L.enabled, SHADER_UNIFORM_INT);
    SetShaderValue(map_shader, L.typeLoc, &L.type, SHADER_UNIFORM_INT);
    float pos[3] = {L.position.x, L.position.y, L.position.z};
    float tgt[3] = {L.target.x, L.target.y, L.target.z};
    SetShaderValue(map_shader, L.positionLoc, pos, SHADER_UNIFORM_VEC3);
    SetShaderValue(map_shader, L.targetLoc, tgt, SHADER_UNIFORM_VEC3);
    SetShaderValue(map_shader, L.colorLoc, L.color, SHADER_UNIFORM_VEC4);
    SetShaderValue(map_shader, L.intensityLoc, &L.intensity,
                   SHADER_UNIFORM_FLOAT);
    SetShaderValue(map_shader, L.spotInnerCosLoc, &L.spot_inner_cos,
                   SHADER_UNIFORM_FLOAT);
    SetShaderValue(map_shader, L.spotOuterCosLoc, &L.spot_outer_cos,
                   SHADER_UNIFORM_FLOAT);
  }

  void init_pbr_light(Vector3 position, Vector3 target, Color col,
                      float intensity) {
    if (pbr_light_count_ >= kMaxPbrLights)
      return;
    int idx = pbr_light_count_++;
    PbrLightGPU& L = pbr_lights_[idx];
    L.enabled = 1;
    L.type = kPbrLightPoint;
    L.position = position;
    L.target = target;
    L.color[0] = col.r / 255.0f;
    L.color[1] = col.g / 255.0f;
    L.color[2] = col.b / 255.0f;
    L.color[3] = col.a / 255.0f;
    L.intensity = intensity;
    L.spot_inner_cos = 1.0f;
    L.spot_outer_cos = 0.0f;
    L.enabledLoc =
        GetShaderLocation(map_shader, TextFormat("lights[%i].enabled", idx));
    L.typeLoc =
        GetShaderLocation(map_shader, TextFormat("lights[%i].type", idx));
    L.positionLoc =
        GetShaderLocation(map_shader, TextFormat("lights[%i].position", idx));
    L.targetLoc =
        GetShaderLocation(map_shader, TextFormat("lights[%i].target", idx));
    L.colorLoc =
        GetShaderLocation(map_shader, TextFormat("lights[%i].color", idx));
    L.intensityLoc =
        GetShaderLocation(map_shader, TextFormat("lights[%i].intensity", idx));
    L.spotInnerCosLoc = GetShaderLocation(
        map_shader, TextFormat("lights[%i].spotInnerCos", idx));
    L.spotOuterCosLoc = GetShaderLocation(
        map_shader, TextFormat("lights[%i].spotOuterCos", idx));
    push_pbr_light(L);
  }

  // Half-angle cone in degrees: inner = full strength, outer = falloff to black.
  void init_pbr_spot_light(Vector3 position, Vector3 target, Color col,
                           float intensity, float inner_half_angle_deg,
                           float outer_half_angle_deg) {
    if (pbr_light_count_ >= kMaxPbrLights)
      return;
    int idx = pbr_light_count_++;
    PbrLightGPU& L = pbr_lights_[idx];
    L.enabled = 1;
    L.type = kPbrLightSpot;
    L.position = position;
    L.target = target;
    L.color[0] = col.r / 255.0f;
    L.color[1] = col.g / 255.0f;
    L.color[2] = col.b / 255.0f;
    L.color[3] = col.a / 255.0f;
    L.intensity = intensity;
    L.spot_inner_cos = cosf(inner_half_angle_deg * DEG2RAD);
    L.spot_outer_cos = cosf(outer_half_angle_deg * DEG2RAD);
    L.enabledLoc =
        GetShaderLocation(map_shader, TextFormat("lights[%i].enabled", idx));
    L.typeLoc =
        GetShaderLocation(map_shader, TextFormat("lights[%i].type", idx));
    L.positionLoc =
        GetShaderLocation(map_shader, TextFormat("lights[%i].position", idx));
    L.targetLoc =
        GetShaderLocation(map_shader, TextFormat("lights[%i].target", idx));
    L.colorLoc =
        GetShaderLocation(map_shader, TextFormat("lights[%i].color", idx));
    L.intensityLoc =
        GetShaderLocation(map_shader, TextFormat("lights[%i].intensity", idx));
    L.spotInnerCosLoc = GetShaderLocation(
        map_shader, TextFormat("lights[%i].spotInnerCos", idx));
    L.spotOuterCosLoc = GetShaderLocation(
        map_shader, TextFormat("lights[%i].spotOuterCos", idx));
    push_pbr_light(L);
  }

  void sync_pbr_shader_frame() {
    if (pbr_light_count_ > kPlayerPbrLightIndex) {
      PbrLightGPU& pl = pbr_lights_[kPlayerPbrLightIndex];
      pl.position = (Vector3){camPos.x, camPos.y - 0.35f, camPos.z};
      pl.target = pl.position;
    }

    SetShaderValue(map_shader, GetShaderLocation(map_shader, "numOfLights"),
                   &pbr_light_count_, SHADER_UNIFORM_INT);

    float vp[3] = {camera.position.x, camera.position.y, camera.position.z};
    if (map_shader.locs[SHADER_LOC_VECTOR_VIEW] != -1) {
      SetShaderValue(map_shader, map_shader.locs[SHADER_LOC_VECTOR_VIEW], vp,
                     SHADER_UNIFORM_VEC3);
    }
    for (int i = 0; i < pbr_light_count_; i++)
      push_pbr_light(pbr_lights_[i]);
  }

  void assign_pbr_to_model(Model& m) {
    for (int i = 0; i < m.materialCount; i++) {
      m.materials[i].shader = map_shader;
    }
  }

  void update_camera() {
    float sensitivity = 0.005f;
    int mouseDeltaX = -GetMouseDelta().x;
    int mouseDeltaY = -GetMouseDelta().y;

    yaw += mouseDeltaX * sensitivity;

    float radius = 1.0f;
    this->camera.position = camPos;

    if (is_freeroam) {
      pitch += mouseDeltaY * sensitivity;
      constexpr float kMaxPitch = 1.4f;
      if (pitch > kMaxPitch)
        pitch = kMaxPitch;
      if (pitch < -kMaxPitch)
        pitch = -kMaxPitch;

      float cosPitch = cosf(pitch);
      Vector3 forward = {-sinf(yaw) * cosPitch, sinf(pitch),
                         -cosf(yaw) * cosPitch};
      this->camera.target.x = camPos.x + forward.x * radius;
      this->camera.target.y = camPos.y + forward.y * radius;
      this->camera.target.z = camPos.z + forward.z * radius;
    } else {
      constexpr float kMaxYaw = 1.0f;
      if (yaw > kMaxYaw)
        yaw = kMaxYaw;
      if (yaw < -kMaxYaw)
        yaw = -kMaxYaw;

      this->camera.target.x = camPos.x - sinf(yaw) * radius;
      this->camera.target.y = camPos.y;
      this->camera.target.z = camPos.z - cosf(yaw) * radius;
    }

    this->camera.up = {0.0f, 12.0f, 0.0f};
    this->camera.fovy = 45.0f;
  }

  void apply_free_cam_move() {
    float dt = GetFrameTime();
    float fx = -sinf(yaw);
    float fz = -cosf(yaw);
    float rx = -cosf(yaw);
    float rz = sinf(yaw);
    float s = kFreeCamMoveSpeed * dt;
    if (IsKeyDown(KEY_W)) {
      camPos.x += fx * s;
      camPos.z += fz * s;
    }
    if (IsKeyDown(KEY_S)) {
      camPos.x -= fx * s;
      camPos.z -= fz * s;
    }
    if (IsKeyDown(KEY_D)) {
      camPos.x -= rx * s;
      camPos.z -= rz * s;
    }
    if (IsKeyDown(KEY_A)) {
      camPos.x += rx * s;
      camPos.z += rz * s;
    }
    if (is_freeroam) {
      if (IsKeyDown(KEY_SPACE)) {
        camPos.y += .01;
      }
      if (IsKeyDown(KEY_LEFT_CONTROL)) {
        camPos.y -= .01;
      }
    }
  }

 public:
  MainScene(Camera& cam) : Scene(cam), camera(cam) {
    SetMouseCursor(MOUSE_CURSOR_DEFAULT);

    static constexpr const char* kFreddyPath =
        "assets/freddy_fazbear_from_fnaf_help_wanted.glb";
    static constexpr const char* kMap1Path = "assets/fnaf_1_hw_map.glb";
    static constexpr const char* kMap2Path = "assets/fnaf_2_hw_map_updated.glb";
    static constexpr const char* kPbrVsPath = "assets/shaders/glsl100/pbr.vs";
    static constexpr const char* kPbrFsPath = "assets/shaders/glsl100/pbr.fs";

    SceneAssetPreloader preloader;
    preloader.PreloadAll({kFreddyPath, kMap1Path, kMap2Path},
                         {kPbrVsPath, kPbrFsPath});
    preloader.BeginServe();

    this->freddy = LoadModel(kFreddyPath);
    this->map = LoadModel(kMap1Path);
    this->p_map = LoadModel(kMap2Path);
    for (int i = 0; i < this->map.materialCount; i++) {
      if (this->map.materials[i].maps[MATERIAL_MAP_DIFFUSE].texture.id == 0) {
        printf("Missing texture on material %d\n", i);
      }
    }

    float distanceInFront = 6.0f;

    this->freddyInitialPos.x = camPos.x + sinf(yaw) * distanceInFront;
    this->freddyInitialPos.y =
        camPos.y - 2.0f;  // Lowered slightly so he isn't eye-level
    this->freddyInitialPos.z = camPos.z + cosf(yaw) * distanceInFront;

    this->camera.position = camPos;
    this->camera.target = this->freddyInitialPos;

    this->map_shader = LoadShader(kPbrVsPath, kPbrFsPath);

    preloader.EndServe();

    map_shader.locs[SHADER_LOC_MAP_ALBEDO] =
        GetShaderLocation(map_shader, "albedoMap");
    map_shader.locs[SHADER_LOC_MAP_METALNESS] =
        GetShaderLocation(map_shader, "mraMap");
    map_shader.locs[SHADER_LOC_MAP_NORMAL] =
        GetShaderLocation(map_shader, "normalMap");
    map_shader.locs[SHADER_LOC_MAP_EMISSION] =
        GetShaderLocation(map_shader, "emissiveMap");
    map_shader.locs[SHADER_LOC_COLOR_DIFFUSE] =
        GetShaderLocation(map_shader, "albedoColor");
    map_shader.locs[SHADER_LOC_VECTOR_VIEW] =
        GetShaderLocation(map_shader, "viewPos");

    // numOfLights is set in sync_pbr_shader_frame() to match pbr_light_count_.
    // Adjust rgb & strength for light
    Vector3 ambient_rgb = {0.0f, 0.0f, 0.0f};
    float ambient_strength = -0.5f;
    SetShaderValue(map_shader, GetShaderLocation(map_shader, "ambientColor"),
                   &ambient_rgb, SHADER_UNIFORM_VEC3);
    SetShaderValue(map_shader, GetShaderLocation(map_shader, "ambient"),
                   &ambient_strength, SHADER_UNIFORM_FLOAT);

    Vector2 tiling = {1.0f, 1.0f};
    Vector2 offset = {0.0f, 0.0f};
    SetShaderValue(map_shader, GetShaderLocation(map_shader, "tiling"), &tiling,
                   SHADER_UNIFORM_VEC2);
    SetShaderValue(map_shader, GetShaderLocation(map_shader, "offset"), &offset,
                   SHADER_UNIFORM_VEC2);

    float normal_value = 1.0f;
    float metallic_value = 0.0f;
    float roughness_value = 0.55f;
    float ao_value = 1.0f;
    float emissive_power = 0.0f;
    SetShaderValue(map_shader, GetShaderLocation(map_shader, "normalValue"),
                   &normal_value, SHADER_UNIFORM_FLOAT);
    SetShaderValue(map_shader, GetShaderLocation(map_shader, "metallicValue"),
                   &metallic_value, SHADER_UNIFORM_FLOAT);
    SetShaderValue(map_shader, GetShaderLocation(map_shader, "roughnessValue"),
                   &roughness_value, SHADER_UNIFORM_FLOAT);
    SetShaderValue(map_shader, GetShaderLocation(map_shader, "aoValue"),
                   &ao_value, SHADER_UNIFORM_FLOAT);
    SetShaderValue(map_shader, GetShaderLocation(map_shader, "emissivePower"),
                   &emissive_power, SHADER_UNIFORM_FLOAT);
    Vector4 emissive_color = ColorNormalize(CLITERAL(Color){0, 0, 0, 255});
    SetShaderValue(map_shader, GetShaderLocation(map_shader, "emissiveColor"),
                   &emissive_color, SHADER_UNIFORM_VEC4);

    int use_albedo = 1;
    int use_normal = 0;
    int use_mra = 0;
    int use_emissive = 0;
    SetShaderValue(map_shader, GetShaderLocation(map_shader, "useTexAlbedo"),
                   &use_albedo, SHADER_UNIFORM_INT);
    SetShaderValue(map_shader, GetShaderLocation(map_shader, "useTexNormal"),
                   &use_normal, SHADER_UNIFORM_INT);
    SetShaderValue(map_shader, GetShaderLocation(map_shader, "useTexMRA"),
                   &use_mra, SHADER_UNIFORM_INT);
    SetShaderValue(map_shader, GetShaderLocation(map_shader, "useTexEmissive"),
                   &use_emissive, SHADER_UNIFORM_INT);
    // PbrLightId order: PlayerFollow, FirstFloor*, SecondFloor*, HallSpot,
    // then kInitSpotOrder spotlights (1F/2F feeds including party rooms).
    init_pbr_light({0.0f, 5.0f, 0.0f}, {0.0f, 5.0f, 0.0f}, SKYBLUE,
                   kPlayerLightIntensity);
    init_pbr_light({1.5f, 6.0f, -3.5f}, {0.0f, 0.0f, 0.0f}, ORANGE, 1.0f);
    init_pbr_light({-0.5f, 6.0f, -2.0f}, {0.0f, 6.0f, 0.0f}, BLUE, 1.0f);
    init_pbr_light({-2.0f, 55.5f, -4.0f}, {-0.5f, 52.5f, -4.0f}, PURPLE, 1.3f);
    init_pbr_light({2.0f, 55.5f, -4.0f}, {-0.5f, 52.5f, -3.0f}, BLUE, 1.3f);
    init_pbr_light({0.0f, 53.5f, -4.0f}, {0.0f, 50.5f, -4.0f}, ORANGE, 1.0f);

    // PbrLightId::SecondFloorHallSpot — spotlight toward far hall
    init_pbr_spot_light({-0.0f, 53.5f, -7.5f}, {-0.0f, 52.5f, -32.0f}, WHITE,
                        28.0f, 30.0f, 32.0f);

    for (int i = 0; i < CameraMaps::kInitSpotOrderCount; i++) {
      const SecurityCamera& sc = CameraMaps::kInitSpotOrder[i];
      init_pbr_spot_light(sc.light_pos, sc.light_target, WHITE,
                          sc.spot_intensity, sc.spot_inner_deg,
                          sc.spot_outer_deg);
    }

    // PbrLightId::FirstFloorHallKey* — A/D only; original 1F hall positions (pre-offset).
    init_pbr_spot_light({-3.8f, 7.5f, 2.0f}, {-4.0f, 4.0f, -4.0f}, WHITE,
                        28.0f, 30.0f, 32.0f);
    init_pbr_spot_light({3.8f, 7.5f, 2.0f}, {4.0f, 4.0f, -4.0f}, WHITE, 28.0f,
                        30.0f, 32.0f);
    set_pbr_light_enabled(PbrLightId::FirstFloorHallKeyLHS, false);
    set_pbr_light_enabled(PbrLightId::FirstFloorHallKeyRHS, false);

    assign_pbr_to_model(this->map);
    assign_pbr_to_model(this->p_map);
    assign_pbr_to_model(this->freddy);
    this->camera.fovy = 50.0f;
    this->camera.target.x = -1.0f;
    this->camera.target.z = 10.0f;
  }

  bool set_pbr_light_enabled(int index, bool enabled) {
    if (index < 0 || index >= pbr_light_count_)
      return false;
    pbr_lights_[index].enabled = enabled ? 1 : 0;
    return true;
  }

  bool pbr_light_enabled(PbrLightId id) const {
    return pbr_light_enabled(static_cast<int>(id));
  }

  bool pbr_light_enabled(int index) const {
    if (index < 0 || index >= pbr_light_count_)
      return false;
    return pbr_lights_[index].enabled != 0;
  }

  int pbr_light_count() const { return pbr_light_count_; }

  bool set_pbr_light_enabled(PbrLightId id, bool enabled) {
    return set_pbr_light_enabled(static_cast<int>(id), enabled);
  }

  void update(Scene*& curr_scene, GameState& state,
              EMSCRIPTEN_WEBSOCKET_T& socket) override {
    if (state.check_camera_restore_feed) {
      camera_nav_.active_feed = state.check_camera_saved_active_feed;
      state.check_camera_restore_feed = false;
    }

    if (IsKeyPressed(KEY_B))
      camera_nav_.TogglePanel(state.is_player_one);

    if (camera_nav_.active_feed >= 0 && IsKeyPressed(KEY_C) &&
        !state.check_camera_in_flight) {
      int n_cams = 0;
      const SecurityCamera* cams =
          CameraMaps::MapForPlayer(state.is_player_one, &n_cams);
      int idx = camera_nav_.active_feed;
      if (idx >= 0 && idx < n_cams && cams[idx].sim_room_alias != nullptr) {
        state.check_camera_entities.clear();
        state.check_camera_status = "Checking cameras…";
        state.check_camera_saved_active_feed = camera_nav_.active_feed;
        camera_nav_.active_feed = -1;
        state.check_camera_in_flight = true;
        ws::send_check_camera(socket, cams[idx].sim_room_alias);
      }
    }

    if (camera_nav_.active_feed >= 0) {
      int n = 0;
      const SecurityCamera* map =
          CameraMaps::MapForPlayer(state.is_player_one, &n);
      if (camera_nav_.active_feed >= n)
        camera_nav_.active_feed = n > 0 ? n - 1 : 0;
      const PbrLightId active_light = map[camera_nav_.active_feed].spot_light;
      for (int i = 0; i < CameraMaps::kInitSpotOrderCount; i++) {
        const SecurityCamera& sc = CameraMaps::kInitSpotOrder[i];
        set_pbr_light_enabled(sc.spot_light, sc.spot_light == active_light);
      }
      set_pbr_light_enabled(PbrLightId::FirstFloorHallKeyLHS, false);
      set_pbr_light_enabled(PbrLightId::FirstFloorHallKeyRHS, false);
    } else {
      if (state.is_player_one) {
        for (int i = 0; i < CameraMaps::kInitSpotOrderCount; i++) {
          const SecurityCamera& sc = CameraMaps::kInitSpotOrder[i];
          set_pbr_light_enabled(sc.spot_light, false);
        }
        set_pbr_light_enabled(PbrLightId::FirstFloorHallKeyLHS,
                              IsKeyDown(KEY_A));
        set_pbr_light_enabled(PbrLightId::FirstFloorHallKeyRHS,
                              IsKeyDown(KEY_D));
      } else {
        set_pbr_light_enabled(PbrLightId::FirstFloorHallKeyLHS, false);
        set_pbr_light_enabled(PbrLightId::FirstFloorHallKeyRHS, false);
        set_pbr_light_enabled(PbrLightId::SecondFloorHallSpot,
                              IsKeyDown(KEY_W));
        set_pbr_light_enabled(PbrLightId::SecondFloorLHS, IsKeyDown(KEY_A));
        set_pbr_light_enabled(PbrLightId::SecondFloorRHS, IsKeyDown(KEY_D));
      }
    }

    if (!state.is_player_one && !is_freeroam) {
      camPos = {0.0f, 52.5f, 0.0f};
    }

    if (is_freeroam) {
      apply_free_cam_move();
    }

    ClearBackground(BLACK);

    if (camera_nav_.active_feed >= 0) {
      int n = 0;
      const SecurityCamera* map =
          CameraMaps::MapForPlayer(state.is_player_one, &n);
      if (camera_nav_.active_feed < n)
        CameraMaps::ApplySecurityCameraView(
            map[camera_nav_.active_feed], static_cast<Camera3D&>(this->camera));
    } else {
      this->update_camera();
    }
    sync_pbr_shader_frame();
    BeginDrawing();
    // DrawText("Socket", 0, 0, 24, BLACK);
    // DrawTexture(this->texture, 0, 0, WHITE);
    // DrawSphere({-1.0f, 54.0f, -11.5f}, 1.0f, WHITE);
    BeginMode3D(this->camera);
    Vector3 rotationAxis = {0.0f, 1.0f,
                            0.0f};  // Rotate around the Y-axis (spinning)
    float rotationAngle = 180.0f;   // Angle in degrees

    DrawModelEx(this->freddy, this->freddyInitialPos, rotationAxis,
                rotationAngle, (Vector3){1.0f, 1.0f, 1.0f}, WHITE);
    // DrawModelEx(this->map, (Vector3){0.0f, 0.5f, 2.5f}, rotationAxis, rotationAngle, (Vector3){100.0f, 100.00f, 100.0f}, WHITE);
    DrawModel(this->map, (Vector3){0.0f, 2.5f, -2.5f}, 1.0f, WHITE);
    DrawModel(this->p_map, (Vector3){1.0f, 50.0f, -13.0f}, 1.0f, WHITE);

    // Green sphere at player light position (matches lights[3] in sync_pbr_shader_frame)
    Vector3 player_marker = {1.0f, 53.0f, 0.0f};
    if (is_freeroam) {
      player_marker = {camPos.x, camPos.y - 0.35f, camPos.z};
    }
    EndMode3D();
    camera_nav_.DrawPanel(state.is_player_one);
    std::string x_text = std::to_string(this->camera.position.x);
    std::string y_text = std::to_string(this->camera.position.y);
    std::string z_text = std::to_string(this->camera.position.z);
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

    EndDrawing();
  };
  void listen() override {
    if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) &&
        IsKeyPressed(KEY_Q)) {
      if (is_freeroam) {
        camPos.x += sinf(yaw) * 2.0f;
        camPos.z += cosf(yaw) * 2.0f;
        pitch = 0.0f;
      }
      is_freeroam = !is_freeroam;
    }
  }
};
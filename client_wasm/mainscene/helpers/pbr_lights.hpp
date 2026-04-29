#pragma once
#include "../../camera_nav.hpp"
#include "../../pbr_light_ids.hpp"
#include "raylib.h"
#include <cmath>

namespace mainscene {

inline constexpr int kMaxPbrLights = 32;
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

namespace detail {
inline void push_pbr_light(Shader& map_shader, const PbrLightGPU& L) {
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
}  // namespace detail

inline void init_pbr_light(Shader& map_shader, PbrLightGPU* lights, int& count,
                           Vector3 position, Vector3 target, Color col,
                           float intensity) {
  if (count >= kMaxPbrLights)
    return;
  int idx = count++;
  PbrLightGPU& L = lights[idx];
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
  detail::push_pbr_light(map_shader, L);
}

// Half-angle cone in degrees: inner = full strength, outer = falloff to black.
inline void init_pbr_spot_light(Shader& map_shader, PbrLightGPU* lights,
                                int& count, Vector3 position, Vector3 target,
                                Color col, float intensity,
                                float inner_half_angle_deg,
                                float outer_half_angle_deg) {
  if (count >= kMaxPbrLights)
    return;
  int idx = count++;
  PbrLightGPU& L = lights[idx];
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
  detail::push_pbr_light(map_shader, L);
}

inline void sync_pbr_shader_frame(Shader& map_shader, Camera& camera,
                                  PbrLightGPU* pbr_lights, int pbr_light_count,
                                  Vector3 camPos) {
  if (pbr_light_count > kPlayerPbrLightIndex) {
    PbrLightGPU& pl = pbr_lights[kPlayerPbrLightIndex];
    pl.position = (Vector3){camPos.x, camPos.y - 0.35f, camPos.z};
    pl.target = pl.position;
  }

  static int num_of_lights_loc = -1;
  if (num_of_lights_loc == -1) {
    num_of_lights_loc = GetShaderLocation(map_shader, "numOfLights");
  }
  SetShaderValue(map_shader, num_of_lights_loc, &pbr_light_count,
                 SHADER_UNIFORM_INT);

  float vp[3] = {camera.position.x, camera.position.y, camera.position.z};
  if (map_shader.locs[SHADER_LOC_VECTOR_VIEW] != -1) {
    SetShaderValue(map_shader, map_shader.locs[SHADER_LOC_VECTOR_VIEW], vp,
                   SHADER_UNIFORM_VEC3);
  }
  for (int i = 0; i < pbr_light_count; i++)
    detail::push_pbr_light(map_shader, pbr_lights[i]);
}

inline void assign_pbr_to_model(Shader& map_shader, Model& m) {
  for (int i = 0; i < m.materialCount; i++) {
    m.materials[i].shader = map_shader;
  }
}

inline void setup_pbr_shader_locs(Shader& map_shader) {
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
}

inline void setup_pbr_shader_uniform_defaults(Shader& map_shader) {
  // numOfLights is set in sync_pbr_shader_frame() to match pbr light count.
  Vector3 ambient_rgb = {0.0f, 0.0f, 0.0f};
  float ambient_strength = 1.0f;
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
}

inline bool set_pbr_light_enabled(PbrLightGPU* lights, int pbr_light_count,
                                  int index, bool enabled) {
  if (index < 0 || index >= pbr_light_count)
    return false;
  lights[index].enabled = enabled ? 1 : 0;
  return true;
}

inline bool pbr_light_enabled(const PbrLightGPU* lights, int pbr_light_count,
                              int index) {
  if (index < 0 || index >= pbr_light_count)
    return false;
  return lights[index].enabled != 0;
}

// PbrLightId order: PlayerFollow, FirstFloor*, SecondFloor*, HallSpot,
// then kInitSpotOrder spotlights (1F/2F feeds including party rooms).
inline void init_all_scene_pbr_lights(Shader& map_shader, PbrLightGPU* lights,
                                      int& count) {
  init_pbr_light(map_shader, lights, count, {0.0f, 5.0f, 0.0f},
                   {0.0f, 5.0f, 0.0f}, SKYBLUE, kPlayerLightIntensity);
  init_pbr_light(map_shader, lights, count, {1.5f, 6.0f, -3.5f},
                   {0.0f, 0.0f, 0.0f}, ORANGE, 1.0f);
  init_pbr_light(map_shader, lights, count, {-0.5f, 6.0f, -2.0f},
                   {0.0f, 6.0f, 0.0f}, BLUE, 1.0f);
  init_pbr_light(map_shader, lights, count, {-2.0f, 55.5f, -4.0f},
                   {-0.5f, 52.5f, -4.0f}, PURPLE, 1.3f);
  init_pbr_light(map_shader, lights, count, {2.0f, 55.5f, -4.0f},
                   {-0.5f, 52.5f, -3.0f}, BLUE, 1.3f);
  init_pbr_light(map_shader, lights, count, {0.0f, 53.5f, -4.0f},
                   {0.0f, 50.5f, -4.0f}, ORANGE, 1.0f);

  // PbrLightId::SecondFloorHallSpot — spotlight toward far hall
  init_pbr_spot_light(map_shader, lights, count, {-0.0f, 53.5f, -7.5f},
                      {-0.0f, 52.5f, -32.0f}, WHITE, 28.0f, 30.0f, 32.0f);

  for (int i = 0; i < CameraMaps::kInitSpotOrderCount; i++) {
    const SecurityCamera& sc = CameraMaps::kInitSpotOrder[i];
    init_pbr_spot_light(map_shader, lights, count, sc.light_pos, sc.light_target,
                        WHITE, sc.spot_intensity, sc.spot_inner_deg,
                        sc.spot_outer_deg);
  }

  // PbrLightId::FirstFloorHallKey* — A/D only; original 1F hall positions
  // (pre-offset).
  init_pbr_spot_light(map_shader, lights, count, {-3.8f, 7.5f, 2.0f},
                      {-4.0f, 4.0f, -4.0f}, WHITE, 28.0f, 30.0f, 32.0f);
  init_pbr_spot_light(map_shader, lights, count, {3.8f, 7.5f, 2.0f},
                      {4.0f, 4.0f, -4.0f}, WHITE, 28.0f, 30.0f, 32.0f);
  set_pbr_light_enabled(lights, count,
                        static_cast<int>(PbrLightId::FirstFloorHallKeyLHS),
                        false);
  set_pbr_light_enabled(lights, count,
                        static_cast<int>(PbrLightId::FirstFloorHallKeyRHS),
                        false);
}

}  // namespace mainscene

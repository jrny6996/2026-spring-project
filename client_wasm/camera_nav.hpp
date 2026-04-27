#pragma once

#include "pbr_light_ids.hpp"
#include "raylib.h"

// One security feed: spotlight, fixed 3D view, and label (map UI uses eye_pos).
// sim_room_alias matches server GameGraphNode.AliasName for this feed.
struct SecurityCamera {
  const char* label;
  const char* sim_room_alias;
  PbrLightId spot_light;
  Vector3 light_pos;
  Vector3 light_target;
  float spot_intensity;
  float spot_inner_deg;
  float spot_outer_deg;
  Vector3 eye_pos;
  Vector3 eye_target;
  float eye_fovy;
};

namespace CameraMaps {

inline constexpr SecurityCamera kPlayerOne[] = {
    {"1F LHS",
     "lhs_hall",
     PbrLightId::FirstFloorLHS,
     {-3.8f, 7.5f, 0.0f},
     {-4.0f, 4.0f, -6.0f},
     28.0f,
     30.0f,
     32.0f,
     {-3.8f, 7.5f, 0.0f},
     {-4.0f, 4.0f, -6.0f},
     55.0f},
    {"1F RHS",
     "rhs_hall",
     PbrLightId::FirstFloorRHS,
     {3.8f, 7.5f, 0.0f},
     {4.0f, 4.0f, -6.0f},
     28.0f,
     30.0f,
     32.0f,
     {3.8f, 7.5f, 0.0f},
     {4.0f, 4.0f, -6.0f},
     55.0f},
    {"LHS Closet",
     "lhs_closet",
     PbrLightId::FirstFloorLHSCloset,
     {-4.5f, 7.5f, 2.0f},
     {-4.0f, 4.0f, 4.5f},
     22.0f,
     24.0f,
     38.0f,
     {-4.5f, 7.5f, 2.0f},
     {-4.0f, 4.0f, 4.5f},
     52.0f},
    {"RHS Closet",
     "rhs_closet",
     PbrLightId::FirstFloorRHSCloset,
     {4.5f, 7.5f, 2.0f},
     {4.0f, 4.0f, 4.5f},
     22.0f,
     24.0f,
     38.0f,
     {4.5f, 7.5f, 2.0f},
     {4.0f, 4.0f, 4.5f},
     52.0f},
    {"1F Party 1",
     "party_room",
     PbrLightId::FirstFloorParty,
     {10.0f, 7.5f, -20.0f},
     {2.5f, 4.0f, -26.0f},
     28.0f,
     30.0f,
     32.0f,
     {10.0f, 7.5f, -20.0f},
     {2.5f, 4.0f, -26.0f},
     55.0f},
    {"Stage",
     "stage",
     PbrLightId::FirstFloorStage,
     {-4.5f, 7.0f, -35.0f},
     {0.7f, 7.0f, -43.5f},
     28.0f,
     26.0f,
     36.0f,
     {-4.5f, 7.0f, -35.0f},
     {0.7f, 7.0f, -43.5f},
     50.0f},
    {"Repair",
     "party_room",
     PbrLightId::FirstFloorParty3,
     {-18.1f, 8.5f, -29.5f},
     {-20.0f, 4.8f, -32.0f},
     24.0f,
     28.0f,
     34.0f,
     {-18.1f, 8.5f, -29.5f},
     {-20.0f, 4.8f, -32.0f},
     52.0f},
    {"Bathrooms",
     "party_room",
     PbrLightId::FirstFloorParty4,
     {12.5f, 7.5f, -24.5f},
     {16.5f, 4.0f, -30.0f},
     24.0f,
     28.0f,
     34.0f,
     {12.5f, 7.5f, -24.5f},
     {16.5f, 4.0f, -30.0f},
     52.0f},
};

inline constexpr SecurityCamera kPlayerTwo[] = {
    {"2F LHS",
     "lhs_party",
     PbrLightId::SecondFloorLHS,
     {-2.0f, 51.5f, -3.0f},
     {-10.0f, 50.5f, -3.0f},
     12.0f,
     30.0f,
     32.0f,
     {-2.0f, 51.5f, -3.0f},
     {-10.0f, 50.5f, -3.0f},
     55.0f},
    {"2F RHS",
     "rhs_hall",
     PbrLightId::SecondFloorRHS,
     {2.0f, 51.5f, -3.0f},
     {10.0f, 50.5f, -3.0f},
     12.0f,
     30.0f,
     32.0f,
     {2.0f, 51.5f, -3.0f},
     {10.0f, 50.5f, -3.0f},
     55.0f},
    {"2F LHS Party 2",
     "lhs_party",
     PbrLightId::SecondFloorParty2,
     {-4.5f, 55.5f, -10.0f},
     {-8.0f, 53.5f, -10.0f},
     24.0f,
     28.0f,
     34.0f,
     {-4.5f, 55.5f, -10.0f},
     {-8.0f, 53.5f, -10.0f},
     55.0f},
    {"2F LHS Party 1",
     "lhs_repair",
     PbrLightId::SecondFloorParty1,
     {-6.5f, 55.5f, -28.0f},
     {-8.0f, 53.5f, -24.0f},
     24.0f,
     28.0f,
     34.0f,
     {-6.5f, 55.5f, -28.0f},
     {-8.0f, 53.5f, -24.0f},
     55.0f},
    {"2F RHS Party 2",
     "rhs_party",
     PbrLightId::SecondFloorParty2RHS,
     {4.5f, 55.5f, -10.0f},
     {8.0f, 53.5f, -10.0f},
     24.0f,
     28.0f,
     34.0f,
     {4.5f, 55.5f, -10.0f},
     {8.0f, 53.5f, -10.0f},
     55.0f},
    {"2F RHS Party 1",
     "rhs_party",
     PbrLightId::SecondFloorParty1RHS,
     {6.5f, 55.5f, -28.0f},
     {8.0f, 53.5f, -24.0f},
     24.0f,
     28.0f,
     34.0f,
     {6.5f, 55.5f, -28.0f},
     {8.0f, 53.5f, -24.0f},
     55.0f},
    {"2F Circus",
     "middle_hall_two",
     PbrLightId::SecondFloorCircus,
     {22.0f, 53.0f, -22.0f},
     {32.0f, 53.0f, -22.5f},
     24.0f,
     28.0f,
     34.0f,
     {22.0f, 53.0f, -22.0f},
     {32.0f, 53.0f, -22.5f},
     55.0f},
    {"2F Stage",
     "toy_stage",
     PbrLightId::SecondFloorStage,
     {32.0f, 53.0f, -36.0f},
     {36.0f, 53.0f, -38.5f},
     24.0f,
     28.0f,
     34.0f,
     {32.0f, 53.0f, -36.0f},
     {36.0f, 53.0f, -38.5f},
     55.0f},
};

inline constexpr int kPlayerOneCount =
    sizeof(kPlayerOne) / sizeof(kPlayerOne[0]);
inline constexpr int kPlayerTwoCount =
    sizeof(kPlayerTwo) / sizeof(kPlayerTwo[0]);

/// Pixel size of the floor-plan area (identical for P1 and P2 panels).
inline constexpr int kSurveillanceMapW = 268;
inline constexpr int kSurveillanceMapH = 268;
/// Extra margin around min/max XZ from all cameras (fraction of each axis span).
inline constexpr float kSurveillanceMapPadFrac = 0.08f;
inline constexpr float kSurveillanceCamHitRadiusPx = 18.0f;
inline constexpr float kSurveillanceCamDotRadiusPx = 5.0f;
inline constexpr int kSurveillanceLabelFont = 14;

struct SurveillanceMapBounds {
  float min_x = 0.0f;
  float max_x = 1.0f;
  float min_z = 0.0f;
  float max_z = 1.0f;
};

inline SurveillanceMapBounds ComputeSurveillanceMapBounds() {
  SurveillanceMapBounds b;
  b.min_x = b.min_z = 1.0e12f;
  b.max_x = b.max_z = -1.0e12f;

  auto expand = [&](Vector3 p) {
    if (p.x < b.min_x)
      b.min_x = p.x;
    if (p.x > b.max_x)
      b.max_x = p.x;
    if (p.z < b.min_z)
      b.min_z = p.z;
    if (p.z > b.max_z)
      b.max_z = p.z;
  };

  for (int i = 0; i < kPlayerOneCount; i++)
    expand(kPlayerOne[i].eye_pos);
  for (int i = 0; i < kPlayerTwoCount; i++)
    expand(kPlayerTwo[i].eye_pos);

  float dx = b.max_x - b.min_x;
  float dz = b.max_z - b.min_z;
  if (dx < 1.0e-3f)
    dx = 1.0f;
  if (dz < 1.0e-3f)
    dz = 1.0f;

  float pad_x = dx * kSurveillanceMapPadFrac + 0.35f;
  float pad_z = dz * kSurveillanceMapPadFrac + 0.35f;
  b.min_x -= pad_x;
  b.max_x += pad_x;
  b.min_z -= pad_z;
  b.max_z += pad_z;
  return b;
}

/// Position inside the map rectangle: (0,0) top-left, +Y downward (screen space).
/// Z increases downward on screen (matches negative-Z-forward map layout).
inline Vector2 EyeToSurveillanceMapPx(Vector3 eye, const SurveillanceMapBounds& b) {
  float dx = b.max_x - b.min_x;
  float dz = b.max_z - b.min_z;
  if (dx < 1.0e-6f)
    dx = 1.0e-6f;
  if (dz < 1.0e-6f)
    dz = 1.0e-6f;
  float nx = (eye.x - b.min_x) / dx;
  float nz = (eye.z - b.min_z) / dz;
  return {nx * (float)kSurveillanceMapW, nz * (float)kSurveillanceMapH};
}

inline const SecurityCamera* MapForPlayer(bool is_player_one, int* count) {
  if (is_player_one) {
    *count = kPlayerOneCount;
    return kPlayerOne;
  }
  *count = kPlayerTwoCount;
  return kPlayerTwo;
}

// kInitSpotOrder spotlights: PbrLightId indices 7..22; hall A/D spots are 23..24
// (see mainscene::init_all_scene_pbr_lights).
inline constexpr SecurityCamera kInitSpotOrder[] = {
    kPlayerOne[0], kPlayerOne[1], kPlayerTwo[0], kPlayerTwo[1], kPlayerOne[4],
    kPlayerOne[5], kPlayerOne[6], kPlayerOne[7], kPlayerOne[2], kPlayerOne[3],
    kPlayerTwo[2], kPlayerTwo[3], kPlayerTwo[4], kPlayerTwo[5], kPlayerTwo[6],
    kPlayerTwo[7],
};
inline constexpr int kInitSpotOrderCount =
    sizeof(kInitSpotOrder) / sizeof(kInitSpotOrder[0]);

inline void ApplySecurityCameraView(const SecurityCamera& c, Camera3D& cam) {
  cam.position = c.eye_pos;
  cam.target = c.eye_target;
  cam.up = {0.0f, 1.0f, 0.0f};
  cam.fovy = c.eye_fovy;
  cam.projection = CAMERA_PERSPECTIVE;
}

}  // namespace CameraMaps

struct CameraNavState {
  bool panel_open = false;
  /// -1 = gameplay; else index into the active player's CameraMaps table.
  int active_feed = -1;
  int last_feed_p1 = 0;
  int last_feed_p2 = 0;

  int& LastFeedSlot(bool is_player_one) {
    return is_player_one ? last_feed_p1 : last_feed_p2;
  }

  void TogglePanel(bool is_player_one) {
    int n = 0;
    CameraMaps::MapForPlayer(is_player_one, &n);

    if (panel_open) {
      int& last = LastFeedSlot(is_player_one);
      if (active_feed >= 0)
        last = active_feed;
      if (last < 0 || last >= n)
        last = 0;
      panel_open = false;
      active_feed = -1;
      return;
    }

    panel_open = true;
    int& last = LastFeedSlot(is_player_one);
    if (last < 0 || last >= n)
      last = 0;
    active_feed = last;
  }

  void DrawPanel(bool is_player_one) {
    if (!panel_open)
      return;

    int n = 0;
    const SecurityCamera* cams = CameraMaps::MapForPlayer(is_player_one, &n);

    static const CameraMaps::SurveillanceMapBounds kMapBounds =
        CameraMaps::ComputeSurveillanceMapBounds();

    const int sw = GetScreenWidth();
    const int map_w = CameraMaps::kSurveillanceMapW;
    const int map_h = CameraMaps::kSurveillanceMapH;
    const int margin = 24;
    const int edge_pad = 12;
    const int title_h = 28;
    const int outer_w = map_w + 2 * edge_pad;
    const int outer_h = title_h + map_h + 2 * edge_pad + 8;

    const int outer_left =
        is_player_one ? margin : (sw - margin - outer_w);
    const int base_y = 72;
    const int map_x = outer_left + edge_pad;
    const int map_y = base_y + title_h;

    DrawRectangle(outer_left - edge_pad, base_y - edge_pad - 8,
                  outer_w + 2 * edge_pad, outer_h + 16, Fade(BLACK, 0.88f));
    DrawRectangleLines(outer_left - edge_pad, base_y - edge_pad - 8,
                       outer_w + 2 * edge_pad, outer_h + 16,
                       Fade(WHITE, 0.35f));

    const char* title = is_player_one ? "First Floor" : "Second Floor";
    DrawText(title, outer_left, base_y - 26, 18, LIGHTGRAY);

    DrawRectangle(map_x, map_y, map_w, map_h,
                  Fade(CLITERAL(Color){25, 28, 35, 255}, 0.95f));
    DrawRectangleLines(map_x, map_y, map_w, map_h, Fade(WHITE, 0.25f));

    Vector2 m = GetMousePosition();

    for (int i = 0; i < n; i++) {
      Vector2 local = CameraMaps::EyeToSurveillanceMapPx(cams[i].eye_pos,
                                                         kMapBounds);
      Vector2 center = {map_x + local.x, map_y + local.y};
      bool hot =
          CheckCollisionPointCircle(m, center, CameraMaps::kSurveillanceCamHitRadiusPx);
      Color bg = MAROON;
      if (active_feed != i)
        bg = hot ? Fade(DARKGRAY, 0.95f) : Fade(GRAY, 0.75f);

      if (hot || active_feed == i)
        DrawCircleLines((int)center.x, (int)center.y,
                        (int)CameraMaps::kSurveillanceCamHitRadiusPx,
                        Fade(WHITE, 0.45f));
      DrawCircleV(center, CameraMaps::kSurveillanceCamDotRadiusPx, bg);
      DrawCircleLines((int)center.x, (int)center.y,
                      (int)CameraMaps::kSurveillanceCamDotRadiusPx,
                      Fade(WHITE, 0.7f));

      int lw = MeasureText(cams[i].label, CameraMaps::kSurveillanceLabelFont);
      float tx = center.x - (float)lw * 0.5f;
      float ty = center.y + CameraMaps::kSurveillanceCamHitRadiusPx + 2.0f;
      if (ty + (float)CameraMaps::kSurveillanceLabelFont > map_y + map_h)
        ty = center.y - CameraMaps::kSurveillanceCamHitRadiusPx -
             (float)CameraMaps::kSurveillanceLabelFont - 2.0f;
      DrawText(cams[i].label, (int)tx, (int)ty,
               CameraMaps::kSurveillanceLabelFont, RAYWHITE);

      if (hot && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        active_feed = i;
    }
  }
};

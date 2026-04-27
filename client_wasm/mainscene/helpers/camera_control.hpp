#pragma once
#include "raylib.h"
#include <cmath>

namespace mainscene {

inline constexpr float kFreeCamMoveSpeed = 12.0f;

inline void update_office_camera(Camera& camera, Vector3& camPos, float& yaw,
                                 float& pitch, bool is_freeroam) {
  float sensitivity = 0.005f;
  int mouseDeltaX = -GetMouseDelta().x;
  int mouseDeltaY = -GetMouseDelta().y;

  yaw += mouseDeltaX * sensitivity;

  float radius = 1.0f;
  camera.position = camPos;

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
    camera.target.x = camPos.x + forward.x * radius;
    camera.target.y = camPos.y + forward.y * radius;
    camera.target.z = camPos.z + forward.z * radius;
  } else {
    constexpr float kMaxYaw = 1.0f;
    if (yaw > kMaxYaw)
      yaw = kMaxYaw;
    if (yaw < -kMaxYaw)
      yaw = -kMaxYaw;

    camera.target.x = camPos.x - sinf(yaw) * radius;
    camera.target.y = camPos.y;
    camera.target.z = camPos.z - cosf(yaw) * radius;
  }

  camera.up = {0.0f, 12.0f, 0.0f};
  camera.fovy = 45.0f;
}

inline void apply_free_cam_move(Vector3& camPos, float yaw, bool is_freeroam) {
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

inline void on_freeroam_shortcut(Vector3& camPos, float yaw, float& pitch,
                                 bool& is_freeroam) {
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

}  // namespace mainscene

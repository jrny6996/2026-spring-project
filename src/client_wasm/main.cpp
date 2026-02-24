
#include "Scene.hpp"
#include "menu.hpp"
#include "raylib.h"

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------

int screenWidth = 1280;
int screenHeight = 720;
Camera3D camera = {0};
Scene* curr_scene = nullptr;
auto bg_color = CLITERAL(Color){0, 0, 10, 255};
void UpdateDrawFrame(void);
int main() {

  // BoundingBox bounds = GetMeshBoundingBox(model.meshes[0]);
  camera.position = (Vector3){10.0f, 10.0f, 10.0f};  // Camera position
  camera.target = (Vector3){0.0f, 0.0f, 0.0f};       // Camera looking at point
  camera.up = (Vector3){0.0f, 1.0f,
                        0.0f};  // Camera up vector (rotation towards target)
  camera.fovy = 45.0f;          // Camera field-of-view Y
  camera.projection = CAMERA_PERSPECTIVE;

  auto menu = init_menu(camera);
  curr_scene = &menu;
  // DisableCursor();
  InitWindow(screenWidth, screenHeight, "Spring 26 Independant Study");

#if defined(PLATFORM_WEB)
  emscripten_set_main_loop(UpdateDrawFrame, 0, 1);
#else
  SetTargetFPS(60);
  while (!WindowShouldClose()) {
    UpdateDrawFrame();
  }
#endif

  CloseWindow();
  return 0;
}

void UpdateDrawFrame(void) {
  UpdateCamera(&camera, CAMERA_FREE);
  BeginDrawing();

  ClearBackground(bg_color);
  curr_scene->listen();
  curr_scene->update(curr_scene);

  EndDrawing();
}
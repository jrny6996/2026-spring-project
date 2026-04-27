#include "Scene.hpp"
#include "menu.hpp"
#include "raylib.h"

#include <emscripten/emscripten.h>
#include <emscripten/websocket.h>
#include <cstdio>
#include <optional>
#include <string>
#include "GameState.hpp"
#include "ws_init.hpp"

// Globals
int screenWidth = GetMonitorWidth(0);
int screenHeight =GetMonitorHeight(0);
Camera3D camera = {0};
Scene* curr_scene = init_menu(camera);
Color bg_color = {0, 0, 10, 255};
GameState game_state;
WsContext ctx;
EMSCRIPTEN_WEBSOCKET_T socket = ws::init("ws://localhost:6789/echo", &ctx);

// Main loop function (required by emscripten)
void MainLoop(void) {

  curr_scene->listen();
  curr_scene->update(curr_scene, game_state, socket);
}

int main(void) {
  ctx.state = &game_state;

  InitWindow(screenWidth, screenHeight, "raylib [core] example - world screen");

  camera.position = (Vector3){10.0f, 10.0f, 10.0f};
  camera.target = (Vector3){0.0f, 0.0f, 0.0f};
  camera.up = (Vector3){0.0f, 1.0f, 0.0f};
  camera.fovy = 145.0f;
  camera.projection = CAMERA_PERSPECTIVE;

  // DisableCursor();
  // SetTargetFPS(60);

  if (socket <= 0) {
    printf("Failed to connect\n");
    return 1;
  }
  emscripten_set_main_loop(MainLoop, 0, 1);
  emscripten_set_main_loop_timing(EM_TIMING_RAF, 0);

  return 0;
}
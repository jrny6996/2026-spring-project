#include "Scene.hpp"
#include "menu.hpp"
#include "raylib.h"

#include <emscripten/emscripten.h>
#include <emscripten/websocket.h>
#include <cstdio>
#include <cstdlib>
#include <optional>
#include <string>
#include "GameState.hpp"
#include "ws_init.hpp"

EM_JS(char*, wasm_alloc_ws_url, (), {
  var loc = (typeof window !== "undefined" && window.location)
                ? window.location.href
                : "http://127.0.0.1:6789/";
  var u = new URL(loc);
  var n = u.searchParams.get("night");
  if (!n || !/^[1-7]$/.test(String(n)))
    n = "1";
  var pr = u.protocol === "https:" ? "wss:" : "ws:";
  // Page from emrun (e.g. npm dev on :6931) has no /echo; Go game server is on :6789.
  var wsHost = u.host || "127.0.0.1:6789";
  if (u.protocol === "http:" && u.port === "6931")
    wsHost = (u.hostname || "127.0.0.1") + ":6789";
  var s = pr + "//" + wsHost + "/echo?night=" + encodeURIComponent(n);
  var tok = (typeof localStorage !== "undefined") ? localStorage.getItem("jwt") : "";
  if (tok && tok.length > 0)
    s += "&token=" + encodeURIComponent(tok);
  var len = lengthBytesUTF8(s) + 1;
  var p = _malloc(len);
  stringToUTF8(s, p, len);
  return p;
});

// #include <jet/live.hpp>
//   jet::Live live;

// Globals
// int screenWidth = GetMonitorWidth(0);
// int screenHeight =GetMonitorHeight(0);
int screenWidth = 1280;
int screenHeight = 720;
Camera3D camera = {0};
Scene* curr_scene = init_menu(camera);
Color bg_color = {0, 0, 10, 255};
GameState game_state;
WsContext ctx;
EMSCRIPTEN_WEBSOCKET_T socket = 0;

// Main loop function (required by emscripten)
void MainLoop(void) {

  curr_scene->listen();
  curr_scene->update(curr_scene, game_state, socket);
  // live.update();
}

int main(void) {
  ctx.state = &game_state;

  char* ws_url = wasm_alloc_ws_url();
  socket = ws::init(ws_url, &ctx);
  free(ws_url);

  InitWindow(screenWidth, screenHeight, "raylib [core] game - world screen");

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
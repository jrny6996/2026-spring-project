#pragma once
#include "GameState.hpp"
#include "MainScene.hpp"
#include "Scene.hpp"
#include "raylib.h"
#include "ws_init.hpp"

#include <cstdio>
#include <cstring>
#include <iostream>

class Menu : public Scene {
 private:
  MainScene* main_scene = nullptr;
  Texture2D clipboardTexture{};
  int start_btn_x = 0;
  int start_btn_y = 0;
  int start_font = 36;
  int padding = 12;
  std::string join_lobby_input = "";
  bool join_active = false;
  bool loaded_resources = false;
  int warmup_frame_gate_ = 0;

  // All menu copy is centred on the screen; callers only pick the baseline.
  void draw_text_centered(const char* text, int y, int font, Color color) {
    DrawText(text, GetScreenWidth() / 2 - MeasureText(text, font) / 2, y, font,
             color);
  }

  void draw_asset_loading_screen(float progress) {
    const int pct = static_cast<int>(progress * 100.0f);
    BeginDrawing();
    ClearBackground(BLACK);

    draw_text_centered("Preparing assets", GetScreenHeight() / 2 - 78, 44,
                       LIGHTGRAY);

    char progress_line[96];
    std::snprintf(progress_line, sizeof(progress_line), "%d%%", pct);
    draw_text_centered(progress_line, GetScreenHeight() / 2 - 12, 40, ORANGE);

    draw_text_centered("Loading 3D models into memory...",
                       GetScreenHeight() / 2 + 40, 20, Fade(SKYBLUE, 0.95f));
    EndDrawing();
  }

  void draw_join_text_input(int center_x, int y) {
    int width = 300;
    int height = 48;

    int x = center_x - width / 2;

    Vector2 mouse = GetMousePosition();

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      join_active = (mouse.x > x && mouse.x < x + width && mouse.y > y &&
                     mouse.y < y + height);
    }

    if (join_active) {
      int key = GetCharPressed();
      while (key > 0) {
        if (key >= 32 && key <= 125 && join_lobby_input.length() < 128) {
          join_lobby_input += (char)key;
        }
        key = GetCharPressed();
      }

      if (IsKeyPressed(KEY_BACKSPACE) && !join_lobby_input.empty()) {
        join_lobby_input.pop_back();
      }
    }

    DrawRectangle(x, y, width, height, join_active ? DARKGRAY : GRAY);
    DrawRectangleLines(x, y, width, height, LIGHTGRAY);

    const char* display = join_lobby_input.empty() ? "Enter Lobby ID..."
                                                   : join_lobby_input.c_str();

    DrawText(display, center_x - MeasureText(display, 20) / 2, y + 12, 20,
             LIGHTGRAY);
  }

  void compute_layout(const char* title, const char* btntext, int& title_x,
                      int& title_y, int& btn_x, int& btn_y, int& btn_width,
                      GameState& state) {
    int title_font = 24;
    int btn_font = this->start_font;

    int title_width = MeasureText(title, title_font);
    title_x = (GetScreenWidth() / 2) - (title_width / 2);
    title_y = (GetScreenHeight() / 2) - (title_font / 2) - 100;
    if (strcmp(title, "Welcome") != 0) {

      float scale = 0.25f;

      Vector2 position = {static_cast<float>(title_x + title_width + 10),
                          static_cast<float>(title_y - title_font / 2)};

      DrawTextureEx(this->clipboardTexture, position, 0.0f, scale, WHITE);

      float width = this->clipboardTexture.width * scale;
      float height = this->clipboardTexture.height * scale;

      Rectangle bounds = {position.x, position.y - 2, width, height + 8};

      Vector2 mouse = GetMousePosition();

      if (CheckCollisionPointRec(mouse, bounds)) {
        DrawRectangleLines(bounds.x, bounds.y, bounds.width, bounds.height,
                           WHITE);

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
          const char* lobby_id = state.lobbyId.c_str();
          SetClipboardText(lobby_id);
          printf("Clipboard clicked: %s!\n", lobby_id);
        }
      }
    }
    btn_width = MeasureText(btntext, btn_font);
    btn_x = (GetScreenWidth() / 2) - (btn_width / 2);
    btn_y = title_y + 48;
  }

  void draw_title(const char* title, int x, int y) {
    DrawText(title, x, y, 24, LIGHTGRAY);
  }

  void draw_create_button(const char* text, int x, int y, int width) {
    DrawRectangle(x - padding, y - padding, width + padding * 2,
                  start_font + padding * 2, RED);

    DrawText(text, x, y, start_font, LIGHTGRAY);
  }

  void draw_join_button(int x, int y, int width) {
    int offset_y = y + 72;

    DrawRectangle(x - padding, offset_y - padding, width + padding * 2,
                  start_font + padding * 2, GRAY);

    DrawText("JOIN LOBBY", x, offset_y, start_font, LIGHTGRAY);
  }

  void draw_cursor(Vector2 mouse_pos) {
    DrawCircle(mouse_pos.x, mouse_pos.y, 12, RAYWHITE);
  }

  bool is_hovering(Vector2 mouse, int x, int y, int width, int height) {
    return mouse.x > x - padding && mouse.x < x + width + padding &&
           mouse.y > y - padding && mouse.y < y + height + padding;
  }

  void handle_create_click(Scene*& curr_scene, GameState& state,
                           EMSCRIPTEN_WEBSOCKET_T& socket) {
    if (state.menu_creating_lobby)
      return;
    state.menu_error.clear();
    state.menu_creating_lobby = true;
    ws::send_invite(socket);
    std::cout << "Requesting lobby...\n";
  }

  void handle_join_click(Scene*& curr_scene, GameState& state,
                         EMSCRIPTEN_WEBSOCKET_T& socket) {
    if (join_lobby_input.empty()) {
      state.menu_error = "Enter a lobby id to join.";
      return;
    }
    state.menu_error.clear();
    ws::send_join(socket, join_lobby_input);
    std::cout << "Join lobby: " << join_lobby_input << "\n";
  }

 public:
  Menu(Camera& cam) : Scene(cam) {
    start_btn_x = (GetScreenHeight() / 2);
    start_btn_y = (GetScreenWidth() / 2);
  }

  void update(Scene*& curr_scene, GameState& state,
              EMSCRIPTEN_WEBSOCKET_T& socket) override {
    if (curr_scene != this) {
      return;
    }

    // Boot: construct MainScene once; proxies are instant, heavy models stream
    // while this loading screen is visible.
    if (main_scene == nullptr) {
      ShowCursor();
      BeginDrawing();
      ClearBackground(BLACK);
      draw_text_centered("loading assets", GetScreenHeight() / 2 - 18, 36,
                         LIGHTGRAY);
      EndDrawing();

      main_scene = new MainScene(camera);
      return;
    }

    if (state.gameStarted && state.has_player_slot && !main_scene->assets_ready()) {
      // Throttle warmup cadence to limit frame-time spikes on wasm/main thread.
      warmup_frame_gate_++;
      if ((warmup_frame_gate_ % 6) == 0)
        main_scene->prime_assets_step();
      HideCursor();
      draw_asset_loading_screen(main_scene->assets_progress());
      return;
    }

    if (state.gameStarted && state.has_player_slot && main_scene->assets_ready()) {
      ShowCursor();
      curr_scene = main_scene;
      return;
    }

    ShowCursor();

    if (!loaded_resources) {
      Image clip_img = LoadImage("assets/images/clipboard.png");
      this->clipboardTexture = LoadTextureFromImage(clip_img);
      UnloadImage(clip_img);
      loaded_resources = true;
    }

    BeginDrawing();

    Vector2 mouse_pos = GetMousePosition();
    int title_x, title_y, btn_x, btn_y, btn_width;

    if (state.lobbyId.empty()) {
      const char* title = "Welcome";
      const char* btntext = state.menu_creating_lobby ? "Creating lobby…"
                                                      : "Create Lobby";
      compute_layout(title, btntext, title_x, title_y, btn_x, btn_y, btn_width,
                     state);

      draw_title(title, title_x, title_y);
      draw_create_button(btntext, btn_x, btn_y, btn_width);

      draw_join_text_input(GetScreenWidth() / 2, btn_y + 140);

      int join_width = MeasureText("JOIN LOBBY", start_font);
      int join_x = (GetScreenWidth() / 2) - (join_width / 2);
      draw_join_button(join_x, btn_y, join_width);

      if (!state.menu_error.empty()) {
        draw_text_centered(state.menu_error.c_str(), btn_y + 210, 18, MAROON);
      }

      draw_text_centered("Join an existing lobby with the id above.",
                         btn_y + 240, 16, Fade(GRAY, 0.9f));

      bool hovering_create =
          is_hovering(mouse_pos, btn_x, btn_y, btn_width, start_font);
      bool hovering_join =
          is_hovering(mouse_pos, join_x, btn_y + 72, join_width, start_font);

      if (!state.menu_creating_lobby && (hovering_create || hovering_join)) {
        SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
          if (hovering_create) {
            handle_create_click(curr_scene, state, socket);
          } else if (hovering_join) {
            handle_join_click(curr_scene, state, socket);
          }
        }
      } else {
        SetMouseCursor(MOUSE_CURSOR_DEFAULT);
      }
    } else {
      // In lobby (host created or guest joined)
      const char* role_line =
          state.is_lobby_host ? "You are the host" : "Joined existing lobby";
      const char* title = state.lobbyId.c_str();
      const char* btntext = "Start Game";
      compute_layout(title, btntext, title_x, title_y, btn_x, btn_y, btn_width,
                     state);

      draw_title(title, title_x, title_y);
      int ry = title_y + 32;
      draw_text_centered(role_line, ry, 18, SKYBLUE);
      ry += 28;

      if (state.is_lobby_host) {
        const char* start_txt = "Press SPACE to start (both players ready)";
        int start_width = MeasureText(start_txt, start_font);
        int sx = (GetScreenWidth() / 2) - (start_width / 2);
        draw_create_button(start_txt, sx, btn_y + 24, start_width);
        if (IsKeyPressed(KEY_SPACE)) {
          ws::send_start(socket);
        }
      } else {
        draw_text_centered("Waiting for host to start the game…",
                           btn_y + 40, 22, LIGHTGRAY);
      }

      if (!state.menu_error.empty()) {
        draw_text_centered(state.menu_error.c_str(), btn_y + 120, 18, MAROON);
      }

      SetMouseCursor(MOUSE_CURSOR_DEFAULT);
    }

    draw_cursor(mouse_pos);

    EndDrawing();
  }

  void listen() override {
    static char clipboardBuffer[1024] = {0};

    if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_V)) {
      EM_ASM(
          {
            navigator.clipboard.readText().then(function(text) {
              if (!text)
                return;

              stringToUTF8(text, $0, 1024);
            });
          },
          clipboardBuffer);

      this->join_lobby_input = clipboardBuffer;
    }
    ClearBackground(BLACK);
  }
};

inline Scene* init_menu(Camera& cam) {
  return new Menu(cam);
}

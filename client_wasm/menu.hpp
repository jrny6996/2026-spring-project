#pragma once
#include "GameState.hpp"
#include "MainScene.hpp"
#include "Scene.hpp"
#include "raylib.h"
#include "ws_init.hpp"

#include <cstring>
#include <iostream>

class Menu : public Scene {
 private:
  Scene* main_scene = nullptr;
  Texture2D clipboardTexture{};
  int start_btn_x = 0;
  int start_btn_y = 0;
  int start_font = 36;
  int padding = 12;
  std::string join_lobby_input = "";
  bool join_active = false;
  bool loaded_resources = false;

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

    DrawText(display, x + 10, y + 12, 20, LIGHTGRAY);
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

    DrawText(
        "JOIN LOBBY",
        (GetScreenWidth() / 2) - (MeasureText("JOIN LOBBY", start_font) / 2),
        offset_y, start_font, LIGHTGRAY);
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

    // Boot: only "loading assets" until MainScene (models, shaders) is ready.
    if (main_scene == nullptr) {
      BeginDrawing();
      ClearBackground(BLACK);
      const char* msg = "loading assets";
      int tw = MeasureText(msg, 36);
      DrawText(msg, GetScreenWidth() / 2 - tw / 2,
               GetScreenHeight() / 2 - 18, 36, LIGHTGRAY);
      EndDrawing();

      main_scene = new MainScene(camera);
      return;
    }

    if (state.gameStarted && state.has_player_slot) {
      curr_scene = main_scene;
      return;
    }

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
      draw_join_button(btn_x, btn_y, btn_width);

      if (!state.menu_error.empty()) {
        DrawText(state.menu_error.c_str(), title_x, btn_y + 210, 18, MAROON);
      }

      DrawText("Join an existing lobby with the id above.", title_x,
               btn_y + 240, 16, Fade(GRAY, 0.9f));

      bool hovering_create =
          is_hovering(mouse_pos, btn_x, btn_y, btn_width, start_font);
      bool hovering_join =
          is_hovering(mouse_pos, btn_x, btn_y + 72, btn_width, start_font);

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
      DrawText(role_line, title_x, ry, 18, SKYBLUE);
      ry += 28;

      if (state.is_lobby_host) {
        const char* start_txt = "Press SPACE to start (both players ready)";
        int sx = GetScreenWidth() / 2 - MeasureText(start_txt, start_font) / 2;
        draw_create_button(start_txt, sx, btn_y + 24,
                             MeasureText(start_txt, start_font));
        if (IsKeyPressed(KEY_SPACE)) {
          ws::send_start(socket);
        }
      } else {
        const char* wait_txt = "Waiting for host to start the game…";
        int wx = GetScreenWidth() / 2 - MeasureText(wait_txt, 22) / 2;
        DrawText(wait_txt, wx, btn_y + 40, 22, LIGHTGRAY);
      }

      if (!state.menu_error.empty()) {
        DrawText(state.menu_error.c_str(), title_x, btn_y + 120, 18, MAROON);
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

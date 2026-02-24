#pragma once
#include "Scene.hpp"
#include "raylib.h"

class Menu : public Scene {
 public:
  Menu(Camera& cam) : Scene(cam) {}

  void update(Scene* curr_scene) override {
    const char* str = "Welcome!";
    int font_size = 12;
    int str_width = MeasureText(str, font_size);
    int y = (GetScreenHeight() / 2) - (font_size / 2);
    int x = (GetScreenWidth() / 2) - (str_width / 2);
    // std::cout << y << ", " << GetScreenHeight() << "\n";
    int currentFPS = GetFPS();
    DrawText(str, x, y, font_size, LIGHTGRAY);
    const char* fps = TextFormat("FPS: %i", curr_scene);
    DrawText(fps, 12, 12, font_size, LIGHTGRAY);
  }
};

Menu init_menu(Camera& cam) {
  Menu menu(cam);

  return menu;
}
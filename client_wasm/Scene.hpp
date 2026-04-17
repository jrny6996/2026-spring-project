#pragma once
#include <iostream>
#include "GameState.hpp"
#include "raylib.h"

#include <emscripten/emscripten.h>
#include <emscripten/websocket.h>
class Scene {
 private:
  /* data */
 public:
  Camera& camera;
  Scene(Camera& cam) : camera(cam) {};

  virtual void update(Scene*& curr_scene, GameState& state,
                      EMSCRIPTEN_WEBSOCKET_T& socket) {};
  virtual void listen() {}
};

class ThreeDScene : public Scene {
 private:
 public:
  ThreeDScene(Camera3D& cam) : Scene(cam) {};
};

class ModelInSpace {
 public:
  ModelInSpace() {}

  void move() {};
};
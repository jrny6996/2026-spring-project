#pragma once
#include <iostream>
#include "raylib.h"

class Scene {
 private:
  /* data */
 public:
  Camera& camera;
  Scene(Camera& cam) : camera(cam) {};
  void listen() {};

  virtual void update(Scene* curr_scene) {};
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
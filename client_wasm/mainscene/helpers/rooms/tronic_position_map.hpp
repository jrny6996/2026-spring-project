#pragma once
#include <initializer_list>
#include <map>
#include <string>
#include "raylib.h"

class TronicPosition {
 public:
  Vector3 position;
  Vector3 rotationAxis;
  float rotationAngle;
};
class TronicPositionWithKeys {
 public:
  std::string key;
  TronicPosition position;
      TronicPositionWithKeys(const std::string& key, const TronicPosition& position)
        : key(key), position(position) {}
};
class TronicPositionMap {
 public:
  /// Draw scale for this GLB (map geometry uses 1.0; each rip uses different units).
  Vector3 scale{0.05f, 0.05f, 0.05f};
  /// Added to every layout position so characters on the same slot do not sit in one spot.
  Vector3 character_offset{0.0f, 0.0f, 0.0f};
  Color tint{WHITE};
  const Model& model;
  std::map<std::string, TronicPosition> pos_map{};

  TronicPositionMap(const Model& model) : model(model) {}
  // reference
  // void DrawModelEx(Model model, Vector3 position, Vector3 rotationAxis, float rotationAngle, Vector3 scale, Color tint)
  void set_position_map(
      std::initializer_list<TronicPositionWithKeys> positions) {
    for (auto position_kv : positions) {
      this->pos_map[position_kv.key] =
          position_kv.position;
    };
  }
  void draw(std::string room_key) {
    if (pos_map.find(room_key) != pos_map.end()) {
      const auto pos = pos_map[room_key];
      DrawModelEx(this->model, pos.position, pos.rotationAxis,
                  pos.rotationAngle, this->scale, this->tint);
    }
  }
};
#pragma once
#include <memory>
#include <vector>
#include "tronic_position_map.hpp"

std::vector<TronicPositionMap> create_tronic_positions(const Model& freddy) {

  std::vector<TronicPositionMap> map_vector;
  TronicPositionMap freddy_map(freddy);
  freddy_map.set_position_map({{"toy_stage", {{0, 0, 0}, {0, 1, 0}, 0.0f}},
                               {"stage", {{1, 0, 0}, {0, 1, 0}, 90.0f}}});

  map_vector.emplace_back(freddy);
  return map_vector
};

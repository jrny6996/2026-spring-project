#pragma once
#include <map>
#include <memory>
#include "tronic_position_map.hpp"

std::map<std::string, TronicPositionMap> create_tronic_positions(
    const Model& freddy) {

  std::map<std::string, TronicPositionMap> tronicMapMap;
  TronicPositionMap freddy_map(freddy);
  freddy_map.set_position_map({{"toy_stage", {{0, 0, 0}, {0, 1, 0}, 0.0f}},
                               {"stage", {{-1.0f, 4.5f, -42.0f}, {0, 1, 0}, 90.0f}}});

  tronicMapMap.emplace("freddy", freddy_map);
  return tronicMapMap;
}

#pragma once
#include <map>
#include <string>
#include "tronic_position_map.hpp"

/// Disambiguates P1 vs P2 coordinates when the server uses the same room alias
/// on both graphs (e.g. rhs_hall).
inline std::string tronic_room_key(bool is_player_one,
                                   const char* sim_room_alias) {
  if (!sim_room_alias || sim_room_alias[0] == '\0')
    return {};
  if (is_player_one)
    return std::string(sim_room_alias);
  return std::string("p2_") + sim_room_alias;
}

inline std::map<std::string, TronicPositionMap> create_tronic_positions(
    const Model& freddy) {

  std::map<std::string, TronicPositionMap> tronicMapMap;
  TronicPositionMap freddy_map(freddy);
  freddy_map.set_position_map(
      {{"stage", {{-1.0f, 4.5f, -42.0f}, {0, 1, 0}, 0.0f}},
       {"party_room", {{-2.0f, 3.0f, -30.0f}, {0, 1, 0}, 45.0f}},
       {"lhs_hall", {{-6.0f, 3.0f, -17.0f}, {0, 1, 0}, 0.0f}},
       {"rhs_hall", {{4.0f, 4.5f, -18.0f}, {0, 1, 0}, 0.0f}},
       
       {"lhs_closet", {{-3.5f, 3.0f, 2.5f}, {0, 1, 0}, 0.0f}},
       {"rhs_closet", {{3.5f, 3.0f, 2.5f}, {0, 1, 0}, 0.0f}},



       {"p2_lhs_party", {{-8.0f, 52.0f, -12.0f}, {0, 1, 0}, 0.0f}},
       {"p2_rhs_hall", {{2.0f, 52.0f, -8.0f}, {0, 1, 0}, 0.0f}},
       {"p2_lhs_repair", {{-7.0f, 52.0f, -26.0f}, {0, 1, 0}, 0.0f}},
       {"p2_rhs_party", {{8.0f, 52.0f, -12.0f}, {0, 1, 0}, 0.0f}},
       {"p2_middle_hall_two", {{28.0f, 52.0f, -22.0f}, {0, 1, 0}, 0.0f}},
       {"p2_toy_stage", {{33.0f, 52.0f, -37.0f}, {0, 1, 0}, 0.0f}},
       {"toy_stage", {{0.0f, 0.0f, 0.0f}, {0, 1, 0}, 0.0f}}});

  tronicMapMap.emplace("freddy", freddy_map);
  return tronicMapMap;
}

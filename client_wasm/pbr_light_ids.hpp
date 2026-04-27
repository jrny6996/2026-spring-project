#pragma once

// Registration order must match mainscene::init_all_scene_pbr_lights in
// mainscene/helpers/pbr_lights.hpp.
enum class PbrLightId : int {
  PlayerFollow = 0,
  FirstFloorOrange = 1,
  FirstFloorBlue = 2,
  SecondFloorPurple = 3,
  SecondFloorBlue = 4,
  SecondFloorOrange = 5,
  SecondFloorHallSpot = 6,
  FirstFloorLHS = 7,
  FirstFloorRHS = 8,
  SecondFloorLHS = 9,
  SecondFloorRHS = 10,
  FirstFloorParty = 11,
  FirstFloorStage = 12,
  FirstFloorParty3 = 13,
  FirstFloorParty4 = 14,
  FirstFloorLHSCloset = 15,
  FirstFloorRHSCloset = 16,
  SecondFloorParty2 = 17,
  SecondFloorParty1 = 18,
  SecondFloorParty2RHS = 19,
  SecondFloorParty1RHS = 20,
  SecondFloorCircus = 21,
  SecondFloorStage = 22,
  /// 1F hall door lights (A/D); original LHS/RHS hall positions, after kInitSpotOrder.
  FirstFloorHallKeyLHS = 23,
  FirstFloorHallKeyRHS = 24,
};

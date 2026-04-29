#pragma once
#include <cctype>
#include <map>
#include <string>
#include "tronic_position_map.hpp"

/// One animatronic mesh: model reference, draw scale, and per-character world nudge.
struct TronicCharacterSpec {
  const Model& model;
  Vector3 scale;
  Vector3 character_offset;
};

/// The four classic characters; toy entities reuse the same spec as their mesh.
struct TronicRosterSpec {
  TronicCharacterSpec freddy;
  TronicCharacterSpec bonnie;
  TronicCharacterSpec chica;
  TronicCharacterSpec foxy;
};

/// Maps a live sim row to the tronic map key (freddy, bonnie, …). Lowercases
/// the server `name` and falls back to `entityId` when the name is empty or not a
/// known animatronic id.
inline std::string tronic_key_from_sim_entity(int entity_id, const std::string& name) {
  std::string key;
  if (!name.empty()) {
    key.reserve(name.size());
    for (unsigned char c : name)
      key += static_cast<char>(std::tolower(c));
  }
  if (key == "freddy" || key == "bonnie" || key == "chica" || key == "foxy" ||
      key == "toy_freddy" || key == "toy_bonnie" || key == "toy_chica" ||
      key == "toy_foxy")
    return key;
  switch (entity_id) {
    case 1:
      return "freddy";
    case 2:
      return "bonnie";
    case 3:
      return "chica";
    case 4:
      return "foxy";
    case 11:
      return "toy_freddy";
    case 12:
      return "toy_bonnie";
    case 13:
      return "toy_chica";
    case 14:
      return "toy_foxy";
    default:
      return key;
  }
}

/// 3D `pos_map` key for a sim `roomAlias` and `entityId`. FNaF1 classic ids 1–4
/// live on the first–floor graph; their 3D slots use the same strings as the server
/// (e.g. "stage", never "p2_stage") for both P1 and P2 clients. Toy line ids 11–14
/// use the 2F graph; map those server aliases to the `p2_*` keys in the tronic
/// table (e.g. toy_stage -> p2_toy_stage).
inline std::string tronic_3d_pos_key(int entity_id, const char* sim_room_alias) {
  if (!sim_room_alias || sim_room_alias[0] == '\0')
    return {};
  const std::string a(sim_room_alias);
  if (entity_id >= 1 && entity_id <= 4)
    return a;
  if (entity_id >= 11) {
    if (a == "toy_stage")
      return "p2_toy_stage";
    if (a == "lhs_party")
      return "p2_lhs_party";
    if (a == "lhs_party_two")
      return "p2_lhs_party_two";
    if (a == "lhs_repair")
      return "p2_lhs_repair";
    if (a == "rhs_party_one")
      return "p2_rhs_party_one";
    if (a == "rhs_party_two")
      return "p2_rhs_party_two";
    if (a == "p2_rhs_hall")
      return "p2_rhs_hall";
    if (a == "middle_hall_two" || a == "middle_hall_one" || a == "center_door" ||
        a == "facade" || a == "lhs_vent" || a == "rhs_vent" ||
        a == "player_two_office" || a == "p2_mangle_room")
      return "p2_" + a;
  }
  return a;
}

/// Shared room layout for every animatronic mesh (positions are per room alias).
inline TronicPositionMap make_tronic_layout(const Model& model) {
  TronicPositionMap m(model);
  m.set_position_map(
      {{"stage", {{-1.0f, 4.25f, -42.0f}, {0, 1, 0}, 0.0f}},
       {"party_room", {{-2.0f, 3.0f, -30.0f}, {0, 1, 0}, 45.0f}},
       {"repair", {{-20.0f, 3.0f, -32.0f}, {0, 1, 0}, 0.0f}},
       {"bathrooms", {{16.5f, 3.0f, -30.0f}, {0, 1, 0}, 0.0f}},
       {"lhs_hall", {{-4.5f, 2.5f, -13.0f}, {0, 1, 0}, 0.0f}},
       {"rhs_hall", {{4.0f, 2.5f, -13.0f}, {0, 1, 0}, 0.0f}},

       {"lhs_closet", {{-3.35f, 2.5f, 3.5f}, {0, 1, 0}, 180.0f}},
       {"rhs_closet", {{3.35f, 2.5f, 3.5f}, {0, 1, 0}, 180.0f}},

       {"p2_lhs_party", {{-8.0f, 52.0f, -12.0f}, {0, 1, 0}, 0.0f}},
       {"p2_lhs_party_two", {{-8.0f, 52.0f, -10.0f}, {0, 1, 0}, 0.0f}},
       {"p2_rhs_hall", {{2.0f, 52.0f, -8.0f}, {0, 1, 0}, 0.0f}},
       {"p2_lhs_repair", {{-7.0f, 52.0f, -26.0f}, {0, 1, 0}, 0.0f}},
       {"p2_rhs_party_one", {{8.0f, 52.0f, -24.0f}, {0, 1, 0}, 0.0f}},
       {"p2_rhs_party_two", {{8.0f, 52.0f, -12.0f}, {0, 1, 0}, 0.0f}},
       {"p2_middle_hall_two", {{28.0f, 52.0f, -22.0f}, {0, 1, 0}, 0.0f}},
       {"p2_toy_stage", {{33.0f, 52.0f, -37.0f}, {0, 1, 0}, 0.0f}},
       {"toy_stage", {{0.0f, 0.0f, 0.0f}, {0, 1, 0}, 0.0f}}});
  return m;
}

inline TronicPositionMap make_tronic_map(const TronicCharacterSpec& spec) {
  TronicPositionMap m = make_tronic_layout(spec.model);
  m.scale = spec.scale;
  m.character_offset = spec.character_offset;
  return m;
}

/// Keys must match server entity `name` (see game.go / sim entities).
inline std::map<std::string, TronicPositionMap> create_tronic_positions(
    const TronicRosterSpec& roster) {
  std::map<std::string, TronicPositionMap> out;
  out.emplace("freddy", make_tronic_map(roster.freddy));
  out.emplace("bonnie", make_tronic_map(roster.bonnie));
  out.emplace("chica", make_tronic_map(roster.chica));
  out.emplace("foxy", make_tronic_map(roster.foxy));
  out.emplace("toy_freddy", make_tronic_map(roster.freddy));
  out.emplace("toy_bonnie", make_tronic_map(roster.bonnie));
  out.emplace("toy_chica", make_tronic_map(roster.chica));
  out.emplace("toy_foxy", make_tronic_map(roster.foxy));
  return out;
}

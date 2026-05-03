#pragma once
#include <string>
#include <vector>

struct CheckCameraEntityRow {
  int id = 0;
  std::string name;
};

/// One animatronic from server `state.simEntities` (live sim positions).
struct SimEntityRow {
  int id = 0;
  std::string name;
  std::string room_alias;
};

class GameState {
 public:
  std::string lobbyId;
  /// Last camera-check message for on-screen HUD (set over WebSocket).
  std::string check_camera_status;
  /// Entities reported for the last camera-room check (server sim).
  std::vector<CheckCameraEntityRow> check_camera_entities;
  /// Live sim positions (from periodic `state` messages while the game runs).
  std::vector<SimEntityRow> sim_entities;
  bool lobby_created = false;
  /// True if this client created the lobby (invite); false if they joined with an id.
  bool is_lobby_host = false;
  bool gameStarted = false;
  int gameTime = 0;
  /// Server lobby night (1–7), from `nightNum` in state JSON.
  int night_num = 1;
  /// Last menu-related error (e.g. failed join); shown on welcome screen.
  std::string menu_error;
  /// True after clicking Create until the server responds (invite or error).
  bool menu_creating_lobby = false;

  bool has_player_slot = false;
  bool is_player_one = false;
  bool p2_mask_down = false;

  /// Shared night resources (from server `state`).
  int power = 30;
  int music_box_wind = 0;
  /// Second player joined this lobby (still connected slot).
  bool p2_in_lobby = false;
  /// P2 eliminated; P1 may use SPACE to charge at half rate.
  bool p2_lost = false;

  /// Security-camera check: server round-trip in progress; feed is suspended.
  bool check_camera_in_flight = false;
  bool check_camera_restore_feed = false;
  int check_camera_saved_active_feed = -1;
  /// True when the in-flight check was from the C key (suspend feed until reply).
  bool check_camera_suspend_feed_for_request = false;
  /// Last `roomAlias` from a checkCamera or state payload.
  std::string check_camera_last_room_alias;

  void printState() const {
    printf("Lobby: %s, Started: %s, Time: %d\n", lobbyId.c_str(),
           gameStarted ? "true" : "false", gameTime);
  }
};
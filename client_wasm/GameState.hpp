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
  /// Last menu-related error (e.g. failed join); shown on welcome screen.
  std::string menu_error;
  /// True after clicking Create until the server responds (invite or error).
  bool menu_creating_lobby = false;

  bool has_player_slot = false;
  bool is_player_one = false;

  /// Security-camera check: server round-trip in progress; feed is suspended.
  bool check_camera_in_flight = false;
  bool check_camera_restore_feed = false;
  int check_camera_saved_active_feed = -1;

  void printState() const {
    printf("Lobby: %s, Started: %s, Time: %d\n", lobbyId.c_str(),
           gameStarted ? "true" : "false", gameTime);
  }
};
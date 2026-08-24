#pragma once

#include "GameState.hpp"
#include "raylib.h"

namespace audio_runtime {

inline bool enabled_ = false;
inline bool assets_loaded_ = false;
inline Music menu_music_{};
inline Music game_music_{};
inline Sound door_pound_{};

inline bool music_valid(const Music& m) {
  return m.ctxData != nullptr;
}

inline bool sound_valid(const Sound& s) {
  return s.frameCount > 0;
}

inline void ensure_device_ready() {
  if (enabled_ && !IsAudioDeviceReady()) {
    InitAudioDevice();
  }
}

inline void ensure_assets_loaded() {
  if (assets_loaded_ || !enabled_ || !IsAudioDeviceReady()) {
    return;
  }
  menu_music_ = LoadMusicStream("assets/sound/FNaF_1_Remaster_Main_Menu.ogg");
  game_music_ = LoadMusicStream("assets/sound/ColdPresc_B.ogg");
  door_pound_ = LoadSound("assets/sound/Door Pounding Me.mp3");

  if (music_valid(menu_music_)) {
    menu_music_.looping = true;
    SetMusicVolume(menu_music_, 0.60f);
  }
  if (music_valid(game_music_)) {
    game_music_.looping = true;
    SetMusicVolume(game_music_, 0.55f);
  }
  if (sound_valid(door_pound_)) {
    SetSoundVolume(door_pound_, 0.90f);
  }
  assets_loaded_ = true;
}

inline void stop_all_audio() {
  if (!assets_loaded_ || !IsAudioDeviceReady()) {
    return;
  }
  if (music_valid(menu_music_)) {
    StopMusicStream(menu_music_);
  }
  if (music_valid(game_music_)) {
    StopMusicStream(game_music_);
  }
}

inline void set_enabled(bool enabled) {
  enabled_ = enabled;
  ensure_device_ready();
  ensure_assets_loaded();
  if (!enabled_) {
    stop_all_audio();
  }
}

inline void update(const GameState& state) {
  ensure_device_ready();
  ensure_assets_loaded();

  if (!enabled_ || !assets_loaded_ || !IsAudioDeviceReady()) {
    return;
  }

  const bool in_game = state.gameStarted;
  if (in_game) {
    if (music_valid(menu_music_) && IsMusicStreamPlaying(menu_music_)) {
      StopMusicStream(menu_music_);
    }
    if (music_valid(game_music_) && !IsMusicStreamPlaying(game_music_)) {
      PlayMusicStream(game_music_);
    }
    if (music_valid(game_music_)) {
      UpdateMusicStream(game_music_);
    }
  } else {
    if (music_valid(game_music_) && IsMusicStreamPlaying(game_music_)) {
      StopMusicStream(game_music_);
    }
    if (music_valid(menu_music_) && !IsMusicStreamPlaying(menu_music_)) {
      PlayMusicStream(menu_music_);
    }
    if (music_valid(menu_music_)) {
      UpdateMusicStream(menu_music_);
    }
  }
}

inline void play_door_pound() {
  ensure_device_ready();
  ensure_assets_loaded();
  if (!enabled_ || !assets_loaded_ || !IsAudioDeviceReady()) {
    return;
  }
  if (sound_valid(door_pound_)) {
    PlaySound(door_pound_);
  }
}

}  // namespace audio_runtime

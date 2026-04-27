#pragma once
#include "raylib.h"
#include <cmath>

/// Short UI click; no external asset. Safe to call if InitAudioDevice was run.
inline Sound LoadProceduralCamSwitchSound() {
  if (!IsAudioDeviceReady())
    return Sound{};
  const unsigned int rate = 22050;
  const float dur = 0.09f;
  const unsigned int n = static_cast<unsigned int>(static_cast<float>(rate) * dur);
  if (n < 8)
    return Sound{};
  Wave w{};
  w.sampleRate = rate;
  w.sampleSize = 16;
  w.channels = 1;
  w.frameCount = n;
  w.data = MemAlloc(n * (unsigned int)sizeof(short));
  if (!w.data)
    return Sound{};
  auto* samples = static_cast<short*>(w.data);
  for (unsigned int i = 0; i < n; i++) {
    float t = static_cast<float>(i) / static_cast<float>(rate);
    float v = 0.0f;
    v += sinf(2.0f * (float)PI * 1200.0f * t) * 0.2f;
    v += sinf(2.0f * (float)PI * 800.0f * t) * 0.25f;
    v *= 1.0f - (static_cast<float>(i) / static_cast<float>(n));
    if (v > 1.0f)
      v = 1.0f;
    if (v < -1.0f)
      v = -1.0f;
    samples[i] = static_cast<short>(v * 30000.0f);
  }
  Sound s = LoadSoundFromWave(w);
  UnloadWave(w);
  if (IsSoundValid(s))
    SetSoundVolume(s, 0.35f);
  return s;
}

inline void UnloadIfValid(Sound& s) {
  if (IsSoundValid(s)) {
    UnloadSound(s);
    s = Sound{};
  }
}

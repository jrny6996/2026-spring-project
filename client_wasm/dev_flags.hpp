#pragma once

/// Set to true for local debugging (cmd/ctrl+` freeroam + fullbright, manual T tick, coord HUD).
/// Player builds should keep this false.
#if defined(CLIENT_DEBUG_BUILD)
inline constexpr bool is_dev = true;
#else
inline constexpr bool is_dev = false;
#endif

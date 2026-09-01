// Adam Kavanagh - D00247069
#pragma once
#include <cstddef>
#include <cstdint>

// ---------------------------------------------------------------------------
// Global game constants.
//
// The world size is deliberately a compile-time constant rather than being
// derived from the window size. Every client and the server must agree on the
// exact same arena dimensions, otherwise quantised network positions (see
// net_compression.hpp) would be decoded differently on each machine and the
// clients would disagree about where the map boundaries are.
// ---------------------------------------------------------------------------
constexpr auto kTimePerFrame = 1.f / 60.f;

// Matches the pixel size of Media/Textures/Road to Caen.png so the map art is
// drawn 1:1 with no tiling seams. Both axes fit inside an int16_t, which is
// what lets positions be sent as 2 bytes each instead of 4.
constexpr float kWorldWidth = 3168.f;
constexpr float kWorldHeight = 2304.f;

// Seconds the victory / defeat screen stays up before returning to the menu.
constexpr float kResultScreenSeconds = 8.f;

// Multiplayer match rules.
constexpr float kMatchDurationSeconds = 15.f * 60.f;	// brief: 15 minutes of play
constexpr uint16_t kScoreToWin = 20;					// kills for a team to win early
constexpr float kRespawnDelaySeconds = 4.f;
constexpr int kTankHitPoints = 100;

// Networking rates.
constexpr float kServerTickRate = 20.f;		// server -> client snapshots per second
constexpr float kClientTickRate = 20.f;		// client -> server state updates per second
constexpr std::size_t kMaxPlayers = 16;		// brief requires >= 15 simultaneous players

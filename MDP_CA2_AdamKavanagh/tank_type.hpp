// Adam Kavanagh - D00247069
#pragma once
#include <cstdint>

// The two playable hulls. Which one a player gets is decided by their network
// identifier (see team_assignment.hpp) so that every client independently
// arrives at the same answer without the server having to send the type.
enum class TankType
{
    kSherman,   // Allies
    kPanzer,    // Axis
    kTankTypeCount
};

enum class TeamID : uint8_t
{
    kAllies,
    kAxis,
    kNone
};

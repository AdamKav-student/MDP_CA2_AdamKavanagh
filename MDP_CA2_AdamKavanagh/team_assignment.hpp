// Adam Kavanagh - D00247069
#pragma once
#include "tank_type.hpp"
#include <cstdint>

// Team allocation is a pure function of the player's network identifier, so
// every client works out every other player's team without the server having
// to spend a byte on it. Identifiers are handed out 1, 2, 3, ... by the
// server, so this alternates players between the two sides and keeps the
// teams balanced as people join.
inline TeamID AssignTeam(uint8_t identifier)
{
    return (identifier % 2 == 1) ? TeamID::kAllies : TeamID::kAxis;
}

inline TankType AssignTankType(uint8_t identifier)
{
    return AssignTeam(identifier) == TeamID::kAllies ? TankType::kSherman : TankType::kPanzer;
}

// Adam Kavanagh - D00247069
#pragma once
#include <SFML/System/Vector2.hpp>
#include <cstdint>

// Layout of the single-player training map, which the World builds when it is
// constructed with networked = false: the player's Sherman and one stationary
// Panzer to destroy.
namespace TutorialConfig
{
    // The two tanks must not share an identifier, otherwise the Player's
    // identifier filter would drive both of them at once. Identifier 0 is
    // never handed out by the server, so the training enemy can safely use it.
    constexpr uint8_t kPlayerIdentifier = 1;
    constexpr uint8_t kEnemyIdentifier = 0;

    inline sf::Vector2f GetPlayerSpawnPosition(sf::Vector2f world_size)
    {
        return sf::Vector2f(world_size.x * 0.5f, world_size.y * 0.65f);
    }

    inline sf::Vector2f GetEnemySpawnPosition(sf::Vector2f world_size)
    {
        return sf::Vector2f(world_size.x * 0.5f, world_size.y * 0.35f);
    }
}

#pragma once
#include <SFML/System/Vector2.hpp>
#include <cstdint>

// Configuration for the single-player tutorial World (loaded when the
// player chooses "single player" from the menu, as opposed to "host"/"join"
// which go to MultiplayerGameState). A minimal scene: the player's own
// Sherman, one stationary Panzer to destroy, then back to the menu.
namespace TutorialConfig
{
    // Identifiers: the player's tank and the stationary enemy must use
    // different identifiers, or the enemy would respond to the player's
    // input too (Player filters commands by exact identifier match - see
    // player.cpp). 0 is deliberately left unused by any Player so the
    // enemy never receives movement/turret/fire commands.
    constexpr uint8_t kPlayerIdentifier = 1;
    constexpr uint8_t kEnemyIdentifier = 0;

    // World-space spawn positions - adjust to taste once dropped into your
    // actual map/world bounds.
    inline sf::Vector2f GetPlayerSpawnPosition(sf::Vector2f world_size)
    {
        return sf::Vector2f(world_size.x * 0.5f, world_size.y * 0.8f);
    }

    inline sf::Vector2f GetEnemySpawnPosition(sf::Vector2f world_size)
    {
        return sf::Vector2f(world_size.x * 0.5f, world_size.y * 0.2f);
    }
}

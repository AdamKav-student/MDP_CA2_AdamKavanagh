// Adam Kavanagh - D00247069
#pragma once
#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Vector2.hpp>

// Minimum-translation-vector push-out for two overlapping axis-aligned boxes.
// Returns the offset that must be applied to "mover" to separate it from
// "blocker" along whichever axis is least penetrated.
sf::Vector2f ResolveAabbPushOut(const sf::FloatRect& mover, const sf::FloatRect& blocker);

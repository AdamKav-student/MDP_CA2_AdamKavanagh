// Adam Kavanagh - D00247069
#pragma once
#include "debris_type.hpp"
#include <SFML/System/Vector2.hpp>
#include <vector>

struct DebrisPlacement
{
    DebrisType      m_type;
    sf::Vector2f    m_relative_position;    // 0..1 across the world bounds
    float           m_rotation_degrees;
};

// A fixed, hand-authored layout. Because it is identical on every machine and
// never changes at runtime, obstacles cost zero network bandwidth - the
// clients simply build the same map from this table when the World is created.
const std::vector<DebrisPlacement>& GetDebrisLayout();

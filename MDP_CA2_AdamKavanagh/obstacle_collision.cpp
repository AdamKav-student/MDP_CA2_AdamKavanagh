// Adam Kavanagh - D00247069
#include "obstacle_collision.hpp"
#include <algorithm>
#include <cmath>

sf::Vector2f ResolveAabbPushOut(const sf::FloatRect& mover, const sf::FloatRect& blocker)
{
    const float mover_left = mover.position.x;
    const float mover_right = mover.position.x + mover.size.x;
    const float mover_top = mover.position.y;
    const float mover_bottom = mover.position.y + mover.size.y;

    const float blocker_left = blocker.position.x;
    const float blocker_right = blocker.position.x + blocker.size.x;
    const float blocker_top = blocker.position.y;
    const float blocker_bottom = blocker.position.y + blocker.size.y;

    const float overlap_x = std::min(mover_right, blocker_right) - std::max(mover_left, blocker_left);
    const float overlap_y = std::min(mover_bottom, blocker_bottom) - std::max(mover_top, blocker_top);

    if (overlap_x <= 0.f || overlap_y <= 0.f)
    {
        return sf::Vector2f(0.f, 0.f);
    }

    // Push out along the shallower axis - that is the smallest correction that
    // separates the two boxes, so the tank slides along the obstacle instead
    // of being teleported around it.
    if (overlap_x < overlap_y)
    {
        const float direction = (mover_left + mover_right) < (blocker_left + blocker_right) ? -1.f : 1.f;
        return sf::Vector2f(direction * overlap_x, 0.f);
    }

    const float direction = (mover_top + mover_bottom) < (blocker_top + blocker_bottom) ? -1.f : 1.f;
    return sf::Vector2f(0.f, direction * overlap_y);
}

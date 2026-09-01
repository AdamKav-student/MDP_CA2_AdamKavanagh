// Adam Kavanagh - D00247069
#include "turret_node.hpp"
#include "utility.hpp"
#include <cmath>

namespace
{
    float WrapDegrees(float degrees)
    {
        degrees = std::fmod(degrees, 360.f);
        if (degrees < 0.f)
        {
            degrees += 360.f;
        }
        return degrees;
    }
}

TurretNode::TurretNode(const sf::Texture& texture, const sf::IntRect& texture_rect, sf::Vector2f pivot)
    : SceneNode()
    , m_sprite(texture, texture_rect)
    , m_texture_rect(texture_rect)
{
    m_sprite.setOrigin(pivot);
    setRotation(sf::degrees(kSpriteForwardOffsetDegrees));
}

void TurretNode::RotateBy(float delta_degrees)
{
    rotate(sf::degrees(delta_degrees));
}

float TurretNode::GetLocalRotationDegrees() const
{
    return WrapDegrees(getRotation().asDegrees() - kSpriteForwardOffsetDegrees);
}

void TurretNode::SetLocalRotationDegrees(float degrees)
{
    setRotation(sf::degrees(WrapDegrees(degrees + kSpriteForwardOffsetDegrees)));
}

float TurretNode::GetBarrelLength() const
{
    // Barrel tip sits at the bottom edge of the unrotated sprite rect, which
    // is this far from the pivot the sprite is drawn around.
    return static_cast<float>(m_texture_rect.size.y) - m_sprite.getOrigin().y;
}

sf::Vector2f TurretNode::GetMuzzleWorldPosition() const
{
    return GetWorldTransform().transformPoint(sf::Vector2f(0.f, GetBarrelLength()));
}

sf::Vector2f TurretNode::GetBarrelDirection() const
{
    // Taking the difference of two transformed points means the hull rotation,
    // the turret rotation and the sprite offset are all folded in for free.
    const sf::Transform world = GetWorldTransform();
    const sf::Vector2f pivot = world.transformPoint(sf::Vector2f(0.f, 0.f));
    const sf::Vector2f tip = world.transformPoint(sf::Vector2f(0.f, GetBarrelLength()));
    return Utility::Normalise(tip - pivot);
}

float TurretNode::GetBarrelRotationDegrees() const
{
    // Angle in the game's convention: 0 degrees points up the screen, growing
    // clockwise, which is what sf::Transformable::setRotation expects.
    const sf::Vector2f direction = GetBarrelDirection();
    return static_cast<float>(Utility::ToDegrees(std::atan2(direction.x, -direction.y)));
}

void TurretNode::DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const
{
    target.draw(m_sprite, states);
}

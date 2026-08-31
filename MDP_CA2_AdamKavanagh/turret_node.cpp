#include "turret_node.hpp"
#include <cmath>

namespace
{
    float WrapDegrees(float degrees)
    {
        degrees = std::fmod(degrees, 360.f);
        if (degrees < 0.f)
            degrees += 360.f;
        return degrees;
    }
}

TurretNode::TurretNode(const sf::Texture& texture, const sf::IntRect& texture_rect, sf::Vector2f pivot)
    : SceneNode()
    , m_sprite(texture, texture_rect)
    , m_texture_rect(texture_rect)
{
    m_sprite.setOrigin(pivot);
    // Base rotation only - corrects for the barrel being drawn pointing
    // down in the sprite. From here on, rotation only changes via
    // RotateBy()/SetLocalRotationDegrees(), i.e. player input.
    setRotation(sf::degrees(kSpriteForwardOffsetDegrees));
}

void TurretNode::RotateBy(float delta_degrees)
{
    rotate(sf::degrees(delta_degrees));
}

float TurretNode::GetLocalRotationDegrees() const
{
    // Reported relative to the hull's forward direction (i.e. with the
    // sprite's built-in 180 degree offset removed), so 0 means "aligned
    // with the hull".
    return WrapDegrees(getRotation().asDegrees() - kSpriteForwardOffsetDegrees);
}

void TurretNode::SetLocalRotationDegrees(float degrees)
{
    setRotation(sf::degrees(WrapDegrees(degrees + kSpriteForwardOffsetDegrees)));
}

sf::Vector2f TurretNode::GetMuzzleWorldPosition() const
{
    // Barrel tip is at the bottom of the (unrotated) sprite rect, i.e. local
    // point (0, height - pivot.y) once the sprite origin is set to the pivot.
    sf::Vector2f local_tip(0.f, static_cast<float>(m_texture_rect.size.y) - m_sprite.getOrigin().y);
    return GetWorldTransform().transformPoint(local_tip);
}

void TurretNode::DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const
{
    target.draw(m_sprite, states);
}

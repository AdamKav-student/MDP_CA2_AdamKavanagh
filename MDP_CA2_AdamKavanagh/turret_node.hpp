// Adam Kavanagh - D00247069
#pragma once
#include "scene_node.hpp"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Rect.hpp>

// The rotating gun mounted on a Tank. It is attached as a child scene node, so
// it inherits the hull's transform automatically; its own rotation is the
// aiming offset relative to the hull.
class TurretNode : public SceneNode
{
public:
    TurretNode(const sf::Texture& texture, const sf::IntRect& texture_rect, sf::Vector2f pivot);

    void RotateBy(float delta_degrees);

    // Aim angle relative to the hull, 0 meaning "gun aligned with the hull".
    // This is the value that gets quantised into a single byte and sent over
    // the network.
    float GetLocalRotationDegrees() const;
    void SetLocalRotationDegrees(float degrees);

    // World-space muzzle position and the direction the barrel points in,
    // both derived from the accumulated scene-graph transform so the hull's
    // own rotation is taken into account.
    sf::Vector2f GetMuzzleWorldPosition() const;
    sf::Vector2f GetBarrelDirection() const;
    float GetBarrelRotationDegrees() const;

private:
    virtual void DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const override;
    float GetBarrelLength() const;

private:
    sf::Sprite  m_sprite;
    sf::IntRect m_texture_rect;

    // The turret art is drawn with the barrel pointing down the sprite, while
    // the game's convention is that 0 degrees faces up the screen. This fixed
    // offset reconciles the two.
    static constexpr float kSpriteForwardOffsetDegrees = 180.f;
};

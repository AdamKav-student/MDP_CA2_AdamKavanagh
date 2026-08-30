#pragma once

#include "scene_node.hpp"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Rect.hpp>

class TurretNode : public SceneNode
{
public:
    TurretNode(const sf::Texture& texture, const sf::IntRect& texture_rect, sf::Vector2f pivot);

    void RotateBy(float delta_degrees);

    float GetLocalRotationDegrees() const;

    void SetLocalRotationDegrees(float degrees);

    sf::Vector2f GetMuzzleWorldPosition() const;

private:
    virtual void DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const override;

private:
    sf::Sprite		m_sprite;
    sf::IntRect		m_texture_rect;

    static constexpr float kSpriteForwardOffsetDegrees = 180.f;
};
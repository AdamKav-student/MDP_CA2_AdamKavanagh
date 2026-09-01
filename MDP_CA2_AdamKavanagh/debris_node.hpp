// Adam Kavanagh - D00247069
#pragma once
#include "scene_node.hpp"
#include "debris_type.hpp"
#include <SFML/Graphics/Sprite.hpp>

// A static, indestructible piece of map cover. Tanks are pushed out of it and
// shells are absorbed by it.
class DebrisNode : public SceneNode
{
public:
    DebrisNode(DebrisType type, const sf::Texture& texture);

    virtual unsigned int GetCategory() const override;
    virtual sf::FloatRect GetBoundingRect() const override;

private:
    virtual void DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const override;

private:
    DebrisType  m_type;
    sf::Sprite  m_sprite;
};

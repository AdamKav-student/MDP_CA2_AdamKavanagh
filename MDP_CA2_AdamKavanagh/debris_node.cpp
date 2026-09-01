// Adam Kavanagh - D00247069
#include "debris_node.hpp"
#include "data_tables.hpp"
#include "utility.hpp"

namespace
{
    const std::vector<DebrisData> Table = InitializeDebrisData();
}

DebrisNode::DebrisNode(DebrisType type, const sf::Texture& texture)
    : SceneNode(ReceiverCategories::kObstacle)
    , m_type(type)
    , m_sprite(texture, Table[static_cast<int>(type)].m_rect)
{
    Utility::CentreOrigin(m_sprite);
}

unsigned int DebrisNode::GetCategory() const
{
    return static_cast<unsigned int>(ReceiverCategories::kObstacle);
}

sf::FloatRect DebrisNode::GetBoundingRect() const
{
    return GetWorldTransform().transformRect(m_sprite.getGlobalBounds());
}

void DebrisNode::DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const
{
    target.draw(m_sprite, states);
}

// Adam Kavanagh - D00247069
#include "projectile.hpp"
#include "data_tables.hpp"
#include "utility.hpp"

namespace
{
    const std::vector<ProjectileData> Table = InitializeProjectileData();
}

Projectile::Projectile(ProjectileType type, const TextureHolder& textures)
    : Entity(1)
    , m_type(type)
    , m_sprite(textures.Get(Table[static_cast<int>(type)].m_texture), Table[static_cast<int>(type)].m_texture_rect)
    , m_owner_identifier(0)
{
    Utility::CentreOrigin(m_sprite);
    m_sprite.setScale(sf::Vector2f(2.f, 2.f));
}

unsigned int Projectile::GetCategory() const
{
    return m_type == ProjectileType::kAlliesShell
        ? static_cast<unsigned int>(ReceiverCategories::kAlliesProjectile)
        : static_cast<unsigned int>(ReceiverCategories::kAxisProjectile);
}

sf::FloatRect Projectile::GetBoundingRect() const
{
    return GetWorldTransform().transformRect(m_sprite.getGlobalBounds());
}

float Projectile::GetMaxSpeed() const
{
    return Table[static_cast<int>(m_type)].m_speed;
}

int Projectile::GetDamage() const
{
    return Table[static_cast<int>(m_type)].m_damage;
}

void Projectile::SetOwnerIdentifier(uint8_t identifier)
{
    m_owner_identifier = identifier;
}

uint8_t Projectile::GetOwnerIdentifier() const
{
    return m_owner_identifier;
}

void Projectile::DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const
{
    target.draw(m_sprite, states);
}

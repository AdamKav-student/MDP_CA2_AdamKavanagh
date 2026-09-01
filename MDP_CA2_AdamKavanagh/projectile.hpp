// Adam Kavanagh - D00247069
#pragma once
#include "entity.hpp"
#include "resource_identifiers.hpp"
#include "projectile_type.hpp"

// A tank shell. Shells are simulated locally on every client from the moment
// the firing event arrives - they are never individually replicated, which is
// what keeps the bandwidth flat as the player count grows (see
// DOCUMENTATION.md).
class Projectile : public Entity
{
public:
    Projectile(ProjectileType type, const TextureHolder& textures);

    virtual unsigned int GetCategory() const override;
    virtual sf::FloatRect GetBoundingRect() const override;

    float GetMaxSpeed() const;
    int GetDamage() const;

    // Identifier of the tank that fired this shell, so a kill can be
    // attributed to the right player and self-damage ignored.
    void SetOwnerIdentifier(uint8_t identifier);
    uint8_t GetOwnerIdentifier() const;

private:
    virtual void DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const override;

private:
    ProjectileType  m_type;
    sf::Sprite      m_sprite;
    uint8_t         m_owner_identifier;
};

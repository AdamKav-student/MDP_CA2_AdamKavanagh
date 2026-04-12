#pragma once
#include "entity.hpp"
#include "resource_identifiers.hpp"
#include "projectile_type.hpp"

class Projectile : public Entity
{
public:
	Projectile(ProjectileType type, const TextureHolder& textures);
	void GuideTowards(sf::Vector2f position);
	bool IsGuided() const;

	virtual unsigned int GetCategory() const override;
	virtual sf::FloatRect GetBoundingRect() const override;

	float GetMaxSpeed() const;
	float GetDamage() const;

private:
	virtual void UpdateCurrent(sf::Time dt, CommandQueue& commands) override;
	virtual void DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const override;

private:
	ProjectileType m_type;
	sf::Sprite m_sprite;
	sf::Vector2f m_target_direction;
};


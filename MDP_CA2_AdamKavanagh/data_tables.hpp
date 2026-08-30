#pragma once
#include "texture_id.hpp"
#include "tank_type.hpp"
#include "debris_type.hpp"
#include <SFML/System/Time.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Vector2.hpp>
#include <vector>

// Replaces AircraftData with TankData; ProjectileData/ParticleData kept 

struct TankData
{
    int				m_hitpoints;
    float			m_speed;				// pixels/second
    float			m_turret_rotate_speed;	// degrees/second, arrow-key turret aiming
    sf::Time		m_fire_interval;		// minimum time between shots

    TextureID		m_texture;
    sf::IntRect		m_hull_rect;
    sf::IntRect		m_turret_rect;
    sf::Vector2f	m_turret_pivot;			// origin within m_turret_rect, local pixel coords

    TeamID			m_team;
};

struct DebrisData
{
    TextureID	m_texture;
    sf::IntRect	m_rect;
    bool		m_blocks_movement;
};

struct ProjectileData
{
    int			m_damage;
    float		m_speed;
    TextureID	m_texture;
    sf::IntRect	m_texture_rect;
};

struct ParticleData
{
    sf::Color	m_color;
    sf::Time	m_lifetime;
};

std::vector<TankData> InitializeTankData();
std::vector<DebrisData> InitializeDebrisData();
std::vector<ProjectileData> InitializeProjectileData();
std::vector<ParticleData> InitializeParticleData();

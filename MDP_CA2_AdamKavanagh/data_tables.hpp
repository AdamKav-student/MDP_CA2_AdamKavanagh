// Adam Kavanagh - D00247069
#pragma once
#include "texture_id.hpp"
#include "tank_type.hpp"
#include "debris_type.hpp"
#include "projectile_type.hpp"
#include "particletype.hpp"
#include <SFML/System/Time.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Vector2.hpp>
#include <vector>

struct TankData
{
    int             m_hitpoints;
    float           m_speed;                // pixels/second, forward
    float           m_reverse_factor;       // multiplier applied when reversing
    float           m_hull_rotate_speed;    // degrees/second
    float           m_turret_rotate_speed;  // degrees/second
    sf::Time        m_fire_interval;        // minimum time between shots

    TextureID       m_texture;
    sf::IntRect     m_hull_rect;
    sf::IntRect     m_turret_rect;
    sf::Vector2f    m_turret_pivot;         // origin inside m_turret_rect, local pixels

    TeamID          m_team;
};

struct DebrisData
{
    TextureID   m_texture;
    sf::IntRect m_rect;
};

struct ProjectileData
{
    int         m_damage;
    float       m_speed;
    TextureID   m_texture;
    sf::IntRect m_texture_rect;
};

struct ParticleData
{
    sf::Color   m_color;
    sf::Time    m_lifetime;
};

std::vector<TankData> InitializeTankData();
std::vector<DebrisData> InitializeDebrisData();
std::vector<ProjectileData> InitializeProjectileData();
std::vector<ParticleData> InitializeParticleData();

#include "data_tables.hpp"
#include "tank_type.hpp"
#include "debris_type.hpp"
#include "projectile_type.hpp"
#include "particletype.hpp"
#include "SFML/Graphics/Rect.hpp"


std::vector<TankData> InitializeTankData()
{
    std::vector<TankData> data(static_cast<int>(TankType::kTankTypeCount));

    // Panzer - Axis. 1st hull in the sheet.
    data[static_cast<int>(TankType::kPanzer)].m_hitpoints = 100;
    data[static_cast<int>(TankType::kPanzer)].m_speed = 120.f;
    data[static_cast<int>(TankType::kPanzer)].m_turret_rotate_speed = 120.f;
    data[static_cast<int>(TankType::kPanzer)].m_fire_interval = sf::seconds(1.5f);
    data[static_cast<int>(TankType::kPanzer)].m_texture = TextureID::kTankSheet;
    data[static_cast<int>(TankType::kPanzer)].m_hull_rect = sf::IntRect({ 1, 2 }, { 74, 132 });
    data[static_cast<int>(TankType::kPanzer)].m_turret_rect = sf::IntRect({ 81, 4 }, { 58, 128 });
    data[static_cast<int>(TankType::kPanzer)].m_turret_pivot = sf::Vector2f(28.5f, 41.6f);
    data[static_cast<int>(TankType::kPanzer)].m_team = TeamID::kAxis;

    // Sherman - Allies. 2nd hull in the sheet.
    data[static_cast<int>(TankType::kSherman)].m_hitpoints = 100;
    data[static_cast<int>(TankType::kSherman)].m_speed = 120.f;
    data[static_cast<int>(TankType::kSherman)].m_turret_rotate_speed = 120.f;
    data[static_cast<int>(TankType::kSherman)].m_fire_interval = sf::seconds(1.5f);
    data[static_cast<int>(TankType::kSherman)].m_texture = TextureID::kTankSheet;
    data[static_cast<int>(TankType::kSherman)].m_hull_rect = sf::IntRect({ 146, 8 }, { 54, 122 });
    data[static_cast<int>(TankType::kSherman)].m_turret_rect = sf::IntRect({ 207, 12 }, { 50, 103 });
    data[static_cast<int>(TankType::kSherman)].m_turret_pivot = sf::Vector2f(24.5f, 33.5f);
    data[static_cast<int>(TankType::kSherman)].m_team = TeamID::kAllies;

    // Reserved - unused for now. 3rd/green hull in the sheet.
    data[static_cast<int>(TankType::kReserved)].m_hitpoints = 100;
    data[static_cast<int>(TankType::kReserved)].m_speed = 120.f;
    data[static_cast<int>(TankType::kReserved)].m_turret_rotate_speed = 120.f;
    data[static_cast<int>(TankType::kReserved)].m_fire_interval = sf::seconds(1.5f);
    data[static_cast<int>(TankType::kReserved)].m_texture = TextureID::kTankSheet;
    data[static_cast<int>(TankType::kReserved)].m_hull_rect = sf::IntRect({ 265, 12 }, { 51, 117 });
    data[static_cast<int>(TankType::kReserved)].m_turret_rect = sf::IntRect({ 320, 12 }, { 31, 79 });
    data[static_cast<int>(TankType::kReserved)].m_turret_pivot = sf::Vector2f(15.0f, 24.5f);
    data[static_cast<int>(TankType::kReserved)].m_team = TeamID::kNone;

    return data;
}

std::vector<DebrisData> InitializeDebrisData()
{
    std::vector<DebrisData> data(static_cast<int>(DebrisType::kDebrisTypeCount));

    data[static_cast<int>(DebrisType::kAmmoCrate)].m_texture = TextureID::kTankSheet;
    data[static_cast<int>(DebrisType::kAmmoCrate)].m_rect = sf::IntRect({ 4, 139 }, { 93, 51 });
    data[static_cast<int>(DebrisType::kAmmoCrate)].m_blocks_movement = true;

    data[static_cast<int>(DebrisType::kDeadTree)].m_texture = TextureID::kTankSheet;
    data[static_cast<int>(DebrisType::kDeadTree)].m_rect = sf::IntRect({ 102, 136 }, { 93, 75 });
    data[static_cast<int>(DebrisType::kDeadTree)].m_blocks_movement = true;

    data[static_cast<int>(DebrisType::kBrokenFence)].m_texture = TextureID::kTankSheet;
    data[static_cast<int>(DebrisType::kBrokenFence)].m_rect = sf::IntRect({ 199, 137 }, { 95, 43 });
    data[static_cast<int>(DebrisType::kBrokenFence)].m_blocks_movement = true;

    data[static_cast<int>(DebrisType::kWheelWreck)].m_texture = TextureID::kTankSheet;
    data[static_cast<int>(DebrisType::kWheelWreck)].m_rect = sf::IntRect({ 247, 181 }, { 116, 90 });
    data[static_cast<int>(DebrisType::kWheelWreck)].m_blocks_movement = true;

    return data;
}

std::vector<ProjectileData> InitializeProjectileData()
{
    std::vector<ProjectileData> data(static_cast<int>(ProjectileType::kProjectileCount));

    // Both teams' shells share the same stats/sprite; only the type (and
    // therefore GetCategory()) differs, which is what filters friendly fire.
    // Still reusing the bullet sprite from kEntities/Entities.png; give it
    // its own region on TankSheet.png later if you want a shell-specific look.
    data[static_cast<int>(ProjectileType::kAxisShell)].m_damage = 25;
    data[static_cast<int>(ProjectileType::kAxisShell)].m_speed = 300.f;
    data[static_cast<int>(ProjectileType::kAxisShell)].m_texture = TextureID::kEntities;
    data[static_cast<int>(ProjectileType::kAxisShell)].m_texture_rect = sf::IntRect({ 175, 64 }, { 3, 14 });

    data[static_cast<int>(ProjectileType::kAlliesShell)].m_damage = 25;
    data[static_cast<int>(ProjectileType::kAlliesShell)].m_speed = 300.f;
    data[static_cast<int>(ProjectileType::kAlliesShell)].m_texture = TextureID::kEntities;
    data[static_cast<int>(ProjectileType::kAlliesShell)].m_texture_rect = sf::IntRect({ 175, 64 }, { 3, 14 });

    return data;
}
std::vector<ParticleData> InitializeParticleData()
{
	std::vector<ParticleData> data(static_cast<int>(ParticleType::kParticleCount));

	data[static_cast<int>(ParticleType::kPropellant)].m_color = sf::Color(255, 255, 50);
	data[static_cast<int>(ParticleType::kPropellant)].m_lifetime = sf::seconds(0.5f);

	data[static_cast<int>(ParticleType::kSmoke)].m_color = sf::Color(50, 50, 50);
	data[static_cast<int>(ParticleType::kSmoke)].m_lifetime = sf::seconds(2.5f);
	return data;
}
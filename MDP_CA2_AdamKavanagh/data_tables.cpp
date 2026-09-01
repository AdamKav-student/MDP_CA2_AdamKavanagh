// Adam Kavanagh - D00247069
#include "data_tables.hpp"

// All rects below index into Media/Textures/Sprite-Sheet.png (365 x 274). The
// sheet holds three hull/turret pairs across the top row and the map debris
// underneath. Turret barrels are drawn pointing down in the sheet, which is
// why TurretNode applies a fixed 180 degree offset.

std::vector<TankData> InitializeTankData()
{
    std::vector<TankData> data(static_cast<int>(TankType::kTankTypeCount));

    // Sherman - Allies. Second (yellow) hull in the sheet.
    TankData& sherman = data[static_cast<int>(TankType::kSherman)];
    sherman.m_hitpoints = 100;
    sherman.m_speed = 120.f;
    sherman.m_reverse_factor = 0.6f;
    sherman.m_hull_rotate_speed = 90.f;
    sherman.m_turret_rotate_speed = 120.f;
    sherman.m_fire_interval = sf::seconds(1.5f);
    sherman.m_texture = TextureID::kTankSheet;
    sherman.m_hull_rect = sf::IntRect({ 146, 8 }, { 54, 122 });
    sherman.m_turret_rect = sf::IntRect({ 207, 12 }, { 50, 103 });
    sherman.m_turret_pivot = sf::Vector2f(25.f, 33.5f);
    sherman.m_team = TeamID::kAllies;

    // Panzer - Axis. First (grey) hull in the sheet.
    TankData& panzer = data[static_cast<int>(TankType::kPanzer)];
    panzer.m_hitpoints = 100;
    panzer.m_speed = 115.f;
    panzer.m_reverse_factor = 0.6f;
    panzer.m_hull_rotate_speed = 85.f;
    panzer.m_turret_rotate_speed = 110.f;
    panzer.m_fire_interval = sf::seconds(1.4f);
    panzer.m_texture = TextureID::kTankSheet;
    panzer.m_hull_rect = sf::IntRect({ 1, 2 }, { 74, 132 });
    panzer.m_turret_rect = sf::IntRect({ 81, 4 }, { 58, 128 });
    panzer.m_turret_pivot = sf::Vector2f(29.f, 42.f);
    panzer.m_team = TeamID::kAxis;

    return data;
}

std::vector<DebrisData> InitializeDebrisData()
{
    std::vector<DebrisData> data(static_cast<int>(DebrisType::kDebrisTypeCount));

    data[static_cast<int>(DebrisType::kAmmoCrate)].m_texture = TextureID::kTankSheet;
    data[static_cast<int>(DebrisType::kAmmoCrate)].m_rect = sf::IntRect({ 4, 139 }, { 93, 51 });

    data[static_cast<int>(DebrisType::kDeadTree)].m_texture = TextureID::kTankSheet;
    data[static_cast<int>(DebrisType::kDeadTree)].m_rect = sf::IntRect({ 102, 136 }, { 93, 75 });

    data[static_cast<int>(DebrisType::kBrokenFence)].m_texture = TextureID::kTankSheet;
    data[static_cast<int>(DebrisType::kBrokenFence)].m_rect = sf::IntRect({ 199, 137 }, { 95, 43 });

    data[static_cast<int>(DebrisType::kWheelWreck)].m_texture = TextureID::kTankSheet;
    data[static_cast<int>(DebrisType::kWheelWreck)].m_rect = sf::IntRect({ 247, 181 }, { 116, 90 });

    return data;
}

std::vector<ProjectileData> InitializeProjectileData()
{
    std::vector<ProjectileData> data(static_cast<int>(ProjectileType::kProjectileCount));

    // Both teams fire the same shell; only the type (and therefore the
    // category returned by Projectile::GetCategory) differs, which is what
    // filters friendly fire.
    for (int i = 0; i < static_cast<int>(ProjectileType::kProjectileCount); ++i)
    {
        data[i].m_damage = 25;
        data[i].m_speed = 420.f;
        data[i].m_texture = TextureID::kEntities;
        data[i].m_texture_rect = sf::IntRect({ 175, 64 }, { 3, 14 });
    }

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

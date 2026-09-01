// Adam Kavanagh - D00247069
#include "tank.hpp"
#include "data_tables.hpp"
#include "projectile.hpp"
#include "sound_node.hpp"
#include "utility.hpp"
#include <cmath>
#include <memory>
#include <string>

namespace
{
    const std::vector<TankData> Table = InitializeTankData();

    // The hull art is drawn facing down the sprite sheet - the engine deck is
    // at the top of each hull rect and the driver's plate at the bottom -
    // while the game's convention is that 0 degrees faces up the screen, the
    // way Player's drive commands and TurretNode both assume. Rotating the
    // sprite about its centred origin reconciles the two without touching the
    // tank's own transform, so movement, collision and the turret mount are
    // all unaffected.
    constexpr float kHullSpriteForwardOffsetDegrees = 180.f;
}

Tank::Tank(TankType type, const TextureHolder& textures, const FontHolder& fonts)
    : Entity(Table[static_cast<int>(type)].m_hitpoints)
    , m_type(type)
    , m_identifier(0)
    , m_hull_sprite(textures.Get(Table[static_cast<int>(type)].m_texture), Table[static_cast<int>(type)].m_hull_rect)
    , m_turret(nullptr)
    , m_health_display(nullptr)
    , m_explosion(textures.Get(TextureID::kExplosion))
    , m_is_firing(false)
    , m_fire_countdown(sf::Time::Zero)
    , m_is_marked_for_removal(false)
    , m_show_explosion(true)
    , m_explosion_began(false)
{
    const TankData& data = Table[static_cast<int>(type)];

    Utility::CentreOrigin(m_hull_sprite);
    m_hull_sprite.setRotation(sf::degrees(kHullSpriteForwardOffsetDegrees));

    m_explosion.SetFrameSize(sf::Vector2i(256, 256));
    m_explosion.SetNumFrames(16);
    m_explosion.SetDuration(sf::seconds(1.f));
    Utility::CentreOrigin(m_explosion);

    std::unique_ptr<TurretNode> turret(new TurretNode(
        textures.Get(data.m_texture), data.m_turret_rect, data.m_turret_pivot));
    m_turret = turret.get();
    AttachChild(std::move(turret));

    // The shell is spawned into the scene graph rather than as a child of the
    // tank, so that it keeps flying after the tank that fired it is destroyed.
    m_fire_command.category = static_cast<unsigned int>(ReceiverCategories::kScene);
    m_fire_command.action = [this, &textures](SceneNode& node, sf::Time)
        {
            CreateShell(node, textures);
        };

    std::string health_text = "";
    std::unique_ptr<TextNode> health_display(new TextNode(fonts, health_text));
    // Red rather than white: this label tracks the tank across the whole map,
    // so it cannot rely on a backing plate the way the fixed HUD readouts do,
    // and red separates it from both the pale sand and the dark hedgerows.
    health_display->SetColour(sf::Color(220, 45, 45));
    health_display->setPosition(sf::Vector2f(0.f, 80.f));
    m_health_display = health_display.get();
    AttachChild(std::move(health_display));

    UpdateTexts();
}

unsigned int Tank::GetCategory() const
{
    return GetTeam() == TeamID::kAllies
        ? static_cast<unsigned int>(ReceiverCategories::kAlliesTank)
        : static_cast<unsigned int>(ReceiverCategories::kAxisTank);
}

TeamID Tank::GetTeam() const
{
    return Table[static_cast<int>(m_type)].m_team;
}

TankType Tank::GetTankType() const
{
    return m_type;
}

uint8_t Tank::GetIdentifier() const
{
    return m_identifier;
}

void Tank::SetIdentifier(uint8_t identifier)
{
    m_identifier = identifier;
}

float Tank::GetMaxSpeed() const
{
    return Table[static_cast<int>(m_type)].m_speed;
}

float Tank::GetReverseFactor() const
{
    return Table[static_cast<int>(m_type)].m_reverse_factor;
}

float Tank::GetHullRotateSpeed() const
{
    return Table[static_cast<int>(m_type)].m_hull_rotate_speed;
}

float Tank::GetTurretRotateSpeed() const
{
    return Table[static_cast<int>(m_type)].m_turret_rotate_speed;
}

void Tank::RotateTurretBy(float delta_degrees)
{
    m_turret->RotateBy(delta_degrees);
}

float Tank::GetTurretRotationDegrees() const
{
    return m_turret->GetLocalRotationDegrees();
}

void Tank::SetTurretRotationDegrees(float degrees)
{
    m_turret->SetLocalRotationDegrees(degrees);
}

void Tank::Fire()
{
    // Only latches the intent - the fire-rate cooldown in
    // CheckProjectileLaunch decides whether a shell actually leaves the
    // barrel, so holding the key does not spawn a shell every frame.
    if (Table[static_cast<int>(m_type)].m_fire_interval != sf::Time::Zero)
    {
        m_is_firing = true;
    }
}

void Tank::UpdateTexts()
{
    if (IsDestroyed())
    {
        m_health_display->SetString("");
        return;
    }

    m_health_display->SetString(std::to_string(GetHitPoints()) + " HP");
    // Keep the label upright in world space no matter which way the hull faces.
    m_health_display->setRotation(sf::degrees(-getRotation().asDegrees()));
}

sf::FloatRect Tank::GetBoundingRect() const
{
    return GetWorldTransform().transformRect(m_hull_sprite.getGlobalBounds());
}

bool Tank::IsMarkedForRemoval() const
{
    // Stay in the scene while the explosion plays out, then disappear.
    return IsDestroyed() && (m_explosion.IsFinished() || !m_show_explosion);
}

void Tank::Remove()
{
    Entity::Remove();
    m_show_explosion = false;
}

void Tank::PlayLocalSound(CommandQueue& commands, SoundEffect effect)
{
    sf::Vector2f world_position = GetWorldPosition();

    Command command;
    command.category = static_cast<unsigned int>(ReceiverCategories::kSoundEffect);
    command.action = DerivedAction<SoundNode>(
        [effect, world_position](SoundNode& node, sf::Time)
        {
            node.PlaySound(effect, world_position);
        });

    commands.Push(command);
}

void Tank::DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const
{
    if (IsDestroyed() && m_show_explosion)
    {
        target.draw(m_explosion, states);
    }
    else
    {
        target.draw(m_hull_sprite, states);
    }
}

void Tank::UpdateCurrent(sf::Time dt, CommandQueue& commands)
{
    if (IsDestroyed())
    {
        m_explosion.Update(dt);

        if (!m_explosion_began)
        {
            SoundEffect effect = (Utility::RandomInt(2) == 0) ? SoundEffect::kExplosion1 : SoundEffect::kExplosion2;
            PlayLocalSound(commands, effect);
            m_explosion_began = true;
        }
        return;
    }

    CheckProjectileLaunch(dt, commands);
    Entity::UpdateCurrent(dt, commands);
    UpdateTexts();
}

void Tank::CheckProjectileLaunch(sf::Time dt, CommandQueue& commands)
{
    if (m_fire_countdown > sf::Time::Zero)
    {
        m_fire_countdown -= dt;
    }

    if (m_is_firing && m_fire_countdown <= sf::Time::Zero)
    {
        commands.Push(m_fire_command);
        PlayLocalSound(commands, GetTeam() == TeamID::kAllies ? SoundEffect::kAlliedGunfire : SoundEffect::kEnemyGunfire);
        m_fire_countdown += Table[static_cast<int>(m_type)].m_fire_interval;
    }

    m_is_firing = false;
}

void Tank::CreateShell(SceneNode& node, const TextureHolder& textures) const
{
    ProjectileType type = GetTeam() == TeamID::kAllies ? ProjectileType::kAlliesShell : ProjectileType::kAxisShell;

    std::unique_ptr<Projectile> shell(new Projectile(type, textures));

    // The shell leaves the muzzle travelling along the turret's world
    // direction rather than the hull's, so aiming actually matters.
    shell->setPosition(m_turret->GetMuzzleWorldPosition());
    shell->setRotation(sf::degrees(m_turret->GetBarrelRotationDegrees()));
    shell->SetVelocity(m_turret->GetBarrelDirection() * shell->GetMaxSpeed());
    shell->SetOwnerIdentifier(m_identifier);

    node.AttachChild(std::move(shell));
}

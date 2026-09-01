// Adam Kavanagh - D00247069
#pragma once
#include "entity.hpp"
#include "tank_type.hpp"
#include "resource_identifiers.hpp"
#include "text_node.hpp"
#include "turret_node.hpp"
#include "command_queue.hpp"
#include "animation.hpp"
#include "sound_effect.hpp"
#include <SFML/System/Time.hpp>

// A single tank: an armoured hull that drives, with an independently aimed
// turret attached as a child scene node. One Tank exists on every client for
// every connected player - the locally controlled one is simulated from
// keyboard input, the rest are driven by the state the server relays.
class Tank : public Entity
{
public:
    Tank(TankType type, const TextureHolder& textures, const FontHolder& fonts);

    virtual unsigned int GetCategory() const override;
    TeamID GetTeam() const;
    TankType GetTankType() const;

    uint8_t GetIdentifier() const;
    void SetIdentifier(uint8_t identifier);

    float GetMaxSpeed() const;
    float GetReverseFactor() const;
    float GetHullRotateSpeed() const;
    float GetTurretRotateSpeed() const;

    // Turret aiming. The angle is stored relative to the hull's forward
    // direction: the turret is a child scene node, so it swings round with the
    // hull for free and this offset only changes on player input (or when a
    // remote tank's relayed turret angle is applied).
    void RotateTurretBy(float delta_degrees);
    float GetTurretRotationDegrees() const;
    void SetTurretRotationDegrees(float degrees);

    void Fire();
    void UpdateTexts();

    virtual sf::FloatRect GetBoundingRect() const override;
    virtual bool IsMarkedForRemoval() const override;
    virtual void Remove() override;

    void PlayLocalSound(CommandQueue& commands, SoundEffect effect);

private:
    virtual void DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const override;
    virtual void UpdateCurrent(sf::Time dt, CommandQueue& commands) override;

    void CheckProjectileLaunch(sf::Time dt, CommandQueue& commands);
    void CreateShell(SceneNode& node, const TextureHolder& textures) const;

private:
    TankType        m_type;
    uint8_t         m_identifier;
    sf::Sprite      m_hull_sprite;
    TurretNode*     m_turret;
    TextNode*       m_health_display;
    Animation       m_explosion;

    Command         m_fire_command;
    bool            m_is_firing;
    sf::Time        m_fire_countdown;

    bool            m_is_marked_for_removal;
    bool            m_show_explosion;
    bool            m_explosion_began;
};

#pragma once

#include "entity.hpp"
#include "tank_type.hpp"
#include "resource_identifiers.hpp"
#include "text_node.hpp"
#include "turret_node.hpp"
#include "command_queue.hpp"
#include <SFML/System/Time.hpp>

class tank : public Entity
{
public:
    Tank(TankType type, const TextureHolder& textures, const FontHolder& fonts);

    virtual unsigned int GetCategory() const override;
    TeamID GetTeam() const;

    uint8_t GetIdentifier() const;
    void SetIdentifier(uint8_t identifier);

    float GetMaxSpeed() const;

    void RotateTurretBy(float delta_degrees);
    float GetTurretLocalRotationDegrees() const;
    void SetTurretLocalRotationDegrees(float degrees); 
    sf::Vector2f GetMuzzleWorldPosition() const;

    void Fire();
    bool IsFiring() const;	

    void UpdateTexts();

    sf::FloatRect GetBoundingRect() const override;
 
    bool IsMarkedForRemoval() const override;
    void Remove() override;

private:
    virtual void DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const override;
    virtual void UpdateCurrent(sf::Time dt, CommandQueue& commands) override;

private:
    TankType		m_type;
    uint8_t			m_identifier;
    sf::Sprite		m_hull_sprite;
    TurretNode* m_turret;
    TextNode* m_health_display;

    bool			m_wants_to_fire;
    bool			m_is_firing;
    sf::Time		m_fire_countdown;
    bool			m_show_wreck;
};


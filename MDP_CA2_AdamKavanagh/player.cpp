#include "player.hpp"
#include "tank.hpp"
#include "receiver_categories.hpp"
#include "utility.hpp"
#include <cmath>

namespace
{
    // Rotate/move rates - could be pulled from TankData via the Tank itself
    // if you want per-type differences; kept simple here.
    const float kHullRotateSpeed = 100.f;	// degrees/second
    const float kTurretRotateSpeed = 120.f; // degrees/second, matches TankData default
}

Player::Player()
    : m_movement_keys(0)
    , m_turret_keys(1)
    , m_identifier(0)
{
    InitialiseActions();
}

void Player::InitialiseActions()
{
    uint8_t identifier = m_identifier;

    m_action_binding[Action::kMoveForward].action = DerivedAction<Tank>(
        [](Tank& tank, sf::Time dt)
        {
            float rad = tank.getRotation().asRadians();
            sf::Vector2f forward(std::sin(rad), -std::cos(rad));
            tank.move(forward * tank.GetMaxSpeed() * dt.asSeconds());
        });

    m_action_binding[Action::kMoveBackward].action = DerivedAction<Tank>(
        [](Tank& tank, sf::Time dt)
        {
            float rad = tank.getRotation().asRadians();
            sf::Vector2f forward(std::sin(rad), -std::cos(rad));
            // Reverse at reduced speed, like a real tank
            tank.move(-forward * tank.GetMaxSpeed() * 0.6f * dt.asSeconds());
        });

    m_action_binding[Action::kRotateHullLeft].action = DerivedAction<Tank>(
        [](Tank& tank, sf::Time dt)
        {
            tank.rotate(sf::degrees(-kHullRotateSpeed * dt.asSeconds()));
        });

    m_action_binding[Action::kRotateHullRight].action = DerivedAction<Tank>(
        [](Tank& tank, sf::Time dt)
        {
            tank.rotate(sf::degrees(kHullRotateSpeed * dt.asSeconds()));
        });

    m_action_binding[Action::kTurretLeft].action = DerivedAction<Tank>(
        [](Tank& tank, sf::Time dt)
        {
            tank.RotateTurretBy(-kTurretRotateSpeed * dt.asSeconds());
        });

    m_action_binding[Action::kTurretRight].action = DerivedAction<Tank>(
        [](Tank& tank, sf::Time dt)
        {
            tank.RotateTurretBy(kTurretRotateSpeed * dt.asSeconds());
        });

    m_action_binding[Action::kFire].action = DerivedAction<Tank>(
        [](Tank& tank, sf::Time)
        {
            tank.Fire();
        });

    // Route every action only to the Tank belonging to this Player, and to
    // any tank (either team) since we filter by identifier below.
    for (auto& pair : m_action_binding)
    {
        Command& command = pair.second;
        command.category = static_cast<unsigned int>(ReceiverCategories::kAnyTank);

        auto original_action = command.action;
        command.action = DerivedAction<Tank>(
            [original_action, identifier](Tank& tank, sf::Time dt)
            {
                if (tank.GetIdentifier() == identifier)
                {
                    original_action(tank, dt);
                }
            });
    }
}

void Player::HandleEvent(const sf::Event& event, CommandQueue& commands)
{
    // Reserved for one-shot (press-only) actions if you add any later
    // (e.g. a single-shot fire mode instead of the current hold-to-fire).
    (void)event;
    (void)commands;
}

void Player::HandleRealtimeInput(CommandQueue& commands)
{
    // Poll both key groups every frame and push the bound commands for any
    // action currently held down.
    for (int action_index = 0; action_index < static_cast<int>(Action::kActionCount); ++action_index)
    {
        Action action = static_cast<Action>(action_index);
        if (!IsRealtimeAction(action))
            continue;

        bool is_turret_action = (action == Action::kTurretLeft || action == Action::kTurretRight);
        const KeyBinding& binding = is_turret_action ? m_turret_keys : m_movement_keys;

        sf::Keyboard::Key key = binding.GetAssignedKey(action);
        if (key != sf::Keyboard::Key::Unknown && sf::Keyboard::isKeyPressed(key))
        {
            commands.Push(m_action_binding[action]);
        }
    }
}

void Player::AssignKey(bool turret_group, Action action, sf::Keyboard::Key key)
{
    if (turret_group)
        m_turret_keys.AssignKey(action, key);
    else
        m_movement_keys.AssignKey(action, key);
}

sf::Keyboard::Key Player::GetAssignedKey(bool turret_group, Action action) const
{
    return turret_group ? m_turret_keys.GetAssignedKey(action) : m_movement_keys.GetAssignedKey(action);
}

void Player::SetIdentifier(uint8_t identifier)
{
    m_identifier = identifier;
    InitialiseActions(); // rebuild bindings so the identifier filter captures the new value
}

uint8_t Player::GetIdentifier() const
{
    return m_identifier;
}

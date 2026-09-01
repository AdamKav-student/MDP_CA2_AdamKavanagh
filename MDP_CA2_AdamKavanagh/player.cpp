// Adam Kavanagh - D00247069
#include "player.hpp"
#include "tank.hpp"
#include "network_protocol.hpp"
#include "receiver_categories.hpp"
#include <cmath>

Player::Player(PacketSender* sender, uint8_t identifier, const KeyBinding* binding)
    : m_key_binding(binding)
    , m_identifier(identifier)
    , m_sender(sender)
{
    // Every action starts released. Remote tanks are driven entirely through
    // these proxies.
    for (int i = 0; i < static_cast<int>(Action::kActionCount); ++i)
    {
        m_action_proxies[static_cast<Action>(i)] = false;
    }

    InitialiseActions();
}

void Player::InitialiseActions()
{
    // Driving is done by moving the hull along its own facing rather than by
    // setting a velocity: a tank has no sideways movement, so an "up" key and
    // a "left" key are not independent axes the way they were for the plane
    // this project grew out of.
    m_action_binding[Action::kMoveForward].action = DerivedAction<Tank>(
        [](Tank& tank, sf::Time dt)
        {
            const float radians = tank.getRotation().asRadians();
            const sf::Vector2f forward(std::sin(radians), -std::cos(radians));
            tank.move(forward * tank.GetMaxSpeed() * dt.asSeconds());
        });

    m_action_binding[Action::kMoveBackward].action = DerivedAction<Tank>(
        [](Tank& tank, sf::Time dt)
        {
            const float radians = tank.getRotation().asRadians();
            const sf::Vector2f forward(std::sin(radians), -std::cos(radians));
            tank.move(-forward * tank.GetMaxSpeed() * tank.GetReverseFactor() * dt.asSeconds());
        });

    m_action_binding[Action::kRotateHullLeft].action = DerivedAction<Tank>(
        [](Tank& tank, sf::Time dt)
        {
            tank.rotate(sf::degrees(-tank.GetHullRotateSpeed() * dt.asSeconds()));
        });

    m_action_binding[Action::kRotateHullRight].action = DerivedAction<Tank>(
        [](Tank& tank, sf::Time dt)
        {
            tank.rotate(sf::degrees(tank.GetHullRotateSpeed() * dt.asSeconds()));
        });

    m_action_binding[Action::kTurretLeft].action = DerivedAction<Tank>(
        [](Tank& tank, sf::Time dt)
        {
            tank.RotateTurretBy(-tank.GetTurretRotateSpeed() * dt.asSeconds());
        });

    m_action_binding[Action::kTurretRight].action = DerivedAction<Tank>(
        [](Tank& tank, sf::Time dt)
        {
            tank.RotateTurretBy(tank.GetTurretRotateSpeed() * dt.asSeconds());
        });

    m_action_binding[Action::kFire].action = DerivedAction<Tank>(
        [](Tank& tank, sf::Time)
        {
            tank.Fire();
        });

    // Wrap every action so it only ever reaches the one tank this Player owns.
    const uint8_t identifier = m_identifier;
    for (auto& pair : m_action_binding)
    {
        Command& command = pair.second;
        command.category = static_cast<unsigned int>(ReceiverCategories::kAnyTank);

        auto inner_action = command.action;
        command.action = [inner_action, identifier](SceneNode& node, sf::Time dt)
            {
                if (static_cast<Tank&>(node).GetIdentifier() == identifier)
                {
                    inner_action(node, dt);
                }
            };
    }
}

void Player::HandleEvent(const sf::Event& event, CommandQueue& commands)
{
    if (!IsLocal())
    {
        return;
    }

    // A held key going down or coming up is the only thing worth sending: the
    // server relays the transition and every other client keeps applying the
    // action locally until the matching release arrives. Sending the key state
    // every frame instead would multiply this traffic by 60.
    Action action;

    if (const auto* key_pressed = event.getIf<sf::Event::KeyPressed>())
    {
        if (m_key_binding->CheckAction(key_pressed->scancode, action) && IsRealtimeAction(action))
        {
            sf::Packet packet;
            packet << static_cast<uint8_t>(Client::PacketType::kPlayerRealtimeChange);
            packet << m_identifier;
            packet << static_cast<uint8_t>(action);
            packet << true;
            if (m_sender)
            {
                m_sender->SendPacket(packet);
            }
        }
    }
    else if (const auto* key_released = event.getIf<sf::Event::KeyReleased>())
    {
        if (m_key_binding->CheckAction(key_released->scancode, action) && IsRealtimeAction(action))
        {
            sf::Packet packet;
            packet << static_cast<uint8_t>(Client::PacketType::kPlayerRealtimeChange);
            packet << m_identifier;
            packet << static_cast<uint8_t>(action);
            packet << false;
            if (m_sender)
            {
                m_sender->SendPacket(packet);
            }
        }
    }
}

void Player::HandleRealtimeInput(CommandQueue& commands)
{
    if (!IsLocal())
    {
        return;
    }

    // The local tank is moved immediately from the keyboard rather than
    // waiting for the server to echo the input back - without this the tank
    // you are driving would lag behind your own key presses by a round trip.
    std::vector<Action> active_actions;
    m_key_binding->GetRealtimeActions(active_actions);

    for (Action action : active_actions)
    {
        commands.Push(m_action_binding[action]);
    }
}

void Player::HandleRealtimeNetworkInput(CommandQueue& commands)
{
    if (IsLocal())
    {
        return;
    }

    for (const auto& pair : m_action_proxies)
    {
        if (pair.second && IsRealtimeAction(pair.first))
        {
            commands.Push(m_action_binding[pair.first]);
        }
    }
}

void Player::HandleNetworkEvent(Action action, CommandQueue& commands)
{
    commands.Push(m_action_binding[action]);
}

void Player::HandleNetworkRealtimeChange(Action action, bool action_enabled)
{
    m_action_proxies[action] = action_enabled;
}

void Player::DisableAllRealtimeActions()
{
    // Called when the window loses focus or the game is paused, so a tank is
    // not left driving into a wall while nobody is looking. The releases are
    // pushed to the server too, otherwise every other client would keep
    // driving this tank forwards.
    for (auto& pair : m_action_proxies)
    {
        pair.second = false;

        if (IsLocal() && m_sender)
        {
            sf::Packet packet;
            packet << static_cast<uint8_t>(Client::PacketType::kPlayerRealtimeChange);
            packet << m_identifier;
            packet << static_cast<uint8_t>(pair.first);
            packet << false;
            m_sender->SendPacket(packet);
        }
    }
}

bool Player::IsLocal() const
{
    return m_key_binding != nullptr;
}

uint8_t Player::GetIdentifier() const
{
    return m_identifier;
}

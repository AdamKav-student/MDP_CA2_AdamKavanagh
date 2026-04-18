#include "player.hpp"
#include "command_queue.hpp"
#include "aircraft.hpp"

#include "network_protocol.hpp"
#include <SFML/Network/Packet.hpp>

#include "utility.hpp"

#include <map>

struct AircraftMover
{
    AircraftMover(float dir, int identifier)
        : direction(dir)
        , aircraft_id(identifier)
    {}

    void operator()(Aircraft& aircraft, sf::Time) const
    {
        if (aircraft.GetIdentifier() == aircraft_id)
        {
            // Compute heading from aircraft rotation and accelerate in that direction
            float radians = aircraft.getRotation().asRadians();
            // Adjust so that rotation 0 points upwards (sfml rotation 0 -> up)
            double adj = radians - Utility::ToRadians(90.f);
            sf::Vector2f heading(static_cast<float>(std::cos(adj)), static_cast<float>(std::sin(adj)));
            aircraft.Accelerate(heading * aircraft.GetMaxSpeed() * direction);
        }
    }

    float direction;
    int aircraft_id;
};

struct AircraftRotator
{
    AircraftRotator(float angularVel, int identifier)
        : angular_velocity(angularVel)
        , aircraft_id(identifier)
    {}

    void operator()(Aircraft& aircraft, sf::Time dt) const
    {
        if (aircraft.GetIdentifier() == aircraft_id)
        {
            // Rotate by angular velocity (degrees per second)
            aircraft.rotate(sf::degrees(angular_velocity * dt.asSeconds()));
        }
    }

    float angular_velocity; // degrees per second
    int aircraft_id;
};

struct AircraftFireTrigger
{
    AircraftFireTrigger(int identifier)
        : aircraft_id(identifier)
    {
    }

    void operator() (Aircraft& aircraft, sf::Time) const
    {
        if (aircraft.GetIdentifier() == aircraft_id)
            aircraft.Fire();
    }

    int aircraft_id;
};

struct AircraftMissileTrigger
{
    AircraftMissileTrigger(int identifier)
        : aircraft_id(identifier)
    {
    }

    void operator() (Aircraft& aircraft, sf::Time) const
    {
        if (aircraft.GetIdentifier() == aircraft_id)
            aircraft.LaunchMissile();
    }

    int aircraft_id;
};

Player::Player(sf::TcpSocket* socket, uint8_t identifier, const KeyBinding* binding)
    : m_key_binding(binding)
    , m_current_mission_status(MissionStatus::kMissionRunning)
    , m_identifier(identifier)
    , m_socket(socket)
{
    InitialiseActions();

    for (auto& pair : m_action_binding)
    {
        pair.second.category = static_cast<unsigned int>(ReceiverCategories::kPlayerAircraft);
    }
}

void Player::HandleEvent(const sf::Event& event, CommandQueue& command_queue)
{
    const auto* key_pressed = event.getIf<sf::Event::KeyPressed>();
    if (key_pressed)
    {
        Action action;
        if (m_key_binding && m_key_binding->CheckAction(key_pressed->scancode, action) && !IsRealtimeAction(action))
        {
            // Network connected -> send event over network
            if (m_socket)
            {
                sf::Packet packet;
                packet << static_cast<uint8_t>(Client::PacketType::kPlayerEvent);
                packet << m_identifier;
                packet << static_cast<uint8_t>(action);
                m_socket->send(packet);
            }

            // Network disconnected -> local event
            else
            {
                command_queue.Push(m_action_binding[action]);
            }
        }
    }

    struct KeyStatus {
        sf::Keyboard::Scancode code;
        bool isPressed;
    };

    std::optional<KeyStatus> keyData;
    if (const auto* press = event.getIf<sf::Event::KeyPressed>())
        keyData = { press->scancode, true };
    else if (const auto* release = event.getIf<sf::Event::KeyReleased>())
        keyData = { release->scancode, false };

    // Realtime change (network connected)
    if (keyData && m_socket)
    {
        Action action;
        if (m_key_binding && m_key_binding->CheckAction(keyData->code, action) && IsRealtimeAction(action))
        {
            // Send realtime change over network
            sf::Packet packet;
            packet << static_cast<uint8_t>(Client::PacketType::kPlayerRealtimeChange);
            packet << m_identifier;
            packet << static_cast<uint8_t>(action);
            packet << keyData->isPressed;
            m_socket->send(packet);
        }
    }
}

bool Player::IsLocal() const
{
    // No key binding means this player is remote
    return m_key_binding != nullptr;
}

void Player::DisableAllRealtimeActions(bool enable)
{
    for (auto& action : m_action_proxies)
    {
        sf::Packet packet;
        packet << static_cast<uint8_t>(Client::PacketType::kPlayerRealtimeChange);
        packet << m_identifier;
        packet << static_cast<uint8_t>(action.first);
        packet << enable;
        m_socket->send(packet);
    }
}

void Player::HandleRealTimeInput(CommandQueue& command_queue)
{
    // Check if this is a networked game and local player or just a single player game
    if ((m_socket && IsLocal()) || !m_socket)
    {
        // Lookup all actions and push corresponding commands to queue
        std::vector<Action> activeActions = m_key_binding->GetRealtimeActions();
        for (Action action : activeActions)
            command_queue.Push(m_action_binding[action]);
    }
}

void Player::HandleRealtimeNetworkInput(CommandQueue& commands)
{
    if (m_socket && !IsLocal())
    {
        // Traverse all realtime input proxies. Because this is a networked game, the input isn't handled directly
        for (auto pair : m_action_proxies)
        {
            if (pair.second && IsRealtimeAction(pair.first))
                commands.Push(m_action_binding[pair.first]);
        }
    }
}

void Player::HandleNetworkEvent(Action action, CommandQueue& commands)
{
    commands.Push(m_action_binding[action]);
}

void Player::HandleNetworkRealtimeChange(Action action, bool actionEnabled)
{
    m_action_proxies[action] = actionEnabled;
}


void Player::SetMissionStatus(MissionStatus status)
{
    m_current_mission_status = status;
}

MissionStatus Player::GetMissionStatus() const
{
    return m_current_mission_status;
}

void Player::InitialiseActions()
{
    // Rotate left / right with A / D
    m_action_binding[Action::kMoveLeft].action = DerivedAction<Aircraft>(AircraftRotator(-180.f, m_identifier));
    m_action_binding[Action::kMoveRight].action = DerivedAction<Aircraft>(AircraftRotator(+180.f, m_identifier));
    // Move forward / backward with W / S (direction 1 = forward, -1 = backward)
    m_action_binding[Action::kMoveUp].action = DerivedAction<Aircraft>(AircraftMover(1.f, m_identifier));
    m_action_binding[Action::kMoveDown].action = DerivedAction<Aircraft>(AircraftMover(-1.f, m_identifier));
    m_action_binding[Action::kBulletFire].action = DerivedAction<Aircraft>(AircraftFireTrigger(m_identifier));
    m_action_binding[Action::kMissileFire].action = DerivedAction<Aircraft>(AircraftMissileTrigger(m_identifier));
}

#pragma once
#include "action.hpp"
#include "key_binding.hpp"
#include "command_queue.hpp"
#include <map>
#include "SFML/Window/Event.hpp"

// Represents one local/remote client controlling one Tank. Local co-op has
// been removed: m_movement_keys and m_turret_keys both belong to the SAME
// player now (movement/fire vs turret rotation), rather than two separate
// players sharing a keyboard.
class Player
{
public:
    Player();

    void HandleEvent(const sf::Event& event, CommandQueue& commands);
    void HandleRealtimeInput(CommandQueue& commands);

    void AssignKey(bool turret_group, Action action, sf::Keyboard::Key key);
    sf::Keyboard::Key GetAssignedKey(bool turret_group, Action action) const;

    void SetIdentifier(uint8_t identifier);
    uint8_t GetIdentifier() const;

private:
    void InitialiseActions();

private:
    KeyBinding					m_movement_keys;	// forward/back, hull rotate, fire
    KeyBinding					m_turret_keys;		// turret rotate left/right
    std::map<Action, Command>	m_action_binding;
    uint8_t						m_identifier;
};

// Adam Kavanagh - D00247069
#pragma once
#include "action.hpp"
#include <SFML/Window/Keyboard.hpp>
#include <map>
#include <vector>

// Keyboard layout for the single local player. Scancodes are used rather than
// key codes so the bindings follow the physical keys regardless of the
// keyboard layout the machine is set to.
class KeyBinding
{
public:
    KeyBinding();

    void AssignKey(Action action, sf::Keyboard::Scancode key);
    sf::Keyboard::Scancode GetAssignedKey(Action action) const;

    bool CheckAction(sf::Keyboard::Scancode key, Action& out_action) const;
    void GetRealtimeActions(std::vector<Action>& out_actions) const;

private:
    std::map<sf::Keyboard::Scancode, Action> m_key_map;
};

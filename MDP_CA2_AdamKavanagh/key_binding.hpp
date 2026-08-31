#pragma once
#include "action.hpp"
#include <SFML/Window/Keyboard.hpp>
#include <map>

// One KeyBinding instance now represents one *control group* for a single
// local player (since local co-op has been removed): a client creates two
// of these - one for hull movement/fire, one for turret rotation - so the
// existing per-player rebinding UI (two columns) can be reused as
// "Movement" / "Turret" instead of "Player 1" / "Player 2".
class KeyBinding
{
public:
    explicit KeyBinding(int control_preset); // 0 = movement defaults, 1 = turret defaults

    void AssignKey(Action action, sf::Keyboard::Key key);
    sf::Keyboard::Key GetAssignedKey(Action action) const;

    bool CheckAction(sf::Keyboard::Key key, Action& out_action) const;
    bool IsRealtimeAction(Action action) const;

private:
    std::map<sf::Keyboard::Key, Action> m_key_map;
};

#include "key_binding.hpp"

KeyBinding::KeyBinding(int control_preset)
{
    if (control_preset == 0)
    {
        // Movement / fire defaults
        m_key_map[sf::Keyboard::Key::W] = Action::kMoveForward;
        m_key_map[sf::Keyboard::Key::S] = Action::kMoveBackward;
        m_key_map[sf::Keyboard::Key::A] = Action::kRotateHullLeft;
        m_key_map[sf::Keyboard::Key::D] = Action::kRotateHullRight;
        m_key_map[sf::Keyboard::Key::Space] = Action::kFire;
    }
    else
    {
        // Turret defaults
        m_key_map[sf::Keyboard::Key::Left] = Action::kTurretLeft;
        m_key_map[sf::Keyboard::Key::Right] = Action::kTurretRight;
    }
}

void KeyBinding::AssignKey(Action action, sf::Keyboard::Key key)
{
    // Remove any existing binding of this key (avoid one key firing two actions)
    for (auto it = m_key_map.begin(); it != m_key_map.end(); )
    {
        if (it->second == action)
            it = m_key_map.erase(it);
        else
            ++it;
    }
    m_key_map[key] = action;
}

sf::Keyboard::Key KeyBinding::GetAssignedKey(Action action) const
{
    for (const auto& pair : m_key_map)
    {
        if (pair.second == action)
            return pair.first;
    }
    return sf::Keyboard::Key::Unknown;
}

bool KeyBinding::CheckAction(sf::Keyboard::Key key, Action& out_action) const
{
    auto found = m_key_map.find(key);
    if (found == m_key_map.end())
        return false;

    out_action = found->second;
    return true;
}

bool KeyBinding::IsRealtimeAction(Action action) const
{
    return ::IsRealtimeAction(action);
}

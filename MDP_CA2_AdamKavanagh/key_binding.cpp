// Adam Kavanagh - D00247069
#include "key_binding.hpp"

KeyBinding::KeyBinding()
{
    // WASD drives the hull, the arrow keys traverse the turret independently,
    // and space fires. Driving and aiming being on separate hands is what
    // makes a turret worth having.
    m_key_map[sf::Keyboard::Scancode::W] = Action::kMoveForward;
    m_key_map[sf::Keyboard::Scancode::S] = Action::kMoveBackward;
    m_key_map[sf::Keyboard::Scancode::A] = Action::kRotateHullLeft;
    m_key_map[sf::Keyboard::Scancode::D] = Action::kRotateHullRight;
    m_key_map[sf::Keyboard::Scancode::Left] = Action::kTurretLeft;
    m_key_map[sf::Keyboard::Scancode::Right] = Action::kTurretRight;
    m_key_map[sf::Keyboard::Scancode::Space] = Action::kFire;
}

void KeyBinding::AssignKey(Action action, sf::Keyboard::Scancode key)
{
    // Drop any previous binding for this action so a key is never left
    // triggering two things at once.
    for (auto itr = m_key_map.begin(); itr != m_key_map.end(); )
    {
        if (itr->second == action)
        {
            itr = m_key_map.erase(itr);
        }
        else
        {
            ++itr;
        }
    }
    m_key_map[key] = action;
}

sf::Keyboard::Scancode KeyBinding::GetAssignedKey(Action action) const
{
    for (const auto& pair : m_key_map)
    {
        if (pair.second == action)
        {
            return pair.first;
        }
    }
    return sf::Keyboard::Scancode::Unknown;
}

bool KeyBinding::CheckAction(sf::Keyboard::Scancode key, Action& out_action) const
{
    auto found = m_key_map.find(key);
    if (found == m_key_map.end())
    {
        return false;
    }

    out_action = found->second;
    return true;
}

void KeyBinding::GetRealtimeActions(std::vector<Action>& out_actions) const
{
    for (const auto& pair : m_key_map)
    {
        if (sf::Keyboard::isKeyPressed(pair.first) && IsRealtimeAction(pair.second))
        {
            out_actions.push_back(pair.second);
        }
    }
}

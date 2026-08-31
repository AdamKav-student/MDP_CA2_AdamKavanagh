#pragma once
enum class Action
{
    kMoveForward,
    kMoveBackward,
    kRotateHullLeft,
    kRotateHullRight,
    kTurretLeft,
    kTurretRight,
    kFire,
    kActionCount
};

// True for actions that should be applied continuously while the key is
// held (polled every frame via KeyBinding::IsRealtimeAction).
inline bool IsRealtimeAction(Action action)
{
    switch (action)
    {
    case Action::kMoveForward:
    case Action::kMoveBackward:
    case Action::kRotateHullLeft:
    case Action::kRotateHullRight:
    case Action::kTurretLeft:
    case Action::kTurretRight:
    case Action::kFire:
        return true;
    default:
        return false;
    }
}
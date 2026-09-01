// Adam Kavanagh - D00247069
#pragma once

// One action per thing a tank crew can do. The values are sent over the
// network as a single byte (see network_protocol.hpp), so the order matters:
// never reorder without bumping both ends.
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

// Every tank action is continuous ("held down") rather than one-shot. Keeping
// this predicate means the realtime/event split in Player and in the protocol
// still works if a one-shot action is added later.
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

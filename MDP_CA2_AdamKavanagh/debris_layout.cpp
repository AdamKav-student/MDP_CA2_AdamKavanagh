// Adam Kavanagh - D00247069
#include "debris_layout.hpp"

namespace
{
    const std::vector<DebrisPlacement> Layout =
    {
        // Centre strongpoint - the contested ground both teams push towards.
        { DebrisType::kWheelWreck,  { 0.50f, 0.50f },   0.f },
        { DebrisType::kBrokenFence, { 0.42f, 0.44f },  20.f },
        { DebrisType::kBrokenFence, { 0.58f, 0.56f }, 200.f },
        { DebrisType::kDeadTree,    { 0.50f, 0.38f },   0.f },
        { DebrisType::kDeadTree,    { 0.50f, 0.62f },  35.f },

        // Northern (Axis) half.
        { DebrisType::kAmmoCrate,   { 0.22f, 0.20f },  10.f },
        { DebrisType::kDeadTree,    { 0.33f, 0.15f },  70.f },
        { DebrisType::kBrokenFence, { 0.68f, 0.18f },  90.f },
        { DebrisType::kWheelWreck,  { 0.80f, 0.26f }, 140.f },
        { DebrisType::kDeadTree,    { 0.62f, 0.30f },  15.f },

        // Southern (Allied) half.
        { DebrisType::kAmmoCrate,   { 0.78f, 0.80f }, 190.f },
        { DebrisType::kDeadTree,    { 0.67f, 0.85f }, 250.f },
        { DebrisType::kBrokenFence, { 0.32f, 0.82f }, 270.f },
        { DebrisType::kWheelWreck,  { 0.20f, 0.74f } , 320.f },
        { DebrisType::kDeadTree,    { 0.38f, 0.70f }, 195.f },

        // Flank cover so the map edges are not open runs.
        { DebrisType::kAmmoCrate,   { 0.08f, 0.48f },  90.f },
        { DebrisType::kAmmoCrate,   { 0.92f, 0.52f },  90.f },
        { DebrisType::kBrokenFence, { 0.12f, 0.34f },   0.f },
        { DebrisType::kBrokenFence, { 0.88f, 0.66f },   0.f },
    };
}

const std::vector<DebrisPlacement>& GetDebrisLayout()
{
    return Layout;
}

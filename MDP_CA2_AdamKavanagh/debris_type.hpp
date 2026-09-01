// Adam Kavanagh - D00247069
#pragma once

// Static map obstacles cut from Media/Textures/Sprite-Sheet.png. They block
// tanks and stop shells, and are laid out identically on every client from the
// fixed table in debris_layout.cpp, so no obstacle data is ever sent over the
// network.
enum class DebrisType
{
    kAmmoCrate,
    kDeadTree,
    kBrokenFence,
    kWheelWreck,
    kDebrisTypeCount
};

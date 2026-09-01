// Adam Kavanagh - D00247069
#pragma once

// Shells are typed by the team that fired them; that is what stops friendly
// fire, since collision matching is done on the team-tagged categories in
// receiver_categories.hpp.
enum class ProjectileType
{
    kAlliesShell,
    kAxisShell,
    kProjectileCount
};

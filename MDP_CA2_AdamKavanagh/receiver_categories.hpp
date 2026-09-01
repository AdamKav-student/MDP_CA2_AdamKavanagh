// Adam Kavanagh - D00247069
#pragma once

// Command routing / collision categories. Tanks and shells are tagged by team
// so that HandleCollisions() can express "an Axis shell hit an Allied tank"
// without downcasting and comparing teams by hand.
enum class ReceiverCategories
{
    kNone = 0,
    kScene = 1 << 0,
    kParticleSystem = 1 << 1,
    kSoundEffect = 1 << 2,
    kNetwork = 1 << 3,
    kObstacle = 1 << 4,

    kAlliesTank = 1 << 5,
    kAxisTank = 1 << 6,
    kAlliesProjectile = 1 << 7,
    kAxisProjectile = 1 << 8,

    kAnyTank = kAlliesTank | kAxisTank,
    kAnyProjectile = kAlliesProjectile | kAxisProjectile
};

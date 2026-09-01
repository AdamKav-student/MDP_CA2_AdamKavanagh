// Adam Kavanagh - D00247069
#pragma once
enum class TextureID
{
    // Menus / UI
    kTitleScreen,
    kSettingsScreen,
    kVictoryScreen,
    kDefeatScreen,
    kButtons,

    // In-game
    kTankSheet,     // hulls, turrets and debris - Media/Textures/Sprite-Sheet.png
    kBattlefield,   // Media/Textures/Road to Caen.png
    kEntities,      // shell sprite
    kExplosion,
    kParticle,

    kNumTextures,
};

// Adam Kavanagh - D00247069
#pragma once
#include <SFML/Graphics.hpp>
#include "resource_identifiers.hpp"
#include "scene_node.hpp"
#include "scene_layers.hpp"
#include "tank.hpp"
#include "tank_type.hpp"
#include "command_queue.hpp"
#include "bloom_effect.hpp"
#include "sound_player.hpp"
#include "sprite_node.hpp"
#include "network_node.hpp"
#include <array>

// The battlefield. The same class serves both game modes: constructed with
// networked = false it builds the single-tank training map, and with
// networked = true it builds an empty arena that tanks are added to as
// players connect.
class World
{
public:
    explicit World(sf::RenderTarget& output_target, FontHolder& font, SoundPlayer& sounds, bool networked = false);

    void Update(sf::Time dt);
    void Draw();

    sf::FloatRect GetViewBounds() const;
    sf::FloatRect GetWorldBounds() const;
    CommandQueue& GetCommandQueue();

    Tank* AddTank(uint8_t identifier);
    void RemoveTank(uint8_t identifier);
    Tank* GetTank(uint8_t identifier) const;

    // Tells the world which tank the camera should follow and, in multiplayer,
    // which tank this machine is allowed to apply damage to.
    void SetLocalPlayerIdentifier(uint8_t identifier);
    uint8_t GetLocalPlayerIdentifier() const;

    // Training-mode outcomes. Only meaningful when networked == false.
    bool IsTutorialComplete() const;
    bool IsTutorialPlayerDestroyed() const;

    bool PollGameAction(GameActions::Action& out);

private:
    void LoadTextures();
    void BuildScene();
    void BuildTrainingScene();
    void BuildMultiplayerScene();
    void AddBattlefieldBackground();
    void AddDebris();

    void KeepTanksInsideWorld();
    void HandleCollisions();
    void DestroyProjectilesOutsideWorld();
    void UpdateCamera();
    void UpdateSounds();

    // In multiplayer only the client that owns a tank is allowed to reduce its
    // hitpoints; every other client just watches the relayed value. That is
    // what stops the same shell being counted several times.
    bool IsAuthoritativeFor(const Tank& tank) const;

private:
    sf::RenderTarget&   m_target;
    sf::RenderTexture   m_scene_texture;
    sf::View            m_camera;
    TextureHolder       m_textures;
    FontHolder&         m_fonts;
    SoundPlayer&        m_sounds;

    SceneNode           m_scene_graph;
    std::array<SceneNode*, static_cast<int>(SceneLayers::kLayerCount)> m_scene_layers;
    sf::FloatRect       m_world_bounds;
    sf::Vector2f        m_spawn_position;

    CommandQueue        m_command_queue;
    std::vector<Tank*>  m_tanks;

    Tank*               m_training_enemy;
    bool                m_training_enemy_destroyed;
    bool                m_training_player_destroyed;

    uint8_t             m_local_player_identifier;

    BloomEffect         m_bloom_effect;
    bool                m_networked_world;
    NetworkNode*        m_network_node;
};

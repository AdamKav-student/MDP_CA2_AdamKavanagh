#pragma once
#include <SFML/Graphics.hpp>
#include "resource_identifiers.hpp"
#include "scene_node.hpp"
#include "scene_layers.hpp"
#include "aircraft.hpp"
#include "tank.hpp"
#include "tank_type.hpp"
#include "command_queue.hpp"
#include "bloom_effect.hpp"
#include "sound_player.hpp"
#include "sprite_node.hpp"

#include <array>
#include "pickup_type.hpp"
#include "network_node.hpp"

class World
{
public:
    explicit World(sf::RenderTarget& output_target, FontHolder& font, SoundPlayer& sounds, bool networked = false);
    void Update(sf::Time dt);
    void Draw();

    sf::FloatRect GetViewBounds() const;
    CommandQueue& GetCommandQueue();

    Aircraft* AddAircraft(uint8_t identifier);
    void RemoveAircraft(uint8_t identifier);
    void SetCurrentBattleFieldPosition(float line_y);
    void SetWorldHeight(float height);

    // Tank equivalents of the above, added for the tank conversion.
    Tank* AddTank(uint8_t identifier);
    void RemoveTank(uint8_t identifier);
    Tank* GetTank(int identifier) const;

    // Identifies which tank in m_player_tanks belongs to this client, for
    // camera-follow. Must be called once, right after your own tank spawns
    // (mirrors how AddAircraft/AddTank both hand back the identifier you gave them).
    void SetLocalPlayerIdentifier(uint8_t identifier);

    // True once the tutorial's stationary enemy Panzer has been destroyed.
    // Only meaningful when this World was constructed with networked=false.
    bool IsTutorialComplete() const;

    void AddEnemy(AircraftType type, float relx, float rely);
    void SortEnemies();

    bool HasAlivePlayer() const;
    bool HasPlayerReachedEnd() const;

    void SetWorldScrollCompensation(float compensation);
    Aircraft* GetAircraft(int identifier) const;
    sf::FloatRect GetBattleFieldBounds() const;
    void CreatePickup(sf::Vector2f position, PickupType type);
    bool PollGameAction(GameActions::Action& out);

private:
    void LoadTextures();
    void BuildScene();
    void BuildTutorialScene();       // single-player: player Sherman + stationary Panzer
    void BuildMultiplayerTankScene(); // networked: debris only, tanks arrive via AddTank
    void AddTiledBackground();       // shared by both - the "loops on itself" background
    void AdaptPlayerVelocity();
    void AdaptPlayerPosition();
    void AdaptTankPositions();       // tank equivalent of AdaptPlayerPosition

    void SpawnEnemies();
    void AddEnemies();

    void GuideMissiles();

    void HandleCollisions();

    void DestroyEntitiesOutsideView();

    void UpdateSounds();

private:
    struct SpawnPoint
    {
        SpawnPoint(AircraftType type, float x, float y) :m_type(type), m_x(x), m_y(y)
        {

        }
        AircraftType m_type;
        float m_x;
        float m_y;
    };

private:
    sf::RenderTarget& m_target;
    sf::RenderTexture m_scene_texture;
    sf::View m_camera;
    TextureHolder m_textures;
    FontHolder& m_fonts;
    SoundPlayer& m_sounds;
    SceneNode m_scene_graph;
    std::array<SceneNode*, static_cast<int>(SceneLayers::kLayerCount)> m_scene_layers;
    sf::FloatRect m_world_bounds;
    sf::Vector2f m_spawn_position;
    float m_scroll_speed;
    float m_scrollspeed_compensation;

    std::vector<Aircraft*> m_player_aircraft;

    // Tank conversion additions
    std::vector<Tank*> m_player_tanks;
    Tank* m_tutorial_enemy_tank;
    uint8_t m_local_player_identifier;

    CommandQueue m_command_queue;

    std::vector<SpawnPoint> m_enemy_spawn_points;
    std::vector<Aircraft*> m_active_enemies;

    BloomEffect m_bloom_effect;
    bool m_networked_world;
    NetworkNode* m_network_node;
    SpriteNode* m_finish_sprite;
};

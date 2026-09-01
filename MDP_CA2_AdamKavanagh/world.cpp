// Adam Kavanagh - D00247069
#include "world.hpp"
#include "constants.hpp"
#include "projectile.hpp"
#include "particle_node.hpp"
#include "particletype.hpp"
#include "sound_node.hpp"
#include "debris_node.hpp"
#include "debris_layout.hpp"
#include "obstacle_collision.hpp"
#include "team_assignment.hpp"
#include "tutorial_config.hpp"
#include "posteffect.hpp"
#include <algorithm>
#include <set>
#include <functional>
#include <memory>
#include <utility>

World::World(sf::RenderTarget& output_target, FontHolder& font, SoundPlayer& sounds, bool networked)
    : m_target(output_target)
    , m_camera(output_target.getDefaultView())
    , m_textures()
    , m_fonts(font)
    , m_sounds(sounds)
    , m_scene_graph(ReceiverCategories::kNone)
    , m_scene_layers()
    // Fixed size, identical on every machine - see constants.hpp for why this
    // must not depend on the window.
    , m_world_bounds(sf::Vector2f(0.f, 0.f), sf::Vector2f(kWorldWidth, kWorldHeight))
    , m_spawn_position(kWorldWidth / 2.f, kWorldHeight / 2.f)
    , m_tanks()
    , m_training_enemy(nullptr)
    , m_training_enemy_destroyed(false)
    , m_training_player_destroyed(false)
    , m_local_player_identifier(0)
    , m_networked_world(networked)
    , m_network_node(nullptr)
{
    m_scene_texture.resize({ m_target.getSize().x, m_target.getSize().y });
    LoadTextures();
    BuildScene();
    m_camera.setCenter(m_spawn_position);
}

void World::LoadTextures()
{
    m_textures.Load(TextureID::kTankSheet, "Media/Textures/Sprite-Sheet.png");
    m_textures.Load(TextureID::kBattlefield, "Media/Textures/Road to Caen.png");
    m_textures.Load(TextureID::kEntities, "Media/Textures/Entities.png");
    m_textures.Load(TextureID::kExplosion, "Media/Textures/Explosion.png");
    m_textures.Load(TextureID::kParticle, "Media/Textures/Particle.png");
}

void World::BuildScene()
{
    for (int i = 0; i < static_cast<int>(SceneLayers::kLayerCount); ++i)
    {
        // Only the entity layer answers to scene-wide commands - that is where
        // shells get attached when a tank fires.
        ReceiverCategories category = (i == static_cast<int>(SceneLayers::kEntities))
            ? ReceiverCategories::kScene : ReceiverCategories::kNone;
        SceneNode::Ptr layer(new SceneNode(category));
        m_scene_layers[i] = layer.get();
        m_scene_graph.AttachChild(std::move(layer));
    }

    AddBattlefieldBackground();

    std::unique_ptr<ParticleNode> smoke_node(new ParticleNode(ParticleType::kSmoke, m_textures));
    m_scene_layers[static_cast<int>(SceneLayers::kGround)]->AttachChild(std::move(smoke_node));

    std::unique_ptr<ParticleNode> propellant_node(new ParticleNode(ParticleType::kPropellant, m_textures));
    m_scene_layers[static_cast<int>(SceneLayers::kGround)]->AttachChild(std::move(propellant_node));

    std::unique_ptr<SoundNode> sound_node(new SoundNode(m_sounds));
    m_scene_graph.AttachChild(std::move(sound_node));

    if (m_networked_world)
    {
        std::unique_ptr<NetworkNode> network_node(new NetworkNode());
        m_network_node = network_node.get();
        m_scene_graph.AttachChild(std::move(network_node));

        BuildMultiplayerScene();
    }
    else
    {
        BuildTrainingScene();
    }
}

void World::AddBattlefieldBackground()
{
    // The map art is exactly the size of the arena, so it is drawn once at
    // 1:1 with no tiling seams. setRepeated is still enabled so that a
    // smaller replacement texture would tile rather than stretch.
    sf::Texture& texture = m_textures.Get(TextureID::kBattlefield);
    texture.setRepeated(true);

    const sf::IntRect texture_rect(
        sf::Vector2i(0, 0),
        sf::Vector2i(static_cast<int>(m_world_bounds.size.x), static_cast<int>(m_world_bounds.size.y)));

    std::unique_ptr<SpriteNode> background(new SpriteNode(texture, texture_rect));
    background->setPosition(m_world_bounds.position);
    m_scene_layers[static_cast<int>(SceneLayers::kBackground)]->AttachChild(std::move(background));
}

void World::AddDebris()
{
    for (const DebrisPlacement& placement : GetDebrisLayout())
    {
        const sf::Vector2f position(
            m_world_bounds.position.x + placement.m_relative_position.x * m_world_bounds.size.x,
            m_world_bounds.position.y + placement.m_relative_position.y * m_world_bounds.size.y);

        std::unique_ptr<DebrisNode> debris(new DebrisNode(placement.m_type, m_textures.Get(TextureID::kTankSheet)));
        debris->setPosition(position);
        debris->setRotation(sf::degrees(placement.m_rotation_degrees));
        m_scene_layers[static_cast<int>(SceneLayers::kGround)]->AttachChild(std::move(debris));
    }
}

void World::BuildTrainingScene()
{
    // Training map: the player's own Sherman and a single stationary Panzer to
    // destroy. No debris, so there is nothing between the two tanks while the
    // player is learning to drive and aim.
    std::unique_ptr<Tank> player_tank(new Tank(TankType::kSherman, m_textures, m_fonts));
    player_tank->setPosition(TutorialConfig::GetPlayerSpawnPosition(m_world_bounds.size));
    player_tank->SetIdentifier(TutorialConfig::kPlayerIdentifier);
    m_tanks.push_back(player_tank.get());
    m_scene_layers[static_cast<int>(SceneLayers::kEntities)]->AttachChild(std::move(player_tank));

    SetLocalPlayerIdentifier(TutorialConfig::kPlayerIdentifier);

    std::unique_ptr<Tank> enemy_tank(new Tank(TankType::kPanzer, m_textures, m_fonts));
    enemy_tank->setPosition(TutorialConfig::GetEnemySpawnPosition(m_world_bounds.size));
    enemy_tank->setRotation(sf::degrees(180.f));
    enemy_tank->SetIdentifier(TutorialConfig::kEnemyIdentifier);
    m_training_enemy = enemy_tank.get();
    m_tanks.push_back(enemy_tank.get());
    m_scene_layers[static_cast<int>(SceneLayers::kEntities)]->AttachChild(std::move(enemy_tank));

    m_spawn_position = TutorialConfig::GetPlayerSpawnPosition(m_world_bounds.size);
}

void World::BuildMultiplayerScene()
{
    // No tanks here - they arrive through AddTank() as players connect. The
    // obstacles come from the shared fixed layout, so every client builds an
    // identical map without a single byte being sent.
    AddDebris();
}

void World::Update(sf::Time dt)
{
    DestroyProjectilesOutsideWorld();

    while (!m_command_queue.IsEmpty())
    {
        m_scene_graph.OnCommand(m_command_queue.Pop(), dt);
    }

    HandleCollisions();
    KeepTanksInsideWorld();

    // A destroyed tank stays in the scene until its explosion finishes, so the
    // training flags are latched here before the wreck is swept away.
    if (!m_networked_world)
    {
        if (m_training_enemy && m_training_enemy->IsDestroyed())
        {
            m_training_enemy_destroyed = true;
        }
        if (Tank* player = GetTank(m_local_player_identifier))
        {
            if (player->IsDestroyed())
            {
                m_training_player_destroyed = true;
            }
        }
    }

    // Checked before the erase: remove_if leaves the tail in an unspecified
    // state, so it is not somewhere to go looking for a particular pointer.
    if (m_training_enemy && m_training_enemy->IsMarkedForRemoval())
    {
        m_training_enemy = nullptr;
    }

    auto first_to_remove = std::remove_if(m_tanks.begin(), m_tanks.end(), std::mem_fn(&Tank::IsMarkedForRemoval));
    m_tanks.erase(first_to_remove, m_tanks.end());

    m_scene_graph.RemoveWrecks();
    m_scene_graph.Update(dt, m_command_queue);

    UpdateCamera();
    UpdateSounds();
}

void World::Draw()
{
    if (PostEffect::IsSupported())
    {
        m_scene_texture.clear();
        m_scene_texture.setView(m_camera);
        m_scene_texture.draw(m_scene_graph);
        m_scene_texture.display();
        m_bloom_effect.Apply(m_scene_texture, m_target);
    }
    else
    {
        m_target.setView(m_camera);
        m_target.draw(m_scene_graph);
    }
}

CommandQueue& World::GetCommandQueue()
{
    return m_command_queue;
}

sf::FloatRect World::GetViewBounds() const
{
    return sf::FloatRect(m_camera.getCenter() - m_camera.getSize() / 2.f, m_camera.getSize());
}

sf::FloatRect World::GetWorldBounds() const
{
    return m_world_bounds;
}

Tank* World::GetTank(uint8_t identifier) const
{
    for (Tank* tank : m_tanks)
    {
        if (tank->GetIdentifier() == identifier)
        {
            return tank;
        }
    }
    return nullptr;
}

Tank* World::AddTank(uint8_t identifier)
{
    // Identifiers are unique for the life of a match, so a request to add one
    // that is already here means the same spawn was announced twice. Hand back
    // what exists rather than building a second hull that would sit on top of
    // the first and answer to the same input.
    if (Tank* existing = GetTank(identifier))
    {
        return existing;
    }

    // The hull (and therefore the team) is derived from the identifier, so the
    // server never has to tell anyone which side a player is on.
    std::unique_ptr<Tank> tank(new Tank(AssignTankType(identifier), m_textures, m_fonts));
    tank->setPosition(m_spawn_position);
    tank->SetIdentifier(identifier);

    m_tanks.push_back(tank.get());
    m_scene_layers[static_cast<int>(SceneLayers::kEntities)]->AttachChild(std::move(tank));
    return m_tanks.back();
}

void World::RemoveTank(uint8_t identifier)
{
    if (Tank* tank = GetTank(identifier))
    {
        // Remove() rather than Destroy() so a disconnecting player's tank just
        // disappears instead of exploding as though it had been killed.
        tank->Remove();
        m_tanks.erase(std::find(m_tanks.begin(), m_tanks.end(), tank));
        if (tank == m_training_enemy)
        {
            m_training_enemy = nullptr;
        }
    }
}

void World::SetLocalPlayerIdentifier(uint8_t identifier)
{
    m_local_player_identifier = identifier;
}

uint8_t World::GetLocalPlayerIdentifier() const
{
    return m_local_player_identifier;
}

bool World::IsTutorialComplete() const
{
    return m_training_enemy_destroyed;
}

bool World::IsTutorialPlayerDestroyed() const
{
    return m_training_player_destroyed;
}

bool World::PollGameAction(GameActions::Action& out)
{
    return m_network_node != nullptr && m_network_node->PollGameAction(out);
}

bool World::IsAuthoritativeFor(const Tank& tank) const
{
    // Offline every tank is ours to simulate. Online, only the tank this
    // client drives - everyone else's hitpoints arrive from the server.
    return !m_networked_world || tank.GetIdentifier() == m_local_player_identifier;
}

void World::KeepTanksInsideWorld()
{
    // Clamped against the arena, not the camera: in multiplayer the view is
    // just a window onto a much larger world and a tank must not be stopped
    // by the edge of somebody's screen.
    const float border = 40.f;

    for (Tank* tank : m_tanks)
    {
        sf::Vector2f position = tank->getPosition();
        position.x = std::clamp(position.x, m_world_bounds.position.x + border, m_world_bounds.position.x + m_world_bounds.size.x - border);
        position.y = std::clamp(position.y, m_world_bounds.position.y + border, m_world_bounds.position.y + m_world_bounds.size.y - border);
        tank->setPosition(position);
    }
}

namespace
{
    bool MatchesCategories(SceneNode::Pair& colliders, ReceiverCategories type1, ReceiverCategories type2)
    {
        const unsigned int category1 = colliders.first->GetCategory();
        const unsigned int category2 = colliders.second->GetCategory();

        if ((static_cast<unsigned int>(type1) & category1) && (static_cast<unsigned int>(type2) & category2))
        {
            return true;
        }

        if ((static_cast<unsigned int>(type1) & category2) && (static_cast<unsigned int>(type2) & category1))
        {
            std::swap(colliders.first, colliders.second);
            return true;
        }

        return false;
    }
}

void World::HandleCollisions()
{
    std::set<SceneNode::Pair> collision_pairs;
    m_scene_graph.CheckSceneCollision(m_scene_graph, collision_pairs);

    for (SceneNode::Pair pair : collision_pairs)
    {
        // An enemy shell struck a tank.
        if (MatchesCategories(pair, ReceiverCategories::kAlliesTank, ReceiverCategories::kAxisProjectile) ||
            MatchesCategories(pair, ReceiverCategories::kAxisTank, ReceiverCategories::kAlliesProjectile))
        {
            auto& tank = static_cast<Tank&>(*pair.first);
            auto& shell = static_cast<Projectile&>(*pair.second);

            // The shell is consumed on every client so the visuals agree, but
            // the damage is only applied by the machine that owns the tank.
            shell.Destroy();

            if (!IsAuthoritativeFor(tank) || tank.IsDestroyed())
            {
                continue;
            }

            tank.Damage(shell.GetDamage());

            if (tank.IsDestroyed() && m_network_node)
            {
                // Report our own death, naming the shooter. Because only the
                // victim ever sends this, the server counts the kill once no
                // matter how many clients saw the impact.
                m_network_node->NotifyGameAction(GameActions::kTankDestroyed,
                    tank.GetIdentifier(), shell.GetOwnerIdentifier(), tank.GetWorldPosition());
            }
        }
        // A tank drove into map cover.
        else if (MatchesCategories(pair, ReceiverCategories::kAnyTank, ReceiverCategories::kObstacle))
        {
            auto& tank = static_cast<Tank&>(*pair.first);
            tank.move(ResolveAabbPushOut(tank.GetBoundingRect(), pair.second->GetBoundingRect()));
        }
        // A shell hit map cover.
        else if (MatchesCategories(pair, ReceiverCategories::kAnyProjectile, ReceiverCategories::kObstacle))
        {
            static_cast<Projectile&>(*pair.first).Destroy();
        }
        // Two tanks collided - push them apart rather than letting them
        // overlap, and let the owning client resolve its own tank.
        else if (MatchesCategories(pair, ReceiverCategories::kAnyTank, ReceiverCategories::kAnyTank))
        {
            auto& first = static_cast<Tank&>(*pair.first);
            auto& second = static_cast<Tank&>(*pair.second);

            if (IsAuthoritativeFor(first))
            {
                first.move(ResolveAabbPushOut(first.GetBoundingRect(), second.GetBoundingRect()));
            }
            if (IsAuthoritativeFor(second))
            {
                second.move(ResolveAabbPushOut(second.GetBoundingRect(), first.GetBoundingRect()));
            }
        }
    }
}

void World::DestroyProjectilesOutsideWorld()
{
    Command command;
    command.category = static_cast<unsigned int>(ReceiverCategories::kAnyProjectile);
    command.action = DerivedAction<Entity>([this](Entity& entity, sf::Time)
        {
            if (m_world_bounds.findIntersection(entity.GetBoundingRect()) == std::nullopt)
            {
                entity.Remove();
            }
        });
    m_command_queue.Push(command);
}

void World::UpdateCamera()
{
    Tank* local_tank = GetTank(m_local_player_identifier);
    if (!local_tank)
    {
        return;
    }

    // Follow the local tank, but never show anything outside the arena.
    sf::Vector2f position = local_tank->getPosition();
    const sf::Vector2f half_size = m_camera.getSize() / 2.f;

    const float min_x = m_world_bounds.position.x + half_size.x;
    const float max_x = m_world_bounds.position.x + m_world_bounds.size.x - half_size.x;
    const float min_y = m_world_bounds.position.y + half_size.y;
    const float max_y = m_world_bounds.position.y + m_world_bounds.size.y - half_size.y;

    // If the window is wider than the map there is nothing to clamp to, so
    // centre on the map instead of flipping the bounds inside out.
    position.x = (min_x > max_x) ? (min_x + max_x) / 2.f : std::clamp(position.x, min_x, max_x);
    position.y = (min_y > max_y) ? (min_y + max_y) / 2.f : std::clamp(position.y, min_y, max_y);

    m_camera.setCenter(position);
}

void World::UpdateSounds()
{
    if (Tank* local_tank = GetTank(m_local_player_identifier))
    {
        m_sounds.SetListenerPosition(local_tank->GetWorldPosition());
    }
    else
    {
        m_sounds.SetListenerPosition(m_camera.getCenter());
    }

    m_sounds.RemoveStoppedSounds();
}

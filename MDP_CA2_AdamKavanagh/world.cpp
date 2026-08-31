#include "world.hpp"
#include "sprite_node.hpp"
#include <iostream>
#include "state.hpp"
#include <SFML/System/Angle.hpp>
#include "Projectile.hpp"
#include "pickup.hpp"
#include "particle_node.hpp"
#include "particletype.hpp"
#include "sound_node.hpp"

// Tank conversion additions
#include "debris_node.hpp"
#include "debris_layout.hpp"
#include "obstacle_collision.hpp"
#include "team_assignment.hpp"
#include "tutorial_config.hpp"

World::World(sf::RenderTarget& output_target, FontHolder& font, SoundPlayer& sounds, bool networked)
    : m_target(output_target)
    , m_camera(output_target.getDefaultView())
    , m_textures()
    , m_fonts(font)
    , m_sounds(sounds)
    , m_scene_graph(ReceiverCategories::kNone)
    , m_scene_layers()
    // Widened from the original vertical-scroll corridor (camera-width x
    // 1500) into an open square-ish arena, since tanks need room to
    // maneuver in both axes, not just scroll upward. Adjust the multiplier
    // to taste - this is just a starting size.
    , m_world_bounds(sf::Vector2f(0.f, 0.f), sf::Vector2f(m_camera.getSize().x * 2.5f, m_camera.getSize().y * 2.5f))
    , m_spawn_position(m_world_bounds.size.x / 2.f, m_world_bounds.size.y / 2.f)
    , m_scroll_speed(0.f)
    , m_scrollspeed_compensation(1.f)
    , m_player_aircraft()
    , m_player_tanks()
    , m_tutorial_enemy_tank(nullptr)
    , m_local_player_identifier(0)
    , m_enemy_spawn_points()
    , m_active_enemies()
    , m_networked_world(networked)
    , m_network_node(nullptr)
    , m_finish_sprite(nullptr)
{
    m_scene_texture.resize({ m_target.getSize().x, m_target.getSize().y });
    LoadTextures();
    BuildScene();
    m_camera.setCenter(m_spawn_position);
}

void World::SetWorldScrollCompensation(float compensation)
{
    m_scrollspeed_compensation = compensation;
}

void World::Update(sf::Time dt)
{
    // Scroll the world (dormant for tanks - m_scroll_speed stays 0.f unless
    // something still sets it for the old plane path)
    m_camera.move(sf::Vector2f(0, m_scroll_speed * dt.asSeconds() * m_scrollspeed_compensation));

    for (Aircraft* a : m_player_aircraft)
    {
        a->SetVelocity(0.f, 0.f);
    }
    // Deliberately NOT doing this for m_player_tanks - Tank doesn't use
    // Entity's velocity system (hull movement is direct move() calls in
    // player.cpp's command lambdas), so there's nothing to zero here.

    DestroyEntitiesOutsideView();
    GuideMissiles();

    //Process commands from the scenegraph
    while (!m_command_queue.IsEmpty())
    {
        m_scene_graph.OnCommand(m_command_queue.Pop(), dt);
    }
    AdaptPlayerVelocity(); // aircraft only - do NOT call this on tanks, see AdaptPlayerVelocity's comment

    HandleCollisions();

    auto first_to_remove = std::remove_if(m_player_aircraft.begin(), m_player_aircraft.end(), std::mem_fn(&Aircraft::IsMarkedForRemoval));
    m_player_aircraft.erase(first_to_remove, m_player_aircraft.end());

    auto first_tank_to_remove = std::remove_if(m_player_tanks.begin(), m_player_tanks.end(), std::mem_fn(&Tank::IsMarkedForRemoval));
    m_player_tanks.erase(first_tank_to_remove, m_player_tanks.end());

    m_scene_graph.RemoveWrecks();

    SpawnEnemies();

    m_scene_graph.Update(dt, m_command_queue);
    AdaptPlayerPosition();
    AdaptTankPositions();

    // Camera follow: prefer the local player's own tank if one exists,
    // otherwise fall back to the old aircraft-following behaviour (kept so
    // this still works during the transition while Aircraft is still around).
    if (Tank* local_tank = GetTank(m_local_player_identifier))
    {
        sf::Vector2f position = local_tank->getPosition();
        sf::Vector2f halfSize = m_camera.getSize() / 2.f;

        float minX = m_world_bounds.position.x + halfSize.x;
        float maxX = m_world_bounds.position.x + m_world_bounds.size.x - halfSize.x;
        float minY = m_world_bounds.position.y + halfSize.y;
        float maxY = m_world_bounds.position.y + m_world_bounds.size.y - halfSize.y;

        if (minX > maxX) position.x = (minX + maxX) / 2.f; else position.x = std::max(minX, std::min(position.x, maxX));
        if (minY > maxY) position.y = (minY + maxY) / 2.f; else position.y = std::max(minY, std::min(position.y, maxY));

        m_camera.setCenter(position);
    }
    else if (!m_player_aircraft.empty())
    {
        sf::Vector2f position = m_player_aircraft.front()->getPosition();
        sf::Vector2f halfSize = m_camera.getSize() / 2.f;

        float minX = m_world_bounds.position.x + halfSize.x;
        float maxX = m_world_bounds.position.x + m_world_bounds.size.x - halfSize.x;
        float minY = m_world_bounds.position.y + halfSize.y;
        float maxY = m_world_bounds.position.y + m_world_bounds.size.y - halfSize.y;

        if (minX > maxX) position.x = (minX + maxX) / 2.f; else position.x = std::max(minX, std::min(position.x, maxX));
        if (minY > maxY) position.y = (minY + maxY) / 2.f; else position.y = std::max(minY, std::min(position.y, maxY));

        m_camera.setCenter(position);
    }

    //UpdateSounds();
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

Aircraft* World::GetAircraft(int identifier) const
{
    for (Aircraft* a : m_player_aircraft)
    {
        if (a->GetIdentifier() == identifier)
        {
            return a;
        }
    }
    return nullptr;
}

void World::RemoveAircraft(uint8_t identifier)
{
    Aircraft* aircraft = GetAircraft(identifier);
    if (aircraft)
    {
        aircraft->Destroy();
        m_player_aircraft.erase(std::find(m_player_aircraft.begin(), m_player_aircraft.end(), aircraft));
    }
}

Aircraft* World::AddAircraft(uint8_t identifier)
{
    std::unique_ptr<Aircraft> player(new Aircraft(AircraftType::kEagle, m_textures, m_fonts));
    player->setPosition(m_camera.getCenter());
    std::cout << "World::AddAircraft " << +identifier << std::endl;
    player->SetIdentifier(identifier);

    m_player_aircraft.emplace_back(player.get());
    m_scene_layers[static_cast<int>(SceneLayers::kUpperAir)]->AttachChild(std::move(player));
    return m_player_aircraft.back();
}

Tank* World::GetTank(int identifier) const
{
    for (Tank* t : m_player_tanks)
    {
        if (t->GetIdentifier() == identifier)
        {
            return t;
        }
    }
    return nullptr;
}

void World::RemoveTank(uint8_t identifier)
{
    Tank* tank = GetTank(identifier);
    if (tank)
    {
        tank->Destroy();
        m_player_tanks.erase(std::find(m_player_tanks.begin(), m_player_tanks.end(), tank));
    }
}

Tank* World::AddTank(uint8_t identifier)
{
    TankType type = AssignTankType(identifier);
    std::unique_ptr<Tank> tank(new Tank(type, m_textures, m_fonts));
    tank->setPosition(m_camera.getCenter());
    std::cout << "World::AddTank " << +identifier << std::endl;
    tank->SetIdentifier(identifier);

    m_player_tanks.emplace_back(tank.get());
    m_scene_layers[static_cast<int>(SceneLayers::kUpperAir)]->AttachChild(std::move(tank));
    return m_player_tanks.back();
}

void World::SetLocalPlayerIdentifier(uint8_t identifier)
{
    m_local_player_identifier = identifier;
}

bool World::IsTutorialComplete() const
{
    return m_tutorial_enemy_tank != nullptr && m_tutorial_enemy_tank->IsDestroyed();
}

void World::CreatePickup(sf::Vector2f position, PickupType type)
{
    std::unique_ptr<Pickup> pickup(new Pickup(type, m_textures));
    pickup->setPosition(position);
    pickup->SetVelocity(0.f, 1.f);
    m_scene_layers[static_cast<int>(SceneLayers::kUpperAir)]->AttachChild(std::move(pickup));
}

bool World::PollGameAction(GameActions::Action& out)
{
    return m_network_node->PollGameAction(out);
}

void World::SetCurrentBattleFieldPosition(float lineY)
{
    m_camera.setCenter(sf::Vector2f(m_camera.getCenter().x, lineY - m_camera.getSize().y / 2));
    m_spawn_position.y = m_world_bounds.size.y;
}

void World::SetWorldHeight(float height)
{
    m_world_bounds.size.y = height;
}

CommandQueue& World::GetCommandQueue()
{
    return m_command_queue;
}

bool World::HasAlivePlayer() const
{
    return !m_player_aircraft.empty();
}

bool World::HasPlayerReachedEnd() const
{
    if (Aircraft* aircraft = GetAircraft(1))
    {
        return !m_world_bounds.contains(aircraft->getPosition());
    }
    return false;
}

void World::LoadTextures()
{
    m_textures.Load(TextureID::kEntities, "Media/Textures/Entities.png");
    // Sprite-sheet for player models
    m_textures.Load(TextureID::kSherman, "Media/Textures/S1.png");
    m_textures.Load(TextureID::kTankSheet, "Media/Textures/TankSheet.png");
    m_textures.Load(TextureID::kExplosion, "Media/Textures/Explosion.png");
    m_textures.Load(TextureID::kFinishLine, "Media/Textures/FinishLine.png");
    m_textures.Load(TextureID::kMap1, "Media/Textures/Road to caen.png");
    m_textures.Load(TextureID::kParticle, "Media/Textures/Particle.png");
}

void World::AddTiledBackground()
{
    // "Road to caen" tiled across the full (now much larger) world bounds.
    // setRepeated(true) + an IntRect bigger than the texture's real pixel
    // size makes SFML wrap/tile it seamlessly - this is what makes the
    // background "loop on itself" instead of showing once and stopping.
    // (The old version only padded this in Y, to hide the scroll's leading
    // edge before it scrolled into view - that padding is gone since
    // there's no scrolling any more; this now tiles in both X and Y.)
    sf::Texture& texture = m_textures.Get(TextureID::kMap1);
    texture.setRepeated(true);

    sf::IntRect texture_rect(
        sf::Vector2i(0, 0),
        sf::Vector2i(static_cast<int>(m_world_bounds.size.x), static_cast<int>(m_world_bounds.size.y)));

    std::unique_ptr<SpriteNode> background_sprite(new SpriteNode(texture, texture_rect));
    background_sprite->setPosition(sf::Vector2f(m_world_bounds.position.x, m_world_bounds.position.y));
    m_scene_layers[static_cast<int>(SceneLayers::kBackground)]->AttachChild(std::move(background_sprite));
}

void World::BuildScene()
{
    //Initialise the different layers
    for (int i = 0; i < static_cast<int>(SceneLayers::kLayerCount); i++)
    {
        ReceiverCategories category = (i == static_cast<int>(SceneLayers::kUpperAir)) ? ReceiverCategories::kScene : ReceiverCategories::kNone;
        SceneNode::Ptr layer(new SceneNode(category));
        m_scene_layers[i] = layer.get();
        m_scene_graph.AttachChild(std::move(layer));
    }

    AddTiledBackground();

    // No finish line in tank mode - the old finish-line sprite block is
    // removed. (m_finish_sprite stays declared in world.hpp for now, just
    // unused, rather than ripping out the member during this pass.)

    //Add the particle nodes to the scene
    std::unique_ptr<ParticleNode> smokeNode(new ParticleNode(ParticleType::kSmoke, m_textures));
    m_scene_layers[static_cast<int>(SceneLayers::kLowerAir)]->AttachChild(std::move(smokeNode));

    std::unique_ptr<ParticleNode> propellantNode(new ParticleNode(ParticleType::kPropellant, m_textures));
    m_scene_layers[static_cast<int>(SceneLayers::kLowerAir)]->AttachChild(std::move(propellantNode));

    //Add sound effect node
    std::unique_ptr<SoundNode> soundNode(new SoundNode(m_sounds));
    m_scene_graph.AttachChild(std::move(soundNode));

    if (m_networked_world)
    {
        std::unique_ptr<NetworkNode> network_node(new NetworkNode());
        m_network_node = network_node.get();
        m_scene_graph.AttachChild(std::move(network_node));

        BuildMultiplayerTankScene();
    }
    else
    {
        // World now serves as the single-player tutorial, replacing the
        // old scripted plane campaign (AddEnemies()/finish line).
        BuildTutorialScene();
    }
}

void World::BuildTutorialScene()
{
    std::unique_ptr<Tank> player_tank(new Tank(TankType::kSherman, m_textures, m_fonts));
    player_tank->setPosition(TutorialConfig::GetPlayerSpawnPosition(m_world_bounds.size));
    player_tank->SetIdentifier(TutorialConfig::kPlayerIdentifier);
    m_player_tanks.push_back(player_tank.get());
    m_scene_layers[static_cast<int>(SceneLayers::kUpperAir)]->AttachChild(std::move(player_tank));
    SetLocalPlayerIdentifier(TutorialConfig::kPlayerIdentifier);

    std::unique_ptr<Tank> enemy_tank(new Tank(TankType::kPanzer, m_textures, m_fonts));
    enemy_tank->setPosition(TutorialConfig::GetEnemySpawnPosition(m_world_bounds.size));
    enemy_tank->SetIdentifier(TutorialConfig::kEnemyIdentifier);
    m_tutorial_enemy_tank = enemy_tank.get();
    m_scene_layers[static_cast<int>(SceneLayers::kUpperAir)]->AttachChild(std::move(enemy_tank));
}

void World::BuildMultiplayerTankScene()
{
    // No tanks spawned here - they arrive via AddTank() as players
    // connect, exactly like AddAircraft already worked. Just scatter debris.
    for (const auto& placement : GetDebrisLayout())
    {
        sf::Vector2f pos(m_world_bounds.position.x + placement.m_relative_position.x * m_world_bounds.size.x,
            m_world_bounds.position.y + placement.m_relative_position.y * m_world_bounds.size.y);
        std::unique_ptr<DebrisNode> debris(new DebrisNode(placement.m_type, m_textures.Get(TextureID::kTankSheet)));
        debris->setPosition(pos);
        debris->setRotation(sf::degrees(placement.m_rotation_degrees));
        m_scene_layers[static_cast<int>(SceneLayers::kBackground)]->AttachChild(std::move(debris));
    }
}

void World::AdaptPlayerVelocity()
{
    // Aircraft only. Deliberately never called for Tanks: this divides
    // diagonal velocity by sqrt(2), which assumes two independent
    // orthogonal inputs (plane WASD strafing). A tank's velocity being
    // diagonal is just normal angled-facing, not a key-combo, and Tank
    // doesn't use Entity's velocity system anyway (see the note in Update()).
    for (Aircraft* aircraft : m_player_aircraft)
    {
        sf::Vector2f velocity = aircraft->GetVelocity();

        //If they are moving diagonally divide by sqrt 2
        if (velocity.x != 0.f && velocity.y != 0.f)
        {
            aircraft->SetVelocity(velocity / std::sqrt(2.f));
        }
        // Auto-scrolling removed: do not add scrolling velocity to aircraft
    }
}

void World::AdaptPlayerPosition()
{
    //keep player on the screen
    sf::FloatRect view_bounds = GetViewBounds();
    const float border_distance = 40.f;

    for (Aircraft* aircraft : m_player_aircraft)
    {
        sf::Vector2f position = aircraft->getPosition();
        position.x = std::min(position.x, view_bounds.position.x + view_bounds.size.x - border_distance);
        position.x = std::max(position.x, view_bounds.position.x + border_distance);
        position.y = std::min(position.y, view_bounds.position.y + view_bounds.size.y - border_distance);
        position.y = std::max(position.y, view_bounds.position.y + border_distance);
        aircraft->setPosition(position);
    }

}

void World::AdaptTankPositions()
{
    // Same idea as AdaptPlayerPosition, but for tanks - keeps every tank
    // within the current view.
    sf::FloatRect view_bounds = GetViewBounds();
    const float border_distance = 40.f;

    for (Tank* tank : m_player_tanks)
    {
        sf::Vector2f position = tank->getPosition();
        position.x = std::min(position.x, view_bounds.position.x + view_bounds.size.x - border_distance);
        position.x = std::max(position.x, view_bounds.position.x + border_distance);
        position.y = std::min(position.y, view_bounds.position.y + view_bounds.size.y - border_distance);
        position.y = std::max(position.y, view_bounds.position.y + border_distance);
        tank->setPosition(position);
    }
}

void World::SpawnEnemies()
{
    //Spawn an enemy when it is relevent i.e in BattlefieldBounds
    while (!m_enemy_spawn_points.empty() && m_enemy_spawn_points.back().m_y > GetBattleFieldBounds().position.y)
    {
        SpawnPoint spawn = m_enemy_spawn_points.back();
        std::unique_ptr<Aircraft> enemy(new Aircraft(spawn.m_type, m_textures, m_fonts));
        enemy->setPosition(sf::Vector2f(spawn.m_x, spawn.m_y));
        enemy->setRotation(sf::degrees(180.f));

        //If the game is networked the server is responsible for spawning pickups

        if (m_networked_world)
        {
            enemy->DisablePickups();
        }

        m_scene_layers[static_cast<int>(SceneLayers::kUpperAir)]->AttachChild(std::move(enemy));
        m_enemy_spawn_points.pop_back();
    }
}

void World::AddEnemy(AircraftType type, float relx, float rely)
{
    SpawnPoint spawn(type, m_spawn_position.x + relx, m_spawn_position.y - rely);
    m_enemy_spawn_points.emplace_back(spawn);
}

void World::AddEnemies()
{
    // No longer called from BuildScene() - tank mode has no scripted plane
    // waves. Left in place, unused, until Phase 5 cleanup removes it along
    // with Aircraft.
    if (m_networked_world)
    {
        return;
    }
    AddEnemy(AircraftType::kRaptor, 0.f, 500.f);
    SortEnemies();
}

void World::SortEnemies()
{
    //Sort all enemies according to their y-value, such that lower enemies are checked first for spawning
    std::sort(m_enemy_spawn_points.begin(), m_enemy_spawn_points.end(), [](SpawnPoint lhs, SpawnPoint rhs)
        {
            return lhs.m_y < rhs.m_y;
        });
}

sf::FloatRect World::GetViewBounds() const
{
    return sf::FloatRect(m_camera.getCenter() - m_camera.getSize() / 2.f, m_camera.getSize());
}

sf::FloatRect World::GetBattleFieldBounds() const
{
    //Return camera bounds + a small area off screen where the enemies spawn
    sf::FloatRect bounds = GetViewBounds();
    bounds.position.y -= 100.f;
    bounds.size.y += 100.f;
    return bounds;
}

void World::GuideMissiles()
{
    //Target the closest enemy in the world
    Command enemyCollector;
    enemyCollector.category = static_cast<int>(ReceiverCategories::kEnemyAircraft);
    enemyCollector.action = DerivedAction<Aircraft>([this](Aircraft& enemy, sf::Time)
        {
            if (!enemy.IsDestroyed())
            {
                m_active_enemies.emplace_back(&enemy);
            }
        });

    Command missileGuider;
    missileGuider.category = static_cast<int>(ReceiverCategories::kAlliedProjectile);
    missileGuider.action = DerivedAction<Projectile>([this](Projectile& missile, sf::Time)
        {
            // NOTE: Projectile::IsGuided()/GuideTowards() are removed once
            // you apply the projectile.cpp patch from
            // TANK_CONVERSION_README.md (no more missiles) - this whole
            // method becomes dead code at that point (the command it
            // pushes will just never find anything guided to act on).
            // Left compiling for now since Aircraft/old bullets still
            // reference Projectile the old way until Phase 5 cleanup.
        });
    m_command_queue.Push(enemyCollector);
    m_command_queue.Push(missileGuider);
    m_active_enemies.clear();
}

bool MatchesCategories(SceneNode::Pair& colliders, ReceiverCategories type1, ReceiverCategories type2)
{
    unsigned int category1 = colliders.first->GetCategory();
    unsigned int category2 = colliders.second->GetCategory();

    if ((static_cast<int>(type1) & category1) && (static_cast<int>(type2) & category2))
    {
        return true;
    }
    else if ((static_cast<int>(type1) & category2) && (static_cast<int>(type2) & category1))
    {
        std::swap(colliders.first, colliders.second);
        return true;
    }
    else
    {
        return false;
    }

}

void World::HandleCollisions()
{
    std::set<SceneNode::Pair> collision_pairs;
    m_scene_graph.CheckSceneCollision(m_scene_graph, collision_pairs);

    for (SceneNode::Pair pair : collision_pairs)
    {
        if (MatchesCategories(pair, ReceiverCategories::kPlayerAircraft, ReceiverCategories::kEnemyAircraft))
        {
            auto& player = static_cast<Aircraft&>(*pair.first);
            auto& enemy = static_cast<Aircraft&>(*pair.second);
            //Collision response
            player.Damage(enemy.GetHitPoints());
            enemy.Destroy();
        }
        else if (MatchesCategories(pair, ReceiverCategories::kPlayerAircraft, ReceiverCategories::kPickup))
        {
            auto& player = static_cast<Aircraft&>(*pair.first);
            auto& pickup = static_cast<Pickup&>(*pair.second);
            //Collision response
            pickup.Apply(player);
            pickup.Destroy();
            player.PlayLocalSound(m_command_queue, SoundEffect::kCollectPickup);
        }
        else if (MatchesCategories(pair, ReceiverCategories::kPlayerAircraft, ReceiverCategories::kEnemyProjectile) || MatchesCategories(pair, ReceiverCategories::kEnemyAircraft, ReceiverCategories::kAlliedProjectile))
        {
            auto& aircraft = static_cast<Aircraft&>(*pair.first);
            auto& projectile = static_cast<Projectile&>(*pair.second);
            //Collision response
            aircraft.Damage(projectile.GetDamage());
            projectile.Destroy();
        }
        // Opposing-team shell hits a tank
        else if (MatchesCategories(pair, ReceiverCategories::kAxisTeamTank, ReceiverCategories::kAlliesTeamProjectile) ||
            MatchesCategories(pair, ReceiverCategories::kAlliesTeamTank, ReceiverCategories::kAxisTeamProjectile))
        {
            auto& tank = static_cast<Tank&>(*pair.first);
            auto& shell = static_cast<Projectile&>(*pair.second);

            tank.Damage(static_cast<int>(shell.GetDamage()));
            shell.Destroy();
            // No local kill-registration here - in multiplayer, GameServer
            // detects the kill itself from the relayed hitpoints reaching
            // zero. In the tutorial, IsTutorialComplete() checks
            // m_tutorial_enemy_tank->IsDestroyed() directly - see GameState.
        }
        // Tank vs debris - physically blocked
        else if (MatchesCategories(pair, ReceiverCategories::kAnyTank, ReceiverCategories::kObstacle))
        {
            auto& tank = static_cast<Tank&>(*pair.first);
            auto& debris = *pair.second;
            tank.move(ResolveAabbPushOut(tank.GetBoundingRect(), debris.GetBoundingRect()));
        }
        // Shell vs debris - shell destroyed
        else if (MatchesCategories(pair, ReceiverCategories::kAnyTankProjectile, ReceiverCategories::kObstacle))
        {
            static_cast<Projectile&>(*pair.first).Destroy();
        }
    }
}

void World::DestroyEntitiesOutsideView()
{
    Command command;
    command.category = static_cast<int>(ReceiverCategories::kEnemyAircraft)
        | static_cast<int>(ReceiverCategories::kProjectile)
        | static_cast<int>(ReceiverCategories::kAnyTankProjectile);
    command.action = DerivedAction<Entity>([this](Entity& e, sf::Time dt)
        {
            //Does the object intersect with the battlefield
            if (GetBattleFieldBounds().findIntersection(e.GetBoundingRect()) == std::nullopt)
            {
                e.Remove();
            }
        });
    m_command_queue.Push(command);

}

void World::UpdateSounds()
{
    sf::Vector2f listener_position;

    // 0 players (multiplayer mode, until server is connected) -> view center
    if (m_player_aircraft.empty())
    {
        listener_position = m_camera.getCenter();
    }

    // 1 or more players -> mean position between all aircrafts
    else
    {
        for (Aircraft* aircraft : m_player_aircraft)
        {
            listener_position += aircraft->GetWorldPosition();
        }

        listener_position /= static_cast<float>(m_player_aircraft.size());
    }

    m_sounds.SetListenerPosition(listener_position);

    m_sounds.RemoveStoppedSounds();
}

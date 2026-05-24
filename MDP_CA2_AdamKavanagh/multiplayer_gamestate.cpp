#include "multiplayer_gamestate.hpp"
#include "music_player.hpp"
#include "utility.hpp"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Network/Packet.hpp>
#include <SFML/Network/IpAddress.hpp>

#include <fstream>
#include "pickup_type.hpp"
#include <iostream>

sf::IpAddress GetAddressFromFile()
{
    {
        //Try to open existing file
        std::ifstream input_file("ip.txt");
        std::string ip_address;
        if (input_file >> ip_address)
        {
            if (auto address = sf::IpAddress::resolve(ip_address))
            {
                return *address;
            }
        }
    }

    //If the open/read failed, create a new file
    std::ofstream output_file("ip.txt");
    sf::IpAddress local_address = sf::IpAddress::LocalHost;
    output_file << local_address.toString();
    return local_address;
}

MultiplayerGameState::MultiplayerGameState(StateStack& stack, Context context, bool is_host)
    :State(stack, context)
    , m_world(*context.window, *context.fonts, *context.sound, true)
    , m_window(*context.window)
    , m_texture_holder(*context.textures)
    , m_connected(false)
    , m_game_server(nullptr)
    , m_active_state(true)
    , m_has_focus(true)
    , m_host(is_host)
    , m_game_started(false)
    , m_client_timeout(sf::seconds(1.f))
    , m_time_since_last_packet(sf::seconds(0.f))
    , m_broadcast_text(context.fonts->Get(FontID::kMain))
    , m_player_invitation_text(context.fonts->Get(FontID::kMain))
    , m_failed_connection_text(context.fonts->Get(FontID::kMain))
{
    m_broadcast_text.setPosition(sf::Vector2f(1024.f / 2, 100.f));

    m_player_invitation_text.setCharacterSize(20);
    m_player_invitation_text.setFillColor(sf::Color::White);
    m_player_invitation_text.setString("Press Enter to spawn player 2");
    m_player_invitation_text.setPosition(sf::Vector2f(1000 - m_player_invitation_text.getLocalBounds().size.x, 760 - m_player_invitation_text.getLocalBounds().size.y));

    //Use this for "Attempt to connect" and "Failed to connect" messages
    m_failed_connection_text.setCharacterSize(35);
    m_failed_connection_text.setFillColor(sf::Color::White);
    m_failed_connection_text.setString("Attempting to connect...");
    Utility::CentreOrigin(m_failed_connection_text);
    m_failed_connection_text.setPosition(sf::Vector2f(m_window.getSize().x / 2.f, m_window.getSize().y / 2.f));

    //Render an establishing connection frame for user feedback
    m_window.clear(sf::Color::Black);
    m_window.draw(m_failed_connection_text);
    m_window.display();
    m_failed_connection_text.setString("Failed to connect to server");
    Utility::CentreOrigin(m_failed_connection_text);

    //If this is the host, create a server
    std::optional<sf::IpAddress> ip;

    if (m_host)
    {
        m_game_server.reset(new GameServer(sf::Vector2f(m_window.getSize())));
        ip = sf::IpAddress::LocalHost;
    }
    else
    {
        ip = GetAddressFromFile();
    }

    if (ip)
    {
        auto status = m_socket.connect(*ip, SERVER_PORT, sf::seconds(5.f));

        if (status == sf::Socket::Status::Done)
        {
            m_connected = true;
        }
        else
        {
            m_failed_connection_clock.restart();
        }
    }
    else
    {
        m_failed_connection_clock.restart();
    }

    //Set socket to non-blocking
    m_socket.setBlocking(false);

    //Play the game music
    context.music->Play(MusicThemes::kMissionTheme);

}

void MultiplayerGameState::Draw()
{
    if (m_connected)
    {
        m_world.Draw();

        //Show the broadcast message in default view
        m_window.setView(m_window.getDefaultView());

        if (!m_broadcasts.empty())
        {
            m_window.draw(m_broadcast_text);
        }
        // Co-op text rendering removed here
    }
    else
    {
        m_window.draw(m_failed_connection_text);
    }
}

bool MultiplayerGameState::Update(sf::Time dt)
{
    //Connected to the Server: Handle all the network logic
    if (m_connected)
    {
        if (!m_active_state)
            DisableAllRealtimeActions(true);

        m_world.Update(dt);

        // --- TURRET MOUSE AIMING ---
        sf::Vector2i mouse_pos = sf::Mouse::getPosition(m_window);
        sf::Vector2f world_mouse_pos = m_window.mapPixelToCoords(mouse_pos);

        // Aim the local player's turret at the cursor (NO PACKET STUFF HERE)
        for (uint8_t identifier : m_local_player_identifiers)
        {
            if (Aircraft* aircraft = m_world.GetAircraft(identifier))
            {
                aircraft->AimTurretTowards(world_mouse_pos);
            }
        }

        //Remove players whose aircraft were destroyed
        bool found_local_plane = false;
        for (auto itr = m_players.begin(); itr != m_players.end();)
        {
            if (std::find(m_local_player_identifiers.begin(), m_local_player_identifiers.end(), itr->first) != m_local_player_identifiers.end())
            {
                found_local_plane = true;
            }

            if (!m_world.GetAircraft(itr->first))
            {
                itr = m_players.erase(itr);
                if (m_players.empty())
                {
                    RequestStackPush(StateID::kGameOver);
                }
            }
            else
            {
                ++itr;
            }
        }

        if (!found_local_plane && m_game_started)
        {
            RequestStackPush(StateID::kGameOver);
        }

        //Only handle the realtime input if the window has focus and the game is unpaused
        if (m_active_state && m_has_focus)
        {
            CommandQueue& commands = m_world.GetCommandQueue();
            for (auto& pair : m_players)
            {
                pair.second->HandleRealTimeInput(commands);
            }
        }

        //Always handle the network input
        CommandQueue& commands = m_world.GetCommandQueue();
        for (auto& pair : m_players)
        {
            pair.second->HandleRealtimeNetworkInput(commands);
        }

        //Handle messages from the server that may have arrived
        sf::Packet packet;
        if (m_socket.receive(packet) == sf::Socket::Status::Done)
        {
            m_time_since_last_packet = sf::seconds(0.f);
            uint8_t packet_type;
            packet >> packet_type;
            HandlePacket(packet_type, packet);
        }
        else
        {
            //Check for timeout with the server
            if (m_time_since_last_packet > m_client_timeout)
            {
                m_connected = false;
                m_failed_connection_text.setString("Lost connection to the server");
                Utility::CentreOrigin(m_failed_connection_text);

                m_failed_connection_clock.restart();
            }
        }

        UpdateBroadcastMessage(dt);

        //Events occurring in the game
        GameActions::Action game_action;
        while (m_world.PollGameAction(game_action))
        {
            sf::Packet action_packet;
            action_packet << static_cast<uint8_t>(Client::PacketType::kGameEvent);
            action_packet << static_cast<uint8_t>(game_action.type);
            action_packet << game_action.position.x;
            action_packet << game_action.position.y;

            (void)m_socket.send(action_packet);
        }

        // --- REGULAR POSITION UPDATES (PACKET DECLARED HERE) ---
        if (m_tick_clock.getElapsedTime() > sf::seconds(1.f / 20.f))
        {
            sf::Packet position_update_packet;

            position_update_packet << static_cast<uint8_t>(Client::PacketType::kStateUpdate);
            position_update_packet << static_cast<uint8_t>(m_local_player_identifiers.size());

            for (uint8_t identifier : m_local_player_identifiers)
            {
                if (Aircraft* aircraft = m_world.GetAircraft(identifier))
                {
                    position_update_packet << identifier
                        << aircraft->getPosition().x
                        << aircraft->getPosition().y
                        << static_cast<uint8_t>(aircraft->GetHitPoints())
                        << static_cast<uint8_t>(aircraft->GetMissileAmmo())
                        << aircraft->getRotation().asDegrees() // Chassis heading
                        << aircraft->GetTurretRotation();      // Turret heading
                }
            }

            (void)m_socket.send(position_update_packet);
            m_tick_clock.restart();
        }
        m_time_since_last_packet += dt;
    }
    else if (m_failed_connection_clock.getElapsedTime() >= sf::seconds(5.f))
    {
        RequestStackClear();
        RequestStackPush(StateID::kMenu);
    }
    return true;
}



bool MultiplayerGameState::HandleEvent(const sf::Event& event)
{
    CommandQueue& commands = m_world.GetCommandQueue();

    //Forward events to all players
    for (auto& pair : m_players)
    {
        pair.second->HandleEvent(event, commands);
    }
    const auto* key_pressed = event.getIf<sf::Event::KeyPressed>();
    if (key_pressed)
    {
        // Co-op enter key check removed here

        //If escape is pressed, show the pause screen
        if (key_pressed->scancode == sf::Keyboard::Scancode::Escape)
        {
            DisableAllRealtimeActions(false);
            RequestStackPush(StateID::kNetworkPause);
        }
    }
    else if (event.is<sf::Event::FocusGained>())
    {
        m_has_focus = true;
    }
    else if (event.is<sf::Event::FocusLost>())
    {
        m_has_focus = false;
    }
    return true;
}

void MultiplayerGameState::OnActivate()
{
    m_active_state = true;
}

void MultiplayerGameState::OnDestroy()
{
    if (!m_host && m_connected)
    {
        //Inform server this client is dying
        sf::Packet packet;
        packet << static_cast<uint8_t>(Client::PacketType::kQuit);
        m_socket.send(packet);
    }
}

void MultiplayerGameState::DisableAllRealtimeActions(bool enable)
{
    m_active_state = enable;
    for (uint8_t identifier : m_local_player_identifiers)
    {
        m_players[identifier]->DisableAllRealtimeActions(enable);
    }
}

void MultiplayerGameState::UpdateBroadcastMessage(sf::Time elapsed_time)
{
    if (m_broadcasts.empty())
    {
        return;
    }

    //Update broadcast timer
    m_broadcast_elapsed_time += elapsed_time;
    if (m_broadcast_elapsed_time > sf::seconds(2.f))
    {
        //If message has expired, remove it
        m_broadcasts.erase(m_broadcasts.begin());

        //Continue to display the next broadcast message
        if (!m_broadcasts.empty())
        {
            m_broadcast_text.setString(m_broadcasts.front());
            Utility::CentreOrigin(m_broadcast_text);
            m_broadcast_elapsed_time = sf::Time::Zero;
        }
    }
}

void MultiplayerGameState::HandlePacket(uint8_t packet_type, sf::Packet& packet)
{
    switch (static_cast<Server::PacketType>(packet_type))
    {
    case Server::PacketType::kBroadcastMessage:
    {
        std::string message;
        packet >> message;
        m_broadcasts.push_back(message);

        if (m_broadcasts.size() == 1)
        {
            m_broadcast_text.setString(m_broadcasts.front());
            Utility::CentreOrigin(m_broadcast_text);
            m_broadcast_elapsed_time = sf::Time::Zero;
        }
    }
    break;

    case Server::PacketType::kSpawnSelf:
    {
        uint8_t aircraft_identifier;
        sf::Vector2f aircraft_position;
        packet >> aircraft_identifier >> aircraft_position.x >> aircraft_position.y;

        Aircraft* aircraft = m_world.AddAircraft(aircraft_identifier);
        aircraft->setPosition(aircraft_position);
        m_players[aircraft_identifier].reset(new Player(&m_socket, aircraft_identifier, GetContext().keys1));
        m_local_player_identifiers.push_back(aircraft_identifier);
        m_game_started = true;
    }
    break;

    case Server::PacketType::kPlayerConnect:
    {
        uint8_t aircraft_identifier;
        sf::Vector2f aircraft_position;
        packet >> aircraft_identifier >> aircraft_position.x >> aircraft_position.y;

        Aircraft* aircraft = m_world.AddAircraft(aircraft_identifier);
        aircraft->setPosition(aircraft_position);
        m_players[aircraft_identifier].reset(new Player(&m_socket, aircraft_identifier, nullptr));
    }
    break;

    case Server::PacketType::kPlayerDisconnect:
    {
        uint8_t aircraft_identifier;
        packet >> aircraft_identifier;
        m_world.RemoveAircraft(aircraft_identifier);
        m_players.erase(aircraft_identifier);
    }
    break;

    case Server::PacketType::kInitialState:
    {
        uint8_t aircraft_count;
        float world_height, current_scroll;
        packet >> world_height >> current_scroll;

        m_world.SetWorldHeight(world_height);
        m_world.SetCurrentBattleFieldPosition(current_scroll);

        packet >> aircraft_count;
        for (uint8_t i = 0; i < aircraft_count; ++i)
        {
            uint8_t aircraft_identifier;
            uint8_t hitpoints;
            uint8_t missile_ammo;
            sf::Vector2f aircraft_position;
            packet >> aircraft_identifier >> aircraft_position.x >> aircraft_position.y >> hitpoints >> missile_ammo;

            Aircraft* aircraft = m_world.AddAircraft(aircraft_identifier);
            aircraft->setPosition(aircraft_position);
            aircraft->SetHitpoints(hitpoints);
            aircraft->SetMissileAmmo(missile_ammo);

            m_players[aircraft_identifier].reset(new Player(&m_socket, aircraft_identifier, nullptr));
        }
    }
    break;

    case Server::PacketType::kPlayerEvent:
    {
        uint8_t aircraft_identifier;
        uint8_t action;
        packet >> aircraft_identifier >> action;

        auto itr = m_players.find(aircraft_identifier);
        if (itr != m_players.end())
        {
            itr->second->HandleNetworkEvent(static_cast<Action>(action), m_world.GetCommandQueue());
        }
    }
    break;

    case Server::PacketType::kPlayerRealtimeChange:
    {
        uint8_t aircraft_identifier;
        uint8_t action;
        bool action_enabled;
        packet >> aircraft_identifier >> action >> action_enabled;

        auto itr = m_players.find(aircraft_identifier);
        if (itr != m_players.end())
        {
            itr->second->HandleNetworkRealtimeChange(static_cast<Action>(action), action_enabled);
        }
    }
    break;

    case Server::PacketType::kSpawnEnemy:
    {
        float height;
        uint8_t type;
        float relative_x;
        packet >> type >> height >> relative_x;

        m_world.AddEnemy(static_cast<AircraftType>(type), relative_x, height);
        m_world.SortEnemies();
    }
    break;

    case Server::PacketType::kMissionSuccess:
    {
        RequestStackPush(StateID::kMissionSuccess);
    }
    break;

    case Server::PacketType::kSpawnPickup:
    {
        uint8_t type;
        sf::Vector2f position;
        packet >> type >> position.x >> position.y;
        m_world.CreatePickup(position, static_cast<PickupType>(type));
    }
    break;

    case Server::PacketType::kUpdateClientState:
    {
        float current_world_position;
        uint8_t aircraft_count;
        packet >> current_world_position >> aircraft_count;

        float current_view_position = m_world.GetViewBounds().position.y + m_world.GetViewBounds().size.y;
        m_world.SetWorldScrollCompensation(current_view_position / current_world_position);

        for (uint8_t i = 0; i < aircraft_count; ++i)
        {
            sf::Vector2f aircraft_position;
            uint8_t aircraft_identifier;
            uint8_t hitpoints;
            uint8_t ammo;

            // --- NEW: Unpack the rotations! ---
            float chassis_rot, turret_rot;
            packet >> aircraft_identifier >> aircraft_position.x >> aircraft_position.y >> hitpoints >> ammo >> chassis_rot >> turret_rot;

            Aircraft* aircraft = m_world.GetAircraft(aircraft_identifier);
            bool is_local_plane = std::find(m_local_player_identifiers.begin(), m_local_player_identifiers.end(), aircraft_identifier) != m_local_player_identifiers.end();

            if (aircraft && !is_local_plane)
            {
                // Smoothly interpolate positions
                sf::Vector2f interpolated_position = aircraft->getPosition() + (aircraft_position - aircraft->getPosition()) * 0.1f;
                aircraft->setPosition(interpolated_position);
                aircraft->SetHitpoints(hitpoints);
                aircraft->SetMissileAmmo(ammo);

                // Apply networked rotations
                aircraft->setRotation(sf::degrees(chassis_rot));
                aircraft->SetTurretRotation(turret_rot);
            }
        }
    }
    break;
    }
}
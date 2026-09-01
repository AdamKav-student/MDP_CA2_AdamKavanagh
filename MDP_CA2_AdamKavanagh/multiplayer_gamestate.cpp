// Adam Kavanagh - D00247069
#include "multiplayer_gamestate.hpp"
#include "constants.hpp"
#include "music_player.hpp"
#include "team_assignment.hpp"
#include "utility.hpp"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Network/IpAddress.hpp>
#include <SFML/Network/Packet.hpp>

#include <fstream>
#include <iostream>
#include <sstream>

namespace
{
    // A joining client reads the host's address from ip.txt beside the
    // executable, so the game can be pointed at another machine in the lab
    // without rebuilding it.
    sf::IpAddress GetAddressFromFile()
    {
        std::ifstream input_file("ip.txt");
        std::string ip_address;
        if (input_file >> ip_address)
        {
            if (auto address = sf::IpAddress::resolve(ip_address))
            {
                return *address;
            }
        }

        std::ofstream output_file("ip.txt");
        output_file << sf::IpAddress::LocalHost.toString();
        return sf::IpAddress::LocalHost;
    }

    std::string FormatClock(uint16_t seconds_remaining)
    {
        std::ostringstream stream;
        stream << seconds_remaining / 60 << ':'
            << (seconds_remaining % 60 < 10 ? "0" : "") << seconds_remaining % 60;
        return stream.str();
    }
}

MultiplayerGameState::MultiplayerGameState(StateStack& stack, Context context, bool is_host)
    : State(stack, context)
    , m_world(*context.window, *context.fonts, *context.sound, true)
    , m_window(*context.window)
    , m_local_player_identifier(0)
    , m_has_local_tank(false)
    , m_connected(false)
    , m_game_server(nullptr)
    , m_broadcast_text(context.fonts->Get(FontID::kMain))
    , m_scoreboard_text(context.fonts->Get(FontID::kMain))
    , m_failed_connection_text(context.fonts->Get(FontID::kMain))
    , m_allies_score(0)
    , m_axis_score(0)
    , m_seconds_remaining(static_cast<uint16_t>(kMatchDurationSeconds))
    , m_active_state(true)
    , m_has_focus(true)
    , m_host(is_host)
    , m_game_started(false)
    , m_client_timeout(sf::seconds(5.f))
    , m_time_since_last_packet(sf::Time::Zero)
{
    m_broadcast_text.setCharacterSize(22);
    m_broadcast_text.setPosition(sf::Vector2f(m_window.getSize().x / 2.f, 90.f));

    m_scoreboard_text.setCharacterSize(22);
    m_scoreboard_text.setFillColor(sf::Color::White);
    m_scoreboard_text.setPosition(sf::Vector2f(20.f, 20.f));

    m_failed_connection_text.setCharacterSize(35);
    m_failed_connection_text.setFillColor(sf::Color::White);
    m_failed_connection_text.setString("Attempting to connect...");
    Utility::CentreOrigin(m_failed_connection_text);
    m_failed_connection_text.setPosition(sf::Vector2f(m_window.getSize().x / 2.f, m_window.getSize().y / 2.f));

    // Draw one frame by hand so the player sees something while the connect
    // call blocks.
    m_window.clear(sf::Color::Black);
    m_window.draw(m_failed_connection_text);
    m_window.display();
    m_failed_connection_text.setString("Failed to connect to the server");
    Utility::CentreOrigin(m_failed_connection_text);

    std::optional<sf::IpAddress> ip;
    if (m_host)
    {
        m_game_server.reset(new GameServer());
        ip = sf::IpAddress::LocalHost;
    }
    else
    {
        ip = GetAddressFromFile();
    }

    if (ip && m_socket.connect(*ip, SERVER_PORT, sf::seconds(5.f)) == sf::Socket::Status::Done)
    {
        m_connected = true;
    }
    else
    {
        m_failed_connection_clock.restart();
    }

    // Non-blocking from here on: the game loop must keep running whether or
    // not a packet happens to be waiting.
    m_socket.setBlocking(false);

    UpdateScoreboardText();
    context.music->Play(MusicThemes::kMissionTheme);
}

void MultiplayerGameState::Draw()
{
    if (!m_connected)
    {
        m_window.setView(m_window.getDefaultView());
        m_window.draw(m_failed_connection_text);
        return;
    }

    m_world.Draw();

    m_window.setView(m_window.getDefaultView());
    m_window.draw(m_scoreboard_text);

    if (!m_broadcasts.empty())
    {
        m_window.draw(m_broadcast_text);
    }
}

bool MultiplayerGameState::Update(sf::Time dt)
{
    if (!m_connected)
    {
        // Give up after five seconds of failing to connect and go back rather
        // than leaving the player staring at an error forever.
        if (m_failed_connection_clock.getElapsedTime() >= sf::seconds(5.f))
        {
            RequestStackClear();
            RequestStackPush(StateID::kMenu);
        }
        return true;
    }

    m_world.Update(dt);

    // Players are only removed when the server says that client has gone
    // (kPlayerDisconnect). A tank being absent just means its owner is
    // between being knocked out and respawning, and dropping the Player then
    // would leave the respawned tank with nothing driving it.
    CommandQueue& commands = m_world.GetCommandQueue();

    if (m_active_state && m_has_focus)
    {
        for (auto& pair : m_players)
        {
            pair.second->HandleRealtimeInput(commands);
        }
    }

    // Remote tanks are always driven, focus or not - they belong to somebody
    // else's keyboard.
    for (auto& pair : m_players)
    {
        pair.second->HandleRealtimeNetworkInput(commands);
    }

    sf::Packet packet;
    if (m_socket.receive(packet) == sf::Socket::Status::Done)
    {
        m_time_since_last_packet = sf::Time::Zero;
        uint8_t packet_type = 0;
        packet >> packet_type;
        HandlePacket(packet_type, packet);
    }
    else if (m_time_since_last_packet > m_client_timeout)
    {
        m_connected = false;
        m_failed_connection_text.setString("Lost connection to the server");
        Utility::CentreOrigin(m_failed_connection_text);
        m_failed_connection_clock.restart();
    }

    UpdateBroadcastMessage(dt);
    SendPendingGameActions();
    FlushSendQueue();

    if (m_tick_clock.getElapsedTime() > sf::seconds(1.f / kClientTickRate))
    {
        SendOwnState();
        m_tick_clock.restart();
    }

    m_time_since_last_packet += dt;
    return true;
}

void MultiplayerGameState::SendPacket(const sf::Packet& packet)
{
    constexpr std::size_t kMaxQueuedPackets = 64;
    if (m_send_queue.size() >= kMaxQueuedPackets)
    {
        // Our own state update is superseded 20 times a second, so dropping
        // the oldest one is better than growing without bound.
        m_send_queue.pop_front();
    }
    m_send_queue.push_back(packet);
}

void MultiplayerGameState::FlushSendQueue()
{
    while (!m_send_queue.empty())
    {
        const sf::Socket::Status status = m_socket.send(m_send_queue.front());

        if (status == sf::Socket::Status::Done)
        {
            m_send_queue.pop_front();
            continue;
        }

        if (status == sf::Socket::Status::Partial || status == sf::Socket::Status::NotReady)
        {
            break;
        }

        m_connected = false;
        m_failed_connection_text.setString("Lost connection to the server");
        Utility::CentreOrigin(m_failed_connection_text);
        m_failed_connection_clock.restart();
        m_send_queue.clear();
        break;
    }
}

void MultiplayerGameState::SendOwnState()
{
    if (!m_has_local_tank)
    {
        return;
    }

    Tank* tank = m_world.GetTank(m_local_player_identifier);

    TankSnapshot snapshot;
    snapshot.m_identifier = m_local_player_identifier;

    if (!tank)
    {
        // Between being knocked out and respawning there is no tank to report,
        // but the stream must not go quiet: the server evicts a peer it has
        // not heard from for three seconds, and the respawn delay is four.
        snapshot.m_hitpoints = 0;

        sf::Packet dead_packet;
        dead_packet << static_cast<uint8_t>(Client::PacketType::kStateUpdate);
        dead_packet << snapshot;
        SendPacket(dead_packet);
        return;
    }

    // One 8-byte snapshot, 20 times a second. This is the only continuous
    // traffic a client generates.
    snapshot.SetPosition(tank->getPosition());
    snapshot.SetHullRotation(tank->getRotation().asDegrees());
    snapshot.SetTurretRotation(tank->GetTurretRotationDegrees());
    snapshot.m_hitpoints = NetCompression::PackHitpoints(tank->GetHitPoints());

    sf::Packet packet;
    packet << static_cast<uint8_t>(Client::PacketType::kStateUpdate);
    packet << snapshot;
    SendPacket(packet);
}

void MultiplayerGameState::SendPendingGameActions()
{
    GameActions::Action action;
    while (m_world.PollGameAction(action))
    {
        sf::Packet packet;
        packet << static_cast<uint8_t>(Client::PacketType::kGameEvent);
        packet << static_cast<uint8_t>(action.type);
        packet << action.subject;
        packet << action.other;
        packet << NetCompression::PackCoordinate(action.position.x);
        packet << NetCompression::PackCoordinate(action.position.y);
        SendPacket(packet);
    }
}

bool MultiplayerGameState::HandleEvent(const sf::Event& event)
{
    CommandQueue& commands = m_world.GetCommandQueue();

    for (auto& pair : m_players)
    {
        pair.second->HandleEvent(event, commands);
    }

    if (const auto* key_pressed = event.getIf<sf::Event::KeyPressed>())
    {
        if (key_pressed->scancode == sf::Keyboard::Scancode::Escape)
        {
            DisableAllRealtimeActions();
            m_active_state = false;
            RequestStackPush(StateID::kNetworkPause);
        }
    }
    else if (event.is<sf::Event::FocusGained>())
    {
        m_has_focus = true;
    }
    else if (event.is<sf::Event::FocusLost>())
    {
        // Release every key on the way out, otherwise this tank would carry on
        // driving on everybody else's screen.
        m_has_focus = false;
        DisableAllRealtimeActions();
    }

    return true;
}

void MultiplayerGameState::OnActivate()
{
    m_active_state = true;
}

void MultiplayerGameState::OnDestroy()
{
    if (m_connected)
    {
        // A clean goodbye means the server frees the slot immediately instead
        // of waiting out the timeout.
        sf::Packet packet;
        packet << static_cast<uint8_t>(Client::PacketType::kQuit);
        // Sent directly rather than queued: this state is about to be
        // destroyed, so there will be no later flush.
        m_socket.setBlocking(true);
        (void)m_socket.send(packet);
    }
}

void MultiplayerGameState::DisableAllRealtimeActions()
{
    // Local crew only. Clearing the proxies of remote tanks would freeze them
    // on this screen until their owner next pressed or released a key.
    auto local = m_players.find(m_local_player_identifier);
    if (m_has_local_tank && local != m_players.end())
    {
        local->second->DisableAllRealtimeActions();
    }
}

void MultiplayerGameState::UpdateBroadcastMessage(sf::Time elapsed_time)
{
    if (m_broadcasts.empty())
    {
        return;
    }

    m_broadcast_elapsed_time += elapsed_time;
    if (m_broadcast_elapsed_time > sf::seconds(2.5f))
    {
        m_broadcasts.erase(m_broadcasts.begin());

        if (!m_broadcasts.empty())
        {
            m_broadcast_text.setString(m_broadcasts.front());
            Utility::CentreOrigin(m_broadcast_text);
            m_broadcast_elapsed_time = sf::Time::Zero;
        }
    }
}

void MultiplayerGameState::UpdateScoreboardText()
{
    std::ostringstream stream;
    stream << "ALLIES " << m_allies_score << "   -   AXIS " << m_axis_score
        << "      " << FormatClock(m_seconds_remaining);

    if (m_has_local_tank)
    {
        stream << "\nYou: " << (AssignTeam(m_local_player_identifier) == TeamID::kAllies ? "Allies" : "Axis")
            << " #" << static_cast<int>(m_local_player_identifier);
    }

    m_scoreboard_text.setString(stream.str());
}

void MultiplayerGameState::ApplySnapshot(const TankSnapshot& snapshot, bool is_spawn)
{
    Tank* tank = m_world.GetTank(snapshot.m_identifier);
    if (!tank)
    {
        return;
    }

    const bool is_local = (snapshot.m_identifier == m_local_player_identifier);

    if (is_spawn)
    {
        // A spawn or respawn is applied verbatim to everybody, our own tank
        // included: the server decides where a tank comes back.
        tank->setPosition(snapshot.GetPosition());
        tank->setRotation(sf::degrees(snapshot.GetHullRotation()));
        tank->SetTurretRotationDegrees(snapshot.GetTurretRotation());
        tank->SetHitpoints(snapshot.m_hitpoints);
        return;
    }

    if (is_local)
    {
        // Our own tank is simulated locally from our own keyboard, so the
        // relayed copy of it is ignored - snapping to it would fight the
        // player's input and produce visible rubber-banding.
        return;
    }

    // Remote tanks are eased towards the authoritative position rather than
    // teleported. Between snapshots they keep moving under the relayed key
    // state, so this only has to correct the small drift that builds up.
    const sf::Vector2f target = snapshot.GetPosition();
    tank->setPosition(tank->getPosition() + (target - tank->getPosition()) * 0.25f);
    tank->setRotation(sf::degrees(snapshot.GetHullRotation()));
    tank->SetTurretRotationDegrees(snapshot.GetTurretRotation());
    tank->SetHitpoints(snapshot.m_hitpoints);
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

    case Server::PacketType::kInitialState:
    {
        uint8_t tank_count = 0;
        packet >> m_seconds_remaining >> m_allies_score >> m_axis_score >> tank_count;

        for (uint8_t i = 0; i < tank_count; ++i)
        {
            TankSnapshot snapshot;
            packet >> snapshot;

            Tank* tank = m_world.AddTank(snapshot.m_identifier);
            tank->setPosition(snapshot.GetPosition());
            tank->setRotation(sf::degrees(snapshot.GetHullRotation()));
            tank->SetTurretRotationDegrees(snapshot.GetTurretRotation());
            tank->SetHitpoints(snapshot.m_hitpoints);

            m_players[snapshot.m_identifier].reset(new Player(this, snapshot.m_identifier, nullptr));
        }

        UpdateScoreboardText();
    }
    break;

    case Server::PacketType::kSpawnSelf:
    {
        TankSnapshot snapshot;
        packet >> snapshot;

        m_local_player_identifier = snapshot.m_identifier;
        m_has_local_tank = true;

        Tank* tank = m_world.AddTank(snapshot.m_identifier);
        tank->setPosition(snapshot.GetPosition());
        tank->setRotation(sf::degrees(snapshot.GetHullRotation()));

        m_world.SetLocalPlayerIdentifier(snapshot.m_identifier);

        // Only this Player gets the key bindings; every other Player on this
        // machine is a network-driven proxy.
        m_players[snapshot.m_identifier].reset(new Player(this, snapshot.m_identifier, GetContext().keys));
        m_game_started = true;

        UpdateScoreboardText();
    }
    break;

    case Server::PacketType::kPlayerConnect:
    {
        TankSnapshot snapshot;
        packet >> snapshot;

        Tank* tank = m_world.AddTank(snapshot.m_identifier);
        tank->setPosition(snapshot.GetPosition());
        tank->setRotation(sf::degrees(snapshot.GetHullRotation()));

        m_players[snapshot.m_identifier].reset(new Player(this, snapshot.m_identifier, nullptr));
    }
    break;

    case Server::PacketType::kPlayerDisconnect:
    {
        uint8_t identifier = 0;
        packet >> identifier;
        m_world.RemoveTank(identifier);
        m_players.erase(identifier);
    }
    break;

    case Server::PacketType::kPlayerEvent:
    {
        uint8_t identifier = 0;
        uint8_t action = 0;
        packet >> identifier >> action;

        auto itr = m_players.find(identifier);
        if (itr != m_players.end())
        {
            itr->second->HandleNetworkEvent(static_cast<Action>(action), m_world.GetCommandQueue());
        }
    }
    break;

    case Server::PacketType::kPlayerRealtimeChange:
    {
        uint8_t identifier = 0;
        uint8_t action = 0;
        bool action_enabled = false;
        packet >> identifier >> action >> action_enabled;

        auto itr = m_players.find(identifier);
        if (itr != m_players.end())
        {
            itr->second->HandleNetworkRealtimeChange(static_cast<Action>(action), action_enabled);
        }
    }
    break;

    case Server::PacketType::kUpdateClientState:
    {
        uint8_t tank_count = 0;
        packet >> tank_count;

        for (uint8_t i = 0; i < tank_count; ++i)
        {
            TankSnapshot snapshot;
            packet >> snapshot;
            ApplySnapshot(snapshot, false);
        }
    }
    break;

    case Server::PacketType::kTankDestroyed:
    {
        uint8_t victim = 0;
        uint8_t killer = 0;
        packet >> victim >> killer;

        // Destroy() rather than Remove() so the wreck plays its explosion.
        if (Tank* tank = m_world.GetTank(victim))
        {
            tank->Destroy();
        }

        std::ostringstream stream;
        stream << "Player " << static_cast<int>(killer) << " knocked out Player " << static_cast<int>(victim);
        m_broadcasts.push_back(stream.str());
        if (m_broadcasts.size() == 1)
        {
            m_broadcast_text.setString(m_broadcasts.front());
            Utility::CentreOrigin(m_broadcast_text);
            m_broadcast_elapsed_time = sf::Time::Zero;
        }
    }
    break;

    case Server::PacketType::kTankRespawn:
    {
        TankSnapshot snapshot;
        packet >> snapshot;

        // The wreck may already have been swept out of the scene graph, so
        // rebuild the tank if it is gone.
        if (!m_world.GetTank(snapshot.m_identifier))
        {
            m_world.AddTank(snapshot.m_identifier);
        }
        ApplySnapshot(snapshot, true);
    }
    break;

    case Server::PacketType::kScoreUpdate:
    {
        packet >> m_allies_score >> m_axis_score >> m_seconds_remaining;
        UpdateScoreboardText();
    }
    break;

    case Server::PacketType::kMissionEnd:
    {
        uint8_t winning_team = 0;
        packet >> winning_team >> m_allies_score >> m_axis_score;

        const bool we_won = m_has_local_tank
            && static_cast<TeamID>(winning_team) == AssignTeam(m_local_player_identifier);

        RequestStackPush(we_won ? StateID::kMissionSuccess : StateID::kGameOver);
    }
    break;
    }
}

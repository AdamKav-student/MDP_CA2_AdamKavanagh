// Adam Kavanagh - D00247069
#include "game_server.hpp"
#include "constants.hpp"
#include "network_protocol.hpp"
#include "team_assignment.hpp"
#include <SFML/Network/Packet.hpp>
#include <SFML/System/Sleep.hpp>
#include <iostream>
#include <string>
#include <memory>

namespace
{
    std::string PlayerName(uint8_t identifier)
    {
        return "Player " + std::to_string(static_cast<int>(identifier));
    }
}

GameServer::GameServer()
    : m_waiting_thread_end(false)
    , m_listening_state(false)
    // Generous compared to the 20 Hz client update rate: on a busy lab network
    // a couple of dropped ticks should not evict a player who is still there.
    , m_client_timeout(sf::seconds(3.f))
    , m_max_connected_players(kMaxPlayers)
    , m_connected_players(0)
    , m_peers(1)
    , m_tank_identifier_counter(1)
    , m_allies_score(0)
    , m_axis_score(0)
    , m_match_start_time(sf::Time::Zero)
    , m_match_over(false)
    , m_last_score_broadcast(sf::Time::Zero)
    , m_high_scores("high_scores.txt")
{
    m_listener_socket.setBlocking(false);
    m_peers[0].reset(new RemotePeer());

    // Everything the server thread reads is now built, so it is safe to run.
    m_thread = std::thread(&GameServer::ExecutionThread, this);
}

GameServer::~GameServer()
{
    m_waiting_thread_end = true;
    if (m_thread.joinable())
    {
        m_thread.join();
    }
}

GameServer::RemotePeer::RemotePeer()
    : m_tank_identifier(0)
    , m_ready(false)
    , m_timed_out(false)
{
    // Non-blocking is essential: a blocking receive would stall the whole
    // server on one silent client.
    m_socket.setBlocking(false);
}

sf::Time GameServer::Now() const
{
    return m_clock.getElapsedTime();
}

void GameServer::SetListening(bool enable)
{
    if (enable)
    {
        if (!m_listening_state)
        {
            m_listening_state = (m_listener_socket.listen(SERVER_PORT) == sf::Socket::Status::Done);
        }
    }
    else
    {
        m_listener_socket.close();
        m_listening_state = false;
    }
}

void GameServer::ExecutionThread()
{
    SetListening(true);

    const sf::Time tick_rate = sf::seconds(1.f / kServerTickRate);
    sf::Time tick_time = sf::Time::Zero;
    sf::Clock tick_clock;

    m_match_start_time = Now();

    while (!m_waiting_thread_end)
    {
        HandleIncomingConnections();
        HandleIncomingPackets();
        FlushSendQueues();
        // FlushSendQueues can discover a peer whose socket has gone, so the
        // sweep runs here too rather than only after a read timeout.
        HandleDisconnections();

        tick_time += tick_clock.restart();

        while (tick_time >= tick_rate)
        {
            Tick();
            tick_time -= tick_rate;
        }

        // The host runs a game client in the same process, so the server
        // thread yields rather than spinning.
        sf::sleep(sf::milliseconds(10));
    }
}

void GameServer::Tick()
{
    if (m_match_over)
    {
        return;
    }

    ProcessRespawns();
    BroadcastChangedTankStates();

    // The clock and the scoreboard change slowly, so they go out once a
    // second rather than on every 20 Hz tick.
    if (Now() - m_last_score_broadcast >= sf::seconds(1.f))
    {
        BroadcastScores();
        m_last_score_broadcast = Now();
    }

    if (m_allies_score >= kScoreToWin)
    {
        EndMatch(TeamID::kAllies);
    }
    else if (m_axis_score >= kScoreToWin)
    {
        EndMatch(TeamID::kAxis);
    }
    else if (GetSecondsRemaining() == 0)
    {
        TeamID winner = TeamID::kNone;
        if (m_allies_score > m_axis_score)
        {
            winner = TeamID::kAllies;
        }
        else if (m_axis_score > m_allies_score)
        {
            winner = TeamID::kAxis;
        }
        EndMatch(winner);
    }
}

uint16_t GameServer::GetSecondsRemaining() const
{
    const float elapsed = (Now() - m_match_start_time).asSeconds();
    const float remaining = kMatchDurationSeconds - elapsed;
    return remaining <= 0.f ? 0 : static_cast<uint16_t>(remaining);
}

sf::Vector2f GameServer::GetSpawnPosition(uint8_t identifier) const
{
    // Teams start on opposite ends of the arena and are fanned out across it,
    // so a full 16-player lobby does not spawn on top of itself. The slot is
    // derived from the identifier, which keeps this deterministic.
    const TeamID team = AssignTeam(identifier);
    const int slot = identifier / 2;                    // 0, 1, 2, ... per team
    const float lane = static_cast<float>(slot % (static_cast<int>(kMaxPlayers) / 2));
    const float spacing = kWorldWidth / (static_cast<float>(kMaxPlayers) / 2.f + 1.f);

    const float x = spacing * (lane + 1.f);
    const float y = (team == TeamID::kAllies) ? kWorldHeight * 0.88f : kWorldHeight * 0.12f;

    return sf::Vector2f(x, y);
}

void GameServer::HandleIncomingConnections()
{
    if (!m_listening_state)
    {
        return;
    }

    if (m_listener_socket.accept(m_peers[m_connected_players]->m_socket) != sf::Socket::Status::Done)
    {
        return;
    }

    const uint8_t identifier = m_tank_identifier_counter;

    TankInfo& info = m_tank_info[identifier];
    info.m_snapshot = TankSnapshot();
    info.m_snapshot.m_identifier = identifier;
    info.m_snapshot.SetPosition(GetSpawnPosition(identifier));
    info.m_snapshot.SetHullRotation(AssignTeam(identifier) == TeamID::kAllies ? 0.f : 180.f);
    info.m_snapshot.SetTurretRotation(0.f);
    info.m_snapshot.m_hitpoints = NetCompression::PackHitpoints(kTankHitPoints);
    info.m_last_broadcast = info.m_snapshot;
    info.m_ever_broadcast = false;
    info.m_team = AssignTeam(identifier);
    info.m_alive = true;

    m_peers[m_connected_players]->m_tank_identifier = identifier;

    // Order matters: the newcomer must learn about the world that already
    // exists before anybody is told about the newcomer, otherwise it would
    // receive its own kPlayerConnect and end up with a duplicate tank.
    InformWorldState(*m_peers[m_connected_players]);

    sf::Packet spawn_self;
    spawn_self << static_cast<uint8_t>(Server::PacketType::kSpawnSelf);
    spawn_self << info.m_snapshot;
    QueuePacket(*m_peers[m_connected_players], spawn_self);

    sf::Packet notify;
    notify << static_cast<uint8_t>(Server::PacketType::kPlayerConnect);
    notify << info.m_snapshot;
    for (PeerPtr& peer : m_peers)
    {
        if (peer.get() != m_peers[m_connected_players].get() && peer->m_ready)
        {
            QueuePacket(*peer, notify);
        }
    }

    m_peers[m_connected_players]->m_ready = true;
    m_peers[m_connected_players]->m_last_packet_time = Now();

    BroadcastMessage(PlayerName(identifier) + " joined the "
        + (info.m_team == TeamID::kAllies ? "Allies" : "Axis"));
    BroadcastMessage(m_high_scores.GetSummary());

    ++m_tank_identifier_counter;
    ++m_connected_players;

    if (m_connected_players >= m_max_connected_players)
    {
        SetListening(false);
    }
    else
    {
        m_peers.emplace_back(PeerPtr(new RemotePeer()));
    }
}

void GameServer::HandleIncomingPackets()
{
    bool detected_timeout = false;

    for (PeerPtr& peer : m_peers)
    {
        if (!peer->m_ready)
        {
            continue;
        }

        sf::Packet packet;
        while (peer->m_socket.receive(packet) == sf::Socket::Status::Done)
        {
            HandleIncomingPacket(packet, *peer, detected_timeout);
            peer->m_last_packet_time = Now();
            packet.clear();
        }

        if (Now() > peer->m_last_packet_time + m_client_timeout)
        {
            peer->m_timed_out = true;
            detected_timeout = true;
        }
    }

    if (detected_timeout)
    {
        HandleDisconnections();
    }
}

void GameServer::HandleIncomingPacket(sf::Packet& packet, RemotePeer& receiving_peer, bool& detected_timeout)
{
    uint8_t packet_type = 0;
    packet >> packet_type;

    switch (static_cast<Client::PacketType>(packet_type))
    {
    case Client::PacketType::kQuit:
    {
        receiving_peer.m_timed_out = true;
        detected_timeout = true;
    }
    break;

    case Client::PacketType::kPlayerEvent:
    {
        uint8_t identifier = 0;
        uint8_t action = 0;
        packet >> identifier >> action;

        sf::Packet relay;
        relay << static_cast<uint8_t>(Server::PacketType::kPlayerEvent);
        relay << identifier << action;
        SendToAll(relay);
    }
    break;

    case Client::PacketType::kPlayerRealtimeChange:
    {
        uint8_t identifier = 0;
        uint8_t action = 0;
        bool action_enabled = false;
        packet >> identifier >> action >> action_enabled;

        // Straight relay: a key going down or coming up is 4 bytes and every
        // other client needs it to keep driving this tank between snapshots.
        sf::Packet relay;
        relay << static_cast<uint8_t>(Server::PacketType::kPlayerRealtimeChange);
        relay << identifier << action << action_enabled;
        SendToAll(relay);
    }
    break;

    case Client::PacketType::kStateUpdate:
    {
        TankSnapshot snapshot;
        packet >> snapshot;

        auto found = m_tank_info.find(snapshot.m_identifier);
        if (found == m_tank_info.end())
        {
            break;
        }

        // Only the peer that owns a tank is allowed to move it. Without this
        // check a modified client could push any other player around the map.
        if (receiving_peer.m_tank_identifier != snapshot.m_identifier)
        {
            break;
        }

        // A dead tank keeps reporting until the server respawns it; ignoring
        // those updates stops a wreck being dragged around the map.
        if (found->second.m_alive)
        {
            found->second.m_snapshot = snapshot;
        }
    }
    break;

    case Client::PacketType::kGameEvent:
    {
        uint8_t action = 0;
        uint8_t subject = 0;
        uint8_t other = 0;
        int16_t x = 0;
        int16_t y = 0;
        packet >> action >> subject >> other >> x >> y;

        if (static_cast<GameActions::Type>(action) == GameActions::kTankDestroyed)
        {
            // Only the victim's own client reports its death, and only for the
            // tank it owns, so each kill is counted exactly once.
            if (receiving_peer.m_tank_identifier == subject)
            {
                RegisterKill(subject, other);
            }
        }
    }
    break;
    }
}

void GameServer::RegisterKill(uint8_t victim, uint8_t killer)
{
    auto victim_entry = m_tank_info.find(victim);
    if (victim_entry == m_tank_info.end() || !victim_entry->second.m_alive)
    {
        return;
    }

    victim_entry->second.m_alive = false;
    victim_entry->second.m_snapshot.m_hitpoints = 0;
    victim_entry->second.m_respawn_at = Now() + sf::seconds(kRespawnDelaySeconds);

    auto killer_entry = m_tank_info.find(killer);
    const bool friendly_fire = killer_entry != m_tank_info.end()
        && killer_entry->second.m_team == victim_entry->second.m_team;

    if (killer_entry != m_tank_info.end() && killer != victim && !friendly_fire)
    {
        ++killer_entry->second.m_score;
        if (killer_entry->second.m_team == TeamID::kAllies)
        {
            ++m_allies_score;
        }
        else
        {
            ++m_axis_score;
        }
    }

    sf::Packet packet;
    packet << static_cast<uint8_t>(Server::PacketType::kTankDestroyed);
    packet << victim << killer;
    SendToAll(packet);

    BroadcastScores();
}

void GameServer::ProcessRespawns()
{
    for (auto& entry : m_tank_info)
    {
        if (entry.second.m_alive || Now() < entry.second.m_respawn_at)
        {
            continue;
        }

        entry.second.m_alive = true;
        entry.second.m_snapshot.SetPosition(GetSpawnPosition(entry.first));
        entry.second.m_snapshot.SetHullRotation(entry.second.m_team == TeamID::kAllies ? 0.f : 180.f);
        entry.second.m_snapshot.SetTurretRotation(0.f);
        entry.second.m_snapshot.m_hitpoints = NetCompression::PackHitpoints(kTankHitPoints);
        entry.second.m_last_broadcast = entry.second.m_snapshot;

        sf::Packet packet;
        packet << static_cast<uint8_t>(Server::PacketType::kTankRespawn);
        packet << entry.second.m_snapshot;
        SendToAll(packet);
    }
}

void GameServer::BroadcastChangedTankStates()
{
    // Delta filtering: a tank that has not moved, turned, aimed or taken
    // damage since the previous tick is left out of the packet entirely.
    // A lobby where half the players are sitting still costs half as much.
    std::vector<const TankInfo*> changed;
    changed.reserve(m_tank_info.size());

    for (auto& entry : m_tank_info)
    {
        if (!entry.second.m_ever_broadcast || entry.second.m_snapshot != entry.second.m_last_broadcast)
        {
            entry.second.m_last_broadcast = entry.second.m_snapshot;
            entry.second.m_ever_broadcast = true;
            changed.push_back(&entry.second);
        }
    }

    if (changed.empty())
    {
        return;
    }

    sf::Packet packet;
    packet << static_cast<uint8_t>(Server::PacketType::kUpdateClientState);
    packet << static_cast<uint8_t>(changed.size());
    for (const TankInfo* info : changed)
    {
        packet << info->m_snapshot;
    }

    SendToAll(packet);
}

void GameServer::BroadcastScores()
{
    sf::Packet packet;
    packet << static_cast<uint8_t>(Server::PacketType::kScoreUpdate);
    packet << m_allies_score << m_axis_score << GetSecondsRemaining();
    SendToAll(packet);
}

void GameServer::EndMatch(TeamID winning_team)
{
    if (m_match_over)
    {
        return;
    }
    m_match_over = true;

    // Persistence: the best individual kill count of this match is offered to
    // the on-disk table, which is then rewritten so the record outlives the
    // process.
    for (const auto& entry : m_tank_info)
    {
        m_high_scores.Submit(PlayerName(entry.first), entry.second.m_score);
    }
    m_high_scores.Save();

    sf::Packet packet;
    packet << static_cast<uint8_t>(Server::PacketType::kMissionEnd);
    packet << static_cast<uint8_t>(winning_team);
    packet << m_allies_score << m_axis_score;
    SendToAll(packet);

    std::cout << "Server: match over. Allies " << m_allies_score << " - Axis " << m_axis_score << std::endl;
}

void GameServer::HandleDisconnections()
{
    for (auto itr = m_peers.begin(); itr != m_peers.end(); )
    {
        if (!(*itr)->m_timed_out)
        {
            ++itr;
            continue;
        }

        if ((*itr)->m_tank_identifier != 0)
        {
            sf::Packet packet;
            packet << static_cast<uint8_t>(Server::PacketType::kPlayerDisconnect) << (*itr)->m_tank_identifier;
            SendToAll(packet);
            m_tank_info.erase((*itr)->m_tank_identifier);
        }

        --m_connected_players;
        itr = m_peers.erase(itr);

        // Connected peers always occupy [0, m_connected_players) with exactly
        // one spare slot behind them for the listener to accept into. Topping
        // up to that invariant, rather than pushing a slot per disconnect,
        // stops the vector growing over a long match.
        if (m_connected_players < m_max_connected_players)
        {
            while (m_peers.size() < m_connected_players + 1)
            {
                m_peers.emplace_back(PeerPtr(new RemotePeer()));
            }
            SetListening(true);
        }

        BroadcastMessage("A player has disconnected");
    }
}

void GameServer::InformWorldState(RemotePeer& peer)
{
    sf::Packet packet;
    packet << static_cast<uint8_t>(Server::PacketType::kInitialState);
    packet << GetSecondsRemaining() << m_allies_score << m_axis_score;
    packet << static_cast<uint8_t>(m_tank_info.size());

    for (const auto& entry : m_tank_info)
    {
        packet << entry.second.m_snapshot;
    }

    QueuePacket(peer, packet);
}

void GameServer::BroadcastMessage(const std::string& message)
{
    sf::Packet packet;
    packet << static_cast<uint8_t>(Server::PacketType::kBroadcastMessage);
    packet << message;
    SendToAll(packet);
}

void GameServer::SendToAll(sf::Packet& packet)
{
    for (PeerPtr& peer : m_peers)
    {
        if (peer->m_ready)
        {
            QueuePacket(*peer, packet);
        }
    }
}

void GameServer::QueuePacket(RemotePeer& peer, const sf::Packet& packet)
{
    // A client that has stopped draining its socket must not be allowed to
    // grow this queue without bound and take the server's memory with it.
    // Dropping the oldest state updates is the right thing to lose: the next
    // snapshot supersedes them anyway.
    constexpr std::size_t kMaxQueuedPackets = 128;
    if (peer.m_send_queue.size() >= kMaxQueuedPackets)
    {
        peer.m_send_queue.pop_front();
    }

    peer.m_send_queue.push_back(packet);
}

void GameServer::FlushSendQueues()
{
    for (PeerPtr& peer : m_peers)
    {
        while (peer->m_ready && !peer->m_send_queue.empty())
        {
            const sf::Socket::Status status = peer->m_socket.send(peer->m_send_queue.front());

            if (status == sf::Socket::Status::Done)
            {
                peer->m_send_queue.pop_front();
                continue;
            }

            if (status == sf::Socket::Status::Partial || status == sf::Socket::Status::NotReady)
            {
                // Leave the packet where it is and come back next loop; the
                // packet object itself remembers how much has gone out.
                break;
            }

            peer->m_timed_out = true;
            break;
        }
    }
}

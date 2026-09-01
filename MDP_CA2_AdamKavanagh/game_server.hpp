// Adam Kavanagh - D00247069
#pragma once
#include "net_compression.hpp"
#include "tank_type.hpp"
#include "high_score.hpp"
#include <SFML/Network/TcpListener.hpp>
#include <SFML/Network/Packet.hpp>
#include <SFML/Network/TcpSocket.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/System/Vector2.hpp>
#include <atomic>
#include <deque>
#include <cstdint>
#include <map>
#include <memory>
#include <thread>
#include <vector>

// The authoritative match host. It runs on its own thread inside the hosting
// client's process, owns the scoreboard, the match clock and the respawn
// queue, and relays player input and compressed tank snapshots between every
// connected client over TCP.
//
// It deliberately does NOT simulate the world: tank movement is run on the
// clients, which report their own tank's state 20 times a second. That keeps
// the server cheap enough to sit alongside a running game client, at the cost
// of trusting clients about their own position - see DOCUMENTATION.md.
class GameServer
{
public:
    GameServer();
    ~GameServer();

private:
    struct RemotePeer
    {
        RemotePeer();

        sf::TcpSocket           m_socket;
        sf::Time                m_last_packet_time;
        std::vector<uint8_t>    m_tank_identifiers;
        bool                    m_ready;
        bool                    m_timed_out;

        // Outgoing packets that TCP has not accepted yet. A non-blocking
        // socket can report Partial when the kernel send buffer fills up -
        // with 15 clients being broadcast to 20 times a second that is a
        // question of when, not if - and SFML requires the very same packet
        // object to be handed back to send() until it completes.
        std::deque<sf::Packet>  m_send_queue;
    };

    struct TankInfo
    {
        TankSnapshot    m_snapshot;         // last state reported by its owner
        TankSnapshot    m_last_broadcast;   // what the clients were last told
        TeamID          m_team = TeamID::kNone;
        uint16_t        m_score = 0;        // kills
        bool            m_alive = true;
        sf::Time        m_respawn_at = sf::Time::Zero;
        bool            m_ever_broadcast = false;
    };

    typedef std::unique_ptr<RemotePeer> PeerPtr;

private:
    void SetListening(bool enable);
    void ExecutionThread();
    void Tick();
    sf::Time Now() const;

    void HandleIncomingConnections();
    void HandleIncomingPackets();
    void HandleIncomingPacket(sf::Packet& packet, RemotePeer& receiving_peer, bool& detected_timeout);
    void HandleDisconnections();

    void InformWorldState(RemotePeer& peer);
    void BroadcastMessage(const std::string& message);
    void SendToAll(sf::Packet& packet);
    void QueuePacket(RemotePeer& peer, const sf::Packet& packet);
    void FlushSendQueues();

    void BroadcastChangedTankStates();
    void BroadcastScores();
    void ProcessRespawns();
    void RegisterKill(uint8_t victim, uint8_t killer);
    void EndMatch(TeamID winning_team);

    sf::Vector2f GetSpawnPosition(uint8_t identifier) const;
    uint16_t GetSecondsRemaining() const;

private:
    // Started at the very end of the constructor, never in the initialiser
    // list: the thread touches the listener and the peer list immediately, so
    // it must not be able to run before those are constructed.
    std::thread         m_thread;
    std::atomic<bool>   m_waiting_thread_end;

    sf::Clock           m_clock;
    sf::TcpListener     m_listener_socket;
    bool                m_listening_state;
    sf::Time            m_client_timeout;

    std::size_t         m_max_connected_players;
    std::size_t         m_connected_players;

    std::vector<PeerPtr>            m_peers;
    std::map<uint8_t, TankInfo>     m_tank_info;
    uint8_t                         m_tank_identifier_counter;

    uint16_t            m_allies_score;
    uint16_t            m_axis_score;
    sf::Time            m_match_start_time;
    bool                m_match_over;

    sf::Time            m_last_score_broadcast;

    HighScoreTable      m_high_scores;
};

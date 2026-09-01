// Adam Kavanagh - D00247069
#pragma once
#include "state.hpp"
#include "world.hpp"
#include "player.hpp"
#include "packet_sender.hpp"
#include "hud_panel.hpp"
#include "game_server.hpp"
#include "network_protocol.hpp"
#include <SFML/Graphics/Text.hpp>
#include <SFML/Network/Packet.hpp>
#include <SFML/Network/TcpSocket.hpp>
#include <deque>
#include <map>
#include <memory>
#include <vector>

// The networked game. Hosting also spins up a GameServer in this process; the
// host then connects to it over the loopback interface exactly like any other
// client, so there is only ever one code path for actually playing.
class MultiplayerGameState : public State, public PacketSender
{
public:
    MultiplayerGameState(StateStack& stack, Context context, bool is_host);

    virtual void Draw() override;
    virtual bool Update(sf::Time dt) override;
    virtual bool HandleEvent(const sf::Event& event) override;
    virtual void OnActivate() override;
    virtual void OnDestroy() override;

    void DisableAllRealtimeActions();

private:
    void HandlePacket(uint8_t packet_type, sf::Packet& packet);
    void UpdateBroadcastMessage(sf::Time elapsed_time);
    void PushBroadcast(const std::string& message);
    void ShowFrontBroadcast();
    void UpdateScoreboardText();
    void ApplySnapshot(const TankSnapshot& snapshot, bool is_spawn);
    void SendOwnState();
    void SendPendingGameActions();

    // All outgoing traffic goes through here. On a non-blocking TCP socket a
    // send can come back Partial, and SFML needs the same packet handed back
    // until it completes, so unsent packets wait in a queue instead of being
    // silently dropped.
    virtual void SendPacket(const sf::Packet& packet) override;
    void FlushSendQueue();

private:
    typedef std::unique_ptr<Player> PlayerPtr;

private:
    World                   m_world;
    sf::RenderWindow&       m_window;

    std::map<uint8_t, PlayerPtr>    m_players;
    uint8_t                 m_local_player_identifier;
    bool                    m_has_local_tank;

    sf::TcpSocket           m_socket;
    std::deque<sf::Packet>  m_send_queue;
    bool                    m_connected;
    std::unique_ptr<GameServer> m_game_server;
    sf::Clock               m_tick_clock;

    std::vector<std::string> m_broadcasts;
    sf::Text                m_broadcast_text;
    HudPanel                m_broadcast_panel;
    sf::Time                m_broadcast_elapsed_time;

    sf::Text                m_scoreboard_text;
    HudPanel                m_scoreboard_panel;
    sf::Text                m_failed_connection_text;
    sf::Clock               m_failed_connection_clock;

    uint16_t                m_allies_score;
    uint16_t                m_axis_score;
    uint16_t                m_seconds_remaining;

    bool                    m_active_state;
    bool                    m_has_focus;
    bool                    m_host;
    bool                    m_game_started;
    sf::Time                m_client_timeout;
    sf::Time                m_time_since_last_packet;
};

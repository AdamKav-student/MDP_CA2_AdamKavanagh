// Adam Kavanagh - D00247069
#pragma once
#include "action.hpp"
#include "key_binding.hpp"
#include "command_queue.hpp"
#include "packet_sender.hpp"
#include <SFML/Window/Event.hpp>
#include <map>

// One Player object exists on every client for every tank in the match. The
// one holding a KeyBinding is this machine's own crew; the rest are proxies
// whose held keys are driven by kPlayerRealtimeChange packets relayed by the
// server, which is what lets remote tanks keep driving smoothly in between
// the 20 Hz position snapshots.
class Player
{
public:
    Player(PacketSender* sender, uint8_t identifier, const KeyBinding* binding);

    void HandleEvent(const sf::Event& event, CommandQueue& commands);
    void HandleRealtimeInput(CommandQueue& commands);
    void HandleRealtimeNetworkInput(CommandQueue& commands);

    void HandleNetworkEvent(Action action, CommandQueue& commands);
    void HandleNetworkRealtimeChange(Action action, bool action_enabled);

    void DisableAllRealtimeActions();
    bool IsLocal() const;

    uint8_t GetIdentifier() const;

private:
    void InitialiseActions();

private:
    const KeyBinding*           m_key_binding;
    std::map<Action, Command>   m_action_binding;
    std::map<Action, bool>      m_action_proxies;   // held-key state of a remote tank
    uint8_t                     m_identifier;
    PacketSender*               m_sender;
};

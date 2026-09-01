// Adam Kavanagh - D00247069
#pragma once
#include "net_compression.hpp"
#include <SFML/System/Vector2.hpp>
#include <cstdint>

// Dynamic port range (> 49151).
const unsigned short SERVER_PORT = 50000;

// ---------------------------------------------------------------------------
// Application protocol, carried over a single TCP connection per client using
// sf::Packet (which frames each message with a 4-byte length prefix, so the
// stream is turned back into discrete messages for us).
//
// Every packet begins with a single uint8_t drawn from one of the two enums
// below. Anything else in the packet is described in the comment beside the
// enumerator.
// ---------------------------------------------------------------------------
namespace Server
{
    enum class PacketType : uint8_t
    {
        // std::string. Shown briefly across the top of every client's screen.
        kBroadcastMessage,

        // uint16 seconds_remaining, uint16 allies_score, uint16 axis_score,
        // uint8 tank_count, then tank_count x TankSnapshot. Sent once, to a
        // newly connected client, so it can build the world as it stands.
        kInitialState,

        // TankSnapshot. "This tank is yours" - the receiving client creates a
        // Player bound to the local key bindings for it.
        kSpawnSelf,

        // TankSnapshot. Another client's tank joined.
        kPlayerConnect,

        // uint8 identifier. That client left; remove its tank.
        kPlayerDisconnect,

        // uint8 identifier, uint8 action. A one-shot action was triggered.
        kPlayerEvent,

        // uint8 identifier, uint8 action, bool enabled. A held key on a remote
        // tank went down or came up; the receiving client keeps applying that
        // action locally every frame until told otherwise. This is what makes
        // remote tanks move smoothly between the 20 Hz snapshots.
        kPlayerRealtimeChange,

        // uint8 tank_count, then tank_count x TankSnapshot. The regular 20 Hz
        // world update. Only tanks whose snapshot changed since the previous
        // tick are included.
        kUpdateClientState,

        // uint8 victim_identifier, uint8 killer_identifier. Authoritative
        // confirmation of a kill, used for the explosion and the scoreboard.
        kTankDestroyed,

        // TankSnapshot. A destroyed tank came back at a fresh spawn point.
        kTankRespawn,

        // uint16 allies_score, uint16 axis_score, uint16 seconds_remaining.
        kScoreUpdate,

        // uint8 winning_team (TeamID), uint16 allies_score, uint16 axis_score.
        // The match is over; clients show the result screen.
        kMissionEnd
    };
}

namespace Client
{
    enum class PacketType : uint8_t
    {
        // uint8 identifier, uint8 action.
        kPlayerEvent,

        // uint8 identifier, uint8 action, bool enabled.
        kPlayerRealtimeChange,

        // TankSnapshot for this client's own tank, 20 times a second.
        kStateUpdate,

        // uint8 action (GameActions::Type), uint8 subject, uint8 other,
        // int16 x, int16 y. Used to report a locally detected kill.
        kGameEvent,

        // No payload. Sent when the client leaves cleanly.
        kQuit
    };
}

namespace GameActions
{
    enum Type : uint8_t
    {
        // "My tank was destroyed by <other>". Only ever sent by the client
        // that owns the destroyed tank, so the server counts each kill once.
        kTankDestroyed
    };

    struct Action
    {
        Action() = default;
        Action(Type type, uint8_t subject, uint8_t other, sf::Vector2f position)
            : type(type), subject(subject), other(other), position(position)
        {
        }

        Type            type = kTankDestroyed;
        uint8_t         subject = 0;    // the tank the event happened to
        uint8_t         other = 0;      // the tank responsible, if any
        sf::Vector2f    position;
    };
}

// Adam Kavanagh - D00247069
#pragma once
#include "constants.hpp"
#include <SFML/Network/Packet.hpp>
#include <SFML/System/Vector2.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>

// ---------------------------------------------------------------------------
// Wire compression helpers.
//
// The naive way to describe a tank is six floats and an int (position, hull
// angle, turret angle, hitpoints, identifier) which sf::Packet would put on
// the wire as 25 bytes. None of those values actually need 32 bits:
//
//   * positions are bounded by the fixed arena (0..3168 x 0..2304) and a
//     sub-pixel position is meaningless on screen, so a rounded int16_t is
//     lossless as far as the player can tell    -> 2 bytes each
//   * angles are periodic, so 0..360 degrees maps onto a whole byte with
//     ~1.4 degrees of error, well below what is visible on a turret
//                                               -> 1 byte each
//   * hitpoints are 0..100                      -> 1 byte
//   * identifiers are 1..16                     -> 1 byte
//
// That gives the 8-byte TankSnapshot below, a little over three times smaller
// than the uncompressed form. See DOCUMENTATION.md for the resulting
// bandwidth figures.
// ---------------------------------------------------------------------------
namespace NetCompression
{
    inline int16_t PackCoordinate(float value)
    {
        // Clamped so a tank that has drifted outside the arena (which can
        // happen for a frame while a correction is being applied) can never
        // wrap around to the far side of the map when it is decoded.
        const float clamped = std::clamp(value, -32000.f, 32000.f);
        return static_cast<int16_t>(std::lround(clamped));
    }

    inline float UnpackCoordinate(int16_t value)
    {
        return static_cast<float>(value);
    }

    inline uint8_t PackAngle(float degrees)
    {
        float wrapped = std::fmod(degrees, 360.f);
        if (wrapped < 0.f)
        {
            wrapped += 360.f;
        }
        // 256 steps over the circle: quantisation error is at most 0.7 degrees.
        return static_cast<uint8_t>(std::lround(wrapped * 256.f / 360.f)) ;
    }

    inline float UnpackAngle(uint8_t packed)
    {
        return static_cast<float>(packed) * 360.f / 256.f;
    }

    inline uint8_t PackHitpoints(int hitpoints)
    {
        return static_cast<uint8_t>(std::clamp(hitpoints, 0, 255));
    }
}

// The single structure used for game state synchronisation, in both
// directions. Exactly 8 bytes on the wire.
struct TankSnapshot
{
    uint8_t m_identifier = 0;
    int16_t m_x = 0;
    int16_t m_y = 0;
    uint8_t m_hull_rotation = 0;    // packed 0..255 <-> 0..360 degrees
    uint8_t m_turret_rotation = 0;  // packed, relative to the hull
    uint8_t m_hitpoints = 0;

    static constexpr std::size_t kWireSize = 8;

    sf::Vector2f GetPosition() const
    {
        return sf::Vector2f(NetCompression::UnpackCoordinate(m_x), NetCompression::UnpackCoordinate(m_y));
    }

    void SetPosition(sf::Vector2f position)
    {
        m_x = NetCompression::PackCoordinate(position.x);
        m_y = NetCompression::PackCoordinate(position.y);
    }

    float GetHullRotation() const { return NetCompression::UnpackAngle(m_hull_rotation); }
    void SetHullRotation(float degrees) { m_hull_rotation = NetCompression::PackAngle(degrees); }

    float GetTurretRotation() const { return NetCompression::UnpackAngle(m_turret_rotation); }
    void SetTurretRotation(float degrees) { m_turret_rotation = NetCompression::PackAngle(degrees); }

    // Used by the server to skip tanks that have not moved since the previous
    // tick, so an idle 16-player lobby costs almost nothing.
    bool operator==(const TankSnapshot& other) const
    {
        return m_identifier == other.m_identifier
            && m_x == other.m_x
            && m_y == other.m_y
            && m_hull_rotation == other.m_hull_rotation
            && m_turret_rotation == other.m_turret_rotation
            && m_hitpoints == other.m_hitpoints;
    }

    bool operator!=(const TankSnapshot& other) const { return !(*this == other); }
};

inline sf::Packet& operator<<(sf::Packet& packet, const TankSnapshot& snapshot)
{
    return packet << snapshot.m_identifier
        << snapshot.m_x << snapshot.m_y
        << snapshot.m_hull_rotation << snapshot.m_turret_rotation
        << snapshot.m_hitpoints;
}

inline sf::Packet& operator>>(sf::Packet& packet, TankSnapshot& snapshot)
{
    return packet >> snapshot.m_identifier
        >> snapshot.m_x >> snapshot.m_y
        >> snapshot.m_hull_rotation >> snapshot.m_turret_rotation
        >> snapshot.m_hitpoints;
}

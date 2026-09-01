// Adam Kavanagh - D00247069
#pragma once
#include <SFML/Network/Packet.hpp>

// Everything that wants to put a packet on the wire goes through this instead
// of touching the socket. SFML requires a packet that came back Partial to be
// re-sent, unmodified, before anything else goes out on the same socket, so
// there can only be one place that owns that retry queue - having Player write
// to the socket directly would interleave with it and corrupt the stream.
class PacketSender
{
public:
	virtual ~PacketSender() = default;
	virtual void SendPacket(const sf::Packet& packet) = 0;
};

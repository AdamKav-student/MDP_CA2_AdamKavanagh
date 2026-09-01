# Armoured Warfare: 1944

Adam Kavanagh — D00247069
Multiplayer & Distributed Programming, CA2 — DkIT

A top-down 2D tank game in C++20 and SFML 3.0.1, with a client/server
multiplayer mode built on TCP sockets. Converted from a vertically-scrolling
aeroplane shooter.

* **Training** — offline tutorial map: destroy the lone Panzer, then the
  victory screen returns you to the menu after eight seconds.
* **Host / Join** — up to 16 players (the brief requires 15) over TCP on port
  50000. Allies vs Axis, first to twenty knock-outs or the higher score after
  fifteen minutes.

Controls: `W`/`S` drive, `A`/`D` turn the hull, `←`/`→` traverse the turret,
`Space` fires. All rebindable from *Controls and Guide*.

See **[DOCUMENTATION.md](DOCUMENTATION.md)** for the client and server
overview, the protocol, the serialisation and compression scheme, how game
state is synchronised, persistence, bandwidth estimates and player capacity,
and the known synchronisation issues.

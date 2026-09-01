# Armoured Warfare: 1944 — Technical Documentation

**Adam Kavanagh — D00247069**
BSc (Hons) Computing in Games Development, Stage 4
Multiplayer & Distributed Programming — CA2

---

## 1. What the game is

A top-down 2D tank game built on SFML 3.0.1 in C++20. It began life as a
vertically-scrolling aeroplane shooter; every part of the gameplay layer has
been rebuilt around armour.

The main menu offers three ways in:

| Menu entry | State | What it does |
|---|---|---|
| **Training** | `GameState` | Offline tutorial map: your Sherman and one stationary Panzer. |
| **Host** | `MultiplayerGameState(true)` | Starts a `GameServer` in-process and connects to it over loopback. |
| **Join** | `MultiplayerGameState(false)` | Connects to the address in `ip.txt`. |

### Training mission

`World` constructed with `networked = false` builds the tutorial scene: the
player's Sherman at the south of the map and a single stationary Panzer to the
north (`tutorial_config.hpp`). The Panzer has no `Player` attached, so it never
moves or fires — it is a gunnery target. When it is destroyed,
`World::IsTutorialComplete()` latches, `GameState` pushes `ResultState`, the
victory image (`Media/Menu/Victory Image.png`) is shown full-screen for eight
seconds with a visible countdown, and the stack is then cleared back to the
main menu. Being knocked out instead shows the defeat image on the same
eight-second timer.

### Multiplayer match

Up to 16 players (`kMaxPlayers`, the brief requires 15) fight over a
3168 × 2304 arena scattered with cover. Teams are decided by network
identifier: odd numbers crew Shermans for the Allies, even numbers crew
Panzers for the Axis. Shells only damage the opposing team. Knocked-out tanks
respawn at their team's end of the map after four seconds, so a match runs
continuously for its full fifteen minutes. First team to twenty knock-outs
wins early; otherwise the higher score at the final whistle takes it.

### Controls

`W`/`S` drive and reverse, `A`/`D` turn the hull, `←`/`→` traverse the turret
independently of the hull, `Space` fires. All seven are rebindable from
**Controls and Guide** on the menu. Splitting driving from aiming across two
hands is what makes the turret worth having, and is the main reason the
control scheme could not simply be inherited from the aeroplane version.

---

## 2. Overview of the client

The client is a state stack (`StateStack`) over a scene graph (`SceneNode`).

```
Application
 └── StateStack
      ├── TitleState / MenuState / SettingsState / PauseState / ResultState
      ├── GameState                (offline training)
      └── MultiplayerGameState     (networked)
           ├── World               ← scene graph, collision, camera, rendering
           ├── std::map<uint8_t, Player>   ← one per tank in the match
           └── sf::TcpSocket       ← one connection to the server
```

**`World`** owns the scene graph and everything visual. Its layers are
`kBackground` (map art), `kGround` (debris and particle systems) and
`kEntities` (tanks and shells). It runs collision resolution, keeps tanks
inside the arena, follows the local tank with the camera (clamped so the view
never leaves the map) and positions the audio listener. The same class serves
both game modes; the only difference is which scene it builds in its
constructor.

**`Tank`** is an `Entity` whose hull sprite drives, with a `TurretNode`
attached as a child scene node. Because the turret is a child, it inherits the
hull's transform for free — its own rotation is purely the aiming offset, and
the muzzle position and barrel direction fall out of the accumulated
scene-graph transform (`TurretNode::GetMuzzleWorldPosition`,
`GetBarrelDirection`). Firing is rate-limited inside the tank, so holding the
fire key cannot spawn a shell per frame.

**`Player`** is the input layer, and there is one instance **per tank in the
match, on every client**. Exactly one of them holds a `KeyBinding` — that is
this machine's crew. The rest are proxies: their held-key state is driven by
`kPlayerRealtimeChange` packets relayed from the server, and every frame they
push the same movement commands into the same command queue that the local
player uses. A remote tank is therefore genuinely being *simulated* locally,
not interpolated between waypoints.

Every action a `Player` pushes is wrapped in an identifier filter, so a
command emitted for tank 7 is ignored by tanks 1–6 even though it is broadcast
to the whole `kAnyTank` category.

**`PacketSender`** is a small interface implemented by
`MultiplayerGameState`. Nothing else in the client touches the socket. The
reason is explained in §7 — SFML requires a partially-sent packet to be
retried before anything else goes out, so there can only be one place that
owns the outgoing queue.

---

## 3. Overview of the server and the protocol

`GameServer` runs on its own `std::thread` inside the hosting player's
process. The host then connects to it over loopback exactly like anyone else,
so there is only one code path for actually playing the game.

The server is **authoritative over the match, not over the simulation**. It
owns:

* the roster of connected peers and the one tank each of them drives,
* the last reported state of every tank,
* team scores, individual scores and the fifteen-minute match clock,
* the respawn queue,
* the persistent high-score table.

It deliberately does **not** simulate movement or physics. Each client
simulates its own tank and reports it; the server relays. This keeps the
server cheap enough to sit alongside a running game client on the same lab
machine, at the cost of trusting clients about their own position (see §7).

### Packet types

Every message is an `sf::Packet` whose first byte is the packet type. SFML
prefixes each packet with a 4-byte length, which is what turns the TCP byte
stream back into discrete messages.

**Server → Client** (`Server::PacketType`)

| Type | Payload | Purpose |
|---|---|---|
| `kBroadcastMessage` | `string` | Join/leave/kill notices, high-score announcement. |
| `kInitialState` | `u16 secs, u16 allies, u16 axis, u8 count, count × TankSnapshot` | Sent once to a joining client so it can build the world as it stands. |
| `kSpawnSelf` | `TankSnapshot` | "This tank is yours." Receiver binds the keyboard to it. |
| `kPlayerConnect` | `TankSnapshot` | Somebody else joined. |
| `kPlayerDisconnect` | `u8 id` | Remove that tank. |
| `kPlayerEvent` | `u8 id, u8 action` | One-shot action relay. |
| `kPlayerRealtimeChange` | `u8 id, u8 action, bool` | A held key on a remote tank went down or up. |
| `kUpdateClientState` | `u8 count, count × TankSnapshot` | The 20 Hz world snapshot — **changed tanks only**. |
| `kTankDestroyed` | `u8 victim, u8 killer` | Authoritative kill confirmation. |
| `kTankRespawn` | `TankSnapshot` | A tank came back; applied verbatim by everyone. |
| `kScoreUpdate` | `u16 allies, u16 axis, u16 secs` | Scoreboard and clock, once a second. |
| `kMissionEnd` | `u8 team, u16 allies, u16 axis` | Match over. |

**Client → Server** (`Client::PacketType`)

| Type | Payload | Purpose |
|---|---|---|
| `kStateUpdate` | `TankSnapshot` | This client's own tank, 20 times a second. |
| `kPlayerEvent` | `u8 id, u8 action` | One-shot action. |
| `kPlayerRealtimeChange` | `u8 id, u8 action, bool` | Key down / key up transition. |
| `kGameEvent` | `u8 action, u8 subject, u8 other, i16 x, i16 y` | Locally-detected kill report. |
| `kQuit` | — | Clean disconnect. |

### Connection handshake

Ordering matters and is deliberate:

1. `accept()` succeeds; the server allocates the next identifier, derives the
   team and the spawn point from it, and records the tank.
2. `kInitialState` goes to the newcomer **first**, describing the world as it
   already is.
3. `kSpawnSelf` tells the newcomer which of those tanks is its own.
4. `kPlayerConnect` goes to *everyone else*.

Doing (2) before (4) is what stops a joining client receiving its own
`kPlayerConnect` and ending up with a duplicate tank.

---

## 4. Structures, serialisation and compression

Everything about a tank that has to be replicated fits in one structure,
`TankSnapshot` (`net_compression.hpp`), used in **both** directions:

```cpp
struct TankSnapshot
{
    uint8_t m_identifier;      // 1..16
    int16_t m_x, m_y;          // world pixels, rounded
    uint8_t m_hull_rotation;   // 0..255  <->  0..360 degrees
    uint8_t m_turret_rotation; // 0..255, relative to the hull
    uint8_t m_hitpoints;       // 0..100
};                             // 8 bytes on the wire
```

The compression is driven by what the values actually need rather than what
the C++ types default to:

| Field | Naive | Chosen | Why it is safe |
|---|---|---|---|
| identifier | `int32` (4 B) | `uint8` (1 B) | At most 16 players. |
| x, y | `float` (4 B each) | `int16` (2 B each) | The arena is fixed at 3168 × 2304, which fits `int16` exactly, and sub-pixel positions are invisible. |
| hull angle | `float` (4 B) | `uint8` (1 B) | 256 steps around the circle = 1.4° resolution, ≤ 0.7° error. |
| turret angle | `float` (4 B) | `uint8` (1 B) | As above; a 0.7° error on a barrel is roughly one pixel at the muzzle. |
| hitpoints | `int32` (4 B) | `uint8` (1 B) | Range is 0–100. |
| **Total** | **24 B** | **8 B** | **A third of the size.** |

Three further things keep the traffic down, and they matter more than the
field packing does:

1. **Delta filtering.** `GameServer::BroadcastChangedTankStates()` compares
   each tank's snapshot with what was last sent and omits any tank that has
   not moved, turned, aimed or taken damage. A player sitting still costs
   nothing at all. `TankSnapshot::operator==` exists purely for this.

2. **Shells are never replicated.** A shell is spawned locally on every client
   the moment the firing input arrives, from the firer's own turret transform,
   and is simulated identically everywhere. With a 1.5 s fire interval and 15
   players, replicating shells individually would have been the single largest
   traffic source in the game; instead it is free.

3. **The map is never sent.** Obstacle positions live in a fixed table
   (`debris_layout.cpp`) that every client builds from at startup, and team
   and hull type are pure functions of the identifier
   (`team_assignment.hpp`). None of it costs a byte.

4. **Input is sent as transitions, not as state.** A held key produces two
   4-byte messages — one down, one up — rather than 60 messages a second
   describing that it is still held.

---

## 5. Game state synchronisation

Synchronisation is split by *who is allowed to be right about what*.

**Position and orientation** — the owning client is authoritative. It
simulates its own tank straight from the keyboard (no waiting for a server
round trip, so there is no input lag on the tank you are driving) and reports
an 8-byte snapshot 20 times a second. The server relays it. On every other
machine that tank keeps being driven between snapshots by the relayed key
state, and `ApplySnapshot()` eases it 25% of the way towards the
authoritative position each tick rather than teleporting it. The relayed copy
of a client's *own* tank is discarded — snapping to it would fight the
player's own input and produce visible rubber-banding.

**Damage** — the *victim's* client is authoritative. Every client simulates
every shell, so several machines will see the same impact; only the machine
that owns the tank applies the damage (`World::IsAuthoritativeFor`). The
shell is destroyed everywhere so the visuals agree, but the hitpoints move
once. Everybody else picks the new hitpoints up from the next snapshot.

**Kills and score** — the server is authoritative. When a victim's own client
takes its tank to zero it emits a `kTankDestroyed` game event naming the
shooter. Because only the victim ever reports its own death, and the server
additionally checks that the reporting peer actually owns that tank, each kill
is counted exactly once regardless of how many clients saw the impact. The
server then confirms the kill to everyone, so the explosion plays in sync,
and schedules the respawn.

**Spawns and respawns** — the server is authoritative and clients apply them
verbatim, their own tank included. This is the one case where a client's
position is overridden.

**The clock and the scoreboard** — the server, broadcast once a second.

---

## 6. Game persistence

`HighScoreTable` (`high_score.hpp` / `.cpp`) keeps the five best individual
kill counts the server has ever seen in `high_scores.txt`, one
`<score> <name>` per line.

* The table is **loaded** when the `GameServer` is constructed.
* Every joining player is sent the current record as a broadcast message
  ("Record to beat: 14 kills by Player 3"), so persistence is visible in-game
  rather than only on disk.
* At the end of a match, every player's score is offered to the table and the
  file is rewritten, so records survive the server process closing.

The format is deliberately plain text so it can be inspected or reset without
a tool, and a missing file is treated as "no record yet" rather than an error.

---

## 7. TCP: how it shaped the implementation, and known issues

### What TCP gave

Ordering and reliability are load-bearing here. The whole
`kPlayerRealtimeChange` scheme — send a key-down, send a key-up, and have every
other machine keep driving that tank in between — only works because those two
messages cannot be lost or reordered. Over UDP a dropped key-up would leave a
tank driving forwards forever on every other screen, and the design would have
had to send full key state continuously instead. The same applies to
`kTankDestroyed` and `kPlayerDisconnect`: a lost one would strand a wreck or a
ghost tank in the world permanently.

`sf::Packet`'s length prefix also solves message framing for free, which a raw
TCP stream does not give you.

### What TCP cost

**Head-of-line blocking.** A lost segment stalls *everything* behind it,
including snapshots that have already been superseded. Under packet loss the
symptom is not a small position error, it is a freeze followed by a jump.
This is the main argument for UDP in this genre, and it is the reason the
client eases remote tanks towards their authoritative position instead of
snapping — the correction after a stall is spread over several frames.

**Partial sends.** The sockets are non-blocking, because a blocking send would
let one slow client stall the whole server. That means `send()` can return
`Partial` when the kernel send buffer fills — with 15 clients being written to
20 times a second, that is a question of when, not if. SFML requires the *same
unmodified packet* to be handed back to `send()` before anything else goes out
on that socket, so both ends carry an explicit outgoing queue
(`GameServer::FlushSendQueues`, `MultiplayerGameState::FlushSendQueue`) that
retries the front packet and only pops it on `Done`. This is also why `Player`
writes through the `PacketSender` interface instead of holding the socket: two
writers on one socket would interleave and corrupt the stream.

The queues are capped and drop their *oldest* entry when full, because the
thing being dropped is a superseded state update — the next one contains
strictly better information.

**Nagle's algorithm.** SFML does not set `TCP_NODELAY`, so small writes may be
coalesced and delayed up to ~40 ms. At a 20 Hz tick rate that is a meaningful
share of a frame budget, and it is a plausible first thing to change if input
felt sluggish across the lab network.

**Connection cost.** Each client is a full TCP connection with its own kernel
buffers, which is what puts the practical ceiling on player count long before
bandwidth does.

### Known synchronisation issues

1. **Shell divergence.** Shells are simulated independently on each client
   from the relayed turret transform. Because that transform arrives up to one
   tick late and is quantised to 0.7°, a shell's path differs slightly between
   machines. Over the ~1 s flight time of a long shot the difference can be
   several tank widths, so a client can occasionally see a shell miss that the
   victim's machine scored as a hit. The victim's machine wins, which is the
   fair way round, but it is visible.

2. **Client-authoritative position.** A modified client could place its tank
   anywhere. The server only enforces that a peer may move *its own* tanks; it
   does not sanity-check speed or teleports. Fixing it properly means moving
   movement simulation server-side, which was out of scope for the time
   available.

3. **Local damage under packet loss.** Hitpoints on remote tanks track the
   relayed value, so during a stall a remote tank can appear healthy for a
   moment after it was actually knocked out. It corrects on the next
   `kUpdateClientState`.

4. **Explosion timing.** The victim explodes locally the instant its own
   client applies the fatal damage, but on other machines only when
   `kTankDestroyed` arrives, so the explosion is one round trip late for
   spectators.

5. **Fire-rate drift.** Each client runs its own tank's cooldown. Because a
   remote tank's "fire held" state arrives late, its shell timing can be a
   frame or two out of step with the firer's own view.

6. **No lag compensation.** There is no rewinding of the world to the
   shooter's view at the time of the shot. On a LAN (sub-millisecond RTT) this
   is invisible; over the internet it would need addressing.

---

## 8. Bandwidth and player capacity

### Measured message sizes

Every SFML packet carries a 4-byte length prefix, so wire cost is
`4 + payload`.

| Message | Payload | On the wire |
|---|---|---|
| Client `kStateUpdate` | 1 + 8 = 9 B | **13 B** |
| Client/server realtime change | 1 + 1 + 1 + 1 = 4 B | **8 B** |
| Server `kUpdateClientState`, *n* changed tanks | 1 + 1 + 8n B | **6 + 8n B** |
| Server `kScoreUpdate` | 1 + 6 = 7 B | **11 B** |
| Server `kTankDestroyed` | 1 + 2 = 3 B | **7 B** |

### Per client

**Upstream (client → server)**

* State updates: 20 Hz × 13 B = **260 B/s**
* Input transitions, assume a busy player producing 6 key changes/s:
  6 × 8 = **48 B/s**
* ≈ **310 B/s ≈ 2.5 kbit/s**

**Downstream (server → client), 15 players, worst case all 15 moving**

* Snapshots: 20 Hz × (6 + 8 × 15) = 20 × 126 = **2 520 B/s**
* Relayed input: 15 players × 6 changes/s × 8 B = **720 B/s**
* Scores and events: ≈ **20 B/s**
* ≈ **3 260 B/s ≈ 26 kbit/s**

### Server aggregate, 15 players

| Direction | Application data | With 40 B/segment IP+TCP overhead |
|---|---|---|
| Server → clients | 15 × 3 260 ≈ **49 kB/s** (391 kbit/s) | ≈ 61 kB/s (489 kbit/s) |
| Clients → server | 15 × 310 ≈ **4.7 kB/s** (37 kbit/s) | ≈ 17 kB/s (134 kbit/s) |
| **Total** | **≈ 54 kB/s (431 kbit/s)** | **≈ 78 kB/s (623 kbit/s)** |

Over a full fifteen-minute match that is roughly **49 MB** of application data
through the server in total, or about **70 MB** once IP and TCP headers are
counted.

Delta filtering means this is a genuine worst case. In practice a good share
of tanks are stationary, reloading or dead at any moment; with ten of the
fifteen moving the aggregate falls to roughly **42 kB/s**.

### How many players can this support?

Aggregate downstream scales as *O(n²)* — each of *n* clients receives state
for *n* tanks:

    server downstream ≈ n × 20 × (6 + 8n) bytes/s = 120n + 160n² bytes/s

| Network | Usable throughput | Players before bandwidth binds |
|---|---|---|
| **DKIT lab, switched 1 Gbit/s Ethernet** | ~800 Mbit/s = 100 MB/s | **≈ 790** |
| **DKIT lab, 100 Mbit/s Ethernet** | ~80 Mbit/s = 10 MB/s | **≈ 250** |
| **Campus Wi-Fi, ~30 Mbit/s effective** | 3.75 MB/s | **≈ 152** |
| **Home broadband host, 10 Mbit/s upstream** | 1.25 MB/s | **≈ 88** |
| **Home broadband host, 2 Mbit/s upstream** | 250 kB/s | **≈ 39** |

On any wired DKIT network bandwidth is emphatically **not** the limit. The
real ceilings, in the order they would be hit:

1. **`kMaxPlayers = 16`**, a deliberate cap sized to the brief's 15-player
   requirement. Raising it is a one-line change.
2. **Server loop cost.** The server polls every socket every 10 ms and
   rebuilds one snapshot packet per tick, copying it into each peer's queue.
   That copy makes broadcasting *O(n)* allocations per tick; at a few hundred
   players it would want a shared buffer instead.
3. **Per-connection kernel buffers**, at roughly 64 kB each by default —
   about 16 MB at 250 players, which is fine, but it is real memory the
   hosting machine also needs for the game it is running.

So: the brief's 15 players for 15 minutes is comfortable with a very large
margin, the design would scale to the low hundreds on the DKIT wired network
before anything needed rewriting, and the first thing to break would be the
server's per-tick packet copying rather than the network.

---

## 9. Code structure

```
MDP_CA2_AdamKavanagh/
├── Application / StateStack / State ....... program shell and screen stack
├── title_state, menu_state, settings_state,
│   pause_state, result_state .............. UI screens
├── game_state ............................. offline training mission
├── multiplayer_gamestate .................. networked match (client side)
│
├── world .................................. scene graph, collision, camera
├── tank, turret_node, projectile, entity .. gameplay entities
├── debris_node, debris_layout ............. static map cover
├── obstacle_collision ..................... AABB push-out resolution
├── team_assignment, tutorial_config ....... shared deterministic rules
├── data_tables ............................ all tuning values in one place
│
├── player, key_binding, action ............ input, local and network-driven
├── packet_sender .......................... single owner of socket writes
│
├── game_server ............................ authoritative match host (thread)
├── network_protocol ....................... packet type definitions
├── net_compression ........................ TankSnapshot + quantisation
├── network_node ........................... gameplay → network event queue
├── high_score ............................. persistence
│
└── scene_node, command, command_queue,
    resource_holder, sound_player, ......... engine infrastructure
    music_player, particle_node, bloom_effect
```

Design points worth calling out:

* **Data tables, not magic numbers.** Every tank, shell, obstacle and particle
  value lives in `data_tables.cpp`. Rebalancing does not mean touching game
  logic.
* **Shared deterministic rules.** `team_assignment.hpp`,
  `debris_layout.cpp` and `constants.hpp` are compiled into both the client
  and the server, which is what lets so much state be *derived* rather than
  transmitted. `kWorldWidth`/`kWorldHeight` in particular are compile-time
  constants specifically so that quantised positions decode identically on
  every machine — deriving the arena size from the window size, as the
  original scrolling game did, would have broken synchronisation between
  machines with different resolutions.
* **One input path.** Local and remote tanks are driven by the same `Player`
  class pushing the same commands into the same queue. There is no separate
  "remote tank" code path to drift out of step.
* **Single socket writer.** Enforced by `PacketSender`, for the partial-send
  reason in §7.

---

## 10. Building and running

* Visual Studio 2022, x64, C++20 (Debug and Release are both configured
  against the bundled `SFML-3.0.1`).
* The SFML DLLs sit beside the project and are copied to the output directory.
* **Host:** run the game, choose *Host*. The server listens on TCP port
  **50000**.
* **Join:** put the host's IP address in `ip.txt` beside the executable, run
  the game, choose *Join*. If `ip.txt` is missing it is created pointing at
  `127.0.0.1`.
* `high_scores.txt` is created beside the executable by the host after the
  first completed match.

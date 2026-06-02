# Epic 11 — Scripting Hot Reload & Networking

**Goal:** Deliver the two major runtime features that separate a complete engine from a rendering framework: live script hot-reload (so developers iterate without stopping the editor) and a foundational networking layer (authoritative server / client architecture suitable for real-time and turn-based games).

**Depends on:** 06-scripting, 10-3d-features  
**Blocks:** —  (this is the last planned epic; future work branches from here)

---

## Scope

### Scripting Hot Reload

#### Architecture: Shared Library Swap
Game scripts are compiled into a separate shared library (`game_scripts.dll` / `game_scripts.so`). At runtime the engine holds the library handle and function pointers. When sources change:

1. `FileWatcher` detects `.h` / `.cpp` change in the scripts directory
2. Engine posts `ScriptRecompileRequestedEvent`
3. A background build process (`cmake --build` targeting `game_scripts` only) is launched
4. On build success, the engine:
   a. Pauses the Update loop
   b. Serializes all script component state (via `EMBER_FIELD` reflection)
   c. Calls `onDestroy()` on all live script instances
   d. Unloads the old library (`dlclose` / `FreeLibrary`)
   e. Loads the new library (`dlopen` / `LoadLibrary`)
   f. Re-creates script instances via new factory functions
   g. Deserializes saved state into new instances
   h. Calls `onStart()` on all instances
   i. Resumes Update loop
5. Total pause target: <500ms on a modern developer machine for a small project

#### Requirements on Script Authors
- All serialized state must be in `EMBER_FIELD` members (pointer members, caches, etc. are re-initialized in `onStart`)
- Scripts must not cache raw `Entity` values across hot-reload (Entity IDs are stable, but pointer-based caches are not)
- Scripts must not hold references to heap memory allocated by the old library (i.e., no `new` in script code that isn't freed before `onDestroy`)

#### Implementation Details
- `ScriptLibrary` class: `load(path)`, `unload()`, `getFactory(typeName)` → `ScriptComponent*()`
- `EMBER_REGISTER_SCRIPT` macro updated to export a `createXxx()` C-linkage factory function
- Hot reload disabled in Release/Shipping builds (`EMBER_HOT_RELOAD` flag)
- Build invocation uses `std::system` or `boost::process` (prefer the latter to avoid shell injection); build stdout/stderr captured and shown in editor console panel
- Editor console panel added to display script build output

#### Editor Support
- Status bar shows: "Scripts: Up to date" / "Scripts: Compiling..." / "Scripts: Error (see console)"
- `Console Panel` — new editor panel showing engine log + script compile output, filterable by level
- Manual "Recompile Scripts" button in editor toolbar (for when auto-reload is too aggressive)

---

### Networking (`engine/networking/`)

#### Design: Client-Server with ECS Replication
- **Transport:** [GameNetworkingSockets](https://github.com/ValveSoftware/GameNetworkingSockets) (GNS) — chosen for: reliable/unreliable channels, encryption, NAT traversal, MIT-compatible license
- **Architecture:** Authoritative server; clients send input, server simulates, server replicates state
- **Protocol:** Custom binary protocol over GNS unreliable/reliable channels; no off-the-shelf RPC in this epic

#### Core Components

`NetworkManager` — singleton-style service (injected via `Application`)
- `startServer(port, maxClients)` / `stopServer()`
- `connectToServer(address, port)` / `disconnect()`
- `isServer()`, `isClient()`, `isOffline()`
- `sendReliable(clientId, data)`, `sendUnreliable(clientId, data)` (server-side)
- `sendToServer(data)` (client-side)
- `onMessageReceived` callback / event

`NetworkEntity` component:
```cpp
struct NetworkEntity {
    u32  networkId;       // stable ID assigned by server
    bool isOwner;         // true on the owning client
    bool isReplicated;    // false = local-only entity
};
```

`ReplicatedComponent` concept — components tagged for replication:
```cpp
EMBER_REPLICATE(Transform)          // server → all clients, unreliable
EMBER_REPLICATE_RELIABLE(Health)    // server → all clients, reliable
EMBER_REPLICATE_OWNER(InputState)   // owner client → server, unreliable
```

#### Replication System (`NetworkReplicationSystem`)
- Server side:
  - Each frame, diffs replicated component state against last-sent snapshot per client
  - Serializes changed fields into a compact binary packet
  - Sends unreliable for Transform/physics state, reliable for important state changes (health, events)
  - New entity spawned: send `SpawnEntity` reliable message to all clients
  - Entity destroyed: send `DestroyEntity` reliable message
- Client side:
  - Receives state packets; applies to ECS components (interpolated for Transform, immediate for others)
  - Dead reckoning: extrapolate Transform between received packets using last known velocity
  - Prediction: owner-client predicts their own `Transform` from input; server reconciles

#### Message Types
```
SpawnEntity     — networkId, entityType, initial component data
DestroyEntity   — networkId
ComponentUpdate — networkId, componentBitmask, delta-encoded field data
InputState      — sequence number, key bitmask, mouse delta, analog axes
GameEvent       — type, payload (reliable, ordered)
```

#### Lobby & Connection
- Simple lobby: server advertises on LAN via UDP broadcast; `NetworkManager::discoverServers()` scans
- Direct connect by IP:port also supported
- Max clients: 64 (GNS soft limit; configurable)
- Connection lifecycle events: `ClientConnectedEvent`, `ClientDisconnectedEvent`, `ServerConnectedEvent`, `ServerDisconnectedEvent`

#### Editor Support
- `NetworkPanel` — shows: server/client status, connected clients list, bandwidth in/out, packet loss %
- "Start Server" / "Connect" buttons for quick local testing
- Play mode: can launch as server, client, or offline (dropdown in play toolbar)

#### Sample: Networked Pong
Implement a minimal networked Pong game in the sandbox to validate the system:
- Server simulates ball + paddle physics
- Two clients control paddles via `InputState` messages
- Score replicated via reliable `ComponentUpdate`
- Connects over localhost

---

## Exit Criteria

### Hot Reload
- [ ] Modify `PlayerController::speed` in source → save → scripts recompile and reload in <500ms without crashing
- [ ] Script state (speed, jumpForce values set in editor) survives hot reload
- [ ] Hot reload during Play mode works (scripts reloaded mid-play)
- [ ] Build error in a script shows in Console panel; engine continues running with old version

### Networking
- [ ] Two processes on localhost: server starts, client connects, `ClientConnectedEvent` fires on server
- [ ] Server spawns an entity; client receives `SpawnEntity` message and entity appears in client's world
- [ ] Transform replication: server moves entity; client sees it move with <50ms latency on localhost
- [ ] Networked Pong sample: two clients play a complete round without desync
- [ ] `NetworkPanel` shows accurate bandwidth figures

---

## Key Files Created

```
engine/scripting/ScriptLibrary.h/.cpp          (hot reload DLL management)
editor/panels/ConsolePanel.h/.cpp
engine/networking/include/ember/networking/NetworkManager.h
engine/networking/include/ember/networking/NetworkEntity.h
engine/networking/include/ember/networking/Replication.h
engine/networking/NetworkReplicationSystem.h/.cpp
engine/networking/LobbyDiscovery.h/.cpp
editor/panels/NetworkPanel.h/.cpp
sandbox/NetworkedPong/PongServer.cpp
sandbox/NetworkedPong/PongClient.cpp
third_party/GameNetworkingSockets/
tests/networking/TestReplicationRoundtrip.cpp
tests/scripting/TestHotReload.cpp
```

---

## Notes & Decisions

- Hot reload is developer-only (`EMBER_HOT_RELOAD`); shipped games always statically link scripts
- The shared library boundary requires C-linkage factory functions — RTTI and exceptions do not cross DLL boundaries safely on all platforms; scripts must not throw exceptions that cross the library boundary
- The networking layer intentionally does not implement rollback netcode (GGPO-style) in this epic — the dead reckoning + server reconciliation model is sufficient for the sample games and straightforward to replace; rollback is a separate research epic
- GNS depends on OpenSSL for encryption; this adds to the build complexity on Windows — the `cmake/Dependencies.cmake` will need a FindOpenSSL path
- Entity interpolation uses a 3-snapshot ring buffer per `NetworkEntity`; interpolation delay is 2 frames (configurable) to smooth jitter

---

## Future Epics (Beyond This Roadmap)

The following areas are acknowledged but not planned in detail:

- **Epic 12 — Audio System:** Full 3D spatial audio via miniaudio; audio bus/mixer; AudioSource + AudioListener components
- **Epic 13 — Advanced Animation:** Blend trees, animation state machines, IK solvers, root motion
- **Epic 14 — Advanced Rendering:** Deferred rendering pipeline, SSAO, SSR, volumetric fog, GPU particles
- **Epic 15 — Rollback Networking:** GGPO-style input delay + rollback for fighting/racing games
- **Epic 16 — Console / Mobile:** Platform abstraction for console SDKs; touch input; mobile GPU optimization
- **Epic 17 — Scripting v2:** Lua or Python scripting as an alternative to C++ for modding support

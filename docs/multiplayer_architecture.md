# Multiplayer Architecture & Community Server Audit

This document presents the complete audit of the community server located in `tools/disney-infinity-community-server`, followed by a feasibility study and technical roadmap for cleanly restoring **Disney Infinity 3.0 Gold Edition** multiplayer via **CrabeLoader**.

---

## 1. Background & Problem Statement

When Disney shut down its online services in 2016–2017, Disney released the **Gold Edition** versions on Steam. For that release:
- Official servers were cut.
- Multiplayer menus were hidden and locked in the game's UI.
- The underlying Avalanche Software network engine remained compiled into the `DisneyInfinity3.exe` executable.

The `tools/disney-infinity-community-server` folder contains a community attempt (the `starbrightlab` repository) to recreate this backend.

---

## 2. Audit & Review of `disney-infinity-community-server`

### 2.1. Architecture & Stack
* **Technologies:** Node.js 18+, Express, Socket.IO, PostgreSQL / Supabase, Winston, Helmet, Multer.
* **Dual interface:**
  1. A modern web API (`/api/v1/*`) intended for an application or website.
  2. An emulation layer for the game's legacy endpoints:
     - `routes/config.js`: Discovery directory `/coregames/config/v1/infinity3/:platform/`.
     - `routes/infinity-api.js` & `routes/disney-ugc.js`: UGC and profiles.
     - `routes/did-compat.js`: Disney ID authentication (`/coregames/did/v3/*`).
     - `routes/sessions-compat.js`: Game session management (`/coregames/sessions/v1/*`).

---

### 2.2. What is valid and functional
- **Config Discovery:**
  The implementation in `routes/config.js` is accurate. At startup, the game client unconditionally queries `/coregames/config/v1/infinity3/steam/` (or `/pc/`, `/wiiu/`) to learn the URLs of all sub-services (`url_inf_ugc`, `url_cg_matchmaking`, `domain_cg_natneg`, etc.). Without a valid JSON response to this call, the game locks into offline mode.
- **Toybox Management (UGC):**
  The handling of Toybox binary files (multipart upload, metadata, screenshots) correctly reflects the format expected by the game.
- **Basic application security:**
  Per-IP/category rate-limiting, bcrypt password hashing, security headers via Helmet.

---

### 2.3. Major flaws and critical bugs

| Issue | Affected files | Impact |
| :--- | :--- | :--- |
| **Runtime DB Crash (`pool.query`)** | `controllers/profile.js`, `controllers/achievements.js`, `controllers/sync.js`, `controllers/analytics.js`, `services/achievementService.js` | During migration to the Supabase client, the `pool` export was removed from `config/database.js`. Calling `pool.query()` immediately throws `TypeError: Cannot read properties of undefined (reading 'query')`. |
| **Ghost data (SQL stubs)** | `config/database.js`, `socket.js`, `controllers/friends.js` | The compatibility `query()` function returns `{ rows: [], rowCount: 0 }` for any SQL query other than `SELECT NOW()`. Friend insertions (`INSERT INTO friends`) or request updates are never executed. Data is silently lost. |
| **Engine incompatibility (WebRTC / Socket.IO)** | `controllers/networking.js`, `socket.js` | The server implements WebRTC ICE candidate exchanges and Socket.IO events. **Disney Infinity 3.0 uses neither WebRTC nor Socket.IO**. The C++ game communicates via native UDP sockets (Avalanche/GameSpy NAT negotiation) and raw HTTP. |
| **Fictitious client configuration** | `client-integration/Infinity3Config.xml` | This XML file has never been read by the game. The engine hard-codes its base URLs in the binary; there is no native mechanism for loading external XML network config. |

**Server verdict:**
As-is, the server does not allow multiplayer gameplay. It can serve as a base for hosting Toyboxes (UGC), but its real-time architecture (Socket.IO / WebRTC) and database bugs block any real game session.

---

## 3. Analysis of the Network Engine in the PC Binary

The `DisneyInfinity3.exe` binary (Win32) still contains the full set of Avalanche Software native network game functions:

### 3.1. Status and authorization functions
* `IsMultiplayerAllowed`: Returns a boolean indicating whether multiplayer mode is allowed (disabled by default in the Gold Edition).
* `IsOnline`, `IsOnlineContentAllowed`: Network connection status check.
* `IsSignedIntoDisneyID`, `IsSignedIntoPlatform`: Authentication verification.
* `UI_IsSteamGame`, `UI_IsTOGOOfflineGame`: Build configuration flags.

### 3.2. Session and replication functions
* `Network_StartGettingList`, `Network_SetJoinSession`, `Network_SetSessionName`, `Network_RemoveFromGettingList`: Low-level network session management.
* `UI_LockGame`, `UI_UnlockGame`, `UI_GameIsLocked`: Host session locking/unlocking (see `src/api/22_system.lua`).
* `UI_KickPlayer`, `Players_NumPlayers`, `Players_MaxPlayers`: Management of connected peers in the instance.

---

## 4. Feasibility of a Multiplayer Mod via CrabeLoader

### 4.1. Why CrabeLoader is the ideal solution
Traditional community approaches require players to:
1. Modify their system `C:\Windows\System32\drivers\etc\hosts` file.
2. Install third-party SSL certificates on their OS to intercept HTTPS traffic.
3. Manually patch bytes in the `DisneyInfinity3.exe` binary (which breaks signatures and complicates updates).

**CrabeLoader solves all three problems** because it runs directly inside the game's memory space via the `bink2w32.dll` proxy with [MinHook](file:///home/crabe/Dev/Perso/CrabeLoader/src/minhook).

---

### 4.2. Technical roadmap for CrabeLoader

```
┌─────────────────────────────────────────────────────────────┐
│                       CrabeLoader                           │
│                                                             │
│  ┌───────────────────────┐       ┌───────────────────────┐  │
│  │   Lua Hooks / API     │       │   C++ Native Hooks    │  │
│  │  - UI Unlock          │       │  - Winsock/HTTP Hook  │  │
│  │  - Multiplayer Menu   │       │  - Auth/Flags Bypass  │  │
│  └───────────┬───────────┘       └───────────┬───────────┘  │
└──────────────┼───────────────────────────────┼──────────────┘
               │                               │
               ▼                               ▼
       Disney Infinity 3.0 Engine    Network Redirection
       - Replication logic           - Option A: Local server
       - Physics & Toybox objects    - Option B: Steamworks P2P
```

#### Step 1: In-Memory Network Rerouting (C++)
Instead of modifying Windows' `hosts` file:
- Add a hook in CrabeLoader on Win32 network functions (`connect`, `getaddrinfo`, or the game's HTTP API).
- When the game initiates a connection to `disney.go.com` or `api.disney.com`, CrabeLoader automatically redirects the socket to the local address (`127.0.0.1:3000`) or to a dedicated community server.

#### Step 2: Flag & UI Unlock (Lua + Memory)
- Hook the verification natives:
  - Force `IsMultiplayerAllowed()` to return `true`.
  - Force `IsOnline()` and `IsSignedIntoDisneyID()` to return `true`.
- Re-enable hidden menu entries in the UI (via `src/api/15_menu.lua` and the original menu scripts).

#### Step 3: Matchmaking & Transport (Two options)

##### Option A: Via Fixed Dedicated Server (Legacy Avalanche)
- Fix `tools/disney-infinity-community-server` to repair the database and session/matchmaking emulation.
- Set up a UDP NAT negotiation server compatible with the `domain_cg_natneg` field returned by the config.
- Clients exchange their IP/port addresses and the Avalanche engine establishes its native P2P connection.

##### Option B (Recommended): Direct Steamworks P2P Integration
- Disney Infinity 3.0 Gold already bundles `steam_api.dll`.
- By intercepting session calls (`Network_SetJoinSession`, etc.) and bridging them to Steam lobby APIs (`SteamMatchmaking()->CreateLobby`, `JoinLobby`, Steam friend invitations):
  - **No external server is needed** for direct multiplayer between friends.
  - NAT traversal is handled directly by Steam relays (Steam Datagram Relay).

---

## 5. Summary & Next Steps

1. **Do not use `disney-infinity-community-server` for real-time as-is:**
   The server requires a full overhaul of its SQL layer and network model (removing Socket.IO/WebRTC in favour of sockets adapted to the game).
2. **Implement the foundations in CrabeLoader:**
   - Create a C++ hook module to redirect game URLs in memory.
   - Force the return values of authentication and multiplayer permission natives.
   - Use Wireshark and loader logs to trace packets emitted when the game tries to create or join a session.

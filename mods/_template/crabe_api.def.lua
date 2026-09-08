---@meta
--- CrabeLoader API Type Definitions for VS Code (Lua Language Server / EmmyLua)
--- Place this file in your workspace or mod directory for full IntelliSense autocompletion.

---@class Crabe
---@field public version string The semver version of CrabeLoader (e.g. "0.2.0").
---@field public versionMajor number The major version integer.
---@field public versionMinor number The minor version integer.
---@field public versionPatch number The patch version integer.
Crabe = {}

--- Logs a message to the CrabeLoader console overlay.
---@param line any Message or object to convert to string and write to overlay.
function Crabe.write(line) end

--- Logs a formatted message to both the overlay and loader.log.
---@param message string The message string.
function Crabe.log(message) end

--- Flushes all buffered log lines from the internal queue.
---@return string joinedLines All buffered lines concatenated.
function Crabe.flush() end

--- Sets the display mode of the game window.
---@param mode "windowed"|"borderless"
---@return boolean success
function Crabe.SetWindowMode(mode) end

--- Gets the current display mode of the game window.
---@return "windowed"|"borderless"
function Crabe.GetWindowMode() end

--- Returns the host player ID (defaults to 0 if not yet set).
---@param playerId? number Optional player ID fallback.
---@return number hostPlayerId
function Crabe.hostPlayer(playerId) end

--- Splits a comma-separated string into a table of trimmed strings.
---@param csv string Comma-separated values.
---@return string[] items Table of trimmed strings.
function Crabe.splitList(csv) end

--------------------------------------------------------------------------------
-- Crabe.Events: Unified Event Bus
--------------------------------------------------------------------------------

---@class CrabeEvents
Crabe.Events = {}

--- Registers an event handler for the given event name.
--- Standard events: "init", "tick", "update", "playerDeath".
---@param eventName string Event identifier.
---@param handler fun(...: any): any Callback function.
---@return fun(...: any): any handler The registered handler.
function Crabe.Events.on(eventName, handler) end

--- Registers a one-shot event handler that unregisters itself after firing once.
---@param eventName string Event identifier.
---@param handler fun(...: any): any Callback function.
---@return fun(...: any): any wrapper The registered wrapper.
function Crabe.Events.once(eventName, handler) end

--- Unregisters a previously registered event handler.
---@param eventName string Event identifier.
---@param handler fun(...: any): any Handler function to remove.
---@return boolean success True if successfully removed.
function Crabe.Events.off(eventName, handler) end

--- Emits an event with optional arguments to all registered listeners.
---@param eventName string Event identifier.
---@param ... any Arguments forwarded to the handlers.
function Crabe.Events.emit(eventName, ...) end

--- Clears registered listeners.
---@param eventName? string If specified, clears only this event; otherwise clears all.
function Crabe.Events.clear(eventName) end

--------------------------------------------------------------------------------
-- Crabe.Menu: In-Game ImGui Mod Menu (F5)
--------------------------------------------------------------------------------

---@class CrabeMenuItem
---@field public label string Button or display label.
---@field public action fun(): any Callback function when clicked.

---@class CrabeMenu
Crabe.Menu = {}

--- Registers a new category tab in the F5 mod menu.
---@param categoryName string Category title.
function Crabe.Menu.registerCategory(categoryName) end

--- Adds an actionable item/button to a registered category.
---@param categoryName string Category title.
---@param item CrabeMenuItem Item definition.
function Crabe.Menu.addItem(categoryName, item) end

--- Adds a toggle switch to a category.
---@param categoryName string Category title.
---@param label string Toggle label.
---@param getFn fun(): boolean Getter returning current bool state.
---@param setFn fun(state: boolean): any Setter called on toggle.
function Crabe.Menu.addToggle(categoryName, label, getFn, setFn) end

--- Adds a numerical slider to a category.
---@param categoryName string Category title.
---@param label string Slider label.
---@param getFn fun(): number Getter returning current number.
---@param setFn fun(value: number): any Setter called on change.
---@param min number Minimum value.
---@param max number Maximum value.
---@param step? number Increment step (default: 1.0).
function Crabe.Menu.addSlider(categoryName, label, getFn, setFn, min, max, step) end

--- Sets the status message displayed at the bottom of the F5 menu.
---@param message string Status text.
function Crabe.Menu.setStatus(message) end

--------------------------------------------------------------------------------
-- Crabe.Multiplayer: Quazal Net-Z P2P Subsystem
--------------------------------------------------------------------------------

---@class NatInfo
---@field public available boolean True if NAT subsystem is active.
---@field public publicIp string Detected public WAN IP.
---@field public localIp string Detected local LAN IP.
---@field public portForwarded boolean True if UPnP port forward succeeded.
---@field public port number Port forwarded (typically 3074).
---@field public natType string NAT description ("Open", "Moderate", "Strict").

---@class LocationStringOpts
---@field public publicIp string Public WAN IPv4 address.
---@field public publicPort? number Public port (default: 3074).
---@field public privateIp? string Local LAN IPv4 address (default: publicIp).
---@field public privatePort? number Local port (default: publicPort).
---@field public hostDid? string Host Disney ID GUID.
---@field public gameName? string Internal game identifier (default: "IN2PC").

---@class ParsedLocation
---@field public ip string Resolved IPv4 address.
---@field public port number Resolved port.
---@field public hostDid string Host Disney ID GUID.
---@field public gameName string Game identifier.

---@class CrabeMultiplayer
Crabe.Multiplayer = {}

--- Checks if the multiplayer subsystem is active in the engine.
---@return boolean available
function Crabe.Multiplayer.isAvailable() end

--- Queries current NAT and port forwarding status.
---@return NatInfo info
function Crabe.Multiplayer.getNatInfo() end

--- Triggers an UPnP port forward on the router for P2P traffic.
---@param port? number Port to open (default: 3074).
---@param protocol? "UDP"|"TCP" Protocol (default: "UDP").
---@return boolean success
function Crabe.Multiplayer.triggerPortForward(port, protocol) end

--- Primes the HTTP redirector to inject a direct connect target into the friends list.
---@param friendName string Display name for the peer.
---@param ip string Target IPv4 address.
---@param port? number Target port (default: 3074).
---@param hostDid? string Optional host GUID.
---@return boolean success
function Crabe.Multiplayer.setDirectConnect(friendName, ip, port, hostDid) end

--- Encodes connection endpoints into a Net-Z JSON locationString.
---@param opts LocationStringOpts Endpoint details.
---@return string locationString
function Crabe.Multiplayer.buildLocationString(opts) end

--- Parses an engine locationString back into an IP and port table.
---@param locationStr string Raw locationString.
---@return ParsedLocation parsed
function Crabe.Multiplayer.parseLocationString(locationStr) end

--------------------------------------------------------------------------------
-- Crabe.Speedhack
--------------------------------------------------------------------------------

---@class CrabeSpeedhack
Crabe.Speedhack = {}

--- Sets the global game speed multiplier.
---@param speed number Speed multiplier (e.g. 0.5 for half speed, 2.0 for double).
function Crabe.Speedhack.setGameSpeed(speed) end

--- Gets the current game speed multiplier.
---@return number speed
function Crabe.Speedhack.getGameSpeed() end

--- Resets the game speed multiplier to normal (1.0).
function Crabe.Speedhack.resetGameSpeed() end

--------------------------------------------------------------------------------
-- Crabe.VirtualReader (Character Injection)
--------------------------------------------------------------------------------

---@class CharacterEntry
---@field public Name string Character identifier (e.g. "AVG_Thor", "TCW_MaceWindu").
---@field public baseCharacter? string Existing catalog name to inherit from.
---@field public sku_id? string Numeric SKU identifier.
---@field public Icon? string Path to character icon asset.

---@class CrabeVirtualReader
Crabe.VirtualReader = {}

--- Exposes a character shipped with the game assets but hidden from the catalog.
---@param entry CharacterEntry
---@return table row Catalog row
function Crabe.VirtualReader.exposeCharacter(entry) end

--- Reskins an existing character under a new catalog entry.
---@param entry CharacterEntry
---@return table row Catalog row
function Crabe.VirtualReader.addCharacter(entry) end

--- Lists all currently registered characters.
---@return table[] characters
function Crabe.VirtualReader.listCharacters() end

--------------------------------------------------------------------------------
-- Game Engine Hooks
--------------------------------------------------------------------------------

---@class Game
Game = {}

--- Registers a per-frame callback running at ~60 Hz.
---@param fn fun(dt: number): any
function Game.onTick(fn) end

--- Registers a death watcher callback firing on character death.
---@param playerId number Player ID to watch.
---@param fn fun(playerId: number): any Callback.
function Game.onDeath(playerId, fn) end

--- Queries whether a character is currently dead.
---@param playerId number
---@return boolean isDead
function Game.IsCharacterDead(playerId) end

--------------------------------------------------------------------------------
-- Common Avalanche / Disney Infinity Engine Natives
--------------------------------------------------------------------------------

--- Sets session mode (false = hosting, true = joining).
---@param isJoin boolean
function Network_SetJoinSession(isJoin) end

--- Sets the session identifier or join string for the network engine.
---@param sessionName string
function Network_SetSessionName(sessionName) end

--- Opens the in-game friends overlay.
---@param unused number
function UI_ViewFriends(unused) end

--- Opens the in-game multiplayer invitations overlay.
function UI_ViewInvitations() end

--- Sends in-game text chat across the active multiplayer session.
---@param text string
function TransmitChatText(text) end

--- Returns the primary host player ID.
---@return number playerId
function Players_GetHostPlayerID() end

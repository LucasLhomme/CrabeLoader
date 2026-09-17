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
-- Crabe.Mod: Mod Lifecycle Management
--------------------------------------------------------------------------------

---@class ModDef
---@field public id string Unique alphanumeric identifier for the mod.
---@field public name string Display name of the mod.
---@field public onInit? fun(): any Called once when the Lua VM and API are ready.
---@field public onUpdate? fun(dt: number): any Called every frame (~60 Hz) on the main script thread.
---@field public onDraw? fun(): any Called during DX11 Present to render Dear ImGui interfaces.
---@field public onShutdown? fun(): any Called when the mod is unloaded or hot-reloaded (F4).

---@class CrabeMod
Crabe.Mod = {}

--- Registers a mod with its lifecycle hooks.
---@param def ModDef Mod definition table.
function Crabe.Mod.register(def) end

--- Retrieves a registered mod by ID.
---@param id string Mod ID.
---@return ModDef? mod Mod definition or nil.
function Crabe.Mod.get(id) end

--- Lists all currently registered mods.
---@return ModDef[] mods Table of registered mod definitions.
function Crabe.Mod.list() end

--------------------------------------------------------------------------------
-- Crabe.Sandbox: Mod Isolation & Cross-Mod Exports
--------------------------------------------------------------------------------

---@class CrabeSandbox
Crabe.Sandbox = {}

--- Creates an isolated environment table for a mod with protected global tables.
---@param modName string Unique name or ID of the mod.
---@param opt? table Optional configuration options.
---@return table env Sandboxed environment table.
function Crabe.Sandbox.create(modName, opt) end

--- Exports a shared service or table to other sandboxed mods.
---@param name string Service identifier.
---@param value any Value, function, or table to expose.
function Crabe.Sandbox.export(name, value) end

--- Imports a shared service exposed by another mod.
---@param name string Service identifier.
---@return any value The exported value or nil.
function Crabe.Sandbox.import(name) end

--------------------------------------------------------------------------------
-- Crabe.Events: Unified Event Bus
--------------------------------------------------------------------------------

---@class CrabeEvents
Crabe.Events = {}

--- Registers an event handler for the given event name.
--- Standard events: "init", "tick", "update", "keyDown", "reload", "playerDeath".
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
-- Crabe.Memory: Native Memory & Code Cave Primitives
--------------------------------------------------------------------------------

---@class CrabeMemory
Crabe.Memory = {}

--- Scans process memory for an IDA-style AOB pattern (e.g. "55 8B EC ?? ?? 8B").
---@param pattern string IDA-style hex pattern with optional wildcards (??).
---@return number address Target address, or 0 if not found.
function Crabe.Memory.patternScan(pattern) end

--- Writes executable byte patches to memory with automatic memory protection.
---@param address number Virtual address in memory.
---@param bytes number[] Table of byte values (0-255).
---@return boolean success True if patch succeeded.
---@return string? error Error message if failed.
function Crabe.Memory.patchBytes(address, bytes) end

--- Reads a 32-bit floating point value from memory.
---@param address number Virtual address.
---@return number value Float value.
function Crabe.Memory.readFloat(address) end

--- Writes a 32-bit floating point value to memory.
---@param address number Virtual address.
---@param value number Float value to write.
---@return boolean success
function Crabe.Memory.writeFloat(address, value) end

--- Reads an unsigned 32-bit integer from memory.
---@param address number Virtual address.
---@return number value Integer value.
function Crabe.Memory.readU32(address) end

--- Writes an unsigned 32-bit integer to memory.
---@param address number Virtual address.
---@param value number Integer value to write.
---@return boolean success
function Crabe.Memory.writeU32(address, value) end

--- Installs an x86 code cave using HDE32 dynamic instruction boundary analysis.
---@param address number Hook site virtual address.
---@param shellcodeBytes number[] Assembly payload bytes.
---@param stolenLen? number Optional manual stolen byte count override.
---@return number trampolineAddress Address of allocated trampoline, or 0 on failure.
function Crabe.Memory.installCodeCave(address, shellcodeBytes, stolenLen) end

--------------------------------------------------------------------------------
-- Crabe.Hooks: Engine Chunk & Bytecode Interception
--------------------------------------------------------------------------------

---@class CrabeHooks
Crabe.Hooks = {}

--- Arms a Lua patch to run immediately after a chunk containing matchPattern is executed.
---@param matchPattern string Substring or identifier matching chunk source.
---@param luaCode string Lua code string to execute after chunk completion.
function Crabe.Hooks.patchChunk(matchPattern, luaCode) end

--- Arms a Lua patch to run immediately after a chunk with the exact name executes.
---@param chunkName string Full name of target chunk.
---@param luaCode string Lua code string to execute.
function Crabe.Hooks.patchNamedChunk(chunkName, luaCode) end

--- Completely replaces the source code of a chunk before bytecode compilation.
---@param matchPattern string Substring or identifier matching chunk source.
---@param luaCode string Replacement Lua source code.
function Crabe.Hooks.overrideChunk(matchPattern, luaCode) end

--- Installs an x86 code cave (facade over Crabe.Memory.installCodeCave).
---@param address number Hook site virtual address.
---@param shellcodeBytes number[] Assembly payload bytes.
---@param stolenLen? number Optional manual stolen byte count override.
---@return number trampolineAddress Address of allocated trampoline, or 0 on failure.
function Crabe.Hooks.installCodeCave(address, shellcodeBytes, stolenLen) end

--------------------------------------------------------------------------------
-- Crabe.Input: Keybinding Management
--------------------------------------------------------------------------------

---@class CrabeInput
Crabe.Input = {}

--- Binds a virtual key to a callback handler.
---@param vk number Virtual Key code (e.g. 0x74 for VK_F5).
---@param callback fun(): any Function called on key press.
function Crabe.Input.bindKey(vk, callback) end

--- Unbinds a previously registered virtual key callback.
---@param vk number Virtual Key code.
function Crabe.Input.unbindKey(vk) end

--------------------------------------------------------------------------------
-- Crabe.ImGui: Dear ImGui Rendering Primitives
--------------------------------------------------------------------------------

---@class CrabeImGui
Crabe.ImGui = {}

--- Begins an ImGui window. Must be called inside onDraw.
---@param title string Window title.
---@param open? boolean Window open state.
---@return boolean isVisible True if window is open and uncollapsed.
function Crabe.ImGui.begin(title, open) end

--- Ends the current ImGui window.
function Crabe.ImGui.endWindow() end

--- Renders text in the current window.
---@param text string Text to display.
function Crabe.ImGui.text(text) end

--- Renders a clickable button.
---@param label string Button label.
---@return boolean clicked True if clicked this frame.
function Crabe.ImGui.button(label) end

--- Renders a checkbox toggle.
---@param label string Checkbox label.
---@param checked boolean Current checked state.
---@return boolean changed True if clicked this frame.
---@return boolean newState New boolean value.
function Crabe.ImGui.checkbox(label, checked) end

--- Renders a floating point slider.
---@param label string Slider label.
---@param value number Current value.
---@param min number Minimum value.
---@param max number Maximum value.
---@return boolean changed True if dragged this frame.
---@return number newValue New float value.
function Crabe.ImGui.sliderFloat(label, value, min, max) end

--- Renders an integer slider.
---@param label string Slider label.
---@param value number Current value.
---@param min number Minimum value.
---@param max number Maximum value.
---@return boolean changed True if dragged this frame.
---@return number newValue New integer value.
function Crabe.ImGui.sliderInt(label, value, min, max) end

--- Draws a visual separator line.
function Crabe.ImGui.separator() end

--- Places the next widget on the same line as the previous widget.
function Crabe.ImGui.sameLine() end

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
-- Crabe.VirtualReader: Character & Figurine Roster Injection
--------------------------------------------------------------------------------

---@class CharacterEntry
---@field public Name string Character identifier (e.g. "AVG_Thor", "TCW_MaceWindu").
---@field public baseCharacter? string Existing catalog name to inherit from.
---@field public sku_id? string Numeric SKU identifier.
---@field public Icon? string Path to character icon asset.
---@field public ProgressionTree? string Stem of the progression tree file.

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
-- Game: Guarded Engine Facade
--------------------------------------------------------------------------------

---@class Game
Game = {}

--- Registers a per-frame callback running at ~60 Hz on the script thread.
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

--- Returns the local player entity pointer or index if available.
---@return number? player
function Game.GetLocalPlayer() end

--- Sets the health value of a player entity.
---@param player number
---@param health number
function Game.SetPlayerHealth(player, health) end

--- Changes the active player avatar to the specified SKU ID.
---@param skuId number
function Avatar_ChangeAvatar(skuId) end

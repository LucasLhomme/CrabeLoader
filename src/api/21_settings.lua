-- CrabeLoader
-- File description:
-- Lets a mod add its own option to one of the game's settings screens, from any Lua state.
-- Installs when the screen's chunk loads, because the screen table does not exist before that.
-- Renders nothing itself; the game's own BuildList owns the list this appends to.
--
-- Authors: @LucasLhomme

-- A settings screen is a global table the game creates while running
-- assets/presentation/<screen>.lua, and its BuildList method is what fills the
-- list the menu renders. Disassembling settingsvideo.lua (Lua 5.1 bytecode,
-- big-endian) shows the shape every screen follows:
--
--     local optionList = {}
--     ...
--     optionList[#optionList + 1] = someOption
--     self.listData = optionList          -- last statement before return
--
-- So an option is added by wrapping BuildList and appending to self.listData
-- once the original has returned.
--
-- The hard part is *when*. The screen table does not exist when mods load --
-- probed in the running game, SettingsVideo was nil in the front-end state
-- right after injection -- so a mod that reaches for it at load time finds
-- nothing. Polling from Game.onTick was the previous answer and it never
-- worked; this uses the loader's chunk-patch mechanism instead, which runs Lua
-- immediately after a chunk's own pcall returns. At that moment the screen
-- table has just been built and BuildList is the game's own.
--
-- The match hint is the screen's name, which appears in several chunks. That is
-- fine and deliberate: chunk-patch rules persist and re-arm on every matching
-- chunk, so the installer runs a few times and only succeeds on the one where
-- the screen table actually carries a BuildList.
--
-- The patch alone is not enough, and the reason is worth writing down because
-- it cost a session to find. A chunk patch runs after the chunk's own pcall
-- returns -- but settingsvideo.lua *returns* its table, and the caller assigns
-- it to the global afterwards. So at patch time the global is still nil by a
-- few instructions. Probed live: the patch fired twice and found nothing, then
-- the same _install call a minute later succeeded. It is a race, not a wrong
-- hook point, so the patch is backed by a bounded retry on the tick that stops
-- the moment it lands.

Crabe = Crabe or {}
Crabe.Settings = Crabe.Settings or {}

-- screen name -> ordered list of entries, and the owner that added each.
Crabe.Settings._pending = Crabe.Settings._pending or {}
Crabe.Settings._armed = Crabe.Settings._armed or {}
Crabe.Settings._retrying = Crabe.Settings._retrying or {}

local kValidScreen = "^[A-Za-z_][A-Za-z0-9_]*$"

-- The table the game's screen globals actually live in. Not _G: this module is
-- run with setfenv against the state's own globals, and inside a sandbox _G is
-- a different table -- reading the screen out of the wrong one is how this
-- silently finds nothing. Captured once, at load, which is when the module's
-- environment is the right one by construction.
local kGlobals = (getfenv and getfenv(1)) or _G

-- The list BuildList populated. One field, confirmed by disassembly rather
-- than guessed; a fallback to other names would hide a real failure.
local kListField = "listData"

-- Defined further down, next to the tick budget it spends; declared here
-- because _retry and addOption above it both reach for it, and a local
-- declared after its use site is captured as a nil global, not as itself.
local scheduleRetry

local function warn(line)
    if Crabe.write then
        Crabe.write("! [Crabe.Settings] " .. tostring(line))
    end
end

-- Appends every entry registered for this screen that is not already in the
-- list. Runs after the game's own BuildList, inside a pcall: a settings screen
-- the player still has to use must not be taken down by a mod's option.
local function appendEntries(screenName, self)
    local list = self[kListField]
    if type(list) ~= "table" then
        warn(screenName .. ": BuildList returned with self." .. kListField .. " = "
            .. type(list) .. "; no option was added")
        return
    end

    local entries = Crabe.Settings._pending[screenName]
    if not entries then return end

    for _, record in ipairs(entries) do
        local present = false
        for _, existing in ipairs(list) do
            if type(existing) == "table" and existing.id == record.entry.id then
                present = true
                break
            end
        end
        if not present then
            list[#list + 1] = record.entry
        end
    end
end

-- Wraps the screen's BuildList, once, however many times this is called.
--
-- The original is stashed on the screen table rather than held in a local: a
-- hot reload re-runs this while the screen table survives, and capturing the
-- already-wrapped function would stack a new layer on every reload.
function Crabe.Settings._install(screenName)
    local screen = rawget(kGlobals, screenName)
    if type(screen) ~= "table" or type(screen.BuildList) ~= "function" then
        return false
    end

    if screen.__crabeSettingsOriginal == nil then
        screen.__crabeSettingsOriginal = screen.BuildList
    end
    local original = screen.__crabeSettingsOriginal

    screen.BuildList = function(self, ...)
        local result = original(self, ...)
        local ok, err = pcall(appendEntries, screenName, self)
        if not ok then
            warn(screenName .. ": " .. tostring(err))
        end
        return result
    end

    if not Crabe.Settings._armed[screenName] then
        Crabe.Settings._armed[screenName] = "installed"
        if Crabe.write then
            Crabe.write("[Crabe.Settings] installed on " .. screenName .. ".BuildList")
        end
    end
    return true
end

-- Public only so the chunk patch, which runs as its own chunk in the game's
-- globals, can reach it. Not part of the API a mod calls.
function Crabe.Settings._retry(screenName)
    if type(screenName) ~= "string" then return end
    scheduleRetry(screenName)
end

-- Adds an option to a settings screen.
--
--   Crabe.Settings.addOption("SettingsVideo", {
--       id = "myOption",
--       text = "My Option",              -- or a "@Scn_..." localisation key
--       widgetType = "Toggle",           -- Toggle | Slider | LR_Toggle
--       get = function(self, id) return true end,
--       set = function(self, id, value) end,
--   })
--
-- get and set are called the way the game calls its own: get(self, id) and
-- set(self, id, value), where `value` is what the player just chose. Taken
-- from the disassembly of settingsvideo.lua, whose seven option pairs are all
-- 2-parameter getters and 3-parameter setters. A setter that ignores `value`
-- and flips from the current state instead looks right and does the wrong
-- thing the moment the menu and the mod disagree about what the current state
-- is -- which is exactly what the first version of window_mode did.
--
-- Safe to call from any Lua state and at any time: the option is remembered and
-- installed when that screen's chunk loads. Returns false, having said why,
-- rather than raising -- a mod that gets this wrong should not fail to load.
function Crabe.Settings.addOption(screenName, entry)
    if type(screenName) ~= "string" or not string.find(screenName, kValidScreen) then
        warn("addOption: screen name must be a Lua identifier, got " .. tostring(screenName))
        return false
    end
    if type(entry) ~= "table" or type(entry.id) ~= "string" or entry.id == "" then
        warn("addOption: entry must be a table with a non-empty string id")
        return false
    end

    local owner = (Crabe.Registry and Crabe.Registry._current) or "core"
    local entries = Crabe.Settings._pending[screenName]
    if not entries then
        entries = {}
        Crabe.Settings._pending[screenName] = entries
    end

    -- Same id from the same owner twice is a reload, not a second option.
    for index, record in ipairs(entries) do
        if record.entry.id == entry.id then
            entries[index] = { owner = owner, entry = entry }
            return true
        end
    end

    entries[#entries + 1] = { owner = owner, entry = entry }

    -- Revoked on hot reload with everything else the mod registered, so a
    -- removed mod's option does not outlive it.
    if Crabe.Registry and Crabe.Registry.track then
        Crabe.Registry.track(function()
            for index = #entries, 1, -1 do
                if entries[index].entry.id == entry.id and entries[index].owner == owner then
                    table.remove(entries, index)
                    break
                end
            end
        end)
    end

    -- The chunk that builds this screen may already have run -- a mod loading
    -- into a state that has been up for a while, or a hot reload -- so try now,
    -- and arm the patch for the case where it has not.
    if not Crabe.Settings._install(screenName) then
        scheduleRetry(screenName)
    end

    if not Crabe.Settings._armed[screenName .. ":patch"] then
        Crabe.Settings._armed[screenName .. ":patch"] = true
        -- screenName is validated above, so neither string below can be
        -- closed early or carry a second statement.
        local installer = "if Crabe and Crabe.Settings and "
            .. "not Crabe.Settings._install('" .. screenName .. "') then "
            .. "Crabe.Settings._retry('" .. screenName .. "') end"

        -- The exact chunk name, read out of a debug log of a real boot:
        -- "loadbuffer Presentation/SettingsVideo.lua (12654 bytes)". A named
        -- patch fires on that chunk alone.
        if Crabe.Hooks and Crabe.Hooks.patchNamedChunk then
            Crabe.Hooks.patchNamedChunk("Presentation/" .. screenName .. ".lua", installer)
        end

        -- And the content hint as well, for a screen whose chunk is named
        -- differently or not at all. It matches several chunks, which costs
        -- nothing: the install is guarded and idempotent.
        if Crabe.Hooks and Crabe.Hooks.patchChunk then
            Crabe.Hooks.patchChunk(screenName, installer)
        end
    end

    return true
end

-- Retries _install for a short window after a patch fired and found nothing.
--
-- Bounded on purpose: a screen the player never opens must not leave a
-- callback polling for the rest of the session. kRetryTicks is about eight
-- seconds at 60 Hz, which covers the gap between the chunk returning and the
-- caller assigning the global many times over.
local kRetryTicks = 500

function scheduleRetry(screenName)
    if Crabe.Settings._retrying[screenName] then return end
    if not (Game and Game.onTick) then return end

    Crabe.Settings._retrying[screenName] = true
    local ticksLeft = kRetryTicks

    Game.onTick(function()
        if not Crabe.Settings._retrying[screenName] then return end
        ticksLeft = ticksLeft - 1
        if Crabe.Settings._install(screenName) or ticksLeft <= 0 then
            Crabe.Settings._retrying[screenName] = nil
        end
    end)
end

-- Every option currently registered for a screen, in the order they load.
function Crabe.Settings.listOptions(screenName)
    local entries = Crabe.Settings._pending[screenName]
    local out = {}
    if entries then
        for index, record in ipairs(entries) do
            out[index] = record.entry
        end
    end
    return out
end

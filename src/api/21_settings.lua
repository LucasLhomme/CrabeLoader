-- CrabeLoader
-- File description:
-- Lets a mod add its own option to one of the game's settings screens, from any Lua state.
-- Installs when the screen's chunk loads, because the screen table does not exist before that.
-- Renders nothing itself; the game's own BuildList owns the list this appends to.
--
-- Authors: @LucasLhomme

Crabe = Crabe or {}
Crabe.Settings = Crabe.Settings or {}

Crabe.Settings._pending = Crabe.Settings._pending or {}
Crabe.Settings._armed = Crabe.Settings._armed or {}
Crabe.Settings._retrying = Crabe.Settings._retrying or {}

local kValidScreen = "^[A-Za-z_][A-Za-z0-9_]*$"
local kGlobals = (getfenv and getfenv(1)) or _G
local kListField = "listData"
local kPollTicks = 30

local scheduleRetry
local refreshChoice

local function warn(line)
    if Crabe.write then
        Crabe.write("! [Crabe.Settings] " .. tostring(line))
    end
end

-- Helper returning clamped choice index for an LR_Toggle choice record.
local function choiceIndex(self, record)
    local count = #record.choice.choices
    local index = tonumber(record.choice.get(self, record.entry.id)) or 1
    index = math.floor(index)
    if index < 1 then index = 1 end
    if index > count then index = count end
    return index
end

-- Updates choice entry enumValue to current choice label.
function refreshChoice(self, record)
    record.entry.enumValue = record.choice.choices[choiceIndex(self, record)]
end

-- Finds registered choice record by id for the given screen name.
local function findChoice(screenName, id)
    local entries = Crabe.Settings._pending[screenName]
    if not entries or id == nil then return nil end
    for _, record in ipairs(entries) do
        if record.choice and record.entry.id == id then
            return record
        end
    end
    return nil
end

-- Appends registered settings entries to the screen listData.
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
        if record.choice then
            refreshChoice(self, record)
        end
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

-- Wraps the screen BuildList and NextEnumValue methods.
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

        local hooks = Crabe.Settings._buildHooks and Crabe.Settings._buildHooks[screenName]
        if hooks then
            for _, record in ipairs(hooks) do
                local okHook, errHook = pcall(record.fn, self)
                if not okHook then
                    warn(screenName .. " hook: " .. tostring(errHook))
                end
            end
        end
        return result
    end

    screen.__crabeSettingsWrapper = screen.BuildList

    if screen.__crabeSettingsNextEnum == nil and type(screen.NextEnumValue) == "function" then
        screen.__crabeSettingsNextEnum = screen.NextEnumValue
    end
    local originalNextEnum = screen.__crabeSettingsNextEnum
    screen.NextEnumValue = function(self, id, newDirection, ...)
        local data = id ~= nil and self.listData and self.listData[tonumber(id)]
        local record = data and findChoice(screenName, data.id)
        if not record then
            if originalNextEnum then
                return originalNextEnum(self, id, newDirection, ...)
            end
            return nil
        end

        local index = choiceIndex(self, record)
        if newDirection == "LEFT" or tonumber(newDirection) == 0 then
            index = math.max(1, index - 1)
        elseif newDirection == "RIGHT" or tonumber(newDirection) == 1 then
            index = math.min(#record.choice.choices, index + 1)
        end
        local ok, err = pcall(record.choice.set, self, record.entry.id, index)
        if not ok then
            warn(screenName .. ": " .. record.entry.id .. ".set: " .. tostring(err))
        end
        refreshChoice(self, record)
        return record.entry.enumValue
    end

    if not Crabe.Settings._armed[screenName] then
        Crabe.Settings._armed[screenName] = "installed"
        if Crabe.write then
            Crabe.write("[Crabe.Settings] installed on " .. screenName .. ".BuildList")
        end
    end
    return true
end

-- Arms chunk patch for the settings screen.
local function armPatch(screenName)
    if not Crabe.Settings._armed[screenName .. ":patch"] then
        Crabe.Settings._armed[screenName .. ":patch"] = true
        local installer = "if Crabe and Crabe.Settings and "
            .. "not Crabe.Settings._install('" .. screenName .. "') then "
            .. "Crabe.Settings._retry('" .. screenName .. "') end"

        if Crabe.Hooks and Crabe.Hooks.patchNamedChunk then
            Crabe.Hooks.patchNamedChunk("Presentation/" .. screenName .. ".lua", installer)
        end

        if Crabe.Hooks and Crabe.Hooks.patchChunk then
            Crabe.Hooks.patchChunk(screenName, installer)
        end
    end
end

-- Retries settings screen installation via scheduler.
function Crabe.Settings._retry(screenName)
    if type(screenName) ~= "string" then return end
    scheduleRetry(screenName)
end

-- Registers a mod option on a game settings screen.
function Crabe.Settings.addOption(screenName, entry)
    if type(screenName) ~= "string" or not string.find(screenName, kValidScreen) then
        warn("addOption: screen name must be a Lua identifier, got " .. tostring(screenName))
        return false
    end
    if type(entry) ~= "table" or type(entry.id) ~= "string" or entry.id == "" then
        warn("addOption: entry must be a table with a non-empty string id")
        return false
    end

    local choice = nil
    if entry.choices ~= nil then
        if entry.widgetType ~= "LR_Toggle" or type(entry.choices) ~= "table" or #entry.choices == 0
            or type(entry.get) ~= "function" or type(entry.set) ~= "function" then
            warn("addOption: " .. entry.id .. ": a choice row needs widgetType = \"LR_Toggle\", "
                .. "a non-empty choices list, get and set")
            return false
        end
        choice = { choices = entry.choices, get = entry.get, set = entry.set }
        entry = { id = entry.id, text = entry.text, widgetType = "LR_Toggle", enumValue = entry.choices[1] }
    end

    local owner = (Crabe.Registry and Crabe.Registry._current) or "core"
    local entries = Crabe.Settings._pending[screenName]
    if not entries then
        entries = {}
        Crabe.Settings._pending[screenName] = entries
    end

    for index, record in ipairs(entries) do
        if record.entry.id == entry.id then
            entries[index] = { owner = owner, entry = entry, choice = choice }
            return true
        end
    end

    entries[#entries + 1] = { owner = owner, entry = entry, choice = choice }

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

    Crabe.Settings._install(screenName)
    scheduleRetry(screenName)

    armPatch(screenName)
    return true
end

-- Registers a callback invoked after the settings screen BuildList runs.
-- Allows mods to inspect or extend screen populated data.
function Crabe.Settings.onBuild(screenName, fn)
    if type(screenName) ~= "string" or not string.find(screenName, kValidScreen) then
        warn("onBuild: screen name must be a Lua identifier, got " .. tostring(screenName))
        return false
    end
    if type(fn) ~= "function" then
        warn("onBuild: callback must be a function")
        return false
    end

    local owner = (Crabe.Registry and Crabe.Registry._current) or "core"
    Crabe.Settings._buildHooks = Crabe.Settings._buildHooks or {}
    local hooks = Crabe.Settings._buildHooks[screenName]
    if not hooks then
        hooks = {}
        Crabe.Settings._buildHooks[screenName] = hooks
    end

    local record = { owner = owner, fn = fn }
    hooks[#hooks + 1] = record

    if Crabe.Registry and Crabe.Registry.track then
        Crabe.Registry.track(function()
            for index = #hooks, 1, -1 do
                if hooks[index] == record then
                    table.remove(hooks, index)
                    break
                end
            end
        end)
    end

    Crabe.Settings._install(screenName)
    scheduleRetry(screenName)

    armPatch(screenName)
    return true
end

-- Returns true if the screen BuildList is already wrapped.
local function isWrapped(screenName)
    local screen = rawget(kGlobals, screenName)
    return type(screen) == "table"
        and screen.__crabeSettingsWrapper ~= nil
        and screen.BuildList == screen.__crabeSettingsWrapper
end

-- Schedules periodic retries to install settings wrapper on screen.
function scheduleRetry(screenName)
    if Crabe.Settings._retrying[screenName] then return end
    if not (Game and Game.onTick) then return end

    Crabe.Settings._retrying[screenName] = true
    local ticks = 0

    local registry = Crabe.Registry
    local previous = registry and registry._current
    if registry then registry._current = nil end
    Game.onTick(function()
        ticks = ticks + 1
        if ticks % kPollTicks ~= 0 then return end
        if not isWrapped(screenName) then
            Crabe.Settings._install(screenName)
        end
    end)
    if registry then registry._current = previous end
end

-- Returns list of registered options for the specified screen.
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

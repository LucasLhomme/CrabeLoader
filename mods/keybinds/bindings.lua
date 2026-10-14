local Bindings = {}

Bindings.KEYBOARD = "keyboard"
Bindings.MOUSE = "mouse"
Bindings.PAD = "pad"

Bindings.FILE = "Crabe/keybinds.json"

local MOUSE_NAMES = { [0] = "Left click", "Right click", "Middle click", "Mouse 4", "Mouse 5", "Mouse 6", "Mouse 7", "Mouse 8" }

local PAD_NAMES = {
    [0x0001] = "D-pad Up", [0x0002] = "D-pad Down", [0x0004] = "D-pad Left", [0x0008] = "D-pad Right",
    [0x0010] = "Start", [0x0020] = "Back", [0x0040] = "Left stick click", [0x0080] = "Right stick click",
    [0x0100] = "LB", [0x0200] = "RB", [0x1000] = "A", [0x2000] = "B", [0x4000] = "X", [0x8000] = "Y",
}

local entries = {}
local menuOpen = false
local swallowing = false
local guards = {}

--- Reports whether this CrabeLoader can remap inputs; an older one cannot.
function Bindings.supported()
    local input = Crabe.Input
    return input ~= nil and input.remapKey ~= nil and input.clearRemap ~= nil and input.isKeyDown ~= nil
end

-- Returns the index of the entry that remaps `from` on `device`, or nil.
local function find(device, from)
    for index, entry in ipairs(entries) do
        if entry.device == device and entry.from == from then return index end
    end
    return nil
end

--- Returns what to show for one input: a key as the player's layout names it, a button by its label.
function Bindings.name(device, code)
    if device == Bindings.KEYBOARD then
        local name = Crabe.Input.keyName(code)
        if name ~= "" then return name end
        return string.format("Key 0x%02X", code)
    end
    if device == Bindings.MOUSE then return MOUSE_NAMES[code] or ("Mouse " .. (code + 1)) end
    return PAD_NAMES[code] or string.format("Button 0x%X", code)
end

--- Returns the entries, in the order they were made. Each is { device, from, to }.
function Bindings.list()
    return entries
end

-- Pushes one entry into the loader's remap table; false when the loader refuses it.
local function send(entry)
    if entry.device == Bindings.KEYBOARD then return Crabe.Input.remapKey(entry.from, entry.to) end
    if entry.device == Bindings.MOUSE then return Crabe.Input.remapMouseButton(entry.from, entry.to) end
    return Crabe.Input.remapPadButton(entry.from, entry.to)
end

-- Makes the game see nothing from one physical input.
local function swallow(device, code)
    if device == Bindings.KEYBOARD then return Crabe.Input.remapKey(code, 0) end
    if device == Bindings.MOUSE then return Crabe.Input.remapMouseButton(code, -1) end
    return Crabe.Input.remapPadButton(code, 0)
end

--- Rebuilds the loader's table from the entries, the menu state and the held inputs. Returns how many it refused.
--- While a game menu has the input the entries step aside, so the menu keeps its usual keys.
function Bindings.apply()
    if not Bindings.supported() then return 0 end
    swallowing = false
    Crabe.Input.clearRemap()
    local refused = 0
    if not menuOpen then
        for _, entry in ipairs(entries) do
            if not send(entry) then
                refused = refused + 1
                Crabe.write(string.format("[Key Bindings] remap refused: %s %s -> %s", entry.device, tostring(entry.from), tostring(entry.to)))
            end
        end
    end
    for _, guard in ipairs(guards) do swallow(guard.device, guard.code) end
    return refused
end

--- Stores one remap, replacing any earlier one on the same input; mapping an input to itself removes it.
--- `to` is a code of the same device. Returns false when the loader refuses it.
function Bindings.set(device, from, to)
    if not Bindings.supported() then return false end
    local index = find(device, from)
    if from == to then
        if index then table.remove(entries, index) end
        return Bindings.apply() == 0
    end
    local entry = { device = device, from = from, to = to }
    if index then entries[index] = entry else entries[#entries + 1] = entry end
    if send(entry) then
        Bindings.apply()
        return true
    end
    table.remove(entries, find(device, from))
    Bindings.apply()
    return false
end

-- Returns what the game sees when `physical` is pressed: its entry's target, or itself.
local function gameSees(device, physical)
    local index = find(device, physical)
    if index then return entries[index].to end
    return physical
end

--- Returns the physical input that makes the game see `default`, or nil when nothing does.
function Bindings.producing(device, default)
    for _, entry in ipairs(entries) do
        if entry.device == device and entry.to == default then return entry.from end
    end
    if gameSees(device, default) == default then return default end
    return nil
end

--- Makes `key` do what the game's own `default` input does, swapping with the input that did it until now.
--- Every input therefore keeps a job. Returns false when the loader refuses an entry.
function Bindings.bind(device, default, key)
    local old = Bindings.producing(device, default)
    if old == key then return true end
    local keyUsed = gameSees(device, key)
    if not Bindings.set(device, key, default) then return false end
    if old and not Bindings.set(device, old, keyUsed) then return false end
    return true
end

--- Makes the game see nothing from any key, mouse button or pad button until apply() runs.
--- A capture uses it so the key the player picks does not also act in the game.
function Bindings.swallowAll()
    if not Bindings.supported() then return end
    Crabe.Input.clearRemap()
    for code = 1, 255 do Crabe.Input.remapKey(code, 0) end
    for button = 0, 7 do Crabe.Input.remapMouseButton(button, -1) end
    for bit = 0, 15 do Crabe.Input.remapPadButton(2 ^ bit, 0) end
    swallowing = true
end

--- Tells whether a game menu has the input; the remaps step aside while one does.
function Bindings.setMenuOpen(open)
    open = (open == true)
    if open == menuOpen then return end
    menuOpen = open
    if not swallowing then Bindings.apply() end
end

--- Keeps one physical input away from the game until `isDown()` says it was released.
--- The game never saw it go down during a capture, so letting it through while held would read as a fresh press.
function Bindings.guard(device, code, isDown)
    for _, guard in ipairs(guards) do
        if guard.device == device and guard.code == code then return end
    end
    guards[#guards + 1] = { device = device, code = code, isDown = isDown }
    if not swallowing then Bindings.apply() end
end

--- Call once per tick: lets go of the guarded inputs that have been released.
function Bindings.update()
    local changed = false
    for index = #guards, 1, -1 do
        if not guards[index].isDown() then
            table.remove(guards, index)
            changed = true
        end
    end
    if changed and not swallowing then Bindings.apply() end
end

--- Removes every remap of one device and leaves the others.
function Bindings.clearDevice(device)
    for index = #entries, 1, -1 do
        if entries[index].device == device then table.remove(entries, index) end
    end
    Bindings.apply()
end

--- Removes every remap.
function Bindings.clear()
    entries = {}
    Bindings.apply()
end

--- Returns the entries as plain data for the settings file.
function Bindings.export()
    local saved = {}
    for index, entry in ipairs(entries) do
        saved[index] = { d = entry.device, f = entry.from, t = entry.to }
    end
    return saved
end

--- Restores entries read from the settings file, skipping any malformed one, then applies them.
--- Returns how many entries the loader refused.
function Bindings.restore(saved)
    entries = {}
    if type(saved) == "table" then
        for _, item in ipairs(saved) do
            local device, from, to = item.d, tonumber(item.f), tonumber(item.t)
            local known = (device == Bindings.KEYBOARD or device == Bindings.MOUSE or device == Bindings.PAD)
            if known and from and to and to >= 0 and from ~= to and not find(device, from) then
                entries[#entries + 1] = { device = device, from = from, to = to }
            end
        end
    end
    return Bindings.apply()
end

--- Writes the remaps to Crabe/keybinds.json, next to crabe.toml.
function Bindings.save()
    local ok, err = Crabe.Storage.save(Bindings.FILE, { version = 1, remaps = Bindings.export() })
    if not ok then Crabe.write("[Key Bindings] could not save " .. Bindings.FILE .. ": " .. tostring(err)) end
    return ok
end

--- Reads the remaps saved by a previous session and applies them; a missing file means no remap.
function Bindings.load()
    local saved = Crabe.Storage.load(Bindings.FILE)
    return Bindings.restore(type(saved) == "table" and saved.remaps or nil)
end

return Bindings

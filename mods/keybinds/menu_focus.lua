local MenuFocus = {}

local CLASS = "Screen"
local FSM_CLASS = "FSM_Standard"

local entered = setmetatable({}, { __mode = "k" })
local holding = setmetatable({}, { __mode = "k" })
local installed = false
local failed = false
local lastOpen = false

-- Calls a game function whose failure is expected in some states; returns its result, or nil when it raised.
local function poll(fn, ...)
    local ok, result = pcall(fn, ...)
    if ok then return result end
    return nil
end

-- Writes one line about the menu state to loader.log.
local function trace(format, ...)
    Crabe.write("[Key Bindings] menu focus: " .. string.format(format, ...))
end

-- Returns a screen's class name for the log, or "?".
local function nameOf(screen)
    local name = poll(screen.className, screen)
    return name and tostring(name) or "?"
end

-- Reports whether a screen is a menu or dialog that wants the input, rather than a HUD or overlay.
-- That is the game's own answer, from its screen state machine and GetNeedsInput.
local function wantsInput(screen, hudMode)
    local data = screen.ScreenData
    local fsm = type(data) == "table" and data.fsm or nil
    if hudMode ~= nil and type(fsm) == "table" and fsm.mode == hudMode then return false end
    local needs = poll(screen.GetNeedsInput, screen)
    return needs ~= nil and needs ~= false
end

-- Returns the game's name for the HUD screen mode, or nil while its state machine class is not loaded.
local function hudModeName()
    local fsm = poll(require, FSM_CLASS)
    return type(fsm) == "table" and fsm.HUD_STANDARD or nil
end

--- Wraps onEnter and onExit of the game's base screen class; safe to call again after a hot reload.
--- The originals are kept on the class, so a reinstall never wraps a wrapper.
--- Returns false while the game has no such class yet.
function MenuFocus.install()
    local cls = poll(require, CLASS)
    if type(cls) ~= "table" or type(cls.onEnter) ~= "function" or type(cls.onExit) ~= "function" then
        if not failed then
            failed = true
            trace("the game's %s class is not available yet; remaps also act inside menus until it is", CLASS)
        end
        return false
    end
    local originals = rawget(cls, "__crabeFocusOriginals")
    if not originals then
        originals = { onEnter = cls.onEnter, onExit = cls.onExit }
        rawset(cls, "__crabeFocusOriginals", originals)
    end
    cls.onEnter = function(self, ...)
        local result = originals.onEnter(self, ...)
        entered[self] = true
        return result
    end
    cls.onExit = function(self, ...)
        entered[self] = nil
        holding[self] = nil
        return originals.onExit(self, ...)
    end
    installed = true
    failed = false
    return true
end

--- Call once per tick: judges the screens entered since the last call and returns whether a menu holds the input.
--- A screen is judged a tick after it entered, because a HUD sets its mode after onEnter returns.
--- Installs the wrapper by itself once the game's class exists.
function MenuFocus.poll()
    if not installed and not MenuFocus.install() then return false end
    if next(entered) ~= nil then
        local hudMode = hudModeName()
        for screen in pairs(entered) do
            entered[screen] = nil
            if wantsInput(screen, hudMode) then holding[screen] = true end
        end
    end
    local open = next(holding) ~= nil
    if open ~= lastOpen then
        lastOpen = open
        local names = {}
        for screen in pairs(holding) do names[#names + 1] = nameOf(screen) end
        table.sort(names)
        trace("%s (%s)", open and "a menu has the input" or "no menu has the input", table.concat(names, ", "))
    end
    return open
end

return MenuFocus

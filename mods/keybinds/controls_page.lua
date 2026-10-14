local Bindings = require("mods.keybinds.bindings")
local Input = require("mods.keybinds.input")
local Glyphs = require("mods.keybinds.glyphs")

local ControlsPage = {}

ControlsPage.SCREEN = "SettingsExtras"

ControlsPage.PAGES = {
    keyboard = {
        title = "Keyboard and mouse bindings",
        reset = "Reset keyboard and mouse",
        devices = { [Bindings.KEYBOARD] = true, [Bindings.MOUSE] = true },
    },
    pad = {
        title = "Controller bindings",
        reset = "Reset controller",
        devices = { [Bindings.PAD] = true },
    },
}

local DEFAULT_PAGE = "keyboard"
local CAPTURE_SECONDS = 8
local GLYPH_SIZE = 20
local GLYPH_SINK = 19

ControlsPage.ACTIONS = {
    { id = "forward", device = Bindings.KEYBOARD, default = 0x11, label = "Move forward" },
    { id = "back", device = Bindings.KEYBOARD, default = 0x1F, label = "Move back" },
    { id = "left", device = Bindings.KEYBOARD, default = 0x1E, label = "Move left" },
    { id = "right", device = Bindings.KEYBOARD, default = 0x20, label = "Move right" },
    { id = "jump", device = Bindings.KEYBOARD, default = 0x39, label = "Jump" },
    { id = "interact", device = Bindings.KEYBOARD, default = 0x12, label = "Interact / pick up" },
    { id = "sprint", device = Bindings.KEYBOARD, default = 0x2A, label = "Sprint / target" },
    { id = "block", device = Bindings.KEYBOARD, default = 0x1D, label = "Block" },
    { id = "swap", device = Bindings.KEYBOARD, default = 0x10, label = "Swap hero" },
    { id = "ribbon", device = Bindings.KEYBOARD, default = 0x0F, label = "Toy Box menu" },
    { id = "tool1", device = Bindings.KEYBOARD, default = 0x02, label = "Previous tool" },
    { id = "tool2", device = Bindings.KEYBOARD, default = 0x03, label = "Next tool" },
    { id = "attack", device = Bindings.MOUSE, default = 0, label = "Attack" },
    { id = "throw", device = Bindings.MOUSE, default = 1, label = "Throw / shoot" },
    { id = "padJump", device = Bindings.PAD, default = 0x1000, label = "Jump" },
    { id = "padAttack", device = Bindings.PAD, default = 0x4000, label = "Attack" },
    { id = "padAlt", device = Bindings.PAD, default = 0x8000, label = "Alternate attack" },
    { id = "padBlock", device = Bindings.PAD, default = 0x2000, label = "Block" },
    { id = "padPause", device = Bindings.PAD, default = 0x0010, label = "Pause menu" },
    { id = "padRibbon", device = Bindings.PAD, default = 0x0020, label = "Toy Box menu" },
    { id = "padSwap", device = Bindings.PAD, default = 0x0040, label = "Swap hero" },
    { id = "padToolLeft", device = Bindings.PAD, default = 0x0004, label = "Previous tool" },
    { id = "padToolRight", device = Bindings.PAD, default = 0x0008, label = "Next tool" },
}

local RESET_ID = "crabe_bind_reset"
local ROW_PREFIX = "crabe_bind_"

local PROMPTS = {
    [Bindings.KEYBOARD] = "press a key",
    [Bindings.MOUSE] = "press a mouse button",
    [Bindings.PAD] = "press a button",
}

local TOKEN = {}

local capture = nil
local requestedPage = nil
local activeMenu = nil
local lastIndex = nil
local pendingWheel = nil

-- Writes one step of a rebind to loader.log, since what the game does with a key cannot be seen from outside.
local function trace(format, ...)
    Crabe.write("[Key Bindings] " .. string.format(format, ...))
end

-- Calls a game function and logs the error it raises instead of propagating it. Returns true on success.
local function run(label, fn, ...)
    local ok, err = pcall(fn, ...)
    if not ok then trace("%s failed: %s", label, tostring(err)) end
    return ok
end

-- Returns the page a screen instance shows: the one latched at its first build, else the one asked for.
local function pageOf(menu)
    local id = (menu and rawget(menu, "crabePage")) or requestedPage or DEFAULT_PAGE
    return ControlsPage.PAGES[id] and id or DEFAULT_PAGE
end

-- Returns the name of the key an action is on now, or "(unbound)".
local function valueText(action)
    local physical = Bindings.producing(action.device, action.default)
    if physical == nil then return "(unbound)" end
    return Bindings.name(action.device, physical)
end

-- Returns a row's text: the action then its key, as the game's icon when it has one, else its name in brackets.
local function rowText(action)
    if capture and capture.action == action then
        return string.format("%s: [ %s ]", action.label, PROMPTS[action.device])
    end
    local physical = Bindings.producing(action.device, action.default)
    local icon = physical ~= nil and Glyphs.icon(action.device, physical) or nil
    if icon then return string.format("%s: %s", action.label, Glyphs.markup(icon, GLYPH_SIZE, GLYPH_SINK)) end
    return string.format("%s: [%s]", action.label, valueText(action))
end

-- Rewrites the rows and has the game rebuild the list, keeping the cursor where it was.
local function redraw(menu)
    if not menu or not menu.listData then return end
    for _, row in ipairs(menu.listData) do
        if row.crabeAction then row.text = rowText(row.crabeAction) end
    end
    if menu.selectedIndex then menu.startId = menu.selectedIndex end
    run("list rebuild", menu.Rebuild, menu)
end

-- Keeps every input still held away from the game until it is released.
-- The capture hid them, so the key just chosen or the Escape that cancelled would otherwise act as a new press.
local function guardHeld()
    for _, held in ipairs(Input.heldInputs()) do Bindings.guard(held.device, held.code, held.isDown) end
end

-- Hands the keys and the pad back to the game, whatever ended the capture.
local function releaseInputs()
    Crabe.Input.captureKeys(nil)
    Crabe.Input.capturePad(false)
    Bindings.apply()
    guardHeld()
end

-- Ends a capture without changing any binding; the list is redrawn unless the screen is closing.
local function endCapture(redrawAfter)
    if not capture then return end
    local finished = capture
    capture = nil
    releaseInputs()
    if redrawAfter then redraw(finished.menu) end
end

-- Starts waiting for the input that will do `action`, with every input hidden from the game meanwhile.
local function beginCapture(menu, action)
    trace("waiting for %s on '%s' (%s, default %#x)", PROMPTS[action.device], action.label, action.device, action.default)
    if capture then endCapture(false) end
    capture = { menu = menu, action = action, elapsed = 0 }
    Bindings.swallowAll()
    Crabe.Input.captureKeys(Input.CAPTURE_KEYS)
    Crabe.Input.capturePad(true)
    Input.primeCapture()
    redraw(menu)
end

-- Rebinds the captured action to the input just pressed, when it comes from the right device, and saves it.
local function pressed(press)
    local action = capture.action
    if press.device ~= action.device then
        trace("ignored a %s press (%#x): '%s' waits for the %s", press.device, press.code, action.label, action.device)
        return
    end
    local finished = capture
    capture = nil
    Crabe.Input.captureKeys(nil)
    Crabe.Input.capturePad(false)
    Bindings.apply()
    local ok = Bindings.bind(action.device, action.default, press.code)
    guardHeld()
    if ok then Bindings.save() end
    trace("'%s' (%s %#x) now on %#x: %s, %d remap(s)", action.label, action.device, action.default, press.code,
        ok and "bound" or "REFUSED by the loader", #Bindings.list())
    redraw(finished.menu)
end

-- Puts every input of the page's devices back on its own job, and saves.
local function resetPage(menu)
    for device in pairs(ControlsPage.PAGES[pageOf(menu)].devices) do Bindings.clearDevice(device) end
    Bindings.save()
    redraw(menu)
end

-- Builds the list row of one action.
local function newRow(menu, action)
    return {
        id = ROW_PREFIX .. action.id,
        text = rowText(action),
        effectName = ROW_PREFIX .. action.id,
        widgetType = "Button",
        buttonState = "",
        crabeAction = action,
        onSelected = function()
            trace("row '%s' chosen", action.label)
            beginCapture(menu, action)
        end,
    }
end

-- Builds the list row that resets the page.
local function resetRow(menu)
    return {
        id = RESET_ID,
        text = ControlsPage.PAGES[pageOf(menu)].reset,
        effectName = RESET_ID,
        widgetType = "Button",
        buttonState = "",
        crabeReset = true,
        onSelected = function() resetPage(menu) end,
    }
end

-- Reports whether a list row is one this page added.
local function isOurs(row)
    return type(row) == "table" and (row.crabeAction ~= nil or row.crabeReset == true)
end

--- Makes the game's SettingsExtras class show the key bindings; safe to call again, also after a hot reload.
--- SettingsExtras is a list screen the game opens only on consoles, so on PC nothing else uses it.
--- The originals are kept on the class, so a reinstall never wraps a wrapper.
function ControlsPage.install(cls)
    local originals = rawget(cls, "__crabeNativeOriginals")
    if not originals then
        originals = {
            onEnter = cls.onEnter,
            BuildList = cls.BuildList,
            SelectItem = cls.SelectItem,
            onExit = cls.onExit,
            MovieLoaded = cls.MovieLoaded,
        }
        rawset(cls, "__crabeNativeOriginals", originals)
    end

    cls.onEnter = function(self, ...)
        activeMenu = self
        lastIndex, pendingWheel = nil, nil
        return originals.onEnter(self, ...)
    end

    cls.BuildList = function(self, ...)
        rawset(self, "crabePage", pageOf(self))
        local result = originals.BuildList(self, ...)
        self.listData = self.listData or {}
        local devices = ControlsPage.PAGES[pageOf(self)].devices
        for _, action in ipairs(ControlsPage.ACTIONS) do
            if devices[action.device] then self.listData[#self.listData + 1] = newRow(self, action) end
        end
        self.listData[#self.listData + 1] = resetRow(self)
        return result
    end

    cls.SelectItem = function(self, option, ...)
        local row = self.listData and self.listData[tonumber(option)]
        if isOurs(row) then
            row.onSelected(row)
            return
        end
        return originals.SelectItem(self, option, ...)
    end

    cls.onExit = function(self, ...)
        if capture and capture.menu == self then endCapture(false) end
        if activeMenu == self then activeMenu = nil end
        return originals.onExit(self, ...)
    end

    cls.MovieLoaded = function(self, ...)
        local result = originals.MovieLoaded(self, ...)
        self:SetButtonLegend("Btn_Legend_Select,Btn_Legend_Back")
        return result
    end

    cls.GetHeaderLabel = function(self) return ControlsPage.PAGES[pageOf(self)].title end

    rawset(cls, "__crabeNativeToken", TOKEN)
end

--- Opens one of the pages ("keyboard" or "pad") from a game menu screen, the way the game opens its sub-screens.
--- The screen's script is loaded first, so the wrappers are in place before its first build.
function ControlsPage.open(menu, page)
    if not Bindings.supported() then return end
    if not ControlsPage.PAGES[page] then page = DEFAULT_PAGE end
    run("screen load", require, ControlsPage.SCREEN)
    local cls = _G[ControlsPage.SCREEN]
    if type(cls) ~= "table" then
        trace("the game has no %s screen to hold the key bindings", ControlsPage.SCREEN)
        return
    end
    if rawget(cls, "__crabeNativeToken") ~= TOKEN then ControlsPage.install(cls) end
    requestedPage = page
    trace("opening the %s page", page)
    menu:AddScreen({
        screenName = ControlsPage.SCREEN,
        playerNum = menu.InstanceData and menu.InstanceData.playerNum,
    })
    menu:SetFocus(false)
end

--- Cancels a capture in progress without changing any binding.
function ControlsPage.cancel()
    endCapture(true)
end

-- Scrolls the open page by the wheel when the game did not move the selection for that notch itself.
-- A notch is acted on a tick later, and only if the selection stayed put, so a working wheel never scrolls twice.
local function scrollWheel()
    local menu = activeMenu
    local index = menu.selectedIndex
    lastIndex = lastIndex or index
    local scrolled = false
    if pendingWheel then
        local pending = pendingWheel
        pendingWheel = nil
        if index == pending.baseline then
            local call = pending.notches > 0 and "_root.PreviousElement" or "_root.NextElement"
            for _ = 1, math.abs(pending.notches) do
                run("wheel scroll", menu.CallFlashFunction, menu, call, true, false)
            end
            scrolled = true
        end
    end
    local notches = Input.takeWheel()
    if notches ~= 0 and index == lastIndex then
        pendingWheel = { baseline = index, notches = notches }
    end
    lastIndex = (not scrolled) and index or nil
end

--- Call once per tick: reads the input pressed while a row waits for one, and the wheel on an open page.
--- A capture ends on Escape or Backspace, or after eight seconds, without changing anything.
function ControlsPage.update(dt)
    if capture then
        capture.elapsed = capture.elapsed + (dt or 0)
        local cancelled = Input.captureCancelled()
        if cancelled or capture.elapsed > CAPTURE_SECONDS then
            trace("capture ended without an input: %s after %.1f s", cancelled and "cancelled" or "timed out", capture.elapsed)
            endCapture(true)
            return
        end
        local press = Input.pollCapture()
        if press then pressed(press) end
        return
    end
    if activeMenu then scrollWheel() end
end

--- Ends a capture and forgets the open page, for a shutdown or a hot reload.
function ControlsPage.shutdown()
    endCapture(false)
    activeMenu = nil
end

return ControlsPage

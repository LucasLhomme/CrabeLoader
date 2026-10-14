local Glyphs = {}

local KEYBOARD = {
    [0x01] = "esc", [0x0F] = "tab", [0x39] = "spacebar", [0x1C] = "enter", [0x9C] = "enter",
    [0x0E] = "backspace", [0x3A] = "caps", [0x2A] = "shift", [0x36] = "shift",
    [0x1D] = "ctrl", [0x9D] = "ctrl", [0x38] = "alt", [0xB8] = "alt",
    [0xC8] = "arrowup", [0xD0] = "arrowdown", [0xCB] = "arrowleft", [0xCD] = "arrowright",
    [0xD2] = "insert", [0xD3] = "delete", [0xC7] = "home", [0xCF] = "end", [0xC9] = "pgup", [0xD1] = "pgdn",
    [0x0C] = "minus", [0x0D] = "equals", [0x27] = "semicolon", [0x28] = "apostrophe", [0x29] = "tilde",
    [0x57] = "f11", [0x58] = "f12",
}

-- Names a row of keys, left to right, from the scan code of its first key.
local function addRun(first, names)
    for offset = 1, #names do KEYBOARD[first + offset - 1] = names:sub(offset, offset) end
end
addRun(0x10, "qwertyuiop")
addRun(0x1E, "asdfghjkl")
addRun(0x2C, "zxcvbnm")
addRun(0x02, "1234567890")
for number = 1, 10 do KEYBOARD[0x3A + number] = "f" .. number end

local MOUSE = { [0] = "mouselmb", [1] = "mousermb", [2] = "mousemmb" }

local PAD = {
    [0x1000] = "buttona", [0x2000] = "buttonb", [0x4000] = "buttonx", [0x8000] = "buttony",
    [0x0100] = "buttonlb", [0x0200] = "buttonrb", [0x0010] = "buttonstart", [0x0020] = "buttonback",
    [0x0001] = "buttondp_u", [0x0002] = "buttondp_d", [0x0004] = "buttondp_l", [0x0008] = "buttondp_r",
    [0x0040] = "lstick", [0x0080] = "rstick",
}

local TABLES = { keyboard = KEYBOARD, mouse = MOUSE, pad = PAD }

--- Returns the game's icon for an input ("cbi_keyw", "cbi_buttona"), one of the cbi_* textures it ships, or nil.
--- A letter follows the name the player's layout gives the key (AZERTY shows Z where QWERTY has W).
--- Other keys follow their scan code, mouse buttons are 0-based and pad buttons are XInput bits.
function Glyphs.icon(device, code)
    if device == "keyboard" then
        local label = Crabe.Input.keyName(code)
        if label:match("^%a$") then return "cbi_key" .. label:lower() end
    end
    local names = TABLES[device]
    local name = names and names[code]
    if not name then return nil end
    return ((device == "keyboard") and "cbi_key" or "cbi_") .. name
end

--- Returns the inline image markup that draws an icon in a line of menu text, as the game's own texts do.
--- `size` and `sink` are in the menu movie's pixels: its row text is 16 px high in a 23.6 px field.
--- A larger `sink` lowers the icon and a smaller one raises it.
function Glyphs.markup(icon, size, sink)
    return string.format('<img src="img://%s" width="%d" height="%d" align="baseline" vspace="%d"/>', icon, size, size, -sink)
end

return Glyphs

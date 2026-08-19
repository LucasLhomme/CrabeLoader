-- Mod menu. Mods declare entries here; the overlay draws them.
--
-- An entry is one of:
--   { label = "...", submenu = { title = "...", items = { ... } } }
--   { label = "...", action  = function() return status end }
--   { label = "...", toggle  = true, state = false,
--                    onToggle = function(on) return status end }
--   { label = "...", cycle   = { "x1", "x2" }, index = 1,
--                    onCycle = function(value, index) return status end }
--
-- The status string a handler returns is shown under the list; returning
-- nothing just leaves the previous one.
--
-- Navigation state lives here rather than in C++, so the overlay stays a
-- renderer: it reports what _describe() gives it and calls _activate()/_back()
-- when the player clicks. Handlers therefore always run on the game's Lua
-- thread, never on the render thread.

Crabe = Crabe or {}
Crabe.Menu = Crabe.Menu or {}

local Menu = Crabe.Menu

Menu.root = Menu.root or { title = "CRABE MENU", items = {} }
Menu.stack = Menu.stack or { { menu = Menu.root, index = 1 } }
Menu.status = Menu.status or ""

local function currentFrame()
    return Menu.stack[#Menu.stack]
end

-- Adds a top-level entry.
function Menu.register(entry)
    if type(entry) ~= "table" or type(entry.label) ~= "string" then
        error("Crabe.Menu.register: expected a table with a label (string)", 2)
    end
    local items = Menu.root.items
    items[#items + 1] = entry
    return entry
end

-- Adds an entry under a named submenu, creating that submenu on first use.
-- Lets several mods share one category without knowing about each other.
function Menu.registerInCategory(categoryLabel, entry)
    if type(categoryLabel) ~= "string" or categoryLabel == "" then
        error("Crabe.Menu.registerInCategory: category must be a non-empty string", 2)
    end
    if type(entry) ~= "table" or type(entry.label) ~= "string" then
        error("Crabe.Menu.registerInCategory: expected a table with a label (string)", 2)
    end

    local category
    for _, item in ipairs(Menu.root.items) do
        if item.label == categoryLabel and item.submenu then
            category = item
            break
        end
    end

    if not category then
        category = { label = categoryLabel,
                     submenu = { title = string.upper(categoryLabel), items = {} } }
        local items = Menu.root.items
        items[#items + 1] = category
    end

    local items = category.submenu.items
    items[#items + 1] = entry
    return entry
end

-- A handler failing must not take the menu down with it, so each one runs
-- under pcall and reports the error as its status line.
local function runHandler(fn, ...)
    if type(fn) ~= "function" then return nil end

    local ok, result = pcall(fn, ...)
    if not ok then return "Erreur: " .. tostring(result) end
    return result
end

-- What the overlay shows, as lines it can parse without a Lua parser:
--   T=<title>  one per view
--   L=<label>  one per item, suffixed with its state
--   S=<status> one per view
function Menu._describe()
    local frame = currentFrame()
    if not frame then return "T=CRABE MENU\nS=\n" end

    local menu = frame.menu
    local out = { "T=" .. tostring(menu.title) }

    for _, item in ipairs(menu.items) do
        local suffix = ""
        if item.submenu then
            suffix = "  >"
        elseif item.toggle then
            suffix = item.state and "  [ON]" or "  [OFF]"
        elseif item.cycle then
            suffix = "  [" .. tostring(item.cycle[item.index or 1]) .. "]"
        end
        out[#out + 1] = "L=" .. tostring(item.label) .. suffix
    end

    out[#out + 1] = "S=" .. tostring(Menu.status or "")
    return table.concat(out, "\n") .. "\n"
end

-- Activates the index-th entry of the current view (1-based) and returns the
-- view that results -- entering a submenu, flipping a toggle, advancing a
-- cycle, or running an action.
function Menu._activate(index)
    local frame = currentFrame()
    if not frame then return Menu._describe() end

    local item = frame.menu.items[index]
    if not item then return Menu._describe() end

    frame.index = index

    if item.submenu then
        Menu.stack[#Menu.stack + 1] = { menu = item.submenu, index = 1 }
        Menu.status = ""
        return Menu._describe()
    end

    local status
    if item.toggle then
        item.state = not item.state
        status = runHandler(item.onToggle, item.state)
    elseif item.cycle then
        local count = #item.cycle
        item.index = (item.index or 1) % count + 1
        status = runHandler(item.onCycle, item.cycle[item.index], item.index)
    else
        status = runHandler(item.action)
    end

    if status ~= nil then Menu.status = tostring(status) end
    return Menu._describe()
end

-- Leaves the current submenu. A no-op at the root, so the overlay can call it
-- unconditionally.
function Menu._back()
    if #Menu.stack > 1 then
        Menu.stack[#Menu.stack] = nil
        Menu.status = ""
    end
    return Menu._describe()
end

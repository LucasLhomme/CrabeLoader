-- Mod menu. Mods declare entries here; the overlay draws them.
--
-- An entry is one of:
--   { label = "...", submenu = { title = "...", items = { ... } } }
--   { label = "...", submenu = { title = "...", build = function() ... end } }
--   { label = "...", action  = function() return status end }
--   { label = "...", toggle  = true, state = false,
--                    onToggle = function(on) return status end }
--   { label = "...", cycle   = { "x1", "x2" }, index = 1,
--                    onCycle = function(value, index) return status end }
--
-- The status string a handler returns is shown under the list; returning
-- nothing just leaves the previous one.
--
-- A submenu's build function runs each time the submenu is opened and returns
-- the item list. It is how a menu shows something that only exists once the
-- game is running. Returning a string instead of a table shows that string as
-- the status and leaves the previous list alone.
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
-- Clears all menu entries and resets navigation stack.
function Menu.clear()
    Menu.root.items = {}
    Menu.stack = { { menu = Menu.root, index = 1 } }
    Menu.status = ""
end

-- Adds a top-level entry. If an entry with this label already exists, updates it.
function Menu.register(entry)
    if type(entry) ~= "table" or type(entry.label) ~= "string" then
        error("Crabe.Menu.register: expected a table with a label (string)", 2)
    end
    local items = Menu.root.items
    for i, existing in ipairs(items) do
        if existing.label == entry.label then
            items[i] = entry
            return entry
        end
    end
    items[#items + 1] = entry
    return entry
end

-- Adds an entry under a named submenu, creating that submenu on first use.
-- Lets several mods share one category without knowing about each other.
-- Idempotent: replaces any existing entry with the same label.
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
    for i, existing in ipairs(items) do
        if existing.label == entry.label then
            items[i] = entry
            return entry
        end
    end
    items[#items + 1] = entry
    return entry
end

-- A handler failing must not take the menu down with it, so each one runs
-- under pcall and reports the error as its status line.
local function runHandler(fn, ...)
    if type(fn) ~= "function" then return nil end

    local ok, result = pcall(fn, ...)
    if ok then return result end

    -- Also send it to loader.log. The status line is one line in a small
    -- window, so a long message is easy to miss entirely -- which reads as
    -- "the entry does nothing" rather than "the entry failed, here is why".
    local message = "Menu error: " .. tostring(result)
    if Crabe and Crabe.write then Crabe.write(message) end
    return message
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
        -- A submenu may carry a build function instead of a fixed list. Lists
        -- that only exist once the game is running -- levels, inventories --
        -- would be empty if they were captured when the mod loaded, so they
        -- are rebuilt every time the submenu is entered.
        if type(item.submenu.build) == "function" then
            local built, failure = item.submenu.build()

            if type(built) == "table" then
                item.submenu.items = built
                Menu.status = ""
            else
                -- Keep whatever was there and say why, rather than dropping
                -- the player into an empty list with no explanation.
                item.submenu.items = item.submenu.items or {}
                Menu.status = tostring(failure or built or "nothing to list yet")
            end
        else
            Menu.status = ""
        end

        Menu.stack[#Menu.stack + 1] = { menu = item.submenu, index = 1 }
        return Menu._describe()
    end

    -- Every activation is logged with the value the handler receives. A toggle
    -- reporting the opposite of what the player pressed is invisible from the
    -- outside, and that ambiguity has cost more time here than any real bug.
    local function trace(kind, value)
        if Crabe and Crabe.write then
            Crabe.write(string.format("Menu: %s '%s' -> %s",
                kind, tostring(item.label), tostring(value)))
        end
    end

    local status
    if item.toggle then
        trace("toggle was", item.state)
        item.state = not item.state
        trace("toggle now", item.state)
        status = runHandler(item.onToggle, item.state)
    elseif item.cycle then
        local count = #item.cycle
        item.index = (item.index or 1) % count + 1
        trace("cycle", item.cycle[item.index])
        status = runHandler(item.onCycle, item.cycle[item.index], item.index)
    else
        trace("action", "run")
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

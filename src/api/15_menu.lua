-- CrabeLoader
-- File description:
-- The mod menu: mods declare entries and categories here and the overlay draws them.
-- Declared on the script thread; the overlay reads it on the render thread and never calls back.
-- Draws nothing itself; rendering is src/presentation/overlay.cpp.
--
-- Authors: @LucasLhomme

Crabe = Crabe or {}
Crabe.Menu = Crabe.Menu or {}

local Menu = Crabe.Menu

Menu.root = Menu.root or { title = "CRABE MENU", items = {} }
Menu.stack = Menu.stack or { { menu = Menu.root, index = 1 } }
Menu.status = Menu.status or ""

-- Helper returning current menu navigation stack frame.
local function currentFrame()
    return Menu.stack[#Menu.stack]
end

-- Clears all menu entries and resets navigation stack.
function Menu.clear()
    Menu.root.items = {}
    Menu.stack = { { menu = Menu.root, index = 1 } }
    Menu.status = ""
end

-- Registers a top-level menu entry, updating if label already exists.
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

-- Registers a menu entry in a named submenu, creating it if needed.
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

-- Runs a menu callback safely under pcall and logs failures.
local function runHandler(fn, ...)
    if type(fn) ~= "function" then return nil end

    local ok, result = pcall(fn, ...)
    if ok then return result end

    local message = "Menu error: " .. tostring(result)
    if Crabe and Crabe.write then Crabe.write(message) end
    return message
end

-- Serializes current menu frame state into plain text for overlay.
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

-- Activates entry at 1-based index and returns serialized view.
function Menu._activate(index)
    local frame = currentFrame()
    if not frame then return Menu._describe() end

    local item = frame.menu.items[index]
    if not item then return Menu._describe() end

    frame.index = index

    if item.submenu then
        if type(item.submenu.build) == "function" then
            local built, failure = item.submenu.build()

            if type(built) == "table" then
                item.submenu.items = built
                Menu.status = ""
            else
                item.submenu.items = item.submenu.items or {}
                Menu.status = tostring(failure or built or "nothing to list yet")
            end
        else
            Menu.status = ""
        end

        Menu.stack[#Menu.stack + 1] = { menu = item.submenu, index = 1 }
        return Menu._describe()
    end

    -- Logs menu activation event with kind and value.
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
        local ok, result = pcall(item.onToggle, item.state)
        if not ok then
            item.state = not item.state
            status = "Error: " .. tostring(result)
            if Crabe and Crabe.write then Crabe.write("Menu error: " .. tostring(result)) end
        else
            status = result
        end
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

-- Navigates back to the parent submenu frame.
function Menu._back()
    if #Menu.stack > 1 then
        Menu.stack[#Menu.stack] = nil
        Menu.status = ""
    end
    return Menu._describe()
end

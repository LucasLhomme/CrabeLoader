-- Shared plumbing for the Lua API suite.
--
-- The suite runs under a standalone Lua 5.1.5 interpreter (tests/CMakeLists.txt
-- fetches and builds it) against src/api/*.lua read from the working tree --
-- the same files tools/embed_api.py bakes into the DLL, not a copy of them.

local harness = {}

harness.root = "."

local function normalise(path)
    return (path:gsub("\\", "/"))
end

function harness.path(...)
    return normalise(harness.root .. "/" .. table.concat({ ... }, "/"))
end

function harness.readFile(path)
    local file, err = io.open(path, "rb")
    if not file then
        error("could not read " .. path .. ": " .. tostring(err), 0)
    end
    local contents = file:read("*a")
    file:close()
    return contents
end

-- Lua 5.1 has no directory listing, so a shell provides one: `dir /b` is what
-- the Windows CI runner has, `ls -1` covers a developer on anything else.
function harness.listLuaFiles(dir)
    local names = {}
    local commands = {
        'dir /b "' .. dir:gsub("/", "\\") .. '\\*.lua" 2>nul',
        'ls -1 "' .. dir .. '"/*.lua 2>/dev/null',
    }

    for _, command in ipairs(commands) do
        local pipe = io.popen(command)
        if pipe then
            for line in pipe:lines() do
                local trimmed = line:gsub("%s+$", "")
                local name = trimmed:match("([^/\\]+)$")
                if name and name:sub(-4) == ".lua" then
                    names[#names + 1] = name
                end
            end
            pipe:close()
        end
        if #names > 0 then break end
    end

    -- tools/embed_api.py embeds sorted(api_dir.glob("*.lua")) and the loader
    -- injects them in that order. Deriving the order here the same way is the
    -- point: a module added tomorrow cannot silently fall out of sequence
    -- between the DLL and this suite. Every name is lowercase ASCII, so Lua's
    -- byte-order sort and Python's agree.
    table.sort(names)
    return names
end

-- ---------------------------------------------------------------------------
-- API injection
-- ---------------------------------------------------------------------------

local apiModules = nil

-- Compiles every API module once. The chunks are re-run per state: setfenv on
-- a chunk rebinds the closures it creates, so one compile serves every test.
function harness.apiModules()
    if apiModules then return apiModules end

    local dir = harness.path("src", "api")
    local names = harness.listLuaFiles(dir)

    if #names == 0 then
        error("no Lua API modules found in " .. dir ..
              " (the directory listing produced nothing -- is io.popen available?)", 0)
    end

    apiModules = {}
    for index, name in ipairs(names) do
        local chunk, err = loadfile(dir .. "/" .. name)
        if not chunk then
            error("API module " .. name .. " does not compile: " .. tostring(err), 0)
        end
        apiModules[index] = { name = name, chunk = chunk }
    end
    return apiModules
end

local stubs = nil

-- A fresh, isolated API state: every module injected in embed order into a
-- private environment.
--
-- Global *reads* fall through to the interpreter's own globals (string, table,
-- pcall) while global *writes* land in this table, so `Crabe` and `Game` belong
-- to this state alone. That is what keeps one test from inheriting the
-- subscriptions of the one before it -- a second lua_State, which is what the
-- game would give us, is not something a standalone interpreter can hand out.
function harness.newApiState()
    local env = {}
    setmetatable(env, { __index = _G })

    -- 00_core.lua inserts a disk loader into package.loaders. Without a private
    -- copy, every injection would push another one onto the interpreter's.
    env.package = {
        path = package.path,
        cpath = package.cpath,
        loaded = {},
        preload = {},
        loaders = {},
    }
    for index, loader in ipairs(package.loaders) do
        env.package.loaders[index] = loader
    end

    stubs = stubs or assert(loadfile(harness.path("tests", "lua", "stubs.lua")))()
    env.CrabeTestNatives = stubs.install(env)

    for _, module in ipairs(harness.apiModules()) do
        setfenv(module.chunk, env)
        local ok, err = pcall(module.chunk)
        if not ok then
            error("API module " .. module.name .. " failed to load: " .. tostring(err), 0)
        end
    end
    return env
end

-- ---------------------------------------------------------------------------
-- Assertions
-- ---------------------------------------------------------------------------

function harness.assertEquals(actual, expected, what)
    if actual ~= expected then
        error(string.format("%s: expected %s, got %s",
                            what or "assertEquals", tostring(expected), tostring(actual)), 2)
    end
end

function harness.assertTrue(value, what)
    if not value then
        error(string.format("%s: expected a truthy value, got %s",
                            what or "assertTrue", tostring(value)), 2)
    end
end

function harness.assertFalse(value, what)
    if value then
        error(string.format("%s: expected a falsy value, got %s",
                            what or "assertFalse", tostring(value)), 2)
    end
end

function harness.assertNil(value, what)
    if value ~= nil then
        error(string.format("%s: expected nil, got %s",
                            what or "assertNil", tostring(value)), 2)
    end
end

function harness.assertNotNil(value, what)
    if value == nil then
        error((what or "assertNotNil") .. ": expected a value, got nil", 2)
    end
end

function harness.assertContains(text, needle, what)
    if type(text) ~= "string" or not text:find(needle, 1, true) then
        error(string.format("%s: expected to find %q in %q",
                            what or "assertContains", needle, tostring(text)), 2)
    end
end

function harness.assertNotContains(text, needle, what)
    if type(text) == "string" and text:find(needle, 1, true) then
        error(string.format("%s: expected not to find %q in %q",
                            what or "assertNotContains", needle, tostring(text)), 2)
    end
end

return harness

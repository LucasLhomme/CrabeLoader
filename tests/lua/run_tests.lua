-- Runner for the Lua API suite.
--
--   crabe_lua tests/lua/run_tests.lua <repo-root>
--
-- Loads src/api/*.lua in the order tools/embed_api.py embeds them, then runs
-- every tests/lua/test_*.lua against a fresh injection of that API. Exits
-- non-zero on the first failing case so CTest and CI fail with it.

local root = ...

if not root or root == "" then
    local function fileExists(p)
        local f = io.open(p, "rb")
        if f then f:close() return true end
        return false
    end

    if fileExists("./tests/lua/harness.lua") then
        root = "."
    elseif fileExists("./harness.lua") and fileExists("../../src/api/00_core.lua") then
        root = "../.."
    elseif fileExists("../lua/harness.lua") and fileExists("../src/api/00_core.lua") then
        root = ".."
    else
        io.stderr:write("usage: crabe_lua run_tests.lua [repo-root]\n")
        os.exit(2)
    end
end

root = root:gsub("\\", "/"):gsub("/+$", "")

local harness = assert(loadfile(root .. "/tests/lua/harness.lua"))()
harness.root = root

local modules = harness.apiModules()
io.write(string.format("[lua] %d API module(s) in embed order, first %s, last %s\n",
                       #modules, modules[1].name, modules[#modules].name))

local suiteNames = {}
for _, name in ipairs(harness.listLuaFiles(root .. "/tests/lua")) do
    if name:sub(1, 5) == "test_" then
        suiteNames[#suiteNames + 1] = name
    end
end

if #suiteNames == 0 then
    io.stderr:write("[lua] no test_*.lua files found in " .. root .. "/tests/lua\n")
    os.exit(2)
end

local passed, failed = 0, {}

for _, fileName in ipairs(suiteNames) do
    local path = root .. "/tests/lua/" .. fileName
    local suite = assert(loadfile(path))(harness)

    io.write("\n" .. (suite.name or fileName) .. "\n")

    for _, case in ipairs(suite.cases) do
        local title, body = case[1], case[2]

        -- A fresh API state per case: no test inherits another's
        -- subscriptions, and each can read its own baseline.
        local ok, err = xpcall(function()
            body(harness.newApiState(), harness)
        end, debug.traceback)

        if ok then
            passed = passed + 1
            io.write("  ok    " .. title .. "\n")
        else
            failed[#failed + 1] = { file = fileName, title = title, err = err }
            io.write("  FAIL  " .. title .. "\n")
            io.write("        " .. tostring(err):gsub("\n", "\n        ") .. "\n")
        end
    end
end

io.write(string.format("\n[lua] %d passed, %d failed\n", passed, #failed))

if #failed > 0 then
    io.write("[lua] failures:\n")
    for _, failure in ipairs(failed) do
        io.write("  " .. failure.file .. ": " .. failure.title .. "\n")
    end
    os.exit(1)
end

os.exit(0)

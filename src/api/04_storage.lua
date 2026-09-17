Crabe = Crabe or {}
Crabe.Storage = Crabe.Storage or {}

local parseValue, parseArray, parseObject

--- Advances the string index past ASCII whitespace characters.
--- Returns the next non-whitespace character position.
local function skipWhitespace(str, idx)
    local len = #str
    while idx <= len do
        local b = string.byte(str, idx)
        if b == 32 or b == 9 or b == 10 or b == 13 then
            idx = idx + 1
        else
            break
        end
    end
    return idx
end

--- Parses a JSON quoted string token and handles escape sequences.
--- Returns the decoded string and the next parse position.
local function parseString(str, idx)
    idx = idx + 1
    local len = #str
    local buf = {}
    while idx <= len do
        local c = string.sub(str, idx, idx)
        if c == '"' then
            return table.concat(buf), idx + 1
        elseif c == '\\' then
            idx = idx + 1
            local esc = string.sub(str, idx, idx)
            if esc == '"' then
                buf[#buf + 1] = '"'
            elseif esc == '\\' then
                buf[#buf + 1] = '\\'
            elseif esc == '/' then
                buf[#buf + 1] = '/'
            elseif esc == 'n' then
                buf[#buf + 1] = '\n'
            elseif esc == 'r' then
                buf[#buf + 1] = '\r'
            elseif esc == 't' then
                buf[#buf + 1] = '\t'
            elseif esc == 'b' then
                buf[#buf + 1] = '\b'
            elseif esc == 'f' then
                buf[#buf + 1] = '\f'
            else
                buf[#buf + 1] = esc
            end
            idx = idx + 1
        else
            buf[#buf + 1] = c
            idx = idx + 1
        end
    end
    error("JSON parse error: unterminated string", 2)
end

--- Matches and parses a numeric JSON value at the current index.
--- Returns the converted Lua number and next parse position.
local function parseNumber(str, idx)
    local match = string.match(str, "^%-?%d+%.?%d*[eE]?[%+%-]?%d*", idx)
    if not match or match == "" or match == "-" then
        error("JSON parse error: invalid number at index " .. idx, 2)
    end
    return tonumber(match), idx + #match
end

--- Parses a JSON array structure into a sequential Lua table.
--- Recursively evaluates contained values until the closing bracket.
parseArray = function(str, idx)
    idx = idx + 1
    local arr = {}
    idx = skipWhitespace(str, idx)
    if string.sub(str, idx, idx) == "]" then
        return arr, idx + 1
    end
    while true do
        local val
        val, idx = parseValue(str, idx)
        arr[#arr + 1] = val
        idx = skipWhitespace(str, idx)
        local c = string.sub(str, idx, idx)
        if c == "]" then
            return arr, idx + 1
        elseif c == "," then
            idx = skipWhitespace(str, idx + 1)
        else
            error("JSON parse error: expected ',' or ']' at index " .. idx, 2)
        end
    end
end

--- Parses a JSON object structure into a key-value Lua dictionary table.
--- Recursively evaluates key strings and values until closing brace.
parseObject = function(str, idx)
    idx = idx + 1
    local obj = {}
    idx = skipWhitespace(str, idx)
    if string.sub(str, idx, idx) == "}" then
        return obj, idx + 1
    end
    while true do
        if string.sub(str, idx, idx) ~= '"' then
            error("JSON parse error: expected key string at index " .. idx, 2)
        end
        local key
        key, idx = parseString(str, idx)
        idx = skipWhitespace(str, idx)
        if string.sub(str, idx, idx) ~= ":" then
            error("JSON parse error: expected ':' after key at index " .. idx, 2)
        end
        idx = skipWhitespace(str, idx + 1)
        local val
        val, idx = parseValue(str, idx)
        obj[key] = val
        idx = skipWhitespace(str, idx)
        local c = string.sub(str, idx, idx)
        if c == "}" then
            return obj, idx + 1
        elseif c == "," then
            idx = skipWhitespace(str, idx + 1)
        else
            error("JSON parse error: expected ',' or '}' at index " .. idx, 2)
        end
    end
end

--- Dispatches parsing to the appropriate JSON datatype handler.
--- Evaluates objects, arrays, strings, booleans, null, or numbers.
parseValue = function(str, idx)
    idx = skipWhitespace(str, idx)
    local c = string.sub(str, idx, idx)
    if c == "{" then
        return parseObject(str, idx)
    elseif c == "[" then
        return parseArray(str, idx)
    elseif c == '"' then
        return parseString(str, idx)
    elseif c == "t" and string.sub(str, idx, idx + 3) == "true" then
        return true, idx + 4
    elseif c == "f" and string.sub(str, idx, idx + 4) == "false" then
        return false, idx + 5
    elseif c == "n" and string.sub(str, idx, idx + 3) == "null" then
        return nil, idx + 4
    else
        return parseNumber(str, idx)
    end
end

--- Recursively converts a Lua value into valid JSON formatted text.
--- Handles tables as arrays or key-value objects depending on indices.
local function encodeValue(val)
    local t = type(val)
    if t == "nil" then
        return "null"
    elseif t == "boolean" then
        return tostring(val)
    elseif t == "number" then
        if val ~= val then
            return "null"
        elseif val == math.huge then
            return "1e+999"
        elseif val == -math.huge then
            return "-1e+999"
        else
            return tostring(val)
        end
    elseif t == "string" then
        local s = string.gsub(val, '\\', '\\\\')
        s = string.gsub(s, '"', '\\"')
        s = string.gsub(s, '\n', '\\n')
        s = string.gsub(s, '\r', '\\r')
        s = string.gsub(s, '\t', '\\t')
        return '"' .. s .. '"'
    elseif t == "table" then
        local isArray = true
        local maxIdx = 0
        local count = 0
        for k, _ in pairs(val) do
            count = count + 1
            if type(k) ~= "number" or k <= 0 or math.floor(k) ~= k then
                isArray = false
                break
            elseif k > maxIdx then
                maxIdx = k
            end
        end
        if isArray and count == maxIdx then
            local parts = {}
            for i = 1, maxIdx do
                parts[i] = encodeValue(val[i])
            end
            return "[" .. table.concat(parts, ",") .. "]"
        else
            local parts = {}
            for k, v in pairs(val) do
                local kStr = tostring(k)
                local safeKey = string.gsub(kStr, '\\', '\\\\')
                safeKey = string.gsub(safeKey, '"', '\\"')
                parts[#parts + 1] = '"' .. safeKey .. '":' .. encodeValue(v)
            end
            return "{" .. table.concat(parts, ",") .. "}"
        end
    else
        return "null"
    end
end

--- Serializes a Lua table or primitive value into a JSON string.
--- Returns the resulting JSON formatted string.
function Crabe.Storage.encode(dataTable)
    return encodeValue(dataTable)
end

--- Deserializes a JSON formatted string into a Lua table structure.
--- Returns the decoded value or nil and error message upon failure.
function Crabe.Storage.decode(jsonString)
    if type(jsonString) ~= "string" or jsonString == "" then
        return nil, "empty or non-string input"
    end
    local ok, res = pcall(function()
        local val, _ = parseValue(jsonString, 1)
        return val
    end)
    if not ok then
        return nil, res
    end
    return res
end

--- Serializes a table to JSON and persists it to a file on disk.
--- Returns boolean success flag and optional error message.
function Crabe.Storage.save(filename, dataTable)
    if type(filename) ~= "string" or filename == "" then
        error("Crabe.Storage.save: expected non-empty filename string", 2)
    end
    local jsonStr = Crabe.Storage.encode(dataTable)
    local file, err = io.open(filename, "w")
    if not file then
        return false, err
    end
    file:write(jsonStr)
    file:close()
    return true
end

--- Reads a file from disk and parses its JSON content into a Lua table.
--- Returns the decoded table or nil and error message if missing.
function Crabe.Storage.load(filename)
    if type(filename) ~= "string" or filename == "" then
        error("Crabe.Storage.load: expected non-empty filename string", 2)
    end
    local file, err = io.open(filename, "r")
    if not file then
        return nil, err
    end
    local content = file:read("*a")
    file:close()
    if not content or content == "" then
        return nil, "empty file content"
    end
    return Crabe.Storage.decode(content)
end

--- Returns a mod-scoped storage interface targeting mods/<modName>/.
--- Provides bound load and save helpers tailored for the specified mod.
function Crabe.Storage.forMod(modName)
    local prefix = "mods/" .. tostring(modName) .. "/"
    local scoped = {}
    scoped.load = function(filename)
        return Crabe.Storage.load(prefix .. filename)
    end
    scoped.save = function(filename, dataTable)
        return Crabe.Storage.save(prefix .. filename, dataTable)
    end
    return scoped
end

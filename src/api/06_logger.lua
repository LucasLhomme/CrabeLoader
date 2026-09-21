-- CrabeLoader
-- File description:
-- Per-mod logger objects with levels, so a line is attributable without each call naming the mod.
-- Each instance keeps its own buffer as well as writing out, which is what a mod UI can render.
-- Writes nothing directly -- everything goes through Crabe.write in src/api/00_core.lua.
--
-- Authors: @LucasLhomme

Crabe = Crabe or {}
Crabe.Logger = Crabe.Logger or {
    _loggers = {}
}

local LoggerInstance = {}
LoggerInstance.__index = LoggerInstance

--- Dispatches a formatted log message to Crabe.write and the internal buffer.
--- Concatenates variable arguments with tab delimiters.
function LoggerInstance:log(level, ...)
    local count = select("#", ...)
    local parts = {}
    for i = 1, count do
        parts[i] = tostring((select(i, ...)))
    end
    local msg = table.concat(parts, "\t")
    local formatted = "[" .. self.name .. "][" .. tostring(level) .. "] " .. msg
    if Crabe.write then
        Crabe.write(formatted)
    end
    local buf = self._buffer
    buf[#buf + 1] = formatted
    while #buf > self._maxBuffer do
        table.remove(buf, 1)
    end
    return formatted
end

--- Emits an informational log entry with [INFO] severity level.
--- Forwards all provided arguments to the logger dispatch.
function LoggerInstance:info(...)
    return self:log("INFO", ...)
end

--- Emits a warning log entry with [WARN] severity level.
--- Forwards all provided arguments to the logger dispatch.
function LoggerInstance:warn(...)
    return self:log("WARN", ...)
end

--- Alias for warn emitting a log entry with [WARN] severity level.
--- Forwards all provided arguments to the logger dispatch.
function LoggerInstance:warning(...)
    return self:log("WARN", ...)
end

--- Emits an error log entry with [ERROR] severity level.
--- Forwards all provided arguments to the logger dispatch.
function LoggerInstance:error(...)
    return self:log("ERROR", ...)
end

--- Emits a debug log entry with [DEBUG] severity level.
--- Forwards all provided arguments to the logger dispatch.
function LoggerInstance:debug(...)
    return self:log("DEBUG", ...)
end

--- Returns the array of stored log lines recorded in the local buffer.
--- Provides historical entries up to the configured buffer limit.
function LoggerInstance:getBuffer()
    return self._buffer
end

--- Clears all log entries stored within the local buffer.
--- Resets the buffer to an empty array.
function LoggerInstance:clear()
    self._buffer = {}
end

--- Creates or retrieves a named logger instance for a mod.
--- Returns a LoggerInstance configured with independent buffer.
function Crabe.Logger.create(modName)
    local name = tostring(modName or "Mod")
    if Crabe.Logger._loggers[name] then
        return Crabe.Logger._loggers[name]
    end
    local obj = {
        name = name,
        _buffer = {},
        _maxBuffer = 200
    }
    setmetatable(obj, LoggerInstance)
    Crabe.Logger._loggers[name] = obj
    return obj
end

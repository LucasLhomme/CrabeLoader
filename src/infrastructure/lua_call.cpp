/*
** CrabeLoader
** File description:
** Calls into the game Lua C API, and hosts the loadfile, loadbuffer and pcall detours.
** Every call saves and restores the stack top, so a failure never leaves the VM stack skewed.
** Resolves no address -- that pass is src/infrastructure/lua_symbols.cpp.
**
** Authors: @LucasLhomme
*/

#include <cstring>
#include <format>

#include "infrastructure/lua_call.hpp"
#include "application/loader.hpp"
#include "shared/logger.hpp"

namespace crabe::infrastructure {

// Lua's LUA_MULTRET: return every value the chunk produced.
constexpr int kLuaMultret = -1;

namespace {

thread_local bool g_inLoaderTick = false;

class ScopedFlag final {
public:
    explicit ScopedFlag(bool& flag) : _flag(flag) { _flag = true; }
    ~ScopedFlag() { _flag = false; }

    ScopedFlag(const ScopedFlag&) = delete;
    ScopedFlag& operator=(const ScopedFlag&) = delete;

private:
    bool& _flag;
};

}

LuaCall& LuaCall::get()
{
    static LuaCall instance;
    return instance;
}

// Stack-reading/pushing addresses are stored even when unresolved: they are
// called directly, never hooked, so a missing one degrades a feature instead
// of breaking the hooks installed above.
bool LuaCall::initialize(const LuaApiAddresses& addresses)
{
    bool anyInstalled = false;

    anyInstalled |= _hookLoadfile.installLogged(addresses.loadfile,
                            reinterpret_cast<void*>(&LuaCall::hkLoadfile), "LuaCall", "luaL_loadfile");
    anyInstalled |= _hookLoadbuffer.installLogged(addresses.loadbuffer,
                            reinterpret_cast<void*>(&LuaCall::hkLoadbuffer), "LuaCall", "luaL_loadbuffer");
    anyInstalled |= _hookPcall.installLogged(addresses.pcall,
                            reinterpret_cast<void*>(&LuaCall::hkPcall), "LuaCall", "lua_pcall");

    _getfield = reinterpret_cast<t_lua_getfield>(addresses.getfield);
    _tolstring = reinterpret_cast<t_lua_tolstring>(addresses.tolstring);
    _settop = reinterpret_cast<t_lua_settop>(addresses.settop);
    _gettop = reinterpret_cast<t_lua_gettop>(addresses.gettop);
    _pushnumber = reinterpret_cast<t_lua_pushnumber>(addresses.pushnumber);
    _tonumber = reinterpret_cast<t_lua_tonumber>(addresses.tonumber);
    _toboolean = reinterpret_cast<t_lua_toboolean>(addresses.toboolean);
    _pushlstring = reinterpret_cast<t_lua_pushlstring>(addresses.pushlstring);
    _pushboolean = reinterpret_cast<t_lua_pushboolean>(addresses.pushboolean);
    _pushnil = reinterpret_cast<t_lua_pushnil>(addresses.pushnil);
    _isnumber = reinterpret_cast<t_lua_isnumber>(addresses.isnumber);
    _pushcclosure = reinterpret_cast<t_lua_pushcclosure>(addresses.pushcclosure);
    _rawset = reinterpret_cast<t_lua_rawset>(addresses.rawset);

    if (!_tolstring || !_settop) {
        crabe::shared::Logger::getInstance().warning(
            "LuaCall: lua_tolstring/lua_settop unresolved; snippet results and errors will be unavailable.");
    }

    return anyInstalled;
}

void LuaCall::uninitialize()
{
    _hookLoadfile.remove();
    _hookLoadbuffer.remove();
    _hookPcall.remove();
}

LuaCall::t_luaL_loadfile LuaCall::originalLoadfile() const
{
    return reinterpret_cast<t_luaL_loadfile>(_hookLoadfile.getOriginal());
}

LuaCall::t_luaL_loadbuffer LuaCall::originalLoadbuffer() const
{
    return reinterpret_cast<t_luaL_loadbuffer>(_hookLoadbuffer.getOriginal());
}

LuaCall::t_lua_pcall LuaCall::originalPcall() const
{
    return reinterpret_cast<t_lua_pcall>(_hookPcall.getOriginal());
}

// lua_tolstring converts the stack slot to a string in place; fine for a
// value we own and drop right away, but must never target a slot still
// belonging to the game.
std::string LuaCall::popString(void* L, int index) const
{
    if (!_tolstring || !_settop)
        return {};

    const char* text = _tolstring(L, index, nullptr);
    std::string out = text ? text : "";

    _settop(L, -2);
    return out;
}

int __cdecl LuaCall::hkLoadfile(void* L, const char* filename)
{
    crabe::shared::Logger::getInstance().debug("Lua: loadfile {}", filename ? filename : "<null>");
    crabe::application::Loader::get().onLuaState(L);
    return LuaCall::get().originalLoadfile()(L, filename);
}

// The game ships precompiled bytecode and never calls loadfile, so this is
// the only place a valid Lua state is observed (onLuaState no-ops after the
// first call).
int __cdecl LuaCall::hkLoadbuffer(void* L, const char* buff, size_t size, const char* name)
{
    crabe::shared::Logger::getInstance().debug("Lua: loadbuffer {} ({} bytes)", name ? name : "<null>", size);
    crabe::application::Loader::get().onLuaState(L);

    if (const std::string* replacement = crabe::application::Loader::get().findLoadOverride(buff, size)) {
        crabe::shared::Logger::getInstance().info("LuaCall: load override matched, substituting chunk ({} -> {} bytes).",
                                    size, replacement->size());
        return LuaCall::get().originalLoadbuffer()(L, replacement->data(), replacement->size(), name);
    }

    crabe::application::Loader::get().armPatchIfMatched(buff, size);
    crabe::application::Loader::get().armPatchIfNameMatched(name);

    return LuaCall::get().originalLoadbuffer()(L, buff, size, name);
}

// Called thousands of times/sec: no logging, no allocation when nothing is
// queued. Also the only safe place to run queued keybind calls -- the game's
// own thread holding L, not the input-polling thread.
int __cdecl LuaCall::hkPcall(void* L, int nargs, int nresults, int errfunc)
{
    crabe::application::Loader& loader = crabe::application::Loader::get();
    loader.ensureRuntimeReady(L);
    if (!g_inLoaderTick && loader.isGameState(L)) {
        ScopedFlag tickScope(g_inLoaderTick);
        loader.runTicks(L);
    }

    int result = LuaCall::get().originalPcall()(L, nargs, nresults, errfunc);

    if (loader.hasArmedPatch()) {
        crabe::application::Loader::ChunkRule armed = loader.takeArmedPatch();
        LuaCall::get().runPatch(L, armed.source, armed.label);
    }

    return result;
}

// Borrows the game's own pending lua_pcall's stack; must be handed back at
// exactly the height it was found at, or the game calls the wrong slot.
bool LuaCall::runFile(void* L, const char* path) const
{
    t_luaL_loadfile loadfile = originalLoadfile();
    t_lua_pcall pcall = originalPcall();

    if (!L || !loadfile || !pcall) {
        crabe::shared::Logger::getInstance().error("LuaCall: cannot run '{}': Lua state or hooks unavailable.", path);
        return false;
    }

    ChunkResult result = runChunk(L, kLuaMultret, [&]() { return loadfile(L, path); });

    if (result.stage == ChunkResult::Stage::LoadFailed) {
        crabe::shared::Logger::getInstance().error("LuaCall: luaL_loadfile('{}') failed (status {}): {}",
                                    path, result.status, result.text);
    } else if (result.stage == ChunkResult::Stage::CallFailed) {
        crabe::shared::Logger::getInstance().error("LuaCall: lua_pcall('{}') failed (status {}): {}",
                                    path, result.status, result.text);
    }
    return static_cast<bool>(result);
}

bool LuaCall::runBuffer(void* L, const char* buff, size_t size, const char* name) const
{
    t_luaL_loadbuffer loadbuffer = originalLoadbuffer();
    t_lua_pcall pcall = originalPcall();

    if (!L || !loadbuffer || !pcall) {
        crabe::shared::Logger::getInstance().error("LuaCall: cannot run buffer '{}': Lua state or hooks unavailable.",
                                    name ? name : "unnamed");
        return false;
    }

    std::string chunkName = name ? (std::string("@") + name) : "@embedded";
    ChunkResult result = runChunk(L, kLuaMultret, [&]() {
        return loadbuffer(L, buff, size, chunkName.c_str());
    });

    if (result.stage == ChunkResult::Stage::LoadFailed) {
        crabe::shared::Logger::getInstance().error("LuaCall: luaL_loadbuffer('{}') failed (status {}): {}",
                                    name ? name : "unnamed", result.status, result.text);
    } else if (result.stage == ChunkResult::Stage::CallFailed) {
        crabe::shared::Logger::getInstance().error("LuaCall: lua_pcall('{}') failed (status {}): {}",
                                    name ? name : "unnamed", result.status, result.text);
    }
    return static_cast<bool>(result);
}

bool LuaCall::runGlobalIfExists(void* L, const std::string& functionName) const
{
    t_luaL_loadbuffer loadbuffer = originalLoadbuffer();
    t_lua_pcall pcall = originalPcall();

    if (!L || !loadbuffer || !pcall)
        return false;

    std::string chunk = std::format("if {0} then {0}() end", functionName);

    ChunkResult result = runChunk(L, kLuaMultret, [&]() {
        return loadbuffer(L, chunk.c_str(), chunk.size(), functionName.c_str());
    });

    if (result.stage == ChunkResult::Stage::LoadFailed) {
        crabe::shared::Logger::getInstance().error("LuaCall: failed to compile keybind chunk for '{}' (status {}).",
                                    functionName, result.status);
    }
    return static_cast<bool>(result);
}

bool LuaCall::runPatch(void* L, const std::string& patchSource, const std::string& label) const
{
    t_luaL_loadbuffer loadbuffer = originalLoadbuffer();
    t_lua_pcall pcall = originalPcall();

    if (!L || !loadbuffer || !pcall)
        return false;

    // The label becomes the chunk name, so a Lua traceback inside the patch
    // points at whatever registered it instead of a generic "patch".
    std::string chunkName = "=" + (label.empty() ? std::string("crabe_patch") : label);

    ChunkResult result = runChunk(L, kLuaMultret, [&]() {
        return loadbuffer(L, patchSource.c_str(), patchSource.size(), chunkName.c_str());
    });

    if (result.stage == ChunkResult::Stage::LoadFailed) {
        crabe::shared::Logger::getInstance().error("LuaCall: patch '{}' failed to compile (status {}): {}",
                                    label, result.status, result.text);
    } else if (result.stage == ChunkResult::Stage::CallFailed) {
        crabe::shared::Logger::getInstance().error("LuaCall: patch '{}' failed to run (status {}): {}",
                                    label, result.status, result.text);
    } else {
        crabe::shared::Logger::getInstance().info("LuaCall: patch '{}' applied ({} bytes).",
                                   label, patchSource.size());
    }
    return static_cast<bool>(result);
}

// lua_toboolean stands in for a type check: false for nil, and no lua_type is
// resolved. Indexing a nil with lua_getfield would raise a Lua error, which
// longjmps straight out of here.
bool LuaCall::callTick(void* L, double dt) const
{
    t_lua_pcall pcall = originalPcall();

    if (!L || !pcall || !_getfield || !_gettop || !_settop || !_pushnumber || !_toboolean)
        return false;

    constexpr int kLuaGlobalsIndex = -10002;
    constexpr int kLuaOk = 0;

    int savedTop = _gettop(L);

    _getfield(L, kLuaGlobalsIndex, "Game");
    if (!_toboolean(L, -1)) {
        _settop(L, savedTop);
        return false;
    }

    _getfield(L, -1, "_runTicks");
    if (!_toboolean(L, -1)) {
        _settop(L, savedTop);
        return false;
    }

    _pushnumber(L, dt);
    int status = pcall(L, 1, 0, 0);
    _settop(L, savedTop);

    return status == kLuaOk;
}

// Executes Crabe.Mod.dispatchDraw() natively without string compilation or GC overhead.
bool LuaCall::dispatchModDraw(void* L) const
{
    t_lua_pcall pcall = originalPcall();

    if (!L || !pcall || !_getfield || !_gettop || !_settop || !_toboolean)
        return false;

    constexpr int kLuaGlobalsIndex = -10002;
    constexpr int kLuaOk = 0;
    int savedTop = _gettop(L);

    _getfield(L, kLuaGlobalsIndex, "Crabe");
    if (!_toboolean(L, -1)) {
        _settop(L, savedTop);
        return false;
    }

    _getfield(L, -1, "Mod");
    if (!_toboolean(L, -1)) {
        _settop(L, savedTop);
        return false;
    }

    _getfield(L, -1, "dispatchDraw");
    if (!_toboolean(L, -1)) {
        _settop(L, savedTop);
        return false;
    }

    int status = pcall(L, 0, 0, 0);
    _settop(L, savedTop);

    return status == kLuaOk;
}

// Names what is missing rather than just refusing: each address here is
// separately resolved, so a silent false means re-deriving by hand which of
// nine things failed.
bool LuaCall::registerNativeFunction(void* L, const char* tableName, const char* fieldName,
                                      t_lua_cfunction cFunction) const
{
    struct Requirement { const char* name; const void* value; };
    const Requirement required[] = {
        { "L",                L },
        { "cFunction",        reinterpret_cast<const void*>(cFunction) },
        { "lua_getfield",     reinterpret_cast<const void*>(_getfield) },
        { "lua_toboolean",    reinterpret_cast<const void*>(_toboolean) },
        { "lua_pushlstring",  reinterpret_cast<const void*>(_pushlstring) },
        { "lua_pushcclosure", reinterpret_cast<const void*>(_pushcclosure) },
        { "lua_rawset",       reinterpret_cast<const void*>(_rawset) },
        { "lua_gettop",       reinterpret_cast<const void*>(_gettop) },
        { "lua_settop",       reinterpret_cast<const void*>(_settop) },
    };

    for (const auto& requirement : required) {
        if (requirement.value) continue;

        crabe::shared::Logger::getInstance().error("LuaCall: registerNativeFunction({}.{}): {} is unavailable.",
                                    tableName, fieldName, requirement.name);
        return false;
    }

    constexpr int kLuaGlobalsIndex = -10002;
    int savedTop = _gettop(L);

    _getfield(L, kLuaGlobalsIndex, tableName);
    if (!_toboolean(L, -1)) {
        _settop(L, savedTop);
        crabe::shared::Logger::getInstance().error("LuaCall: registerNativeFunction: global table '{}' does not exist.", tableName);
        return false;
    }

    _pushlstring(L, fieldName, std::strlen(fieldName));
    _pushcclosure(L, cFunction, 0);
    _rawset(L, -3);

    _settop(L, savedTop);
    return true;
}

const char* LuaCall::argToString(void* L, int idx) const
{
    return _tolstring ? _tolstring(L, idx, nullptr) : nullptr;
}

double LuaCall::argToNumber(void* L, int idx, double fallback) const
{
    if (!_tonumber) return fallback;

    return _tonumber(L, idx);
}

// Plain Lua truthiness: everything except false and nil is true.
//
// Deliberately not second-guessing a numeric 0 into false. Lua says 0 is true,
// the wrappers in src/api reject anything that is not a boolean, and a native
// that disagrees with its own language is a worse trap than the one this
// replaces.
bool LuaCall::argToBoolean(void* L, int idx, bool fallback) const
{
    if (!L || !_toboolean) return fallback;

    return _toboolean(L, idx) != 0;
}

void LuaCall::pushString(void* L, const std::string& value) const
{
    if (_pushlstring) _pushlstring(L, value.c_str(), value.size());
}

void LuaCall::pushNumber(void* L, double value) const
{
    if (_pushnumber) _pushnumber(L, value);
}

// Pushes a boolean value to the top of the Lua stack.
void LuaCall::pushBoolean(void* L, bool value) const
{
    if (_pushboolean) _pushboolean(L, value ? 1 : 0);
}

// Pushes a nil value to the top of the Lua stack.
void LuaCall::pushNil(void* L) const
{
    if (_pushnil) _pushnil(L);
}

/// Returns the number of elements on the Lua stack.
int LuaCall::getTop(void* L) const
{
    return _gettop ? _gettop(L) : 0;
}

/// Checks whether the stack value at the given index is a number.
bool LuaCall::isNumber(void* L, int idx) const
{
    return _isnumber ? (_isnumber(L, idx) != 0) : false;
}

bool LuaCall::hasReturnSupport() const
{
    return _pushlstring != nullptr && _pushnumber != nullptr;
}

// No Lua-side pcall wrapper: lua_pcall already traps runtime errors, and the
// error object it leaves on the stack is readable directly via popString.
bool LuaCall::runSnippet(void* L, const std::string& code, std::string& out) const
{
    out.clear();

    t_luaL_loadbuffer loadbuffer = originalLoadbuffer();
    t_lua_pcall pcall = originalPcall();

    if (!L || !loadbuffer || !pcall) {
        out = "Lua state or hooks unavailable.";
        return false;
    }

    ChunkResult result = runChunk(L, 1, [&]() {
        return loadbuffer(L, code.c_str(), code.size(), "=console");
    });

    out = std::move(result.text);
    if (result)
        return true;

    if (out.empty()) {
        out = result.stage == ChunkResult::Stage::LoadFailed
            ? std::format("compile failed (status {}).", result.status)
            : std::format("runtime error (status {}).", result.status);
    }
    return false;
}

bool LuaCall::runSnippet(void* L, const std::string& code) const
{
    std::string ignored;
    return runSnippet(L, code, ignored);
}

// The injection-safe counterpart of runSnippet: the chunk is fixed at compile
// time and every piece of caller data travels as a Lua string argument, so it
// can never be parsed as code. runChunk cannot serve here -- it hardcodes
// pcall(L, 0, ...), i.e. zero arguments -- hence the duplicated but necessary
// load/push/call/restore sequence below.
bool LuaCall::runChunkWithArgs(void* L,
                               std::string_view chunk,
                               std::span<const std::string> args,
                               std::string& outError) const
{
    constexpr int kLuaOk = 0;

    outError.clear();

    t_luaL_loadbuffer loadbuffer = originalLoadbuffer();
    t_lua_pcall pcall = originalPcall();

    if (!L || !loadbuffer || !pcall) {
        outError = "Lua state or hooks unavailable (luaL_loadbuffer/lua_pcall unresolved).";
        return false;
    }
    if (!args.empty() && !_pushlstring) {
        outError = std::format("lua_pushlstring is unavailable; cannot pass {} chunk argument(s).",
                               args.size());
        return false;
    }

    int savedTop = _gettop ? _gettop(L) : 0;
    auto restoreTop = [&]() {
        if (_gettop && _settop) _settop(L, savedTop);
    };

    int status = loadbuffer(L, chunk.data(), chunk.size(), "=crabe_chunk");
    if (status != kLuaOk) {
        std::string text = popString(L, -1);
        restoreTop();
        outError = text.empty()
            ? std::format("chunk failed to compile (status {}); no Lua error message available.", status)
            : std::format("chunk failed to compile (status {}): {}", status, text);
        return false;
    }

    for (const std::string& arg : args)
        _pushlstring(L, arg.c_str(), arg.size());

    status = pcall(L, static_cast<int>(args.size()), 0, 0);
    if (status == kLuaOk) {
        restoreTop();
        return true;
    }

    // lua_pcall leaves exactly one error object behind on failure, but only
    // read it if the stack really grew: popString rewrites the slot it reads,
    // and a slot below savedTop belongs to the game.
    std::string text;
    if (!_gettop || _gettop(L) > savedTop)
        text = popString(L, -1);
    restoreTop();

    outError = text.empty()
        ? std::format("chunk failed to run (status {}); no Lua error message available.", status)
        : std::format("chunk failed to run (status {}): {}", status, text);
    return false;
}

LuaCall::ChunkResult LuaCall::runChunk(void* L, int nresults,
                                       const std::function<int()>& load) const
{
    constexpr int kLuaOk = 0;

    ChunkResult result;
    t_lua_pcall pcall = originalPcall();

    if (!L || !pcall) {
        result.stage = ChunkResult::Stage::LoadFailed;
        result.text = "Lua state or hooks unavailable.";
        return result;
    }

    int savedTop = _gettop ? _gettop(L) : 0;
    auto restoreTop = [&]() {
        if (_gettop && _settop) _settop(L, savedTop);
    };

    result.status = load();
    if (result.status != kLuaOk) {
        result.stage = ChunkResult::Stage::LoadFailed;
        result.text = popString(L, -1);
        restoreTop();
        return result;
    }

    result.status = pcall(L, 0, nresults, 0);

    // Only read a value that is actually there: with LUA_MULTRET a chunk may
    // return nothing, and popString would then read past the top and pop a
    // slot that belongs to the caller.
    if (!_gettop || _gettop(L) > savedTop)
        result.text = popString(L, -1);

    restoreTop();

    if (result.status != kLuaOk)
        result.stage = ChunkResult::Stage::CallFailed;
    return result;
}

} // namespace crabe::infrastructure


/*
** CrabeLoader
** File description:
** Calls the RealmManager_LoadSkyDomeInZone script native with a one-argument script frame.
** The handler reads nothing from its frame but that argument, so the frame holds only it.
** The handler and the argument reader are both checked byte by byte before the first call.
**
** Authors: @LucasLhomme
*/

#include "infrastructure/engine_sky.hpp"

#include <cstddef>
#include <cstring>
#include <windows.h>

#include "infrastructure/crash_handler.hpp"
#include "infrastructure/memory.hpp"
#include "infrastructure/script_natives.hpp"
#include "shared/logger.hpp"

namespace {

    using crabe::infrastructure::script_natives::bytesAt;

    // RealmManager_LoadSkyDomeInZone, measured on di3-gold-steam-1.0 (rva 0x6934C0), __cdecl
    // with the script frame as its only argument:
    //   +0x18  mov ecx, [esp+0x1C]        -- the frame
    //   +0x1E  call ScriptFrame::PopString
    //   +0x23  mov ecx, [g_realmManager]  -- then clears the skies and loads the named realm
    constexpr const char* kNative = "RealmManager_LoadSkyDomeInZone";
    constexpr const char* kSymbol = "Script_RealmManager_LoadSkyDomeInZone";
    constexpr std::uintptr_t kFrameLoadOffset = 0x18;
    constexpr std::uintptr_t kPopCallOffset = 0x1E;
    constexpr std::uintptr_t kManagerLoadOffset = 0x23;

    // ScriptFrame::PopString (rva 0x332140) reads the frame this way:
    //   mov edx, [ecx+0x20]; mov eax, [edx]; dec eax; mov [edx], eax   -- *frame->top -= 1
    //   mov ecx, [ecx+0x0C]; mov edx, [ecx]; mov ecx, [edx]            -- ***frame->values
    //   cmp dword ptr [ecx+eax*8], 2                                   -- 8-byte slot, 2 = string
    constexpr std::uint8_t kPopStringPrologue[] = {
        0x8B, 0x51, 0x20, 0x8B, 0x02, 0x48, 0x89, 0x02, 0x8B, 0x49, 0x0C,
        0x8B, 0x11, 0x8B, 0x0A, 0x83, 0x3C, 0xC1, 0x02,
    };
    constexpr std::uint32_t kStringType = 2;

    struct ScriptValue {
        std::uint32_t type;
        const char* text;
    };

    // Only the two fields PopString reads; the rest of a real frame is never touched.
    struct ScriptFrame {
        std::uint8_t unused0[0x0C];
        ScriptValue*** values;
        std::uint8_t unused1[0x20 - 0x10];
        std::int32_t* top;
    };
    static_assert(offsetof(ScriptFrame, values) == 0x0C && offsetof(ScriptFrame, top) == 0x20);

    using HandlerFn = void(__cdecl*)(ScriptFrame* frame);

} // namespace

namespace crabe::infrastructure {

EngineSky& EngineSky::get()
{
    static EngineSky instance;
    return instance;
}

void EngineSky::resolveOnce()
{
    if (_attempted)
        return;
    _attempted = true;

    auto& logger = crabe::shared::Logger::getInstance();
    const auto base = reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
    const std::uintptr_t handler = script_natives::find(kNative);
    if (!handler || !script_natives::matchesProfile(base, kSymbol, handler)) {
        logger.warning("EngineSky: {} not found where expected (0x{:X}); skies cannot be loaded.", kNative, handler);
        return;
    }

    const std::uintptr_t popString = crabe::memory::resolveCall(handler + kPopCallOffset);
    if (!bytesAt(handler + kFrameLoadOffset, { 0x8B, 0x4C, 0x24, 0x1C })
        || !bytesAt(handler + kManagerLoadOffset, { 0x8B, 0x0D })
        || !popString
        || !crabe::memory::isReadable(popString, sizeof(kPopStringPrologue))
        || std::memcmp(reinterpret_cast<const void*>(popString), kPopStringPrologue, sizeof(kPopStringPrologue)) != 0) {
        logger.warning("EngineSky: {} at rva 0x{:X} does not have the expected shape; refusing.", kNative, handler - base);
        return;
    }

    _handler = handler;
    logger.info("EngineSky: resolved {} (rva 0x{:X}).", kNative, handler - base);
}

bool EngineSky::loadSkyDome(const std::string& realmName)
{
    resolveOnce();
    if (!_handler || realmName.empty())
        return false;

    ScriptValue argument{ kStringType, realmName.c_str() };
    ScriptValue* slots = &argument;
    ScriptValue** slotsRef = &slots;
    std::int32_t top = 1;
    ScriptFrame frame{};
    frame.values = &slotsRef;
    frame.top = &top;

    const auto handler = reinterpret_cast<HandlerFn>(_handler);
    const bool completed = CrashHandler::runGuarded([&] { handler(&frame); }, "EngineSky::loadSkyDome");
    crabe::shared::Logger::getInstance().info("EngineSky: sky '{}' {}.", realmName, completed ? "loaded" : "faulted");
    return completed;
}

} // namespace crabe::infrastructure

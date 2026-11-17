/*
** CrabeLoader
** File description:
** Calls Script VM natives with a private frame: value slots at +0x0C, stack top at +0x20.
** Arguments go in declaration order (natives pop right to left); results come back in push order.
** Natives that read other frame members (the calling script's owner) fault safely instead.
**
** Authors: @LucasLhomme
*/

#include "infrastructure/engine_script_call.hpp"

#include <array>
#include <cstddef>

#include "infrastructure/crash_handler.hpp"
#include "infrastructure/script_natives.hpp"
#include "shared/logger.hpp"

namespace {

    constexpr std::size_t kCapacity = 64;

    struct Slot {
        std::uint32_t type;
        std::uint32_t bits;
    };

    struct ScriptFrame {
        std::uint8_t unused0[0x0C];
        Slot*** values;
        std::uint8_t unused1[0x20 - 0x10];
        std::int32_t* top;
    };
    static_assert(offsetof(ScriptFrame, values) == 0x0C && offsetof(ScriptFrame, top) == 0x20);

    using HandlerFn = void(__cdecl*)(ScriptFrame* frame);

} // namespace

namespace crabe::infrastructure {

EngineScriptCall& EngineScriptCall::get()
{
    static EngineScriptCall instance;
    return instance;
}

std::uintptr_t EngineScriptCall::resolve(const std::string& name)
{
    std::lock_guard lock(_mutex);
    const auto it = _handlers.find(name);
    if (it != _handlers.end())
        return it->second;
    const std::uintptr_t handler = script_natives::find(name.c_str());
    _handlers.emplace(name, handler);
    if (!handler)
        crabe::shared::Logger::getInstance().warning("EngineScriptCall: no Script VM native named '{}'.", name);
    return handler;
}

bool EngineScriptCall::call(const std::string& name, const std::vector<Value>& args, std::vector<Value>& results)
{
    results.clear();
    if (args.size() > kCapacity)
        return false;
    const std::uintptr_t handler = resolve(name);
    if (!handler)
        return false;

    std::array<Slot, kCapacity> slots{};
    for (std::size_t i = 0; i < args.size(); ++i)
        slots[i] = { static_cast<std::uint32_t>(args[i].type), args[i].bits };
    Slot* slotsPtr = slots.data();
    Slot** slotsRef = &slotsPtr;
    std::int32_t top = static_cast<std::int32_t>(args.size());
    ScriptFrame frame{};
    frame.values = &slotsRef;
    frame.top = &top;

    const auto fn = reinterpret_cast<HandlerFn>(handler);
    const bool completed = CrashHandler::runGuarded([&] { fn(&frame); }, "EngineScriptCall::call");
    if (!completed || top < 0 || static_cast<std::size_t>(top) > kCapacity) {
        crabe::shared::Logger::getInstance().warning("EngineScriptCall: native '{}' {}.", name,
                                                     completed ? "left the frame unbalanced" : "faulted");
        return false;
    }
    for (std::int32_t i = 0; i < top; ++i)
        results.push_back({ static_cast<Type>(slots[static_cast<std::size_t>(i)].type), slots[static_cast<std::size_t>(i)].bits });
    return true;
}

} // namespace crabe::infrastructure

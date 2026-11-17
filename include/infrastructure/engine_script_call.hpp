/*
** CrabeLoader
** File description:
** Declares the bridge that calls any registered Script VM native from Lua through a private frame.
** The frame carries only the value slots and the stack top every Push and Pop reads.
** Knows no gameplay: Lua picks the native and its arguments, the bridge only marshals them.
**
** Authors: @LucasLhomme
*/

#ifndef ENGINE_SCRIPT_CALL_HPP_
#define ENGINE_SCRIPT_CALL_HPP_

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace crabe::infrastructure {

class EngineScriptCall final {
public:
    enum class Type : std::uint32_t {
        Int = 0,
        Float = 1,
        String = 2,
        Handle = 3,
    };

    struct Value {
        Type type{Type::Int};
        std::uint32_t bits{0};
    };

    static EngineScriptCall& get();

    // Calls the native registered under name with args pushed in order, and fills results in push order.
    // Returns false when no native has that name, the call faulted or the frame overflowed. Script thread only.
    [[nodiscard]] bool call(const std::string& name, const std::vector<Value>& args, std::vector<Value>& results);

private:
    EngineScriptCall() = default;

    std::uintptr_t resolve(const std::string& name);

    std::mutex _mutex;
    std::unordered_map<std::string, std::uintptr_t> _handlers;
};

} // namespace crabe::infrastructure

#endif

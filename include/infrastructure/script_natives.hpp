/*
** CrabeLoader
** File description:
** Finds the handlers of the engine's Script VM natives, the second VM the Lua state never sees.
** They register as `push handler; push "Name"; call RegisterFunction`, so a name leads to its
** handler, whose body the callers then check byte by byte before trusting any offset in it.
**
** Authors: @LucasLhomme
*/

#ifndef SCRIPT_NATIVES_HPP_
#define SCRIPT_NATIVES_HPP_

#include <cstdint>
#include <initializer_list>

namespace crabe::infrastructure::script_natives {

/// The handler registered under `name`: the `push imm32` right before `push "name"`. 0 when absent.
[[nodiscard]] std::uintptr_t find(const char* name);

/// Whether the bytes at `address` are readable and equal `expected`.
[[nodiscard]] bool bytesAt(std::uintptr_t address, std::initializer_list<std::uint8_t> expected);

/// Whether `address` is where the active game profile measured `symbol`. True without a
/// profile (an unknown build is checked by its bytes alone), false for an unmeasured symbol.
[[nodiscard]] bool matchesProfile(std::uintptr_t base, const char* symbol, std::uintptr_t address);

template <typename T>
[[nodiscard]] T readAt(std::uintptr_t address)
{
    return *reinterpret_cast<const T*>(address);
}

} // namespace crabe::infrastructure::script_natives

#endif

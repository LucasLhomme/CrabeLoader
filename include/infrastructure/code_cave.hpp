/*
** CrabeLoader
** File description:
** Declares a code cave: stolen bytes relocated so a detour can sit where MinHook cannot.
** A relative jmp costs 5 bytes, so a site must yield at least that much to be usable.
** Not a MinHook detour and not in the inventory -- see infrastructure/hook_registry.hpp.
**
** Authors: @LucasLhomme
*/

#ifndef CODECAVE_HPP_
#define CODECAVE_HPP_

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace crabe::infrastructure {

inline constexpr size_t kJmpLength = 5;
inline constexpr size_t kMaxStolen = 32;

// Measures the number of bytes that must be stolen to accommodate a relative jump.
size_t measureStolenLength(const uint8_t* code, size_t* failedAtOffset = nullptr);

// Returns whether the stolen byte length is within valid bounds.
constexpr bool isStolenLengthAcceptable(size_t stolenLength)
{
    return stolenLength >= kJmpLength && stolenLength <= kMaxStolen;
}

// Returns a shared zeroed memory block identified by name.
uintptr_t acquireSharedBlock(std::string_view name, size_t size);

class CodeCave {
    public:
        CodeCave() = default;
        ~CodeCave();

        CodeCave(const CodeCave&) = delete;
        CodeCave& operator=(const CodeCave&) = delete;

        // Installs a code cave at the given site, preserving stolen instructions.
        bool install(uintptr_t site, const std::vector<uint8_t>& body, size_t stolenLength = 0);

        // Installs a code cave and logs the result.
        bool installLogged(uintptr_t site, const std::vector<uint8_t>& body, size_t stolenLength = 0,
                           const char* owner = nullptr, const char* name = nullptr);

        // Restores the original instructions at the patch site.
        void remove();

        bool isInstalled() const;
        uintptr_t caveAddress() const;

        // Writes a 32-bit value in little-endian order at the specified offset.
        static void putU32(std::vector<uint8_t>& body, size_t at, uint32_t value);

        // Writes bytes to memory under temporary write protection and flushes instruction cache.
        static bool patchBytes(uintptr_t address, const void* bytes, size_t count);

    private:
        uintptr_t _site = 0;
        uint8_t* _cave = nullptr;
        std::vector<uint8_t> _stolen;
        bool _installed = false;
};

} // namespace crabe::infrastructure

#endif /* !CODECAVE_HPP_ */

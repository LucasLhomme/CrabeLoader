/*
** CrabeLoader
** File description:
** Declares CodeCave, a manual x86 code cave for mid-function sites: body, stolen bytes, jump back.
** Stolen bytes are copied verbatim, never relocated, where MinHook's trampoline would be corrupt.
** Not for function entries (use Hook); remove() deliberately leaks the cave page.
**
** Authors: @LucasLhomme
*/

#ifndef CODECAVE_HPP_
#define CODECAVE_HPP_

#include <cstddef>
#include <cstdint>
#include <vector>

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

#endif /* !CODECAVE_HPP_ */

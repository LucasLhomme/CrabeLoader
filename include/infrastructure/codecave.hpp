/*
** CrabeLoader
** File description:
** codecave
*/

#ifndef CODECAVE_HPP_
#define CODECAVE_HPP_

#include <cstddef>
#include <cstdint>
#include <vector>

// A manual x86 code cave: `body` is executed, then the bytes stolen from the
// site, then control jumps back just past them.
//
// Deliberately not Hook/MinHook. MinHook's length disassembler mis-relocates
// the SSE `movss` instructions at the game's damage and velocity sites and
// builds a corrupt trampoline. A cave copies the stolen bytes verbatim, so
// nothing is ever relocated -- which is exactly why these sites need one.
//
// Use Hook for anything that is a real function entry; use CodeCave only for
// mid-function instruction sites.
class CodeCave {
    public:
        CodeCave() = default;
        ~CodeCave();

        CodeCave(const CodeCave&) = delete;
        CodeCave& operator=(const CodeCave&) = delete;

        // Steals `stolenLength` bytes at `site` (>= 5, and a whole number of
        // instructions -- a partial one leaves garbage in the cave), allocates
        // body + stolen + jmp-back, and patches the site with a jmp to it,
        // NOP-padded. `site` must be the exact start of an instruction.
        bool install(uintptr_t site, const std::vector<uint8_t>& body, size_t stolenLength);

        // install() with the outcome logged as "<owner>: <name> ...". A site
        // of 0 means "not found" and is skipped rather than patched: a wrong
        // address overwrites live code and crashes the host process.
        bool installLogged(uintptr_t site, const std::vector<uint8_t>& body, size_t stolenLength,
                           const char* owner, const char* name);

        // Restores the stolen bytes. The cave page itself is intentionally
        // leaked: another thread may be executing inside it right now, and
        // there is no way to know, so freeing it would be a race.
        void remove();

        bool isInstalled() const;
        uintptr_t caveAddress() const;

        // Writes `value` little-endian at `at` in `body` -- how a static's
        // address is patched into an assembled instruction's operand.
        static void putU32(std::vector<uint8_t>& body, size_t at, uint32_t value);

        // memcpy through a temporary PAGE_EXECUTE_READWRITE, then flushes the
        // instruction cache. False if the page could not be unprotected.
        static bool patchBytes(uintptr_t address, const void* bytes, size_t count);

    private:
        uintptr_t _site = 0;
        uint8_t* _cave = nullptr;
        std::vector<uint8_t> _stolen;
        bool _installed = false;
};

#endif /* !CODECAVE_HPP_ */

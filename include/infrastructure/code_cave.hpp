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

namespace crabe::infrastructure {

// A relative jmp is 5 bytes, so a site has to give up at least that much.
inline constexpr size_t kJmpLength = 5;

// Nothing here needs a big cave; refuse anything that looks like a mistake
// rather than allocating it.
inline constexpr size_t kMaxStolen = 32;

// How many bytes must be stolen at `code` to make room for a 5-byte relative
// jmp without splitting an instruction: decodes forward with HDE32 until at
// least kJmpLength bytes are covered, and returns that whole-instruction
// total. 0 means HDE32 could not decode an instruction, and `failedAtOffset`
// (when given) receives the offset it gave up at.
//
// The pure measurement half of CodeCave::install: no executable allocation,
// no live process, so it can be exercised against a plain byte array. It
// reads past `code + kJmpLength` by design -- the instruction straddling that
// boundary has to be decoded in full -- so callers must keep up to 15 bytes
// beyond the fifth readable.
//
// The result is not validated against kMaxStolen; that is the caller's call,
// see isStolenLengthAcceptable.
size_t measureStolenLength(const uint8_t* code, size_t* failedAtOffset = nullptr);

// Whether install() will accept `stolenLength`: enough room for the jmp, and
// not so much that the length looks like a mistake.
constexpr bool isStolenLengthAcceptable(size_t stolenLength)
{
    return stolenLength >= kJmpLength && stolenLength <= kMaxStolen;
}

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
        // NOP-padded. If stolenLength == 0, length is computed dynamically via HDE32.
        bool install(uintptr_t site, const std::vector<uint8_t>& body, size_t stolenLength = 0);

        // install() with the outcome logged as "<owner>: <name> ...". A site
        // of 0 means "not found" and is skipped rather than patched: a wrong
        // address overwrites live code and crashes the host process.
        bool installLogged(uintptr_t site, const std::vector<uint8_t>& body, size_t stolenLength = 0,
                           const char* owner = nullptr, const char* name = nullptr);

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

} // namespace crabe::infrastructure

#endif /* !CODECAVE_HPP_ */

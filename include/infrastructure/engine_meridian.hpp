/*
** CrabeLoader
** File description:
** Guards Meridian graph array operations against corrupted or uninitialized heap pointers.
** Prevents fatal EXCEPTION_ACCESS_VIOLATION when loading node graphs from console updates.
** Does not modify graph topology: only resets unreadable buffer pointers before append.
**
** Authors: @LucasLhomme
*/

#ifndef CRABELOADER_INFRASTRUCTURE_ENGINE_MERIDIAN_HPP_
#define CRABELOADER_INFRASTRUCTURE_ENGINE_MERIDIAN_HPP_

#include <atomic>
#include <cstdint>

#include "infrastructure/hook.hpp"

namespace crabe::infrastructure {

/**
 * @brief Manages the Meridian node graph array hook in the host engine.
 *
 * Visual scripting and game logic graphs (.oct / .m2g) from console updates (PS4 1.06)
 * can contain adjacency link data with uninitialized padding or mismatched struct alignments.
 * When the engine builds node connections via DynArray16::Append (RVA 0x002A9E90), an uninitialized
 * pointer field causes the engine to dereference invalid memory (EXCEPTION_ACCESS_VIOLATION).
 *
 * EngineMeridian intercepts DynArray16::Append, validates that the underlying buffer pointer
 * and capacity/count header are readable and writable, and if corrupted, resets the pointer to
 * nullptr so the engine safely allocates a fresh buffer on the Win32 heap instead of crashing.
 */
class EngineMeridian final {
public:
    static EngineMeridian& get();

    /// Resolves the DynArray16::Append entry point and installs the hook.
    void initialize();

    /// Uninstalls the hook.
    void uninitialize();

    /// Whether the hook is actively installed.
    [[nodiscard]] bool isHooked() const noexcept;

    /// Number of corrupted array pointers that were sanitized.
    [[nodiscard]] std::uint32_t sanitizedCount() const noexcept;

private:
    EngineMeridian() = default;
    ~EngineMeridian() = default;

    EngineMeridian(const EngineMeridian&) = delete;
    EngineMeridian& operator=(const EngineMeridian&) = delete;
    EngineMeridian(EngineMeridian&&) = delete;
    EngineMeridian& operator=(EngineMeridian&&) = delete;

    bool resolve();

    static void __fastcall hkDynArrayAppend(void* thisPtr, void* edxDummy, const std::uint16_t* pVal);

    bool _attempted{false};
    std::uintptr_t _appendTarget{0};
    Hook _appendHook;
    static inline std::atomic<std::uint32_t> s_sanitizedCount{0};
};

} // namespace crabe::infrastructure

#endif // CRABELOADER_INFRASTRUCTURE_ENGINE_MERIDIAN_HPP_

/*
** CrabeLoader
** File description:
** Intercepts the engine's shader CRC lookup to provide safe fallback for missing shaders.
** Prevents fatal EXCEPTION_BREAKPOINT asserts when loading materials from console updates.
** Does not generate shaders: supplies a default zero-parameter shader for unknown CRCs.
**
** Authors: @LucasLhomme
*/

#ifndef CRABELOADER_INFRASTRUCTURE_ENGINE_SHADERS_HPP_
#define CRABELOADER_INFRASTRUCTURE_ENGINE_SHADERS_HPP_

#include <atomic>
#include <cstdint>

#include "infrastructure/hook.hpp"

namespace crabe::infrastructure {

#pragma pack(push, 4)
struct ShaderEntry {
    std::uint32_t crc{0};
    void* shader{nullptr};
};
#pragma pack(pop)

class EngineShaders final {
public:
    static EngineShaders& get();

    // Resolves the shader lookup entry point and installs the hook.
    void initialize();

    // Uninstalls the hook.
    void uninitialize();

    // Whether the hook is actively installed.
    [[nodiscard]] bool isHooked() const noexcept;

    // Number of missing shader lookups that were caught and substituted with a fallback.
    [[nodiscard]] std::uint32_t fallbackCount() const noexcept;

private:
    EngineShaders() = default;
    ~EngineShaders() = default;

    EngineShaders(const EngineShaders&) = delete;
    EngineShaders& operator=(const EngineShaders&) = delete;
    EngineShaders(EngineShaders&&) = delete;
    EngineShaders& operator=(EngineShaders&&) = delete;

    bool resolve();

    static void* __cdecl hkLookupShaderCrc(std::uint32_t targetCrc,
                                           const ShaderEntry* array,
                                           std::uint32_t count);

    static void __cdecl hkParseBindings(void* descriptor, void* stream, void** table);

    static void* findFallbackShader(const ShaderEntry* array, std::uint32_t count) noexcept;

    bool _attempted{false};
    std::uintptr_t _lookupTarget{0};
    std::uintptr_t _parseBindingsTarget{0};
    Hook _lookupHook;
    Hook _parseBindingsHook;
    static inline std::atomic<std::uint32_t> s_fallbackCount{0};
};

} // namespace crabe::infrastructure

#endif // CRABELOADER_INFRASTRUCTURE_ENGINE_SHADERS_HPP_

/*
** CrabeLoader
** File description:
** Serves loose mod textures to the engine's own file loader, below the Win32 file calls.
** Substitutes a loose .tbody for the archive member the engine would otherwise read.
** Doubles as a measurement probe while the marker file crabe_probe_asset_loader.txt exists.
**
** Authors: @LucasLhomme
*/

#ifndef CRABELOADER_INFRASTRUCTURE_ENGINE_ASSET_LOADER_HPP_
#define CRABELOADER_INFRASTRUCTURE_ENGINE_ASSET_LOADER_HPP_

#include <atomic>
#include <cstdint>

#include "infrastructure/hook.hpp"

namespace crabe::infrastructure {

class EngineAssetLoader final {
public:
    static EngineAssetLoader& get();

    // Installs the loader hook unless the profile lacks its symbols or the override is disabled.
    void initialize();

    // Removes the hook and logs the totals it collected.
    void uninitialize();

    // Whether the loader hook is installed.
    [[nodiscard]] bool isHooked() const noexcept;

    // Number of requests answered with a loose mod file instead of the archive member.
    [[nodiscard]] std::uint32_t servedCount() const noexcept;

private:
    EngineAssetLoader() = default;
    ~EngineAssetLoader() = default;

    EngineAssetLoader(const EngineAssetLoader&) = delete;
    EngineAssetLoader& operator=(const EngineAssetLoader&) = delete;
    EngineAssetLoader(EngineAssetLoader&&) = delete;
    EngineAssetLoader& operator=(EngineAssetLoader&&) = delete;

    static void* __cdecl hkLoadFile(const char* path, std::uint32_t* sizeOut,
                                    std::uintptr_t flag0, std::uintptr_t flag1,
                                    std::uintptr_t flag2, std::uintptr_t flag3);

    void* serveOverride(const char* path, std::uint32_t* sizeOut) noexcept;

    void record(const char* path, const std::uint32_t* sizeOut, const void* result, bool overridden,
                std::uintptr_t flag0, std::uintptr_t flag1,
                std::uintptr_t flag2, std::uintptr_t flag3) noexcept;

    Hook _loadFileHook;
    std::uintptr_t _allocator{0};
    std::atomic<bool> _overrideEnabled{false};
    std::atomic<bool> _probing{false};
    std::atomic<std::uint32_t> _totalCalls{0};
    std::atomic<std::uint32_t> _textureCalls{0};
    std::atomic<std::uint32_t> _textureMisses{0};
    std::atomic<std::uint32_t> _served{0};
    std::atomic<std::uint32_t> _serveFailures{0};
    std::atomic<std::uint32_t> _textureLogged{0};
    std::atomic<std::uint32_t> _otherLogged{0};
    std::atomic<std::uint32_t> _servedLogged{0};
};

} // namespace crabe::infrastructure

#endif // CRABELOADER_INFRASTRUCTURE_ENGINE_ASSET_LOADER_HPP_

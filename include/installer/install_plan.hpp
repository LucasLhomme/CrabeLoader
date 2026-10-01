/*
** CrabeLoader
** File description:
** Declares what the installer may do to a game folder, decided from three facts about it.
** This is the rule that keeps the real bink2w32.dll safe: it is only ever renamed once identified.
** Touches no file; game_installer.hpp gathers the facts and carries the decision out.
**
** Authors: @LucasLhomme
*/

#ifndef CRABELOADER_INSTALLER_INSTALL_PLAN_HPP_
#define CRABELOADER_INSTALLER_INSTALL_PLAN_HPP_

#include <array>
#include <cstdint>
#include <string_view>

namespace crabe::installer {

    constexpr std::string_view kGameExecutable = "DisneyInfinity3.exe";
    constexpr std::string_view kProxyName = "bink2w32.dll";
    constexpr std::string_view kBackupName = "bink2w32_orig.dll";
    constexpr std::string_view kStagingName = "bink2w32.dll.crabe-new";
    constexpr std::string_view kModsFolderName = "mods";
    constexpr std::string_view kReadmeName = "LISEZMOI.txt";

    // SHA-256 (lowercase hex) of the Bink DLL shipped with the Steam build of the game.
    constexpr std::array<std::string_view, 1> kKnownOriginalSha256 = {
        "263b75c84dea32bdb844b82d193ebaf8698b7e2f3facd5121a2ddb5130a98772",
    };

    enum class ProxySlot : std::uint8_t {
        Missing,
        KnownOriginal,
        Other,
    };

    struct FolderFacts {
        bool hasGameExecutable{false};
        ProxySlot proxy{ProxySlot::Missing};
        bool hasBackup{false};
    };

    enum class Plan : std::uint8_t {
        FreshInstall,
        Update,
        NotAGameFolder,
        ProxyMissing,
        ProxyUnrecognized,
    };

    [[nodiscard]] std::string_view describe(Plan plan) noexcept;

    // A backup already beside the proxy means a real original is safe, so the proxy is replaced
    // without a question. Without one, the proxy is renamed only if it is a known original.
    [[nodiscard]] Plan planInstall(const FolderFacts& facts) noexcept;

} // namespace crabe::installer

#endif /* !CRABELOADER_INSTALLER_INSTALL_PLAN_HPP_ */

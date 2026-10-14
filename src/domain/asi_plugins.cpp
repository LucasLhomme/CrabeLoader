/*
** CrabeLoader
** File description:
** Implements .asi discovery: the folders Ultimate ASI Loader reads, then every enabled mod.
** A mod's id and profile check follow ModManager, so a disabled mod loads no native code.
** Touches the disk only to list folders and read mod.json; never loads a file.
**
** Authors: @LucasLhomme
*/

#include "domain/asi_plugins.hpp"

#include <algorithm>
#include <cctype>
#include <format>
#include <string_view>
#include <system_error>

#include "domain/mod_id.hpp"
#include "domain/mod_manifest.hpp"

namespace crabe::domain {

    namespace {

        namespace fs = std::filesystem;

        constexpr std::uint16_t kMachineI386 = 0x014C;
        constexpr std::uint16_t kFileIsDll = 0x2000;
        constexpr std::size_t kPeOffsetField = 0x3C;
        constexpr std::size_t kPeHeaderBytes = 24;

        /// Lowercases ASCII letters only, which is all a file-name comparison here needs.
        std::string lowercase(std::string_view text)
        {
            std::string out(text);
            std::ranges::transform(out, out.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return out;
        }

        /// Returns a path component as UTF-8 text, which never throws whatever the file name holds.
        std::string utf8(const fs::path& path)
        {
            const std::u8string text = path.u8string();
            return std::string(text.begin(), text.end());
        }

        /// Lists the .asi files directly inside a folder, sorted by name without regard to case.
        std::vector<fs::path> asiFilesIn(const fs::path& folder)
        {
            std::vector<fs::path> files;
            std::error_code error;
            for (fs::directory_iterator it(folder, error), end; !error && it != end; it.increment(error)) {
                std::error_code typeError;
                if (it->is_regular_file(typeError) && lowercase(utf8(it->path().extension())) == ".asi")
                    files.push_back(it->path());
            }
            std::ranges::sort(files, {}, [](const fs::path& file) { return lowercase(utf8(file.filename())); });
            return files;
        }

        /// Lists the mod folders ModManager would consider, sorted by name without regard to case.
        std::vector<fs::path> modFoldersIn(const fs::path& modsFolder)
        {
            std::vector<fs::path> folders;
            std::error_code error;
            for (fs::directory_iterator it(modsFolder, error), end; !error && it != end; it.increment(error)) {
                std::error_code typeError;
                const std::string name = utf8(it->path().filename());
                if (it->is_directory(typeError) && !name.empty() && name[0] != '.' && name[0] != '_')
                    folders.push_back(it->path());
            }
            std::ranges::sort(folders, {}, [](const fs::path& folder) { return lowercase(utf8(folder.filename())); });
            return folders;
        }

        /// Whether [plugins].skip names this file, compared on the file name alone and without regard to case.
        bool isListedInSkip(const Config& config, const fs::path& file)
        {
            const std::string name = lowercase(utf8(file.filename()));
            return std::ranges::any_of(config.asiSkipList(),
                                       [&](const std::string& entry) { return lowercase(entry) == name; });
        }

        /// Returns the id ModManager filters a mod folder by: the one mod.json declares, else the synthesised one.
        std::string modIdOf(const fs::path& modFolder)
        {
            const ModManifest manifest(modFolder / "mod.json");
            if (manifest.isValid() && !manifest.getId().empty())
                return manifest.getId();
            return synthesiseModId(utf8(modFolder.filename()));
        }

        /// Reads a little-endian 16-bit field.
        std::uint16_t readU16(std::span<const std::byte> bytes, std::size_t at) noexcept
        {
            return static_cast<std::uint16_t>(std::to_integer<unsigned>(bytes[at]) |
                                              (std::to_integer<unsigned>(bytes[at + 1]) << 8));
        }

        /// Reads a little-endian 32-bit field.
        std::uint32_t readU32(std::span<const std::byte> bytes, std::size_t at) noexcept
        {
            return static_cast<std::uint32_t>(readU16(bytes, at)) |
                   (static_cast<std::uint32_t>(readU16(bytes, at + 2)) << 16);
        }

    } // namespace

    std::vector<AsiCandidate> discoverAsiPlugins(const fs::path& gameRoot, const Config& config)
    {
        std::vector<AsiCandidate> candidates;
        const auto addFolder = [&](const fs::path& folder, const std::string& source, const std::string& folderReason) {
            for (fs::path& file : asiFilesIn(folder)) {
                AsiCandidate candidate{std::move(file), source, {}};
                if (!config.asiEnabled())
                    candidate.skipReason = "ASI loading is off ([plugins].asi = false in crabe.toml)";
                else if (isListedInSkip(config, candidate.path))
                    candidate.skipReason = "listed in [plugins].skip in crabe.toml";
                else
                    candidate.skipReason = folderReason;
                candidates.push_back(std::move(candidate));
            }
        };

        addFolder(gameRoot, "game folder", {});
        addFolder(gameRoot / "scripts", "scripts", {});
        addFolder(gameRoot / "plugins", "plugins", {});
        for (const fs::path& modFolder : modFoldersIn(gameRoot / "mods")) {
            const std::string id = modIdOf(modFolder);
            const std::string reason = config.isModEnabled(id)
                ? std::string{}
                : std::format("mod {} is disabled by profile '{}'", id, config.activeProfileName());
            addFolder(modFolder, std::format("mod {}", id), reason);
        }
        return candidates;
    }

    PluginBinary classifyPluginBinary(std::span<const std::byte> header) noexcept
    {
        if (header.size() < kPeOffsetField + 4 || header[0] != std::byte{'M'} || header[1] != std::byte{'Z'})
            return PluginBinary::NotPortableExecutable;
        const std::uint32_t peOffset = readU32(header, kPeOffsetField);
        if (peOffset > header.size() || header.size() - peOffset < kPeHeaderBytes)
            return PluginBinary::NotPortableExecutable;
        if (header[peOffset] != std::byte{'P'} || header[peOffset + 1] != std::byte{'E'} ||
            header[peOffset + 2] != std::byte{0} || header[peOffset + 3] != std::byte{0})
            return PluginBinary::NotPortableExecutable;
        if (readU16(header, peOffset + 4) != kMachineI386)
            return PluginBinary::NotWin32;
        if ((readU16(header, peOffset + 22) & kFileIsDll) == 0)
            return PluginBinary::NotADll;
        return PluginBinary::Win32Dll;
    }

    std::string pluginBinaryProblem(PluginBinary kind)
    {
        switch (kind) {
            case PluginBinary::Win32Dll:
                return {};
            case PluginBinary::NotWin32:
                return "not built for 32-bit Windows (a 64-bit build?); the game only loads 32-bit plugins";
            case PluginBinary::NotADll:
                return "a program, not a DLL";
            case PluginBinary::NotPortableExecutable:
                break;
        }
        return "not a Windows DLL";
    }

} // namespace crabe::domain

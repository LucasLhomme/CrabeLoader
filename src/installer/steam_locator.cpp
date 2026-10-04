/*
** CrabeLoader
** File description:
** Finds the game among the Steam libraries listed in the registry and in libraryfolders.vdf.
** Every failure is an empty answer: not finding the game just means the dialog opens blank.
** Writes nothing, to the registry or to disk.
**
** Authors: @LucasLhomme
*/

#include "installer/steam_locator.hpp"

#include <fstream>
#include <iterator>
#include <string>
#include <system_error>

#include <windows.h>

#include "installer/install_plan.hpp"

#pragma comment(lib, "advapi32.lib")

namespace crabe::installer {

    namespace {

        constexpr std::wstring_view kSteamKeyCurrentUser = L"Software\\Valve\\Steam";
        constexpr std::wstring_view kSteamKeyMachine = L"SOFTWARE\\WOW6432Node\\Valve\\Steam";

        [[nodiscard]] std::optional<std::string> readQuoted(std::string_view& cursor)
        {
            const std::size_t open = cursor.find('"');
            if (open == std::string_view::npos)
                return std::nullopt;

            std::string value;
            std::size_t i = open + 1;
            while (i < cursor.size() && cursor[i] != '"') {
                if (cursor[i] == '\\' && i + 1 < cursor.size()) {
                    value += cursor[i + 1];
                    i += 2;
                } else {
                    value += cursor[i];
                    ++i;
                }
            }
            if (i >= cursor.size())
                return std::nullopt;

            cursor.remove_prefix(i + 1);
            return value;
        }

        [[nodiscard]] std::filesystem::path pathFromUtf8(const std::string& utf8)
        {
            return std::filesystem::path(std::u8string(utf8.begin(), utf8.end()));
        }

        [[nodiscard]] std::optional<std::filesystem::path> readRegistryPath(HKEY root, std::wstring_view subKey,
                                                                            const wchar_t* valueName)
        {
            HKEY key = nullptr;
            const std::wstring subKeyText(subKey);
            if (RegOpenKeyExW(root, subKeyText.c_str(), 0, KEY_READ | KEY_WOW64_32KEY, &key) != ERROR_SUCCESS)
                return std::nullopt;

            DWORD type = 0;
            DWORD bytes = 0;
            std::optional<std::filesystem::path> result;
            if (RegQueryValueExW(key, valueName, nullptr, &type, nullptr, &bytes) == ERROR_SUCCESS
                && type == REG_SZ && bytes >= sizeof(wchar_t)) {
                std::wstring value(bytes / sizeof(wchar_t), L'\0');
                if (RegQueryValueExW(key, valueName, nullptr, &type, reinterpret_cast<LPBYTE>(value.data()), &bytes)
                    == ERROR_SUCCESS) {
                    value.resize(wcsnlen(value.c_str(), value.size()));
                    if (!value.empty())
                        result = std::filesystem::path(value).lexically_normal();
                }
            }
            RegCloseKey(key);
            return result;
        }

        [[nodiscard]] std::string readTextFile(const std::filesystem::path& file)
        {
            std::ifstream stream(file, std::ios::binary);
            if (!stream.is_open())
                return {};
            return std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
        }

    } // namespace

    std::vector<std::filesystem::path> parseLibraryFolders(std::string_view vdfText)
    {
        std::vector<std::filesystem::path> libraries;

        std::size_t lineStart = 0;
        while (lineStart < vdfText.size()) {
            const std::size_t lineEnd = vdfText.find('\n', lineStart);
            std::string_view line = vdfText.substr(lineStart, lineEnd == std::string_view::npos
                                                                 ? std::string_view::npos
                                                                 : lineEnd - lineStart);
            lineStart = lineEnd == std::string_view::npos ? vdfText.size() : lineEnd + 1;

            const auto key = readQuoted(line);
            if (!key || *key != "path")
                continue;
            const auto value = readQuoted(line);
            if (value && !value->empty())
                libraries.push_back(pathFromUtf8(*value));
        }
        return libraries;
    }

    std::optional<std::filesystem::path> findGameInLibraries(const std::vector<std::filesystem::path>& libraries)
    {
        for (const std::filesystem::path& library : libraries) {
            const std::filesystem::path candidate = library / "steamapps" / "common" / kSteamGameFolderName;
            std::error_code ignored;
            if (std::filesystem::exists(candidate / kGameExecutable, ignored))
                return candidate;
        }
        return std::nullopt;
    }

    std::optional<std::filesystem::path> locateSteamGame()
    {
        std::vector<std::filesystem::path> libraries;

        std::optional<std::filesystem::path> steamRoot = readRegistryPath(HKEY_CURRENT_USER, kSteamKeyCurrentUser, L"SteamPath");
        if (!steamRoot)
            steamRoot = readRegistryPath(HKEY_LOCAL_MACHINE, kSteamKeyMachine, L"InstallPath");
        if (!steamRoot)
            return std::nullopt;

        libraries.push_back(*steamRoot);
        const std::string vdf = readTextFile(*steamRoot / "steamapps" / "libraryfolders.vdf");
        for (std::filesystem::path& library : parseLibraryFolders(vdf))
            libraries.push_back(std::move(library));

        return findGameInLibraries(libraries);
    }

} // namespace crabe::installer

/*
** CrabeLoader
** File description:
** Finds another CrabeLoader copy in the process and decides which one runs the loader.
** Reads version resources of modules already mapped; never loads a module, so DllMain may call it.
** The rule itself is loaderRank: bink2w32.dll, then dinput8.dll, then any other name.
**
** Authors: @LucasLhomme
*/

#include "infrastructure/loader_instance.hpp"

#include <array>
#include <cwctype>

#include "shared/loader_marker.hpp"

namespace crabe::infrastructure {

    namespace {

        constexpr std::array<std::wstring_view, 2> kRankedNames = {L"bink2w32.dll", L"dinput8.dll"};

        /// Compares two names the way Windows compares module names, ignoring case.
        [[nodiscard]] bool sameName(std::wstring_view left, std::wstring_view right) noexcept
        {
            if (left.size() != right.size())
                return false;
            for (std::size_t i = 0; i < left.size(); ++i) {
                if (std::towlower(left[i]) != std::towlower(right[i]))
                    return false;
            }
            return true;
        }

        /// Reports whether a loaded module's version resource carries the CrabeLoader description.
        /// FindResource and LoadResource only read a mapped image, which the loader lock allows.
        [[nodiscard]] bool isLoaderModule(HMODULE module) noexcept
        {
            HRSRC info = FindResourceW(module, MAKEINTRESOURCEW(VS_VERSION_INFO), MAKEINTRESOURCEW(16));
            if (!info)
                return false;
            HGLOBAL loaded = LoadResource(module, info);
            const void* bytes = loaded ? LockResource(loaded) : nullptr;
            return bytes && crabe::shared::containsLoaderMarker(bytes, SizeofResource(module, info));
        }

    } // namespace

    int loaderRank(std::wstring_view moduleFileName) noexcept
    {
        for (std::size_t i = 0; i < kRankedNames.size(); ++i) {
            if (sameName(moduleFileName, kRankedNames[i]))
                return static_cast<int>(i);
        }
        return static_cast<int>(kRankedNames.size());
    }

    std::wstring moduleFileName(HMODULE module)
    {
        std::array<wchar_t, MAX_PATH> path{};
        const DWORD length = GetModuleFileNameW(module, path.data(), static_cast<DWORD>(path.size()));
        if (length == 0 || length >= path.size())
            return {};
        const std::wstring_view full(path.data(), length);
        const auto slash = full.find_last_of(L"\\/");
        return std::wstring(slash == std::wstring_view::npos ? full : full.substr(slash + 1));
    }

    std::wstring outrankingLoaderCopy(HMODULE self)
    {
        const int selfRank = loaderRank(moduleFileName(self));
        for (const std::wstring_view name : kRankedNames) {
            if (loaderRank(name) >= selfRank)
                continue;
            HMODULE other = GetModuleHandleW(std::wstring(name).c_str());
            if (other && other != self && isLoaderModule(other))
                return std::wstring(name);
        }
        return {};
    }

} // namespace crabe::infrastructure

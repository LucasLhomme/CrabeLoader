/*
** CrabeLoader
** File description:
** Builds additive bankloads dependency overlays from small per-mod JSON patches.
** AES-CTR and MSXML preserve existing bank records; generated files live in a private cache.
** Character routing and sound event selection remain the responsibility of mods and the game.
**
** Authors: @Weresthebeef
*/
#pragma once
#include <filesystem>
#include <functional>
#include <optional>
#include <string_view>
#include <vector>
namespace crabe::infrastructure {
// Builds a content-addressed encrypted overlay, throwing on any invalid input; never edits source.
std::optional<std::filesystem::path> buildBankDependencyOverlay(
    const std::filesystem::path& source,
    const std::vector<std::filesystem::path>& patches,
    const std::filesystem::path& cache,
    const std::function<bool(std::string_view)>& soundExists);
}

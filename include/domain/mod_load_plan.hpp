/*
** CrabeLoader
** File description:
** Discovers mods and resolves one immutable load plan shared by VFS and Lua execution.
** Profile filtering precedes identity assignment and dependency resolution.
** Executes no Lua and indexes no assets; those consumers use the same accepted candidates.
**
** Authors: @Weresthebeef
*/
#pragma once
#include "domain/dependency_resolver.hpp"
#include <filesystem>
#include <functional>
#include <vector>
namespace crabe::domain {
struct ModCandidate {
    std::string id;
    std::string name;
    std::filesystem::path path;
    bool isDirectory{false};
    ModManifest manifest;
};
struct ModLoadPlan {
    std::filesystem::path folder;
    std::vector<ModCandidate> candidates;
    ResolutionResult resolution;
};
// Reads candidate manifests once, then resolves enabled and compatible mods in dependency order.
ModLoadPlan discoverModLoadPlan(const std::filesystem::path& folder,
    const std::function<bool(std::string_view)>& enabled = {});
}

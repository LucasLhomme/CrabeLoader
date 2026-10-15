/*
** CrabeLoader
** File description:
** Implements the load decision: validity, ranges, conflicts, then a stable topological sort.
** A rejected mod stays addressable, so a mod depending on it can be told exactly what went wrong.
** A guard bank at the foot of this file fails the build if Windows, Lua or MinHook reaches it.
**
** Authors: @LucasLhomme
*/

#include "domain/dependency_resolver.hpp"

#include <algorithm>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <map>
#include <optional>
#include <queue>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace crabe::domain {

    // Returns a readable description of a mod rejection reason.
    std::string_view describe(Rejection reason) noexcept
    {
        switch (reason) {
        case Rejection::MissingDependency:
            return "missing dependency";
        case Rejection::VersionMismatch:
            return "version mismatch";
        case Rejection::CyclicDependency:
            return "cyclic dependency";
        case Rejection::LoaderTooOld:
            return "loader too old";
        case Rejection::LoaderTooNew:
            return "loader too new";
        case Rejection::DuplicateSupersededId:
            return "duplicate id";
        case Rejection::DeclaredConflict:
            return "declared conflict";
        case Rejection::InvalidManifest:
            return "invalid manifest";
        }
        return "unrecognised rejection";
    }

    // Finds the rejection record for the specified mod id if rejected.
    const RejectedMod* ResolutionResult::findRejection(std::string_view id) const noexcept
    {
        for (const RejectedMod& entry : rejected) {
            if (entry.id == id)
                return &entry;
        }
        return nullptr;
    }

    // Checks whether the specified mod id was accepted into the load order.
    bool ResolutionResult::isLoaded(std::string_view id) const noexcept
    {
        return std::find(loadOrder.begin(), loadOrder.end(), id) != loadOrder.end();
    }

    namespace {

        struct Entry {
            ModId id;
            const ModManifest* manifest{nullptr};
            bool alive{true};
        };

        // Formats the manifest version string or returns a placeholder.
        [[nodiscard]] std::string versionLabel(const ModManifest& manifest)
        {
            return manifest.getVersion().empty() ? std::string("(no version)")
                                                 : manifest.getVersion();
        }

        // Returns a readable label for a dependency's requested version range.
        [[nodiscard]] std::string specLabel(const ManifestDependency& dependency)
        {
            if (dependency.versionSpec.empty() || dependency.versionSpec == "*")
                return "(any version)";
            return dependency.versionSpec;
        }

        // Checks whether an entry provides a given capability or mod id.
        [[nodiscard]] bool supplies(const Entry& entry, std::string_view name)
        {
            if (entry.id == name)
                return true;
            const std::vector<std::string>& provided = entry.manifest->getProvides();
            return std::find(provided.begin(), provided.end(), name) != provided.end();
        }

        // Checks whether a supplier manifest satisfies a version range constraint.
        [[nodiscard]] bool versionSatisfies(const ModManifest& supplier, const VersionRange& range)
        {
            if (range.isAny())
                return true;
            const std::optional<SemVer>& version = supplier.getSemVer();
            return version.has_value() && range.matches(*version);
        }

        struct SupersedeKey {
            bool hasVersion{false};
            SemVer version;
            std::string tail;
        };

        // Appends a string value followed by a unit separator.
        void appendField(std::string& out, std::string_view value)
        {
            out += value;
            out += '\x1f';
        }

        // Appends a list of strings followed by a record separator.
        void appendList(std::string& out, const std::vector<std::string>& values)
        {
            for (const std::string& value : values)
                appendField(out, value);
            out += '\x1e';
        }

        // Appends dependencies serialized with unit and record separators.
        void appendDependencies(std::string& out, const std::vector<ManifestDependency>& values)
        {
            for (const ManifestDependency& dependency : values) {
                appendField(out, dependency.id);
                appendField(out, dependency.versionSpec);
            }
            out += '\x1e';
        }

        // Computes a deterministic ordering key for disambiguating duplicate mod IDs.
        [[nodiscard]] SupersedeKey supersedeKeyOf(const ModManifest& manifest)
        {
            SupersedeKey key;
            key.hasVersion = manifest.getSemVer().has_value();
            if (key.hasVersion)
                key.version = *manifest.getSemVer();

            appendField(key.tail, manifest.getVersion());
            appendField(key.tail, manifest.getName());
            appendField(key.tail, manifest.getEntry());
            appendField(key.tail, manifest.getMinLoaderVersion());
            appendField(key.tail, manifest.getMaxLoaderVersion());
            appendDependencies(key.tail, manifest.getDependencies());
            appendDependencies(key.tail, manifest.getOptionalDependencies());
            appendList(key.tail, manifest.getLoadAfter());
            appendList(key.tail, manifest.getLoadBefore());
            appendList(key.tail, manifest.getConflicts());
            appendList(key.tail, manifest.getProvides());
            return key;
        }

        // Compares two supersede keys for deterministic disambiguation.
        [[nodiscard]] std::strong_ordering compareKeys(const SupersedeKey& lhs,
                                                       const SupersedeKey& rhs) noexcept
        {
            if (lhs.hasVersion != rhs.hasVersion)
                return lhs.hasVersion ? std::strong_ordering::greater : std::strong_ordering::less;
            if (lhs.hasVersion) {
                const std::strong_ordering byVersion = lhs.version.compare(rhs.version);
                if (byVersion != std::strong_ordering::equal)
                    return byVersion;
            }
            return lhs.tail.compare(rhs.tail) <=> 0;
        }

    } // namespace

    // Resolves dependencies, conflicts, and topological load order for discovered mods.
    ResolutionResult resolve(std::span<const ResolverInput> inputs, const SemVer& loaderVersion)
    {
        ResolutionResult result;

        std::vector<Entry> entries;
        entries.reserve(inputs.size());

        auto rejectId = [&result](const ModId& id, Rejection reason, std::string detail) {
            result.rejected.push_back(RejectedMod{id, reason, std::move(detail)});
        };
        auto rejectEntry = [&rejectId](Entry& entry, Rejection reason, std::string detail) {
            entry.alive = false;
            rejectId(entry.id, reason, std::move(detail));
        };

        for (const ResolverInput& input : inputs) {
            Entry entry{input.id, input.manifest, true};

            if (entry.manifest == nullptr) {
                entry.alive = false;
                rejectId(entry.id, Rejection::InvalidManifest,
                         "no manifest was supplied for it, so nothing about it could be checked");
            } else if (entry.manifest->isMalformed()) {
                entry.alive = false;
                rejectId(entry.id, Rejection::InvalidManifest,
                         std::format("its mod.json could not be read: {}",
                                     entry.manifest->diagnostic().has_value()
                                         ? entry.manifest->diagnostic()->what()
                                         : std::string("the parser recorded no diagnostic")));
            } else if (entry.id.empty()) {
                entry.alive = false;
                rejectId(entry.id, Rejection::InvalidManifest,
                         std::format("it declares no id (its mod.json is manifestVersion {}, which "
                                     "has no \"id\" key) and none was supplied for it; the load "
                                     "order has to name it somehow",
                                     entry.manifest->getManifestVersion()));
            } else if (const auto ok = validateModId(entry.id); !ok) {
                entry.alive = false;
                rejectId(entry.id, Rejection::InvalidManifest,
                         std::format("its id \"{}\" cannot be used: {}", entry.id, ok.error()));
            }

            entries.push_back(std::move(entry));
        }

        for (Entry& entry : entries) {
            if (!entry.alive || !entry.manifest->isValid())
                continue;

            const std::string& minimum = entry.manifest->getMinLoaderVersion();
            if (!minimum.empty()) {
                if (const auto required = SemVer::parse(minimum); required
                    && loaderVersion < *required) {
                    rejectEntry(entry, Rejection::LoaderTooOld,
                                std::format("it needs CrabeLoader {} or newer, but this loader is {}",
                                            required->toString(), loaderVersion.toString()));
                    continue;
                }
            }

            const std::string& ceiling = entry.manifest->getMaxLoaderVersion();
            if (!ceiling.empty()) {
                if (const auto range = VersionRange::parse(ceiling); range
                    && range->exceededBy(loaderVersion)) {
                    rejectEntry(entry, Rejection::LoaderTooNew,
                                std::format("it was written for CrabeLoader {} at the newest, but "
                                            "this loader is {}",
                                            ceiling, loaderVersion.toString()));
                }
            }
        }

        {
            std::map<ModId, std::size_t> champions;
            std::map<std::size_t, SupersedeKey> keys;

            for (std::size_t index = 0; index < entries.size(); ++index) {
                if (!entries[index].alive)
                    continue;
                keys.emplace(index, supersedeKeyOf(*entries[index].manifest));
                const auto [slot, inserted] = champions.try_emplace(entries[index].id, index);
                if (inserted)
                    continue;
                if (compareKeys(keys.at(index), keys.at(slot->second)) > 0)
                    slot->second = index;
            }

            for (std::size_t index = 0; index < entries.size(); ++index) {
                if (!entries[index].alive)
                    continue;
                const std::size_t champion = champions.at(entries[index].id);
                if (champion == index)
                    continue;
                rejectEntry(entries[index], Rejection::DuplicateSupersededId,
                            std::format("another copy of \"{}\" is present declaring version {}, "
                                        "which supersedes this copy's {}",
                                        entries[index].id,
                                        versionLabel(*entries[champion].manifest),
                                        versionLabel(*entries[index].manifest)));
            }
        }

        auto aliveSortedIndices = [&entries]() {
            std::vector<std::size_t> indices;
            for (std::size_t index = 0; index < entries.size(); ++index) {
                if (entries[index].alive)
                    indices.push_back(index);
            }
            std::sort(indices.begin(), indices.end(),
                      [&entries](std::size_t lhs, std::size_t rhs) {
                          return entries[lhs].id < entries[rhs].id;
                      });
            return indices;
        };

        auto suppliersOf = [&entries, &aliveSortedIndices](std::string_view name) {
            std::vector<std::size_t> found;
            for (std::size_t index : aliveSortedIndices()) {
                if (supplies(entries[index], name))
                    found.push_back(index);
            }
            return found;
        };

        auto describeCandidates = [&entries](const std::vector<std::size_t>& candidates) {
            std::string out;
            for (std::size_t index : candidates) {
                if (!out.empty())
                    out += ", ";
                out += std::format("{} {}", entries[index].id,
                                   versionLabel(*entries[index].manifest));
            }
            return out;
        };

        auto runConflictPass = [&]() {
            bool changed = false;
            const std::vector<std::size_t> snapshot = aliveSortedIndices();

            std::map<ModId, std::size_t> byId;
            for (std::size_t index : snapshot)
                byId.emplace(entries[index].id, index);

            for (std::size_t index : snapshot) {
                if (!entries[index].alive)
                    continue;
                if (!entries[index].manifest->isValid())
                    continue;

                for (const std::string& other : entries[index].manifest->getConflicts()) {
                    if (other == entries[index].id)
                        continue;
                    const auto slot = byId.find(other);
                    if (slot == byId.end() || !entries[slot->second].alive)
                        continue;

                    const ModId declarer = entries[index].id;
                    rejectEntry(entries[slot->second], Rejection::DeclaredConflict,
                                std::format("\"{}\" declares a conflict with it, and both are "
                                            "present",
                                            declarer));
                    rejectEntry(entries[index], Rejection::DeclaredConflict,
                                std::format("it declares a conflict with \"{}\", which is also "
                                            "present",
                                            other));
                    changed = true;
                    break;
                }
            }
            return changed;
        };

        auto runDependencyPass = [&]() {
            bool changed = false;
            for (std::size_t index : aliveSortedIndices()) {
                if (!entries[index].alive)
                    continue;
                if (!entries[index].manifest->isValid())
                    continue;

                for (const ManifestDependency& dependency :
                     entries[index].manifest->getDependencies()) {
                    const std::vector<std::size_t> candidates = suppliersOf(dependency.id);

                    if (candidates.empty()) {
                        const bool wasRejected = result.findRejection(dependency.id) != nullptr;
                        rejectEntry(entries[index], Rejection::MissingDependency,
                                     wasRejected
                                        ? std::format("it requires \"{}\" {}, which was discovered "
                                                      "but rejected",
                                                      dependency.id, specLabel(dependency))
                                        : std::format("it requires \"{}\" {}, which no discovered "
                                                      "mod supplies",
                                                      dependency.id, specLabel(dependency)));
                        changed = true;
                        break;
                    }

                    const bool satisfied = std::any_of(
                        candidates.begin(), candidates.end(), [&](std::size_t candidate) {
                            return versionSatisfies(*entries[candidate].manifest, dependency.range);
                        });
                    if (!satisfied) {
                        rejectEntry(entries[index], Rejection::VersionMismatch,
                                    std::format("it requires \"{}\" {}, but the only version "
                                                "present is {}",
                                                dependency.id, specLabel(dependency),
                                                describeCandidates(candidates)));
                        changed = true;
                        break;
                    }
                }
            }
            return changed;
        };

        for (bool changed = true; changed;) {
            changed = runConflictPass();
            changed = runDependencyPass() || changed;
        }

        const std::vector<std::size_t> survivors = aliveSortedIndices();
        const std::size_t count = survivors.size();

        std::map<std::size_t, std::size_t> nodeOf;
        for (std::size_t node = 0; node < count; ++node)
            nodeOf.emplace(survivors[node], node);

        std::vector<std::vector<std::size_t>> prerequisites(count);
        std::vector<std::vector<std::size_t>> dependents(count);
        std::set<std::pair<std::size_t, std::size_t>> edges;

        auto addEdge = [&](std::size_t before, std::size_t after) {
            if (before == after)
                return;
            if (!edges.emplace(before, after).second)
                return;
            prerequisites[after].push_back(before);
            dependents[before].push_back(after);
        };

        for (std::size_t node = 0; node < count; ++node) {
            const ModManifest& manifest = *entries[survivors[node]].manifest;
            if (!manifest.isValid())
                continue;

            auto link = [&](std::string_view name, bool supplierFirst, const VersionRange* range) {
                for (std::size_t candidate : suppliersOf(name)) {
                    if (range != nullptr && !versionSatisfies(*entries[candidate].manifest, *range))
                        continue;
                    const std::size_t other = nodeOf.at(candidate);
                    if (supplierFirst)
                        addEdge(other, node);
                    else
                        addEdge(node, other);
                }
            };

            for (const ManifestDependency& dependency : manifest.getDependencies())
                link(dependency.id, true, &dependency.range);
            for (const ManifestDependency& dependency : manifest.getOptionalDependencies())
                link(dependency.id, true, &dependency.range);
            for (const std::string& name : manifest.getLoadAfter())
                link(name, true, nullptr);
            for (const std::string& name : manifest.getLoadBefore())
                link(name, false, nullptr);
        }

        std::vector<std::size_t> indegree(count, 0);
        for (std::size_t node = 0; node < count; ++node)
            indegree[node] = prerequisites[node].size();

        std::priority_queue<std::size_t, std::vector<std::size_t>, std::greater<std::size_t>> ready;
        for (std::size_t node = 0; node < count; ++node) {
            if (indegree[node] == 0)
                ready.push(node);
        }

        std::vector<bool> ordered(count, false);
        result.loadOrder.reserve(count);
        result.sourceIndex.reserve(count);
        while (!ready.empty()) {
            const std::size_t node = ready.top();
            ready.pop();
            ordered[node] = true;
            result.loadOrder.push_back(entries[survivors[node]].id);
            result.sourceIndex.push_back(survivors[node]);
            for (std::size_t next : dependents[node]) {
                if (--indegree[next] == 0)
                    ready.push(next);
            }
        }

        if (result.loadOrder.size() != count) {
            enum class Mark : std::uint8_t { White, Grey, Black };

            std::vector<Mark> mark(count, Mark::Black);
            for (std::size_t node = 0; node < count; ++node) {
                if (!ordered[node])
                    mark[node] = Mark::White;
            }

            std::vector<std::string> cycleOf(count);

            auto recordCycle = [&](std::vector<std::size_t> cycle) {
                if (cycle.empty())
                    return;
                std::rotate(cycle.begin(), std::min_element(cycle.begin(), cycle.end()),
                            cycle.end());

                std::string text;
                for (std::size_t member : cycle) {
                    text += entries[survivors[member]].id;
                    text += " -> ";
                }
                text += entries[survivors[cycle.front()]].id;

                for (std::size_t member : cycle) {
                    if (cycleOf[member].empty())
                        cycleOf[member] = text;
                }
            };

            struct Frame {
                std::size_t node{0};
                std::size_t next{0};
            };

            for (std::size_t start = 0; start < count; ++start) {
                if (mark[start] != Mark::White)
                    continue;

                std::vector<Frame> stack;
                std::vector<std::size_t> path;
                mark[start] = Mark::Grey;
                path.push_back(start);
                stack.push_back(Frame{start, 0});

                while (!stack.empty()) {
                    const std::size_t current = stack.back().node;
                    const std::vector<std::size_t>& parents = prerequisites[current];

                    if (stack.back().next >= parents.size()) {
                        mark[current] = Mark::Black;
                        path.pop_back();
                        stack.pop_back();
                        continue;
                    }

                    const std::size_t child = parents[stack.back().next++];
                    if (mark[child] == Mark::Black)
                        continue;
                    if (mark[child] == Mark::Grey) {
                        const auto closes = std::find(path.begin(), path.end(), child);
                        recordCycle(std::vector<std::size_t>(closes, path.end()));
                        continue;
                    }
                    mark[child] = Mark::Grey;
                    path.push_back(child);
                    stack.push_back(Frame{child, 0});
                }
            }

            auto blockingCycle = [&](std::size_t from) -> std::optional<std::size_t> {
                std::vector<bool> seen(count, false);
                std::queue<std::size_t> frontier;
                seen[from] = true;
                frontier.push(from);
                while (!frontier.empty()) {
                    const std::size_t node = frontier.front();
                    frontier.pop();
                    if (!cycleOf[node].empty())
                        return node;
                    for (std::size_t parent : prerequisites[node]) {
                        if (!seen[parent] && !ordered[parent]) {
                            seen[parent] = true;
                            frontier.push(parent);
                        }
                    }
                }
                return std::nullopt;
            };

            for (std::size_t node = 0; node < count; ++node) {
                if (ordered[node])
                    continue;

                Entry& mod = entries[survivors[node]];
                if (!cycleOf[node].empty()) {
                    rejectEntry(mod, Rejection::CyclicDependency,
                                std::format("it is part of a dependency cycle: {}", cycleOf[node]));
                    continue;
                }

                if (const auto blocker = blockingCycle(node); blocker.has_value()) {
                    rejectEntry(mod, Rejection::CyclicDependency,
                                std::format("it must load after \"{}\", which is part of a "
                                            "dependency cycle: {}",
                                            entries[survivors[*blocker]].id, cycleOf[*blocker]));
                    continue;
                }

                rejectEntry(mod, Rejection::CyclicDependency,
                            "it waits on a dependency cycle that could not be spelled out");
            }
        }

        std::sort(result.rejected.begin(), result.rejected.end(),
                  [](const RejectedMod& lhs, const RejectedMod& rhs) {
                      if (lhs.id != rhs.id)
                          return lhs.id < rhs.id;
                      if (lhs.reason != rhs.reason)
                          return lhs.reason < rhs.reason;
                      return lhs.detail < rhs.detail;
                  });

        return result;
    }

    // Resolves mod manifests directly using their declared IDs.
    ResolutionResult resolve(std::span<const ModManifest> manifests, const SemVer& loaderVersion)
    {
        std::vector<ResolverInput> inputs;
        inputs.reserve(manifests.size());
        for (const ModManifest& manifest : manifests)
            inputs.push_back(ResolverInput{manifest.getId(), &manifest});
        return resolve(std::span<const ResolverInput>(inputs), loaderVersion);
    }

} // namespace crabe::domain

#if defined(_WINDOWS_) || defined(_INC_WINDOWS)
#error "dependency_resolver.cpp is not pure: windows.h reached it transitively"
#endif
#if defined(_WINNT_) || defined(_WINDEF_) || defined(_MINWINDEF_) || defined(_WINBASE_)
#error "dependency_resolver.cpp is not pure: a Win32 API header reached it transitively"
#endif
#if defined(_WINUSER_) || defined(_WINGDI_) || defined(_WINREG_) || defined(_MINWINBASE_)
#error "dependency_resolver.cpp is not pure: a Win32 API header reached it transitively"
#endif
#if defined(_WINSOCKAPI_) || defined(_WINSOCK2API_) || defined(_OBJBASE_H_) || defined(__RPC_H__)
#error "dependency_resolver.cpp is not pure: a Win32 API header reached it transitively"
#endif
#if defined(lua_h) || defined(LUA_VERSION) || defined(LUA_REGISTRYINDEX) || defined(lauxlib_h)
#error "dependency_resolver.cpp is not pure: a Lua header reached it transitively"
#endif
#if defined(IMGUI_VERSION) || defined(MH_ALL_HOOKS)
#error "dependency_resolver.cpp is not pure: the renderer or the hook engine reached it"
#endif

/*
** CrabeLoader
** File description:
** Declares the pure decision of which mods load and in what order they run.
** No I/O, clock, Windows or Lua reaches this translation unit, and a guard bank proves it.
** Invents no id for a manifest that declares none -- that policy is domain/mod_id.hpp.
**
** Authors: @LucasLhomme
*/

#ifndef CRABELOADER_DOMAIN_DEPENDENCY_RESOLVER_HPP_
#define CRABELOADER_DOMAIN_DEPENDENCY_RESOLVER_HPP_

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "domain/mod_manifest.hpp"
#include "domain/semver.hpp"

namespace crabe::domain {

    using ModId = std::string;

    enum class Rejection : std::uint8_t {
        MissingDependency,
        VersionMismatch,
        CyclicDependency,
        LoaderTooOld,
        LoaderTooNew,
        DuplicateSupersededId,
        DeclaredConflict,
        InvalidManifest,
    };

    // Returns a human-readable description of a rejection reason.
    [[nodiscard]] std::string_view describe(Rejection reason) noexcept;

    struct RejectedMod {
        ModId id;
        Rejection reason{Rejection::InvalidManifest};
        std::string detail;
    };

    struct ResolutionResult {
        std::vector<ModId> loadOrder;
        std::vector<std::size_t> sourceIndex;
        std::vector<RejectedMod> rejected;

        // Returns the first recorded rejection for the given mod id, or nullptr.
        [[nodiscard]] const RejectedMod* findRejection(std::string_view id) const noexcept;

        // Returns true if the given mod id is present in the final load order.
        [[nodiscard]] bool isLoaded(std::string_view id) const noexcept;
    };

    struct ResolverInput {
        ModId id;
        const ModManifest* manifest{nullptr};
    };

    // Resolves mod load order directly from self-declaring manifestVersion 1 files.
    [[nodiscard]] ResolutionResult resolve(std::span<const ModManifest> manifests,
                                           const SemVer& loaderVersion);

    // Resolves mod dependencies, conflicts, and topological load order against the loader version.
    [[nodiscard]] ResolutionResult resolve(std::span<const ResolverInput> inputs,
                                           const SemVer& loaderVersion);

} // namespace crabe::domain

#endif /* !CRABELOADER_DOMAIN_DEPENDENCY_RESOLVER_HPP_ */

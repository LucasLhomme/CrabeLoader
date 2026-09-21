#ifndef CRABELOADER_DOMAIN_DEPENDENCY_RESOLVER_HPP_
#define CRABELOADER_DOMAIN_DEPENDENCY_RESOLVER_HPP_

// Decides which mods load, and in what order.
//
// Deliberately pure, for the same reason semver.hpp is: it takes manifests and
// a loader version, and it returns an order. No file is read, nothing is
// logged, no clock is consulted, no global is touched, and neither Windows nor
// Lua is reachable from this translation unit. That is what makes every rule
// below a unit test instead of a play session -- see the compile-time guards at
// the foot of src/domain/dependency_resolver.cpp, which fail the build if a
// Win32 or Lua header ever arrives here transitively.
//
// The impure half -- discovering directories, synthesising ids for manifests
// that have none, running Lua, writing the load report -- lives in ModManager.

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "domain/mod_manifest.hpp"
#include "domain/semver.hpp"

namespace crabe::domain {

    // The key the whole resolver turns on. A plain string: it is what a
    // manifest declares ("com.example.my-mod"), what a dependency names, and
    // what the load order is expressed in, so a distinct type would buy
    // nothing but conversions. Every id the resolver accepts satisfies
    // validateModId(); anything else is rejected as InvalidManifest rather
    // than silently participating under a name no other mod could spell.
    using ModId = std::string;

    // Why a mod is not in the load order. Each value is one concrete cause, so
    // the report can say what to fix rather than "could not load".
    enum class Rejection : std::uint8_t {
        MissingDependency,     // a required dependency is absent, or was itself rejected
        VersionMismatch,       // the dependency is present, but no version of it fits
        CyclicDependency,      // it sits on, or behind, a cycle
        LoaderTooOld,          // minLoaderVersion is above the running loader
        LoaderTooNew,          // maxLoaderVersion is below the running loader
        DuplicateSupersededId, // another copy of the same id declares a higher version
        DeclaredConflict,      // it and another present mod declare a conflict
        InvalidManifest,       // unusable mod.json, or an id nothing could reference
    };

    // A short noun phrase, lower case and without trailing punctuation, meant
    // to be printed next to the detail: "missing dependency; requires ...".
    [[nodiscard]] std::string_view describe(Rejection reason) noexcept;

    struct RejectedMod {
        ModId id;
        Rejection reason{Rejection::InvalidManifest};

        // Always non-empty, and always names the other party involved -- the
        // dependency that is missing, the mod it conflicts with, the version
        // that superseded it, the cycle it sits on. Invariant I7: a rejection
        // the reader cannot act on is a rejection that wasted their time.
        std::string detail;
    };

    struct ResolutionResult {
        // Every mod that loads, in the order it must load in. Deterministic:
        // the same manifests in any input order produce the same vector.
        std::vector<ModId> loadOrder;

        // Parallel to loadOrder: where in the input each entry's manifest came
        // from.
        //
        // The id alone cannot answer that. When the input held several
        // manifests under one id, rule 3 kept the highest version, and a caller
        // that went looking for "the first manifest with that id" could well
        // run a different one from the one the order was computed against.
        std::vector<std::size_t> sourceIndex;

        // Every mod that does not, sorted by (id, reason, detail). An id can
        // appear more than once only when it was duplicated in the input.
        std::vector<RejectedMod> rejected;

        // The first rejection *listed* for `id`, or nullptr when `id` was not
        // rejected. An id can carry more than one only when it was duplicated,
        // and then the lowest Rejection value comes first, not the earliest
        // one found -- read `rejected` directly when that matters. Pointers are
        // invalidated by any change to `rejected`.
        [[nodiscard]] const RejectedMod* findRejection(std::string_view id) const noexcept;

        [[nodiscard]] bool isLoaded(std::string_view id) const noexcept;
    };

    // One mod as the resolver sees it: an id, and the manifest that id was
    // taken from or assigned to.
    //
    // This pairing exists because manifestVersion 0 declares no id at all
    // (ModManifest::getId() says so at its declaration), while the resolver is
    // keyed on id throughout. Deciding what such a mod is called is policy --
    // ModManager synthesises "local.<sanitised-folder-name>" -- and policy has
    // no business in here. So the caller decides the name, and the resolver
    // stays ignorant of where it came from.
    //
    // `manifest` must outlive the resolve() call and must not be null; a null
    // one is rejected as InvalidManifest rather than dereferenced.
    struct ResolverInput {
        ModId id;
        const ModManifest* manifest{nullptr};
    };

    // Resolves manifests that already carry their own id -- every valid
    // manifestVersion 1 file does. A manifest whose getId() is empty is
    // rejected as InvalidManifest, because there is no name under which to
    // place it in the order; use the ResolverInput overload to supply one.
    [[nodiscard]] ResolutionResult resolve(std::span<const ModManifest> manifests,
                                           const SemVer& loaderVersion);

    // Resolves mods whose ids the caller has already decided.
    //
    // The rules, applied in this order:
    //
    //  1. A null manifest, an id that fails validateModId(), or a mod.json
    //     that exists but does not parse is rejected as InvalidManifest. An
    //     *absent* manifest is not a defect -- a mod may legitimately be a
    //     bare directory of .lua files -- so it is admitted, declaring no
    //     version, no dependencies and no ordering constraints.
    //  2. minLoaderVersion above, or maxLoaderVersion below, `loaderVersion`
    //     rejects the mod as LoaderTooOld / LoaderTooNew. Comparison is by
    //     full semantic-version precedence, so a prerelease loader sorts below
    //     the release it precedes.
    //  3. Among manifests sharing an id, the highest version survives and the
    //     rest are DuplicateSupersededId.
    //  4. Conflicts and unsatisfied required dependencies are then applied
    //     repeatedly until nothing changes, which is what makes rejection
    //     transitive: if A requires B and B is rejected, A follows.
    //  5. The survivors are sorted on required dependencies, *satisfied*
    //     optional dependencies, loadAfter and loadBefore.
    //  6. Anything a cycle leaves unorderable is rejected as CyclicDependency,
    //     with a detail naming the cycle ("A -> B -> C -> A", read as "A
    //     requires B requires C requires A").
    //  7. Mods that nothing orders relative to each other come out sorted by
    //     id, so the result never depends on the input order.
    //
    // Two names resolve a dependency, a loadAfter or a loadBefore: a mod's own
    // id, and any capability it lists in "provides". Version ranges are
    // matched against the supplying mod's own version. "conflicts" matches ids
    // only -- refusing to load a mod because something it merely provides is
    // unwelcome would be a far bigger hammer than the field advertises.
    [[nodiscard]] ResolutionResult resolve(std::span<const ResolverInput> inputs,
                                           const SemVer& loaderVersion);

} // namespace crabe::domain

#endif /* !CRABELOADER_DOMAIN_DEPENDENCY_RESOLVER_HPP_ */

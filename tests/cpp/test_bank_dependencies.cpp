/*
** CrabeLoader
** File description:
** Runs the production bank overlay builder against independent encrypted fixtures.
** The companion Python suite checks AES output and preservation with a separate XML parser.
** This harness does not initialize the loader or modify an installed game.
**
** Authors: @Weresthebeef
*/
#include "infrastructure/bank_dependency_patch.hpp"
#include <iostream>
#include "domain/mod_id.hpp"
#include "infrastructure/vfs_override_manager.hpp"
int main(int argc, char** argv) {
    if (argc >= 3 && std::string_view(argv[1]) == "--vfs") {
        auto& vfs = crabe::infrastructure::VfsOverrideManager::get();
        vfs.initialize(argv[2], [&](std::string_view id) {
            return argc < 4 || id != crabe::domain::synthesiseModId(argv[3]);
        });
        const auto result = vfs.resolve("sound/bankloads.bdp");
        std::cout << "RESULT=" << (result ? result->string() : "ORIGINAL") << "\n";
        return 0;
    }
    if (argc < 5) return 2;
    try {
        std::vector<std::filesystem::path> patches;
        for (int i = 4; i < argc; ++i) patches.emplace_back(argv[i]);
        const std::filesystem::path sounds(argv[3]);
        const auto result = crabe::infrastructure::buildBankDependencyOverlay(argv[1], patches, argv[2],
            [&](std::string_view name) { return std::filesystem::is_regular_file(sounds / name); });
        std::cout << (result ? result->string() : "UNCHANGED") << "\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << "\n"; return 1; }
}

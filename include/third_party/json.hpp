#ifndef CRABELOADER_THIRD_PARTY_JSON_HPP_
#define CRABELOADER_THIRD_PARTY_JSON_HPP_

// Single point of entry for the vendored nlohmann/json (see
// nlohmann/VENDORED.md). The project compiles at /W4 and treats any warning
// from its own code as a defect; a 900 KB third-party header has no business
// competing for that budget, and it may not be edited to silence anything, so
// it is included at warning level 0 here and nowhere else.

#if defined(_MSC_VER)
#    pragma warning(push, 0)
#elif defined(__GNUC__) || defined(__clang__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wall"
#    pragma GCC diagnostic ignored "-Wextra"
#endif

#include <nlohmann/json.hpp>

#if defined(_MSC_VER)
#    pragma warning(pop)
#elif defined(__GNUC__) || defined(__clang__)
#    pragma GCC diagnostic pop
#endif

#endif /* !CRABELOADER_THIRD_PARTY_JSON_HPP_ */

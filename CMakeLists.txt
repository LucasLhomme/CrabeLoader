cmake_minimum_required(VERSION 3.21)

project(CrabeLoader VERSION 0.2.0 LANGUAGES C CXX)
# Read VERSION file as the single source of truth
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" CRABELOADER_VERSION_RAW)
string(STRIP "${CRABELOADER_VERSION_RAW}" CRABELOADER_VERSION)

project(CrabeLoader VERSION ${CRABELOADER_VERSION} LANGUAGES C CXX)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/include/shared/version.hpp.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/shared/version.hpp"
    @ONLY
)
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/version.rc.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/version.rc"
    @ONLY
)

option(CRABELOADER_AS_SHARED "Build CrabeLoader as a shared library (DLL)." OFF)
option(CRABELOADER_BUILD_IMGUI "Build bundled Dear ImGui sources/backends." ON)
option(CRABELOADER_BUILD_TESTS "Build the unit test suite." OFF)
option(CRABELOADER_BUILD_CLI "Build crabe-cli, the offline mod-authoring tool." OFF)

# The test suite exercises the real crabe-cli.exe (new-mod's scaffold is fed
# straight into validate, end to end), so a test build needs it regardless of
# what the option above says. This does not change what a plain `-DCRABELOADER_
# BUILD_TESTS=OFF` configuration produces: crabe-cli stays off there unless
# asked for explicitly.
if(CRABELOADER_BUILD_TESTS)
    set(CRABELOADER_BUILD_CLI ON)
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Static CRT: the proxy DLL is dropped into a game folder, so it must not
# depend on a VC++ redistributable (debug or release) being installed there.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include(FetchContent)

# Third-party dependencies
set(JSON_BuildTests OFF CACHE INTERNAL "")
FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    URL_HASH SHA256=d6c65aca6b1ed68e7a182f4757257b107ae403032760ed6ef121c9d55e81757d
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

set(BUILD_TESTING OFF CACHE INTERNAL "")
set(TOML_ENABLE_TESTS OFF CACHE INTERNAL "")
set(TOML_BUILD_EXAMPLES OFF CACHE INTERNAL "")
FetchContent_Declare(
    tomlplusplus
    URL https://github.com/marzer/tomlplusplus/archive/refs/tags/v3.4.0.tar.gz
    URL_HASH SHA256=8517f65938a4faae9ccf8ebb36631a38c1cadfb5efa85d9a72e15b9e97d25155
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

FetchContent_MakeAvailable(nlohmann_json tomlplusplus)


set(CRABELOADER_CORE_SOURCES
    src/main.cpp
    src/version.rc

    # Presentation Layer
    src/presentation/overlay.cpp
    src/presentation/render_hook.cpp
    src/presentation/input_hook.cpp
    src/presentation/imgui_bindings.cpp
    src/presentation/draw_buffer.cpp

    # Application Layer
    src/application/loader.cpp
    src/application/loader_content.cpp
    src/application/gateway.cpp
    src/application/lua_runtime.cpp
    src/application/lua_natives.cpp
    src/application/multiplayer/multiplayer_manager.cpp
    src/application/multiplayer/multiplayer_natives.cpp

    # Domain Layer
    src/domain/semver.cpp
    src/domain/mod_manifest.cpp
    src/domain/mod_id.cpp
    src/domain/mod_entry.cpp
    src/domain/dependency_resolver.cpp
    src/domain/game_profile.cpp
    src/domain/mod_manager.cpp
    src/domain/reload_generation.cpp
    src/domain/config.cpp

    # Infrastructure Layer
    src/infrastructure/crash_handler.cpp
    src/infrastructure/crash_reporter.cpp
    src/infrastructure/memory.cpp
    src/infrastructure/game_profile_detect.cpp
    src/infrastructure/hook.cpp
    src/infrastructure/hook_registry.cpp
    src/infrastructure/minhook_backend.cpp
    src/infrastructure/code_cave.cpp
    src/infrastructure/engine_free_camera.cpp
    src/infrastructure/message_hook.cpp
    src/infrastructure/lua_call.cpp
    src/infrastructure/lua_symbols.cpp
    src/infrastructure/vfs_override_manager.cpp
    src/infrastructure/vfs_hook.cpp
    src/infrastructure/multiplayer/memory_patcher.cpp
    src/infrastructure/multiplayer/winhttp_redirector.cpp
    src/infrastructure/multiplayer/fallback_response_provider.cpp
    src/infrastructure/multiplayer/upnp_nat_service.cpp
    src/infrastructure/multiplayer/steamworks_service.cpp

    # Shared Layer
    src/shared/logger.cpp
    src/shared/version.cpp
    src/shared/keybind_names.cpp
)

set(CRABELOADER_MINHOOK_SOURCES
    src/minhook/buffer.c
    src/minhook/hook.c
    src/minhook/trampoline.c
    src/minhook/hde/hde32.c
    src/minhook/hde/hde64.c
)

set(CRABELOADER_IMGUI_SOURCES)
if(CRABELOADER_BUILD_IMGUI)
    list(APPEND CRABELOADER_IMGUI_SOURCES
        include/imgui/imgui.cpp
        include/imgui/imgui_draw.cpp
        include/imgui/imgui_tables.cpp
        include/imgui/imgui_widgets.cpp
        include/imgui/imgui_impl_win32.cpp
        include/imgui/imgui_impl_dx11.cpp
    )
endif()

set(CRABELOADER_SOURCES
    ${CRABELOADER_CORE_SOURCES}
    ${CRABELOADER_MINHOOK_SOURCES}
    ${CRABELOADER_IMGUI_SOURCES}
)

if(CRABELOADER_AS_SHARED)
    add_library(${PROJECT_NAME} SHARED ${CRABELOADER_SOURCES})
    # The game loads bink2w32.dll; proxy.hpp forwards every export to
    # bink2w32_orig.dll. Naming the output anything else means the game never
    # loads the loader at all.
    set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "bink2w32")
else()
    add_executable(${PROJECT_NAME} ${CRABELOADER_SOURCES})
endif()

target_include_directories(${PROJECT_NAME}
    PRIVATE
        include
        include/presentation
        include/application
        include/domain
        include/infrastructure
        include/shared
        include/imgui
        include/minhook
        src
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
)

# Embedded Lua API generation
find_package(Python3 COMPONENTS Interpreter REQUIRED)
file(GLOB CRABELOADER_API_LUA_FILES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/api/*.lua"
)
execute_process(
    COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/tools/embed_api.py"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)
add_custom_command(
    OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/include/application/embedded_api.hpp"
    COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/tools/embed_api.py"
    DEPENDS ${CRABELOADER_API_LUA_FILES} "${CMAKE_CURRENT_SOURCE_DIR}/tools/embed_api.py"
    COMMENT "Embedding Lua API modules into embedded_api.hpp..."
)
add_custom_target(generate_embedded_api
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/application/embedded_api.hpp"
)
add_dependencies(${PROJECT_NAME} generate_embedded_api)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /utf-8)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()

# The crash reporter's stack walk, symbol lookup and MiniDumpWriteDump. Needed
# whether or not Dear ImGui is built, so it is stated outside the block below.
# src/infrastructure/crash_reporter.cpp also carries a #pragma comment(lib) for
# it, so that the test suite and the crash probe -- which compile that
# translation unit without going through this target -- link it too.
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        dbghelp
        nlohmann_json::nlohmann_json
        tomlplusplus::tomlplusplus
)

if(CRABELOADER_BUILD_IMGUI)
    target_link_libraries(${PROJECT_NAME}
        PRIVATE
            d3d11
            dxgi
            d3dcompiler
            user32
            gdi32
            winhttp
            ole32
            oleaut32
            ws2_32
            iphlpapi
    )
endif()

# ---------------------------------------------------------------------------
# crabe-cli -- an offline console tool for mod authors (new-mod, validate,
# resolve, check-api). It links the domain layer only: manifest parsing,
# SemVer and the dependency resolver, plus the "shared" layer domain already
# sits on top of. No Win32 API, no Direct3D, no Dear ImGui, no Lua runtime --
# see tests/resolver_purity.cmake, which audits every one of its translation
# units the same way it already audits dependency_resolver.cpp.
#
# Off by default so a plain configure still produces exactly the one artefact
# (bink2w32.dll or the .exe fallback) it always has; CRABELOADER_BUILD_TESTS
# turns it on above because the test suite drives the real binary.
# ---------------------------------------------------------------------------
if(CRABELOADER_BUILD_CLI)
    find_package(Python3 COMPONENTS Interpreter REQUIRED)

    # check-api's ground truth is docs/crabe_api.def.lua; embedding a copy
    # (mirroring generate_embedded_api below, kept separate because that one
    # feeds the shipped DLL) means crabe-cli.exe can check a mod against it
    # from anywhere, without the CrabeLoader source tree nearby. `--api-def`
    # can still point check-api at a different file at run time.
    set(CRABE_CLI_API_DEF_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/include/cli/embedded_api_def.hpp")
    add_custom_command(
        OUTPUT "${CRABE_CLI_API_DEF_HEADER}"
        COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/tools/embed_api_def.py"
                "${CMAKE_CURRENT_SOURCE_DIR}/docs/crabe_api.def.lua" "${CRABE_CLI_API_DEF_HEADER}"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/docs/crabe_api.def.lua"
                "${CMAKE_CURRENT_SOURCE_DIR}/tools/embed_api_def.py"
        COMMENT "Embedding docs/crabe_api.def.lua for crabe-cli check-api..."
    )
    add_custom_target(generate_cli_api_def DEPENDS "${CRABE_CLI_API_DEF_HEADER}")

    add_executable(crabe-cli
        src/cli/main.cpp
        src/cli/mod_scaffold.cpp
        src/cli/validate_command.cpp
        src/cli/resolve_command.cpp
        src/cli/check_api.cpp
        src/cli/lua_call_scan.cpp

        src/domain/semver.cpp
        src/domain/mod_manifest.cpp
        src/domain/mod_id.cpp
        src/domain/mod_entry.cpp
        src/domain/dependency_resolver.cpp
        src/shared/version.cpp
    )
    add_dependencies(crabe-cli generate_cli_api_def)

    # include/ alone: no src/, no imgui, no minhook, no Lua. Mirrors
    # crabe_resolver_pure's own include directories in tests/CMakeLists.txt.
    target_include_directories(crabe-cli PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(crabe-cli PRIVATE nlohmann_json::nlohmann_json)

    target_compile_definitions(crabe-cli
        PRIVATE
            _CRT_SECURE_NO_WARNINGS
            CRABE_CLI_VERSION="${CRABELOADER_VERSION}"
    )

    if(MSVC)
        target_compile_options(crabe-cli PRIVATE /W4 /utf-8)
    else()
        target_compile_options(crabe-cli PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()

add_executable(test_vfs_override
    tests/cpp/test_vfs_override.cpp
    src/infrastructure/vfs_override_manager.cpp
    src/shared/logger.cpp
    src/shared/version.cpp
)
target_include_directories(test_vfs_override PRIVATE
    include
    include/infrastructure
    include/shared
    src
)
target_compile_definitions(test_vfs_override PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    _CRT_SECURE_NO_WARNINGS
)
if(MSVC)
    target_compile_options(test_vfs_override PRIVATE /W4 /utf-8)
else()
    target_compile_options(test_vfs_override PRIVATE -Wall -Wextra -Wpedantic)
endif()



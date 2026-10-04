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
option(CRABELOADER_BUILD_INSTALLER "Build CrabeInstaller.exe, the one-click installer." OFF)
option(CRABELOADER_INSTALLER_EMBED_DLL "Embed bink2w32.dll inside CrabeInstaller.exe (needs CRABELOADER_AS_SHARED)." ON)

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
    src/presentation/pad_state.cpp
    src/presentation/imgui_bindings.cpp
    src/presentation/draw_buffer.cpp

    # Application Layer
    src/application/loader.cpp
    src/application/loader_content.cpp
    src/application/gateway.cpp
    src/application/lua_runtime.cpp
    src/application/lua_natives.cpp
    src/application/update_checker.cpp
    src/application/update_launcher.cpp

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
    src/domain/update_check.cpp

    # Infrastructure Layer
    src/infrastructure/crash_handler.cpp
    src/infrastructure/crash_reporter.cpp
    src/infrastructure/memory.cpp
    src/infrastructure/game_profile_detect.cpp
    src/infrastructure/hook.cpp
    src/infrastructure/hook_registry.cpp
    src/infrastructure/minhook_backend.cpp
    src/infrastructure/code_cave.cpp
    src/infrastructure/engine_actors.cpp
    src/infrastructure/engine_free_camera.cpp
    src/infrastructure/engine_kinematics.cpp
    src/infrastructure/engine_meridian.cpp
    src/infrastructure/engine_shaders.cpp
    src/infrastructure/engine_sky.cpp
    src/infrastructure/script_natives.cpp
    src/infrastructure/message_hook.cpp
    src/infrastructure/lua_call.cpp
    src/infrastructure/lua_symbols.cpp
    src/infrastructure/vfs_override_manager.cpp
    src/infrastructure/vfs_hook.cpp
    src/infrastructure/update_services.cpp

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

# Multiplayer is work in progress and kept out of the repository (.gitignore):
# it is built only where its sources are present, so a clean clone builds without it.
set(CRABELOADER_MULTIPLAYER_SOURCES
    src/application/multiplayer/multiplayer_manager.cpp
    src/application/multiplayer/multiplayer_natives.cpp
    src/infrastructure/multiplayer/memory_patcher.cpp
    src/infrastructure/multiplayer/winhttp_redirector.cpp
    src/infrastructure/multiplayer/fallback_response_provider.cpp
    src/infrastructure/multiplayer/upnp_nat_service.cpp
    src/infrastructure/multiplayer/steamworks_service.cpp
)
set(CRABELOADER_WITH_MULTIPLAYER ON)
foreach(source IN LISTS CRABELOADER_MULTIPLAYER_SOURCES)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${source}")
        set(CRABELOADER_WITH_MULTIPLAYER OFF)
    endif()
endforeach()
if(CRABELOADER_WITH_MULTIPLAYER)
    list(APPEND CRABELOADER_CORE_SOURCES ${CRABELOADER_MULTIPLAYER_SOURCES})
    message(STATUS "CrabeLoader: multiplayer sources found, building with multiplayer")
else()
    message(STATUS "CrabeLoader: multiplayer sources absent, building without multiplayer")
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
if(CRABELOADER_WITH_MULTIPLAYER)
    target_compile_definitions(${PROJECT_NAME} PRIVATE CRABELOADER_WITH_MULTIPLAYER=1)
endif()

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

add_executable(test_draw_primitives
    tests/cpp/test_draw_primitives.cpp
    src/presentation/draw_buffer.cpp
    src/presentation/pad_state.cpp
    include/imgui/imgui.cpp
    include/imgui/imgui_draw.cpp
    include/imgui/imgui_tables.cpp
    include/imgui/imgui_widgets.cpp
)
target_include_directories(test_draw_primitives PRIVATE
    include
    include/imgui
)
target_compile_definitions(test_draw_primitives PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    _CRT_SECURE_NO_WARNINGS
)
if(MSVC)
    target_compile_options(test_draw_primitives PRIVATE /utf-8)
endif()

add_executable(crabe_fate
    tests/cpp/crabe_fate_main.cpp
    src/infrastructure/vfs_override_manager.cpp
    src/domain/dependency_resolver.cpp
    src/domain/mod_manifest.cpp
    src/domain/mod_id.cpp
    src/domain/semver.cpp
    src/shared/logger.cpp
    src/shared/version.cpp
)
target_include_directories(crabe_fate PRIVATE
    include
    include/domain
    include/infrastructure
    include/shared
    src
)
target_compile_definitions(crabe_fate PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    _CRT_SECURE_NO_WARNINGS
)
target_link_libraries(crabe_fate PRIVATE
    nlohmann_json::nlohmann_json
)
if(MSVC)
    target_compile_options(crabe_fate PRIVATE /W4 /utf-8)
else()
    target_compile_options(crabe_fate PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(test_telemetry
    tests/cpp/test_telemetry.cpp
    src/infrastructure/telemetry_monitor.cpp
)
target_include_directories(test_telemetry PRIVATE
    include
    include/infrastructure
    include/shared
    src
)
target_compile_definitions(test_telemetry PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    _CRT_SECURE_NO_WARNINGS
)
target_link_libraries(test_telemetry PRIVATE
    nlohmann_json::nlohmann_json
)
if(MSVC)
    target_compile_options(test_telemetry PRIVATE /W4 /utf-8)
else()
    target_compile_options(test_telemetry PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ---------------------------------------------------------------------------
# CrabeInstaller -- the one-click installer. It shares nothing with the loader
# but include/shared/version.hpp: it is a plain Win32 program that decides what
# it may do to a game folder (install_plan) and then does it (game_installer).
# The same sources feed test_installer, which drives the real file operations
# in a temporary folder.
# ---------------------------------------------------------------------------
set(CRABE_INSTALLER_CORE_SOURCES
    src/installer/install_plan.cpp
    src/installer/sha256.cpp
    src/installer/game_installer.cpp
    src/installer/mods_readme.cpp
    src/installer/steam_locator.cpp
)

add_executable(test_installer
    tests/cpp/test_installer.cpp
    ${CRABE_INSTALLER_CORE_SOURCES}
)
target_include_directories(test_installer PRIVATE
    include
    include/shared
    src
)
target_compile_definitions(test_installer PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    _CRT_SECURE_NO_WARNINGS
)
if(MSVC)
    target_compile_options(test_installer PRIVATE /W4 /utf-8)
else()
    target_compile_options(test_installer PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(CRABELOADER_BUILD_INSTALLER)
    set(CRABE_INSTALLER_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/installer")
    set(CRABE_INSTALLER_EMBED OFF)
    if(CRABELOADER_INSTALLER_EMBED_DLL AND CRABELOADER_AS_SHARED)
        set(CRABE_INSTALLER_EMBED ON)
    endif()

    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/src/installer/installer.rc.in"
        "${CRABE_INSTALLER_GEN_DIR}/installer.rc"
        @ONLY
    )
    if(CRABE_INSTALLER_EMBED)
        file(GENERATE
            OUTPUT "${CRABE_INSTALLER_GEN_DIR}/$<CONFIG>/payload.rc"
            CONTENT "101 RCDATA \"$<TARGET_FILE:${PROJECT_NAME}>\"\n"
        )
    else()
        file(GENERATE
            OUTPUT "${CRABE_INSTALLER_GEN_DIR}/$<CONFIG>/payload.rc"
            CONTENT "/* no embedded payload: the installer looks for bink2w32.dll beside itself */\n"
        )
    endif()

    add_executable(CrabeInstaller WIN32
        src/installer/main.cpp
        ${CRABE_INSTALLER_CORE_SOURCES}
        src/installer/installer.manifest
        "${CRABE_INSTALLER_GEN_DIR}/installer.rc"
    )
    target_include_directories(CrabeInstaller PRIVATE
        include
        include/shared
        src
        "${CRABE_INSTALLER_GEN_DIR}/$<CONFIG>"
    )
    target_compile_definitions(CrabeInstaller PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        UNICODE
        _UNICODE
        _CRT_SECURE_NO_WARNINGS
    )
    if(MSVC)
        target_compile_options(CrabeInstaller PRIVATE /W4 /utf-8)
    else()
        target_compile_options(CrabeInstaller PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    if(CRABE_INSTALLER_EMBED)
        add_dependencies(CrabeInstaller ${PROJECT_NAME})
        add_custom_command(TARGET CrabeInstaller PRE_BUILD
            COMMAND ${CMAKE_COMMAND} -E touch "${CRABE_INSTALLER_GEN_DIR}/installer.rc"
            COMMENT "Refreshing the embedded bink2w32.dll"
        )
    endif()
endif()

add_executable(test_update_checker
    tests/cpp/test_update_checker.cpp
    src/application/update_checker.cpp
    src/domain/update_check.cpp
    src/domain/semver.cpp
    src/domain/config.cpp
    src/shared/logger.cpp
    src/shared/version.cpp
)
target_include_directories(test_update_checker PRIVATE
    include
    include/application
    include/domain
    include/shared
    src
)
target_compile_definitions(test_update_checker PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    _CRT_SECURE_NO_WARNINGS
)
target_link_libraries(test_update_checker PRIVATE
    nlohmann_json::nlohmann_json
    tomlplusplus::tomlplusplus
)
if(MSVC)
    target_compile_options(test_update_checker PRIVATE /W4 /utf-8)
else()
    target_compile_options(test_update_checker PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(update_probe
    tests/cpp/update_probe.cpp
    src/application/update_checker.cpp
    src/infrastructure/update_services.cpp
    src/domain/update_check.cpp
    src/domain/semver.cpp
    src/shared/logger.cpp
    src/shared/version.cpp
)
target_include_directories(update_probe PRIVATE
    include
    include/application
    include/domain
    include/infrastructure
    include/shared
    src
)
target_compile_definitions(update_probe PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    _CRT_SECURE_NO_WARNINGS
)
target_link_libraries(update_probe PRIVATE
    nlohmann_json::nlohmann_json
)
if(MSVC)
    target_compile_options(update_probe PRIVATE /W4 /utf-8)
else()
    target_compile_options(update_probe PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ---------------------------------------------------------------------------
# crabe_lua -- standalone Lua 5.1.5 interpreter used to execute the Lua API
# test suite (tests/lua/run_tests.lua) offline without requiring the game.
# ---------------------------------------------------------------------------
FetchContent_Declare(lua51
    URL https://www.lua.org/ftp/lua-5.1.5.tar.gz
    URL_HASH SHA256=2640fc56a795f29d28ef15e13c34a47e223960b0240e8cb0a82d9b0738695333
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    SOURCE_SUBDIR crabe-does-not-configure-lua
)
FetchContent_MakeAvailable(lua51)

set(CRABELOADER_LUA_SRC_DIR "${lua51_SOURCE_DIR}/src")
file(GLOB CRABELOADER_LUA_SOURCES "${CRABELOADER_LUA_SRC_DIR}/*.c")
list(REMOVE_ITEM CRABELOADER_LUA_SOURCES
    "${CRABELOADER_LUA_SRC_DIR}/lua.c"
    "${CRABELOADER_LUA_SRC_DIR}/luac.c"
    "${CRABELOADER_LUA_SRC_DIR}/print.c"
)

add_library(crabe_lua51 STATIC ${CRABELOADER_LUA_SOURCES})
target_include_directories(crabe_lua51 PUBLIC "${CRABELOADER_LUA_SRC_DIR}")
target_compile_definitions(crabe_lua51 PRIVATE _CRT_SECURE_NO_WARNINGS)
if(MSVC)
    target_compile_options(crabe_lua51 PRIVATE /W0)
else()
    target_compile_options(crabe_lua51 PRIVATE -w)
endif()

add_executable(crabe_lua "${CRABELOADER_LUA_SRC_DIR}/lua.c")
target_link_libraries(crabe_lua PRIVATE crabe_lua51)
target_compile_definitions(crabe_lua PRIVATE _CRT_SECURE_NO_WARNINGS)
if(MSVC)
    target_compile_options(crabe_lua PRIVATE /W0)
else()
    target_compile_options(crabe_lua PRIVATE -w)
endif()

enable_testing()
add_test(NAME test_vfs_override COMMAND test_vfs_override)
add_test(NAME test_draw_primitives COMMAND test_draw_primitives)
add_test(NAME test_telemetry COMMAND test_telemetry)
add_test(NAME test_update_checker COMMAND test_update_checker)
add_test(NAME test_installer COMMAND test_installer)
add_test(NAME crabe_fate COMMAND crabe_fate)

find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_FOUND)
    add_test(NAME test_lua_suite COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/tests/run_lua_tests.py")
endif()


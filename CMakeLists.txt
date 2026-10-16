cmake_minimum_required(VERSION 3.21)

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

# The build label (include/shared/build_info.hpp): restamped at every build, so two
# builds never look alike in the log, the overlay or a crash report. A release
# build (the release workflow) is labelled with the plain version.
option(CRABELOADER_RELEASE_BUILD "Label this build as a release (plain version) instead of a development build." OFF)
set(CRABE_BUILD_STAMP_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(CRABE_BUILD_STAMP_COMMAND
    "${CMAKE_COMMAND}"
    "-DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
    "-DOUTPUT_DIR=${CRABE_BUILD_STAMP_DIR}"
    "-DVERSION=${CRABELOADER_VERSION}"
    "-DRELEASE=${CRABELOADER_RELEASE_BUILD}"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/tools/build_stamp.cmake"
)
execute_process(COMMAND ${CRABE_BUILD_STAMP_COMMAND})
add_custom_target(crabe_build_stamp
    COMMAND ${CRABE_BUILD_STAMP_COMMAND}
    BYPRODUCTS "${CRABE_BUILD_STAMP_DIR}/build_stamp.cpp" "${CRABE_BUILD_STAMP_DIR}/build_stamp.h"
    COMMENT "Stamping the build label"
    VERBATIM
)

option(CRABELOADER_AS_SHARED "Build CrabeLoader as a shared library (DLL)." OFF)
option(CRABELOADER_BUILD_IMGUI "Build bundled Dear ImGui sources/backends." ON)
option(CRABELOADER_BUILD_TESTS "Build the unit test suite." OFF)
option(CRABELOADER_BUILD_CLI "Build crabe-cli, the offline mod-authoring tool." OFF)
option(CRABELOADER_BUILD_INSTALLER "Build CrabeInstaller.exe, the one-click installer." OFF)
option(CRABELOADER_INSTALLER_EMBED_DLL "Embed CrabeLoader.asi and dinput8.dll inside CrabeInstaller.exe (needs CRABELOADER_AS_SHARED)." ON)

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

# Enable multi-processor compilation (/MP) for MSVC
if(MSVC)
    add_compile_options(/MP)
endif()

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

FetchContent_Declare(
    minhook
    URL https://github.com/TsudaKageyu/minhook/archive/refs/tags/v1.3.3.tar.gz
    URL_HASH SHA256=5bec16358ec9086d4593124bf558635e89135abea2c76e5761ecaf09f4546b19
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    SOURCE_SUBDIR crabe-does-not-configure-minhook
)

FetchContent_Declare(
    imgui
    URL https://github.com/ocornut/imgui/archive/refs/tags/v1.90.9.tar.gz
    URL_HASH SHA256=04943919721e874ac75a2f45e6eb6c0224395034667bf508923388afda5a50bf
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    SOURCE_SUBDIR crabe-does-not-configure-imgui
)

FetchContent_MakeAvailable(nlohmann_json tomlplusplus minhook imgui)

# Compatibility include shims for "imgui/imgui.h" and "minhook/MinHook.h"
set(CRABE_INCLUDE_SHIM_DIR "${CMAKE_CURRENT_BINARY_DIR}/include_shims")
file(WRITE "${CRABE_INCLUDE_SHIM_DIR}/imgui/imgui.h" "#pragma once\n#include <imgui.h>\n")
file(WRITE "${CRABE_INCLUDE_SHIM_DIR}/imgui/imgui_internal.h" "#pragma once\n#include <imgui_internal.h>\n")
file(WRITE "${CRABE_INCLUDE_SHIM_DIR}/imgui/imgui_impl_win32.h" "#pragma once\n#include <imgui_impl_win32.h>\n")
file(WRITE "${CRABE_INCLUDE_SHIM_DIR}/imgui/imgui_impl_dx11.h" "#pragma once\n#include <imgui_impl_dx11.h>\n")
file(WRITE "${CRABE_INCLUDE_SHIM_DIR}/minhook/MinHook.h" "#pragma once\n#include <MinHook.h>\n")
file(MAKE_DIRECTORY "${CRABE_INCLUDE_SHIM_DIR}/minhook/hde")
file(WRITE "${CRABE_INCLUDE_SHIM_DIR}/minhook/hde/hde32.h" "#pragma once\n#include \"${minhook_SOURCE_DIR}/src/hde/hde32.h\"\n")
file(WRITE "${CRABE_INCLUDE_SHIM_DIR}/minhook/hde/hde64.h" "#pragma once\n#include \"${minhook_SOURCE_DIR}/src/hde/hde64.h\"\n")

add_library(minhook STATIC
    "${minhook_SOURCE_DIR}/src/buffer.c"
    "${minhook_SOURCE_DIR}/src/hook.c"
    "${minhook_SOURCE_DIR}/src/trampoline.c"
    "${minhook_SOURCE_DIR}/src/hde/hde32.c"
    "${minhook_SOURCE_DIR}/src/hde/hde64.c"
)
target_include_directories(minhook PUBLIC
    "${minhook_SOURCE_DIR}/include"
    "${minhook_SOURCE_DIR}/src"
    "${CRABE_INCLUDE_SHIM_DIR}"
)
if(MSVC)
    target_compile_options(minhook PRIVATE /W0)
else()
    target_compile_options(minhook PRIVATE -w)
endif()

if(CRABELOADER_BUILD_IMGUI)
    add_library(imgui STATIC
        "${imgui_SOURCE_DIR}/imgui.cpp"
        "${imgui_SOURCE_DIR}/imgui_draw.cpp"
        "${imgui_SOURCE_DIR}/imgui_tables.cpp"
        "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
        "${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp"
        "${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp"
    )
    target_include_directories(imgui PUBLIC
        "${imgui_SOURCE_DIR}"
        "${imgui_SOURCE_DIR}/backends"
        "${CRABE_INCLUDE_SHIM_DIR}"
    )
    target_link_libraries(imgui PUBLIC
        d3d11
    )
    if(MSVC)
        target_compile_options(imgui PRIVATE /utf-8 /W0)
    else()
        target_compile_options(imgui PRIVATE -w)
    endif()
endif()


set(CRABELOADER_CORE_SOURCES
    src/main.cpp
    src/version.rc

    # Presentation Layer
    src/presentation/overlay.cpp
    src/presentation/render_hook.cpp
    src/presentation/input_hook.cpp
    src/presentation/pad_state.cpp
    src/presentation/input_remap.cpp
    src/presentation/input_remap_natives.cpp
    src/presentation/dinput_hook.cpp
    src/presentation/imgui_bindings.cpp
    src/presentation/draw_buffer.cpp
    src/presentation/texture_cache.cpp

    # Application Layer
    src/application/loader.cpp
    src/application/loader_content.cpp
    src/application/gateway.cpp
    src/application/lua_runtime.cpp
    src/application/lua_natives.cpp
    src/application/plugin_api.cpp
    src/application/plugin_host.cpp
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
    src/domain/asi_plugins.cpp
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
    src/infrastructure/engine_avatar.cpp
    src/infrastructure/engine_free_camera.cpp
    src/infrastructure/engine_gateway.cpp
    src/infrastructure/engine_kinematics.cpp
    src/infrastructure/engine_meridian.cpp
    src/infrastructure/engine_resolution.cpp
    src/infrastructure/patch/patch_resolution.cpp
    src/infrastructure/patch/patch_borderless.cpp
    src/infrastructure/patch/patch_keybind.cpp
    src/infrastructure/engine_shaders.cpp
    src/infrastructure/engine_asset_loader.cpp
    src/infrastructure/engine_sky.cpp
    src/infrastructure/script_natives.cpp
    src/infrastructure/message_hook.cpp
    src/infrastructure/lua_call.cpp
    src/infrastructure/lua_symbols.cpp
    src/infrastructure/vfs_override_manager.cpp
    src/infrastructure/vfs_hook.cpp
    src/infrastructure/http_redirect_rules.cpp
    src/infrastructure/http_redirect.cpp
    src/infrastructure/update_services.cpp
    src/infrastructure/portal/portal_crypto.cpp
    src/infrastructure/portal/portal_protocol.cpp
    src/infrastructure/portal/portal_transports.cpp
    src/infrastructure/portal/portal_transport_gip.cpp
    src/infrastructure/portal/usb_device.cpp
    src/infrastructure/portal/portal_worker.cpp
    src/infrastructure/portal/portal_service.cpp
    src/infrastructure/loader_instance.cpp
    src/infrastructure/asi_loader.cpp
    src/infrastructure/entry_point_hook.cpp

    # Shared Layer
    src/shared/logger.cpp
    src/shared/version.cpp
    src/shared/keybind_names.cpp
    src/shared/loader_marker.cpp
)

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
)

if(CRABELOADER_AS_SHARED)
    add_library(${PROJECT_NAME} SHARED ${CRABELOADER_SOURCES})
    # The loader is an .asi plugin: the bootstrap dinput8.dll below loads it,
    # and so does any other ASI loader. proxy.hpp keeps the Bink exports, so the
    # same file still works as the bink2w32.dll of a 1.x install.
    set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "CrabeLoader" SUFFIX ".asi")

    # The game imports dinput8.dll and Windows looks in the game folder first:
    # this small DLL answers as dinput8.dll, forwards DirectInput to the system
    # copy, and starts CrabeLoader.asi at the game's entry point.
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/src/bootstrap/bootstrap.rc.in"
        "${CMAKE_CURRENT_BINARY_DIR}/bootstrap/bootstrap.rc"
        @ONLY
    )
    add_library(CrabeBootstrap SHARED
        src/bootstrap/bootstrap_main.cpp
        src/bootstrap/load_loader.cpp
        src/bootstrap/dinput8_proxy.cpp
        "${CMAKE_CURRENT_BINARY_DIR}/bootstrap/bootstrap.rc"
    )
    set_target_properties(CrabeBootstrap PROPERTIES OUTPUT_NAME "dinput8")
    target_include_directories(CrabeBootstrap PRIVATE include "${CRABE_BUILD_STAMP_DIR}")
    add_dependencies(CrabeBootstrap crabe_build_stamp)
    target_compile_definitions(CrabeBootstrap PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
    if(MSVC)
        target_compile_options(CrabeBootstrap PRIVATE /W4 /utf-8)
    endif()
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
        src
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
)

target_sources(${PROJECT_NAME} PRIVATE "${CRABE_BUILD_STAMP_DIR}/build_stamp.cpp")
target_include_directories(${PROJECT_NAME} PRIVATE "${CRABE_BUILD_STAMP_DIR}")
add_dependencies(${PROJECT_NAME} crabe_build_stamp)
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
        hid
        setupapi
        nlohmann_json::nlohmann_json
        tomlplusplus::tomlplusplus
        minhook
)

if(CRABELOADER_BUILD_IMGUI)
    target_link_libraries(${PROJECT_NAME}
        PRIVATE
            imgui
            d3d11
            dxgi
            d3dcompiler
            user32
            gdi32
            winhttp
            ole32
            oleaut32
            windowscodecs
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

add_executable(test_input_remap
    tests/cpp/test_input_remap.cpp
    src/presentation/input_remap.cpp
)
target_include_directories(test_input_remap PRIVATE
    include
)
if(MSVC)
    target_compile_options(test_input_remap PRIVATE /W4 /utf-8)
else()
    target_compile_options(test_input_remap PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(test_http_redirect
    tests/cpp/test_http_redirect.cpp
    src/infrastructure/http_redirect_rules.cpp
)
target_include_directories(test_http_redirect PRIVATE
    include
)
if(MSVC)
    target_compile_options(test_http_redirect PRIVATE /W4 /utf-8)
else()
    target_compile_options(test_http_redirect PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(test_draw_primitives
    tests/cpp/test_draw_primitives.cpp
    src/presentation/draw_buffer.cpp
    src/presentation/pad_state.cpp
)
target_include_directories(test_draw_primitives PRIVATE
    include
)
target_compile_definitions(test_draw_primitives PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    _CRT_SECURE_NO_WARNINGS
)
target_link_libraries(test_draw_primitives PRIVATE
    imgui
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
    src/shared/loader_marker.cpp
)

add_executable(test_installer
    tests/cpp/test_installer.cpp
    ${CRABE_INSTALLER_CORE_SOURCES}
    src/infrastructure/loader_instance.cpp
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

# test_asi_plugins loads three real plugins built here: asi_probe starts and drives the plugin API,
# asi_probe_refuses returns FALSE from DllMain, asi_probe_init_crash faults in CrabePluginInit.
foreach(probe IN ITEMS asi_probe asi_probe_refuses asi_probe_init_crash)
    add_library(${probe} SHARED tests/cpp/asi_probe.cpp)
    target_include_directories(${probe} PRIVATE include)
    set_target_properties(${probe} PROPERTIES SUFFIX ".asi" PREFIX "")
    if(probe STREQUAL "asi_probe_refuses")
        target_compile_definitions(${probe} PRIVATE CRABE_ASI_PROBE_REFUSES=1)
    elseif(probe STREQUAL "asi_probe_init_crash")
        target_compile_definitions(${probe} PRIVATE CRABE_ASI_PROBE_INIT_CRASHES=1)
    endif()
endforeach()

# test_build_info checks the label this very build was stamped with.
add_executable(test_build_info
    tests/cpp/test_build_info.cpp
    "${CRABE_BUILD_STAMP_DIR}/build_stamp.cpp"
    src/shared/version.cpp
)
add_dependencies(test_build_info crabe_build_stamp)
target_include_directories(test_build_info PRIVATE include)
if(MSVC)
    target_compile_options(test_build_info PRIVATE /W4 /utf-8)
else()
    target_compile_options(test_build_info PRIVATE -Wall -Wextra -Wpedantic)
endif()

# test_bootstrap starts a probe as CrabeLoader.asi the way the bootstrap dinput8.dll does,
# and checks every outcome it reports: started, missing, refused, crashed.
add_executable(test_bootstrap
    tests/cpp/test_bootstrap.cpp
    src/bootstrap/load_loader.cpp
    src/shared/loader_marker.cpp
)
add_dependencies(test_bootstrap asi_probe asi_probe_refuses asi_probe_init_crash)
target_include_directories(test_bootstrap PRIVATE include)
target_compile_definitions(test_bootstrap PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
if(MSVC)
    target_compile_options(test_bootstrap PRIVATE /W4 /utf-8)
else()
    target_compile_options(test_bootstrap PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(test_asi_plugins
    tests/cpp/test_asi_plugins.cpp
    src/domain/asi_plugins.cpp
    src/domain/config.cpp
    src/domain/mod_manifest.cpp
    src/domain/mod_id.cpp
    src/domain/semver.cpp
    src/infrastructure/asi_loader.cpp
    src/application/plugin_api.cpp
    src/shared/loader_marker.cpp
    src/shared/logger.cpp
    src/shared/version.cpp
)
add_dependencies(test_asi_plugins asi_probe asi_probe_refuses asi_probe_init_crash)
target_include_directories(test_asi_plugins PRIVATE
    include
    include/domain
    include/shared
    src
)
target_compile_definitions(test_asi_plugins PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    _CRT_SECURE_NO_WARNINGS
)
target_link_libraries(test_asi_plugins PRIVATE
    nlohmann_json::nlohmann_json
    tomlplusplus::tomlplusplus
)
if(MSVC)
    target_compile_options(test_asi_plugins PRIVATE /W4 /utf-8)
else()
    target_compile_options(test_asi_plugins PRIVATE -Wall -Wextra -Wpedantic)
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
            CONTENT "101 RCDATA \"$<TARGET_FILE:${PROJECT_NAME}>\"\n102 RCDATA \"$<TARGET_FILE:CrabeBootstrap>\"\n"
        )
    else()
        file(GENERATE
            OUTPUT "${CRABE_INSTALLER_GEN_DIR}/$<CONFIG>/payload.rc"
            CONTENT "/* no embedded payload: the installer looks for CrabeLoader.asi and dinput8.dll beside itself */\n"
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
        add_dependencies(CrabeInstaller ${PROJECT_NAME} CrabeBootstrap)
        add_custom_command(TARGET CrabeInstaller PRE_BUILD
            COMMAND ${CMAKE_COMMAND} -E touch "${CRABE_INSTALLER_GEN_DIR}/installer.rc"
            COMMENT "Refreshing the embedded CrabeLoader.asi and dinput8.dll"
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

# test_portal_write -- offline checks of the figure progression read/write path.
add_executable(test_portal_write
    tests/cpp/test_portal_write.cpp
    src/infrastructure/portal/portal_crypto.cpp
    src/infrastructure/portal/portal_protocol.cpp
)
target_include_directories(test_portal_write PRIVATE include src)
target_compile_definitions(test_portal_write PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS)
if(MSVC)
    target_compile_options(test_portal_write PRIVATE /W4 /utf-8)
endif()

# portal_probe -- runs the physical portal worker out of the game (hardware bring-up).
add_executable(portal_probe
    tests/cpp/portal_probe.cpp
    src/infrastructure/portal/portal_crypto.cpp
    src/infrastructure/portal/portal_protocol.cpp
    src/infrastructure/portal/portal_transports.cpp
    src/infrastructure/portal/portal_transport_gip.cpp
    src/infrastructure/portal/usb_device.cpp
    src/infrastructure/portal/portal_worker.cpp
    src/shared/logger.cpp
)
target_include_directories(portal_probe PRIVATE include src)
target_compile_definitions(portal_probe PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS)
if(MSVC)
    target_compile_options(portal_probe PRIVATE /W4 /utf-8)
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
add_test(NAME test_input_remap COMMAND test_input_remap)
add_test(NAME test_draw_primitives COMMAND test_draw_primitives)
add_test(NAME test_telemetry COMMAND test_telemetry)
add_test(NAME test_update_checker COMMAND test_update_checker)
add_test(NAME test_installer COMMAND test_installer)
add_test(NAME test_build_info COMMAND test_build_info)
add_test(NAME test_bootstrap COMMAND test_bootstrap $<TARGET_FILE:asi_probe> $<TARGET_FILE:asi_probe_refuses>
    $<TARGET_FILE:asi_probe_init_crash>)
add_test(NAME test_asi_plugins COMMAND test_asi_plugins $<TARGET_FILE:asi_probe> $<TARGET_FILE:asi_probe_refuses>
    $<TARGET_FILE:asi_probe_init_crash>)
add_test(NAME crabe_fate COMMAND crabe_fate)

find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_FOUND)
    add_test(NAME test_lua_suite COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/tests/run_lua_tests.py")
endif()


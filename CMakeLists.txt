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

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Static CRT: the proxy DLL is dropped into a game folder, so it must not
# depend on a VC++ redistributable (debug or release) being installed there.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

set(CRABELOADER_CORE_SOURCES
    src/main.cpp
    src/version.rc

    # Presentation Layer
    src/presentation/overlay.cpp
    src/presentation/render_hook.cpp
    src/presentation/input_hook.cpp
    src/presentation/imgui_bindings.cpp

    # Application Layer
    src/application/Loader.cpp
    src/application/loader_content.cpp
    src/application/lua_runtime.cpp
    src/application/lua_natives.cpp
    src/application/multiplayer/MultiplayerManager.cpp
    src/application/multiplayer/multiplayer_natives.cpp

    # Domain Layer
    src/domain/ModManifest.cpp
    src/domain/ModManager.cpp

    # Infrastructure Layer
    src/infrastructure/crash_handler.cpp
    src/infrastructure/memory.cpp
    src/infrastructure/hook.cpp
    src/infrastructure/codecave.cpp
    src/infrastructure/message_hook.cpp
    src/infrastructure/luacall.cpp
    src/infrastructure/lua_symbols.cpp
    src/infrastructure/multiplayer/MemoryPatcher.cpp
    src/infrastructure/multiplayer/WinHttpRedirector.cpp
    src/infrastructure/multiplayer/FallbackResponseProvider.cpp
    src/infrastructure/multiplayer/UpnpNatService.cpp
    src/infrastructure/multiplayer/SteamworksService.cpp

    # Shared Layer
    src/shared/logger.cpp
    src/shared/version.cpp
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

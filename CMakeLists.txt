cmake_minimum_required(VERSION 3.21)

project(CrabeLoader VERSION 0.1.0 LANGUAGES C CXX)

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
    src/Loader.cpp
    src/loader_content.cpp
    src/lua_symbols.cpp
    src/hook.cpp
    src/luacall.cpp
    src/lua_runtime.cpp
    src/lua_natives.cpp
    src/memory.cpp
    src/codecave.cpp
    src/cheats.cpp
    src/overlay.cpp
    src/menu.cpp
    src/render_hook.cpp
    src/input_hook.cpp
    src/message_hook.cpp
    src/avatar_relay_hook.cpp
    src/gateway.cpp
    src/logger.cpp
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
        include/imgui
        include/minhook
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
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
    )
endif()

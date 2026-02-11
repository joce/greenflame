cmake_minimum_required(VERSION 3.26)
project(greenflame LANGUAGES CXX)

# So the IDE (and clangd) can find includes for all targets, including tests.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Keep the app as a GUI subsystem executable on Windows.
set(CMAKE_WIN32_EXECUTABLE ON)

# -----------------------------
# greenflame_core (testable library)
# -----------------------------
add_library(greenflame_core STATIC
    src/greenflame_core/rect_px.cpp
    src/greenflame_core/rect_px.h
    src/greenflame_core/monitor_rules.cpp
    src/greenflame_core/monitor_rules.h
    src/greenflame_core/pixel_ops.cpp
    src/greenflame_core/pixel_ops.h
    src/greenflame_core/bmp.cpp
    src/greenflame_core/bmp.h
    src/greenflame_core/selection_handles.cpp
    src/greenflame_core/selection_handles.h
    src/greenflame_core/snap_to_edges.cpp
    src/greenflame_core/snap_to_edges.h
)

# Allow includes like: #include "greenflame_core/rect_px.h"
target_include_directories(greenflame_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(greenflame_core PRIVATE
    NOMINMAX
    WIN32_LEAN_AND_MEAN
    UNICODE
    _UNICODE
)

if (MSVC)
    target_compile_options(greenflame_core PRIVATE /W4 /permissive- /Zc:__cplusplus)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(greenflame_core PRIVATE -Wall -Wextra -Wpedantic -ftime-trace)
else()
    target_compile_options(greenflame_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

# -----------------------------
# greenflame (Win32 GUI app)
# -----------------------------
add_executable(greenflame WIN32
    src/greenflame/main.cpp
    src/greenflame/win/cursor.cpp
    src/greenflame/win/cursor.h
    src/greenflame/win/gdi_capture.cpp
    src/greenflame/win/gdi_capture.h
    src/greenflame/win/save_image.cpp
    src/greenflame/win/dpi.cpp
    src/greenflame/win/dpi.h
    src/greenflame/win/monitors_win.cpp
    src/greenflame/win/monitors_win.h
    src/greenflame/win/overlay_paint.cpp
    src/greenflame/win/overlay_paint.h
    src/greenflame/win/overlay_window.cpp
    src/greenflame/win/overlay_window.h
    src/greenflame/win/tray.cpp
    src/greenflame/win/tray.h
    src/greenflame/win/virtual_screen.cpp
    src/greenflame/win/virtual_screen.h
    src/greenflame/win/window_under_cursor.cpp
    src/greenflame/win/window_under_cursor.h
    app.manifest
    greenflame.rc
)

target_include_directories(greenflame PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/greenflame
)

target_compile_definitions(greenflame PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        UNICODE
        _UNICODE
)

if (MSVC)
    target_compile_options(greenflame PRIVATE /W4 /permissive- /Zc:__cplusplus)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(greenflame PRIVATE -Wall -Wextra -Wpedantic -ftime-trace)
else()
    target_compile_options(greenflame PRIVATE -Wall -Wextra -Wpedantic)
endif()

target_link_libraries(greenflame PRIVATE
    greenflame_core

        # Win32 basics
        user32
        gdi32
        shcore
        shell32
        dwmapi
        comdlg32
        Windowscodecs
        ole32
)

# -----------------------------
# Tests
# -----------------------------
include(CTest) # defines BUILD_TESTING option and enables ctest integration
if (BUILD_TESTING)
    add_subdirectory(tests)
endif()

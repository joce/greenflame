# AGENTS — Project Greenflame

This repository builds **greenflame.exe**, a Windows-only screenshot and annotation tool.

Agents interacting with this repo MUST follow the build, test, and architectural rules below.

---

## Mission

Greenflame is a high-correctness screenshot tool designed for:

- mixed-DPI, multi-monitor Windows setups
- capture-first region selection (GDI capture, then overlay to select and crop)
- optional in-place annotation
- optional CLI mode

Correctness (especially DPI and coordinates) takes priority over convenience.

Inspired by:

- Greenshot (capture and multi-monitor support)
- Flameshot (in-place annotation and CLI)
- Microsoft Snipping Tool (multi-monitor selection rules)

---

## Platform & Toolchain (authoritative)

- OS: **Windows 11**
- Compiler: **MSVC (Visual Studio 2026 (18.2.1 or later))**; **Clang (clang-cl)** is supported as an alternative when the Visual Studio "C++ Clang compiler for Windows" component is installed. MSVC remains the primary compiler.
- Build system: **CMake**
- Generator: **Ninja** (preferred) or **Visual Studio**
- Language standard: **C++20**
- IDE/editor: Cursor, Visual Studio, or CLI

---

## Repository Structure (authoritative)

- `src/greenflame/`
    Win32 GUI application (overlay, rendering, capture glue)

- `src/greenflame_core/`
    **Pure, testable core logic** (NO Win32 UI, NO rendering)
  - geometry types (`*Px`, `*Dip`)
  - coordinate conversions
  - selection rules
  - annotation and undo/redo models

- `tests/`
    Unit tests for `greenflame_core` only

---

## How to build (MANDATORY)

### Prerequisites

The following MUST be available:

- Visual Studio 2026 (18.2.1 or later) with *Desktop development with C++*
- Windows 11 SDK
- CMake ≥ 3.26
- Ninja (recommended)

### Configure (from repo root)

```bat
cmake --preset x64-debug
```

### Build

```bat
cmake --build --preset x64-debug
```

### Output

The executable is produced at:

```bat
build\x64-debug\greenflame.exe
```

### Release build

```bat
cmake --preset x64-release
cmake --build --preset x64-release
```

### Clang build (optional)

With the Visual Studio "C++ Clang compiler for Windows" (or "Clang-cl") component installed:

```bat
cmake --preset x64-debug-clang
cmake --build --preset x64-debug-clang
```

Output: `build\x64-debug-clang\greenflame.exe`. Run tests: `ctest --test-dir build\x64-debug-clang`.

For release with Clang: `cmake --preset x64-release-clang` then `cmake --build --preset x64-release-clang`.

### Static analysis and include analysis

- **clang-tidy:** `compile_commands.json` is generated in the build dir (from `CMAKE_EXPORT_COMPILE_COMMANDS ON`). Use the Clang preset build dir so include paths and defines match. Example: `clang-tidy -p build\x64-debug-clang src\greenflame\win\gdi_capture.cpp` (and similarly for other sources under `src\greenflame\` and `src\greenflame_core\`).
- **Include timing:** Clang builds use `-ftime-trace`; the compiler emits `.json` trace files in the build dir. Open them in Chrome’s `chrome://tracing` to inspect time spent in includes and in the compiler.
- **Include What You Use (IWYU)** can use the same `compile_commands.json` for optional include-cleanup suggestions.

---

## Unit Tests (MANDATORY FOR CORE LOGIC)

### Philosophy

- **All testable logic lives in `greenflame_core`**
- The GUI executable (`greenflame`) must remain thin
- Tests MUST NOT depend on Win32 UI, GDI capture, or WGC
- Tests are required for:
  - geometry and math
  - DPI and coordinate conversions
  - cross-monitor selection rules
  - annotation model logic

### Build tests

Tests are enabled via CMake’s standard `BUILD_TESTING` option (ON by default).

```bat
cmake --preset x64-debug
cmake --build --preset x64-debug
```

### Run tests

```bat
ctest --test-dir build\x64-debug
```

If using a multi-config generator (Visual Studio):

```bat
ctest --test-dir build\x64-debug -C Debug
```

---

## Catch2 Testing (authoritative)

### How Catch2 is integrated

- Test framework: **Catch2 v3**
- Integration method: **CMake FetchContent** in `tests/CMakeLists.txt` (tag v3.5.4). First configure (or when the FetchContent cache is missing) **requires network**; request network permission for that configure step in sandboxed/CI environments.
- No global install and no vcpkg requirement
- The test binary is: `greenflame_tests`

Agents must NOT add ad-hoc test runners or alternate test frameworks.

### Where to add tests

- Add new test files under `tests/`
- Register them in `tests/CMakeLists.txt` as sources of `greenflame_tests`
- Tests must only link against `greenflame_core` (never `greenflame`)

### Writing a test

Use Catch2 v3 macros.

Example structure:

```cpp
# include <catch2/catch_test_macros.hpp>

TEST_CASE("something important")
{
    REQUIRE(1 + 1 == 2);
}
```

### Running tests directly

You can run the test executable (useful for filters):

```bat
build\x64-debug\greenflame_tests.exe
```

Run a subset via Catch2 filters:

```bat
build\x64-debug\greenflame_tests.exe "RectPx*"
```

Prefer `ctest` for standard runs; use direct execution for local filtering.

---

## Rendering & Capture Stack (authoritative)

- **Capture:** GDI (desktop DC, CreateDIBSection, BitBlt with CAPTUREBLT) for full virtual-desktop capture. Capture happens once before the overlay is shown (capture-first pattern).
- **Overlay:** Fullscreen borderless topmost window; drawing via GDI (e.g. BitBlt of captured bitmap, then dim and selection rect). No Direct3D, DirectComposition, or Direct2D in the current path.
- **Future:** Windows Graphics Capture (WGC) may be added later for capture; overlay may remain GDI-based.

---

## DPI & Coordinate Rules (critical)

- Process is **Per-Monitor DPI Aware v2**
- Internal coordinate truth is **physical pixels**
- Explicit types MUST be used:
  - `*Px` for physical pixels
  - `*Dip` for device-independent pixels
- No implicit conversions
- DPI conversion happens exactly once, explicitly

Breaking these rules is considered a correctness bug.

---

## Notes for Agents

- **Indent with 4 spaces (no tabs).**
- Do not introduce third-party libraries without explicit justification
- Do not bypass or “simplify” DPI logic
- Do not move testable logic into the GUI executable
- Prefer failing loudly over silently doing the wrong thing
- When unsure, preserve correctness over convenience
- Your main instinct should be to debug issues and find problems' root causes rather than introduce new abstractions or code paths, or changing the architecture or existing code.

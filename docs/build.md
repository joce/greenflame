---
title: Build Guide
summary: Configure and build Greenflame with MSVC and Clang presets.
audience: contributors
status: authoritative
owners:
  - core-team
last_updated: 2026-02-21
tags:
  - build
  - cmake
  - msvc
  - clang
---

# Build Guide

This document is authoritative for configuring and building Greenflame.

## Prerequisites

The following MUST be available:

- Visual Studio 2026 (18.2.1 or later) with *Desktop development with C++*
- Windows 11 SDK
- CMake >= 3.26
- Ninja (recommended)

## Configure (from repo root)

```bat
cmake --preset x64-debug
```

## Build

```bat
cmake --build --preset x64-debug
```

## Output

The executable is produced at:

```bat
build\x64-debug\greenflame.exe
```

## Release build

```bat
cmake --preset x64-release
cmake --build --preset x64-release
```

## Clang build

With the Visual Studio "C++ Clang compiler for Windows" (or "Clang-cl") component installed:

```bat
cmake --preset x64-debug-clang
cmake --build --preset x64-debug-clang
```

Output: `build\x64-debug-clang\greenflame.exe`.

For release with Clang:

```bat
cmake --preset x64-release-clang
cmake --build --preset x64-release-clang
```

## Static analysis and include analysis (non-mandatory)

- **clang-tidy:** `compile_commands.json` is generated in the build dir (from `CMAKE_EXPORT_COMPILE_COMMANDS ON`). Use the Clang preset build dir so include paths and defines match. Example: `clang-tidy -p build\x64-debug-clang src\greenflame\win\gdi_capture.cpp` (and similarly for other sources under `src\greenflame\` and `src\greenflame_core\`).
- **Include timing:** Clang builds use `-ftime-trace`; the compiler emits `.json` trace files in the build dir. Open them in Chrome's `chrome://tracing` to inspect time spent in includes and in the compiler.
- **Include What You Use (IWYU)** can use the same `compile_commands.json` for optional include-cleanup suggestions.

## Formatting and lint policy before finishing a task

When a working version is reached and behavior is acceptable:

1. Run **clang-format** on changed C/C++ files.
2. During iteration, run **clang-tidy** on changed translation units (`.cpp` files).
3. Before considering the task complete, run **clang-tidy on all translation units** in `src/` and `tests/`.

Rationale: a change in one file (especially headers, inline code, templates, macros, or shared declarations) can surface a tidy warning in another file that was not directly modified.

## Completeness and correctness

- Code iteration can be done on the MSVC debug build only.
- However, all builds (debug and release) must be run on all compilers (MSVC and Clang) and must pass before any task is considered complete. This is a hard requirement.

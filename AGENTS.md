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

## Build & Test Execution (MANDATORY)

Build and test commands have been moved to docs:

- [docs/build.md](docs/build.md) for configure/build/analysis/lint expectations
- [docs/testing.md](docs/testing.md) for test build/run commands and Catch2 usage

These docs are authoritative for execution steps and command lines.

Before considering a task complete:

- all required builds in [docs/build.md](docs/build.md) must pass
- all required test runs in [docs/testing.md](docs/testing.md) must pass

---

## Unit Test Policy (MANDATORY FOR CORE LOGIC)

- **All testable logic lives in `greenflame_core`**
- The GUI executable (`greenflame`) must remain thin
- Tests MUST NOT depend on Win32 UI, GDI capture, or WGC
- Tests are required for:
  - geometry and math
  - DPI and coordinate conversions
  - cross-monitor selection rules
  - annotation model logic
- Use Catch2 v3 integration defined in `tests/CMakeLists.txt`
- Do not add ad-hoc test runners or alternate test frameworks
- Add new test files under `tests/`
- Register them in `tests/CMakeLists.txt` as sources of `greenflame_tests`
- Tests must only link against `greenflame_core` (never `greenflame`)

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
- **Prefer forward declarations over `#include` in header files.** Avoid including headers in other headers whenever possible; use forward declarations for types that are only used as pointers or references. Include the full header only in the `.cpp` that needs the definition.
- **Do not include standard library headers or Windows headers directly in non-PCH files.** If a new standard/Win32 header is needed, add it to the appropriate precompiled header (`src/greenflame/pch.h`, `src/greenflame_core/pch.h`, or `tests/pch.h`) and consume it through PCH.
- **Prefer OOP for Win32/UI-side code when ownership or lifecycle boundaries are non-trivial.** Keep API surfaces small and cohesive; separate responsibilities into focused objects instead of accumulating procedural/global state.
- Do not introduce third-party libraries without explicit justification
- Do not bypass or “simplify” DPI logic
- Do not move testable logic into the GUI executable
- Prefer failing loudly over silently doing the wrong thing
- When unsure, preserve correctness over convenience
- Your main instinct should be to debug issues and find problems' root causes rather than introduce new abstractions or code paths, or changing the architecture or existing code.

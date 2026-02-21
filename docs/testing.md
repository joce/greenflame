---
title: Testing Guide
summary: Build and run unit tests for greenflame_core with Catch2.
audience: contributors
status: authoritative
owners:
  - core-team
last_updated: 2026-02-21
tags:
  - tests
  - ctest
  - catch2
---

# Testing Guide

This document is authoritative for building and running tests in Greenflame.

## Unit test philosophy

- **All testable logic lives in `greenflame_core`**
- The GUI executable (`greenflame`) must remain thin
- Tests MUST NOT depend on Win32 UI, GDI capture, or WGC
- Tests are required for:
  - geometry and math
  - DPI and coordinate conversions
  - cross-monitor selection rules
  - annotation model logic

## Build tests

Tests are enabled via CMake's standard `BUILD_TESTING` option (ON by default).

```bat
cmake --preset x64-debug
cmake --build --preset x64-debug
```

## Run tests

```bat
ctest --test-dir build\x64-debug
```

If using a multi-config generator (Visual Studio):

```bat
ctest --test-dir build\x64-debug -C Debug
```

For Clang debug builds:

```bat
ctest --test-dir build\x64-debug-clang
```

Test must be run and must pass before any task is considered complete. This is a hard requirement.

## Catch2 integration (authoritative)

- Test framework: **Catch2 v3**
- Integration method: **CMake FetchContent** in `tests/CMakeLists.txt` (tag v3.5.4). First configure (or when the FetchContent cache is missing) **requires network**; request network permission for that configure step in sandboxed/CI environments.
- No global install and no vcpkg requirement
- The test binary is: `greenflame_tests`

Do not add ad-hoc test runners or alternate test frameworks.

## Where to add tests

- Add new test files under `tests/`
- Register them in `tests/CMakeLists.txt` as sources of `greenflame_tests`
- Tests must only link against `greenflame_core` (never `greenflame`)

## Writing a test

Use Catch2 v3 macros.

```cpp
# include <catch2/catch_test_macros.hpp>

TEST_CASE("something important")
{
    REQUIRE(1 + 1 == 2);
}
```

## Running tests directly

You can run the test executable (useful for filters):

```bat
build\x64-debug\greenflame_tests.exe
```

Run a subset via Catch2 filters:

```bat
build\x64-debug\greenflame_tests.exe "RectPx*"
```

Prefer `ctest` for standard runs; use direct execution for local filtering.

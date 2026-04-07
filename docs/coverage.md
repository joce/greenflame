---
title: Coverage Guide
summary: How to measure LLVM source coverage for greenflame_core.
audience: contributors
status: authoritative
owners:
  - core-team
last_updated: 2026-04-06
tags:
  - coverage
  - llvm-cov
  - clang
---

# Coverage Guide

This document describes how to measure source coverage for `greenflame_core`
using LLVM's instrumentation-based coverage (`llvm-cov`).

Coverage is a diagnostic tool, not a gate. It helps identify untested paths in
`greenflame_core`. It is not required before considering a task complete.

---

## Prerequisites

In addition to the standard build prerequisites in [build.md](build.md):

- **clang-cl.exe** — provided by the Visual Studio 2026 "C++ Clang compiler
  for Windows" component (already required for the `x64-debug-clang` preset).
- **llvm-profdata.exe** and **llvm-cov.exe** — these ship with the VS
  "C++ Clang compiler for Windows" component at:
  ```
  <VS install>\VC\Tools\Llvm\x64\bin\
  ```
  `scripts\coverage.ps1` adds this directory to `PATH` automatically when
  `VSINSTALLDIR` is set (i.e. when run from a VS Developer Command Prompt or
  after running `VsDevCmd.bat`). In a plain shell, set `VSINSTALLDIR` or add
  the directory to `PATH` manually.

---

## Running coverage

A helper script handles the full workflow:

```powershell
.\scripts\coverage.ps1
```

The script:
1. Configures with the `x64-coverage-clang` CMake preset (Clang + coverage
   instrumentation enabled).
2. Builds `greenflame_tests`.
3. Runs the test binary with `LLVM_PROFILE_FILE` set to capture raw profile
   data.
4. Merges the raw profile with `llvm-profdata`.
5. Prints a summary coverage table scoped to `src\greenflame_core\`.
6. Writes an HTML line-level report to
   `build\x64-coverage-clang\coverage\report\index.html`.

To open the HTML report after the run:

```powershell
start build\x64-coverage-clang\coverage\report\index.html
```

---

## How it works

The `x64-coverage-clang` CMake preset sets `GREENFLAME_ENABLE_COVERAGE=ON`.
When enabled, CMake adds the following flags to `greenflame_core` and
`greenflame_tests`:

| Target              | Compile flags                                        | Link flags                    |
|---------------------|------------------------------------------------------|-------------------------------|
| `greenflame_core`   | `-fprofile-instr-generate -fcoverage-mapping`        | —                             |
| `greenflame_tests`  | `-fprofile-instr-generate -fcoverage-mapping`        | `-fprofile-instr-generate`    |

Because `greenflame_core` is a static library, its instrumented object files
are linked into `greenflame_tests.exe`, which is the binary passed to
`llvm-cov`.

---

## Manual workflow

If you prefer to run the steps yourself:

```powershell
# 1. Configure and build
cmake --preset x64-coverage-clang
cmake --build --preset x64-coverage-clang

# 2. Run tests (generates coverage.profraw)
$env:LLVM_PROFILE_FILE = "$PWD\build\x64-coverage-clang\coverage\coverage.profraw"
.\build\x64-coverage-clang\bin\greenflame_tests.exe
Remove-Item Env:\LLVM_PROFILE_FILE

# 3. Merge profile data
llvm-profdata merge -sparse `
    build\x64-coverage-clang\coverage\coverage.profraw `
    -o build\x64-coverage-clang\coverage\coverage.profdata

# 4. Summary report (scoped to greenflame_core)
llvm-cov report `
    .\build\x64-coverage-clang\bin\greenflame_tests.exe `
    "--instr-profile=build\x64-coverage-clang\coverage\coverage.profdata" `
    src\greenflame_core

# 5. HTML report
llvm-cov show `
    .\build\x64-coverage-clang\bin\greenflame_tests.exe `
    "--instr-profile=build\x64-coverage-clang\coverage\coverage.profdata" `
    "--output-dir=build\x64-coverage-clang\coverage\report" `
    -format=html `
    src\greenflame_core
```

---

## Scope

The report is scoped to `src\greenflame_core\`. GoogleTest internals and test
source files under `tests\` are excluded from the summary by passing the core
source directory as a positional filter to `llvm-cov`.

Coverage of `src\greenflame\` (the Win32 GUI layer) is intentionally excluded:
that code cannot run in the headless test binary and is not a target for
automated coverage.

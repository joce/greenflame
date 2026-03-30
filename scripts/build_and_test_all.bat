@echo off
setlocal EnableDelayedExpansion

cd /d "%~dp0\.."

set "FAILED="
set PRESETS=x64-debug x64-release x64-debug-clang x64-release-clang

for %%P in (%PRESETS%) do (
    set "SKIP=0"

    echo.
    echo ============================================================
    echo  Configure: %%P
    echo ============================================================
    cmake --preset %%P
    if errorlevel 1 (
        echo [FAIL] Configure failed for %%P
        set "FAILED=!FAILED! %%P(configure)"
        set "SKIP=1"
    )

    if "!SKIP!"=="0" (
        echo.
        echo ============================================================
        echo  Build: %%P
        echo ============================================================
        cmake --build --preset %%P
        if errorlevel 1 (
            echo [FAIL] Build failed for %%P
            set "FAILED=!FAILED! %%P(build)"
            set "SKIP=1"
        )
    )

    if "!SKIP!"=="0" (
        echo.
        echo ============================================================
        echo  Test: %%P
        echo ============================================================
        ctest --test-dir build\%%P --output-on-failure
        if errorlevel 1 (
            echo [FAIL] Tests failed for %%P
            set "FAILED=!FAILED! %%P(test)"
        )
    )
)

echo.
echo ============================================================
if defined FAILED (
    echo  RESULT: FAILED --!FAILED!
    echo ============================================================
    exit /b 1
) else (
    echo  RESULT: ALL PRESETS PASSED
    echo ============================================================
    exit /b 0
)

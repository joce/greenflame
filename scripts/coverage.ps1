<#
.SYNOPSIS
    Build greenflame_core with LLVM coverage instrumentation, run tests, and
    produce a coverage report scoped to src/greenflame_core/.

.DESCRIPTION
    Requires:
      - clang-cl.exe, llvm-profdata.exe, and llvm-cov.exe
        These ship with the Visual Studio "C++ Clang compiler for Windows"
        component at:
          <VS install>\VC\Tools\Llvm\x64\bin\
        The script adds that directory to PATH automatically when $env:VSINSTALLDIR
        is set (i.e. when run from a VS Developer Command Prompt or after
        invoking VsDevCmd.bat). If you run from a plain shell, either set
        VSINSTALLDIR or add the Llvm\x64\bin directory to PATH manually.

    Produces (all under build\x64-coverage-clang\coverage\):
      - coverage.profraw   raw instrumentation profile
      - coverage.profdata  merged profile
      - report\            HTML line-level coverage report
      A summary table scoped to src/greenflame_core/ is printed to stdout.

.EXAMPLE
    # From the repo root:
    .\scripts\coverage.ps1

    # Open the HTML report after the script finishes:
    start build\x64-coverage-clang\coverage\report\index.html
#>

$ErrorActionPreference = "Stop"

# ── resolve repo root ──────────────────────────────────────────────────────────
$RepoRoot = Split-Path -Parent $PSScriptRoot
Set-Location $RepoRoot

# ── add VS LLVM bin to PATH if needed ─────────────────────────────────────────
# llvm-cov and llvm-profdata ship with the VS "C++ Clang" component at:
#   <VS install>\VC\Tools\Llvm\x64\bin\
# Prepend that directory when $env:VSINSTALLDIR is set and the tools aren't
# already on PATH.
if ($env:VSINSTALLDIR -and
    -not (Get-Command "llvm-cov" -ErrorAction SilentlyContinue)) {
    $vsLlvmBin = Join-Path $env:VSINSTALLDIR "VC\Tools\Llvm\x64\bin"
    if (Test-Path $vsLlvmBin) {
        $env:PATH = "$vsLlvmBin;$env:PATH"
    }
}

# ── verify required tools ──────────────────────────────────────────────────────
$missing = @()
foreach ($tool in @("cmake", "ninja", "llvm-profdata", "llvm-cov")) {
    if (-not (Get-Command $tool -ErrorAction SilentlyContinue)) {
        $missing += $tool
    }
}
if ($missing.Count -gt 0) {
    Write-Error @"
The following tools were not found on PATH: $($missing -join ', ')
See docs/coverage.md for setup instructions.
"@
}

# ── configure ─────────────────────────────────────────────────────────────────
Write-Host "=== Configure (x64-coverage-clang) ==="
cmake --preset x64-coverage-clang
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

# ── build ─────────────────────────────────────────────────────────────────────
Write-Host ""
Write-Host "=== Build (x64-coverage-clang) ==="
cmake --build --preset x64-coverage-clang
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

# ── paths ─────────────────────────────────────────────────────────────────────
$TestExe  = "build\x64-coverage-clang\bin\greenflame_tests.exe"
$CovDir   = "build\x64-coverage-clang\coverage"
$ProfRaw  = "$CovDir\coverage.profraw"
$ProfData = "$CovDir\coverage.profdata"
$HtmlDir  = "$CovDir\report"
$CoreSrc  = "src\greenflame_core"

New-Item -ItemType Directory -Force -Path $CovDir | Out-Null

# ── run tests with LLVM instrumentation ───────────────────────────────────────
Write-Host ""
Write-Host "=== Run tests ==="
$env:LLVM_PROFILE_FILE = (Resolve-Path -LiteralPath $CovDir).Path + "\coverage.profraw"
& $TestExe
$TestExitCode = $LASTEXITCODE
Remove-Item Env:\LLVM_PROFILE_FILE -ErrorAction SilentlyContinue

if ($TestExitCode -ne 0) {
    Write-Warning "Tests reported failures (exit $TestExitCode). Coverage data was still captured."
}

# ── merge profile data ────────────────────────────────────────────────────────
Write-Host ""
Write-Host "=== Merge profile data ==="
llvm-profdata merge -sparse $ProfRaw -o $ProfData
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

# ── summary report (stdout, scoped to greenflame_core) ────────────────────────
Write-Host ""
Write-Host "=== Coverage summary (src\greenflame_core) ==="
llvm-cov report $TestExe "--instr-profile=$ProfData" $CoreSrc
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

# ── HTML report ───────────────────────────────────────────────────────────────
Write-Host ""
Write-Host "=== HTML report -> $HtmlDir ==="
llvm-cov show $TestExe `
    "--instr-profile=$ProfData" `
    "--output-dir=$HtmlDir" `
    -format=html `
    $CoreSrc
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host ""
Write-Host "Done. Open: $HtmlDir\index.html"
exit $TestExitCode

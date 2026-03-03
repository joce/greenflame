# Pre-commit hook: auto-format staged C/C++ files with clang-format.
#
# Files are reformatted in place and re-staged so the commit always
# contains correctly formatted code. The hook never blocks a commit.
#
# Invoked by pre-commit.bat — do not run this script directly.

if (-not (Get-Command clang-format -ErrorAction SilentlyContinue)) {
    Write-Host "[pre-commit] clang-format not found in PATH — skipping auto-format."
    exit 0
}

$files = @(
    git diff --cached --name-only --diff-filter=d 2>$null |
        Where-Object { $_ -match '\.(cpp|h)$' }
)

if ($files.Count -eq 0) {
    exit 0
}

Write-Host "[pre-commit] clang-format: formatting $($files.Count) file(s)..."

foreach ($file in $files) {
    clang-format -i $file
    git add $file
}

Write-Host "[pre-commit] clang-format: done."

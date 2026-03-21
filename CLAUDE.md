Make sure to respect @AGENTS.md clauses.

Also treat the repository `.clang-tidy` naming rules as mandatory when adding or renaming identifiers; do not wait for nightly `clang-tidy` to catch violations. In particular, constants MUST follow `readability-identifier-naming`: local `const`/`constexpr` variables use `lower_case`, namespace-scope/file-static/static-member/`inline constexpr` constants use `kCamelCase`, and enum constants use unprefixed `CamelCase`. `tests/.clang-tidy` inherits the same naming policy, so test code has no naming exception.

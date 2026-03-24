# Linting and Static Analysis

## Goal
Provide a consistent C++ style and static analysis process for the ResonanceEQ VST3 audio plugin (JUCE + CMake). This helps maintain readability, reliability, and performance in real-time audio code.

## Tools selected
- `clang-format`: automatic code formatting according to a shared style
- `clang-tidy`: static analysis and modern C++ checks
- `cppcheck`: additional static analysis focusing on correctness and potential runtime issues

## Why these tools
- `clang-format` is standard for C++ formatting and integrates with IDEs and CI.
- `clang-tidy` has extensive checks for performance, modern C++ practices, bug patterns, and JUCE-friendly code.
- `cppcheck` provides extra coverage for undefined behavior, null dereference, and other safety issues.

## Configuration files
- `.clang-format`: style settings
- `.clang-tidy`: check list and filters

## Core quality areas
- Style consistency: enforce via `clang-format`
- Readability: limit line length, clear whitespace, include ordering
- Maintainability: avoid legacy constructs, use modern C++ (e.g. `auto`, `constexpr`)
- Reliability: parameter clamping, bounds checking in DSP code
- Performance awareness: in audio code avoid heap allocations in `processBlock`
- Modern C++: `override`, `nullptr`, range-based loops, `const` correctness

## Ignored directories/files for linting
- `build/`, `**/build/`, `**/*_artefacts/`
- `JUCE/` submodule (vendor code)
- `.git/`, IDE metadata (`.vscode/`, `.vs/`)
- generated build artifacts and intermediate files

## How to run
### clang-format
From repository root:
```bat
clang-format -i --style=file Source/**/*.cpp Source/**/*.h
```
or check without modifying:
```bat
clang-format --dry-run --Werror --style=file Source/**/*.cpp Source/**/*.h
```

### clang-tidy
Build first if necessary:
```bat
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022"
cmake --build . --config Release
```
Then:
```bat
cd build
clang-tidy -p . "../Source/**/*.cpp" --warnings-as-errors=* --config-file=../.clang-tidy
```

### cppcheck
```bat
cppcheck --enable=warning,performance,portability --std=c++17 --project=build/compile_commands.json --template=gcc Source
```

### Full check script
- `scripts/run_checks.bat`
- `scripts/run_checks.ps1`

## Pre-commit hooks
1. Configure Git hooks path:
   ```bat
git config core.hooksPath .githooks
```
2. The hook file `.githooks/pre-commit` runs format check, `clang-tidy`, and optional `cppcheck`.

## Build system integration
- `CMakeLists.txt` sets:
  - `CMAKE_EXPORT_COMPILE_COMMANDS ON`
  - optional `CMAKE_CXX_CLANG_TIDY` when `clang-tidy` is present
  - optional `cppcheck` target

## Static typing and analysis
C++ is a statically typed language by design; no separate tools like mypy/TypeScript apply here. Static analysis is performed by:
- `clang-tidy` for code correctness, potential undefined behavior, and C++ core guidelines.
- `cppcheck` for additional runtime and style warning coverage.

## Running full quality checks
From repository root:
```bat
scripts\run_checks.bat
```
or PowerShell:
```powershell
scripts\run_checks.ps1
```

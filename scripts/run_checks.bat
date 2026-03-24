@echo off
setlocal enabledelayedexpansion

echo Running full code quality checks for ResonanceEQ...

set "BUILD_DIR=build-ninja"

if not exist "%BUILD_DIR%" (
    echo ERROR: build directory not found.
    echo Run: cmake -S . -B build
    exit /b 1
)

where clang-format >nul 2>nul
if errorlevel 1 (
    echo ERROR: clang-format not found
    exit /b 1
)

where clang-tidy >nul 2>nul
if errorlevel 1 (
    echo ERROR: clang-tidy not found
    exit /b 1
)

where cppcheck >nul 2>nul
if errorlevel 1 (
    echo WARNING: cppcheck not found, skipping
    set SKIP_CPPCHECK=1
)

echo 1/3: Running clang-format
for %%f in (Source\*.cpp Source\*.h Source\Dsp\*.cpp Source\Dsp\*.h) do (
    if exist "%%f" (
        echo Checking %%f
        clang-format --dry-run --Werror "%%f"
        if errorlevel 1 (
            echo Format error in %%f
            exit /b 1
        )
    )
)

echo 2/3: Running clang-tidy
for %%f in (Source\*.cpp Source\Dsp\*.cpp) do (
    if exist "%%f" (
        echo Analyzing %%f
        clang-tidy "%%f" -p "%BUILD_DIR%"
        if errorlevel 1 (
            echo Tidy error in %%f
            exit /b 1
        )
    )
)

if not defined SKIP_CPPCHECK (
    echo 3/3: Running cppcheck
    cppcheck --enable=warning,style,performance,portability --std=c++17 Source
)

echo All checks passed!
exit /b 0
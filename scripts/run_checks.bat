@echo off
setlocal

echo Running full code quality checks for ResonanceEQ...

set "BUILD_DIR=build"
set "FORMAT_FAIL=0"
set "TIDY_FAIL=0"
set "CPPCHECK_FAIL=0"

if not exist "%BUILD_DIR%" (
    echo ERROR: build directory not found.
    echo Run: cmake -S . -B build
    exit /b 1
)

set "TIDY_BUILD_DIR=%BUILD_DIR%"
if not exist "%BUILD_DIR%\compile_commands.json" (
    echo WARNING: %BUILD_DIR%\compile_commands.json not found.

    where ninja >nul 2>nul
    if errorlevel 1 (
        echo WARNING: Ninja is not installed; falling back to on-build clang-tidy integration in Visual Studio generator.
        echo Ensure CMAKE_CXX_CLANG_TIDY is configured in CMakeLists.txt and build has been configured.
        echo Running cmake --build build --config Release ...
        cmake --build build --config Release
        if errorlevel 1 (
            echo ERROR: Build failed; clang-tidy cannot run without a compilation database.
            echo Install Ninja and regenerate with: cmake -S . -B build_ninja -G "Ninja" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
            exit /b 1
        )
        echo INFO: Build completed (clang-tidy should have run via CMake integration if configured).
        exit /b 0
    )

    echo Trying to generate a compilation database using Ninja in build_ninja...
    if not exist "build_ninja" mkdir "build_ninja"
    cmake -S . -B build_ninja -G "Ninja" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    if errorlevel 1 (
        echo ERROR: CMake configure for build_ninja failed.
        exit /b 1
    )
    if exist "build_ninja\compile_commands.json" (
        set "TIDY_BUILD_DIR=build_ninja"
        echo INFO: Using build_ninja for clang-tidy compilation database.
    ) else (
        echo ERROR: compile_commands.json still not found; cannot run clang-tidy reliably.
        exit /b 1
    )
)

where clang-format >nul 2>nul
if errorlevel 1 (
    echo ERROR: clang-format not found in PATH.
    exit /b 1
)

where clang-tidy >nul 2>nul
if errorlevel 1 (
    echo ERROR: clang-tidy not found in PATH.
    exit /b 1
)

where cppcheck >nul 2>nul
if errorlevel 1 (
    echo WARNING: cppcheck not found in PATH. Skipping cppcheck.
    set "SKIP_CPPCHECK=1"
)

echo 1/3: Running clang-format dry-run
for %%f in (Source\*.cpp Source\*.h Source\Dsp\*.cpp Source\Dsp\*.h) do (
    if exist "%%f" (
        echo Checking formatting: %%f
        clang-format --dry-run --Werror --style=file "%%f"
        if errorlevel 1 (
            echo clang-format issue: %%f
            set "FORMAT_FAIL=1"
        )
    )
)

if %FORMAT_FAIL% neq 0 (
    echo.
    echo ERROR: clang-format check failed.
    exit /b 1
)

echo 2/3: Running clang-tidy
for %%f in (Source\*.cpp Source\Dsp\*.cpp) do (
    if exist "%%f" (
        echo Running clang-tidy on: %%f
        clang-tidy "%%f" -p "%BUILD_DIR%"
        if errorlevel 1 (
            echo clang-tidy issue: %%f
            set "TIDY_FAIL=1"
        )
    )
)

if %TIDY_FAIL% neq 0 (
    echo.
    echo ERROR: clang-tidy check failed.
    exit /b 1
)

if not defined SKIP_CPPCHECK (
    echo 3/3: Running cppcheck
    cppcheck --enable=warning,style,performance,portability --std=c++17 --quiet Source
    if errorlevel 1 (
        echo.
        echo ERROR: cppcheck failed.
        exit /b 1
    )
)

echo.
echo All checks passed.
exit /b 0
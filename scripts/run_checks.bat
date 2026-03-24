@echo off
setlocal enabledelayedexpansion

set PROJECT_ROOT=%~dp0..
set BUILD_DIR=%PROJECT_ROOT%build
set INCLUDE_DIRS=Source Source\Dsp

echo Running full code quality checks for ResonanceEQ...

REM clang-format check
where clang-format >nul 2>nul
if %ERRORLEVEL% neq 0 (
    echo ERROR: clang-format not found in PATH.
    exit /b 1
)

echo 1/3: Running clang-format dry-run
set FORMAT_FAIL=0
for %%d in (%INCLUDE_DIRS%) do (
    for /R %%d %%f in (*.cpp *.h) do (
        clang-format --dry-run --Werror --style=file "%%f"
        if %%ERRORLEVEL%% neq 0 (
            set FORMAT_FAIL=1
            echo clang-format issue: %%f
        )
    )
)

if %FORMAT_FAIL% neq 0 (
    echo clang-format check failed.
    exit /b 1
)

echo 2/3: Running clang-tidy
where clang-tidy >nul 2>nul
if %ERRORLEVEL% neq 0 (
    echo WARNING: clang-tidy not found in PATH. Skipping clang-tidy step.
) else (
    if not exist "%BUILD_DIR%\compile_commands.json" (
        echo compile_commands.json not found. Building project to generate it...
        mkdir "%BUILD_DIR%" 2>nul
        pushd "%BUILD_DIR%"
        cmake .. -G "Visual Studio 17 2022"
        popd
    )

    if not exist "%BUILD_DIR%\compile_commands.json" (
        echo ERROR: compile_commands.json still missing after CMake configuration.
        exit /b 1
    )

    set TIDY_FAIL=0
    for %%f in (Source\*.cpp Source\Dsp\*.cpp) do (
        clang-tidy "%%f" -p "%BUILD_DIR%" --export-fixes="%BUILD_DIR%\clang-tidy-fixes.yaml" --quiet
        if %%ERRORLEVEL%% neq 0 (
            set TIDY_FAIL=1
            echo clang-tidy issue: %%f
        )
    )
    if %TIDY_FAIL% neq 0 (
        echo clang-tidy check failed.
        exit /b 1
    )
)

echo 3/3: Running cppcheck
where cppcheck >nul 2>nul
if %ERRORLEVEL% neq 0 (
    echo WARNING: cppcheck not found in PATH. Skipping cppcheck step.
) else (
    cppcheck --enable=warning,performance,portability --std=c++17 --project="%BUILD_DIR%\compile_commands.json" --template=gcc Source 2> "%BUILD_DIR%\cppcheck.log"
    if %ERRORLEVEL% neq 0 (
        echo cppcheck found issues. See %BUILD_DIR%\cppcheck.log
        exit /b 1
    )
)

echo Code quality checks passed.
exit /b 0

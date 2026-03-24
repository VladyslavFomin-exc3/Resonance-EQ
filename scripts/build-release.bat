@echo off
set BUILD_DIR=build

if not exist %BUILD_DIR% mkdir %BUILD_DIR%

cd %BUILD_DIR%

cmake .. -G "Visual Studio 17 2022"

if %errorlevel% neq 0 (
    echo CMake configuration failed
    exit /b 1
)

cmake --build . --config Release

if %errorlevel% neq 0 (
    echo Build failed
    exit /b 1
)

echo Release build completed successfully
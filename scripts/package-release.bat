@echo off
set PLUGIN_NAME="Harmonic Resonance EQ"
set RELEASE_DIR=release
set SOURCE_DIR="build\ResonanceEQ_artefacts\Release\VST3\%PLUGIN_NAME%.vst3"

if not exist %SOURCE_DIR% (
    echo Release plugin not found
    exit /b 1
)

if exist %RELEASE_DIR% rmdir /s /q %RELEASE_DIR%

mkdir %RELEASE_DIR%

xcopy /e /i /y %SOURCE_DIR% %RELEASE_DIR%\%PLUGIN_NAME%.vst3\

if exist README.md copy README.md %RELEASE_DIR%

if exist LICENSE copy LICENSE %RELEASE_DIR%

echo Release package created in %RELEASE_DIR%
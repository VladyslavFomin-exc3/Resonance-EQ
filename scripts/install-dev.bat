@echo off
set PLUGIN_NAME="Harmonic Resonance EQ"
set VST3_DIR="C:\Program Files\Common Files\VST3"
set SOURCE_DIR="build\ResonanceEQ_artefacts\Debug\VST3\%PLUGIN_NAME%.vst3"

if not exist %SOURCE_DIR% (
    echo Debug plugin not found at %SOURCE_DIR%
    exit /b 1
)

if not exist %VST3_DIR% mkdir %VST3_DIR%

xcopy /e /i /y %SOURCE_DIR% %VST3_DIR%\%PLUGIN_NAME%.vst3\

if %errorlevel% neq 0 (
    echo Installation failed
    exit /b 1
)

echo Development plugin installed successfully
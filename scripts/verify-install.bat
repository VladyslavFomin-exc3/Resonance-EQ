@echo off
set PLUGIN_NAME="Harmonic Resonance EQ"
set VST3_DIR="C:\Program Files\Common Files\VST3"
set TARGET_DIR=%VST3_DIR%\%PLUGIN_NAME%.vst3

if exist %TARGET_DIR% (
    echo Plugin is installed at %TARGET_DIR%
    exit /b 0
) else (
    echo Plugin is not installed
    exit /b 1
)
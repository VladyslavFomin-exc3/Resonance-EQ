@echo off
set PLUGIN_NAME="Harmonic Resonance EQ"
set VST3_DIR="C:\Program Files\Common Files\VST3"
set TARGET_DIR=%VST3_DIR%\%PLUGIN_NAME%.vst3

if exist %TARGET_DIR% (
    rmdir /s /q %TARGET_DIR%
    echo Plugin uninstalled successfully
) else (
    echo Plugin not found
)
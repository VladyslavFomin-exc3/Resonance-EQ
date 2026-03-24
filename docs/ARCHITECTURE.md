# Architecture

## Overview

ResonanceEQ is a VST3 audio plugin built with JUCE framework. It implements spectral equalization with random resonance features.

## Plugin-Host Model

The plugin operates within a host application (DAW) that provides:

- Audio I/O interfaces
- User interface hosting
- Parameter automation
- Preset management

The plugin provides:

- Real-time audio processing
- Custom user interface
- Parameter controls

## Key Components

- **PluginProcessor**: Handles audio processing and parameter management
- **PluginEditor**: Provides the graphical user interface
- **DSP Modules**: EqCurve, ResonanceEngine, SafetyLimiter for audio algorithms

## Build Toolchain

- CMake for build configuration
- Visual Studio 2022 MSVC for compilation
- JUCE for cross-platform audio plugin framework

## Source Organization

- `Source/`: Main source files
- `Source/Dsp/`: Digital signal processing algorithms

## Output

Plugin binary: `build/ResonanceEQ_artefacts/{Debug|Release}/VST3/Harmonic Resonance EQ.vst3`

## Dependencies

- JUCE framework (submodule)
- Windows SDK
- No external runtime dependencies

## Non-Applicable Components

- Web server: Not applicable - this is a desktop plugin
- Application server: Not applicable - runs in DAW host
- Database: Not applicable - no data persistence
- File storage service: Not applicable - local file system only
- Cache service: Not applicable - real-time processing

## Architecture Diagram

```
[DAW Host Application]
    |
    | (VST3 Interface)
    v
[ResonanceEQ Plugin]
    ├── PluginProcessor (Audio Processing)
    ├── PluginEditor (UI)
    └── DSP Modules
        ├── EqCurve
        ├── ResonanceEngine
        └── SafetyLimiter
```
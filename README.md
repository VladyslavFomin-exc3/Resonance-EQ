# ResonanceEQ

VST3 audio plugin for spectral equalization with a random resonance engine.

## Architecture

This is a VST3 audio plugin that runs within a host application (DAW). The plugin processes audio in real-time and provides a user interface for parameter control.

## Requirements

- Git
- CMake (version 3.15 or later)
- Visual Studio 2022 with C++ development tools
- JUCE framework (included as submodule)

## Setup

1. Clone the repository:
   ```
   git clone https://github.com/your-repo/ResonanceEQ.git
   cd ResonanceEQ
   ```

2. Initialize submodules:
   ```
   git submodule update --init --recursive
   ```

3. Create build directory and configure:
   ```
   mkdir build
   cd build
   cmake .. -G "Visual Studio 17 2022"
   ```

4. Build the project:
   ```
   cmake --build . --config Release
   ```

## Build Instructions

### Release Build
```
cmake --build build --config Release
```

### Debug Build
```
cmake --build build --config Debug
```

## Output Location

The built VST3 plugin is located at:
- Release: `build/ResonanceEQ_artefacts/Release/VST3/Harmonic Resonance EQ.vst3`
- Debug: `build/ResonanceEQ_artefacts/Debug/VST3/Harmonic Resonance EQ.vst3`

## Installation

1. Copy the `.vst3` file to the VST3 plugin directory:
   ```
   C:\Program Files\Common Files\VST3
   ```

2. Restart your DAW if it was open.

## Running the Plugin

1. Open your DAW (e.g., Ableton Live, FL Studio).
2. Create a new track or open an existing project.
3. Add the ResonanceEQ plugin to the track's effects chain.
4. The plugin interface will appear, allowing you to adjust the spectral EQ parameters.

## Commands

- Build Release: `cmake --build build --config Release`
- Build Debug: `cmake --build build --config Debug`
- Clean: `cmake --build build --target clean`

## Repository Structure

- `CMakeLists.txt`: Main build configuration
- `Source/`: Plugin source code
  - `PluginProcessor.cpp/h`: Audio processing logic
  - `PluginEditor.cpp/h`: User interface
  - `Dsp/`: DSP algorithms
- `JUCE/`: JUCE framework submodule
- `build/`: Build artifacts (ignored)
- `docs/`: Documentation
- `scripts/`: Automation scripts
- `website/`: Project website

## Troubleshooting

### Build fails
- Ensure Visual Studio 2022 is installed with C++ workload
- Check CMake version >= 3.22
- Run `git submodule update --init --recursive`

### Plugin not found in DAW
- Verify installation path: `C:\Program Files\Common Files\VST3`
- Rescan plugins in DAW
- Check for .vst3 file existence

### Audio issues
- Ensure sample rate compatibility
- Check buffer sizes

## Notes

- No database is required for this audio plugin.
- No backend system is required; the plugin runs entirely within the host DAW.

# Deployment

## Hardware Requirements

- Windows PC (64-bit)
- Minimum 4GB RAM
- Sufficient storage for build artifacts

## Software Requirements

- Windows 10 or later
- Visual Studio 2022
- CMake 3.15+
- JUCE framework

## Deployment Steps

1. Build the project in Release configuration:
   ```
   cmake --build build --config Release
   ```

2. Locate the VST3 plugin file:
   ```
   build/ResonanceEQ_artefacts/Release/VST3/ResonanceEQ.vst3
   ```

3. Copy the `.vst3` file to the VST3 plugin directory:
   ```
   C:\Program Files\Common Files\VST3
   ```

4. Restart the DAW if running.

## Health Check

1. Open the DAW.
2. Scan for new plugins or refresh the plugin list.
3. Add ResonanceEQ to a track.
4. Verify the plugin loads and the interface appears.

No network configuration is required.
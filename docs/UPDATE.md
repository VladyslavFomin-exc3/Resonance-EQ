# Update

## Preparation

1. Close the DAW completely.
2. Ensure no audio is playing through the plugin.

## Backup

1. Locate the current plugin file:
   ```
   C:\Program Files\Common Files\VST3\Harmonic Resonance EQ.vst3
   ```

2. Copy the file to a backup location:
   ```
   copy "C:\Program Files\Common Files\VST3\Harmonic Resonance EQ.vst3" "C:\Backup\ResonanceEQ_backup.vst3"
   ```

## Update Steps

1. Pull the latest code:
   ```
   git pull
   git submodule update --init --recursive
   ```

2. Build the new version:
   ```
   cmake --build build --config Release
   ```

3. Replace the plugin file:
   ```
   copy "build\ResonanceEQ_artefacts\Release\VST3\Harmonic Resonance EQ.vst3" "C:\Program Files\Common Files\VST3\Harmonic Resonance EQ.vst3"
   ```

## Downtime

The DAW must be closed during the update process. Expected downtime: 2-5 minutes.

## Data Migration

Not applicable - plugin settings and presets are managed by the DAW host application.

## Configuration Update

Plugin parameters and presets are stored within DAW projects. No additional configuration update required.

## Post-Update Verification

1. Open the DAW.
2. Load a project using ResonanceEQ.
3. Verify the plugin functions correctly.

## Rollback Procedure

1. Close the DAW.
2. Restore the backup:
   ```
   copy "C:\Backup\ResonanceEQ_backup.vst3" "C:\Program Files\Common Files\VST3\Harmonic Resonance EQ.vst3"
   ```
3. Restart the DAW.

## Risks and Mitigation

- **Plugin incompatibility**: Test in development environment first
- **DAW crash**: Have backup of project files
- **Audio artifacts**: Verify in multiple DAWs
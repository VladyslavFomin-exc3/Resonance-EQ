# Backup

## Backup Strategy

Regular backups of the plugin binary ensure quick recovery in case of issues.

## Types

- Full backup: Complete `.vst3` plugin file

## Frequency

- Before each release or major update
- After successful builds

## Storage

- Local: Store backups in a dedicated folder (e.g., `C:\Backup\Plugins\`)
- Remote: Upload to GitHub releases for versioned backups

## Restore Procedure

1. Close the DAW.
2. Copy the backup `.vst3` file to the VST3 directory:
   ```
   C:\Program Files\Common Files\VST3
   ```
3. Restart the DAW.
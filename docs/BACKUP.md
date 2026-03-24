# Backup

## Backup Strategy

Regular backups of the plugin binary and related assets ensure quick recovery in case of issues.

## Types

- **Full backup**: Complete plugin binary and release packages
- **Incremental backup**: Not applicable - plugin is a single binary
- **Differential backup**: Not applicable - plugin is a single binary

## Frequency

- Before each release or major update
- After successful builds
- Weekly for active development

## Retention / Rotation

- Keep last 5 versions
- Archive old versions to separate storage
- Delete backups older than 1 year

## Backup of:

- **Plugin binary**: `.vst3` file from build artifacts
- **Release packages**: Packaged distributions
- **Configuration files**: Build scripts and settings
- **User presets**: DAW project files containing plugin settings (external to plugin)
- **Logs**: Build logs and CI/CD outputs

## Integrity Verification

- Verify file checksums after backup
- Test plugin loading after restore
- Check file sizes match originals

## Automation Options

- Use provided scripts for consistent backups
- Integrate with CI/CD pipeline for automatic artifact storage
- Schedule regular backups using Windows Task Scheduler

## Full Restore Procedure

1. Close the DAW.
2. Copy backup `.vst3` file to VST3 directory:
   ```
   C:\Program Files\Common Files\VST3
   ```
3. Restart the DAW.
4. Rescan plugins.

## Selective Restore Procedure

1. For plugin binary only: Follow full restore procedure.
2. For presets: Restore DAW project files from backup.
3. For configuration: Restore build scripts and settings files.
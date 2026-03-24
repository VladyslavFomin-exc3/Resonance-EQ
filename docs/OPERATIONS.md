# Operations

## Build Operations

### Clean Build
```
scripts\clean.bat
scripts\build-release.bat
```

### Debug Build
```
scripts\build-debug.bat
```

### Release Build
```
scripts\build-release.bat
```

## Installation

### Development Install
```
scripts\install-dev.bat
```

### Release Install
```
scripts\install-release.bat
```

## Removal

### Uninstall Plugin
```
scripts\uninstall-plugin.bat
```

## Packaging

### Create Release Package
```
scripts\package-release.bat
```

## Verification

### Verify Installation
```
scripts\verify-install.bat
```

## Maintenance Tasks

- Rescan plugins in DAW after installation
- Clean build directory periodically
- Update JUCE submodule for latest features
- Monitor build logs for warnings
- Test plugin in multiple DAWs

## Common Locations

- Build artifacts: `build\ResonanceEQ_artefacts\`
- VST3 folder: `C:\Program Files\Common Files\VST3`
- Source code: `Source\`
- Scripts: `scripts\`
- Documentation: `docs\`
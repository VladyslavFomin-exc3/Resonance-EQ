# CI/CD

## GitHub Actions Pipeline

The project uses GitHub Actions for continuous integration and automated releases.

### build.yml

- **Triggers**: Push to any branch, pull requests
- **Platform**: Windows latest
- **Steps**:
  - Checkout code with submodules
  - Install CMake
  - Build Debug configuration
  - Build Release configuration
  - Upload build artifacts

### release-package.yml

- **Triggers**: Push of version tags (e.g., v1.0.0)
- **Platform**: Windows latest
- **Steps**:
  - Checkout code with submodules
  - Install CMake
  - Build Release configuration
  - Package plugin with documentation
  - Upload release artifact

## Usage

### For Developers
- Push commits to trigger automatic builds
- Check the Actions tab for build status and logs
- Download build artifacts for testing

### For Release Engineers
- Create a git tag to trigger release packaging:
  ```
  git tag v1.0.0
  git push origin v1.0.0
  ```
- Download the release artifact from the Actions workflow
- The artifact contains the packaged plugin ready for distribution

## Validation

The CI/CD pipeline validates:
- Code compilation in both Debug and Release modes
- CMake configuration success
- Build artifact generation
- Plugin packaging integrity
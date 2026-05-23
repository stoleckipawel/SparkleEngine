# Sparkle Launcher GUI Phase 5 Release

## Release Scope
Phase 5 performs the first executable Qt launcher build, packages the runtime files for depot use, and removes the legacy user-facing console launcher target. The shared launcher core remains because the GUI uses it directly for native workflow planning and execution.

## Build Inputs
- Configuration: `DevelopmentEditor`
- Qt SDK: local Qt 6.8.3 MSVC x64 SDK under `build/Qt/6.8.3/msvc2022_64`
- CMake Qt binding: `Qt6_DIR=build/Qt/6.8.3/msvc2022_64/lib/cmake/Qt6`
- Built target: `SparkleLauncher`

## Legacy Console Removal
- Removed the `Sparkle` console executable target from `Tools/Launcher/SparkleLauncher/CMakeLists.txt`.
- Removed `Tools/Launcher/SparkleLauncher/Source/SparkleMain.cpp`.
- Removed the launcher-specific `Tools/Launcher/SparkleLauncher/Private/Cli` implementation.
- Removed stale generated console build artifacts from the active build/package output.

## Package Output
Packaged output is generated under:

```text
build/Package/SparkleLauncher/DevelopmentEditor
```

The package contains `SparkleLauncher.exe`, Qt runtime DLLs/plugins, MSVC runtime DLLs, and sibling tool binaries needed by launcher workflows. The legacy `Sparkle.exe` console executable is not included.

## Validation Results
- CMake regenerated successfully with the local Qt SDK.
- `SparkleLauncher` built successfully as a Windows GUI executable.
- Qt runtime deployment completed with `windeployqt`.
- MSVC runtime DLLs were copied into the package from the Visual Studio redistributable directory.
- Packaged launch smoke passed: the packaged `SparkleLauncher.exe` reached input idle, accepted a close request, and exited with code 0.

## Follow-Up Tracking
- User feedback should be tracked as launcher issues against the Qt GUI surface, not against the removed console app.
- Future packaging automation can promote the manual package steps into a CMake install/package target once the depot layout is finalized.
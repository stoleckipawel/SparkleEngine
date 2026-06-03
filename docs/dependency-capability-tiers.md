# Sparkle Dependency Capability Tiers

Sparkle separates ready-to-run product artifacts from local source rebuild dependencies. A reviewer should be able to launch a packaged Sparkle build first, then opt into rebuilding or recooking only when they want to inspect or modify the source pipeline.

## Tier 0: Ready-To-Run Runtime Package

Purpose:

- launch `SparkleLauncher.exe` from a release package
- run packaged sample editor/runtime binaries
- use pre-cooked sample assets

Expected user action:

- unzip or sync a release package
- launch the package-root launcher

Required local source dependencies:

- none

Notes:

- packaged Qt runtime files, sample binaries, cooked assets, manifests, checksums, and licenses are product artifacts
- rebuilding and recooking remain optional developer workflows

## Tier 1: Source Launcher And Core Workspace

Purpose:

- configure the workspace from source
- build the launcher, core engine/editor/runtime targets, and basic development artifacts

Host prerequisites:

- Visual Studio C++ tools with Windows SDK
- CMake
- Git
- Qt 6 MSVC x64 kit for rebuilding `SparkleLauncher`

Source dependency group:

- `Core Workspace Source Tier`

CMake options:

```cmake
SPARKLE_ENABLE_CONTENT_PIPELINE=OFF
SPARKLE_ENABLE_SHADER_COMPILER=OFF
SPARKLE_ENABLE_KTX_SUPPORT=OFF
```

Notes:

- host tools are verified by `Verify Host Environment`
- syncable source dependencies are managed by `Sync Source Dependencies`
- Git and Qt can be supplied by PATH, explicit environment/cache variables, or standard installer locations

## Tier 2: Content Pipeline

Purpose:

- build cook tools for source scene import, mesh cooking, and texture cooking
- regenerate pre-cooked sample content locally

Additional source dependency group:

- `Content Pipeline Source Tier`

CMake option:

```cmake
SPARKLE_ENABLE_CONTENT_PIPELINE=ON
```

Unlocks:

- `Build Cook Tools`
- `Cook Scene Assets`
- `Cook Textures`
- content portions of `Cook All`

## Tier 3: Shader Toolchain

Purpose:

- build offline shader compiler tooling
- regenerate cooked shader packages locally

Additional source dependency group:

- `Shader Compiler Source Tier`

CMake option:

```cmake
SPARKLE_ENABLE_SHADER_COMPILER=ON
```

Unlocks:

- `Build Cook Tools`
- `Cook Shaders`
- shader portions of `Cook All`

## Tier 4: KTX Container Support

Purpose:

- enable optional KTX2 texture container workflows

Additional source dependency group:

- `KTX Container Source Tier`

CMake option:

```cmake
SPARKLE_ENABLE_KTX_SUPPORT=ON
```

Notes:

- this tier is intentionally disabled by default
- it should be presented as an optional texture capability, not a universal setup requirement

## Discovery And Override Contract

Sparkle should never require a user-specific machine path. Discovery follows this order:

- explicit override variables such as `SPARKLE_QT_ROOT`, `SPARKLE_GIT_EXE`, `SPARKLE_CMAKE_EXE`, `SPARKLE_MSBUILD_EXE`, and `SPARKLE_VSWHERE_EXE`
- standard environment variables such as `QTDIR`, `Qt6_DIR`, and `CMAKE_PREFIX_PATH`
- standard Windows installer locations such as `C:/Qt`, `%ProgramFiles%/Git`, `%ProgramFiles%/CMake`, and `%ProgramFiles(x86)%/Microsoft Visual Studio/Installer/vswhere.exe`

The launcher should show missing optional tiers as capability unlocks, not as launch blockers for ready-to-run package usage.

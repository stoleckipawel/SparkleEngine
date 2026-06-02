# Optional Dependency Groups

SparkleEngine now supports a smaller first-run dependency surface by splitting large third-party and tool stacks into optional groups.

## CMake Options

Use these cache options at configure time:

```powershell
-DSPARKLE_ENABLE_CONTENT_PIPELINE=ON|OFF
-DSPARKLE_ENABLE_SHADER_COMPILER=ON|OFF
-DSPARKLE_ENABLE_KTX_SUPPORT=ON|OFF
```

Defaults:

- `SPARKLE_ENABLE_CONTENT_PIPELINE=ON`
- `SPARKLE_ENABLE_SHADER_COMPILER=ON`
- `SPARKLE_ENABLE_KTX_SUPPORT=OFF`

## What Each Group Controls

`SPARKLE_ENABLE_CONTENT_PIPELINE`

- Enables source import, conversion, and asset/texture cooking tools.
- Builds:
  - `SourceImportAdapters`
  - `MeshCooker`
  - `MaterialCooker`
  - `SceneCooker`
  - `TextureCooker`
  - `AssetCooker`
  - `AssetConverter`
- Fetches:
  - `cgltf`
  - `stb`
  - `tinyexr`
  - `zlib`
  - `assimp`
  - `Compressonator`

`SPARKLE_ENABLE_SHADER_COMPILER`

- Enables the offline shader compiler toolchain.
- Builds:
  - `ShaderCompiler`
- Fetches:
  - `SPIRV-Reflect`
- Still expects local DXC/Slang SDK inputs where required by the shader toolchain.

`SPARKLE_ENABLE_KTX_SUPPORT`

- Enables KTX support fetch/build when needed.
- Fetches:
  - `KTX-Software`
- Default is `OFF` because it is not part of the smallest supported first-run path.

## Recommended Profiles

Minimal launcher/editor-oriented setup:

```powershell
cmake -S . -B build-lite `
  -DSPARKLE_ENABLE_CONTENT_PIPELINE=OFF `
  -DSPARKLE_ENABLE_SHADER_COMPILER=OFF `
  -DSPARKLE_ENABLE_KTX_SUPPORT=OFF
```

Full local development setup:

```powershell
cmake -S . -B build `
  -DSPARKLE_ENABLE_CONTENT_PIPELINE=ON `
  -DSPARKLE_ENABLE_SHADER_COMPILER=ON `
  -DSPARKLE_ENABLE_KTX_SUPPORT=OFF
```

## Launcher Behavior

The launcher now reflects these groups more accurately:

- dependency tracking only shows enabled optional groups
- `Build Cook Tools` only builds enabled cook/shader tools
- cook workflows report when the required optional tool group is disabled

This keeps the default workflow intact while making it much easier to offer lighter sync/build paths or precompiled tool bundles later.

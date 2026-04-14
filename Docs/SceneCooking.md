# Asset Cooking Workflow

Use the repo script below as the normal way to prepare authored engine and project scene assets into Sparkle cooked outputs.

Run it from the repository root.

## Cook Assets

```bat
Scripts\CookAssets.bat <ProjectName> [Debug|Release|RelWithDebInfo]
```

Examples:

```bat
Scripts\CookAssets.bat Showcase
Scripts\CookAssets.bat Showcase Release
```

`CookAssets.bat` recursively cooks every supported scene under both of these roots:

- `Engine\Assets\Meshes\`
- `Projects\<ProjectName>\Assets\Meshes\`

Supported source scene extensions are `.gltf`, `.glb`, and `.fbx`.
If the same relative scene path exists in both roots, the project scene overrides the engine scene.

## Inputs

- `ProjectName` is the project directory under `Projects\`.
- Source scenes are discovered automatically under the engine and project mesh roots.
- Source textures are resolved from the project or engine asset roots during conversion.

Typical source locations:

- `Engine\Assets\Meshes\...`
- `Projects\<ProjectName>\Assets\Meshes\...`
- `Projects\<ProjectName>\Assets\Textures\...`

## What The Script Does

- Validates required build tools.
- Refreshes the build files and syncs missing third-party dependencies through the configure flow.
- Builds the `AssetConverter` target.
- Runs `AssetConverter` from the selected project root so project asset discovery works correctly.
- Recursively cooks every supported scene in the engine and selected project mesh roots.
- Writes all cooked outputs into the selected project's cooked asset tree.

## Outputs

Cooked content is written under:

```text
Projects\<ProjectName>\Assets\Cooked\
```

Asset families are emitted to:

- `Projects\<ProjectName>\Assets\Cooked\SceneManifests\`
- `Projects\<ProjectName>\Assets\Cooked\Meshes\`
- `Projects\<ProjectName>\Assets\Cooked\Materials\`
- `Projects\<ProjectName>\Assets\Cooked\Textures\`

The scene manifest path mirrors the cooked scene asset id under `SceneManifests` with the `.sscn` extension.
That asset id is derived from the scene's relative path under either the engine mesh root or the project mesh root.

Example:

```text
Source: Projects\Showcase\Assets\Meshes\Sponza\Sponza.gltf
Cooked manifest: Projects\Showcase\Assets\Cooked\SceneManifests\Sponza\Sponza.sscn
```

## Notes

- This workflow is explicit and manual by design. Runtime does not auto-cook.
- `CookAssets.bat` is the intended contributor entry point for cooking scene assets in v1.
- If configure fails before the converter builds, fix the repo's normal CMake/dependency issues first and rerun the same command.
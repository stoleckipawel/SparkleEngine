# Scene Cooking Workflow

Use the repo scripts below as the normal way to cook authored scenes into Sparkle cooked outputs.

Run it from the repository root.

## Cook One Scene

```bat
Scripts\CookScene.bat <ProjectName> <ScenePath> [Debug|Release|RelWithDebInfo]
```

Examples:

```bat
Scripts\CookScene.bat Showcase Sponza\Sponza.gltf
Scripts\CookScene.bat Showcase Bistro\BistroExterior.fbx Release
```

If you run `CookScene.bat` without `ProjectName` and `ScenePath`, it will fail intentionally and print the required usage.

## Cook All Scenes In A Project

```bat
Scripts\CookAll.bat <ProjectName> [Debug|Release|RelWithDebInfo]
```

Example:

```bat
Scripts\CookAll.bat Showcase Debug
```

`CookAll.bat` recursively cooks every supported scene under `Projects\<ProjectName>\Assets\Meshes\`.
Supported source scene extensions are `.gltf`, `.glb`, and `.fbx`.

## Inputs

- `ProjectName` is the project directory under `Projects\`.
- `ScenePath` is relative to `Projects\<ProjectName>\Assets\Meshes\`.
- Source textures are resolved from the project or engine asset roots during conversion.

Typical source locations:

- `Projects\<ProjectName>\Assets\Meshes\...`
- `Projects\<ProjectName>\Assets\Textures\...`

## What The Script Does

- Validates required build tools.
- Confirms third-party dependencies are available.
- Generates the solution if it does not exist yet.
- Builds the `AssetConverter` target.
- Runs `AssetConverter` from the selected project root so project asset discovery works correctly.
- `CookScene.bat` cooks one selected scene.
- `CookAll.bat` recursively cooks every supported scene in the project mesh root.

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

The scene manifest path mirrors the source scene path under `SceneManifests` with the `.sscn` extension.

Example:

```text
Source: Projects\Showcase\Assets\Meshes\Sponza\Sponza.gltf
Cooked manifest: Projects\Showcase\Assets\Cooked\SceneManifests\Sponza\Sponza.sscn
```

## Notes

- This workflow is explicit and manual by design. Runtime does not auto-cook.
- `CookScene.bat` and `CookAll.bat` are the intended contributor entry points for scene cooking in v1.
- If configure fails before the converter builds, fix the repo's normal CMake/dependency issues first and rerun the same command.
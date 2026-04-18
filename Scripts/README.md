# Scripts

This folder is the contributor-facing command surface for SparkleEngine.

If you are trying to figure out what to run for what, start here. If a file is under `Scripts/Internal`, it is not a user entrypoint.

## Command Surface

| Command | Purpose | Typical Use |
| --- | --- | --- |
| `SetupWorkspace.bat` | First-time bootstrap | Fresh clone, or when you want the normal first-run setup path |
| `GenerateSolution.bat` | Advanced build-tree refresh | Usually only after CMake changes, project creation, or when the build tree is stale |
| `BuildProject.bat` | Build one project's editor/runtime launch targets | Build `ShowcaseEditor`, `ShowcaseRuntime`, or both for one selected project |
| `CookAllAssets.bat` | Preferred full cook entrypoint | Run the full shader, texture, and scene cook flow for a project from the top-level `Scripts/` folder |
| `Cook/CookShaders.bat` | Cook shader packages for a project | Validate the merged shader manifest and emit cooked shader packages plus the registry |
| `Cook/CookTextures.bat` | Cook texture assets for a project | Collect texture requests from source scenes and emit cooked texture assets |
| `Cook/CookAssets.bat` | Cook scene assets for a project | Batch-convert supported source scenes into cooked scene manifests, mesh assets, and material assets |
| `RunClangFormat.bat` | Run `clang-format` | Apply repo formatting to `Engine/` and `Projects/` sources |
| `CleanWorkspace.bat` | Remove generated artifacts | Clear build outputs, third-party cache, or return to the tracked repo state |

## Quick Workflow Guide

- First-time setup: run `Scripts\SetupWorkspace.bat`
- Build one project: run `Scripts\BuildProject.bat Showcase Both Debug`
- Run the full cook from the top-level scripts folder: run `Scripts\CookAllAssets.bat Showcase Debug`
- Cook shaders for a project: run `Scripts\Cook\CookShaders.bat Showcase Debug`
- Cook textures for a project: run `Scripts\Cook\CookTextures.bat Showcase Debug`
- Cook scene assets for a project: run `Scripts\Cook\CookAssets.bat Showcase Debug`
- Format source files: run `Scripts\RunClangFormat.bat`
- Clean generated artifacts: run `Scripts\CleanWorkspace.bat`

## Structure

- `Scripts/` contains user-facing workflow commands.
- `Scripts/Cook/` contains user-facing cook commands for shader-only, texture-only, and full asset cooking flows.
- `Scripts/Internal/` contains reusable helper modules used by the public commands. Do not treat these as stable user entrypoints.
- `CMake/Dependencies/` contains configure-time dependency modules.
- `CMake/Validation/` contains build-time validation modules such as runtime boundary checks.

## Notes

- `BuildProject.bat` understands the split project targets introduced by the current host model: `<Project>Editor` and `<Project>Runtime`.
- `GenerateSolution.bat` is the single public owner of generator/toolset selection and incremental CMake configure behavior, but most users will reach it indirectly through the higher-level commands.
- Toolchain validation and dependency repair helpers live under `Scripts/Internal` and are invoked through the public workflow scripts.
- `CookAllAssets.bat` is the preferred single-file full cook command in the top-level `Scripts/` folder.
- `Scripts\Cook\CookShaders.bat` validates the merged shader manifest and emits cooked shader packages required for normal runtime startup.
- `Scripts\Cook\CookTextures.bat` enumerates supported source scenes, collects texture cook requests, and emits cooked texture assets.
- `Scripts\Cook\CookAssets.bat` cooks only scene manifests plus cooked mesh/material outputs. Use `CookAllAssets.bat` for the full shader + texture + scene pipeline.
- `CleanWorkspace.bat PRISTINE` removes generated outputs only. It does not delete tracked project assets such as committed Showcase cooked content under `Projects/Showcase/Assets/Cooked`.
- Runtime boundary validation is a CMake target, not a user-run batch command. It lives under `CMake/Validation/` and runs as part of the engine build wiring.
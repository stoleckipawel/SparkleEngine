# Scripts

This folder is the contributor-facing command surface for SparkleEngine.

If you are trying to figure out what to run for what, start here. If a file is under `Scripts/Internal`, it is not a user entrypoint.

## Command Surface

| Command | Purpose | Typical Use |
| --- | --- | --- |
| `SetupWorkspace.bat` | First-time bootstrap | Fresh clone, or when you want the normal first-run setup path |
| `GenerateSolution.bat` | Advanced build-tree refresh | Usually only after CMake changes, project creation, or when the build tree is stale |
| `BuildProject.bat` | Build one project's editor/runtime launch targets | Build `ShowcaseEditor`, `ShowcaseRuntime`, or both for one selected project |
| `CookAllAssets.bat` | Preferred full cook entrypoint | Prepare `AssetCooker` and forward a full project cook request |
| `Cook/CookShaders.bat` | Cook shader packages for a project | Prepare `AssetCooker` and forward the shader cook request |
| `Cook/CookTextures.bat` | Cook texture assets for a project | Prepare `AssetCooker` and forward the texture cook request |
| `Cook/CookAssets.bat` | Cook scene assets for a project | Prepare `AssetCooker` and forward the scene, mesh, and material cook request |
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

## Architecture Notes

- [../docs/architecture/shader-system-design.md](../docs/architecture/shader-system-design.md) is the current shader-system architecture baseline, including the compiler tool, Renderer shader orchestration, RHI backend realization, and no-runtime-compiler invariant.

## Logs

Generated logs are grouped by intent instead of being written directly into the repository root or the root of `logs/`:

- `logs/Projects/<Project>/Full/` contains one full runtime/editor activity log per launched project process, such as `ShowcaseEditor`.
- `logs/Prerequisites/ShaderCompilationLog/<Project>/` contains shader package cooking logs.
- `logs/Prerequisites/TextureCookingLog/<Project>/` contains texture cooking logs.
- `logs/Prerequisites/AssetCookingLog/<Project>/` contains scene, mesh, and material cooking logs.
- Other prerequisite workflows use similarly named folders, such as `BuildLog`, `SolutionGenerationLog`, `ToolchainCheckLog`, `FormatCheckLog`, and workspace setup/cleanup logs.

Each prerequisite folder keeps a timestamped log plus `Latest.txt` for that specific action/scope.

## Notes

- `BuildProject.bat` understands the split project targets introduced by the current host model: `<Project>Editor` and `<Project>Runtime`.
- `GenerateSolution.bat` is the single public owner of generator/toolset selection and incremental CMake configure behavior, but most users will reach it indirectly through the higher-level commands.
- Toolchain validation and dependency repair helpers live under `Scripts/Internal` and are invoked through the public workflow scripts.
- `CookAllAssets.bat` is the preferred single-file full cook command in the top-level `Scripts/` folder and delegates project planning to `AssetCooker`.
- `Scripts\Cook\CookShaders.bat`, `Scripts\Cook\CookTextures.bat`, and `Scripts\Cook\CookAssets.bat` are narrow launch shims over `AssetCooker` category requests.
- `AssetCooker` discovers supported scenes, builds the Phase 2 project cook plan, dispatches the focused tools, and aggregates diagnostics at the tool boundary.
- Build outputs and cooked assets are generated under `build/`; runtime/editor and prerequisite logs are generated under the structured `logs/` hierarchy.
- `CleanWorkspace.bat PRISTINE` removes generated outputs only. It does not delete tracked project assets.
- Runtime boundary validation is a CMake target, not a user-run batch command. It lives under `CMake/Validation/` and runs as part of the engine build wiring.
- The old Phase 1H validation wrapper scripts were removed. Use the documented direct smoke launch steps in `docs/plans/phase1h-d3d12-production-readiness.md` if you still need that evidence flow.
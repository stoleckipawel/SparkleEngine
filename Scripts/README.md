# Scripts

This folder is the contributor-facing command surface for SparkleEngine.

If you are trying to figure out what to run for what, start here. If a file is under `Scripts/Internal`, it is not a user entrypoint.

## Command Surface

| Command | Purpose | Typical Use |
| --- | --- | --- |
| `Setup.bat` | First-time bootstrap | Fresh clone, or when you want the normal first-run setup path |
| `GenerateSolution.bat` | Advanced build-tree refresh | Usually only after CMake changes, project creation, or when the build tree is stale |
| `Build.bat` | Build editor/runtime launch targets | Build `ShowcaseEditor`, `ShowcaseRuntime`, or both for any project |
| `CreateProject.bat` | Create a new project from `TemplateProject` | Start a new project folder and stamp `.sparkle-project` |
| `CheckToolchain.bat` | Validate required host tools | Check CMake, Visual Studio tools, git, and optional clang tooling |
| `SyncThirdParty.bat` | Inspect and repair `build/_deps` | Re-fetch missing or corrupt third-party dependencies |
| `CookAssets.bat` | Cook all engine and project scene assets for a project | Batch-convert all supported scenes under the engine and selected project mesh roots |
| `Format.bat` | Run `clang-format` | Apply repo formatting to `Engine/` and `Projects/` sources |
| `Clean.bat` | Remove generated artifacts | Clear build outputs, third-party cache, or return to a pristine state |

## Quick Workflow Guide

- First-time setup: run `Scripts\Setup.bat`
- Build one project: run `Scripts\Build.bat Showcase Both Debug`
- Create a new project: run `Scripts\CreateProject.bat MyGame`
- Validate the local toolchain: run `Scripts\CheckToolchain.bat`
- Repair third-party downloads: run `Scripts\SyncThirdParty.bat`
- Cook all assets for a project: run `Scripts\CookAssets.bat Showcase Debug`
- Format source files: run `Scripts\Format.bat`
- Clean generated artifacts: run `Scripts\Clean.bat`

## Normal Mental Model

- Most contributors should think in terms of `Setup.bat`, `Build.bat`, `CreateProject.bat`, and `CookAssets.bat`.
- `GenerateSolution.bat` is mostly an advanced or recovery command.
- In normal use, `Build.bat`, `CreateProject.bat`, `CookAssets.bat`, and `Setup.bat` already run solution-generation work for you when needed.

## When To Use GenerateSolution

- You changed CMake files and want to refresh the solution/build tree without building anything yet.
- You created a project and want to regenerate the solution manually.
- You cleaned the build tree and want to rebuild the generated files without running the full first-time setup path.
- You are debugging configure-time dependency or generator issues.

## Structure

- `Scripts/` contains user-facing workflow commands.
- `Scripts/Internal/` contains reusable helper modules used by the public commands. Do not treat these as stable user entrypoints.
- `CMake/Dependencies/` contains configure-time dependency modules.
- `CMake/Validation/` contains build-time validation modules such as runtime boundary checks.

## Notes

- `Build.bat` understands the split project targets introduced by the current host model: `<Project>Editor` and `<Project>Runtime`.
- `GenerateSolution.bat` is the single public owner of generator/toolset selection and incremental CMake configure behavior, but most users will reach it indirectly through the higher-level commands.
- Third-party dependency fetch is part of the configure flow, but `SyncThirdParty.bat` is the explicit repair/status command when you want to inspect or repair `build/_deps` directly.
- `CookAssets.bat` cooks all supported scene sources under `Engine\Assets\Meshes` and `Projects\<Project>\Assets\Meshes` into the selected project's cooked asset output, with project scenes overriding engine scenes on path collisions.
- Runtime boundary validation is a CMake target, not a user-run batch command. It lives under `CMake/Validation/` and runs as part of the engine build wiring.
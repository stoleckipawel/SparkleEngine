# Scripts

Sparkle Launcher is now the primary workflow entrypoint for setup, build, cook, format, clean, validation, and launch tasks. `Sparkle.exe` exposes the same operation ids for automation and CI. Public scripts in this folder are transitional and should not be treated as the normal contributor workflow.

If a file is under `Scripts/Internal`, it is implementation history for the script surface, not a stable user entrypoint.

## Primary Workflow

| Intent | Launcher Operation | CLI Form |
| --- | --- | --- |
| List available automation operations | Operation list | `Sparkle --list-operations` |
| First-time setup or workspace repair | Setup Workspace | `Sparkle workspace.setup` |
| Regenerate CMake/Visual Studio files | Generate Solution | `Sparkle workspace.generate-solution` |
| Build a project editor | Compile Editor | `Sparkle project.build.editor --project Showcase --editor-profile DevelopmentEditor` |
| Build a project runtime | Compile Runtime | `Sparkle project.build.runtime --project Showcase --runtime-profile DevelopmentGame` |
| Launch a built editor | Run Editor | `Sparkle project.launch.editor --project Showcase` |
| Launch a built runtime | Run Runtime | `Sparkle project.launch.runtime --project Showcase` |
| Cook all project assets | Cook All Assets | `Sparkle cook.project --project Showcase --runtime-profile DevelopmentGame` |
| Cook shader packages | Cook Shaders | `Sparkle cook.shaders --project Showcase --runtime-profile DevelopmentGame` |
| Cook texture assets | Build Textures | `Sparkle cook.textures --project Showcase --runtime-profile DevelopmentGame` |
| Cook scene, mesh, and material assets | Build Meshes / Scene Assets | `Sparkle cook.assets --project Showcase --runtime-profile DevelopmentGame` |
| Check or apply formatting | Run Clang Format | `Sparkle quality.format --format-mode check` |
| Run validation gates | Run Validation Gates | `Sparkle quality.validate` |
| Clean generated artifacts | Clean Workspace | `Sparkle workspace.clean --clean-scope selected-cooked --confirm-clean` |

Add `--dry-run` to any CLI operation to print readiness, planned effects, command lines, and log paths without executing the workflow. A ready dry-run exits with `0`, a blocked dry-run exits with `2`, and parse or execution failures exit with `1`.

## Transitional Scripts

| Script | Status | Replacement | Retention Condition |
| --- | --- | --- | --- |
| `SetupWorkspace.bat` | Deprecated bootstrap fallback | `Sparkle workspace.setup` | Retain only until a prebuilt launcher or installer can perform first-run setup from a fresh clone. |
| `GenerateSolution.bat` | Deprecated workflow script | `Sparkle workspace.generate-solution` | Remove or reduce to a wrapper after native configure/generate validation. |
| `BuildProject.bat` | Deprecated workflow script | `Sparkle project.build.editor`, `Sparkle project.build.runtime`, `Sparkle project.launch.editor`, `Sparkle project.launch.runtime` | Remove or reduce to a wrapper after native build and launch validation. |
| `CookAllAssets.bat` | Deprecated workflow script | `Sparkle cook.project` | Remove or reduce to a wrapper after native cook validation. |
| `Cook/CookShaders.bat` | Deprecated workflow script | `Sparkle cook.shaders` | Remove or reduce to a wrapper after native shader cook validation. |
| `Cook/CookTextures.bat` | Deprecated workflow script | `Sparkle cook.textures` | Remove or reduce to a wrapper after native texture cook validation. |
| `Cook/CookAssets.bat` | Deprecated workflow script | `Sparkle cook.assets` | Remove or reduce to a wrapper after native asset cook validation. |
| `RunClangFormat.bat` | Deprecated workflow script | `Sparkle quality.format` | Remove or reduce to a wrapper after native format validation. |
| `CleanWorkspace.bat` | Deprecated workflow script | `Sparkle workspace.clean` | Retain only if launcher self-clean needs a narrow external handoff. |

## Prerequisites

Sparkle Launcher validates the host before CMake configure/build/cook work. Required tools are CMake `3.20.0` or newer, Visual Studio 2022 or newer with the Desktop development with C++ workload, a Windows 10/11 SDK, MSBuild, and Git for Windows `2.25.0` or newer with sparse-checkout support.

Fresh clones also need GitHub access so CMake `FetchContent` can populate `build/_deps`. If `build/_deps` is already populated, setup does not require dependency network access for that run. Optional tools are reported separately: `clang-cl` is available for explicit `ClangCL` builds, `clang-format` powers formatting commands, `clang-tidy` is for local static-analysis workflows, and `git-lfs` is not required because dependency fetches skip LFS blobs.

Cook output can target editor or game profiles, but cook tools themselves are editor-profile targets: `DebugGame` uses tools from `DebugEditor`, `DevelopmentGame` uses tools from `DevelopmentEditor`, and `ShippingGame` uses tools from `ShippingEditor`.

## Structure

- `Scripts/` contains deprecated public workflow scripts during cutover.
- `Scripts/Cook/` contains deprecated focused cook workflow scripts during cutover.
- `Scripts/Internal/` contains implementation modules used by the old public commands. Do not treat these as stable user entrypoints.
- `CMake/Dependencies/` contains configure-time dependency modules.
- `CMake/Validation/` contains build-time validation modules such as runtime boundary checks.

## Architecture Notes

- [../docs/plans/sparkle-launcher-operation-inventory.md](../docs/plans/sparkle-launcher-operation-inventory.md) tracks public script parity, CLI replacements, and retained-script removal conditions.
- [../docs/build-configurations.md](../docs/build-configurations.md) lists the six supported Sparkle build profiles and points to the CMake source of truth.
- [../docs/architecture/shader-system-design.md](../docs/architecture/shader-system-design.md) is the current shader-system architecture baseline, including the compiler tool, Renderer shader orchestration, RHI backend realization, and no-runtime-compiler invariant.

## Logs

Generated logs are grouped by intent instead of being written directly into the repository root or the root of `logs/`:

- `logs/Projects/<Project>/Full/` contains one full runtime/editor activity log per launched project process, such as `ShowcaseEditor`.
- `logs/Prerequisites/ShaderCompilationLog/<Project>/` contains shader package cooking logs.
- `logs/Prerequisites/TextureCookingLog/<Project>/` contains texture cooking logs.
- `logs/Prerequisites/AssetCookingLog/<Project>/` contains scene, mesh, and material cooking logs.
- Other prerequisite workflows use similarly named folders, such as `BuildLog`, `SolutionGenerationLog`, `ToolchainCheckLog`, `FormatCheckLog`, and workspace setup/cleanup logs.

Each prerequisite folder keeps a timestamped log plus `Latest.txt` for that specific action or scope.

## Generated Artifact Layout

- `build/` is the local generated root. Visual Studio/CMake files, binaries, third-party FetchContent sources, shader cache, shader debug artifacts, shader symbols, and cooked runtime assets belong here.
- `build/Cooked/<Project>/` contains cooked runtime assets such as shader packages, texture payloads, scene manifests, meshes, materials, and cook registries.
- `build/Cache/Shaders/` contains local shader cache entries, live recook signals, and transient shader debug artifacts.
- `build/ShaderSymbols/<Project>/` contains generated shader symbol `.pdb` files. These are debug artifacts, not shader source assets.
- `logs/` contains runtime/editor and prerequisite logs only.
- `Projects/*/Assets/` and `Engine/Assets/` contain source assets. Generated cooked assets, shader symbols, or generated shader files should not be written there.
- Source folders named `Debug/` are allowed under `Engine/`, `Tools/`, and `Projects/`. Build configuration outputs should be under `build/`.

## Notes

- The supported build profiles are defined in `CMake/SparkleBuildProfiles.cmake`: `DebugEditor`, `DebugGame`, `DevelopmentEditor`, `DevelopmentGame`, `ShippingEditor`, and `ShippingGame`.
- Tool targets are intentionally editor-profile only. Build `AssetCooker`, `TextureCooker`, `ShaderCompiler`, `AssetConverter`, and supporting tool libraries with `DebugEditor`, `DevelopmentEditor`, or `ShippingEditor`; game profiles are runtime/cook-output profiles, not tool binary profiles.
- Project target derivation follows the build profile target: editor profiles build `<Project>Editor`, and game profiles build `<Project>Runtime`.
- Normal local builds do not run boundary validation or clang-format checks as target dependencies by default. Configure with `-DSPARKLE_BUILD_VALIDATION_ON_BUILD=ON` and/or `-DSPARKLE_RUN_CLANG_FORMAT_ON_BUILD=ON` for CI-style build-integrated checks, or run `Sparkle quality.validate` and `Sparkle quality.format --format-mode check` explicitly.
- Build outputs, shader cache/debug artifacts, shader symbols, and cooked assets are generated under `build/`; runtime/editor and prerequisite logs are generated under the structured `logs/` hierarchy.
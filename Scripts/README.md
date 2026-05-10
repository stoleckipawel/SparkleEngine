# Scripts

This folder is the contributor-facing command surface for SparkleEngine.

If you are trying to figure out what to run for what, start here. If a file is under `Scripts/Internal`, it is not a user entrypoint.

## Command Surface

| Command | Purpose | Typical Use |
| --- | --- | --- |
| `SetupWorkspace.bat` | First-time bootstrap | Fresh clone, or when you want the normal first-run setup path |
| `GenerateSolution.bat` | Advanced build-tree refresh | Usually only after CMake changes, project creation, or when the build tree is stale |
| `BuildProject.bat` | Build one project's editor or game launch target | Build `ShowcaseEditor` or `ShowcaseRuntime` via a named profile |
| `CookAllAssets.bat` | Preferred full cook entrypoint | Check required cook tools and forward a full project cook request |
| `Cook/CookShaders.bat` | Cook shader packages for a project | Check `AssetCooker` + `ShaderCompiler` and forward the shader cook request |
| `Cook/CookTextures.bat` | Cook texture assets for a project | Check `AssetCooker` + `TextureCooker` and forward the texture cook request |
| `Cook/CookAssets.bat` | Cook scene assets for a project | Check `AssetCooker` and forward the scene, mesh, and material cook request |
| `RunClangFormat.bat` | Run `clang-format` | Apply repo formatting to `Engine/` and `Projects/` sources |
| `CleanWorkspace.bat` | Remove generated artifacts | Clear build outputs, third-party cache, or return to the tracked repo state |

## Quick Workflow Guide

- First-time setup: run `Scripts\SetupWorkspace.bat`
- Build one project editor: run `Scripts\BuildProject.bat Showcase DevelopmentEditor`
- Build one project editorless game runtime: run `Scripts\BuildProject.bat Showcase DevelopmentGame`
- Run all CMake validation gates explicitly: run `cmake --build build --config DevelopmentEditor --target sparkle_validation_check`
- Run the CMake dry-run format check explicitly: run `cmake --build build --config DevelopmentEditor --target clang_format_check`
- Run the full cook from the top-level scripts folder: run `Scripts\CookAllAssets.bat Showcase DevelopmentGame`
- Cook shaders for a project: run `Scripts\Cook\CookShaders.bat Showcase DevelopmentGame`
- Cook textures for a project: run `Scripts\Cook\CookTextures.bat Showcase DevelopmentGame`
- Cook scene assets for a project: run `Scripts\Cook\CookAssets.bat Showcase DevelopmentGame`
- Format source files: run `Scripts\RunClangFormat.bat`
- Clean generated artifacts: run `Scripts\CleanWorkspace.bat`

## Prerequisites

`Scripts\SetupWorkspace.bat` starts with `Scripts\Internal\Toolchain\CheckToolchain.bat`, which validates the host before CMake tries to configure. Required tools are Windows PowerShell, CMake `3.20.0` or newer, Visual Studio 2022 or newer with the Desktop development with C++ workload, a Windows 10/11 SDK, MSBuild, and Git for Windows `2.25.0` or newer with sparse-checkout support.

Fresh clones also need GitHub access so CMake `FetchContent` can populate `build\_deps`. If `build\_deps` is already populated, setup does not require the reachability probe to pass for that run. Set `SPARKLE_SKIP_NETWORK_CHECK=1` only when you intentionally run offline with an existing dependency cache.

Dependency sync output is concise by default: configure reports that it is checking the third-party cache, then reports readiness. Configure with `-DSPARKLE_VERBOSE_DEPENDENCIES=ON` when you need dependency names, purpose, source repositories, pinned refs, and expected download sizes. Configure with `-DSPARKLE_GIT_PROGRESS=ON` only when you specifically need low-level Git transfer progress for network debugging.

Optional tools are reported separately: `clang-cl` is available for explicit `ClangCL` builds, `clang-format` powers formatting commands, `clang-tidy` is for local static-analysis workflows, and `git-lfs` is not required because dependency fetches skip LFS blobs.

Cook scripts do not compile cook tools by default. They check only the executables required by the requested cook mode and fail fast if those tools are missing. Build the missing tool targets intentionally, or set `SPARKLE_AUTO_BUILD_COOK_TOOLS=1` for one run when you explicitly want the cook command to build stale or missing cook tools before cooking.

Cook scripts default internal engine/tool logging to warnings with `SPARKLE_LOG_LEVEL=warn`, while keeping cook progress, asset names, summaries, and cooked outputs on the console. Set `SPARKLE_LOG_LEVEL=info` or `debug` when you intentionally need lower-level diagnostics.

## Structure

- `Scripts/` contains user-facing workflow commands. These files should stay thin: parse arguments, print user-facing status, and delegate work.
- `Scripts/Cook/` contains user-facing cook commands for focused asset cooking flows. Cook planning and execution stay in `AssetCooker`.
- `Scripts/Internal/` contains implementation modules used by the public commands. Do not treat these as stable user entrypoints.
- `Scripts/Internal/Core/` contains shared path configuration and logging bootstrap.
- `Scripts/Internal/Build/` contains CMake configure/build orchestration and build-file freshness checks.
- `Scripts/Internal/Toolchain/` contains Visual Studio, CMake, MSBuild, and optional tool validation helpers.
- `Scripts/Internal/Cook/` contains cook-tool preparation helpers only; actual cook behavior belongs in C++ tools.
- `Scripts/Internal/Projects/` contains project and target discovery helpers for script orchestration.
- `Scripts/Internal/Utilities/` contains narrow fallback utilities used by scripts.
- `CMake/Dependencies/` contains configure-time dependency modules.
- `CMake/Validation/` contains build-time validation modules such as runtime boundary checks.

## Architecture Notes

- [../docs/build-configurations.md](../docs/build-configurations.md) lists the six supported Sparkle build profiles and points to the CMake source of truth.
- [../docs/architecture/shader-system-design.md](../docs/architecture/shader-system-design.md) is the current shader-system architecture baseline, including the compiler tool, Renderer shader orchestration, RHI backend realization, and no-runtime-compiler invariant.

## Logs

Generated logs are grouped by intent instead of being written directly into the repository root or the root of `logs/`:

- `logs/Projects/<Project>/Full/` contains one full runtime/editor activity log per launched project process, such as `ShowcaseEditor`.
- `logs/Prerequisites/ShaderCompilationLog/<Project>/` contains shader package cooking logs.
- `logs/Prerequisites/TextureCookingLog/<Project>/` contains texture cooking logs.
- `logs/Prerequisites/AssetCookingLog/<Project>/` contains scene, mesh, and material cooking logs.
- Other prerequisite workflows use similarly named folders, such as `BuildLog`, `SolutionGenerationLog`, `ToolchainCheckLog`, `FormatCheckLog`, and workspace setup/cleanup logs.

Each prerequisite folder keeps a timestamped log plus `Latest.txt` for that specific action/scope.

## Generated Artifact Layout

- `build/` is the local generated root. Visual Studio/CMake files, binaries, third-party FetchContent sources, shader cache, shader debug artifacts, shader symbols, and cooked runtime assets belong here.
- `build/Cooked/<Project>/` contains cooked runtime assets such as shader packages, texture payloads, scene manifests, meshes, materials, and cook registries.
- `build/Cache/Shaders/` contains local shader cache entries, live recook signals, and transient shader debug artifacts.
- `build/ShaderSymbols/<Project>/` contains generated shader symbol `.pdb` files. These are debug artifacts, not shader source assets.
- `logs/` contains runtime/editor and prerequisite logs only.
- `Projects/*/Assets/` and `Engine/Assets/` contain source assets. Generated cooked assets, shader symbols, or generated shader files should not be written there; if a tool creates those folders, they should appear in `git status` as a layout bug.
- Source folders named `Debug/` are allowed under `Engine/`, `Tools/`, and `Projects/`. Build configuration outputs should be under `build/` or, if accidentally emitted at the repository root, are ignored as root-level artifacts.

## Notes

- The supported build profiles are defined in `CMake/SparkleBuildProfiles.cmake`: `DebugEditor`, `DebugGame`, `DevelopmentEditor`, `DevelopmentGame`, `ShippingEditor`, and `ShippingGame`.
- `BuildProject.bat` derives the launch target from the profile suffix: `*Editor` builds `<Project>Editor`, and `*Game` builds `<Project>Runtime`.
- `GenerateSolution.bat` is the single public owner of generator/toolset selection and incremental CMake configure behavior. Normal build/cook scripts call `Scripts\Internal\Build\EnsureBuildFiles.bat`, which skips `GenerateSolution.bat` when the generated build files are current.
- The required local Windows toolchain is Visual Studio 2022 or newer with the C++ workload and a Windows SDK. Scripts resolve the newest installed Visual Studio generator that is also exposed by CMake.
- Set `SPARKLE_CMAKE_GENERATOR`, `SPARKLE_CMAKE_ARCH`, or `SPARKLE_CMAKE_TOOLSET` to override generator selection explicitly. Set `SPARKLE_USE_CLANGCL=1` to request the `ClangCL` toolset; otherwise the default Visual Studio MSVC toolset is used.
- Set `SPARKLE_FORCE_CONFIGURE=1` before running a build/cook script to force `GenerateSolution.bat` even when the freshness check says the build files are current.
- Normal local builds do not run boundary validation or clang-format checks as target dependencies by default. Configure with `-DSPARKLE_BUILD_VALIDATION_ON_BUILD=ON` and/or `-DSPARKLE_RUN_CLANG_FORMAT_ON_BUILD=ON` for CI-style build-integrated checks, or run `sparkle_validation_check` and `clang_format_check` explicitly.
- Toolchain validation lives under `Scripts/Internal/Toolchain` and is invoked through the public workflow scripts.
- `CookAllAssets.bat` is the preferred single-file full cook command in the top-level `Scripts/` folder and delegates project planning to `AssetCooker`.
- `Scripts\Cook\CookShaders.bat`, `Scripts\Cook\CookTextures.bat`, and `Scripts\Cook\CookAssets.bat` are narrow launch entrypoints over `AssetCooker` category requests.
- `AssetCooker` discovers supported scenes, builds the Phase 2 project cook plan, dispatches the focused tools, and aggregates diagnostics at the tool boundary.
- Build outputs, shader cache/debug artifacts, shader symbols, and cooked assets are generated under `build/`; runtime/editor and prerequisite logs are generated under the structured `logs/` hierarchy.
- `CleanWorkspace.bat PRISTINE` removes generated outputs only. It does not delete tracked project assets.
- Runtime boundary validation is a CMake target, not a user-run batch command. It lives under `CMake/Validation/` and runs as part of the engine build wiring.
- The old Phase 1H validation wrapper scripts were removed. Use the documented direct smoke launch steps in `docs/plans/phase1h-d3d12-production-readiness.md` if you still need that evidence flow.

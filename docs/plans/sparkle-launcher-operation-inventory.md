# Sparkle Launcher Operation Inventory

## Purpose

This inventory freezes the Phase 0 migration contract for replacing Sparkle's public script workflow surface with native C++ operations owned by `SparkleLauncherCore`, surfaced by `SparkleLauncher`, and optionally exposed through `Sparkle.exe` for automation.

Existing scripts are behavioral references only. They are not the desired implementation backend for launcher operations.

## Migration Rules

- Normal setup, build, cook, format, clean, validation, and launch workflows must be native `SparkleLauncherCore` operations.
- `SparkleLauncher` may call `SparkleLauncherCore`; it must not shell to public scripts for normal work.
- `Sparkle.exe` may expose automation-friendly operation ids; it must remain a thin dispatcher over `SparkleLauncherCore`.
- Public `.bat` files should be removed once native launcher and CLI parity exists.
- Retained scripts are allowed only for unavoidable fresh-clone bootstrap, narrow OS shell handoff, or temporary wrappers over `Sparkle.exe` during cutover.

## Operation Id Catalog

| Operation Id | Owner | Purpose |
| --- | --- | --- |
| `workspace.setup` | `SparkleLauncherCore` | Fresh-clone or repair setup: validate toolchain and ensure build files. |
| `workspace.generate-solution` | `SparkleLauncherCore` | Configure/generate CMake build files and solution files. |
| `workspace.clean` | `SparkleLauncherCore` | Clean generated workspace artifacts with explicit scopes. |
| `toolchain.check` | `SparkleLauncherCore` | Detect required and optional local tools. |
| `project.discover` | `SparkleLauncherCore` | Discover runnable projects from `.sparkle-project` markers. |
| `project.build` | `SparkleLauncherCore` | Build selected project editor/runtime targets. |
| `project.launch` | `SparkleLauncherCore` | Launch selected project editor/runtime executable. |
| `cook.tools.prepare` | `SparkleLauncherCore` | Ensure required cook tool executables exist for a cook operation. |
| `cook.project` | `SparkleLauncherCore` | Full project asset cook through native tool invocation. |
| `cook.shaders` | `SparkleLauncherCore` | Focused shader package cook. |
| `cook.textures` | `SparkleLauncherCore` | Focused texture cook. |
| `cook.assets` | `SparkleLauncherCore` | Focused scene, mesh, and material asset cook. |
| `quality.format` | `SparkleLauncherCore` | Apply or check clang-format over engine/project sources. |
| `quality.validate` | `SparkleLauncherCore` | Run known CMake validation gates. |
| `logs.open-latest` | `SparkleLauncherCore` | Resolve/open latest relevant log for an operation. |

## Public Script Inventory

| Current Script | Native Replacement | Future State | Behavior To Port | Deletion Condition |
| --- | --- | --- | --- | --- |
| `Scripts/SetupWorkspace.bat` | `workspace.setup` | Remove public workflow script; optional bootstrap handoff only if no launcher binary exists. | Logging bootstrap; toolchain validation; ensure build files current; first-run success/failure summary; optional open-solution prompt as launcher action. | `SparkleLauncher` and `Sparkle.exe workspace.setup` can validate toolchain, ensure build files, report logs, and offer solution/editor opening without calling this script. |
| `Scripts/GenerateSolution.bat` | `workspace.generate-solution` | Remove public workflow script or temporary wrapper over `Sparkle.exe`. | Generator/toolset selection; interactive vs non-interactive mode; configure reason; CMake configure invocation; solution output path; configure failure suggestions; open-solution prompt. | Native operation owns CMake configure/generate, generator/toolset overrides, configure reasons, log routing, and solution opening. |
| `Scripts/BuildProject.bat` | `project.build`, `project.launch`, `project.discover` | Remove public workflow script or temporary wrapper over `Sparkle.exe`. | Project discovery; profile validation; profile-to-target mapping (`*Editor` -> `<Project>Editor`, `*Game` -> `<Project>Runtime`); ensure build files; target build; optional launch executable with project working directory. | Native operations can discover projects, derive targets, ensure build files, build editor/runtime targets, and launch built executables. |
| `Scripts/CookAllAssets.bat` | `cook.project`, `cook.tools.prepare`, `workspace.clean` scoped cooked-output cleanup | Remove public workflow script or temporary wrapper over `Sparkle.exe`. | Target project default `ALL`; default configuration `DevelopmentGame`; incremental/force mode; force recook deletion of `build/Cooked` or `build/Cooked/<Project>`; cook-tool preparation; `AssetCooker cook-project`; log-level default. | Native cook operation supports `ALL` and project scope, incremental/force modes, scoped cleanup, cook-tool preparation, direct `AssetCooker` invocation, and failure summaries. |
| `Scripts/Cook/CookShaders.bat` | `cook.shaders`, `cook.tools.prepare` | Remove public workflow script or temporary wrapper over `Sparkle.exe`. | Required project argument; default `DevelopmentGame`; shader cook tool preparation; `AssetCooker cook-shaders`; log-level default; latest log summary. | Native shader cook operation prepares/locates `AssetCooker` and `ShaderCompiler`, invokes shader cook directly, and reports logs/failures without script routing. |
| `Scripts/Cook/CookTextures.bat` | `cook.textures`, `cook.tools.prepare` | Remove public workflow script or temporary wrapper over `Sparkle.exe`. | Required project argument; default `DevelopmentGame`; texture cook tool preparation; `AssetCooker cook-textures`; log-level default; latest log summary. | Native texture cook operation prepares/locates `AssetCooker` and `TextureCooker`, invokes texture cook directly, and reports logs/failures without script routing. |
| `Scripts/Cook/CookAssets.bat` | `cook.assets`, `cook.tools.prepare` | Remove public workflow script or temporary wrapper over `Sparkle.exe`. | Required project argument; default `DevelopmentGame`; scene cook tool preparation; `AssetCooker cook-assets`; log-level default; latest log summary. | Native scene/mesh/material cook operation prepares/locates required tools, invokes asset cook directly, and reports logs/failures without script routing. |
| `Scripts/RunClangFormat.bat` | `quality.format` | Remove public workflow script or temporary wrapper over `Sparkle.exe`. | Source scan under `Engine/` and `Projects/`; extensions `cpp`, `h`, `hpp`, `hxx`, `hlsl`, `hlsli`; skip `Engine/third_party`; clang-format discovery; per-file progress; modified count; error count; format detail log. | Native format operation supports apply/check modes, exact source scan policy, progress, modified/error summary, and log output. |
| `Scripts/CleanWorkspace.bat` | `workspace.clean` | Remove public workflow script or keep only as exceptional bootstrap/handoff if launcher cannot clean itself while running. | Clean scopes `BUILD`, `DEPS`, `ALL`, `PRISTINE`; confirmation; preserve `build/_deps` for build-only clean; remove project-local generated outputs; root-level CMake/VS artifact cleanup; locked-file diagnostics; PowerShell fallback behavior to replace natively. | Native clean operation exposes explicit scopes, confirmation metadata, locked-file diagnostics, and equivalent generated artifact coverage without broad vague cleanup. |

## Internal Script Behaviors To Port

The public scripts delegate much of their behavior to `Scripts/Internal`. These internals should be treated as source material for `SparkleLauncherCore`, not as stable dependencies.

| Internal Area | Native Destination | Behavior To Port |
| --- | --- | --- |
| `Scripts/Internal/Core/Config.bat` | `RepositoryLocator`, `LauncherPaths`, `BuildLayout` | Root paths, build dir, binary dir, projects dir, logs dir, solution path, project name. |
| `Scripts/Internal/Core/BootstrapLog.bat` | `LogIndex`, `OperationLog` | Operation-scoped log directories, timestamped log files, `Latest.txt` update policy, console/log tee behavior. |
| `Scripts/Internal/Toolchain/CheckToolchain.bat` | `ToolchainValidator` | Required tools, optional tools, version checks, GitHub/dependency-cache reachability policy, actionable missing-tool diagnostics. |
| `Scripts/Internal/Toolchain/OpenVisualStudio.bat` | `OpenSolutionOperation` or launcher shell handoff | Optional solution-opening handoff after generation/setup. This can remain a narrow OS handoff if native launch is not worth owning. |
| `Scripts/Internal/Build/EnsureBuildFiles.bat` | `BuildFileFreshness`, `CMakeWorkflow` | Freshness gate, `SPARKLE_FORCE_CONFIGURE`, source list hash, project marker detection, toolchain/configure changes. |
| `Scripts/Internal/Build/CMakeHelpers.bat` | `CMakeWorkflow`, `BuildWorkflow` | Configure/build command construction, CMake generator/toolset/arch, target build invocation, logging/error classification. |
| `Scripts/Internal/Projects/ProjectDiscovery.bat` | `ProjectDiscovery`, `TargetResolver` | `.sparkle-project` discovery, project list, editor/runtime target derivation. |
| `Scripts/Internal/Cook/CookTools.bat` | `CookToolResolver`, `CookToolBuildOperation` | Editor-profile tool mapping, required tool sets per cook kind, existing executable detection, optional forced rebuild, tool path export. |
| `Scripts/Internal/Utilities/RemoveDirectory.ps1` | `WorkspaceCleanOperation` | Locked-tree removal fallback should become native cleanup/retry behavior or a documented unavoidable OS handoff. |

## Script Retention Policy

| Script Class | Retain? | Reason | Removal Condition |
| --- | --- | --- | --- |
| Public normal workflow scripts | No long term | Launcher and `Sparkle.exe` replace them. | Native operation parity plus final validation. |
| Temporary wrappers over `Sparkle.exe` | Short term only | Give CI/users transition time after native parity. | Docs and automation migrated to launcher/CLI operations. |
| Fresh-clone bootstrapper | Maybe | Needed only if no prebuilt launcher binary is available. | Prebuilt launcher release or installer can open a repo and run setup. |
| Narrow OS shell handoff | Maybe | Some operations, such as opening Visual Studio, may be simpler as shell handoff. | Native shell-execute path exists or the handoff is documented as intentionally retained. |
| Internal build-system CMake modules/targets | Yes | These are build infrastructure, not user-facing scripts. | Not applicable unless replaced by better build-system code. |

## Phase 0 Validation Notes

- Every current public script has a planned native `SparkleLauncherCore` replacement.
- Normal workflows do not have a permanent `.bat` path in the target design.
- Script-owned behavior is listed for porting before deletion.
- Deletion conditions are explicit for each public script.

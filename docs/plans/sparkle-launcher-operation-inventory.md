# Sparkle Launcher Operation Inventory

## Purpose

This inventory tracks the migration contract for replacing Sparkle's public script workflow surface with native C++ operations owned by `SparkleLauncherCore`, surfaced by `SparkleLauncher`, and exposed through `Sparkle.exe` for automation.

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
| `project.discover` | `SparkleLauncherCore` | Discover runnable projects from `.sparkle-project` markers for launcher state. |
| `project.build.editor` | `SparkleLauncherCore` | Build selected project editor target. |
| `project.build.runtime` | `SparkleLauncherCore` | Build selected project runtime target. |
| `project.launch.editor` | `SparkleLauncherCore` | Launch selected project editor executable from the project directory. |
| `project.launch.runtime` | `SparkleLauncherCore` | Launch selected project runtime executable from the project directory. |
| `cook.tools.prepare` | `SparkleLauncherCore` | Ensure required cook tool executables exist for a cook operation. |
| `cook.project` | `SparkleLauncherCore` | Full project asset cook through native tool invocation. |
| `cook.shaders` | `SparkleLauncherCore` | Focused shader package cook. |
| `cook.textures` | `SparkleLauncherCore` | Focused texture cook. |
| `cook.assets` | `SparkleLauncherCore` | Focused scene, mesh, and material asset cook. |
| `quality.format` | `SparkleLauncherCore` | Apply or check clang-format over engine/project sources. |
| `quality.validate` | `SparkleLauncherCore` | Run known CMake validation gates. |
| `logs.open-latest` | Future launcher shell | Resolve/open latest relevant log for an operation. |

## `Sparkle.exe` Automation Surface

`Sparkle.exe` is a thin CLI dispatcher over `SparkleLauncherCore`. It accepts the same operation ids as the launcher shell, plans the operation first, prints readiness/effects/log paths, and either exits after `--dry-run` or executes through the native process runner.

Common options:

```text
Sparkle <operation-id> [--dry-run] [--root <repo-root>] [--project <project-id>]
	[--editor-profile <profile>] [--runtime-profile <profile>]
```

Operation-specific options:

| Option | Applies To | Meaning |
| --- | --- | --- |
| `--target <target>` | Build operations | Add an explicit build target. |
| `--force-configure` | Build operations | Force CMake configure before build execution. |
| `--force-recook` | Cook operations | Use force recook mode. |
| `--confirm-force-recook` | Cook operations | Confirm force recook cleanup before execution. |
| `--shader-package <package-id>` | `cook.shaders` | Focus shader package cooking. |
| `--format-mode check\|apply` | `quality.format` | Choose check-only or applying format mode. |
| `--validation-target <target>` | `quality.validate` | Add a specific validation target. |
| `--clean-scope <scope>` | `workspace.clean` | Choose an explicit destructive clean scope. |
| `--confirm-clean` | `workspace.clean` | Confirm destructive clean execution. |

Exit codes are intentionally simple for automation: `0` for success or ready dry-run, `1` for parse/execution failure, and `2` for a blocked dry-run plan.

## Public Script Inventory

| Current Script | Native Replacement | Phase 6 State | Behavior Ported / Remaining | Removal Condition |
| --- | --- | --- | --- | --- |
| `Scripts/SetupWorkspace.bat` | `workspace.setup` / `Sparkle workspace.setup` | Deprecated public workflow; may remain only as fresh-clone bootstrap until a prebuilt launcher is available. | Native toolchain detection, build-file freshness, configure/generate planning, logs, and dry-run summaries are owned by C++. Optional open-solution handoff remains outside the current operation set. | Prebuilt launcher or installer can open a repo and run setup without requiring a script. |
| `Scripts/GenerateSolution.bat` | `workspace.generate-solution` / `Sparkle workspace.generate-solution` | Deprecated public workflow. | Native operation owns generator/toolset discovery, configure reason, CMake configure invocation, solution path, log routing, and failure summary. | Prompt 10 build validation confirms native configure/generate and docs/automation no longer reference the script as a workflow. |
| `Scripts/BuildProject.bat` | `project.build.editor`, `project.build.runtime`, `project.launch.editor`, `project.launch.runtime` | Deprecated public workflow. | Native operations discover projects, validate profiles, derive `<Project>Editor` and `<Project>Runtime`, ensure build files, build targets, and launch executables from `Projects/<Project>`. | Prompt 10 validation confirms editor/runtime build and launch dry-runs/execution paths through launcher/CLI. |
| `Scripts/CookAllAssets.bat` | `cook.project` / `Sparkle cook.project` | Deprecated public workflow. | Native cook operation supports selected project, default runtime profile, incremental/force mode, scoped force cleanup, cook-tool preparation, `AssetCooker cook-project`, logs, and failure summaries. | Prompt 10 validates native full project cook or accepted source-only equivalent before script removal. |
| `Scripts/Cook/CookShaders.bat` | `cook.shaders` / `Sparkle cook.shaders` | Deprecated public workflow. | Native shader cook prepares/locates `AssetCooker` and `ShaderCompiler`, invokes shader cook directly, supports package focus, and reports logs/failures. | Prompt 10 validates native shader cook execution path. |
| `Scripts/Cook/CookTextures.bat` | `cook.textures` / `Sparkle cook.textures` | Deprecated public workflow. | Native texture cook prepares/locates `AssetCooker` and `TextureCooker`, invokes texture cook directly, and reports logs/failures. | Prompt 10 validates native texture cook execution path. |
| `Scripts/Cook/CookAssets.bat` | `cook.assets` / `Sparkle cook.assets` | Deprecated public workflow. | Native scene/mesh/material cook prepares required tools, invokes asset cook directly, and reports logs/failures. | Prompt 10 validates native asset cook execution path. |
| `Scripts/RunClangFormat.bat` | `quality.format` / `Sparkle quality.format` | Deprecated public workflow. | Native format operation supports check/apply modes, source scan policy, progress, modified/error summary, and log output. | Prompt 10 validates native format check/apply behavior or accepted dry-run evidence. |
| `Scripts/CleanWorkspace.bat` | `workspace.clean` / `Sparkle workspace.clean` | Deprecated public workflow; may remain only for exceptional launcher self-clean handoff if needed. | Native clean operation exposes explicit scopes, confirmation metadata, locked-file diagnostics, and generated artifact cleanup coverage. | Prompt 10 validates safe scopes and any retained self-clean handoff is documented as unavoidable. |

## Script-to-CLI Parity Table

| Public Script | Sparkle Launcher Operation | `Sparkle.exe` Equivalent |
| --- | --- | --- |
| `Scripts/SetupWorkspace.bat` | Setup Workspace | `Sparkle workspace.setup` |
| `Scripts/GenerateSolution.bat` | Generate Solution | `Sparkle workspace.generate-solution` |
| `Scripts/BuildProject.bat <Project> DevelopmentEditor` | Compile Editor | `Sparkle project.build.editor --project <Project> --editor-profile DevelopmentEditor` |
| `Scripts/BuildProject.bat <Project> DevelopmentGame` | Compile Runtime | `Sparkle project.build.runtime --project <Project> --runtime-profile DevelopmentGame` |
| `Scripts/BuildProject.bat` launch prompt | Run Editor / Run Runtime | `Sparkle project.launch.editor --project <Project>` or `Sparkle project.launch.runtime --project <Project>` |
| `Scripts/CookAllAssets.bat <Project> DevelopmentGame` | Cook Project Assets | `Sparkle cook.project --project <Project> --runtime-profile DevelopmentGame` |
| `Scripts/Cook/CookShaders.bat <Project> DevelopmentGame` | Cook Shaders | `Sparkle cook.shaders --project <Project> --runtime-profile DevelopmentGame` |
| `Scripts/Cook/CookTextures.bat <Project> DevelopmentGame` | Cook Textures | `Sparkle cook.textures --project <Project> --runtime-profile DevelopmentGame` |
| `Scripts/Cook/CookAssets.bat <Project> DevelopmentGame` | Cook Scene Assets | `Sparkle cook.assets --project <Project> --runtime-profile DevelopmentGame` |
| `Scripts/RunClangFormat.bat` | Run Clang Format | `Sparkle quality.format --format-mode apply` |
| `Scripts/CleanWorkspace.bat` | Clean Workspace | `Sparkle workspace.clean --clean-scope <scope> --confirm-clean` |

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
| Public normal workflow scripts | Deprecated only during cutover | Launcher and `Sparkle.exe` replace them. | Native operation parity plus final validation. |
| Temporary wrappers over `Sparkle.exe` | Short term only | Give CI/users transition time after native parity. | Docs and automation migrated to launcher/CLI operations. |
| Fresh-clone bootstrapper | Retain if needed | Needed only if no prebuilt launcher binary is available. | Prebuilt launcher release or installer can open a repo and run setup. |
| Narrow OS shell handoff | Retain if needed | Some operations, such as opening Visual Studio or launcher self-clean, may be simpler as shell handoff. | Native shell-execute path exists or the handoff is documented as intentionally retained. |
| Internal build-system CMake modules/targets | Yes | These are build infrastructure, not user-facing scripts. | Not applicable unless replaced by better build-system code. |

## Explicit Retained Scripts

After Phase 6, normal workflow docs must point to Sparkle Launcher and `Sparkle.exe`. The only script cases allowed to remain are:

| Script | Retention Reason | Removal Condition |
| --- | --- | --- |
| `Scripts/SetupWorkspace.bat` | Fresh clone may not yet have a built `SparkleLauncher` or `Sparkle.exe`. | A prebuilt launcher/installer exists for first-run setup. |
| `Scripts/CleanWorkspace.bat` | Possible exceptional handoff if a running launcher cannot remove files it owns. | Native clean validation proves no self-clean handoff is needed, or the handoff is narrowed to a wrapper. |
| Public scripts used by external CI during transition | Temporary compatibility window only, preferably as wrappers over `Sparkle.exe`. | CI has migrated to `Sparkle.exe` operation ids or direct CMake/tool commands. |

## Phase 6 Validation Notes

- Every current public script has a native `SparkleLauncherCore` replacement and `Sparkle.exe` operation id.
- Normal workflows do not have a permanent `.bat` path in the target design.
- Public scripts are deprecated for workflow use until Prompt 10 build/execution validation allows removal or wrapper reduction.
- Retained scripts have explicit bootstrap, handoff, or CI-transition conditions.

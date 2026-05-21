# Sparkle Launcher Operation Inventory

## Purpose

This inventory records the native Sparkle Launcher operation surface after the script migration. SparkleEngine now relies on `SparkleLauncherCore`, `SparkleLauncher`, and `Sparkle.exe` for setup, build, cook, format, validation, clean, launch, and smoke-test workflows.

The old script workflow surface has been removed from the repository. Do not add compatibility wrappers or fallback command files for normal engine work.

## Native Workflow Rules

- Normal setup, build, cook, format, clean, validation, launch, and smoke-test workflows are native `SparkleLauncherCore` operations.
- `SparkleLauncher` may call `SparkleLauncherCore`; it must not shell to command-file wrappers for normal work.
- `Sparkle.exe` exposes automation-friendly operation ids and remains a thin dispatcher over `SparkleLauncherCore`.
- CI should use `Sparkle.exe`, direct CMake targets, or direct C++ tool executables.
- Fresh-clone onboarding should use a built launcher binary, packaged launcher, or direct CMake command until a packaged launcher is available.

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
| `smoke.rhi.editor` | `SparkleLauncherCore` | Launch selected project editor with opt-in RHI smoke validation enabled. |
| `smoke.rhi.runtime` | `SparkleLauncherCore` | Launch selected project runtime with opt-in RHI smoke validation enabled. |
| `cook.tools.prepare` | `SparkleLauncherCore` | Ensure required cook tool executables exist for a cook operation. |
| `cook.project` | `SparkleLauncherCore` | Full project asset cook through native tool invocation. |
| `cook.shaders` | `SparkleLauncherCore` | Focused shader package cook. |
| `cook.textures` | `SparkleLauncherCore` | Focused texture cook. |
| `cook.assets` | `SparkleLauncherCore` | Focused scene, mesh, and material asset cook. |
| `quality.format` | `SparkleLauncherCore` | Apply or check clang-format over engine/project sources. |
| `quality.validate` | `SparkleLauncherCore` | Run known CMake validation gates. |
| `logs.open-latest` | Future launcher shell | Resolve/open latest relevant log for an operation. |

## Launcher Ownership Map

Shared operation files should mostly orchestrate: resolve operation ids, copy typed request data into plans, call the nearest section owner, and build common dry-run/readiness output. Section-specific details belong under the section folder that owns the behavior.

| Area | Orchestration File | Detail Owners | Boundary Rule |
| --- | --- | --- | --- |
| Build/workspace | `Private/BuildWorkflow/BuildWorkspaceOperations.cpp` | `BuildFilesFreshness.cpp`, `BuildFreshnessSignature.cpp`, `BuildToolchainDetection.cpp`, `BuildWorkspaceProcessRequests.cpp`, `CMakeWorkflowProcessRequests.cpp` | Operation planner coordinates toolchain/build decisions; CMake request construction and freshness details stay in focused build files. |
| Cook | `Private/Cook/CookOperations.cpp` | `CookOperationProcessRequests.cpp`, `CookOperationExecutor.cpp` | Operation planner owns cook intent; tool invocation, force-recook cleanup, and execution details stay in cook-specific files. |
| Maintenance | `Private/Maintenance/MaintenanceOperations.cpp` | `MaintenanceOperationProcessRequests.cpp`, `MaintenanceOperationExecutor.cpp`, `Validation/ValidationGateCatalog.cpp` | Maintenance planner coordinates format/validate/clean; validator taxonomy and target grouping live in `Maintenance/Validation`. |
| Launch | `Private/Launch/LaunchOperations.cpp` | `LaunchOperationProcessRequests.cpp`, `LaunchOperationExecutor.cpp`, `Smoke/RhiSmokeLaunchOperations.cpp` | Launch planner derives target/executable and delegates smoke-specific inputs/environment/effects to `Launch/Smoke`. |
| CLI | `Private/Cli/SparkleCli.cpp` | `SparkleCliParser.cpp`, `SparkleCliValueOptions.cpp`, `SparkleCliDispatcher.cpp`, `SparkleCliOutput.cpp` | CLI files parse, print, and dispatch only; workflow behavior stays in `SparkleLauncherCore`. |
| App shell | `Private/Shell/LauncherShell.cpp` | Existing operation owners above | The shell selects project/profile and shows dry-runs; section behavior stays in core operation owners. |

When a new subsection grows beyond a small operation-list entry, add a folder under the nearest owner before putting detailed policy into a shared file.

## Validation Gates

`quality.validate` supports explicit targets and grouped target sets. With no target or group selected, it runs `sparkle_validation_check`.

| Group | Targets | Recommendation |
| --- | --- | --- |
| `aggregate` | `sparkle_validation_check` | Keep as the default full-suite validation entrypoint. |
| `boundaries` | `runtime_cooked_boundary_check`, `framegraph_boundary_check`, `rhi_backend_boundary_check`, `rhi_memory_boundary_check`, `shader_compiler_boundary_check`, `texture_cooker_boundary_check`, `tools_architecture_boundary_check`, `logging_boundary_check` | Keep; these protect architecture boundaries during refactors. |
| `parity` | `rhi_backend_parity_check`, `shader_package_parity_check` | Keep; these protect D3D12/Vulkan and DXIL/SPIR-V equivalence. |
| `readiness` | `geometry_instancing_readiness_check`, `threading_readiness_check`, `advanced_feature_readiness_check` | Keep for now; revisit after the corresponding roadmap work graduates into concrete feature tests. |

## Smoke Tests

Smoke tests are launch operations because they execute the built editor/runtime host. They are not CMake validation targets.

| Operation | Host | Native Hook |
| --- | --- | --- |
| `smoke.rhi.editor` | `<Project>Editor` | Sets `SPARKLE_SMOKE_VALIDATE_RHI=1` and runs the editor profile executable. |
| `smoke.rhi.runtime` | `<Project>Runtime` | Sets `SPARKLE_SMOKE_VALIDATE_RHI=1` and runs the runtime profile executable. |

Smoke options:

| Option | Meaning |
| --- | --- |
| `--smoke-backend <backend>` | Sets `SPARKLE_RHI_BACKEND` for the smoke host. |
| `--smoke-frame-limit <frames>` | Sets `SPARKLE_SMOKE_FRAME_LIMIT`; default is 120 frames. |
| `--smoke-trace` | Sets `SPARKLE_SMOKE_TRACE=1`. |
| `--smoke-skip-level-switching` | Sets `SPARKLE_SMOKE_SKIP_LEVEL_SWITCHING=1`. |

## `Sparkle.exe` Automation Surface

`Sparkle.exe` accepts the same operation ids as the launcher shell, plans the operation first, prints readiness/effects/log paths, and either exits after `--dry-run` or executes through the native process runner.

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
| `--validation-group <group>` | `quality.validate` | Add a grouped validation target set. |
| `--validation-target <target>` | `quality.validate` | Add a specific validation target. |
| `--smoke-backend <backend>` | Smoke operations | Select RHI backend for the launched smoke host. |
| `--smoke-frame-limit <frames>` | Smoke operations | Override the smoke host frame limit. |
| `--smoke-trace` | Smoke operations | Enable trace logging for the smoke host. |
| `--smoke-skip-level-switching` | Smoke operations | Disable automated level switching during the smoke run. |
| `--clean-scope <scope>` | `workspace.clean` | Choose an explicit destructive clean scope. |
| `--confirm-clean` | `workspace.clean` | Confirm destructive clean execution. |

Exit codes are intentionally simple for automation: `0` for success or ready dry-run, `1` for parse/execution failure, and `2` for a blocked dry-run plan.

## Removed Legacy Surface

The previous command-file workflow has been removed. Native replacements are:

| Intent | Native Operation | CLI Form |
| --- | --- | --- |
| First-time setup or workspace repair | Setup Workspace | `Sparkle workspace.setup` |
| Regenerate CMake/Visual Studio files | Generate Solution | `Sparkle workspace.generate-solution` |
| Build a project editor | Compile Editor | `Sparkle project.build.editor --project <Project> --editor-profile DevelopmentEditor` |
| Build a project runtime | Compile Runtime | `Sparkle project.build.runtime --project <Project> --runtime-profile DevelopmentGame` |
| Launch a built editor | Run Editor | `Sparkle project.launch.editor --project <Project>` |
| Launch a built runtime | Run Runtime | `Sparkle project.launch.runtime --project <Project>` |
| Run editor RHI smoke test | Run Editor RHI Smoke Test | `Sparkle smoke.rhi.editor --project <Project>` |
| Run runtime RHI smoke test | Run Runtime RHI Smoke Test | `Sparkle smoke.rhi.runtime --project <Project>` |
| Cook all project assets | Cook All Assets | `Sparkle cook.project --project <Project> --runtime-profile DevelopmentGame` |
| Cook shader packages | Cook Shaders | `Sparkle cook.shaders --project <Project> --runtime-profile DevelopmentGame` |
| Cook texture assets | Build Textures | `Sparkle cook.textures --project <Project> --runtime-profile DevelopmentGame` |
| Cook scene, mesh, and material assets | Build Meshes / Scene Assets | `Sparkle cook.assets --project <Project> --runtime-profile DevelopmentGame` |
| Check or apply formatting | Run Clang Format | `Sparkle quality.format --format-mode check` |
| Run validation gates | Run Validation Gates | `Sparkle quality.validate` |
| Clean generated artifacts | Clean Workspace | `Sparkle workspace.clean --clean-scope selected-cooked --confirm-clean` |

## Cutover Validation Notes

- The repository no longer contains command-file workflow entrypoints or internal script helper modules.
- Launcher code does not call command-file wrappers for normal workflow execution.
- Native generate-solution and compile-editor execution have been validated through `Sparkle.exe`.
- Cook, format, clean, launch, and project discovery paths have native dry-run validation through `Sparkle.exe` and `SparkleLauncher`.

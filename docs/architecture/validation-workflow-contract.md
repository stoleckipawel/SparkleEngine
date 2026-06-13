# Validation Workflow Contract

Status: architecture validation contract
Date: 2026-06-13

## Purpose

Validation should prove the paths developers and reviewers actually use. For editor/runtime launches, the canonical user path is the Sparkle Launcher launch workflow. A direct executable launch is acceptable for automation only when it mirrors the launcher plan: same target, profile, project working directory, environment variables, readiness assumptions, logs, and artifact paths.

This contract complements [tooling-pipeline-contract.md](tooling-pipeline-contract.md), [rendering-coverage-status.md](rendering-coverage-status.md), and the validation milestone stages in [rhi-renderer-review-ready-implementation-plan.md](../plans/rhi-renderer-review-ready-implementation-plan.md).

## Canonical Validation Order

| Priority | Path | When to use | Evidence required |
| --- | --- | --- | --- |
| 1 | Launcher operation plan and execution | User-facing launch, smoke, build/cook/launch workflow, reviewer path, or feature validation that depends on project setup. | Operation id, project id, target, profile, working directory, environment overrides, log path, artifacts, readiness messages, exit code, and failure summary. |
| 2 | Launcher-shaped direct execution | Automation or debugging where opening the GUI would slow the loop. | Explicit statement that the command mirrors `LaunchOperationPlan`, including executable path, `Projects/<ProjectId>` working directory, smoke environment, logs, and artifacts. |
| 3 | Direct tool target command | Tools with no launcher workflow yet, such as shader/package inspection or focused cooker commands. | Target name, command line, input files, output artifacts, report path, exit code, and whether the command should become a launcher workflow. |
| 4 | Build-only check | Compile or boundary-only validation. | Target/config/generator, command output result, generated target caveats, and reason runtime/tool execution was not needed. |

## Launcher-Shaped Runtime And Editor Smoke

Launcher smoke validation is owned by `Tools/Launcher/SparkleLauncher/Private/Launch/Smoke` and the `LaunchOperationPlan` contract.

Launcher-shaped validation must match these fields:

| Contract field | Expected shape |
| --- | --- |
| Operation kind | `RunProject`, normally operation id `project.run.smoke` or an equivalent launcher smoke operation. |
| Project | `Showcase` unless the stage explicitly validates another project. |
| Target | `editor` or `runtime`, matching the user-facing launch target. |
| Profile | Editor uses `DevelopmentEditor`; runtime uses the launcher-resolved runtime profile unless the stage specifies another profile. |
| Executable | Launcher-resolved project executable under `artifacts/dev/projects/<Project>/<target>/<Profile>/`. |
| Working directory | `Projects/<ProjectId>`, not the executable directory. This is required for project markers and cooked asset resolution. |
| Smoke enable | `SPARKLE_SMOKE_VALIDATE_RHI=1`. |
| Frame count | `SPARKLE_SMOKE_FRAME_LIMIT`, defaulting to `120` when the launcher request is empty. |
| Backend | `SPARKLE_RHI_BACKEND=D3D12` or `SPARKLE_RHI_BACKEND=Vulkan` when backend parity is being validated. |
| View mode | `SPARKLE_SMOKE_VIEW_MODE`, used by editor smoke captures. Lit is `0`; GBuffer normal is `3`. |
| Capture path | `SPARKLE_SMOKE_SCENE_COLOR_CAPTURE`, used by editor smoke capture. Artifact names must include backend and view mode. |
| Trace | `SPARKLE_SMOKE_TRACE=1` only when the stage needs trace-level evidence. |
| Level switching | `SPARKLE_SMOKE_SKIP_LEVEL_SWITCHING=1` when a stage needs a stable single-scene comparison. |
| Log path | `SPARKLE_LOG_FILE` or the launcher operation log path. Logs are validation artifacts and should sit beside capture artifacts when running directly. |

## Acceptance Rules

- A smoke run is not accepted because the executable launched. It must produce logs and, when requested, capture artifacts.
- A direct executable run must state whether it followed launcher-shaped execution. If it did not, the result is diagnostic only and cannot close a validation milestone.
- Backend parity stages must run both D3D12 and Vulkan through the same project, target type, frame count, startup path, and view modes unless the difference is explicitly the subject of the test.
- Editor capture validation must include Lit and at least one debug/normal mode when the stage touches frame graph, resources, upscaling, presentation, or backend layout/state handling.
- Runtime validation must scan logs for error/critical diagnostics. Exit code `0` does not override validation-layer errors.
- Unresolved frame graph handles, unresolved barrier warnings, silent upscaler fallback, missing capture artifacts, missing cooked assets, and validation-layer errors are milestone blockers unless a later stage owns a named exception.
- Generated validation artifacts belong under `artifacts/validation/<stage-or-feature>/`, not source or docs folders.
- If a useful validation requires repeated manual setup, add or improve a launcher workflow rather than keeping the process as private knowledge.

## When To Add Launcher Support

Add or extend a launcher workflow when validation needs any of the following:

| Need | Launcher-facing shape |
| --- | --- |
| Repeated backend/view/capture matrix | Launcher smoke presets or request fields. |
| Build/cook/run sequence | Launcher operation with readiness checks and planned effects. |
| Artifact paths reviewers should open | Operation record with report/log/capture paths. |
| Tool output that users need to reproduce | Launcher operation or inspector command surfaced through LauncherCore. |
| Long-running validation | Process request with progress/history/report output rather than GUI-owned work. |

Do not add launcher UI controls for one-off experiments. Add LauncherCore support when the validation is a durable workflow; add GUI controls when a developer or reviewer should run it repeatedly.

## Milestone Evidence Template

Every validation milestone should record:

| Field | Required content |
| --- | --- |
| Launcher basis | Launcher operation id, target, project, profile, and whether execution used the GUI, LauncherCore plan, or launcher-shaped direct execution. |
| Command shape | Executable/tool path, working directory, arguments, environment overrides, and log path. |
| Inputs | Project, scene/startup level, backend, view mode, frame count, feature flags, cooked artifact assumptions. |
| Outputs | Logs, captures, reports, JSON summaries, package lists, screenshots, or cooked artifacts. |
| Diagnostics | Frame graph warnings, validation-layer errors, DLSS/upscaler status, ray tracing status, backend capabilities, failure domain, and error/critical log scan. |
| Decision | Passed, failed, unsupported with reason, or blocked with owner stage. |


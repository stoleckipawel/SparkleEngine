# Sparkle Launcher GUI Phase 2 Feature Map

## Console Feature Inventory
- `--list-operations`: GUI operation list is populated from `GetBuildWorkspaceOperationDefinitions`, `GetCookOperationDefinitions`, `GetMaintenanceOperationDefinitions`, and `GetLaunchOperationDefinitions`.
- `--list-validation-targets`: GUI settings expose validation groups and explicit validation target input; known groups are `aggregate`, `boundaries`, `parity`, and `readiness`.
- `workspace.setup`: GUI operation backed by `PlanBuildWorkspaceOperation` / `RunBuildWorkspaceOperationPlan`.
- `workspace.generate-solution`: GUI operation backed by `PlanBuildWorkspaceOperation` / `RunBuildWorkspaceOperationPlan`.
- `toolchain.check`: GUI operation backed by `PlanBuildWorkspaceOperation` / `RunBuildWorkspaceOperationPlan`.
- `project.build.editor`: GUI operation backed by `PlanBuildWorkspaceOperation` / `RunBuildWorkspaceOperationPlan`.
- `project.build.runtime`: GUI operation backed by `PlanBuildWorkspaceOperation` / `RunBuildWorkspaceOperationPlan`.
- `cook.tools.prepare`: GUI operation backed by `PlanBuildWorkspaceOperation` / `RunBuildWorkspaceOperationPlan`.
- `cook.project`: GUI operation backed by `PlanCookOperation` / `RunCookOperationPlan`.
- `cook.shaders`: GUI operation backed by `PlanCookOperation` / `RunCookOperationPlan`.
- `cook.textures`: GUI operation backed by `PlanCookOperation` / `RunCookOperationPlan`.
- `cook.assets`: GUI operation backed by `PlanCookOperation` / `RunCookOperationPlan`.
- `quality.format`: GUI operation backed by `PlanMaintenanceOperation` / `RunMaintenanceOperationPlan`.
- `quality.validate`: GUI operation backed by `PlanMaintenanceOperation` / `RunMaintenanceOperationPlan`.
- `workspace.clean`: GUI operation backed by `PlanMaintenanceOperation` / `RunMaintenanceOperationPlan`.
- `project.launch.editor`: GUI operation backed by `PlanLaunchOperation` / `RunLaunchOperationPlan`.
- `project.launch.runtime`: GUI operation backed by `PlanLaunchOperation` / `RunLaunchOperationPlan`.
- `smoke.rhi.editor`: GUI operation backed by `PlanLaunchOperation` / `RunLaunchOperationPlan`.
- `smoke.rhi.runtime`: GUI operation backed by `PlanLaunchOperation` / `RunLaunchOperationPlan`.

## CLI Option Mapping
- `--root`: repository root is discovered during Qt bootstrap and passed into every backend request.
- `--project`: project list selection feeds `LauncherOperationRequest::ProjectId`.
- `--editor-profile`: settings page editor profile combo feeds build, maintenance, and editor launch requests.
- `--runtime-profile`: settings page runtime profile combo feeds build, cook, and runtime launch requests.
- `--target`: settings page build target input feeds `BuildWorkspaceOperationRequest::SelectedTargets`.
- `--force-configure`: settings page checkbox feeds `BuildWorkspaceOperationRequest::ForceConfigure`.
- `--force-recook`: settings page checkbox switches cook requests to `CookMode::Force`.
- `--confirm-force-recook`: settings page checkbox feeds `CookOperationRequest::ForceRecookConfirmed`.
- `--shader-package`: settings page shader package input feeds `CookOperationRequest::ShaderPackages`.
- `--format-mode`: settings page format mode combo feeds `MaintenanceOperationRequest::RequestedFormatMode`.
- `--validation-group`: settings page validation group input feeds `MaintenanceOperationRequest::ValidationGroups`.
- `--validation-target`: settings page validation target input feeds `MaintenanceOperationRequest::ValidationTargets`.
- `--clean-scope`: settings page clean scope combo feeds `MaintenanceOperationRequest::RequestedCleanScope`.
- `--confirm-clean`: settings page checkbox feeds `MaintenanceOperationRequest::DestructiveActionConfirmed`.
- `--smoke-backend`: settings page smoke backend input feeds `LaunchOperationRequest::SmokeBackend`.
- `--smoke-frame-limit`: settings page smoke frame limit input feeds `LaunchOperationRequest::SmokeFrameLimit`.
- `--smoke-trace`: settings page checkbox feeds `LaunchOperationRequest::SmokeTrace`.
- `--smoke-skip-level-switching`: settings page checkbox feeds `LaunchOperationRequest::SmokeSkipLevelSwitching`.

## Backend Guiderails
- The GUI does not invoke `Sparkle.exe`, `SparkleCli`, batch files, or `QProcess`.
- Operations run through `SparkleLauncherCore` plan/run APIs with `NativeProcessRunner`, matching the CLI backend path while keeping the console hidden.
- `LauncherBackend` accepts a process-runner factory so tests can inject an `IProcessRunner` mock instead of launching real tools.
- Qt signals stream operation start, output chunks, preview readiness, and completion back to the main window.
- Readiness failures are shown as blocked previews or skipped run attempts instead of starting partial workflows.

## Acceptance Criteria
- Every CLI operation id is listed in the GUI operation list.
- Every CLI option has a Qt-side input or an equivalent GUI state source.
- Preview uses native plan APIs and shows readiness, effects, destructive confirmations, logs, and dry-run command details.
- Run uses native run APIs and streams captured output into the launcher UI.
- No full build or package step is run for this phase.
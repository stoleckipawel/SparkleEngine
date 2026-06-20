# Application Lifecycle And Error Taxonomy

## Purpose

This document describes the current SparkleEngine host lifecycle across runtime, editor, launcher-assisted local build workflows, shader recook, and RHI smoke validation. Its purpose is to make startup, backend creation, project-facing load behavior, frame execution, validation, and shutdown easy to reason about before new rendering and tooling features expand the host surface.

## Non-goals

- This is not a feature design for a new application framework.
- This does not add new launcher actions, runtime commands, or validation flows.
- This does not claim a richer project-open workflow than the current source proves.
- This does not merge runtime and editor responsibilities into one host contract.

## Runtime Host Responsibilities

Current runtime host responsibility is centered in `Application`, `RuntimeApplication`, and `RunRuntimeApplication(...)`.

- `Application::ConfigureProcessFromCommandLine()` applies process-wide configuration before host creation.
- `Application::Run()` owns the common `Initialize() -> Tick() -> Shutdown()` lifecycle and wraps it in `Diagnostics::BeginTraceSession()` / `Diagnostics::EndTraceSession()`.
- `RuntimeApplication::Initialize()` creates the runtime timer, window, input system, `GameScene`, `SceneAssetManager`, `LevelManager`, `Renderer`, and optional `RuntimeConsoleHost`.
- `RuntimeApplication::BeginFrame()` owns per-frame event polling, deferred input processing, close/minimize handling, and `LevelManager::ProcessPendingLevelChange()`.
- `RuntimeApplication::UpdateRuntime()` advances gameplay-facing scene state through `GameScene::Update(...)`.
- `RuntimeApplication::Tick()` decides whether to exit, skip render, or execute the normal runtime frame.
- `RuntimeApplication::Shutdown()` tears down runtime-owned services in reverse dependency order.

## Editor Host Responsibilities

Current editor host responsibility is centered in `EditorApplication` and `RunEditorApplication(...)`.

- `EditorApplication` owns an embedded `RuntimeApplication`.
- Editor mode disables the runtime console and clears the input capture query so the editor UI can own host routing.
- `EditorApplication::Initialize()` constructs `ShaderRecookCoordinator` and `UI`, then binds diagnostics providers and editor shader console commands.
- `EditorApplication::Tick()` reuses the runtime begin/update/end frame flow, but inserts editor viewport request submission, host-frame recording, present pass UI composition, shader recook polling, and diagnostics presentation.
- `EditorApplication::Shutdown()` destroys the UI first, then shuts down the embedded runtime host.

## Startup Sequence

### Runtime startup

1. External entrypoint calls `RunRuntimeApplication()` or `RunRuntimeApplication(RuntimeApplicationOptions)`.
2. `Application::ConfigureProcessFromCommandLine()` runs before host construction.
3. If `RhiSmokeValidation::IsRequested()` is true, control is diverted into `RhiSmokeValidation::RunProject(...)`.
4. Otherwise `RuntimeApplication app(...)` is constructed.
5. `Application::Run()` begins a trace session and calls `RuntimeApplication::Initialize()`.
6. `RuntimeApplication::Initialize()` creates:
   - `Timer`
   - `Window`
   - `InputSystem`
   - `GameScene`
   - `GameCameraController`
   - optional `SceneSetupCallback`
   - `Assets::SceneAssetManager`
   - `LevelManager`
   - `Renderer`
   - optional `RuntimeConsoleHost`
7. `LevelManager` initialization immediately attempts startup level resolution and load.
8. Control enters the `while (Tick())` main loop.

### Editor startup

1. External entrypoint calls `RunEditorApplication()` or `RunEditorApplication(EditorApplicationOptions)`.
2. `Application::ConfigureProcessFromCommandLine()` runs before host construction.
3. If `RhiSmokeValidation::IsRequested()` is true, control is diverted into `RhiSmokeValidation::RunEditor(...)`.
4. Otherwise `EditorApplication app(...)` is constructed.
5. `Application::Run()` begins a trace session and calls `EditorApplication::Initialize()`.
6. `EditorApplication::Initialize()`:
   - creates or reuses `RuntimeApplication`
   - forces `RuntimeApplicationOptions.EnableRuntimeConsole = false`
   - creates `ShaderRecookCoordinator`
   - initializes the embedded runtime host
   - clears runtime input capture query
   - starts editor input routing with `BeginInputRoutingFrame(false, false)`
   - creates `UI`
   - connects renderer-backed diagnostics providers
   - connects shader console commands to the recook coordinator
7. Control enters the editor `while (Tick())` main loop.

## Tool/Dependency Discovery Sequence

This sequence belongs to the launcher/tool host, not the runtime or editor process.

1. Launcher gathers workspace feature settings through `GetLauncherWorkspaceFeatureSettings()`.
2. `DetectBuildToolchain(...)` inspects host tools and SDKs such as:
   - CMake
   - Git
   - Ninja
   - MSBuild / Visual Studio discovery
   - Rider
   - `vswhere`
   - Qt
   - shader compiler SDK root
   - Vulkan SDK root
3. The launcher records tool status in `BuildToolchainStatus` and `ToolchainItemStatus`.
4. `CheckBuildFilesFreshness(...)` evaluates build-directory, cache, solution, generator, feature-set, stamp, and input freshness.
5. `PlanBuildWorkspaceOperation(...)` determines readiness and planned effects for actions such as:
   - `CheckToolchain`
   - `SyncSourceTiers`
   - `GenerateBuildFiles`
   - `OpenIde`
   - `BuildAll`
   - `CompileLauncher`
   - `CompileEditor`
   - `CompileRuntime`
   - `BuildCookTools`
   - `AssembleRelease`
6. `RunBuildWorkspaceOperationPlan(...)` executes process steps built by `BuildProcessStepsForPlan(...)`.
7. Configure requests are emitted through `MakeCMakeConfigureRequest(...)`.
8. Build requests are emitted through `MakeCMakeBuildRequest(...)`.
9. Configure failure text is reduced into a user-facing summary through `ExtractConfigureFailureDetail(...)`.
10. For selected stale or corrupt dependency-cache failures, launcher code clears stale configure state and retries configure once.

## Backend/Device Creation Sequence

Application-layer source proves the following ownership and ordering:

1. `RuntimeApplication::Initialize()` constructs `Renderer` after scene and level services exist.
2. The runtime host passes `Timer`, `GameScene`, `Window`, and `LevelManager` into `Renderer`.
3. The editor host later reaches backend-facing presentation work through `Renderer`, `RenderHardwareInterface`, and `RhiPresentationService`.

The exact internal device creation sequence below `Renderer` belongs to Renderer/RHI implementation and is documented separately in [RHIContract.md](./RHIContract.md) and [RendererFrameGraph.md](./RendererFrameGraph.md).

Source-backed application contract:

- Application owns when renderer construction happens.
- Renderer/RHI own how backend selection, device creation, queue creation, and presentation services are realized internally.
- Editor host may consume presentation services through the renderer, but does not own backend creation policy.

Needs source confirmation:

- Exact adapter selection ordering exposed to application-level callers.
- Whether backend fallback can occur after `Renderer` construction begins.
- Whether device creation errors are translated into structured host-visible error categories yet.

## Project Load Sequence

SparkleEngine does not currently expose a large project-open host API in `Application`. The current source-backed load path is level- and scene-oriented.

1. Runtime host creates `Assets::SceneAssetManager`.
2. Runtime host creates `LevelManager`.
3. `LevelManager` constructor calls `InitializeStartupLevel()`.
4. Startup level name is resolved from `SPARKLE_STARTUP_LEVEL`, or defaults to `"Sponza"`.
5. `LevelRegistry` resolves the requested level or a default level.
6. `LoadLevelFromUnloadedState(...)`:
   - broadcasts `OnLevelWillLoad`
   - unloads all scene assets
   - loads `LevelDesc` into `GameScene`
   - loads referenced scene assets through `SceneAssetManager`
   - appends scene asset payload into `GameScene`
   - applies the primary camera
7. During normal frame execution, `LevelManager::ProcessPendingLevelChange()` processes deferred level change requests.
8. Failed level changes attempt fallback loading of the `"Empty"` level.

Current interpretation:

- "Project load" at the application layer currently means startup-level and level-change readiness more than a standalone editor project document workflow.

Needs source confirmation:

- Whether a wider editor project-open/save model exists outside the modules reviewed for this document.

## Shader Cook/Recook Relationship

Runtime and editor behavior are intentionally different.

### Runtime

- Runtime host does not create `ShaderRecookCoordinator`.
- Runtime smoke validation can trigger `renderer.ReloadCookedShaders()` at a configured frame through `RhiSmokeFrameControl`.
- Runtime shader reload waits for `renderer.GetCommandSubmissionService().WaitForIdle()` first.

### Editor

- Editor host owns `ShaderRecookCoordinator`.
- The UI can request recook or reload.
- `ShaderRecookCoordinator::Update(...)` also monitors external source changes through `ShaderSourceChangeTracker`.
- Recook runs out-of-process through `ShaderCompilerProcess`.
- `ShaderCompilerProcess::RunCook(...)` resolves `ShaderCompiler.exe` from build output.
- Recook currently resolves the working project directory through `Paths::ProjectRoot()` and reports `"Projects/Showcase was not found from the current build output."` if the directory is unavailable.
- On successful fresh publication acceptance, the editor waits for RHI idle and then reloads cooked shaders.
- On failure, previous cooked shader packages remain active.

Useful source-backed editor command names:

- `RecompileShaders`
- `ReloadShaders`
- `ListShaders`
- `ListShaderBackends`
- `ListShaderTargets`

## Main Loop / Frame Sequence

### Common host wrapper

`Application::Run()` executes:

1. `Diagnostics::BeginTraceSession()`
2. `Initialize()`
3. `while (Tick()) {}`
4. `Shutdown()`
5. `Diagnostics::EndTraceSession()`

### Runtime frame sequence

1. `RuntimeApplication::BeginFrame()`
2. `InputSystem::BeginFrame()`
3. `Window::PollEvents()`
4. `InputSystem::ProcessDeferredEvents()`
5. exit if `Window::ShouldClose()`
6. if minimized:
   - `InputSystem::EndFrame()`
   - `Window::WaitForEvent()`
   - skip render
7. `LevelManager::ProcessPendingLevelChange()`
8. `RuntimeApplication::UpdateRuntime()`
9. either:
   - `RuntimeConsoleHost::TickFrame(...)`, which wraps runtime update and renderer execution, or
   - direct `Renderer::OnRender()`
10. `RuntimeApplication::EndFrame()`
11. `InputSystem::EndFrame()`

### Editor frame sequence

1. `RuntimeApplication::BeginFrame()`
2. consume optional shader recook request from the UI
3. `ShaderRecookCoordinator::Update(...)`
4. `RuntimeApplication::UpdateRuntime()`
5. `RuntimeApplication::SubmitViewportRenderRequest(...)`
6. `Renderer::PrepareHostFrame()`
7. `Renderer::RecordHostFrame()`
8. read `ViewportRenderProducts`
9. `Renderer::BeginViewportPresentation(RenderOutputFlags::SceneColor)`
10. push scene-color presentation data into the UI
11. `UI::Update()`
12. `RhiPresentationService::BeginPresentRenderPass(...)`
13. `UI::Render()`
14. `RhiPresentationService::EndPresentRenderPass()`
15. `Renderer::EndViewportPresentation(...)`
16. `Renderer::SubmitHostFrame()`
17. `RuntimeApplication::EndFrame()`

## Validation/Capture Sequence

Current validation entry is environment-driven.

1. `RhiSmokeValidation::IsRequested()` checks `SPARKLE_SMOKE_VALIDATE_RHI`.
2. Runtime launch redirects into `RhiSmokeValidation::RunProject(...)`.
3. Editor launch redirects into `RhiSmokeValidation::RunEditor(...)`.
4. `RhiSmokeSession::LoadConfig()` reads smoke-validation environment state such as:
   - `SPARKLE_SMOKE_TRACE`
   - `SPARKLE_SMOKE_FRAME_LIMIT`
   - `SPARKLE_SMOKE_RESTORE_FRAME`
   - `SPARKLE_SMOKE_MAXIMIZE_FRAME`
   - `SPARKLE_SMOKE_SHADER_RELOAD_FRAME`
   - `SPARKLE_SMOKE_SKIP_LEVEL_SWITCHING`
   - `SPARKLE_SMOKE_LEVEL_SWITCH_INTERVAL_FRAMES`
5. Session setup logs diagnostics capabilities and initializes frame control.
6. Per-frame validation may:
   - log renderer evidence
   - advance camera motion checks
   - advance level switching checks
   - trigger cooked shader reload
   - restore or maximize the window
   - request shutdown at frame limit
7. Editor smoke validation can additionally:
   - override view mode
   - capture scene color
   - emit capture metadata and timing CSV artifacts through `RhiSmokeCaptureArtifacts`
8. Validation returns non-zero when frame-control state marks the run as failed.

Current capture artifacts include:

- capture output path
- JSON metadata
- timing CSV
- backend and adapter identity
- frame graph unresolved barrier warnings
- upscaler provider/status/reason
- ray tracing capability and timing evidence

## Shutdown Sequence

### Runtime shutdown

1. `Application::Run()` exits the loop.
2. `RuntimeApplication::Shutdown()` resets:
   - `RuntimeConsoleHost`
   - `Renderer`
   - `LevelManager`
   - `SceneAssetManager`
   - `GameScene`
   - `InputSystem`
   - `Window`
   - `Timer`
3. `Application::Run()` ends the trace session.

### Editor shutdown

1. `Application::Run()` exits the loop.
2. `EditorApplication::Shutdown()` destroys `UI`.
3. Embedded `RuntimeApplication::Shutdown()` runs.
4. `Application::Run()` ends the trace session.

Needs source confirmation:

- Whether the editor should also explicitly release `ShaderRecookCoordinator` during shutdown as a documented ordering guarantee. The current code leaves coordinator destruction to object teardown rather than an explicit reset step.

## Error Taxonomy

This taxonomy is written to match the current host/tooling surface, even where the code still reports plain strings instead of structured error objects.

### missing SDK

Definition:
Required SDK for an enabled workflow is not installed or not discoverable.

Current source evidence:

- launcher toolchain state tracks Vulkan SDK root and shader compiler SDK root
- configure failure extraction prioritizes `VULKAN_SDK` and shader-toolchain file failures

Expected host owner:

- launcher/toolchain detection
- configure/build workflow planning

Typical user-facing effect:

- `ConfigurePrerequisitesAvailable` becomes false
- `GenerateBuildFiles` or build workflows are blocked before or during configure

### missing source dependency

Definition:
Enabled repository dependency cache is incomplete, stale, corrupt, or missing.

Current source evidence:

- `HasIncompleteEnabledSourceDependencies(...)`
- configure retry on stale `_deps` cache
- configure failure extraction prioritizes NVAPI/Streamline/source-cache failures

Expected host owner:

- launcher/source-dependency and configure workflow

Typical user-facing effect:

- readiness message asks for `Prepare Workspace`
- configure may clear `_deps` and retry once

### unsupported hardware

Definition:
Selected or enabled feature cannot run on the current adapter or device capability set.

Current source evidence:

- host graphics capability gating for NVIDIA Streamline workspace enablement
- smoke capture metadata records adapter identity and ray tracing capability

Expected host owner:

- launcher capability presentation
- renderer/RHI diagnostics

Needs source confirmation:

- structured application-level propagation path for unsupported adapter/backend selection failure

### invalid project

Definition:
Project-facing content, level selection, or project-root assumptions are invalid for the requested host action.

Current source evidence:

- startup level resolution can fail if no registered level is available
- targeted level change can fail and fall back to `"Empty"`
- shader recook reports failure when `Projects/Showcase` cannot be resolved from current build output

Expected host owner:

- runtime level-management path
- editor shader recook tooling path

### shader cook failure

Definition:
Out-of-process shader recook or shader package replacement did not complete successfully.

Current source evidence:

- `ShaderCompilerProcessResult.ExitCode`
- `ShaderRecookCoordinator` keeps previous cooked packages active on recook failure
- manual reload can be rejected by runtime validation
- smoke validation can fail on shader reload rejection

Expected host owner:

- editor shader recook subsystem
- runtime smoke validation when shader reload is scripted

### backend creation failure

Definition:
Renderer/RHI backend could not be constructed for the requested host run.

Current source evidence:

- application layer constructs `Renderer` during runtime initialization

Needs source confirmation:

- exact exception, status, or logging contract emitted when renderer construction fails
- whether backend selection failure is recoverable at the application layer

Expected host owner:

- renderer/RHI for low-level reason
- application host for startup failure surfacing

### runtime validation failure

Definition:
Automated runtime/editor smoke validation reached a failed state while executing a validation session.

Current source evidence:

- `RhiSmokeFrameControlState.Failed`
- renderer evidence failure
- camera motion validation failure
- level switching incompletion at frame limit
- shader reload rejection during smoke validation

Expected host owner:

- `RhiSmokeValidation`
- `RhiSmokeSession`
- `RhiSmokeFrameControl`

### editor-only failure

Definition:
Failure belongs to the editor host surface and should not be treated as a runtime-host requirement.

Current source evidence:

- editor UI creation and diagnostics wiring
- `ShaderRecookCoordinator`
- editor-only console commands
- editor smoke viewport capture path
- editor CMake target includes validation and shader recook files excluded from runtime target

Expected host owner:

- `EditorApplication`
- editor module UI/tooling

## New Lifecycle Validation Checklist

Use this checklist before changing startup, frame, validation, or shutdown behavior.

1. Confirm whether the change belongs to runtime host, editor host, launcher/tooling host, or renderer/RHI internals.
2. Keep `Application::Run()` lifecycle ordering explicit and unchanged unless the change intentionally updates trace/session semantics.
3. Preserve runtime/editor separation:
   - runtime remains cooked-content oriented
   - editor-only validation and recook stay out of the runtime target
4. Verify startup ownership for:
   - process-wide config
   - window/input creation
   - scene/level services
   - renderer creation
   - optional console/UI services
5. If adding a project-facing load step, document whether it is:
   - runtime startup content selection
   - editor-only authoring workflow
   - launcher/build workflow
6. If adding a validation path, define:
   - entry condition
   - per-frame evidence collection
   - failure conditions
   - output artifacts
   - exit code behavior
7. If adding a shader workflow hook, state whether it is:
   - reload only
   - out-of-process recook
   - editor only
   - smoke-validation only
8. Make sure failure reporting maps to one of the taxonomy categories in this document.
9. Add or update related architecture docs when ownership moves across:
   - Application
   - GameFramework
   - Renderer
   - RHI
   - Launcher
10. Validate that runtime-only builds still exclude editor-only code paths declared in `Engine/Application/CMakeLists.txt`.

## Known Gaps

- Application-layer error reporting is still mostly string-based and distributed across launcher, validation, and tool subsystems rather than a unified typed error model.
- The reviewed application source does not expose a broader editor project-open/save lifecycle beyond startup level and shader-tool working-directory assumptions.
- Backend/device creation details are not surfaced to application callers as a documented structured contract yet.
- Smoke validation is environment-variable driven; a stable first-class validation command surface is not yet documented at the application layer.
- Shader recook currently contains a concrete `Projects/Showcase` assumption in `ShaderCompilerProcess`, which should eventually become a documented project-resolution contract instead of a hard-coded workflow detail.

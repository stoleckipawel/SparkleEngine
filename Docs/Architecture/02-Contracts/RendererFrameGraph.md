# Renderer Frame Graph And Pass Contract

## Purpose

This document defines the current renderer-side contract for SparkleEngine frame construction, frame graph authoring, pass execution, and frame resource ownership. The goal is to make renderer extension predictable before adding more passes, provider SDK integrations, ray tracing work, or future neural rendering paths.

This is a source-backed contract, not a design rewrite. Where the current implementation is unclear from source, the document marks it as `Needs source confirmation`.

## Non-Goals

- This document does not add new renderer passes.
- This document does not change runtime behavior.
- This document does not redefine backend-native RHI ownership.
- This document does not claim Vulkan or provider parity beyond what current source proves.
- This document does not describe asset import or authoring workflows.

## Renderer Module Responsibilities

The Renderer module owns high-level frame construction and render execution. In current source that includes:

- Host-frame orchestration through `Renderer` and `FramePipeline`.
- Frame graph construction, compilation, execution, and transient resource materialization through `FrameGraph`.
- Pass registration and pass execution glue through `FrameGraph`, `FrameGraphBuilder`, `ShaderPass`, and `PassExecutionContext`.
- Runtime scene snapshot consumption through `FrameContext` building.
- Temporal data generation through `TemporalDataBuilder`.
- Ray tracing scene preparation and pass-facing ray tracing services through `RenderRayTracingScene` and `RenderRayTracingPassServices`.
- Per-frame diagnostics through `FrameExecutionDiagnostics` and `PassExecutionDiagnostics`.

The Renderer module does not own:

- Native backend device creation and queue management. That belongs to RHI.
- Backend-native descriptor heaps, native resource handles, and low-level barriers as authoring concepts. Renderer consumes them through RHI and frame-graph abstractions.
- Gameplay scene ownership. That belongs to GameFramework.
- Application lifecycle ownership. That belongs to Application.

## Relationship Between Renderer, RHI, GameFramework, And Application

Dependency and ownership direction:

- Application owns runtime and editor lifecycle and hosts the renderer.
- GameFramework owns runtime scene and asset state.
- Renderer consumes runtime scene snapshots and translates them into frame data and passes.
- RHI owns backend-facing device services, native resource interop, command submission, diagnostics backends, and presentation services.

Operationally:

- `Renderer` exposes `PrepareHostFrame()`, `RecordHostFrame()`, `SubmitHostFrame()`, viewport presentation, and diagnostics capture.
- `FramePipeline` coordinates frame begin/setup/record/submit/end.
- `FramePipeline::SetupFrame()` captures a `GameScene` snapshot, updates the render camera, and refreshes viewport products.
- `FramePipeline::RecordFrame()` builds a `FrameContext`, prepares ray tracing state, calls `FrameGraph::Setup()`, compiles a `FrameGraphPlan`, and executes it.
- `FrameGraph` executes passes through `RenderCommandContext` plus `PassRuntimeServices`, both of which sit on top of RHI-owned services.

Contract rule:

- Renderer may own render-time scheduling, pass composition, and frame resource declarations.
- Renderer must not become the owner of backend-native API policy that belongs in RHI.

## Frame Lifecycle

The current frame lifecycle visible in source is:

1. `PrepareHostFrame()`
2. `BeginFrame()`
3. `SetupFrame()`
4. `RecordHostFrame()`
5. `RecordFrame()`
6. `SubmitHostFrame()`
7. `SubmitFrame()`
8. `EndFrame()`

More specifically:

1. `BeginFrame()`
   - Handles resize-driven rebuilds.
   - Resets temporal history when resize or extent changes invalidate history.
   - Begins the backend frame through `RenderDeviceServices::BeginFrame()`.
   - Resolves prior frame GPU timings.

2. `SetupFrame()`
   - Ticks the timer.
   - Refreshes viewport render products.
   - Captures the game-scene snapshot.
   - Loads scene textures into renderer-side texture systems.
   - Updates camera and per-frame constants.

3. `RecordFrame()`
   - Consumes temporal history reset requests from scene render state.
   - Builds `FrameContext` from scene snapshot, camera state, scene extent, scene-data builders, ray tracing scene, per-view data, lighting data, and temporal data.
   - Builds upscaler input contract if an upscaler subsystem is present.
   - Binds or clears persistent ray tracing resources.
   - Calls `FrameGraph::Setup(frame)`.
   - Calls `FrameGraph::Compile()`.
   - Creates `PassRuntimeServices`.
   - Executes the compiled frame graph with diagnostics.

4. `SubmitFrame()`
   - Submits the backend frame.

5. `EndFrame()`
   - Advances frames in flight.

Contract rule:

- `FramePipeline` owns per-frame orchestration and state refresh.
- `FrameGraph` owns pass registration, dependency compilation, barrier planning, and pass execution for that frame.

## Pass Lifecycle

The pass lifecycle in current source has three distinct phases:

1. Authoring/registration
2. Setup/declaration
3. Execution

### 1. Authoring/Registration

Passes are registered through `FrameGraph` or `FrameGraphBuilder`:

- `AddPass(...)`
- `AddRasterPass<TPass>(...)`
- `AddComputePass<TPass>(...)`
- `AddRasterShaderPass<TPass>(...)`
- `AddComputeShaderPass<TPass>(...)`

Typed authored passes use:

- pass parameter allocation through `AllocPassParameters<TPass>()` or `AllocParameters<TParameters>()`
- setup-time parameter usage declaration
- execute-time runtime lookup through `PassRuntimeServices::GetPassRuntime<TPass>()`

### 2. Setup/Declaration

Each registered pass provides a setup callback that receives `PassResourceBuilder&` and optionally `const FrameContext&`.

During setup, a pass must:

- declare every frame-graph resource it reads, writes, or otherwise uses
- bind typed pass parameters that expose frame-graph handles
- rely on `PassResourceBuilder` and shader-pass declaration helpers rather than issuing backend work

Setup is declarative. It is where the frame graph learns dependencies and resource usages.

### 3. Execution

At execution time the pass receives `PassExecutionContext`, which includes:

- `RenderCommandContext& Commands`
- `const FrameContext& Frame`
- `const PassRuntimeServices& RuntimeServices`
- `PassExecutionDiagnostics& Diagnostics`
- `FrameGraphResourceCommands Resources`

Execution may:

- bind pipelines and parameter sets
- resolve frame-graph resources through `Resources`
- emit draws, dispatches, markers, and timers

Execution must not redefine resource ownership or scheduling. That work should already be expressed by setup declarations and the compiled frame-graph plan.

## Pass Registration And Discovery

The current registration shape is centralized in frame building code, not dynamic runtime discovery.

Observed source-backed pattern:

- `BuildFrame(...)` creates top-level frame resources.
- `BuildFrame(...)` calls frame assembly helpers such as `AddGBufferPasses(...)`, `AddRayTracingInfrastructurePasses(...)`, `AddLightingPasses(...)`, `AddPostProcessingPasses(...)`, `AddDebugPasses(...)`, and `AddPresentationPass(...)`.
- Those helpers allocate typed pass parameters and register typed shader passes through `FrameGraphBuilder`.

Contract rule:

- New passes should be added through frame assembly helpers or equivalent renderer-owned registration points.
- Passes should not self-register through global side effects.
- Registration order is authoring order; execution order is determined later by frame-graph compilation.

`Needs source confirmation`:

- Whether there are any secondary pass registration paths outside the main frame assembly path that should be treated as first-class contract surface.

## Resource Declaration Rules

Renderer pass declarations currently use:

- `PassResourceDeclaration`
- `PassResourceBuilder`
- `PassResourceDeclarationSink`
- `DeclareShaderPassParameterUsages(...)`
- `FrameGraphResourceContractDiagnostics::ValidatePassDeclarations(...)`

Each declaration records:

- a `FrameGraphResourceHandle`
- a `ResourceUsage`
- a human-readable label

Contract rules:

1. Every pass must declare all frame-graph resource reads and writes during setup.
2. Passes must use typed handles:
   - `FrameGraphTextureHandle`
   - `FrameGraphBufferHandle`
   - `FrameGraphAccelerationStructureHandle`
3. Parameter-driven shader passes must expose valid `PassParameterSet` bindings for all frame-graph resources they use.
4. Uniform data and sampler sets are not frame-graph resources and should not be treated as ownership edges in the frame graph.
5. Pass labels should be stable and descriptive enough for diagnostics.

Authoring implication:

- If a pass touches a texture, buffer, or acceleration structure at execute time, that resource should already have been declared in setup.

## Transient Versus Persistent Resource Ownership

Current source exposes two major ownership classes:

- transient frame-graph resources created inside the frame graph
- persistent imported or reserved resources that are rebound to external/native resources across frames

### Transient Resources

Transient resources are created with calls such as:

- `CreateTexture(...)`
- `CreateBuffer(...)`

`FrameGraph` owns their logical lifetime, compilation tracking, and transient materialization plan. The source also shows explicit transient aliasing and compiled barrier emission helpers inside `FrameGraph`.

Contract rule:

- Renderer passes may author transient resources through frame assembly and pass declarations.
- RHI still owns the native resource implementation beneath the frame-graph abstraction.

### Persistent Resources

Persistent resources are represented by reserve/import/bind flows such as:

- `ImportPersistentTexture(...)`
- `ImportPersistentBuffer(...)`
- `ReservePersistentBuffer(...)`
- `ImportPersistentAccelerationStructure(...)`
- `ReservePersistentAccelerationStructure(...)`
- `BindPersistentBuffer(...)`
- `BindPersistentAccelerationStructure(...)`
- corresponding clear methods

Observed use:

- ray tracing scene TLAS and PTLAS-related buffers are rebound from renderer-managed scene data into reserved frame-graph handles during `RecordFrame()`

Contract rule:

- Persistent frame-graph handles may outlive a single frame-graph compile, but their backing native resources are still supplied externally.
- Renderer systems that own those native resources must bind or clear them before frame-graph execution.

## History Resource Ownership

History in current renderer source exists in more than one form:

- temporal history validity and previous jitter/camera data through `TemporalDataBuilder`
- upscaler history managed by `UpscalerSubsystem`
- denoiser history through `FrameGraphDenoiserRegistration` resources such as `ShadowVisibilityDenoisedHistory`

Contract rules:

1. Temporal history validity belongs to renderer frame state, not to individual ad hoc passes.
2. History reset decisions must happen in renderer-owned coordination points such as:
   - resize handling
   - scene extent changes
   - scene render state reset requests
3. A pass that depends on history must consume a renderer-owned history contract rather than silently caching unmanaged state.

Current source-backed status:

- `TemporalDataBuilder` tracks previous pose, previous jitter, reset requests, and history validity.
- `UpscalerSubsystem` exposes history reset hooks and frame setup hooks.
- `ShadowVisibilityDenoisedHistory` exists as a frame-graph texture registration.

`Needs source confirmation`:

- a single unified renderer-wide history ownership surface for all temporal and denoiser consumers

## Render Target, Depth, Motion Vector, Exposure, Normal, Color, History, Jitter, Camera Matrices, And Frame Index Contracts

This section states what a new pass should assume today.

### Color

Current source-backed frame products:

- `SceneColor`
- `FinalSceneColor`
- `BackBuffer`

`BuildFrame(...)` creates `SceneColor` and `FinalSceneColor`, and imports `BackBuffer`.

Contract rule:

- Main scene color outputs are renderer-owned frame-graph textures.
- Presentation is a separate renderer step and should consume frame products rather than becoming a hidden side effect inside arbitrary passes.

### Depth

Current source-backed depth resources:

- `MainDepth`
- `GBufferDeviceZ`

Contract rule:

- Depth ownership belongs to renderer frame assembly and pass declarations.
- Passes that need depth must consume the declared depth handles instead of inferring depth ownership from scene state.

### Normals

Current source-backed normal resource:

- `GBufferNormal`

It is created in GBuffer setup and consumed by multiple lighting and visualization passes.

Contract rule:

- Normal data is a renderer-produced frame resource.
- Passes that consume normals should read the named normal frame-graph handle supplied by frame assembly or pass parameter bindings.

### Motion Vectors

Current source-backed motion-vector resource:

- `GBufferMotionVector`

It is exposed as a frame build result and fed into upscaling contracts.

Contract rule:

- Motion vectors are a first-class frame resource.
- Temporal, denoising, and upscaling consumers should consume the declared motion-vector handle rather than deriving their own motion history path.

### Exposure

`Needs source confirmation`.

This prompt requires exposure to be part of the contract vocabulary. Current searched source does not show a clearly named renderer-owned frame-graph exposure texture or buffer in the reviewed frame assembly path.

Current contract statement:

- exposure is a renderer frame-resource concept for post, upscaling, and neural-rendering readiness
- the current implementation stores exposure in a 1x1 `R32G32B32A32_Float` frame-graph resource plus previous/current persistent exposure history textures

### History

Current source-backed history-related surfaces:

- `TemporalDataBuilder` history validity and previous jitter/camera state
- `ShadowVisibilityDenoisedHistory`
- upscaler-managed history reset/setup

Contract rule:

- History is a renderer-owned contract surface, not pass-private hidden state.

### Jitter

Current source-backed jitter ownership:

- generated by `TemporalJitterPatterns`
- produced into `PerTemporalConstantBufferData` by `TemporalDataBuilder`
- stored in `RenderViewData` through `perTemporalData` and `temporalState`

Contract rule:

- Jitter generation belongs to renderer temporal systems.
- Passes should consume jitter through frame/view data or dedicated parameter bindings, not by recomputing their own independent sequence.

### Camera Matrices

Current source-backed camera data ownership:

- camera matrices are stored in `PerViewConstantBufferData`
- `RenderViewData` carries `perViewData.Camera`
- `BuildFrameContext(...)` populates the main view from the render camera

Contract rule:

- Camera matrices belong to renderer view/frame data.
- Passes should consume camera matrices through `FrameContext` or pass parameter bindings backed by renderer-owned per-view data.

### Frame Index

Current source-backed frame index usage:

- upscaler input contract uses `m_systems->GetTimer().GetFrameCount()`
- viewport capture requests and contracts also carry frame index fields

Contract rule:

- Frame index is a renderer-owned frame-level input and should be passed through explicit contracts.
- Passes should not invent independent frame counters.

## Barrier And Scheduling Expectations

Current source makes the ownership split fairly clear:

- Passes declare usage.
- `FrameGraph::Compile()` derives execution order and barrier plans.
- `FrameGraph::Execute()` emits compiled barriers and transient aliasing barriers.
- `FrameGraphResourceStateTracker` tracks logical resource state.
- `FramePipeline::TransitionRenderProduct(...)` handles temporary external presentation transitions for viewport presentation flows.

Contract rules:

1. Pass authors own declaration accuracy, not barrier authoring.
2. Frame graph owns scheduling, dependency ordering, and compiled barrier planning.
3. RHI owns actual command-list state transition execution primitives.
4. External presentation or inspection transitions should be isolated to renderer-owned bridge code such as viewport presentation helpers.

Failure indicator already exposed in source:

- `GetLastUnresolvedBarrierWarningCount()`

That warning count should be treated as a signal of declaration or planner mismatch, not something a pass works around manually.

## GPU/CPU Timing And Diagnostics Expectations

Current diagnostics surfaces include:

- `FrameExecutionDiagnostics`
- `PassExecutionDiagnostics`
- backend diagnostics through `RenderDiagnostics`
- renderer memory diagnostics and smoke diagnostics exposed from `Renderer`

Frame-level behavior visible in source:

- frame begin resolves prior timings
- frame execution opens a `"GPU Frame"` scope and timer
- resolved timings are published to the live profiler and ray tracing scene diagnostics

Pass-level behavior visible in source:

- passes receive `PassExecutionDiagnostics`
- passes may begin pass GPU events, timers, sub-events, markers, and sub-timers

Contract rules:

1. GPU timing and markers belong in renderer and RHI diagnostics surfaces, not custom one-off utilities hidden inside passes.
2. A meaningful pass should use pass diagnostics labels that remain stable across captures.
3. Diagnostics should use pass names and resource labels that make sense to an external reviewer.

## Ray Tracing Pass Expectations

Current renderer source shows a ray tracing path built around:

- `RenderRayTracingScene`
- `RenderRayTracingPassServices`
- `RayTracingSceneFrameData`
- `RayTracingSceneFramePlan`
- reserved persistent frame-graph TLAS and PTLAS-support buffers

Observed ownership pattern:

- `BuildFrame(...)` creates ray tracing scene frame-graph resources.
- `RecordFrame()` asks `RenderRayTracingScene` to prepare against scene data.
- If a TLAS is bound, `FramePipeline` binds persistent acceleration structure and PTLAS-support buffers into frame-graph handles.
- Passes consume ray tracing services through `PassRuntimeServices`.

Contract rules:

1. Ray tracing passes must consume acceleration structures through frame-graph handles and renderer pass services, not through direct backend-native handles.
2. Ray tracing scene preparation belongs to renderer scene-prep systems, not to individual shading passes.
3. Backend-native acceleration structure details must remain below this layer.
4. A ray tracing pass should fail gracefully when the renderer has not bound required persistent acceleration-structure resources.

`Needs source confirmation`:

- exact contract boundaries between classic TLAS, partitioned TLAS strategy, and pass-facing resource expectations for every ray tracing consumer

## How To Add A Pass

Use this checklist when authoring a new renderer pass.

1. Choose the correct pass kind.
   - Use raster if the pass binds render targets and graphics pipeline state.
   - Use compute if the pass dispatches compute work.

2. Put the pass in Renderer, not GameFramework or RHI.
   - Scene ownership stays in GameFramework.
   - Backend-native device details stay in RHI.

3. Define or reuse a typed pass parameter structure.
   - Include all frame-graph resources that the pass reads or writes.
   - Include uniforms and samplers through the existing parameter-set model.

4. Declare resources during setup.
   - Use `PassResourceBuilder`.
   - Use `DeclareShaderPassParameterUsages(...)` or the typed `ShaderPass` helpers.
   - Ensure every texture, buffer, or acceleration structure touched at execute time is declared here.

5. Register the pass through frame assembly.
   - Prefer a renderer-owned helper called from `BuildFrame(...)` or the relevant frame-construction path.
   - Allocate parameters through `FrameGraphBuilder`.

6. Execute through `PassExecutionContext`.
   - Use `Commands`, `Resources`, `Frame`, `RuntimeServices`, and `Diagnostics`.
   - Get pipeline runtime through `RuntimeServices.GetPassRuntime<TPass>()`.

7. Consume renderer-owned frame/view state.
   - Use camera matrices, temporal data, jitter, and frame index from renderer-owned contracts.
   - Do not create hidden duplicate state caches inside the pass.

8. Add pass diagnostics.
   - Use stable pass labels and GPU scopes.

9. Validate ownership boundaries.
   - Do not reach down to backend-native API objects from Renderer pass code.
   - Do not pull gameplay ownership into the pass.

10. Verify failure behavior.
   - The pass should tolerate missing optional services by producing a clear no-op or source-backed guarded path.

Acceptance checkpoint for a new pass:

- another developer can identify its inputs, outputs, and owning registration point without reverse-engineering command-list code

## How To Add A Frame Resource

Use this checklist when introducing a new renderer frame resource.

1. Decide whether the resource is transient or persistent.
   - transient: created and owned logically by the frame graph for frame execution
   - persistent: reserved/imported and rebound to an external/native resource across frames

2. Name it clearly.
   - Use stable names such as `SceneColor`, `MainDepth`, `GBufferNormal`, or similarly precise names.

3. Create or reserve it in frame assembly.
   - Prefer top-level frame construction helpers such as `BuildFrame(...)` or the specific subsystem-owned registration helper.

4. Thread the handle through explicit contracts.
   - add it to a frame target struct, frame build result, parameter set, or other renderer-owned contract surface
   - do not hide it behind implicit globals

5. Declare it in every consuming pass.
   - read, write, or use must be visible in setup declarations

6. Define its owner.
   - Renderer owns logical render-time meaning and scheduling.
   - RHI owns underlying backend-native implementation.
   - external systems that provide persistent backing resources must bind them before execution.

7. Define reset/invalidation rules if the resource is temporal or history-bearing.
   - resize
   - extent change
   - camera cut
   - explicit render-state reset

8. Add diagnostics names and labels.
   - resource naming should help barrier diagnostics, captures, and reviewer navigation

9. Confirm whether viewport/product exposure is required.
   - not every frame resource should become a viewport output

10. Document gaps if the resource is a planned contract.
   - use `Needs source confirmation` rather than guessing

## Failure Modes And Validation Expectations

The reviewed source already suggests a few failure and validation categories:

- invalid or incomplete parameter bindings
- undeclared frame-graph resource usage
- unresolved barrier warnings
- missing optional services such as upscaler or ray tracing scene
- invalid viewport product resolution
- missing presentation texture id resolution

Validation expectations:

1. Setup-time resource declarations should be validated through frame-graph contract diagnostics.
2. Shader-pass parameter sets should fail clearly when required frame-graph bindings are absent.
3. Barrier mismatch signals should surface through unresolved-barrier warning counts and diagnostics.
4. Optional provider or ray tracing systems should remain provider-scoped and guarded.
5. Viewport presentation bridges should validate that the requested product and underlying resource exist.

## Known Gaps

- A single explicit document for the top-level frame assembly contract and all builder helper entry points does not yet exist outside source.
- Exposure is not clearly surfaced as a source-proven frame resource in the reviewed files.
- A single unified renderer-wide temporal/history contract across temporal AA, denoising, and upscaling is not yet obvious from the reviewed source.
- The reviewed source shows ray tracing resource preparation and binding, but a pass-by-pass contract for every ray tracing consumer still needs deeper confirmation.
- The exact set of reviewer-facing viewport outputs beyond scene color and optional depth should be documented more explicitly in a follow-up renderer contract.

## Source Anchors

Primary reviewed files for this contract:

- `Engine/Renderer/Public/Renderer.h`
- `Engine/Renderer/Private/FramePipeline/FramePipeline.h`
- `Engine/Renderer/Private/FramePipeline/FramePipeline.cpp`
- `Engine/Renderer/Private/FrameGraph/FrameGraph.h`
- `Engine/Renderer/Private/FrameGraph/Builder/FrameGraphBuilder.h`
- `Engine/Renderer/Private/FrameGraph/Builder/FrameGraphBuilder.cpp`
- `Engine/Renderer/Private/FrameGraph/Builder/PassResourceBuilder.h`
- `Engine/Renderer/Private/FrameGraph/Execution/PassExecutionContext.h`
- `Engine/Renderer/Private/FrameGraph/PassRuntimeServices.h`
- `Engine/Renderer/Private/FrameGraph/Diagnostics/FrameGraphResourceContractDiagnostics.h`
- `Engine/Renderer/Private/Passes/ShaderPass.h`
- `Engine/Renderer/Private/Passes/RenderPassDefinition.h`
- `Engine/Renderer/Private/Frame/Frame.h`
- `Engine/Renderer/Private/Frame/Frame.cpp`
- `Engine/Renderer/Private/Frame/Targets/FrameRenderTargets.h`
- `Engine/Renderer/Private/Frame/GBuffer.cpp`
- `Engine/Renderer/Private/Frame/Builders/TemporalDataBuilder.h`
- `Engine/Renderer/Private/Frame/FrameContext.h`
- `Engine/Renderer/Private/Diagnostics/FrameExecutionDiagnostics.h`
- `Engine/Renderer/Private/Diagnostics/PassExecutionDiagnostics.h`
- `Engine/Renderer/Private/RayTracing/RenderRayTracingPassServices.h`
- `Engine/Renderer/Private/RayTracing/RayTracingSceneFramePlan.h`

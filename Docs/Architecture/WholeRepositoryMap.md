# D. Whole Repository Architecture Map

Status: current source-backed reviewer map; descriptive, not a normative architecture or strategy contract
Last verified: repository-wide map 2026-08-28 at committed `master` revision `20814381`; Launcher ownership and repository code-style routes reverified 2026-08-31 at committed `master` revision `61fe39d9`
Scope: implemented repository structure, target boundaries, runtime and tool flows, project catalog, and current source-evidence limits

## Intent And Authority

This is the shortest current route through the repository. It names implemented owners and executable build boundaries without duplicating their detailed contracts. Code and build configuration remain the authority for implementation. Recheck the named paths when a change moves an owner or target.

Target capability belongs to [Principal Graphics Requirements](../Strategy/Requirements.md), binding implementation rules belong to [Engineering Standards](../Engineering/Standards/README.md), and focused decisions belong to their architecture documents. In particular:

- [Renderer and RHI Architecture Boundary](RendererRhiBoundary.md) owns Renderer/RHI dependency and mechanism rules.
- [World Coordinate, Units, and Transform Contract](WorldCoordinateAndUnits.md) owns spatial semantics.
- [Editor Viewport Camera Architecture](EditorViewportCamera.md) owns the editor-view and scene-camera split.
- [Shader Authoring and Cooked Shader Architecture](Shaders/ShaderAuthoringAndCookedPrograms.md) owns shader-system delivery and its current source-consistency record.
- [Bistro and San Miguel Acceptance Workloads](../Engineering/BistroAndSanMiguelWorkloads.md) owns workload gates and evidence meaning.

Dated assessments retain the source state they observed. They are not silently rewritten into current architecture claims.

## Repository At A Glance

```text
SparkleEngine
|-- CMake/                 build profiles, artifact/project helpers, dependencies, boundary check
|-- Config/                engine defaults
|-- Engine/                runtime modules and engine shaders/assets
|-- Tools/                 launcher, shader compiler, importers, and cookers
|-- Projects/Showcase/     editor/runtime products, level catalog, small source content
|-- Docs/                  strategy, architecture, and engineering authorities
|-- artifacts/             generated validation/development output; not source authority
|-- build/                 generated build trees and dependency cache
|-- logs/, Saved/          generated runtime and user-local state
`-- .sparkle               repository marker used by tooling
```

The top-level CMake project requires C++20, loads the Sparkle build profiles and artifact/project contracts, adds `Engine` and `Tools`, then discovers runnable projects through `Projects/*/.sparkle-project`. Optional content-pipeline, shader-compiler, KTX, NVIDIA Streamline, sanitizer, and strict-warning features are explicit CMake options.

## Build And Module Boundaries

| Source owner | Primary target or product | Direct engine dependency shape |
| --- | --- | --- |
| `Engine/Core` | `SparkleCore` | bottom engine layer; public spdlog contract |
| `Engine/Tasks` | `SparkleTasks` | private `SparkleCore` |
| `Engine/Platform` | `SparklePlatform` | public `SparkleCore` |
| `Engine/RHI` | `SparkleRHICommon`, `SparkleRHIDiagnostics`, backend targets, `SparkleRHI` | public `SparkleCore`; private Platform and selected backends |
| `Engine/GameFramework` | `SparkleGameFramework` | public Core, Platform, and Tasks; no Renderer dependency |
| `Engine/Renderer` | registration/provider targets and `SparkleRenderer` | public Core and RHI; private Platform, GameFramework, Tasks, providers, and ImGui |
| `Engine/Editor` | `SparkleEditor` | public Core, RHI, and Renderer; private GameFramework, Platform, and ImGui |
| `Engine/Application` | `SparkleApplication`, `SparkleApplicationEditor` | runtime host publishes Renderer; editor host layers Editor over the runtime host |
| `Projects/Showcase` | `ShowcaseEditor`, `ShowcaseRuntime` | explicit editor and runtime launch products |

The active RHI target split is:

```text
SparkleRHICommon --object sources--> SparkleRHI
                                      /      \
                            SparkleRHI_D3D12  SparkleRHI_Vulkan
                                      \      /
                              SparkleRHIDiagnostics
```

`SparkleRHI` privately links the selected backend targets; each backend privately links `SparkleRHIDiagnostics`. `SparkleRHI_D3D12` is enabled by default. Vulkan is enabled when the SDK is found, or required explicitly by configuration. CMake assertions reject common/backend source leakage, and the root `architecture_boundary_check` target enforces the repository-level Renderer/RHI rules.

## Runtime Product Flow

```text
ShowcaseEditor / ShowcaseRuntime
        |
SparkleApplicationEditor / SparkleApplication
        |
LevelSession -> GameWorld -> compiled GameSystemGraph
        |
RenderFrameSubmissionExtractor
        |
immutable RenderFrameSubmission
        |
Renderer facade -> RenderCoordinator
        |
RendererHost -> FramePipeline
        |
RenderScene + RenderView + FrameGraph + feature Passes
        |
RenderDeviceServices -> public RHI services
        |
SparkleRHI_D3D12 or SparkleRHI_Vulkan
```

`GameWorld` is gameplay/level authority. It evaluates systems and publishes a sequenced structural `RenderSceneDelta`, immutable per-frame dynamic data, resource tables, and `RenderViewInput` through `RenderFrameSubmission`; the Renderer never queries ECS storage directly.

`RenderCoordinator` owns serial or render-thread submission, bounded frame/control queues, publication of read state, and shutdown settlement. `RendererHost` owns the renderer backend/service graph. `FramePipeline` accepts one monotonic submission, prepares scene/view/GPU state, executes the frame graph, submits the frame, and advances frame-in-flight state.

`SparkleTasks` is the shared task topology/execution module used by GameFramework, Renderer, Application, and the cooking path. A subsystem must not introduce a competing general scheduler.

## Current Renderer Navigation Overlay

The current Renderer owner map is:

| Concern | Current owner |
| --- | --- |
| Public submission, settings, viewport products, capture, and bounded diagnostics | `Engine/Renderer/Public` and the `Renderer` facade |
| Serial/threaded coordination, control queue, frame queue, published read state | `Private/Concurrency` |
| Backend configuration and service lifetime | `Private/Host` |
| Persistent render-scene state, GPU scene, material tables, scene preparation, ray-tracing scene and table plan | `Private/Scene` |
| Per-frame camera/display/temporal state and prepared view work | `Private/View` |
| Frame lifetime, topology rebuild, graph construction/execution, and retired graph generations | `Private/Frame` |
| Technique and product setup/recording | `Private/Passes` |
| Generic graph resources, dependency compilation, barriers, transients, and execution | `Private/FrameGraph` |
| Compute/graphics/ray-tracing runtime materialization and binding | `Private/Pipeline` and `Private/PipelineRuntime` |
| BLAS/TLAS strategy, execution plans, shared RT composition, and RT diagnostics | `Private/RayTracing` |
| Upscaling, ray reconstruction, and Streamline integration | `Private/Providers`, `Private/Upscaling`, `Private/RayReconstruction`, and `Private/Streamline` |

The old private `SceneData`, `Camera`, `FramePipeline`, and `Frame/Core` navigation roots no longer exist. The current high-level route is:

```text
RenderCoordinator
  -> RendererHost
  -> Frame/FramePipeline
       -> Scene/Preparation + Scene/GpuScene + Scene/RayTracing
       -> View/RenderViewBuilder + RenderViewPreparation
       -> Frame/Graph/BuildRenderFrameGraph
            -> Passes/<feature>
            -> FrameGraph/<generic infrastructure>
       -> Frame/Graph/ExecuteRenderFrameGraph
       -> RenderDeviceServices::SubmitFrame
```

The `FrameGraph` object is rebuilt when output/topology, provider selection, lighting/GBuffer selection, shader generation, or a used shader-table-plan generation changes. During each recorded frame, imported resources and typed parameters are applied, then the existing graph runs setup, compile, pass preparation, and execution. This is implemented source shape; its CPU cost still requires measurement before a caching change is justified.

## Renderer Feature Map

| Feature | Current source owner and state |
| --- | --- |
| GBuffer | `Passes/GBuffer`; explicit rasterized and ray-traced algorithms |
| Lighting | `Passes/Lighting`; ReSTIR direct/indirect and reference path-traced branches feed shared composite/sky/presentation products |
| Exposure and presentation | `Passes/PostProcessing` and `Passes/Presentation`; exposure, upscaling, optional visualization, tone mapping, output encoding |
| Persistent GPU scene | `Scene/GpuScene`; geometry, material, lighting, and ray-tracing bindings derived from `RenderScene` |
| Ray-tracing scene | `Scene/RayTracing` plus `RayTracing/Acceleration`; classic and capability-gated partitioned TLAS share scene identity |
| Dual RT execution | GBuffer and direct-shadow effects have typed inline-query and native-pipeline source frontends selected before graph construction |
| Shader-table mapping | `RayTracingShaderTablePlan` owns Surface/ShadowVisibility order, checked record indexing, instance contributions, invalidation, and bounded metrics |
| Image providers | Renderer-owned provider stack with NVIDIA Streamline/DLSS adapters and non-provider presentation paths |
| Diagnostics and capture | bounded Renderer/RHI snapshots plus viewport capture; generated evidence is not implementation authority |

The native D3D12/Vulkan ray-tracing pipeline, shader-table, and trace-command source paths are present, as are reachable GBuffer and shadow compositions. Current documentation does not claim paired native execution, output parity, reload/retirement behavior, or performance proof; those remain explicit Phase 12 obligations in the shader implementation authority.

## RHI Map

`RenderHardwareInterface` exposes focused services for resources, descriptors, pipelines, uploads, ray tracing, interop, capture, diagnostics, and presentation. `RenderDeviceServices` owns backend creation, command recording/submission, queue waits/tokens, resize, frame begin/submit, and frames-in-flight.

Common RHI code owns backend-neutral contracts and validation, including shader map/library records, shader parameters, graphics/ray-tracing descriptors, and checked shader-table packing. Native object creation and command emission remain inside `Private/D3D12` or `Private/Vulkan`.

Current ray-tracing vocabulary distinguishes acceleration-structure, inline-query, and native-pipeline capabilities. Public contracts include classic and partitioned TLAS, ray-tracing pipeline composition descriptors, shader tables, and `TraceRays`; backend-private implementations lower them to D3D12 state objects/dispatch tables or Vulkan ray-tracing pipelines/group handles/SBT regions.

## Shader Source, Cook, And Runtime Flow

```text
virtual /Engine or /Project shader path
        |
typed GlobalShader registration + Parameters
        |
ShaderCompiler catalog and immutable compile jobs
        |
DXC or Slang -> DXIL/SPIR-V + reflection
        |
transactional GlobalShaderMap.smap
              + CookedShaderLibrary.slib
              + dependency manifest
        |
RenderPassRuntimeCache generation
        |
typed compute / graphics / ray-tracing materialization
        |
FrameGraph Draw / Dispatch / TraceRays
```

`Engine/Renderer/ShaderRegistrations` contains 35 typed shader registrations at the verified revision. Product source registrations use vertex, pixel, compute, ray-generation, miss, closest-hit, and any-hit stages. Geometry, hull, domain, intersection, and callable remain schema/compiler vocabulary without a current Renderer product registration.

The ShaderCompiler default selection cooks the catalog; `--shader-id` selects one registered shader and repeatable `--changed` paths select affected types through the persisted dependency manifest. Default targets are DXIL SM 6.6 and SPIR-V 1.6. Every selected job compiles, while identical jobs within one operation share a producer/result; no persistent compiler-result cache is retained.

The old authored shader-package identity, package registry, and `.sparkshader` runtime path have been replaced by the generated global shader map and content-addressed code library. Runtime generation replacement validates before activation and retires old runtime objects by submitted-queue state.

## Content And Level Flow

```text
project level/catalog + external or repository source asset
        |
SourceImporters (glTF/GLB/FBX production routes)
        |
Scene/Mesh/Material/Texture cookers via AssetCooker
        |
cooked-only GameFramework loaders
        |
LevelSession -> SceneLoadTaskGraph -> GameWorld
```

`Projects/Showcase/Levels.catalog` is the single Showcase level/acquisition catalog. At the verified revision it contains 16 level records:

- six asset-pack-free runtime levels: `Empty`, `Sponza`, `ABeautifulGame`, `DamagedHelmet`, `DiffuseTransmissionPlant`, and `CesiumMan`;
- seven external-pack runtime-supported levels: `ModernSponza`, `ModernSponzaCandles`, `ModernSponzaKnight`, `BistroExterior`, `BistroInteriorWine`, `LPSHead`, and `CornellBox`;
- three source-readiness records that are not runtime-supported: `JungleRuins`, `SanMiguelHigh`, and `SanMiguelLow`.

The default runtime level is `Empty`; `SPARKLE_STARTUP_LEVEL` selects another catalog level. If catalog loading fails or `Empty` is absent, `LevelRegistry` creates the built-in empty fallback. External assets remain outside normal source history and are acquired through catalog metadata. Runtime support is distinct from download support and from the user's selected active set.

The catalog also records disabled future Modern Sponza add-ons for Ivy, Trees, Flood, and Volumetric Explosion with explicit implementation blockers. Their presence in metadata is not runtime support.

## Tools And Launcher Map

| Tool area | Responsibility |
| --- | --- |
| `Tools/Launcher/SparkleLauncher` | GUI/workflow shell plus reusable core operations for level sync/run, build generation/compilation, cook, clean, dependency sync, settings, and toolchain detection |
| `Tools/Shaders/ShaderCompiler` | typed catalog validation, compile planning/execution, reflection, map/library publication, changed-source selection, inspection, and optional analysis artifacts |
| `Tools/Shaders/ShaderContracts` | narrow shared shader-contract catalog surface used by the tool build |
| `Tools/Import/SourceImporters` | canonical source-scene import and normalization |
| `Tools/Cooking/*Cooker` | texture, mesh, material, scene, and project cook products |
| `Tools/Support/ToolConsoleSupport` | shared host-tool console support |

The Launcher GUI uses the deployed `RepositoryRoot.txt` as its repository authority. `LauncherMainWindow` remains the lifecycle and composition shell; `LauncherWorkflowPanel` owns workflow-catalog navigation and selection state, while `LauncherActivityPanel` owns the in-memory run list and output presentation. Quick Start projects the level catalog and resolves a requested level/run mode through existing typed sync, build, cook, and final-run capabilities. The Build, Cook, Sync, and Clean pages project those same backend operations; they do not own parallel implementations.

`ShowcaseEditor` links the editor host and `ShowcaseRuntime` links the runtime host. Distribution packaging remains manual and outside Launcher ownership.

## Validation And Evidence Boundaries

- `architecture_boundary_check` is mandatory when Renderer/RHI boundaries change.
- `code_style_check` checks present tracked owned C++, headers, HLSL, and HLSLI with clang-format 22.1.3 plus the authored rules clang-format cannot enforce. The C/C++ subset is baseline-clean; the all-source target intentionally remains failing until the shader acceptance gate permits shader migration.
- `ShaderCompilerCliValidation` is a focused custom target owned by the ShaderCompiler build.
- No active CTest registration is present in the verified source tree; do not infer an automated suite from local generated build artifacts.
- Generated build, cooked, log, capture, and validation artifacts are evidence only when their revision, configuration, command, hardware/runtime provenance, and result are recorded.
- A responsive process, source reachability, successful configure, or one backend does not prove visual correctness, parity, performance, or shipment.

The repository-wide documentation reconciliation remains a static source/build-configuration audit. The 2026-08-30 Launcher ownership update adds a focused `SparkleLauncher` `DevelopmentEditor` target-build result, and the code-style migration adds a passing C/C++ format check. Shader validation is blocked before migration, so neither update adds shader-cook, runtime, capture, performance, or whole-repository build evidence.

## Primary Source Routes

- `CMakeLists.txt`, `Engine/CMakeLists.txt`, and each module/tool `CMakeLists.txt`
- `CMake/ArchitectureBoundaryCheck.cmake`
- `CMake/CodeStyle.ps1`
- `Engine/Application/Public/RuntimeApplication.h`
- `Engine/GameFramework/Public/World/GameWorld.h`
- `Engine/GameFramework/Public/Rendering/RenderFrameSubmission.h`
- `Engine/GameFramework/Private/World/Extraction/RenderFrameSubmissionExtractor.*`
- `Engine/Renderer/Public/Renderer.h`
- `Engine/Renderer/Private/Concurrency/Coordinator/RenderCoordinator.*`
- `Engine/Renderer/Private/Host/RendererHost.*`
- `Engine/Renderer/Private/Frame/FramePipeline.*`
- `Engine/Renderer/Private/Frame/Graph/BuildRenderFrameGraph.*`
- `Engine/Renderer/Private/Frame/Graph/ExecuteRenderFrameGraph.*`
- `Engine/Renderer/Private/Scene/RayTracing/RayTracingShaderTablePlan.*`
- `Engine/Renderer/Private/Pipeline/RenderPassRuntimeCache.*`
- `Engine/RHI/Public/Device/RenderHardwareInterface.h`
- `Engine/RHI/Public/Device/RenderDeviceServices.h`
- `Engine/RHI/Public/RayTracing`
- `Tools/Shaders/ShaderCompiler/Private/Cooking`
- `Tools/Launcher/SparkleLauncher/Private`
- `Projects/Showcase/Levels.catalog`

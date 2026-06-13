# Repository Threading Readiness

Status: after/target crosscutting contract
Date: 2026-06-13
Last synchronized: 2026-06-13

Related architecture:

- [repository-target-architecture.md](repository-target-architecture.md)
- [repository-target-graphs.md](repository-target-graphs.md)
- [repository-target-folder-architecture.md](repository-target-folder-architecture.md)
- [system-design-index.md](system-design-index.md)
- [../../plans/rhi-renderer-review-ready-implementation-plan.md](../../plans/rhi-renderer-review-ready-implementation-plan.md)

## Purpose

This document defines the threading-ready shape SparkleEngine should converge toward while the current refactor proceeds. It does not require implementing a job system, render thread, async compute scheduler, or parallel cooker immediately. It requires the architecture to avoid choices that would make those systems hard later.

The target is simple: mutable state has one owner, cross-system handoffs use immutable or versioned contracts, GPU command recording can be split into independent batches, tool work can be planned as deterministic jobs, and diagnostics can explain which job, frame, thread, queue, or artifact failed.

## Reference Basis

These sources are used as calibration points for the target shape:

| Reference | Source | Pattern used by Sparkle |
| --- | --- | --- |
| NVIDIA Donut core/thread pool | https://github.com/NVIDIA-RTX/Donut/blob/main/include/donut/engine/ThreadPool.h | A small engine-level thread pool queues tasks and waits for completion without making renderer modules depend on app/UI code. |
| NVIDIA Donut threaded rendering sample | https://github.com/NVIDIA-RTX/Donut-Samples/blob/main/examples/threaded_rendering/threaded_rendering.cpp | Each cubemap face records into its own command list, waits for worker tasks, then submits a command-list array. Sparkle should keep command recording data separable by pass/view/batch. |
| AMD Cauldron thread pool | https://github.com/GPUOpen-LibrariesAndSDKs/Cauldron/blob/master/src/common/Misc/threadpool.h | Tool/runtime support code can expose focused task execution without turning every system into a generic global service. |
| AMD Cauldron D3D12 command-list ring | https://github.com/GPUOpen-LibrariesAndSDKs/Cauldron/blob/master/src/DX12/base/CommandListRing.h | Backend command allocators/lists are pooled per back buffer and per requested command-list count. Sparkle RHI should keep per-frame command allocation ownership explicit. |
| AMD Cauldron Vulkan command-list ring | https://github.com/GPUOpen-LibrariesAndSDKs/Cauldron/blob/master/src/VK/base/CommandListRing.h | Vulkan command buffers/pools/fences are recycled per frame, making queue and fence ownership visible. |
| Diligent Samples Tutorial 06 | https://github.com/DiligentGraphics/DiligentSamples/tree/master/Tutorials/Tutorial06_Multithreading | Parallel rendering uses worker threads with their own deferred contexts and command lists; dynamic resources are finished after submission. Sparkle should avoid shared mutable pass state during recording. |
| Diligent Samples Tutorial 23 | https://github.com/DiligentGraphics/DiligentSamples/tree/master/Tutorials/Tutorial23_CommandQueues | Compute, transfer, and graphics queues use explicit contexts and fences. Sparkle should model queue/fence ownership before adding async compute/transfer. |
| NVIDIA NVRHI programming guide | https://github.com/NVIDIA-RTX/NVRHI/blob/main/doc/ProgrammingGuide.md | Command lists can be recorded in parallel, so resource state cannot be tracked as one global mutable timeline. Sparkle frame graph plans and command batches must carry entry/exit state intent before parallel recording is introduced. |
| NVIDIA async compute guidance | https://developer.nvidia.com/blog/advanced-api-performance-async-compute-and-overlap/ | Async overlap should be driven by measurement, resource hazards, queue fences, and whole-frame evidence, not by speculative scheduling. |

## Non-Goals

- Do not implement a thread pool or job system just because this document exists.
- Do not add locks to make unsafe ownership "thread-safe."
- Do not split a stable single-thread path into jobs without profiling evidence and diagnostics.
- Do not introduce async compute until frame graph hazards, queue ownership, and measurement commands can prove the win.
- Do not preserve mutable shared state as a hidden dependency and call it a future optimization point.

## Target Principles

1. Single-writer ownership. Each mutable domain has exactly one writer during a phase: gameplay mutation, render snapshot build, frame graph compile, command recording, queue submission, asset cook output, launcher operation history.
2. Immutable handoff. Cross-system data moves as snapshots, DTOs, manifests, command batches, reports, or versioned schemas. Consumers do not reach back into producer-private mutable state.
3. Phase separation. Build, snapshot, compile, record, submit, present, import, cook, inspect, and report are separate phases even if they initially run on one thread.
4. Per-frame and per-batch storage. Frame data, command allocators, descriptor scratch, upload scratch, transient graph resources, and tool temp outputs must be scoped so concurrent work cannot trample shared state.
5. Explicit queues and fences. Graphics, compute, transfer, capture/readback, and tool-process work must name queue/context ownership and synchronization points before parallel execution is added.
6. Determinism first. Parallelizable cook/import/shader work must produce stable artifact IDs, output paths, logs, and ordering independent of thread scheduling.
7. Diagnostics include execution identity. Failures should name subsystem, stage, frame index, pass, package/artifact id, job id, thread/batch label, queue, and fence/counter when relevant.
8. Complexity earns its right. A future threading abstraction is accepted only when current callers, validation value, and smaller alternatives are documented.

## Repository Threading Planes

| Plane | Threading-ready owner | Mutable state rule | Handoff contract | Later parallelism enabled |
| --- | --- | --- | --- | --- |
| Core | Foundation primitives only | No domain policy or hidden global scheduler; Stage 24 removed named tool/launcher path policy from Core utilities. | Small value types, diagnostics, optional future task primitives. | Shared low-level utilities without cross-module policy leaks. |
| Platform | OS/window/input owner | UI/window events stay host-owned and are copied into runtime requests; Stage 24 moved ImGui capture policy to the host through an input capture query. | Input/event snapshots or host requests. | Main-thread host loop plus worker-side preparation. |
| GameFramework | Runtime scene and cooked loading | Gameplay scene mutates in the runtime phase only; lighting snapshots are dynamic runtime data, while fixed GPU limits stay in renderer/RHI packing code. | Immutable snapshots and `AssetContracts` cooked records. | Simulation/cook loading can run apart from renderer frame preparation. |
| Renderer scene staging | Renderer frame pipeline | Render-domain scene data is built from snapshots, not live gameplay objects. | Frame-scoped render snapshots, resource update requests. | Parallel culling, skinning prep, material/mesh staging, visibility preparation. |
| Frame graph | Renderer frame graph | Setup records declarations; compile owns dependency/barrier plan; execution consumes a frozen plan. | `FrameGraphPlan`, pass resource declarations, command batches. | Parallel pass setup/compile analysis and independent command recording batches. |
| Pass authoring | Renderer passes | Pass setup/execute cannot mutate shared pass-global state without a declared owner. | Pass definitions, parameter blocks, graph resource handles, diagnostics labels. | Per-pass or per-view command recording. |
| Pipeline runtime | Renderer pipeline runtime and RHI pipeline service | PSO/package caches use explicit keys and generation counters. | `ShaderContracts`, `PsoKey`, package generation, binding layout id. | Background package validation and PSO warmup without implicit type-index coupling. |
| RHI command recording | RHI command service and backend command rings | Command list/allocator ownership is per frame, queue, and recording batch. | RHI command list descriptors, batch ids, queue/fence records. | D3D12/Vulkan parallel command recording and controlled queue submission. |
| RHI queues | RHI queue/submission service | Queue submission is centralized; workers record but do not submit arbitrarily. | Queue submission packets, fences, resource-state ownership. | Async transfer/compute/readback with explicit synchronization. |
| Ray tracing | Renderer RT scene plus RHI AS services | BLAS/TLAS caches have a clear update owner and generation. | AS build requests, TLAS frame data, capability/fallback reports. | Parallel BLAS build preparation and measured async AS build candidates. |
| Source import | SourceImporters | Importer jobs write imported DTOs and diagnostics, not runtime state. | Imported DTOs, source diagnostics, stable source ids. | Parallel source import per asset/file. |
| Focused cooking | Focused cookers | A job owns one output artifact or temp output directory. | Cooked artifact schema, temp output, final atomic publish, report. | Parallel texture/mesh/material/scene/shader cook steps. |
| AssetCooker | Cook orchestration | Planner owns the dependency graph; worker jobs do focused work. | Cook plan, process/library requests, reports, artifact manifest. | Job graph execution, process pools, cache reuse, retry/cancel. |
| ShaderCompiler | Shader tooling | Package output is keyed by package id, backend format, options, and generation. | Shader package manifests, reflection, inspection reports. | Parallel shader compile/reflection/package validation. |
| Launcher | Workflow product | Qt models/UI mutate on the UI thread; LauncherCore owns operation state and process evidence. | Process requests, operation reports, history records. | Nonblocking build/cook/launch workflows and cancellation. |
| CMake/CI | Validation wiring | CI is not a runtime dependency. | Local commands and reports. | Parallel CI jobs without changing architecture ownership. |

Stage 31 artifact validation evidence is tracked in [../artifact-validation-matrix.md](../artifact-validation-matrix.md). It keeps shader packages, cooked textures, materials, meshes, scenes, animations, skeletons, cook plans, and summaries as deterministic producer/consumer records instead of opaque build outputs.

Stage 32 project and engine asset ownership is tracked in [../project-asset-ownership-contract.md](../project-asset-ownership-contract.md). Future parallel cook or smoke jobs should consume project source assets, cook plans, and validation reports; they should not discover work from generated project logs, local UI state, or broken level descriptors.

## Core Engine Receivers

These are the high-value receiver points that must be ready before introducing a render thread, command-recording workers, async queues, or background scene preparation.

| Receiver | Current producer | Data accepted | Mutable owner after Stage 35 | Threading-ready expectation |
| --- | --- | --- | --- | --- |
| `GameScene` snapshot capture | GameFramework runtime scene | `GameSceneSnapshot` with camera, animations, lights, textures, meshes, and materials | GameFramework writes during simulation/load; Renderer reads only the captured value | Future simulation and render preparation can separate because Renderer does not read live component internals while recording. |
| `FramePipeline` scene staging | `GameScene::CaptureSnapshot()` | `RenderSceneSnapshot` | `FramePipeline` owns the render snapshot for the frame phase | Level lifecycle events no longer mutate the frame snapshot through `RendererSystemRoot`; frame setup captures and frame recording consumes one owner-local snapshot. |
| `RenderSceneDataBuilder` | `FramePipeline` | `RenderSceneSnapshot` | Renderer scene staging builders own transient render-domain data | Future culling, material staging, mesh batching, and skinning preparation can become jobs fed by immutable frame input. |
| `FrameContext` | Scene staging, camera, lighting, temporal builders | Render-domain frame data, per-view data, mesh instances, skinning data, RT scene data | `FramePipeline::RecordFrame` owns the frame context | Future workers should receive frame-context slices or pass input packets, not reach back into GameFramework or `RendererSystemRoot`. |
| `FrameGraph` setup/compile | Renderer frame composition and passes | Resource declarations, pass declarations, external imports | Frame graph owns declarations until compile; compiled `FrameGraphPlan` becomes execution input | Future pass setup and command recording can split only after plan/resource/barrier identity is frozen. |
| RHI command recording | Frame graph execution | RHI descriptors, resolved resources/views, barrier plans, command context | RHI backend owns command lists/allocators by frame and queue | Future worker recording must use explicit command batches with pass/view/batch id, entry/exit state, queue type, and allocator/list ownership. |
| RHI submission | `FramePipeline::SubmitFrame` | Ordered command batches and presentation/capture handoffs | RHI device/queue service owns submission | Future async compute/transfer must wait/signal through queue packets; workers must not submit directly. |
| Shader package/runtime publication | Shader registration target and ShaderCompiler | Shader catalogs, backend catalogs, package manifests, reflection, PSO keys | Runtime/compiler consumers read sorted snapshots and immutable package generations | Future shader jobs and PSO warmup can compare stable catalogs without depending on static initialization or filesystem order. |
| Launcher/tool workflows | Launcher UI and CLI requests | Operation requests, process requests, cook plans, reports | LauncherCore and AssetCooker own orchestration state | Future nonblocking UI/workflow execution can schedule stable process requests and reports instead of widget-owned work. |

## Stage 35 Hardening Evidence

| Edge hardened | Blocking shape found | Change made | Validation |
| --- | --- | --- | --- |
| GameFramework -> Renderer -> FrameContext | `RenderSceneSnapshot` lived in `RendererSystemRoot` and could be reset/refreshed by lifecycle coordination outside the frame phase. | `FramePipeline` now owns the render snapshot. `SceneRenderStateCoordinator` only invalidates renderer-owned caches and requests temporal reset; it no longer receives or mutates the frame snapshot. | `ShowcaseRuntime` built in `build-vs2026` with `DevelopmentEditor`. `rg "GetSceneSnapshot\|m_sceneSnapshot"` shows the snapshot is local to `FramePipeline` plus its consumers. |
| Renderer/RHI shader catalog -> ShaderCompiler/runtime | Consumers read mutable registration vectors whose visible order depended on registration order. | `GlobalShaderRegistry` now publishes sorted shader and hit-group snapshots; `FindByName` reads the frozen snapshot. | `ShaderCompiler` built; `ShaderCompiler.exe list-shaders --validate` reported `17` valid typed registrations and `10` packages. |
| Shader backend catalog -> ShaderCompiler jobs | Backend registration enumeration returned mutable registration storage directly. | Builtin shader backends now publish a sorted descriptor snapshot to consumers. | `ShaderCompiler` built in `build-vs2026`. |
| Launcher toolchain discovery -> workflow/process requests | Recursive executable and Qt kit discovery inherited filesystem iterator order. | Tool resolver and Qt kit detection now sort normalized candidate paths before selecting discovered tools/kits. | `SparkleLauncher` built in `build-vs2026`; Qt deploy still reports the existing `VCINSTALLDIR` warning but target exits successfully. |
| Launcher maintenance clean plans -> process steps | Project-generated cleanup targets and clean process steps inherited project directory iterator order. | Maintenance planning/process request generation now collects and sorts project directories before emitting targets/steps. | `SparkleLauncher` built in `build-vs2026`. |
| Shader recook status -> diagnostics | Runtime status text used weak internal wording. | Status now names the shader compiler process directly without implementation/provenance phrasing. | `ShowcaseRuntime` built in `build-vs2026`. |

## Required Data Shapes

Every cross-system handoff must choose one of these shapes before a threaded implementation is attempted:

| Shape | Use for | Required fields |
| --- | --- | --- |
| Immutable snapshot | GameFramework to Renderer, host inputs to runtime, frame data to passes. | Producer, frame/generation id, schema version if persisted, immutable payload, diagnostics label. |
| DTO or manifest | Source import, cooking, shader package definitions, pass catalogs. | Stable id, source path or package id, schema version, producer tool, consumer, validation command. |
| Command batch | Frame graph/RHI recording and future worker submissions. | Frame index, pass/view/batch id, queue type, command-list owner, resource access set, debug label. |
| Queue submission packet | RHI submission and future async compute/transfer. | Queue type, ordered command batches, wait/signal fences, resource ownership transitions, evidence label. |
| Tool job request | AssetCooker, ShaderCompiler, focused cookers, LauncherCore. | Job id, input paths, output temp/final paths, profile/options, dependencies, cancellation token, report path. |
| Report | Diagnostics, cook results, validation, launcher history. | Producer, stage, job/frame/pass/package/artifact id, status, reason, paths, elapsed time when available. |

## Forbidden Threading Shapes

- Shared mutable `Renderer`, `FrameContext`, `RenderSceneData`, pass parameter, cooker, or launcher state accessed by multiple workers without a phase owner.
- Command lists or command allocators borrowed from a global pool without frame/queue/batch ownership.
- Background jobs that mutate GameFramework components, Renderer resources, or cooked artifacts after the consumer has started reading them.
- Global caches keyed by C++ type or pointer identity when package id, artifact id, frame generation, or PSO key is the real contract.
- Async compute/transfer passes that do not declare read/write resources, queue type, waits, signals, and profiling evidence.
- Tool jobs writing final artifacts directly before validation succeeds.
- UI widgets starting threads or tools directly instead of sending requests to LauncherCore.

## System Readiness Checklist

Use this checklist in every stage:

- Owner: Which subsystem owns the mutable state during this phase?
- Phase: Is the work build, import, cook, load, snapshot, graph setup, graph compile, record, submit, present, inspect, or report?
- Handoff: Which snapshot, DTO, manifest, command batch, queue packet, request, or report crosses the boundary?
- Isolation: What per-frame, per-job, per-thread, per-queue, or per-artifact storage prevents shared mutation?
- Ordering: What generation, dependency, queue, fence, or validation step makes ordering explicit?
- Diagnostics: What job/frame/pass/package/artifact id appears in logs and failure reports?
- Determinism: Would output remain stable if the work later runs on two workers?
- Complexity: What code is intentionally kept single-threaded for now, and why?

## Stage Impact

This contract is mandatory target context for every implementation stage. Stages do not have to implement threading, but they must avoid shapes that would block it.

| Stage area | Threading-ready consequence |
| --- | --- |
| Boundary and folder stages | Forbid private edges that force workers to share live owner internals. |
| Shader/pass stages | Pass definitions and package catalogs must be immutable inputs to cook/runtime jobs. |
| RHI service stages | Command, descriptor, upload, capture, and queue services must name frame/queue/batch ownership. |
| Renderer facade/frame graph stages | Build host protocol, frame pipeline, graph setup, compile, and execution as separate phases. |
| Scene/resource stages | Renderer consumes snapshots and emits resource update requests rather than reading live gameplay state. |
| Ray tracing stages | BLAS/TLAS build/update owners and generations must be explicit before parallel AS work is considered. |
| Tooling stages | Import/cook/compiler operations must become deterministic jobs with stable reports and temp output policy. |
| Launcher stages | UI, workflow state, process execution, and history stay separated for nonblocking operation. |
| Build/CI/final stages | Threading readiness is scored as architecture evidence, not as a performance claim. |

## Acceptance Evidence

The repository is threading-ready when:

- Every durable root has a documented mutable-state owner and allowed handoff shape.
- Renderer does not consume mutable GameFramework internals.
- Frame graph pass setup, compile, and execute phases are distinct and diagnostics can report pass/resource/batch identity.
- RHI command recording and submission contracts name frame, queue, batch, allocator/list ownership, waits, and signals.
- Shader/package/PSO identity uses explicit immutable keys and generation counters.
- Import/cook/shader tools can describe work as jobs with deterministic inputs, outputs, dependencies, and reports.
- Launcher workflow state can be driven asynchronously through process requests and operation history without widgets owning work.
- No stage keeps a global mutable registry, compatibility path, or service locator without owner, validation value, and removal stage.

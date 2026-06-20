# Performance And Memory Diagnostics Plan

## Purpose

This document defines the diagnostics surface SparkleEngine should present for principal-level review of GPU, CPU, memory, shader, and backend behavior before heavier rendering, SDK, ray tracing, or neural-rendering features arrive. It focuses on making existing and planned metrics reviewable through stable ownership, editor presentation, and text artifacts.

## Non-goals

- This does not add telemetry code.
- This does not add new editor panels.
- This does not claim a metric exists unless the source reviewed for this document proves it.
- This does not collapse allocator-native diagnostics into renderer summaries without preserving ownership boundaries.
- This is not a backend parity guarantee by itself.

## Review Goal

A principal reviewer should be able to answer the following without stepping through the code:

- Which backend is active, on which adapter, with which capabilities?
- What GPU memory budget and usage does the engine see right now?
- Is memory pressure coming from textures, meshes, ray tracing, upload traffic, or transient work?
- Are descriptor and shader/pipeline surfaces bounded and visible?
- Which passes cost time on the GPU, and how does that compare with CPU orchestration?
- Are shader cache and shader package workflows observable?
- Can the same evidence be surfaced in both editor views and text artifacts?

## Metric Status Meanings

- `existing`: source-backed metric or snapshot exists today.
- `partial`: source-backed surface exists, but presentation, coverage, or persistence is incomplete.
- `planned`: metric is required by the architecture plan but not yet exposed as a concrete data surface.
- `unknown`: reviewed source was not enough to prove the metric.

## Metrics To Expose

| Metric | Current status | Current source-backed surface | Notes |
| --- | --- | --- | --- |
| active backend | existing | `RhiCapabilities.BackendApi`, `RendererSmokeDiagnosticsSnapshot.BackendApi` | Already captured in renderer smoke diagnostics. |
| adapter/vendor/device | existing | `RhiAdapterIdentity`, `RendererSmokeAdapterDiagnostics` | Includes adapter name, driver description, vendor ID, and device ID. |
| API version / feature level | partial | `RhiCapabilities` feature/capability fields, descriptor model, queue support, ray tracing capabilities | Exact API version / D3D feature level field is not yet a single explicit metric surface. |
| validation state | partial | `RhiDiagnosticsCapabilities`, `RenderMessageDiagnostics`, `RenderFailureDiagnostics` | Capability flags exist, but no unified high-level validation-state summary is exposed yet. |
| enabled providers | partial | `RendererSmokeUpscalerDiagnostics`, ray tracing provider fields in `RendererSmokeRayTracingDiagnostics` | Upscaler and PTLAS/top-level provider state exists; a unified provider summary across all feature classes is still missing. |
| memory budget | existing | `RhiMemoryUsageSnapshot.TotalBudgetBytes`, per-category `BudgetBytes` | Allocator-backed and available through `RenderMemoryDiagnostics`. |
| memory usage | existing | `RhiMemoryUsageSnapshot` totals and per-category stats | Includes total used, allocated, transient, delayed destruction, and category-level bytes. |
| residency pressure if available | partial | `RendererMemoryMonitor`, `RendererMemoryPressureLevel`, `RendererMemoryCategoryPressure` | Pressure classification exists as a renderer summary; explicit residency pressure beyond budget ratio is not yet separate. |
| allocation counts | existing | `RhiMemoryCategoryStats.AllocationCount`, `ResourceCount`, `BlockCount`, `DelayedDestructionAllocationCount` | Allocator-native counts exist today. |
| descriptor heap / pool usage | partial | `RhiBindingLimits`, descriptor allocation/release services, descriptor model | Capacity is visible, but live occupancy/pressure counters are not yet exposed as diagnostics. |
| pipeline cache stats | planned | none proven in reviewed source | Shader package generation exists, but no explicit pipeline cache stats surface was found. |
| shader package load timing | partial | shader package generation is exposed; reload path exists; no dedicated timing snapshot was found | Needs explicit timing surface around load/reload operations. |
| pass GPU timings | existing | `FrameExecutionDiagnostics`, `PassExecutionDiagnostics`, `ResolvedGpuTiming`, editor GPU profiler tab | Timing capture and resolve path exists. |
| CPU frame timings | existing | `Diagnostics::LiveProfiler`, editor CPU profiler tab, `SPARKLE_CPU_SCOPE(...)` usage | CPU scope capture and editor visualization exist. |
| upload pressure | partial | `RhiMemoryCategory::Upload`, `SceneMemoryReport.UploadBytes`, upload services/call sites | Upload memory bytes are visible; a first-class pressure metric is still missing. |

## Data Ownership By Module

### RHI ownership

RHI owns backend-native and allocator-native truth.

Owned metrics:

- active backend base identity
- adapter/vendor/device identity
- descriptor model and binding limits
- upload/readback capability flags
- queue capability flags
- present support
- memory allocator backend identity
- allocator-backed memory budget and usage snapshots
- per-allocation debug names
- diagnostic capability flags
- timestamp-query support and resolved tick source
- debug message availability
- live-object/crash diagnostics availability

Primary source-backed types:

- `RhiCapabilities`
- `RhiAdapterIdentity`
- `RhiDiagnosticsCapabilities`
- `RenderDiagnostics`
- `RenderTimingDiagnostics`
- `RenderMessageDiagnostics`
- `RenderFailureDiagnostics`
- `RenderMemoryDiagnostics`
- `RhiMemoryUsageSnapshot`
- `RhiMemoryCategoryStats`
- `RhiMemoryAllocationInfo`

Allocator-specific ownership:

- D3D12 allocator-backed data belongs to `D3D12GpuMemoryAllocator`
- Vulkan allocator-backed data belongs to `VulkanGpuMemoryAllocator`

This separation is important: renderer summaries may aggregate allocator outputs, but must not pretend to be the allocator truth source.

### Renderer ownership

Renderer owns frame-level interpretation, pass-level timings, scene-facing memory summaries, and provider-oriented smoke summaries.

Owned metrics:

- unresolved frame-graph barrier warnings
- resolved GPU frame timing summaries
- pass GPU timing tree
- renderer-level memory pressure classification
- texture streaming policy recommendation snapshot
- scene memory report grouped by renderer-relevant categories
- upscaler/provider smoke status
- ray tracing smoke diagnostics and pass timings

Primary source-backed types:

- `RendererSmokeDiagnosticsSnapshot`
- `RendererSmokeFrameGraphDiagnostics`
- `RendererSmokeFrameTimingDiagnostics`
- `RendererSmokeUpscalerDiagnostics`
- `RendererSmokeRayTracingDiagnostics`
- `RendererMemoryDiagnosticsSnapshot`
- `RendererMemoryCategoryPressure`
- `TextureStreamingMemoryPolicySnapshot`
- `SceneMemoryReport`
- `FrameExecutionDiagnostics`
- `PassExecutionDiagnostics`
- `ResolvedGpuTiming`

### Editor ownership

Editor owns presentation and interactive inspection of diagnostics already provided by runtime and renderer systems.

Current editor evidence:

- `ProfilerPanel` captures `Diagnostics::LiveProfiler` snapshots
- GPU and CPU tabs already exist
- `UI::SetDiagnosticsProviders(...)` wires memory, mesh, texture, shader, and smoke diagnostics into panels
- `ViewportRayTracingDebugOverlay` displays live RT-related smoke metrics
- `UsedMeshesPanel` and `UsedTexturesPanel` surface memory-oriented asset views
- `UsedShadersPanel` surfaces shader artifact inspection

Editor should remain a consumer of diagnostics, not the owner of allocator or backend truth.

### Tooling / launcher ownership

Tooling should own persisted text artifacts and repeatable scenario runs, not low-level metric generation itself.

Examples:

- smoke capture sidecars
- launcher operation logs
- shader inspection output
- future baseline reports and summaries

## Existing Metrics Found

The following concrete diagnostics surfaces already exist in source:

- `RhiDiagnosticsCapabilities`
- `RenderDiagnostics` service family
- `RenderMemoryDiagnostics::GetLatestMemorySnapshot()`
- `RenderMemoryDiagnostics::WriteAllocatorJsonDump(...)`
- `RhiMemoryUsageSnapshot` totals and category/allocation breakdowns
- `RhiCapabilities` backend, binding, queue, upload, present, allocator, and interop capability data
- `RendererMemoryMonitor`
- `RendererMemoryDiagnosticsSnapshot`
- `SceneMemoryReport`
- `RendererSmokeDiagnosticsSnapshot`
- `RendererSmokeRayTracingDiagnostics`
- `FrameExecutionDiagnostics`
- `PassExecutionDiagnostics`
- `ResolvedGpuTiming`
- editor `ProfilerPanel` CPU/GPU tabs
- editor asset-oriented mesh/texture memory inspection panels
- editor shader artifact inspection

## Data Model Shape To Preserve

The diagnostics plan should keep three layers distinct:

1. Backend-native truth:
   - D3D12MA / VMA / backend capability and timestamp data
2. Renderer summaries:
   - pressure classification
   - pass timing summaries
   - provider status
   - scene-facing grouping
3. Editor and text presentation:
   - interactive views
   - smoke reports
   - baseline snapshots

Do not merge these into one giant untyped dump. Reviewers should be able to tell whether a number came from:

- backend allocator state
- renderer aggregation
- editor display logic

## Editor Display Expectations

The editor should make diagnostics legible under review pressure.

### Existing display surfaces to keep using

- `ProfilerPanel` for CPU and GPU timing trees
- `ViewportRayTracingDebugOverlay` for RT/provider-specific live metrics
- `UsedMeshesPanel` for CPU/GPU mesh footprint and residency-style state
- `UsedTexturesPanel` for texture memory and residency-style state
- `UsedShadersPanel` for shader artifact and package inspection

### Expected presentation behavior

1. Show high-level backend identity near the top of a review session:
   - backend
   - adapter name
   - vendor/device IDs
   - driver description
2. Keep CPU and GPU timing views separate but navigable together.
3. Show renderer memory pressure as a renderer summary, with a drill-down path into allocator-backed categories.
4. Show upload-related data separately from scene asset residency so transient traffic is not mistaken for stable scene cost.
5. Keep provider status explicit:
   - enabled provider
   - unavailable / unsupported / failed reasoning where available
6. Avoid hiding lack of data:
   - if a backend lacks a metric, show that clearly rather than synthesizing a fake value.

### Display additions that are planned, not yet present

- a unified backend/memory/perf summary panel
- explicit descriptor heap/pool occupancy view
- pipeline cache statistics view
- shader package load/reload timing panel
- baseline scenario comparison view

## Text Artifact Expectations

Diagnostics must be reviewable outside the editor.

### Existing artifact-capable surfaces

- allocator JSON dumps through `RenderMemoryDiagnostics::WriteAllocatorJsonDump(...)`
- smoke capture image + `.json` + `.timing.csv`
- shader debug artifacts including `reflection.json`
- trace export at `logs/trace.json`
- launcher logs per operation

### Planned artifact behavior

For each baseline scenario, emit a machine-readable and reviewer-readable bundle containing:

- backend identity
- adapter/vendor/device
- capability and validation summary
- memory snapshot
- renderer memory summary
- resolved GPU timing list
- CPU frame timing summary
- provider status summary
- scenario metadata

Recommended artifact layout:

- `artifacts/diagnostics/perf/<scenario>/<timestamp-or-run-id>/summary.json`
- `artifacts/diagnostics/perf/<scenario>/<timestamp-or-run-id>/summary.md`
- `artifacts/diagnostics/perf/<scenario>/<timestamp-or-run-id>/allocator.json`
- `artifacts/diagnostics/perf/<scenario>/<timestamp-or-run-id>/gpu-timings.json`
- `artifacts/diagnostics/perf/<scenario>/<timestamp-or-run-id>/cpu-trace.json`

These paths are planned conventions for future implementation, not current guaranteed outputs.

## Baseline Scenarios

Each scenario should be captured on both D3D12 and Vulkan where backend support exists.

### empty frame

Goal:

- establish minimum host + renderer frame cost
- verify backend startup, presentation, and idle frame timing

Metrics of interest:

- backend identity
- final frame GPU time
- CPU frame time
- memory baseline
- descriptor baseline

Current status:

- partial

Why partial:

- frame timing capture exists, but there is no dedicated named empty-frame baseline workflow yet

### one static mesh / material

Goal:

- establish the smallest representative content frame

Metrics of interest:

- mesh bytes
- texture bytes
- upload bytes
- pass timings
- frame time

Current status:

- partial

Why partial:

- mesh/texture diagnostics exist, but no baseline capture workflow is exposed as a stable scenario command

### many materials

Goal:

- expose binding, shader-package, and descriptor scaling pressure

Metrics of interest:

- descriptor usage
- shader package generation/load behavior
- pass GPU timing shifts
- material-heavy memory footprint

Current status:

- planned

### descriptor pressure

Goal:

- show how descriptor usage approaches RHI binding limits

Metrics of interest:

- live descriptor occupancy
- binding limits
- failure/warning thresholds

Current status:

- planned

Reason:

- descriptor limits exist in `RhiCapabilities`, but live occupancy diagnostics were not found in reviewed source

### upload pressure

Goal:

- show transient bandwidth and staging pressure during heavy streaming or asset bring-up

Metrics of interest:

- upload bytes
- upload allocation counts
- frame-time disruption
- delayed destruction growth

Current status:

- partial

Reason:

- upload category bytes exist in memory summaries, but no first-class upload-pressure metric/report exists yet

### shader compile cache miss

Goal:

- show cold shader cook/load cost

Metrics of interest:

- cook time
- cache misses
- package generation / reload timing
- artifact generation

Current status:

- partial

Reason:

- ShaderCompiler reports cache hits/misses during cook, but runtime-facing timing summaries are still incomplete

### shader compile cache hit

Goal:

- show warm-path shader workflow cost and stability

Metrics of interest:

- cache hits
- reduced cook time
- reload timing

Current status:

- partial

### backend startup / shutdown

Goal:

- show backend bring-up cost, diagnostics capability state, and teardown cleanliness

Metrics of interest:

- adapter/backend identity
- validation capability flags
- startup CPU timing
- initial memory snapshot
- shutdown/live-object or crash-diagnostics readiness where supported

Current status:

- partial

Reason:

- application and smoke startup/shutdown flows exist, but no dedicated startup/shutdown performance report artifact is exposed yet

## Metric Ownership Table

| Metric family | Owning module | Primary source type(s) | Current status |
| --- | --- | --- | --- |
| backend identity | RHI | `RhiCapabilities`, `RhiAdapterIdentity` | existing |
| diagnostic capability state | RHI | `RhiDiagnosticsCapabilities` | existing |
| allocator-backed memory | RHI backend allocators | `RhiMemoryUsageSnapshot`, `D3D12GpuMemoryAllocator`, `VulkanGpuMemoryAllocator` | existing |
| renderer memory pressure summary | Renderer | `RendererMemoryDiagnosticsSnapshot`, `RendererMemoryMonitor` | existing |
| pass GPU timing | Renderer + RHI timing diagnostics | `FrameExecutionDiagnostics`, `PassExecutionDiagnostics`, `ResolvedGpuTiming` | existing |
| CPU frame timing | Core diagnostics + Editor presentation | `Diagnostics::LiveProfiler` snapshots | existing |
| provider status summary | Renderer provider integration | `RendererSmokeUpscalerDiagnostics`, `RendererSmokeRayTracingDiagnostics` | partial |
| descriptor pressure | RHI + Renderer presentation | planned occupancy snapshot built on descriptor services and limits | planned |
| pipeline cache stats | RHI / Renderer pipeline ownership | no reviewed concrete surface | planned |
| shader package load timing | Renderer shader/runtime path | generation exists, timing surface incomplete | partial |
| upload pressure summary | RHI allocator data + Renderer summary | `RhiMemoryCategory::Upload`, `SceneMemoryReport.UploadBytes` | partial |

## Principal-Level Review Support

This diagnostics plan should support principal-level review in four ways:

1. It should reveal hardware-aware thinking:
   - budgets
   - pressure
   - descriptor limits
   - upload behavior
   - backend capability shape
2. It should show production discipline:
   - stable ownership
   - no hidden backend-native leaks into editor-only summaries
   - honest distinction between existing and planned metrics
3. It should make performance reasoning navigable:
   - CPU frame tree
   - GPU pass tree
   - memory summary
   - provider state
4. It should produce artifacts that can be inspected offline by a reviewer or hiring panel.

## New Metric Checklist

1. Identify whether the metric belongs first to:
   - RHI
   - Renderer
   - Editor
   - tooling/artifact generation
2. Decide whether it is:
   - allocator-native truth
   - renderer summary
   - editor presentation
3. Mark the metric as `existing`, `partial`, `planned`, or `unknown`.
4. Define units explicitly:
   - bytes
   - counts
   - milliseconds
   - ratios
   - booleans/capabilities
5. Define backend coverage honestly.
6. Define whether the metric is frame-local, rolling, point-in-time, or scenario-batch.
7. Add a text artifact path before adding a flashy UI-only view.
8. Preserve D3D12MA/VMA-backed raw data separately from renderer-level interpretation.
9. If the metric depends on provider state, keep the provider-neutral contract wording.
10. Tie the metric to at least one baseline scenario.

## Known Gaps

- No explicit API-version / D3D feature-level summary field was found as a unified diagnostics surface.
- No live descriptor heap/pool occupancy snapshot was found in the reviewed source.
- No explicit pipeline cache statistics snapshot was found.
- Shader package generation is exposed, but shader package load/reload timing is not yet a first-class diagnostics struct.
- Upload activity is partly visible through memory categories and scene reports, but not yet elevated into a stable upload-pressure diagnostic.
- Existing smoke diagnostics are strong for backend identity, provider state, frame timing, and ray tracing, but they are not yet packaged as a generalized performance baseline report.
- Editor presentation surfaces exist in several focused panels, but there is not yet one principal-review dashboard that ties backend, memory, pass timing, and provider state together.

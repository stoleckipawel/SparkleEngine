# G. Advanced Graphics Engine Executive Summary

Status: advanced graphics requirements summary
Date: 2026-07-04
Scope: engine direction derived from senior graphics, graphics tools, advanced rendering, and neural rendering expectations

## Purpose

This document turns advanced graphics engineering expectations into engine requirements. The goal is to steer SparkleEngine toward evidence that matters for advanced graphics and neural rendering work while preserving the repository's deletion-first cleanup direction.

The engine should read as a compact, serious renderer-first engine that proves:

- modern C++ and high-level shader engineering
- D3D12 and Vulkan workload understanding
- real-time rendering, rasterization, ray tracing, GI, and path tracing fundamentals
- GPU debugging and profiling fluency
- shader compiler, reflection, cook, and runtime ABI discipline
- first-principles performance thinking across CPU, GPU, memory, descriptors, and pipelines
- readiness for neural rendering and GPU inference without premature dependency bloat
- clean technical writing and production-oriented code review judgment

## Executive Decision

Prioritize making SparkleEngine smaller, more explicit, and more evidence-rich.

Do not chase feature count. Build a repo where a senior reviewer can quickly answer:

- Where is the D3D12/Vulkan boundary?
- How are frame graph resources, barriers, descriptors, memory, and queues owned?
- How do shaders move from source to cooked runtime package?
- How is a frame analyzed for CPU/GPU cost?
- How are ray tracing, GI, path tracing, and temporal/provider signals represented?
- How would a neural rendering prototype become an efficient shader/kernel path?
- What is product code, what is experimental, and what has been intentionally deleted?

Persona filter:

- The engine should develop the advanced graphics engineer described in `H_AdvancedGraphicsEngineerPersona.md`.
- Every staged refactor should improve at least one persona pillar: explicit graphics API ownership, renderer feature depth, shader/kernel craft, GPU architecture thinking, neural rendering readiness, debugging/tool fluency, or product engineering discipline.
- If a task cannot name the preserved capability and the code/API/content weight it removes, it should not be promoted into the near-term plan.

## Top Priorities

### P0. Product Identity And Review Path

Write and enforce a short product identity:

- Sparkle is a renderer-first engine.
- It supports multiple projects and levels as a first-class workflow.
- The default review path uses a curated level set, while heavyweight media is optional.
- Level/content organization must prevent source, cook, launch, and package clutter.
- Tools exist only for build, cook, launch, shader pipeline, package if owned, and workload inspection.
- Unowned research scaffolding is gated, minimized, or deleted.

Why this matters:

- Senior graphics roles value judgment. A slim, declared product scope is stronger than a sprawling codebase.

### Late. D3D12/Vulkan Workload Analysis

The repo should eventually prove it can analyze how graphics APIs are used, but this is late-stage work after renderer features and cleanup are stronger.

Target evidence:

- one frame-graph/RHI workload summary that reports passes, queues, resource classes, barriers, transient allocations, descriptor pressure, pipeline count, shader packages, ray tracing builds, and GPU timing
- backend capability comparison for D3D12 versus Vulkan
- a small workflow that opens the data in text/CSV/JSON only when explicitly requested

Deletion-first rule:

- This should replace current scattered diagnostics and reports, not add another observer layer.
- Do not start this by adding new documentation, diagnostics, logging, validation, scaffolding, wrappers, or abstractions.

### P0. Shader Engineering And ABI Discipline

Keep shader compiler strength as a centerpiece.

Target evidence:

- HLSL SM6-first shader code with clear shared libraries for BRDF, geometry, lighting, ray tracing, material, and display
- Slang support positioned for future neural rendering and cross-target shader work
- DXIL/SPIR-V package cooking
- reflection and parameter contract verification
- package inspection commands
- no default debug artifact bundle unless explicitly requested

Skip:

- extra shader demos that do not serve the renderer
- debug bundles and stats CSV in default workflows
- generated code unless it deletes more duplicated binding code than it adds

### P0. GPU Debugging And Capture Capability

Keep professional graphics debugging support:

- PIX/RenderDoc/Nsight markers
- object names
- timestamp queries
- backend debug layers
- fatal API result checks
- screenshot/BMP capture as an intentional editor/tool capability

Cut:

- routine logs
- smoke/ad hoc ownership of screenshot/BMP capture
- public diagnostic snapshots that only feed panels or reports

Late target evidence:

- after feature cleanup, use existing debugger/profiler support to inspect one frame, one pass, one shader package, one memory snapshot, and one ray tracing build.

### P1. Ray Tracing, GI, And Path Tracing

Make ray tracing scope honest without demoting PTLAS.

Target evidence:

- two product acceleration structure paths: classic TLAS and PTLAS
- clear BLAS/TLAS lifecycle
- D3D12 and Vulkan PTLAS capability preserved where supported
- PTLAS implementation minimized toward the original reference flow
- direct lighting reservoir path described as native reservoir-based direct lighting
- reference path tracing defined as either debug reference or progressive/offline reference
- GI/path tracing work tied to material/light transport correctness, not to extra debug views

Cut:

- future GPU-pack scaffolding
- PTLAS planner metrics/diagnostics that do not build, update, or trace
- metrics that do not drive shipping decisions

### P1. Neural Rendering And GPU Inference Readiness

Do not bolt on a heavy ML stack yet. Prepare the architecture first.

Target evidence:

- shader ABI can represent tensor-like resources, feature profiles, specialization constants, and provider resource contracts
- Slang/HLSL path is kept clean for neural shader experiments
- one design note maps model-to-shader translation: source model, operator subset, tensor layout, resource binding, dispatch schedule, memory footprint, precision, validation
- one small future prototype can demonstrate a neural denoiser/operator only if it replaces an existing debug/demo path

Skip for now:

- bundling PyTorch, TensorFlow, ONNX Runtime, CUDA, or vendor-specific compute backends into the engine
- training workflows inside the engine
- broad ML framework integration without a concrete renderer feature

### Late. CPU/GPU Performance Evidence

Profiling and measurement should be late-stage work. First make the renderer feature surface worth profiling: ray tracing, GI/path tracing, post-processing, denoising, upscaling, RHI, frame graph, shaders, and passes.

Target evidence:

- CPU cost of frame graph setup/compile, scene snapshot, texture loading, shader package load, and renderer submit
- GPU pass timings and ray tracing build timings
- memory budget and transient allocation pressure
- descriptor allocator occupancy
- pipeline/shader package count

Rule:

- Add performance evidence only by consolidating existing scattered diagnostics or using profiler-visible scopes.
- Do not add new profiling frameworks, report formats, logs, validation layers, scaffolding, wrappers, or abstractions.

### P1. Existing Decision Text

Existing decision text should stay lean and senior:

- precise ownership
- explicit tradeoffs
- no inflated claims
- no company or individual names
- no direct external reference trail
- clear "ship", "experimental", "delete", and "skip" labels

## What To Get

These are the capabilities the repo should grow toward, mostly by replacing and consolidating existing code:

| Capability | Desired evidence | Preferred implementation style |
| --- | --- | --- |
| Workload analysis | Frame graph/RHI workload review for D3D12/Vulkan after feature cleanup. | Late-stage consolidation using existing profiler/debugger hooks. |
| Shader optimization | Cooked shader packages, reflection, layout verification, backend targets. | Keep current compiler pipeline; trim debug output defaults. |
| API debugging and capture | PIX/RenderDoc/Nsight markers, debug layers, screenshot/BMP capture. | Preserve backend-native support and harden capture with narrow ownership. |
| GPU performance | Pass timings, memory budget, descriptors, pipeline pressure. | Late profiler-driven pass using existing scopes/snapshots only. |
| Ray tracing/GI | BLAS/TLAS lifecycle, classic TLAS and PTLAS, reservoir direct lighting, path/reference mode. | Preserve both TLAS paths; minimize PTLAS implementation. |
| Neural readiness | Slang/HLSL-friendly ABI, tensor/operator design note, feature gates. | Architecture first, tiny prototype later. |
| Productization | Curated level set, optional content packs, smaller launcher, clear packages. | Remove bloat before adding polish. |

## What To Prioritize

1. Organize Showcase content into curated levels plus optional heavy content packs.
2. Preserve screenshot/BMP capture and harden its ownership/cost.
3. Clean and harden renderer/RHI/frame graph/shader/pass code without new scaffolding.
4. Preserve graphics debugger support.
5. Keep and sharpen shader compiler/cook/runtime ABI.
6. Refactor PTLAS to a minimal functional D3D12/Vulkan implementation.
7. Make reference path tracing honest.
8. Slim launcher to build, cook, run, clean, package if owned.
9. Reduce public renderer/RHI observation APIs.
10. Defer profiling/measurement tasks until the renderer feature surface is ready.

## What To Skip

- broad ML framework integration
- training pipelines inside the engine
- vendor-specific compute backend work without a renderer feature
- more debug panels
- more runtime logs
- more screenshots/capture artifacts
- more benchmark/report formats
- more documentation files
- more diagnostics or validation systems
- new wrappers, abstractions, or scaffolding
- uncataloged levels or heavy assets in the core repo
- adding a render graph replacement
- claiming SDK equivalence for native implementations

## What To Avoid In Wording

Avoid:

- naming companies, individuals, or external source material
- saying the engine is built for a specific outside target
- claiming production parity with vendor SDKs
- saying "RTXDI equivalent" unless the SDK is integrated and validated
- calling research code shipping code
- describing diagnostics as product features

Use instead:

- "advanced graphics expectations"
- "advanced graphics requirements"
- "vendor-neutral provider boundary"
- "native reservoir-based direct lighting"
- "neural rendering readiness"
- "workload analysis"
- "profiler/debugger support"
- "product path" and "experimental path"

## Skip/Keep Table

| Area | Keep | Skip or remove |
| --- | --- | --- |
| RHI | D3D12/Vulkan backends, memory allocators, descriptors, pipelines, commands, ray tracing, debug layers, hardened screenshot/BMP capture. | Broad diagnostics as API, unused snapshots, smoke-owned capture paths. |
| Renderer | Frame graph, typed passes, shader ABI, classic TLAS and PTLAS, direct lighting reservoir, clear reference mode. | Future scaffolding, fallback-as-architecture, extra debug views. |
| Tools | Shader compiler, package inspection, minimal cook, minimal launcher. | Diagnostic artifacts, stats CSV by default, tool APIs with no consumer. |
| Content | Multiple levels, curated default level set, manifests/content catalogs, optional large packs. | Uncataloged multi-GB showcase content in core repo. |
| ML/neural | Slang/HLSL readiness, tensor/operator design note. | Runtime ML frameworks, training stack, vendor-specific kernels without feature need. |
| Docs | Existing architecture maps and decision text. | New docs before code cleanup, outside-reference trails, company names, cosmetic docs. |

## Success Bar

The repository is moving in the right direction when:

- a reviewer can understand the engine identity in one page
- the repo is much smaller
- multiple levels remain supported without polluting default workflows
- public API is smaller
- default workflows produce product outputs, not reports
- D3D12/Vulkan ownership is obvious
- shader source-to-runtime package flow is obvious
- late workload analysis replaces scattered diagnostics only after feature cleanup
- graphics debugger/profiler support remains strong
- screenshot/BMP capture remains preserved and low-cost
- ray tracing supports both classic TLAS and PTLAS as product paths where the backend supports them
- neural rendering is prepared architecturally without heavy dependencies

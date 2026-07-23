# G. Advanced Graphics Engine Executive Summary

Status: advanced graphics requirements summary
Date: 2026-07-24
Scope: engine direction derived from principal developer-technology, advanced graphics, graphics tools, path tracing, AI, neural rendering, and GPU systems expectations

## Purpose

This document turns advanced graphics engineering expectations into engine requirements. The goal is to steer SparkleEngine toward evidence that matters for advanced graphics and neural rendering work while preserving the repository's deletion-first cleanup direction.

The engine should read as a compact, serious renderer-first engine that proves:

- modern C++ and high-level shader engineering
- D3D12 and Vulkan workload understanding
- real-time rendering, rasterization, ray tracing, GI, and path tracing fundamentals
- GPU debugging and profiling fluency
- shader compiler, reflection, cook, and runtime ABI discipline
- first-principles performance thinking across CPU, GPU, memory, descriptors, and pipelines
- a real neural graphics result built on GPU-inference-ready architecture without premature dependency bloat
- neural model/operator, training/export, inference, quality, and performance understanding
- partner-facing integration and hardware/driver problem solving
- linear algebra, calculus, numerical analysis, and performance modeling connected to code
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
- What real neural graphics feature proves that path, and how does its quality/cost compare with the classical baseline?
- Which model, data, precision, layout, and training/export decisions determine the runtime result?
- What do CPU/GPU architecture and the driver contribute to the measured bottleneck?
- Could another engine team adopt, debug, and tune the feature from the available contracts and guidance?
- What is product code, what is experimental, and what has been intentionally deleted?

Near-term change gate:

- Modify existing planning text; do not add documentation or policy files unless the replacement removes more than it adds.
- Reject new diagnostics, runtime logs, validation paths, report formats, panels, wrapper layers, abstract contracts, and future-feature scaffolding.
- Preserve the shader compiler/cook/runtime ABI through deletion and simplification; clean renderer, RHI, frame graph, shader, and pass code before measurement-only work.
- Accept a rendering feature only when its integration is direct and contextual and its support code is outweighed by deletion or consolidation.

## Principal Developer Technology Success Contract

The canonical role matrix is `NV-PDTE-01` through `NV-PDTE-15` in [H. Advanced Graphics Engineer Persona](H_AdvancedGraphicsEngineerPersona.md). The executive interpretation is:

1. Keep the renderer/RHI/task/editor cleanup sequence intact.
2. Turn neural readiness into one real replacement-based neural graphics feature after the required data, shader, resource, and profiling boundaries are stable.
3. Demonstrate path tracing and neural rendering as product systems with quality, latency, memory, frame-pacing, backend, and fallback evidence.
4. Treat training/offline model preparation and runtime inference as separate owned workloads; ship only validated immutable artifacts and the smallest required inference path.
5. Require math derivation/reference tests for material algorithms and predicted-versus-measured performance reasoning.
6. Build hardware/driver diagnosis into normal evidence: exact configuration, capability state, validation, captures, reduced reproduction, and scoped conclusion.
7. Make one completed strategic feature usable as a partner-shaped integration case and explainable as a live demo, whitepaper-quality note, and talk outline.
8. Treat AI tools as fallible accelerators. Verify every generated code, shader, model, test, citation, and claim independently.

This raises the final bar without authorizing a generic ML framework, new telemetry product, speculative future-hardware API, training UI, second scheduler, or vendor-branded architecture.

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

### P1. Neural Rendering Foundation And Product Feature

Do not bolt on a heavy ML stack. Prepare the architecture, then use it for one real feature.

Foundation evidence:

- existing shader ABI remains capable of representing tensor-like resources, feature profiles, specialization constants, and provider resource contracts
- existing Slang/HLSL path stays clean for neural shader experiments
- deterministic immutable model/operator artifacts can cross cook/runtime boundaries with explicit shape, layout, precision, capability, and lifetime

Feature evidence:

- one neural graphics path replaces or materially improves an existing denoising, reconstruction, sampling, texture/material, animation, or rendering path
- a named classical baseline/fallback remains usable
- model/operator math, data provenance, training or fine-tuning, export/cook, and inference ownership are reproducible
- tensor layouts, precision, operators, batching, dispatch, memory, and synchronization are chosen from captures
- quality metrics and visual failure cases are reported beside CPU/GPU latency, memory, and frame pacing
- D3D12/Vulkan and hardware capability status are explicit; unsupported paths fall back honestly
- the accepted feature deletes its temporary experiment and compatibility scaffolding

Skip:

- bundling runtime ML frameworks or vendor-specific compute backends into the engine
- training workflows inside the runtime engine or editor
- broad ML framework integration without a concrete renderer feature
- empty tensor/model abstractions, mock models, or provider toggles presented as implementation
- quality-only or speed-only claims

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
| Neural foundation | Slang/HLSL-friendly ABI, tensor-like resource/precision/layout contracts, capability gates, deterministic artifacts. | Keep the architecture clean and feature-owned. |
| Neural graphics implementation | Real model/operator path, classical fallback, quality/performance frontier, deterministic training/export and runtime inference. | One replacement vertical slice; no general runtime ML framework. |
| Math and performance modeling | Derivation/reference tests, numerical limits, and predicted-versus-measured CPU/GPU cost. | Keep evidence with the owning algorithm and existing tools. |
| Driver/hardware diagnosis | Exact hardware/driver/config, native validation, reduced reproducer, capability fallback. | Backend-private fixes and scoped conclusions. |
| Technology transfer | Partner-shaped adoption case, live demo, concise whitepaper/talk-quality explanation. | Produce after the implementation and evidence are complete. |
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
11. Establish deterministic model-artifact, shape/layout/precision, and capability contracts without adding a generic ML layer.
12. Implement one real neural graphics replacement feature and tune its quality/performance frontier.
13. Produce one hardware/driver reduced-repro case and one partner-shaped integration case from completed work.
14. Publish a code-backed demo, whitepaper-quality result, and talk outline only after the relevant gates pass.

## What To Skip

- broad ML framework integration
- training pipelines inside the runtime engine; isolated feature-owned offline study/export is allowed
- vendor-specific compute backend work without a renderer feature
- empty neural/tensor/model scaffolding or fake AI workloads
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

- naming companies, individuals, or external source material in release/product wording; internal source-trace and role-requirement documents may name exact sources
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
- "neural rendering readiness" only for the foundation, never as the final feature claim
- "neural graphics feature" only after real model/operator, quality, performance, artifact, and fallback gates pass
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
| ML/neural | Existing Slang/HLSL foundation plus one future product-owned neural graphics replacement with deterministic artifacts and measured inference. | Runtime ML frameworks, runtime training stack, mock models, vendor-specific kernels without feature need. |
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
- at least one real neural graphics feature proves the preparation with model/artifact, quality, performance, memory, backend, and fallback evidence
- one path-traced and one neural workload can be explained from math through shader/RHI execution to hardware/driver behavior
- another engineer can reproduce, integrate, debug, and tune the strategic feature from the handoff material
- completed strategic work has a live demo and whitepaper/talk-quality explanation without overstated claims

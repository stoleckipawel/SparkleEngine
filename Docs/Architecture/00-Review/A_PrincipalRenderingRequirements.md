# A. Principal Rendering Requirements

Status: neutral background requirements summary
Date: 2026-06-20
Scope: principal-level rendering, GPU SDK, low-level graphics, high-performance compute, GPU architecture, and neural rendering expectations

## Target Reviewer Profile

This engine should be prepared for reviewers who are looking for principal-level evidence in:

- D3D12/Vulkan explicit API control.
- GPU architecture and hardware-aware performance reasoning.
- Rendering architecture that is modular, debuggable, and hard to break.
- SDK integration discipline across major GPU vendor ecosystems.
- Shader compiler, reflection, cook, and runtime shader ABI thinking.
- Ray tracing and neural rendering readiness.
- Diagnostics, validation, profiling, memory telemetry, and repeatable reviewer workflows.
- Clear engineering judgment, not just feature count.

## Requirement Clusters

| Cluster | What reviewers judge | SparkleEngine target |
| --- | --- | --- |
| Explicit graphics API control | D3D12/Vulkan ownership of memory, descriptors, queues, command lists, synchronization, barriers, pipeline state, and GPU/CPU lifetime. | RHI contracts must explain who owns resources, states, descriptors, queues, fences, and native handles. |
| Hardware-aware performance | Ability to reason about memory bandwidth, cache behavior, occupancy, queue scheduling, async work, residency, descriptor pressure, shader cost, and frame timing. | Preserve compact runtime facts that drive policy; defer new measurement systems until feature cleanup is complete. |
| Cross-IHV SDK integration | Vendor SDKs should not leak through the engine. Feature gates, hardware gates, dependency gates, and runtime failures must be explicit. | Use provider interfaces and capability states for upscaling, reconstruction, denoisers, frame generation, ray tracing extensions, and future neural paths. |
| Render architecture | Reviewers expect recognizable pass/frame graph structure, scene data boundaries, resource lifetime rules, and pass authoring rules. | Make pass registration, resource declarations, transient/persistent resources, scheduling, and barriers obvious in code shape before adding documentation. |
| RHI design discipline | Recognized RHIs balance low overhead, explicitness, lifetime tracking, backend portability, and bounded native escape hatches. | Keep explicit-control boundaries clear through smaller public APIs and direct backend-owned implementation. |
| Shader/compiler pipeline | Modern advanced graphics work asks for HLSL/SM6, Slang, DXC, SPIR-V/DXIL, reflection, shader contracts, feature profiles, and cook/runtime ABI design. | Keep ShaderCompiler and shader ABI as product-level systems while removing debug artifacts from default workflows. |
| Ray tracing readiness | Expected knowledge includes BLAS/TLAS lifecycle, ray tracing pipelines, ray queries, shader tables, AS update/compaction, and backend feature parity. | Keep classic TLAS and PTLAS as product features; minimize PTLAS scaffolding while preserving D3D12/Vulkan capability. |
| Neural rendering readiness | Advanced neural rendering work increasingly involves Slang, neural shading, training/inference concepts, cooperative vectors/tensor-like paths, denoisers, and neural texture/material workflows. | Prepare through shader/provider/resource flexibility, not through heavy runtime ML dependencies or new diagnostic scaffolding. |
| Production reviewability | Principal review values code navigation, explicit tradeoffs, clear ownership, and evidence that changes can be made safely. | Make the repo easier to review by deleting stale systems, shrinking public APIs, and preserving real capabilities. |

## Reference Project Patterns

| Reference | Recognized pattern | What Sparkle should borrow |
| --- | --- | --- |
| GPUOpen Cauldron | Simple, extensible D3D12/Vulkan framework used for FidelityFX samples, prototypes, and SDK demos. | Keep experiments and SDK integrations provider/sample-friendly. Do not let SDK code shape the whole renderer. |
| Reusable rendering framework | Scene graph, passes, renderer helpers, and sample/application split. | Keep renderer, scene data, and game framework boundaries obvious. Make the pass system easy to inspect. |
| Explicit RHI helper layer | API abstraction over D3D12/Vulkan concepts with resource/pipeline/descriptor helpers, lifetime management, upload suballocation, and native API access. | Preserve Sparkle's direct mental model; avoid wrappers unless they remove more code than they add. |
| Low-level rendering interface | Explicitness, low overhead, backend coverage, and avoidance of hidden high-level management. | Preserve a direct mental model of command lists, queues, resources, barriers, descriptors, and synchronization. |
| Provider bridge framework | Integration layer between game/renderer and external features using tagged resources and correct pipeline placement. | Keep provider resource contracts narrow for depth, motion vectors, exposure, history, jitter, frame index, and camera state. |
| Neural shading examples | Slang-centered neural rendering and inference-like shader patterns with D3D/Vulkan feature prerequisites. | Keep Slang/profile/capability gating ready for neural features without changing renderer ownership rules later. |
| D3D12MA / VMA | Production-recognized memory allocation libraries with pooling, statistics, budget, mapping, naming, and debug concepts. | Expose allocator-backed memory diagnostics and budget pressure instead of hiding allocators entirely. |

## Portfolio Review Signals To Aim For

Strong signals:

- A reviewer can find the RHI contract in under one minute.
- D3D12 and Vulkan backend parity is listed honestly.
- Vendor SDK code is isolated and capability-gated.
- The shader pipeline shows source, include closure, compilation target, reflection, contracts, cook package, and runtime load.
- Frame graph/pass rules are explicit enough that a reviewer can add a pass without guessing.
- Diagnostics show active backend, memory budget, descriptors, pipelines, shader cache, pass timings, and enabled providers.
- Boundary checks are executable and visible.
- Limitations are documented honestly.

Weak signals to avoid:

- Renderer code directly depending on D3D12/Vulkan/NVAPI except through documented provider/backend boundaries.
- Feature flags without capability/failure states.
- Shader compilation behavior only discoverable by stepping through tools.
- Passes that allocate resources or depend on history without declaring it.
- Memory allocators used correctly but invisible to diagnostics.
- Impressive features with no validation matrix or performance evidence.

## Requirement Coverage Targets

| Requirement | Target foundation before feature expansion |
| --- | --- |
| D3D12/Vulkan explicit control | RHI lifecycle, barriers, descriptors, queues, fences, and native handle policy obvious in the code and existing docs. |
| SDK readiness | Provider-neutral capability model and resource contract kept narrow and product-owned. |
| Shader/compiler strength | Shader ABI, reflection, cook cache, feature profiles, and inspection commands kept strong while debug artifacts are opt-in. |
| Performance readiness | Memory budget, descriptor pressure, pass timings, shader cache, and pipeline cache visible. |
| Ray tracing readiness | Classic TLAS and PTLAS both functional where supported. |
| Neural rendering readiness | Slang/profile/capability gates and future provider resource needs preserved without runtime ML bloat. |
| Review readiness | Existing review docs and code ownership make the engine understandable without new planning sprawl. |

## Source Trail

Direct source links are intentionally omitted from this background note. Active source-backed comparisons live in `E_ExternalRendererRepositoryComparison.md`; active execution guidance lives in `F_StagedDeletionFirstImprovementPlan.md`, `G_AdvancedGraphicsEngineExecutiveSummary.md`, and `H_AdvancedGraphicsEngineerPersona.md`.


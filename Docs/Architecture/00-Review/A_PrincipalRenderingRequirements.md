# A. Principal Rendering Requirements

Status: role-grounded principal requirements summary
Date: 2026-07-24
Scope: principal-level rendering, developer technology, GPU SDK, low-level graphics, high-performance compute, GPU architecture, path tracing, AI, and neural rendering expectations

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
- Partner-facing technology adoption, reduced reproducers, and best-practice guidance.
- Neural algorithm/model implementation and optimization, not only neural-ready interfaces.
- Strong linear algebra, calculus, numerical reasoning, and performance modeling tied to executable evidence.
- CPU/GPU architecture and driver-facing diagnosis on current hardware, with capability-driven preparation for future hardware.
- Technical demos, whitepaper-quality analysis, conference-ready communication, organization, and prioritization.
- Verified use of AI tools alongside independent AI/ML fundamentals and engineering review.

## NVIDIA Principal Developer Technology Additive Requirements

The canonical role requirements are `NV-PDTE-01` through `NV-PDTE-15` in [H. Advanced Graphics Engineer Persona](H_AdvancedGraphicsEngineerPersona.md). They are binding additions to this requirements summary.

| Requirement group | Raised repository bar |
|---|---|
| Partner adoption and collaboration | A feature is not complete merely because it runs in Showcase. Its contract, capability/fallback behavior, reproduction steps, failure handling, integration cost, and handoff guidance must be clear enough for another engine team to adopt. |
| Path-traced rendering | At least one representative path-traced workload must be owned end to end and validated for correctness, quality, latency, memory, D3D12/Vulkan behavior, and current hardware limits. |
| Neural graphics | Architectural readiness must culminate in at least one real replacement-based neural graphics vertical slice with a real model/operator path, deterministic artifact flow, classical fallback, and quality/performance evidence. |
| Neural model and workload tuning | Training/offline preparation and runtime inference must be analyzed separately. Tensor layout, precision, operators, batching, memory, scheduling, export, and inference latency require measured decisions. |
| Mathematics | Rendering and neural changes must be derivable and testable: coordinate spaces, estimators, filters, gradients, loss/quality metrics, numerical stability, and cost models cannot remain unexplained copied formulas. |
| Low-level CPU/GPU optimization | Evidence must connect algorithms to CPU topology/cache behavior, GPU waves/cache/bandwidth/registers/occupancy, driver submission, frame pacing, and input-to-present latency. |
| Hardware and driver collaboration readiness | Capability gates, API correctness, validation, hardware/driver matrices, reduced reproducers, and application-versus-driver root-cause analysis must be normal engineering outputs. |
| AI fundamentals and AI-assisted engineering | Model provenance, data split, optimization objective, generalization limits, quantization/deployment choices, and independent review of AI-generated code/design are required. |
| Communication and leadership | Completed strategic work must be explainable as a concise design review, reproducible case study, live demo, whitepaper-quality note, and talk outline with honest limitations and priorities. |
| Experience and platform differentiators | The repository demonstrates equivalent principal depth through repeated independent ownership. Linux or driver-development support is claimed only after real native build, run, validation, and debugging evidence. |

No individual prompt must advance every group, but every prompt must preserve the applicable contracts and report its `NV-PDTE-*` status. The complete portfolio must provide direct evidence for all technical groups; credentials or work history are never inferred from repository code.

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
| Neural rendering implementation | Advanced neural rendering work involves Slang/neural shaders, training and inference, cooperative vectors/tensor-like paths, precision/layout decisions, denoisers, reconstruction, and neural texture/material workflows. | Preserve shader/provider/resource flexibility, then prove one real replacement feature without adding a general runtime ML framework or diagnostic scaffolding. |
| Mathematical and numerical rigor | Principal work requires linear algebra, calculus, estimators, gradients, error analysis, and performance models connected to implementation. | Keep derivations close to feature tests and validate reference values, stability, quality metrics, and predicted versus measured cost. |
| Developer technology transfer | Advanced technology must survive another team's constraints, integration process, debugging, and performance budget. | Keep adoption surfaces narrow, capability/fallback states explicit, reproducers minimal, and handoff guidance tied to code and captures. |
| Driver and future-hardware reasoning | Current and next-generation performance depends on API/driver behavior, architecture, and capability evolution. | Separate application defects from driver behavior, maintain reduced reproducers, record tested hardware/driver facts, and keep untested future claims as hypotheses. |
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
- A partner-shaped integration case can be reproduced and adopted without private knowledge.
- Rendering math and neural model/operator choices are derived, tested, and connected to measured hardware cost.
- A real neural graphics feature reports both quality and performance against a classical baseline.
- Hardware/driver findings include exact configuration, native validation, a reduced reproducer where applicable, and no unsupported generalization.
- Strategic work has a concise review narrative, live demo, and whitepaper-quality technical explanation.

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
| Neural rendering foundation | Slang/profile/capability gates, tensor-like resource contracts, precision/layout metadata, and provider resource needs preserved without runtime ML bloat. |
| Neural graphics implementation | One product-owned replacement feature with deterministic training/export provenance, efficient inference, classical fallback, quality metrics, and CPU/GPU/memory evidence. |
| AI/ML workload depth | Training/offline preparation and runtime inference profiled independently; data, loss/metric, precision, batching, memory, and deployment tradeoffs documented. |
| Mathematics and modeling | Feature equations, coordinate spaces, numerical behavior, and cost predictions tied to executable reference tests and measurements. |
| Driver/hardware readiness | D3D12/Vulkan validation, exact hardware/driver metadata, capability-driven fallback, and reduced driver/application repro workflow. |
| Technology transfer | Integration case study, adoption guide, live demo, and whitepaper/talk-quality explanation for completed principal work. |
| Review readiness | Existing review docs and code ownership make the engine understandable without new planning sprawl. |

## Source Trail

Direct external source links are intentionally omitted from this background note. Active source-backed comparisons live in [E. External Renderer Repository Comparison](E_ExternalRendererRepositoryComparison.md). Persona traceability lives in [H. Advanced Graphics Engineer Persona](H_AdvancedGraphicsEngineerPersona.md); execution and coding gates live in [K. Multithreaded Engine Implementation Prompt Series](K_MultithreadedEngineImplementationPromptSeries.md) and [L. SparkleEngine Integration Style Guide](L_SparkleEngineIntegrationStyleGuide.md).


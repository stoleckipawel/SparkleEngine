# A. Principal Rendering Role Requirements

Status: first-pass external requirements review  
Date: 2026-06-20  
Scope: NVIDIA/AMD principal-level rendering, GPU SDK, low-level graphics, high-performance compute, GPU architecture, and neural rendering expectations

## Target Reviewer Profile

This engine should be prepared for reviewers who are looking for principal-level evidence in:

- D3D12/Vulkan explicit API control.
- GPU architecture and hardware-aware performance reasoning.
- Rendering architecture that is modular, debuggable, and hard to break.
- SDK integration discipline across NVIDIA and AMD ecosystems.
- Shader compiler, reflection, cook, and runtime shader ABI thinking.
- Ray tracing and neural rendering readiness.
- Diagnostics, validation, profiling, memory telemetry, and repeatable reviewer workflows.
- Clear engineering judgment, not just feature count.

## Requirement Clusters

| Cluster | What reviewers judge | SparkleEngine target |
| --- | --- | --- |
| Explicit graphics API control | D3D12/Vulkan ownership of memory, descriptors, queues, command lists, synchronization, barriers, pipeline state, and GPU/CPU lifetime. | RHI contracts must explain who owns resources, states, descriptors, queues, fences, and native handles. |
| Hardware-aware performance | Ability to reason about memory bandwidth, cache behavior, occupancy, queue scheduling, async work, residency, descriptor pressure, shader cost, and frame timing. | Add visible telemetry for memory budget, pass timing, descriptor/pipeline pressure, shader cache behavior, and CPU/GPU frame orchestration. |
| Cross-IHV SDK integration | NVIDIA/AMD SDKs should not leak through the engine. Feature gates, hardware gates, dependency gates, and runtime failures must be explicit. | Use provider interfaces and capability states for DLSS, Streamline, FidelityFX, denoisers, frame generation, ray tracing extensions, and future neural paths. |
| Render architecture | Reviewers expect recognizable pass/frame graph structure, scene data boundaries, resource lifetime rules, pass authoring rules, and diagnostics. | Renderer docs should describe pass registration, resource declarations, transient/persistent resources, scheduling, barriers, timing, and validation. |
| RHI design discipline | Recognized RHIs balance low overhead, explicitness, lifetime tracking, backend portability, and bounded native escape hatches. | Sparkle must document which parts are explicit, which parts are tracked/helped, and when backend-native access is allowed. |
| Shader/compiler pipeline | Modern roles ask for HLSL/SM6, Slang, DXC, SPIR-V/DXIL, reflection, shader contracts, feature profiles, and cook/runtime ABI design. | ShaderCompiler and shader contracts should be documented as a product-level pipeline, not as build plumbing. |
| Ray tracing readiness | Expected knowledge includes BLAS/TLAS lifecycle, ray tracing pipelines, ray queries, shader tables, AS update/compaction, and backend feature parity. | RHI ray tracing services need lifecycle docs, backend parity tables, validation tests, and renderer resource contracts. |
| Neural rendering readiness | NVIDIA roles and SDK examples increasingly involve Slang, neural shading, training/inference concepts, cooperative vectors/tensor-like paths, denoisers, and neural texture/material workflows. | Prepare capability/profile gates, provider resource contracts, shader ABI extension points, and diagnostics before adding features. |
| Production reviewability | Principal review values code navigation, explicit tradeoffs, automated checks, clear ownership, and evidence that changes can be made safely. | Add architecture docs, ADRs, validation matrix, reviewer guide, and visible boundary checks. |

## Reference Project Patterns

| Reference | Recognized pattern | What Sparkle should borrow |
| --- | --- | --- |
| AMD Cauldron | Simple, extensible D3D12/Vulkan framework used for FidelityFX samples, prototypes, and SDK demos. | Keep experiments and SDK integrations provider/sample-friendly. Do not let SDK code shape the whole renderer. |
| NVIDIA Donut | Reusable rendering framework with scene graph and passes, backed by NVRHI, explicitly not a full game engine. | Keep renderer, scene data, and game framework boundaries obvious. Make the pass system easy to inspect. |
| NVIDIA NVRHI | API abstraction over D3D11/D3D12/Vulkan with resource/pipeline/descriptor helpers, optional barrier tracking, lifetime management, upload suballocation, and native API access. | Document Sparkle's automation versus explicit-control choices. Native escape hatches must be intentional and auditable. |
| NVIDIA NRI | Low-level rendering interface focused on explicitness, low overhead, backend coverage, and avoiding hidden high-level management. | Preserve a direct mental model of command lists, queues, resources, barriers, descriptors, and synchronization. |
| NVIDIA Streamline | Plugin framework between game and render API; features need tagged resources and correct pipeline placement. | Centralize provider resource contracts for depth, motion vectors, exposure, history, jitter, frame index, and camera state. |
| NVIDIA RTX Neural Shading | Slang-centered examples for neural rendering training/inference with D3D/Vulkan feature prerequisites. | Keep Slang/profile/capability gating ready for neural features without changing renderer ownership rules later. |
| AMD D3D12MA / VMA | Production-recognized memory allocation libraries with pooling, statistics, budget, mapping, naming, and debug concepts. | Expose allocator-backed memory diagnostics and budget pressure instead of hiding allocators entirely. |

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
| D3D12/Vulkan explicit control | RHI lifecycle, barriers, descriptors, queues, fences, and native handle policy documented. |
| SDK readiness | Provider-neutral capability model and resource contract documented. |
| Shader/compiler strength | Shader ABI, reflection, cook cache, feature profiles, and inspection commands documented. |
| Performance readiness | Memory budget, descriptor pressure, pass timings, shader cache, and pipeline cache visible. |
| Ray tracing readiness | AS lifecycle and backend parity documented. |
| Neural rendering readiness | Slang/profile/capability gates and future provider resource needs documented. |
| Review readiness | 10-minute, 30-minute, and deep-dive reviewer paths documented. |

## Sources

- AMD GPUOpen, "How do I become a graphics programmer?": https://gpuopen.com/learn/how_do_you_become_a_graphics_programmer/
- AMD GPUOpen, Cauldron Framework: https://gpuopen.com/fidelityfx-cauldron-framework/
- AMD GPUOpen, FidelityFX SDK: https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK
- AMD GPUOpen, D3D12 Memory Allocator: https://gpuopen.com/d3d12-memory-allocator/
- AMD GPUOpen, Vulkan Memory Allocator: https://gpuopen.com/vulkan-memory-allocator/
- NVIDIA RTX, NVRHI: https://github.com/NVIDIA-RTX/NVRHI
- NVIDIA RTX, Donut: https://github.com/NVIDIA-RTX/Donut
- NVIDIA RTX, NRI: https://github.com/NVIDIA-RTX/NRI
- NVIDIA RTX, RTX Neural Shading: https://github.com/NVIDIA-RTX/RTXNS
- NVIDIA Developer, Streamline: https://developer.nvidia.com/rtx/streamline
- NVIDIA Careers, Principal Graphics Developer Tools Engineer: https://nvidia.wd5.myworkdayjobs.com/NVIDIAExternalCareerSite/job/US-CA-Santa-Clara/Principal-Graphics-Developer-Tools-Engineer_JR2019836
- NVIDIA Careers, CUDA UMD GPU Kernel Scheduling: https://jobs.nvidia.com/careers/job/893393903977
- NVIDIA Careers, Neural Graphics Engineer: https://jobs.nvidia.com/careers/job/893393627432


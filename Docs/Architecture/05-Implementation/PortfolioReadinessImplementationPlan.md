# Portfolio Readiness Implementation Plan

Status: staged code and asset execution plan  
Date: 2026-06-20  
Scope: code, tool, workflow, diagnostics, validation, and curated asset changes needed to make SparkleEngine portfolio-review ready

## Purpose

The architecture review documents now describe what SparkleEngine is aiming for and what reviewers will judge. This document converts those requirements into staged implementation prompts. Each prompt is intended to be copy/pasted into a weaker coding agent and still converge on the expected shape because it includes source documents, code areas, deliverables, acceptance criteria, validation steps, and reporting requirements.

This plan does not add mature rendering features for their own sake. It prepares feature-ready foundations and MVP proof paths for PSO management, high-end ray tracing, path tracing, neural rendering readiness, and eventually multithreaded execution. The goal is to make the existing engine easier to validate, easier to inspect, harder to break, and ready for those major features to land one by one.

## Primary Review Sources

The implementation plan is governed by the review folder. These documents define what "ready" means:

- [A_PrincipalRoleRequirements.md](../00-Review/A_PrincipalRoleRequirements.md)
- [B_EngineArchitectureScorecard.md](../00-Review/B_EngineArchitectureScorecard.md)
- [PrincipalRenderingReadiness.md](../00-Review/PrincipalRenderingReadiness.md)
- [C_FoundationStagedPlan.md](../00-Review/C_FoundationStagedPlan.md)
- [ReviewerGuide.md](../00-Review/ReviewerGuide.md)

Supporting docs refine details, but they must not invert the priority. Validation, launcher workflow, and contract documents exist to prove and protect the architecture refactor. They are not the architecture refactor by themselves.

Supporting references:

- [README.md](../README.md)
- [BoundaryRules.md](../01-Boundaries/BoundaryRules.md)
- [RHIContract.md](../02-Contracts/RHIContract.md)
- [RendererFrameGraph.md](../02-Contracts/RendererFrameGraph.md)
- [RendererProviderContract.md](../02-Contracts/RendererProviderContract.md)
- [ShaderPipeline.md](../02-Contracts/ShaderPipeline.md)
- [RuntimeSceneData.md](../02-Contracts/RuntimeSceneData.md)
- [ApplicationLifecycle.md](../02-Contracts/ApplicationLifecycle.md)
- [ValidationMatrix.md](../03-Validation/ValidationMatrix.md)
- [PerformanceDiagnosticsPlan.md](../03-Validation/PerformanceDiagnosticsPlan.md)
- [LauncherWorkflowReadiness.md](../04-Workflows/LauncherWorkflowReadiness.md)

## Refactor Thesis

This plan is only successful if the repository changes in ways a principal rendering reviewer can see in code:

- RHI ownership becomes more explicit and more inspectable.
- D3D12 and Vulkan backend capability differences become source-backed data, not scattered assumptions.
- Renderer pass/resource/frame ownership becomes harder to misuse.
- Provider integration becomes vendor-neutral before more SDKs arrive.
- Shader ABI and runtime pipeline behavior become regression-tested, not just documented.
- Runtime scene data remains independent of Renderer/RHI while still feeding renderer snapshots cleanly.
- PSO management becomes an explicit runtime/cache surface with identity, invalidation, compatibility, and timing evidence.
- Ray tracing and path tracing become concrete foundation MVPs with AS lifecycle, RT pipeline/SBT expectations, accumulation/history ownership, sampling/reset rules, and proof scenarios.
- Neural rendering readiness becomes capability/profile/provider contract data, not an undocumented future hope.
- Multithreaded execution is introduced only after ownership, lifetime, diagnostics, and feature-foundation MVPs are stable enough to parallelize safely.
- Performance, memory, descriptor, shader, and provider diagnostics become typed data surfaces first, then editor/tool/report views.
- Launcher and validation workflows prove the architecture rather than papering over weak architecture.

The anti-goal is also explicit: do not glue validation commands onto unclear ownership and call that portfolio-ready.

## Acceptance Targets

The implementation work should move the repository toward these review outcomes.

## Plan Tracking

- Total prompts in this plan: `32`
- Tracking format used below: `Prompt XX of 32`
- Milestone tag format used below: `M0` through `M10`
- Practical progress rule: when `Prompt 08 of 32` is complete, you are `8 / 32` through the prompt plan.

## Production Reference Discipline

Every implementation prompt must start by comparing SparkleEngine against production-proven public repositories. The goal is not to copy code. The goal is to prevent architecture from being invented in isolation.

Reference-backed execution means:

- Before coding, inspect the prompt-specific reference repositories in the table below.
- Extract the concrete pattern being used: ownership boundary, service shape, lifetime rule, validation hook, artifact shape, or pass/pipeline layout.
- Apply the pattern only where it improves SparkleEngine's existing code path.
- Do not add a new abstraction only because a requirement says "make this visible."
- Do not add a parallel descriptive layer that mirrors existing code unless a reference repo has the same kind of runtime-consumed layer and SparkleEngine will consume it in validation or diagnostics.
- If no reference supports the intended design, stop and choose a smaller change against existing code.
- The report for every prompt must include `Reference evidence used` and `Reference patterns rejected`.

Reference gate:
Each row in the prompt reference matrix is part of that prompt's acceptance criteria. Before editing code, the agent must
write a short implementation note listing the reference repositories inspected, the concrete pattern being adopted, and
the patterns rejected. If the intended architecture shape has no public reference precedent in the listed repositories or
an equally strong source named in the report, the agent must not invent it. It should reduce the task to a smaller,
source-backed refactor against SparkleEngine's existing code.

Core reference repositories:

- [NRI](https://github.com/NVIDIA-RTX/NRI): low-level D3D12/Vulkan-style explicit rendering interface, useful when a prompt touches RHI boundaries, queues, descriptors, memory, barriers, or backend parity.
- [NVRHI](https://github.com/NVIDIA-RTX/NVRHI) and [NVRHI Programming Guide](https://github.com/NVIDIA-RTX/NVRHI/blob/main/doc/ProgrammingGuide.md): resource states, command lists, binding sets, lifetime, barriers, descriptor/resource usage, pipeline APIs, and portability tradeoffs.
- [NVIDIA Donut](https://github.com/NVIDIA-RTX/Donut) and [Donut Samples](https://github.com/NVIDIA-RTX/Donut-Samples): practical renderer/app framework shape over NVRHI, device manager, scene loading, render passes, threaded rendering, and ray tracing samples.
- [AMD Cauldron](https://github.com/GPUOpen-LibrariesAndSDKs/Cauldron): AMD production sample framework for DirectX 12/Vulkan prototyping, useful for backend abstraction, render modules, FidelityFX integration, and sample workflow shape.
- [FidelityFX SDK](https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK): provider integration, AMD SDK sample structure, shader/runtime packaging expectations.
- [Falcor](https://github.com/NVIDIAGameWorks/Falcor): render graph, ray tracing, path tracing, shader, scene, diagnostics, and research/prototype renderer architecture.
- [RTXPT](https://github.com/NVIDIA-RTX/RTXPT): path tracing integration, neural-graphics-adjacent renderer structure, and high-end RT/path tracing proof shape.
- [Streamline](https://github.com/NVIDIA-RTX/Streamline) and [Streamline Sample](https://github.com/NVIDIA-RTX/Streamline_Sample): provider-style SDK integration and capability checks.
- [NRD](https://github.com/NVIDIA-RTX/NRD): denoiser input contracts, low ray-per-pixel path tracing denoise expectations, and temporal/resource requirements.
- [RTXNTC](https://github.com/NVIDIA-RTX/RTXNTC): neural texture compression readiness, material/texture channel grouping, and runtime integration constraints.
- [Slang](https://github.com/shader-slang/slang) and [Neural Shading SIGGRAPH 2025 examples](https://github.com/shader-slang/neural-shading-s25): shader profile gates, Slang-based neural shading experiments, and cooperative-vector readiness.

Prompt reference matrix:

| Prompt | Required reference starting points |
| --- | --- |
| Prompt 01 | NRI, NVRHI Programming Guide, Donut DeviceManager/App shape, Cauldron device/backend abstractions |
| Prompt 02 | NRI backend contracts, NVRHI backend capabilities, Cauldron D3D12/Vulkan device layers |
| Prompt 03 | Donut render passes, Donut Samples, Falcor RenderGraph, NVRHI command/resource state patterns |
| Prompt 04 | Streamline, Streamline Sample, FidelityFX SDK, Cauldron FidelityFX integration |
| Prompt 05 | NVRHI shader/pipeline APIs, Donut shader compiler usage, Cauldron shader/runtime patterns, Slang |
| Prompt 06 | Donut scene/component graph, Cauldron scene/content systems, Falcor scene model |
| Prompt 07 | NVRHI validation/lifetime diagnostics, Cauldron diagnostics patterns, Falcor profiling/graph evidence |
| Prompt 08 | Donut app/device manager lifecycle, Cauldron sample lifecycle, Falcor application/sample lifecycle |
| Prompt 09 | NVRHI pipeline state APIs, NRI pipeline abstractions, Cauldron PSO/shader runtime handling |
| Prompt 10 | NVRHI ray tracing tutorial/samples, Donut ray tracing samples, Falcor ray tracing, RTXPT |
| Prompt 11 | Falcor path tracer docs, RTXPT, NRD, Donut ray tracing samples |
| Prompt 12 | Slang, neural-shading-s25, RTXNTC, Streamline, FidelityFX SDK |
| Prompt 13 | Donut Samples, Falcor sample graphs, RTXPT sample scenarios, NRD sample expectations |
| Prompt 14 | Donut threaded rendering sample, NVRHI parallel command list guidance, Falcor task/render graph scheduling |
| Prompt 15 | Donut/Cauldron sample workflow commands, existing SparkleLauncher patterns |
| Prompt 16 | Donut/Cauldron configuration/readiness reporting patterns, existing SparkleLauncher reports |
| Prompt 17 | Donut headless/basic samples, Falcor sample graph loading, Cauldron sample startup |
| Prompt 18 | NRI/NVRHI backend startup, Donut DeviceManager, Cauldron device creation |
| Prompt 19 | NRI/NVRHI capability reporting, Cauldron D3D12/Vulkan parity, Falcor device feature reporting |
| Prompt 20 | Falcor diagnostics/profiling, NVRHI validation, Cauldron profiler/metrics |
| Prompt 21 | NVRHI descriptor/resource lifetime, Cauldron descriptor/resource pools, NRI descriptors |
| Prompt 22 | NVRHI pipeline APIs, Cauldron shader/PSO handling, Donut shader compilation |
| Prompt 23 | Donut shader compiler, NVRHI shader reflection/pipelines, Slang/DXC usage in references |
| Prompt 24 | Streamline capability checks, FidelityFX SDK feature availability, Cauldron SDK backend |
| Prompt 25 | Streamline resource tags/contracts, NRD inputs, FidelityFX SDK input resources |
| Prompt 26 | Falcor RenderGraph validation, NVRHI resource state tracking, Donut pass conventions |
| Prompt 27 | Donut scene graph, Cauldron content systems, Falcor scene invalidation patterns |
| Prompt 28 | Donut/Cauldron app failure paths, SparkleLauncher recovery patterns |
| Prompt 29 | Donut Samples, Cauldron samples, Falcor sample scenes |
| Prompt 30 | Falcor/Mogwai inspection workflow, Cauldron sample UI, existing Sparkle editor UI |
| Prompt 31 | Donut/Cauldron sample runner expectations, SparkleLauncher operation orchestration |
| Prompt 32 | The implemented code plus all references used by completed prompts |

| Target | Source | Implementation meaning |
| --- | --- | --- |
| A reviewer can run architecture, generate, build, cook, smoke, shader inspection, and diagnostics through stable commands. | [ReviewerGuide.md](../00-Review/ReviewerGuide.md), [ValidationMatrix.md](../03-Validation/ValidationMatrix.md) | Add missing launcher/CLI operations and artifacts instead of relying on oral instructions. |
| Architecture boundary checks are visible in launcher/build workflows. | [B_EngineArchitectureScorecard.md](../00-Review/B_EngineArchitectureScorecard.md), [BoundaryRules.md](../01-Boundaries/BoundaryRules.md) | Promote `architecture_boundary_check` to a reviewer-facing action. |
| Planned validation rows become existing or partial with artifacts. | [ValidationMatrix.md](../03-Validation/ValidationMatrix.md) | Add stable commands for renderer empty frame, backend startup/shutdown, smoke artifacts, shader regression, backend reporting. |
| Diagnostics expose backend, adapter, API level, validation state, providers, memory, descriptors, pipelines, shader load timing, pass GPU time, CPU time, and upload pressure. | [PerformanceDiagnosticsPlan.md](../03-Validation/PerformanceDiagnosticsPlan.md) | Add typed snapshots and text artifacts before adding UI-only polish. |
| Provider integrations report the six architectural capability states. | [RendererProviderContract.md](../02-Contracts/RendererProviderContract.md) | Add a shared mapping layer for `unavailable`, `missing dependency`, `unsupported hardware`, `available`, `enabled`, `runtime failed`. |
| Provider resource contracts are explicit. | [RendererProviderContract.md](../02-Contracts/RendererProviderContract.md), [RendererFrameGraph.md](../02-Contracts/RendererFrameGraph.md) | Validate color, depth, motion vectors, exposure, normals, history, jitter, camera matrices, and frame index. |
| RHI backend parity is reviewable. | [RHIContract.md](../02-Contracts/RHIContract.md), [ValidationMatrix.md](../03-Validation/ValidationMatrix.md) | Export a capability/parity report for D3D12 and Vulkan instead of relying on source reading. |
| Shader pipeline has regression evidence. | [ShaderPipeline.md](../02-Contracts/ShaderPipeline.md) | Add golden/reflection/package inspection tests and canonical package examples. |
| Runtime scene data remains decoupled from Renderer/RHI. | [RuntimeSceneData.md](../02-Contracts/RuntimeSceneData.md) | Add validation or tests for dependency direction and snapshot/invalidation expectations. |
| Launcher dependency policy is first-class. | [LauncherWorkflowReadiness.md](../04-Workflows/LauncherWorkflowReadiness.md) | Make required/optional/hardware-gated/backend-gated/provider-gated states easier to inspect and export. |
| Portfolio assets exercise baseline scenarios. | [PerformanceDiagnosticsPlan.md](../03-Validation/PerformanceDiagnosticsPlan.md) | Add small curated scenes/assets for empty frame, one mesh, many materials, descriptor pressure, upload pressure, shader cache hit/miss, startup/shutdown. |
| Ray tracing foundations are ready for high-end work. | [A_PrincipalRoleRequirements.md](../00-Review/A_PrincipalRoleRequirements.md), [PrincipalRenderingReadiness.md](../00-Review/PrincipalRenderingReadiness.md), [RHIContract.md](../02-Contracts/RHIContract.md) | Add AS lifecycle, SBT/RT pipeline, RT capability, validation, diagnostics, and parity surfaces before adding complex RT effects. |
| Path tracing can arrive as an MVP without reshaping Renderer/RHI. | [A_PrincipalRoleRequirements.md](../00-Review/A_PrincipalRoleRequirements.md), [RendererFrameGraph.md](../02-Contracts/RendererFrameGraph.md), [ShaderPipeline.md](../02-Contracts/ShaderPipeline.md) | Prepare accumulation, sampling, camera/material/light inputs, denoiser hooks, history reset, and baseline scene contracts. |
| PSO management is explicit and measurable. | [A_PrincipalRoleRequirements.md](../00-Review/A_PrincipalRoleRequirements.md), [RHIContract.md](../02-Contracts/RHIContract.md), [PerformanceDiagnosticsPlan.md](../03-Validation/PerformanceDiagnosticsPlan.md) | Add pipeline identity, cache/library, compile timing, miss/hit, invalidation, and shader package relationships. |
| Neural rendering readiness is concrete but feature-gated. | [A_PrincipalRoleRequirements.md](../00-Review/A_PrincipalRoleRequirements.md), [ShaderPipeline.md](../02-Contracts/ShaderPipeline.md), [RendererProviderContract.md](../02-Contracts/RendererProviderContract.md) | Add Slang/profile/capability gates, tensor/cooperative-vector placeholders, neural provider resource contracts, and diagnostics without adding a neural feature yet. |
| Multithreading lands after feature-foundation MVPs. | [A_PrincipalRoleRequirements.md](../00-Review/A_PrincipalRoleRequirements.md), [B_EngineArchitectureScorecard.md](../00-Review/B_EngineArchitectureScorecard.md), [RuntimeSceneData.md](../02-Contracts/RuntimeSceneData.md) | Add job-system and render-work scheduling only after RHI/Renderer/PSO/RT/path-tracing/neural MVP contracts are stable enough to parallelize safely. |

## Review-Driven Refactor Priorities

These are ordered by principal rendering readiness impact, not by how easy they are.

| Priority | Review source | Code movement required |
| --- | --- | --- |
| 1. RHI explicit control | Requirement cluster: D3D12/Vulkan explicit control. Scorecard: RHI, D3D12, Vulkan. | Make device, queues, command lists, fences, resources, descriptors, barriers, memory, interop, diagnostics, and capability reporting easier to inspect and harder to bypass. |
| 2. Renderer architecture | Requirement cluster: render architecture. Scorecard: renderer frame graph/passes. | Tighten pass registration, resource declaration, persistent/transient ownership, history resources, barrier diagnostics, and frame assembly contracts in code. |
| 3. Hardware-aware performance | Requirement cluster: hardware-aware performance. Scorecard: validation/performance evidence, D3D12/Vulkan. | Add typed diagnostics for memory budget, descriptor pressure, upload pressure, pass GPU timings, CPU timings, shader/package timing, and baseline scenarios. |
| 4. SDK/provider architecture | Requirement cluster: cross-IHV SDK integration and neural readiness. Scorecard: SDK/upscaling/provider. | Move SDK assumptions behind provider-neutral capability/resource contracts and state mappings before adding FidelityFX upscaling, DLSS/FSR frame generation, DLSS Ray Reconstruction / NRD-style denoising, or FidelityFX-style denoisers. Neural readiness stays in shader/profile/backend capability gates until Prompt 12 has source-backed data. |
| 5. Shader/compiler pipeline | Requirement cluster: shader/compiler pipeline. Scorecard: shader compiler/contracts/cook. | Add source-backed ABI tests, golden reflection/package coverage, feature/profile matrices, and runtime load timing. |
| 6. Runtime data boundary | Requirement cluster: render architecture and production modularity. Scorecard: GameFramework runtime data. | Preserve GameFramework independence while making scene snapshots, renderer translation, invalidation, and cooked compatibility explicit in code/tests. |
| 7. Ray tracing and path tracing foundations | Requirement cluster: ray tracing readiness. Scorecard: RHI, renderer, validation/performance evidence. | Prove AS lifecycle, RT pipeline/SBT contracts, path-tracing pass scaffolding, accumulation/history, sampling, and denoiser/provider hooks before advanced RT/path tracing work. |
| 8. PSO and shader runtime management | Requirement clusters: explicit graphics API control and hardware-aware performance. Scorecard: RHI and shader compiler/cook. | Make PSO identity, pipeline cache/library behavior, compile timing, shader package compatibility, and invalidation measurable. |
| 9. Neural rendering foundation | Requirement cluster: neural rendering readiness. Scorecard: provider integration and shader compiler/cook. | Add capability/profile gates and provider resource contracts for future neural paths without implementing neural rendering prematurely. |
| 10. Multithreading/job-system readiness | Requirement clusters: hardware-aware performance and production modularity. Scorecard: RHI, renderer, runtime data. | Add a job system and render-work scheduling after feature-foundation MVPs are stable, so concurrency follows clear ownership instead of hiding unclear contracts. |
| 11. Application/editor/tool review path | Requirement cluster: production reviewability. Scorecard: application, editor, launcher, docs. | Add structured lifecycle failures, editor diagnostics dashboard, launcher readiness reports, and portfolio run aggregation after the core architecture surfaces exist. |

## Universal Rules For Every Implementation Prompt

Copy these rules into every implementation session.

```text
You are working in the SparkleEngine repository.

Hard constraints:
- Read the review folder first, especially `A_PrincipalRoleRequirements.md`, `B_EngineArchitectureScorecard.md`, and `PrincipalRenderingReadiness.md`.
- Read the prompt's required production references from the `Prompt reference matrix` before coding.
- Start from patterns that exist in NRI, NVRHI, Donut, Cauldron, Falcor, RTXPT, Streamline, FidelityFX SDK, NRD, RTXNTC, or Slang. Do not invent a new architecture shape without source precedent.
- Do not add major rendering features unless the prompt explicitly asks for a validation, diagnostics, or baseline asset path.
- Do not add new third-party SDK integrations.
- Do not weaken architecture boundary checks.
- Do not broaden renderer-native D3D12/Vulkan/NVAPI/Streamline permissions.
- Keep GameFramework free of Renderer/RHI dependencies.
- Keep backend-native code inside RHI backend-private folders or provider-scoped exceptions documented by ADR.
- Do not satisfy a review requirement by adding only a wrapper, report, or launcher action when the scorecard calls for architectural code movement.
- Do not satisfy a review requirement by adding a parallel descriptive model that mirrors existing code. If a new type is added, it must be owned by the real subsystem and consumed by runtime, validation, diagnostics, or tooling.
- Daily refactor principle: every touched file must leave in better form than it was found. Prefer deleting, replacing, or simplifying unnecessary code over adding new code beside it.
- When adding a new type/function/path, state which existing complexity, ambiguity, duplication, or unsafe behavior it removes.
- Remove no-longer-needed code in the same touched path. Do not keep obsolete wrappers, stale comments, dead helpers, or misleading names just to minimize the diff.
- Do not parallelize unclear ownership. Multithreading/job-system work must land after RHI, Renderer, PSO, RT/path tracing, neural-readiness, diagnostics, and scene snapshot MVP contracts are stable.
- Prefer existing module patterns, existing launcher operation models, existing diagnostics types, and existing CMake target conventions.
- Add source-backed tests or validation commands when the prompt changes behavior.
- Update architecture docs only when the code change changes the actual contract or current status.
- Report changed files, production references inspected, reference patterns adopted/rejected, validation commands, acceptance criteria pass/fail, and follow-ups.
```

## Stage Order

1. Refactor RHI ownership, capabilities, memory, descriptors, interop, and diagnostics into reviewer-visible code surfaces.
2. Refactor renderer frame assembly, pass contracts, frame resources, history, barriers, and diagnostics so extension is safe.
3. Refactor provider capability/resource contracts so SDK integrations are vendor-neutral and future-ready.
4. Harden shader ABI, runtime loading, feature/profile gates, and regression coverage.
5. Harden GameFramework-to-Renderer scene snapshot ownership and invalidation.
6. Prepare PSO management, high-end ray tracing, path tracing, and neural rendering MVP foundations.
7. Add performance/memory diagnostics data plane and baseline artifacts.
8. Add the job system and multithreaded work scheduling last, after the feature-foundation MVPs are stable.
9. Surface validation through launcher, smoke, editor, and portfolio reports.
10. Add curated baseline assets and final reviewer packaging.

## Milestone Delivery Rules

Milestones are the shape of delivery. Prompts are the implementation packets inside each milestone.

Every milestone must leave the engine in a better working state:

- It must be independently buildable.
- It must produce at least one engine-visible result: a command, artifact, diagnostics snapshot, editor panel, smoke path, baseline scene, or runtime behavior.
- The visible result must come from real architecture/data, not a showcase-only side path.
- It must update validation status from unknown/planned toward partial/existing where the code proves it.
- It must have a clear rollback boundary and must not require later milestones to make the repository usable again.
- It must not weaken architecture boundaries or dependency direction.

The "cool thing" for each milestone is intentionally practical. It should make the engine feel more capable because the foundation got stronger.

## Milestone Roadmap

| Milestone | Engine evolution | Cool but non-showcase outcome | Main prompt bundle | Done when |
| --- | --- | --- | --- | --- |
| M0. Reviewer spine and baseline truth | The repository can explain what it is, what is available, and what is missing. | One launcher/report path can summarize toolchain, backend, dependency, provider, failure taxonomy, and boundary status with honest unavailable states. | Prompt 08, Prompt 15, Prompt 16, Prompt 28, Prompt 31, Prompt 32 | A reviewer can run or dry-run the evidence path before deeper rendering work starts. |
| M1. Hardware and RHI truth layer | RHI owns device, queue, fence, descriptor, memory, barrier, diagnostics, and native interop truth. | Backend capability and memory reports show D3D12/Vulkan support, debug/capture state, allocator stats, and unsupported features without source spelunking. | Prompt 01, Prompt 02, Prompt 18, Prompt 19 | Backend startup/shutdown and parity artifacts are source-backed and architecture boundaries pass. |
| M2. Frame assembly and renderer ownership | Renderer pass/resource/frame ownership becomes explicit enough to extend safely. | Empty-frame smoke emits frame graph/resource/timing artifacts that prove the frame can be assembled without hidden backend ownership. | Prompt 03, Prompt 17, Prompt 20, Prompt 26 | A new pass has an obvious place to declare resources, diagnostics, history, and failure state. |
| M3. Shader and PSO runtime | Shader packages and PSOs become runtime assets with identity, compatibility, cache, and timing evidence. | A shader/PSO inspection path shows package metadata, reflection, pipeline identity, cache status, compile/create timing, and failure reasons. | Prompt 05, Prompt 09, Prompt 22, Prompt 23 | PSO creation is no longer an opaque side effect inside passes. |
| M4. Runtime scene and asset baselines | GameFramework feeds renderer snapshots without learning RHI/Renderer details. | Curated tiny scenes exercise empty frame, one mesh, many materials, descriptor pressure, upload pressure, and shader cache hit/miss. | Prompt 06, Prompt 27, Prompt 29 | Scene mutation, snapshot, invalidation, and asset compatibility are visible in code/tests/artifacts. |
| M5. Provider readiness | SDK integration becomes provider-neutral for upscaling, frame generation, and denoising before additional SDKs arrive. | Provider diagnostics report the six capability states for the current DLSS upscaler path and planned DLSS/FSR frame-generation and denoiser families using the same vocabulary. | Prompt 04, Prompt 24, Prompt 25 | DLSS/Streamline are examples of the provider architecture, not the architecture itself. |
| M6. High-end ray tracing foundation MVP | Ray tracing has explicit AS lifecycle, RT pipeline, SBT, capability, and diagnostics ownership. | Supported hardware can run an RT readiness/AS lifecycle smoke; unsupported hardware produces a clean unavailable artifact. | Prompt 10 | BLAS/TLAS, scratch memory, AS memory, RT pipeline identity, and backend parity are reviewable. |
| M7. Path tracing foundation MVP | Path tracing can arrive as a renderer feature without redoing RHI, scene, shader, or provider contracts. | A path tracing readiness scenario proves accumulation/history reset, sampling frame index, material/light inputs, and denoiser hook state. | Prompt 11 | The next prompt can implement a minimal path tracer against named contracts instead of inventing architecture. |
| M8. Cross-feature proof | The feature foundations are wired together instead of living as isolated types. | One curated scenario set emits PSO, RT, path tracing readiness, neural readiness, descriptor, upload, memory, and timing sections. | Prompt 07, Prompt 13, Prompt 21, Prompt 30 | The editor or artifact bundle can show the engine is ready for heavy rendering work. |
| M9. Job system and multithreading last | Concurrency follows stable ownership instead of hiding unclear lifetime bugs. | Single-thread and multithread modes run the same smoke paths while diagnostics show job count, wait time, utilization, and CPU scheduling cost. | Prompt 14 | Initial parallel work operates on immutable snapshots or clearly owned data, with deterministic fallback. |
| M10. Portfolio-ready review run | All previous milestones are packaged into a repeatable reviewer path. | One portfolio review operation collects build/cook/smoke/shader/backend/diagnostics artifacts and writes a manifest. | Prompt 31, Prompt 32 | The repository can be evaluated under time pressure without a guided tour. |

Do not advance a milestone just because a report exists. Advance it only when the report is backed by the underlying RHI, Renderer, shader, provider, scene, or workflow changes the milestone requires.

## Refactor Track A: Principal Architecture Movement

Use these prompts before or alongside the validation prompts. They are the main work. The later validation and workflow prompts prove this work happened.

## Prompt 01 of 32 (M1): RHI Explicit Ownership And Capability Refactor

```text
Task:
Refactor the RHI public and backend-facing surfaces so explicit ownership and capability facts are inspectable in code.

Goal:
Satisfy the principal rendering requirement for explicit D3D12/Vulkan control over devices, queues, command lists, fences, descriptors, resources, barriers, pipeline state, memory, diagnostics, and native interop.

Primary review sources:
- Docs/Architecture/00-Review/A_PrincipalRoleRequirements.md
- Docs/Architecture/00-Review/B_EngineArchitectureScorecard.md
- Docs/Architecture/00-Review/PrincipalRenderingReadiness.md

Supporting docs:
- Docs/Architecture/02-Contracts/RHIContract.md
- Docs/Architecture/01-Boundaries/BoundaryRules.md
- Docs/Architecture/03-Validation/PerformanceDiagnosticsPlan.md

Production references to inspect first:
- NVIDIA NRI for explicit D3D12/Vulkan-level ownership surfaces and low-overhead backend contracts.
- NVRHI Programming Guide for resource state tracking, command list lifetime, binding sets, deferred destruction, and native escape hatches.
- NVIDIA Donut `DeviceManager` and Donut Samples for a production sample framework that keeps device ownership in the app/RHI host rather than adding a separate descriptive ownership graph.
- AMD Cauldron device/backend/resource abstractions for a cross-IHV DirectX 12/Vulkan framework pattern.

Code to inspect:
- Engine/RHI/Public/**/*.h
- Engine/RHI/Private/D3D12/**/*.h
- Engine/RHI/Private/D3D12/**/*.cpp
- Engine/RHI/Private/Vulkan/**/*.h
- Engine/RHI/Private/Vulkan/**/*.cpp
- Engine/Renderer/Private/**/*.cpp only where it consumes public RHI contracts

Implementation requirements:
- Daily refactor principle for every touched file:
  - Leave the file simpler, clearer, or more enforceable than it was before the change.
  - Prefer removing obsolete, duplicated, unused, or misleading code over adding new layers beside it.
  - Replace bad local patterns when they are in the touched path; do not preserve them just because they already exist.
  - Every new type/function must either reduce existing complexity, remove ambiguity, enforce a real contract, or feed runtime/validation/diagnostics/tooling.
  - Report what code was removed, simplified, or deliberately left unchanged because it was outside scope.
- Before editing code, write short implementation notes in the report:
  - Which reference files/classes were inspected.
  - Which concrete pattern is being applied.
  - Which tempting abstraction was rejected and why.
- Audit the public RHI surface and identify where ownership is implicit or scattered.
- Harden the actual ownership-bearing surfaces, not a parallel description of them:
  - `RenderDeviceServices` for backend creation, frame lifecycle, command submission, and shutdown.
  - D3D12/Vulkan backend RHI classes for device, adapter, queues, command allocators/lists, fences, and native handles.
  - `RhiResourceService` implementations for owned resources, transient memory blocks, aliasing resources, delayed destruction, initial state, and native resource access.
  - `RhiDescriptorService` implementations for descriptor allocations, descriptor tables, resource views, and release rules.
  - `RhiPipelineService` implementations for binding layouts, graphics PSOs, compute PSOs, shader binary compatibility, and creation failure reporting.
  - `RhiDiagnosticsService` and existing diagnostics types for validation, debug messages, object names, memory diagnostics, and timing.
  - `RhiInteropService` for native handle access by explicit consumer and reason.
- Add or refine functions/types only when they replace ambiguity at the real owner boundary for:
  - device and adapter identity
  - queue support and queue ownership
  - command list/command context lifetime
  - fence submission and completion ownership
  - resource lifetime and delayed destruction
  - resource states/barrier ownership
  - descriptor allocation/lifetime
  - pipeline and shader binary compatibility
  - memory allocator identity and budget support
  - diagnostics and validation support
  - native interop consumers and capability state
- Prefer small cohesive service methods, named policy structs, validation checks, or diagnostics emitted by the owning service over broad global helpers.
- Do not add an `RhiOwnershipModel`-style inventory, global ownership graph, or meta-report as the main implementation. If an ownership fact needs to be reported, expose it from the subsystem that owns the fact and consume it in a validation/reporting path.
- Do not add a type merely to make prose true. Every new public type must either:
  - replace unclear ownership at a call site
  - carry backend capability data that affects behavior
  - feed an existing/new validation artifact
  - remove duplicated backend-specific logic from consumers
- Keep backend-native implementation inside backend-private folders.
- Preserve Renderer access through public RHI services only.
- Add comments only where ownership would otherwise be unclear.

Acceptance criteria:
- A reviewer can identify ownership of devices, queues, command submission, fences, resources, barriers, descriptors, memory, diagnostics, and interop from the actual public RHI types and backend service boundaries.
- At least one implicit ownership hazard is removed or made enforceable in the actual service/call path, not only documented.
- D3D12 and Vulkan report truthful capability data through the same public shape.
- Renderer code does not gain direct backend-native dependencies.
- Any missing backend capability is explicit, not silently assumed.
- Architecture boundary check passes.

Validation:
- Build affected RHI targets.
- Run architecture boundary check.
- Run any existing D3D12/Vulkan smoke path that local SDKs permit.
- Search for new direct D3D12/Vulkan usage outside allowed backend/provider paths.

Report:
- Changed files.
- Production references inspected.
- Reference patterns adopted and rejected.
- Ownership surfaces changed.
- Backend parity notes.
- Commands run.
- Acceptance criteria pass/fail.
```

## Prompt 02 of 32 (M1): Backend Capability, Debug, Capture, And Memory Parity Refactor

```text
Task:
Refactor D3D12 and Vulkan backend capability/debug/memory reporting into a comparable backend parity surface.

Goal:
Move from prose-level backend parity to source-backed capability data that supports reviewer inspection and later report generation.

Primary review sources:
- Docs/Architecture/00-Review/A_PrincipalRoleRequirements.md
- Docs/Architecture/00-Review/B_EngineArchitectureScorecard.md
- Docs/Architecture/00-Review/PrincipalRenderingReadiness.md

Supporting docs:
- Docs/Architecture/02-Contracts/RHIContract.md
- Docs/Architecture/03-Validation/PerformanceDiagnosticsPlan.md

Production references to inspect first:
- NVIDIA NRI backend contracts for explicit device/backend capability patterns.
- NVRHI backend capability and device abstractions for backend-neutral feature reporting.
- AMD Cauldron D3D12/Vulkan device layers for cross-IHV debug, capture, memory, and unavailable-state reporting.

Code to inspect:
- Engine/RHI/Private/D3D12/**/*.cpp
- Engine/RHI/Private/Vulkan/**/*.cpp
- Engine/RHI/Public/**/*Capabilities*.h
- Engine/RHI/Public/**/*Diagnostic*.h
- Engine/RHI/Public/**/*Memory*.h

Implementation requirements:
- Daily refactor principle for every touched file:
  - Leave the file simpler, clearer, or more enforceable than it was before the change.
  - Prefer removing obsolete, duplicated, unused, or misleading code over adding new layers beside it.
  - Replace bad local patterns when they are in the touched path; do not preserve them just because they already exist.
  - Every new type/function must either reduce existing complexity, remove ambiguity, enforce a real contract, or feed runtime/validation/diagnostics/tooling.
  - Report what code was removed, simplified, or deliberately left unchanged because it was outside scope.
- Add or refine backend capability fields for:
  - API version or feature level
  - validation/debug layer state
  - timestamp query support
  - capture service support
  - crash diagnostics support
  - live object reporting support
  - object naming support
  - allocator budget support
  - residency or pressure support where available
  - ray tracing and PTLAS support state
- Add consistent object naming hooks where backend code already creates named GPU objects.
- Add D3D12/Vulkan memory allocator stats parity where allocator APIs support it.
- Report unsupported capabilities explicitly instead of leaving absent fields ambiguous.
- Do not implement new rendering features; this is capability/debug/memory surface work.

Acceptance criteria:
- D3D12 and Vulkan produce comparable capability snapshots.
- Unsupported or unavailable Vulkan features are represented honestly.
- D3D12 debug/capture/live-object capabilities are visible through backend-neutral diagnostics.
- Memory allocator diagnostics remain allocator-native first and renderer-summary second.
- Backend-native code remains in backend-private folders.

Validation:
- Build D3D12 and Vulkan targets where available.
- Run smoke/backend validation where local SDKs permit.
- Run architecture boundary check.

Report:
- Changed files.
- New/changed capability fields.
- D3D12/Vulkan support table.
- Commands run.
- Acceptance criteria pass/fail.
```

## Prompt 03 of 32 (M2): Renderer Frame Assembly And Resource Ownership Refactor

```text
Task:
Refactor renderer frame assembly so pass registration, resource declarations, persistent resources, transient resources, history, and diagnostics are explicit in code.

Goal:
Make the renderer architecture recognizable to principal reviewers as a safe pass/frame graph system rather than a set of implicit frame-side effects.

Primary review sources:
- Docs/Architecture/00-Review/A_PrincipalRoleRequirements.md
- Docs/Architecture/00-Review/B_EngineArchitectureScorecard.md
- Docs/Architecture/00-Review/PrincipalRenderingReadiness.md

Supporting docs:
- Docs/Architecture/02-Contracts/RendererFrameGraph.md
- Docs/Architecture/02-Contracts/RuntimeSceneData.md
- Docs/Architecture/03-Validation/PerformanceDiagnosticsPlan.md

Production references to inspect first:
- NVIDIA Donut render passes and sample frame flow for practical pass/resource ownership patterns.
- Donut Samples for reviewer-friendly sample pass integration and frame lifecycle shape.
- Falcor RenderGraph for explicit resource declaration, pass registration, and graph diagnostics.
- NVRHI command/resource state patterns for renderer-to-RHI barrier and lifetime boundaries.

Code to inspect:
- Engine/Renderer/Public/**/*.h
- Engine/Renderer/Private/FrameGraph/**/*.h
- Engine/Renderer/Private/FrameGraph/**/*.cpp
- Engine/Renderer/Private/FramePipeline/**/*.h
- Engine/Renderer/Private/FramePipeline/**/*.cpp
- Engine/Renderer/Private/Frame/**/*.h
- Engine/Renderer/Private/Passes/**/*.h
- Engine/Renderer/Private/Passes/**/*.cpp
- Engine/Renderer/Private/Temporal/**/*.h
- Engine/Renderer/Private/RayTracing/**/*.h

Implementation requirements:
- Daily refactor principle for every touched file:
  - Leave the file simpler, clearer, or more enforceable than it was before the change.
  - Prefer removing obsolete, duplicated, unused, or misleading code over adding new layers beside it.
  - Replace bad local patterns when they are in the touched path; do not preserve them just because they already exist.
  - Every new type/function must either reduce existing complexity, remove ambiguity, enforce a real contract, or feed runtime/validation/diagnostics/tooling.
  - Report what code was removed, simplified, or deliberately left unchanged because it was outside scope.
- Identify frame assembly entry points and make pass registration/resource ownership obvious.
- Separate or name:
  - transient frame graph resources
  - persistent/history resources
  - imported/back-buffer resources
  - provider input resources
  - viewport products
  - ray tracing acceleration-structure resources
- Add typed contracts or structs where resources are currently passed implicitly.
- Make reset/invalidation rules explicit for history resources.
- Ensure pass setup declares all resources that execution touches.
- Surface frame graph diagnostics for missing declarations, unresolved barriers, invalid products, and missing optional services.
- Avoid introducing backend-native access into Renderer.

Acceptance criteria:
- A reviewer can trace how color, depth, normals, motion vectors, exposure, history, jitter, camera matrices, and frame index enter frame assembly.
- New passes have an obvious registration and resource declaration pattern.
- Persistent resources and transient resources are distinguishable in code.
- Frame graph diagnostics summarize contract failures.
- Architecture boundary check passes.

Validation:
- Build Renderer.
- Run renderer smoke if available.
- Run architecture boundary check.
- Search Renderer for disallowed backend-native dependencies.

Report:
- Changed files.
- Resource ownership changes.
- Diagnostics added.
- Commands run.
- Acceptance criteria pass/fail.
```

## Prompt 04 of 32 (M5): Provider-Neutral SDK Architecture Refactor

```text
Task:
Refactor provider integration so upscalers, frame generation, and denoisers share a provider-neutral architecture.

Goal:
Prevent DLSS/Streamline from becoming the architecture and prepare the repo for NVIDIA/AMD SDK review without adding new SDK features.

Planned provider families for this milestone:
- upscaling: current DLSS/Streamline path first, later FSR-style AMD upscaling
- frame generation: future DLSS Frame Generation on NVIDIA and FSR Frame Generation on AMD
- denoising: future DLSS Ray Reconstruction / NRD-style NVIDIA denoising and FidelityFX-style AMD alternatives

Out of scope for this prompt:
- neural rendering provider category support; Prompt 12 adds neural readiness only when shader/profile/backend gates are source-backed
- ray tracing extension as a standalone provider category; ray tracing backend features stay in RHI/renderer ray tracing contracts unless a concrete SDK integration requires a provider boundary

Primary review sources:
- Docs/Architecture/00-Review/A_PrincipalRoleRequirements.md
- Docs/Architecture/00-Review/B_EngineArchitectureScorecard.md
- Docs/Architecture/00-Review/PrincipalRenderingReadiness.md

Supporting docs:
- Docs/Architecture/02-Contracts/RendererProviderContract.md
- Docs/Architecture/01-Boundaries/BoundaryRules.md
- Docs/Architecture/04-Workflows/LauncherWorkflowReadiness.md

Production references to inspect first:
- NVIDIA Streamline for provider-style SDK integration and capability checks.
- Streamline Sample for concrete runtime/provider wiring and failure handling.
- FidelityFX SDK for AMD provider integration and feature availability shape.
- AMD Cauldron FidelityFX integration for cross-IHV provider/backend boundaries.

Code to inspect:
- Engine/Renderer/Private/Upscaling/**/*.h
- Engine/Renderer/Private/Upscaling/**/*.cpp
- Engine/Renderer/Private/Upscaling/NvidiaDlss/**/*.h
- Engine/Renderer/Private/Upscaling/NvidiaDlss/**/*.cpp
- Engine/RHI/Public/Interop/**/*.h
- Tools/Launcher/SparkleLauncher/Private/Core/SourceDependencyState.cpp
- Tools/Launcher/SparkleLauncher/Private/Core/HostGraphicsCapabilities.cpp

Implementation requirements:
- Daily refactor principle for every touched file:
  - Leave the file simpler, clearer, or more enforceable than it was before the change.
  - Prefer removing obsolete, duplicated, unused, or misleading code over adding new layers beside it.
  - Replace bad local patterns when they are in the touched path; do not preserve them just because they already exist.
  - Every new type/function must either reduce existing complexity, remove ambiguity, enforce a real contract, or feed runtime/validation/diagnostics/tooling.
  - Report what code was removed, simplified, or deliberately left unchanged because it was outside scope.
- Add provider-neutral capability state mapping for exactly:
  - `unavailable`
  - `missing dependency`
  - `unsupported hardware`
  - `available`
  - `enabled`
  - `runtime failed`
- Add provider category vocabulary:
  - upscaler
  - denoiser
  - frame generation
- Do not add provider categories that are not part of the staged implementation path. `neural rendering` is introduced later by Prompt 12 only if it is tied to concrete shader/profile/backend readiness data.
- Keep current DLSS implementation as one provider implementation.
- Add resource contract validation vocabulary for color, depth, motion vectors, exposure, normals, history, jitter, camera matrices, and frame index.
- Move provider-independent assumptions out of NVIDIA-specific files where practical.
- Keep native interop access routed through RHI interop services.
- Do not add FidelityFX or a new provider yet.
- Do not add DLSS Frame Generation, FSR Frame Generation, DLSS Ray Reconstruction, NRD, or FidelityFX denoiser implementations yet. This prompt only makes the shared provider model ready for those follow-up categories.

Acceptance criteria:
- Provider-neutral types exist outside NVIDIA-specific code.
- Shared provider category code includes only staged categories: upscaler, denoiser, and frame generation.
- DLSS maps into the shared provider model without behavior regression.
- Missing SDK, unsupported hardware, available, enabled, and runtime-failed states are distinguishable in diagnostics.
- Required/optional resource contract state is visible in code.
- Provider-native API use remains provider-scoped and boundary checked.

Validation:
- Build Renderer and provider targets.
- Run provider smoke path if available.
- Run launcher readiness/reporting path if touched.
- Run architecture boundary check.

Report:
- Changed files.
- Provider-neutral API shape.
- State mapping table.
- Commands run.
- Acceptance criteria pass/fail.
```

## Prompt 05 of 32 (M3): Shader ABI And Runtime Pipeline Hardening Refactor

```text
Task:
Harden shader registration, reflection, cooked package ABI, cache identity, and runtime load/pipeline creation paths.

Goal:
Turn Sparkle's already strong shader pipeline into principal-level evidence through code structure and regression coverage.

Primary review sources:
- Docs/Architecture/00-Review/A_PrincipalRoleRequirements.md
- Docs/Architecture/00-Review/B_EngineArchitectureScorecard.md
- Docs/Architecture/00-Review/PrincipalRenderingReadiness.md

Supporting docs:
- Docs/Architecture/02-Contracts/ShaderPipeline.md
- Docs/Architecture/03-Validation/PerformanceDiagnosticsPlan.md
- Docs/Architecture/03-Validation/ValidationMatrix.md

Production references to inspect first:
- NVRHI shader and pipeline APIs for runtime shader/pipeline object relationships.
- NVIDIA Donut shader compiler usage for practical shader cook/runtime registration patterns.
- AMD Cauldron shader/runtime patterns for sample-framework shader packaging.
- Slang for profile, target, reflection, and future neural shader readiness constraints.

Code to inspect:
- Tools/Shaders/ShaderCompiler/**/*.h
- Tools/Shaders/ShaderCompiler/**/*.cpp
- Tools/Shaders/ShaderContracts/**/*
- Engine/Renderer/ShaderRegistrations/**/*
- Engine/RHI/Public/**/*Shader*.h
- Engine/RHI/Public/**/*Pipeline*.h
- Engine/Renderer/Private/**/*Shader*.cpp

Implementation requirements:
- Daily refactor principle for every touched file:
  - Leave the file simpler, clearer, or more enforceable than it was before the change.
  - Prefer removing obsolete, duplicated, unused, or misleading code over adding new layers beside it.
  - Replace bad local patterns when they are in the touched path; do not preserve them just because they already exist.
  - Every new type/function must either reduce existing complexity, remove ambiguity, enforce a real contract, or feed runtime/validation/diagnostics/tooling.
  - Report what code was removed, simplified, or deliberately left unchanged because it was outside scope.
- Identify the canonical shader package ABI structs and runtime load path.
- Make package metadata, target/profile, reflection, binding layout hash, feature flags, and cache identity explicit and inspectable.
- Add regression coverage for representative shader packages.
- Add runtime load/reload timing hooks.
- Add or update feature/profile matrix code where the compiler already supports targets.
- Keep DXIL/SPIR-V behavior source-backed; do not claim unsupported profiles.
- Avoid changing shader semantics unless needed to fix discovered ABI issues.

Acceptance criteria:
- A reviewer can trace shader source to reflection to cooked package to runtime pipeline creation.
- `list-shaders --validate`, `inspect-shader`, and `inspect-package` are covered by regression or validation flow.
- Runtime load timing is visible in diagnostics or reports.
- Cache identity changes are intentional and testable.
- Shader feature/profile support is reported honestly.

Validation:
- Run ShaderCompiler validation commands.
- Run at least one representative package cook and inspect path.
- Build affected shader/runtime targets.
- Run architecture boundary check.

Report:
- Changed files.
- ABI surfaces touched.
- Representative shaders/packages tested.
- Commands run.
- Acceptance criteria pass/fail.
```

## Prompt 06 of 32 (M4): Runtime Scene Snapshot Boundary Refactor

```text
Task:
Refactor and validate the GameFramework-to-Renderer scene snapshot boundary.

Goal:
Protect the architecture reviewers care about: GameFramework owns runtime scene data, Renderer owns render translation, and RHI/backend details stay out of gameplay data.

Primary review sources:
- Docs/Architecture/00-Review/A_PrincipalRoleRequirements.md
- Docs/Architecture/00-Review/B_EngineArchitectureScorecard.md
- Docs/Architecture/00-Review/PrincipalRenderingReadiness.md

Supporting docs:
- Docs/Architecture/02-Contracts/RuntimeSceneData.md
- Docs/Architecture/02-Contracts/RendererFrameGraph.md
- Docs/Architecture/01-Boundaries/BoundaryRules.md

Production references to inspect first:
- NVIDIA Donut scene/component graph for renderer-facing scene translation patterns.
- AMD Cauldron scene/content systems for asset and runtime data boundaries.
- Falcor scene model for scene snapshots, renderer-owned data, and invalidation patterns.

Code to inspect:
- Engine/GameFramework/CMakeLists.txt
- Engine/GameFramework/Public/Scene/**/*.h
- Engine/GameFramework/Private/Scene/**/*.cpp
- Engine/GameFramework/Public/Assets/**/*.h
- Engine/Renderer/Private/SceneData/**/*.h
- Engine/Renderer/Private/SceneData/**/*.cpp
- Engine/Renderer/Private/FramePipeline/**/*.cpp

Implementation requirements:
- Daily refactor principle for every touched file:
  - Leave the file simpler, clearer, or more enforceable than it was before the change.
  - Prefer removing obsolete, duplicated, unused, or misleading code over adding new layers beside it.
  - Replace bad local patterns when they are in the touched path; do not preserve them just because they already exist.
  - Every new type/function must either reduce existing complexity, remove ambiguity, enforce a real contract, or feed runtime/validation/diagnostics/tooling.
  - Report what code was removed, simplified, or deliberately left unchanged because it was outside scope.
- Identify mutable GameFramework scene ownership and renderer snapshot consumption.
- Make snapshot creation, renderer translation, and invalidation rules explicit in code.
- Add or refine types for stable asset ids/handles, material snapshots, mesh snapshots, texture snapshots, camera snapshots, lighting snapshots, and animation/skeleton references where needed.
- Add diagnostics or tests for invalidation on level unload, level change, asset append, scene reset, and render-state reset.
- Add cooked asset compatibility/version checks if missing or unclear.
- Do not add Renderer/RHI dependencies to GameFramework.

Acceptance criteria:
- GameFramework still links only to allowed lower-level modules.
- Renderer consumes snapshots or renderer-owned translated data, not mutable GameFramework internals as render-owned state.
- Invalidation paths are named and testable or logged.
- Cooked/runtime asset compatibility is at least checked or explicitly reported.
- Architecture boundary check passes.

Validation:
- Run architecture boundary check.
- Build GameFramework and Renderer.
- Run runtime project load smoke if available.
- Run relevant scene/cook tests if present.

Report:
- Changed files.
- Boundary protections added.
- Invalidation behavior summary.
- Commands run.
- Acceptance criteria pass/fail.
```

## Prompt 07 of 32 (M8): Diagnostics Data Plane Refactor

```text
Task:
Refactor diagnostics into a typed data plane from RHI allocator/backend truth through renderer summaries to editor/tooling presentation.

Goal:
Make hardware-aware performance and memory reasoning visible in code before adding heavier rendering, SDK, or neural features.

Primary review sources:
- Docs/Architecture/00-Review/A_PrincipalRoleRequirements.md
- Docs/Architecture/00-Review/B_EngineArchitectureScorecard.md
- Docs/Architecture/00-Review/PrincipalRenderingReadiness.md

Supporting docs:
- Docs/Architecture/03-Validation/PerformanceDiagnosticsPlan.md
- Docs/Architecture/02-Contracts/RHIContract.md
- Docs/Architecture/02-Contracts/RendererFrameGraph.md

Production references to inspect first:
- NVRHI validation and lifetime diagnostics for source-backed GPU object evidence.
- AMD Cauldron diagnostics/profiler patterns for backend and renderer metrics.
- Falcor profiling and graph evidence for renderer-facing performance diagnostics.

Code to inspect:
- Engine/RHI/Public/**/*Diagnostic*.h
- Engine/RHI/Public/**/*Memory*.h
- Engine/RHI/Private/D3D12/**/*Memory*.cpp
- Engine/RHI/Private/Vulkan/**/*Memory*.cpp
- Engine/Renderer/Private/Diagnostics/**/*.h
- Engine/Renderer/Private/Diagnostics/**/*.cpp
- Engine/Editor/Private/**/*Profiler*.h
- Tools/Launcher/SparkleLauncher/Private/Launch/Smoke/**/*.cpp

Implementation requirements:
- Daily refactor principle for every touched file:
  - Leave the file simpler, clearer, or more enforceable than it was before the change.
  - Prefer removing obsolete, duplicated, unused, or misleading code over adding new layers beside it.
  - Replace bad local patterns when they are in the touched path; do not preserve them just because they already exist.
  - Every new type/function must either reduce existing complexity, remove ambiguity, enforce a real contract, or feed runtime/validation/diagnostics/tooling.
  - Report what code was removed, simplified, or deliberately left unchanged because it was outside scope.
- Keep three layers distinct:
  - backend allocator/native truth
  - renderer interpretation/summaries
  - editor/tool/artifact presentation
- Add typed snapshots for missing/partial metrics:
  - API version or feature level
  - validation state
  - enabled/requested providers
  - descriptor occupancy/pressure
  - pipeline cache stats or explicit unavailable state
  - shader package load timing
  - upload pressure
  - pass GPU timings
  - CPU frame timings
- Add stable units and status flags to every metric.
- Emit diagnostics to text artifacts before building more UI.
- Do not collapse backend allocator data into renderer-only summaries.

Acceptance criteria:
- A reviewer can inspect where each metric originates.
- Missing metrics are explicitly unavailable, planned, or unsupported.
- D3D12MA/VMA-backed memory data remains identifiable.
- Descriptor and upload pressure have first-class typed fields.
- Artifact output can consume the same data model as editor UI.

Validation:
- Build affected RHI/Renderer/Editor targets.
- Run diagnostics artifact path if available.
- Run architecture boundary check.

Report:
- Changed files.
- New diagnostics types.
- Metrics covered and remaining gaps.
- Commands run.
- Acceptance criteria pass/fail.
```

## Prompt 08 of 32 (M0): Application Lifecycle And Typed Failure Refactor

```text
Task:
Refactor application, validation, and launcher failure reporting around typed lifecycle categories.

Goal:
Make startup, backend creation, project load, shader cook, runtime validation, editor-only failure, and shutdown behavior reviewable and actionable.

Primary review sources:
- Docs/Architecture/00-Review/A_PrincipalRoleRequirements.md
- Docs/Architecture/00-Review/B_EngineArchitectureScorecard.md
- Docs/Architecture/00-Review/PrincipalRenderingReadiness.md

Supporting docs:
- Docs/Architecture/02-Contracts/ApplicationLifecycle.md
- Docs/Architecture/04-Workflows/LauncherWorkflowReadiness.md
- Docs/Architecture/03-Validation/ValidationMatrix.md

Production references to inspect first:
- NVIDIA Donut app/device manager lifecycle for device creation, frame loop, and shutdown shape.
- AMD Cauldron sample lifecycle for startup/backend/failure separation.
- Falcor application/sample lifecycle for runtime/editor-style validation and diagnostics flow.

Code to inspect:
- Engine/Application/Public/**/*.h
- Engine/Application/Private/**/*.cpp
- Engine/Editor/**/*.cpp
- Tools/Launcher/SparkleLauncher/Private/Gui/Models/LauncherRecoveryUiModel.cpp
- Tools/Launcher/SparkleLauncher/Private/Gui/Shell/LauncherMainWindowOperations.cpp
- Tools/Launcher/SparkleLauncher/Private/Launch/Smoke/**/*.cpp

Implementation requirements:
- Daily refactor principle for every touched file:
  - Leave the file simpler, clearer, or more enforceable than it was before the change.
  - Prefer removing obsolete, duplicated, unused, or misleading code over adding new layers beside it.
  - Replace bad local patterns when they are in the touched path; do not preserve them just because they already exist.
  - Every new type/function must either reduce existing complexity, remove ambiguity, enforce a real contract, or feed runtime/validation/diagnostics/tooling.
  - Report what code was removed, simplified, or deliberately left unchanged because it was outside scope.
- Add structured failure categories for:
  - missing SDK
  - missing source dependency
  - unsupported hardware
  - invalid project
  - shader cook failure
  - backend creation failure
  - runtime validation failure
  - editor-only failure
- Preserve existing detailed messages.
- Ensure runtime target remains free of editor-only recook/validation implementation.
- Add failure categories to smoke metadata and launcher reports where applicable.
- Route recovery suggestions from category plus context, not brittle string matching where practical.

Acceptance criteria:
- Failure reports include a category and detailed reason.
- Backend creation failure is distinguishable from invalid project and missing SDK.
- Editor-only failures are not treated as runtime requirements.
- Existing recovery messaging still works.
- Runtime/editor target separation remains intact.

Validation:
- Build launcher/application/editor targets.
- Run relevant dry-run and smoke paths.
- Run architecture boundary check.

Report:
- Changed files.
- Failure category mapping.
- Commands run.
- Acceptance criteria pass/fail.
```

## Refactor Track B: Future Feature Foundation MVPs

These prompts prepare the repository for high-end ray tracing, path tracing, PSO management, neural rendering, and multithreaded execution. They are not final feature implementations. They establish the minimum architecture, diagnostics, validation, and assets needed so each future feature can be delivered without reshaping the engine again.

In this track, "MVP" means a minimal executable proof of the foundation: typed code surfaces, capability states, diagnostics, validation/smoke paths, and small baseline scenarios. It does not mean a polished feature showcase.

The required order is intentional:

1. PSO management foundation.
2. High-end ray tracing foundation.
3. Path tracing foundation.
4. Neural rendering foundation.
5. Cross-feature MVP proof scenarios.
6. Job system and multithreading foundation last.

Multithreading is last because parallel execution will magnify unclear lifetime, ownership, barrier, PSO, scene snapshot, and diagnostics contracts. The engine should first know what work exists before it starts spreading that work across threads.

## Prompt 09 of 32 (M3): PSO Management Foundation MVP

```text
Task:
Build the foundation for explicit PSO and pipeline cache management.

Goal:
Prepare the engine for high-end rendering workloads where PSO identity, cache reuse, compile timing, invalidation, shader-package compatibility, and pipeline-library behavior are measurable and reviewable.

Primary review sources:
- Docs/Architecture/00-Review/A_PrincipalRoleRequirements.md
- Docs/Architecture/00-Review/B_EngineArchitectureScorecard.md
- Docs/Architecture/00-Review/PrincipalRenderingReadiness.md

Supporting docs:
- Docs/Architecture/02-Contracts/RHIContract.md
- Docs/Architecture/02-Contracts/ShaderPipeline.md
- Docs/Architecture/03-Validation/PerformanceDiagnosticsPlan.md

Production references to inspect first:
- NVRHI pipeline state APIs for backend-neutral graphics/compute pipeline identity.
- NVIDIA NRI pipeline abstractions for low-level pipeline creation and backend parity.
- AMD Cauldron PSO and shader runtime handling for practical cache/timing diagnostics.

Code to inspect:
- Engine/RHI/Public/**/*Pipeline*.h
- Engine/RHI/Public/**/*Shader*.h
- Engine/RHI/Private/D3D12/**/*Pipeline*.cpp
- Engine/RHI/Private/Vulkan/**/*Pipeline*.cpp
- Engine/Renderer/Private/Passes/**/*.h
- Engine/Renderer/Private/Passes/**/*.cpp
- Engine/Renderer/ShaderRegistrations/**/*
- Tools/Shaders/ShaderCompiler/**/*.cpp

Implementation requirements:
- Daily refactor principle for every touched file:
  - Leave the file simpler, clearer, or more enforceable than it was before the change.
  - Prefer removing obsolete, duplicated, unused, or misleading code over adding new layers beside it.
  - Replace bad local patterns when they are in the touched path; do not preserve them just because they already exist.
  - Every new type/function must either reduce existing complexity, remove ambiguity, enforce a real contract, or feed runtime/validation/diagnostics/tooling.
  - Report what code was removed, simplified, or deliberately left unchanged because it was outside scope.
- Add a backend-neutral PSO identity model that includes:
  - shader package id
  - shader entry/stage set
  - binding layout id/hash
  - render target/depth formats
  - raster/depth/blend state where applicable
  - compute/ray tracing state where applicable
  - backend target format
- Add a PSO diagnostics snapshot with:
  - created pipeline count
  - cache hit/miss where source-backed
  - compile/create time
  - failure reason
  - shader package compatibility reason
  - backend-specific unavailable fields marked clearly
- Add an explicit unavailable state if a backend does not yet expose pipeline library/cache functionality.
- Do not hide expensive pipeline creation inside passes without a named runtime/cache surface.
- Do not change shader semantics.

Acceptance criteria:
- A reviewer can identify how a PSO is keyed, created, reused, invalidated, and diagnosed.
- Pipeline creation timing is visible in diagnostics or artifacts.
- Missing pipeline cache support is explicit, not silent.
- PSO identity ties back to shader package ABI and RHI pipeline services.
- D3D12 and Vulkan behavior is represented honestly.
- Architecture boundary check passes.

Validation:
- Build RHI, Renderer, ShaderCompiler where affected.
- Run shader inspection/cook commands for at least one package.
- Run a smoke or baseline path that creates graphics PSOs.
- Inspect diagnostics for pipeline creation/cache fields.
- Run architecture boundary check.

Report:
- Changed files.
- PSO identity fields.
- Cache/timing fields added.
- Commands run.
- Acceptance criteria pass/fail.
```

## Prompt 10 of 32 (M6): High-End Ray Tracing Foundation MVP

```text
Task:
Build the foundation for high-end ray tracing architecture without adding a full new renderer feature.

Goal:
Prepare the engine for advanced ray tracing work by making BLAS/TLAS lifecycle, RT pipeline state, shader table expectations, AS update/compaction policy, backend parity, diagnostics, and validation explicit.

Primary review sources:
- Docs/Architecture/00-Review/A_PrincipalRoleRequirements.md
- Docs/Architecture/00-Review/B_EngineArchitectureScorecard.md
- Docs/Architecture/00-Review/PrincipalRenderingReadiness.md

Supporting docs:
- Docs/Architecture/02-Contracts/RHIContract.md
- Docs/Architecture/02-Contracts/RendererFrameGraph.md
- Docs/Architecture/02-Contracts/ShaderPipeline.md
- Docs/Architecture/03-Validation/PerformanceDiagnosticsPlan.md

Production references to inspect first:
- NVRHI ray tracing tutorial/sample patterns for BLAS/TLAS, RT pipelines, and shader table expectations.
- NVIDIA Donut ray tracing samples for practical renderer integration.
- Falcor ray tracing architecture for scene-to-AS and graph-based RT diagnostics.
- RTXPT for high-end path/ray tracing proof structure and artifact expectations.

Code to inspect:
- Engine/RHI/Public/**/*RayTracing*.h
- Engine/RHI/Private/D3D12/**/*RayTracing*.cpp
- Engine/RHI/Private/Vulkan/**/*RayTracing*.cpp
- Engine/Renderer/Private/RayTracing/**/*.h
- Engine/Renderer/Private/RayTracing/**/*.cpp
- Engine/Renderer/Private/FrameGraph/**/*.h
- Engine/Renderer/Private/FramePipeline/**/*.cpp
- Tools/Shaders/ShaderCompiler/**/*RayTracing* if present

Implementation requirements:
- Daily refactor principle for every touched file:
  - Leave the file simpler, clearer, or more enforceable than it was before the change.
  - Prefer removing obsolete, duplicated, unused, or misleading code over adding new layers beside it.
  - Replace bad local patterns when they are in the touched path; do not preserve them just because they already exist.
  - Every new type/function must either reduce existing complexity, remove ambiguity, enforce a real contract, or feed runtime/validation/diagnostics/tooling.
  - Report what code was removed, simplified, or deliberately left unchanged because it was outside scope.
- Make the RT ownership model explicit in code:
  - BLAS input ownership
  - BLAS build/update flags
  - TLAS build/update ownership
  - scratch buffer ownership
  - AS buffer ownership
  - compaction support state
  - shader table or equivalent binding expectations
  - RT pipeline identity and creation diagnostics
- Add backend-neutral diagnostics for:
  - RT supported/unsupported
  - BLAS/TLAS count
  - build/update timing
  - scratch memory usage
  - AS memory usage
  - classic TLAS versus partitioned TLAS provider path
  - backend-specific unavailable features
- Ensure Renderer consumes RT resources through RHI/frame-graph handles and pass services.
- Add validation for missing acceleration structures or unsupported backend capability.
- Do not add a full path tracer in this prompt.

Acceptance criteria:
- A reviewer can trace AS lifecycle from scene data to RHI services to frame graph binding.
- D3D12/Vulkan RT capability differences are visible and honest.
- RT pipeline/shader-table expectations are named even if not fully implemented on every backend.
- RT diagnostics appear in a smoke/baseline artifact or typed diagnostics snapshot.
- Missing RT support fails/skips clearly.
- Architecture boundary check passes.

Validation:
- Build RHI and Renderer.
- Run existing RT smoke if available.
- Run D3D12 backend path and Vulkan path where available.
- Inspect RT diagnostics output.
- Run architecture boundary check.

Report:
- Changed files.
- RT lifecycle surfaces added.
- Backend support/unavailable table.
- Commands run.
- Acceptance criteria pass/fail.
```

## Prompt 11 of 32 (M7): Path Tracing Foundation MVP

```text
Task:
Create the architectural MVP foundation for a future path tracer.

Goal:
Prepare renderer, shader, scene, resource, diagnostics, and asset contracts so a minimal path tracing implementation can land next without reshaping the engine.

Primary review sources:
- Docs/Architecture/00-Review/A_PrincipalRoleRequirements.md
- Docs/Architecture/00-Review/B_EngineArchitectureScorecard.md
- Docs/Architecture/00-Review/PrincipalRenderingReadiness.md

Supporting docs:
- Docs/Architecture/02-Contracts/RendererFrameGraph.md
- Docs/Architecture/02-Contracts/RHIContract.md
- Docs/Architecture/02-Contracts/RuntimeSceneData.md
- Docs/Architecture/02-Contracts/ShaderPipeline.md
- Docs/Architecture/02-Contracts/RendererProviderContract.md

Production references to inspect first:
- Falcor path tracer structure for accumulation, scene inputs, denoising hooks, and diagnostics.
- RTXPT for modern high-end path tracing integration and validation proof shape.
- NRD for denoiser input contracts and temporal/resource expectations.
- NVIDIA Donut ray tracing samples for a smaller sample-framework path to RT/path tracing readiness.

Code to inspect:
- Engine/Renderer/Private/RayTracing/**/*.h
- Engine/Renderer/Private/Passes/**/*.h
- Engine/Renderer/Private/Temporal/**/*.h
- Engine/Renderer/Private/Frame/Targets/**/*.h
- Engine/Renderer/Private/SceneData/**/*.h
- Engine/GameFramework/Public/Scene/**/*.h
- Engine/Renderer/ShaderRegistrations/**/*
- Tools/Shaders/ShaderCompiler/**/*

Implementation requirements:
- Daily refactor principle for every touched file:
  - Leave the file simpler, clearer, or more enforceable than it was before the change.
  - Prefer removing obsolete, duplicated, unused, or misleading code over adding new layers beside it.
  - Replace bad local patterns when they are in the touched path; do not preserve them just because they already exist.
  - Every new type/function must either reduce existing complexity, remove ambiguity, enforce a real contract, or feed runtime/validation/diagnostics/tooling.
  - Report what code was removed, simplified, or deliberately left unchanged because it was outside scope.
- Add renderer-owned path tracing readiness contracts for:
  - camera state and jitter/reset interaction
  - sample index and accumulation frame index
  - accumulation/history texture ownership
  - history invalidation on camera cut, resize, scene reset, material/geometry changes
  - material and light data required by a path tracer
  - sky/environment input contract where source-backed
  - denoiser/provider hook point
  - output product naming
- Add shader registration/package placeholders only if the current shader pipeline can support them without fake functionality.
- Add diagnostics fields for accumulation reset reason, samples accumulated, selected RT backend, AS availability, and denoiser hook state.
- Add a tiny baseline scene contract for future path tracing proof.
- Do not implement a full path tracing algorithm here unless explicitly requested by a later prompt.

Acceptance criteria:
- Future path tracing can be added as a renderer pass/provider path against named contracts.
- Accumulation/history ownership is explicit and resettable.
- Scene/material/light data requirements are listed in code-facing structs or diagnostics.
- Denoiser integration point is provider-neutral.
- Missing RT or shader capability reports unavailable clearly.
- Architecture boundary check passes.

Validation:
- Build Renderer and ShaderCompiler if touched.
- Run frame graph/resource diagnostics path if available.
- Run architecture boundary check.
- Inspect diagnostics for path tracing readiness fields.

Report:
- Changed files.
- Path tracing readiness contracts added.
- Missing future implementation pieces.
- Commands run.
- Acceptance criteria pass/fail.
```

## Prompt 12 of 32 (M5): Neural Rendering Foundation MVP

```text
Task:
Create the architectural MVP foundation for future neural rendering paths.

Goal:
Prepare shader/profile gates, provider resource contracts, diagnostics, and backend capability reporting for neural rendering without adding a neural rendering feature prematurely.

Primary review sources:
- Docs/Architecture/00-Review/A_PrincipalRoleRequirements.md
- Docs/Architecture/00-Review/B_EngineArchitectureScorecard.md
- Docs/Architecture/00-Review/PrincipalRenderingReadiness.md

Supporting docs:
- Docs/Architecture/02-Contracts/ShaderPipeline.md
- Docs/Architecture/02-Contracts/RendererProviderContract.md
- Docs/Architecture/02-Contracts/RHIContract.md
- Docs/Architecture/03-Validation/PerformanceDiagnosticsPlan.md

Production references to inspect first:
- Slang for shader profile gates, target capabilities, and reflection constraints.
- Neural Shading SIGGRAPH 2025 examples for Slang-based neural shading experiment structure.
- RTXNTC for neural texture compression integration constraints and material/texture data expectations.
- Streamline and FidelityFX SDK for provider capability/failure-state patterns without hardwiring a single vendor.

Code to inspect:
- Tools/Shaders/ShaderCompiler/Backends/Slang/**/*.h
- Tools/Shaders/ShaderCompiler/Backends/Slang/**/*.cpp
- Tools/Shaders/ShaderCompiler/Private/Compiler/**/*.h
- Engine/RHI/Public/**/*Capabilities*.h
- Engine/Renderer/Private/Upscaling/**/*.h
- Engine/Renderer/Private/Diagnostics/**/*.h

Implementation requirements:
- Daily refactor principle for every touched file:
  - Leave the file simpler, clearer, or more enforceable than it was before the change.
  - Prefer removing obsolete, duplicated, unused, or misleading code over adding new layers beside it.
  - Replace bad local patterns when they are in the touched path; do not preserve them just because they already exist.
  - Every new type/function must either reduce existing complexity, remove ambiguity, enforce a real contract, or feed runtime/validation/diagnostics/tooling.
  - Report what code was removed, simplified, or deliberately left unchanged because it was outside scope.
- Add source-backed neural-readiness capability fields, not fake feature support:
  - Slang backend available
  - target/profile support
  - cooperative-vector/tensor-like feature state as unavailable/planned unless source proves support
  - required provider resource vocabulary
  - backend interop capability needed by external neural providers
- Add provider category support for `neural rendering`.
- Add diagnostics that explain why neural paths are unavailable, missing dependency, unsupported hardware, available, enabled, or runtime failed.
- Add a shader profile/capability matrix entry for future neural profiles, clearly marked unavailable/planned where not implemented.
- Do not add model loading, training, inference, tensor kernels, or neural assets in this prompt.

Acceptance criteria:
- Neural readiness is represented as capability/profile data, not as a hidden TODO.
- Slang/profile gating is visible to shader tooling and diagnostics.
- Provider resource contracts include neural-relevant inputs without forcing current providers to consume them.
- Unsupported neural features report unavailable/planned honestly.
- No new third-party SDK is introduced.

Validation:
- Run ShaderCompiler backend/target listing.
- Run provider diagnostics or readiness report if available.
- Run architecture boundary check.

Report:
- Changed files.
- Neural readiness capability fields.
- Unsupported/planned feature list.
- Commands run.
- Acceptance criteria pass/fail.
```

## Prompt 13 of 32 (M8): Cross-Feature Foundation MVP Proof Scenarios

```text
Task:
Add minimal proof scenarios that exercise PSO, ray tracing readiness, path tracing readiness, neural readiness, diagnostics, and baseline assets without implementing final features.

Goal:
Prove the foundations are real and wired together before starting feature-by-feature delivery.

Primary review sources:
- Docs/Architecture/00-Review/A_PrincipalRoleRequirements.md
- Docs/Architecture/00-Review/B_EngineArchitectureScorecard.md
- Docs/Architecture/00-Review/PrincipalRenderingReadiness.md

Supporting docs:
- Docs/Architecture/03-Validation/ValidationMatrix.md
- Docs/Architecture/03-Validation/PerformanceDiagnosticsPlan.md
- Docs/Architecture/02-Contracts/RendererFrameGraph.md

Production references to inspect first:
- Donut Samples for compact feature proof scenarios that are useful without becoming a showcase detour.
- Falcor sample graphs for scenario-driven renderer validation.
- RTXPT sample scenarios for path tracing/RT proof artifact expectations.
- NRD sample expectations for denoiser and temporal input validation.

Code/assets to inspect:
- Projects/*
- Engine/Renderer/Private/Diagnostics/**/*
- Tools/Launcher/SparkleLauncher/Private/Launch/Smoke/**/*
- Tools/Shaders/ShaderCompiler/**/*
- Tools/Cooking/**/*

Implementation requirements:
- Daily refactor principle for every touched file:
  - Leave the file simpler, clearer, or more enforceable than it was before the change.
  - Prefer removing obsolete, duplicated, unused, or misleading code over adding new layers beside it.
  - Replace bad local patterns when they are in the touched path; do not preserve them just because they already exist.
  - Every new type/function must either reduce existing complexity, remove ambiguity, enforce a real contract, or feed runtime/validation/diagnostics/tooling.
  - Report what code was removed, simplified, or deliberately left unchanged because it was outside scope.
- Add or reuse small scenarios named explicitly for:
  - PSO creation/cache baseline
  - RT capability/AS lifecycle baseline
  - path tracing readiness baseline
  - neural readiness capability baseline
  - descriptor pressure baseline
  - upload pressure baseline
- Emit diagnostics that show each scenario is either passed, unavailable, skipped, or failed with a reason.
- Keep assets small and source-control friendly.
- Do not add final path tracing/neural features here.
- Prefer one curated project/scenario set over many disconnected demos.

Acceptance criteria:
- Each foundation area has at least one scenario or artifact proving its readiness state.
- Missing hardware/SDK support is represented as unavailable/skipped, not as a generic failure.
- Artifacts include backend, adapter, capabilities, memory, descriptor, PSO, RT, provider/neural-readiness, and timing sections where supported.
- The scenario names are stable enough for launcher and CI use.

Validation:
- Run cook for the scenario project.
- Run D3D12 scenario smoke.
- Run Vulkan scenario smoke if available.
- Inspect generated artifacts.
- Run architecture boundary check.

Report:
- Changed files and assets.
- Scenario list.
- Artifact paths.
- Commands run.
- Acceptance criteria pass/fail.
```

## Prompt 14 of 32 (M9): Job System And Multithreading Foundation Last

```text
Task:
Add the job system and initial multithreaded scheduling foundation after the PSO, ray tracing, path tracing, neural-readiness, diagnostics, and scene snapshot MVPs are stable.

Goal:
Prepare SparkleEngine for high-performance compute/render workloads without using threads to hide unclear ownership.

Primary review sources:
- Docs/Architecture/00-Review/A_PrincipalRoleRequirements.md
- Docs/Architecture/00-Review/B_EngineArchitectureScorecard.md
- Docs/Architecture/00-Review/PrincipalRenderingReadiness.md

Supporting docs:
- Docs/Architecture/02-Contracts/RHIContract.md
- Docs/Architecture/02-Contracts/RendererFrameGraph.md
- Docs/Architecture/02-Contracts/RuntimeSceneData.md
- Docs/Architecture/02-Contracts/ApplicationLifecycle.md
- Docs/Architecture/03-Validation/PerformanceDiagnosticsPlan.md

Production references to inspect first:
- NVIDIA Donut threaded rendering/sample infrastructure for practical low-risk parallel renderer work.
- NVRHI parallel command list guidance for command recording and submission constraints.
- Falcor task/render graph scheduling for graph-aware multithreaded preparation patterns.

Prerequisites:
- Prompt 01 through Prompt 13 complete or intentionally deferred with documented reason.
- Scene snapshot ownership is clear.
- RHI command submission/fence ownership is clear.
- Renderer pass/resource ownership is clear.
- Diagnostics can separate CPU scheduling cost from GPU work.

Code to inspect:
- Engine/Core/**/*Thread*.h
- Engine/Core/**/*Job*.h
- Engine/Application/**/*.cpp
- Engine/Renderer/Private/FramePipeline/**/*.cpp
- Engine/Renderer/Private/FrameGraph/**/*.cpp
- Engine/GameFramework/Private/Scene/**/*.cpp
- CMake target structure for Core/Application/Renderer

Implementation requirements:
- Daily refactor principle for every touched file:
  - Leave the file simpler, clearer, or more enforceable than it was before the change.
  - Prefer removing obsolete, duplicated, unused, or misleading code over adding new layers beside it.
  - Replace bad local patterns when they are in the touched path; do not preserve them just because they already exist.
  - Every new type/function must either reduce existing complexity, remove ambiguity, enforce a real contract, or feed runtime/validation/diagnostics/tooling.
  - Report what code was removed, simplified, or deliberately left unchanged because it was outside scope.
- Add a small engine-owned job system in the lowest appropriate module, likely Core, with:
  - worker lifecycle
  - task submission
  - wait/group primitive
  - graceful shutdown
  - deterministic single-thread fallback
  - profiling scopes
- Define thread ownership:
  - GameFramework mutation thread or phase
  - snapshot capture phase
  - renderer build/prepare phase
  - RHI submission thread ownership
  - editor/tooling constraints
- Add only safe initial parallelization:
  - CPU-side scene snapshot preparation
  - renderer scene-data preparation
  - shader/package or asset-side preparation where ownership is already immutable
- Do not parallelize RHI command list recording until command allocator/list/thread-safety policy is explicit.
- Add diagnostics for job count, worker utilization, wait time, and frame scheduling cost.

Acceptance criteria:
- Job system lives in an appropriate low-level module and does not create dependency cycles.
- Single-thread fallback is available for debugging and deterministic validation.
- Initial parallel work operates on immutable snapshots or clearly owned data.
- RHI command submission ownership is not violated.
- CPU timing diagnostics show job scheduling cost.
- Architecture boundary check passes.

Validation:
- Build Core, Application, Renderer, Editor.
- Run single-thread mode.
- Run multithreaded mode.
- Run runtime/editor smoke.
- Run architecture boundary check.
- Compare CPU timing artifacts before/after where possible.

Report:
- Changed files.
- Job system API shape.
- Parallelized work items.
- Safety assumptions.
- Commands run.
- Acceptance criteria pass/fail.
```

## Prompt 15 of 32 (M0): Launcher Validation Category And Architecture Boundary Action

```text
Task:
Add a first-class launcher validation workflow category and expose the architecture boundary check as a reviewer-facing action.

Goal:
Make architecture boundary validation visible from SparkleLauncher instead of requiring reviewers to know the CMake target name.

Source docs to read first:
- Docs/Architecture/01-Boundaries/BoundaryRules.md
- Docs/Architecture/03-Validation/ValidationMatrix.md
- Docs/Architecture/04-Workflows/LauncherWorkflowReadiness.md
- Docs/Architecture/00-Review/ReviewerGuide.md

Production references to inspect first:
- NVIDIA Donut sample workflow shape for reviewer-facing validation/build actions.
- AMD Cauldron sample workflow conventions for build/run validation grouping.
- Existing SparkleLauncher workflow catalog and operation patterns; prefer extending these over inventing a second launcher workflow model.

Code to inspect:
- CMakeLists.txt
- CMake/ArchitectureBoundaryCheck.cmake
- Tools/Launcher/SparkleLauncher/Private/Gui/Models/LauncherWorkflowCatalog.cpp
- Tools/Launcher/SparkleLauncher/Private/Build/**/*.cpp
- Tools/Launcher/SparkleLauncher/Private/Maintenance/**/*.cpp
- Tools/Launcher/SparkleLauncher/Public/SparkleLauncher/*Operations.h
- Tools/Launcher/SparkleLauncher/Private/Shell/LauncherShell.cpp

Implementation requirements:
- Daily refactor principle for every touched file:
  - Leave the file simpler, clearer, or more enforceable than it was before the change.
  - Prefer removing obsolete, duplicated, unused, or misleading code over adding new layers beside it.
  - Replace bad local patterns when they are in the touched path; do not preserve them just because they already exist.
  - Every new type/function must either reduce existing complexity, remove ambiguity, enforce a real contract, or feed runtime/validation/diagnostics/tooling.
  - Report what code was removed, simplified, or deliberately left unchanged because it was outside scope.
- Add a launcher operation id for architecture boundary validation, for example `validation.architecture-boundaries`.
- Put it in a new or explicit `Validation` workflow category.
- Execute the existing CMake architecture check; do not reimplement the scanner in launcher code.
- Support dry-run output that shows the exact CMake command.
- Route failure output to the launcher operation log.
- Add recovery/help text that points to `Docs/Architecture/01-Boundaries/BoundaryRules.md`.
- Preserve the existing `architecture_boundary_check` CMake target.

Acceptance criteria:
- A reviewer can run the boundary check from launcher CLI.
- The GUI workflow catalog exposes a Validation category or equivalent validation grouping.
- The operation uses existing CMake architecture check behavior.
- Boundary violations fail the operation.
- The launcher log contains the invoked command and failure output.
- No architecture rules are weakened.
- `Docs/Architecture/03-Validation/ValidationMatrix.md` is updated from launcher-planned/implicit to source-backed for this action.

Validation:
- Run the new launcher dry-run operation.
- Run the new launcher operation if local toolchain permits.
- Run `cmake --build build --target architecture_boundary_check --config <profile>` or direct CMake script equivalent.
- Run `rg -n "validation.architecture|architecture_boundary_check|Validation" Tools/Launcher CMake Docs/Architecture`.

Report:
- Changed files.
- Commands run.
- Acceptance criteria pass/fail.
- Any follow-up needed for CI integration.
```

## Prompt 16 of 32 (M0): Launcher Readiness Report Artifact

```text
Task:
Add a reviewer-readable and machine-readable launcher readiness report.

Goal:
Make dependency, toolchain, generator, backend, and provider readiness exportable instead of requiring screenshots or manual UI inspection.

Source docs to read first:
- Docs/Architecture/04-Workflows/LauncherWorkflowReadiness.md
- Docs/Architecture/03-Validation/ValidationMatrix.md
- Docs/Architecture/02-Contracts/RendererProviderContract.md

Production references to inspect first:
- NVIDIA Donut configuration/readiness reporting patterns for device/tool setup evidence.
- AMD Cauldron configuration and runtime capability reporting patterns.
- Existing SparkleLauncher readiness/toolchain reports; extend current launcher reporting instead of creating a parallel report system.

Code to inspect:
- Tools/Launcher/SparkleLauncher/Public/SparkleLauncher/BuildWorkspaceOperations.h
- Tools/Launcher/SparkleLauncher/Private/Build/BuildToolchainDetection.cpp
- Tools/Launcher/SparkleLauncher/Private/Core/SourceDependencyState.cpp
- Tools/Launcher/SparkleLauncher/Private/Core/HostGraphicsCapabilities.cpp
- Tools/Launcher/SparkleLauncher/Private/Gui/Models/LauncherDependencyUiModel.cpp
- Tools/Launcher/SparkleLauncher/Private/Shell/LauncherShell.cpp

Implementation requirements:
- Daily refactor principle for every touched file:
  - Leave the file simpler, clearer, or more enforceable than it was before the change.
  - Prefer removing obsolete, duplicated, unused, or misleading code over adding new layers beside it.
  - Replace bad local patterns when they are in the touched path; do not preserve them just because they already exist.
  - Every new type/function must either reduce existing complexity, remove ambiguity, enforce a real contract, or feed runtime/validation/diagnostics/tooling.
  - Report what code was removed, simplified, or deliberately left unchanged because it was outside scope.
- Add a CLI-accessible report operation, for example `validation.readiness-report`.
- Emit `summary.json` and `summary.md` under `artifacts/diagnostics/launcher/readiness/<timestamp-or-run-id>/`.
- Include:
  - selected IDE
  - selected CMake generator/platform/toolset
  - resolved CMake/MSBuild/Ninja/Rider/Git/ClangFormat paths
  - Qt kit state
  - Vulkan SDK state
  - shader compiler SDK state
  - source dependency groups and enabled/ready/incomplete counts
  - host graphics vendor summary
  - NVIDIA dependency group state
  - backend-gated readiness notes
  - provider-gated readiness notes
- Use current source behavior as truth.
- Do not create fake readiness for unavailable SDKs.

Acceptance criteria:
- The operation can be dry-run and run from CLI.
- The report includes required, optional, hardware-gated, backend-gated, and provider-gated categories.
- Missing Vulkan SDK is represented as optional or required according to current feature/backend state.
- NVIDIA dependencies are represented as hardware-gated by detected host adapter state.
- JSON is stable enough for automation.
- Markdown is readable by a reviewer.

Validation:
- Run `SparkleLauncher --dry-run validation.readiness-report`.
- Run `SparkleLauncher --run validation.readiness-report`.
- Inspect generated JSON and Markdown.
- Run `SparkleLauncher --run toolchain.check` and compare major readiness facts.

Report:
- Changed files.
- Artifact path generated.
- Acceptance criteria pass/fail.
- Any unclear readiness categories.
```

## Prompt 17 of 32 (M2): Renderer Empty Frame Smoke Path

```text
Task:
Implement a stable renderer empty-frame validation path.

Goal:
Turn the planned `Renderer empty frame` validation row into a real reviewer command with artifact output.

Source docs to read first:
- Docs/Architecture/03-Validation/ValidationMatrix.md
- Docs/Architecture/03-Validation/PerformanceDiagnosticsPlan.md
- Docs/Architecture/02-Contracts/RendererFrameGraph.md
- Docs/Architecture/02-Contracts/ApplicationLifecycle.md

Production references to inspect first:
- NVIDIA Donut headless/basic samples for minimal renderer startup and frame validation.
- Falcor sample graph loading for small graph-backed smoke scenarios.
- AMD Cauldron sample startup path for backend-aware smoke behavior.

Code to inspect:
- Engine/Application/Private/Validation/**/* if present
- Engine/Renderer/Private/FramePipeline/**/*.cpp
- Engine/Renderer/Private/Diagnostics/**/*.h
- Tools/Launcher/SparkleLauncher/Private/Launch/Smoke/**/*.cpp
- Tools/Launcher/SparkleLauncher/Private/Launch/**/*.cpp
- Projects/Showcase or existing project smoke fixtures

Implementation requirements:
- Daily refactor principle for every touched file:
  - Leave the file simpler, clearer, or more enforceable than it was before the change.
  - Prefer removing obsolete, duplicated, unused, or misleading code over adding new layers beside it.
  - Replace bad local patterns when they are in the touched path; do not preserve them just because they already exist.
  - Every new type/function must either reduce existing complexity, remove ambiguity, enforce a real contract, or feed runtime/validation/diagnostics/tooling.
  - Report what code was removed, simplified, or deliberately left unchanged because it was outside scope.
- Add a stable smoke mode or launcher option that runs a minimal frame without requiring feature-heavy scene content.
- Prefer using existing smoke/session infrastructure.
- Emit artifacts:
  - capture metadata JSON
  - timing CSV or JSON
  - backend identity
  - frame count
  - pass timing summary where available
  - memory snapshot summary where available
- Support D3D12 and Vulkan where available.
- Treat missing Vulkan SDK as skipped/unavailable, not as D3D12 failure.
- Keep the path deterministic and short.

Acceptance criteria:
- `Renderer empty frame` in the validation matrix can be updated from `planned` to `existing` or `partial` with a real command.
- Command runs without requiring authored showcase content beyond a minimal project shell.
- Artifacts are written to a deterministic diagnostics path.
- Failure owner is clear: Application, Renderer, or RHI backend.
- No GameFramework to RHI dependency is introduced.

Validation:
- Run D3D12 empty-frame smoke.
- Run Vulkan empty-frame smoke if SDK/backend are available.
- Inspect generated metadata and timing artifacts.
- Run architecture boundary check.

Report:
- Changed files.
- Commands run.
- Artifact paths.
- Acceptance criteria pass/fail.
```

## Prompt 18 of 32 (M1): RHI Backend Startup/Shutdown Conformance Harness

```text
Task:
Add a backend-focused RHI startup/shutdown validation harness.

Goal:
Make D3D12 and Vulkan backend bring-up/teardown reviewable without relying only on full application smoke runs.

Source docs to read first:
- Docs/Architecture/02-Contracts/RHIContract.md
- Docs/Architecture/03-Validation/ValidationMatrix.md
- Docs/Architecture/03-Validation/PerformanceDiagnosticsPlan.md

Production references to inspect first:
- NVIDIA NRI backend startup and device creation flow for low-level backend conformance.
- NVRHI backend startup/device abstractions for backend-neutral validation expectations.
- NVIDIA Donut `DeviceManager` for practical app-hosted device startup/shutdown.
- AMD Cauldron device creation flow for D3D12/Vulkan startup parity.

Code to inspect:
- Engine/RHI/Public/**/*.h
- Engine/RHI/Private/D3D12/**/*.cpp
- Engine/RHI/Private/Vulkan/**/*.cpp
- Engine/Application/Private/Validation/**/* if present
- Tools/Launcher/SparkleLauncher/Private/Launch/Smoke/**/*.cpp

Implementation requirements:
- Daily refactor principle for every touched file:
  - Leave the file simpler, clearer, or more enforceable than it was before the change.
  - Prefer removing obsolete, duplicated, unused, or misleading code over adding new layers beside it.
  - Replace bad local patterns when they are in the touched path; do not preserve them just because they already exist.
  - Every new type/function must either reduce existing complexity, remove ambiguity, enforce a real contract, or feed runtime/validation/diagnostics/tooling.
  - Report what code was removed, simplified, or deliberately left unchanged because it was outside scope.
- Add a small validation executable, CMake target, or launcher operation that:
  - selects backend
  - creates RHI/device services
  - queries capabilities
  - captures diagnostics capabilities
  - captures initial memory snapshot if available
  - shuts down cleanly
- Emit JSON under `artifacts/diagnostics/rhi/startup/<backend>/<run-id>/summary.json`.
- Include backend, adapter, vendor/device IDs, shader binary format, descriptor model, queue support, present support, ray tracing capability, memory allocator backend, validation state, timestamp support, and interop support.
- Do not add backend-native dependencies outside RHI backend-private code.

Acceptance criteria:
- D3D12 startup/shutdown validation has a stable command.
- Vulkan startup/shutdown validation has a stable command when Vulkan SDK/backend are available.
- Missing Vulkan support reports unavailable/skipped clearly.
- Summary JSON maps to fields in `RhiCapabilities` and diagnostics services.
- Architecture boundary check passes.

Validation:
- Run D3D12 validation.
- Run Vulkan validation if available.
- Compare output against `RHIContract.md` backend parity table.
- Run CMake architecture boundary check.

Report:
- Changed files.
- Commands run.
- Artifact examples.
- Backend gaps discovered.
```

## Prompt 19 of 32 (M1): Backend Parity Review Report

```text
Task:
Create a backend parity report generator for D3D12 and Vulkan.

Goal:
Make backend capability differences visible as data rather than prose.

Source docs to read first:
- Docs/Architecture/02-Contracts/RHIContract.md
- Docs/Architecture/03-Validation/ValidationMatrix.md
- Docs/Architecture/00-Review/ReviewerGuide.md

Production references to inspect first:
- NVIDIA NRI capability reporting and backend-specific support surfaces.
- NVRHI backend capability/device feature reporting.
- AMD Cauldron D3D12/Vulkan parity and unavailable-feature handling.
- Falcor device feature reporting for reviewer-facing backend evidence.

Code to inspect:
- Engine/RHI/Public/**/*Capabilities*.h
- Engine/RHI/Public/**/*Diagnostics*.h
- Engine/RHI/Private/D3D12/**/*.cpp
- Engine/RHI/Private/Vulkan/**/*.cpp
- Tools/Launcher/SparkleLauncher/Private/Launch/Smoke/**/*.cpp

Implementation requirements:
- Daily refactor principle for every touched file:
  - Leave the file simpler, clearer, or more enforceable than it was before the change.
  - Prefer removing obsolete, duplicated, unused, or misleading code over adding new layers beside it.
  - Replace bad local patterns when they are in the touched path; do not preserve them just because they already exist.
  - Every new type/function must either reduce existing complexity, remove ambiguity, enforce a real contract, or feed runtime/validation/diagnostics/tooling.
  - Report what code was removed, simplified, or deliberately left unchanged because it was outside scope.
- Add a report command or validation mode that compares D3D12 and Vulkan capability snapshots.
- Emit:
  - `backend-parity.json`
  - `backend-parity.md`
- Include at minimum:
  - backend availability
  - shader binary format
  - descriptor model
  - queue support
  - upload/readback support
  - timestamp query support
  - presentation support
  - allocator backend
  - memory budget support
  - allocator JSON dump support
  - native interop support
  - ray tracing service support
  - capture service support
  - crash/live-object diagnostics capability
- Mark each item as `same`, `different`, `unsupported`, `unavailable`, or `needs source confirmation`.

Acceptance criteria:
- Report can be generated by one command.
- Markdown is readable in a reviewer packet.
- JSON is stable enough for automation.
- Vulkan absence is reported honestly.
- The report does not claim parity where the source does not prove it.
- `Docs/Architecture/03-Validation/BackendParityReview.md` is created or the README planned-doc entry is updated if the report makes that doc unnecessary.

Validation:
- Generate the report on the current machine.
- Run architecture boundary check.
- Run `rg -n "backend-parity|BackendParity" Engine Tools Docs`.

Report:
- Changed files.
- Artifact path.
- Acceptance criteria pass/fail.
- Capability mismatches found.
```

## Prompt 20 of 32 (M2): Unified Diagnostics Snapshot And Baseline Artifact Writer

```text
Task:
Add a unified diagnostics snapshot and artifact writer for reviewer baseline runs.

Goal:
Make backend, memory, provider, GPU timing, CPU timing, and shader/runtime facts available in one report bundle.

Source docs to read first:
- Docs/Architecture/03-Validation/PerformanceDiagnosticsPlan.md
- Docs/Architecture/03-Validation/ValidationMatrix.md
- Docs/Architecture/02-Contracts/RHIContract.md
- Docs/Architecture/02-Contracts/RendererFrameGraph.md
- Docs/Architecture/02-Contracts/RendererProviderContract.md

Production references to inspect first:
- Falcor diagnostics/profiling output for unified renderer/backend evidence.
- NVRHI validation diagnostics for backend and lifetime status.
- AMD Cauldron profiler/metrics patterns for memory/timing artifacts.

Code to inspect:
- Engine/RHI/Public/**/*Diagnostic*.h
- Engine/RHI/Public/**/*Memory*.h
- Engine/Renderer/Private/Diagnostics/**/*.h
- Engine/Renderer/Private/FrameGraph/**/*.cpp
- Engine/Editor/Private/**/*Profiler*.h
- Tools/Launcher/SparkleLauncher/Private/Launch/Smoke/**/*.cpp

Implementation requirements:
- Daily refactor principle for every touched file:
  - Leave the file simpler, clearer, or more enforceable than it was before the change.
  - Prefer removing obsolete, duplicated, unused, or misleading code over adding new layers beside it.
  - Replace bad local patterns when they are in the touched path; do not preserve them just because they already exist.
  - Every new type/function must either reduce existing complexity, remove ambiguity, enforce a real contract, or feed runtime/validation/diagnostics/tooling.
  - Report what code was removed, simplified, or deliberately left unchanged because it was outside scope.
- Add a typed diagnostics snapshot structure owned by the appropriate module boundary.
- Keep backend allocator truth separate from renderer summaries.
- Add an artifact writer that emits:
  - `summary.json`
  - `summary.md`
  - `allocator.json` when supported
  - `gpu-timings.json` or timing CSV
  - CPU trace or CPU timing summary if available
- Include metrics named in `PerformanceDiagnosticsPlan.md`:
  - active backend
  - adapter/vendor/device
  - API version or feature level when available
  - validation state
  - enabled providers
  - memory budget
  - memory usage
  - allocation counts
  - pass GPU timings
  - CPU frame timings
  - upload pressure where available
- Mark missing metrics explicitly as unavailable or planned.

Acceptance criteria:
- A baseline run emits a single folder with reviewer-readable and machine-readable summaries.
- Data ownership is preserved: RHI allocator data, renderer summaries, editor/tool presentation remain distinct.
- The output can be attached to a portfolio review without screenshots.
- Existing smoke diagnostics are reused where practical.
- No backend-native dependencies leak upward.

Validation:
- Run one smoke/baseline command that writes the artifact bundle.
- Inspect JSON and Markdown.
- Run architecture boundary check.

Report:
- Changed files.
- Artifact path.
- Acceptance criteria pass/fail.
- Metrics still unavailable.
```

## Prompt 21 of 32 (M8): Descriptor Pressure And Upload Pressure Diagnostics

```text
Task:
Add first-class descriptor pressure and upload pressure diagnostics.

Goal:
Close two planned/partial metrics from the diagnostics plan before heavier rendering features arrive.

Source docs to read first:
- Docs/Architecture/03-Validation/PerformanceDiagnosticsPlan.md
- Docs/Architecture/02-Contracts/RHIContract.md
- Docs/Architecture/02-Contracts/RendererFrameGraph.md

Production references to inspect first:
- NVRHI descriptor/resource lifetime patterns for descriptor pressure accounting.
- AMD Cauldron descriptor/resource pool patterns for cross-backend usage reporting.
- NVIDIA NRI descriptor abstractions for low-level descriptor ownership and capability shape.

Code to inspect:
- Engine/RHI/Public/**/*Descriptor*.h
- Engine/RHI/Private/D3D12/**/*Descriptor*.cpp
- Engine/RHI/Private/Vulkan/**/*Descriptor*.cpp
- Engine/RHI/Public/**/*Memory*.h
- Engine/Renderer/Private/Diagnostics/**/*.h
- Engine/Renderer/Private/Textures/**/*.cpp
- Engine/Renderer/Private/SceneData/**/*.cpp

Implementation requirements:
- Daily refactor principle for every touched file:
  - Leave the file simpler, clearer, or more enforceable than it was before the change.
  - Prefer removing obsolete, duplicated, unused, or misleading code over adding new layers beside it.
  - Replace bad local patterns when they are in the touched path; do not preserve them just because they already exist.
  - Every new type/function must either reduce existing complexity, remove ambiguity, enforce a real contract, or feed runtime/validation/diagnostics/tooling.
  - Report what code was removed, simplified, or deliberately left unchanged because it was outside scope.
- Add descriptor occupancy counters or snapshots at the RHI descriptor service layer.
- Expose limits and current usage separately.
- Add warning thresholds without failing normal runs.
- Add upload pressure summary derived from upload memory bytes, upload allocation counts, and delayed destruction where available.
- Feed both metrics into the unified diagnostics artifact writer.
- Keep units explicit:
  - descriptor counts
  - bytes
  - ratios
  - frame-local or rolling status

Acceptance criteria:
- Descriptor pressure metric moves from `planned` to `existing` or `partial`.
- Upload pressure metric moves from `partial` toward `existing`.
- D3D12 and Vulkan coverage is reported honestly.
- Metrics are visible in text artifacts.
- No renderer pass reads backend-native descriptor heaps directly.

Validation:
- Run diagnostics/baseline command.
- Confirm descriptor and upload sections appear in JSON/Markdown.
- Run architecture boundary check.

Report:
- Changed files.
- Commands run.
- Backend coverage.
- Remaining metric gaps.
```

## Prompt 22 of 32 (M3): Pipeline Cache Stats And Shader Package Load Timing

```text
Task:
Add pipeline cache statistics and shader package load/reload timing.

Goal:
Make shader/pipeline runtime cost reviewable, not only cook-time behavior.

Source docs to read first:
- Docs/Architecture/02-Contracts/ShaderPipeline.md
- Docs/Architecture/03-Validation/PerformanceDiagnosticsPlan.md
- Docs/Architecture/03-Validation/ValidationMatrix.md

Production references to inspect first:
- NVRHI pipeline APIs for cacheable pipeline state and backend-neutral pipeline handles.
- AMD Cauldron shader/PSO handling for runtime timing and shader-package compatibility.
- NVIDIA Donut shader compilation/runtime usage for practical shader package load evidence.

Code to inspect:
- Tools/Shaders/ShaderCompiler/Private/Cooking/**/*.cpp
- Engine/Renderer/ShaderRegistrations/**/*.cpp
- Engine/RHI/Public/**/*Pipeline*.h
- Engine/RHI/Public/**/*Shader*.h
- Engine/Renderer/Private/**/*Shader*.cpp
- Engine/Editor/Private/**/*Shader*.cpp

Implementation requirements:
- Daily refactor principle for every touched file:
  - Leave the file simpler, clearer, or more enforceable than it was before the change.
  - Prefer removing obsolete, duplicated, unused, or misleading code over adding new layers beside it.
  - Replace bad local patterns when they are in the touched path; do not preserve them just because they already exist.
  - Every new type/function must either reduce existing complexity, remove ambiguity, enforce a real contract, or feed runtime/validation/diagnostics/tooling.
  - Report what code was removed, simplified, or deliberately left unchanged because it was outside scope.
- Find the runtime shader package load/reload path.
- Add timing scopes around package load, reflection/metadata load, pipeline creation, and reload replacement where applicable.
- Add pipeline cache counters if a cache exists; if no cache exists, add an explicit `pipeline cache not implemented` field rather than fake stats.
- Include shader package load timing in diagnostics artifact output.
- Include cache hit/miss data from ShaderCompiler cook output where available.
- Keep cook-time and runtime-load-time metrics separate.

Acceptance criteria:
- `shader package load timing` becomes source-backed in diagnostics output.
- `pipeline cache stats` reports real stats or an explicit unavailable state.
- Shader inspection commands still work.
- No shader ABI behavior is changed accidentally.

Validation:
- Run `ShaderCompiler list-shaders --validate`.
- Run a shader cook cache miss and hit scenario if practical.
- Run runtime/editor load path and inspect diagnostics output.
- Run architecture boundary check.

Report:
- Changed files.
- Commands run.
- Timing fields added.
- Any unavailable cache stats explained.
```

## Prompt 23 of 32 (M3): Shader Golden Reflection And Package Regression Corpus

```text
Task:
Add shader compiler golden/reflection/package regression coverage.

Goal:
Turn the strong shader pipeline into visible, repeatable portfolio evidence.

Source docs to read first:
- Docs/Architecture/02-Contracts/ShaderPipeline.md
- Docs/Architecture/03-Validation/ValidationMatrix.md
- Docs/Architecture/00-Review/B_EngineArchitectureScorecard.md

Production references to inspect first:
- NVIDIA Donut shader compiler usage for concrete shader validation and packaging patterns.
- NVRHI shader reflection/pipeline expectations for runtime ABI compatibility.
- Slang and DXC reference usage in public shader tooling for profile/target/reflection validation.

Code to inspect:
- Tools/Shaders/ShaderCompiler/Private/Cli/**/*.cpp
- Tools/Shaders/ShaderCompiler/Private/Contracts/**/*.cpp
- Tools/Shaders/ShaderCompiler/Private/Cooking/**/*.cpp
- Tools/Shaders/ShaderContracts/**/*
- Engine/Renderer/ShaderRegistrations/**/*
- Tests or CMake test conventions if present

Implementation requirements:
- Daily refactor principle for every touched file:
  - Leave the file simpler, clearer, or more enforceable than it was before the change.
  - Prefer removing obsolete, duplicated, unused, or misleading code over adding new layers beside it.
  - Replace bad local patterns when they are in the touched path; do not preserve them just because they already exist.
  - Every new type/function must either reduce existing complexity, remove ambiguity, enforce a real contract, or feed runtime/validation/diagnostics/tooling.
  - Report what code was removed, simplified, or deliberately left unchanged because it was outside scope.
- Add a regression test or validation command that covers:
  - `list-shaders --validate`
  - reflection output for representative shaders
  - DXIL target cook where supported
  - SPIR-V target cook where supported
  - package inspection output
  - include closure/cache key behavior
- Store small golden files only when stable and useful.
- Avoid committing machine-specific binary paths.
- Add canonical reviewer command examples with real package ids discovered from source.
- Emit test artifacts under a predictable diagnostics/test path.

Acceptance criteria:
- Shader validation can be run by a reviewer or CI command.
- At least one representative package is cooked and inspected.
- Reflection contract failures fail the validation.
- Golden artifacts are deterministic or documented as generated outputs.
- `ValidationMatrix.md` shader rows remain accurate.

Validation:
- Run the new shader regression command/test.
- Run `ShaderCompiler inspect-shader <real-id>`.
- Run `ShaderCompiler inspect-package <generated-package>`.
- Run format check for touched source.

Report:
- Changed files.
- Commands run.
- Representative shader/package ids used.
- Acceptance criteria pass/fail.
```

## Prompt 24 of 32 (M5): Provider Capability State Mapping

```text
Task:
Add a shared provider capability-state mapping layer.

Goal:
Make provider diagnostics, launcher readiness, and renderer runtime reporting converge on the six architectural states.

Source docs to read first:
- Docs/Architecture/02-Contracts/RendererProviderContract.md
- Docs/Architecture/04-Workflows/LauncherWorkflowReadiness.md
- Docs/Architecture/01-Boundaries/BoundaryRules.md

Production references to inspect first:
- NVIDIA Streamline capability checks and runtime state handling.
- FidelityFX SDK feature availability reporting and provider state shape.
- AMD Cauldron SDK backend/provider integration patterns.

Code to inspect:
- Engine/Renderer/Private/Providers/RenderProviderModel.h if present
- Engine/Renderer/Private/Providers/RenderProviderModel.cpp if present
- Engine/Renderer/Private/Upscaling/UpscalerProvider.h
- Engine/Renderer/Private/Upscaling/**/*.cpp
- Engine/Renderer/Private/Upscaling/NvidiaDlss/**/*.cpp
- Engine/Renderer/Private/Diagnostics/**/*.h
- Tools/Launcher/SparkleLauncher/Private/Core/SourceDependencyState.cpp
- Tools/Launcher/SparkleLauncher/Private/Core/HostGraphicsCapabilities.cpp

Implementation requirements:
- Daily refactor principle for every touched file:
  - Leave the file simpler, clearer, or more enforceable than it was before the change.
  - Prefer removing obsolete, duplicated, unused, or misleading code over adding new layers beside it.
  - Replace bad local patterns when they are in the touched path; do not preserve them just because they already exist.
  - Every new type/function must either reduce existing complexity, remove ambiguity, enforce a real contract, or feed runtime/validation/diagnostics/tooling.
  - Report what code was removed, simplified, or deliberately left unchanged because it was outside scope.
- Add an engine-facing provider capability state enum or adapter that maps to exactly:
  - `unavailable`
  - `missing dependency`
  - `unsupported hardware`
  - `available`
  - `enabled`
  - `runtime failed`
- Preserve existing provider-specific enums if they carry extra detail.
- Keep provider categories scoped to staged code:
  - `upscaler` is operational in this prompt set
  - `denoiser` is staged for DLSS Ray Reconstruction / NRD-style NVIDIA paths and FidelityFX-style AMD alternatives
  - `frame generation` is staged for DLSS Frame Generation and FSR Frame Generation
  - do not add `neural rendering` here; Prompt 12 owns that category when neural readiness is source-backed
  - do not add `ray tracing extension` here unless a concrete SDK integration requires it
- Add mapping functions and reviewer-readable reason strings.
- Add diagnostics output for requested provider, active provider, mapped state, failure domain, reason, runtime version when known, and fallback reason.
- Add launcher/readiness report mapping where source dependency and hardware states imply provider states.
- Do not add a new provider implementation.

Acceptance criteria:
- The six state strings appear exactly and consistently in diagnostics/report output.
- DLSS/Streamline maps into the shared state model without becoming the architecture.
- The shared category model does not include unstaged categories.
- Missing SDK, unsupported hardware, runtime failure, available, and enabled are distinguishable.
- Provider fallback remains deterministic.
- Architecture boundary check passes.

Validation:
- Run provider diagnostics on current hardware.
- Run launcher readiness report.
- Search for state strings and mapping function.
- Run architecture boundary check.

Report:
- Changed files.
- Commands run.
- State mapping table.
- Acceptance criteria pass/fail.
```

## Prompt 25 of 32 (M5): Provider Resource Contract Validation

```text
Task:
Strengthen renderer provider resource contract validation.

Goal:
Ensure future upscalers, denoisers, frame generation, and neural paths can depend on explicit renderer-owned inputs.

Source docs to read first:
- Docs/Architecture/02-Contracts/RendererProviderContract.md
- Docs/Architecture/02-Contracts/RendererFrameGraph.md

Production references to inspect first:
- NVIDIA Streamline resource tagging/contracts for provider input validation.
- NRD input contracts for denoiser resource requirements and temporal inputs.
- FidelityFX SDK input resource contracts for provider-specific required/optional resources.

Code to inspect:
- Engine/Renderer/Private/Providers/RenderProviderModel.h if present
- Engine/Renderer/Private/Providers/RenderProviderModel.cpp if present
- Engine/Renderer/Private/Upscaling/UpscalerInputContract.h
- Engine/Renderer/Private/Upscaling/UpscalerInputContractBuilder.cpp
- Engine/Renderer/Private/Upscaling/**/*.cpp
- Engine/Renderer/Private/Frame/Targets/**/*.h
- Engine/Renderer/Private/Temporal/**/*.h
- Engine/Renderer/Private/Diagnostics/**/*.h

Implementation requirements:
- Daily refactor principle for every touched file:
  - Leave the file simpler, clearer, or more enforceable than it was before the change.
  - Prefer removing obsolete, duplicated, unused, or misleading code over adding new layers beside it.
  - Replace bad local patterns when they are in the touched path; do not preserve them just because they already exist.
  - Every new type/function must either reduce existing complexity, remove ambiguity, enforce a real contract, or feed runtime/validation/diagnostics/tooling.
  - Report what code was removed, simplified, or deliberately left unchanged because it was outside scope.
- Audit current upscaler input contract for:
  - color
  - depth
  - motion vectors
  - exposure
  - normals
  - history
  - jitter
  - camera matrices
  - frame index
- Add explicit optional/required flags and validation reasons where missing.
- Do not force every current provider to require every resource.
- Add diagnostics showing which resources were present, optional, required, or missing.
- Add room for normals/exposure/history to be validated provider-by-provider.
- Add staged resource-contract profiles for planned provider families without implementing the providers:
  - upscaler profile: color, depth, motion vectors, history, jitter, camera matrices, frame index; exposure optional; normals optional
  - frame-generation profile: color, depth, motion vectors, history, jitter, camera matrices, frame index; exposure optional; normals optional until a concrete provider requires them
  - denoiser profile: color or signal input, depth, normals, history, jitter, camera matrices, frame index; motion vectors optional/required per denoiser; exposure optional
- Do not make every resource globally required just because one future provider may need it.
- Avoid hidden globals or provider-side discovery of renderer resources.

Acceptance criteria:
- Provider input diagnostics list all required resource vocabulary entries.
- Resource-contract profiles exist for staged upscaler, frame-generation, and denoiser families, even if only the upscaler profile is executed today.
- Missing required provider resources produce clear validation failure or fallback reason.
- Optional resources are explicitly optional, not silently ignored.
- Existing DLSS behavior remains functionally equivalent unless it previously relied on invalid hidden state.
- No backend-native access is added.

Validation:
- Run upscaler/provider smoke path if available.
- Run renderer empty-frame smoke with provider disabled/fallback.
- Inspect diagnostics artifacts.
- Run architecture boundary check.

Report:
- Changed files.
- Commands run.
- Resource contract table before/after.
- Acceptance criteria pass/fail.
```

## Prompt 26 of 32 (M2): Frame Graph Contract Diagnostics Hardening

```text
Task:
Harden frame graph resource contract diagnostics for pass authors.

Goal:
Make renderer extension safer by failing or warning clearly when passes use undeclared or invalid resources.

Source docs to read first:
- Docs/Architecture/02-Contracts/RendererFrameGraph.md
- Docs/Architecture/03-Validation/ValidationMatrix.md

Production references to inspect first:
- Falcor RenderGraph validation for pass/resource diagnostics.
- NVRHI resource state tracking for barrier and resource-state evidence.
- NVIDIA Donut pass conventions for sample-framework pass authoring checks.

Code to inspect:
- Engine/Renderer/Private/FrameGraph/**/*.h
- Engine/Renderer/Private/FrameGraph/**/*.cpp
- Engine/Renderer/Private/Passes/**/*.h
- Engine/Renderer/Private/FramePipeline/**/*.cpp
- Engine/Renderer/Private/FrameGraph/Diagnostics/**/*.h

Implementation requirements:
- Daily refactor principle for every touched file:
  - Leave the file simpler, clearer, or more enforceable than it was before the change.
  - Prefer removing obsolete, duplicated, unused, or misleading code over adding new layers beside it.
  - Replace bad local patterns when they are in the touched path; do not preserve them just because they already exist.
  - Every new type/function must either reduce existing complexity, remove ambiguity, enforce a real contract, or feed runtime/validation/diagnostics/tooling.
  - Report what code was removed, simplified, or deliberately left unchanged because it was outside scope.
- Identify existing frame graph diagnostics for undeclared resources, invalid bindings, unresolved barriers, and invalid viewport products.
- Add missing diagnostic counters or messages where the contract expects them.
- Surface diagnostic summary in smoke/baseline artifacts.
- Keep pass authoring flow unchanged unless needed for validation.
- Avoid broad refactors of frame graph scheduling.

Acceptance criteria:
- A pass with missing resource declaration can be detected by existing or new validation.
- Unresolved barrier warning count is visible in diagnostics output.
- Viewport product failures explain the missing product/resource.
- Diagnostics use stable pass and resource names.
- No backend-native dependency is introduced into Renderer.

Validation:
- Run renderer empty-frame smoke.
- Run a normal project smoke.
- If tests exist, add a small unit/validation case for bad frame graph setup.
- Run architecture boundary check.

Report:
- Changed files.
- Commands run.
- Diagnostics added.
- Acceptance criteria pass/fail.
```

## Prompt 27 of 32 (M4): Runtime Scene Snapshot And Invalidation Validation

```text
Task:
Add validation coverage for GameFramework-to-Renderer scene snapshot ownership and invalidation.

Goal:
Keep future renderer features from coupling GameFramework to Renderer/RHI.

Source docs to read first:
- Docs/Architecture/02-Contracts/RuntimeSceneData.md
- Docs/Architecture/02-Contracts/RendererFrameGraph.md
- Docs/Architecture/01-Boundaries/BoundaryRules.md

Production references to inspect first:
- NVIDIA Donut scene graph for scene-to-renderer data translation patterns.
- AMD Cauldron content systems for asset/runtime data invalidation behavior.
- Falcor scene invalidation patterns for renderer snapshot refresh and diagnostics.

Code to inspect:
- Engine/GameFramework/CMakeLists.txt
- Engine/GameFramework/Public/Scene/**/*.h
- Engine/GameFramework/Private/Scene/**/*.cpp
- Engine/Renderer/Private/SceneData/**/*.h
- Engine/Renderer/Private/SceneData/**/*.cpp
- CMake/ArchitectureBoundaryCheck.cmake

Implementation requirements:
- Daily refactor principle for every touched file:
  - Leave the file simpler, clearer, or more enforceable than it was before the change.
  - Prefer removing obsolete, duplicated, unused, or misleading code over adding new layers beside it.
  - Replace bad local patterns when they are in the touched path; do not preserve them just because they already exist.
  - Every new type/function must either reduce existing complexity, remove ambiguity, enforce a real contract, or feed runtime/validation/diagnostics/tooling.
  - Report what code was removed, simplified, or deliberately left unchanged because it was outside scope.
- Add tests, validation, or architecture-check coverage that confirms GameFramework does not depend on Renderer/RHI.
- Add source-level validation or debug diagnostics for scene snapshot capture and renderer translation if appropriate.
- Document or enforce invalidation on:
  - level unload
  - level change
  - asset append
  - scene reset
- Avoid adding runtime data features unless required to prove the boundary.

Acceptance criteria:
- GameFramework remains linked only to Core/Platform-level dependencies.
- Snapshot-to-renderer translation ownership is visible in code or diagnostics.
- Invalidation events are named and testable or logged.
- Architecture boundary check covers the important dependency direction.
- Runtime scene changes do not require RHI types in GameFramework.

Validation:
- Run architecture boundary check.
- Run relevant GameFramework/Renderer tests if present.
- Run runtime project smoke.

Report:
- Changed files.
- Commands run.
- Acceptance criteria pass/fail.
- Any lifetime/threading behavior that still needs confirmation.
```

## Prompt 28 of 32 (M0): Application Error Taxonomy And Structured Failure Reporting

```text
Task:
Add structured lifecycle failure categories across launcher/application validation paths.

Goal:
Replace scattered string-only failure interpretation with reviewer-readable categories aligned with the application lifecycle contract.

Source docs to read first:
- Docs/Architecture/02-Contracts/ApplicationLifecycle.md
- Docs/Architecture/04-Workflows/LauncherWorkflowReadiness.md
- Docs/Architecture/03-Validation/ValidationMatrix.md

Production references to inspect first:
- NVIDIA Donut app failure/startup paths for backend and project-load failure separation.
- AMD Cauldron app/sample failure paths for dependency/backend readiness behavior.
- Existing SparkleLauncher recovery model; improve it instead of replacing it with unrelated failure routing.

Code to inspect:
- Engine/Application/Public/**/*.h
- Engine/Application/Private/**/*.cpp
- Tools/Launcher/SparkleLauncher/Private/Gui/Models/LauncherRecoveryUiModel.cpp
- Tools/Launcher/SparkleLauncher/Private/Gui/Shell/LauncherMainWindowOperations.cpp
- Tools/Launcher/SparkleLauncher/Private/Launch/Smoke/**/*.cpp

Implementation requirements:
- Daily refactor principle for every touched file:
  - Leave the file simpler, clearer, or more enforceable than it was before the change.
  - Prefer removing obsolete, duplicated, unused, or misleading code over adding new layers beside it.
  - Replace bad local patterns when they are in the touched path; do not preserve them just because they already exist.
  - Every new type/function must either reduce existing complexity, remove ambiguity, enforce a real contract, or feed runtime/validation/diagnostics/tooling.
  - Report what code was removed, simplified, or deliberately left unchanged because it was outside scope.
- Introduce typed or structured categories for:
  - missing SDK
  - missing source dependency
  - unsupported hardware
  - invalid project
  - shader cook failure
  - backend creation failure
  - runtime validation failure
  - editor-only failure
- Preserve existing user-facing detail strings.
- Add category fields to validation/readiness artifacts.
- Update recovery routing where a category clearly maps to a prerequisite action.
- Avoid moving editor-only code into runtime targets.

Acceptance criteria:
- Failure artifacts include one of the taxonomy categories where applicable.
- Existing recovery messages still appear.
- Backend creation failure has a clear category even if exact backend reason is nested.
- Runtime-only builds remain free of editor-only validation/recook code.
- Docs are updated only if code behavior changes the taxonomy.

Validation:
- Run launcher dry-run paths.
- Run a validation path with a controlled missing prerequisite if safe.
- Run architecture boundary check.
- Build affected launcher/application targets.

Report:
- Changed files.
- Commands run.
- Category mapping summary.
- Acceptance criteria pass/fail.
```

## Prompt 29 of 32 (M4): Curated Portfolio Baseline Assets And Scenarios

```text
Task:
Add small curated project assets/scenarios for performance and validation baselines.

Goal:
Give reviewers repeatable content that exercises the baseline scenarios in the diagnostics plan without depending on heavy external assets.

Source docs to read first:
- Docs/Architecture/03-Validation/PerformanceDiagnosticsPlan.md
- Docs/Architecture/03-Validation/ValidationMatrix.md
- Docs/Architecture/02-Contracts/RuntimeSceneData.md

Production references to inspect first:
- Donut Samples for small stable reviewer scenarios and source-control-friendly sample content.
- AMD Cauldron samples for compact validation scenes and sample metadata conventions.
- Falcor sample scenes for scenario selection and diagnostics-driven sample validation.

Project/assets to inspect:
- Projects/*
- Tools/Cooking/**/*
- existing cooked asset manifests or sample scenes
- existing Showcase project conventions

Implementation requirements:
- Daily refactor principle for every touched file:
  - Leave the file simpler, clearer, or more enforceable than it was before the change.
  - Prefer removing obsolete, duplicated, unused, or misleading code over adding new layers beside it.
  - Replace bad local patterns when they are in the touched path; do not preserve them just because they already exist.
  - Every new type/function must either reduce existing complexity, remove ambiguity, enforce a real contract, or feed runtime/validation/diagnostics/tooling.
  - Report what code was removed, simplified, or deliberately left unchanged because it was outside scope.
- Add or reuse a small reviewer project or scenario set with:
  - empty frame
  - one static mesh/material
  - many materials
  - descriptor pressure
  - upload pressure
  - shader compile cache hit/miss scenario
  - backend startup/shutdown scenario
- Keep assets small and source-controllable.
- Prefer procedural or minimal authored assets when possible.
- Add cook metadata/manifests according to existing project conventions.
- Do not add large binary assets without a clear reason.
- Ensure baseline names are stable for launcher/smoke commands.

Acceptance criteria:
- Each baseline scenario has a stable id/name.
- Cook all can prepare the scenario assets.
- Runtime/editor smoke can select at least the empty frame and one static mesh scenario.
- Diagnostics artifact writer can label the scenario in output.
- Asset additions do not bloat the repository unnecessarily.

Validation:
- Run `SparkleLauncher --run cook.project` or targeted cook for the reviewer project.
- Run empty frame and one mesh smoke.
- Run shader cache hit/miss scenario if implemented.
- Inspect generated artifacts.

Report:
- Changed files and asset files.
- Asset size summary.
- Commands run.
- Acceptance criteria pass/fail.
```

## Prompt 30 of 32 (M8): Editor Principal Review Dashboard

```text
Task:
Add an editor-facing principal review diagnostics dashboard.

Goal:
Make backend, memory, timing, provider, shader, and asset diagnostics visible in one reviewer-friendly place.

Source docs to read first:
- Docs/Architecture/03-Validation/PerformanceDiagnosticsPlan.md
- Docs/Architecture/00-Review/B_EngineArchitectureScorecard.md
- Docs/Architecture/00-Review/ReviewerGuide.md

Production references to inspect first:
- Falcor/Mogwai inspection workflow for compact rendering diagnostics review.
- AMD Cauldron sample UI for feature/backend diagnostics presentation.
- Existing Sparkle editor diagnostics UI; extend existing panels/data providers instead of creating a disconnected showcase panel.

Code to inspect:
- Engine/Editor/Private/**/*Profiler*.h
- Engine/Editor/Private/**/*Profiler*.cpp
- Engine/Editor/Private/**/*Rendering*.h
- Engine/Editor/Private/**/*Rendering*.cpp
- Engine/Editor/Private/**/*Shaders*.h
- Engine/Editor/Private/**/*Meshes*.h
- Engine/Editor/Private/**/*Textures*.h
- Engine/Renderer/Private/Diagnostics/**/*.h

Implementation requirements:
- Daily refactor principle for every touched file:
  - Leave the file simpler, clearer, or more enforceable than it was before the change.
  - Prefer removing obsolete, duplicated, unused, or misleading code over adding new layers beside it.
  - Replace bad local patterns when they are in the touched path; do not preserve them just because they already exist.
  - Every new type/function must either reduce existing complexity, remove ambiguity, enforce a real contract, or feed runtime/validation/diagnostics/tooling.
  - Report what code was removed, simplified, or deliberately left unchanged because it was outside scope.
- Reuse existing panels and diagnostics providers where possible.
- Add a compact dashboard view that shows:
  - active backend
  - adapter/vendor/device
  - validation state
  - enabled/requested providers
  - memory budget and usage
  - descriptor pressure if available
  - pass GPU timing summary
  - CPU frame timing summary
  - shader package/load state
  - upload pressure if available
- Add an export button or command if editor patterns already support it; otherwise point to the artifact writer.
- Do not make Editor own RHI allocator truth or renderer summaries.

Acceptance criteria:
- A reviewer can find the dashboard from the editor UI.
- The dashboard uses existing diagnostics data providers or the new unified snapshot.
- Missing metrics are displayed as unavailable, not fabricated.
- UI does not introduce runtime target dependencies.
- Text does not overflow obvious compact panels.

Validation:
- Build editor.
- Launch editor.
- Inspect dashboard on a normal project.
- Run architecture boundary check.

Report:
- Changed files.
- Screenshots or concise UI description.
- Commands run.
- Acceptance criteria pass/fail.
```

## Prompt 31 of 32 (M10): Portfolio Review Run Aggregator

```text
Task:
Add a repeatable portfolio review run that executes the main reviewer path and collects artifacts.

Goal:
Let a reviewer or hiring panel run one command and receive a coherent evidence bundle.

Source docs to read first:
- Docs/Architecture/00-Review/ReviewerGuide.md
- Docs/Architecture/03-Validation/ValidationMatrix.md
- Docs/Architecture/04-Workflows/LauncherWorkflowReadiness.md
- Docs/Architecture/03-Validation/PerformanceDiagnosticsPlan.md

Production references to inspect first:
- NVIDIA Donut sample runner/workflow expectations for repeatable reviewer paths.
- AMD Cauldron sample runner/workflow conventions for build/run/capture organization.
- Existing SparkleLauncher operation orchestration; aggregate current operations rather than duplicating them.

Code to inspect:
- Tools/Launcher/SparkleLauncher/Private/Shell/LauncherShell.cpp
- Tools/Launcher/SparkleLauncher/Private/Build/**/*.cpp
- Tools/Launcher/SparkleLauncher/Private/Cook/**/*.cpp
- Tools/Launcher/SparkleLauncher/Private/Launch/Smoke/**/*.cpp
- Tools/Shaders/ShaderCompiler/Private/Cli/**/*.cpp

Implementation requirements:
- Daily refactor principle for every touched file:
  - Leave the file simpler, clearer, or more enforceable than it was before the change.
  - Prefer removing obsolete, duplicated, unused, or misleading code over adding new layers beside it.
  - Replace bad local patterns when they are in the touched path; do not preserve them just because they already exist.
  - Every new type/function must either reduce existing complexity, remove ambiguity, enforce a real contract, or feed runtime/validation/diagnostics/tooling.
  - Report what code was removed, simplified, or deliberately left unchanged because it was outside scope.
- Add a portfolio review operation, for example `validation.portfolio-review`.
- The operation should orchestrate or clearly dry-run:
  - architecture boundary check
  - toolchain/readiness report
  - generate build files
  - build all
  - cook all
  - shader validation/inspection
  - D3D12 smoke
  - Vulkan smoke if available
  - diagnostics baseline artifact collection
- Support `--dry-run`.
- Allow skipping expensive build/cook phases only through explicit options.
- Emit a final `portfolio-review.md` and `portfolio-review.json` manifest with pass/fail/skipped statuses and artifact paths.

Acceptance criteria:
- One command describes or runs the reviewer path.
- Each sub-step has a stable status: passed, failed, skipped, unavailable.
- Failure owner and log path are recorded.
- Artifacts are collected under one root folder.
- Vulkan absence is unavailable/skipped, not a failure of D3D12.
- The operation does not hide failures behind a green summary.

Validation:
- Run dry-run portfolio review.
- Run at least a partial local portfolio review path that does not require unavailable SDKs.
- Inspect manifest.
- Run architecture boundary check independently.

Report:
- Changed files.
- Commands run.
- Manifest path.
- Acceptance criteria pass/fail.
```

## Prompt 32 of 32 (M10): Documentation Status Refresh After Code Implementation

```text
Task:
Refresh architecture docs after the implementation prompts land.

Goal:
Keep docs honest so the reviewer sees current behavior, not stale plans.

Source docs to read first:
- Docs/Architecture/README.md
- Docs/Architecture/00-Review/ReviewerGuide.md
- Docs/Architecture/00-Review/B_EngineArchitectureScorecard.md
- Docs/Architecture/03-Validation/ValidationMatrix.md
- Docs/Architecture/03-Validation/PerformanceDiagnosticsPlan.md
- Docs/Architecture/04-Workflows/LauncherWorkflowReadiness.md

Production references to inspect first:
- The implemented SparkleEngine code and artifacts produced by Prompts 01 through 31.
- The production references named in each completed prompt's report.
- Do not update statuses from external reference repos alone; update only when SparkleEngine source and validation artifacts prove the status.

Implementation requirements:
- Daily refactor principle for every touched file:
  - Leave the file simpler, clearer, or more enforceable than it was before the change.
  - Prefer removing obsolete, duplicated, unused, or misleading code over adding new layers beside it.
  - Replace bad local patterns when they are in the touched path; do not preserve them just because they already exist.
  - Every new type/function must either reduce existing complexity, remove ambiguity, enforce a real contract, or feed runtime/validation/diagnostics/tooling.
  - Report what code was removed, simplified, or deliberately left unchanged because it was outside scope.
- Update statuses from `planned` or `partial` only when source and commands prove the change.
- Add generated command names and artifact paths.
- Update README planned docs if they are implemented or no longer needed.
- Add a short `Portfolio Review Evidence` section linking the aggregator output path convention.
- Do not rewrite contracts into marketing language.

Acceptance criteria:
- No doc claims coverage that code does not provide.
- Validation matrix rows reflect current commands and artifact outputs.
- Performance diagnostics metrics reflect current source status.
- Reviewer guide points to the portfolio review command if implemented.
- Relative markdown links resolve.

Validation:
- Run a relative markdown link check.
- Run `rg -n "planned|partial|unknown" Docs/Architecture` and review remaining instances.
- Run `git diff -- Docs/Architecture`.

Report:
- Changed files.
- Remaining planned/partial items.
- Acceptance criteria pass/fail.
```

## Portfolio Ready Definition

The repository is ready for principal-level portfolio evaluation when the following are true:

- RHI code makes device, queue, command list, fence, descriptor, resource, barrier, memory, diagnostics, and native interop ownership explicit.
- D3D12 and Vulkan backend capabilities, debug/capture support, allocator stats, and unsupported features are comparable through backend-neutral data.
- Renderer frame assembly makes pass registration, resource declaration, transient/persistent ownership, history resources, barriers, and viewport products explicit.
- Provider architecture is neutral enough for the staged upscaler, frame-generation, and denoiser families without reshaping Renderer. Neural rendering readiness is handled separately through shader/profile/backend gates until a concrete provider category is justified.
- Shader compiler/runtime code has visible ABI, reflection, cache identity, package inspection, runtime loading, and regression evidence.
- PSO management has explicit identity, cache/library support status, compile timing, hit/miss or unavailable state, shader package compatibility checks, and invalidation behavior.
- Ray tracing foundations expose backend capabilities, BLAS/TLAS lifecycle, scratch/AS memory ownership, RT pipeline identity, shader table expectations, and diagnostics.
- Path tracing can land against named renderer contracts for accumulation/history, sampling, jitter/reset, material/light inputs, output products, and provider-neutral denoiser hooks.
- Neural rendering readiness is visible through shader profile gates, provider category/resource contracts, backend capability states, and honest unavailable/planned reporting.
- GameFramework remains free of Renderer/RHI dependencies, and scene snapshot/invalidation behavior is visible in code or tests.
- Diagnostics artifacts show backend identity, adapter, validation state, provider state, memory, timings, descriptor pressure, upload pressure, and shader package timing where supported.
- Cross-feature proof scenarios exercise PSO, RT readiness, path tracing readiness, neural readiness, descriptor pressure, upload pressure, and baseline assets.
- A job system exists only after those foundations, includes single-thread fallback, and initially parallelizes immutable or clearly owned work.
- The architecture boundary check is runnable from CMake and launcher.
- D3D12 and Vulkan backend startup/shutdown are reportable, with Vulkan absence handled honestly.
- A renderer empty-frame smoke path exists and emits artifacts.
- The launcher can export readiness, dependency, backend, and provider state.
- Generate/build/cook/format/smoke/shader inspection workflows have stable commands.
- Curated baseline scenes/assets exist and are small enough to keep in source control.
- The editor has a compact review dashboard or a clear artifact-driven substitute.
- One portfolio review operation can dry-run or run the main evidence path and produce a manifest.

## Suggested Commit Slices

1. M0 reviewer spine: launcher validation category, readiness dry-run, dependency/backend/provider status export.
2. M1 RHI ownership: device, queue, command, fence, descriptor, resource, barrier, memory, diagnostics, and interop surfaces.
3. M1 backend parity: D3D12/Vulkan debug, capture, memory, allocator, unavailable-feature, startup/shutdown, and parity reports.
4. M2 renderer ownership: frame assembly, pass resources, history, barriers, empty-frame smoke, and frame diagnostics.
5. M3 PSO runtime: pipeline identity, runtime cache/library status, shader package compatibility, compile/create timing, and invalidation.
6. M3 shader pipeline: ABI, reflection, profile gates, package inspection, runtime loading, and regression corpus.
7. M4 scene and assets: GameFramework-to-Renderer snapshot boundary, invalidation, curated tiny scenes, descriptor pressure, upload pressure.
8. M5 provider and neural readiness: provider-neutral SDK state mapping, neural capability/profile gates, resource contracts, readiness artifacts.
9. M6 ray tracing foundation MVP: AS lifecycle, scratch/AS memory, RT pipeline/SBT contracts, backend parity, and diagnostics.
10. M7 path tracing foundation MVP: accumulation/history, sampling/reset, scene input, output product, denoiser hook, and readiness scenario.
11. M8 diagnostics proof: unified diagnostics data plane, artifact writer, cross-feature scenario set, editor review dashboard.
12. M9 job system last: low-level job system, single-thread fallback, safe initial parallelization, scheduling diagnostics.
13. M10 portfolio packaging: build/cook/smoke/shader/backend/diagnostics aggregator, manifest, final doc status refresh.

Keep each commit independently buildable and independently reviewable.

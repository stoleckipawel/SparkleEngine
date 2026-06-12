# RHI/Renderer Review-Ready Implementation Plan

Status: execution plan draft  
Date: 2026-06-12  
Scope: `Engine/RHI`, `Engine/Renderer`, D3D12, Vulkan, ray tracing, frame graph, shader/pass runtime, PSO handling, upscaling, smoke validation, and reviewer-facing repository presentation

## Purpose

This document turns the architecture review and acceptance rubric into an execution runbook. The goal is to make SparkleEngine feel designed as a coherent renderer/RHI system rather than a set of accumulated fixes.

Primary source documents:

- `docs/plans/rhi-renderer-architecture-review.md`
- `docs/plans/architecture-review-acceptance-rubric.md`

Current code evidence used while writing this plan:

- `Engine/RHI/Public/Device/RenderHardwareInterface.h`
- `Engine/RHI/Public/Commands/RenderCommandList.h`
- `Engine/RHI/Public/Pipeline/RhiPipelineStateDesc.h`
- `Engine/RHI/Private/Shaders/BuiltinGlobalShaders.cpp`
- `Engine/RHI/Private/Shaders/DirectLightingShaders.cpp`
- `Engine/Renderer/Public/Renderer.h`
- `Engine/Renderer/Private/Renderer.cpp`
- `Engine/Renderer/Private/Pipeline/RenderPassPipelineTraits.h`
- `Engine/Renderer/Private/Pipeline/PipelineStateManager.h`
- `Engine/Renderer/Private/Pipeline/RenderPassShaderRuntime.h`
- `Engine/Renderer/Private/Pipeline/PassBinder.cpp`
- `Engine/Application/Private/Validation/RhiSmokeEditorValidation.cpp`
- `Engine/Application/Private/Validation/RhiSmokeValidation.cpp`
- `Engine/Renderer/CMakeLists.txt`
- `Engine/RHI/CMakeLists.txt`

## Execution Rules

- Stages are numbered only as `1`, `2`, `3`, and so on. Do not introduce nested stage numbers.
- Each stage must be completed cleanly before moving to the next stage.
- Temporary compatibility adapters are allowed only inside a stage. They must be removed before that stage is accepted unless the stage explicitly says otherwise.
- Do not keep legacy paths "just in case." If the new path replaces the old path and validation passes, delete the old path in the same stage or the immediately following cleanup stage.
- Do not run full builds after every small edit. Run build/runtime validation at the milestone stages in this document, or earlier only when a local compile failure blocks progress.
- Every strategic code stage must include the rubric fields from `docs/plans/architecture-review-acceptance-rubric.md`: owner, dependency impact, D3D12/Vulkan impact, validation plan, risks, and rollback path.
- Every renderer pass or shader change must obey the hard gate from `docs/plans/rhi-renderer-architecture-review.md`: adding an ordinary renderer shader pass must not require editing `Engine/RHI`.
- Every RHI change must answer whether it introduces a GPU/API concept, a backend implementation detail, or a renderer convenience. Renderer conveniences do not belong in RHI.
- Every backend parity claim must be backed by logs, smoke reports, screenshots/captures, or a clearly marked measurement plan.

## Acceptance Criteria Traceability

This section verifies that the execution plan touches every criterion from `docs/plans/architecture-review-acceptance-rubric.md` and every review-ready goal from `docs/plans/rhi-renderer-architecture-review.md`.

### Rubric Criteria Coverage

| Rubric criterion | Covered by stages | Required evidence before final acceptance |
| --- | --- | --- |
| Problem framing | 1, 2, 22 | Coverage status map names the subsystem, current risk, intended owner, and linked stage. Final rubric scoring cites the exact issue each completed stage solved. |
| Requirements and constraints | 1, 2, 5, 10, 15, 20, 22 | Architecture docs and milestone reports state D3D12/Vulkan, ray tracing, DLSS, frame graph, debug view, shader cooking, platform, and validation constraints. |
| Separation of concerns | 3, 4, 7, 8, 9, 11, 12, 19, 22 | Boundary checks pass; RHI has no Renderer-private includes; Renderer has no backend-private includes outside documented provider integration; Application has no backend-native capture implementation. |
| Cohesion and interface size | 6, 7, 11, 12, 16, 19, 22 | RHI method ownership table is complete; root facades shrink behind named services; `Renderer` becomes a host facade; old god-object responsibilities are deleted or owned elsewhere. |
| Tradeoff reasoning | 2, 6, 7, 9, 16, 17, 22 | Design docs record alternatives for shader registration ownership, RHI service extraction, interop, pass definition, PSO keying, and backend parity. |
| Quality attributes | 1, 2, 10, 15, 20, 22 | Each strategic stage updates quality impact for maintainability, reliability, portability, performance reasoning, operability, and reviewability. |
| Risk and technical debt visibility | 1, 3, 5, 10, 15, 20, 22 | Coverage status tracks `Accepted`, `Needs refactor`, and `Needs design decision`; final gate has no unowned `Needs refactor` rows. |
| Runtime behavior clarity | 2, 11, 12, 14, 16, 17, 18, 20, 21 | Docs include frame execution, frame graph, pass definition, shader package, PSO, ray tracing, presentation, and backend flow diagrams. |
| Observability and diagnostics | 7, 8, 10, 14, 15, 16, 18, 20, 22 | Smoke reports include frame graph diagnostics, capability reports, debug names/markers, PSO keys, shader package IDs, DLSS status, RT status, and capture artifacts. |
| Reliability/failure handling | 7, 8, 9, 14, 18, 20, 22 | Missing DLSS/RT/extensions/capture support produces deterministic reasons; unresolved frame graph resources fail development smoke. |
| Performance reasoning | 2, 16, 19, 20, 21, 22 | PSO/runtime changes include measurement plan or logs; README and final scoring avoid unsupported performance claims. |
| Portability/backend parity | 3, 5, 7, 8, 9, 10, 18, 19, 20, 22 | D3D12/Vulkan service responsibilities are symmetric where appropriate; lit and debug/normal captures exist for both APIs; known differences are documented. |
| Maintainability and naming | 2, 11, 12, 13, 14, 17, 18, 19, 22 | Folder ownership docs, naming conventions, pass/frame split, ray tracing terms, and backend service names are updated to match final code. |
| Testability | 3, 5, 8, 10, 15, 20, 22 | Boundary checks, build targets, shader compiler validation, smoke validation, captures, and final command list are repeatable. |
| Communication/reviewability | 1, 2, 21, 22 | Docs, diagrams, README, reviewer path, feature matrix, known issues, validation artifacts, and final rubric score make the repo inspectable. |

Critical rubric categories are covered by multiple stages:

- Requirements and constraints: 1, 2, 5, 10, 15, 20, 22.
- Separation of concerns: 3, 4, 7, 8, 9, 11, 12, 19, 22.
- Tradeoff reasoning: 2, 6, 7, 9, 16, 17, 22.
- Runtime behavior clarity: 2, 11, 12, 14, 16, 17, 18, 20, 21.
- Observability and diagnostics: 7, 8, 10, 14, 15, 16, 18, 20, 22.
- Portability/backend parity: 3, 5, 7, 8, 9, 10, 18, 19, 20, 22.
- Testability: 3, 5, 8, 10, 15, 20, 22.

### Portfolio Skill Signal Coverage

| Portfolio signal | Covered by stages | Required final evidence |
| --- | --- | --- |
| Role relevance | 21, 22 | README states C++20 renderer/RHI scope, D3D12/Vulkan, frame graph, ray tracing, shader tooling, and upscaling. |
| Modern C++ systems skill | 6, 7, 11, 13, 16, 19, 22 | Ownership docs, service extraction, RAII/lifetime contracts, allocator/resource lifetime notes, and clean build commands. |
| Graphics API fluency | 7, 8, 10, 18, 19, 20 | D3D12/Vulkan resource state/layout, descriptors, PSO, swap chain, ray tracing, and capture parity evidence. |
| Shader and pipeline systems | 4, 5, 16, 17, 20 | Renderer-owned shader registration, explicit PSO keys, pass definition model, shader compiler validation. |
| Rendering fundamentals | 10, 14, 18, 20, 21 | Lit, normal/debug, GBuffer, lighting, shadows, temporal/upscaling captures and notes. |
| GPU architecture and performance reasoning | 16, 19, 20, 21, 22 | Timing/diagnostic output, PSO/runtime logs, memory diagnostics, and no unsupported performance claims. |
| Cross-backend architecture | 3, 7, 8, 9, 19, 20 | Mechanical boundary checks and D3D12/Vulkan parity report. |
| Debuggability and validation | 3, 8, 10, 14, 15, 20 | Smoke reports, logs, capture artifacts, validation failure policy. |
| Reliability and fallback behavior | 7, 8, 9, 18, 20 | Deterministic feature fallback reasons for DLSS, RT, capture, and backend capabilities. |
| Testability and CI thinking | 3, 5, 10, 15, 20, 22 | Local/CI-ready commands for boundary checks, formatting, shader compiler, build, smoke. |
| Documentation and onboarding | 2, 21, 22 | Architecture docs, README, reviewer path, feature matrix, known issues. |
| Communication and design rationale | 1, 2, 6, 21, 22 | Decision notes, alternatives, risks, non-goals, final rubric score. |
| Git/review hygiene | 21, 22 | CONTRIBUTING or equivalent review guide, commit/PR conventions, no generated junk in source docs. |
| Product/demo clarity | 10, 20, 21 | Showcase launch path, screenshots/captures, feature matrix, current backend status. |
| Collaboration readiness | 21, 22 | Build/report/validation instructions, known issue guidance, license/status links, contribution path. |

### Architecture Review Goal Coverage

| Architecture review goal | Covered by stages | Required final evidence |
| --- | --- | --- |
| Module dependency direction is mechanically checked | 3, 5, 20, 22 | Boundary check passes with no stale exceptions. |
| RHI method ownership is documented | 6, 7, 19, 22 | RHI contract map covers every public method and final service owner. |
| D3D12 and Vulkan backend folders remain symmetric and backend-private | 3, 19, 20, 22 | Backend services are separate, symmetric where appropriate, and parity-tested. |
| Renderer pass orchestration has a documented convention | 2, 14, 17, 22 | `Frame/*` composition and `Passes/*` execution split is documented and reflected in code. |
| Ray tracing ownership is explained from scene data to TLAS binding | 2, 18, 20, 22 | Ray tracing contract and smoke evidence cover BLAS/TLAS/pass usage. |
| DLSS/native interop has a documented backend contract | 7, 9, 10, 20, 22 | Upscaler/native interop contract and per-backend DLSS status logs. |
| D3D12/Vulkan smoke validation passes with no unresolved frame graph warnings | 10, 15, 20, 22 | Smoke logs and captures show no unresolved resource/barrier warnings. |
| Visual debug modes are validated for both APIs | 10, 20, 22 | D3D12 and Vulkan debug/normal captures exist and are linked from final evidence. |

Traceability conclusion:

- The plan covers all rubric criteria, all critical categories, all portfolio skill signals, and all architecture review definition-of-done goals.
- The only criteria that rely primarily on late-stage work are repo presentation, Git/review hygiene, and collaboration readiness. Stage 21 and Stage 22 are therefore mandatory, not optional polish.

## Reference Basis By Stage

This plan should not drift into invented architecture. Each stage has an external reference basis from existing graphics repositories, SDKs, API documentation, or established architecture patterns.

The references are inspiration and calibration points, not copy-paste targets. Sparkle should borrow the ownership model, vocabulary, and validation habits that fit the engine, while preserving its own module boundaries and current working behavior.

Reference index:

- NVIDIA Donut: https://github.com/NVIDIA-RTX/Donut
- NVIDIA Donut Samples: https://github.com/NVIDIA-RTX/Donut-Samples
- NVIDIA NVRHI: https://github.com/NVIDIA-RTX/NVRHI
- NVIDIA NVRHI tutorial: https://github.com/NVIDIAGameWorks/nvrhi/blob/main/doc/Tutorial.md
- NVIDIA NVRHI technical blog: https://developer.nvidia.com/blog/writing-portable-rendering-code-with-nvrhi/
- NVIDIA NRI: https://github.com/NVIDIA-RTX/NRI
- NVIDIA Falcor: https://github.com/NVIDIAGameWorks/Falcor
- NVIDIA Falcor getting started docs: https://github.com/NVIDIAGameWorks/Falcor/blob/master/docs/getting-started.md
- NVIDIA Streamline: https://github.com/NVIDIA-RTX/Streamline
- NVIDIA Streamline programming guide: https://github.com/NVIDIA-RTX/Streamline/blob/main/docs/ProgrammingGuide.md
- NVIDIA Streamline manual hooking guide: https://github.com/NVIDIA-RTX/Streamline/blob/main/docs/ProgrammingGuideManualHooking.md
- AMD Cauldron: https://github.com/GPUOpen-LibrariesAndSDKs/Cauldron
- AMD FidelityFX SDK: https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK
- AMD FidelityFX SDK overview: https://gpuopen.com/amd-fidelityfx-sdk-1/
- Diligent Engine: https://github.com/DiligentGraphics/DiligentEngine
- Diligent Core PSO model: https://github.com/DiligentGraphics/DiligentCore
- Microsoft D3D12 PSO docs: https://learn.microsoft.com/en-us/windows/win32/direct3d12/pipelines-and-shaders-with-directx-12
- Microsoft D3D12 pipeline state management docs: https://learn.microsoft.com/en-us/windows/win32/direct3d12/managing-graphics-pipeline-state-in-direct3d-12
- Microsoft DXR spec: https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html
- NVIDIA DXR tutorial: https://developer.nvidia.com/rtx/raytracing/dxr/dx12-raytracing-tutorial-part-1
- Khronos Vulkan validation layers: https://github.com/KhronosGroup/Vulkan-ValidationLayers
- Vulkan development tools guide: https://docs.vulkan.org/guide/latest/development_tools.html
- Khronos Vulkan ray tracing sample: https://github.com/KhronosGroup/Vulkan-Samples/blob/main/samples/extensions/ray_tracing_basic/README.adoc
- NVIDIA Vulkan ray tracing tutorial: https://nvpro-samples.github.io/vk_raytracing_tutorial_KHR/
- arc42 overview: https://arc42.org/overview
- CMU SEI ATAM: https://www.sei.cmu.edu/library/architecture-tradeoff-analysis-method-collection/
- ISO/IEC 25010: https://www.iso.org/standard/35733.html
- ADR practice: https://adr.github.io/
- Michael Nygard ADR article: https://www.cognitect.com/blog/2011/11/15/documenting-architecture-decisions
- Composition Root pattern: https://blog.ploeh.dk/2011/07/28/CompositionRoot/
- Facade pattern: https://refactoring.guru/design-patterns/facade
- Command pattern in game code: https://gameprogrammingpatterns.com/command.html
- GitHub portfolio guidance: https://flatironschool.com/blog/github-profile-and-git-practices-for-job-seekers/
- README guidance: https://coding-boot-camp.github.io/full-stack/github/professional-readme-guide/

Stage-to-reference map:

| Stage | Reference basis | Pattern or solution to follow | What Sparkle should borrow | What Sparkle should not copy blindly |
| --- | --- | --- | --- | --- |
| 1 | arc42, CMU SEI ATAM, ISO/IEC 25010, ADR practice | Quality-attribute review and decision tracking | Coverage status, quality risks, decision questions, traceable acceptance evidence | Heavy enterprise ceremony that slows small implementation steps |
| 2 | arc42, Donut, Falcor docs | Reviewer-facing architecture docs and glossary | Clear module map, render pipeline vocabulary, diagrams, and navigation paths | Falcor/Donut terminology where Sparkle already has better local names |
| 3 | NVRHI, NRI, Cauldron, Diligent Engine | Layered RHI plus backend-private implementation trees | Mechanical checks that protect RHI/Renderer/backend direction | A generic RHI shape that ignores Sparkle's existing modules |
| 4 | Donut, Falcor RenderPasses, NVRHI | Renderer passes live above hardware abstraction | Renderer-owned shader registration and pass metadata above RHI infrastructure | Putting every shader concept into Renderer; generic shader package/runtime primitives still belong lower |
| 5 | NVRHI tutorial, Donut Samples, Vulkan validation layers | Early validation milestone after boundary and shader moves | Build the shader compiler and run boundary checks before larger refactors | Treating a successful compile as visual/backend parity |
| 6 | NVRHI, NRI, Diligent Engine | Focused graphics abstraction contracts | RHI method ownership table and service categories | Splitting interfaces before caller evidence proves the boundary |
| 7 | NVRHI technical blog, Diligent Engine, Streamline docs | Explicit services for resources, descriptors, barriers, diagnostics, interop | Service-owned RHI responsibilities and deterministic native metadata | A catch-all interop service that becomes another service locator |
| 8 | Vulkan validation layers, Vulkan development tools guide, Diligent Engine | Validation/capture through graphics-layer services | Application owns test orchestration; RHI/backend owns native capture/readback details | D3D12-only capture path hidden behind a generic name |
| 9 | Streamline, Streamline sample/manual hooking docs, FidelityFX SDK | Vendor feature provider layer with backend capability support | Provider-owned SDK code, backend-owned API setup, deterministic fallback reasons | Vendor SDK details in common renderer pass code or root RHI policy |
| 10 | Donut Samples, NVRHI tutorial, Vulkan validation layers | Milestone validation with artifacts | Backend-specific smoke evidence, logs, captures, and feature status | "It launched" as an acceptance substitute |
| 11 | Donut, Falcor, Composition Root, Facade pattern | Thin public facade plus explicit composition root | `Renderer` as host facade; subsystem wiring in one owned root | Moving the god object to a differently named class |
| 12 | Donut app/render split, NVRHI abstraction, Facade pattern | Host protocol hides presentation/resource details | Viewport/presentation bridge that hides frame graph and resource-state details from Application | Application/editor manually driving frame graph transitions |
| 13 | Donut scene/component graph, Falcor scene/render separation | Render-domain snapshots between scene and renderer | Immutable render DTOs for meshes, materials, lights, camera, skinning, temporal data | Renderer directly depending on gameplay internals for convenience |
| 14 | Falcor RenderGraph, Donut reusable passes, Vulkan validation tools | Graph contract plus diagnostics | Development failures for unresolved resources/barriers and diagnostic dumps | Suppressing warnings to keep smoke green |
| 15 | NVRHI tutorial, Donut Samples, Vulkan validation layers | Milestone validation after renderer/frame graph restructuring | D3D12/Vulkan smoke with lit/debug captures and frame graph diagnostics | Moving to PSO redesign before graph warnings are clean |
| 16 | D3D12 PSO docs, D3D12 pipeline state management, Diligent Core PSO model | Explicit immutable PSO descriptors and cache keys | Printable PSO key, separated package loading, binding layout, validation, and PSO creation | `std::type_index` as final runtime identity |
| 17 | Falcor RenderPasses, Donut reusable passes, Diligent render state notation ideas | Declarative pass definition | One renderer-owned pass definition should drive graph intent and pipeline runtime lookup | Backend-specific pass definitions or duplicated central traits |
| 18 | Microsoft DXR spec, NVIDIA DXR tutorial, Khronos Vulkan ray tracing sample, NVIDIA Vulkan ray tracing tutorial, Donut Samples RT | BLAS/TLAS ownership and API-neutral ray tracing contracts | Clear split between renderer AS scene ownership and RHI AS build descriptors/commands | Shadow/pass concepts in RHI ray tracing structs |
| 19 | NVRHI, NRI, Cauldron, Diligent Engine | Symmetric backend services under a common abstraction | D3D12/Vulkan service symmetry for commands, descriptors, memory, pipeline, resources, diagnostics | Merging API-specific details into common code too early |
| 20 | Donut Samples, NVRHI tutorial, Vulkan validation layers, Streamline/FidelityFX docs | Full backend parity validation | Lit/debug captures, DLSS/RT/frame graph/PSO logs, backend feature reports | Exact image match claims where numeric/API differences require tolerance |
| 21 | GitHub portfolio guidance, README guidance, Donut/Falcor/Cauldron repo presentation | Reviewer-facing repo entry point | README, feature matrix, reviewer path, screenshots/captures, known issues, validation commands | Marketing copy without evidence |
| 22 | Architecture rubric, arc42, ADR practice, CMU SEI ATAM | Final quality gate and decision record | Rubric scoring, final cleanup, evidence index, no lingering legacy contradictions | Calling the repo review-ready with weak critical criteria |

Reference use rules:

- Before implementing a stage, inspect at least one listed reference and one Sparkle code path named by that stage.
- Add an ADR or design note when Sparkle intentionally diverges from the reference model.
- Prefer references from NVIDIA, AMD, Khronos, Microsoft, or established open-source graphics engines for GPU/API behavior.
- Use general software architecture references only for process and pattern vocabulary, not for GPU contract details.
- If a stage discovers a better reference implementation, add it here before using it as a design basis.

## Stage 1 - Baseline Status And Evidence Freeze

Goal:

- Convert the architecture review's coverage audit into a tracked status map before any major refactor.
- Make current debt explicit instead of relying on memory.

Source references:

- `rhi-renderer-architecture-review.md`: `Whole-Codebase Coverage Audit`, `Coverage Acceptance Criteria`, `Initial Proposed Work Items`
- `architecture-review-acceptance-rubric.md`: `The Criteria`, `Sparkle-Specific Review Questions`

External implementation references:

- arc42 quality and architecture documentation structure: https://arc42.org/overview
- CMU SEI ATAM quality-attribute review method: https://www.sei.cmu.edu/library/architecture-tradeoff-analysis-method-collection/
- ADR practice for traceable decisions: https://adr.github.io/

Code references:

- `Engine/Renderer`
- `Engine/RHI`
- `Engine/Application/Private/Validation`
- `Tools/Launcher/SparkleLauncher/Private/Launch/Smoke`

Tutor note:

- What is wrong today: we know several important risks, but they live as prose and memory rather than a tracked status table.
- What changes: every Renderer/RHI area gets a status, owner, risk, validation artifact, and linked stage.
- Why it improves the engine: this prevents "we forgot that subsystem" refactors and teaches you to drive architecture from evidence instead of anxiety.

Implementation prompt:

```text
Using the arc42, ATAM, and ADR references listed in this stage, create a tracked architecture status document for all Renderer/RHI coverage rows. For each row, assign status Accepted, Needs refactor, or Needs design decision. Add owner layer, primary risk, validation artifact, related stage in this execution plan, and final acceptance evidence. Do not change runtime code in this stage.
```

Positive guardrails:

- Use the exact folder rows from the architecture review as the starting point.
- Mark uncertainty honestly.
- Prefer specific evidence such as file names, command names, and expected artifacts.
- Keep this as a living checklist that future stages update.

Negative guardrails:

- Do not mark a subsystem accepted because it "seems fine."
- Do not combine unrelated subsystems into vague buckets.
- Do not start refactoring before the baseline is written.

Legacy cleanup:

- None yet. This is an evidence-freeze stage.

Acceptance:

- New status document exists, likely `docs/architecture/rendering-coverage-status.md`.
- Every coverage row has a status and owner.
- Every `Needs refactor` row links to a later stage in this document.
- Every `Needs design decision` row has an explicit question.

Validation:

- Docs-only. No build required.

## Stage 2 - Reviewer Architecture Docs And Vocabulary

Goal:

- Create the architecture docs that make the system reviewable by someone new to the repo.
- Freeze vocabulary before renaming or moving systems.

Source references:

- `rhi-renderer-architecture-review.md`: `Proposed Review Process`, `Target Hierarchy`, `System Edge Review`
- `architecture-review-acceptance-rubric.md`: `Documentation and onboarding`, `Communication and design rationale`

External implementation references:

- arc42 architecture documentation structure: https://arc42.org/overview
- NVIDIA Donut repository organization: https://github.com/NVIDIA-RTX/Donut
- NVIDIA Falcor documentation entry point: https://github.com/NVIDIAGameWorks/Falcor/blob/master/docs/getting-started.md

Code references:

- `Engine/Renderer/Public/Renderer.h`
- `Engine/RHI/Public/Device/RenderHardwareInterface.h`
- `Engine/Renderer/Private/FrameGraph`
- `Engine/Renderer/Private/RayTracing`
- `Engine/Renderer/Private/Pipeline`

Tutor note:

- What is wrong today: the code has real systems, but a new reviewer has to infer vocabulary and ownership from source files.
- What changes: we write the map, glossary, contracts, and diagrams that explain the engine before someone dives into implementation details.
- Why it improves the engine: good architecture is not only code shape; it is also shared language that prevents future features from choosing random names and boundaries.

Implementation prompt:

```text
Using arc42's documentation structure and the repository navigation style visible in Donut and Falcor, add architecture docs for rendering vocabulary, system map, RHI contract map, frame graph contract, ray tracing contract, pass authoring contract, and pipeline runtime contract. Link them from the existing architecture review. Keep docs precise, code-referenced, and aligned with current code, even where the current code is imperfect.
```

Positive guardrails:

- Define terms such as RHI, backend, command context, command list, frame graph pass, pass runtime, PSO key, native interop, BLAS, TLAS, and upscaler provider.
- Include simple diagrams for frame execution, shader package flow, PSO creation, and backend boundaries.
- Keep references to code files clickable and current.
- Make docs say what owns what, not only what exists.

Negative guardrails:

- Do not invent architecture that is not implemented or planned in this runbook.
- Do not use vague claims like "clean architecture" without concrete ownership.
- Do not duplicate the same explanation across many docs.

Legacy cleanup:

- Remove stale references to deleted planning docs.

Acceptance:

- `docs/architecture/rendering-glossary.md` exists.
- `docs/architecture/rendering-system-map.md` exists.
- `docs/architecture/rhi-contract-map.md` exists.
- `docs/architecture/frame-graph-contract.md` exists.
- `docs/architecture/ray-tracing-contract.md` exists.
- `docs/architecture/pass-authoring-contract.md` exists.
- `docs/architecture/pipeline-runtime-contract.md` exists.
- Main planning docs link to the new architecture docs.

Validation:

- Docs-only. No build required.

## Stage 3 - Mechanical Boundary Guardrails

Goal:

- Add automated checks that enforce the intended layer direction before moving code.

Source references:

- `rhi-renderer-architecture-review.md`: `Phase 1: Boundary Audit`, `Strategic Refactor Tracks`, `Shader Registration Lives In RHI But Reaches Into Renderer`
- `architecture-review-acceptance-rubric.md`: `Separation of concerns`, `Testability`

External implementation references:

- NVIDIA NVRHI as a focused RHI layer: https://github.com/NVIDIA-RTX/NVRHI
- NVIDIA NRI as a graphics abstraction boundary: https://github.com/NVIDIA-RTX/NRI
- AMD Cauldron DX12/VK backend separation: https://github.com/GPUOpen-LibrariesAndSDKs/Cauldron
- Diligent Engine backend abstraction layout: https://github.com/DiligentGraphics/DiligentEngine

Current violations to protect against:

- `Engine/RHI/Private/Shaders/DirectLightingShaders.cpp` includes `Renderer/Private/RayTracing/RayTracedShadowUniformData.h`.
- `Engine/Renderer/CMakeLists.txt` links `Vulkan::Vulkan` when Streamline is enabled.
- `Engine/Application/Private/Validation/RhiSmokeEditorValidation.cpp` includes D3D12 headers and owns D3D12-native capture logic.

Tutor note:

- What is wrong today: the intended layer order exists in guidance, but the compiler does not enforce it.
- What changes: forbidden include and dependency checks turn architecture from a promise into a guardrail.
- Why it improves the engine: once boundaries are mechanical, refactors become safer because new code cannot quietly reintroduce the same coupling.

Implementation prompt:

```text
Using the backend separation patterns in NVRHI, NRI, Cauldron, and Diligent Engine as the boundary model, add local and CMake/CI-friendly forbidden dependency checks for Renderer/RHI architecture boundaries. The checks must report actionable file paths and reasons. Initially allow documented transitional exceptions only where this plan has not yet migrated the code. Remove each exception in the stage that fixes it.
```

Positive guardrails:

- Check `Engine/RHI` for `Renderer/Private` includes.
- Check normal `Engine/Renderer` paths for D3D12/Vulkan private headers or native API identifiers, excluding explicitly documented provider integration paths during migration.
- Check D3D12 and Vulkan backend folders do not include each other.
- Check Application validation does not grow new backend-native dependencies.
- Add the check in a form usable by CI and local developers.

Negative guardrails:

- Do not suppress violations silently.
- Do not create a broad allowlist that hides future architectural drift.
- Do not block generated or third-party files unless they are part of engine source policy.

Legacy cleanup:

- No broad cleanup yet. Transitional allowlist entries must include the stage that removes them.

Acceptance:

- New check target or script exists, likely `architecture_boundary_check`.
- Existing violations are listed as temporary exceptions with removal stages.
- The check can be run without building the editor.

Validation:

- Run the boundary check.
- No full renderer build required unless the check is integrated into CMake and configure/build wiring must be verified.

## Stage 4 - Move Renderer Shader Registration Out Of RHI

Goal:

- Enforce the hard gate: ordinary renderer shader passes must not require RHI edits.
- Remove renderer-specific pass shader declarations from `Engine/RHI/Private/Shaders`.

Source references:

- `rhi-renderer-architecture-review.md`: `Shader Registration Lives In RHI But Reaches Into Renderer`, `Shader Pass And PSO Handling`
- `architecture-review-acceptance-rubric.md`: `Shader and pipeline systems`, `Cross-backend architecture`

External implementation references:

- NVIDIA Donut reusable rendering framework above NVRHI: https://github.com/NVIDIA-RTX/Donut
- NVIDIA NVRHI hardware abstraction layer: https://github.com/NVIDIA-RTX/NVRHI
- NVIDIA Falcor render pass model: https://github.com/NVIDIAGameWorks/Falcor

Code references:

- `Engine/RHI/Private/Shaders/BuiltinGlobalShaders.cpp`
- `Engine/RHI/Private/Shaders/GBufferShaders.cpp`
- `Engine/RHI/Private/Shaders/DirectLightingShaders.cpp`
- `Engine/RHI/Private/Shaders/IndirectLightingShaders.cpp`
- `Engine/RHI/Private/Shaders/LightingCompositeShaders.cpp`
- `Engine/RHI/Private/Shaders/SkyShaders.cpp`
- `Engine/RHI/Private/Shaders/VisualizeBuffersShaders.cpp`
- `Engine/RHI/Private/Shaders/ComputeClearShader.cpp`
- `Engine/Renderer/Private/Passes`
- `Engine/Renderer/Private/Pipeline/RenderPassPipelineTraits.h`
- `Tools/Shaders/ShaderCompiler/Private/Cooking/ShaderCookPlanner.cpp`

Tutor note:

- What is wrong today: renderer-specific shader pass declarations live in RHI, and DirectLighting even pulls renderer-private shadow data into RHI.
- What changes: shader pass registration moves up to Renderer or a neutral shader-authoring layer, while RHI keeps only generic shader package and layout primitives.
- Why it improves the engine: an RHI should know how to create/bind GPU objects, not know that a renderer has GBuffer, Sky, or DirectLighting passes.

Implementation prompt:

```text
Using Donut and Falcor as examples of renderer-owned passes above a hardware abstraction, and NVRHI as the lower abstraction boundary, move renderer pass shader registration from Engine/RHI/Private/Shaders into Renderer-owned shader registration files or a neutral shader authoring module that depends downward only. Preserve generic RHI shader infrastructure and genuinely generic builtin test shaders in RHI. Delete the old renderer pass registration files from RHI after the new registrations are wired and the shader compiler can still enumerate the same packages.
```

Positive guardrails:

- Renderer owns pass names, shader paths, entry points, expected stages, binding layout IDs, and pass-specific uniform structs.
- RHI owns only generic shader package/reflection/layout/runtime primitives.
- The shader compiler can still collect registrations without making RHI depend on Renderer.
- Keep package IDs stable unless a deliberate migration note says otherwise.
- Use `DirectLighting` as the proof because it currently includes renderer-private shadow data from RHI.

Negative guardrails:

- Do not move `RayTracedShadowUniformData` into RHI just to satisfy the include rule.
- Do not keep duplicate renderer pass registrations in both RHI and Renderer.
- Do not add backend-specific registration code.
- Do not solve this by weakening the boundary check.

Legacy cleanup:

- Delete renderer pass registration files from `Engine/RHI/Private/Shaders` once moved.
- Remove renderer pass calls from `RegisterBuiltinGlobalShaders`.
- Remove transitional boundary-check exceptions for RHI-to-Renderer includes.

Acceptance:

- `rg "Renderer/Private" Engine/RHI` returns no violations.
- Adding Bloom, SSAO, SSR, debug visualization, lighting variants, or material shaders would require no `Engine/RHI` edit.
- Shader compiler package enumeration still includes all expected renderer pass packages.
- Docs name the new shader registration ownership.

Validation:

- Defer full build to Stage 5.

## Stage 5 - Validation Milestone A: Boundaries And Shader Registration

Goal:

- Validate the first major section: docs, boundary checks, and shader registration ownership.

External implementation references:

- NVIDIA NVRHI tutorial validation mindset: https://github.com/NVIDIAGameWorks/nvrhi/blob/main/doc/Tutorial.md
- NVIDIA Donut Samples as executable graphics evidence: https://github.com/NVIDIA-RTX/Donut-Samples
- Khronos Vulkan validation layers: https://github.com/KhronosGroup/Vulkan-ValidationLayers

Tutor note:

- What is wrong today: it is easy to finish a structural move and only later discover shader cooking or registration broke.
- What changes: this stage validates the first boundary slice before deeper refactors make failures harder to localize.
- Why it improves the engine: milestone validation teaches you to split big architecture work into chunks that can prove they did not break the foundation.

Implementation prompt:

```text
Using the validation habits from NVRHI tutorials, Donut Samples, and Vulkan validation layers, run the smallest meaningful validation for the boundary and shader-registration migration. Verify boundary checks, shader compiler build, renderer build surface, and shader package enumeration. Record commands, results, and remaining exceptions in the coverage status document.
```

Positive guardrails:

- Validate both the mechanical check and the actual build path.
- Capture failure output in the status doc if a build fails.
- Prefer targeted build commands over full solution build unless dependencies require it.

Negative guardrails:

- Do not proceed with known RHI-to-Renderer include violations.
- Do not accept duplicated shader package registration.
- Do not leave deleted files referenced by CMake or build tooling.

Suggested validation:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config DevelopmentEditor --target ShaderCompiler -- /nologo /v:minimal /m:1
cmake --build build --config DevelopmentEditor --target SparkleLauncher -- /nologo /v:minimal /m:1
cmake --build build --config DevelopmentEditor --target clang_format_check -- /nologo /v:minimal
```

Acceptance:

- Boundary check passes with no permanent exceptions for RHI-to-Renderer includes.
- `ShaderCompiler` builds.
- `SparkleLauncher` builds or a more relevant runtime/editor target builds if target names change.
- Coverage status is updated.

## Stage 6 - RHI Method Ownership And Service Extraction Design

Goal:

- Classify the large `RenderHardwareInterface` before splitting it.
- Prevent future root-facade bloat.

Source references:

- `rhi-renderer-architecture-review.md`: `RHI Interface Is Too Broad`, `Phase 2: RHI Contract Classification`
- `architecture-review-acceptance-rubric.md`: `Cohesion and interface size`, `Tradeoff reasoning`

External implementation references:

- NVIDIA NVRHI focused graphics abstraction: https://github.com/NVIDIA-RTX/NVRHI
- NVIDIA NRI low-level rendering interface: https://github.com/NVIDIA-RTX/NRI
- Diligent Engine device/context/pipeline abstraction: https://github.com/DiligentGraphics/DiligentEngine

Code references:

- `Engine/RHI/Public/Device/RenderHardwareInterface.h`
- `Engine/RHI/Private/D3D12/D3D12RenderHardwareInterface.cpp`
- `Engine/RHI/Private/Vulkan/VulkanRenderHardwareInterface.cpp`
- `Engine/RHI/Public/Commands/RenderCommandList.h`

Tutor note:

- What is wrong today: `RenderHardwareInterface` is convenient, but it mixes device, resources, descriptors, pipelines, constants, RT, presentation, diagnostics, capture, interop, and UI.
- What changes: we classify every method before extracting anything, so the split is based on real caller pressure.
- Why it improves the engine: reviewers trust an interface more when every method has one reason to exist and one owner.

Implementation prompt:

```text
Using NVRHI, NRI, and Diligent Engine as examples of focused graphics abstraction boundaries, create a complete RHI method ownership table. Categorize every RenderHardwareInterface method by service: device/capability, command queue/list, resources, descriptors/views, pipelines/binding layouts, constants/uploads, ray tracing, presentation, diagnostics, interop, capture/readback, UI. Map current callers and propose extraction order. Do not split the interface before this table is complete.
```

Positive guardrails:

- Every method gets one primary owner category.
- Methods may list secondary users, but not multiple primary owners.
- Each category names D3D12 and Vulkan implementation files.
- Identify which methods are renderer conveniences and should move upward or behind a narrower bridge.

Negative guardrails:

- Do not start by creating many abstract interfaces without caller evidence.
- Do not move methods just to reduce line count.
- Do not hide API-specific requirements behind vague "misc" categories.

Legacy cleanup:

- Mark methods that should disappear from the root facade after service extraction.

Acceptance:

- `docs/architecture/rhi-contract-map.md` contains every public RHI method.
- Categories with more than 10 methods have an extraction proposal.
- New RHI methods are forbidden unless added to the map with owner and caller.

Validation:

- Docs-only unless helper scripts are added.

## Stage 7 - Extract First RHI Services: Interop, Capture, Diagnostics, Presentation

Goal:

- Move the most cross-cutting pressure points out of the root RHI facade first.
- Give Application validation and DLSS a stable backend-owned route instead of native API leakage.

Source references:

- `rhi-renderer-architecture-review.md`: `Native Interop Is Necessary But Needs A Formal Contract`, `System Edge Review`, `Track 4`
- `architecture-review-acceptance-rubric.md`: `Reliability/failure handling`, `Observability and diagnostics`

External implementation references:

- NVIDIA NVRHI portable rendering code article: https://developer.nvidia.com/blog/writing-portable-rendering-code-with-nvrhi/
- Diligent Engine service-like graphics abstractions: https://github.com/DiligentGraphics/DiligentEngine
- NVIDIA Streamline programming guide: https://github.com/NVIDIA-RTX/Streamline/blob/main/docs/ProgrammingGuide.md

Code references:

- `RenderHardwareInterface::GetDeviceHandle`
- `RenderHardwareInterface::GetGraphicsQueueHandle`
- `RenderHardwareInterface::UpgradePresentationInterface`
- `RenderHardwareInterface::CaptureTextureToBmp`
- `RenderHardwareInterface::GetNativeTextureViewInfo`
- `RenderHardwareInterface::ResolveImGuiTextureId`
- `RenderHardwareInterface::BeginPresentRenderPass`
- `RenderHardwareInterface::BeginPresentOverlayPass`
- `RenderHardwareInterface::EndPresentRenderPass`
- `Engine/RHI/Public/Interop/RhiNativeHandles.h`
- `Engine/RHI/Public/Diagnostics/RhiDiagnostics.h`
- `Engine/RHI/Public/UI/RhiImGuiRenderer.h`

Tutor note:

- What is wrong today: external SDKs, capture, diagnostics, UI, and presentation all pressure the root RHI facade.
- What changes: these become narrow services so renderer/application code asks for the capability it needs instead of grabbing the whole backend.
- Why it improves the engine: service boundaries make native interop explicit and stop one integration from turning RHI into a bag of unrelated escape hatches.

Implementation prompt:

```text
Using NVRHI/Diligent-style service responsibilities and Streamline's explicit interop requirements as references, extract the first narrow RHI services for external interop, capture/readback, diagnostics, and presentation/UI. Keep RenderHardwareInterface as a facade during migration, but delegate to service objects internally. Update D3D12 and Vulkan implementations symmetrically. Keep temporary facade forwarding only until callers are migrated.
```

Positive guardrails:

- Keep D3D12 and Vulkan service names symmetric.
- Let RHI fill native metadata deterministically.
- Make provider needs explicit: Streamline D3D12, Streamline Vulkan, future FSR/NRD.
- Keep diagnostics and capture errors actionable.

Negative guardrails:

- Do not add more `void*` casting in Renderer or Application.
- Do not make Renderer include backend-private headers.
- Do not let interop structs become undifferentiated bags of fields without consumer notes.

Legacy cleanup:

- Remove facade methods from direct callers after service migration where practical.
- Remove transitional casts outside backend/provider integration.

Acceptance:

- DLSS/upscaling provider talks to explicit interop/presentation services.
- Application validation no longer needs native device/queue handles directly after Stage 8.
- D3D12/Vulkan service implementations have matching responsibilities.

Validation:

- Defer full build to Stage 10 unless interface extraction creates immediate compile blockers.

## Stage 8 - Move Smoke Capture And Backend-Native Validation Behind RHI

Goal:

- Remove backend-native D3D12 capture code from Application validation.
- Make smoke validation usable for both D3D12 and Vulkan.

Source references:

- `rhi-renderer-architecture-review.md`: `Application Validation -> Backend APIs`, `Backend Parity Matrix`
- `architecture-review-acceptance-rubric.md`: `Debuggability and validation`, `Testability`

External implementation references:

- Khronos Vulkan validation layers: https://github.com/KhronosGroup/Vulkan-ValidationLayers
- Vulkan development tools guide: https://docs.vulkan.org/guide/latest/development_tools.html
- Diligent Engine backend abstraction and testing style: https://github.com/DiligentGraphics/DiligentEngine

Code references:

- `Engine/Application/Private/Validation/RhiSmokeEditorValidation.cpp`
- `Engine/Application/Private/Validation/RhiSmokeValidation.cpp`
- `Tools/Launcher/SparkleLauncher/Private/Launch/Smoke/RhiSmokeLaunchOperations.cpp`
- RHI capture/readback service from Stage 7

Tutor note:

- What is wrong today: Application validation knows how to do D3D12-native capture, which means the test layer owns backend details.
- What changes: Application asks RHI to capture/read back; D3D12 and Vulkan own the native work.
- Why it improves the engine: tests should verify behavior across backends, not become another backend implementation.

Implementation prompt:

```text
Using Vulkan validation tooling and Diligent-style backend abstraction as references, refactor RHI smoke capture/readback so Application validation requests capture through an RHI-owned service. Remove D3D12 headers and D3D12-specific capture implementation from Application. Extend smoke evidence to support backend, view mode, capture path, frame graph warning status, DLSS status, and ray tracing status.
```

Positive guardrails:

- Application owns test orchestration, not backend implementation.
- Backend capture code lives in RHI backend or a clearly marked backend-owned validation helper.
- Smoke validation can capture lit and debug/normal view modes for D3D12 and Vulkan.
- Smoke failures should be deterministic and logged with backend, frame, view mode, and output path.

Negative guardrails:

- Do not keep a D3D12-only capture fallback in Application.
- Do not make Vulkan capture a TODO while claiming backend parity.
- Do not allow smoke to pass while frame graph unresolved-resource warnings are present.

Legacy cleanup:

- Delete Application-local BMP/D3D12 readback helpers after RHI capture service works.
- Remove temporary boundary-check exception for Application D3D12 native headers.

Acceptance:

- `RhiSmokeEditorValidation.cpp` includes no D3D12/Vulkan native headers.
- Smoke capture uses RHI service on both D3D12 and Vulkan or reports unsupported with a precise reason.
- Launcher smoke options expose or document backend, view mode, and capture evidence path.

Validation:

- Defer full runtime smoke to Stage 10.

## Stage 9 - Formalize Upscaling And Native Interop Contracts

Goal:

- Make DLSS and future external providers cleanly separated from RHI and Renderer policy.
- Fix current ambiguity around Vulkan linkage in `Engine/Renderer/CMakeLists.txt`.

Source references:

- `rhi-renderer-architecture-review.md`: `Native Interop Is Necessary But Needs A Formal Contract`, `Vendor SDKs Should Stay Out Of Core RHI Policy`
- `architecture-review-acceptance-rubric.md`: `Reliability and fallback behavior`, `Graphics API fluency`

External implementation references:

- NVIDIA Streamline repository: https://github.com/NVIDIA-RTX/Streamline
- NVIDIA Streamline programming guide: https://github.com/NVIDIA-RTX/Streamline/blob/main/docs/ProgrammingGuide.md
- NVIDIA Streamline manual hooking guide: https://github.com/NVIDIA-RTX/Streamline/blob/main/docs/ProgrammingGuideManualHooking.md
- AMD FidelityFX SDK provider/backend pattern: https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK

Code references:

- `Engine/Renderer/Private/Upscaling`
- `Engine/Renderer/Private/Upscaling/NvidiaDlss`
- `Engine/Renderer/CMakeLists.txt`
- `Engine/RHI/Public/Interop/RhiNativeHandles.h`
- `Engine/RHI/Private/D3D12/Device/D3D12ExternalFeatureInteropCapabilities.*`
- `Engine/RHI/Private/Vulkan/Device/VulkanExternalFeatureInteropCapabilities.*`

Tutor note:

- What is wrong today: DLSS needs native resource details, but without a formal contract those details can leak into Renderer or root RHI policy.
- What changes: provider code owns vendor SDK calls, RHI backends own API setup/native metadata, and common renderer code stays provider-neutral.
- Why it improves the engine: vendor integrations are powerful but contagious; isolating them keeps future FSR/NRD integrations from warping the renderer architecture.

Implementation prompt:

```text
Using Streamline's programming/manual-hooking guides and FidelityFX SDK's provider/backend separation as references, document and implement a provider-facing upscaler input and native interop contract. Keep NVIDIA Streamline implementation inside the NvidiaDlss provider, keep backend extension/device setup inside RHI backends, and remove unexplained renderer-level Vulkan linkage or document it as a provider SDK requirement with a narrow wrapper. Make failure reasons visible.
```

Positive guardrails:

- Provider code may use vendor SDK headers.
- Backend code may enable provider-required API features and log why.
- Renderer pass/frame code should see provider-neutral upscaler contracts.
- DLSS unavailable states must say whether the cause is SDK, driver, backend, feature, resource state, or input contract.

Negative guardrails:

- Do not let `NvidiaDlss` policy leak into common RHI.
- Do not let Streamline-specific details appear in general renderer pass code.
- Do not use passthrough fallback without logging the reason.

Legacy cleanup:

- Remove or narrow `Vulkan::Vulkan` linkage from `SparkleRenderer` if the provider can be isolated behind RHI/provider wrapper.
- Delete obsolete native handle fields after the new contract replaces them.

Acceptance:

- Upscaler contract doc exists.
- D3D12 and Vulkan DLSS setup paths log capability/failure reasons.
- Renderer common code remains provider-neutral.
- Boundary check exceptions for Renderer Vulkan native linkage are removed or documented as a narrow provider exception.

Validation:

- Defer full backend DLSS smoke to Stage 10.

## Stage 10 - Validation Milestone B: RHI Services, Capture, Interop, Upscaling

Goal:

- Validate the second major section: RHI service extraction, smoke capture isolation, and upscaling interop.

External implementation references:

- NVIDIA Donut Samples validation-by-running approach: https://github.com/NVIDIA-RTX/Donut-Samples
- NVIDIA NVRHI tutorial: https://github.com/NVIDIAGameWorks/nvrhi/blob/main/doc/Tutorial.md
- Khronos Vulkan validation layers: https://github.com/KhronosGroup/Vulkan-ValidationLayers
- NVIDIA Streamline programming guide: https://github.com/NVIDIA-RTX/Streamline/blob/main/docs/ProgrammingGuide.md

Tutor note:

- What is wrong today: upscaler, capture, and interop bugs can look like rendering noise or backend instability unless evidence names the active path.
- What changes: milestone validation records backend, view mode, capture artifact, DLSS status, and frame graph health.
- Why it improves the engine: good graphics engineering means failures explain themselves enough that you know which system to inspect first.

Implementation prompt:

```text
Using Donut Samples, NVRHI tutorial practices, Vulkan validation layers, and Streamline diagnostics as references, build the affected editor/runtime targets and run smoke validation for D3D12 and Vulkan with lit and debug/normal captures. Verify Application has no backend-native capture code and DLSS reports active provider or deterministic fallback. Record exact commands and artifacts.
```

Positive guardrails:

- Validate both backend selection paths.
- Capture logs and screenshots/BMPs with backend and view mode in file names.
- Treat frame graph unresolved handles as a failure.
- Treat silent DLSS fallback as a failure.

Negative guardrails:

- Do not accept "it launched" without capture/log evidence.
- Do not compare D3D12 and Vulkan only in lit mode; include normal/debug view mode.

Suggested validation:

```powershell
cmake --build build --config DevelopmentEditor --target SparkleLauncher -- /nologo /v:minimal /m:1
cmake --build build --config DevelopmentEditor --target ShaderCompiler -- /nologo /v:minimal /m:1
cmake --build build --config DevelopmentEditor --target clang_format_check -- /nologo /v:minimal

$env:SPARKLE_SMOKE_VALIDATE_RHI='1'
$env:SPARKLE_SMOKE_FRAME_LIMIT='120'
$env:SPARKLE_SMOKE_SCENE_COLOR_CAPTURE_FRAME='30'
$env:SPARKLE_RHI_BACKEND='D3D12'
$env:SPARKLE_SMOKE_VIEW_MODE='0'
$env:SPARKLE_SMOKE_SCENE_COLOR_CAPTURE='artifacts/validation/d3d12-lit.bmp'
# Launch Showcase editor/runtime target here through the existing launcher or executable path.

$env:SPARKLE_RHI_BACKEND='Vulkan'
$env:SPARKLE_SMOKE_VIEW_MODE='0'
$env:SPARKLE_SMOKE_SCENE_COLOR_CAPTURE='artifacts/validation/vulkan-lit.bmp'
# Launch Showcase editor/runtime target here.
```

Acceptance:

- D3D12 and Vulkan smoke runs complete.
- Captures are produced or unsupported capture is reported with a backend-specific reason.
- DLSS status is logged for both backends.
- No Application source includes D3D12/Vulkan native headers for smoke capture.
- Coverage status is updated.

## Stage 11 - Decompose Renderer Into Facade, System Root, Frame Pipeline

Goal:

- Reduce `Renderer.cpp` from central hub to clear public facade.
- Make frame lifecycle and subsystem ownership reviewable.

Source references:

- `rhi-renderer-architecture-review.md`: `Renderer Is Too Central`, `Target Hierarchy`, `Track 3`
- `architecture-review-acceptance-rubric.md`: `Runtime behavior clarity`, `Maintainability and naming`

External implementation references:

- NVIDIA Donut app/render layering: https://github.com/NVIDIA-RTX/Donut
- NVIDIA Falcor renderer/application organization: https://github.com/NVIDIAGameWorks/Falcor
- Composition Root pattern: https://blog.ploeh.dk/2011/07/28/CompositionRoot/
- Facade pattern: https://refactoring.guru/design-patterns/facade

Code references:

- `Engine/Renderer/Public/Renderer.h`
- `Engine/Renderer/Private/Renderer.cpp`
- `Engine/Renderer/Private/Frame`
- `Engine/Renderer/Private/Diagnostics`
- `Engine/Renderer/Private/Upscaling`
- `Engine/Renderer/Private/RayTracing`

Tutor note:

- What is wrong today: `Renderer` is doing too many jobs, so any feature change risks touching the central hub.
- What changes: construction, frame scheduling, feature systems, and public host API become separate responsibilities.
- Why it improves the engine: a facade is good when it hides complexity, but dangerous when it owns all complexity; this stage restores that distinction.

Implementation prompt:

```text
Using Donut/Falcor application-renderer separation plus the Composition Root and Facade patterns as references, extract Renderer internals into explicit ownership objects: RendererSystemRoot for subsystem construction/lifetime, FramePipeline for begin/setup/record/submit/end frame, and feature systems for ray tracing/upscaling/meshes/textures/materials. Keep Renderer as the public host facade. Preserve behavior while moving ownership out of Renderer.cpp.
```

Positive guardrails:

- Keep public API stable until host protocol is intentionally revised in Stage 12.
- Move coherent groups, not random functions.
- Make ownership visible in constructor dependencies.
- Keep diagnostics and logging names stable or improve them deliberately.

Negative guardrails:

- Do not create a new god object with a different name.
- Do not spread frame state through global/singleton access.
- Do not expose private feature systems through `Renderer.h`.

Legacy cleanup:

- Delete moved private methods from `Renderer.cpp`.
- Remove unused includes and forward declarations from `Renderer.h`.
- Update coverage status for `Renderer.cpp` root orchestration.

Acceptance:

- `Renderer.cpp` is a facade and host boundary, not the full frame scheduler.
- Frame execution can be documented as `Renderer -> FramePipeline -> FrameGraph/PassSystem`.
- Subsystem construction is centralized in a system root or equivalent composition object.

Validation:

- Defer full build to Stage 15 unless refactor creates obvious compile blockers.

## Stage 12 - Add Viewport Presentation Bridge And Clean Host Protocol

Goal:

- Remove ad hoc editor/application manipulation of renderer products and transitions.
- Make Application -> Renderer edge a stable host protocol.

Source references:

- `rhi-renderer-architecture-review.md`: `Application -> Renderer`, `Renderer Public Coverage`, `Target Hierarchy`
- `architecture-review-acceptance-rubric.md`: `Communication/reviewability`, `Separation of concerns`

External implementation references:

- NVIDIA Donut app/render split: https://github.com/NVIDIA-RTX/Donut
- NVIDIA NVRHI abstraction model: https://github.com/NVIDIA-RTX/NVRHI
- Facade pattern: https://refactoring.guru/design-patterns/facade

Code references:

- `Engine/Renderer/Public/Viewport/ViewportContracts.h`
- `Engine/Renderer/Public/Renderer.h`
- `Renderer::ResolveRenderProductTextureId`
- `Renderer::ResolveRenderProductResource`
- `Renderer::TransitionRenderProduct`
- `Engine/Application/Private`
- Editor UI code that consumes viewport products

Tutor note:

- What is wrong today: Application/editor paths can manually resolve render products and drive transitions, which exposes renderer internals.
- What changes: a presentation bridge owns viewport products, ImGui texture IDs, and state transitions.
- Why it improves the engine: host code should request presentation, not know the frame graph resource state machine.

Implementation prompt:

```text
Using Donut's app/render split, NVRHI's abstraction boundary, and the Facade pattern as references, introduce a viewport/presentation bridge that owns render product publication, texture ID resolution, and required state transitions for editor/runtime presentation. Replace public Renderer methods that expose manual render product transitions with a host-facing protocol. Remove old public transition helpers once all callers migrate.
```

Positive guardrails:

- Application asks for a viewport product; Renderer/presentation bridge handles resource state details.
- ImGui texture ID resolution stays behind an RHI UI/presentation bridge.
- The host protocol names lifecycle operations clearly.

Negative guardrails:

- Do not make Application call frame graph resource APIs.
- Do not expose `NativeResourceHandle` to editor UI unless a capture/test service explicitly needs it.
- Do not duplicate transition logic between Application and Renderer.

Legacy cleanup:

- Remove public `TransitionRenderProduct` after migration.
- Remove public native resource resolution if only smoke/capture needed it; route capture through RHI service.
- Update docs and call sites.

Acceptance:

- Application/editor no longer performs manual frame graph product transitions.
- `Renderer.h` reads as host protocol plus diagnostics, not internal access.
- Viewport presentation contract is documented.

Validation:

- Defer full build to Stage 15.

## Stage 13 - Clean Scene Data, Mesh, Texture, Temporal Ownership

Goal:

- Make data flow from GameFramework to Renderer explicit and immutable.
- Stabilize supporting systems before deeper frame graph/pass work.

Source references:

- `rhi-renderer-architecture-review.md`: `Renderer -> GameFramework`, `Renderer Private Coverage`
- `architecture-review-acceptance-rubric.md`: `Modern C++ systems skill`, `Rendering fundamentals`

External implementation references:

- NVIDIA Donut scene/application organization: https://github.com/NVIDIA-RTX/Donut
- NVIDIA Falcor scene/rendering separation: https://github.com/NVIDIAGameWorks/Falcor

Code references:

- `Engine/Renderer/Private/SceneData`
- `Engine/Renderer/Private/Meshes`
- `Engine/Renderer/Private/Textures`
- `Engine/Renderer/Private/Temporal`
- `Engine/GameFramework`

Tutor note:

- What is wrong today: renderer scene input can become coupled to gameplay structures if we keep adding direct access.
- What changes: Renderer consumes render-domain snapshots and DTOs for things it needs to draw.
- Why it improves the engine: render snapshots make the renderer easier to test, cache, parallelize, and reason about independently from gameplay.

Implementation prompt:

```text
Using Donut's scene/application organization and Falcor's scene/rendering separation as references, review and tighten the render snapshot boundary. Ensure Renderer consumes render-domain DTOs for meshes, materials, cameras, lights, skinning, and temporal state. Document lifetime and ownership for mesh cache, material cache, texture manager, and temporal jitter. Remove direct gameplay internals from renderer paths where a render snapshot can carry the data.
```

Positive guardrails:

- Preserve current rendering behavior.
- Keep DTOs simple and immutable per frame.
- Tie mesh/texture/material diagnostics to smoke evidence.
- Document temporal jitter conventions because DLSS and debug modes depend on them.

Negative guardrails:

- Do not introduce broad data-copy churn without reason.
- Do not put RHI objects into GameFramework.
- Do not let material/texture loading become a source-import/cooking concern inside Renderer.

Legacy cleanup:

- Remove unused snapshot adapters or duplicated DTO paths after migration.
- Remove empty/private placeholder folders if they are not planned, especially `Renderer/Private/Denoising`, or document their exact purpose.

Acceptance:

- Scene data contract doc names every render-domain DTO and owner.
- Renderer no longer depends on gameplay internals where a snapshot should be used.
- Mesh, material, texture, and temporal diagnostics are represented in smoke/reporting plan.

Validation:

- Defer full build to Stage 15.

## Stage 14 - Harden Frame Graph Contract And Diagnostics

Goal:

- Make frame graph resource/barrier failures impossible to ignore.
- Preserve the strong frame graph architecture while making contracts explicit.

Source references:

- `rhi-renderer-architecture-review.md`: `Frame Orchestration And Pass Implementation Are Still Blurry`, `Phase 3: Frame Graph Contract Review`
- `architecture-review-acceptance-rubric.md`: `Runtime behavior clarity`, `Observability and diagnostics`, `Reliability/failure handling`

External implementation references:

- NVIDIA Falcor RenderGraph system: https://github.com/NVIDIAGameWorks/Falcor
- NVIDIA Donut reusable passes and graph-style orchestration: https://github.com/NVIDIA-RTX/Donut
- Vulkan validation tooling philosophy: https://docs.vulkan.org/guide/latest/development_tools.html

Code references:

- `Engine/Renderer/Private/FrameGraph`
- `Engine/Renderer/Private/FrameGraph/Execution/FrameGraphBarrierPlanPlayback.cpp`
- `Engine/Renderer/Private/FrameGraph/Diagnostics`
- `Engine/Renderer/Private/Frame`
- `Engine/Renderer/Private/Passes`

Tutor note:

- What is wrong today: frame graph unresolved resources can show up as warnings instead of hard contract failures.
- What changes: graph declaration, compile, transient planning, barrier playback, and execution get explicit contracts and diagnostics.
- Why it improves the engine: a frame graph is valuable only if it makes dependencies more reliable, not if it hides broken resources behind warnings.

Implementation prompt:

```text
Using Falcor RenderGraph, Donut pass orchestration, and Vulkan validation tooling as references, turn frame graph warnings for unresolved resources, unresolved aliasing barriers, invalid external resources, and incompatible resource usages into explicit development validation failures with diagnostic dumps. Document declare/compile/plan/allocate/execute contracts and ensure pass composition files only wire graph resources while pass implementation files own execution details.
```

Positive guardrails:

- Every warning class gets a contract explanation and remediation hint.
- Diagnostics must include pass name, resource handle, declared usage, resolved state, and physical allocation when relevant.
- Keep frame graph internals private.
- Make `Frame/*` orchestration naming consistent.

Negative guardrails:

- Do not suppress warnings to make smoke pass.
- Do not move pass-specific shader behavior into frame graph.
- Do not let transient aliasing become opaque again.

Legacy cleanup:

- Remove fallback paths that silently skip unresolved barriers/resources.
- Remove stale diagnostic code that cannot be triggered or tested.

Acceptance:

- Unresolved frame graph resources fail development smoke.
- Transient aliasing diagnostics explain physical block reuse.
- Frame composition versus pass execution ownership is documented and visible in names.

Validation:

- Defer full build/runtime smoke to Stage 15.

## Stage 15 - Validation Milestone C: Renderer Facade, Frame Pipeline, Frame Graph

Goal:

- Validate the renderer decomposition and frame graph contract before changing pass/PSO architecture.

External implementation references:

- NVIDIA NVRHI tutorial: https://github.com/NVIDIAGameWorks/nvrhi/blob/main/doc/Tutorial.md
- NVIDIA Donut Samples: https://github.com/NVIDIA-RTX/Donut-Samples
- Khronos Vulkan validation layers: https://github.com/KhronosGroup/Vulkan-ValidationLayers

Tutor note:

- What is wrong today: if renderer decomposition or graph validation breaks presentation, later pass/PSO work will be blamed incorrectly.
- What changes: this milestone proves the host facade, presentation bridge, frame pipeline, and frame graph are stable before deeper pipeline work.
- Why it improves the engine: validation milestones isolate risk so each large refactor has a trustworthy baseline.

Implementation prompt:

```text
Using NVRHI tutorial validation, Donut Samples, and Vulkan validation layers as references, build the editor/runtime targets and run D3D12/Vulkan smoke validation with lit and debug/normal view modes. Confirm frame graph diagnostics are clean, render products present correctly, and Application no longer owns renderer resource transitions.
```

Positive guardrails:

- Validate window resize/restore/maximize if smoke supports it.
- Validate shader reload if the current milestone touched pipeline/runtime ownership.
- Capture frame graph diagnostic output.

Negative guardrails:

- Do not move into PSO/pass runtime redesign with unresolved frame graph warnings.
- Do not accept editor viewport presentation if it only works through legacy transition helpers.

Suggested validation:

```powershell
cmake --build build --config DevelopmentEditor --target SparkleLauncher -- /nologo /v:minimal /m:1
cmake --build build --config DevelopmentEditor --target clang_format_check -- /nologo /v:minimal

$env:SPARKLE_SMOKE_VALIDATE_RHI='1'
$env:SPARKLE_SMOKE_FRAME_LIMIT='120'
$env:SPARKLE_SMOKE_SHADER_RELOAD_FRAME='60'
$env:SPARKLE_SMOKE_SCENE_COLOR_CAPTURE_FRAME='30'
$env:SPARKLE_RHI_BACKEND='D3D12'
$env:SPARKLE_SMOKE_VIEW_MODE='0'
$env:SPARKLE_SMOKE_SCENE_COLOR_CAPTURE='artifacts/validation/d3d12-stage15-lit.bmp'
# Launch editor/runtime smoke.

$env:SPARKLE_RHI_BACKEND='Vulkan'
$env:SPARKLE_SMOKE_VIEW_MODE='0'
$env:SPARKLE_SMOKE_SCENE_COLOR_CAPTURE='artifacts/validation/vulkan-stage15-lit.bmp'
# Launch editor/runtime smoke.
```

Acceptance:

- D3D12 and Vulkan smoke pass with no frame graph unresolved-resource warnings.
- Renderer public host API no longer exposes manual product transitions.
- Coverage status is updated.

## Stage 16 - Introduce Explicit PSO Key And Pipeline Runtime Library

Goal:

- Replace implicit pass-type runtime identity with an explicit PSO/runtime key model.
- Separate shader package loading, binding layout creation, validation, and PSO creation.

Source references:

- `rhi-renderer-architecture-review.md`: `Shader Pass And PSO Handling`, `Track 5`
- `architecture-review-acceptance-rubric.md`: `Shader and pipeline systems`, `Performance reasoning`, `Portability/backend parity`

External implementation references:

- Microsoft D3D12 pipelines and shaders docs: https://learn.microsoft.com/en-us/windows/win32/direct3d12/pipelines-and-shaders-with-directx-12
- Microsoft D3D12 pipeline state management docs: https://learn.microsoft.com/en-us/windows/win32/direct3d12/managing-graphics-pipeline-state-in-direct3d-12
- Diligent Core pipeline state object model: https://github.com/DiligentGraphics/DiligentCore

Code references:

- `Engine/Renderer/Private/Pipeline/PipelineStateManager.h`
- `Engine/Renderer/Private/Pipeline/RenderPassPipelineTraits.h`
- `Engine/Renderer/Private/Pipeline/RenderPassShaderRuntime.h`
- `Engine/RHI/Public/Pipeline/RhiPipelineStateDesc.h`
- `Engine/RHI/Private/D3D12/Pipeline`
- `Engine/RHI/Private/Vulkan/Pipeline`

Tutor note:

- What is wrong today: runtime identity is centered on pass C++ type and central traits, not a complete backend-visible PSO key.
- What changes: PSO identity becomes explicit, printable, and based on shader package, render state, formats, features, and backend.
- Why it improves the engine: PSO systems are easier to debug and cache when the key describes the actual GPU pipeline, not the C++ class that asked for it.

Implementation prompt:

```text
Using Microsoft D3D12 PSO documentation and Diligent Core's pipeline-state model as references, introduce a PipelineRuntimeLibrary with explicit PipelineKey/PsoKey structures. The key must include backend, shader package ID/generation/hash, pipeline kind, shader stages, render target formats, depth format, raster/depth/blend state, vertex layout, required feature/permutation bits, and binding layout identity. Separate shader package loading from PSO creation. Keep the old pass runtime path only until all passes are migrated, then delete it.
```

Positive guardrails:

- PSO keys are printable in logs.
- D3D12 and Vulkan receive equivalent normalized descriptors.
- Shader reload invalidates by package generation/hash where possible.
- Runtime errors name pass, package, backend, key fields, and suggested fix.

Negative guardrails:

- Do not use `std::type_index` as the final runtime key.
- Do not keep central trait specialization as the long-term ordinary-pass registration mechanism.
- Do not hide PSO variants in ad hoc lambdas.

Legacy cleanup:

- Mark `RenderPassPipelineTraits` and old lazy runtime storage as legacy at stage start.
- Delete them after Stage 17 migration.

Acceptance:

- New `PipelineRuntimeLibrary` exists.
- PSO key is explicit and logged.
- Runtime package loading and PSO creation are separate responsibilities.
- D3D12/Vulkan pipeline creation paths consume normalized descriptors.

Validation:

- Defer full build to Stage 20 unless compile issues block migration.

## Stage 17 - Introduce Declarative Pass Definition And Migrate Passes

Goal:

- Make adding a shader pass high-level and Renderer-owned.
- Remove central pass traits and duplicate shader package declarations.

Source references:

- `rhi-renderer-architecture-review.md`: `Target Shader Pass Model`, `Hard Gate: Renderer Shader Passes Must Not Require RHI Edits`
- `architecture-review-acceptance-rubric.md`: `Role relevance`, `Shader and pipeline systems`, `Maintainability and naming`

External implementation references:

- NVIDIA Falcor RenderPasses model: https://github.com/NVIDIAGameWorks/Falcor
- NVIDIA Donut reusable passes above NVRHI: https://github.com/NVIDIA-RTX/Donut
- Diligent Engine render state and pipeline abstractions: https://github.com/DiligentGraphics/DiligentEngine

Code references:

- `Engine/Renderer/Private/Passes`
- `Engine/Renderer/Private/Frame`
- `Engine/Renderer/Private/Pipeline`
- `Engine/Renderer/Public/ShaderParameters`
- `Engine/RHI/Public/Shaders`
- `Tools/Shaders/ShaderCompiler`

Tutor note:

- What is wrong today: adding a pass means touching too many central systems and understanding too much low-level ceremony.
- What changes: a pass definition becomes the high-level source of pass intent, resources, shader package, render state, and dispatch/draw behavior.
- Why it improves the engine: a regular shader pass is a renderer feature; the engine should make the common path simple while keeping the deep systems powerful.

Implementation prompt:

```text
Using Falcor RenderPasses, Donut reusable passes, and Diligent render-state abstractions as references, add a Renderer-owned RenderPassDefinition model that describes pass name, shader package, pipeline kind, render state, resources, dispatch/draw behavior, feature requirements, and binding behavior. Migrate one simple proof pass first, preferably VisualizeBuffers or ComputeClear. Then migrate all ordinary passes. Delete the old central RenderPassPipelineTraits path after migration.
```

Positive guardrails:

- Pass authoring should require one pass definition file and shader files for ordinary passes.
- Pass definitions should feed frame graph declaration and pipeline runtime lookup.
- Binding should be reflection-driven or generated where possible.
- Resource declarations should be close to pass intent.

Negative guardrails:

- Do not require RHI edits for ordinary renderer passes.
- Do not keep both old and new pass systems indefinitely.
- Do not make pass definitions backend-specific.
- Do not sacrifice diagnostics to reduce code.

Legacy cleanup:

- Delete `RenderPassPipelineTraits.h` once all passes migrate.
- Delete old duplicate `DescribeShaderPackage` paths if the pass definition becomes the source of truth.
- Remove dead binding override paths that no migrated pass uses.

Acceptance:

- All existing ordinary passes are migrated or explicitly classified as special cases.
- Adding a new simple compute pass requires no RHI edit and no central trait edit.
- Shader package ID and binding layout ID have one source of truth.
- Pass validation errors are actionable.

Validation:

- Defer full build/runtime validation to Stage 20.

## Stage 18 - Clean Ray Tracing Ownership And Contracts

Goal:

- Preserve the useful renderer/RHI ray tracing split while making lifetime and naming contractual.

Source references:

- `rhi-renderer-architecture-review.md`: `Ray Tracing Is Mostly Well-Bounded`, `RayTracing coverage rows`
- `architecture-review-acceptance-rubric.md`: `Graphics API fluency`, `Rendering fundamentals`, `Reliability/failure handling`

External implementation references:

- Microsoft DXR specification: https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html
- NVIDIA DXR tutorial: https://developer.nvidia.com/rtx/raytracing/dxr/dx12-raytracing-tutorial-part-1
- Khronos Vulkan ray tracing sample: https://github.com/KhronosGroup/Vulkan-Samples/blob/main/samples/extensions/ray_tracing_basic/README.adoc
- NVIDIA Vulkan ray tracing tutorial: https://nvpro-samples.github.io/vk_raytracing_tutorial_KHR/

Code references:

- `Engine/RHI/Public/RayTracing/RhiRayTracingDesc.h`
- `Engine/Renderer/Private/RayTracing`
- `Engine/Renderer/Private/Frame/RayTracingScene*`
- `Engine/Renderer/Private/Passes/DirectLightingPass.*`
- D3D12/Vulkan AS build implementations in command lists

Tutor note:

- What is wrong today: the renderer/RHI split is mostly good, but names and ownership around RT scene, TLAS frame data, pass services, and shadow data can blur.
- What changes: the lifetime from scene snapshot to BLAS cache, TLAS build, frame graph AS import, pass binding, and shader-visible data is made contractual.
- Why it improves the engine: ray tracing bugs are often ownership bugs; clear AS lifetime and API-neutral descriptors make both D3D12 and Vulkan easier to trust.

Implementation prompt:

```text
Using the DXR spec, NVIDIA DXR tutorial, Khronos Vulkan ray tracing sample, and NVIDIA Vulkan ray tracing tutorial as references, document and clean ray tracing ownership from scene snapshot to BLAS cache, TLAS build, frame graph AS registration, pass service binding, and shader-visible shadow data. Keep RHI ray tracing descriptors generic. Rename or split ambiguous types where ownership is unclear. Validate D3D12/Vulkan AS builds and ray-traced shadows after camera movement.
```

Positive guardrails:

- Renderer owns scene acceleration structure lifetime.
- RHI owns AS descriptors, prebuild info, scratch/result allocation primitives, and command-list build operations.
- Direct lighting/shadow data stays with renderer pass or shared render-data module, not RHI.
- Diagnostics report TLAS instance count, rejected meshes, missing GPU mesh data, and unsupported feature reasons.

Negative guardrails:

- Do not put shadow pass concepts in `RhiRayTracingDesc.h`.
- Do not include D3D12/Vulkan headers in renderer ray tracing code.
- Do not hide unsupported RT fallback behind black/noisy output.

Legacy cleanup:

- Remove old ambiguous helper names after replacements exist.
- Remove duplicated RT capability checks if a single capability report owns them.

Acceptance:

- Ray tracing contract doc matches code.
- D3D12/Vulkan shadow behavior is stable during camera rotation.
- RHI ray tracing public structs contain GPU/API concepts only.

Validation:

- Include in Stage 20 full visual parity.

## Stage 19 - Slim Backend Facades And Enforce D3D12/Vulkan Service Symmetry

Goal:

- Make backend implementation reviewable by service area instead of giant facade classes.
- Ensure D3D12 and Vulkan are separated, symmetric, and parity-driven.

Source references:

- `rhi-renderer-architecture-review.md`: `D3D12 Backend Coverage`, `Vulkan Backend Coverage`, `Backend Parity Matrix`
- `architecture-review-acceptance-rubric.md`: `Graphics API fluency`, `Portability/backend parity`, `Modern C++ systems skill`

External implementation references:

- NVIDIA NVRHI backend abstraction: https://github.com/NVIDIA-RTX/NVRHI
- NVIDIA NRI backend-oriented rendering interface: https://github.com/NVIDIA-RTX/NRI
- AMD Cauldron backend folders: https://github.com/GPUOpen-LibrariesAndSDKs/Cauldron
- Diligent Engine backend implementations: https://github.com/DiligentGraphics/DiligentEngine

Code references:

- `Engine/RHI/Private/D3D12/D3D12RenderHardwareInterface.cpp`
- `Engine/RHI/Private/Vulkan/VulkanRenderHardwareInterface.cpp`
- `Engine/RHI/Private/D3D12/Commands`
- `Engine/RHI/Private/Vulkan/Commands`
- `Engine/RHI/Private/D3D12/Descriptors`
- `Engine/RHI/Private/Vulkan/Descriptors`
- `Engine/RHI/Private/D3D12/Memory`
- `Engine/RHI/Private/Vulkan/Memory`
- `Engine/RHI/Private/D3D12/Pipeline`
- `Engine/RHI/Private/Vulkan/Pipeline`
- `Engine/RHI/Private/D3D12/Resources`
- `Engine/RHI/Private/Vulkan/Resources`

Tutor note:

- What is wrong today: D3D12 and Vulkan root facade files implement too many responsibilities, which makes backend parity hard to audit.
- What changes: backend responsibilities move into symmetric service areas for commands, descriptors, memory, pipeline, resources, diagnostics, swap chain, UI, and interop.
- Why it improves the engine: reviewers can compare D3D12 and Vulkan subsystem by subsystem instead of reading two giant backend facades.

Implementation prompt:

```text
Using NVRHI, NRI, Cauldron, and Diligent Engine backend organization as references, decompose D3D12RenderHardwareInterface and VulkanRenderHardwareInterface behind service-owned implementation objects. Preserve backend folder symmetry for commands, descriptors, memory, pipeline, resources, diagnostics, swap chain, UI, and interop. Make API-specific differences explicit in type conversion and service implementation files. Remove root-facade forwarding code when callers use services.
```

Positive guardrails:

- Keep backend-private code inside backend folders.
- Type conversion files should be total over public enum/desc values.
- Depth, winding, viewport, culling, and resource state/layout conventions must be documented and parity-tested.
- Service names should match across D3D12 and Vulkan where responsibilities match.

Negative guardrails:

- Do not move backend code into Renderer.
- Do not merge D3D12 and Vulkan implementation details into common code unless the abstraction is truly backend-neutral.
- Do not leave old facade methods forwarding forever.

Legacy cleanup:

- Delete obsolete root-facade methods after service migration.
- Delete unused backend helper methods and duplicate conversion code.
- Remove duplicated CMake source grouping if obsolete.

Acceptance:

- Backend root facade files are materially smaller and mostly wire services.
- D3D12/Vulkan service folders are symmetric where appropriate.
- Public RHI method map is updated to reflect remaining facade methods and services.

Validation:

- Include in Stage 20 full backend validation.

## Stage 20 - Validation Milestone D: Full Renderer/RHI Backend Parity

Goal:

- Validate the full architectural refactor before portfolio-facing cleanup.

External implementation references:

- NVIDIA Donut Samples executable graphics evidence: https://github.com/NVIDIA-RTX/Donut-Samples
- NVIDIA NVRHI tutorial: https://github.com/NVIDIAGameWorks/nvrhi/blob/main/doc/Tutorial.md
- Khronos Vulkan validation layers: https://github.com/KhronosGroup/Vulkan-ValidationLayers
- NVIDIA Streamline programming guide: https://github.com/NVIDIA-RTX/Streamline/blob/main/docs/ProgrammingGuide.md
- AMD FidelityFX SDK: https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK

Tutor note:

- What is wrong today: backend parity has been judged manually through editor launches and screenshots, which is useful but not enough for a review-ready repo.
- What changes: final renderer/RHI validation produces named evidence for build, smoke, captures, DLSS, RT, frame graph, and PSO runtime.
- Why it improves the engine: external reviewers trust repeatable evidence more than claims, especially for graphics bugs that can be machine-dependent.

Implementation prompt:

```text
Using Donut Samples, NVRHI tutorial validation, Vulkan validation layers, Streamline diagnostics, and FidelityFX-style feature reporting as references, run the full D3D12/Vulkan validation suite for build, shader compiler, launcher, smoke validation, lit captures, normal/debug captures, ray tracing, DLSS/upscaling, frame graph diagnostics, PSO runtime logs, and boundary checks. Record artifacts and compare against the architecture rubric.
```

Positive guardrails:

- Validate D3D12 and Vulkan in the same scene and camera path.
- Capture lit and normal/debug outputs.
- Check logs for PSO keys, shader package IDs, backend capability reports, DLSS provider status, RT capability status, and frame graph diagnostics.
- Keep exact commands and artifact paths.

Negative guardrails:

- Do not accept visual parity based only on memory.
- Do not ignore validation warnings that indicate contract drift.
- Do not hide known differences; document them with reason and owner.

Suggested validation:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config DevelopmentEditor --target ShaderCompiler -- /nologo /v:minimal /m:1
cmake --build build --config DevelopmentEditor --target SparkleLauncher -- /nologo /v:minimal /m:1
cmake --build build --config DevelopmentEditor --target clang_format_check -- /nologo /v:minimal

# Run D3D12 editor smoke lit.
$env:SPARKLE_SMOKE_VALIDATE_RHI='1'
$env:SPARKLE_SMOKE_FRAME_LIMIT='120'
$env:SPARKLE_SMOKE_SCENE_COLOR_CAPTURE_FRAME='30'
$env:SPARKLE_RHI_BACKEND='D3D12'
$env:SPARKLE_SMOKE_VIEW_MODE='0'
$env:SPARKLE_SMOKE_SCENE_COLOR_CAPTURE='artifacts/validation/d3d12-final-lit.bmp'
# Launch Showcase editor/runtime.

# Run Vulkan editor smoke lit.
$env:SPARKLE_RHI_BACKEND='Vulkan'
$env:SPARKLE_SMOKE_VIEW_MODE='0'
$env:SPARKLE_SMOKE_SCENE_COLOR_CAPTURE='artifacts/validation/vulkan-final-lit.bmp'
# Launch Showcase editor/runtime.

# Run D3D12 and Vulkan normal/debug captures with the appropriate RenderViewMode value.
```

Acceptance:

- Build targets pass.
- Boundary checks pass with no architecture exceptions except documented provider-only SDK integration if still required.
- D3D12/Vulkan smoke runs pass.
- Frame graph unresolved-resource warnings are zero.
- Normal/debug view modes work for both backends.
- Lit output is within agreed tolerance or differences are documented.
- DLSS is active when requested and supported, or fallback reason is deterministic.
- RT/shadow behavior is stable under camera rotation.
- Coverage status is updated.

## Stage 21 - Portfolio And Repository Review Presentation

Goal:

- Make the repo understandable and impressive to an external NVIDIA/AMD-style reviewer.

Source references:

- `architecture-review-acceptance-rubric.md`: `Portfolio Review Skill Signals`, `Sparkle Portfolio Acceptance Checklist`
- `rhi-renderer-architecture-review.md`: `Definition Of Done For This Review Track`

External implementation references:

- GitHub portfolio guidance: https://flatironschool.com/blog/github-profile-and-git-practices-for-job-seekers/
- README guidance: https://coding-boot-camp.github.io/full-stack/github/professional-readme-guide/
- NVIDIA Donut repository presentation: https://github.com/NVIDIA-RTX/Donut
- NVIDIA Falcor repository presentation: https://github.com/NVIDIAGameWorks/Falcor
- AMD Cauldron repository presentation: https://github.com/GPUOpen-LibrariesAndSDKs/Cauldron

Code/doc references:

- Top-level `README.md` currently appears absent.
- `docs/architecture`
- `docs/plans`
- `Tools/Launcher/SparkleLauncher`
- validation artifacts from Stage 20

Tutor note:

- What is wrong today: even good engine code is hard to evaluate if the repo lacks an entry point, feature matrix, reviewer path, and validation instructions.
- What changes: README and reviewer docs make the technical work inspectable without insider guidance.
- Why it improves the engine: presentation is not vanity; it is how a reviewer verifies your engineering judgment quickly and fairly.

Implementation prompt:

```text
Using GitHub portfolio/README guidance and the repo presentation style of Donut, Falcor, and Cauldron as references, create a reviewer-facing repository entry point. Add a top-level README with technical scope, architecture diagram, feature matrix, build/launch commands, D3D12/Vulkan status, validation commands, screenshots/captures, known issues, and a reviewer's path through the most representative code. Add CONTRIBUTING or equivalent review/commit guidance, bug report guidance, and links to architecture docs and final validation artifacts.
```

Positive guardrails:

- Show role-relevant skills: C++20 systems, D3D12/Vulkan, frame graph, shader compiler/cook, PSO runtime, ray tracing, DLSS/upscaling, diagnostics, validation.
- Include honest known issues.
- Point reviewers to exact files and docs.
- Make build commands copy-pasteable.
- Include commit/PR conventions for large rendering changes.
- Include bug report or issue guidance for graphics validation failures.
- Link license/status information if it exists, or add a known gap if it does not.

Negative guardrails:

- Do not write marketing copy that hides unfinished areas.
- Do not claim performance wins without measurements.
- Do not include machine-local paths as durable instructions.
- Do not make repo hygiene and collaboration readiness optional after the renderer code is done.

Legacy cleanup:

- Remove stale docs and references to deleted plan files.
- Remove generated artifacts from source-controlled docs unless they are intentionally curated assets.

Acceptance:

- Top-level README exists and passes the portfolio checklist.
- Reviewer can find build commands, launch commands, architecture docs, feature matrix, screenshots/captures, and known issues from the repo root.
- Reviewer can find contribution/review guidance, commit/PR expectations, bug report guidance, and validation artifact expectations.
- Docs mention final validation artifacts and current backend status.

Validation:

- Docs/link check by `rg` and manual open/read.
- No runtime build required unless README commands are verified in Stage 22.

## Stage 22 - Final Cleanup, Rubric Scoring, And Review-Ready Gate

Goal:

- Finish clean.
- Delete legacy paths.
- Score the repo against the acceptance rubric at a high bar.

Source references:

- `architecture-review-acceptance-rubric.md`: all criteria and score scale
- `rhi-renderer-architecture-review.md`: `Definition Of Done For This Review Track`
- This execution plan: every stage acceptance section

External implementation references:

- arc42 quality and risk documentation: https://arc42.org/overview
- ADR practice: https://adr.github.io/
- Michael Nygard ADR article: https://www.cognitect.com/blog/2011/11/15/documenting-architecture-decisions
- CMU SEI ATAM: https://www.sei.cmu.edu/library/architecture-tradeoff-analysis-method-collection/

Tutor note:

- What is wrong today: a long refactor can finish with small temporary paths, stale docs, and weak criteria still hanging around.
- What changes: final cleanup deletes contradictions, scores the rubric, and collects evidence before calling the repo review-ready.
- Why it improves the engine: "done" means the architecture, code, docs, and validation all agree with each other.

Implementation prompt:

```text
Using arc42, ADR practice, and ATAM-style quality review as references, perform final cleanup and review-readiness scoring. Remove transitional adapters, stale allowlist entries, dead files, duplicate docs, obsolete CMake references, and unused legacy code. Score every acceptance rubric category. Do not mark the project review-ready unless all critical categories score 3 or have an explicitly accepted reason and no category scores below 2.
```

Positive guardrails:

- Treat cleanup as mandatory work, not optional polish.
- Run final boundary checks and builds after cleanup.
- Update all docs to match final code.
- Keep a final evidence index with commands, logs, captures, and screenshots.

Negative guardrails:

- Do not leave legacy code "temporarily" after final scoring.
- Do not accept weak rubric scores because the refactor took a long time.
- Do not hide remaining risks; either fix them or document them as non-blocking with owner and reason.

Legacy cleanup:

- Delete old pass runtime/traits path after pass migration.
- Delete old RHI shader registration files for renderer passes.
- Delete stale presentation transition helpers.
- Delete temporary boundary-check exceptions.
- Delete unused native interop fields.
- Delete empty or unexplained folders.
- Delete stale docs or references to deleted docs.

Final validation:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config DevelopmentEditor --target ShaderCompiler -- /nologo /v:minimal /m:1
cmake --build build --config DevelopmentEditor --target SparkleLauncher -- /nologo /v:minimal /m:1
cmake --build build --config DevelopmentEditor --target clang_format_check -- /nologo /v:minimal
```

Final acceptance:

- All critical rubric categories score 3 unless a documented, accepted exception exists.
- No rubric category scores below 2.
- Coverage status has no unowned `Needs refactor` rows.
- Boundary checks pass.
- D3D12 and Vulkan smoke evidence exists.
- Lit and normal/debug captures exist for both backends.
- DLSS/RT/frame graph/PSO state is visible in validation evidence.
- README and docs let an external reviewer understand and validate the repo.
- The repo has no known legacy code path contradicting the final architecture.

## Final Review-Ready Definition

SparkleEngine is review-ready for the targeted renderer/RHI scope only when all of these are true:

- RHI does not depend on Renderer.
- Renderer does not own backend-native API details outside documented provider integration.
- Application does not own backend-native validation/capture implementation.
- Adding an ordinary renderer shader pass does not require RHI edits.
- `Renderer` is a facade/host boundary, not a giant orchestration hub.
- `RenderHardwareInterface` is categorized and slimmed behind service responsibilities.
- Frame graph contract failures are actionable and fail development smoke.
- PSO creation is keyed, logged, and backend-normalized.
- D3D12 and Vulkan backend services are separated, symmetric where appropriate, and parity-tested.
- Ray tracing ownership is clear from scene data to TLAS binding and pass usage.
- DLSS/upscaling interop is provider-owned and backend-supported through explicit contracts.
- Validation artifacts prove lit and debug/normal parity across D3D12 and Vulkan.
- Repository docs make the system navigable for an external graphics reviewer.

# B. SparkleEngine Architecture Scorecard

Status: first-pass internal architecture review  
Date: 2026-06-20  
Scope: Renderer, RHI, GameFramework, Application, Editor, runtime tooling, shader/cook/build foundations

## Scoring Rubric

| Score | Meaning |
| --- | --- |
| 5 | Reviewer-ready. Clear contracts, validation, diagnostics, docs, and known extension path. |
| 4 | Strong foundation. Mostly reviewable, with focused documentation or validation gaps. |
| 3 | Functional and promising, but important behavior is implicit or scattered. |
| 2 | Feature exists, but ownership, extension, or failure modes are hard to reason about. |
| 1 | Present only as early scaffolding or isolated implementation. |
| 0 | Missing. |

## Summary Scores

| Area | Score | Readiness summary |
| --- | ---: | --- |
| Build/module boundaries/architecture guardrails | 4.0 | Strong executable boundaries; needs human-facing docs and ADR links. |
| RHI public contract | 3.8 | Good target split and service shape; needs explicit lifetime/barrier/descriptor/queue docs. |
| D3D12 backend | 3.7 | Good backend isolation, D3D12MA, NVAPI placement; needs debug/perf/memory telemetry docs. |
| Vulkan backend | 3.2 | Real optional backend path; needs parity, validation, capture, and RT feature docs. |
| Renderer frame graph/pipeline/passes | 3.6 | Good module shape; pass lifecycle and resource contracts need formalization. |
| SDK/upscaling/provider integration | 3.4 | DLSS/Streamline isolation is promising; provider-neutral contracts are missing. |
| Shader compiler/contracts/cook pipeline | 4.0 | Strongest portfolio area; needs ABI docs and regression/golden path. |
| GameFramework runtime data layer | 3.8 | Good renderer/RHI separation; needs data ownership, mutation, and snapshot docs. |
| Application runtime/editor host | 3.5 | Good split; lifecycle and error taxonomy need documentation. |
| Editor tooling | 3.3 | Useful panels exist; needs diagnostic data source model and reviewer dashboard. |
| Launcher/dependency/workflows | 3.7 | Strong workflow surface; needs reviewer workflows and dependency policy docs. |
| Validation/tests/performance evidence | 3.2 | Boundary checks exist; needs conformance, backend parity, perf baselines. |
| Documentation/portfolio presentation | 1.8 | Architecture is better than its review surface. This is the biggest gap. |

## 1. Build, Module Boundaries, And Architecture Guardrails

Score: 4.0 / 5

Evidence:

- `CMake/ArchitectureBoundaryCheck.cmake` scans architecture boundaries.
- RHI is prevented from including Renderer-private headers.
- Renderer native D3D12/Vulkan use is blocked except counted NVIDIA provider exceptions.
- D3D12 and Vulkan backend cross-contamination is checked.
- Application validation is guarded from backend-native dependencies.

Gaps:

- Boundary checks need human docs explaining the architecture they enforce.
- Counted exceptions need ADRs or provider-contract links.
- Reviewer path should expose boundary checks as a first-class validation action.

Next work:

- Add architecture map and boundary rule documentation.
- Add ADR entries for native API exceptions.
- Surface boundary checks in launcher/build validation.

## 2. RHI Public Contract

Score: 3.8 / 5

Evidence:

- RHI has common contracts plus D3D12/Vulkan backend-private implementations.
- Public surface includes commands, device, diagnostics, interop, memory, pipelines, presentation, ray tracing, resources, shader parameters, shaders, UI, and validation.
- D3D12 uses D3D12MA and backend-private NVAPI.
- Vulkan uses VMA when SDK/backend are available.
- GPU validation/live object reporting are configuration-gated.

Gaps:

- Resource state and barrier ownership need a written contract.
- Descriptor heap/pool ownership and lifetime need documentation.
- Queue/fence/command-list threading assumptions are not yet visible.
- Native handle escape hatches need policy and failure states.
- D3D12/Vulkan parity matrix is missing.

Next work:

- Write `RHIContract.md`.
- Add backend parity table.
- Add new backend and new interop checklists.

## 3. D3D12 Backend

Score: 3.7 / 5

Evidence:

- Backend-private target links D3D12, DXGI, DXGUID, D3DCompiler, D3D12MA, and optional NVAPI.
- NVAPI is restricted to the RHI backend layer.
- Development/shipping validation definitions are separated.

Gaps:

- Debug layer, GPU-based validation, DRED, PIX naming/capture, and live-object reporting need documentation.
- Residency and memory budget telemetry need a visible route.
- Queue and fence lifecycle should be diagrammed.
- NVAPI feature reports should flow through backend-neutral RHI capabilities.

Next work:

- Add D3D12 backend note.
- Add object naming and capture checklist.
- Add residency/budget counters before memory-heavy features arrive.

## 4. Vulkan Backend

Score: 3.2 / 5

Evidence:

- Vulkan backend target is gated by SDK discovery.
- Vulkan Memory Allocator is used when backend is enabled.
- Vulkan absence is now surfaced where backend/build readiness matters.

Gaps:

- D3D12/Vulkan feature parity is not documented.
- Validation layers, debug utils naming, and capture tooling need docs.
- Ray tracing and advanced feature support should be matrixed.

Next work:

- Add Vulkan backend note parallel to D3D12.
- Add validation/capture recipes.
- Add backend capability matrix.

## 5. Renderer Frame Graph, Frame Pipeline, And Passes

Score: 3.6 / 5

Evidence:

- Renderer is separated from import/conversion/authoring dependencies.
- Private layout includes frame graph, frame pipeline, passes, scene data, temporal, ray tracing, textures, diagnostics, and settings.
- Public layout exposes renderer entrypoints, frame graph handles/descriptors, scene data, shader parameters, diagnostics, settings, and viewport contracts.

Gaps:

- Pass lifecycle is not documented.
- Resource import/export rules between Renderer and RHI need to be visible.
- Persistent versus per-frame resource ownership is not obvious.
- Frame graph scheduling, barriers, aliasing, and diagnostics need a model.

Next work:

- Write `RendererFrameGraph.md`.
- Add resource contract table for color, depth, normals, motion vectors, exposure, history, jitter, frame index, and camera matrices.
- Add pass authoring checklist.

## 6. SDK, Upscaling, And Provider Integration

Score: 3.4 / 5

Evidence:

- NVIDIA DLSS provider sources are isolated in a dedicated provider target.
- Streamline linkage is conditional.
- Vulkan linkage is restricted to the NVIDIA provider and counted by architecture checks.
- Launcher dependency sync is becoming hardware-aware.

Gaps:

- Provider-neutral interface for upscalers, denoisers, frame generation, or neural features is missing.
- Required input resources are not centralized.
- Capability states need to separate missing dependency, unsupported hardware, available, enabled, and runtime-failed.
- AMD FidelityFX/Cauldron-style readiness should be represented before adding FidelityFX features.

Next work:

- Write `RendererProviderContract.md`.
- Move SDK assumptions behind provider capability structs.
- Treat Streamline/DLSS as one provider, not the whole architecture.

## 7. Shader Compiler, Shader Contracts, And Cook Pipeline

Score: 4.0 / 5

Evidence:

- ShaderCompiler uses DXC and Slang SDKs.
- It links SPIR-V reflection, RHI, and renderer shader registrations.
- Shader contracts, reflection, cooking, cache keys, include closure handling, and verification code exist.
- Renderer shader registrations are isolated as an object target.

Gaps:

- Shader ABI needs a compact reviewer document.
- Registration, reflection output, cooked package layout, and RHI pipeline creation should be diagrammed.
- Golden tests and regression corpus need to be visible.
- Feature/profile matrix is missing.

Next work:

- Write `ShaderPipeline.md`.
- Add shader feature capability matrix.
- Add reviewer commands for compile, inspect, and verify.

## 8. GameFramework Runtime Data Layer

Score: 3.8 / 5

Evidence:

- GameFramework owns assets, cooked asset types, levels, scenes, camera, lighting, materials, meshes, skeletons, and textures.
- It depends on Core and Platform, not Renderer/RHI.
- Renderer consumes runtime scene/material data without owning import/conversion.

Gaps:

- Data layout, ownership, threading, and update frequency need documentation.
- Scene snapshot/update rules need a document.
- Asset versioning and cooked compatibility should be explicit.

Next work:

- Write `RuntimeSceneData.md`.
- Add mutation/snapshot rules.
- Add cooked asset compatibility notes.

## 9. Application Host And Runtime/Editor Split

Score: 3.5 / 5

Evidence:

- Application has runtime and editor hosts.
- Runtime target excludes editor-only validation/recook files.
- Editor app target includes shader recook and validation.
- Public application API is small.

Gaps:

- Startup/frame/shutdown lifecycle needs a diagram.
- Error taxonomy should distinguish missing SDK, unsupported hardware, invalid project, shader cook failure, backend failure, and validation failure.
- Capture/replay/smoke validation commands should be stable.

Next work:

- Write `ApplicationLifecycle.md`.
- Add structured error categories.
- Add smoke validation recipes.

## 10. Editor Tooling

Score: 3.3 / 5

Evidence:

- Editor has viewport, hierarchy, profiler, console, assets, shader resources, rendering settings, used meshes/shaders/textures, and inspection panels.
- Editor directly depends on RHI and Renderer, which is appropriate for tool inspection.

Gaps:

- Principal-review dashboard is missing.
- Diagnostic panel data sources need to be documented.
- Capture buttons/links should map to backend capture flows.

Next work:

- Write `EditorDiagnostics.md`.
- Add stable diagnostics model structs before more UI panels.

## 11. Launcher, Dependency Sync, And Workflow Actions

Score: 3.7 / 5

Evidence:

- Launcher has tool resolution, CMake generator modeling, workflow operations, dependency UI models, build/cook/clean actions, GPU-aware source sync, and status pages.
- NVIDIA dependencies can be hardware-gated.
- Vulkan SDK absence is surfaced where backend selection/build readiness matters.

Gaps:

- Reviewer workflows need one-click or one-command equivalents.
- Dependency decisions need policy: required, optional, hardware-gated, backend-gated, provider-gated.
- Architecture checks should be a visible validation action.

Next work:

- Write `LauncherWorkflowReadiness.md`.
- Map every workflow action to a CLI command or documented operation.
- Surface architecture boundary checks.

## 12. Validation, Tests, And Performance Evidence

Score: 3.2 / 5

Evidence:

- Architecture boundary check exists.
- Application validation and RHI smoke validation paths exist in source layout.
- Shader compiler verification/cook inspection exists.

Gaps:

- RHI conformance tests should be explicit per backend.
- Golden image tests, shader package tests, and backend parity tests need a plan.
- Performance regression tracking is not obvious.
- Memory and descriptor pressure tests should exist before heavier renderer/neural features.

Next work:

- Write `ValidationMatrix.md`.
- Add perf baselines: empty frame, one mesh, many materials, descriptor pressure, upload pressure, shader compile cache hit/miss.

## 13. Documentation And Portfolio Presentation

Score: 1.8 / 5

Evidence:

- Module CMake comments are useful.
- Architecture checks encode rules.
- There was no dedicated architecture docs hierarchy before this review.

Gaps:

- Reviewers should not have to discover architecture by grep.
- The portfolio story needs 10-minute, 30-minute, and deep-dive paths.
- Tradeoffs and non-goals need to be written down.

Next work:

- Add architecture README.
- Add core contract docs.
- Add reviewer guide.
- Add limitations page.


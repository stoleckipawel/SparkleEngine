# C. Foundation Staged Plan

Status: first-pass execution plan  
Date: 2026-06-20  
Scope: staged foundation work before adding major renderer, SDK, ray tracing, or neural rendering features

Implementation prompts for each staged work item are maintained in `Docs/Architecture/00-Review/D_ImplementationPrompts.md`. Use those prompts when delegating the work to weaker models; they include explicit context files, constraints, deliverables, validation steps, and acceptance criteria.

## Planning Principle

Do not add feature mass yet. First make the engine easier to extend, harder to break, and easier for a principal NVIDIA/AMD reviewer to navigate.

The next work should improve:

- Contracts.
- Boundaries.
- Validation.
- Diagnostics.
- Performance visibility.
- SDK/provider arrival points.
- Reviewer workflow.

## Stage 0: Make The Existing Architecture Legible

Goal: reviewers can navigate the engine without oral context.

Actions:

- Add `Docs/Architecture/README.md` with module map and dependency direction.
- Add short module briefs for RHI, Renderer, GameFramework, Application, Editor, Launcher, ShaderCompiler, and AssetCooker.
- Convert architecture boundary rules into documented human-facing rules.
- Add an ADR index for counted boundary exceptions.
- Add a clear non-goals section: not a finished game editor, not Unreal, not a feature zoo.

Exit criteria:

- A reviewer can open one README and know where to inspect RHI, renderer frame graph, shader compilation, SDK integration, and validation.

Recommended first commits:

- Architecture README.
- Boundary rule documentation.
- ADR folder with first entry for NVIDIA DLSS/Streamline native provider exceptions.

## Stage 1: Write The Core Contracts

Goal: extension points are clear before features arrive.

Actions:

- Write `Docs/Architecture/02-Contracts/RHIContract.md`.
- Write `Docs/Architecture/02-Contracts/RendererFrameGraph.md`.
- Write `Docs/Architecture/02-Contracts/RendererProviderContract.md`.
- Write `Docs/Architecture/02-Contracts/ShaderPipeline.md`.
- Write `Docs/Architecture/02-Contracts/RuntimeSceneData.md`.
- Write `Docs/Architecture/02-Contracts/ApplicationLifecycle.md`.
- Add backend, provider, pass, shader feature, and validation checklists.

Exit criteria:

- Adding a backend, pass, SDK provider, shader feature, or runtime validation path has a documented route and owner boundaries.

Required contract topics:

| Contract | Must answer |
| --- | --- |
| RHI | Who owns devices, queues, command lists, fences, descriptors, resources, barriers, pipeline state, native handles, diagnostics, and interop? |
| Renderer frame graph | How does a pass declare resources, dependencies, persistent state, transient state, timing scopes, and validation? |
| Provider integration | How do SDK providers query capability, receive RHI handles/resources, report failure, and expose debug state? |
| Shader pipeline | How does shader source become reflected, validated, cooked, loaded, and bound? |
| Runtime scene data | What is mutable, what is snapshotted, what is renderer-owned, and what is asset-owned? |
| Application lifecycle | What happens during startup, tool discovery, backend creation, project load, frame loop, shutdown, and failure? |

## Stage 2: Turn Boundaries Into Continuous Evidence

Goal: architecture is hard to break accidentally.

Actions:

- Promote architecture boundary check to launcher/build workflow visibility.
- Add RHI backend parity matrix.
- Add validation matrix with command, backend coverage, artifact output, and failure owner.
- Add shader compiler golden/reflection regression cases.
- Add smoke validation for runtime/editor startup.
- Add generate/build/cook workflow validation.

Exit criteria:

- "It still works" can be shown by commands, artifacts, and backend-specific pass/fail status.

Validation categories:

- Architecture boundary validation.
- RHI backend creation and teardown.
- Swapchain/presentation smoke.
- Shader compile/reflection/cook verification.
- Renderer frame smoke.
- Runtime project load.
- Editor startup.
- Dependency/generator/tool resolution.
- Backend availability reporting.

## Stage 3: Add Performance And Memory Review Surfaces

Goal: principal-level performance thinking is visible before new features add complexity.

Actions:

- Add memory budget/residency reporting in RHI.
- Add descriptor heap/pool pressure reporting.
- Add pipeline cache and shader package load timing.
- Add GPU timestamp scopes per renderer pass.
- Add CPU frame orchestration timings.
- Surface active backend, adapter, feature level/API version, enabled providers, and validation state.
- Emit diagnostics both in editor panels and text artifacts.

Exit criteria:

- A reviewer can see frame cost, pass cost, memory budget, descriptor pressure, shader/pipeline cache behavior, active backend, and active SDK providers.

Baseline scenarios:

- Empty frame.
- One static mesh/material.
- Many materials.
- Descriptor pressure.
- Upload pressure.
- Shader compile cache miss.
- Shader compile cache hit.
- Backend startup/shutdown.

## Stage 4: Prepare SDK And Neural Rendering Arrival Points

Goal: advanced SDK/neural features can be added without reshaping core modules.

Actions:

- Introduce provider-neutral capability states:
  - unavailable
  - missing dependency
  - unsupported hardware
  - available
  - enabled
  - runtime failed
- Centralize provider resource contracts:
  - color
  - depth
  - motion vectors
  - exposure
  - normals
  - history
  - jitter
  - camera matrices
  - frame index
- Add Slang/DXC/profile feature matrix.
- Add future extension notes for cooperative vectors, tensor-like paths, neural materials, neural texture compression, denoisers, and upscalers.
- Keep feature implementations out of this stage unless they are needed to prove the contract.

Exit criteria:

- DLSS, FidelityFX, denoisers, ray tracing experiments, or neural shading can be added as providers/passes against known contracts.

Review rule:

- Streamline/DLSS should remain one implementation of the provider model, not the architecture that all future SDKs must imitate.

## Stage 5: Package The Portfolio Review Path

Goal: the engine can be reviewed under time pressure.

Actions:

- Add 10-minute review path.
- Add 30-minute review path.
- Add deep-dive review path.
- Add limitations page.
- Add "why this architecture" summary.
- Add commands for generate, build all, cook all, boundary check, shader inspection, and runtime smoke.

10-minute path:

- Open architecture README.
- Run boundary check.
- Generate/build/cook.
- Run one runtime smoke path.
- Inspect one shader package.

30-minute path:

- RHI contract.
- Renderer frame graph/pass authoring.
- Shader pipeline.
- Provider contract.
- Diagnostics dashboard.
- Validation matrix.

Deep-dive path:

- Backend implementation.
- Memory/descriptor model.
- Queue/fence/barrier behavior.
- Frame graph validation.
- Shader ABI.
- SDK integration and capability handling.
- Performance artifacts.

Exit criteria:

- The repo itself answers "why should I trust this architecture?" before a reviewer needs to ask.

## Immediate Execution Order

1. Add architecture README and module dependency map.
2. Add RHI contract.
3. Add renderer frame graph/pass contract.
4. Add shader pipeline/ABI document.
5. Add provider contract for SDK/upscaler/denoiser/neural arrivals.
6. Add validation matrix.
7. Add performance/memory diagnostics plan.
8. Add reviewer guide.

Prompt mapping:

| Work item | Prompt |
| --- | --- |
| Architecture README and module map | `Prompt 01: Architecture README And Module Map` |
| Boundary rules and ADR index | `Prompt 02: Architecture Boundary Rules And ADR Index` |
| RHI contract | `Prompt 03: RHI Contract` |
| Renderer frame graph/pass contract | `Prompt 04: Renderer Frame Graph And Pass Contract` |
| Provider contract | `Prompt 05: Renderer Provider Contract` |
| Shader pipeline/ABI document | `Prompt 06: Shader Pipeline And ABI Document` |
| Runtime scene data contract | `Prompt 07: Runtime Scene Data Contract` |
| Application lifecycle/error taxonomy | `Prompt 08: Application Lifecycle And Error Taxonomy` |
| Validation matrix | `Prompt 09: Validation Matrix` |
| Performance/memory diagnostics plan | `Prompt 10: Performance And Memory Diagnostics Plan` |
| Reviewer guide | `Prompt 11: Reviewer Guide` |
| Launcher workflow readiness | `Prompt 12: Launcher Workflow Readiness` |

## Do Not Do Yet

- Do not add major new rendering features before contracts exist.
- Do not add FidelityFX or more NVIDIA SDK features before provider contracts exist.
- Do not add neural rendering implementation before shader/profile/capability extension points are documented.
- Do not grow editor UI panels before diagnostics data ownership is stable.
- Do not hide missing SDK/hardware states behind generic "optional dependency" messaging.

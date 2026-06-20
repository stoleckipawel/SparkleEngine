# Principal Rendering Architecture Readiness

Status: first-pass architecture review  
Date: 2026-06-20  
Scope: RHI, renderer, game framework, application host, editor, runtime tools, shader/cook/build foundations  
Non-goal: adding new rendering features before the engine foundation is easier to extend, harder to break, and easier to review

This is the combined audit. The requested A/B/C documents are split here:

- `Docs/Architecture/A_PrincipalRoleRequirements.md`
- `Docs/Architecture/B_EngineArchitectureScorecard.md`
- `Docs/Architecture/C_FoundationStagedPlan.md`

## Why This Exists

SparkleEngine is being prepared for principal-level portfolio review for low-level hardware-aware rendering, GPU SDK, high-performance compute, GPU architecture, rendering, and neural rendering roles, with NVIDIA and AMD as explicit target reviewers.

The target is not "more features." The target is a codebase where a senior graphics reviewer can quickly answer:

- Where are the low-level API boundaries?
- How are memory, barriers, descriptors, queues, and pipelines owned?
- How does a new rendering pass, backend, upscaler, denoiser, ray tracing feature, or neural rendering path arrive without destabilizing the rest of the engine?
- How are platform, SDK, and hardware capabilities detected and reported?
- How are correctness, performance, and architecture boundaries checked?
- Which tradeoffs were intentional?

## External Reference Signal

The reference projects and role requirements point to the same pattern: reviewers reward explicitness, modularity, diagnostic clarity, and evidence that the author understands what should stay hidden versus what must remain controllable.

### Role Requirement Clusters

| Cluster | What principal-level reviewers look for | Engine implication |
| --- | --- | --- |
| Explicit graphics API control | Modern D3D12/Vulkan knowledge, manual responsibility for resource state, descriptors, synchronization, memory, queues, and GPU/CPU lifetime. AMD's graphics programmer guidance emphasizes that D3D12/Vulkan move more responsibility from driver to application and expose lower-level GPU control. | RHI contracts must describe exact ownership and lifetime rules. Hidden automation must be intentional and bypassable where performance/debugging requires it. |
| Cross-IHV and SDK integration | NVIDIA/AMD roles repeatedly ask for modern graphics APIs, GPU architecture knowledge, performance analysis, and SDK integration. Recognized SDK samples isolate vendor integrations and keep feature capability checks explicit. | NVIDIA and AMD integrations should live behind provider interfaces, capability models, and documented resource contracts. The renderer should not grow native API dependencies casually. |
| Render architecture | Donut is a reusable rendering framework with render passes and a scene graph, not a full game engine. Cauldron is designed as an easy-to-extend D3D12/Vulkan prototyping framework with resource management, glTF, skinning, PBR, postprocessing, and FidelityFX hosting. | Sparkle should make the same separation obvious: runtime scene data, renderer frame orchestration, RHI services, and toolchain should be independently navigable. |
| RHI design discipline | NVRHI provides D3D11/D3D12/Vulkan abstraction, lifetime tracking, optional automatic barriers, binding abstractions, native escape hatches, and upload buffer suballocation. NRI stresses explicitness, low overhead, low-level coverage, modularity, and avoids hidden high-level management. | Sparkle's RHI should state which model it follows per subsystem: explicit, tracked, or helper-assisted. Escape hatches must be named and bounded. |
| Shader/compiler pipeline | Neural rendering and SDK roles increasingly mention Slang, HLSL/SM6, Vulkan/D3D12, DXC, SPIR-V/DXIL, shader reflection, and training/inference awareness. NVIDIA RTX Neural Shading examples center Slang plus D3D/Vulkan preview features. | Shader compilation, reflection, cooked package ABI, and backend feature gates are portfolio-critical. This area should be documented like a product, not treated as build plumbing. |
| Performance and diagnostics | Principal-level GPU reviewers expect profiling, memory budgets, capture paths, GPU validation, benchmark discipline, and ability to reason from counters to code. AMD ML/GPU performance roles mention quantization, GPU kernels, memory/cache behavior, and low-level ISA awareness. | The engine needs visible perf budgets, capture recipes, memory telemetry, pass timings, and regression hooks before feature expansion. |
| Ray tracing and neural readiness | NVIDIA roles and SDKs call out ray tracing, neural rendering, advanced rendering, cooperative vectors/tensor paths, denoisers, upscalers, and resource contracts like motion vectors/depth/history. | Sparkle should prepare clean extension points for AS build/update, denoiser/upscaler/neural providers, feature capability negotiation, and shader model/Slang profile selection. |
| Production reviewability | A portfolio engine is judged by the first 30 minutes as much as by deep code. Reviewers need maps, contracts, build paths, validation commands, and decisions that explain why the architecture exists. | Add reviewer guides, architecture diagrams, scorecards, ADRs, checklists, and CI-visible boundary checks. |

### Reference Project Patterns To Borrow

| Reference | Recognized pattern | How Sparkle should use it |
| --- | --- | --- |
| AMD Cauldron | Small, extensible rendering framework for D3D12/Vulkan samples and FidelityFX integration. Clear sample/framework split and pragmatic resource management. | Keep Sparkle's feature arrivals sample/provider-friendly. Do not let experimental SDK integration leak into core renderer/RHI contracts. |
| NVIDIA Donut | Rendering framework with reusable passes, scene graph, NVRHI-backed API portability, and explicit "not a full game engine" scope. | Keep renderer, scene data, and game framework boundaries crisp. Make pass registration, pass resources, and scene snapshots reviewable. |
| NVIDIA NVRHI | Backend abstraction with resource/pipeline/descriptor/barrier helpers, optional automatic tracking, lifetime management, and native access for API-specific features. | Decide and document where Sparkle provides automation versus explicit control. Backend-native access should be intentional and audited. |
| NVIDIA NRI | Lower-level RHI that prioritizes explicitness, low overhead, all low-level API features, and avoids hidden management. | For hardware-aware work, Sparkle should preserve a direct mental model of command lists, queues, resource state, descriptors, and synchronization. |
| NVIDIA Streamline | Plugin framework between game and render API; features require resource tagging and correct pipeline placement. | Sparkle needs a provider-neutral resource contract model for depth, motion vectors, exposure, history, color, jitter, and frame indices. |
| NVIDIA RTX Neural Shading | Slang-centered training/inference examples with D3D/Vulkan feature prerequisites and neural rendering assumptions. | Sparkle's shader toolchain and RHI feature model should be ready for profile/capability-gated neural paths without changing core ownership rules. |
| AMD D3D12MA / VMA | Production-recognized memory allocation libraries for D3D12 and Vulkan, with budget, pooling, mapping, statistics, and allocation naming/debugging concepts. | Sparkle should expose allocator-backed memory diagnostics and budget pressure rather than treating allocators as invisible implementation details. |

## Scoring Rubric

| Score | Meaning |
| --- | --- |
| 5 | Reviewer-ready. Clear contracts, tests/validation, diagnostics, docs, and known extension path. |
| 4 | Strong foundation. Mostly reviewable, with focused documentation or validation gaps. |
| 3 | Functional and promising, but important behavior is implicit or scattered. |
| 2 | Feature exists, but ownership, extension, or failure modes are hard to reason about. |
| 1 | Present only as early scaffolding or isolated implementation. |
| 0 | Missing. |

Scores are current-state architecture scores, not talent scores. They are meant to guide staging.

## Current Scorecard

### 1. Build, Module Boundaries, and Architecture Guardrails

Score: 4.0 / 5

Evidence:

- `CMake/ArchitectureBoundaryCheck.cmake` scans RHI, Renderer, and Application validation code for boundary violations.
- RHI cannot include Renderer-private headers.
- Renderer is guarded from D3D12/Vulkan native API identifiers except counted NVIDIA DLSS provider exceptions.
- D3D12 and Vulkan backends are checked against cross-contamination.
- Application validation is guarded from backend-native capture/API dependencies.
- Module CMake files describe intended dependencies and source grouping.

Reviewer-positive signal:

- The codebase already treats architecture as something executable, not just aspirational.
- Counted exceptions are especially good: they show the difference between intentional debt and accidental leakage.

Gaps:

- The boundary rules are stronger than the human-facing documentation around them.
- Exceptions should link to a provider contract or ADR explaining why they exist and what would retire them.
- There is not yet a reviewer landing page that says "start here, then inspect these modules."

Foundation work:

- Add an architecture map that mirrors the enforced CMake boundaries.
- Add an ADR index for every counted native/API boundary exception.
- Emit boundary check output in a reviewer-friendly command or CI summary.

### 2. RHI Public Contract

Score: 3.8 / 5

Evidence:

- RHI is split into common contracts plus backend-private implementations.
- Public surface includes commands, device, diagnostics, interop, memory, pipelines, presentation, ray tracing, resources, shader parameters, shaders, UI, and validation.
- D3D12 and Vulkan backends are separate targets.
- D3D12 uses D3D12 Memory Allocator and NVAPI integration inside the backend.
- Vulkan is optional and gated on SDK discovery.
- GPU validation and live-object reporting are configuration-gated.

Reviewer-positive signal:

- The module layout already reads like a real RHI, not a thin include soup.
- Memory allocator choices align with production patterns.
- The backend split gives a good place to add conformance tests and capability matrices.

Gaps:

- Resource state and barrier ownership need a written contract.
- Descriptor ownership, heap/ring allocation behavior, and lifetime rules need a reviewer-facing explanation.
- Command queue and command list threading assumptions should be explicit.
- Native handle escape hatches need a policy: who can request them, for what, and how failures are represented.
- D3D12/Vulkan parity needs a matrix covering device creation, swapchain, buffers, textures, descriptors, pipelines, timestamp queries, ray tracing, and presentation.

Foundation work:

- Write `RHIContract.md`: device lifetime, resource lifetime, command submission, barriers, descriptors, queues, pipeline creation, diagnostics, interop.
- Add a backend parity table beside the contract.
- Add a "new backend checklist" and a "new SDK interop checklist."

### 3. D3D12 Backend

Score: 3.7 / 5

Evidence:

- Backend-private target links D3D12, DXGI, DXGUID, D3DCompiler, D3D12MA, and optional NVAPI.
- NVAPI is fetched or provided via paths and restricted to the D3D12 RHI backend.
- RHI compile definitions distinguish validation/live object reporting between development and shipping configurations.

Reviewer-positive signal:

- Vendor API integration is in the right layer.
- Memory allocation is delegated to a recognized production library.
- Build-time requirements are explicit enough to fail early.

Gaps:

- Debug layer, GPU-based validation, DRED, PIX naming/capture conventions, and live-object reporting should be documented.
- Residency/memory budget telemetry needs a documented path.
- Queue model and fence ownership need a concise lifecycle diagram.
- NVAPI usage should have a feature capability surface that renderer code consumes without knowing NVAPI.

Foundation work:

- Add a D3D12 backend note documenting device creation, queues, fences, descriptor heaps, allocator strategy, debug tooling, and NVAPI feature reporting.
- Add named GPU object conventions and capture checklist.
- Add residency/budget counters before expanding memory-heavy features.

### 4. Vulkan Backend

Score: 3.2 / 5

Evidence:

- Vulkan backend target is gated by SDK discovery.
- Vulkan Memory Allocator is used when Vulkan is enabled.
- The launcher now surfaces Vulkan SDK absence where it matters instead of making source sync noisy.

Reviewer-positive signal:

- Vulkan is treated as a real backend, not just a compile definition.
- Optional SDK handling is appropriate for a Windows-first local workflow.

Gaps:

- Feature parity with D3D12 is not documented.
- Validation layer setup, debug utils naming, capture tooling, and portability assumptions need to be explicit.
- Ray tracing and advanced feature capability reporting should be matrixed against D3D12.

Foundation work:

- Add Vulkan backend notes parallel to D3D12 notes.
- Add validation and capture recipes.
- Keep Vulkan absence visible in backend selection and build diagnostics, not as generic dependency clutter.

### 5. Renderer Frame Graph, Frame Pipeline, and Passes

Score: 3.6 / 5

Evidence:

- Renderer is separated from source import/conversion/authoring dependencies.
- Private layout includes frame graph, frame pipeline, pass modules, scene data, temporal, ray tracing, textures, diagnostics, and settings.
- Public layout exposes renderer entrypoints, frame graph handles/descriptors, scene data, shader parameters, diagnostics, settings, and viewport contracts.
- Renderer publicly depends on Core and RHI, while Platform and GameFramework remain implementation details.

Reviewer-positive signal:

- The renderer is moving toward a production-recognizable structure.
- The folder names match the mental map reviewers expect: pass graph, frame orchestration, scene data, temporal history, ray tracing, diagnostics.

Gaps:

- Pass lifecycle is not yet documented as a contract.
- Resource import/export rules between renderer and RHI need to be visible.
- It is not yet obvious which renderer passes own persistent resources versus per-frame resources.
- Frame graph scheduling, barriers, aliasing, and diagnostics need a written model.
- There should be a clear "how to add a pass" guide that does not require spelunking.

Foundation work:

- Add `RendererFrameGraph.md`: pass registration, resource declarations, pass dependencies, transient/persistent resources, debug names, timing, and validation.
- Add a resource contract table for color, depth, normals, motion vectors, exposure, history, jitter, frame index, and camera matrices.
- Add pass authoring checklist and failure modes.

### 6. SDK/Upscaling/Provider Integration

Score: 3.4 / 5

Evidence:

- NVIDIA DLSS provider sources are isolated into a dedicated renderer provider target.
- Streamline linkage is conditional.
- Vulkan linkage is restricted to the NVIDIA provider target and counted by architecture checks.
- Launcher dependency sync is becoming hardware-aware for NVIDIA dependencies.

Reviewer-positive signal:

- Vendor SDK integration is already not allowed to quietly spread through the renderer.
- Conditional dependency handling is going in the correct direction for NVIDIA/AMD machines.

Gaps:

- There is not yet a provider-neutral interface for upscalers, denoisers, frame generation, or neural rendering features.
- Required input resource contracts are not centralized.
- Capability negotiation should describe unsupported, available, enabled, and runtime-failed states.
- AMD FidelityFX/Cauldron-style integration readiness should be represented even before adding FidelityFX features.

Foundation work:

- Add `RendererProviderContract.md`: provider lifetime, RHI handles, required resources, capability queries, failure reporting, and debug overlays.
- Move SDK-specific assumptions behind provider capability structs.
- Keep Streamline/DLSS as one provider implementation, not as the shape of the whole provider architecture.

### 7. Shader Compiler, Shader Contracts, and Cook Pipeline

Score: 4.0 / 5

Evidence:

- ShaderCompiler uses DXC and Slang SDK roots, imports DXC and Slang runtime files, links SPIR-V reflection, RHI, and renderer shader registrations.
- Shader contracts, reflection, cooking, cache keys, include closure handling, and verification code already exist in the tools tree.
- Renderer shader registrations are isolated as an object target.

Reviewer-positive signal:

- This is one of Sparkle's strongest principal-level areas because it shows tooling, ABI thinking, and multi-backend shader awareness.
- Slang is especially aligned with modern NVIDIA neural rendering and cross-target shader workflows.

Gaps:

- The shader ABI is not yet explained in a compact reviewer document.
- The relationship between shader registration, reflection output, cooked package layout, and RHI pipeline creation should be diagrammed.
- Golden tests/regression corpora should be visible.
- Shader profile/capability gating for SM6, SPIR-V, ray tracing, mesh/task shaders, and future neural paths should be matrixed.

Foundation work:

- Add `ShaderPipeline.md`: source, include closure, frontends, targets, reflection, contracts, cook cache, cooked package ABI, runtime load.
- Add shader feature capability matrix.
- Add a small reviewer command list: compile one shader, inspect reflection, verify package.

### 8. GameFramework Runtime Data Layer

Score: 3.8 / 5

Evidence:

- GameFramework owns assets, cooked asset types, levels, scenes, camera, lighting, materials, meshes, skeletons, and textures.
- It depends on Core and Platform, not Renderer/RHI.
- The renderer consumes runtime scene/material data without source import/conversion dependencies.

Reviewer-positive signal:

- Runtime data is not coupled to GPU backend code.
- The split supports renderer experimentation without corrupting asset/source import ownership.

Gaps:

- Data layout, ownership, threading, and update frequency are not summarized.
- Scene snapshot/update rules need a document.
- Asset versioning and cooked runtime compatibility should be explicit.

Foundation work:

- Add `RuntimeSceneData.md`: ownership, mutation model, frame snapshot model, asset IDs, material/mesh texture handles, versioning.
- Add "renderer consumes this, never mutates that" rules.

### 9. Application Host and Runtime/Editor Split

Score: 3.5 / 5

Evidence:

- Application has runtime and editor hosts.
- Runtime target excludes editor-only validation/recook files.
- Editor app target includes shader recook and validation.
- Public application API remains small.

Reviewer-positive signal:

- The module does not look like a monolithic main loop.
- Editor-only workflows are mostly separate from runtime path.

Gaps:

- App lifecycle needs a state diagram: startup, tool discovery, RHI/device creation, renderer creation, project load, frame loop, shutdown.
- Error taxonomy should distinguish missing SDK, unsupported hardware, invalid project, shader cook failure, backend failure, and runtime validation failure.
- Capture/replay/smoke validation should have stable command entrypoints.

Foundation work:

- Add `ApplicationLifecycle.md`.
- Add structured error categories shared by launcher/status UI and runtime validation.
- Add smoke validation recipes for D3D12 and Vulkan where available.

### 10. Editor Tooling

Score: 3.3 / 5

Evidence:

- Editor exposes panels for viewport, hierarchy, profiler, console, assets, shader resources, rendering settings, used meshes/shaders/textures, and material/texture inspection.
- Editor depends on RHI and Renderer directly, which is appropriate for tool-facing rendering inspection.

Reviewer-positive signal:

- There is already a place to surface renderer internals.
- Profiler/resource panels are exactly the kind of portfolio signal reviewers can inspect quickly.

Gaps:

- The editor should have a principal-review dashboard or documented layout: frame timing, pass timing, memory, descriptors, pipeline cache, shader state, active backend, enabled SDK providers.
- Panels need documented data sources so they do not become ad hoc UI over time.
- Capture buttons/links should map to documented backend capture flows.

Foundation work:

- Add `EditorDiagnostics.md`: each diagnostics panel, data source, ownership, update rate, and intended reviewer signal.
- Add stable diagnostics model structs before adding more UI.

### 11. Launcher, Dependency Sync, and Workflow Actions

Score: 3.7 / 5

Evidence:

- Launcher has tool resolution, CMake generator modeling, workflow operations, dependency UI models, build/cook/clean actions, GPU-aware source sync, and status pages.
- NVIDIA dependencies can be hardware-gated.
- Vulkan SDK absence is surfaced where backend selection/build readiness matters.

Reviewer-positive signal:

- This helps demonstrate SDK-level thinking beyond renderer code.
- The launcher can become a controlled reviewer path instead of relying on oral setup instructions.

Gaps:

- Reviewer workflows need one-click or one-command equivalents: generate, build all, cook all, run smoke, format check, boundary check.
- Dependency decisions need a concise policy document: required, optional, hardware-gated, backend-gated, provider-gated.

Foundation work:

- Add `LauncherWorkflowReadiness.md`.
- Make every workflow action map to a CLI command or documented operation.
- Surface architecture boundary checks as first-class validation, not hidden CMake behavior.

### 12. Validation, Tests, and Performance Evidence

Score: 3.2 / 5

Evidence:

- Architecture boundary check exists.
- Application validation and RHI smoke-style validation exist in the source layout.
- Shader compiler verification/cook inspection exists.

Reviewer-positive signal:

- There is already a validation culture to build on.

Gaps:

- RHI conformance tests should be explicit per backend.
- Golden image tests, shader package tests, and backend parity tests need a visible plan.
- Performance regression tracking is not yet obvious.
- Memory and descriptor pressure tests should exist before heavy renderer/neural features arrive.

Foundation work:

- Add `ValidationMatrix.md` with test category, command, backend coverage, artifact output, and failure owner.
- Add non-feature perf baselines: empty frame, one mesh, many materials, descriptor pressure, upload pressure, shader compile cache hit/miss.

### 13. Documentation and Portfolio Presentation

Score: 1.8 / 5

Evidence:

- Some module CMake comments are strong.
- Architecture checks encode rules.
- There is not yet a dedicated docs hierarchy for reviewer navigation.

Reviewer-positive signal:

- The code has better architecture than the documentation currently reveals.

Gaps:

- Reviewers should not have to discover the architecture by grep.
- The portfolio story needs a curated path: 10-minute review, 30-minute review, deep dive.
- Tradeoffs and non-goals need to be written down.

Foundation work:

- Build a docs tree:
  - `Docs/Architecture/README.md`
  - `Docs/Architecture/RHIContract.md`
  - `Docs/Architecture/RendererFrameGraph.md`
  - `Docs/Architecture/ShaderPipeline.md`
  - `Docs/Architecture/RuntimeSceneData.md`
  - `Docs/Architecture/ApplicationLifecycle.md`
  - `Docs/Architecture/ValidationMatrix.md`
  - `Docs/Architecture/ADR/`

## Requirement Coverage Matrix

| Requirement cluster | Current readiness | Highest-value foundation work |
| --- | --- | --- |
| D3D12/Vulkan explicit control | Medium-high | RHI contract, queue/barrier/descriptor docs, backend parity tests |
| GPU memory/performance awareness | Medium | Memory budget telemetry, descriptor/pipeline metrics, perf baseline recipes |
| Cross-IHV SDK readiness | Medium | Provider-neutral SDK contracts, capability model, dependency policy |
| Shader/compiler sophistication | High | Shader ABI docs, regression corpus, feature matrix |
| Ray tracing readiness | Medium | AS lifecycle docs, backend parity matrix, ray tracing validation cases |
| Neural rendering readiness | Early-medium | Slang/profile capability map, provider resource contract, future tensor/cooperative vector extension notes |
| Production modularity | Medium-high | Human docs mirroring CMake boundaries, ADRs for exceptions |
| Reviewer navigation | Low | Reviewer guide, architecture README, staged validation commands |

## Staged Plan

### Stage 0: Make The Existing Architecture Legible

Goal: reviewers can navigate the engine without oral context.

Actions:

- Add `Docs/Architecture/README.md` with the module map and dependency direction.
- Add short module briefs for RHI, Renderer, GameFramework, Application, Editor, Launcher, ShaderCompiler, AssetCooker.
- Convert this scorecard into tracked review items.
- Link CMake boundary rules to docs and ADRs.
- Add "what this engine is not" so reviewer expectations are framed: not Unreal, not a finished game editor, not a feature zoo.

Exit criteria:

- A reviewer can open one README and know where to inspect RHI, renderer frame graph, shader compilation, SDK integration, and validation.

### Stage 1: Write The Core Contracts

Goal: extension points are clear before features arrive.

Actions:

- Write `RHIContract.md`.
- Write `RendererFrameGraph.md`.
- Write `RendererProviderContract.md`.
- Write `ShaderPipeline.md`.
- Write `RuntimeSceneData.md`.
- Write `ApplicationLifecycle.md`.
- Add backend/provider/pass checklists.

Exit criteria:

- Adding a backend, pass, SDK provider, shader feature, or runtime validation path has a documented route and owner boundaries.

### Stage 2: Turn Boundaries Into Continuous Evidence

Goal: architecture is hard to break accidentally.

Actions:

- Promote architecture boundary check to launcher/build workflow visibility.
- Add RHI backend parity matrix.
- Add validation matrix with commands and artifact paths.
- Add shader compiler golden/reflection regression cases.
- Add smoke validation for runtime/editor startup, generate/build/cook flow, and backend availability.

Exit criteria:

- "It still works" can be shown by commands, artifacts, and backend-specific pass/fail status.

### Stage 3: Add Performance And Memory Review Surfaces

Goal: principal-level performance thinking is visible before new features add complexity.

Actions:

- Add memory budget/residency reporting in RHI.
- Add descriptor heap/pool pressure reporting.
- Add pipeline cache and shader package load timings.
- Add GPU timestamp scopes per renderer pass.
- Add CPU frame orchestration timings.
- Surface the data in editor diagnostics and text artifacts.

Exit criteria:

- A reviewer can see frame cost, pass cost, memory budget, descriptor pressure, shader/pipeline cache behavior, active backend, and active SDK providers.

### Stage 4: Prepare SDK And Neural Rendering Arrival Points

Goal: advanced SDK/neural features can be added without reshaping core modules.

Actions:

- Introduce provider-neutral capability states: unavailable, unsupported hardware, missing dependency, available, enabled, runtime failed.
- Centralize required resource contracts for upscalers, denoisers, frame generation, ray tracing, and future neural paths.
- Add Slang/DXC/profile feature matrix.
- Add future extension notes for cooperative vectors/tensor-like paths, neural materials, neural texture compression, and denoiser/upscaler providers.
- Keep feature implementations out of this stage unless needed to prove the contract.

Exit criteria:

- DLSS, FidelityFX, denoisers, ray tracing experiments, or neural shading can be added as providers/passes against known contracts.

### Stage 5: Package The Portfolio Review Path

Goal: the engine can be reviewed under time pressure.

Actions:

- Add a 10-minute review path:
  - architecture map
  - boundary check
  - generate/build/cook
  - one runtime smoke run
  - one shader inspection
- Add a 30-minute review path:
  - RHI contract
  - frame graph/pass authoring
  - shader pipeline
  - provider contract
  - diagnostics dashboard
- Add a deep-dive path:
  - backend implementation
  - memory/descriptor model
  - frame graph validation
  - shader ABI
  - SDK integration and capability handling
- Add a limitations page that honestly names missing features and why the foundation was prioritized first.

Exit criteria:

- The repo itself answers "why should I trust this architecture?" before a reviewer needs to ask.

## Immediate Priority List

1. Architecture README and module dependency map.
2. RHI contract.
3. Renderer frame graph/pass contract.
4. Shader pipeline/ABI document.
5. Provider contract for SDK/upscaler/denoiser/neural arrivals.
6. Validation matrix.
7. Performance/memory diagnostics plan.
8. Reviewer guide.

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

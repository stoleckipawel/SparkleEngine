# J. Multithreaded Engine Architecture and Learning Program

Status: proposed target architecture and implementation program
Date: 2026-07-16
Last adversarial conformance review: 2026-07-18
Last NVIDIA/AMD/Epic naming review: 2026-07-18 (repository revisions pinned in the source matrix)
Scope: runtime, renderer, RHI, editor, asset and shader tools, learning evidence, and portfolio presentation
Governing requirements: [A. Principal Rendering Requirements](A_PrincipalRenderingRequirements.md), [E. External Renderer Repository Comparison](E_ExternalRendererRepositoryComparison.md), [G. Advanced Graphics Engine Executive Summary](G_AdvancedGraphicsEngineExecutiveSummary.md), and [H. Advanced Graphics Engineer Persona](H_AdvancedGraphicsEngineerPersona.md)
Repository context: [D. Whole Repository Architecture Map](D_WholeRepositoryArchitectureMap.md)
Implementation companion: [K. Multithreaded Engine Implementation Prompt Series](K_MultithreadedEngineImplementationPromptSeries.md)

## How to Use This as a Learning Document

This document has two simultaneous jobs:

1. define Sparkle's required multithreaded architecture; and
2. teach enough concurrency, ECS/DOD, renderer, RHI, lifetime, and profiling reasoning to implement and defend that architecture.

Do not read it as a feature checklist. Read it as a chain of proofs. Each later mechanism depends on an earlier ownership claim:

~~~text
C++ memory model and ownership
          │
          ▼
job/task lifetime and dependency graphs
          │
          ▼
data-oriented ECS storage + frozen structural epochs
          │
          ▼
immutable publication + stable generations
          │
          ▼
game/editor/render pipeline and bounded backpressure
          │
          ▼
persistent GPU scene + deferred retirement
          │
          ▼
parallel renderer preparation and command recording
          │
          ▼
native D3D12/Vulkan validation and measured portfolio evidence
~~~

If an earlier proof is missing, later parallelism is not ready. For example, a worker pool does not make `GameScene` safe; immutable packets do not make Vulkan command pools safe; parallel command lists do not prove useful speedup; and a data-only ECS does not automatically infer task dependencies.

### The Study Loop

Use the same seven-step loop for every topic:

1. **Observe:** find the current Sparkle code path and describe what it does today without proposing a fix.
2. **Model:** name the owner, readers, writers, lifetime, publication point, and ordering constraints.
3. **Pre-mortem:** select applicable MT hazard IDs from the Professional Multithreading Failure Atlas and predict how each failure would appear in this exact path.
4. **Build serially:** express the new contract through the deterministic serial executor or serial consumer first.
5. **Parallelize one proven range:** add tasks only where inputs and output ownership are explicit.
6. **Try to falsify it:** inject delays, cancellation, stale generations, small workloads, shutdown, and backend-specific validation.
7. **Measure and teach back:** explain what improved, what stayed serial, what became more expensive, and why the result remains correct.

The objective is not merely to remember APIs. You should be able to predict failure before running the code, draw the happens-before and lifetime edges, and explain why a rejected design is unsafe or unnecessarily complex.

### Required Prior Knowledge and What This Program Teaches

You should already be comfortable with modern C++ value/reference semantics, RAII, templates, atomics at a basic level, containers, and the engine's current frame flow. This program then develops:

- the C++ memory-model concepts needed for real publication and synchronization
- work-stealing job-system mechanics and structured task lifetime
- ECS identity, storage, queries, structural mutation, and DOD tradeoffs
- game/render decoupling, frame latency, bounded queues, and temporal identity
- CPU versus GPU scheduling and native command-recording ownership
- deterministic tools, cancellation, transactional publication, and editor affinity
- concurrency debugging, profiling, serial equivalence, and performance reporting

When a concept remains unclear, return to the current code example cited in the local audit trail. Concrete engine state is the teaching substrate; abstract terminology is secondary.

### A Tutor's Mental Model: Follow One Frame

Trace one future frame through the target engine:

1. The main thread pumps window/input events. No worker touches ImGui or platform-window state.
2. The world owner commits queued entity/editor commands. ECS component composition may change only here.
3. Structure freezes. Typed ECS queries now refer to stable component pools for this update epoch.
4. `GameSystemGraph` converts query/resource hazards into task prerequisites. `SparkleTasks` runs safe ranges.
5. Task-local results and entity commands merge deterministically. No ID or output ordering depends on which core finishes first.
6. The owner evaluates/commits derived state and publishes immutable world/editor/render generations with release semantics.
7. The render coordinator acquire-consumes a bounded frame packet and updates render-owned proxies/GPU-scene dirty ranges.
8. Renderer preparation tasks produce private results. The frame graph remains the resource/queue/barrier authority.
9. Audited recording groups lease worker-local D3D12 or Vulkan contexts. Workers record; they do not submit.
10. The render coordinator submits/presents in compiled order. GPU resources retire only after completion tokens prove last use.
11. The producer reuses CPU packet storage only after render acknowledgement; this is separate from GPU retirement.

At every arrow ask four questions: who writes, who reads, what establishes happens-before, and what prevents early reuse? If any answer is “a mutex somewhere” or “the frame probably finished,” the contract is incomplete.

### Guided Curriculum Map

| Lesson | Read primarily | Learn to explain | Practical proof |
|---|---|---|---|
| 1. Current frame and ownership | Current Repository State; Thread Roles | why current serial code is correct and where lifetime coupling exists | annotated current CPU timeline |
| 2. Memory model | Learning Foundations; Ownership State Machine | data race, atomicity, release/acquire, publication, reclamation | packet publication stress test |
| 3. Engine job system | SparkleTasks Task Runtime Design | ready queues, dependencies, continuations, sleep/wake, cancellation, scopes | serial and 1/2/N-worker graph tests |
| 4. ECS and DOD | GameFramework ECS sections | entity versus object, sparse/dense storage, queries, structural epochs, AoS/SoA choices | randomized registry/query reference tests |
| 5. Cross-thread world/render contract | Cross-Thread Scene Contract | stable generations, deltas, dynamic streams, proxies, backpressure | headless packet replay without `GameScene` |
| 6. Render pipeline | Renderer Redesign | render ownership, persistent GPU data, preparation DAG, submission authority | serial/threaded renderer parity capture |
| 7. Native recording | RHI/frame-graph sections | D3D12 allocator/list and Vulkan pool/buffer rules, barriers, context retirement | paired backend validation under delayed tasks |
| 8. Async product workflows | Loading, Editor, Tools | I/O lanes, stale results, atomic publication, UI affinity, deterministic tools | cancel/fail/reload tests retaining old generation |
| 9. Feature preservation | Advanced Graphics Feature Preservation | why threading must preserve RT, temporal, providers, shader ABI, capture | feature-specific serial/parallel matrix |
| 10. Performance and portfolio | Verification, Performance, Portfolio | Amdahl's law, critical path, task grain, CPU/GPU separation, honest claims | reproducible p50/p95 captures and live demonstration |
| 11. Atomic and synchronization depth | Expert Concurrency Core; Tutorial 10 | modification order, CAS loops, ABA, progress guarantees, primitive selection | bounded-queue/deque litmus and reclamation tests |
| 12. CPU topology and scheduler behavior | Expert Concurrency Core; Tutorial 11 | physical/logical cores, SMT, cache domains, priority inversion, oversubscription | worker-count/topology capture matrix |
| 13. Parallel algorithms and streaming | Tutorials 12-13 | partition, reduction, scan, deterministic merge, I/O/decode/upload pipelines | real cooker/ECS/PSO cold-cache workloads |
| 14. GPU queue and frame latency | Tutorial 14; Renderer/RHI sections | CPU tasks versus GPU queues, overlap, fences, pacing, backpressure, input-to-present | D3D12/Vulkan queue and latency captures |
| 15. Production diagnosis and interview defense | Tutorial 15; Interview Readiness Audit | races, deadlocks, stalls, crash evidence, tool choice, causal explanation | injected failures, traces, whiteboard and coding drills |
| 16. Professional failure recognition | Professional Multithreading Failure Atlas; K hazard traceability | recognize MT-01–MT-44 from code and traces, select a source-backed engine pattern, and design a falsifying test | pre-mortem and hazard-closure report for each implementation prompt |

### Teach-Back Standard

Before advancing from a lesson, explain it without reading the document. A satisfactory explanation contains:

- one Sparkle source example from the current state
- the invariant being introduced
- the synchronization or ownership edge enforcing it
- one tempting incorrect implementation and its failure mode
- the applicable MT hazard ID and the NVIDIA/AMD/Epic or specification precedent that makes the chosen pattern recognizable
- the chosen design's cost or limitation
- the test that would disprove your claim if the implementation were wrong

“It is thread-safe,” “the ECS handles it,” “the frame graph handles it,” and “the job system waits” are not sufficient explanations. Name the specific storage, owner, edge, phase, token, or generation.

### Division of Responsibility Between J and K

Use this document while learning, reviewing a design, or deciding whether a stage is safe. Use [K](K_MultithreadedEngineImplementationPromptSeries.md) while changing code. K deliberately repeats the repository-coherence, inspect-before-add, daily-refactor, deletion, validation, and stop/go rules in every implementation prompt so they cannot be lost when one prompt is used independently.

## Executive Decision

Sparkle should introduce multithreading as an engine-wide ownership and data-flow redesign, not as a collection of isolated thread launches.

The target is:

- a small engine-owned task runtime used by the runtime, renderer, editor, and offline tools
- a bounded data-oriented ECS kernel in GameFramework with generational entities, typed packed component pools, queries, and deferred structural commands
- a dedicated render coordinator thread that exclusively owns mutable renderer and RHI state
- a bounded, one-frame-ahead game/editor-to-render pipeline
- stable handles, immutable frame packets, scene deltas, and render proxies instead of raw game pointers
- a persistent GPU scene updated by dirty ranges instead of rebuilding and uploading most scene arrays every frame
- a preparation task graph whose nodes produce data and whose dependencies make synchronization explicit
- parallel frame-graph command recording using per-worker, per-frame, per-queue RHI contexts
- deterministic serial modes, instrumentation, correctness tests, and benchmark evidence

This is the pivotal change because it forces Sparkle to answer the production-engine questions that threads expose:

1. Who owns each piece of mutable state?
2. When is data published, and when may its storage be reused?
3. Which work is independent?
4. What order must remain deterministic?
5. Where may a thread block?
6. How are CPU tasks, GPU queues, and frames in flight kept distinct?
7. How is shutdown, cancellation, hot reload, and device-resource retirement made safe?

A job system alone would not answer those questions. A render thread alone would merely move the current serial work. Parallel command recording alone would fail on the current lazy pipeline caches, shared upload allocator, and command-pool ownership. The program therefore changes the data model first, then turns the new independence into parallel work.

## Product and Portfolio Thesis

The intended result is not “Sparkle uses many threads.” It is:

> Sparkle is a renderer-first D3D12/Vulkan engine with explicit CPU task dependencies, a thread-owned renderer, immutable cross-thread data, persistent GPU-scene updates, and parallel command recording whose correctness and speedup can be demonstrated in both serial and parallel modes.

That statement fits the advanced graphics engineer persona. It demonstrates:

- modern C++ concurrency and memory-model reasoning
- explicit D3D12 and Vulkan command recording ownership
- CPU/GPU pipelining and frames-in-flight lifetime management
- data-oriented scene extraction and persistent GPU data
- ECS query/component access integrated with task dependencies rather than arbitrary object updates
- frame-graph dependency and resource-state reasoning
- editor and hot-reload integration rather than a renderer-only demo
- offline content-pipeline parallelism and deterministic publication
- profiling, testing, debugging modes, and honest performance reporting

The work should remain renderer-first. The existing GameFramework → Renderer → RHI responsibility direction is sound. A new Tasks foundation module is not a fourth rendering abstraction layer; it is shared execution infrastructure below the systems that use it.

## Adversarial Conformance Review

### Verdict

The core proposal is technically credible but the original draft was not sufficient merely because it contained a task system, render thread, immutable packets, and parallel command recording. Against A, E, G, and H, it had eight material weaknesses:

1. It did not explicitly bind the multithreading program to the renderer-first product identity.
2. It risked growing a broad task-diagnostics and renderer-observation surface contrary to the deletion-first direction.
3. It treated classic TLAS, PTLAS, native reservoir-based direct lighting, reference path tracing, temporal/provider paths, and screenshot capture as examples or tests rather than preservation requirements.
4. It did not state how the shader ABI, HLSL/Slang cook pipeline, reflection, feature profiles, and future tensor-like resources remain valid under parallel preparation and reload.
5. It called external providers serial islands but did not define provider ownership, capability/failure state, or resource publication rules.
6. It did not connect runtime/task changes to curated multi-level content, package ownership, optional heavy content, and a smaller launcher/tool surface.
7. It proposed early and new-looking diagnostics without reconciling G/H’s rule that broad workload analysis is late and must consolidate existing hooks.
8. It did not require every multithreading change set to simplify or delete the path it replaces.

The amendments below make those constraints normative. They do not claim the code already satisfies them.

### Status Vocabulary and No-Implied-Support Rule

Every capability in this document has one of four meanings:

- **Current:** verified in the repository audit and usable now.
- **Required target:** must be implemented and pass its acceptance gate before the multithreading program is complete.
- **Deferred:** deliberately outside the current sequence and not implied by foundational code.
- **Explicit non-goal:** must not be introduced by this program.

Unless a section explicitly says “Current,” architectural descriptions are required targets. A class sketch, task-graph diagram, test name, or portfolio artifact is not evidence that the capability exists. “Supports,” “parallel,” “safe,” “production,” and “parity” may only be used in release-facing text after the relevant acceptance tests pass on both backends.

### Requirements Traceability Matrix

| Governing requirement | Adversarial finding in the original J | Binding response in this program | Acceptance evidence |
|---|---|---|---|
| A: explicit D3D12/Vulkan ownership | Strong command-context proposal, but RHI/frame-graph automation boundary remained implicit | Add an explicit threading-aware RHI ownership contract | Backend owner assertions, state-plan parity, native validation |
| A: hardware-aware performance | CPU scheduling was detailed; GPU bandwidth, descriptor, residency, list fragmentation, and pipeline effects were not gates | Add CPU/GPU non-regression and memory-budget gates | Existing profiler captures and allocator/capability data |
| A: cross-IHV SDK discipline | Providers were only called serial islands | Add provider instance, tagged-resource, capability, failure, and callback ownership rules | Provider matrix and threaded reload/resize tests |
| A: render/pass architecture | Strong frame-graph recording design | Require declared histories/resources and forbid execute-time resource/PSO creation | Parallel-safety audit per pass |
| A/D: renderer-front-end CPU work | “Transforms, lighting, visibility, batching” was too coarse to prove common engine cases | Bind separate view/LOD, light classification, shadow-caster, retained/dynamic draw, sort/instancing, skinning, RT-input and GPU-scene tasks | Prompt 17 output ledger, stable serial/parallel sets and critical-path capture |
| A: explicit RHI CPU work | Command recording and PSOs were covered, but native buffer/image/view, allocation/bind, descriptor update, aggregation and retirement cases were not individually closed | Add the renderer/RHI use-case completeness audit and Prompt 27 staged native-work contract | Per-operation safety/owner ledger, cold-cache/concurrency matrix and backend validation |
| A/E: command production architecture | Unreal-style translation, native recording, “merging,” batching and submission could be conflated | Teach five layers, retain Sparkle direct native recording, qualify every merge, and gate any future software RHI stream by ADR/profile evidence | Same compiled plan, deterministic aggregation, batch/latency sweep, no speculative `RhiThread` |
| A: shader/compiler ABI | Parallel cooking and reload were covered, but ABI preservation was implied | Add immutable shader-generation and compiler-context contract | DXIL/SPIR-V cook, reflection/layout parity, generation swap tests |
| A: classic TLAS/PTLAS product readiness | Mentioned in GPU-scene data, tests, and benchmarks, not protected as equal product paths | Add explicit RT preservation contract and per-backend gates | Build/update/trace/lifetime tests for both paths where supported |
| A: neural rendering readiness | Only deferred heavy ML features | Preserve tensor-like resource/layout/profile expressiveness without adding ML runtime | Shader contract tests and one replacement-based prototype gate |
| A: production reviewability | Detailed plan risked becoming planning sprawl | Keep this as the single internal implementation program and require code-shaped evidence | No new policy/report documents; narrow APIs and deletion ledger |
| E: declared product identity | “Renderer-first” appeared, but did not constrain scheduler/tool scope | Tasks is internal execution infrastructure for a compact renderer-first engine, not a product identity | Package/module dependency audit |
| E: no extra rendering abstraction | Tasks and recording leases could be mistaken for another RHI layer | Tasks owns CPU execution only; frame graph owns scheduling; RHI remains explicit | Include/dependency boundary checks |
| E: application-owned SDK integration | Provider scheduling ownership was incomplete | Renderer owns resources, passes, camera/temporal signals, and GPU ordering | No provider-owned frame graph or scene model |
| E: product/preview/research separation | Missing | Add feature classification and prohibit research work from shaping shipping graphs/data | Shipping-default graph contains product paths only |
| E: samples/content/package discipline | Tool parallelism covered throughput, not product scope | Restrict work to manifests/catalogs and intentional runtime/editor/tools/content packages | Deterministic curated and optional-pack cook/package tests |
| G: deletion-first change gate | New task/runtime types were not tied strongly enough to removal | Add an architecture budget and deletion/preservation ledger to every change set | Replaced path removed by stage exit |
| G: late workload analysis | Stage 0 and diagnostics language could create a new measurement system | Separate minimal pre-change capture from late consolidated workload analysis | No new profiler framework, panel, log, or default report |
| G: debugging and screenshot/BMP preservation | Capture was just a render command/test case | Add a nonblocking capture ownership pipeline and preserve native debugger hooks | D3D12/Vulkan capture test without device idle |
| G: smaller public observation APIs | Render/task diagnostic snapshots risked API expansion | Instrument privately through existing hooks; publish only product-owned results | Public-header diff and consumer audit |
| G: engine-wide architecture, not renderer patch | GameFramework/editor were only deferred candidate jobs | Add stable world identity, declared systems, deferred mutation, immutable editor views, and transactional scene loading | Serial/parallel world replay, load cancellation, editor responsiveness, deletion gates |
| G/H: data-oriented production-engine foundation | System domains existed, but storage remained unspecified and ECS was explicitly deferred | Add a bounded private sparse-set ECS, typed queries, hot/cold components, structural epochs, and evidence gate for archetypes | Storage/query/command tests, cache/iteration measurements, deletion of object-vector path |
| H: small vertical slices and direct integration | The task runtime could become a general framework project | Bound scheduler features and require vertical tool/renderer consumers | No unused task primitive or speculative wrapper |
| H: shader/GPU/RT depth | CPU architecture was stronger than feature-depth integration | Add feature preservation contracts for RT, lighting, temporal, providers, shaders, and capture | Feature-specific threaded parity matrix |
| H: professional tools, not bespoke diagnostics | New task timeline language was ambiguous | Reuse existing markers/debug layers/profiler scopes; no task panel | PIX/RenderDoc/Nsight-visible roles and scopes |
| H: learn multithreading by useful production work | Renderer-only evidence would not prove lifecycle/editor/gameplay understanding | Use existing animation plus a needed async scene-load/editor-operation path to demonstrate distinct concurrency patterns | Code, stress tests, captures, and live cancel/fail/reload demo |
| H: every addition simplifies nearby code | Present in some stage deletion lists, not global | Make simplification a program-level completion gate | Legacy futures, snapshots, waits, pools, and host paths removed |

### Binding Product Identity

For this program:

> Sparkle is a compact renderer-first engine with multiple projects and levels, an editor, and owned build/cook/launch workflows. Multithreading exists to make those product paths cleaner and faster; it is not a standalone scheduler SDK, a new renderer framework, or a reason to preserve unowned tools and research scaffolding.

Consequences:

- SparkleTasks is internal engine infrastructure. It does not promise a public plugin ecosystem or general-purpose API stability.
- The frame graph remains the renderer scheduler; SparkleTasks only executes CPU work made legal by existing ownership and graph decisions.
- The RHI remains the explicit low-level D3D12/Vulkan service layer. `RhiCommandRecordingLease` expresses temporary ownership; it does not hide queues, commands, barriers, descriptors, or native backend rules.
- Tools may use SparkleTasks only when they are part of the owned build, cook, shader, launch, clean, or package workflows.
- Research features may consume the infrastructure, but may not add scheduler primitives, packet fields, or shipping graph complexity without a product feature need.

### Additive Capability, Reductive Structure

“Additive” means the engine gains real multithreaded capability while preserving the advanced graphics plan. It does not mean every proposed helper, diagnostic, or compatibility path is retained.

Every implementation change set must carry two short ledgers in its review description:

1. **Capability preservation:** product features and backend paths that must remain working.
2. **Replacement/deletion:** old future, thread, snapshot, wait, cache, allocator route, public API, or scaffolding removed by the change.

Rules:

- A foundational task-runtime change may temporarily add more code than it deletes, but it must have named consumers and a dated deletion target for superseded paths.
- No compatibility path survives a stage exit without an owner and removal stage.
- New public headers require at least two real module consumers or a necessary host boundary.
- No public API exists only to expose task queues, worker state, timings, or debug status.
- No new runtime log stream, debug panel, report format, validation framework, benchmark framework, or profiler integration is authorized by this plan.
- New tests and private development assertions are permitted when they protect ownership or native API correctness; they are not product features.

### Internal Research Note Versus Product Decision Text

E requires source-backed comparison, while G/H require product decision text to avoid company names and direct external trails. This document is the internal source-backed implementation study and is the deliberate exception. Code comments, public README material, runtime UI, package descriptions, and the concise product architecture overview must use Sparkle’s own ownership vocabulary and must not claim parity with or equivalence to any external SDK or engine.

## What “Multithreaded Renderer” Actually Means

Several kinds of concurrency are often collapsed into one phrase. Sparkle must name and measure them separately.

| Kind | Where work overlaps | Sparkle today | Target |
|---|---|---:|---:|
| Background concurrency | File/process work overlaps the application | One shader-recook future and a process-output reader thread | Managed background and blocking-I/O lanes |
| Task parallelism | Independent CPU work overlaps inside one frame or cook | No general system | Work-stealing task runtime and dependency graphs |
| Game/render pipelining | Game frame N+1 overlaps render frame N | No | Dedicated RenderThread and bounded `RenderFrameQueue` |
| Parallel command recording | CPU workers build command lists/buffers together | No | Frame-graph recording groups and worker-local RHI contexts |
| GPU queue concurrency | Graphics, compute, and copy execute concurrently | Yes | Preserve, measure, and feed it more efficiently |
| Frames in flight | CPU and GPU work on different frames | Yes at the RHI/frame-resource level | Make CPU packet/resource ownership explicit |

GPU async compute is not CPU multithreading. A copy queue upload is not a background CPU task. Multiple frames in flight do not make the application loop multithreaded. The final documentation, profiler, and portfolio material should keep these distinctions visible.

### Renderer/RHI Multithreading Use-Case Completeness Audit

The governing documents require more than a task runtime and a few parallel passes. A requires explicit command-list, queue, descriptor, memory, pipeline, shader, barrier, ray-tracing, and lifetime competence. D identifies current CPU work in scene construction, frame-graph setup/compile, shader/texture loading, pipeline creation, recording, and submission. E requires a renderer/RHI split grounded in NVRHI, Donut, and other production references. G/H require demonstrable graphics-engine skill without weakening the renderer's existing product features. Therefore, a renderer/RHI use case is complete only when it has an owner, a serial oracle, a bounded parallel policy, native-API validation, performance evidence, and a K prompt.

The status words are binding:

- **Implement** means build and retain a useful Sparkle product path after serial/parallel parity and performance gates.
- **Measured candidate** means build only when a named current path is a measured critical-path or hitch source; delete a losing parallel variant.
- **Study/lab** means understand and, where specified, exercise the mechanism without adding a second shipping architecture.
- **Know and defer** means be able to defend why the technique exists and why Sparkle does not yet need it.
- **Reject** means the mechanism would duplicate an existing authority or solve no durable Sparkle problem.

#### Renderer Front-End and Scene Preparation

| Common engine use case | Sparkle decision | Existing Sparkle substrate | Required proof / K home |
|---|---|---|---|
| Apply scene changes to render-owned proxies | Implement | `RenderSceneSnapshot`, planned `RenderWorldDelta`, current `RenderSceneDataBuilder` | Stable generation and previous-frame identity; Prompts 12, 15, 17 |
| Transform propagation, world bounds, previous transforms | Implement | `MeshDrawTransform`, snapshot transforms, skinning state | Range ownership, serial equivalence, bounds reduction; Prompts 08, 10, 17, 26 |
| Per-view frustum visibility, distance/LOD and render relevance | Implement | Current camera/view and mesh classifications, but no complete retained visibility contract | Deterministic visible sets and view identity; Prompt 17 |
| Light visibility, classification and compact light packets | Implement | Current light arrays and native reservoir-lighting paths | Stable light IDs/order, visible-light lists, serial/parallel reservoir parity; Prompt 17 |
| Shadow-view setup, light/frustum intersection and caster lists | Implement when shadow path consumes raster caster lists; otherwise build the planning contract with the first such path | Current shadow visibility/direct-shadow feature surface | Per-light/view private results, deterministic caster order, large-light stress; Prompts 17, 21 |
| Skinning, morph and animation-derived render data | Implement | Existing skeletal mesh, morph weights and joint matrices | Game-system producer plus render preparation consumer; Prompts 10, 17 |
| Raster pass eligibility and material/pipeline classification | Implement | `MeshRenderItem`, `RenderMeshKind`, material handles and pass runtimes | Immutable tables, no lazy cache mutation, stable pass buckets; Prompt 17 |
| Static/retained versus dynamic draw preparation | Implement incrementally, using current `MeshDraw` and `MeshInstanceBatch` rather than cloning Unreal's types | `MeshDraw`, `MeshRenderItem`, `MeshInstanceBatchBuilder` | Cache only view-independent state; explicit invalidation/lifetime; Prompt 17 |
| Draw sorting, state bucketing, compatible instancing and batch formation | Implement | `MeshInstanceBatchBuilder` already has batch keys and auto/preserved groups | Stable keys/tie-breaks, transparent-order preservation, measured crossover; Prompts 17, 26 |
| Per-view/per-pass constants and descriptor table preparation | Implement | pass parameter/binder, RHI upload and descriptor services | Preassigned or worker-local slices, no hot shared cursor; Prompts 17-20 |
| GPU-scene dirty-range coalescing, scan/compaction and upload planning | Implement | current rebuilt scene uploads; planned persistent `GpuScene` | Dirty-only work, deterministic compaction and retirement; Prompts 15, 26 |
| BLAS eligibility/build inputs and update/refit/rebuild/compaction decisions | Implement where supported by current RT product paths | classic acceleration-structure RHI and mesh RT data | Per-asset ownership, scratch/result lifetime, cold/create cost; Prompts 16, 17, 21, 27 |
| Classic TLAS and PTLAS instance planning | Implement separately | existing classic TLAS and PTLAS contracts | Add/remove/update/generation parity, never conflate build modes; Prompts 17, 21 |
| Frame-graph declaration production | Keep coordinator commit serial; measured private producers may run in parallel | mutable `FrameGraphBuilder` and typed pass setup | No concurrent builder mutation; private declarations merge deterministically; Prompt 17/20 only after evidence |
| Frame-graph compile algorithms: culling, lifetime analysis, barrier and queue planning | Measured candidate after topology reuse is evaluated | existing `FrameGraphCompiler` | Same compiled plan and diagnostics; never split barrier authority; Prompt 20/23 |

This table deliberately names light and shadow work more precisely than “build lighting.” Runtime light processing can include classifying visible lights, building light-grid or compact light records, deriving shadow views, intersecting light frusta with scene bounds, and building caster lists. The GPU lighting/shadow algorithm remains a renderer decision; CPU tasks only prepare independent immutable inputs. Unreal Swarm/Lightmass is a different case: distributed offline static-light baking. Sparkle should know that architecture but not create a distributed bake farm until a real static-light product and content workflow exist.

#### RHI Back End, Resource Work, and Command Production

| Common engine use case | Sparkle decision | Boundary that must remain visible | Required proof / K home |
|---|---|---|---|
| Shader source compilation and permutation cooking | Implement as bounded tool/process work | Compiling source is not creating a native pipeline | Prompts 04, 22, 27 |
| Shader package I/O, reflection/layout validation and generation publication | Implement | Workers build candidates; one owner publishes a complete immutable generation | Prompts 16, 27 |
| Graphics/compute PSO creation, precaching and cache persistence | Implement | Key discovery/deduplication, native creation, cache commit and not-ready policy are distinct stages | Prompt 27 |
| Ray-tracing pipeline/library/collection creation | Measured candidate; D3D12 collection or Vulkan pipeline-library mechanisms are backend-specific experiments | Do not leak backend pipeline-library nouns into common RHI | Prompt 27, with Prompt 21 RT parity |
| Buffer/image and view creation | Implement as bounded resource-stage work where the backend/device contract permits it | Exclusive output objects; render-owned commit and generation | Prompt 27 |
| GPU memory allocation/suballocation and memory binding | Implement owner-local or parallel by independent arenas; measure driver/allocator contention | Allocation, binding, residency, upload, and publication are not one operation | Prompts 15, 16, 27 |
| Descriptor allocation and descriptor writes/updates | Implement by lifetime domain | Persistent registry remains render-owned; transient recording pages are worker-local/preassigned | Prompts 18, 19, 27 |
| Decode/decompress/transcode and upload-copy preparation | Implement staged | Blocking I/O, CPU transform, copy recording, GPU completion and readiness are separate | Prompts 04, 09, 16, 27 |
| Command allocator/pool/list acquisition, begin, end and reset | Implement | One exclusive lease; GPU token controls reuse | Prompts 18, 19 |
| Pass-level parallel native command recording | Implement | Same frame-graph plan, private native list/buffer, coordinator submission | Prompt 20 |
| Intra-pass draw/dispatch/build-input chunk recording | Implement only for measured heavy passes | Bounded chunks, deterministic range order, serial threshold | Prompt 21 |
| Platform-neutral software RHI command encoding and later translation | Study/lab; know and defer in production | Unreal's `FRHICommand` replay layer is not present in Sparkle today | Tutorial 16; ADR gate below |
| Command-list aggregation/chaining and native submission batching | Implement | Aggregation preserves logical order; it does not concatenate native command buffers | Prompts 20, 21, 28 |
| Barrier/preamble/postamble recording | Implement under compiler/coordinator authority | Workers do not discover global state from completion order | Prompt 20 |
| Graphics/compute/copy queue submit and present | Keep one coordinator owner, not a parallel worker workload | Native queues often require external synchronization; ordering/timeline ownership stays singular | Prompts 20, 28 |
| Deferred destruction, descriptor/resource retirement and garbage collection | Implement as token-driven owner work | CPU task completion is not GPU last use | Prompts 15, 16, 22 |
| Readback, screenshot and capture encode/write | Implement staged, with provider/native calls as serial islands until audited | GPU readback completion precedes CPU encode/write; UI does not poll a mutable buffer | Prompts 14, 21, 22 |
| Device-loss/crash-dump and validation callback intake | Keep callback intake narrow and owner processing serialized | This is a correctness boundary, not useful fan-out work | Prompts 19, 22, 29 |

NVIDIA's Vulkan guidance explicitly lists command recording, buffer/image creation, descriptor updates, pipeline creation, and memory allocation/binding as task-graph candidates. That is a candidate inventory, not permission to call one device or allocator object concurrently without checking its API synchronization contract. Sparkle must pair every native operation with exclusive input/output ownership, a device-loss policy, a concurrency/memory budget, and backend validation.

#### Offline, Tool, and GPU-Concurrency Cases That Must Not Be Mislabelled

| Case | Decision | Why |
|---|---|---|
| Static light baking and distributed lighting build | Know and defer | Useful Epic Swarm/Lightmass precedent, but no current Sparkle static-lighting product justifies agents/coordinator/network scheduling |
| Shader/texture/mesh cooking and package emission | Implement | Real current tools with independent inputs, deterministic fan-in and transactional outputs |
| Mipmap/texture compression and mesh processing | Measured candidate in existing cookers | Useful only when current stages are CPU-critical and deterministic |
| GPU async compute/copy | Preserve and measure | GPU queue overlap is heterogeneous scheduling, not CPU multithreading |
| GPU-driven culling/indirect or device-generated commands | Future renderer feature, not a CPU-threading checkbox | It may remove CPU draw work rather than parallelize it; evaluate as renderer architecture |

#### Completeness Verdict

The prior J/K direction covered the broad architecture but was not sufficiently falsifiable at the use-case level. The additive closure is:

1. Prompt 17 must separately prove visibility/LOD, light preparation, shadow planning/caster lists, retained/dynamic draw preparation, sorting/instancing, skinning/morph, RT inputs, and dirty GPU-scene planning where those paths exist.
2. Prompts 18-21 must distinguish command preparation, native recording, optional software translation, ordered aggregation, native submission batching, and queue submission.
3. Prompt 27 must include shader workers, PSOs, RT pipelines where applicable, buffer/image/view creation, allocation/binding, descriptor updates, and resource readiness as bounded stages rather than one vague “resource creation” task.
4. Prompts 14/16/22 must prove readback/capture, retirement, validation callback, reload, resize, and shutdown ownership.
5. Every unavailable product feature is recorded as know/defer, not silently claimed as implemented. Sparkle does not add a fake light baker, RHI thread, or GPU-driven renderer merely to fill a résumé table.

## Current Repository State

### What Is Already Strong

The repository is not starting from a toy rendering loop.

- Renderer responsibilities are separated from the RHI and GameFramework.
- D3D12 and Vulkan are explicit backends rather than hidden behind a legacy immediate-mode API.
- The frame graph owns pass declarations, resource dependencies, compilation, barriers, and queue-typed submission batches.
- Graphics, compute, and copy queues already use explicit cross-queue wait tokens.
- Resource and descriptor retirement is fence-aware.
- Frame-graph histories and explicit pass parameter structures are already in place.
- Shader packages, reflection, parameter verification, recooking, and publication form a meaningful tools-to-runtime pipeline.
- The editor already splits host rendering into prepare, record, and submit calls.
- The asset pipeline has separate planners, importers, cookers, registries, and output publication concepts.

Those systems are the right substrate. The new design should expose their real independence rather than replace them.

### Current CPU Frame

The runtime is effectively:

~~~text
Main thread
  pump input/window
  apply level changes
  update GameScene
  capture/copy scene snapshot
  build render-scene arrays and GPU uploads
  prepare ray tracing state
  set up + compile frame graph
  record every pass serially
  submit queues
  present
~~~

The editor is also host-thread ordered:

~~~text
Main/editor thread
  poll shader recook
  update game/editor state
  renderer.PrepareHostFrame
  renderer.RecordHostFrame
  update and record UI
  renderer.SubmitHostFrame
~~~

This ordering is simple and currently correct, but it leaves the game, editor, renderer preparation, and command recording on one CPU critical path.

### Concurrency Inventory

| Area | Current evidence | Architectural consequence |
|---|---|---|
| Application | Runtime and editor ticks call game update and renderer phases sequentially | There is no CPU frame pipeline |
| GameFramework | Controllers, animation, morph updates, and snapshot capture run serially | Good future task candidates, but ownership must be clarified first |
| GameFramework controllers | Every controller receives mutable `GameScene&`; camera and Showcase controllers retain raw subsystem/object pointers or vector indices | Arbitrary controllers cannot be placed on workers; replace them with declared systems and narrow resource views |
| Transform/camera derived state | `const` world-matrix and camera-direction getters lazily write mutable caches; there is no hierarchy or committed transform buffer | Concurrent readers can race today; derived transform/camera evaluation must become an explicit phase before parallel readers |
| Animation/morph | Each clip allocates/evaluates a pose serially; morph application performs a nested scan and mutates mesh-owned skeletal state | Natural per-instance work exists, but outputs need private slots, stable targets, and deterministic merge/commit |
| Level/scene loading | Registry, manifest, payload, and referenced data are read synchronously; the old scene is cleared before replacement loading succeeds | Build a cancellable staged load into an isolated package, then atomically commit or retain the old scene |
| Core events | `Event` add/remove/broadcast mutate and traverse fixed callback storage without synchronization; level events broadcast inline | Treat existing events as owner-thread-only; workers publish results/commands and never broadcast arbitrary callbacks |
| Editor scene access | Panels retain `GameScene*`, identify objects by vector index, rebuild lists from live containers, and mutate scene data during ImGui drawing | Publish an immutable editor read model and translate UI edits into stable-ID commands committed at a world boundary |
| Editor background work | Shader recook is a bespoke polled `std::async`; no document/generation-scoped operation service exists | Replace one-off futures with scoped tasks, cancellation, immutable results, and main/render-owner application |
| Scene handoff | GameSceneSnapshot deep-copies many vectors but MeshInstanceSnapshot contains const Mesh* | This is neither a cheap packet nor a safe cross-thread boundary |
| Renderer scene build | RenderSceneDataBuilder rebuilds temporal, light, mesh, batch, skinning, and material-facing arrays | Large preparation fan-out exists but is encoded as one mutable procedure |
| GPU scene data | BuildRenderSceneGpuData creates/uploads light, mesh-instance, joints, RT vertices, indices, instances, and materials per frame | Static data churn hides the value of parallel preparation |
| Frame graph | Compile emits ordered queue submission batches with waits | Dependency information already exists and can drive CPU recording groups |
| Frame-graph execute | Batches and passes are recorded and submitted serially | GPU scheduling is explicit; CPU recording is not parallel |
| Pipeline state | Pass runtime storage and PSOs are lazily created from a mutable type-indexed map | Parallel pass entry could race and cause expensive surprise work |
| Pass binding | Uniform constants allocate through the shared RHI upload service during recording | Parallel recording needs worker-local or preplanned upload allocation |
| D3D12 recording | A frame/queue context advances mutable command allocator/list slots | A slot cannot be shared by recording jobs |
| Vulkan recording | A frame/queue context owns a command pool and advances mutable slots | Vulkan command pools require external synchronization; per-worker pools are required |
| Queue submission | Backend submission paths use mutexes/timeline tokens | Submission can remain render-thread owned; recording should not contend here |
| Scene lifetime | SceneRenderStateCoordinator directly observes level events, waits for idle, and clears renderer caches | Cross-thread commands and deferred retirement must replace direct calls |
| Shader recook | One std::async process task is polled; reload calls renderer.WaitForIdle | Good pilot for a background lane and generation-based safe swap |
| Asset cooker | Stages, scenes, texture requests, texture cooks, and shader cook nodes loop serially | Natural task DAG with high-value coarse work |
| Launcher | ProcessRunner uses a reader thread around blocking child-process I/O | Blocking I/O must not consume frame workers |

### Binding Legacy-Concurrency Convergence Audit

The preceding table describes architectural opportunities. This audit is stricter: it records the concurrency mechanisms that already exist and makes each one an explicit migration obligation. Existing code receives no grandfather clause. A mutex, atomic, thread, future, native wait, or device-idle call may remain only when its owner, protected invariant, lifetime, blocking policy, and target architecture are defensible after the relevant K prompt.

Use these dispositions consistently:

- **KEEP + HARDEN** means the synchronization protects a real boundary that tasks do not remove. Narrow its critical section, state its invariant, test it, and keep it private.
- **OWNER-ONLY** means move mutation to the named owner and use commands/results at the boundary. A defensive native mutex may remain only if the API or provider can genuinely enter from more than one thread.
- **REPLACE** means migrate the consumer to SparkleTasks or an owned data protocol and delete the old mechanism in the named prompt.
- **DELETE WAIT** means replace a global drain with a completion token, generation, or deferred-retirement rule. Final shutdown and unrecoverable device reinitialization are separate, explicitly labelled cases.
- **PROVE OR REMOVE** means the current code suggests cross-thread intent but the repository does not yet demonstrate a necessary cross-thread consumer. Do not preserve speculative synchronization.

| ID | Current source and mechanism | Adversarial finding | Required target and owning K prompt | Closure proof |
|---|---|---|---|---|
| LC-01 | [ShaderRecookCoordinator.cpp](../../../Engine/Application/Private/ShaderRecook/ShaderRecookCoordinator.cpp) uses scoped Background/BlockingIo tasks and owner-thread generation acceptance; the old future/async/polling path is deleted | **PROCESS-LIFETIME PART CLOSED in Prompt 04.** Request/result values cross the task boundary; stale/failing generations cannot publish. Reload still calls `Renderer::WaitForIdle` and remains LC-16 data-path debt | **DELETE WAIT** through generation swap and token retirement in Prompt 16 | no future/async/polling remains; failure preserves active packages; stale completion is rejected; Prompt 16 proves accepted reload does not idle the device |
| LC-02 | [LauncherBackend.cpp](../../../Tools/Launcher/SparkleLauncher/Private/Gui/App/LauncherBackend.cpp) launches bounded BlockingIo tasks; mapping/execution are separate private operation files and immutable Qt messages return through the owner queue | **CLOSED in Prompt 04:** no operation QThread exists and QObject/window access stays on the GUI owner | re-audit app-close/cancel/restart in Prompt 22 | bounded worker count and Qt-affinity capture falsify the closure |
| LC-03 | [ProcessRunner.cpp](../../../Tools/Launcher/SparkleLauncher/Private/Core/ProcessRunner.cpp) is a thin adapter to Core `Process::ChildProcess`; overlapped pipe/process/cancel events and Windows job ownership replace reader thread, polling, and atomic cancellation | **CLOSED in Prompt 04:** one BlockingIo call owns the child tree, output pipe, cancellation, diagnostics, and terminal result | harden staged process variants in Prompt 27 | disposable descendant-pipe cancellation settled sub-second; launch/read/exit fault injection remains the falsifier |
| LC-04 | [AssetCookerToolProcess.cpp](../../../Tools/Cooking/AssetCooker/Private/Dispatch/AssetCookerToolProcess.cpp) uses the same cancellable Core child-process contract | **CLOSED in Prompt 04:** outer CLI may synchronously call the adapter, while scheduled callers must select BlockingIo; there is no native infinite wait in the adapter | call graph and Prompt 27 staged-process stress retain the policy | no FrameCritical/Background stack may contain the event-driven child wait |
| LC-05 | [InputSystem.h](../../../Engine/Platform/Public/Input/InputSystem.h) and [InputSystem.cpp](../../../Engine/Platform/Private/Input/InputSystem.cpp) previously invoked callbacks while holding `m_CallbackMutex` | **CLOSED in Prompt 03:** the registry and dispatch path are explicitly owner-thread-only, the unnecessary mutex is deleted, and each dispatch invokes a copied eligible-callback snapshot | **OWNER-ONLY:** workers publish commands/results rather than input callbacks. Registry changes affect the next snapshot; nested dispatch takes a fresh snapshot; nested deferred-phase processing does not recursively drain | transient self-unsubscribe, unsubscribe-other, subscribe-during-dispatch, nested immediate dispatch, and deferred tests passed; no callback executes under a registry lock |
| LC-06 | [Logger.cpp](../../../Engine/Core/Private/Diagnostics/Logger.cpp) uses a registry mutex, a mutex-backed sink, a relaxed level atomic, and release/acquire initialization publication | Logging is legitimate cross-thread infrastructure, not a job-system workload. Replacing it with tasks would be incorrect, but lock scope and initialization edges must be explainable | **KEEP + HARDEN** in Prompts 00 and 24; keep registry/sink internals private, avoid calling unknown code under the registry lock, and document each atomic invariant | concurrent initialize/get/set/shutdown policy is tested; every atomic order has a stated edge; no scheduler dependency or public mutable registry is introduced |
| LC-07 | [Timer.h](../../../Engine/Core/Public/Time/Timer.h) uses a relaxed atomic pause flag | Relaxed is sufficient only for an independent flag; the atomic must not be mistaken for publication of timer state, and no necessary cross-thread caller is established by its presence | **PROVE OR REMOVE** in Prompts 00 and 24: document a real cross-thread pause contract or restore owner-thread non-atomic state; never use the flag to publish adjacent fields | call-site audit names the writer/readers and invariant; test proves only the flag is communicated, or the unnecessary atomic is gone |
| LC-08 | [StreamlineRuntimeSupport.cpp](../../../Engine/Renderer/Private/Streamline/StreamlineRuntimeSupport.cpp) protects global initialization/device/presentation state with one mutex and holds it across Streamline API calls | A global lock makes provider affinity implicit and risks lock-held re-entry or long external calls; it must not become permission for arbitrary threads to call the provider | **OWNER-ONLY/NARROW** in Prompts 13, 16, and 28: render coordinator owns frame/device/presentation provider actions; application lifecycle owns init/shutdown; retain synchronization only for a documented SDK callback boundary and do not hold an engine lock across provider calls when a two-phase transition is possible | provider affinity assertions, enabled/disabled/failure/shutdown stress, no uncontrolled shared queue use, and a documented SDK re-entry policy |
| LC-09 | [D3D12CommandQueue.cpp](../../../Engine/RHI/Private/D3D12/Commands/D3D12CommandQueue.cpp) serializes submit/state with `m_submissionMutex` and native event waits with `m_cpuWaitMutex` | Mutex-protected submission is not a submission architecture. Infinite CPU waits are valid only at declared host/reuse/drain boundaries, never inside frame tasks | **OWNER-ONLY** in Prompts 13 and 18; render coordinator submits. Keep a narrow CPU-wait serialization primitive only if multiple declared host waiters remain; replace routine drains with tokens in Prompt 16 and verify in Prompt 28 | wrong-thread submit asserts; workers only record; every CPU wait has a boundary/timeout diagnostic; ordinary reload/scene work uses tokens |
| LC-10 | [VulkanCommandQueue.cpp](../../../Engine/RHI/Private/Vulkan/Commands/VulkanCommandQueue.cpp) locks the shared native queue around submit/present and submitted-value reads | Vulkan requires external queue synchronization, so the native mutex can be necessary; it does not justify multi-owner submission | **KEEP + HARDEN under OWNER-ONLY** in Prompts 13 and 19: the coordinator owns submit/present; the mutex documents Vulkan/provider sharing only where unavoidable; Prompt 28 proves provider queue ownership | one submission authority, Vulkan external-synchronization compliance, provider sharing contract, and no worker submit/present |
| LC-11 | [D3D12DescriptorAllocator.cpp](../../../Engine/RHI/Private/D3D12/Descriptors/D3D12DescriptorAllocator.cpp) uses one mutex for persistent allocation/free | The lock is acceptable for infrequent persistent descriptors but becomes a frame-hot contention point if parallel recording allocates through it | **KEEP PERSISTENT / REPLACE TRANSIENT** in Prompt 18: preplan or lease worker-local transient descriptor pages; render owner retains persistent allocation and fence-delayed reclamation | parallel recording performs no contended per-draw allocation; pages cannot reset before their GPU token; persistent handle reuse is validated |
| LC-12 | [VulkanDescriptorAllocator.cpp](../../../Engine/RHI/Private/Vulkan/Descriptors/VulkanDescriptorAllocator.cpp) places table, registration, write, lookup, retirement, and recycling under one mutex | One broad allocator lock hides distinct persistent, per-frame, and recording-time ownership and could serialize every worker | **SPLIT BY LIFETIME** in Prompt 19: worker/frame-local pool leases for transient recording, render-owned persistent registry, token/generation retirement; retain only measured shared-state locks | command-pool/descriptor-pool external synchronization passes validation; no lock-held native/update work on the recording hot path; stale handles are rejected |
| LC-13 | [D3D12LinearAllocator.cpp](../../../Engine/RHI/Private/D3D12/Resources/D3D12LinearAllocator.cpp) uses atomic compare/exchange for a shared bump offset and relaxed high-water tracking | A lock-free bump pointer can still be a contended cache line; reset safety is external and not encoded by the atomics; acquire/release does not by itself make reuse safe | **REPLACE HOT-PATH SHARING** with worker-local pages/leases and GPU-token reset in Prompt 18; formally audit remaining atomics in Prompt 24 | allocation ranges never overlap, reset-before-retire fails, workers do not contend on one offset, and memory orders state the actual publication invariant |
| LC-14 | [D3D12GpuMemoryAllocator.cpp](../../../Engine/RHI/Private/D3D12/Memory/D3D12GpuMemoryAllocator.cpp) and [VulkanGpuMemoryAllocator.cpp](../../../Engine/RHI/Private/Vulkan/Memory/VulkanGpuMemoryAllocator.cpp) lock allocation-record, diagnostic, and pending-release state | These locks protect real bookkeeping/lifetime state, but snapshots and destruction must not widen a frame-hot critical section or return unstable pointers | **KEEP + HARDEN** in Prompts 16, 18/19, and 24: render-owner mutation where possible, copy diagnostic aggregates under lock then format/use after unlock, token-gated release, no worker-visible mutable record pointer | delayed-GPU and concurrent diagnostic stress; no external/provider callback while locked; returned handles/records have an explicit lifetime |
| LC-15 | [VulkanRhi.cpp](../../../Engine/RHI/Private/Vulkan/Device/VulkanRhi.cpp) protects validation callback messages with `m_diagnosticMessagesMutex` | Driver callbacks may be concurrent, so synchronization is required; consuming or logging while holding the callback lock can create re-entry and stalls | **KEEP + HARDEN** in Prompts 00 and 24: callback only appends bounded owned data; owner swaps/drains then processes outside the lock | concurrent callback/drain stress has bounded memory, no re-entry deadlock, and preserves validation messages |
| LC-16 | Renderer/RHI lifecycle contains many `WaitForIdle` calls: shader acceptance, scene invalidation, image-provider refresh, frame-graph refresh, resize, swap-chain/context/UI/backend destruction, and explicit flush | The sites are not equivalent. Current Vulkan resize can drain in FramePipeline, device services, and swap chain for one operation. Treating them uniformly would either preserve stalls or remove necessary destruction safety | **CLASSIFY ALL** in Prompt 00; **DELETE DATA-PATH WAITS** in Prompts 13-16 and 22; keep exactly one owner-level final drain for shutdown/device reinitialization. Resize/recreate uses the narrowest required token or one documented drain until tokenized replacement is proven | a site-by-site ledger names caller, reason, frequency, owner, queues/resources covered, replacement, and prompt; no duplicate nested drain; profiler proves no ordinary scene/reload/capture idle |
| LC-17 | [LauncherGuiApp.cpp](../../../Tools/Launcher/SparkleLauncher/Private/Gui/App/LauncherGuiApp.cpp) launches a generation-specific shadow copy; [LauncherMainWindowOperations.cpp](../../../Tools/Launcher/SparkleLauncher/Private/Gui/Shell/LauncherMainWindowOperations.cpp) launches the selected editor/runtime product | **TIMING DEBT CLOSED in Prompt 04:** the 150 ms deletion/copy retry is gone. Both remaining `startDetached` calls intentionally transfer lifetime to a replacement launcher or independent selected product; neither escapes owned operation work | Prompt 22 restart/application-close stress remains the falsifier; stale generation cleanup is storage maintenance, not synchronization |
| LC-18 | D3D12 frame resources and command queues use native infinite fence waits; Vulkan uses timeline/device waits | GPU completion sometimes requires a blocking host boundary, but an infinite worker wait causes pool starvation and masks device-loss/hang diagnosis | **OWNER-ONLY** in Prompts 13, 18, 19, and 28: workers return dependencies; coordinator/host parks only at frame-slot reuse, explicit flush, or shutdown, with development timeout/hang diagnostics and device-loss policy | repository audit finds no GPU wait on a SparkleTasks worker; delayed/hung GPU tests identify the waited token and settle shutdown policy |

#### Current `WaitForIdle` Call-Site Census

This census distinguishes calls from wrapper declarations/forwarders. Prompt 00 must regenerate it from the current call graph; these initial classifications are not implementation claims.

| Call-site group | Current purpose | Initial classification | Required convergence |
|---|---|---|---|
| `ShaderRecookCoordinator::ApplyCompletedResult` | accepted shader reload | data-path debt | generation-safe swap plus last-use token retirement; zero idle in Prompt 16 |
| `SceneRenderStateCoordinator::InvalidateSceneScopedRendererState` | level/scene renderer-cache clear | data-path debt | sequenced scene-generation invalidation and deferred retirement in Prompts 13/15/16 |
| `RendererSystemRoot::RefreshImageProviders` | recreate provider-facing image state | rare transition currently implemented as global drain | coordinator command, generation ownership, and narrow last-use waits in Prompts 13/16; retain a drain only if the provider contract proves it unavoidable |
| `FramePipeline::RefreshFrameExecution` | destroy/rebuild frame contexts and graph | resize/settings data-path debt | retire the old frame-execution generation by its queue tokens; no unconditional drain in Prompt 16 |
| `FramePipeline` pending-resize branch | pre-resize drain, then calls `RefreshFrameExecution`, which drains again | duplicate data-path debt on both backends | one resize owner; eliminate the duplicate before tokenization is considered complete |
| `VulkanRenderDeviceServices::ResizeSwapChain` and `VulkanSwapChain::Resize` | each drains again beneath the FramePipeline resize | nested Vulkan duplicate; one resize can currently reach four idle calls when the outer refresh path is counted | collapse immediately to one coordinator-owned boundary, then use per-generation/per-image completion where supported in Prompts 13/16/19 |
| `CloseExecuteAndFlushCurrentFrame` in D3D12/Vulkan services | explicit submit-and-flush API | declared host/debug boundary, subject to call-site audit | never reachable from a frame worker or routine update; name the reason and waited tokens; delete consumers using it as lifecycle convenience |
| D3D12/Vulkan ImGui backend `Shutdown` | destroy UI backend resources | teardown | preserve safety but make it part of one top-level coordinator drain; nested child shutdown must not drain independently |
| `RendererSystemRoot`, D3D12/Vulkan device services, `VulkanRhi`, `VulkanSwapChain`, and `VulkanCommandContext` destructors | layered teardown safety | final shutdown, but duplicated across ownership layers | one owner performs the final GPU drain before child destruction; lower layers assert safe teardown or use already-complete tokens |
| D3D12/Vulkan RHI and render-device `WaitForIdle` methods | wrapper/native implementation | mechanism, not a classification | keep a narrow private final-drain operation; prohibit using the public-looking wrapper as routine lifetime management |

The current resize path is a useful lesson: every individual wait can look locally defensive while their composition is globally poor. High-quality concurrency review follows the full call chain and counts native drains per user operation; it does not stop after classifying method names.

This ledger is a verified starting snapshot, not permission to ignore later discoveries. Prompt 00 must regenerate the search from the then-current repository and append any new item before implementation begins. Prompt 22 must drive the ledger to zero **unclassified** primitives; it must not drive the repository to zero mutexes or atomics. The quality target is zero accidental ownership, zero duplicate scheduling systems, zero unexplained blocking, and zero lock-held arbitrary callbacks.

### Lock and Wait Review Standard

For every retained or introduced critical section, the implementing prompt must answer all of the following in code review:

1. What invariant and fields does the lock protect?
2. Which thread roles may acquire it, and what is the total lock order?
3. Can it be acquired from a task that may suspend, join, invoke a callback, call a provider/driver, perform I/O, allocate unpredictably, or wait for the GPU? If yes, redesign or prove why the boundary is unavoidable.
4. Is user, UI, event, provider, logging, destruction, or native API code executed while held? Default answer must be no; use snapshot-under-lock/apply-after-unlock or a two-phase state transition.
5. Is the data better expressed as single-owner state, immutable publication, per-worker storage, deterministic merge, or a completion token?
6. What contention, re-entry, cancellation, shutdown, and failure test would falsify the design?

A mutex is not automatically legacy debt, and lock-free code is not automatically higher quality. `VulkanCommandQueue` external synchronization and validation-callback ingestion are examples of legitimate boundaries. The InputSystem callback-under-lock and a shared atomic upload bump pointer on a future recording hot path are examples that require architectural correction. The deciding questions are ownership, lifetime, blocking, and measured contention.

### Required Repository Audit Query

At Prompt 00, after every stage touching concurrency, and at Prompt 22/29 closure, search at least the owned Engine/Tools/Projects source roots for:

~~~text
std::thread|std::jthread|std::async|std::future|std::mutex|std::shared_mutex
std::condition_variable|std::atomic|memory_order|wait_for|sleep_for
QThread|QtConcurrent|QFuture|QMutex|QThreadPool|msleep|startDetached
WaitForSingleObject|CreateEvent|SetEventOnCompletion|WaitForIdle|vkDeviceWaitIdle
TaskExecutor|TaskScope|TaskEvent|ParallelFor|BlockingIo
~~~

Every hit is assigned a ledger ID, owner, disposition, target prompt, and evidence. Generated, build, external, and third-party trees may be excluded from the ownership ledger, but wrapped third-party behavior and internal worker counts remain part of oversubscription and shutdown analysis.

### The Most Important Correctness Finding

The current GameSceneSnapshot looks detached because it owns vectors, but MeshInstanceSnapshot carries a raw const Mesh pointer. The renderer also reaches GameScene through RendererSystemRoot for diagnostics and coordinates scene invalidation through direct event callbacks.

That means a dedicated render thread cannot safely be introduced by simply moving Renderer::OnRender to another thread. The game could unload or mutate the pointed-to mesh while the renderer is consuming it. A mutex around GameScene would serialize the two systems and make frame latency unpredictable. The boundary must use stable asset handles, immutable asset ownership, and render-owned proxies.

### The Most Important Performance Finding

The current renderer repeatedly transforms a full scene snapshot into full render arrays and creates/uploads structured GPU data every frame. This makes CPU work and upload traffic proportional to the whole scene even when only camera and transforms changed.

The primary optimization is therefore not “parallelize the rebuild.” It is:

1. make static state persistent,
2. encode structural changes as deltas,
3. upload dynamic and dirty ranges,
4. then parallelize the remaining meaningful work.

Parallelizing avoidable work is not a production architecture.

## Learning Foundations the Implementation Must Demonstrate

### Data Race and Ownership

A data race occurs when threads access the same memory concurrently, at least one access writes it, and the accesses are not ordered by synchronization. In C++, a data race is undefined behavior, not merely an occasional stale read.

Sparkle’s default rule should be stronger than “lock shared state”:

> Mutable data has one owning thread or task. Cross-thread data is immutable, transferred, or represented by an explicitly synchronized concurrent object.

This yields clearer systems than widespread mutex protection.

### Happens-Before and Publication

Writing a frame packet and placing its slot index in `RenderFrameQueue` are separate operations. The consumer must not see the index before the packet contents are visible.

The frame queue should use a release operation when the producer publishes a ready slot and an acquire operation when RenderThread consumes it. Reusing the slot requires a second acknowledged transition. The queue implementation may use atomics and semaphores internally, but its public contract must describe:

- producer-only writable state
- immutable published state
- consumer-only processing state
- retired/reusable state

### Mutexes, Atomics, and Semaphores

- Use a mutex when several values form one invariant or an operation must be mutually exclusive.
- Use atomics for small independent state transitions, counters, indices, and publication protocols.
- Use a semaphore or condition variable to park a thread awaiting work or a bounded slot.
- Do not replace a straightforward mutex with an intricate lock-free structure without measured contention and a complete lifetime proof.

The task scheduler’s injection queue and sleep/wake mechanism may use locks. Work-stealing deques can be specialized later. Correct ownership and task granularity matter more than a “lock-free” label.

Synchronization review rules:

- every atomic documents the invariant it protects and why its memory order is sufficient
- release/acquire is used for ownership publication; relaxed ordering is limited to independent statistics/counters
- condition-variable waits always recheck a predicate and sleep/wake state changes cannot lose a notification
- locks have a stable order where more than one can be held
- no external callback, provider call, file operation, queue submission, or task wait occurs while holding a scheduler/renderer cache lock
- generational handles and slot states prevent stale reuse/ABA-style confusion
- a lock-free queue may not store reclaimable raw pointers without a proven reclamation scheme

### False Sharing and Cache Locality

Two unrelated atomics on one cache line can make cores repeatedly invalidate the same line. Scheduler hot counters, worker deque indices, per-worker allocators, and frame slot state should use destructive-interference-size-aware padding. Data-oriented render arrays should separate hot, frequently updated fields from cold asset metadata.

### Deadlock, Livelock, Starvation, and Oversubscription

- Deadlock: tasks or threads wait in a cycle.
- Livelock: threads run but repeatedly prevent useful progress.
- Starvation: a task is continually postponed by other work.
- Oversubscription: too many runnable/blocking threads compete for the available cores.

Sparkle should prohibit waiting from normal worker tasks. Dependencies and continuations express ordering. Host-thread waits are reserved for application boundaries, shutdown, tests, and bounded frame backpressure. Blocking file/process operations use their own limited lane.

### Task Granularity and the Critical Path

Task scheduling has a cost. A parallel_for over a dozen trivial transforms can be slower than a loop. Each parallel algorithm needs:

- a sequential threshold
- a grain-size policy
- enough independent work to amortize scheduling
- measurement of the dependency graph’s critical path, not just total task count

Amdahl’s law still applies: accelerating parallel portions cannot remove serial setup, joins, submission, or presentation. The architecture must reduce serial work as well as distribute work.

### ECS, DOD, and Job Systems Are Different Layers

The implementation must teach the distinction:

- **ECS** answers how world instances are identified, how data components are stored/queried, and where behavior systems live.
- **Data-oriented design** starts from access patterns, working sets, transformations, and hardware behavior; ECS is one useful organizational tool, not proof that data is well designed.
- **Job/task system** schedules independent work and dependencies; it does not discover safe component access automatically unless the system/query layer supplies that contract.

Key concepts to demonstrate:

- entity ID versus object address and generation-based stale-handle rejection
- composition versus inheritance
- hot/cold component separation
- AoS, SoA, and array-of-structures-of-arrays tradeoffs
- sparse-set lookup and dense iteration
- archetype/chunk locality versus composition-change cost
- query leading-set choice and multi-component gather behavior
- value mutation versus structural mutation
- frozen iteration epochs and deferred command playback
- read/write access hazards feeding a task DAG
- cache lines, prefetching, false sharing, vectorization opportunity, and allocation stability

A system that stores one heap object per entity and invokes virtual `Update()` in parallel is not data-oriented merely because it uses entity/component terminology. Conversely, a packed renderer array can be excellent DOD without being an ECS. Sparkle must be able to explain and measure both.

## Expert Concurrency Core

The first foundation is enough to avoid obvious races. An AMD/NVIDIA graphics or systems interview can go deeper: the candidate may be asked to reason about a legal execution, choose a primitive, debug a scheduler trace, explain a cache-coherence regression, or separate CPU concurrency from GPU queue concurrency. This section establishes that deeper vocabulary. It does not authorize clever production code without a measured Sparkle use case.

### The C++ Memory Model Beyond “Use Acquire/Release”

Every atomic object has a modification order. A successful release/acquire publication can make earlier non-atomic payload writes visible, but only when the acquire observes the release or its release sequence. Atomics do not automatically order unrelated state, protect an object's lifetime, make a compound invariant indivisible, or flush CPU data to a GPU.

| Operation/order | What it is good for | What it does not prove | Sparkle example |
|---|---|---|---|
| `memory_order_relaxed` | indivisible counter/index operation with no cross-object visibility claim | publication, lifetime, or relative ordering of payload fields | statistics and monotonic debug counters only |
| release store / acquire load | one-way publication of fully initialized immutable state | safe reuse after consumption or GPU completion | frame-slot `Writing → Ready` transition |
| acquire-release read-modify-write | state transition that both consumes prior publication and publishes subsequent work | correctness of a badly defined state machine | claim/retire transitions in bounded slots |
| sequentially consistent atomic | easiest global atomic ordering model and best starting point for a litmus test | compound non-atomic invariants or progress | first reference implementation of a tiny protocol |
| compare-exchange loop | conditional state/index update | absence of ABA, starvation, or safe pointer reclamation | bounded index/state transition after generation proof |
| atomic fence | ordering when the synchronization carrier is separate from payload access | a substitute for a clearly paired synchronizes-with edge | avoid initially; use only with a written proof and test |

Interview rule: first make the protocol correct with a mutex or sequentially consistent atomics, identify the exact synchronizes-with edge, then justify weaker ordering. “x86 is strongly ordered” is not a C++ proof and does not cover ARM hosts or compiler reordering.

For each non-relaxed atomic protocol, draw:

1. the atomic object and all its legal states;
2. which write publishes which payload;
3. which read observes that write;
4. who owns the payload before and after the edge;
5. the separate edge that permits memory reuse/destruction;
6. behavior when a generation wraps, cancellation races completion, or shutdown begins.

### Progress, ABA, and Reclamation

Thread-safe and lock-free answer different questions. Mutual exclusion may be correct and fast under low contention. Lock-free means system-wide progress is guaranteed despite a stalled participant; it does not guarantee that a particular participant progresses. Wait-free adds per-operation progress bounds and is substantially harder. Neither label implies low latency, fairness, small memory use, or correct reclamation.

ABA occurs when a location changes `A → B → A`; a compare-exchange sees `A` again and cannot tell that the identity/lifetime changed. Generational indices avoid this in Sparkle's bounded task, entity, asset, and frame-slot handles. A tagged index is not sufficient if generation wrap can occur while a stale handle remains reachable. Raw-pointer lock-free structures additionally need reclamation such as epochs, hazard pointers, or another proof that removed nodes remain alive. Sparkle should not invent such a scheme for its first scheduler.

Production decision:

- bounded index/generation protocols are allowed after exhaustive state-machine tests;
- worker-local queues may later adopt a proven bounded work-stealing deque behind the private executor;
- cross-module raw-pointer lock-free containers are forbidden;
- epoch/hazard-pointer/RCU techniques are learning subjects until a measured consumer and lifetime design exist.

### Synchronization Primitive Selection

| Need | Default primitive/pattern | Review questions |
|---|---|---|
| protect several fields that form one invariant | mutex + scoped lock | Is the critical section bounded? Can callbacks/I/O/waits escape it? |
| sleep until a predicate changes | condition variable or semaphore | Is the predicate checked in a loop? Can a wake be lost? Is shutdown part of the predicate? |
| one-time construction | static initialization or `call_once` | Is destruction/order across modules still safe? |
| N-way phase rendezvous | task fan-in; latch/barrier only in isolated tests or fixed teams | Would a late/cancelled participant deadlock the phase? |
| single producer/single consumer stream | bounded SPSC state machine | What is the full/empty/backpressure policy? Who reclaims slots? |
| many producers into one owner | bounded MPSC/injection queue, initially mutex-backed | Is contention material? Is per-producer batching cheaper? |
| independent numeric aggregation | task-local reduction + deterministic merge | Is floating-point ordering allowed to change? |
| asynchronous ownership operation | task scope + continuation + generation | Who commits, cancels, and destroys it? |

Spin waiting is acceptable only for a measured, extremely short ownership handoff where the running owner is guaranteed to be scheduled. Spinning while the owner is descheduled burns a core and can worsen the delay. Adaptive spin-then-park policies belong inside proven primitives, not at call sites.

### Scheduler and OS Pathologies

Expert reasoning includes failures that do not look like data races:

- **priority inversion:** a high-priority render/pacing thread waits for a lock held by background work;
- **lock convoy:** many threads wake and serialize through the same lock;
- **thundering herd:** one event wakes far more workers than useful work exists;
- **preemption:** the critical-path task is runnable but not scheduled;
- **migration:** a hot task moves across cores/cache domains and loses locality;
- **SMT contention:** sibling logical processors compete for execution/cache resources;
- **NUMA/chiplet traffic:** workers repeatedly consume memory homed in another locality/cache domain;
- **oversubscription:** engine workers, driver threads, shader/compiler libraries, tools, and OS work exceed useful runnable capacity;
- **priority starvation:** background work never settles or foreground work is flooded by incorrectly promoted tasks.

Worker count must therefore be workload- and machine-policy driven, not `hardware_concurrency() - 1`. Sparkle should expose one internal worker-count override, record physical/logical processor and cache-domain information in benchmark metadata, and measure 0/1/2/N plus selected physical-core counts. The default should let the OS schedule normally. CPU Sets, affinity, priority, or QoS become opt-in experiments only when Windows Performance Analyzer or equivalent evidence proves a scheduling problem. Hard pinning every worker is not the target architecture.

### Parallel Algorithm Vocabulary

`parallel_for` is only one pattern. Expert engine work should recognize:

| Pattern | Engine use | Correctness hazard | Sparkle proof |
|---|---|---|---|
| map/independent range | transforms, animation pose evaluation, draw preparation | overlapping output or hidden shared cache | disjoint range and byte/state equivalence |
| filter/compaction | visibility lists, dirty records | nondeterministic append and output capacity | count/scan/scatter or task-local lists + ordered merge |
| reduction | bounds, statistics, light/bin summaries | non-associative floating point and shared accumulator contention | task-local partials + defined merge order |
| prefix scan | output offsets, compacted upload ranges | phase dependencies and off-by-one errors | serial oracle plus randomized sizes |
| sort/bucket/partition | material/pass grouping, deterministic cooker products | unstable ordering and skewed buckets | explicit stable key/tie-break and worst-case data |
| producer/consumer pipeline | I/O → decode → validate → upload/commit | unbounded queues, stale work, ownership gaps | bounded stages, cancellation, generation commit |
| DAG/fork-join | systems, tools, renderer preparation | worker waits and excessive tiny nodes | prerequisites/continuations and critical-path capture |
| double/triple buffering | frame packets, dynamic uploads | confusing CPU reuse with GPU completion | independent CPU acknowledge and GPU retirement tokens |

SIMD and GPU wave parallelism are different layers but interact with CPU DOD. Dense, aligned, branch-light ranges may improve both CPU vectorization and task scalability. More CPU threads cannot repair a bandwidth-bound layout; use counters and scaling curves to distinguish compute, memory, synchronization, and scheduler limits.

### CPU Tasks, GPU Queues, and Heterogeneous Concurrency

Four timelines must remain distinct:

1. CPU worker execution;
2. CPU render coordination/submission;
3. GPU graphics/compute/copy queue execution;
4. presentation/display pacing.

An asynchronous compute pass can overlap graphics on the GPU while its command buffer was recorded serially on the CPU. Parallel command recording can reduce CPU time while producing one serialized GPU queue. A copy queue may overlap uploads, but cross-queue fences, ownership transfers, bandwidth contention, and additional submissions can erase the benefit. A second CPU frame in flight can increase throughput while increasing input-to-present latency.

For every concurrency claim, state which timelines overlap, which dependency/fence permits it, which resource changes owner, and which metric proves improvement. Never call GPU async compute “multithreaded rendering” without qualification.

## Guided Concept Tutorials and Learning Exercises

The sections below are the tutor narrative for the architecture that follows. They introduce the reasoning in dependency order. Implementation details and binding rules later in the document remain authoritative.

### Tutorial 1 — A Thread Does Not Make Data Safe

Start from the current temptation: move `Renderer::OnRender` or `GameScene::Update` onto another thread. The function now runs concurrently, but every object it reaches retains its old ownership. A `MeshInstanceSnapshot` raw mesh pointer can outlive its scene; `Transform` and camera getters can write lazy caches during nominal reads; editor panels can mutate the same scene arrays; renderer caches can be cleared from level callbacks.

The central lesson is that a data race is about unsynchronized conflicting memory access, not about whether a crash occurs. Two reads are safe only when they are truly reads. A read and write need an ordering edge or exclusive ownership. Object lifetime is an additional problem: perfectly synchronized access to a destroyed object is still invalid.

Three broad solutions exist:

| Strategy | Advantage | Cost / limitation | Sparkle use |
|---|---|---|---|
| one owner thread | strongest local invariant, few locks | commands/publications required for other threads | world structural commit, render/RHI state, editor UI |
| immutable publication | readers need no mutation lock | generation storage and reclamation required | frame packets, read views, scene-load packages |
| exclusive data partition | true parallel writes | range construction and non-overlap proof required | ECS component ranges, pose outputs, recording contexts |

Broad locks are sometimes correct, but they are rarely the desired architecture for frame-critical scene/renderer separation. A shared `GameScene` mutex would turn every editor/game/render access into lock ordering and contention policy while still leaving pointer lifetime and GPU retirement unresolved.

Learning exercise:

1. Trace `GameScene::Update`, `CaptureSnapshot`, renderer scene build, and level clearing.
2. Mark each mutable object with one current writer and every reader.
3. Identify one const method that mutates a cache.
4. Describe an interleaving that makes a raw mesh pointer stale.
5. Redesign the interaction using one owner plus publication, without writing code.

Teach-back questions:

- Why is “only one thread usually writes” not a synchronization contract?
- Why does a mutex around the pointer not automatically protect the pointee's lifetime?
- Which Sparkle state should remain thread-owned instead of becoming generally thread-safe?

### Tutorial 2 — Happens-Before, Publication, and Reclamation

Suppose GameThread fills a frame packet, publishes its slot through `RenderFrameQueue`, and RenderThread sees that slot. Seeing a valid slot index does not by itself guarantee the packet writes are visible in the required order. Publication needs a release operation by the producer and an acquire operation by the consumer, or an equivalent mutex/semaphore contract.

Think in three events:

~~~text
producer writes payload
        │ sequenced-before
producer release-publishes slot
        │ synchronizes-with
consumer acquire-claims slot
        │ sequenced-before
consumer reads payload
~~~

That proves visibility, not reuse. A second edge is needed before the producer resets the arena. GPU resources add a third lifetime because CPU consumption and GPU completion are different events.

Use atomics for small state machines/counters whose invariant fits the atomic protocol. Use mutexes for compound mutable invariants where lock scope is clear. Use semaphores/condition variables to park threads when no work exists. An atomic container pointer does not make the container's elements safe; an atomic reference count does not prove GPU last use.

Learning exercise:

- draw the frame-slot transitions `Free → Writing → Ready → Rendering → Retired → Free`
- label the release/acquire edge
- label CPU acknowledgement separately from GPU completion
- explain what goes wrong if `Ready` is published before the final packet field is written
- explain what goes wrong if the arena resets immediately after render acquire

### Tutorial 3 — From Thread Pool to Job System

A thread pool answers “where can this callable run?” A production job system must additionally answer “when is it ready, what owns it, what follows it, what if it fails, and how does it settle during shutdown?”

SparkleTasks uses a fixed worker set because permanent animation, culling, recording, and cooker threads would be idle at different times. Work stealing lets an idle worker help another queue. Prerequisite counters make a task runnable only when dependencies complete. Continuations and fan-in replace worker waits. Structured scopes bind asynchronous work to application, world, document, asset, frame, or tool lifetimes.

The critical distinction:

~~~text
bad:  parent task launches child and blocks a worker waiting
good: child completion decrements a continuation prerequisite
~~~

Waiting on a worker can exhaust the pool when every worker waits for work that is queued but cannot run. Main/render owner threads may perform bounded joins at explicit phase boundaries because they are coordinators, not general worker tasks.

Tradeoffs:

- work stealing improves balance but makes execution order nondeterministic
- very small jobs cost more to enqueue than to execute
- priorities help latency but cannot encode correctness
- lock-free deques may reduce contention but increase reclamation/ABA complexity
- blocking I/O needs separate lanes so file/process waits do not occupy frame workers

Learning exercise:

1. Draw a diamond graph A → {B,C} → D.
2. State each prerequisite counter transition.
3. Add cancellation of B and define whether C and D run.
4. Run the thought experiment with zero, one, two, and N workers.
5. Choose a grain threshold for 10, 1,000, and 100,000 transform entries and explain why it must be measured.

### Tutorial 4 — ECS, DOD, and Parallel Systems

Before Prompt 05, the unused `Entity` owned heap-allocated polymorphic components and called virtual update methods. That was an object model, even though the types were named Entity and Component. Prompt 05 deletes that owner and establishes the serial identity/sparse-storage kernel; Prompt 06 adds frozen typed queries and deterministic deferred structure. Prompts 07–11 still must migrate the live scene, move behavior into explicit systems, publish data, and prove parallel ranges.

Sparse-set storage solves two opposing needs:

- dense component arrays support fast iteration
- sparse entity-slot lookup supports flexible per-entity composition

A multi-component query leads with the smallest included pool and tests other sparse mappings. This is simpler than archetype chunks but may gather from multiple arrays. Archetypes improve locality for stable component combinations but require moving entities when composition changes. Sparkle chooses sparse sets first because current scale and migration cost favor simplicity; storage remains private so measured evidence can justify a later change.

DOD asks what the system reads together. `MeshInstance` should hold small instance handles/state, not own a `Mesh`. `WorldTransform` is hot in extraction; editor labels are cold. But splitting every scalar into a different array can increase gathers and complexity. Layout follows access evidence, not ideology.

Prompt 07 makes that reasoning concrete in the live scene. `GameScene` now owns the private ECS world, while the public `Scene*` objects are non-owning read/command adapters and cannot become a second mutable scene. `MeshInstance` contains resource/material/source handles and classification, `LocalTransform` and `Visibility` occupy independently queried columns, and `Name`, `AuthoredIdentity`, and `EditorMetadata` stay cold. A generation-validated mesh-resource handle resolves only inside the world facade; destroying an entity invalidates and releases the resource slot. Skeletal morph weights are authoritative in a generation-validated deformation-state store referenced by `MorphState`; mutable mesh geometry is a derived compatibility cache for today's renderer, not a second authored state. Animation clips remain resource definitions while each clip entity owns its `AnimationState`. Prompts 08-11 still must make derived evaluation explicit, publish the world/render streams, schedule systems, and retire the remaining compatibility adapters.

The camera conversion also establishes a rule for every scene domain: **data view, collection policy, behavior, and render publication are different responsibilities**. `SceneCameraView` is bound to one stable entity and only reads or commits camera values. `SceneCameras` enumerates and selects. `GameCameraController` owns input intent and navigation math. `CameraSnapshot` is immutable publication; Renderer builds its own `RenderCamera`. This follows the role boundaries visible in NVIDIA Donut's separate engine scene camera and application `FirstPersonCamera`, Epic's component/view-target, controller, and player-camera-manager roles, and AMD Cauldron's camera data/component manager/derived view state. Sparkle does not copy the sample frameworks' object ownership or callback-driven update model because that would conflict with its ECS columns and future access-declared systems. The same test applies to meshes and lights: a view may edit one instance, a collection may enumerate/select, a system may transform many instances, and a snapshot may publish derived data; no one facade receives all four jobs.

Do not confuse a migration boundary with a destination abstraction. After Prompt 07, `SceneAnimations` failed the abstraction test—it merely forwarded calls, had no outside consumer, and owned no policy—so it was removed immediately. `SceneCameras`, `SceneLighting`, and `SceneMeshes` still isolate live synchronous consumers from private ECS storage, but their existence is time-bounded. Prompts 08–12 replace them with immutable world reads, compiled game systems, editor semantic commands, transactional construction, and renderer packets. Once those consumers migrate, the forwarding facades and corresponding `GameScene` friend declarations are deleted. Moving the same forwarding methods onto `GameScene`, exposing `SceneWorld`, or publishing the registry would reduce the class count while making ownership worse; the target is fewer *authorities and access paths*, not superficially fewer C++ types.

### What “scene” means in the final engine

Production engines do not eliminate scene/world classes; they give each one an unambiguous ownership domain. Unreal's `UWorld` is the top-level gameplay sandbox and its Mass entity subsystem hosts the entity manager for that world. AMD Cauldron's `Scene` is explicitly the graphics framework's scene representation and includes current-camera/render information. NVIDIA Donut similarly distinguishes engine scene data from application camera behavior. Sparkle currently uses `GameScene`, private `SceneWorld`, numerous `Scene*` containers, `GameSceneSnapshot`, and Renderer `RenderScene*` simultaneously. The problem is not the word; it is that several names appear to own the same pipeline.

The final Sparkle vocabulary is therefore:

- `GameWorld`: the sole authoritative runtime world lifetime, owner-thread commit boundary, system graph host, and publisher. Prompt 08 replaces `GameScene` with this name.
- private `GameWorldState`: ECS registry, component pools, world resources, structural commit, and derived-state storage. It replaces the competing `SceneWorld` name and is never an SDK exposed to Editor or Renderer.
- `WorldReadView` and `WorldChangeJournal`: immutable generation-pinned gameplay/editor publication.
- `GameSystemGraph`: behavior over declared component/resource access. It replaces controller callbacks and domain update facades.
- `EditorSceneModel`: an editor-owned projection; “Scene” is valid here because the type clearly belongs to Editor and is not runtime authority.
- `RenderWorld`, `RenderWorldDelta`, and `RenderFrameDynamicData`: renderer-owned persistent state and two input streams. Renderer `RenderScene*` names are acceptable only for renderer-internal scene representations/passes, never gameplay storage.
- `SceneAsset`, `ImportedScene`, `CookedScene`, and `SceneLoadPackage`: ingestion vocabulary. These values describe a scene asset/package and end at transactional world commit.

Everything else must justify a narrower noun. Camera/light/mesh instance values are ECS components. Sky is a `SkyEnvironment` world resource. Materials, textures, skeletons, meshes, and animation clips are immutable resource stores addressed by generation/version handles. Material variants are an immutable `MaterialVariantSet` plus world selection state. Pose, morph, skinning, visibility, and render arrays are derived outputs. `AnimationSampler`, `AnimationPoseEvaluator`, and `MorphWeightEvaluator` describe operations; they are not scenes. Authored `LightDesc`/camera blueprints are ingestion or editor-model values; they are not runtime owners. Prefixing all of these with `Scene` hides those distinctions and makes accidental duplicate ownership easier.

This yields one recognizable production pipeline:

`Cooked scene assets -> immutable load package -> GameWorldState -> declared GameSystems -> immutable world publication -> editor model and renderer streams -> RenderWorld/GPU scene`

The migration rule is strict: a temporary `Scene*` facade may exist only while it blocks raw ECS exposure for a named live consumer, and the prompt that supplies the replacement must delete it. No forwarding compatibility aliases survive a gate. This is consistent with Epic's separation of world, Mass entity manager, fragments/processors, and representation subsystems; the exact Sparkle class names and two-stream boundary remain Sparkle design decisions rather than claims that NVIDIA or AMD ship this ECS architecture.

Parallel ECS safety comes from a frozen structural epoch:

1. commit creates/destroys entities and adds/removes components;
2. queries are built against stable stores;
3. jobs mutate only declared, exclusive component ranges;
4. jobs record structural commands rather than changing stores;
5. deterministic playback starts the next commit.

Learning exercise:

- hand-simulate sparse/dense arrays through create, add, swap-remove, destroy, and slot reuse
- show why a stored dense index becomes invalid after compaction
- design read/write queries for camera motion, transform evaluation, and render extraction
- identify which pairs may overlap and which require dependencies
- compare sparse-set and archetype costs for frequent visibility-tag changes

### Tutorial 5 — Game/Render Pipelining and Backpressure

A render thread provides throughput only when the game can publish self-contained data and continue. If it immediately waits for rendering, work moved threads but did not overlap. If it publishes raw game pointers, overlap is unsafe. If the queue is unbounded, throughput may look good while input latency and memory grow.

Sparkle therefore uses bounded frame slots and supports:

- serial reference mode for correctness
- threaded zero-ahead mode for ownership with lower pipeline depth
- threaded one-ahead mode for game/render CPU overlap

Backpressure is deliberate. When the renderer falls behind, the producer eventually parks before allocating an unlimited history. This converts overload into bounded latency/memory rather than a growing queue.

Temporal rendering makes IDs and frame tags essential. Motion vectors, jitter, exposure, history resets, frame-generation markers, TLAS/PTLAS instances, and provider resources must refer to the correct produced/consumed frames. “Previous” belongs to the render world when rendering can lag gameplay.

Learning exercise:

- draw main N+1, render N, GPU N-1 on one timeline
- calculate the maximum CPU packet count for zero-ahead and one-ahead modes
- explain what happens when render takes twice as long as game update
- identify which temporal signals must travel in the packet and which history remains renderer-owned

### Tutorial 6 — Persistent GPU State Before Parallel Rebuilds

Parallelizing a full scene rebuild uses more cores on work that should often disappear. A production renderer distinguishes:

- structural changes: create/destroy proxy, asset generation change
- dynamic values: transform, pose, light parameters
- persistent static state: geometry/material/resource bindings

The render world owns stable proxies and the GPU scene owns persistent slots. Deltas update structure; dirty ranges update values. Upload rings handle current dynamic data. Old allocations/resources retire after GPU tokens, not after a C++ object leaves scope.

Benefits include lower CPU preparation, upload bandwidth, allocation churn, and stable temporal/RT identity. Costs include capacity management, fragmentation, dirty tracking, compaction, and recovery when a delta consumer falls behind.

Learning exercise:

- classify every field of one mesh instance as asset-static, structural, dynamic, temporal, or derived
- determine which change requires a new proxy versus a dirty range
- describe how an old BLAS/material/shader generation remains alive while frames reference it
- explain why device idle is correct at final shutdown but harmful for routine reload

### Tutorial 7 — Parallel Command Recording Is Native Ownership Work

Frame-graph dependencies can expose recording groups, but native APIs constrain the implementation:

- a D3D12 command allocator/list cannot be concurrently reset/recorded by multiple tasks
- a Vulkan command pool requires external synchronization; command buffers inherit pool lifetime constraints
- resource-state transitions between independently recorded streams require a compiled entry/exit plan
- upload and transient descriptor allocators cannot expose one contended mutable cursor
- PSOs/layouts/pass runtime state cannot be lazily created inside concurrent recording

The solution is per-worker, per-frame, per-queue recording contexts leased to one task, plus preplanned resource states and worker-local/preassigned transient allocation. Workers close command streams; the render coordinator submits them in compiled order.

More command lists are not automatically faster. Each list has CPU close/submit overhead, driver cost, memory, and possible GPU scheduling consequences. Small passes remain grouped/serial; large draw passes may use intra-pass chunks when pass-level parallelism is insufficient.

Learning exercise:

- draw buffered-frame × worker × queue allocator/pool ownership
- explain why a worker may record but not submit
- identify one existing lazy renderer service that must be materialized before fan-out
- predict when splitting a pass into eight lists will regress performance

### Tutorial 8 — Cancellation, Editor Affinity, and Transactional Work

Background loading/cooking is not “launch work and call a callback.” The operation belongs to a scope, publishes a generation, can be superseded, and must leave the previous product state usable on failure.

The scene-load example teaches the full lifecycle:

1. capture an immutable request on the owner thread;
2. perform blocking reads on the I/O lane;
3. decode/validate task-local data in background work;
4. merge deterministically into an immutable package;
5. reject stale request/document/world generations;
6. commit on the world owner;
7. let renderer resources upload/retire through their own lifetime system.

ImGui, editor transactions, selection, window messages, and scene structural commit remain owner-thread operations. Workers return structured results; they do not invoke UI or arbitrary events. Cancellation is cooperative and does not mean already published/GPU-used data may be freed immediately.

Learning exercise:

- enumerate cancellation points before read, during decode, after package publication, and after GPU upload begins
- state what remains active after each cancellation
- design a stale-result test for closing an editor document during load
- explain why progress updates must be bounded/coalesced and marshalled to the owner

### Tutorial 9 — Determinism, Profiling, and Honest Speedup

Task execution order is nondeterministic; engine results should be deterministic where product behavior requires it. Stable entity/asset/system/partition keys make merges reproducible. A serial executor provides reference semantics and dramatically reduces debugging search space.

Measure the critical path, not total work or worker count. If B and C run in parallel after A but B is ten times longer, optimizing C does not materially shorten the frame. Amdahl's law limits gains from remaining serial commit, graph setup, submission, and presentation.

Always separate:

- CPU task parallelism
- game/render frame pipelining
- GPU queue concurrency
- frames in flight
- shader/kernel performance

A higher FPS can hide increased input latency, GPU time, command-list overhead, memory, or descriptor pressure. Report p50/p95/p99 CPU and GPU times, pipeline mode, core count, backend, validation state, and workload. A negative result with a correct causal explanation is stronger engineering evidence than an unexplained favorable number.

Final teach-back challenge:

> Starting from input for frame N+1, explain every owner, task dependency, publication edge, frame/generation tag, native recording context, submission authority, and retirement event until the GPU finishes frame N. Then identify three places where extra parallelism would be unsafe or counterproductive.

### Tutorial 10 — Atomic Protocols, Litmus Tests, and Lifetime

Treat an atomic protocol as a tiny distributed state machine. Start with Sparkle's bounded `RenderFrameQueue` because its payload, ownership, and capacity are concrete. The producer owns `Writing`; release-publishes `Ready`; the renderer acquire-claims `Rendering`; CPU acknowledgement permits packet-arena reuse. GPU completion is deliberately absent from this CPU queue protocol and belongs to render-owned retirement.

Build three test versions:

1. mutex/condition-variable reference;
2. sequentially consistent atomic state machine;
3. acquire/release version with identical externally observed behavior.

Inject yields between every meaningful operation, random producer/consumer delays, wrap tiny generations rapidly, stop while full/empty, and verify every sequence number is consumed once. A test that happens to run clean is not a proof, but the exercise forces the invariant and suspicious interleavings into reviewable code.

Interview drills:

- Explain why release-publishing a pointer does not keep the pointee alive.
- Explain when relaxed ordering is sufficient for a completion statistic but not a ready flag.
- Write a correct compare-exchange retry loop and explain why `expected` changes after failure.
- Give an ABA example using a recycled task slot and repair it with bounded generations/lifetime.
- Distinguish atomicity, ordering, visibility, ownership, and reclamation.

Do not add a generic lock-free queue in this tutorial. The artifact is a verified protocol test for the production `RenderFrameQueue`/task slot that Sparkle actually needs.

### Tutorial 11 — Work Stealing Meets Real CPU Topology

A textbook scheduler assumes identical cores and uniform memory. Desktop machines expose physical cores, SMT siblings, chiplets/cache domains, heterogeneous performance/efficiency cores, driver threads, and power-management behavior. NVIDIA's CPU guidance specifically warns that increasing worker count can reduce game performance through cache pressure, atomics, SMT sharing, migration, context switches, and power effects. The correct response is measurement and policy, not permanent hard affinity.

Use the completed SparkleTasks runtime to run four workloads:

- compute-heavy independent transforms;
- cache-heavy sparse gathers;
- mixed short/long tasks with a critical chain;
- frame work alongside background shader/texture compilation.

Sweep serial, 1, 2, physical-core-like, logical-core-like, and capped N worker counts. Capture work/steal ratios, ready time, running time, context switches, migrations, cache/branch counters where available, p95/p99 frame time, and background completion. Repeat with SMT/affinity experiments only as diagnostics.

Failure exercises:

- hold a lock in a background task and make a high-priority coordinator need it;
- enqueue thousands of high-priority tiny tasks ahead of a critical continuation;
- wake every worker for one task;
- block workers on nested children;
- let a third-party compiler create its own internal workers.

Explain priority inversion, convoying, starvation, oversubscription, and thundering herd from the resulting trace. The production correction should normally be ownership, dependency, batching, lane budgets, or fewer workers—not arbitrary priority escalation.

### Tutorial 12 — Reduction, Scan, Compaction, and Deterministic Merge

Real interview questions often move beyond “split this loop.” Sparkle has useful examples:

- reduce transformed bounds for a scene or mesh batch;
- compact dirty GPU-scene ranges;
- build offsets for variable-size cooker records;
- bucket visible draws by pass/material key;
- merge per-task entity commands deterministically.

Implement the serial oracle first. For reduction, give each task a private partial and merge in a defined order. For compaction, compare task-local vectors plus ordered merge with count/scan/scatter. For floating point, decide whether bitwise stability is required; associativity is not guaranteed, so a different reduction tree may change results. For buckets, include stable tie-breakers so stealing order cannot change output packages or captures.

Adversarial sizes include 0, 1, just below/above grain, highly skewed buckets, all/none dirty, duplicate keys, maximum capacity, and cancellation between phases. Measure total work, critical path, allocations, bandwidth, and scaling. Retain the simplest algorithm that meets the actual workload.

Teach-back:

- Why is one atomic append index sometimes correct but still slow?
- Why does scan have multiple synchronization phases?
- When is task-local buffering preferable to a concurrent container?
- How do determinism requirements alter a reduction or sort?

### Tutorial 13 — Asynchronous I/O, Decode, Resource Creation, and PSO Hitches

File I/O is not CPU parallel work. A worker blocked in `ReadFile` does not execute another job. A production streaming pipeline separates request, I/O completion, CPU decompression/decode, validation, owner commit, GPU upload, and residency publication. Each stage has a capacity, cancellation rule, generation, and error product.

Sparkle's first implementation can use a bounded blocking-I/O lane because it is portable and fits the current engine. The design must leave an internal completion boundary so Windows overlapped I/O or DirectStorage can replace the read mechanism without changing world/render ownership. DirectStorage is a future measured backend, not a prerequisite or a second asset system.

PSO and native resource creation are related hitch sources. Some device/driver creation calls may be expensive or internally synchronized. Sparkle should:

1. derive required shader/PSO keys from owned scene/pass data;
2. deduplicate through one render-owned generation cache;
3. compile/create asynchronously only where the backend contract permits;
4. budget background concurrency and peak memory;
5. publish immutable ready generations at a render boundary;
6. select an explicit missing policy: delay proxy/draw, bounded fallback, or known loading barrier;
7. record cold-cache hits, misses, late requests, compile time, and memory;
8. never lazily create PSOs inside parallel recording tasks.

Test cold and warm caches separately. Force cancellation, compile failure, stale shader generation, device loss/failure where supported, and “needed before ready.” A warm developer cache must not hide first-run hitches.

### Tutorial 14 — CPU/GPU Overlap, Frame Pacing, and Latency

Throughput and responsiveness are separate products. A one-frame-ahead render pipeline may improve CPU utilization and FPS while increasing the age of input shown on screen. Unbounded CPU lead can stack frames in the driver/GPU queue. Frame generation adds presentation and queue ownership constraints that cannot be inferred from FPS.

Build one correlated frame identity through:

~~~text
input sample → simulation start/end → packet publish/consume
→ record start/end → queue submit → GPU start/end → present start/end
~~~

Then compare:

- serial, threaded zero-ahead, and threaded one-ahead;
- GPU-bound and CPU-bound scenes;
- VSync/VRR/present modes supported by the product;
- serial versus parallel command recording;
- graphics-only versus measured async-compute/copy candidates;
- frame-generation/provider disabled and enabled paths.

Use queue timelines to prove overlap. Extra async compute is rejected when fences, bandwidth contention, or lost occupancy make the frame worse. Extra CPU lead is rejected when latency rises without a required throughput benefit. Vendor latency integrations may be added only through the existing provider-neutral boundary; the engine-owned frame markers and bounded `RenderFrameQueue` remain authoritative.

Interview drills:

- Distinguish CPU frame time, GPU frame time, present interval, and input-to-present latency.
- Explain why two queues do not guarantee GPU overlap.
- Explain why batching command-list submissions may help CPU cost but delay GPU availability.
- Explain a deadlock caused by sharing a present/async queue with a provider that requires exclusive use.
- Decide where backpressure should occur in a GPU-bound frame.

### Tutorial 15 — Production Concurrency Forensics

An expert is expected to diagnose evidence, not only design greenfield code. Use a narrowing workflow:

1. reproduce with exact build, backend, worker count, scene, cache state, and validation state;
2. classify crash, corruption, deadlock, livelock, starvation, hitch, low scaling, or latency regression;
3. compare serial and parallel modes;
4. capture a system timeline before changing code;
5. find the blocked/ready/running critical path and its waking thread;
6. inspect lock ownership, task dependencies, queue fences, object generations, and GPU last use;
7. form one falsifiable hypothesis and inject delay/failure near the suspected edge;
8. fix the ownership/protocol, add the smallest regression test, then remeasure.

Tool intent matters:

| Question | Suitable evidence |
|---|---|
| Which CPU thread/task delayed the frame? | existing profiler plus WPA/ETW, Nsight Systems, or equivalent timeline |
| Is a thread ready but preempted or affinity-restricted? | WPA CPU Usage (Precise), context switches, ready-thread stacks |
| Are CPU and GPU timelines overlapping? | Nsight Systems/PIX/GPUView plus engine markers |
| What do AMD GPU queues/barriers/waves do? | Radeon GPU Profiler |
| Is Vulkan/D3D12 ownership/state invalid? | native validation/debug layers and GPU-assisted validation where practical |
| Is this a C++ race or memory lifetime error? | sanitizer-supported configuration, debugger, crash dump, deterministic stress |
| Did instrumentation distort the result? | release/profile comparison with markers/counters selectively disabled |

Prepare three incident narratives: a race/lifetime defect, a deadlock/stall, and a scaling regression. Each narrative needs evidence, root cause, rejected hypotheses, fix, regression test, and post-fix measurement.

### Tutorial 16 — Draw Preparation, Recording, Translation, Aggregation, and Submission

These terms are often mixed together in renderer discussions. They describe different representations, owners, and opportunities for parallelism. A strong interview answer follows one draw all the way to the GPU and says exactly where its data changes form.

#### The Five Layers

~~~text
render-owned scene/proxies
  -> visible MeshDraw / MeshInstanceBatch data
  -> compiled FrameGraph pass + RecordingPlan
  -> Sparkle RenderCommandList backed by one native D3D12 list or Vulkan buffer
  -> ordered submission batch
  -> native graphics / compute / copy queue
~~~

1. **Preparation** decides *what* should be drawn or dispatched. It performs visibility, LOD, light/shadow classification, pass eligibility, draw sorting, instancing, RT build-input selection, and immutable parameter preparation. It should not emit native commands while global caches are still changing.
2. **Recording** calls RHI/native methods to encode commands into an exclusively owned command list or buffer. Recording can happen concurrently because each task owns a different native object and transient allocation range.
3. **Translation** means replaying one command representation into another. Unreal's render front end records platform-neutral `FRHICommand` objects; an RHI thread or translate task executes them through an `IRHICommandContext`. Sparkle's `RenderCommandList` is currently a virtual interface directly implemented by D3D12/Vulkan command objects, so Sparkle has no equivalent software replay layer.
4. **Aggregation** orders already-recorded lists/buffers into the sequence required by the compiled graph. It is a stable fan-in operation, not bytewise concatenation and not completion-order submission.
5. **Submission batching** chooses how many ordered closed lists/buffers are passed to a native queue call and when. Fewer calls reduce CPU fixed cost; delaying a batch too long can leave the GPU idle or increase latency.

The essential ownership timeline is:

~~~text
coordinator: compile plan -> reserve/prewarm -> launch recording tasks -----------> ordered submit -> present
worker A:                                      lease -> begin -> record -> close --/
worker B:                                      lease -> begin -> record -> close -/
GPU:                                                                       execute in compiled order
~~~

Worker completion order is deliberately absent from GPU semantics.

#### Unreal's Two Command-List Layers

Epic's public model is valuable because it makes translation explicit:

~~~text
RenderThread / frontend tasks
  encode FRHICommand records into FRHICommandList
RHIThread / translate tasks
  execute records through IRHICommandContext
platform RHI
  records/submits native command lists or buffers
~~~

That layer can decouple a high-level render producer from platform translation, preserve a serial/bypass mode, and allow frontend and backend parallelism. It also costs command memory, captured-argument copies, replay dispatch, a new lifetime boundary, more frame lead, and another debugging representation. Operations that cannot be deferred safely may force a flush or require data copying.

Sparkle should learn this architecture without pretending it already exists. The current direct path is simpler:

~~~text
frame-graph execute task
  -> RenderCommandList virtual call
  -> D3D12RenderCommandList / VulkanRenderCommandList
  -> native command object
~~~

Adding `RhiThread` now would be naming without mechanism. Adding the mechanism would mean defining a complete immutable software command ABI for every command, copying pointer-referenced data, versioning/debugging the stream, replaying it, and proving lower critical-path time than direct native recording. The production ADR gate is therefore:

- a trace shows direct RHI/backend translation or submit preparation is a persistent render-coordinator critical path after parallel native recording;
- a software command stream supplies a durable platform, capture/replay, validation, or scheduling benefit beyond “another thread”;
- memory, latency, lifetime and debugging costs are measured;
- serial direct, software-stream direct-translate, and threaded translate modes execute the same semantic plan;
- the accepted design replaces a path rather than coexisting indefinitely as an unaudited second RHI.

Until all five conditions hold, Sparkle implements direct parallel native recording and studies Unreal translation as a know-and-defer alternative.

#### What “Merge” Can Mean

Use the qualified term; never say only “merge command lists.”

| Qualified operation | Meaning | Sparkle owner |
|---|---|---|
| **draw merge / dynamic instancing** | Replace compatible draws with an instanced/batched draw after state/binding comparison | renderer preparation (`MeshInstanceBatchBuilder` or its coherent successor) |
| **task-result merge** | Stable concatenate, reduce, scan/scatter, or sort task-private results | SparkleTasks continuation / renderer preparation join |
| **recording-group aggregation** | Place closed command lists/buffers in compiled submission order | frame-graph executor |
| **render-pass merge** | Combine compatible graph passes/rendering scopes while preserving resource semantics | frame-graph compiler, only after measurement |
| **native submit batching** | Pass several ordered lists/buffers in one queue submission | render coordinator / RHI submission service |
| **driver optimization** | Native driver may optimize across work supplied together | driver; never an engine correctness assumption |

Native D3D12 command lists and Vulkan command buffers are opaque. Sparkle does not append their bytes, reopen them from another worker, or let a worker “merge” by submitting early.

#### Building Recording Groups and Chunks

Pass-level recording exploits independent graph work. Intra-pass recording divides one expensive pass. The algorithm is the same shape:

1. estimate CPU recording cost from prior scopes and current draw/dispatch/build-input counts;
2. retain serial execution below a measured crossover;
3. partition stable ordered ranges by estimated cost, not merely equal object count;
4. prepare shared immutable pass state once;
5. assign every range a private command object, upload slice, descriptor slice and result slot;
6. place barriers, render-target state, common viewport/scissor and render-pass preamble in compiler/coordinator work unless the backend contract explicitly assigns them to each list;
7. record only the range body on workers;
8. join all range tasks without making workers wait;
9. aggregate result slots by `SubmissionOrderKey` and range index;
10. submit at a point that balances CPU overhead, GPU starvation and latency.

Cost partitioning matters. Ten thousand cheap state-identical draws and one hundred descriptor-heavy draws are not necessarily equal recording work. Initial heuristics can use draw/dispatch counts; retained policy needs measured record time per group/pass and must avoid self-tuning instability inside one frame.

#### D3D12 Shape

- One command list and allocator are recorded by only one task at a time.
- Allocator reuse is gated by completion of all GPU work recorded with it.
- The practical ownership dimension is buffered frame × queue × recording context, bounded for memory.
- Independent direct command lists are the baseline. Bundles are an optional measured reuse mechanism, not the synonym for parallel recording.
- An ordered array of closed command lists can be supplied in one `ExecuteCommandLists` call. This reduces fixed submit cost and gives the runtime visibility of the batch, but it does not relax the frame graph's order or barriers.
- Splitting every pass into a separate `ExecuteCommandLists` call can add CPU overhead and unintended ordering boundaries; accumulating the whole frame can increase GPU starvation/latency. Prompt 28 measures the middle ground.

#### Vulkan Shape

- Command pools are externally synchronized, so a worker uses only its leased pool/context.
- Independent passes may use separate primary command buffers.
- Draw-heavy work inside one rendering scope may use worker-recorded secondary command buffers executed by an ordered primary through `vkCmdExecuteCommands`.
- Secondary buffers require correct inheritance/dynamic-rendering information and can cost more than they save; they are a policy option, not the common RHI abstraction.
- AMD's Detroit example keeps pipeline barriers, render targets, and viewport setup out of worker secondary buffers, records descriptor/buffer/draw bodies in parallel, then restores serial render-list order. Sparkle adopts this as an instructive starting pattern, not a universal restriction for every Vulkan path.
- Command-buffer boundaries do not create all required memory dependencies. The frame graph remains responsible for barriers, queue-family ownership and cross-queue synchronization.

#### Light, Shadow, and Mesh Examples

“Parallel lighting” can mean very different work:

- CPU classification of visible lights and construction of compact light records;
- parallel light-grid/cluster input generation;
- per-light shadow-view and frustum construction;
- parallel intersection of scene bounds with shadow frusta and deterministic caster-list merge;
- shadow draw preparation and command recording by light/view/chunk;
- GPU tiled/clustered lighting or asynchronous compute, which is GPU concurrency rather than CPU tasks;
- offline static-light baking, potentially distributed, which is a tools/content product.

Sparkle's useful near-term examples are the first five because they feed existing lighting, shadow, mesh, and frame-graph paths. It must preserve stable light identities required by reservoirs and transparent/caster ordering. It should not claim a distributed light-bake implementation merely because Unreal Swarm exists.

Epic's mesh-draw design contributes three enduring lessons. First, cache only view-independent draw state and retain a simpler dynamic path for small/editor work. Second, cached draw references require explicit invalidation and resource lifetime. Third, parallel pass setup and dispatch should scale by pass and draw count while stable ordering survives task completion. Sparkle applies those lessons to current `MeshDraw`, `MeshRenderItem`, `MeshInstanceBatch`, and GPU-scene structures; it does not create an `FMeshDrawCommand` clone or copy Unreal prefixes.

#### Interview Teach-Back

Be able to answer these without notes while drawing the ownership timeline:

- Why can two D3D12 command lists record concurrently while one list cannot?
- Why does a per-thread Vulkan command pool solve correctness and reduce contention, and when does it waste memory?
- What does Unreal translate, and why is Sparkle's current RHI not doing that?
- What are five meanings of “merge” and which owner performs each?
- Why can fewer queue submissions improve CPU time yet worsen latency?
- Where do barriers live when workers record only pass bodies?
- How do transparent and shadow-caster order remain deterministic?
- When should a small pass stay serial?
- How do shader compilation, PSO creation, resource upload, and command recording differ in ownership and readiness?
- Why is async compute not evidence of CPU multithreading?

## AMD/NVIDIA Expert Interview Readiness Audit

### Evidence Boundary

The job-posting observations below are snapshots accessed on 2026-07-18, not promises about every team or future opening. They identify recurring competencies in current official AMD and NVIDIA postings and are combined with the public implementation sources in this document. Sparkle is evidence of these competencies only after the corresponding K prompt passes; architecture text alone is not experience.

### Current Role Signal Matrix

| Official role signal | Recurring expectation | Sparkle evidence target |
|---|---|---|
| [AMD Senior Graphics Software Engineer — Rendering](https://careers.amd.com/careers-home/jobs/80570?lang=en-us) | modern C++, real-time rendering, GPU/APU architecture, productizing research, partner integration | preserve advanced raster/RT/temporal paths through a production ownership redesign; document integration contracts |
| [AMD Senior GPU SDK Engineer — Frame Timing](https://careers.amd.com/careers-home/jobs/86334?lang=en-us) | D3D12/DXGI, frame timing/pacing, latency, telemetry, parallel algorithms, engine integration, validation | correlated frame identity, bounded pipeline, backend queue captures, p50/p95/p99 latency and pacing evidence |
| [AMD Senior C++ Software Development Engineer](https://careers.amd.com/careers-home/jobs/87049?lang=en-us) | concurrent APIs, architecture, algorithms/data models, profiling/debugging, testing, documentation, end-to-end delivery | SparkleTasks/ECS design, stress suite, deterministic tools, measured product integration and deletion closure |
| [NVIDIA Graphics Tools Software Engineer](https://nvidia.wd5.myworkdayjobs.com/en-US/NVIDIAExternalCareerSite/job/Graphics-Tools-Software-Engineer_JR2017973) | C++, D3D/Vulkan, OS threads/memory/IPC, crash dumps, concurrency/performance diagnosis, GPU architecture | dual-backend recording ownership, diagnostic labs, WPA/Nsight/native-validation incident reports |
| [NVIDIA CUDA C++ Core Libraries Engineer](https://nvidia.wd5.myworkdayjobs.com/en-US/NVIDIAExternalCareerSite/job/US-CA-Santa-Clara/Senior-Software-Engineer--CUDA-C---Core-Libraries_JR2021114) | performance/concurrency/compatibility, API/ABI design, testing, profiling, benchmarking, lifecycle ownership | bounded private APIs, serial reference, stable handles, reproducible benchmark matrix and long-term maintainability |
| [NVIDIA Rendering Research Scientist](https://nvidia.wd5.myworkdayjobs.com/en-US/NVIDIAExternalCareerSite/job/US-WA-Redmond/Senior-Research-Scientist--Rendering_JR2019582) | rendering theory, C/C++, GPU and parallel programming, real-time performance, productized research | advanced-feature preservation, CPU/GPU causality, ReSTIR/path-tracing integration evidence, honest limits |
| [NVIDIA Graphics/Parallel Programming Architect — Memory Models](https://nvidia.wd5.myworkdayjobs.com/en-US/NVIDIAExternalCareerSite/job/Senior-Graphics-and-Parallel-Programming-Architect--Memory-Models_JR2016112) | memory models, parallel algorithms/architectures, simulation, test plans and validation | atomic litmus/state-machine labs, cache/topology reasoning, adversarial validation—not a claim of hardware-architect parity |

The common bar is not memorizing APIs. It is the ability to move from first principles to maintainable implementation, diagnose across software/hardware boundaries, quantify performance, preserve compatibility, and communicate decisions to specialists.

### Adversarial Coverage Matrix

| Competency | Earlier J coverage | Required strengthening | Evidence class |
|---|---|---|---|
| data races, happens-before, atomics | strong publication foundation | legal-execution reasoning, CAS, relaxed/SC tradeoff, ABA/reclamation | production protocol + focused lab |
| mutexes, sleeping, wakeup | scheduler contract present | spurious/lost wake, convoy, herd, priority inversion, spin/park reasoning | scheduler implementation + fault injection |
| work stealing and task graphs | strong | topology/SMT/cache-domain effects, fairness and critical-path ready time | production scheduler measurements |
| structured concurrency | strong | compare with sender/receiver/coroutines and defend bounded custom scope | design defense; no second runtime |
| parallel algorithms | mostly `parallel_for` and DAG | reduction, scan, compaction, partition, deterministic merge | real ECS/cooker/GPU-scene workloads |
| data layout and memory hierarchy | ECS/DOD strong | coherence traffic, bandwidth scaling, SIMD interaction, NUMA/chiplet awareness | layouts + counters/scaling lab |
| async loading | transactional scene loading strong | distinguish I/O completion from CPU jobs; staged capacity and future DirectStorage seam | production streaming pipeline |
| shader/PSO/resource hitches | prewarm/generation present | cold-cache dedupe, memory budget, late/miss/fallback policy | production render residency work |
| D3D12/Vulkan recording | strong | explicit API validation under migration/delay and batch/submission cost | production dual-backend work |
| GPU queue concurrency | partial async compute/copy policy | measured queue overlap, ownership transfer, fence/bandwidth cost | frame-graph/RHI integration |
| frame pacing and latency | bounded `RenderFrameQueue` and input-to-present metric | correlated stage markers, CPU lead policy, provider queue constraints | production telemetry through existing hooks |
| production debugging | stress/validation strong | OS ready-time/context-switch analysis, crash dumps, incident narratives | focused diagnostic lab + regressions |
| portability | D3D12/Vulkan strong | weakly ordered CPU reasoning and topology-neutral defaults | tests and documented policy |
| communication/leadership | portfolio narrative present | concise incident/design review and adversarial interview defense | teach-back and reviewed artifacts |

### Implement, Lab, or Know-and-Reject

To keep Sparkle coherent:

**Implement in the product because current problems justify it:** the shared task runtime, ECS/system graph, immutable world/render contract, bounded frame pipeline, persistent GPU scene, staged async loading/cooking, PSO/residency generations, parallel command recording, queue/latency markers, and existing-tool instrumentation.

**Implement as focused tests or benchmark labs against real internals:** memory-order litmus tests, ABA/generation wrap, priority inversion, worker topology/count sweeps, reduction/scan variants, contention/false-sharing microbenchmarks, delay injection, and forensic trace exercises. These belong in existing test/benchmark surfaces and must not create shipping subsystems.

**Understand and be able to reject until evidence changes:** general hazard-pointer/RCU framework, custom fibers/coroutine runtime, permanent affinity map, one thread per subsystem, universal lock-free containers, multi-GPU rendering, distributed simulation, physics/audio/networking subsystems, and GPU compute ports added only for résumé keywords.

### Expert Question Bank

Answer each with a Sparkle example, invariant, tradeoff, failure test, and measurement—not a definition alone.

1. What is a data race in the C++ abstract machine, and why can apparently correct x64 code still be invalid?
2. Show the synchronizes-with edge for frame packet publication and the separate edge for reuse.
3. When is `memory_order_relaxed` correct? Give one correct and one incorrect Sparkle use.
4. Explain ABA and reclamation in a recycled task/packet slot.
5. Compare mutex, semaphore, condition variable, atomic wait, spin lock, and task continuation.
6. Define lock-free, wait-free, obstruction-free, and fairness; which does SparkleTasks promise?
7. How can a lost wake happen, and how does the predicate/state protocol prevent it?
8. Explain false sharing and design a test that separates it from true lock contention.
9. Why can fewer workers improve performance on a high-core-count CPU?
10. Diagnose priority inversion between background compilation and the render coordinator.
11. Describe work stealing, local versus injected work, steal cost, and critical-path behavior.
12. Why are worker waits dangerous? Compare continuation, help-while-waiting, and controlled oversubscription.
13. How do cancellation and owner destruction settle a tree of nested tasks?
14. Design deterministic parallel reduction and compaction for dirty GPU-scene records.
15. When does parallel sorting/bucketing lose to serial work?
16. How do ECS structural epochs convert unsafe mutation into parallel ranges?
17. Why does DOD sometimes improve multithreading and sometimes expose memory-bandwidth limits?
18. Separate asynchronous I/O, CPU decompression, GPU upload, and residency publication.
19. How do you prevent stale async load/shader results from replacing newer state?
20. Why can parallel PSO compilation create memory or frame-time regressions?
21. State D3D12 allocator/list and Vulkan pool/buffer ownership constraints.
22. Why cannot independently recorded command lists infer cross-list resource state safely?
23. Why does the render coordinator submit even when workers record?
24. Distinguish CPU task parallelism, parallel recording, GPU async compute, and frames in flight.
25. When does an async compute/copy queue make performance worse?
26. How can one-frame-ahead rendering improve throughput but hurt input latency?
27. Design backpressure for a renderer that is consistently slower than simulation.
28. Given a p99 hitch, choose between engine profiler, WPA, Nsight Systems, PIX/GPUView, RGP, native validation, and sanitizer.
29. How do you prove speedup causality rather than report a higher FPS?
30. Which attractive concurrency feature did Sparkle reject, and what evidence would reopen the decision?

### Whiteboard and Coding Drills

- Draw and implement a bounded SPSC generation queue with close/cancel semantics; test wrap and shutdown.
- Given a task DAG with mixed costs, identify the critical path, maximum theoretical speedup, and a useful scheduling order.
- Repair a deadlock involving world commit, renderer cache, and a completion callback without adding a global recursive mutex.
- Convert an atomic append compaction into task-local count/scan/scatter and state determinism policy.
- Review a D3D12/Vulkan recording-context design and find every illegal concurrent reset, descriptor allocation, and lifetime reuse.
- Analyze a synthetic trace with worker starvation, GPU bubbles, and queued frames; propose one falsifiable experiment per suspected cause.
- Defend why Sparkle uses sparse sets now, why task access declarations are separate, and what measurements would justify archetype chunks.

Readiness requires live performance: explain while drawing ownership and timelines, write a small correct protocol, critique a flawed design, and interpret a trace. A memorized answer without executable Sparkle evidence does not close the competency.

## External Repository Study

GitHub implementation observations below are tied to a listed commit wherever the architectural conclusion depends on source details. Fast-moving standards/reference projects and current product guidance are identified by access date and must be rechecked before implementation. The goal is to learn mechanisms and constraints, not clone any one engine’s architecture.

### Source Matrix

| Source | Inspected revision | Relevant mechanisms | Sparkle lesson |
|---|---|---|---|
| NVIDIA Donut | bc1ea24b0486f1c00d89327fe16c0b4dd11c5937 | Small FIFO thread pool used for asynchronous scene/texture loading | Useful background-content pattern; insufficient for a renderer dependency graph |
| NVIDIA nvpro_core | bc19d6ac3ef62938d0ea0e099735878457ce1b6e | `parallel_batches`/`parallel_ranges`, command pools, ring fences, batch submission | Range/batch and command terminology is precise; Sparkle keeps CPU task queues distinct from GPU command queues |
| NVIDIA NVRHI | 8e8c36e37558acec333204619b95d9d2fcdc4a79 | Concurrent command-list recording and explicit state assumptions across lists | Parallel recording requires explicit entry/exit or permanent resource states |
| AMD FidelityFX SDK Cauldron2 | 60f4ea81909200d8542eca14dccb2628b763a9a3 | Task manager used mainly for content loading; fan-in completion callback | Continuations are useful; a loading pool is not a general frame scheduler |
| AMD Render Pipeline Shaders | f3330f5306d15af8529a310f6255225c864b0961 | render-graph builder/node IDs, command batches, record info/context, acquired command buffers; sample `Job`/`WaitHandle` pool | Borrow render-command specificity and parallel range ownership; do not import sample-only `Job` vocabulary into SparkleTasks |
| AMD Cauldron (legacy) | b92d559bd083f44df9f8f42a6ad149c1584ae94c | Simple global FIFO worker pool | Good historical sample, not the target scheduler architecture |
| Epic Unreal Engine documentation | UE 5.8 documentation, accessed 2026-07-17 | Tasks DAG, game/render/RHI separation, render proxies, RDG parallel execute, asynchronous level-loading constraints | Strong ownership and dependency model; defer global/UI/transaction interaction during worker loading; do not add a separate RHI thread without need |
| Epic mesh drawing / RHI translation documentation | UE 5.7/5.8 documentation, accessed 2026-07-18 | retained/dynamic mesh draw preparation, parallel pass setup/dispatch, software RHI command translation, ordered async list submission | Separate preparation, translation, aggregation, and native submission; reuse Sparkle draw data rather than cloning Unreal types |
| Epic MassEntity documentation | UE 5.8 documentation, accessed 2026-07-17 | Data-only entities/fragments, archetypes, queries, processors, deferred command buffer | Strong ECS/DOD model; Sparkle should borrow separation and deferred structure without copying Mass scale |
| O3DE AzCore + Atom | 68683f23fb747380d3efa2424bd5f30242e9c5a2 | Retained task graph, task executor, jobified frame scheduler, thread-local command pools | Closest open-source structural reference for scheduler-to-renderer integration |
| Unity EntityComponentSystemSamples | 6786a741ee1f118ed14cecfa02beae8e926937b0 | Entities/Jobs examples, component queries, command-buffer playback, deterministic parallel sort keys | Direct public proof of ECS-to-job integration and deferred structural mutation |
| EnTT | 1333fa53129e7cfded5a9640c4336a254049917b | Versioned entity identifiers, sparse-set component pools, const/writable views and smallest-pool multi-component iteration | Appropriate bounded C++ storage reference when archetype chunks are premature |
| Google Filament | 398d13c4abd148ea6e139b05e5418efa37d284d3 | Work-stealing jobs, parent completion counts, parallel_for, cache-line-sized job records | Strong scheduler mechanics; Sparkle should use dependencies instead of normal worker waits |
| Microsoft DirectX Graphics Samples | 357ade6ec6ff0d9dcadc48f35c7a28e37c0cdf7a | Per-frame/per-worker allocators and command lists, serial ordered submission | Clear D3D12 proof of parallel recording constraints |
| NVIDIA CPU/API guidance | articles accessed 2026-07-18 | worker-count/topology scaling, balanced command recording, separate submission, resource-creation and queue costs | Measure physical/logical-core policies and native work distribution; do not maximize threads blindly |
| NVIDIA Vulkan/PSO/RT pipeline guidance | articles accessed 2026-07-18 | parallel command recording, image/buffer creation, descriptor updates, allocation/binding, asynchronous PSOs and RT pipeline collections | Treat native creation as bounded keyed work with exclusive results, cold-cache/memory evidence, and backend-specific policy |
| NVIDIA stdexec | 711da5971a8e8e940763c11bf6bbeb1c1bb22c3a | composable senders, schedulers, structured scopes, I/O and GPU execution contexts | Study future standard direction; keep one bounded SparkleTasks runtime rather than importing a second model |
| AMD Ryzen/Radeon guidance | articles/tools accessed 2026-07-18 | CPU-bound diagnosis, parallel PSO/list verification, queue/barrier/wave timelines | Require cold-cache CPU captures and AMD GPU queue evidence, not vendor-neutral assumptions alone |
| AMD RPS and Detroit rendering studies | articles/source accessed 2026-07-18 | whole-graph ranges, within-pass secondary command buffers, render-list partitioning by draw count, stable ordered fan-in | Compile meaningful ranges; keep barriers/preamble authoritative; preserve transparent and pass order independent of completion |
| AMD FidelityFX frame interpolation | SDK guide accessed 2026-07-18 | explicit game/compute/present/acquire queue ownership and frame pacing | Provider integrations must declare queue exclusivity, synchronization and pacing through the existing boundary |
| Microsoft DirectStorage samples | main reviewed 2026-07-18 | queued small reads, CPU/GPU decompression, low-CPU streaming | Preserve a staged I/O completion seam; adopt only after current blocking-I/O pipeline is measured |

### NVIDIA Donut

Donut’s ThreadPool is intentionally small: fixed std::threads, a global FIFO queue guarded by a mutex and condition variable, and a pending-task count. Scene and TextureCache use it to move file reads and CPU decoding away from the foreground, then perform later GPU-facing finalization.

Borrow:

- asynchronous content work that produces a later publication/finalization step
- fixed worker ownership and clean shutdown
- keeping loading separate from GPU submission

Do not copy as the engine scheduler:

- one contended global FIFO
- no prerequisite DAG
- busy-yield completion wait
- swallowed task exceptions
- no distinction between latency-sensitive work and blocking background work

### NVIDIA NVRHI

NVRHI explicitly supports concurrently recording multiple command lists. Its documentation also explains the critical resource-state limitation: automatic tracking inside each command list cannot infer arbitrary state transitions between independently recorded lists. Entry states, exit states, permanent states, and submission order must be known.

Sparkle already has stronger frame-graph knowledge than an immediate renderer. The frame-graph compiler should convert that knowledge into a recording plan:

- each recording group receives declared initial resource states
- each group leaves declared final states
- inter-group barriers are emitted in ordered primary/coordinator work
- submission order remains identical to the compiled serial order

### AMD FidelityFX SDK / Cauldron2

Cauldron2’s task manager is explicitly described as content-loading support while the main loop remains single-threaded. It uses a FIFO worker queue and a completion object that can invoke a callback after a group finishes.

Borrow:

- nonblocking fan-in continuation
- keeping content-loading work off the main loop
- a simple first implementation before exotic scheduler mechanisms

Do not present it as evidence that a renderer is multithreaded. Its own scope statement is the opposite.

### Epic Unreal Engine

Epic’s public documentation supplies three important design constraints:

1. The game thread owns gameplay objects, while the render thread consumes distinct render-owned representations. Render code must not dereference mutable game objects.
2. The game thread may produce a frame ahead of rendering, with explicit fences/deferred cleanup for lifetime.
3. Render Dependency Graph setup declares resources and dependencies; eligible execute work can be recorded in parallel, while side effects and direct immediate-command-list use constrain parallel execution.

Epic’s current Tasks System documentation emphasizes prerequisite graphs, nested tasks, pipes, and task events. It also warns against busy-waiting. Sparkle should copy the conceptual rule—dependencies before waits—without trying to reproduce Unreal’s full generality.

Epic's asynchronous level-loading guidance supplies an equally important negative contract: worker loading should avoid global-system, UI, transaction, delegate-registration, and nested synchronous-load interaction, then finalize at a controlled later phase. Sparkle's smaller equivalent is stronger and easier to prove: workers construct an isolated immutable `SceneLoadPackage`; only the main/world owner commits it, and failure leaves the active scene unchanged.

Direct Unreal source is access-controlled. This document therefore treats the official public documentation as the inspected authority and does not imply an audit of private engine source.

### Epic MassEntity, Unity DOTS Samples, and EnTT

The usual connection between ECS, data-oriented design, and job systems is real, but they are not synonyms:

- an ECS supplies identity, component storage, queries, and structural-change rules
- DOD chooses layouts and iteration around measured access patterns
- a job system executes independent ranges and dependency graphs

Epic MassEntity is the high-scale archetype reference. Entities and fragments are data-only, processors consume query batches, and composition changes are deferred through a command buffer because moving entities between archetypes during iteration would invalidate processing. Mass demonstrates the destination principles, not the minimum storage complexity Sparkle needs today.

Unity's public ECS samples make the job connection concrete. Jobs normally do not perform structural changes directly; per-job command buffers are played back at controlled system points, and parallel writers use deterministic sort keys because recording order follows nondeterministic thread scheduling. This directly supports Sparkle's task-local command buffers and ordered commit.

EnTT supplies the smaller C++ storage precedent: versioned opaque entity identifiers, per-type sparse-set pools, cheap const/writable views, and multi-component iteration led by the smallest pool. It demonstrates that a real ECS does not require an archetype/chunk runtime on day one.

Sparkle should build the narrow integration rather than import all three designs: a private sparse-set ECS kernel in GameFramework, task-graph systems over typed views, and deferred structural commands. Archetype chunks become a measured future storage alternative, not an architectural promise.

The inspected NVIDIA and AMD repositories remain primary references for loading pools, renderer preparation, and RHI work, but they do not provide a comparable general game-world ECS in the reviewed scope. The document does not invent an NVIDIA/AMD ECS precedent; Epic Mass, Unity's public ECS samples, and EnTT are the relevant authorities for this specific layer.

### O3DE AzCore and Atom

O3DE is the most directly reusable architectural study:

- AzCore exposes task tokens, dependencies, retained graphs, events, priorities, and executors.
- Atom’s FrameScheduler compiles a frame graph and can jobify command-list recording.
- ScopeProducer separates dependency setup, resource compilation, and command-list building.
- DX12 uses per-thread suballocators so frame-buffer-count × worker-count command allocator ownership avoids contention.
- Vulkan uses thread-local command-pool suballocators for the same external-synchronization reason.
- ThreadLocalContext makes repeated worker access cheap and provides a way to reset/collect all contexts after frames retire.

Sparkle should borrow the separation and ownership model. It need not reproduce O3DE’s APIs, module scale, or every scheduler feature. The inspected current task graph also contains unfinished cycle-checking work; source prestige does not remove the need for Sparkle-specific validation.

### Google Filament

Filament’s JobSystem is valuable for scheduler internals: a fixed-size cache-line-conscious job record, object-pool allocation, parent unfinished counts, per-thread work-stealing deques, worker sleeping, an adopted caller thread, and parallel_for.

Borrow:

- work-first scheduling and stealing
- parent/group completion accounting
- small allocation-conscious job representation
- worker naming, affinity/priority awareness, and false-sharing avoidance

Change:

- normal Sparkle task code should not synchronously wait and “help” indefinitely
- explicit prerequisites and continuations should make the graph inspectable
- background blocking work should not share the frame-worker pool

### Microsoft D3D12 Multithreading Sample

The sample makes the native API rule concrete: command lists and command allocators must not be used concurrently. It allocates per-frame, per-worker recording state, lets workers record independent portions, then submits closed command lists in the required serial order.

This is a foundational demonstration rather than a scalable job system. Sparkle should retain its ownership proof while replacing permanent event-driven renderer threads with tasks over worker-local contexts.

### NVIDIA CPU Scheduling, API, and Structured-Concurrency Guidance

NVIDIA's explicit-API CPU guidance reinforces Sparkle's recording design: distribute command-list creation/recording, close/reset on recording workers, keep expensive recording away from the submission thread, minimize submission calls, and count multi-queue synchronization cost. It also calls out resource creation and acceleration-structure work as CPU hitch sources that may need dedicated scheduling.

NVIDIA's worker-count guidance adds the missing hardware reality. Logical-core count is not a universal worker count because SMT siblings, heterogeneous cores, asymmetric/chiplet caches, atomics, context switches, memory pressure, and power behavior change the result. Sparkle therefore measures worker caps and physical/logical-core-like configurations; it does not infer quality from utilization or maximum worker count.

NVIDIA stdexec demonstrates the direction of standard C++ asynchronous composition: schedulers, `when_all`/bulk algorithms, structured scopes, cancellation paths, coroutine interop, and different CPU/I/O/GPU execution contexts. Sparkle should learn the vocabulary and compare semantics. It should not layer stdexec over SparkleTasks during this program: two task ownership models would undermine the single-runtime goal. A future replacement is an ADR requiring migration/deletion, not coexistence by default.

### AMD CPU/GPU Profiling, Frame Timing, and Queue Guidance

AMD's Ryzen guide makes production validation concrete: profile optimized builds, beware logging/stats serialization and cache pollution, verify parallel PSO creation and command-list generation with system traces, and determine CPU/GPU boundedness before optimizing. Radeon GPU Profiler then exposes GPU queue submissions, async compute/copy interaction, barriers, wave occupancy, cache behavior, and event timing.

The interview lesson is tool selection and correlation. A task trace answers CPU scheduling; RGP answers RDNA queue/wave behavior; neither alone proves input latency or C++ race freedom. Sparkle must carry stable frame/task/pass identifiers through existing markers so captures can be correlated without a permanent new telemetry product.

AMD FidelityFX frame-interpolation documentation is also a concurrency contract, not only an image-quality integration. Some Vulkan queues may be shared, some should be dedicated, and sharing a queue that the provider expects to own can deadlock or delay acquire/present synchronization. Sparkle's provider boundary must explicitly describe queue capability, ownership, submission serialization, frame identity, shutdown, and fallback.

### Epic PSO Precaching and Scheduler Failure Lessons

Epic's PSO precaching guidance demonstrates asynchronous compilation as a bounded product problem: cold caches, deduplication, background thread count, large per-compile memory, foreground contention, proxy/draw fallback, late/missed requests, and loading-screen policy all matter. Sparkle's smaller version must measure peak memory and frame impact, not merely move compilation off the render thread.

Epic's Tasks documentation records why opportunistic busy-waiting was deprecated: unrelated work, nested waits, long tasks, stack growth, deadlocks, and latency surprises. Its current oversubscription mechanism wakes standby capacity around known blocking regions. Sparkle keeps the simpler policy—normal worker tasks do not wait; blocking work is isolated—while understanding controlled oversubscription as a design alternative for a future measured need.

### Microsoft and Khronos Platform Contracts

The C++ standard memory model defines races and happens-before independently of x64 habits. Vulkan requires callers to externally synchronize many queue/pool/buffer operations and explicitly notes weakly ordered hosts. D3D12 permits multithreaded recording but forbids concurrent use of one allocator/list. Windows CPU traces distinguish running time from ready/preempted time, which is essential when “low utilization” is actually scheduling interference.

Microsoft DirectStorage demonstrates a future streaming backend with batched small reads and CPU/GPU decompression. Sparkle first needs the portable staged request/I/O/decode/commit/upload contract. If current I/O measurements later justify DirectStorage, only the read/decompression producer changes; asset identity, cancellation, generation acceptance, render upload, and packaging stay coherent.

### Engine-Wide Reference-to-Decision Ledger

The engine-wide additions are not justified by fashion or by the existence of a scheduler. Their reference lineage and Sparkle-specific adaptation are explicit:

| Sparkle decision | Primary top-tier precedent | What is borrowed | What is deliberately Sparkle-specific |
|---|---|---|---|
| one shared prerequisite task runtime | Epic Tasks, O3DE TaskGraph/Executor, Filament JobSystem | DAG dependencies, nested/group completion, events, fixed workers, stealing, small task records | bounded public concepts, explicit lanes, serial executor, no fibers/general framework |
| background content work with later owner finalization | NVIDIA Donut Scene/Texture loading, AMD Cauldron2 TaskManager, Epic async-level guidance | file/decode work off foreground, fan-in completion, delayed finalization, avoid UI/global interaction during load | immutable `SceneLoadPackage`, deterministic ID merge, old-scene retention, world-generation commit |
| game/editor data separated from rendering ownership | Epic game/render proxy model, O3DE Atom scheduler/RHI ownership | owner-specific representations, sequenced publication, no mutable gameplay dereference by render work | `WorldReadView`, `RenderWorldDelta`, current Sparkle camera/light/mesh/material domains |
| bounded ECS world storage | Epic MassEntity, Unity ECS samples, EnTT | data-only identity/components, typed queries, deferred structural mutation, packed iteration | GameFramework-private sparse-set pools first; no Mass/DOTS-scale framework or public generic SDK |
| declared GameFramework system dependencies | Epic/O3DE task graphs plus Mass/Unity query access | dependencies before waits, read/write component access, explicit graph validation | Sparkle phases and non-ECS resource domains integrated with typed ECS queries |
| serial owner plus queued mutation | Epic task pipes/named-thread ownership and documented game-thread loading finalization | serialize access to invariant-heavy state without making arbitrary callers lock it | one world commit, typed commands, deterministic merge, editor transaction integration |
| stable generations and stale-result rejection | retained task/run handles in Epic/O3DE plus renderer/resource-generation practices across NVRHI/Atom | generation-bound lifetime and explicit acceptance | world object IDs, editor document generations, bounded journal resync |
| editor-main ownership during background operations | Epic async-loading guidance and common game/editor thread separation | no UI/transaction/global callbacks from loader workers | private `EditorOperationService` and immutable current-workflow result models |
| per-worker native recording state | O3DE Atom, Microsoft D3D12 sample, NVRHI resource-state contract | thread-local allocators/pools and ordered submission | Sparkle frame-graph recording groups and existing RHI types |
| topology-aware worker policy | NVIDIA worker-count guidance, AMD Ryzen profiling, Windows scheduler traces | cap/sweep worker counts, distinguish physical/logical cores, diagnose migration/preemption | topology-neutral default, one internal override, no permanent affinity without proof |
| staged streaming and cold-cache PSO policy | Epic async loading/PSO precaching, NVIDIA resource-creation guidance, Microsoft DirectStorage | separate I/O/decode/create/commit, dedupe, budgets, late/miss policy, cold-cache tests | current asset/shader catalogs and one generation/residency authority |
| measured CPU/GPU queue concurrency | Epic RDG, AMD RGP/FidelityFX queue guidance, NVIDIA explicit-API guidance | graph-derived dependencies, async/copy overlap, fence/submission cost, provider queue ownership | existing Sparkle frame graph remains sole scheduling/barrier authority |
| correlated frame pacing/latency | AMD frame-timing role/FSR guidance, NVIDIA Reflex/PCL markers | simulation/render/present frame identity, bounded CPU lead, latency decomposition | provider-neutral engine markers and existing profiler hooks; no vendor-only architecture |
| production concurrency forensics | NVIDIA Nsight role/tools, AMD uProf/RGP/Ryzen guidance, Windows WPA | timeline-first critical-path analysis, crash/validation evidence, exact reproductions | injected Sparkle failures and regression tests; no default report subsystem |

The command buffer, read view, and change journal are a Sparkle composition of these ownership/generation principles, not a claim that NVIDIA, AMD, Epic, or O3DE exposes identical APIs. This distinction matters: reference prestige validates mechanisms, while current Sparkle data flow determines the actual interface.

## Canonical Concurrency and Rendering Vocabulary

Naming is part of the architecture. A reviewer should be able to infer whether an object is CPU work, an OS thread, a renderer message, a native command-recording resource, or a completion proof before opening its implementation. Sparkle therefore adopts the common semantic overlap of the inspected NVIDIA, AMD/GPUOpen, and Epic sources; it does **not** copy vendor prefixes, Unreal's `F`/`T` prefixes, C API Hungarian notation, or a sample's local spelling.

The inspected sources do not expose one universal dialect. NVIDIA Donut and AMD Cauldron2 use `Task`; AMD RPS's test-only pool uses `Job`; Epic describes Tasks as a job manager; NVIDIA stdexec uses sender/scheduler/operation vocabulary. The stable industry distinction is more important than choosing a brand: CPU schedulable work is a **task**, a physical execution agent is a **thread/worker**, and RHI/GPU work is a **command list/buffer** submitted to a **command queue**. Sparkle uses that distinction everywhere.

### Vendor Naming Crosswalk and Sparkle Decision

| Responsibility | NVIDIA precedent | AMD/GPUOpen precedent | Epic precedent | Sparkle canonical name | Rejected synonyms in new code |
|---|---|---|---|---|---|
| schedulable CPU unit | Donut `ThreadPoolTask`; stdexec `task` | Cauldron2 `Task`; RPS sample `Job` | `UE::Tasks::FTask` / `TTask` | `Task` | `Job`, `WorkItem`, `Work`, `ThreadTask` |
| debug/configuration record for CPU work | task callable/name in the reviewed runtimes | Cauldron `Task`; explicit RPS record info conventions | task debug name + priority | `TaskDesc`, matching Sparkle's existing `*Desc` convention | `JobInfo`, `TaskConfig`, `TaskData` |
| build-time graph reference | stdexec composed sender expressions | `RpsNodeId`, `RpsRenderGraphBuilder` | prerequisite task handles | `TaskNodeHandle` | bare `TaskNode`, pointer-like `TaskRef`, integer `TaskId` at the public boundary |
| immutable reusable dependency topology | stdexec composition | `RpsRenderGraph` after update/schedule | prerequisite DAG / TaskGraph | `CompiledTaskGraph` | `JobGraph`, mutable `TaskGraph` after compile |
| one submitted graph/single-task instance | sender operation/execution concepts | graph execute/record invocation | `FTask` is a handle to an actual launched task | `TaskExecution` | `TaskRun`, `JobInstance`, `Future` |
| per-submission bindings/state | execution context conventions | `RpsRenderGraphExecuteInfo`, record info/context | task body plus captured/explicit inputs | `TaskExecutionContext` | `TaskRunContext`, `JobContext`, unqualified `Context` |
| engine that makes tasks runnable | scheduler/execution context; Donut `ThreadPool` | Cauldron `TaskManager`; RPS `RpsAfxThreadPool` | scheduler and worker backend | `TaskExecutor` | public `TaskManager`, `JobSystem` type, `ThreadPool` type |
| lifetime-bounded structured work | stdexec `async_scope` | completion callback/fan-in is a narrower precursor | nested tasks and task lifetime | `TaskScope` | `TaskOwner`, `AsyncGroup`, unstructured fire-and-forget |
| externally triggered dependency | completion/wait primitives | RPS pool `WaitHandle` is a sample-local blocking handle | `FTaskEvent` | `TaskEvent` | generic `Event` in Tasks, `TaskFence`, CPU `Semaphore` as public API |
| data-parallel range operation | nvpro `parallel_batches`, `parallel_ranges`; stdexec bulk algorithms | RPS partitions command and draw ranges | `ParallelFor` | `ParallelFor` with `ParallelForPolicy` | `ParallelJobs`, `ForEachAsync` |
| latency class, not a physical queue/thread | scheduler/execution-context policy | background loading pool / explicit GPU queue classes | task priority classes | `TaskLane::{FrameCritical, Background, BlockingIo}` | `TaskLane::Frame`, `FastQueue`, `SlowThread`, priority used as a correctness edge |
| runtime/game owner thread | Donut application main loop | Cauldron main loop | `GameThread` | `GameThread` in runtime hosts | ambiguous `Main` in runtime-only ownership assertions |
| editor/UI owner thread | no editor-specific contract in inspected sources | no editor-specific contract in inspected sources | game/editor main-thread constraints | `EditorThread` in the editor host | `UiWorker`, generic `MainThread` when editor affinity matters |
| renderer owner thread/service | explicit submission-thread guidance | render host owns acquire/record/submit ordering | `RenderThread` | physical role/debug name `RenderThread`; owning type `RenderCoordinator` | service class `RenderThread`, `RendererWorker`, `RenderManager` |
| optional deferred RHI translation thread | no equivalent required by NVRHI | no equivalent in the inspected RPS design | `RHIThread` | reserved; do not use until such a thread exists | calling recording workers an `RHIThread` |
| game-to-render bounded publication | producer/consumer queues and ring resources | queued frames / command batches | game-to-render enqueued work and frame lead | `RenderFrameQueue` containing `RenderFramePacket` slots | `RenderFrameMailbox`, `FramePipe`, `FrameBuffer` |
| ordered non-frame renderer control | command enqueue patterns | explicit host operations | render commands / render command pipes | `RenderControlCommandQueue` | unqualified `CommandQueue`, `RenderThreadCommands`, `MessageBus` |
| renderer pass-facing command API | NVRHI `ICommandList` | RPS callback command buffer/context | `FRHICommandList`, `IRHICommandContext` | existing `RenderCommandContext` | `RenderContext`, `GraphicsContext`, `CommandEncoder` alias |
| native backend recording owner | NVRHI `ICommandList`; Vulkan/D3D12 native objects | `RpsRuntimeCommandBuffer`, acquired command buffers | parallel `IRHICommandContext` | backend-private `<Backend>CommandRecordingContext` | `WorkerContext`, `ThreadContext`, `CommandListPoolItem` |
| exclusive borrowed recording capability | command-list ownership/lifetime tracker | acquire/record contract | acquired parallel command context | public `RhiCommandRecordingLease` | `RecordingContextLease`, `CommandContextHandle`, raw allocator/pool pointer |
| GPU submission ordering/completion proof | NVRHI `CommandQueue`, execution instance; nvpro fences | RPS command batches/fence IDs | RHI command-list submit/fence | existing `ERhiQueueType` and `RhiSubmissionToken` | CPU `TaskEvent`, `TaskToken`, bare `Fence` in cross-module code |
| deterministic scheduled GPU unit | nvpro `BatchSubmission` | `RpsCommandBatch` / batch layout | RHI submit state and parallel command lists | existing `FrameGraphSubmissionBatch`; `RecordingGroup` for its recordable subset | `TaskBatch`, `RenderJob`, `CommandTask` |

The crosswalk is semantic evidence, not an assertion that the vendor APIs are interchangeable. For example, AMD RPS `WaitHandle` belongs to a small test framework, Epic `FTask` is a reference-counted launched-task handle, and NVRHI's returned command-list instance value is queue completion information. Sparkle adopts the responsibility a name communicates, then applies its own lifetime and boundedness rules.

### Binding Naming Grammar

1. **Task, thread, command, and queue are never synonyms.** A task is schedulable CPU work; a worker thread executes tasks; a render control command crosses into the render owner; a command list/buffer records RHI work; a GPU queue executes submitted command buffers.
2. **`Record`, `Submit`, and `Execute` describe different stages.** CPU task bodies execute. Workers record command lists/buffers. The render coordinator submits them. GPU queues execute them. Do not say a worker “submits a recording task” when it only records commands.
3. **A `Graph` is topology; an `Execution` is one submission.** `CompiledTaskGraph` is immutable and reusable. `TaskExecution` has one generation, terminal state, result, and completion lifetime. `TaskExecutionContext` contains bindings/state for that submission.
4. **A `Handle` identifies; a `Token` proves; a `Lease` borrows exclusively; a `Context` supplies transient operating state.** Handles are copyable only when identity/lifetime rules allow it and normally carry generation validation. Tokens prove ordering, completion, or permission but do not expose an object's general API. A lease is move-only, scoped, and returns exclusive capacity. A context is not an owner and must not become a service locator.
5. **A `Scope` owns asynchronous lifetime.** `TaskScope` cancellation and settlement follow the owning application/world/document/frame/tool lifetime. A scope is not a queue, a task group, or a mutex namespace.
6. **A `Lane` is executor policy, not correctness or hardware identity.** `FrameCritical`, `Background`, and `BlockingIo` select capacity and latency policy. Dependencies determine correctness. `ERhiQueueType::{Graphics, Compute, Copy}` names GPU capability; these types must never be overloaded.
7. **A `Queue` name must say what it queues.** Use `RenderFrameQueue`, `RenderControlCommandQueue`, private `TaskInjectionQueue`, and existing backend `CommandQueue` names. Bare `Queue`, `WorkQueue`, and `CommandQueue` are forbidden outside a scope where the qualifier is already encoded by the containing backend type.
8. **A service is named for the responsibility it coordinates.** Use `TaskExecutor`, `RenderCoordinator`, `EditorOperationService`, and `FrameGraphSubmissionExecutor`. Do not add `*Manager` or `*System` merely because a type owns several fields. Existing established names are changed only when their responsibility is materially false or the touched migration would otherwise create two meanings.
9. **Avoid implementation-advertising names.** `Async*`, `ThreadSafe*`, `LockFree*`, `MultiThreaded*`, `New*`, `*2`, and `MT*` do not establish ownership or correctness. Asynchrony belongs in the operation contract and return type; thread safety belongs in documented invariants and tests.
10. **Use existing Sparkle lexical conventions.** New C++ types use PascalCase without Unreal `F`/`T` prefixes; descriptors remain `*Desc`; RHI public types retain the `Rhi`/`ERhi` convention; backend-private types retain `D3D12` or `Vulkan`; acronyms follow current names such as `Rhi`, `Gpu`, `Io`, `D3D12`, and `Vulkan`. Do not normalize unrelated legacy spellings during this program.
11. **Thread role names must tell the truth.** Runtime, editor, renderer, task, and blocking-I/O roles use debugger/profiler names such as `Sparkle.GameThread`, `Sparkle.EditorThread`, `Sparkle.RenderThread`, `Sparkle.Task.FrameCritical.0`, and `Sparkle.Task.BlockingIo.0`. `RHIThread` is reserved until Sparkle actually owns a deferred translation thread.
12. **One concept has one canonical search term.** Compatibility aliases are allowed only during the prompt that deletes their callers; they may not enter new signatures, tests, documentation, captures, or profiler labels.

### Planned-Name Reconciliation Before Implementation

These are design-document renames, not claims that the corresponding code exists:

| Earlier plan spelling | Canonical spelling | Reason and migration rule |
|---|---|---|
| `TaskNode` token | `TaskNodeHandle` | says it is a generation-validated reference, not the task record itself |
| `TaskRun` | `TaskExecution` | matches one submitted execution and avoids a vague verb-as-type beside compiled topology |
| `TaskRunContext` | `TaskExecutionContext` | binds the context to one submission and leaves plain `Context` unavailable |
| `TaskLane::Frame` | `TaskLane::FrameCritical` | communicates latency/capacity policy rather than ownership of a frame |
| `TaskSchedulerConfig` | `TaskExecutorConfig` | configuration belongs to the executor host contract; no second public scheduler object exists |
| `RenderThread` service type | `RenderCoordinator` | separates the physical thread role from the object owning renderer lifecycle and submission |
| `RenderFrameMailbox` | `RenderFrameQueue` | matches the bounded producer/consumer structure and avoids introducing actor terminology nowhere else used in Sparkle |
| `RenderThreadCommands` | `RenderControlCommandQueue` | distinguishes lifecycle/settings commands from RHI command recording and GPU queues |
| `RecordingContextLease` | `RhiCommandRecordingLease` | establishes module, command-recording responsibility, and move-only exclusive borrowing |
| generic `WorkerRecordingContext` | backend-private `<Backend>CommandRecordingContext` | records native backend commands; task-worker identity is runtime state, not the type's ownership |
| “job system” as a C++ type/module name | `SparkleTasks` task runtime; “job system” only as the recognized architectural category | keeps searchable interview vocabulary without creating `Job` and `Task` aliases |

Before Prompt 01 adds a public symbol, search the repository and this document for its canonical term and every rejected synonym. If an existing strong Sparkle type already satisfies the responsibility, use or extend it. If a current name materially conflicts, the owning prompt records use/extend/replace/add, updates call sites and profiler labels atomically, and deletes the old spelling before its gate. No permanent typedef, duplicate header, or documentation alias remains merely to make a vendor name appear in the repository.

## Target Architecture

### Dependency Shape

The rendering ownership layers remain:

~~~text
GameFramework  ─────► Renderer ─────► RHI
     │                    │            │
     └────────────┬───────┴────────────┘
                  ▼
              SparkleTasks
                  │
                  ▼
             SparkleCore
~~~

Application and offline tools may also host SparkleTasks. SparkleTasks must not depend on GameFramework, Renderer, RHI, editor UI, assets, or platform windows.

Suggested repository shape:

~~~text
Engine/
  Tasks/
    Public/
      TasksAPI.h
      TaskTypes.h
      TaskGraph.h
      TaskExecutionContext.h
      TaskExecution.h
      TaskExecutor.h
      TaskScope.h
      TaskEvent.h
      ParallelFor.h
    Private/
      Graph/
        TaskGraph.*
        TaskGraphInternal.h
        ParallelFor.cpp
      Execution/
        TaskTypes.cpp
        TaskExecution.cpp
        TaskExecutionInternal.h
        SerialTaskExecution.cpp
      Scheduling/
        TaskExecutor.cpp
        TaskExecutorImplementation.*
        TaskExecutorRuntime.h
        ScheduledTaskExecution.*
        TaskWorkerScheduling.cpp
      Lifetime/
        TaskScope.*
        TaskScopeInternal.h
        TaskEvent.cpp
      Profiling/
        TaskProfiler.*

Engine/GameFramework/
  Public/
    World/EntityId.h
    World/WorldReadView.h
    World/WorldCommand.h
    Assets/SceneLoadRequest.h
    Assets/SceneLoadResult.h
  Private/
    World/ECS/EntityRegistry.*
    World/ECS/ComponentStorage.*
    World/ECS/ComponentTypeRegistry.*
    World/ECS/Query.*
    World/ECS/EntityCommandBuffer.*
    World/GameSystemGraph.*
    World/WorldCommandCommit.*
    World/WorldChangeJournal.*
    World/WorldReadPublisher.*
    Assets/SceneLoadOperation.*
    Scene/Transforms/TransformEvaluator.*
    Scene/Animations/AnimationEvaluationSystem.*

Engine/Editor/
  Private/
    Operations/EditorOperationService.*
    Scene/EditorSceneModel.*
    Scene/EditorCommandQueue.*
    Scene/EditorTransactionService.*

Engine/Renderer/
  Public/
    Renderer.h
    RenderFramePacket.h
    RenderObjectId.h
  Private/
    Threading/
      RenderCoordinator.*
      RenderFrameQueue.*
      RenderControlCommandQueue.*
    SceneData/
      RenderWorld.*
      RenderWorldDelta.*
      RenderFrameDynamicData.*
      RenderProxyRegistry.*
      GpuScene.*
    FrameGraph/
      FrameGraphRecordingPlan.*
      RecordingGroup.*

Engine/RHI/
  Public/
    RhiCommandRecordingLease.h
  Private/<backend>/
    CommandRecordingContext.*
    WorkerUploadArena.*
    WorkerDescriptorArena.*
~~~

The names above are canonical under the vocabulary section. A backend-private type expands `<Backend>` in C++ (`D3D12CommandRecordingContext` or `VulkanCommandRecordingContext`) even when the backend directory makes a shorter filename reasonable. The responsibilities and dependency direction are binding.

Only stable cross-module contracts belong in `Public`. System graph compilation, journal retention, editor models/operations, worker scheduling, and concrete load stages stay private. If existing conventions favor fewer files, co-locate small types; the tree communicates responsibility, not a requirement to manufacture one class per line.

### Threading-Aware RHI and Frame-Graph Contract

Multithreading must sharpen the existing boundary rather than move policy into another wrapper.

| Concern | Owner | Parallelism rule |
|---|---|---|
| Logical pass/resource/history declaration | Renderer pass + frame graph | Completed before recording; immutable during parallel execute |
| Pass ordering, queue assignment, barriers, aliasing, and cross-queue edges | FrameGraphCompiler | One compiled authority; workers consume the plan |
| Native command allocator/pool/list/buffer ownership | D3D12/Vulkan RHI | One leased context per recording task; never concurrently reused |
| Descriptor heap/set implementation | RHI backend | Worker-local transient blocks; persistent descriptors remain backend-owned |
| Upload/readback allocation and mapping | RHI service/backend allocator | Worker-local pages or preassigned slices; token-retired |
| Queue submission, fence/timeline values, present | Render coordinator through RHI | Single CPU owner; compiled ordering preserved |
| Resource allocation and physical destruction | Render coordinator/RHI resource service | No creation/destruction from recording workers; deferred GPU-safe retirement |
| Persistent/transient lifetime policy | Renderer frame graph/GPU scene above explicit RHI resources | Packet lifetime and GPU lifetime remain separate |
| Backend capability truth | Immutable RHI capability snapshot | Read-only from workers; no backend-specific feature guessing |
| Native interop | Private backend or narrow provider bridge | Never stored in GameFramework packets or generic task inputs |

Pass authoring rules remain:

- setup declares every read, write, history, queue preference, and persistent/imported resource
- compile resolves ordering, lifetime, aliasing, barriers, queue ownership, and recording eligibility
- execute records commands only; it does not discover hidden dependencies, allocate undeclared resources, create pipelines, or retain packet memory
- a provider that needs native interop receives only tagged resources and timing/camera data through its narrow bridge

`RhiCommandRecordingLease` is a move-only capability proving temporary exclusive access to backend recording state. It is not a new command API and must not become a convenience wrapper over `RenderCommandContext`.

### Public-Surface Budget

The multithreading seam should be smaller than the implementation behind it.

Allowed public concepts:

- TaskExecutor host configuration and the minimal task/graph primitives used by more than one owned module
- TaskScope/cancellation seam required to bind runs to application/world/document/asset/tool owners
- stable `EntityId`, immutable `WorldReadView`, typed editor/game world commands, and scene-load request/result contracts used across Application/GameFramework/Editor
- RenderFrameSubmission or equivalent packet-publication seam between Application/GameFramework and Renderer
- stable asset/render handles required to cross that seam
- existing explicit RHI command/resource/capability contracts

Private concepts:

- worker queues, deques, thread IDs, task records, dependency counters, profiler events, graph validation, packet slot states
- system graph compiler, resource-domain hazard tables, command merge/commit implementation, journal storage/retention, scene-load stages, editor scene model/operation registry
- RenderWorld, render proxies, GPU-scene allocation indices, recording groups, worker context pools, upload pages
- task/renderer timing and queue-depth observations

A proposed public type fails review when its only consumer is an editor panel, test, report, or future feature.

### Thread Roles

| Role | Owns | May access | Must not do |
|---|---|---|---|
| GameThread | Runtime window pump, input, GameScene structural commit, level command application, world/read/render publication | Immutable asset registry and narrow product-owned renderer results | Touch mutable renderer/RHI state; block for routine render completion |
| EditorThread | ImGui/UI model, editor transactions/commands, viewport and operation requests | Published `WorldReadView`/editor model and narrow immutable renderer/operation results | Traverse worker-mutated GameScene storage; give live ImGui/editor pointers to workers or RenderThread |
| Render coordinator | RendererSystemRoot, FramePipeline coordination, RenderWorld, GPU caches, RHI creation/destruction, submit/present | Published frame packets and task results | Dereference GameScene; execute blocking tool/file work |
| FrameCritical task workers | Data-pure game/render tasks and command recording with leased local contexts | Immutable inputs, disjoint output slices, concurrent task runtime | Wait on other tasks, submit queues, mutate global caches, destroy RHI resources |
| Background/BlockingIo task workers | File reads, child processes, imports, compression/compilation where safe | Explicit task input and output publication objects | Consume latency-sensitive FrameCritical capacity |
| GPU queues | Execute compiled command streams | RHI resources covered by barriers/fences | Be described as CPU task concurrency |

The row label `Render coordinator` names the owning service; its physical role and debugger label are `RenderThread`. Each engine-created thread must have a stable name visible to the platform debugger and existing PIX/Nsight/profiler-visible CPU instrumentation. Use `Sparkle.GameThread`, `Sparkle.EditorThread`, `Sparkle.RenderThread`, `Sparkle.Task.FrameCritical.0…N`, `Sparkle.Task.Background.0…M`, and `Sparkle.Task.BlockingIo.0…M` where those roles exist. A command-line tool uses `Sparkle.ToolMain`. This requirement does not authorize a new profiling framework.

### Frame Pipeline

The normal mode is one CPU frame of game/render decoupling:

~~~text
time ───────────────────────────────────────────────────────────────►

Main/game       commit + system graph N+1 ── publish world/read/render N+1 ── N+2
                         │
Render thread   Prepare N ─ task graph ─ record N ─ submit N
                                                   │
GPU             Execute N-1 ─────────────────────── Execute N

Background/IO   scene/cook operation K ── immutable result ── owner commit later
~~~

The maximum queue depth is bounded by explicit frame slots. A slow renderer eventually parks the main thread while acquiring a free slot; it does not allocate an unbounded backlog and increase input latency.

Required modes:

- Threaded, one-frame-ahead: product default after validation.
- Threaded, zero-ahead: useful for latency comparisons and debugging.
- Serial reference: main thread runs the same packet consumer without a render thread.
- Headless/tool host: task runtime without window, present, or render thread.

The serial reference must use the same data contracts. It is not permission to keep the legacy raw-pointer path.

### Ownership State Machine

Each frame slot follows one direction:

~~~text
Free
  │ producer acquires
  ▼
Writing
  │ release-publish
  ▼
Ready
  │ render acquire-consumes
  ▼
Rendering
  │ CPU packet no longer needed
  ▼
Retired
  │ render acknowledgement / arena reset
  ▼
Free
~~~

GPU-resource retirement is separate. A CPU packet can be reusable after render recording has copied/consumed it, while upload pages, descriptors, command allocators, and transient resources remain tied to a GPU completion token.

Do not encode both lifetimes in one boolean.

## SparkleTasks Task Runtime Design (Engine Job System)

`SparkleTasks` **is Sparkle's engine job system**, expressed with the canonical `Task` vocabulary shared by Epic Tasks, NVIDIA Donut/stdexec, and AMD Cauldron2. “Job system” remains the architectural/interview category and a useful research search term; `Job` does not become a second C++ noun for the same scheduled unit. The runtime implements fixed workers, work stealing, task records, dependencies, fan-out/fan-in, `ParallelFor`, cancellation, lanes, sleeping/waking, deterministic serial execution, scopes, and shutdown.

It is not a wrapper around `std::async`, a thread-per-subsystem design, or merely an offline thread pool. Its renderer, ECS, editor, loading, and tool consumers are required acceptance evidence.

### Why an Engine-Owned Task Runtime

An engine-owned implementation is justified here because it is part of the learning and portfolio goal, it needs integration with render thread roles and existing profiler/debugger hooks, and it will be reused by owned tools. It should still be deliberately bounded. The engine does not need to invent fibers, a coroutine runtime, a distributed build system, or a lock-free scheduler research project.

The first production-capable version should provide:

- a fixed worker set configured once per host
- local worker queues plus stealing
- a thread-safe external injection path
- prerequisites and continuation scheduling
- fan-out and fan-in
- nested task creation without waiting
- parallel_for with grain-size and serial threshold
- cancellation and explicit failure reporting
- structured task scopes bound to host-owned lifetimes
- foreground/frame and background/blocking lanes
- deterministic serial execution
- thread/task names, priorities, and private capture-time timing/dependency events
- safe repeated startup/shutdown in tests and tool processes

### Public Concepts

Illustrative API, not a frozen header:

~~~cpp
enum class TaskLane : uint8_t
{
    FrameCritical,
    Background,
    BlockingIo
};

struct TaskDesc
{
    TaskName name;
    TaskLane lane = TaskLane::FrameCritical;
    TaskPriority priority = TaskPriority::Normal;
    CancellationToken cancellation;
};

class TaskGraphBuilder
{
public:
    template<class Fn>
    TaskNodeHandle Add(TaskDesc desc, Fn&& function);

    void DependsOn(TaskNodeHandle task, TaskNodeHandle prerequisite);
    TaskNodeHandle WhenAll(TaskName name, std::span<const TaskNodeHandle> prerequisites);
    CompiledTaskGraph Compile();
};

class TaskExecutor
{
public:
    TaskExecution Submit(const CompiledTaskGraph&, TaskExecutionContext&);
    TaskExecution Submit(TaskDesc, TaskFunction);
    void PumpAdoptedThread(TaskLane, uint32_t budget);
};

template<class Range, class Fn>
TaskNodeHandle ParallelFor(
    TaskGraphBuilder& graph,
    TaskDesc desc,
    Range range,
    ParallelForPolicy policy,
    Fn&& function);
~~~

Important semantic decisions:

- `TaskNodeHandle` is a builder-local, generation-validated handle, not a pointer or a runtime task object.
- A compiled graph owns immutable topology and may be reusable if its bindings are supplied per run.
- `TaskExecution` tracks one submission generation and its terminal accounting; `TaskExecutionContext` supplies that submission's bindings/state.
- Handles use generation counters so stale handles fail deterministically.
- A task function receives its context explicitly; scheduler thread-local globals are not business-data channels.
- Tasks return an explicit TaskResult or are noexcept. Exceptions at the boundary are captured, reported, and cause a defined dependent cancellation policy. They are never silently swallowed.

### Prompt 01 Implemented Baseline — Serial Semantics Before Scheduling

As of 2026-07-18, `Engine/Tasks` implements the deliberately serial subset of this design. This is the executable reference model that later worker implementations must preserve, not a temporary single-threaded API:

- `TaskGraphBuilder` issues builder-identity and generation-validated `TaskNodeHandle` values; public callers never receive a task-record pointer.
- `Compile()` snapshots immutable topology and rejects invalid names/policies, invalid/foreign/stale handles, self or duplicate edges, both ordinary dependency cycles and nested start/completion deadlocks, and configured task/edge overflow before any body can run.
- The serial `TaskExecutor` chooses the lowest builder index among ready nodes. This makes the oracle deterministic without claiming that later parallel body order will be deterministic.
- A nested child may start only after its parent's body. The parent reaches logical completion only after its own body and every nested descendant settle; continuations of the parent therefore observe the whole group.
- The first executed failure is retained. Normal dependents settle as cancelled without running; nodes explicitly marked `Cleanup` still run once their prerequisites settle. Every accepted node obtains exactly one terminal `TaskResult`.
- Each submission has a distinct execution generation and owns the terminal results it exposes. A compiled graph is reusable with separate `TaskExecutionContext` bindings; graph callables are released with the graph and are not retained by an already-settled execution.
- Builder and executor capacities are explicit. There is no overflow path that silently admits more task or edge records, and rejected submissions execute no bodies.

The current public concepts already have named future consumers: renderer extraction/recording DAGs, GameFramework system phases, editor document operations, asset/tool pipelines, and RHI command preparation. Their integrations remain deferred; no consumer dependency points back into `SparkleTasks`. Ready queues, runtime records, adjacency representation, and settlement counters remain private so Prompt 02 can change scheduling mechanics without changing graph meaning.

The implementation was gated with a transient contract harness that repeated every scenario through the serial executor, checked graph reuse and retained-result lifetime, and exhaustively enumerated all directed four-node graphs against an independent topological oracle. For every accepted DAG it injected failure and cancellation at each node and verified deterministic body selection plus exactly-once terminal accounting. A transient dependency scan rejected forbidden product/graphics/platform dependencies, worker/wait primitives, and rejected task aliases. Both passed before their test source, CTest target, and boundary script were removed; Sparkle retains the production contract and this evidence rather than a prompt-specific maintained test subsystem. MT-03–06 and MT-10 are avoided because this phase publishes no cross-thread payload and contains no worker or wait protocol; MT-13–15, MT-23, MT-29, and MT-44 were falsified through immutable topology, dual-cycle validation, unfinished-count settlement, bounded rejection, generation validation, and exhaustive oracle comparison.

### Structured Task Scopes and Owner Lifetime

An executor lifetime is too broad to prove that captured engine objects are alive. Every asynchronous run therefore belongs to a `TaskScope` owned by an existing product lifetime:

| Scope | Owner | Cancellation/settlement boundary |
|---|---|---|
| Application | runtime/editor/tool host | host stops accepting work, cancels children, drains before services are destroyed |
| World/scene generation | `GameScene` generation | level replacement or scene destruction cancels unpublished work; accepted commits settle first |
| Editor document | editor scene/document generation | close/reload cancels operations; stale results fail generation validation |
| Asset generation | asset request/publication generation | superseding request cancels decode; published/GPU-used data retires by its separate lifetime token |
| Render frame | bounded packet/frame slot | frame tasks settle before packet storage is acknowledged for reuse |
| Tool invocation | one cook/build/package command | cancellation drains child processes/I/O and prevents publication |

Required rules:

- scopes form a parent/child tree; cancellation flows down and completion flows up
- destroying an owner without cancelling and settling its scope is a development assertion
- a task does not capture raw `this` unless the scope is a member destroyed after settlement and the capture is locally provable
- cross-frame or cross-generation work captures immutable values, stable handles, and the expected generation
- cancellation requests are nonblocking; cleanup is represented by continuations/finally tasks, not arbitrary cancellation callbacks executed on the requesting thread
- a stale result is normal control flow and is discarded without mutating its former owner
- renderer GPU retirement remains token-based and is not confused with CPU task-scope completion

This is the engine-wide replacement for isolated `std::future`, detached thread, and raw-callback lifetimes. It follows the prerequisite/nested-task lesson from Epic and retained-run lifetime lesson from O3DE while keeping Sparkle's public surface smaller.

### Scheduler Mechanics

Recommended first architecture:

1. Each frame worker owns a double-ended ready queue.
2. A worker pushes and pops its local work from the preferred end.
3. An idle worker steals from another worker’s opposite end.
4. External producer threads place work in a mutex-protected injection queue.
5. Workers use a semaphore or condition variable when no runnable work exists.
6. Completion atomically decrements each dependent’s remaining-prerequisite counter.
7. The thread that makes a dependent ready schedules it immediately, favoring locality.
8. A group/parent holds an unfinished-child count and schedules its continuation when it reaches zero.

Start with correctness-oriented queues if necessary. A locked per-worker queue can validate semantics before a specialized Chase–Lev deque. The public task graph must not depend on queue implementation details.

Task records should be small, stable, and allocation-conscious:

- inline callable storage for normal lambdas
- a slab/object pool rather than one heap allocation per task
- separated or padded hot atomics
- immutable metadata after submission
- a per-run arena for graph instances and edges
- generation validation in development builds

### Prompt 02 Implemented Baseline — Fixed Workers Without Changing Graph Meaning

As of 2026-07-18, zero configured lane workers select the deterministic caller-thread oracle. Prompt 02 originally proved one fixed 1/2/N worker set; Prompt 03 evolved that host policy into `TaskExecutorConfig::{FrameCriticalWorkerCount, BackgroundWorkerCount, BlockingIoWorkerCount}` without changing graph identity or result meaning. The one executor owns every worker and names it `Sparkle.Task.<Lane>.<index>` through Core's existing `Threading::SetCurrentThreadRole`; SparkleTasks adds no public scheduler, subsystem pool, priority, or affinity abstraction.

The public executor remains intentionally synchronous at this stage: `Submit()` is a host boundary and returns a settled `TaskExecution`. This keeps the caller-owned `TaskExecutionContext` alive without inventing Prompt 03's `TaskScope` early. A task running on one of the executor's workers cannot synchronously submit back to that executor; it receives deterministic rejection instead of consuming a worker while waiting. Child and continuation work must be expressed in the compiled DAG. This is the practical MT-13 rule: workers execute nodes, hosts wait at declared boundaries.

The scheduling mechanism is private:

- each worker owns a mutex-protected deque, consumes its preferred end, and thieves consume the opposite end;
- host submissions enter a synchronized injection queue; a worker that makes a dependent ready pushes it locally, preserving work-first locality;
- an epoch protected by the parking mutex is changed after publication and checked with a condition-variable predicate; workers rescan while holding that mutex before sleeping, closing the notify-before-park window without polling;
- prerequisite counts, nested unfinished counts, parent publication flags, schedule claims, and terminal claims use explicit release/acquire or acquire-release transitions; the callable/result payload itself is protected by the run result mutex;
- each run allocates its runtime-state array and terminal-result storage once from the compiled bounded size. `MaximumActiveExecutions` bounds simultaneous runs, making the theoretical queued-record ceiling `MaximumActiveExecutions × MaximumTasksPerExecution`; overflow rejects before a body starts;
- worker queue state is cache-line aligned because adjacent workers write independent mutex/deque state. Per-task runtime records are deliberately not padded: padding 1,024 records without cache/remote-traffic evidence would multiply run memory for résumé value, so Prompt 25 owns that A/B decision.

The lifecycle state machine is `Accepting → Draining|Cancelling → Stopping → Stopped`. Acceptance and active-run registration share the state mutex, so shutdown cannot miss a run between admission and registration. Drain closes admission and lets accepted graphs finish. Cancel also marks accepted parallel runs: already-running bodies finish, queued normal bodies settle cancelled, and cleanup nodes execute. Zero-worker work has no queue to revoke, so an already-executing serial body drains; cooperative mid-body cancellation remains Prompt 03. Workers stop only after the active-run count reaches zero, every joinable thread is joined, repeated shutdown is idempotent, and late submission is rejected.

Transient Prompt 02 validation was created, executed, and removed under K Rule 11. Prompt 01 parity and nested/failure/cleanup semantics passed at 0/1/2/8 workers with randomized yields. A 1,024-node DAG repeated 250 times crossed 1,021,500 atomic prerequisite transitions; concurrent external producers completed 400 submissions; bounded active-run overflow rejected; a root-local 256-node fan-out executed on multiple worker IDs and produced real opposite-thread steals; recursive same-executor submission rejected at one worker; 40 repeated 0–4-worker shutdown cycles, running-work drain, queued-work cancellation, cleanup settlement, and late rejection passed. Eight parked workers consumed 0 ms process CPU over a 200 ms idle interval.

On the measured 16-physical-core/32-logical-thread host, the deliberately skewed transient batch took approximately 10.2–11.8 ms at one worker, 2.0–2.4 ms at 16 workers, 2.3–2.4 ms at 32 workers, and 2.3–2.8 ms at 33 workers. Parked enqueue-to-start was 128–195 µs in DevelopmentEditor; local dependent start was 50–67 µs and stolen dependent start 25–28 µs in the recorded runs. These are characterization points, not defaults or durable benchmarks. The current Visual Studio/MSVC Windows configuration has no supported ThreadSanitizer mode; the strongest available gate was repeated optimized stress plus one DebugEditor run. ETW context-switch traces, cache/remote-traffic counters, p95/p99 distributions, third-party nested-pool budgeting, grain-size policy, and a physical/logical default decision remain explicitly unclaimed and belong to Prompts 23–25.

Hazard closure for this slice is therefore bounded: MT-13 rejects recursive worker submission; MT-14/15 use parked predicate/epoch progress and passed idle plus external-publication races; MT-17 keeps one executor-owned pool; MT-18/20 retain serial control and measured topology/skew behavior; MT-21 preserves dependency-driven readiness; MT-22 adds no priority correctness; MT-23 bounds active runs; MT-25 aligns only measured-by-layout worker state; MT-41 records the sanitizer gap; MT-42 uses atomic checkpoints for correctness ordering; MT-43 labels timing as characterization rather than a product claim; and MT-44 deliberately retains locked deques with no reclamation problem. MT-16, MT-19, and MT-24 are not falsely closed: BlockingIo lanes, grain policy, and third-party inner-thread budgets do not exist in this prompt and are owned by later stages.

### Prompt 03 Implemented Baseline — Structured Lifetime and Lane Policy

Prompt 03 completes the reusable engine job-system contract without introducing `Job` as a C++ vocabulary. `TaskScope` represents Application, World, Document, AssetGeneration, Frame, and ToolInvocation ownership. Parent cancellation closes admission and flows to descendants; a child contributes to its parent's unfinished state until it settles. Scope owners must explicitly join or cancel/join at a bounded owner boundary, and development builds assert if destruction begins before settlement. Asynchronous user data must be empty or held by the context's `shared_ptr`; borrowed stack context remains available only to synchronous `Submit`. The scope and its execution state retain all scheduler records/results needed until exactly-once publication, so dropping a `TaskExecution` handle does not detach the run.

Cancellation is cooperative, not forcible thread termination. Each task body receives a `stop_token` and cheap `IsCancellationRequested()` checkpoint. Requesting cancellation changes scheduler state and invokes only runtime-owned wake notifications; it never synchronously calls arbitrary task/user callbacks. Queued Normal bodies settle cancelled when cancellation or a failed prerequisite reaches them, while Cleanup bodies still execute. Workers cannot call bounded `TaskExecution::WaitFor` or recursively submit to their executor; continuations, prerequisite edges, nested completion, and `TaskEvent` express progress instead. Only the recorded scope owner may make a bounded join.

`TaskEvent` is a one-shot external-completion bridge, not a callback bus. Identity plus generation rejects foreign, stale, reset, and double signals. A BlockingIo task parks on its condition predicate and cancellation notification; there is no polling. Event state is retained while a waiter is active, and reset is rejected while waiters exist. Core `Event` remains owner-thread-affine and unchanged.

`ParallelFor` emits exclusive `[begin,end)` nested ranges beneath a group node. Grain size, serial threshold, and maximum partition count are explicit call-site policy; invalid zero values fail graph compilation. The range function receives the same lane and cancellation context as any task. This is deliberately a graph-building primitive rather than an immediate hidden fork/join that would block its caller.

One `TaskExecutor` owns lane-isolated worker capacity and private queues. FrameCritical, Background, and BlockingIo workers share the same graph/execution/scope contract but steal only within their lane, so blocking native/event waits cannot consume frame workers and sustained background CPU work cannot consume all interactive capacity. A parallel graph using an unconfigured lane rejects before a body starts. FrameCritical depending on Background or BlockingIo rejects at compile time because priority/capacity is not a correctness mechanism; completion of external work must instead publish a new frame-critical continuation at an owner boundary. Zero total workers retains the serial reference executor.

Private Windows TraceLogging instrumentation emits topology dependencies plus task begin/end records with name, lane, run generation, task/worker index, duration, and outcome. It adds no public snapshot, panel, report, callback, CVar, or log stream, and the disabled provider avoids timestamp work. The provider compiled and linked in focused DevelopmentEditor/DebugEditor builds. A live ETW capture could not be started from the non-administrator validation shell, so capture contents remain explicitly unclaimed rather than replaced with permanent test hooks.

Transient validation ran twenty repeated mixed-lane DevelopmentEditor cycles, then a focused DebugEditor owner-destruction death case; the harness was deleted. It covered scope settlement, nested completion through ParallelFor, cancellation/event wake, cleanup, invalid lane/policy compilation, lane isolation under sustained background/I/O load, ordered drain/cancel shutdown, and LC-05 callback re-entry. The current Windows/MSVC configuration still has no supported ThreadSanitizer mode; repeated optimized stress and the debug negative run are the strongest available local gates, not a substitute claim for TSan.

### Dependency Rules

The normal synchronization vocabulary is:

- prerequisite edge
- WhenAll/fan-in
- continuation
- event completed by an external system
- bounded host-thread wait

The following is invalid inside a frame worker:

~~~cpp
auto child = executor.Submit(...);
child.Wait(); // prohibited in normal worker code
~~~

Use:

~~~cpp
TaskNodeHandle child = graph.Add(...);
TaskNodeHandle continuation = graph.Add(...);
graph.DependsOn(continuation, child);
~~~

Development builds should detect a worker blocking on a `TaskExecution` and report the task name and submission graph. This rule prevents pool exhaustion and makes the critical path inspectable.

### Graph Validation and Boundedness

TaskGraphBuilder::Compile must reject:

- self-dependencies and dependency cycles
- stale or foreign builder tokens
- duplicate edges when they would corrupt counters
- a graph whose task/edge count exceeds configured arena limits
- a continuation whose completion policy cannot settle
- invalid cross-lane dependencies

Graph storage is bounded per run. Overflow returns a controlled build/submission failure before any task starts; it does not silently heap-allocate an unbounded graph during a frame.

Task priority is a scheduling hint, never a correctness mechanism. A FrameCritical task must not depend on an unscheduled low-priority Background task or a BlockingIo task. Runtime content that is not ready uses an existing resident fallback or delays publication at an explicit host boundary. If an external event is genuinely frame-critical, its continuation is injected into the FrameCritical lane only after the external operation completes.

### Executor and Render Shutdown Order

Shutdown is part of the concurrency design:

1. Application stops accepting gameplay/editor commands that can publish new frames.
2. `RenderFrameQueue` closes and wakes both producer and consumer.
3. Render coordinator consumes or cancels accepted packets according to explicit policy.
4. Recording/preparation task runs settle; no task may retain renderer state afterward.
5. Render coordinator performs the one legitimate final GPU drain, destroys swapchain/RHI state on its owner thread, and exits.
6. Background tool/process tasks are cancelled or drained according to product workflow.
7. Task executors stop accepting work, wake workers, join threads, and release task arenas last.

Device loss follows a separate controlled RenderThread failure path; it must not strand a frame-queue slot, task continuation, or editor waiter.

### Events

TaskEvent exists for work completed outside the worker executor: an asynchronous file operation, child process, platform callback, or optional GPU readback. It is not a general replacement for graph edges.

An event has:

- a single explicit completion transition
- a generation or one-shot lifetime
- registered continuations
- cancellation behavior
- no polling loop on a worker

GPU frame completion should normally remain in the RHI token/retirement system. Converting every GPU fence to a CPU TaskEvent would encourage CPU waits and is not required for rendering.

### Lanes and Oversubscription

Frame tasks and blocking work require different policies.

Frame-critical lane:

- approximately hardware_concurrency minus main, render, and essential platform threads
- short, nonblocking tasks
- work stealing
- latency-sensitive priority

Background lane:

- low-priority CPU work such as hashing, planning, and safe compilation
- concurrency limited so it cannot consume all cores during interactive use

BlockingIo lane:

- small number of threads allowed to wait on files/pipes/processes
- never stolen onto frame workers
- can signal continuation events into frame or background lanes

Offline tools may configure most cores for the background/computation lane because no interactive frame exists. The same API can host a different policy.

Do not count the OS, shader compiler’s own internal threads, D3D/Vulkan driver threads, compression-library threads, and child processes as free capacity. Thread counts must be configurable and visible in an explicit launch configuration or profiler capture, not a routine runtime report.

### Cancellation and Failure

Cancellation is cooperative:

- a cancellation source owns state
- tasks observe it at coarse safe points
- a cancelled prerequisite causes dependents to cancel unless they are explicitly cleanup/finally tasks
- task completion counters always settle exactly once
- partially written public outputs are never accepted

Failure policy differs by host:

- Runtime frame tasks: record the first failure, cancel dependents, enter a controlled fatal/error path; do not render partially initialized data.
- Editor background tasks: keep the previous published generation and display diagnostics.
- Offline cooks: finish or cancel independent work according to policy, collect deterministic diagnostics, and return failure without publishing the new manifest.

### Determinism and Debuggability

Scheduler execution order is intentionally nondeterministic. Engine results need not be.

Require:

- stable sort keys before render submission
- deterministic merge order for per-task outputs
- stable asset registry and manifest ordering
- no task-completion order embedded in IDs or file layout
- fixed random seeds for tests
- serial scheduler mode
- worker counts 1, 2, and N in CI/stress tests
- optional randomized stealing/yield injection in concurrency tests

In development/profile builds, a private adapter should emit the following fields only into profiler/debugger hooks the engine already owns:

- task name/category/lane
- enqueue, start, and finish timestamps
- worker ID
- prerequisite count
- parent/run ID
- cancelled/failed/succeeded status
- queue delay
- execution duration

These events are capture-time evidence, not a public task-diagnostics API, persistent runtime log, report format, or editor panel. Shipping builds may compile out fields that do not serve an existing product-owned profiler integration.

### Features Explicitly Deferred

Do not put these in the first milestone:

- fibers or arbitrary stackful task suspension
- language coroutine integration
- NUMA placement
- platform-specific lock-free wait primitives as public API
- priority inheritance
- distributed cooking
- a completely general heterogeneous CPU/GPU graph
- automatic inference of data hazards from captured C++ objects

They can be revisited only after the renderer has measured evidence that the simpler design is insufficient.

## Cross-Thread Scene Contract

### Stable Identity

Every renderable logical object gets a generational ID:

~~~cpp
struct RenderObjectId
{
    uint32_t index;
    uint32_t generation;
};
~~~

Assets cross the boundary through immutable, reference-counted or registry-pinned handles:

- MeshAssetHandle
- MaterialAssetHandle
- TextureAssetHandle
- SkeletonAssetHandle

No RenderFramePacket field contains Mesh*, GameObject*, Component*, vector storage owned by GameScene, or callbacks into game objects.

### Two Data Streams

Do not deep-copy the whole scene every frame. Publish two related streams.

RenderWorldDelta contains infrequent structural changes:

~~~cpp
struct RenderWorldDelta
{
    SceneGeneration scene;
    SequenceNumber firstSequence;
    SequenceNumber lastSequence;

    Span<RenderProxyCreate> creates;
    Span<RenderProxyDestroy> destroys;
    Span<RenderProxyStaticUpdate> staticUpdates;
    Span<MaterialUpdate> materialUpdates;
    Span<LightStructureUpdate> lightUpdates;
    Span<AssetResidencyCommand> assetCommands;
};
~~~

RenderFrameDynamicData contains current frame values. The following named streams are the design contract; exact column grouping remains evidence-driven:

~~~cpp
struct TransformStream
{
    Span<RenderObjectId> objects;
    Span<Matrix3x4> current;
    Span<Matrix3x4> previous;
};

struct BoundsStream
{
    Span<RenderObjectId> objects;
    Span<Bounds> bounds;
    Span<VisibilityBits> visibility;
};

struct SkinningStream
{
    Span<SkinningRange> instances; // object + matrix offset/count
    Span<Matrix3x4> jointMatrices;
};

struct MorphStream
{
    Span<MorphRange> instances; // object + weight offset/count
    Span<float> weights;
};

struct RenderFrameDynamicData
{
    FrameId frame;
    CameraPacket camera;
    TransformStream transforms;
    BoundsStream bounds;
    SkinningStream skinning;
    MorphStream morph;
    Span<DynamicLightPacket> lights;
};
~~~

The concrete SoA layout should follow measured access patterns. The design point is stable IDs and dense, immutable dynamic arrays, not one copied polymorphic object graph.

### Binding DOD Contract for the Two Streams

The two-stream boundary is a data-oriented transform, not merely a thread-safe DTO layer. Richard Fabian's *Data-Oriented Design* starts from the type, frequency, quantity, shape, probability, producer, consumer, and transformation of real data. Sparkle applies that discipline explicitly:

| Question | `RenderWorldDelta` | `RenderFrameDynamicData` | Persistent `RenderWorld` / GPU scene |
|---|---|---|---|
| Frequency | only committed create/destroy/static/resource changes | once per accepted frame for values actually consumed that frame | retained across frames; updated only by accepted delta/dynamic dirty ranges |
| Shape | bounded operation batches; AoS records are valid when each operation consumes all fields | dense columns or cohesive AoS/AoSoA blocks chosen per pass access; no object graph | stable-slot tables, free lists/generations, dirty bitsets/ranges, packed GPU-facing tables |
| Identity | `RenderObjectId` plus scene/sequence generation | dense row-to-`RenderObjectId` mapping; row order is not durable identity | stable render slot/generation mapped explicitly from `RenderObjectId` |
| Ownership | packet-slot arena, immutable after publication | packet-slot arena, immutable after publication | render coordinator owns mutation and GPU lifetime |
| Consumer | delta apply, residency, proxy/static-table maintenance | view/culling/motion/skinning/light preparation | render preparation, raster/RT instance building, upload/retirement |
| Failure policy | reject gap/stale/duplicate sequence or request full resync | reject wrong scene/frame generation; never partially publish | retain last accepted generation; token-retire replaced storage |

Binding rules:

- GameFramework ECS is the authoritative mutable world-instance source. `RenderWorld` is a deliberately derived, versioned, render-owned projection—not a second gameplay authority. No renderer change flows backward into ECS storage.
- Extraction is an explicit bulk transform over a frozen world epoch. It reads only declared component/resource columns, writes packet-owned non-overlapping ranges, and publishes after deterministic fan-in. It never calls per-entity virtual render functions or chases renderer-owned pointers.
- Split data by consumer and frequency before splitting by C++ type. Camera/view constants, transforms, previous transforms, bounds, visibility, skinning offsets, flat joint matrices, morph offsets/weights, and light data are separate only when their passes use them differently. Cohesive fields that are always read together stay together.
- Variable-length data uses offsets/counts into flat packet arrays. A packet row must not contain an owning vector, allocator, callback, mutex, service, or pointer into GameFramework storage.
- `RenderWorldDelta` is not forced into SoA. Structural operations are relatively cold and normally consume a whole record; compact typed AoS batches can be the better layout. Measure before columnizing them.
- `RenderFrameDynamicData` is not a generic `SoA<T...>` abstraction chosen for appearance. Each named stream documents producer, consumers, cardinality, update frequency, stable key, layout, alignment, and serial/parallel partition unit.
- Renderer application resolves IDs once into stable render slots, then hot renderer loops operate on packed render-owned slot/range data. Repeated hash lookup, sparse ECS join, asset pointer traversal, and per-object polymorphism are forbidden in frame-hot preparation.
- Static mesh/material/skeleton/texture payloads cross as immutable versioned handles and residency commands, never repeated per-frame copies. Dynamic instance values cross separately.
- Dirty tracking exists at the narrowest useful granularity: structural operation, component column, render slot, contiguous range, or GPU page. A single transform change must not rebuild scene-wide arrays or upload unrelated materials/lights.
- Previous-frame/temporal data has one owner and explicit rollover. Do not duplicate current/previous transforms independently in GameFramework, packet construction, RenderWorld, and GPU buffers without a named need and generation rule.
- Sorting, bucketing, compaction, and deduplication use stable keys and task-local outputs followed by deterministic merge. Task completion order never defines render row, draw, light, or upload order.
- DOD success is measured against the replaced path: bytes read/written, allocation count, packet bytes, extraction/apply time, cache misses, bandwidth, branch behavior, dirty/upload bytes, and serial/parallel crossover. “Uses ECS/SoA” is not evidence.

The source boundary is deliberately honest:

| Source precedent | What Sparkle adopts | What Sparkle does **not** claim |
|---|---|---|
| Richard Fabian, *Data-Oriented Design* | design from real data shape/frequency/access and transforms; normalize hot/cold relationships; avoid generic abstraction that obscures the actual stream | that every field must be SoA or every object model must become one universal ECS |
| Epic MassEntity | data-only fragments, query batches, transient views, chunk-aware iteration, deferred composition changes | that Sparkle already needs Mass-scale archetype/chunk storage |
| Epic game/render proxy model | game-owned source and renderer-owned mirror/proxy, explicit dirty propagation, no game-object dereference on render work | that Unreal's class hierarchy or exact proxy API is Sparkle's packet schema |
| NVIDIA Donut `Scene` | persistent material/geometry/instance GPU buffers and distinct structure/transform dirty state | that Donut supplies a general GameFramework ECS or Sparkle's two-stream API |
| AMD Cauldron/Detroit/RDNA guidance | separate API-independent scene transforms from GPU textures/buffers/passes; indexed resource arrays; split vertex streams by pass access; stable ordered batching | that AMD publishes a general game-world ECS matching Sparkle's design |

The inspected NVIDIA and AMD renderer repositories support persistent renderer tables, access-specific streams, indexed resources, and batching, but they do not establish a general game-world ECS. Epic MassEntity and Fabian are the binding references for GameFramework DOD; Epic's proxy model plus the NVIDIA/AMD renderer sources are the binding references for the extraction and renderer side. Where none of those sources supports a proposed abstraction, the plan must stop and request a product/evidence decision rather than inventing it for portfolio value.

### Frame Packet

~~~cpp
struct RenderFramePacket
{
    FrameId frame;
    SceneGeneration scene;
    RenderSettingsGeneration settings;
    RenderWorldDelta worldDelta;
    RenderFrameDynamicData dynamic;
    ViewFamilyPacket views;
    EditorRenderPacket editor;
};
~~~

The main thread builds the packet in a slot-local linear arena. Publishing seals the packet. The render thread may retain only stable handles/IDs or copy required data into render-owned state; it must not retain arena pointers after acknowledging the slot.

### Render-Owned Proxies

RenderWorld applies deltas in sequence order and owns:

- RenderProxy records indexed by RenderObjectId
- stable mesh/material/texture asset handles
- current and previous transforms keyed by stable ID
- bounds and visibility state
- material-to-pipeline classification
- raster instance data
- ray-tracing geometry/instance state
- dirty flags and GPU-scene allocation indices

Game destruction publishes RenderProxyDestroy. The render thread removes the logical proxy at the correct sequence point and retires GPU allocations using completion tokens. It never calls device idle for an ordinary scene change.

### Why Not a Shared Scene Mutex

A shared mutex would:

- leave game object lifetimes coupled to renderer progress
- make the game thread’s update latency depend on render traversal
- allow accidental render access to gameplay-only state
- hide ownership mistakes until load spikes
- make one-frame pipelining largely ineffective

Explicit packets cost design effort, but that effort is exactly the production-engine skill this program intends to demonstrate.

## Renderer Redesign

### Render Thread Ownership

Renderer becomes a thread-safe application facade, while RendererSystemRoot and FramePipeline move behind RenderThread.

Application-facing operations become one of:

- packet publication
- ordered render command publication
- narrow product-owned result reads, such as capture completion or an already-owned capability/memory view
- explicit boundary wait used only by shutdown, resize policy, or tests

Example render commands:

- ResizeSwapchain
- ReplaceShaderPackageGeneration
- ChangeRenderFeatureSettings
- BeginSceneGeneration
- EndSceneGeneration
- RequestCapture
- Shutdown

Each command has a sequence relative to frame packets where ordering matters. A “reload now” method that directly mutates PipelineStateManager from the editor thread is not valid.

Renderer and the RHI are created and destroyed on the render coordinator. Backend APIs should assert the owning thread for resource creation/destruction, queue submission, swapchain operations, and mutable global cache access. Worker-safe recording APIs assert a leased recording context instead.

### Persistent GPU Scene

GpuScene replaces most per-frame BuildRenderSceneGpuData churn.

Persistent allocations:

- static mesh vertex/index data
- ray-tracing hit vertex/index data
- material records
- stable raster/RT instance slots
- light slots where stable identity is useful
- mesh metadata and draw/dispatch arguments

Per-frame or dirty uploads:

- camera/view constants
- current transforms and previous transforms
- changed light parameters
- skinning matrices
- morph weights
- created/destroyed/changed instance records
- dirty material ranges
- TLAS/PTLAS instance updates and related compact metadata

Suggested update flow:

~~~text
Apply RenderWorldDelta
  ├─ allocate/free proxy + GPU-scene slots
  ├─ resolve immutable asset handles
  └─ mark dirty ranges

Apply RenderFrameDynamicData
  ├─ write transform/light/skinning staging ranges
  └─ update previous-frame state by stable RenderObjectId

BuildGpuSceneUpdatePlan
  ├─ coalesce dirty ranges
  ├─ reserve upload arena slices
  └─ emit copy/compute update passes
~~~

Capacity growth creates a replacement buffer, copies live data, switches bindings at a frame boundary, and retires the previous resource by queue completion token. It does not call WaitForIdle.

This design also fixes temporal identity. Previous matrices should not be keyed by the current vector position of a snapshot; they follow a generational RenderObjectId.

### Render Preparation Graph

After the render coordinator applies ordered deltas, it creates a per-frame preparation graph. An initial form:

~~~text
                         Apply frame dynamic data
                                  │
                 ┌────────────────┼────────────────┐
                 ▼                ▼                ▼
          Update transforms   Build lighting   Build skinning
                 │                │                │
                 ├──────┐         │         ┌──────┤
                 ▼      ▼         ▼         ▼      ▼
          Frustum/LOD  RT plan  Shadow plan  Mesh classification
                 │      │         │         │
                 └──────┴────┬────┴─────────┘
                             ▼
                   Sort and merge packets
                             │
                             ▼
                    GPU-scene update plan
                             │
                             ▼
                 Frame-graph setup and compile
~~~

Candidate task outputs must be independent:

- each task writes a private vector/arena slice
- joins merge results in stable key order
- material and pipeline classification reads immutable prewarmed tables
- no task creates RHI objects
- no task mutates PipelineStateManager or TextureCache
- no task calls GameScene

The first production DAG must expose the renderer-heavy cases rather than hide them behind one `Build lighting` or `Mesh classification` node. Its logical products are:

- transformed bounds and previous-transform ranges
- per-view visibility/relevance/LOD results with stable object tie-breaks
- visible-light classifications and compact light records preserving `RenderLightId`
- shadow-view/frustum work and caster lists where the current shadow path consumes them
- skinning/morph output ranges
- raster pass/material eligibility records derived from immutable pipeline/layout tables
- retained view-independent `MeshDraw` data plus dynamic per-view records, using explicit generation invalidation
- stable state buckets and `MeshInstanceBatch` results, with transparent-order constraints
- BLAS build/update inputs and distinct classic TLAS/PTLAS instance plans
- dirty GPU-scene ranges and upload/copy descriptions

Do not invent an Unreal-shaped draw-command class merely to make the list familiar. Start from Sparkle's existing `MeshDraw`, `MeshRenderItem`, `MeshInstanceBatch`, and `MeshInstanceBatchBuilder`; refactor or replace them only where one responsibility, cache lifetime, or immutable recording input is otherwise impossible to state.

Frame-graph setup should initially remain on the render coordinator. Epic’s RDG design similarly separates setup from eligible parallel execute work, and Sparkle’s graph builder is mutable. Later, expensive setup producers may generate declarations privately, but a parallel mutation free-for-all is not a first target.

Frame-graph setup/compile remains serial initially. Only if existing profiler scopes later show repeated compile is a material critical-path cost may Sparkle retain/reuse compiled topology behind a direct FrameGraphKey covering resolution classes, product feature configuration, view count, backend capabilities, and explicit capture modes. Such a cache must replace repeated work, have complete invalidation tests, and remain frame-graph-private; it is not a prerequisite or a generalized cache subsystem.

### Command Recording Plan

The frame-graph compiler currently knows pass order, resource usage, barriers, queues, and cross-queue dependencies. Extend its output with recording groups.

~~~cpp
struct RecordingGroup
{
    RecordingGroupId id;
    RhiQueueType queue;
    RecordingPolicy policy;
    Span<CompiledPassId> passes;
    Span<ResourceState> entryStates;
    Span<ResourceState> exitStates;
    uint32_t estimatedCommandCount;
    uint32_t estimatedRecordingCost;
    SubmissionOrderKey order;
};

enum class RecordingPolicy : uint8_t
{
    SerialRenderThread,
    ParallelSafe,
    ExternalProvider
};
~~~

Execution becomes:

1. render coordinator prewarms pass runtimes and reserves shared frame resources
2. eligible recording groups are submitted as tasks
3. each task leases its own worker recording context
4. each group emits commands using only declared immutable inputs and local allocators
5. all groups for a submission batch join
6. render coordinator emits/coalesces required inter-group barriers where necessary
7. render coordinator submits closed lists/buffers in compiled order
8. existing GPU queue waits/tokens preserve inter-queue dependencies

Parallel completion order must not change GPU submission order.

`RecordingGroup` is a private compiled unit, not automatically one command list, one task, one pass, or one native submission. The execution layer may combine adjacent tiny groups into one recording task, split one measured-heavy group into ordered chunks, aggregate several closed native lists into one submission batch, or submit an early batch to avoid GPU starvation. Those decisions must preserve the same compiled pass/resource order and be observable through existing profiler counters.

The following terms are mandatory in implementation and review:

- **recording group**: compiled CPU recording ownership and state contract
- **recording chunk**: an ordered intra-group range such as draws, instances, shadow views, or RT build inputs
- **submission batch**: ordered closed native command objects supplied to one queue operation
- **translation**: replay of a software RHI command stream into another command representation; not present in the approved Sparkle design
- **aggregation**: deterministic fan-in/order of closed command objects; never native-buffer concatenation

### Choosing Recording Granularity

There are two useful levels.

Pass-level recording:

- good for independent compute/copy passes and moderate raster passes
- maps naturally to frame-graph dependencies
- may be too fine for tiny passes

Intra-pass draw chunking:

- useful for depth, shadow, GBuffer, visibility, and other draw-heavy raster passes
- one setup task partitions a stable sorted draw list
- workers record chunks
- primary submission executes chunks in deterministic order
- backend may use D3D12 direct command lists/bundles and Vulkan primary/secondary command buffers according to measured overhead and render-pass constraints

The compiler should group tiny adjacent passes instead of producing hundreds of microtasks. A measured minimum recording-cost threshold belongs in policy.

Partition by estimated recording cost when draw counts are misleading, retain a serial path below the crossover, and bound lists by both available contexts and native-memory budget. Capture draws/dispatches/build inputs per list, recording time, close/finalization time, list count, submit call count, submit CPU time, first-work availability, GPU gaps, and p95/p99 latency. “More balanced workers” is not sufficient evidence if submission is delayed or the GPU loses performance.

### Pass Parallel-Safety Audit

Parallel-safe is opt-in. Every pass must be audited against:

| Question | Required parallel-safe answer |
|---|---|
| Can GetPassRuntime lazily create a runtime/PSO? | No; prewarmed before task launch |
| Can binding allocate from a shared upload cursor? | No; use worker-local/preassigned slice |
| Can descriptor allocation mutate a shared unsynchronized heap? | No; use local arena or proven concurrent allocator |
| Can execute update a renderer cache? | No |
| Can execute create/destroy an RHI resource? | No |
| Can execute call an external SDK with global context? | Only if that provider documents safety; otherwise serial island |
| Does execute depend on another task’s incidental completion order? | No; dependency must be explicit |
| Are all resources/states declared to the frame graph? | Yes |
| Is output command ordering deterministic? | Yes |

PipelineStateManager currently lazily inserts type-indexed runtime storage and can create shader runtime objects from GetPassRuntime. This is a hard parallel boundary. Compile/prewarm all required pass runtimes before recording tasks, then expose an immutable FramePipelineRuntimeView.

PassBinder currently allocates uniform constants through the shared upload service while recording. Replace that with one of:

- preallocate pass constant slices during frame preparation, or
- give every worker recording context a frame-local upload arena

The first is maximally deterministic; the second is more flexible. Sparkle can support both, but each binding must identify its allocation owner.

External providers such as Streamline, debug capture, presentation, ImGui integration, and readback/capture operations should begin as serial render-thread islands.

### D3D12 Recording Contexts

Native ownership rules:

- a command allocator cannot be reset until its submitted GPU work completes
- a command allocator cannot be used concurrently
- a command list instance cannot be recorded concurrently

Allocate logical contexts by buffered frame × queue × worker, plus coordinator contexts:

~~~text
D3D12FrameRecordingState[frameSlot][queue]
  coordinator allocator/list
  worker contexts
    worker 0 allocator + list slots + upload page + descriptor page
    worker 1 allocator + list slots + upload page + descriptor page
    ...
~~~

One recording task owns one leased list until Close. A worker-local context can recycle list objects while retaining allocator ownership rules. The frame slot resets only after its GPU completion tokens are satisfied.

Avoid a globally locked “get command list” pool on the hot path. O3DE’s per-thread suballocator and Microsoft’s frame-resource sample support the buffered-frame × worker ownership model.

### Vulkan Recording Contexts

Vulkan command pools are externally synchronized. The target is:

~~~text
VulkanFrameRecordingState[frameSlot][queue]
  coordinator command pool
  worker 0 command pool + primary/secondary buffers + descriptor/upload pages
  worker 1 command pool + primary/secondary buffers + descriptor/upload pages
  ...
~~~

A worker records only buffers allocated from its own pool. Pools reset after the frame slot’s GPU work completes. If tasks can migrate between workers, a lease captures the actual worker context at execution time; callers never cache a raw pool by expected worker ID.

Secondary command buffers are appropriate only where inheritance/rendering information and measured driver behavior justify them. Parallel primary buffers can be preferable for independent passes. The frame graph should describe recording groups without exposing one backend’s secondary/bundle mechanism.

### Upload and Descriptor Allocation

Parallel command recording creates allocator pressure. The design should use:

- one large per-frame upload allocation split into worker-local pages
- atomic or locked slow-path page acquisition
- lock-free bump allocation within a worker page
- alignment-aware constant/structured upload slices
- per-worker transient descriptor blocks
- a coordinator-owned overflow assertion and existing allocator/profiler counter path
- GPU-token-based page retirement

Observe through existing allocator/profiler hooks during explicit captures:

- bytes reserved/used/wasted per worker
- slow-path page acquisitions
- descriptor high-water marks
- upload overflow/fallback events

A shared atomic offset may be correct for coarse uploads but becomes a cache-line bottleneck for every draw constant. Local pages make ownership visible and reduce contention.

### Resource State and Barrier Rules

Parallel command lists cannot discover global state by execution timing. FrameGraphCompiler owns:

- initial state for every group
- transitions within a group
- final state for every group
- aliasing barriers
- ownership/queue-family transitions where required
- cross-queue waits and signals
- ordered submission keys

If two passes require an inter-list transition that cannot safely be embedded in either parallel group, the compiler emits a small coordinator barrier list or constrains them to one serial group.

Validation mode should compare the serial plan and parallel plan:

- same pass order
- same declared resource states
- same queue wait graph
- same final external states
- same output image within defined tolerance

### Submission Is Not a Worker Task

Only the render coordinator submits queue work and presents. This gives:

- deterministic order
- one owner for queue timeline values
- simpler device-lost handling
- no competition around swapchain and external providers
- clear CPU traces

The existing backend submission mutexes remain defensive and useful for non-frame operations, but the normal frame path should not depend on multiple workers racing to submit.

### Why Sparkle Should Not Add an RHI Thread Yet

Unreal’s game/render/RHI thread split is appropriate to an engine with a substantial deferred RHI command translation layer and many platforms. Sparkle currently records its RHI abstraction directly into backend command lists/buffers.

Adding another RHI thread now would require a new software command stream:

- encode every RHI call
- copy all captured data safely
- decode it later
- add another lifetime and latency boundary
- debug two command representations

There is no current measurement showing this would outperform direct parallel native recording. The render coordinator plus recording workers is the recommended target. Reconsider an RHI translation thread only if profiles show render-thread submission/translation is a persistent critical path and the new stream has a clear platform benefit.

## Advanced Graphics Feature Preservation Contract

Multithreading is accepted only if it strengthens the renderer-first evidence defined by A/G/H. The task system and data redesign do not authorize feature demotion.

### Product and Experimental Classification

| Capability | Required classification during this program | Multithreading obligation |
|---|---|---|
| D3D12 and Vulkan RHI | Product | Paired ownership, lifetime, validation, and recording milestones |
| Frame graph and typed passes | Product | Remains the only render scheduler and barrier authority |
| Shader compiler/cook/reflection/runtime ABI | Product | Immutable generations and deterministic parallel cook |
| Classic TLAS | Product where backend capability exists | Build/update/trace/lifetime parity preserved |
| PTLAS | Product where backend capability exists | Minimal build/update/trace/lifetime path preserved; no diagnostic scaffolding |
| Native reservoir-based direct lighting | Product renderer feature | Histories, light/material ownership, and deterministic reset preserved |
| Reference path tracing | Must be explicitly labeled progressive/offline reference or debug reference before its threading change | Accumulation/reset and material/light correctness preserved |
| Screenshot/BMP capture | Product editor/tool capability | Nonblocking render/readback/encode ownership, bounded requests |
| Existing temporal/upscaling/denoising/provider paths | Product or developer preview according to existing capability policy | Tagged inputs and frame-order contracts preserved |
| Neural rendering experiments | Research/experimental until a concrete replacement feature exists | No shipping packet/scheduler fields and no runtime ML dependency |

Feature classification is not a new registry or panel. Use the existing feature/capability configuration and concise product documentation. Shipping defaults must not depend on research tasks or research-only packet fields.

### Ray Tracing: BLAS, Classic TLAS, and PTLAS

The persistent GPU scene must support both product acceleration-structure paths rather than forcing them into one abstraction that fits neither.

Shared rules:

- immutable mesh asset generations own geometry identity
- render-owned proxies own instance identity and current/previous transforms
- BLAS creation/update/compaction decisions occur on the render coordinator from immutable plans
- CPU workers may build geometry/instance descriptors and partition inputs, but cannot create or destroy native acceleration structures
- scratch/result resources and build/read dependencies are declared to the frame graph
- copy/compute/graphics queue assignment is chosen from backend capability and measured workload, not from task completion order
- compaction and replacement retire old resources using the queue token that last references them
- asset unload, level change, and shader reload never invalidate an in-flight shader table or acceleration structure

Classic TLAS rules:

- stable instances map to persistent BLAS handles and explicit masks/flags/hit-group data
- update versus rebuild policy remains renderer-owned and backend-capability-aware
- shader table and pipeline generation are immutable for a recording run

PTLAS rules:

- retain the minimal product flow: capability gate, compact descriptor input, backend build/update, resource lifetime, and trace use
- do not add scheduler-specific PTLAS planner layers, future GPU-pack structures, metrics, or debug views
- D3D12 and Vulkan capability differences are explicit; unsupported is a truthful state, not a fallback pretending to be PTLAS
- parallel CPU descriptor preparation must not change partition identity or trace behavior

Acceptance requires, for every supported backend/path:

- initial build and trace
- transform-only update
- add/remove instance
- asset/BLAS replacement
- frames-in-flight destruction
- level reload and shader generation swap
- serial/threaded and serial/parallel-recording image parity

### Native Reservoir Lighting and Reference Path Tracing

The render data redesign must not make research-shaped lighting code own the scene model.

Native reservoir-based direct lighting:

- Sparkle owns light buffers, materials, GBuffer addressing, TLAS selection, histories, passes, and shader scheduling
- stable light/object IDs, not task completion order or transient vector position, define temporal identity
- history reset is explicit for camera cuts, resolution changes, scene generations, material/light topology changes, and feature toggles
- worker tasks may prepare light/alias/reservoir inputs into private slices; deterministic merge defines GPU order

Reference path tracing:

- the mode’s product classification and convergence intent are stated before optimization
- accumulation state remains frame-graph history owned
- sample index, random seed policy, camera cuts, material changes, and scene generations produce deterministic reset behavior
- threading work improves data preparation or recording without adding debug views or pretending the reference path is a shipping real-time integrator

### Temporal, Denoising, Upscaling, and Frame-Generation Signals

The frame packet/view contract must preserve renderer-owned:

- current and previous camera transforms
- jitter and unjittered projection
- motion-vector convention
- depth convention and exposure
- frame and reset indices
- render and display resolution
- history validity
- camera-cut/teleport state

These are versioned values, not pointers into GameFramework or editor state. A one-frame-ahead CPU pipeline must not accidentally add another temporal frame of delay to motion vectors, provider tags, or frame-generation markers. Frame generation and presentation integrations remain serial render-thread/provider islands until their API-specific threading and latency contract is proven.

### Shader ABI and Parallel Cook/Reload

The shader system remains a product-level system throughout the migration.

Required invariants:

- HLSL SM6 remains the primary authored path
- Slang and the existing cross-target path remain available for appropriate shaders and future neural experiments
- DXIL and SPIR-V package targets, include-closure identity, reflection, parameter layouts, specialization constants, feature profiles, and runtime validation remain deterministic
- a cooked package generation is immutable after publication
- a frame/recording run sees one coherent shader-package, binding-layout, and pipeline generation
- worker command recording reads a prewarmed immutable runtime view
- compiler/backend sessions are thread-confined or protected according to their actual API contract
- parallel cook completion order never affects package IDs, shader blob IDs, registry ordering, or publication
- failure keeps the previous complete generation active

The task program must not add generated binding wrappers, another shader graph, default debug bundles, or stats artifacts. Existing parameter-struct verification remains the authority.

Neural readiness is preserved by keeping the ABI able to describe structured/tensor-like buffers and textures, precision/layout metadata, specialization constants, and provider resource contracts. This does not authorize PyTorch/ONNX runtimes, training workflows, CUDA/HIP backends, or generic CPU/GPU task-graph unification.

### Vendor-Neutral Provider Threading Contract

The renderer owns integration policy. A provider may supply algorithms or API calls, but it does not own Sparkle’s scene, frame graph, resources, or scheduler.

Each provider integration must define:

- compile/dependency availability
- backend/hardware capability
- user-enabled state
- operational state for the current frame
- explicit failure/fallback state
- owning thread for instance creation, evaluation, and destruction
- whether any API entry point is safe for parallel recording

Provider input is a narrow tagged set such as depth, motion vectors, exposure, reactive/transparency masks, history, jitter, frame index, camera state, input/output extents, and renderer-owned resources. Native handles remain inside the provider/backend bridge.

Default rule:

- provider instances and callbacks are render-thread confined
- provider passes are SerialRenderThread or ExternalProvider recording groups
- background SDK work uses the background/IO lane only when the SDK documents the callback/lifetime contract
- provider completion is marshalled back as an ordered render command/event
- no provider callback enters GameScene, editor UI, task scheduler internals, or queue submission directly

ParallelSafe may be granted only by a provider-specific audit. “The SDK is internally threaded” is not evidence that its public context is reentrant.

### Screenshot/BMP Capture Ownership

Capture remains a low-cost product editor/tool capability:

~~~text
Editor/main RequestCapture
        │ sequenced render command
        ▼
Frame graph declares copy/readback after selected product
        │ GPU completion token
        ▼
Render coordinator publishes owned readback payload
        │ bounded background task
        ▼
Encode/write BMP or requested owned format
        │
        ▼
Narrow completion result to editor/tool
~~~

Rules:

- no device-idle wait
- no live framebuffer or editor pointer crosses threads
- request count and readback memory are bounded
- resize/scene generation identifies or cancels stale requests
- encoding/file I/O never occupies a frame worker
- capture does not create a permanent report system, panel, or default artifact

### Debugger and Profiler Preservation

Threading must preserve:

- PIX/RenderDoc/Nsight-compatible GPU markers
- backend debug layers and fatal API result handling
- native object names
- timestamp queries and queue correlation
- shader debugging/disassembly workflow
- screenshot capture

CPU thread names and existing profiler-visible scopes are added or consolidated only where they replace ambiguity. No bespoke task debugger, frame report, or validation UI is part of this program.

### CPU/GPU Non-Regression Gate

A lower renderer CPU time is not a success if it causes:

- more GPU barriers or queue bubbles without a justified dependency change
- worse transient aliasing or memory budget pressure
- materially higher descriptor occupancy
- unnecessary pipeline/descriptor rebinding
- command-list fragmentation that increases driver or GPU overhead
- duplicate uploads or loss of residency
- worse BLAS/TLAS/PTLAS build/update behavior
- shader permutation or pipeline-count growth

Use existing frame-graph, allocator, capability, timestamp, and debugger data to compare serial and parallel modes. Broad workload reporting remains a late consolidation step after the feature paths are stable.

### CPU Task Parallelism Does Not Replace GPU Architecture Work

The program must keep three optimization decisions separate:

1. CPU data preparation and command recording.
2. GPU queue scheduling and overlap.
3. Shader/kernel execution efficiency.

Persistent GPU-scene layouts are chosen from shader access patterns: alignment, coalescing, hot/cold separation, bandwidth, descriptor access, and update frequency. They are not chosen solely because a CPU parallel_for can fill them conveniently.

Likewise:

- async compute is assigned only when resource dependencies and measured GPU overlap justify it
- CPU task completion does not choose GPU queue order
- more command lists do not justify more barriers or queue transfers
- wave/subgroup use, LDS/shared memory, register pressure, occupancy, divergence, and shader permutation cost remain shader/pass concerns
- parallel CPU preparation must preserve compact GPU inputs rather than emit one allocation or descriptor per task

Portfolio evidence should explain which improvement came from CPU overlap, which came from reduced upload/data churn, and which—if any—came from GPU queue or shader changes. It must not attribute all gains to “multithreading.”

## GameFramework Integration

### First Principle

GameFramework parallelism is not a prerequisite for proving the renderer packet boundary, but it **is required for this complete engine-wide program** and lands before the render-thread migration in Stages 2A/2B. GameScene remains structurally main-thread owned while tasks operate on immutable inputs, task-private results, or explicitly partitioned output arrays.

Initial useful jobs:

- animation evaluation per independent skeleton/instance
- controller phases where dependencies are declared
- skinning matrix generation
- morph weight evaluation
- bounds updates
- transform hierarchy subtrees after parent dependencies
- render packet extraction by typed component query/dense array

### Phase Graph

The high-level phase shape is:

~~~text
Input + commands
       │
       ▼
Pre-animation controllers
       │
       ├──────────────┐
       ▼              ▼
Animation jobs    Independent simulation jobs
       │              │
       ▼              │
Morph/skinning        │
       └───────┬──────┘
               ▼
Post-animation controllers
               │
               ▼
Transform/bounds propagation
               │
               ▼
Render packet extraction
~~~

Do not run arbitrary components concurrently because they happen to be in a vector. Components must declare read/write domains or be scheduled by known phase ownership. Begin with animation instances and extraction ranges whose independence is easy to prove.

### Previous-Frame Values

The game may publish current transforms while the render world owns render-history transforms. This avoids copying renderer temporal state back into GameFramework. Camera cut/teleport flags explicitly reset history. Stable IDs rather than snapshot array indices pair current and previous data.

### Adversarial Finding: the Current Scene API Is Not a Parallel Contract

The phase sketch above is a direction, not sufficient architecture. The existing `GameScene` has useful subsystem boundaries, but its mutation model is serial and implicit:

- `GameSceneController::Update(GameScene&, ...)` grants every controller write access to the whole scene
- the camera controller stores a raw active-camera pointer that can change on scene load
- the Showcase controller stores mesh vector indices and writes mesh transforms directly
- editor code also reads and writes those containers directly during UI construction
- `Transform` and camera direction have mutable lazy caches written from `const` getters
- the sequential `Entity` component container has no repository consumer; `Component` remains a base used by camera/mesh types, but its raw owner pointer and virtual update/render hooks are not a system-scheduling contract

Nothing in that model proves which work is independent. A pool, mutex around `GameScene`, or `ParallelFor` over components would hide hazards rather than create a production architecture.

The target **is a bounded ECS conversion**, but not an immediate archetype/chunk framework. `GameScene` remains the authoritative world facade while a private ECS kernel owns entity identity and typed component pools. Systems query data and declare access; structural commands change composition only at commit. Delete the unused owning `Entity` class. Split the current polymorphic `Component` base from runtime data: migrate hot camera/mesh/transform state into data components, while resource-owning or editor-facing behavior remains in explicit services/facades until its consumers are converted.

This is a pivotal architecture change with a controlled scope. Sparkle gains the normal engine concepts—entities, components, systems, queries, data-oriented iteration, and deferred structural changes—without claiming to reproduce Unreal Mass, Unity DOTS, or a general-purpose ECS SDK.

### Binding World Ownership Model

The main/game thread remains the sole authority for structural world state:

- create/destroy scene objects
- attach/detach loaded scene packages
- change active level and scene generation
- apply editor transactions
- register/unregister systems
- publish the committed world generation

Worker tasks may:

- read immutable input views for the current update run
- write only task-private results or an exclusively partitioned output range
- enqueue typed commands for the next commit
- never retain a mutable subsystem reference after their task returns
- never call existing `Event::Broadcast`, ImGui, renderer, RHI, or platform-window APIs

The renderer consumes only `RenderWorldDelta` and `RenderFrameDynamicData`. The editor consumes only a published `WorldReadView` plus command results. Neither consumer receives a live `GameScene&` across a concurrent boundary.

### Stable World Identity Before Parallelism

Vector positions are storage locations, not object identity. Introduce a compact generational `EntityId` before editor snapshots, command buffers, or parallel evaluation:

~~~cpp
struct EntityId
{
    uint32_t slot;
    uint32_t generation;
};
~~~

Rules:

- camera, light, mesh instance, and future gameplay objects exposed outside their owner container use stable IDs
- an entity is only this opaque identity plus its component composition; it owns no virtual update/render behavior
- dense arrays may move objects internally; slot-to-dense indirection is owner-private
- a destroyed slot increments its generation before reuse
- selection, editor commands, animation targets, change journals, and render extraction never use a raw vector index as durable identity
- immutable mesh/material/texture/skeleton asset handles remain separate from world-instance IDs
- `EntityId` is not automatically `RenderObjectId`; the extraction boundary owns the explicit mapping

This gives the ECS, editor, load pipeline, render world, and future spatial queries one consistent stale-handle rule. `EntityId` is GameFramework's world identity; only explicit extraction creates a separate renderer identity.

### Bounded Sparkle ECS Kernel

The ECS is private GameFramework runtime infrastructure, not a new top-level product/module:

~~~text
GameScene / World facade
        │ owner-thread structural API
        ▼
EntityRegistry ─────────────► EntityId allocator + generations
        │
        ├── ComponentStorage<LocalTransform>
        ├── ComponentStorage<WorldTransform>
        ├── ComponentStorage<MeshInstance>
        ├── ComponentStorage<Camera>
        ├── ComponentStorage<Light>
        ├── ComponentStorage<AnimationState>
        └── ComponentStorage<EditorMetadata> ...
        │
        ▼ typed Query<const A, B...> + declared access
GameSystemGraph ─────────────► SparkleTasks range jobs
        │
        ▼
task-local EntityCommandBuffers ──► deterministic owner commit
~~~

The minimum kernel owns:

- versioned entity allocation, validation, destruction, and reuse
- one type-erased registry of component storages, with typed access at call sites
- `ComponentStorage<T>` using sparse entity-slot lookup plus dense entity/component arrays
- add/remove/get/contains operations restricted by the structural epoch
- const and writable query construction with explicit included/excluded component sets
- stable component schema/type IDs for serialization and editor commands, separate from compiler-local C++ type IDs
- deterministic bulk creation and command playback
- storage/version counters for stale-view assertions and change tracking

It does **not** initially own scripting, reflection-driven behavior, networking replication, automatic serialization of arbitrary C++ types, prefabs, archetype chunks, entity relationships, or a public plugin API.

### Sparse-Set Storage and DOD Rules

Each component type has its own packed storage:

~~~cpp
template<class T>
struct ComponentStorage
{
    Vector<uint32_t> sparse;      // entity slot -> dense index or invalid
    Vector<EntityId> denseEntity;
    Vector<T> denseComponent;
};
~~~

This provides contiguous iteration for a single hot component and O(1)-style entity lookup while allowing entities to have arbitrary component combinations. Removing a component may swap the last dense element into the hole and update sparse lookup; therefore dense positions never escape a structural epoch.

DOD rules:

- components contain runtime data, stable handles, and small values—not owning `unique_ptr<Mesh>`, callbacks, mutexes, or subsystem services
- immutable mesh/texture/material/skeleton/animation resources remain in asset registries; components store handles and instance state
- hot and cold data are separate components rather than one convenient object (`WorldTransform` versus `EditorMetadata`, for example)
- AoS versus SoA is chosen per measured access pattern; ECS does not mean blindly splitting every vector field
- component arrays use reusable/paged allocation and alignment appropriate to their hot loops
- task partitions write non-overlapping dense ranges; partition boundaries avoid multiple workers updating the same cache line where practical
- query and storage versions assert if structural mutation invalidates an active view
- systems never retain component references, query iterators, or dense indices across commit

### Initial Component Model

The first conversion maps existing useful data rather than inventing gameplay features:

| Data component | Replaces/extracts from | Layout and ownership intent |
|---|---|---|
| `LocalTransform` | mutable `Transform` inside camera/mesh objects | compact TRS mutation input |
| `WorldTransform` | lazy world/inverse cache | explicitly evaluated matrices; immutable after phase |
| `WorldBounds` | planned transform/mesh bounds extraction | add only when bounds are exposed for culling, picking, or extraction; then keep hot query data |
| `MeshInstance` | `MeshComponent` instance fields | asset/material/skeleton handles, mesh kind, group/source identity; no owned mesh resource |
| `Visibility` | base `Component::m_visible` | small independently queried/tag-like state |
| `Camera` | `CameraComponent` state | projection/movement-facing values; derived direction/matrices evaluated explicitly |
| `Light` | `SceneLightDesc` entries | packed light data separated from editor labels |
| `AnimationState` | clip playback state | clip/skeleton handles, time, speed, loop/pause flags |
| `MorphState` / `SkinningState` | mutable skeletal mesh/pose outputs | instance-owned handles/offsets into committed output buffers |
| `Name` / `EditorMetadata` | outliner labels and authoring-only data | cold storage excluded from frame-critical queries |

`Mesh` asset data, cooked manifests, material definitions, skeleton definitions, textures, and shader packages are not ECS components. They are immutable/versioned resources referenced by handles. This prevents entity movement or component compaction from moving heavyweight resources.

The current `SceneCameras`, `SceneMeshes`, `SceneLighting`, and animation containers may act as compatibility facades over the registry during migration, but the final runtime source of instance truth is component storage. A facade must not keep a second mutable copy.

### Authoring, Loading, and Runtime ECS Boundary

Cooked/editor scene data should not serialize raw component memory or compiler type IDs. The load pipeline translates existing `LevelDesc` and `SceneAssetPayload` data into deterministic entity blueprints:

~~~text
LevelDesc + cooked SceneAssetPayload
             │ explicit translators + stable component schema versions
             ▼
SceneLoadPackage
  EntityBlueprint[]
    source identity / optional authored GUID
    component records (Transform, MeshInstance, Camera, Light, ...)
             │ owner structural commit
             ▼
EntityId + typed runtime component pools
~~~

Rules:

- authored/source identity is distinct from ephemeral runtime `EntityId`; loading maps it deterministically
- component file schemas have explicit stable IDs and versions; runtime C++ layout may change independently
- translators are explicit for the small owned component set, not a generic reflection/serialization framework
- editor undo/redo and save operate on semantic component values through commands/read views
- scene-package commit bulk-creates entities and inserts sorted component batches to reduce allocation and improve dense layout
- runtime-only components and derived outputs such as `WorldTransform` or skinning buffers are rebuilt, not serialized as authoring truth
- unknown/newer required component schema rejects the package before mutating the active world; optional data follows an explicit compatibility policy

This makes the ECS useful to level loading, editor authoring, and renderer extraction rather than an isolated simulation container.

### Queries, Access Declarations, and Jobs

A system declares both its component query and non-ECS resources:

~~~cpp
using MotionQuery = Query<
    Read<MovementIntent>,
    Read<MotionParameters>,
    Write<LocalTransform>>;

struct MotionSystemDesc
{
    Phase phase = Phase::Simulation;
    MotionQuery query;
    ResourceReads<InputSnapshot> resources;
};
~~~

Query rules:

- single-component queries iterate that component's dense array directly
- multi-component queries lead with the smallest included pool and resolve other components through sparse lookup
- const/read versus mutable/write access is represented in the query type/descriptor and feeds system hazard analysis
- a query is created after structural commit and is valid only for that frozen update epoch
- the graph selects a serial threshold and partitions the leading dense range into coarse stable chunks
- each range task receives a view bounded to its partition; it cannot perform structural mutation
- systems with disjoint write component sets and compatible resources may overlap
- two systems writing the same component type require a declared dependency or separate phases
- deterministic results use entity/system/partition keys, never task completion order

The ECS query does not schedule work by itself. `GameSystemGraph` compiles access hazards and prerequisites; SparkleTasks executes the resulting ranges. This separation proves understanding of all three layers rather than calling the ECS “the job system.”

### Structural Epochs

World update alternates between two explicit states:

~~~text
StructuralCommit
  create/destroy entities; add/remove components; compact stores; build query plans
        │
        ▼
StructureFrozen
  systems read/write declared component ranges in parallel; record commands only
        │
        ▼
Deterministic command merge ──► next StructuralCommit
~~~

Direct add/remove/create/destroy during `StructureFrozen` is a development assertion. Task-local command buffers may use temporary entity tokens scoped to that buffer; deterministic playback remaps them to real `EntityId` values. This follows the Mass/Unity command-buffer precedent and makes storage movement safe.

### Why Sparse Sets First, Not Archetype Chunks

| Choice | Benefits | Costs | Decision |
|---|---|---|---|
| Polymorphic object/component vectors | simple object ownership | pointer chasing, weak query model, unsafe parallel behavior, no packed hot iteration | replace |
| Sparse-set component pools | compact implementation, packed per-component iteration, flexible composition, natural migration from current subsystem arrays | multi-component queries perform sparse lookups; related component streams are not automatically co-located | **initial Sparkle ECS** |
| Archetype/chunk storage | excellent multi-component locality and chunk-level job partitioning for stable compositions | entity moves on composition change, fragmentation, chunk allocator/query complexity, harder editor mutation and migration | evidence-gated future option |
| Import EnTT directly | mature C++ implementation and tests | external API/lifetime semantics become foundational; less ownership-learning evidence; custom graph/editor serialization still required | reference, not initial dependency |

Archetypes are reconsidered only if captures show multi-component sparse joins or cache misses are material after avoidable work is removed, entity counts/compositions justify chunks, and a prototype wins representative animation/transform/render-extraction workloads. The upgrade may be per-storage/query family; the public `EntityId`, commands, systems, and editor/render seams must not change.

### ECS-Aware World System Graph

Replace arbitrary controller execution with a small registry of product-owned `GameSystem` descriptors. A descriptor declares:

- stable system ID/name
- update phase
- explicit prerequisites where phase order is insufficient
- typed ECS query with component read/write access
- coarse read/write declarations for non-ECS resources such as input, asset catalogs, output arenas, and publication services
- execution policy: serial owner, parallel-for ranges, or task body
- determinism key and enabled state

Initial access maps directly to systems Sparkle already owns; it is not field-level reflection:

| Component/resource domain | Current owner/data | Initial writers | Initial readers |
|---|---|---|---|
| Input intent | input/camera controller state | input collection on main | camera movement system |
| `Camera`, `LocalTransform` | `SceneCameras` | camera movement/editor command | transform evaluation, editor view |
| `AnimationState` | `SceneAnimations::PlaybackState` | playback advance | pose and morph sampling |
| Skeleton definitions | `SceneSkeletons` immutable generation | scene-package commit only | pose evaluation |
| Pose outputs | committed pose buffer | pose evaluation merge | skinning/render extraction |
| Morph outputs | committed morph buffer | morph evaluation merge | morph application/render extraction |
| `LocalTransform` | camera/mesh transform state | movement/editor/animation systems | transform evaluation |
| `WorldTransform`, `WorldBounds` | explicit component pools | transform/bounds evaluation | extraction, editor, future queries |
| `Light`, sky/material resources | scene ECS plus resource registries | editor/world commands | extraction and editor read view |
| ECS structure | entity registry, component compositions and generations | structural commit only | query build, graph input capture, publication |

Graph compilation adds an edge for declared prerequisites and for component/resource hazards. Read/read work may overlap; write/read, read/write, and write/write access to the same component/resource is ordered. Ambiguous or legacy work runs on the serial-owner lane. Development builds reject cycles, undeclared writes detected by guarded test views, duplicate system IDs, and a system whose declared access is unavailable in its phase.

Do not infer hazards from captured C++ pointers. The declarations are reviewed system contracts and are tested against serial equivalence.

### Engine Update and Publication Phases

The concrete target graph is:

~~~text
Pump platform/input on main
          │
          ▼
Apply queued editor/game commands + commit prior structural changes
          │                         sole structural mutation point
          ▼
Capture immutable UpdateInputs and compile/instantiate system run
          │
          ├───────────────┬────────────────────┐
          ▼               ▼                    ▼
 Camera/movement     Playback advance     Independent simulation systems
          │               │                    │
          │        ┌──────┴────────┐           │
          │        ▼               ▼           │
          │   Pose evaluation   Morph sampling │
          │        │               │           │
          └────────┴───────┬───────┴───────────┘
                           ▼
              Deterministic result merge
                           │
                           ▼
               Transform + bounds evaluation
                           │
                           ▼
           Commit deferred WorldCommandBuffers
                           │
               ┌───────────┴────────────┐
               ▼                        ▼
       Publish WorldReadView      Build change journal
               │                        │
               └───────────┬────────────┘
                           ▼
                Extract render packet/delta
~~~

The first implementation executes this graph through the serial executor. Only after byte/state equivalence tests pass are individual nodes enabled for workers. The graph is rebuilt only when the registered system topology changes; per-frame run data is bound without reallocating its topology.

GameFramework and renderer preparation use the same frame executor, not separate gameplay and rendering pools. With a one-frame render pipeline, workers may execute game-system tasks for frame N while render preparation/recording tasks for frame N-1 are ready. The scheduler balances those ready tasks; the main and render coordinators retain their distinct commit/submission ownership. Frame-run priority may reduce latency but cannot encode dependencies. Each coordinator joins only its own required fan-in at a bounded host boundary, and no worker blocks waiting for the other frame graph.

If simultaneous game/render fan-out causes destructive contention, tune task grain and per-run concurrency limits from captures. Do not reserve permanent worker subsets per subsystem unless repeated measurements on target core counts prove that shared stealing violates latency budgets.

### Narrow System Views, Not Mutable `GameScene&`

A system function receives only the data promised by its descriptor:

~~~cpp
struct PoseEvaluationInputs
{
    Span<const AnimationClip> clips;
    Span<const PlaybackSample> playback;
    SkeletonGenerationView skeletons;
};

struct PoseEvaluationOutputs
{
    Span<PoseOutputSlot> exclusiveSlots;
};
~~~

The camera movement system receives input intent and one camera write target, not meshes, materials, or the level manager. The Showcase motion becomes a named movement system over a stable-ID target list and an exclusive transform-command/result range. This makes its PTLAS demonstration a real parallel-system example rather than project code reaching into scene containers.

Provide a temporary `LegacyControllerSystem` adapter only if required to keep the repository runnable during migration. It is main-thread-only, declares a write to all world domains, cannot overlap anything, and has a dated deletion gate. New code cannot register through it.

### Deferred World Mutation and Deterministic Command Buffers

Workers cannot safely resize component stores, invalidate handles, invoke lifecycle callbacks, or change the active level. ECS systems use a task-local `EntityCommandBuffer` for entity/component/value operations. A narrow `WorldCommandBuffer` envelopes those batches together with non-ECS level, asset-attachment, camera-history, and service commands for the same owner commit. The distinction keeps ECS reusable inside GameFramework without pretending every world service is a component.

Typed records include:

- set local transform / visibility / light / sky / material variant
- create or destroy an entity, including temporary command-buffer entity tokens
- add, remove, or replace a data component
- attach an accepted scene-load package
- request active camera or level transition
- mark camera cut/teleport/history discontinuity

Each command carries source system ID, stable target ID, expected target generation, and a deterministic local sequence. Per-task buffers are merged by `(phase, systemOrder, partitionIndex, localSequence)`, never task completion order.

Conflict policy is explicit:

- two writes to an exclusive domain in the same phase are a graph/declaration error
- deliberate last-writer behavior must be represented by a declared dependency, not incidental order
- a stale target rejects the command and returns a typed result to the requesting owner
- create/destroy and container reallocation occur only in commit
- add/remove component commands invalidate composition/query versions only at commit, never during system iteration
- callbacks/events caused by commit are queued for an owner-thread dispatch phase after invariants are restored

Editor transactions produce the same typed world commands as runtime systems. Undo/redo stores inverse semantic commands or before/after values keyed by stable ID; it never stores a pointer into a scene vector.

### World Change Journal and Immutable Read View

The commit phase writes one sequenced change journal. It is the authoritative source for:

- incremental `RenderWorldDelta` extraction
- rebuilding only dirty portions of the editor outliner/read model
- save-dirty tracking and level serialization
- future streaming/spatial-index updates
- deterministic replay tests

Do not add a general event-sourcing framework. Keep a bounded, typed journal for one or a few consumer generations, and require each consumer to acknowledge its sequence. If a consumer falls behind the retained window, it requests a full immutable baseline and resumes from its sequence.

`WorldReadView` is an immutable publication containing only editor/game query data currently needed: stable ID, type, name/label metadata, visibility, local/world transform, camera/light/sky properties, asset handle, and generation/sequence. It owns or references generation-pinned immutable storage. Publication uses the same release/acquire discipline as frame packets; reclamation occurs only after readers release the generation.

This is a bounded read-copy-publish pattern, not a second mutable world. It replaces the editor's live `GameScene*` traversal and avoids a shared scene mutex.

### Transform Contract

The current lazy `Transform::RebuildWorldIfNeeded() const` mutates cache state during a nominal read. Two readers after a mutation can race, and worker extraction cannot prove read-only behavior.

Required redesign:

- local TRS is mutation input owned by the world/commit path
- world matrix, inverse transpose, and bounds are explicit evaluated outputs for a world generation
- read views and render extraction consume precomputed immutable outputs; getters do not lazily write
- dirty local transforms are compacted into stable work ranges
- each output slot has one writer in an evaluation run

Sparkle has no transform hierarchy today. Do not build a hierarchy scheduler merely for a portfolio checkbox. When parent/child data becomes a real scene requirement, validate acyclicity at commit, build parent-before-child depth ranges/topological order, and run nodes at the same depth in parallel. Parent transforms are prerequisites; subtree tasks are permitted only when they write disjoint descendant ranges.

### First GameFramework Parallel Workload: Animation to Morph/Skinning

The existing path is a strong first teaching workload because it already contains real computation and clear boundaries:

1. advance playback state on the owner or in exclusively indexed slots
2. resolve clip-to-skeleton targets once per accepted asset generation, not by scanning every skeleton every frame
3. allocate pose/morph output offsets before fan-out
4. evaluate independent animation instances into private output slots
5. compose each skeleton parent-before-child inside its task; large skeletons may later use depth batches only if measured
6. merge pose and morph records by stable animation/target key
7. apply morph results to per-instance state, not by nested scanning and mutating shared mesh asset objects
8. publish immutable skinning/morph data for render extraction

Grain policy starts at one animation instance or a coarse contiguous range. Small counts remain serial. Memory comes from per-run arenas or reusable output buffers; there is no heap vector construction per joint/task in the steady-state path.

This single path demonstrates data decomposition, exclusive writes, DAG dependencies, nested hierarchy ordering, deterministic merge, allocator discipline, serial thresholds, and renderer publication without inventing an unrelated demo subsystem.

### Previous-Frame Values: Binding Rule

The game publishes current committed transforms and explicit camera-cut/teleport flags. The render world owns render-history transforms, previous skinning data, and history validity because render consumption may lag the game by zero or one frame. Stable IDs, scene generation, and frame sequence—not snapshot array positions—pair values. The editor read view does not become the owner of temporal renderer state.

### Systems Deliberately Not Added for the Multithreading Showcase

Do not add physics, navigation, AI, audio mixing, networking, scripting, world partition, a second generic ECS framework, immediate archetype chunks, or a spatial BVH merely to increase the thread count. The bounded ECS exists to improve current scene/update/editor/render data flow; additional subsystems and storage sophistication become candidates only when a real Sparkle project needs them.

A world spatial index is a good *future* consumer of committed transform/bounds deltas for editor picking, gameplay ray/proximity queries, and streaming. Defer it until at least two of those consumers exist. When justified, build/refit task-private nodes and publish an immutable query generation; never share the renderer's mutable culling/acceleration structures with GameFramework.

## Runtime Asset Loading and Residency

The current frame path must not replace synchronous texture/mesh work with worker tasks that the render thread immediately waits on. Runtime assets use an explicit state machine:

~~~text
Unloaded
   │ request + immutable asset ID
   ▼
Reading              blocking-I/O lane
   ▼
Decoding/Validating  bounded background lane
   ▼
ReadyForUpload       immutable cooked payload publication
   ▼
Uploading            render coordinator + frame-graph copy work
   ▼
Resident             only after GPU completion token
   │
   ▼
Evicting/Retired     budget policy + last-use token
~~~

Failed is an explicit terminal state for a generation and retains a useful diagnostic at the tool/editor boundary without producing routine runtime logs.

Rules:

- GameFramework and frame packets carry stable asset IDs/handles, not decoded object pointers.
- File reads and decompression never run on frame workers.
- RHI resources are created and uploads scheduled by the render coordinator.
- Copy-queue work and first graphics/compute use are connected through existing frame-graph/RHI queue tokens.
- A proxy becomes resident only after upload completion; until then it uses an existing product fallback or is omitted according to feature policy.
- Cancellation and level-generation changes can discard unpublished CPU payloads; uploaded resources retire by GPU token.
- CPU decode concurrency and pending upload bytes are bounded by memory budgets.
- Persistent GPU-scene allocation consults existing allocator/budget information before growth or eviction.
- No worker waits for residency. Readiness produces an event/ordered command and affects a later frame.

This same flow applies to mesh, texture, material, animation, and shader-package generations with type-specific decode/upload stages.

### Transactional Asynchronous Level and Scene Loading

The current `LevelManager` path is synchronous and destructive: it broadcasts unload events, clears `GameScene`, then reads the registry/manifests and assembles payloads. If loading fails, the previous playable scene has already been discarded. `SceneAssetManager` also stores loaded IDs and lazily mutates its registry state, so calling its current methods concurrently would race.

Replace that path with a `SceneLoadOperation` in the world/editor-document task scope:

~~~text
Request(level name, request generation)
          │ main thread resolves immutable LevelDesc/catalog entry
          ▼
Read registry + scene manifests                 BlockingIo lane
          │
          ├────────────┬────────────┬──────────────┐
          ▼            ▼            ▼              ▼
 Validate metadata  Decode meshes  Materials   Animations/skeletons
          │            │            │              │
          └────────────┴──────┬─────┴──────────────┘
                              ▼
            Deterministic assemble + cross-reference validation
                              │
                              ▼
               Immutable SceneLoadPackage generation
                              │ completion to main owner
                              ▼
     Validate request/document generation + atomically commit world commands
                              │
                ┌─────────────┴─────────────┐
                ▼                           ▼
       publish new scene              reject/cancel/fail
       retire old generation          keep old scene active
~~~

Binding rules:

- registry/catalog content is loaded into an immutable generation before workers resolve IDs
- a load operation owns independent loader/importer contexts; it never shares the current mutable `SceneAssetPayload`
- each scene asset produces a task-local partial package with local indices/handles
- the deterministic join assigns final material/mesh/skeleton/object IDs by catalog key, never completion order
- cross-asset references are validated before publication
- CPU and pending-upload bytes have weighted limits; a large scene cannot enqueue unbounded decoded payloads
- cancellation checks occur between read/decode/build stages, but published/GPU-enqueued assets use normal generation retirement
- a newer level request cancels the older unpublished request; stale completion cannot win the commit race
- progress is an immutable/coalesced result marshalled to the editor/main owner, not a worker callback into UI
- `OnLevelWillLoad`, changed, and failed notifications are owner-thread lifecycle results after state transitions; existing `Event` remains thread-affine or is replaced by a typed owner queue
- old scene and renderer generation remain usable until the new package passes validation and its commit begins

This is a deliberately useful new subsystem: it improves runtime level transitions, editor opening/reloading, automated level validation, and future streaming. It demonstrates blocking-I/O separation, fan-out/fan-in, cancellation, failure containment, deterministic merge, immutable publication, and safe owner-thread commit in one portfolio feature.

Do not initially support partially interactive worlds, background UObject-style construction, or world partition. The first contract is whole-package preparation plus atomic commit; streaming cells can reuse it later if the content scale justifies them.

## Editor Integration

### Editor/Main and Render Separation

The editor main thread owns:

- ImGui NewFrame and editor UI state
- scene/property transactions
- asset browser state
- viewport requests and input focus
- tool process controls

The render thread owns:

- viewport render targets
- frame graph and passes
- renderer feature settings after sequenced publication
- ImGui GPU buffers, pipeline, and draw recording
- presentation

The editor sends versioned commands and packet data. It may read only narrow immutable results required by an existing product workflow, such as capture completion, viewport product generation, or an already-owned capability/memory view. Task queues, pass internals, worker state, and cache observations do not become a broad RenderDiagnosticsSnapshot API.

### ImGui Data

ImDrawData points at transient ImGui-owned lists that are reused by the next UI frame. The editor must not hand those pointers to a lagging render thread.

At EndFrame, either:

- clone ImDrawList output into the frame packet, or
- convert it into a compact engine-owned UiDrawPacket with copied vertices, indices, texture handles, clip rectangles, and commands

The render thread consumes that immutable copy. Texture IDs become stable renderer texture handles, not arbitrary editor pointers.

### Viewport Publication

Editor viewport results should be published as:

- stable viewport product ID
- image/descriptor generation
- dimensions and color-space metadata
- frame produced
- lifetime token valid until the editor releases or a defined number of frames retire

The editor must not query live renderer caches for an image pointer while the render thread resizes or replaces it.

### Shader Recook and Reload

Replace the one-off std::async path with:

1. blocking-I/O/background task launches and monitors ShaderCompiler
2. compiler writes a complete new publication generation
3. editor validates metadata and queues ReplaceShaderPackageGeneration
4. render coordinator validates and materializes the new immutable package/runtime set
5. swap occurs at a safe frame boundary
6. old PSOs/layouts/packages retire through GPU completion tokens
7. failure leaves the previous generation active

Routine shader reload must no longer call renderer.WaitForIdle. Device idle remains acceptable for shutdown, unrecoverable backend reinitialization, or an explicitly documented rare debug path.

### Level Changes

Current direct level-event subscriptions and cache clearing become ordered data:

~~~text
BeginSceneGeneration(newGeneration)
  create/update deltas for new scene
  frame packets tagged with new generation
EndSceneGeneration(oldGeneration)
~~~

The render thread rejects stale deltas, removes old proxies, and retires their GPU resources. Main-thread level destruction is no longer coupled to an RHI idle wait.

### Adversarial Finding: Current Panels Are Live Scene Mutators

Today `UI` and scene panels retain `GameScene*`. Outliner entries are reconstructed from live vectors; `SceneObjectSelection` stores a type plus vector index; inspectors mutate `Transform`, `SceneLighting`, `SceneSky`, cameras, visibility, and material variants directly while drawing ImGui. That is safe only because the whole path is serial.

Putting a mutex in every inspector would create lock ordering, long UI critical sections, unstable index races, and no undo/redo boundary. Allowing workers to mutate the scene while ImGui traverses it would be incorrect.

The target editor seam is:

~~~text
Published WorldReadView generation ──► EditorSceneModel ──► ImGui panels
          ▲                                      │
          │                                      ▼
World commit + typed result ◄── EditorCommandQueue / Transaction
~~~

- `EditorSceneModel` is rebuilt incrementally from the world journal or replaced from a full read-view generation
- selection is `EntityId`, validated whenever the model generation changes
- inspectors edit local draft values and enqueue a semantic command only when a widget actually changes
- commands are applied at the next world commit point, with a typed accepted/rejected result
- continuous drags may coalesce into one transaction while still producing bounded preview commands
- undo/redo is owned by the editor main thread and applies through the same command seam
- panels never keep `MeshComponent*`, `SceneLightDesc*`, camera pointers, or spans into mutable scene storage
- UI remains responsive while a load/cook operation runs because it displays the last committed model plus operation state

The direct `GameScene*` editor host service, index-based selection, and `SceneObjectActions` direct-mutation helpers are deletion targets once all current panels use the command/model seam.

### Editor Operation Service

Add one private `EditorOperationService` over SparkleTasks; do not create a second editor thread pool. It owns document/application task scopes and supports only current or approved product operations:

| Operation | Worker stages | Owner-thread/result stage | Stale/cancel behavior |
|---|---|---|---|
| Open/reload level | staged scene-load graph | validate and enqueue atomic scene commit | old document remains; superseded request discarded |
| Shader recook | process launch, pipe drain, publication validation | enqueue shader-generation replacement | old package remains active |
| Asset import/cook | read/import/build/cook | publish accepted generation and refresh catalog model | temporary generation rejected/cleaned later |
| Thumbnail/mesh preview preparation | decode/CPU geometry preparation where safe | publish immutable preview generation; renderer uploads/renders | bounded cache/request generation drops stale result |
| Asset/outliner search | immutable model partition scan | ordered result list publication | newest query generation wins |
| Save/package validation | immutable snapshot serialization/validation | atomic file/publication replace and status | active file stays valid on failure |

Each operation exposes an ID, state, bounded progress summary, cancellation request, and final typed result. This is workflow state, not a task debugger. It does not expose worker IDs, queues, arbitrary dependency graphs, or render caches in the editor.

### Editor Thread-Affinity Matrix

| Data/API | Owner | Worker access |
|---|---|---|
| ImGui context, widgets, docking, selection, transactions | editor main | none |
| `EditorSceneModel` publication | editor main swaps generation | read-only task inputs only when generation-pinned |
| `GameScene` structure and commit | game/main owner | none; workers use views/results |
| file decode/import/cook intermediates | operation task/private arena | exclusive task or immutable child inputs |
| viewport targets, descriptors, GPU preview resources | render coordinator | immutable requests/results only |
| diagnostics/status text | editor main model | workers return structured result; no direct callback |

Window messages, input subscriptions, and existing `Event` callbacks stay on their declared owner thread. Cross-thread completion enters through the operation result queue or `TaskEvent` continuation and is applied in `UI::Update`/application update, never inside a worker.

### Responsive Editor Budgets and Backpressure

- interactive frame work has priority over background preview/search/cook work
- asset and thumbnail operations are bounded by decoded bytes and pending renderer-upload bytes
- repeated property edits coalesce by target/property/transaction rather than allocating an unbounded queue
- repeated reload/search requests use latest-generation-wins cancellation
- viewport and UI packets use bounded frame slots
- application/document shutdown cancels operations, drains child I/O, rejects late publications, then destroys editor models

The portfolio demonstration must show a level load or cook running while the editor remains interactive, cancellation leaving the old scene/publication intact, and stale completion being rejected. It must also show that heavy Background work cannot starve the FrameCritical lane.

## Tools and Content Pipeline

### One Runtime, Different Host Policy

Offline tools should link SparkleTasks but configure it for throughput:

- no render coordinator
- adopted main thread may help execute compute tasks
- most available cores assigned to compute/background lane
- small blocking-I/O lane
- stable deterministic publication order
- progress events marshalled to one console/UI owner

Task parallelism does not broaden tool ownership. The launcher remains a thin orchestrator for build, cook, run, clean, and package if those workflows are retained. Shader/package inspection stays intentional and opt-in. The scheduler must not create a resident asset daemon, distributed build service, default stats pipeline, or diagnostic shell.

### Catalog, Content-Pack, and Package Discipline

Parallel discovery starts from owned project manifests/catalogs. It must not make recursive scanning of uncataloged heavy content the default workflow.

Required product shape:

- a curated in-repository level/content set for build, runtime smoke, editor review, D3D12/Vulkan validation, and the multithreading demo
- arbitrary additional levels referenced through project catalogs/manifests
- heavyweight media delivered as an optional content pack, release asset, LFS policy, or owned media-fetch workflow
- deterministic content identity independent of task completion order or absolute checkout path

Package fan-out may build intentional outputs:

~~~text
Validated cooked generation
       ├─ Runtime package
       ├─ Editor package
       ├─ Symbols package
       ├─ Development tools package
       └─ Optional content package(s)
~~~

Each output has an owner and explicit inputs. Package assembly joins only after its required cook graph succeeds. A package kind with no owned consumer should be deleted rather than parallelized.

### Asset Cook Graph

The current AssetCooker stage loop and per-scene loop can become:

~~~text
Discover project inputs
          │
          ├───────────────────┬────────────────────┐
          ▼                   ▼                    ▼
   Shader cook graph   Import scene A..N    Default textures
                              │                    │
                 ┌────────────┼────────────┐       │
                 ▼            ▼            ▼       │
             Mesh build  Material build  Anim/skel │
                 │            │            │       │
                 └──────┬─────┴─────┬──────┘       │
                        ▼           ▼              ▼
                Scene manifests  Texture request union
                                      │
                                      ▼
                               Texture cook A..N
          └───────────────────┬────────────────────┘
                              ▼
                 Validate complete output set
                              │
                              ▼
                   Publish registries/manifests
~~~

Important constraints:

- imported scene objects are immutable after import or confined to one scene subgraph
- a non-thread-safe importer gets one instance/context per task or a serialized resource lane
- output paths are unique per asset task
- diagnostics collect by stable asset key, not task finish order
- shared texture request deduplication happens in a deterministic merge
- registries are written only after all required assets succeed

### Texture Cooker

The current request batch uses one TextureAssetCooker and a serial loop. Parallelize per request only after auditing:

- COM initialization per worker thread
- decoder and image-library thread safety
- independent cooker instance or immutable cooker
- output-path uniqueness
- compression libraries’ internal thread counts
- memory budget for simultaneous high-resolution source/mip chains

Use a weighted concurrency limiter based on estimated source/decompressed bytes, not only number of tasks. Eight 8K EXR jobs can exhaust memory even on a sixteen-core machine.

### Shader Cooker

ShaderPackageCooker currently executes cook nodes sequentially. Its plan already supplies natural node identity and output/cache semantics.

Parallelization rules:

- one backend/compiler session per worker when the API requires thread confinement
- explicit concurrency limits for DXC/Slang if they spawn internal workers or consume large memory
- artifact-cache records written atomically
- package emission waits for all constituent stages
- registry publication waits for all packages
- cache hits remain tasks only when the scheduling cost is justified
- console progress is serialized

The task graph should preserve deterministic package and shader-blob IDs regardless of compile completion order.

### Process Runner

Child-process pipe reads are blocking operations. Keep them on the blocking-I/O lane and publish structured progress/result events. Cancellation should:

- signal the child according to platform policy
- continue draining/closing pipes safely
- settle the task exactly once
- retain captured diagnostics
- never invoke UI callbacks from the pipe reader thread

### Transactional Output

All parallel cookers should write to generation-specific temporary output, validate it, then atomically publish a registry/manifest pointer or rename the completed generation. A cancelled or failed cook leaves the previous generation usable and cleans temporary data on a later safe pass.

### Repository-Wide Concurrency Pattern Coverage

The portfolio should not claim mastery because many threads appear in a trace. It should demonstrate why different problems require different concurrency patterns:

| Pattern/concept | Sparkle production use | What it proves | Characteristic failure to test |
|---|---|---|---|
| fixed workers + work stealing | animation, renderer preparation, recording, cook graphs | load balancing, grain size, false-sharing-aware scheduler state | tiny-task overhead, starvation, shutdown race |
| sparse-set ECS + typed queries | transforms, cameras, lights, mesh/animation instances | generational identity, packed iteration, data/component separation | sparse/dense corruption, stale views, poor multi-pool locality |
| frozen structural epoch + entity commands | create/destroy/add/remove from game/editor systems | safe composition change around parallel iteration | mutation during query, nondeterministic playback |
| prerequisite DAG/fan-in | system phases, pose/morph joins, render preparation, cooks | happens-before without worker blocking | cycle, failed prerequisite, unsettled continuation |
| structured cancellation/scopes | scene load, editor document, asset generation, tools | object lifetime and cooperative cancellation | callback/result after owner destruction |
| exclusive partitioned writes | pose slots, transforms, culling/recording ranges | race-free data parallelism without locks | overlap/off-by-one and allocator contention |
| deterministic task-local merge | world commands, animation outputs, manifests | result determinism despite nondeterministic scheduling | completion-order IDs/output drift |
| immutable release/acquire publication | world read view, frame packets, load packages, tool generations | visibility, ownership transfer, reclamation | stale generation and early reuse |
| bounded producer/consumer queue | game-to-render frames, editor/render requests | backpressure, latency, sleep/wake semantics | unbounded backlog or lost wakeup |
| serial owner / actor-like command queue | world commit, render/RHI submission, editor transactions | affinity and invariant protection without broad locks | reentrancy and out-of-order commands |
| blocking-I/O lane + continuation | manifests, files, compiler/process pipes | oversubscription control and nonblocking frame workers | pipe/file wait exhausting compute workers |
| deferred retirement | render assets, shader generations, published read storage | CPU versus consumer/GPU lifetime separation | use-after-free and never-reclaimed generations |
| external event/task completion | process exit, async I/O completion, optional readback | bridge from non-worker completion into a DAG | double completion/cancel race |
| deterministic serial executor | every task graph | reference semantics, debugging, low-core support | parallel-only behavior or hidden side effects |
| GPU queue/timeline synchronization | frame graph/RHI | CPU tasks are not GPU concurrency; explicit queue ownership | missing barrier/wait or needless serialization |

This matrix is also a scope check. If a new subsystem demonstrates no new production need and no concept not already covered, do not add it for portfolio volume.

## Architecture Decision Record

### Decision Matrix

| Question | Chosen direction | Rejected/deferred alternative | Reason |
|---|---|---|---|
| General concurrency primitive | Engine-owned bounded task runtime | More std::async calls | No prerequisites, lanes, diagnostics, worker control, or consistent shutdown |
| Scheduler model | Work stealing plus explicit task DAG | Only a global FIFO pool | Frame workloads are nested and irregular; a FIFO becomes a contention/load-balance limit |
| Worker waiting | Dependencies and continuations | Busy-yield or routine worker Wait | Avoids wasted CPU and pool deadlock; makes critical path visible |
| Blocking operations | Separate limited I/O lane | Run on frame workers | Prevents pipe/file waits from starving a frame |
| Game world model | Private sparse-set ECS with data components, typed queries, and deferred structure | Polymorphic entity update or immediate Mass/DOTS-scale archetypes | Gives normal DOD/ECS architecture and job partitioning at bounded implementation cost |
| ECS structural changes | Task-local deterministic command buffers at owner commit | Concurrent registry mutation/locks | Keeps queries stable and IDs reproducible while systems run in parallel |
| ECS future storage | Preserve private storage seam and evidence-gate archetype chunks | Promise one storage forever or build two paths now | Allows measured evolution without changing game/editor/render contracts |
| Game/render boundary | Immutable bounded packets + deltas | Shared GameScene with mutex | Establishes ownership and permits real pipelining |
| Renderer ownership | Dedicated render coordinator | Arbitrary thread-safe RendererSystemRoot | Fewer locks, stable RHI ownership, simpler lifetime reasoning |
| CPU pipeline depth | Bounded one frame ahead | Unbounded render queue | Controls latency and memory |
| Scene GPU data | Persistent GPU scene + dirty ranges | Full rebuild/upload in parallel | Removes work and bandwidth before distributing it |
| CPU render work | Preparation DAG + parallel recording | Only move serial renderer to render thread | A render thread alone overlaps game/render but leaves renderer cores idle |
| GPU submission | Render coordinator in compiled order | Worker queue submission | Preserves deterministic queue tokens and state transitions |
| RHI thread | No separate translation thread initially | Unreal-style additional RHI thread | Sparkle directly records RHI commands; no measured need for another stream |
| Pass safety | Opt-in parallel flag/audit | Assume all frame-graph passes safe | Current lazy caches/upload allocation make that unsafe |
| Frame graph setup | Coordinator-owned initially | Concurrent graph mutation | Setup is mutable and usually cheaper than recording; reduce/cache first |
| Debug reference | Same architecture in serial mode | Keep old legacy path | One data contract prevents divergence |
| Tool outputs | Deterministic fan-in and transactional publication | Publish as tasks finish | Reproducibility and failure safety |
| Advanced suspension | Defer fibers/coroutines | Implement immediately | High complexity without current blocking-task requirement |

### Tradeoff Ledger

| Decision | Benefits | Costs / risks |
|---|---|---|
| Engine-owned task runtime | Deep learning, unified execution policy, private profiler events, exact lane/render integration | Scheduler maintenance and subtle concurrency correctness burden |
| Work stealing | Adapts to irregular animation/culling/recording workloads | Execution order is nondeterministic; deque and shutdown logic are harder |
| Private sparse-set ECS | Packed per-component iteration, generational identity, flexible composition, direct job ranges | Sparse multi-component joins, storage invariant burden, migration from object facades |
| Data-only hot components | Better locality, explicit dependencies, easier immutable extraction | More explicit systems/conversion code; careless fragmentation can increase gathers |
| Deferred entity commands | Safe parallel iteration and deterministic structural mutation | Composition changes become visible only at commit and temporary entity remapping must be correct |
| Immutable frame packets | No scene lock, replay/testing, clean lifetime boundary | Packet arenas, copy/extraction cost, explicit versioning |
| Structural deltas | Work scales with change; enables persistent GPU state | More bookkeeping, sequence recovery, initial full-sync path |
| Dedicated render coordinator | Game/render overlap and one owner for mutable RHI state | Thread communication, bounded-lag policy, harder debugging without serial mode |
| One-frame-ahead pipeline | Higher throughput and smoother CPU utilization | Adds latency and buffers another frame of CPU data |
| Persistent GPU scene | Far less allocation/upload churn and stable temporal identity | Capacity growth, fragmentation, compaction, and retirement complexity |
| Parallel preparation | Uses cores on large scenes; exposes dependencies | Private scratch/merge cost and task-grain tuning |
| Parallel command recording | Reduces high draw/pass CPU record time | More command lists, per-worker memory, barrier/state and driver overhead |
| Per-worker RHI contexts | Native ownership proof and nearly contention-free hot path | buffered-frame × worker × queue memory scales quickly |
| Coordinator-only submission | Stable ordering/tokens and simpler device failure handling | Submission remains a serial fraction |
| Serial frame-graph setup initially | Easier correctness and one graph owner | Setup/compile can become the next critical-path limit |
| No separate RHI thread | No second command stream or added translation latency | Leaves future platform/translation overlap opportunity unused |
| Separate blocking lane | Frame work cannot be starved by files/pipes | More threads and cross-lane continuation policy |
| Deterministic merge/publication | Reproducible frames/cooks and reliable caching | Sorting/merge work and stricter ID design |

### Why Not Use Only a Third-Party Library

A mature library such as oneTBB, Taskflow, enkiTS, or cpp-taskflow could reduce scheduler work. That is a valid product decision, but it weakens this specific learning goal unless Sparkle still owns the data, renderer, RHI, and diagnostic design.

Recommended approach:

- implement the focused SparkleTasks runtime
- keep its public concepts small enough that the scheduler backend could be replaced
- if scheduler overhead becomes a measured problem, compare it in an isolated test against a reputable library without adding a benchmark framework
- document where a production team might buy rather than build

The portfolio value is not that a deque was invented. It is that scheduling semantics, memory ownership, frame pipelining, native command contexts, and engine integration are understood.

### Why Not One Permanent Thread per Subsystem

Permanent animation, culling, lighting, render-pass, asset, and shader threads create load imbalance. When animation is idle its core cannot automatically help a large shadow pass. A small set of role threads plus a shared task executor lets available workers follow current work while ownership remains explicit.

Use permanent threads only for roles that require affinity or a serial event loop:

- main/window thread
- render coordinator
- limited blocking-I/O workers
- platform/API services that mandate a thread

### Why Not “Lock-Free Everywhere”

`RenderFrameQueue` is single-producer/single-consumer and a good bounded specialization. Worker queues are performance-sensitive and can benefit from work-stealing deques. Many other structures are not.

A mutex-protected rare shader-generation swap is simpler than a lock-free lifetime protocol. A locked external injection queue is acceptable if ordinary worker spawning stays local. Measure contention before introducing harder reclamation schemes.

## Positive Patterns to Demonstrate

### Ownership Before Locks

- one owner for GameScene
- one owner for RenderWorld/RendererSystemRoot
- one owner for queue submission
- one worker lease for each command allocator/pool
- immutable data after publication
- completion-token retirement for GPU-visible data

### Data-Oriented Boundaries

- stable generational IDs
- dense transforms, bounds, lights, and skinning ranges
- deltas for structure, arrays for frame dynamics
- per-task private output then deterministic merge
- persistent GPU indices instead of transient vector positions

### Structured Concurrency

- graph/run lifetime encloses child tasks
- cancellation reaches descendants
- completion settles once
- a join is a named graph node
- shutdown stops acceptance, drains/cancels by policy, joins workers, then destroys scheduler memory

### Serial Equivalence

Every major parallel path supports:

- single worker
- N workers
- parallel recording off
- render thread off while consuming the same packet

The same output and invariants should hold. This makes concurrency a selectable execution policy rather than a second engine.

### Explicit Backpressure

- bounded frame slots
- bounded process/IO concurrency
- weighted texture memory limit
- bounded upload pages with an observable slow path
- bounded compiler process/session count

### Deferred Destruction

- logical removal now
- physical RHI destruction after the last relevant queue token
- generation swaps leave old data alive until safe
- no routine device-idle synchronization

## Negative Patterns to Prevent

| Negative pattern | Why it fails | Enforced alternative |
|---|---|---|
| Thread per feature/subsystem | Load imbalance and oversubscription | Task graph on a bounded worker set |
| std::async per operation | Uncontrolled threads/semantics and weak diagnostics | executor lane and `TaskExecution` |
| Raw pointers in frame packets | Cross-thread lifetime race | Stable handles and packet-owned values |
| Mutex around the whole GameScene | Serializes frame pipeline and hides ownership | RenderWorld proxies and deltas |
| Make every component/object thread-safe | Distributed locks cannot express frame-phase invariants and make composition unsafe | Owner commit, declared system domains, immutable views |
| Call polymorphic `Entity::Update` and label it ECS | Object traversal retains hidden behavior/dependencies and poor hot iteration | Data-only component pools plus explicit query systems |
| Put asset ownership/services inside components | Compaction moves heavyweight state and couples world to loaders/renderer | Stable asset handles in small instance components |
| Structural mutation while queries/jobs run | Sparse/dense indices or archetype locations become invalid | Frozen structural epoch plus task-local command buffers |
| Use task completion order for entity commands | Entity IDs/composition differ between runs | Stable system/partition/local sort keys at playback |
| Split every component into SoA fields automatically | Complexity and gather cost may exceed locality benefit | Access-pattern-driven AoS/SoA choice with captures |
| Build archetype chunks before scale evidence | Large allocator/query/migration project delays current engine improvements | Sparse-set kernel first; storage implementation remains private |
| Parallel arbitrary `GameSceneController` calls | Whole-scene mutable access has undeclared hazards | Narrow registered systems; serial legacy adapter only during migration |
| Lazy mutable cache in a `const` getter | Nominal readers race after invalidation | Explicit owner/evaluation phase and immutable published output |
| Editor panels read/write live scene on workers | UI pointers, vector indices, and transactions cross unsafe lifetimes | Stable-ID read model plus owner-thread commands |
| Worker broadcasts arbitrary callbacks | Callback thread affinity, reentrancy, and owner lifetime are unknown | Typed result/command queue dispatched by owner after commit |
| Clear old level before new load validates | Failure destroys usable state and cancellation cannot roll back | Isolated load package plus atomic commit |
| Global mutable loader/registry shared by tasks | Data race and nondeterministic IDs/output | Immutable catalog generation and task-local partial packages |
| Detached/background task captures raw owner | Completion after document/world/service destruction | Structured task scope and generation validation |
| Busy polling futures/fences | Burns cores and increases latency | Event/semaphore/continuation |
| Worker task waits for child | Pool exhaustion/deadlock risk | Dependency edge and continuation |
| Unbounded render queue | Memory growth and input latency | Bounded frame slots |
| Shared mutable pass context | Data race and order dependence | Immutable context + local recording lease |
| Same D3D12 allocator/list on two tasks | Native API violation | Per-worker/per-frame ownership |
| Same Vulkan command pool on two tasks | External synchronization violation | Per-worker/per-frame pool |
| Workers submit GPU queues | Nondeterministic tokens/order and contention | Coordinator submission |
| Lazy PSO creation in recording tasks | Cache race and unpredictable long tasks | Prewarm/materialize before fan-out |
| Global upload cursor per binding | Hot atomic/cache-line contention | Worker pages or preplanned slices |
| Device WaitForIdle for scene/reload | Destroys overlap and scales poorly | Versioned swap + deferred retirement |
| Parallelize full scene copy | Spends cores on avoidable work | Persistent state + dirty deltas |
| Task completion order defines output | Nondeterministic builds/rendering | Stable keys and ordered merge |
| Tiny task per object/draw | Scheduler overhead dominates | Chunking and serial thresholds |
| Blocking decoder/compiler internally oversubscribes | Core and memory collapse | Concurrency limiter and nested-thread policy |
| Lock-free structure without reclamation proof | Rare correctness failures | Simple ownership/lock until measured |
| CPU tasks called “async compute” | Misleading architecture and metrics | Separate CPU task, GPU queue, and frame timelines |

## Professional Multithreading Failure Atlas

The negative-pattern summary above is intentionally compact. This atlas is the implementation review contract. Its purpose is to help the owner recognize common failures from code, traces, symptoms, and interview scenarios before they become repository conventions.

The names used here are deliberately conventional. A professional engine programmer should immediately recognize **task DAG**, **structured/nested task lifetime**, **owner thread**, **immutable publication**, **game/render proxy**, **bounded producer-consumer queue**, **serial fallback**, **parallel-for grain**, **worker-local allocator**, **per-thread/per-frame command pool**, **deterministic fan-in**, **fence/timeline retirement**, **critical path**, and **backpressure**. Sparkle may use engine-specific types, but it must not invent obscure names for established ideas.

### Vendor Evidence and Implementation Key

The atlas cites these primary sources and implementations by short key:

| Key | Primary evidence | What Sparkle learns—not blindly copies |
|---|---|---|
| NV-CPU | NVIDIA, [Limiting CPU Threads for Better Game Performance](https://developer.nvidia.com/blog/limiting-cpu-threads-for-better-game-performance/) | logical-core-minus-N is not a universal policy; oversubscription, SMT sharing, chiplet/cache traffic, locks, atomics, false sharing, context switches, and power behavior can reverse scaling |
| NV-VK | NVIDIA, [Vulkan Dos and Don'ts](https://developer.nvidia.com/blog/vulkan-dos-donts/) and [Advanced API Performance: Command Buffers](https://developer.nvidia.com/blog/advanced-api-performance-command-buffers/) | explicit APIs require application-side recording parallelism; command-buffer setup/reset, tiny buffers, submit count, queue waits, reuse, and batching/latency must be measured |
| NV-PAR | NVIDIA nvpro_core [parallel_work](https://github.com/nvpro-samples/nvpro_core/blob/master/nvh/parallel_work.hpp) and [ring/allocator documentation](https://github.com/nvpro-samples/nvpro_core/blob/master/nvvk/README.md) | familiar range/batch APIs, workload-dependent serial thresholds, ringed GPU-lifetime resources, per-thread resource allocators, and explicit warnings that sample utilities are not production proof |
| NV-TASK | NVIDIA [stdexec](https://github.com/NVIDIA/stdexec) and Donut [ThreadPool interface](https://github.com/NVIDIA-RTX/Donut/blob/bc1ea24b0486f1c00d89327fe16c0b4dd11c5937/include/donut/engine/ThreadPool.h)/[implementation](https://github.com/NVIDIA-RTX/Donut/blob/bc1ea24b0486f1c00d89327fe16c0b4dd11c5937/src/engine/ThreadPool.cpp) | structured composition and a bounded pool are recognizable precedents; Donut's small pool is a reference baseline, not sufficient evidence for Sparkle's dependencies, scopes, lanes, cancellation, or work stealing |
| NV-TRACE | NVIDIA [Nsight Systems User Guide](https://docs.nvidia.com/nsight-systems/UserGuide/index.html) | inspect scheduling, context switches, long blocking runtime calls, lock waits, file I/O, CPU/GPU gaps, and backtraces rather than inferring from thread utilization |
| AMD-CPU | AMD, [Ryzen CPU Performance Guide](https://gpuopen.com/learn/ryzen-performance/) and [CPU Core Count Detection](https://gpuopen.com/learn/cpu-core-count-detection-windows/) | prefer thread-local/range-local data, prevent false sharing, understand physical/logical topology, and profile pool size instead of assuming it |
| AMD-RDNA | AMD, [RDNA Performance Guide](https://gpuopen.com/learn/rdna-performance-guide/) and [Driver Experiments](https://gpuopen.com/learn/rdts-driver-experiments/) | command allocators are not thread-safe; use allocator-per-recording-thread-per-frame, avoid tiny command buffers/excess submits, reuse allocations, and treat a driver thread-safety workaround fixing a bug as evidence of an application synchronization defect |
| AMD-RPS | AMD Render Pipeline Shaders [Vulkan multithreading test](https://github.com/GPUOpen-LibrariesAndSDKs/RenderPipelineShaders/blob/main/tests/gui/test_multithreading_vk.cpp), [D3D12 test](https://github.com/GPUOpen-LibrariesAndSDKs/RenderPipelineShaders/blob/main/tests/gui/test_multithreading_d3d12.cpp), [shared test](https://github.com/GPUOpen-LibrariesAndSDKs/RenderPipelineShaders/blob/main/tests/gui/test_multithreading_shared.hpp), and [tutorial](https://gpuopen.com/learn/rps-tutorial/rps-tutorial-part4/) | graph-planned command ranges, exclusive command buffers, fan-out/fan-in, backend-specific render-pass handling, and tests across thread counts; sample locks and waits are study material, not a license to place them on Sparkle's final hot path |
| AMD-PORT | AMD, [Porting Detroit: multithreaded render lists](https://gpuopen.com/learn/porting-detroit-3/) | use the job system, pool-per-thread, recycled buffers, draw-count-aware job splitting, and restore deterministic single-thread order before execution, especially for transparency |
| AMD-TASK | AMD FidelityFX Cauldron2 [TaskManager interface](https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK/blob/60f4ea81909200d8542eca14dccb2628b763a9a3/Kits/Cauldron2/dx12/framework/core/taskmanager.h)/[implementation](https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK/blob/60f4ea81909200d8542eca14dccb2628b763a9a3/Kits/Cauldron2/dx12/framework/core/taskmanager.cpp) | a real AMD sample framework has centralized task management; Sparkle still needs its own stronger structured-lifetime and lane contract because a vendor sample is not automatically a production engine architecture |
| EPIC-TASK | Epic, [Tasks System](https://dev.epicgames.com/documentation/en-us/unreal-engine/tasks-systems-in-unreal-engine) and [`ParallelFor`](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Core/ParallelFor) | prerequisites avoid blocking workers; nested tasks express completion lifetime; busy-wait task-helping caused deadlocks, latency, spinning, and stack growth; long/blocking work must not clog the task graph; grain and forced-serial modes are first-class |
| EPIC-OWN | Epic, [Threaded Rendering](https://dev.epicgames.com/documentation/en-us/unreal-engine/threaded-rendering-in-unreal-engine) | game-owned objects must not be cached/read by renderer code; mirror required values into render-owned proxies, use asynchronous commands/fences, and do not guess-and-check race freedom |
| EPIC-RHI | Epic, [Parallel Rendering Overview](https://dev.epicgames.com/documentation/en-us/unreal-engine/parallel-rendering-overview-for-unreal-engine) | preserve single-thread submission order, use owned command data/context, bound frame lead, isolate exceptional flush/copy operations, and keep independent serial/parallel debug controls |
| EPIC-TRACE | Epic, [Task Graph Insights](https://dev.epicgames.com/documentation/en-us/unreal-engine/task-graph-insights-in-unreal-engine-5) and [Timing Insights](https://dev.epicgames.com/documentation/unreal-engine/timing-insights-in-unreal-engine-5) | record create/launch/schedule/start/finish/complete stages, prerequisites/nesting, critical path, thread/core tracks, callers/callees, and context switches |

These sources are precedents, not templates to paste. A sample can use a global wait, lazy creation lock, fixed partition, or small thread pool because its product scope is narrow. Sparkle adopts the recognizable **pattern and invariant**, then validates it against Sparkle's renderer, editor, tools, two backends, lifetime, and latency requirements.

### A. Correctness, Ownership, and Lifetime Failures

| ID | Frequent mistake and recognizable symptom | Required Sparkle pattern | Mandatory falsification |
|---|---|---|---|
| MT-01 | A render task retains `GameScene*`, component/object pointers, vector references, or UI objects. It works until mutation, unload, GC, reallocation, or document close. This is Epic's canonical game/render race. (EPIC-OWN) | Separate game-owned state from render-owned proxies; packet-owned values and stable generational asset/object handles cross the boundary | mutate/unload/reallocate immediately after packet publication while delaying render consumption; no source-owner access or stale dereference |
| MT-02 | A producer publishes a pointer/reference and continues mutating its reachable graph; `const` is treated as synchronization | immutable-after-publication value graph, ownership transfer, or versioned copy-on-write generation | randomized post-publish mutation attempts assert or create a new generation; serial/threaded packet replay is identical |
| MT-03 | An atomic ready flag exists, but payload writes do not happen-before payload reads; relaxed ordering is used as a slogan | locked publication or explicit release-publish/acquire-claim state machine whose payload lifetime is proven | mutex and sequentially-consistent reference protocols plus delayed weak-order stress produce the same observations |
| MT-04 | IDs, slots, task runs, or resources are reused without a generation; stale completion mutates a new occupant—the logical ABA problem | generational handles, monotonic sequence values, bounded wrap policy, and stale-result rejection | force tiny counters/wrap and delayed old completion; new occupant is never accepted as the old operation |
| MT-05 | Detached task/lambda captures `this`, a raw service, document, world, frame arena, or stack reference | structured `TaskScope`; child/nested completion is part of owner completion; immutable input ownership is explicit (NV-TASK, EPIC-TASK) | destroy/cancel every owner immediately after spawn and at every stage; no callback-after-destroy, access, leak, or unjoined task |
| MT-06 | Failure, exception, cancellation, or shutdown skips decrement/signal/publication, so a run or waiter never settles | exactly-once terminal state and finally/cleanup edge for success, failure, cancellation, and rejection | race cancel/fail/complete/shutdown millions of times; every accepted run reaches exactly one terminal result |
| MT-07 | Cancellation is treated as thread termination or immediate resource destruction | cooperative checkpoints; cancellation stops future publication while already-enqueued GPU/native work retires normally | cancel before start, mid-CPU, mid-I/O, after submit, and during shutdown; prior generation remains usable and GPU objects outlive last use |
| MT-08 | Arbitrary callback, event, logging, driver/provider, destruction, or UI code runs under an engine lock; re-entry deadlocks or widens the critical section | snapshot-under-lock/apply-after-unlock, two-phase transition, typed owner-thread command/result | callback self-unsubscribes, subscribes, nests dispatch, logs, destroys its owner, and is deliberately slow; no engine lock is held |
| MT-09 | A function is “thread-safe” but is called from the wrong required thread: Qt/ImGui, windowing, COM mode, provider, Vulkan pool, renderer owner | explicit thread-role/affinity assertions and owner command boundary | deliberately call from every wrong role; development assertion identifies owner and operation before native corruption |
| MT-10 | Shutdown destroys services, arenas, RHI, UI, or provider before producers stop and accepted work settles | ordered shutdown: close acceptance → cancel/drain scopes → close `RenderFrameQueue`/events → settle GPU → destroy owner resources → join owner → stop executor | shutdown with every queue empty/full and tasks blocked/running/completing; no stranded slot, wait, callback, handle, or native validation error |
| MT-11 | A giant mutex is added around `GameScene`, Renderer, registry, or allocator and described as architecture | single mutable owner, immutable read views, commands, range ownership, and narrow locks only for true shared boundaries | contention trace plus wrong-thread assertions show no frame-scale lock convoy; serial mode uses the same ownership path |
| MT-12 | Lock order is implicit; one path takes A→B while callback/shutdown takes B→A | documented global lock order, no blocking/callback under lock, or remove nesting through ownership/commands | lock-order inversion injection and timeout-backed deadlock test identify the cycle; production path has no cycle |

### B. Task-Graph, Progress, and Scheduling Failures

| ID | Frequent mistake and recognizable symptom | Required Sparkle pattern | Mandatory falsification |
|---|---|---|---|
| MT-13 | A worker launches children then waits; all workers can become parents waiting for work that cannot run | prerequisite/subsequent DAG and nested completion, with host waits only at declared boundaries (EPIC-TASK) | fill every worker with a parent spawning a child; completion succeeds at 1/2/N workers without emergency threads |
| MT-14 | A waiting worker spins or executes unrelated tasks recursively; CPU burns, unrelated long work delays resumption, circular dependencies or stack overflow appear | predicate parking and explicit dependencies; no generic busy-wait/help loop. Epic deprecated this behavior for these failure modes (EPIC-TASK) | empty-queue wait consumes negligible CPU; recursive-wait and unrelated-long-task scenarios terminate predictably |
| MT-15 | Condition-variable wait uses `if`, notification is emitted without protected state, or shutdown misses a sleeper | state change and predicate share one synchronization protocol; wait is predicate-based; close wakes all | notify-before-park, park-before-notify, spurious wake, close-while-empty/full, and last-worker shutdown stress |
| MT-16 | File/process/pipe/GPU wait or decoder/compiler blocking occupies frame workers | separate bounded BlockingIo policy and external-completion event; CPU continuation returns to appropriate lane | saturate files/process pipes and delay GPU while frame work continues within budget; task-worker stack never contains native blocking wait |
| MT-17 | Each subsystem/library creates its own pool or permanent thread; context switching and cache pressure grow invisibly | one engine executor family with host/lane policy; inventory third-party internal workers and reserve capacity (NV-CPU) | system trace counts runnable threads, migrations, context switches, and third-party workers under simultaneous cook/render/provider load |
| MT-18 | Worker count is `hardware_concurrency()-N`, logical-core count, or a compile-time constant for every topology/workload | conservative measured default, 0/1/2/N override, physical/logical/topology metadata, interactive/background budget (NV-CPU, AMD-CPU) | tiny/heavy workloads across available topologies; keep fewer workers when p95/latency/throughput wins and record the negative scaling |
| MT-19 | One task per entity/draw/descriptor or an unconditional `ParallelFor`; scheduling is slower than work | coarse ranges, workload-specific grain, serial crossover, and instrumentation. Epic and nvpro expose batch/serial controls (EPIC-TASK, NV-PAR) | sweep grain from serial to tiny; retain measured crossover and ensure tiny scenes choose serial automatically |
| MT-20 | Equal item counts are assumed equal cost; one worker becomes the straggler while others idle | dynamic work distribution/work stealing or cost-aware ranges, exclusive outputs, deterministic fan-in | adversarial skew places expensive items together; trace shows bounded tail and result order remains stable |
| MT-21 | The design creates global barriers after every phase; the frame becomes a sequence of “parallel” islands dominated by the slowest task | dependency DAG exposes earliest-start edges and joins only true consumers; measure critical path, not total work | remove/inject delay into one non-critical branch; unrelated successors start without waiting for a global barrier |
| MT-22 | Priority is required for correctness or used to hide missing dependencies; inversion/starvation follows | dependencies and lanes express correctness; priority is a measured hint with aging/fairness | sustained background/high-priority load cannot starve accepted frame, cleanup, or cancellation/finally work |
| MT-23 | Producer queues, frame packets, captures, uploads, decompressed assets, or compile jobs are unbounded | fixed slots/weighted budgets and explicit reject/block/defer/drop/coalesce policy | slow consumer and burst producer reach a stable memory ceiling and expected backpressure without deadlock |
| MT-24 | Third-party compiler, decoder, provider, middleware, or parallel algorithm spawns internal workers inside N engine tasks | concurrency budget includes nested/internal threads; per-library session limit or serial outer policy | trace total runnable threads and memory at 1/N outer jobs; select the combination that improves throughput without foreground regression |

### C. Data Layout, Contention, and Determinism Failures

| ID | Frequent mistake and recognizable symptom | Required Sparkle pattern | Mandatory falsification |
|---|---|---|---|
| MT-25 | Per-worker counters/flags/queue indices occupy one cache line; there is little lock time but scaling collapses | cache-line-separated hot writable state, worker-local aggregation, read-mostly cold metadata elsewhere (NV-CPU, AMD-CPU) | padding/layout A/B with cache/remote traffic counters; randomized worker placement; keep padding only with evidence |
| MT-26 | Every task contends on one atomic bump pointer, shared vector push, allocator mutex, registry, logger, or descriptor cursor | worker-local pages/arenas/partials plus preassigned ranges and deterministic merge | contention profile shows no frame-hot shared cursor; allocations/ranges never overlap and reset is token-safe |
| MT-27 | Parallelism multiplies scratch, command allocators, descriptor pools, command buffers, and decoded assets by workers × frames × queues | explicit memory equation, capacity budget, page reuse, spill policy, and high-water evidence | maximum configured workers/frames/queues stays within budget; overflow follows a tested deterministic slow/fail path |
| MT-28 | Object-oriented pointer chasing and scattered ownership consume memory bandwidth, so more cores only increase misses | DOD by measured access pattern: dense hot components/ranges, cold metadata split, stable handles, batch iteration | compare cache misses/bandwidth and serial baseline; do not claim DOD because a type is named ECS |
| MT-29 | Completion order determines entity IDs, vector order, diagnostics, package bytes, draw order, or command submission | stable `(system, partition, local)`/asset/pass keys and ordered fan-in; completion order is never semantic (AMD-PORT, EPIC-RHI) | randomized delays/work stealing at 0/1/2/N workers produce identical promised output/order/hash |
| MT-30 | ECS/registry/vector structure changes while ranges or queries execute; dense indices/pointers become invalid | frozen structural epoch and per-task command buffers committed by owner after readers join | randomized create/destroy/add/remove during scheduled reads is rejected/deferred and stale generations fail safely |
| MT-31 | Duplicate “async” and legacy representations both own truth during migration | one authoritative storage/identity with compatibility as read/command adapter and a deletion gate | mutation through every supported facade converges on one source; repository search finds no second owner or new legacy consumer |

### D. Renderer, RHI, and CPU/GPU Concurrency Failures

| ID | Frequent mistake and recognizable symptom | Required Sparkle pattern | Mandatory falsification |
|---|---|---|---|
| MT-32 | Two tasks record through one D3D12 allocator/list or one Vulkan command pool; adding a mutex “fixes” native errors but serializes recording | exclusive `RhiCommandRecordingLease`; allocator/pool per recording context × buffered frame × queue/family (AMD-RDNA, AMD-RPS, NV-VK) | concurrent-use and reset misuse assert; D3D12 GPU validation and Vulkan synchronization validation pass under migration/delay |
| MT-33 | CPU frame end resets/reuses descriptors, upload pages, command allocators, or resources still referenced by GPU | last-use fence/timeline token, ringed frame resources, generation retirement; CPU completion and GPU completion are distinct (NV-PAR) | delay GPU beyond several CPU frames; reset-before-token fails and memory remains valid until the exact queues complete |
| MT-34 | Any worker/provider thread may submit or present because the queue has a mutex | render coordinator is engine submission authority; native queue lock remains only for documented external synchronization/provider sharing | wrong-thread submit/present fails; token/submission order matches serial compiled plan on both backends |
| MT-35 | Many tiny command buffers and submits multiply CPU/GPU overhead; or giant buffers leave most CPU cores idle | measured recording groups with enough commands, bounded list/submit count, backend-specific secondary/bundle policy (NV-VK, AMD-RDNA) | sweep group size/list/submit count; record CPU record time, GPU time, bubbles, memory, and latency—not only worker utilization |
| MT-36 | Pipeline/layout/descriptor/resource creation or cache insertion happens lazily inside parallel recording | prewarm/materialize immutable pass runtime before fan-out; bounded background creation publishes a later generation | cold-cache recording asserts rather than surprise-blocks; randomized first-use never mutates shared runtime from workers |
| MT-37 | `WaitForIdle`, queue wait, readback, or flush becomes ordinary scene/reload/resize/capture synchronization | sequenced command, relevant completion token, versioned swap, and deferred retirement; exactly one final shutdown/device-reinit drain | native idle-call counter is zero for ordinary operations and detects nested wrapper drains; delayed GPU still makes progress |
| MT-38 | Submission batching is treated as always good, or CPU is allowed arbitrarily far ahead; throughput rises while input latency/pacing worsens | bounded zero/one-ahead modes, early-submit/submit-batch policy chosen from correlated latency and queue evidence (NV-VK, EPIC-RHI) | compare CPU/GPU-bound zero/one-ahead and batch sizes; report throughput, queue gaps, backpressure, pacing, and input-to-present |
| MT-39 | Parallel recording changes barriers, transparent order, temporal history, provider tags, or queue ownership even though images often look correct | same compiled frame-graph plan and single-thread semantic order; deterministic merge/submission; feature-specific history/generation identity (AMD-PORT, EPIC-RHI) | serial/parallel barrier/submission plan comparison, transparent overlap scene, temporal reset, RT/provider/capture matrix, both native validators |
| MT-40 | The engine expects the Vulkan/D3D12 driver to provide CPU parallelism, or confuses CPU jobs, GPU async compute, multi-queue execution, and frames in flight | explicitly separate CPU task DAG, render pipeline, recording contexts, GPU queue graph, and frame-lifetime timeline (NV-VK, AMD-RDNA) | profiler diagram labels each timeline and dependency; every overlap claim identifies actual simultaneous intervals and synchronization |

### E. Verification, Debugging, and Performance-Claim Failures

| ID | Frequent mistake and recognizable symptom | Required Sparkle pattern | Mandatory falsification |
|---|---|---|---|
| MT-41 | “It ran 100 times” or “the race detector was quiet” is called proof; only Debug or one backend is tested | ownership proof plus serial oracle, randomized stress, optimized build, supported sanitizer, native validation, and D3D12/Vulkan parity | test matrix records tool/backend/configuration gaps honestly; injected defect is detected by at least one test/tool |
| MT-42 | Tests use sleeps to force order and become flaky or accidentally pass on faster hardware | controllable barriers/events/fault hooks in test ownership seams; virtualized generation/delay, not wall-clock correctness | randomized scheduling with explicit checkpoints; changing machine speed does not change expected state transitions |
| MT-43 | CPU utilization, thread count, FPS, or average time is the result; critical path, p95/p99, latency, memory, GPU regressions, and profiler overhead are ignored | timeline-first causal experiment with serial control, p50/p95/p99, critical path, blocked/runnable states, cache/contention, memory, GPU and latency evidence (NV-TRACE, EPIC-TRACE) | independent rerun, capture-on/off comparison, negative-result retention, and attribution to removed work/data/jobs/pipeline/GPU separately |
| MT-44 | Lock-free is selected for portfolio value without progress, ABA, lifetime, reclamation, or oracle proof | simple owner/lock first; optimize one measured bounded protocol against locked and SC references | wrap/reclamation/contention/failure tests; if lifetime proof is incomplete, the lock-free path is deleted rather than documented as “future fix” |

### How to Use the Atlas During Implementation

Every K prompt must perform a **hazard pre-mortem** before editing:

1. list the MT IDs the change could introduce or expose;
2. identify the source-backed professional pattern being implemented;
3. name the invariant and the concrete failure injection/test/trace that can disprove it;
4. run serial and smallest-thread modes before N-worker performance work;
5. close the hazards in the completion report. “Not observed” is not closure.

When a new mistake is discovered, add it to this atlas only if it is materially distinct. Do not create a new document, lint framework, runtime hazard registry, or shipping diagnostic API. The atlas is a review vocabulary; executable ownership, assertions, tests, validation, and traces are the proof.

## Migration Program

This should land as a sequence of reviewable vertical slices. The architectural moment comes from the completed sequence, not one unreviewable mega-commit. Every stage removes its replaced path in the same stage or records an explicit short-lived compatibility deadline.

### Governing-Document Gate by Stage

| Stage | Advanced graphics capability that must remain intact | Structural reduction required |
|---|---|---|
| 0 | D3D12/Vulkan, frame graph, shader ABI, RT paths, providers, capture, debugger hooks | Classify existing waits/observers; add no framework/report/panel |
| 1 | Shader package determinism and tool correctness | Replace std::async/ad-hoc execution and avoid a second pool |
| 2 | Multi-level world identity, temporal/provider signals, classic TLAS/PTLAS inputs | Remove raw game pointers/indices, unused entity scaffolding, full snapshot seam, direct lifecycle callbacks |
| 2A | Existing level/content behavior and editor scene workflows | Replace destructive synchronous scene loading with cancellable isolated preparation and atomic commit |
| 2B | Existing camera, Showcase PTLAS motion, animation, morph, and editor property behavior | Replace arbitrary mutable controllers/live panel mutations with declared systems, commands, and immutable read views |
| 3 | Presentation, capture, provider affinity, editor viewport behavior | Remove host mutation routes and broad observation APIs |
| 4 | Static/dynamic raster data, BLAS, classic TLAS, PTLAS, materials, shader generations | Remove full reuploads and routine device-idle lifecycle waits |
| 5 | Native reservoir lighting, path/reference accumulation, temporal reset, RT planning | Remove monolithic mutable builders and shared scratch |
| 6 | Typed pass declarations, histories, providers, debugger markers, both backends | Remove single-context-only recording and execute-time lazy state |
| 7 | Curated multi-level workflow, optional content, owned packages/tools, screenshot capture | Remove legacy paths, default reports/artifacts, unowned launcher/tool surfaces |
| 8 | All product features and backend parity | Consolidate evidence through existing hooks; add no permanent measurement product |

### Stage 0 — Invariants, Baseline, and Vocabulary

Goals:

- freeze current visual/performance baselines
- add thread-role assertions and FrameId/SceneGeneration/SequenceNumber types
- document which thread owns existing systems
- add development/test execution controls that future stages will preserve without creating a shipping UI or public feature-flag surface

Code areas:

- Engine/Application
- Engine/Renderer/Public/Renderer.*
- Engine/Renderer/Private/RendererSystemRoot.*
- existing RHI allocator/capability/profiler hooks
- existing benchmark/test launch workflow

Work:

1. Add or consolidate profiler-visible CPU scopes, using only existing instrumentation hooks, for game update, snapshot capture, scene build, GPU-data build, graph setup, compile, record, submit, present, and idle waits.
2. Capture p50/p95 CPU timings and GPU timings for representative scenes on both backends.
3. Capture upload bytes, structured buffer allocations, pass count, command-list count, and queue waits only where existing hooks already expose them; mark unavailable values as unknown rather than building an observer system in Stage 0.
4. Add owner-thread assertions to current renderer/RHI mutators.
5. Define development/test-only execution controls, with no shipping UI:
   - tasks worker count
   - threaded renderer
   - parallel recording
   - pipeline depth
   - deterministic serial scheduler
6. Save reference images and deterministic scene/camera scripts.
7. Record the preservation baseline for classic TLAS, PTLAS, native reservoir lighting, reference path mode, temporal/providers, screenshot capture, shader packages, the existing multi-level/content workflow, and both backends; record the curated-level/optional-pack target as a gap rather than implying it already exists.
8. Audit current public observation APIs and default reports/artifacts so later stages have explicit deletion targets.
9. Regenerate LC-01 through LC-18 from all owned Engine/Tools/Projects concurrency hits. Record owner, invariant, affinity/blocking/lifetime policy, disposition, closing stage/prompt, and falsifying test; count nested native GPU drains per top-level operation.
10. Produce the canonical naming ledger from the vocabulary section. Search symbols, filenames, CMake, tests, comments, profiler labels, and thread labels for both canonical terms and semantic aliases; assign every conflict **keep**, **rename in exact owning prompt**, or **delete as alias**. Do not rename unrelated established code in this baseline stage.

Exit criteria:

- baseline commands/settings and essential captures use the existing validation artifact workflow rather than a new report format
- both D3D12 and Vulkan validation runs are clean
- every standard/Qt/native concurrency primitive and every `WaitForIdle` call site is classified; nested drains and callback-under-lock are recorded, and each replacement has an exact deletion prompt
- CPU tasks, OS threads, render control commands, RHI command recording, GPU queues, handles, tokens, leases, contexts, and scopes have one documented meaning; every planned rename and debugger/profiler label has an exact owning prompt
- product, developer-preview, research, and unsupported states are explicit without adding a new feature registry

Learning evidence:

- a written CPU/GPU timeline explaining what is currently serialized
- a profiler capture identifying the real critical path before changing it

### Stage 1 — SparkleTasks Foundation and Tool Pilot

Goals:

- build the task runtime without entangling renderer data
- prove scheduling, cancellation, shutdown, and determinism
- use coarse offline tasks as the first real workload

Code areas:

- new Engine/Tasks
- root/module CMake files
- Engine/Application/Private/ShaderRecook
- Engine/Platform Input callback registry
- Tools/Cooking/TextureCooker
- Tools/Cooking/AssetCooker child-process dispatch
- Tools/Shaders/ShaderCompiler
- Tools/Launcher process orchestration

Work:

1. Implement fixed workers, local queues/stealing, injection, sleep/wake, task pool, dependencies, WhenAll, cancellation, TaskEvent, ParallelFor, and serial executor.
2. Add tests for fan-out/fan-in, nested spawn, cancellation races, external event completion, stale generations, shutdown with queued work, and exception/failure policy.
3. Replace ShaderRecookCoordinator’s std::async with a background/blocking task and structured result publication.
4. Parallelize texture requests with per-task cooker/COM setup and a memory/concurrency limit.
5. Parallelize safe shader cook nodes with compiler-session constraints and deterministic emission.
6. Keep TaskExecutor/task-graph internals private and verify that every public primitive has at least two owned consumers or is required by the host seam.
7. Add structured application, world/document, asset-generation, frame, and tool-invocation scopes with parent cancellation and settlement assertions.
8. Replace LauncherBackend's QThread-per-operation and ProcessRunner's reader-thread/polling/cancel trio with the same scoped operation and BlockingIo contracts; preserve Qt owner affinity and intentionally independent external-process launches.
9. Remove InputSystem callback-under-lock: declare dispatch affinity and snapshot callbacks before invocation, or prove owner-only registry semantics; cover re-entrant registration/unsubscription.
10. Restrict AssetCookerToolProcess's synchronous native wait to an outer CLI host adapter; all scheduled use settles through BlockingIo.

Delete:

- the ShaderRecookCoordinator std::future/std::async execution path
- LauncherBackend per-operation QThreads, ProcessRunner pipe-reader thread/50 ms polling, and duplicate atomic cancellation after structured migration
- lock-held InputSystem subscriber invocation
- any temporary second ad-hoc pool introduced during the pilot

Exit criteria:

- ThreadSanitizer-capable platform or equivalent stress/diagnostic runs pass
- outputs match serial byte-for-byte where formats are deterministic
- worker counts 0/1/2/N work
- failed/cancelled cooks never publish partial registries
- interactive shader recook cannot starve the FrameCritical lane
- launcher close/cancel/restart cannot strand a QThread, child, pipe, or Qt callback; no task worker performs a native process wait
- input callback self-unsubscribe/nested-dispatch tests cannot deadlock and user callbacks execute outside the registry lock
- DXIL/SPIR-V packages, reflection/layout verification, package IDs, and registries match serial output

Learning evidence:

- scheduler invariants captured in code and this existing document, plus one profiler dependency capture
- throughput scaling table and grain/concurrency discussion
- failure/cancellation demonstration

### Stage 2 — World Identity and Render Data Contract

Goals:

- create the real game/editor-to-render ownership boundary
- establish stable world-instance identity and a sequenced commit journal
- establish the private ECS registry, typed sparse-set component pools, queries, and structural epochs in serial mode
- remove all raw game-object/mesh pointers from render packets
- separate structural deltas from dynamic frame data

Code areas:

- Engine/GameFramework scene and snapshot extraction
- Engine/GameFramework ECS registry/storage/query, world IDs, change journal, transform evaluation, and command commit
- Engine/Editor scene selection/model/action seams
- Engine/Renderer/Public scene handoff types
- Engine/Renderer/Private/SceneData
- Asset handle/registry types
- level lifecycle code

Work:

1. Add `EntityId`, `RenderObjectId`, and stable immutable asset handles with explicit mappings and generation validation.
2. Implement the private entity registry, typed sparse-set `ComponentStorage<T>`, read/write queries, component schema IDs, and frozen-structure assertions.
3. Convert the initial transform, mesh-instance, visibility, camera, light, animation state, and cold editor metadata into data components while compatibility facades remain single-source views.
4. Add the owner-thread `EntityCommandBuffer`/world command commit and bounded sequenced world change journal; run all current mutation through it in serial mode.
5. Make transform evaluation explicit and remove lazy mutable cache writes from concurrent read paths.
6. Build RenderWorldDelta and RenderFrameDynamicData in packet-local arenas from committed journal/ECS state.
7. Create RenderWorld/RenderProxyRegistry and apply sequence-checked deltas.
8. Move editor selection and previous transform/skinning identity from vector positions to stable IDs.
9. Publish level begin/end generations as data.
10. Keep Renderer and ECS system execution serial while consuming the new contracts.
11. Include versioned camera, jitter, motion/depth convention, exposure, resolution, history-reset, and provider tags required by existing temporal/product paths.
12. Prove that multiple cataloged levels and optional asset handles can enter/leave the render world without changing packet ownership.

Delete:

- MeshInstanceSnapshot::mesh raw pointer
- raw vector indices as editor/render durable identity
- lazy transform cache mutation reachable from published read paths
- direct editor scene mutations for each migrated property/action
- the unused `Entity` orchestration container after stable world identity has replaced its intended role; separately simplify only `Component` hooks/owner state proven redundant by current camera/mesh consumers
- duplicate mutable `SceneCameras`/`SceneMeshes`/`SceneLighting` instance storage after each facade is backed by ECS components
- RendererSystemRoot reads of GameScene
- renderer cache invalidation through direct GameFramework event callbacks
- the old full GameSceneSnapshot-to-renderer boundary after parity

Exit criteria:

- renderer can consume a recorded/replayed packet stream without GameScene existing
- command/journal replay reaches the same world state independent of worker/completion order
- entity/component create/add/remove/destroy playback and sparse-set compaction preserve generations, schema identity, and query validity rules
- editor selection becomes stale safely after object deletion/reuse
- level unload/reload stress produces no stale-handle access
- serial visual output matches baseline
- packet memory and build time are measured
- classic TLAS/PTLAS inputs, reservoir-light identity, temporal reset, and reference accumulation remain correct in serial packet-consumer mode

Learning evidence:

- ownership/lifetime state diagram
- acquire/release publication test
- stale generation rejection demo

### Stage 2A — Transactional Asynchronous Scene Loading

Goals:

- keep runtime/editor responsive during registry, manifest, and scene-payload loading
- make cancellation, failure containment, deterministic assembly, and atomic world replacement concrete
- prepare the future residency/streaming path without implementing world partition

Code areas:

- `LevelManager` and level lifecycle results
- `SceneAssetManager`, registry, manifest/payload loaders and appenders
- GameFramework world/scene-generation task scope
- Editor operation service and level UI state

Work:

1. Split level resolution from scene construction; capture an immutable request descriptor on the owner thread.
2. Publish an immutable scene-asset registry/catalog generation for concurrent resolution.
3. Build read, validate, per-asset decode/translate, deterministic assemble, and package-validation task stages with weighted memory limits.
4. Translate current level/scene payloads into deterministically ordered `EntityBlueprint` component records with stable authored identity and schema versions.
5. Return an immutable `SceneLoadPackage` tagged with request, catalog, and document/world generations.
6. Bulk-create ECS entities/components through typed world commands only after generation validation; retain the old scene until acceptance.
7. Add latest-request-wins cancellation, bounded progress publication, and deterministic diagnostics.
8. Marshal lifecycle notifications to the owner thread after state transitions; document existing `Event` as thread-affine.
9. Connect accepted CPU asset generations to the later residency/upload state machine without blocking a worker or frame.

Delete:

- destructive clear-before-load level replacement
- mutable `SceneAssetManager` loaded-ID bookkeeping as a synchronization/lifetime mechanism
- worker-callable inline level callbacks
- any temporary synchronous fallback after editor/runtime parity and failure tests pass

Exit criteria:

- cancelling/superseding a large load never commits stale state
- invalid/missing content leaves the active scene and renderer generation usable
- serial and parallel load modes produce the same IDs, object order, payload, and diagnostics
- repeated load/cancel/reload/document-close stress has no leak, callback-after-destroy, or partial publication
- editor frame latency stays within the explicit interactive budget while background work is limited

Learning evidence:

- load DAG and ownership timeline
- cancellation race and stale-generation demonstration
- before/after main-thread loading hitch capture

### Stage 2B — Data-Oriented ECS System Graph and Editor Model

Goals:

- replace whole-scene mutable controller access with declared resource ownership
- schedule typed ECS query ranges through SparkleTasks with declared component read/write hazards
- make animation/pose/morph/transform the first measured GameFramework parallel path
- remove live editor traversal/mutation of concurrently produced world state

Code areas:

- `GameScene`, `GameSceneController`, camera and Showcase controllers
- `SceneAnimations`, pose/morph evaluators, `SceneMeshes`, `Transform`
- new private GameFramework ECS registry/storage/queries and system graph/views/commands
- editor scene model, selection, outliner, inspectors, actions, transactions

Work:

1. Register current camera, Showcase motion, animation, morph, transform, and extraction behavior as systems with typed component queries, phase, and non-ECS resource declarations.
2. Execute the graph serially and prove state/render parity before enabling workers.
3. Pre-resolve clip/skeleton and morph targets per asset generation; preallocate stable output slots.
4. Partition leading component pools into coarse query ranges and parallelize independent movement/animation instances with exclusive writes and deterministic merge; retain serial thresholds.
5. Commit transform/morph outputs at the declared boundary and publish immutable matrices/bounds.
6. Build `WorldReadView` and incremental `EditorSceneModel` from the committed journal.
7. Convert all current outliner/inspector/material-variant edits to stable-ID editor commands and owner-thread transaction results.
8. Bind system runs, scene loading, searches, and previews to application/world/editor-document task scopes.

Delete:

- `GameSceneController::Update(GameScene&, ...)` and temporary legacy adapter
- camera/raw mesh pointers or indices retained across scene generations
- nested clip-to-skeleton and morph-to-mesh scans from the steady-state update
- lazy `Transform` write-on-read behavior
- remaining virtual per-entity `Initialize`/`Update`/`Render` traversal as a runtime scheduling mechanism
- editor `GameScene*` host service, index selection, and direct `SceneObjectActions` mutation after panel migration

Exit criteria:

- graph compiler rejects cycles and conflicting/undeclared resource writes
- active queries are invalidated only at structural commit and component pools pass sparse/dense invariant stress tests
- serial executor and 1/2/N worker results match for fixed input sequences
- current camera navigation, Showcase PTLAS motion, animation, morphs, editor visibility/property changes, and undo/redo remain correct
- worker tasks cannot resize world containers, broadcast events, call UI, or retain mutable scene storage
- measured animation-heavy work improves without regressing the normal small scene beyond budget

Learning evidence:

- resource-hazard graph and deterministic merge walkthrough
- transform cache race before/after explanation
- editor command/read-publication lifetime trace

### Stage 3 — Dedicated Render Coordinator and Bounded Frame Queue

Goals:

- overlap game/editor frame production with renderer CPU work
- make renderer/RHI thread ownership real

Code areas:

- Engine/Application runtime/editor loops
- Engine/Renderer facade
- new Renderer/Private/Threading
- window/swapchain integration
- editor viewport and UI packet creation

Work:

1. Move RendererSystemRoot, FramePipeline, RHI creation/destruction, submission, and present to RenderThread.
2. Add bounded `RenderFrameQueue` slots with clear publication/reuse transitions.
3. Add sequenced render commands for resize, settings, capture, shader generation, and shutdown.
4. Copy/convert ImGui draw output into EditorRenderPacket.
5. Migrate only required editor consumers to narrow immutable product results and delete broader observation routes made redundant by the thread boundary.
6. Implement threaded one-ahead, threaded zero-ahead, and serial-consumer modes.
7. Move screenshot/readback requests, presentation, and external provider calls through sequenced render-thread ownership.
8. Preserve GPU markers, object names, debug layers, timestamps, and fatal API checks on their owning backend thread.

Delete:

- direct editor calls that mutate renderer internals
- live ImGui draw-data pointers crossing the boundary
- direct Renderer::Prepare/Record/Submit host-thread ownership as the product path

Exit criteria:

- main/game and render CPU tracks overlap in captures
- bounded backpressure works under an artificial slow-render delay
- clean resize/minimize/restore/shutdown on D3D12 and Vulkan
- serial and threaded output parity
- no routine main-thread render completion wait
- screenshot/BMP capture completes without device idle and provider/present sequencing remains valid

Learning evidence:

- frame pipeline trace showing N+1/N/N-1
- latency and throughput comparison for zero/one-ahead modes

### Stage 4 — Persistent GPU Scene and Retirement

Goals:

- stop recreating/uploading unchanged scene-wide arrays
- remove device-idle scene/reload synchronization

Code areas:

- FramePipeline::BuildFrameContext / BuildRenderSceneGpuData
- SceneData builders/caches
- ray-tracing scene and acceleration planning
- RHI upload service and resource retirement
- shader package/runtime generation ownership

Work:

1. Add persistent GPU slots and dirty-range update planning.
2. Upload static raster/RT mesh payloads once per asset generation.
3. Update transforms, lights, skinning, morph, and dirty materials through frame upload rings.
4. Coalesce dirty ranges and emit copy/compute update passes.
5. Swap shader runtime generations at frame boundaries and retire old PSOs/resources by tokens.
6. Replace level cache clears with logical proxy removal plus deferred resource retirement.
7. Preserve separate minimal classic TLAS and PTLAS build/update/trace paths over stable proxies and asset generations.
8. Use existing allocator/budget pressure when growing GPU-scene buffers, upload pages, and acceleration-structure storage.

Delete:

- full per-frame RT vertex/index/material reuploads
- ordinary level-change WaitForIdle
- ordinary shader-reload WaitForIdle
- redundant snapshot comparison caches that deltas make obsolete

Exit criteria:

- static camera scene produces near-zero structural GPU-scene upload
- per-frame upload bytes scale with changed dynamic state
- repeated hot reload/level swap with frames in flight is validation-clean
- no GPU use-after-free under delayed queue completion stress
- classic TLAS and PTLAS initial build, update, add/remove, replacement, and trace tests pass where each backend reports support

Learning evidence:

- before/after upload/resource-allocation graphs
- fence/token retirement walkthrough

### Stage 5 — Renderer Preparation Task Graph

Goals:

- parallelize proven data-independent CPU preparation
- make dependencies and merge order explicit

Code areas:

- RenderSceneDataBuilder decomposition
- animation/skinning packet preparation where selected
- visibility, batching, lighting, and RT planners
- private task profiler adapter using existing hooks

Work:

1. Split monolithic builders into pure task functions with explicit inputs/outputs.
2. Parallelize transforms/bounds, visibility, lighting, skinning, mesh classification, and RT instance planning at coarse measured grains.
3. Merge into stable sort/batch order.
4. Keep graph setup/compile serial initially; add frame-graph-private topology reuse only if existing scopes prove compile is material and invalidation can be bounded by a direct key.
5. Add serial thresholds and workload-size telemetry.
6. Preserve deterministic reservoir-light ordering, path/reference accumulation/reset, temporal/provider inputs, and RT instance identity through task-local outputs and stable merges.

Delete:

- monolithic mutable builder entry points once all call sites use graph outputs
- shared scratch vectors in renderer system objects

Exit criteria:

- outputs match serial mode
- no task mutates renderer global caches
- speedup is positive on representative large scenes and does not regress small scenes beyond budget
- critical-path and worker-utilization traces are readable
- CPU speedup does not change histories, sample indices, light/object identity, provider tags, or RT selection

Learning evidence:

- DAG visualization with timings
- Amdahl/grain-size analysis

### Stage 6 — Parallel Frame-Graph Recording

Goals:

- use frame-graph dependencies to record eligible command streams concurrently
- prove native D3D12/Vulkan ownership and state correctness

Code areas:

- FrameGraphCompiler/Executor
- RHI command contexts
- D3D12 and Vulkan command list/pool implementations
- upload/descriptor services
- PipelineStateManager and PassBinder
- individual render passes

Work:

1. Add recording policies/groups and compiled entry/exit state contracts.
2. Add per-worker/per-frame/per-queue contexts for both backends.
3. Add worker-local upload and transient descriptor pages.
4. Prewarm pass runtime/PSO state before recording fan-out.
5. Audit and opt in passes, beginning with independent compute/copy and large draw passes.
6. Add intra-pass chunking where pass-level parallelism is insufficient.
7. Join and submit on render coordinator in compiled order.
8. Compare serial/parallel barrier and image results.
9. Keep provider, frame-generation/presentation, screenshot/readback, and any unaudited native-interoperability work as explicit serial islands.
10. Verify debugger markers/object names survive recording-group boundaries and command-list splitting.

Delete:

- single mutable NextSlot recording assumption as the only path
- lazy runtime creation reachable from parallel execute
- shared hot-path uniform allocation cursor

Exit criteria:

- native validation and frame-graph validation pass on both backends
- command allocator/pool ownership assertions withstand randomized scheduling
- GPU submission order and output match serial mode
- CPU record time improves in draw/pass-heavy scenes
- tiny scenes automatically remain serial/grouped
- barrier count, descriptor occupancy, command-list count, transient pressure, GPU time, and RT behavior do not regress beyond an explicitly justified budget

Learning evidence:

- D3D12 and Vulkan allocator/pool ownership diagram
- serial versus parallel command recording trace
- explanation of why GPU queue concurrency is separate

### Stage 7 — Editor, Tools, and Reliability Closure

Goals:

- ensure the architecture survives real editor workflows and long-running tools
- remove compatibility debt

Work:

1. Stress viewport recreation, docking, play/edit transitions, shader reload, level reload, capture, minimize, and shutdown.
2. Consolidate task, frame-graph, GPU-scene, upload, and retirement evidence into existing profiler/allocator hooks and delete superseded diagnostic snapshots or reports.
3. Finish deterministic transactional cook publication.
4. Route cancellation and progress through the existing owned tool workflow; do not add a renderer/task diagnostic panel.
5. Audit every remaining standard/Qt/native thread, mutex, atomic, future, wait/detach, callback registry, queue/allocator lock, and `WaitForIdle` call site; reconcile LC-01 through LC-18 and all later discoveries to zero unclassified mechanisms.
6. Remove legacy host-render and snapshot paths.
7. Make asset cooking/package fan-out start from project catalogs, preserve the curated in-repo level set, and support optional heavyweight content packs.
8. Keep only owned build/cook/run/clean/package/inspection launcher paths; delete diagnostic or package variants without consumers.
9. Validate runtime, editor, symbols, development-tools, and optional-content packages only where each package is intentionally owned.
10. Finish the private EditorOperationService migration for current load, recook, import/cook, preview, search, save, and package-validation workflows; expose workflow state rather than scheduler internals.
11. Validate application/world/editor-document scope shutdown with operations at every read/decode/publish/commit stage.

Exit criteria:

- no dual architecture remains
- all background operations use declared lanes
- all mutable cross-thread objects have documented ownership
- automated stress suite runs repeatedly without deadlock, leak, race symptom, or validation error
- default workflows emit product artifacts rather than timing/debug reports, and multiple cataloged levels remain supported

### Stage 8 — Performance and Portfolio Release

Goals:

- tune only with representative evidence
- present the design and its limits professionally

Work:

1. Tune worker count, task grain, recording groups, upload page sizes, and pipeline depth.
2. Capture CPU/GPU timelines and p50/p95 data across backends and core counts.
3. Update the existing concise product overview; do not create another policy or architecture document.
4. Record a deterministic demo and source walkthrough.
5. Publish limitations and future work.
6. Perform the late consolidated D3D12/Vulkan workload review using existing frame-graph, allocator, descriptor, pipeline, shader-package, RT-build, timestamp, and debugger hooks.
7. Verify that every public API and every retained tool/task primitive has a current product consumer.

Exit criteria:

- results are reproducible from documented commands/settings
- gains are reported per subsystem, not as one unexplained FPS number
- correctness modes and backend parity are visible

### Stage 9 — Expert Systems and Interview Readiness Closure

Goals:

- close the gap between a working engine architecture and expert concurrency diagnosis
- prove the additional concepts with real Sparkle workloads or bounded internal labs
- demonstrate AMD/NVIDIA-relevant CPU/GPU performance reasoning without résumé-only subsystems

Work:

1. Harden `RenderFrameQueue`/task-slot atomic protocols with mutex/SC/acquire-release references, generation-wrap stress, cancellation, and shutdown.
2. Characterize physical/logical-core-like worker policies, SMT/cache-domain behavior, oversubscription, ready time, migrations, false sharing, and priority inversion using optimized builds and system traces.
3. Implement serial and parallel reduction, scan/compaction, stable partition/bucketing, and deterministic merge in existing ECS/cooker/GPU-scene workloads.
4. Complete the staged I/O/decode/validate/commit/upload pipeline and cold-cache PSO/resource-creation policy with bounded concurrency and explicit late/miss/fallback behavior.
5. Correlate simulation, render, GPU queue, present, and provider frame identities; measure throughput, pacing, CPU lead, async-compute/copy overlap, and input-to-present latency.
6. Produce three concurrency incident reports from injected/reproduced failures using existing instrumentation plus appropriate external tools.
7. Complete the question bank, whiteboard, coding, trace-analysis, and architecture-defense drills without reading the answers.

Exit criteria:

- every new mechanism has a current Sparkle consumer or exists only in an existing test/benchmark surface
- no second scheduler, generic concurrent-container library, affinity subsystem, telemetry product, or interview-only runtime feature was added
- D3D12/Vulkan and AMD/NVIDIA captures support causal claims and explicitly record hardware/configuration limitations
- the owner can explain memory model, OS scheduling, CPU/GPU queue synchronization, frame latency, and production diagnosis under adversarial questioning

## Recommended Change Sets

The stages can be organized into reviewable change sets:

1. Task runtime core and tests
2. Private task profiler hooks, serial mode, and lanes
3. Texture/shader cooker pilots
4. Structured task scopes and generation/lifetime stress tests
5. Stable entity/render IDs, sparse-set ECS storage/query kernel, command commit, transform evaluation, and change journal
6. Transactional async level/scene load and editor operation result seam
7. Serial ECS-aware GameFramework system graph, component/facade conversion, and controller migration
8. Parallel animation/pose/morph path and deterministic merge
9. Immutable editor scene model, stable selection, and command/transaction conversion
10. Render world delta and dynamic packet
11. Serial renderer conversion to new data contract
12. Bounded `RenderFrameQueue` and render coordinator
13. Editor UI/viewport packet conversion
14. Persistent GPU scene
15. Deferred scene/shader generation retirement
16. Renderer preparation DAG
17. D3D12 worker recording contexts
18. Vulkan worker recording contexts
19. Frame-graph recording groups and first parallel passes
20. Draw-heavy intra-pass parallelism
21. Reliability, metrics, and legacy deletion
22. Atomic protocol and scheduler pathology hardening
23. CPU topology, worker policy, and false-sharing characterization
24. Reduction/scan/compaction integration in real data paths
25. Staged streaming and cold-cache PSO/resource creation
26. GPU queue concurrency, pacing, and correlated latency
27. Production forensic labs and expert interview defense

Each change set should compile and run both backends where it touches shared rendering behavior.

## Verification Strategy

### Task Runtime Unit and Stress Tests

Functional:

- single task
- fan-out/fan-in
- diamond dependency
- nested task generation
- reusable graph with distinct run contexts
- external TaskEvent completion
- priority/lane selection
- serial executor parity

Lifetime:

- stale `TaskNodeHandle`/`TaskExecution` generation
- task captures destroyed input rejected by ownership policy tests
- run destroyed only after children settle
- scheduler shutdown with idle, queued, running, cancelled, and event-blocked work
- repeated scheduler create/destroy in one process

Race/stress:

- millions of small dependency transitions
- randomized yield points
- 1/2/N workers
- simultaneous external submissions
- cancellation racing task start/completion
- failure racing fan-in
- queue wraparound and pool generation wrap policy
- repeated idle/sleep/wake transitions with submissions racing worker parking
- graph compile rejection for cycles, self/duplicate/foreign edges, and configured capacity overflow
- development assertions for worker waits, invalid cross-lane dependencies, and stale/ABA-style handle reuse

Performance:

- enqueue/execute overhead
- local versus stolen task cost
- idle wake latency
- parallel_for crossover grain
- task-record pool high-water mark

### GameFramework, Loading, and Editor Concurrency Tests

ECS storage and queries:

- entity create/destroy/reuse preserves generation and free-list invariants through wrap-policy stress
- add/remove/replace components preserves sparse-to-dense and dense-to-sparse round trips after randomized swap removal
- single/multi-component include/exclude queries match a slow reference model
- const queries cannot obtain writable component access; declared write views cover only their partition
- structural operations during a frozen epoch assert; stale query/storage versions are rejected
- task-local temporary entities remap correctly within their command buffer and cannot escape before playback
- serial and shuffled deterministic command playback produce identical entity/component state
- component schema IDs and serialized order remain stable across build/configuration where promised
- hot component iteration allocation count, bytes touched, cache behavior, and crossover grain are measured against the replaced object path

World identity and commands:

- create/destroy/reuse invalidates stale `EntityId` generations
- deterministic command merge under randomized scheduling and worker counts
- declared read/read systems overlap; read/write and write/write hazards order or reject as specified
- graph compile rejects cycles, duplicate IDs, impossible phase dependencies, and undeclared guarded-test writes
- command conflict, stale-target, and deliberate ordered-writer policies return the expected typed result
- change-journal replay and full-baseline-plus-deltas reach identical world state
- a lagging journal consumer resynchronizes without reading reclaimed storage

Animation and transforms:

- serial/parallel playback, pose, skinning matrix, morph weight, and committed transform outputs match within defined floating-point policy
- clip/skeleton/morph target lookup remains correct across asset-generation replacement
- randomized task completion does not change output order or IDs
- small workloads take the serial path; large workloads use bounded preallocated ranges
- no concurrent read causes a lazy `Transform` cache write
- future hierarchy tests, when hierarchy exists, reject cycles and preserve parent-before-child evaluation

Scene loading and scope lifetime:

- serial/parallel scene package bytes, assigned IDs, and deterministic diagnostics match
- cancel before read, during decode, during fan-in, after package publication, and while commit is queued
- newer request, world generation, or editor-document generation rejects an older completion
- missing/corrupt registry, manifest, mesh, material, animation, or skeleton leaves the old scene active
- memory-weighted limiter holds peak decoded/pending-upload bytes within budget
- application/world/document destruction settles child tasks; raw-owner capture assertions fire in negative tests
- lifecycle events execute only on their owner thread and observe a fully committed state

Editor model and commands:

- deleting/reordering/reloading objects safely invalidates or preserves stable selection as appropriate
- outliner and inspector read a pinned immutable generation while the next generation publishes
- transform/light/sky/camera/visibility/material-variant commands round-trip through commit results
- continuous edit coalescing and undo/redo produce deterministic semantic commands
- repeated search/preview/reload uses latest-generation-wins without callback-after-close
- editor remains interactive while load/cook work runs and background work cannot starve frame tasks

### Frame Packet and Render World Tests

- release/acquire producer-consumer publication
- packet storage poisoned after acknowledgement to catch retention
- stale scene generation rejected
- create/update/destroy sequence replay
- duplicate/out-of-order delta diagnostics
- stable ID reuse only after generation changes
- capture packet stream and replay into headless RenderWorld
- randomized level load/unload while render consumer is delayed
- static scene produces no structural delta

### Renderer Correctness Tests

- serial versus threaded renderer reference images
- serial versus parallel command recording images
- deterministic pass/submission ordering
- frame-graph entry/exit state comparison
- D3D12 debug layer and GPU-based validation
- Vulkan validation layers and synchronization validation
- artificial worker migration and delay
- command context lease misuse assertions
- upload/descriptor overflow path
- PSO prewarm completeness
- resize/minimize/device-loss policy
- frames-in-flight level and shader generation retirement
- raster, RT, PTLAS, async compute, copy upload, UI, and capture paths

### Advanced Feature Preservation Tests

Ray tracing:

- classic TLAS and PTLAS capability truth per backend
- initial build, update, add/remove, BLAS replacement, compaction where owned, trace, and deferred destruction
- shader table/pipeline generation coherence during hot reload
- serial/threaded and serial/parallel-recording parity

Lighting and temporal:

- stable light/object/reservoir identity independent of worker completion order
- camera cut, teleport, resolution, level generation, feature, material, and light-topology reset cases
- reference path sample/seed/accumulation parity
- motion/depth/jitter/exposure/history tags match zero-ahead and one-ahead modes

Shader and providers:

- serial/parallel HLSL/Slang cook produces the same DXIL/SPIR-V package identities and reflection/layout contracts
- one coherent shader/pipeline generation per recording run
- provider instance/thread-affinity assertions
- supported/enabled/operational/failure states under resize, reload, missing dependency, and backend capability changes
- provider resource tags and frame indices remain correct with a lagging render consumer

Capture and debugging:

- screenshot/BMP capture under normal, resized, minimized, cancelled, and bounded-queue conditions
- no device-idle capture path
- marker/object-name/timestamp continuity across split recording groups
- D3D12 and Vulkan native debugger/validation workflow remains usable

### Tool Correctness Tests

- byte-identical serial/parallel deterministic outputs
- stable manifest/registry ordering
- duplicate texture input deduplication
- one task failure leaves old publication active
- cancellation leaves no accepted partial generation
- compiler/importer thread-safety limiter
- process cancellation and pipe drain
- high-memory texture concurrency limit
- clean incremental cache hit/miss behavior
- curated-level and optional-content-pack deterministic cook/package behavior
- runtime/editor/symbols/development-tools package contents where owned

### Reviewability and Deletion Gates

- public-header audit shows no worker queue, task timing, cache, or recording-pool observation API
- include/dependency checks preserve GameFramework → Renderer → RHI and keep SparkleTasks below consumers
- shipping configuration has no research-only tasks, packet fields, panels, logs, or default reports
- every change set’s preservation and deletion ledgers are closed
- no legacy future/thread/snapshot/host-render path remains after its replacement stage
- no new architecture/policy document is required to operate or extend the system

### Concurrency Tooling

Use complementary tools:

- compiler/thread sanitizers where the platform/toolchain supports them
- Visual Studio concurrency and CPU profiling
- PIX CPU/GPU events for D3D12
- RenderDoc and Vulkan validation/synchronization validation
- Nsight Graphics/Systems where useful
- AMD uProf, Radeon GPU Profiler, and GPUView/WPA where useful
- Application Verifier, guard allocators, and intentional packet poisoning
- debugger/crash-dump workflows for blocked threads, invalid lifetimes, and post-mortem call stacks
- a CPU task timeline through the profiler/debugger instrumentation the engine already owns

Native graphics validation cannot find a C++ data race. Thread sanitization cannot validate GPU barriers. Both are required.

## Performance Evaluation

This section defines the late success evidence, not an instruction to build its own telemetry product. Stage 0 captures only the minimum before-state needed to avoid blind architectural change using existing scopes and tools. The full matrix is collected after the feature/data paths are stable by consolidating existing frame-graph, allocator, timestamp, debugger, and profiler hooks. Missing data is not permission to add a public snapshot, default CSV/JSON output, runtime log, or new profiler framework.

Results may be summarized manually in this existing program or the existing product overview. No engine-side benchmark/report generator is required.

### Required Metrics

CPU frame:

- main/game update
- packet extraction and publication
- main-thread backpressure time
- render delta application
- preparation graph critical path
- frame-graph setup and compile
- command recording
- render-thread joins
- submission/present

Task runtime:

- worker utilization
- runnable queue depth
- task count and median/p95 duration
- stolen/local ratio
- queue delay
- scheduler overhead
- blocking lane utilization
- cancellation/failure count
- physical/logical processor count, selected worker cap, and relevant cache/topology metadata
- context switches, migrations, ready/preempted time, wake count, and steal failures in focused system captures
- contention/false-sharing evidence for scheduler/frame-queue hot state
- third-party/internal worker counts during compiler/decode workloads

ECS/GameFramework:

- live entity count and per-component pool count/capacity
- structural commands and component add/remove/move count per frame
- query candidate/match count and leading-pool choice
- serial versus parallel system/query time and crossover grain
- sparse lookup/cache-miss behavior for representative multi-component queries using professional CPU profiling
- component storage bytes, padding, allocation/compaction high-water marks
- transform/animation entities processed and deterministic merge/commit time

Data/GPU scene:

- packet bytes and arena high-water mark
- structural delta count
- dynamic item count
- dirty ranges
- upload bytes
- resource creates/destroys
- deferred-retirement bytes and age

Recording:

- groups/lists per queue
- serial-island time
- per-worker recording time
- upload/descriptor page use
- PSO prewarm misses
- cold-cache PSO/resource create time, late/missed requests, compile concurrency, and peak compile memory

Late consolidated frame/RHI workload:

- passes and queue assignment
- resource classes, histories, transitions, barriers, aliasing, and cross-queue waits
- transient allocation high-water mark and allocator-backed memory budget/pressure
- descriptor occupancy/pressure
- pipeline and shader-package count
- GPU pass timestamps and BLAS/classic-TLAS/PTLAS build/update timings
- graphics/compute/copy queue overlap, signal/wait count, submission batches, queue idle gaps, and bandwidth-contention outcome
- D3D12/Vulkan capability and supported-feature comparison

End result:

- CPU and GPU frame p50/p95/p99
- FPS only as a derived presentation
- input-to-present comparison for pipeline depth modes
- simulation/render-submit/present/GPU stage latency, CPU frames queued ahead, pacing variance, and provider mode
- peak resident memory
- tool throughput and peak memory

### Benchmark Matrix

Use at least:

- a tiny scene that exposes scheduler overhead
- Sponza raster with shadows and representative post processing
- an animation-heavy scene such as CesiumMan multiplied to useful scale
- an ECS structural-churn case with deterministic spawn/destroy/add/remove commands, separate from normal gameplay measurements
- ray tracing/TLAS workload
- PTLAS path if supported by the current validation scene
- editor viewport with active UI
- shader full cook and changed-package cook
- texture cook batch with small images and memory-heavy 4K/8K HDR images
- multi-scene asset cook

Run:

- D3D12 and Vulkan
- serial and threaded renderer
- serial and parallel recording
- replaced object/facade reference where temporarily available, ECS serial, and ECS task-parallel modes during migration
- 1, 2, and N workers
- physical-core-like, logical-core-like, and measured capped worker policies where the host exposes them
- cold and warm caches
- CPU-bound, GPU-bound, VSync/present-mode, and supported provider/frame-generation variants for latency work
- release/profile build with validation separately

### How to Report Speedup

Report subsystem and end-to-end results:

~~~text
Preparation:  6.2 ms → 2.4 ms
Recording:    5.1 ms → 2.0 ms
Serial setup: 1.3 ms → 1.1 ms
Render CPU critical path: 12.6 ms → 5.8 ms
Main/render overlap reduces end-to-end CPU frame interval: ...
GPU frame: unchanged / changed because ...
~~~

Also report regressions:

- small-scene scheduler overhead
- packet-copy cost
- extra frame latency in one-ahead mode
- command-list count/driver overhead
- memory from buffered contexts and packet arenas

Do not claim linear core scaling. Explain the remaining serial fraction, imbalance, dependencies, driver behavior, and memory bandwidth.

### Initial Success Budgets

These are gates to refine after Stage 0, not promises:

- zero correctness/validation regressions
- small-scene parallel policy automatically falls back close to serial cost
- no routine device-idle waits in steady-state frame, scene change, or shader reload
- static-scene GPU upload materially reduced rather than merely redistributed
- large-scene renderer CPU critical path shows a clear, repeatable improvement
- offline batch outputs remain deterministic and throughput improves without uncontrolled memory growth

## Portfolio Presentation

### Persona Learning Outcomes

The program demonstrates H only when the evidence exists in code and captures:

| Persona pillar | Learn by doing | Required proof |
|---|---|---|
| Modern C++ concurrency | task lifetimes, atomics, acquire/release publication, semaphores, work stealing, cancellation, false sharing | scheduler tests and packet-publication stress |
| ECS and data-oriented design | generational entities, sparse-set pools, hot/cold components, typed queries, structural epochs, AoS/SoA judgment | storage/query invariants, replaced object path, cache/iteration comparison |
| Engine/game concurrency | system hazards, deferred mutation, stable identity, deterministic merges, animation/transform decomposition | serial/parallel world replay and animation-heavy scaling evidence |
| Asynchronous architecture | blocking-I/O separation, structured scopes, stale-generation rejection, transactional scene publication | cancel/fail/supersede level-load demonstrations with old scene retained |
| Editor concurrency | immutable read models, main-thread transactions, operation scopes, bounded progress/results | responsive load/cook demo, stable selection, undo/redo, document-close stress |
| Explicit D3D12/Vulkan ownership | per-worker allocator/pool/list/buffer ownership, barriers, queue tokens, deferred destruction | paired backend implementation and native validation |
| Renderer feature ownership | carry RT, reservoir lighting, temporal/provider, capture, and path/reference behavior through the new data/thread model | feature preservation matrix, not only a scheduler demo |
| Shader/kernel craft | deterministic parallel cook, immutable runtime generations, prewarmed layouts/PSOs, GPU-data layouts derived from shader access | DXIL/SPIR-V/reflection parity and shader-visible data review |
| GPU architecture thinking | distinguish CPU critical path, uploads, GPU queues, command-list overhead, bandwidth, descriptors, RT builds, and shader cost | late profiler/debugger captures with causal explanation |
| Neural rendering readiness | retain structured/tensor-like resource and specialization/profile expressiveness without adding a framework | ABI contract test and a future replacement-based feature gate |
| Debugging fluency | investigate races, state/barrier bugs, descriptor lifetime, provider affinity, and backend differences using professional tools | one documented issue reproduced and fixed with existing tools |
| Productization and judgment | bounded latency/memory, deterministic cooks, optional content, packages, small APIs, deletion of old paths | clean default workflow and closed deletion ledger |

Completing only SparkleTasks reaches concurrency-infrastructure evidence. Completing the packet, renderer, feature-preservation, native recording, and productization gates demonstrates an advanced graphics systems engineer.

### The Story

Present the work as seven connected problems:

1. The old frame was serial and the scene snapshot was not a true ownership boundary.
2. Sparkle introduced a tested task DAG and bounded execution lanes.
3. GameFramework became a bounded data-oriented ECS: stable entities, packed typed components, query systems, command buffers, and deterministic commits scheduled through the job system.
4. Scene loading became cancellable staged work with atomic publication, while the editor consumed immutable models and remained responsive.
5. Game/editor state became immutable packets, deltas, and render proxies.
6. The render thread, persistent GPU scene, and frame graph exposed safe parallel work.
7. D3D12/Vulkan worker-local command contexts enabled parallel recording with serial equivalence and measured results.

### Artifacts

Include:

- one architecture diagram
- one ownership table
- one before/after CPU/GPU timeline
- one preparation DAG capture
- D3D12 and Vulkan command-context diagrams
- development/test command-line controls for serial/parallel modes, not a permanent editor panel
- existing test-runner validation and stress output
- manually maintained benchmark tables with machine/core/backend/configuration, not a generated report format
- short sections on failed ideas and tradeoffs
- pinned primary-source study links in this internal study only

### Live Demonstration

A strong demo sequence:

1. run serial reference mode and show the CPU trace
2. start and cancel an asynchronous level load while the editor stays interactive and the old scene remains active
3. run the animation-heavy scene through serial and parallel GameFramework graphs and show identical committed output
4. enable render thread and show game/render overlap
5. enable renderer preparation tasks and show the DAG/worker utilization
6. enable parallel command recording and show ordered multi-list submission
7. switch D3D12/Vulkan
8. trigger shader recook/reload without device idle
9. reload a level with frames in flight
10. change worker count and show stable output
11. show deliberately invalid system-resource and pass declarations rejected by safety assertions

The last item demonstrates understanding better than a perfect happy path: the system knows what is unsafe and explains why.

### Honest Limitations to Publish

Likely initial limitations:

- one render coordinator, not a separate RHI translation thread
- frame-graph setup/compile mostly serial, with topology caching
- only audited passes parallel-recorded
- no fibers/coroutine suspension
- background I/O uses blocking threads rather than a platform async-I/O backend
- scheduler is optimized for one process/NUMA node
- material/GPU-scene compaction may be deferred

Being explicit about these boundaries strengthens the engineering case.

## Risk Register

| Risk | Symptom | Mitigation |
|---|---|---|
| Too much simultaneous change | Long branch, hard regressions | Vertical stages, serial parity, delete old path per stage |
| Scheduler becomes a side project | Months spent on deque details | Bound features; tool pilot; correctness-first queues |
| ECS becomes a second side project | Reflection/prefab/archetype framework grows before current migration | Private sparse-set minimum, explicit non-goals, current components only, deletion per slice |
| DOD makes code fragmented without gains | Many tiny components and sparse gathers obscure ownership | Hot/cold split from access captures; keep cohesive values together; serial comparison |
| ECS storage invalidation bug | Rare stale references after add/remove/compaction | Frozen epochs, versioned views, randomized sparse/dense invariant tests |
| Cross-thread lifetime bug | Rare crash on unload/reload | Stable handles, packet poison, generations, retirement stress |
| One-frame latency is unacceptable | Input feels delayed | Bounded depth, zero-ahead mode, measure input-to-present |
| Parallel tasks increase small-scene time | More overhead than work | Serial thresholds and grouping |
| Driver dislikes many command lists | CPU/GPU regression | Measured group size; backend policy; serial fallback |
| PSO/shader cache races | Hitches/crashes in record tasks | Prewarm immutable runtime view |
| Upload/descriptor contention | Workers serialize on allocator | Local pages and high-water diagnostics |
| Tool memory explosion | OOM on HDR/scene batch | Weighted concurrency budgets |
| Nondeterministic output | Flaky tests/cache misses | Stable IDs/sorts/merge/publication |
| Oversubscription | Low utilization and high latency | Lane budgets; expose external/internal thread counts in explicit configuration/captures |
| Render thread waits on workers too long | Critical path unchanged | DAG profiling, load balance, larger task grains |
| Vulkan/D3D behavior diverges | Backend-only bugs | Common recording contract, native validation, paired milestones |
| Legacy path remains indefinitely | Two architectures drift | Explicit deletion gates |

## Definition of Done

This program is complete when all of the following are true:

### Architecture

- GameScene and editor mutable state are never dereferenced by the render thread.
- GameScene structural mutation has one owner-thread commit point; workers use immutable views, exclusive output ranges, or typed commands.
- stable generational world IDs replace durable vector indices across editor selection, commands, animation targets, journals, and extraction.
- GameFramework instance state uses the private ECS registry and typed component pools; heavyweight assets remain immutable handle-addressed resources.
- the ECS enforces frozen update epochs, deterministic deferred structural commands, and sparse/dense/query invariants.
- the GameFramework system graph declares phases, component queries/access, and non-ECS resource reads/writes and is executable through the serial scheduler.
- committed transform/world-matrix data is immutable to readers; no published `const` read lazily mutates cache state.
- the bounded world change journal drives incremental renderer/editor publication and has a defined lagging-consumer resync path.
- RenderFramePacket contains only packet-owned values or stable immutable handles.
- RendererSystemRoot and mutable RHI state have one render-coordinator owner.
- The packet queue is bounded and supports serial, zero-ahead, and one-ahead modes.
- Scene structure uses sequenced deltas and stable generational IDs.
- The GPU scene persists static state and updates dirty/dynamic ranges.
- The frame graph remains the only render scheduler/barrier authority and the RHI remains explicit.
- SparkleTasks has no renderer, RHI, provider, scene, or product-policy dependency.
- Public task/renderer seams contain no worker/cache/timing observation API.
- planned concurrency symbols use the canonical crosswalk: no permanent owned-source alias remains for `TaskExecution`, `TaskNodeHandle`, `RenderCoordinator`, `RenderFrameQueue`, `RenderControlCommandQueue`, or `RhiCommandRecordingLease`; physical thread labels describe roles that actually exist.

### Tasks

- SparkleTasks supports prerequisites, fan-in, nested work, cancellation, failure, lanes, private capture-time instrumentation, and serial execution.
- Normal worker tasks do not block on other tasks.
- No generic busy-wait/help loop executes unrelated tasks while waiting; idle and dependency waits park through a lost-wakeup-safe predicate protocol.
- FrameCritical and BlockingIo workloads cannot starve each other.
- Scheduler tests pass at 1/2/N workers and during repeated shutdown stress.
- application, world/document, asset-generation, frame, and tool scopes cancel and settle before their captured owners/storage are destroyed.
- `RenderFrameQueue`/task-slot atomic protocols have mutex/SC references, documented memory orders, generation-wrap/ABA tests, and separate reuse/reclamation proof.
- worker policy is measured on physical/logical-core-like configurations; topology metadata, oversubscription, ready time, migrations, false sharing, and third-party workers are accounted for.
- priority inversion, lost wake, thundering herd, worker-wait, cancellation, and shutdown failures have injected regression tests.

### Rendering

- Frame preparation separately closes applicable visibility/LOD, light classification, shadow-caster, retained/dynamic draw, sorting/instancing, skinning/morph, RT-input and GPU-scene dirty-planning use cases with serial equivalence.
- Frame-graph recording groups have explicit state and deterministic submission contracts.
- D3D12 allocators/lists and Vulkan command pools/buffers are worker-local and frame-retired.
- Recording group, command-buffer, and submission sizes are measured; neither tiny-list overhead nor giant-list underutilization is accepted by assumption.
- Preparation, native recording, optional software translation, aggregation, submission batching and queue submission have distinct owners/metrics; the accepted design retains no speculative software RHI stream or translation thread.
- Pass runtime/PSO state is prewarmed before parallel recording.
- Upload and transient descriptor allocations have parallel-safe ownership.
- Both backends pass native and engine validation in serial and parallel modes.
- Classic TLAS and PTLAS retain build/update/trace/lifetime behavior where each backend reports support.
- Native reservoir lighting, reference path accumulation/reset, and temporal/provider signals preserve stable identity and frame semantics.
- Shader package, reflection, parameter-layout, DXIL/SPIR-V, HLSL/Slang, and pipeline-generation invariants hold under parallel cook/reload/record.
- Provider instances obey explicit affinity/capability/failure rules and do not own renderer scheduling or scene data.
- Screenshot/BMP capture and native debugger/profiler hooks remain functional without routine device idle.
- CPU gains do not buy unjustified GPU barrier, descriptor, memory, pipeline, command-list, residency, or RT regressions.
- cold-cache PSO/resource creation has deduplication, concurrency/memory budgets, late/miss/fallback policy, and no recording-time lazy creation.
- Shader compilation, pipeline creation, buffer/image/view creation, allocation/binding, descriptor update, upload and readiness have separately audited stages on both native backends where applicable.
- graphics/compute/copy overlap is graph-derived and proven on queue timelines; extra queues are removed or kept serial when synchronization/bandwidth cost loses.
- simulation, render submission, GPU, present, and provider work share a correlated frame identity and bounded CPU-lead/latency policy.

### Editor and Tools

- ImGui and viewport data cross through owned/versioned packets.
- editor outliner/inspectors read an immutable stable-ID scene model and submit semantic commands; panels do not retain pointers into mutable GameScene storage.
- editor transaction, selection, window/input, and ImGui state remain editor-main-thread owned.
- current background editor workflows use one scoped operation service over SparkleTasks rather than ad-hoc futures or a second pool.
- Shader reload and level changes do not routinely wait for device idle.
- scene/level loading prepares an isolated deterministic package, supports cancellation/supersession, and atomically commits only after validation; failure keeps the previous scene active.
- animation/pose/morph/transform has measured parallel execution with deterministic serial equivalence and small-work serial thresholds.
- editor authoring/read models and renderer extraction use stable entity/component data without exposing ECS storage internals or maintaining a second mutable scene copy.
- shader, texture, and asset cook paths use task graphs where safe.
- tool publication is transactional and deterministic.
- cancellation and progress are observable and safe.
- runtime loading follows bounded read/decode/upload/residency states without frame-worker blocking.
- runtime loading distinguishes I/O completion from CPU tasks and exposes one internal staged seam for a future measured I/O backend without duplicating asset ownership.
- real ECS/cooker/GPU-scene paths demonstrate task-local reduction, scan/compaction or stable partition/merge where useful, with serial oracle and deterministic policy.
- curated multiple levels, optional heavyweight content packs, and owned runtime/editor/tools/symbols/content packages remain supported.
- launcher and cooker task use is restricted to owned product workflows.

### Evidence

- before/after profiler captures and benchmark tables exist
- correctness parity and stress results are reproducible
- CPU task concurrency, render pipelining, GPU queue concurrency, and frames in flight are explained separately
- limitations and tradeoffs are documented
- all replaced legacy paths are removed
- every MT-01–MT-44 hazard is closed by a concrete preventing pattern plus falsifying test/trace, or has a reviewable non-applicability reason
- source precedents remain traceable to official NVIDIA, AMD/GPUOpen, Epic, language, or API materials; vendor samples are never treated as standalone correctness/performance proof
- every change set closes its capability-preservation and replacement/deletion ledgers
- shipping defaults contain no research-only task/data path
- broad workload analysis is late and uses consolidated existing hooks
- no new profiler framework, default report format, task/debug panel, runtime log stream, or additional policy document was introduced
- public/product wording does not claim external SDK equivalence or imply unvalidated support
- the existing concise overview lets a reviewer locate the RHI contract, frame-graph/pass rules, shader source-to-package path, feature classification, and backend support without reading this entire study
- executable include/dependency/boundary checks cover the new Tasks, packet, Renderer, and RHI directions
- executable ownership tests cover GameFramework systems/commands/scopes and the editor read-model/operation boundary
- repository-wide canonical and rejected-alias searches are classified; C++ symbols, filenames, CMake, tests, comments, thread names, and profiler labels agree on the same responsibility vocabulary
- three concurrency incident reports demonstrate race/lifetime, deadlock/stall, and scaling/latency diagnosis with exact reproduction, trace, root cause, fix, regression and post-fix measurement
- AMD/NVIDIA-relevant captures include optimized-build CPU scheduling, D3D12/Vulkan recording, AMD GPU queue/barrier behavior where hardware is available, and NVIDIA/system CPU-GPU timelines where available; unavailable hardware is stated, never simulated as proof
- the owner can complete the expert question bank, bounded-queue atomic coding drill, task-DAG analysis, native recording review, and trace diagnosis without relying on memorized slogans

## Immediate Next Implementation Slice

The first code change after accepting this design should be Stage 0 plus the smallest part of Stage 1:

1. add canonical thread-role/`FrameId`/`SceneGeneration` vocabulary, the planned-name reconciliation ledger, and CPU timing points
2. record current D3D12/Vulkan baselines
3. scaffold SparkleTasks with serial executor and dependency graph tests
4. add the fixed worker executor, cancellation, and private profiler/debugger instrumentation
5. replace ShaderRecookCoordinator’s std::async as the first integration
6. add structured application/world/tool scopes and owner-destruction stress tests
7. begin Stage 2 with stable `EntityId`, owner-thread command commit, and explicit transform evaluation in serial mode

Do not start by moving RendererSystemRoot to a new thread or by parallelizing arbitrary `GameSceneController` calls. The task runtime, structured lifetimes, world identity/commit seam, and cross-thread render data contract need independent serial tests before renderer or gameplay lifetime becomes concurrent.

## Local Repository Audit Trail

These are the principal Sparkle sources inspected for this study. They should be revisited at the corresponding migration gate.

| Source | What it establishes |
|---|---|
| [RuntimeApplication.cpp](../../../Engine/Application/Private/RuntimeApplication.cpp) | GameScene update and Renderer::OnRender are sequential on the application thread |
| [EditorApplication.cpp](../../../Engine/Application/Private/EditorApplication.cpp) | Renderer prepare/record, editor UI, and submit are one ordered host-thread path |
| [GameScene.cpp](../../../Engine/GameFramework/Private/Scene/GameScene.cpp) | Controller, animation, morph, and snapshot phases are currently serial |
| [GameSceneController.h](../../../Engine/GameFramework/Public/Scene/GameSceneController.h), [GameCameraController.cpp](../../../Engine/GameFramework/Private/Scene/Camera/GameCameraController.cpp), and [ShowcaseSceneController.cpp](../../../Projects/Showcase/Src/ShowcaseSceneController.cpp) | Controllers receive mutable whole-scene access and retain raw pointers/vector indices across lifecycle changes |
| [SceneAnimations.cpp](../../../Engine/GameFramework/Private/Scene/Animations/SceneAnimations.cpp), [SceneAnimationPoseEvaluator.cpp](../../../Engine/GameFramework/Private/Scene/Animations/SceneAnimationPoseEvaluator.cpp), and [SceneMorphWeightApplicator.cpp](../../../Engine/GameFramework/Private/Scene/Meshes/SceneMorphWeightApplicator.cpp) | Animation/pose/morph work is serial, allocates per evaluation, scans targets, and contains natural per-instance private-output boundaries |
| [Transform.cpp](../../../Engine/GameFramework/Private/Scene/Transform.cpp) and [CameraComponent.h](../../../Engine/GameFramework/Public/Scene/Camera/CameraComponent.h) | `const` getters lazily mutate world/direction caches, so nominal parallel reads are not race-free |
| [EntityId.h](../../../Engine/GameFramework/Public/World/EntityId.h), private `World/ECS` storage, and [Component.h](../../../Engine/GameFramework/Public/Scene/Component.h) | Prompt 05 replaced the unused owning `Entity` with generational identity and private sparse storage. Camera/mesh compatibility still consumes the simplified visibility/destructor-only `Component`; Prompt 07 must migrate and delete that remainder |
| [LevelManager.cpp](../../../Engine/GameFramework/Private/Level/LevelManager.cpp), [SceneAssetManager.cpp](../../../Engine/GameFramework/Private/Assets/SceneAssetManager.cpp), and [SceneAssetPayloadLoader.cpp](../../../Engine/GameFramework/Private/Assets/SceneAssetPayloadLoader.cpp) | Level replacement clears first and then performs synchronous registry/manifest/payload loading into mutable manager state |
| [Event.h](../../../Engine/Core/Public/Events/Event.h) | Callback storage and inline broadcast are unsynchronized and therefore an owner-thread primitive, not a worker notification channel |
| [SceneObjectSelection.h](../../../Engine/Editor/Public/Scene/SceneObjectSelection.h), [SceneObjectActions.cpp](../../../Engine/Editor/Private/Scene/SceneObjectActions.cpp), and [SceneMeshInspector.cpp](../../../Engine/Editor/Private/Panels/SceneMeshInspector.cpp) | Editor selection uses vector indices and panels directly traverse/mutate live GameScene state during ImGui construction |
| [GameSceneSnapshot.h](../../../Engine/GameFramework/Public/Scene/GameSceneSnapshot.h) and [MeshSnapshot.h](../../../Engine/GameFramework/Public/Scene/Meshes/MeshSnapshot.h) | Most data is copied, but mesh snapshots still carry raw mesh identity/lifetime |
| [FramePipeline.cpp](../../../Engine/Renderer/Private/FramePipeline/FramePipeline.cpp) | Snapshot capture, render-scene build, GPU uploads, graph setup/compile/execute, and submission form the current renderer critical path |
| [RendererSystemRoot.cpp](../../../Engine/Renderer/Private/Host/RendererSystemRoot.cpp) | Renderer root owns caches/services and retains direct GameScene-facing responsibilities |
| [RenderSceneDataBuilder.cpp](../../../Engine/Renderer/Private/SceneData/Builders/RenderSceneDataBuilder.cpp) | Mesh, material-facing, skinning, batching, temporal, and lighting preparation is monolithic and vector-position-sensitive |
| [SceneRenderStateCoordinator.cpp](../../../Engine/Renderer/Private/SceneData/Lifecycle/SceneRenderStateCoordinator.cpp) | Level lifecycle currently calls device idle and directly clears render state |
| [FrameGraphExecution.cpp](../../../Engine/Renderer/Private/FrameGraph/Execution/FrameGraphExecution.cpp) and [FrameGraphSubmissionExecutor.cpp](../../../Engine/Renderer/Private/FrameGraph/Execution/FrameGraphSubmissionExecutor.cpp) | Compiled queue batches exist, while CPU pass recording/submission traversal is serial |
| [PipelineStateManager.h](../../../Engine/Renderer/Private/Pipeline/PipelineStateManager.h) | GetPassRuntime can lazily mutate type-indexed runtime/PSO storage |
| [PassBinder.cpp](../../../Engine/Renderer/Private/Pipeline/PassBinder.cpp) | Pass recording allocates uniform constants from the RHI upload service |
| [D3D12CommandContext.cpp](../../../Engine/RHI/Private/D3D12/Commands/D3D12CommandContext.cpp) | Command allocator/list slots are selected through one mutable frame/queue context |
| [VulkanCommandContext.cpp](../../../Engine/RHI/Private/Vulkan/Commands/VulkanCommandContext.cpp) | Command buffers share a frame/queue command-pool context unsuitable for concurrent recording |
| [ShaderRecookCoordinator.cpp](../../../Engine/Application/Private/ShaderRecook/ShaderRecookCoordinator.cpp) and `ShaderRecookExecutionService.*` | Coordinator owns request/publication/reload policy; the private service owns scoped Background/BlockingIo execution. Accepted reload still waits for RHI idle until Prompt 16 |
| [InputSystem.h](../../../Engine/Platform/Public/Input/InputSystem.h) and [InputSystem.cpp](../../../Engine/Platform/Private/Input/InputSystem.cpp) | The callback registry is mutex-protected, but arbitrary callbacks are invoked while the mutex is held; re-entrant unsubscribe/register is a concrete legacy-quality hazard |
| [Logger.cpp](../../../Engine/Core/Private/Diagnostics/Logger.cpp) and [Timer.h](../../../Engine/Core/Public/Time/Timer.h) | Existing mutex/atomic infrastructure must be kept only with explicit registry/publication/independent-flag invariants; it is not automatically a task-system migration target |
| [AssetCookerDispatcher.cpp](../../../Tools/Cooking/AssetCooker/Private/Dispatch/AssetCookerDispatcher.cpp) | Cook stages and per-scene imports/builds execute serially |
| [AssetCookerToolProcess.cpp](../../../Tools/Cooking/AssetCooker/Private/Dispatch/AssetCookerToolProcess.cpp) | Outer CLI and scheduled callers share the cancellable Core child-process contract; scheduled ownership selects BlockingIo |
| [TextureCookRequestBatchProcessor.cpp](../../../Tools/Cooking/TextureCooker/Private/Cooking/TextureCookRequestBatchProcessor.cpp) and `TextureCookBatchExecutor.*` | Processor owns request/diagnostic/publication policy; the private executor owns bounded tasks, task-local COM/cooker contexts, stop-token bridging, and weighted admission |
| [ShaderPackageCooker.cpp](../../../Tools/Shaders/ShaderCompiler/Private/Cooking/ShaderPackageCooker.cpp) and `ShaderCookPlanExecutor.*` | Cooker remains plan/fan-in/emission policy; the private executor runs bounded task-local compiler sessions and returns node-indexed results |
| [LauncherBackend.cpp](../../../Tools/Launcher/SparkleLauncher/Private/Gui/App/LauncherBackend.cpp), private operation services, and [ProcessRunner.cpp](../../../Tools/Launcher/SparkleLauncher/Private/Core/ProcessRunner.cpp) | Backend owns Qt preview/delivery only; scoped operation lifetime and Core process-tree/pipe mechanics are private. Remaining detach transfers lifetime only to a replacement launcher or selected product |
| [StreamlineRuntimeSupport.cpp](../../../Engine/Renderer/Private/Streamline/StreamlineRuntimeSupport.cpp) | One global mutex protects provider lifecycle/device/presentation state and is held across external SDK calls; future coordinator affinity must narrow this boundary |
| [D3D12CommandQueue.cpp](../../../Engine/RHI/Private/D3D12/Commands/D3D12CommandQueue.cpp) and [VulkanCommandQueue.cpp](../../../Engine/RHI/Private/Vulkan/Commands/VulkanCommandQueue.cpp) | Submission mutexes serialize native queues today; Vulkan external synchronization can remain necessary, but the target engine contract is one coordinator submission owner and no worker GPU waits |
| [D3D12DescriptorAllocator.cpp](../../../Engine/RHI/Private/D3D12/Descriptors/D3D12DescriptorAllocator.cpp), [VulkanDescriptorAllocator.cpp](../../../Engine/RHI/Private/Vulkan/Descriptors/VulkanDescriptorAllocator.cpp), and [D3D12LinearAllocator.cpp](../../../Engine/RHI/Private/D3D12/Resources/D3D12LinearAllocator.cpp) | Shared locks/atomics protect current allocation, but parallel recording requires lifetime-split persistent state and token-safe worker-local transient pages instead of hot shared cursors |
| [D3D12GpuMemoryAllocator.cpp](../../../Engine/RHI/Private/D3D12/Memory/D3D12GpuMemoryAllocator.cpp), [VulkanGpuMemoryAllocator.cpp](../../../Engine/RHI/Private/Vulkan/Memory/VulkanGpuMemoryAllocator.cpp), and [VulkanRhi.cpp](../../../Engine/RHI/Private/Vulkan/Device/VulkanRhi.cpp) | Record/pending-release and validation-callback locks are legitimate boundaries that need narrow snapshot/drain semantics and lifetime tests, not blind deletion |
| [FramePipeline.cpp](../../../Engine/Renderer/Private/FramePipeline/FramePipeline.cpp), [VulkanRenderDeviceServices.cpp](../../../Engine/RHI/Private/Vulkan/Device/VulkanRenderDeviceServices.cpp), and [VulkanSwapChain.cpp](../../../Engine/RHI/Private/Vulkan/SwapChain/VulkanSwapChain.cpp) | Resize can currently reach multiple nested device-idle drains; the migration must count native drains per operation and converge on one owner/token policy |

## Primary Source Trail

The following links are the direct source/documentation trail used for the architectural comparison.

### NVIDIA

- [Donut ThreadPool interface at bc1ea24](https://github.com/NVIDIA-RTX/Donut/blob/bc1ea24b0486f1c00d89327fe16c0b4dd11c5937/include/donut/engine/ThreadPool.h)
- [Donut ThreadPool implementation at bc1ea24](https://github.com/NVIDIA-RTX/Donut/blob/bc1ea24b0486f1c00d89327fe16c0b4dd11c5937/src/engine/ThreadPool.cpp)
- [Donut persistent scene material/geometry/instance buffers and dirty-state interface at bc1ea24](https://github.com/NVIDIA-RTX/Donut/blob/bc1ea24b0486f1c00d89327fe16c0b4dd11c5937/include/donut/engine/Scene.h)
- [nvpro_core `parallel_batches`/`parallel_ranges` vocabulary at bc19d6a](https://github.com/nvpro-samples/nvpro_core/blob/bc19d6ac3ef62938d0ea0e099735878457ce1b6e/nvh/parallel_work.hpp)
- [NVRHI Programming Guide at 8e8c36e](https://github.com/NVIDIA-RTX/NVRHI/blob/8e8c36e37558acec333204619b95d9d2fcdc4a79/doc/ProgrammingGuide.md)
- [NVRHI `ICommandList`, `CommandQueue`, execution, wait, and lifetime-tracker vocabulary at 8e8c36e](https://github.com/NVIDIA-RTX/NVRHI/blob/8e8c36e37558acec333204619b95d9d2fcdc4a79/include/nvrhi/nvrhi.h)
- [Advanced API Performance: CPUs](https://developer.nvidia.com/blog/advanced-api-performance-cpus/)
- [Limiting CPU Threads for Better Game Performance](https://developer.nvidia.com/blog/limiting-cpu-threads-for-better-game-performance/)
- [Vulkan Dos and Don'ts](https://developer.nvidia.com/blog/vulkan-dos-donts/)
- [Advanced API Performance: Command Buffers](https://developer.nvidia.com/blog/advanced-api-performance-command-buffers/)
- [Advanced API Performance: Pipeline State Objects](https://developer.nvidia.com/blog/advanced-api-performance-pipeline-state-objects/)
- [Parallel Shader Compilation for Ray Tracing Pipeline States](https://developer.nvidia.com/blog/parallel-shader-compilation-ray-tracing-pipeline-states/)
- [nvpro_core ring resources, command pools, batching, and per-thread allocator guidance at bc19d6a](https://github.com/nvpro-samples/nvpro_core/blob/bc19d6ac3ef62938d0ea0e099735878457ce1b6e/nvvk/README.md)
- [NVIDIA Nsight Systems](https://developer.nvidia.com/nsight-systems)
- [Nsight Systems User Guide: scheduling, context switches, waits, OS runtime and I/O traces](https://docs.nvidia.com/nsight-systems/UserGuide/index.html)
- [NVIDIA stdexec structured execution vocabulary at 711da59](https://github.com/NVIDIA/stdexec/tree/711da5971a8e8e940763c11bf6bbeb1c1bb22c3a)
- [NVIDIA Reflex](https://developer.nvidia.com/performance-rendering-tools/reflex)
- [Streamline Reflex/PCL programming guide](https://github.com/NVIDIA-RTX/Streamline/blob/main/docs/ProgrammingGuideReflex.md)

### AMD

- [FidelityFX SDK Cauldron2 TaskManager interface at 60f4ea8](https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK/blob/60f4ea81909200d8542eca14dccb2628b763a9a3/Kits/Cauldron2/dx12/framework/core/taskmanager.h)
- [FidelityFX SDK Cauldron2 TaskManager implementation at 60f4ea8](https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK/blob/60f4ea81909200d8542eca14dccb2628b763a9a3/Kits/Cauldron2/dx12/framework/core/taskmanager.cpp)
- [AMD RPS sample `RpsAfxThreadPool`, `Job`, and `WaitHandle` vocabulary at f3330f5](https://github.com/GPUOpen-LibrariesAndSDKs/RenderPipelineShaders/blob/f3330f5306d15af8529a310f6255225c864b0961/tools/app_framework/afx_threadpool.hpp)
- [AMD RPS render-graph, command-batch, record-info, command-buffer, and callback-context vocabulary at f3330f5](https://github.com/GPUOpen-LibrariesAndSDKs/RenderPipelineShaders/blob/f3330f5306d15af8529a310f6255225c864b0961/include/rps/runtime/common/rps_runtime.h)
- [Legacy AMD Cauldron thread pool at b92d559](https://github.com/GPUOpen-LibrariesAndSDKs/Cauldron/blob/b92d559bd083f44df9f8f42a6ad149c1584ae94c/src/common/Misc/threadpool.h)
- [AMD Ryzen CPU Performance Guide](https://gpuopen.com/learn/ryzen-performance/)
- [AMD CPU Core Count Detection on Windows](https://gpuopen.com/learn/cpu-core-count-detection-windows/)
- [AMD RDNA Performance Guide](https://gpuopen.com/learn/rdna-performance-guide/)
- [AMD Radeon Cauldron scene/data/backend separation](https://gpuopen.com/radeon-cauldron-new-sdk-framework/)
- [AMD Porting Detroit Part 1: indexed resource arrays and batched primitives](https://gpuopen.com/learn/porting-detroit-1/)
- [AMD Driver Experiments: detecting command allocator synchronization bugs](https://gpuopen.com/learn/rdts-driver-experiments/)
- [AMD RPS multithreading tutorial](https://gpuopen.com/learn/rps-tutorial/rps-tutorial-part4/)
- [AMD RPS Vulkan multithreading implementation test at f3330f5](https://github.com/GPUOpen-LibrariesAndSDKs/RenderPipelineShaders/blob/f3330f5306d15af8529a310f6255225c864b0961/tests/gui/test_multithreading_vk.cpp)
- [AMD RPS D3D12 multithreading implementation test at f3330f5](https://github.com/GPUOpen-LibrariesAndSDKs/RenderPipelineShaders/blob/f3330f5306d15af8529a310f6255225c864b0961/tests/gui/test_multithreading_d3d12.cpp)
- [AMD RPS shared graph/range/thread-count test contract at f3330f5](https://github.com/GPUOpen-LibrariesAndSDKs/RenderPipelineShaders/blob/f3330f5306d15af8529a310f6255225c864b0961/tests/gui/test_multithreading_shared.hpp)
- [Porting Detroit: deterministic multithreaded render lists and per-thread Vulkan pools](https://gpuopen.com/learn/porting-detroit-3/)
- [Porting Detroit: pipeline-cache, creation, and batching lessons](https://gpuopen.com/learn/porting-detroit-1/)
- [AMD profiling-tool selection guide](https://gpuopen.com/learn/amd-lab-notes/amd-lab-notes-profilers-readme/)
- [Radeon GPU Profiler](https://gpuopen.com/rgp/)
- [Understanding RGP and GPUView queue graphs](https://gpuopen.com/learn/understanding-graphs-in-radeon-gpu-profiler-and-gpuview/)
- [Leveraging asynchronous queues for concurrent execution](https://gpuopen.com/learn/concurrent-execution-asynchronous-queues/)
- [FidelityFX frame-interpolation swapchain](https://gpuopen.com/manuals/fidelityfx_sdk/techniques/frame-interpolation-swap-chain/)
- [FidelityFX Vulkan frame-interpolation queue contract](https://gpuopen.com/manuals/fidelityfx_sdk/fidelityfx_sdk-struct_vkframeinterpolationinfoffx/)

### Epic Games

- [Tasks System](https://dev.epicgames.com/documentation/en-us/unreal-engine/tasks-systems-in-unreal-engine)
- [Tasks System API references (`FTaskHandle`, `FTaskEvent`, `FPipe`)](https://dev.epicgames.com/documentation/en-us/unreal-engine/tasks-system-references-in-unreal-engine)
- [`ParallelFor` API and blocking/grain warnings](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Core/ParallelFor)
- [Threaded Rendering](https://dev.epicgames.com/documentation/en-us/unreal-engine/threaded-rendering-in-unreal-engine)
- [Parallel Rendering Overview](https://dev.epicgames.com/documentation/en-us/unreal-engine/parallel-rendering-overview-for-unreal-engine)
- [Mesh Drawing Pipeline](https://dev.epicgames.com/documentation/unreal-engine/mesh-drawing-pipeline-in-unreal-engine?application_version=5.7)
- [`FRenderCommandList`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RenderCore/FRenderCommandList)
- [`FRHICommandListExecutor`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RHI/FRHICommandListExecutor)
- [`QueueAsyncCommandListSubmit`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RHI/FRHICommandListImmediate/QueueAsyncCommandListSubmit)
- [`IRHICommandContext`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RHI/IRHICommandContext)
- [`FDynamicRHI::RHIGetParallelCommandContext`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RHI/FDynamicRHI/RHIGetParallelCommandContext)
- [Task Graph Insights](https://dev.epicgames.com/documentation/en-us/unreal-engine/task-graph-insights-in-unreal-engine-5)
- [Timing Insights](https://dev.epicgames.com/documentation/en-us/unreal-engine/timing-insights-in-unreal-engine-5)
- [Epic Unreal Tasks source path—official repository access required](https://github.com/EpicGames/UnrealEngine/blob/release/Engine/Source/Runtime/Core/Public/Tasks/Task.h)
- [Epic Unreal ParallelFor source path—official repository access required](https://github.com/EpicGames/UnrealEngine/blob/release/Engine/Source/Runtime/Core/Public/Async/ParallelFor.h)
- [Epic Unreal rendering-thread source path—official repository access required](https://github.com/EpicGames/UnrealEngine/blob/release/Engine/Source/Runtime/RenderCore/Public/RenderingThread.h)
- [Render Dependency Graph](https://dev.epicgames.com/documentation/en-us/unreal-engine/render-dependency-graph-in-unreal-engine)
- [Asynchronous Level Loading](https://dev.epicgames.com/documentation/unreal-engine/asynchronous-level-loading-in-unreal-engine?lang=en-US)
- [PSO Precaching](https://dev.epicgames.com/documentation/unreal-engine/pso-precaching-for-unreal-engine)
- [Shader Development and asynchronous Shader Compile Workers](https://dev.epicgames.com/documentation/unreal-engine/shader-development-in-unreal-engine?lang=en-US)
- [Unreal Swarm static-lighting distribution](https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-swarm-in-unreal-engine)
- [MassEntity Overview](https://dev.epicgames.com/documentation/unreal-engine/overview-of-mass-entity-in-unreal-engine?lang=en-US)
- [Epic graphics programming overview: game/render ownership and renderer-private scene state](https://dev.epicgames.com/documentation/en-us/unreal-engine/graphics-programming-overview-for-unreal-engine)
- [Epic transient `FMassEntityView` lifetime contract](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/MassEntity/FMassEntityView)

### O3DE

- [AzCore TaskGraph at 68683f2](https://github.com/o3de/o3de/blob/68683f23fb747380d3efa2424bd5f30242e9c5a2/Code/Framework/AzCore/AzCore/Task/TaskGraph.h)
- [AzCore TaskExecutor at 68683f2](https://github.com/o3de/o3de/blob/68683f23fb747380d3efa2424bd5f30242e9c5a2/Code/Framework/AzCore/AzCore/Task/TaskExecutor.h)
- [Atom FrameScheduler at 68683f2](https://github.com/o3de/o3de/blob/68683f23fb747380d3efa2424bd5f30242e9c5a2/Gems/Atom/RHI/Code/Include/Atom/RHI/FrameScheduler.h)
- [Atom ScopeProducer at 68683f2](https://github.com/o3de/o3de/blob/68683f23fb747380d3efa2424bd5f30242e9c5a2/Gems/Atom/RHI/Code/Include/Atom/RHI/ScopeProducer.h)
- [Atom DX12 CommandListPool at 68683f2](https://github.com/o3de/o3de/blob/68683f23fb747380d3efa2424bd5f30242e9c5a2/Gems/Atom/RHI/DX12/Code/Source/RHI/CommandListPool.h)
- [Atom Vulkan CommandListAllocator at 68683f2](https://github.com/o3de/o3de/blob/68683f23fb747380d3efa2424bd5f30242e9c5a2/Gems/Atom/RHI/Vulkan/Code/Source/RHI/CommandListAllocator.h)
- [Atom ThreadLocalContext at 68683f2](https://github.com/o3de/o3de/blob/68683f23fb747380d3efa2424bd5f30242e9c5a2/Gems/Atom/RHI/Code/Include/Atom/RHI/ThreadLocalContext.h)

### Google Filament Sources

- [Filament JobSystem interface at 398d13c](https://github.com/google/filament/blob/398d13c4abd148ea6e139b05e5418efa37d284d3/libs/utils/include/utils/JobSystem.h)
- [Filament JobSystem implementation at 398d13c](https://github.com/google/filament/blob/398d13c4abd148ea6e139b05e5418efa37d284d3/libs/utils/src/JobSystem.cpp)

### ECS and Data-Oriented Storage Sources

- [Richard Fabian, *Data-Oriented Design* online edition](https://www.dataorienteddesign.com/dodbook/)
- [Richard Fabian: data type, frequency, quantity, shape, probability, consumers, and access questions](https://www.dataorienteddesign.com/dodbook/node2.html)
- [Unity ECS samples at 6786a74](https://github.com/Unity-Technologies/EntityComponentSystemSamples/tree/6786a741ee1f118ed14cecfa02beae8e926937b0)
- [Unity entity command-buffer guidance at 6786a74](https://github.com/Unity-Technologies/EntityComponentSystemSamples/blob/6786a741ee1f118ed14cecfa02beae8e926937b0/EntitiesSamples/Docs/entity-command-buffers.md)
- [EnTT registry at 1333fa5](https://github.com/skypjack/entt/blob/1333fa53129e7cfded5a9640c4336a254049917b/src/entt/entity/registry.hpp)
- [EnTT sparse set at 1333fa5](https://github.com/skypjack/entt/blob/1333fa53129e7cfded5a9640c4336a254049917b/src/entt/entity/sparse_set.hpp)

### Microsoft

- [DirectX 12 Multithreading sample overview at 357ade6](https://github.com/microsoft/DirectX-Graphics-Samples/blob/357ade6ec6ff0d9dcadc48f35c7a28e37c0cdf7a/Samples/Desktop/D3D12Multithreading/readme.md)
- [D3D12Multithreading FrameResource at 357ade6](https://github.com/microsoft/DirectX-Graphics-Samples/blob/357ade6ec6ff0d9dcadc48f35c7a28e37c0cdf7a/Samples/Desktop/D3D12Multithreading/src/FrameResource.cpp)
- [D3D12 command queue and command-list design](https://learn.microsoft.com/en-us/windows/win32/direct3d12/design-philosophy-of-command-queues-and-command-lists)
- [`ExecuteCommandLists` batching and ordering contract](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12commandqueue-executecommandlists)
- [D3D12 recording command lists and bundles](https://learn.microsoft.com/en-us/windows/win32/direct3d12/recording-command-lists-and-bundles)
- [DirectStorage samples](https://github.com/microsoft/DirectStorage)
- [Windows CPU analysis with WPA](https://learn.microsoft.com/en-us/windows-hardware/test/wpt/cpu-analysis)
- [Windows CPU Sets](https://learn.microsoft.com/en-us/windows/win32/procthread/cpu-sets)
- [Windows thread Quality of Service](https://learn.microsoft.com/en-us/windows/win32/procthread/quality-of-service)

### Language and API Specifications

- [C++ multi-threaded executions and data races](https://eel.is/c++draft/intro.races)
- [C++ atomic ordering](https://eel.is/c++draft/atomics.order)
- [C++ parallel algorithms](https://eel.is/c++draft/algorithms.parallel)
- [Vulkan threading behavior and external synchronization](https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#fundamentals-threadingbehavior)

## Final Recommendation

Approve the architecture as one program with three non-negotiable ordering rules:

1. ownership and packet lifetime before the render thread
2. persistent data before parallelizing scene rebuild work
3. native per-worker recording ownership and pass audits before parallel frame-graph execute

Sparkle already has the rendering abstractions and explicit GPU scheduling needed to make this credible. The pivotal step is to turn implicit serial ordering into explicit ownership, generations, dependencies, and retirement—and then show, with serial equivalence and measurements, that the resulting concurrency is both correct and useful.

## Prompt 00 Verified Before-State and Invariants (2026-07-18)

This is the evidence gate for every later prompt. It records the tree as inspected for Prompt 00; it is not a claim that the target architecture exists. Owned search roots were `Engine`, `Tools`, and `Projects`; generated, build, `External`, and `ThirdParty` trees were excluded from ownership while wrapped SDK behavior remains listed. Governing requirements A, E, G, H, this document, and Prompt 00 in K were re-read before changing code.

### Current serial ownership and frame order

There is no render thread or engine task executor. `RunRuntimeApplication` labels and uses `Sparkle.GameThread`; `RunEditorApplication` labels and uses `Sparkle.EditorThread`. On that same host thread:

1. `RuntimeApplication::BeginFrame` advances input/window/deferred events and level transitions.
2. `RuntimeApplication::UpdateRuntime` calls `GameScene::Update`.
3. `Renderer::OnRender` calls prepare, record, and submit serially.
4. Prepare ticks the timer, captures `GameSceneSnapshot`, performs scene/texture preparation, updates `RenderCamera`, and creates frame data.
5. Record builds renderer scene/GPU data, prepares history and RT state, then runs frame-graph `Setup`, `Compile`, and `Execute`.
6. `FrameGraphSubmissionExecutor` traverses compiled batches deterministically, records/submits each batch, and the backend presents/advances the buffered frame.

The editor inserts recook coordination after begin-frame, then performs game update, viewport request, prepare/record, ImGui/viewport presentation, and submit on `Sparkle.EditorThread`. `RendererState` and `RenderDeviceServicesState` keep implementation pointers private and expose only owner-checked `Systems`/`Pipeline`/`Backend` gateways. They reuse Core's small `OwnerThread` invariant and report the caller through `std::source_location`; facade methods contain no repeated assertion strings and public headers do not expose the mechanism. This documents the current contract without making underlying objects thread-safe.

The critical unsafe future boundary remains `GameSceneSnapshot::MeshInstanceSnapshot::Mesh`, a raw pointer into game/asset lifetime. A vector-valued snapshot is not yet a detached immutable render packet. Prompts 12 and 15 own that correction before Prompt 13 may move renderer ownership.

### Naming and deletion ledger

| Current/planned term | Decision | Exact owner and reason |
|---|---|---|
| `RhiSubmissionToken`, `ERhiQueueType`, `RenderCommandList`, `FrameGraphSubmissionBatch` | **KEEP** | Existing strong GPU completion, GPU queue, recording, and compiled-submit vocabulary; never duplicate with fence/job aliases |
| `FrameGraphPassNode` | **KEEP** | GPU graph node, not CPU `Task`; subsystem qualification prevents collision |
| CLI `CommandRegistry`; launcher `OperationRecord` | **KEEP** | Product command/operation, neither render control command nor `TaskExecution` |
| raw timer/frame counters and ambiguous `FrameIndex` | **RENAME IN OWNING PROMPT** | Prompt 12 introduces `FrameId`; Prompt 28 separates buffered slot and temporal/provider indices |
| missing scene/level generation type | **ADD, DO NOT ALIAS** | `SceneGeneration` in Prompts 12/15; reject `WorldVersion`/`SceneRevision` substitutes |
| request/publication IDs where monotonic order is meant | **RENAME IN OWNING PROMPT** | `SequenceNumber` in Prompts 04/12/16 |
| `std::async`, operation `QThread`, pipe-reader `std::thread` used as scheduling | **DELETE AS ALIASES** | Prompts 04/22 replace them with scoped `TaskExecution`/BlockingIo; a physical thread remains a thread |
| `Job`, `JobSystem`, `ThreadPool`, `WorkItem`, `TaskRun` | **DELETE/REJECT** | Canonical CPU vocabulary is `Task`, `TaskExecutor`, `TaskScope`, `TaskEvent`, `TaskExecution`; Prompt 01 owns implementation |
| `RendererSystemRoot` as serial owner | **RENAME/REHOME IN OWNING PROMPT** | Prompt 13 establishes `RenderCoordinator`; reject temporary `RenderThreadManager` |
| planned cross-owner frame data | **ADD, DO NOT ALIAS** | `RenderFramePacket`/`RenderFrameQueue` in Prompts 12/13; packet owns values/handles through queue lifetime |
| planned recording ownership | **ADD, DO NOT ALIAS** | `RhiCommandRecordingLease` in Prompts 18/19; context/list reference is not exclusive-ownership proof |
| `FenceHandle`, `GpuTask`, `CommandExecution` | **DELETE/REJECT** | Reuse `RhiSubmissionToken`; CPU `TaskExecution` never means GPU execution |
| `Sparkle.RHIThread`/`RHIThread` | **DELETE/REJECT** | No such owner; submission remains with `RenderCoordinator` |

Reserved truthful roles are `Sparkle.GameThread`, `Sparkle.EditorThread`, `Sparkle.RenderThread`, `Sparkle.Task.FrameCritical.N`, `Sparkle.Task.Background.N`, `Sparkle.Task.BlockingIo.N`, and `Sparkle.ToolMain`. Prompt 00 emits only existing roles: game, editor, tool main, `Sparkle.Tool.ShaderRecook`, `Sparkle.Tool.Operation`, and `Sparkle.Tool.ProcessOutput`. Prompt 13 first emits render; Prompts 01/02 first emit task workers. `RHIThread` is not emitted.

### Observation scope and launch controls

The current tree has GPU markers/timestamps, not a general CPU profiler API. `FrameGraphSubmissionExecutor` already emits `GPU Frame/<Queue>/Batch N` and pass scopes, with D3D12 PIX command-list events and Vulkan debug labels. No current cross-backend CPU hook covers game update, snapshot, scene/GPU-data build, graph setup/compile, submit, present, or idle waits. Prompt 00 therefore does **not** invent a profiler framework or pretend GPU markers measure CPU ownership. Capture CPU intervals in WPA/Nsight Systems and GPU work in PIX/Nsight Graphics/RGP; Prompt 05 owns any deliberately reviewed integration. This is explicit MT-43 measurement debt.

Five CVars live together in the dedicated private `Concurrency/ConcurrencyLaunchCVars.cpp` settings unit; the generic command-line parser remains feature-agnostic. Defaults preserve the serial before-state and none has a consumer yet:

| Control | Default | First semantic owner |
|---|---:|---|
| `task.WorkerCount` | `0` | Prompt 01/02; 0 selects measured default, explicit 1/2/N are experiments |
| `task.SerialExecution` | `true` | Prompt 02 serial oracle |
| `r.ThreadedRenderer` | `false` | Prompt 13 owner transition |
| `r.ParallelCommandRecording` | `false` | Prompt 20 comparison |
| `r.RenderPipelineDepth` | `0` | Prompt 13; initially only synchronous 0 and bounded-ahead 1 |

Accepted spellings are `--cvar=name=value`, `--cvar name=value`, and `--set-cvar name=value`. Switch spelling is case-insensitive; registry names are canonical and case-sensitive. Invalid/unknown assignments leave values unchanged. There is no shipping UI or persisted setting.

### Regenerated owned-concurrency ledger

The current exhaustive search reconfirmed LC-01 through LC-18 and found no second task runtime. New `std::thread::id` fields and `SetThreadDescription` calls are affinity evidence, not schedulers. Qt string/list `.join()` hits are semantic false positives.

| IDs / current paths and users | Owner, invariant, blocking/affinity/lifetime policy | Disposition, closing prompt, falsifier |
|---|---|---|
| LC-01 `ShaderRecookCoordinator.{h,cpp}`: scoped task execution plus editor update/reload | Application scope owns Background/BlockingIo work; editor owner accepts request/publication generations | Prompt 04 process path closed; tokenized reload/idle removal Prompt 16; destroy/cancel/stale publication and zero idle audit |
| LC-02 launcher `LauncherBackend` plus private operation mapping/execution | bounded host executor owns operations; Qt UI stays GUI-owner; queued immutable messages close cross-thread delivery | Prompt 04 closed operation QThreads; Prompt 22 repeats cancel/close/restart and Qt affinity checks |
| LC-03 Core `ChildProcessWindows.cpp` through launcher `ProcessRunner` | BlockingIo caller owns child job, descendants, overlapped pipe, cancellation event, and result; no reader thread/polling/atomic cancel family | Prompt 04 closed; Prompt 27 hardens launch/read/exit faults and worker-stack policy |
| LC-04 `AssetCookerToolProcess.cpp` through Core child process | CLI host may call synchronously; scheduled caller must use BlockingIo; event/cancel wait is not an uninterruptible native wait | Prompt 04 closed mechanism; Prompt 27 call graph and cancellation equivalence remain falsifiers |
| LC-05 `InputSystem.{h,cpp}` callback mutex | protects callback vectors, but callback executes while held; self-unsubscribe reacquires same mutex | known failure; Prompt 03 chooses owner/snapshot policy, Prompt 22 closes; watchdog re-entry test |
| LC-06 `Logger.cpp`: registry/sink mutexes and level/initialized atomics | Core logging owns cross-thread state; release/acquire publishes initialization, relaxed level is independent | keep/harden Prompt 24; concurrent init/get/set/shutdown and lock-scope audit |
| LC-07 `Timer.{h,cpp}` relaxed pause atomic | communicates only pause value; publishes no adjacent timer state | prove/remove Prompt 24; writer/reader call graph and independent-flag test |
| LC-08 `StreamlineRuntimeSupport.cpp` global mutex | lifecycle/render owner should own state; external Streamline calls currently occur under lock | narrow/owner Prompts 13/16/28; SDK re-entry, failure/shutdown and lock-held-call audit |
| LC-09 D3D12 `CommandQueue.{h,cpp}` submission/CPU-wait mutexes and events | serializes native submit/fence state and host waits; infinite waits only at reuse/flush/shutdown | owner-only Prompts 13/18, tokens Prompt 16; wrong-thread submit and delayed/hung GPU tests |
| LC-10 Vulkan `CommandQueue.{h,cpp}` native queue mutex/timeline wait | Vulkan external synchronization is legitimate; coordinator remains sole engine submit/present authority | keep/narrow Prompts 13/19/28; validation and provider-sharing tests |
| LC-11 D3D12 `DescriptorAllocator.{h,cpp}` mutex | protects persistent allocation/free; cannot become recording-hot shared allocation | keep persistent/split transient Prompt 18; contention and reset-before-token tests |
| LC-12 Vulkan `DescriptorAllocator.{h,cpp}` mutex | registry/write/retire/recycle share one lock; serial owner masks lifetime partitions | split Prompt 19; pool validation, stale handle, no hot lock-held native update |
| LC-13 D3D12 `LinearAllocator.{h,cpp}` offset/high-water atomics | CAS owns unique ranges; relaxed high-water is diagnostic; GPU-safe reset is external | local leases/token reset Prompt 18, memory-order audit Prompt 24; overlap/contention/delayed-GPU tests |
| LC-14 D3D12/Vulkan `GpuMemoryAllocator.cpp` record mutexes | allocator owns records/pending release/diagnostics; no callback/destruction under record lock | keep/harden Prompts 16/18/19/24; delayed GPU and diagnostic re-entry stress |
| LC-15 `VulkanRhi.{h,cpp}` diagnostics mutex | concurrent driver callback appends owned messages; owner drains | keep/harden Prompt 24; bounded callback/drain and no re-entry under lock |
| LC-16 every renderer/RHI `WaitForIdle` | classified individually below; wrappers are mechanisms, never reasons | remove data-path waits Prompts 13-16, teardown Prompt 22; native idle-call counter/call-chain test |
| LC-17 launcher shadow/product `startDetached` | generation-specific replacement launcher and selected runtime/editor intentionally outlive the current launcher; no timed handoff remains | Prompt 04 timing path closed; Prompt 22 restart/close stress and explicit independent-lifetime audit |
| LC-18 D3D12 fence and Vulkan timeline/device waits | host/frame-slot owner may block only for reuse/flush/shutdown; never future task worker | Prompts 13/18/19/28; delayed/hung GPU and worker-stack audit |

Wrapped third-party concurrency is capacity, not Sparkle ownership: DXC/Slang, Compressonator/texture codecs, Assimp/importers, Streamline/NGX/provider callbacks, drivers, and Qt may create workers. Prompts 24/25/27 capture total runnable threads, bound outer concurrency, and prevent N outer tasks spawning uncontrolled inner pools.

### Lock-order and callback-under-lock record

| Boundary | Current order/finding | Closing rule/evidence |
|---|---|---|
| Input | `m_CallbackMutex -> arbitrary callback`; self-unsubscribe attempts the same non-recursive mutex | Prompt 03 snapshot/invoke or owner-only; watchdog is falsifier |
| Logger | registry mutex protects logger state; each sink has its own mutex | Prompt 24 keeps them non-nested and stresses initialization/sinks; logging is not a scheduled task |
| Streamline | `g_streamlineMutex -> sl::*` external calls | treat external call as re-entry capable; Prompts 13/28 use two-phase transition or prove boundary |
| D3D12 queue | separate submission and CPU-wait mutexes; native wait must not hold unrelated locks | coordinator submit Prompt 13; wait/lock/hang proof Prompt 18/28 |
| Vulkan queue | `SubmissionMutex -> vkQueueSubmit/vkQueuePresent` for external synchronization | retain only native/provider sharing; Prompt 19 native validation |
| Descriptor allocators | allocator mutex protects persistent maps/pools; Vulkan native update may extend scope | Prompts 18/19 split recording-hot transient ownership and audit native calls |
| GPU allocation records | record mutex protects diagnostic/release collections | copy/swap under lock, process/destroy after unlock; Prompt 24 re-entry test |
| Vulkan diagnostics | driver callback logs separately then locks message append; owner drains under same mutex | Prompt 24 bounds ingestion and proves no driver callback/re-entry while locked |
| D3D12 linear allocator | CAS allocates disjoint offsets; relaxed high-water is diagnostic; reset lacks GPU token | Prompt 18 lease/token, Prompt 24 memory-order proof |

Until Prompt 24 publishes a formal total order: do not nest unrelated locks; do not wait, invoke callbacks, log, destroy owners, or call providers/drivers while an engine lock is held unless the ledger proves that native boundary.

Direct lifecycle callback inventory is also explicit: `SceneRenderStateCoordinator` subscribes raw-`this` callbacks to level-will-unload/changed and immediately mutates renderer state, including an idle wait; Prompts 12/13/15 replace this with owner-sequenced `SceneGeneration` commands and a destruction-before-dispatch test. `FramePipeline` subscribes raw `this` to window resize and sets a pending flag; Prompt 13 makes resize an owner command and tests callback-versus-destruction. `RuntimeConsoleOverlay`, `InputSystem`, and `GameCameraController` subscribe raw `this` to window/input events and retain `ScopedEventHandle` cleanup; Prompts 03/22 prove owner affinity, unsubscribe-before-destruction, and re-entry. No callback lifetime is considered safe merely because current serial shutdown order happens to remove its handle first.

### Complete current idle/wait call-site ledger

| Concrete call site | Classification/nested path | Single owner and exact replacement |
|---|---|---|
| shader recook `ReloadCookedShaders -> Renderer::WaitForIdle` | ordinary reload **data-path debt** | generation swap, Prompt 16 |
| `SceneRenderStateCoordinator::InvalidateSceneScopedRendererState` | level-change **data-path debt** | scene-generation command/retirement, Prompts 13/15/16 |
| `RendererSystemRoot::~RendererSystemRoot` | shutdown, duplicated by children | one coordinator final drain, Prompt 22 |
| `RendererSystemRoot::PostLoad -> CloseExecuteAndFlushCurrentFrame` | initialization/upload boundary | coordinator settles exact initial tokens, Prompt 13/16 |
| `RendererSystemRoot::RefreshImageProviders` | provider refresh rare boundary/data debt | provider-generation transition, Prompt 16/28 |
| `FramePipeline::RefreshFrameExecution` | resize/settings data debt | frame-execution generation retirement, Prompt 16 |
| FramePipeline pending-resize pre-drain | duplicate; calls refresh which drains again | delete duplicate; coordinator resize, Prompt 13/16 |
| D3D12 services destructor -> RHI -> each queue | layered shutdown; native event wait per queue | top-level final drain, Prompt 22; queue wait remains private |
| D3D12 services/RHI/HRI `WaitForIdle` | wrapper mechanism | private coordinator operation, Prompt 13/16 |
| D3D12 `CloseExecuteAndFlushCurrentFrame` | explicit host/debug boundary | coordinator/token reason, Prompt 13/16 |
| D3D12 ImGui shutdown wait | child teardown duplicate | parent proves completion, Prompt 22 |
| D3D12 frame-resource begin-frame fence wait | legitimate frame-slot reuse | wait exact slot token, Prompt 18/28 |
| D3D12 queue fence/idle wait | native host mechanism | reuse/flush/shutdown only with timeout, Prompt 28 |
| Vulkan RHI destructor -> `vkDeviceWaitIdle` | shutdown duplicated by services/context/UI | one top-level final drain, Prompt 22 |
| Vulkan services destructor | layered shutdown duplicate | remove after parent proof, Prompt 22 |
| Vulkan services/RHI/HRI idle forwarders | wrapper mechanism | private coordinator operation, Prompt 13/16 |
| Vulkan services resize wait | nested resize drain | delete; coordinator transition, Prompt 13/16/19 |
| Vulkan swap-chain destructor wait | child shutdown drain | parent completion proof, Prompt 22 |
| Vulkan swap-chain resize wait | second nested resize drain | delete, then image/token completion, Prompt 16/19 |
| Vulkan command-context destructor wait | child shutdown drain | context retirement proof, Prompt 19/22 |
| Vulkan ImGui shutdown wait | child teardown drain | parent completion proof, Prompt 22 |
| Vulkan close/execute/flush wait | explicit host/debug boundary | coordinator/token reason, Prompt 13/16 |
| Vulkan timeline wait | queue completion/frame-slot mechanism, not device idle | reuse boundary plus timeout, Prompt 19/28 |
| AssetCooker child process | synchronous outer tool adapter or scheduled BlockingIo call | shared event-driven Core process contract; Prompt 04 closed, Prompt 27 fault stress |
| launcher/editor process execution | scoped BlockingIo completion and stop-token cancellation | reader thread/polling path deleted in Prompt 04; Prompt 27 fault stress |

One Vulkan resize can traverse several defensive drains. The replacement gate counts native idle calls per operation; deleting a wrapper without deleting nested native drains fails.

### Reproducible baseline workload matrix

Build with `cmake -S . -B build` then `cmake --build build --config DevelopmentEditor --target ShowcaseRuntime ShowcaseEditor ShaderCompiler TextureCooker AssetCooker architecture_boundary_check -- /m`. Tools use repository root as working directory; runtime/editor use `Projects/Showcase` so project-root discovery finds `Levels.catalog` and cooked defaults. Runtime is `artifacts/dev/projects/Showcase/runtime/DevelopmentEditor/ShowcaseRuntime.exe`; editor is under `editor/DevelopmentEditor`.

| Workload | Reproduction/fixed state | Preservation status |
|---|---|---|
| Tiny | `SPARKLE_STARTUP_LEVEL=Empty`, `SPARKLE_RHI_BACKEND=D3D12` or `Vulkan`; authored startup camera | supported serial-crossover baseline |
| Sponza | `SPARKLE_STARTUP_LEVEL=Sponza` on each backend; authored camera and fixed settings/window | supported required raster scene |
| Animation | `SPARKLE_STARTUP_LEVEL=CesiumMan`; authored startup camera | supported representative animated scene; stress capture remains evidence work |
| Classic TLAS | Sponza plus `--cvar=r.RayTracing.PreferPartitionedTlas=false`; require reported RT capability | capability-gated, never claim on unsupported adapter/backend |
| PTLAS | `SPARKLE_STARTUP_LEVEL=SponzaPtlas --cvar=r.RayTracing.PreferPartitionedTlas=true` | capability/provider-gated; record explicit skip |
| Editor viewport | launch ShowcaseEditor on each backend, open Sponza, fix viewport/camera, exercise output/presentation | supported; ImGui remains editor-owner |
| Shader cook | run `artifacts/dev/tools/ShaderCompiler/DevelopmentEditor/ShaderCompiler.exe --help`, then its printed global/changed command; retain publication ID/output | supported tool; `--help` is verb authority |
| Texture cook | run `artifacts/dev/tools/TextureCooker/DevelopmentEditor/TextureCooker.exe --help`, then a fixed Showcase request; retain cooked hash | supported tool; capture codec/internal workers |

Each capture records commit, config, backend/adapter/driver, resolution, VSync/present mode, level, camera transform, warm-up/sample frames, CVars, provider and validation state, capture tool/version, CPU p50/p95/p99, GPU time, latency if available, peak memory, thread count/context switches, and output hash/image. The repository has no scripted camera/capture manifest; Prompt 05 owns a non-duplicative reproducibility decision.

Preserve D3D12 and Vulkan, the frame graph/queue batching, raster tiny/Sponza/animation, shader/texture cooking, editor viewport/ImGui, temporal/history/provider/capture paths, classic TLAS where capability reports it, and PTLAS only where backend/provider/adapter support it. Bistro or missing catalog assets are not claimed. Reservoir/reference rendering is preserved only where current scene/settings expose it. Prompt 00 did not alter rendering output or manufacture support.

### MT-01–MT-44 current-architecture pre-mortem

The professional pattern and full teaching falsifier remain normative in the earlier failure atlas. This verified table binds every ID to current Sparkle evidence and one closing prompt; “future risk” means the hazard is concretely prevented by preserving the stated gate, not that a fix exists.

| ID | Current exposure/path | Selected professional pattern | Owner | Falsifying evidence |
|---|---|---|---|---|
| MT-01 | `GameSceneSnapshot` contains raw `Mesh*`; renderer also retains scene access | immutable render proxy + generational handle | 12/15 | unload/mutate immediately after delayed packet |
| MT-02 | snapshot pointer graph can mutate after capture | immutable-after-publication owned graph | 12/15 | post-publish mutation; serial/threaded hash |
| MT-03 | no payload-ready protocol yet; future executor risk | locked publication first; release/acquire only with proof | 01/12 | locked/SC oracle and delayed publication stress |
| MT-04 | shader request/publication IDs are raw counters; future slot reuse lacks generations | strong generation/sequence and stale rejection | 04/12/16 | tiny-wrap delayed completion |
| MT-05 | async/Qt/process lambdas and service captures span owner lifetime | structured `TaskScope`, owned inputs | 04/22 | destroy/cancel at every stage |
| MT-06 | bespoke future/process completion has several failure exits | exactly-once terminal state/finally edge | 04 | fail/cancel/shutdown race settles once |
| MT-07 | ProcessRunner cancellation terminates/waits child; GPU work has separate lifetime | cooperative cancellation + normal native retirement | 04/16 | cancel before/mid/after submit |
| MT-08 | Input callback and Streamline external calls occur under locks | snapshot/apply after unlock, two-phase transition | 03/24/28 | self-unsubscribe/re-entry/slow callback |
| MT-09 | renderer/RHI were implicitly owner-thread-only | explicit creator-thread assertion and commands | 00/13 | wrong-thread facade call names operation |
| MT-10 | layered renderer/RHI/UI/context destructors independently drain | ordered close/drain/destroy/join | 22 | shutdown empty/full/blocked/delayed |
| MT-11 | Streamline global mutex and broad Vulkan descriptor mutex risk becoming architecture | single owner + immutable views + narrow native lock | 13/19/28 | contention trace/wrong-owner injection |
| MT-12 | no documented cross-subsystem total order; callback-under-lock already fails | no nesting/callback/wait under lock, then formal order | 24 | inversion/re-entry watchdog |
| MT-13 | no task executor yet; applicable future deadlock gate | prerequisite DAG, no worker parent-wait | 01/02 | all workers spawn children at 1/2/N |
| MT-14 | zero-time future polling and 50 ms process polling exist | predicate event/parking, explicit dependency | 02/04 | idle wait near-zero CPU; recursive scenario |
| MT-15 | no engine condition variable yet; future executor/queue risk | predicate state and close wake under one protocol | 01/13 | notify-before-park/spurious/close stress |
| MT-16 | native child/GPU waits can occupy arbitrary future workers if reused | bounded BlockingIo and coordinator GPU waits | 04/13/27 | saturate I/O while frame lane progresses |
| MT-17 | async thread, per-operation QThread, reader thread, SDK internal pools coexist | executor family + third-party capacity inventory | 04/25 | system trace thread count/migrations |
| MT-18 | no executor yet; naive `hardware_concurrency-N` would be a concrete introduction risk | measured conservative 0/1/2/N policy | 02/25 | topology/workload scaling sweep |
| MT-19 | full-scene rebuild and future per-draw recording invite tiny tasks | coarse ranges, grain sweep, serial crossover | 06/20/25 | serial-to-tiny grain curve |
| MT-20 | scene/mesh/pass costs are skewed; equal counts will straggle | dynamic/cost-aware ranges + stable fan-in | 06/20 | adversarial clustered expensive work |
| MT-21 | current serial phases and routine idle waits become global barriers if parallelized literally | dependency DAG and true consumer joins | 06/13/16 | delay non-critical branch, observe successors |
| MT-22 | no priority policy yet; future foreground/background competition | dependencies/lanes for correctness, fair hints | 02/25 | sustained background cannot starve frame/cleanup |
| MT-23 | future frames/cooks/uploads could be unbounded; current queued recook is manually one-deep | bounded slots/bytes and explicit backpressure | 04/13/27 | slow consumer reaches stable ceiling |
| MT-24 | DXC/Slang/codecs/Qt/Streamline/driver internal workers are unbudgeted | inventory inner pools and bound outer work | 24/25/27 | compare 1/N outer total runnable threads |
| MT-25 | future per-worker counters plus current atomics may share hot lines | worker-local/cache-separated measured state | 24/25 | padding A/B with cache counters |
| MT-26 | linear allocator CAS, logger, descriptor and allocator locks are shared cursors | local pages/ranges and deterministic merge | 18/19/24 | contention trace and range overlap oracle |
| MT-27 | command/descriptor/scratch state will multiply by worker×frame×queue | explicit capacity equation/page reuse/spill | 18/19/25 | max topology memory/high-water test |
| MT-28 | full snapshot/rebuild uses pointer-heavy object data | measured hot/cold DOD, dense ranges, stable handles | 07-11/15 | cache/bandwidth vs serial baseline |
| MT-29 | completion order could affect entity IDs, package bytes, diagnostics, submission | stable keys and ordered fan-in | 03/06/20/27 | randomized delay, identical hash/order |
| MT-30 | current scene vectors/registries can structurally mutate; future ranges would invalidate | frozen structural epoch + owner commit buffers | 07-11 | randomized structural mutation rejected/deferred |
| MT-31 | migration could leave snapshot and persistent/DOD representations both authoritative | one storage/identity, adapter deletion gate | 07-15 | all facades converge; repository search |
| MT-32 | D3D12 allocators/lists and Vulkan pools are not yet leased per worker | exclusive `RhiCommandRecordingLease` | 18/19 | concurrent-use/reset assertions + native validation |
| MT-33 | frame resources/descriptors/uploads can reset before delayed GPU | exact last-use token and ringed generations | 16/18/19 | GPU delayed beyond several CPU frames |
| MT-34 | queue mutexes presently make arbitrary submit technically callable | coordinator-only submit/present authority | 13/18/19 | wrong-thread submit and serial plan match |
| MT-35 | future pass fan-out can create tiny lists/submits; current batches are serial | measured recording groups and bounded submit count | 20/25 | group/list/submit sweep with GPU/latency |
| MT-36 | lazy cache/pipeline/descriptor/resource mutation during recording is possible | prewarm immutable pass runtime; publish later generation | 16/20 | cold-cache recording must not mutate/block |
| MT-37 | scene/reload/provider/resize ordinary paths call idle; Vulkan resize nests drains | sequence/token retirement; one final drain | 13-16/22 | native idle counter zero on ordinary paths |
| MT-38 | future one-ahead/batching could increase latency; current control is dormant at depth 0 | bounded 0/1 modes chosen by correlated evidence | 13/25 | CPU/GPU-bound throughput/latency comparison |
| MT-39 | parallel merge could change barriers, transparency, history, RT/provider tags | same compiled graph and stable semantic merge | 20/28 | serial/parallel plan and feature matrix |
| MT-40 | current CPU serial, GPU queue graph, frames-in-flight are easy to conflate | separately label task DAG/render pipeline/recording/GPU timeline | 05/13/20 | capture identifies true overlap intervals |
| MT-41 | both backends/configs/native validation are not one automatic proof matrix | serial oracle + stress + optimized/native validation parity | 29 | injected defect detected; gaps explicit |
| MT-42 | launcher 150 ms sleep and process 50 ms polling use wall time for ordering | controllable events/checkpoints, no sleep correctness | 04/29 | machine-speed-independent test |
| MT-43 | no current cross-backend CPU scopes; historical trace artifact is not live infrastructure | timeline-first p50/p95/p99 critical-path experiment | 05/25/29 | capture on/off and independent rerun |
| MT-44 | no owned lock-free queue today; D3D12 CAS allocator is bounded allocation, not a scheduler | locked/owner baseline before any measured lock-free protocol | 01/24 | locked/SC oracle, ABA/reclamation proof or delete |

No MT ID is marked non-applicable merely because its triggering subsystem has not been written. For MT-13, 15, 18, 22, 23, 25, 27, 32, 35, 38, and 44 the concrete architectural reason is “introduction gate”: Prompt 00 confirms no current implementation, and the named future prompt may not introduce it without the listed pattern and falsifier.

### Prompt 00 validation record

Validation executed on 2026-07-18 in `DevelopmentEditor`:

- Fresh CMake generation and builds passed for Core, Platform, GameFramework, both RHI backends, Renderer, Application, Editor, Showcase runtime/editor, ShaderCompiler, TextureCooker, AssetCooker, SparkleLauncher/Probe, and `architecture_boundary_check`. The boundary check reported no new violations.
- Empty and Sponza ran for 12–15 seconds and closed with code 0 on D3D12 and Vulkan. This proves startup/frame/present/shutdown smoke only; it does not replace image comparison, a native-validator matrix, animation, RT, PTLAS, provider, capture, or long-run stress.
- Development backends retain their existing D3D12 debug-layer/Vulkan validation configuration. No validation error appeared in captured process output, but Prompt 00 did not obtain a native PIX/RGP/Nsight capture; record this as an evidence gap, not a pass for every feature.
- An isolated, subsequently deleted CVar probe proved defaults `0/true/false/false/0`, both equals and separated assignment forms, boolean `off`, and that invalid/unknown assignments preserve defaults. Dormant controls did not alter default runtime behavior.
- An isolated, subsequently deleted InputSystem probe constructed a deterministic keyboard backend, subscribed a callback that unsubscribed itself, and dispatched one event. It reached dispatch and terminated with Windows status `0xC0000409` before returning from the callback path when the non-recursive mutex was reacquired. Whether a standard-library build deadlocks or fail-fasts, the invariant violation is proven: callback-under-lock cannot support self-unsubscribe. Prompt 00 intentionally did not fix it.
- `ctest --test-dir build -C DevelopmentEditor --output-on-failure` reported **No tests were found**. Tool `--help` contracts and `SparkleLauncherProbe` ran successfully. The absence of a registered test suite is explicit MT-41 debt.
- WPA/WPR is installed and is the existing CPU/GPU timeline route, but `wpr -start CPU -start GPU -filemode` failed with policy error `0xc5585011` (“Failed to enable the policy to profile system performance”) in this non-elevated session. No ETL was fabricated or checked in. Rerun the documented Sponza command from an authorized WPR session and store the ETL in `build`, then inspect the labeled physical threads in WPA.
- `git diff --check` produced no whitespace errors. The final canonical/rejected-alias and owned-concurrency searches must remain part of each closing prompt because generated/external exclusions and semantic false positives are intentional.

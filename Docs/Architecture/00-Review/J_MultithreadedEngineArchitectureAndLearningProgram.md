# J. Multithreaded Engine Architecture and Learning Program

Status: proposed target architecture and implementation program
Date: 2026-07-16
Last adversarial conformance review: 2026-07-17
Scope: runtime, renderer, RHI, editor, asset and shader tools, learning evidence, and portfolio presentation
Governing requirements: [A. Principal Rendering Requirements](A_PrincipalRenderingRequirements.md), [E. External Renderer Repository Comparison](E_ExternalRendererRepositoryComparison.md), [G. Advanced Graphics Engine Executive Summary](G_AdvancedGraphicsEngineExecutiveSummary.md), and [H. Advanced Graphics Engineer Persona](H_AdvancedGraphicsEngineerPersona.md)
Repository context: [D. Whole Repository Architecture Map](D_WholeRepositoryArchitectureMap.md)

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
- The RHI remains the explicit low-level D3D12/Vulkan service layer. RecordingContextLease expresses temporary ownership; it does not hide queues, commands, barriers, descriptors, or native backend rules.
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
| Game/render pipelining | Game frame N+1 overlaps render frame N | No | Dedicated render thread and bounded packet mailbox |
| Parallel command recording | CPU workers build command lists/buffers together | No | Frame-graph recording groups and worker-local RHI contexts |
| GPU queue concurrency | Graphics, compute, and copy execute concurrently | Yes | Preserve, measure, and feed it more efficiently |
| Frames in flight | CPU and GPU work on different frames | Yes at the RHI/frame-resource level | Make CPU packet/resource ownership explicit |

GPU async compute is not CPU multithreading. A copy queue upload is not a background CPU task. Multiple frames in flight do not make the application loop multithreaded. The final documentation, profiler, and portfolio material should keep these distinctions visible.

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

Writing a frame packet and placing its slot index in a mailbox are separate operations. The consumer must not see the index before the packet contents are visible.

The packet mailbox should use a release operation when the producer publishes a ready slot and an acquire operation when the render thread consumes it. Reusing the slot requires a second acknowledged transition. The queue implementation may use atomics and semaphores internally, but its public contract must describe:

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

## External Repository Study

All GitHub source observations below are tied to the listed commit rather than a moving branch. The goal is to learn mechanisms and constraints, not clone any one engine’s architecture.

### Source Matrix

| Source | Inspected revision | Relevant mechanisms | Sparkle lesson |
|---|---|---|---|
| NVIDIA Donut | bc1ea24b0486f1c00d89327fe16c0b4dd11c5937 | Small FIFO thread pool used for asynchronous scene/texture loading | Useful background-content pattern; insufficient for a renderer dependency graph |
| NVIDIA NVRHI | 8e8c36e37558acec333204619b95d9d2fcdc4a79 | Concurrent command-list recording and explicit state assumptions across lists | Parallel recording requires explicit entry/exit or permanent resource states |
| AMD FidelityFX SDK Cauldron2 | 60f4ea81909200d8542eca14dccb2628b763a9a3 | Task manager used mainly for content loading; fan-in completion callback | Continuations are useful; a loading pool is not a general frame scheduler |
| AMD Cauldron (legacy) | b92d559bd083f44df9f8f42a6ad149c1584ae94c | Simple global FIFO worker pool | Good historical sample, not the target scheduler architecture |
| Epic Unreal Engine documentation | UE 5.8 documentation, accessed 2026-07-17 | Tasks DAG, game/render/RHI separation, render proxies, RDG parallel execute, asynchronous level-loading constraints | Strong ownership and dependency model; defer global/UI/transaction interaction during worker loading; do not add a separate RHI thread without need |
| Epic MassEntity documentation | UE 5.8 documentation, accessed 2026-07-17 | Data-only entities/fragments, archetypes, queries, processors, deferred command buffer | Strong ECS/DOD model; Sparkle should borrow separation and deferred structure without copying Mass scale |
| O3DE AzCore + Atom | 68683f23fb747380d3efa2424bd5f30242e9c5a2 | Retained task graph, task executor, jobified frame scheduler, thread-local command pools | Closest open-source structural reference for scheduler-to-renderer integration |
| Unity EntityComponentSystemSamples | 6786a741ee1f118ed14cecfa02beae8e926937b0 | Entities/Jobs examples, component queries, command-buffer playback, deterministic parallel sort keys | Direct public proof of ECS-to-job integration and deferred structural mutation |
| EnTT | 1333fa53129e7cfded5a9640c4336a254049917b | Versioned entity identifiers, sparse-set component pools, const/writable views and smallest-pool multi-component iteration | Appropriate bounded C++ storage reference when archetype chunks are premature |
| Google Filament | 398d13c4abd148ea6e139b05e5418efa37d284d3 | Work-stealing jobs, parent completion counts, parallel_for, cache-line-sized job records | Strong scheduler mechanics; Sparkle should use dependencies instead of normal worker waits |
| Microsoft DirectX Graphics Samples | 357ade6ec6ff0d9dcadc48f35c7a28e37c0cdf7a | Per-frame/per-worker allocators and command lists, serial ordered submission | Clear D3D12 proof of parallel recording constraints |

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

The command buffer, read view, and change journal are a Sparkle composition of these ownership/generation principles, not a claim that NVIDIA, AMD, Epic, or O3DE exposes identical APIs. This distinction matters: reference prestige validates mechanisms, while current Sparkle data flow determines the actual interface.

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
      Task.h
      TaskGraph.h
      TaskGroup.h
      TaskEvent.h
      ParallelFor.h
      TaskExecutor.h
      TaskScope.h
      TaskSchedulerConfig.h
    Private/
      TaskRecord.h
      TaskPool.*
      Worker.*
      WorkStealingDeque.*
      InjectionQueue.*
      TaskExecutor.*
      TaskGraph.*
      TaskProfilerAdapter.*
    Tests/

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
      RenderThread.*
      RenderFrameMailbox.*
      RenderThreadCommands.*
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
    RecordingContextLease.h
  Private/<backend>/
    WorkerRecordingContext.*
    WorkerUploadArena.*
    WorkerDescriptorArena.*
~~~

Names may follow existing repository conventions, but the responsibilities and dependency direction should remain.

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

RecordingContextLease is a capability token proving temporary exclusive access to backend recording state. It is not a new command API and must not become a convenience wrapper over RenderCommandList.

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
| Main/game thread | Window pump, input, GameScene structural commit, level/editor command application, world/read/render publication | Immutable asset registry and narrow product-owned renderer results | Touch mutable renderer/RHI state; block for routine render completion |
| Editor main thread | ImGui/UI model, editor transactions/commands, viewport and operation requests | Published `WorldReadView`/editor model and narrow immutable renderer/operation results | Traverse worker-mutated GameScene storage; give live ImGui/editor pointers to workers or render thread |
| Render coordinator | RendererSystemRoot, FramePipeline coordination, RenderWorld, GPU caches, RHI creation/destruction, submit/present | Published frame packets and task results | Dereference GameScene; execute blocking tool/file work |
| Frame workers | Data-pure game/render tasks and command recording with leased local contexts | Immutable inputs, disjoint output slices, concurrent task runtime | Wait on other tasks, submit queues, mutate global caches, destroy RHI resources |
| Background/IO workers | File reads, child processes, imports, compression/compilation where safe | Explicit task input and output publication objects | Consume latency-sensitive frame-worker capacity |
| GPU queues | Execute compiled command streams | RHI resources covered by barriers/fences | Be described as CPU task concurrency |

Each engine-created thread must have a stable name visible to the platform debugger and existing PIX/Nsight/profiler-visible CPU instrumentation. The initial names can be Sparkle.Main, Sparkle.Render, Sparkle.Frame.0…N, and Sparkle.Background.0…M. This requirement does not authorize a new profiling framework.

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

## SparkleTasks Job System Design

`SparkleTasks` **is Sparkle's job system**. This document prefers “task” for a scheduled unit and “task graph” for dependencies because Epic and O3DE use that vocabulary, but the engine feature normally called a job system is absolutely being implemented: fixed workers, work stealing, job/task records, dependencies, fan-out/fan-in, parallel-for, cancellation, lanes, sleeping/waking, deterministic serial execution, scopes, and shutdown.

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
    Frame,
    Background,
    BlockingIo
};

struct TaskDesc
{
    TaskName name;
    TaskLane lane = TaskLane::Frame;
    TaskPriority priority = TaskPriority::Normal;
    CancellationToken cancellation;
};

class TaskGraphBuilder
{
public:
    template<class Fn>
    TaskNode Add(TaskDesc desc, Fn&& function);

    void DependsOn(TaskNode task, TaskNode prerequisite);
    TaskNode WhenAll(TaskName name, std::span<const TaskNode> prerequisites);
    CompiledTaskGraph Compile();
};

class TaskExecutor
{
public:
    TaskRun Submit(const CompiledTaskGraph&, TaskRunContext&);
    TaskRun Submit(TaskDesc, TaskFunction);
    void PumpAdoptedThread(TaskLane, uint32_t budget);
};

template<class Range, class Fn>
TaskNode ParallelFor(
    TaskGraphBuilder& graph,
    TaskDesc desc,
    Range range,
    ParallelForPolicy policy,
    Fn&& function);
~~~

Important semantic decisions:

- TaskNode is a builder-local token, not a pointer.
- A compiled graph owns immutable topology and may be reusable if its bindings are supplied per run.
- TaskRun tracks one submission generation.
- Handles use generation counters so stale handles fail deterministically.
- A task function receives its context explicitly; scheduler thread-local globals are not business-data channels.
- Tasks return an explicit TaskResult or are noexcept. Exceptions at the boundary are captured, reported, and cause a defined dependent cancellation policy. They are never silently swallowed.

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
TaskNode child = graph.Add(...);
TaskNode continuation = graph.Add(...);
graph.DependsOn(continuation, child);
~~~

Development builds should detect a worker blocking on a TaskRun and report the task name and submission graph. This rule prevents pool exhaustion and makes the critical path inspectable.

### Graph Validation and Boundedness

TaskGraphBuilder::Compile must reject:

- self-dependencies and dependency cycles
- stale or foreign builder tokens
- duplicate edges when they would corrupt counters
- a graph whose task/edge count exceeds configured arena limits
- a continuation whose completion policy cannot settle
- invalid cross-lane dependencies

Graph storage is bounded per run. Overflow returns a controlled build/submission failure before any task starts; it does not silently heap-allocate an unbounded graph during a frame.

Task priority is a scheduling hint, never a correctness mechanism. A frame task must not depend on an unscheduled low-priority background task or a blocking-I/O task. Runtime content that is not ready uses an existing resident fallback or delays publication at an explicit host boundary. If an external event is genuinely frame-critical, its continuation is injected into the frame lane only after the external operation completes.

### Executor and Render Shutdown Order

Shutdown is part of the concurrency design:

1. Application stops accepting gameplay/editor commands that can publish new frames.
2. Frame mailbox closes and wakes both producer and consumer.
3. Render coordinator consumes or cancels accepted packets according to explicit policy.
4. Recording/preparation task runs settle; no task may retain renderer state afterward.
5. Render coordinator performs the one legitimate final GPU drain, destroys swapchain/RHI state on its owner thread, and exits.
6. Background tool/process tasks are cancelled or drained according to product workflow.
7. Task executors stop accepting work, wake workers, join threads, and release task arenas last.

Device loss follows a separate controlled render-thread failure path; it must not strand a mailbox slot, task continuation, or editor waiter.

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

Frame lane:

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

RenderFrameDynamicData contains current frame values:

~~~cpp
struct RenderFrameDynamicData
{
    FrameId frame;
    CameraPacket camera;
    SoA<RenderObjectId, Matrix3x4, Matrix3x4> currentAndPreviousTransforms;
    SoA<RenderObjectId, Bounds> bounds;
    Span<SkinningPalettePacket> skinning;
    Span<MorphWeightPacket> morphWeights;
    Span<DynamicLightPacket> lights;
};
~~~

The concrete SoA layout should follow measured access patterns. The design point is stable IDs and dense, immutable dynamic arrays, not one copied polymorphic object graph.

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

The portfolio demonstration must show a level load or cook running while the editor remains interactive, cancellation leaving the old scene/publication intact, and stale completion being rejected. It must also show that heavy background work cannot starve the frame lane.

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
| bounded producer/consumer mailbox | main-to-render frames, editor/render requests | backpressure, latency, sleep/wake semantics | unbounded backlog or lost wakeup |
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

The frame packet mailbox is single-producer/single-consumer and a good bounded specialization. Worker queues are performance-sensitive and can benefit from work-stealing deques. Many other structures are not.

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
| std::async per operation | Uncontrolled threads/semantics and weak diagnostics | Executor lane and TaskRun |
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

Exit criteria:

- baseline commands/settings and essential captures use the existing validation artifact workflow rather than a new report format
- both D3D12 and Vulkan validation runs are clean
- every important WaitForIdle site is classified as shutdown, boundary, or data-path debt
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
- Tools/Cooking/TextureCooker
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

Delete:

- the ShaderRecookCoordinator std::future/std::async execution path
- any temporary second ad-hoc pool introduced during the pilot

Exit criteria:

- ThreadSanitizer-capable platform or equivalent stress/diagnostic runs pass
- outputs match serial byte-for-byte where formats are deterministic
- worker counts 0/1/2/N work
- failed/cancelled cooks never publish partial registries
- interactive shader recook cannot starve the frame lane
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

### Stage 3 — Dedicated Render Coordinator and Bounded Mailbox

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
2. Add bounded frame slots and mailbox with clear publication/reuse transitions.
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
5. Audit every remaining thread, mutex, future, and WaitForIdle.
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
12. Bounded mailbox and render coordinator
13. Editor UI/viewport packet conversion
14. Persistent GPU scene
15. Deferred scene/shader generation retirement
16. Renderer preparation DAG
17. D3D12 worker recording contexts
18. Vulkan worker recording contexts
19. Frame-graph recording groups and first parallel passes
20. Draw-heavy intra-pass parallelism
21. Reliability, metrics, and legacy deletion

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

- stale TaskNode/TaskRun generation
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
- Application Verifier, guard allocators, and intentional packet poisoning
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

Late consolidated frame/RHI workload:

- passes and queue assignment
- resource classes, histories, transitions, barriers, aliasing, and cross-queue waits
- transient allocation high-water mark and allocator-backed memory budget/pressure
- descriptor occupancy/pressure
- pipeline and shader-package count
- GPU pass timestamps and BLAS/classic-TLAS/PTLAS build/update timings
- D3D12/Vulkan capability and supported-feature comparison

End result:

- CPU and GPU frame p50/p95/p99
- FPS only as a derived presentation
- input-to-present comparison for pipeline depth modes
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
- cold and warm caches
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

### Tasks

- SparkleTasks supports prerequisites, fan-in, nested work, cancellation, failure, lanes, private capture-time instrumentation, and serial execution.
- Normal worker tasks do not block on other tasks.
- Frame and blocking-I/O workloads cannot starve each other.
- Scheduler tests pass at 1/2/N workers and during repeated shutdown stress.
- application, world/document, asset-generation, frame, and tool scopes cancel and settle before their captured owners/storage are destroyed.

### Rendering

- Frame preparation contains measured task-parallel work.
- Frame-graph recording groups have explicit state and deterministic submission contracts.
- D3D12 allocators/lists and Vulkan command pools/buffers are worker-local and frame-retired.
- Pass runtime/PSO state is prewarmed before parallel recording.
- Upload and transient descriptor allocations have parallel-safe ownership.
- Both backends pass native and engine validation in serial and parallel modes.
- Classic TLAS and PTLAS retain build/update/trace/lifetime behavior where each backend reports support.
- Native reservoir lighting, reference path accumulation/reset, and temporal/provider signals preserve stable identity and frame semantics.
- Shader package, reflection, parameter-layout, DXIL/SPIR-V, HLSL/Slang, and pipeline-generation invariants hold under parallel cook/reload/record.
- Provider instances obey explicit affinity/capability/failure rules and do not own renderer scheduling or scene data.
- Screenshot/BMP capture and native debugger/profiler hooks remain functional without routine device idle.
- CPU gains do not buy unjustified GPU barrier, descriptor, memory, pipeline, command-list, residency, or RT regressions.

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
- curated multiple levels, optional heavyweight content packs, and owned runtime/editor/tools/symbols/content packages remain supported.
- launcher and cooker task use is restricted to owned product workflows.

### Evidence

- before/after profiler captures and benchmark tables exist
- correctness parity and stress results are reproducible
- CPU task concurrency, render pipelining, GPU queue concurrency, and frames in flight are explained separately
- limitations and tradeoffs are documented
- all replaced legacy paths are removed
- every change set closes its capability-preservation and replacement/deletion ledgers
- shipping defaults contain no research-only task/data path
- broad workload analysis is late and uses consolidated existing hooks
- no new profiler framework, default report format, task/debug panel, runtime log stream, or additional policy document was introduced
- public/product wording does not claim external SDK equivalence or imply unvalidated support
- the existing concise overview lets a reviewer locate the RHI contract, frame-graph/pass rules, shader source-to-package path, feature classification, and backend support without reading this entire study
- executable include/dependency/boundary checks cover the new Tasks, packet, Renderer, and RHI directions
- executable ownership tests cover GameFramework systems/commands/scopes and the editor read-model/operation boundary

## Immediate Next Implementation Slice

The first code change after accepting this design should be Stage 0 plus the smallest part of Stage 1:

1. add thread-role/FrameId/SceneGeneration vocabulary and CPU timing points
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
| [Entity.h](../../../Engine/GameFramework/Public/Scene/Entity.h) and [Component.h](../../../Engine/GameFramework/Public/Scene/Component.h) | `Entity` orchestration is unused, while camera/mesh types consume the `Component` base; neither virtual update hooks nor raw owner pointers should dictate the world-system redesign |
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
| [ShaderRecookCoordinator.cpp](../../../Engine/Application/Private/ShaderRecook/ShaderRecookCoordinator.cpp) | A polled std::async job launches the compiler and accepted reloads wait for RHI idle |
| [AssetCookerDispatcher.cpp](../../../Tools/Cooking/AssetCooker/Private/Dispatch/AssetCookerDispatcher.cpp) | Cook stages and per-scene imports/builds execute serially |
| [TextureCookRequestBatchProcessor.cpp](../../../Tools/Cooking/TextureCooker/Private/Cooking/TextureCookRequestBatchProcessor.cpp) | Texture requests use one cooker in a serial batch loop |
| [ShaderPackageCooker.cpp](../../../Tools/Shaders/ShaderCompiler/Private/Cooking/ShaderPackageCooker.cpp) | Planned shader cook nodes execute serially before package emission |
| [ProcessRunner.cpp](../../../Tools/Launcher/SparkleLauncher/Private/Core/ProcessRunner.cpp) | Child-process output uses blocking reader-thread behavior that belongs on an I/O lane |

## Primary Source Trail

The following links are the direct source/documentation trail used for the architectural comparison.

### NVIDIA

- [Donut ThreadPool interface at bc1ea24](https://github.com/NVIDIA-RTX/Donut/blob/bc1ea24b0486f1c00d89327fe16c0b4dd11c5937/include/donut/engine/ThreadPool.h)
- [Donut ThreadPool implementation at bc1ea24](https://github.com/NVIDIA-RTX/Donut/blob/bc1ea24b0486f1c00d89327fe16c0b4dd11c5937/src/engine/ThreadPool.cpp)
- [NVRHI Programming Guide at 8e8c36e](https://github.com/NVIDIA-RTX/NVRHI/blob/8e8c36e37558acec333204619b95d9d2fcdc4a79/doc/ProgrammingGuide.md)

### AMD

- [FidelityFX SDK Cauldron2 TaskManager interface at 60f4ea8](https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK/blob/60f4ea81909200d8542eca14dccb2628b763a9a3/Kits/Cauldron2/dx12/framework/core/taskmanager.h)
- [FidelityFX SDK Cauldron2 TaskManager implementation at 60f4ea8](https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK/blob/60f4ea81909200d8542eca14dccb2628b763a9a3/Kits/Cauldron2/dx12/framework/core/taskmanager.cpp)
- [Legacy AMD Cauldron thread pool at b92d559](https://github.com/GPUOpen-LibrariesAndSDKs/Cauldron/blob/b92d559bd083f44df9f8f42a6ad149c1584ae94c/src/common/Misc/threadpool.h)

### Epic Games

- [Tasks System](https://dev.epicgames.com/documentation/en-us/unreal-engine/tasks-systems-in-unreal-engine)
- [Threaded Rendering](https://dev.epicgames.com/documentation/en-us/unreal-engine/threaded-rendering-in-unreal-engine)
- [Parallel Rendering Overview](https://dev.epicgames.com/documentation/en-us/unreal-engine/parallel-rendering-overview-for-unreal-engine)
- [Render Dependency Graph](https://dev.epicgames.com/documentation/en-us/unreal-engine/render-dependency-graph-in-unreal-engine)
- [Asynchronous Level Loading](https://dev.epicgames.com/documentation/unreal-engine/asynchronous-level-loading-in-unreal-engine?lang=en-US)
- [MassEntity Overview](https://dev.epicgames.com/documentation/unreal-engine/overview-of-mass-entity-in-unreal-engine?lang=en-US)

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

- [Unity ECS samples at 6786a74](https://github.com/Unity-Technologies/EntityComponentSystemSamples/tree/6786a741ee1f118ed14cecfa02beae8e926937b0)
- [Unity entity command-buffer guidance at 6786a74](https://github.com/Unity-Technologies/EntityComponentSystemSamples/blob/6786a741ee1f118ed14cecfa02beae8e926937b0/EntitiesSamples/Docs/entity-command-buffers.md)
- [EnTT registry at 1333fa5](https://github.com/skypjack/entt/blob/1333fa53129e7cfded5a9640c4336a254049917b/src/entt/entity/registry.hpp)
- [EnTT sparse set at 1333fa5](https://github.com/skypjack/entt/blob/1333fa53129e7cfded5a9640c4336a254049917b/src/entt/entity/sparse_set.hpp)

### Microsoft

- [DirectX 12 Multithreading sample overview at 357ade6](https://github.com/microsoft/DirectX-Graphics-Samples/blob/357ade6ec6ff0d9dcadc48f35c7a28e37c0cdf7a/Samples/Desktop/D3D12Multithreading/readme.md)
- [D3D12Multithreading FrameResource at 357ade6](https://github.com/microsoft/DirectX-Graphics-Samples/blob/357ade6ec6ff0d9dcadc48f35c7a28e37c0cdf7a/Samples/Desktop/D3D12Multithreading/src/FrameResource.cpp)

## Final Recommendation

Approve the architecture as one program with three non-negotiable ordering rules:

1. ownership and packet lifetime before the render thread
2. persistent data before parallelizing scene rebuild work
3. native per-worker recording ownership and pass audits before parallel frame-graph execute

Sparkle already has the rendering abstractions and explicit GPU scheduling needed to make this credible. The pivotal step is to turn implicit serial ordering into explicit ownership, generations, dependencies, and retirement—and then show, with serial equivalence and measurements, that the resulting concurrency is both correct and useful.

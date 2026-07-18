# K. Multithreaded Engine Implementation Prompt Series

Status: required execution companion to J; no prompt implies implementation already exists
Date: 2026-07-18
Canonical naming authority: J's NVIDIA/AMD/Epic-grounded concurrency and rendering vocabulary; enforced by Rule 10
Architecture and tutorial source: [J. Multithreaded Engine Architecture and Learning Program](J_MultithreadedEngineArchitectureAndLearningProgram.md)
Governing requirements: [A. Principal Rendering Requirements](A_PrincipalRenderingRequirements.md), [E. External Renderer Repository Comparison](E_ExternalRendererRepositoryComparison.md), [G. Advanced Graphics Engine Executive Summary](G_AdvancedGraphicsEngineExecutiveSummary.md), and [H. Advanced Graphics Engineer Persona](H_AdvancedGraphicsEngineerPersona.md)

## Purpose

This document converts every required target in J into an ordered series of implementation prompts. Use one prompt at a time. Each prompt defines what to inspect, what to implement, what nearby code to improve, what old path to delete, how to validate the result, and when work must stop.

J answers:

- why the architecture exists
- how the concurrency concepts work
- what tradeoffs were selected
- what you must understand and be able to explain

K answers:

- what code to study next
- what exact change to make
- what must not be duplicated or exposed
- what tests and captures prove completion
- what must be deleted before advancing

This is not a request to implement the whole program in one branch or one agent turn. Each prompt is a vertical slice with a hard gate. Do not begin the next prompt until the current gate passes or a documented blocker is accepted by the repository owner.

## Non-Negotiable Rules for Every Prompt

These rules are repeated in compact form inside every copy-ready prompt. Their detailed meaning is defined here.

### 1. Inspect Before Adding

Before adding a file, class, function, concept, service, queue, allocator, handle, event, setting, diagnostic, or test fixture:

1. search the entire repository with `rg`/`rg --files` for names and semantic counterparts;
2. inspect public/private headers, CMake membership, ownership, call sites, tests, and shutdown paths;
3. record the counterpart decision:
   - **use** the existing abstraction unchanged;
   - **extend/refactor** it into the required abstraction;
   - **replace** it and delete the superseded path;
   - **add** only when no coherent counterpart exists;
4. do not create `*2`, `New*`, `Async*`, `ThreadSafe*`, or parallel directory trees as a way to avoid integrating with existing ownership;
5. if two counterparts overlap, converge them before adding a third.

The implementation report must list the searches performed and the use/extend/replace/add decision for every major new type.

### 2. Daily Refactor / Leave It Better

Every touched area must leave the repository more coherent than it was found:

- improve misleading names and comments in the directly touched ownership path;
- remove dead includes, stale members, duplicated helpers, obsolete branches, and unreachable compatibility code exposed by the change;
- split functions only when the split creates a real ownership/data boundary;
- consolidate repeated validation or lifetime logic into the existing owner;
- keep refactoring bounded to the touched subsystem and its direct dependencies;
- never use “refactor later” to leave two production paths after a stage gate.

This rule does not authorize unrelated cleanup. The refactor must reduce complexity required to understand or validate the current prompt.

### 3. One Coherent Product, Not Feature Piles

- Preserve `GameFramework → Renderer → RHI`; keep Tasks below its consumers.
- Keep frame-graph scheduling/barrier authority in the frame graph.
- Keep queue submission, presentation, and mutable RHI ownership on the render coordinator.
- Keep ECS storage private to GameFramework; editor and renderer consume stable contracts, not registry internals.
- Reuse one SparkleTasks runtime; do not add subsystem pools or ad-hoc futures.
- Reuse existing profiler/debugger hooks; do not create task panels, report frameworks, or public queue/cache snapshots.
- Preserve D3D12/Vulkan parity and the feature contracts for raster, classic TLAS, PTLAS, reservoir lighting, reference path tracing, temporal/provider paths, shader ABI, capture, and tools.

### 4. Serial Contract Before Parallel Execution

For every converted path:

1. express the new ownership/data contract;
2. run it through the serial executor/consumer;
3. prove behavior and deterministic output parity;
4. add the smallest useful parallel range;
5. retain serial mode and small-work thresholds as validation/product policy.

Never debug a new data model, new lifetime, and new parallel scheduling policy simultaneously if a serial intermediate can isolate them.

### 5. Capability-Preservation and Deletion Ledgers

Every prompt produces two explicit review ledgers:

- **Preserve:** current features, backends, packages, workflows, and observable behavior that must remain.
- **Delete:** old pointers, futures, waits, copies, caches, callbacks, APIs, compatibility facades, object paths, or diagnostics replaced by the prompt.

A deletion may move to a named later prompt only when simultaneous removal would make the current vertical slice impossible. It must have an owner, exact target prompt, and no new call sites.

### 6. Evidence Before Claims

- A class/API existing is not proof that it works.
- A multithreaded trace is not proof of speedup.
- No race observed is not proof of race freedom.
- One backend passing is not parity.
- FPS alone is not a performance result.
- A test listed but not run is not evidence.

Report exact commands, configurations, worker counts, backend, validation state, workload, and relevant before/after values. Mark unavailable evidence as unavailable; do not invent a new reporting system to fill the gap.

### 7. Stop Conditions

Stop and report rather than guessing when:

- the prerequisite prompt has not passed;
- a required current feature cannot be preserved within the proposed ownership model;
- repository state contradicts J's audit materially;
- a new public API has no necessary cross-module consumer;
- a supposedly independent range has no provable exclusive outputs;
- the serial reference differs and the difference is unexplained;
- native D3D12/Vulkan validation reports an ownership/state error;
- completion requires a broad new subsystem outside the prompt.

### 8. Existing Concurrency Has No Grandfather Clause

J's LC-01 through LC-18 ledger is binding, and Prompt 00 must extend it when the repository changes. For every existing or newly encountered thread, future, mutex, atomic, native/Qt wait, device-idle call, detached process, callback registry, queue lock, or allocator lock:

1. assign an owner and protected invariant;
2. classify it as **keep + harden**, **owner-only**, **replace**, **delete wait**, or **prove or remove**;
3. name the prompt that closes it and the evidence that proves closure;
4. prohibit new consumers of a mechanism already marked for replacement;
5. when touching its ownership path, migrate or narrow it in the same prompt unless doing so would violate a prerequisite; any deferral needs an exact later prompt and no expanded use;
6. never invoke arbitrary callbacks, UI, provider/driver, I/O, logging, or destruction work while holding an engine lock unless a documented API contract makes it unavoidable and a re-entry test exists;
7. do not replace a legitimate boundary mutex with tasks merely to reduce the primitive count. The goal is zero **unclassified** synchronization, not zero synchronization.

Each prompt reruns the concurrency search in its touched subtree and reconciles its results with the legacy ledger. Prompts 22 and 29 rerun it repository-wide.

### 9. Professional Hazard Pre-Mortem

Before editing for any prompt, read J's MT-01 through MT-44 Professional Multithreading Failure Atlas and record the applicable hazard IDs. For each applicable ID:

1. point to the NVIDIA, AMD/GPUOpen, Epic, language, or API precedent behind the chosen recognizable pattern;
2. state how the mistake could enter the exact Sparkle path being changed;
3. state the ownership/progress/lifetime/performance invariant that prevents it;
4. name an assertion, deterministic test, injected failure, native validation run, or timeline experiment capable of disproving the implementation;
5. execute that proof before closing the prompt; “not observed,” high utilization, one clean run, or a vendor sample doing something similar is insufficient;
6. if a source sample uses a lock, wait, lazy creation, fixed partition, or simple pool, copy neither mechanism nor name automatically—first prove that its scope and tradeoff match Sparkle's production path.

Newly discovered distinct hazards extend J and this traceability table, not a new document, runtime hazard registry, lint framework, or shipping diagnostics API.

### 10. Canonical NVIDIA/AMD/Epic-Grounded Vocabulary

J's **Canonical Concurrency and Rendering Vocabulary** is binding. Before adding or renaming a symbol, inspect the exact responsibility and the current repository's established convention, then use the canonical Sparkle term:

- CPU scheduled work is `Task`; a physical executor agent is a worker `Thread`; recorded RHI/GPU work is a `CommandList`/`CommandBuffer`; unqualified/native `CommandQueue` means GPU submission, while every CPU queue fully qualifies its payload.
- Use `TaskNodeHandle`, `CompiledTaskGraph`, `TaskExecution`, `TaskExecutionContext`, `TaskExecutor`, `TaskExecutorConfig`, `TaskScope`, `TaskEvent`, `ParallelFor`, and `TaskLane::{FrameCritical, Background, BlockingIo}` for the planned task runtime. “Job system” is the architectural category, not a second C++ type family.
- Use `GameThread`, `EditorThread`, and `RenderThread` for physical roles; use `RenderCoordinator` for the owning service. Reserve `RHIThread` until a real deferred RHI translation thread exists.
- Use `RenderFrameQueue` for bounded frame publication and `RenderControlCommandQueue` for ordered renderer control. Do not call either a GPU command queue.
- Use the existing `RenderCommandContext` for pass-facing recording, public `RhiCommandRecordingLease` for move-only exclusive borrowing, backend-private `<Backend>CommandRecordingContext` for native recording ownership, existing `ERhiQueueType` for GPU queue class, and existing `RhiSubmissionToken` for GPU completion/order.
- `Handle` identifies, `Token` proves, `Lease` borrows exclusively, `Context` supplies transient state, `Scope` owns asynchronous lifetime, `Execution` is one submission, and `Lane` is scheduling policy. A new name must obey the lifetime meaning it advertises.
- Do not introduce `Job`, `WorkItem`, bare `Context`, bare `Queue`, `*Manager`, `Async*`, `ThreadSafe*`, `LockFree*`, `MultiThreaded*`, `New*`, `*2`, or `MT*` as an escape from a precise responsibility. A justified established existing name is reconciled, not mechanically churned.

Every prompt searches for both the canonical term and rejected aliases in its touched scope. A rename updates filenames, C++ symbols, tests, profiler/debugger labels, comments, CMake references, and documents in one prompt; compatibility aliases are temporary within that prompt and deleted before its gate. The completion report must include a naming reconciliation even when the decision is to retain an accurate existing name.

### 11. Readable Product Code and Proportionate Validation

Concurrency correctness must be encoded at the narrowest reusable ownership boundary, not repeated as logging/assertion boilerplate throughout orchestration code. Follow the owner/context/lease separation used by the NVIDIA, AMD, Epic, and O3DE references in J:

- implementation types own mechanism and invariants; orchestrators express order and policy in short, readable calls;
- prefer an owner-bound access type, scoped lease, typed token, or one gateway assertion over copying `AssertOwnerThread("method")` into every facade method;
- use generalized abstractions only when at least two real consumers share the same lifetime semantics; do not generalize feature policy into Core;
- place CVars/settings in the owning subsystem's dedicated `*CVars`/settings unit, never in a general parser, launch loop, or unrelated orchestrator;
- add only essential correctness assertions, failure diagnostics, and profiler labels with a concrete vendor/API precedent and a falsifying use; no per-item log spam, speculative counters, or diagnostic mirrors of ordinary state;
- do not run a full repository build, all scenes, all backends, and heavyweight captures after every prompt or every edit. At the end of an ordinary code prompt, run the smallest affected target compile and focused deterministic test once. Full D3D12/Vulkan product builds and representative-scene matrices belong to integration gates 00, 05, 13, 20–22, and 29, or when the changed ABI/backend boundary specifically requires them;
- performance captures belong to measurement prompts 05, 23–25, 28, and 29. Other prompts reuse the last compatible baseline and add a new capture only when their acceptance claim depends on timing;
- documentation-only prompts run link/search/format checks, not product builds.

The completion report must say why each validation action was proportionate. More logs, assertions, targets, and repeated full builds are not stronger evidence when they do not cross the changed invariant.

## Required Prompt Completion Report

Every completed prompt returns this report:

1. **Outcome:** what now works; do not imply later stages.
2. **Repository audit:** counterpart searches and decisions.
3. **Ownership change:** owners, readers/writers, publication and lifetime edges.
4. **Files changed:** grouped by responsibility.
5. **Refactors:** nearby debt/duplication removed.
6. **Preservation ledger:** features/backends/workflows verified.
7. **Deletion ledger:** old paths deleted; remaining dated items.
8. **Validation:** commands and results, including serial/worker/backend modes.
9. **Performance evidence:** only metrics relevant to this prompt.
10. **Learning teach-back:** invariant, one rejected alternative, one failure test, one cost.
11. **Legacy-concurrency reconciliation:** LC IDs touched, retained-invariant proof, mechanisms deleted, newly discovered hits, and named deferrals.
12. **Hazard closure:** applicable MT IDs, source-backed pattern, falsifying evidence, and unresolved risk.
13. **Naming reconciliation:** canonical terms and rejected aliases searched, use/rename decisions, profiler/thread-label updates, and proof that no permanent synonym remains.
14. **Gate:** PASS or BLOCKED with concrete reason.

## Prompt Sequence and Dependencies

| Prompt | Deliverable | Depends on |
|---:|---|---|
| 00 | baseline, vocabulary, ownership and preservation inventory | current repository |
| 01 | serial task graph and task-execution contracts | 00 |
| 02 | fixed worker executor, work stealing, sleep/wake | 01 |
| 03 | dependencies, scopes, cancellation, lanes, shutdown, instrumentation | 02 |
| 04 | shader/texture/tool SparkleTasks pilots | 03 |
| 05 | ECS `EntityId`, registry, sparse-set component storage | 03 |
| 06 | ECS queries, structural epochs, deterministic entity commands | 05 |
| 07 | current scene data converted to ECS components/facades | 06 |
| 08 | explicit transform/camera derived-data system and change journal | 07 |
| 09 | transactional asynchronous level/scene loading | 06, 08 |
| 10 | ECS-aware system graph and parallel animation/morph/skinning | 08 |
| 11 | immutable editor scene model, commands, transactions, operations | 09, 10 |
| 12 | stable render IDs, immutable packet/delta contract, headless replay | 08, 10 |
| 13 | dedicated render coordinator and bounded `RenderFrameQueue` | 12 |
| 14 | editor UI/viewport/capture packet conversion | 11, 13 |
| 15 | persistent render/GPU scene and dirty-range updates | 13 |
| 16 | generation-based shader/asset residency and deferred retirement | 04, 15 |
| 17 | renderer preparation task DAG | 10, 15 |
| 18 | D3D12 worker recording contexts and transient allocation | 17 |
| 19 | Vulkan worker recording contexts and transient allocation | 17 |
| 20 | frame-graph recording plan and first parallel passes | 18, 19 |
| 21 | intra-pass scaling and advanced-feature preservation closure | 20 |
| 22 | tools/editor/reliability/package closure and legacy deletion | 14, 16, 21 |
| 23 | initial full-system performance characterization and tuning | 22 |
| 24 | atomic protocol and scheduler-pathology hardening | 23 |
| 25 | CPU topology, worker policy, contention, and false-sharing characterization | 24 |
| 26 | reduction, scan/compaction, partition, and deterministic merge in real paths | 25 |
| 27 | staged I/O and cold-cache PSO/resource-creation hitch control | 26 |
| 28 | GPU queue concurrency, frame pacing, and correlated latency | 27 |
| 29 | production forensics, expert defense, and final portfolio release | 28 |

## J-to-K Scope Traceability

This ledger is a completeness check, not a substitute for reading J. A prompt may refine several concerns, but every major design area in J has a named implementation home and a final proof point.

| J design/learning area | Primary implementation prompts | Closure evidence |
|---|---|---|
| ownership, memory ordering, publication, lifetime, failure vocabulary | 00-03, 24 | ownership ledger, scheduler/protocol stress, atomic state-machine teach-back |
| engine-owned job system and structured concurrency | 01-04, 24-25 | serial executor equivalence, 0/1/2/N/topology stress, shutdown/cancellation/pathology tests |
| ECS/DOD foundations without framework sprawl | 05-08 | storage/query tests, deterministic command playback, scene conversion and change journal |
| game-system graph and useful simulation parallelism | 08-10 | dependency validation, serial/parallel equivalence, animation/skinning measurements |
| transactional loading and asset work | 04, 09, 16, 22, 27 | deterministic outputs, generation/cancellation/cold-cache tests, no partial publication |
| editor ownership, undo/redo, operations, UI affinity | 09-11, 14, 22 | immutable models, semantic commands, lifecycle and cancellation stress |
| immutable game/render boundary and bounded pipelining | 12-14, 24, 28 | packet replay, atomic `RenderFrameQueue` tests, input-to-present and backpressure evidence |
| persistent render/GPU scene and lifetime-safe residency | 15-16 | dirty-range updates, generation retirement, delayed-GPU stress |
| renderer preparation task graph | 17 | serial/parallel equivalence, critical-path and granularity evidence |
| view visibility/LOD, light classification, shadow planning/caster lists, retained/dynamic draw preparation | 15, 17, 21, 26 | stable visible/light/caster/draw sets, cache invalidation, transparent order, large-scene critical path |
| D3D12 and Vulkan native recording ownership | 18-20 | native validation, token/reset misuse tests, worker migration stress |
| frame-graph authority, barriers, recording groups, submission | 20 | compiled plan inspection, serial/parallel execution of the same plan |
| command preparation versus native recording versus optional software translation versus aggregation/submission batching | 17-21, 28 | representation/owner ledger, direct-path parity, list/chunk/batch metrics, explicit RHI-thread ADR rejection |
| advanced graphics and vendor-neutral feature preservation | 16, 21-23, 27-29 | backend/feature matrix, image correctness, temporal/history, queue and capture tests |
| tools, packages, public-surface reduction, legacy deletion | 04, 22 | repository audit-to-zero, reproducible products/packages, closed deletion ledgers |
| reduction/scan/compaction and deterministic parallel algorithms | 06, 10, 15, 26 | serial oracle, randomized edge cases, stable merge and crossover measurements |
| CPU topology, OS scheduling, contention and oversubscription | 03, 23-25 | optimized system traces, topology metadata, worker-count and third-party-thread matrix |
| staged I/O, PSO/resource creation and first-run hitch policy | 04, 09, 16, 27 | bounded stage stress, cold/warm cache evidence, memory/late/miss/fallback metrics |
| shader workers, graphics/compute/RT pipeline creation, buffer/image/view creation, allocation/binding and descriptor writes | 16, 18-19, 21, 27 | separate stage ownership, native safety audit, key deduplication, cold-cache memory/concurrency matrix |
| GPU queue overlap, presentation pacing and latency | 13, 20-21, 28 | correlated frame markers, graphics/compute/copy timelines, pacing and CPU-lead results |
| production concurrency diagnosis and interview defense | 22, 24-29 | injected incidents, exact tool evidence, regressions, coding/whiteboard/trace defense |
| reliability, determinism, performance, and portfolio teaching proof | every prompt, finalized by 22-29 | stress matrix, reproducible measurements, limitations, independent teach-back |
| NVIDIA/AMD/Epic-grounded professional failure prevention | every prompt, repository closure in 22 and 29 | MT-01–MT-44 pre-mortem, recognizable source-backed pattern, falsifying evidence, final non-applicability/closure review |
| NVIDIA/AMD/Epic-grounded canonical vocabulary | 00-03, 12-13, 18-20; enforced by every prompt | naming crosswalk, no CPU-task/GPU-command ambiguity, rejected-alias audit, truthful thread/profiler labels |

If a later repository discovery exposes a J responsibility with no row or prompt, update this ledger and the smallest owning prompt before implementing it. Do not hide the new obligation in a completion report.

## Renderer/RHI Use-Case-to-Prompt Coverage

This is the execution ledger for J's completeness audit. “Covered” means the named prompt must either retain a useful implementation with proof or record a source-backed non-applicability/defer decision. A general task runtime, one parallel pass, or one PSO future does not close the row.

| Renderer/RHI use case | Prompt owner(s) | Mandatory closure evidence |
|---|---:|---|
| render-proxy delta apply and persistent GPU scene | 12, 15 | stable ID/generation, dirty-only update, serial replay |
| transforms, bounds and previous-frame data | 08, 10, 17, 26 | exclusive ranges, reduction oracle, temporal parity |
| per-view visibility, relevance and LOD | 17 | stable visible set per view, 0/1/N parity, large/small crossover |
| light visibility/classification and compact light data | 17 | stable light IDs/order and reservoir/reference-lighting parity |
| shadow views/frusta, light-scene intersection and caster lists | 17, 21 | per-light/view ownership, stable caster order, draw-heavy recording proof when path exists |
| skinning and morph preparation | 10, 17 | world-to-render dependency, range ownership, image/pose parity |
| pass/material/pipeline eligibility | 17 | immutable lookup tables, no lazy runtime/cache mutation |
| retained/static and dynamic draw preparation | 15, 17 | current `MeshDraw`/`MeshInstanceBatch` reuse decision, invalidation and lifetime tests |
| sorting, state bucketing, instancing and draw merging | 17, 26 | stable key/tie-break, transparent order, measured batch improvement |
| per-pass/view constants and descriptors | 17-20 | preassigned/lease-local ranges, overflow/retirement tests |
| GPU-scene dirty compaction and upload plan | 15, 26 | scan/list-merge oracle, bounded dirty ranges, token retirement |
| BLAS inputs and build/update/refit/compaction decision | 16, 17, 21, 27 | asset/generation ownership, scratch/result lifetime, backend validation |
| classic TLAS and PTLAS planning | 17, 21 | distinct initial/update/add/remove/reload matrices |
| frame-graph setup and compile work | 17, 20, 23 | serial owner baseline; only measured private producers/algorithms retained |
| shader cook/compile workers | 04, 22, 27 | bounded processes/tasks, deterministic packages, third-party-thread budget |
| shader package/reflection/layout generation | 16, 27 | immutable generation publish, stale rejection, failure fallback |
| graphics/compute PSO precache/create | 27 | key discovery/dedup, cold-cache hit/miss/too-late, memory and fallback |
| RT pipeline/library/collection creation | 21, 27 | backend capability decision; retain only useful measured path |
| buffer/image/view creation | 16, 27 | native thread-safety audit, exclusive output, owner commit |
| memory allocation/suballocation/binding | 15, 16, 27 | lifetime domain, contention/memory budget, native validation |
| persistent and transient descriptor allocation/update | 18, 19, 27 | lifetime split, no recording hot-lock, update visibility contract |
| decode/decompress/transcode/upload preparation | 04, 09, 16, 27 | staged states, no blocked frame worker, generation readiness |
| D3D12 allocator/list and Vulkan pool/buffer leases | 18, 19 | exclusive use, token-gated reset, worker migration stress |
| pass-level native command recording | 20 | same compiled plan/order/states, both native validation paths |
| intra-pass draw/dispatch/RT-input recording | 21 | measured selected passes, bounded chunks, deterministic order |
| software RHI command stream and translation thread | 20, 29 | explicit know/defer ADR; implementation only after J's profiling/platform gate |
| recording-group aggregation and native submit batching | 20, 21, 28 | order-key fan-in, list/chunk/batch counters, GPU starvation/latency tradeoff |
| barriers, preambles/postambles and queue ownership | 20, 28 | compiler authority, coordinator submit, state/wait equivalence |
| deferred resource/descriptor destruction | 15, 16, 22 | last-use token, delayed-GPU and reload/resize/shutdown tests |
| readback, screenshot, capture encode/write | 14, 21, 22 | staged GPU/CPU ownership, provider affinity, cancel/shutdown proof |
| validation callbacks, device loss and crash evidence | 19, 22, 29 | bounded callback intake, owner processing, injected failure narrative |
| offline distributed static-light build | 29 | know/defer record unless a real Sparkle static-light product is separately approved |
| GPU async queues and GPU-driven command generation | 28, future renderer program | explicitly not claimed as CPU multithreading; measured GPU proof only |

At Prompt 29, every row must point to tests/captures/code, a retained non-applicability decision, or an explicitly separately approved future program. “The job system could do this” is not closure.

## Failure-Atlas-to-Prompt Traceability

This is the minimum hazard pre-mortem scope. A prompt must add any other MT ID made applicable by the actual files it touches.

| Prompt(s) | Minimum J hazard IDs | Required recognizable proof family |
|---:|---|---|
| 00 | MT-01–12, MT-16–18, MT-23–27, MT-31–44 | repository ownership/blocking census, before-state failure reproductions, thread/idle/lock/allocator/native call-chain inventory |
| 01–03 | MT-03–06, MT-10, MT-12–25, MT-41–44 | serial DAG oracle, exactly-once settlement, predicate sleep/wake, nested-child exhaustion, no busy wait, grain/topology/false-sharing stress |
| 04 | MT-05–10, MT-16–19, MT-23–24, MT-27, MT-29, MT-31, MT-41–43 | scoped process/I/O lifetime, Qt affinity, bounded memory/concurrency, deterministic transactional fan-in, close/cancel/failure stress |
| 05–08 | MT-01–04, MT-11, MT-19–21, MT-25–31, MT-41–43 | stable generational identity, frozen structural epochs, exclusive query ranges, deterministic command playback, DOD/cache evidence |
| 09–11 | MT-01–10, MT-16, MT-23–24, MT-27, MT-29–31, MT-41–43 | isolated generation build/commit, immutable editor model, owner commands, undo/redo and document-close cancellation stress |
| 12–14 | MT-01–12, MT-23, MT-27, MT-29, MT-31, MT-33–40, MT-41–43 | packet replay, bounded `RenderFrameQueue`, owner affinity, delayed consumer, copied UI data, sequenced commands, latency/backpressure evidence |
| 15–16 | MT-01–10, MT-23, MT-26–31, MT-33, MT-36–39, MT-41–43 | persistent proxies, generation readiness, dirty ranges, delayed-GPU retirement, no ordinary idle, failure retains prior generation |
| 17 | MT-02–06, MT-09, MT-13, MT-19–31, MT-36, MT-39, MT-41–43 | immutable task inputs, task-private ranges, critical-path DAG, skew/grain tests, deterministic merge, prewarmed runtime |
| 18–20 | MT-09–10, MT-13, MT-19–21, MT-25–27, MT-29, MT-32–41, MT-43 | exclusive backend leases, token reset, native validation, same compiled order, group-size/list/submit/memory sweep, no worker submit/wait |
| 21 | MT-19–21, MT-27, MT-29, MT-32–43 | measured intra-pass ranges, deterministic order, full advanced-feature/backend matrix, CPU gain separated from GPU/list/memory cost |
| 22 | MT-01–44 | repository audit-to-zero, closure of every applicable falsification test, no duplicate/legacy mechanism |
| 23–25 | MT-17–28, MT-35, MT-38, MT-41–44 | topology and worker matrix, contention/cache/context-switch traces, crossover/negative scaling, profiler-overhead control |
| 26–27 | MT-04–07, MT-13, MT-16, MT-19–31, MT-33, MT-36–37, MT-41–43 | deterministic parallel algorithm oracle, bounded staged I/O, cold-cache PSO/resource state machine, generation rejection |
| 28 | MT-17–18, MT-24–27, MT-32–43 | correlated CPU/task/queue/present identity, queue overlap proof, provider ownership, pacing/backpressure and input-latency matrix |
| 29 | MT-01–44 | injected incidents, independent reproduction, final zero-unclassified audit, live design/code/trace defense |

The range notation is inclusive. It does not mean every hazard needs a bespoke test in every prompt; reuse the smallest existing proof that actually crosses the changed ownership path. It does mean no applicable hazard may be omitted because the happy path passed.

## Prompt 00 — Establish the Before-State and Invariants

~~~text
Implement Prompt 00 from K_MultithreadedEngineImplementationPromptSeries.md.

Objective:
Create the verified before-state and invariant vocabulary required for every later multithreading change. This prompt changes no ownership architecture and introduces no new task/ECS/render framework.

Non-negotiable repository rules:
- Enforce J's canonical concurrency/rendering vocabulary and Rule 10: search canonical terms plus rejected aliases in the touched scope, use one responsibility per name, and delete temporary compatibility spellings before the gate.
- Search before adding; use/extend/replace existing timing, validation, launch-setting, and test facilities.
- Apply the daily-refactor rule to touched baseline/launch code and remove only directly exposed duplication/dead code.
- Do not add a profiler framework, task panel, runtime log stream, default JSON/CSV report, public diagnostics snapshot, or permanent feature-registry system.
- Preserve all product rendering paths and both backends.

Inspect first:
- RuntimeApplication and EditorApplication frame order.
- GameScene update/snapshot and level lifecycle.
- Renderer/RendererSystemRoot/FramePipeline stages.
- frame-graph compile/execute/submission.
- RHI frame resources, queue timelines, WaitForIdle sites, allocator/profiler hooks.
- current benchmark/test launch mechanisms and environment/config controls.
- current names, filenames, profiler labels, and thread labels for task/job/work, execution/run, graph/node, context/handle/token/lease, main/game/editor/render/RHI threads, queues, commands, frame publication, and GPU submission; classify semantic collisions rather than matching text alone.
- J's binding LC-01 through LC-18 legacy-concurrency ledger, then every owned-source hit for standard/Qt/native threading, locks, atomics, waits, detach, task-runtime, and device-idle mechanisms. Exclude generated/external trees from ownership, but record wrapped third-party worker behavior.
- A, E, G, H, and J governing documents.

Required implementation:
1. Adopt J's canonical naming crosswalk for `FrameId`, `SceneGeneration`, `SequenceNumber`, `Task`, `TaskExecution`, thread roles, packet lifetime, recording ownership, and GPU completion without duplicating existing strong types. Produce a before-state rename ledger with **keep**, **rename in owning prompt**, or **delete as alias** for every conflicting term. Dormant future controls use canonical labels from day one.
2. Add owner-thread assertions to current renderer/RHI mutators where an existing assertion mechanism can be reused.
3. Add or consolidate profiler-visible scopes for game update, snapshot/extraction, renderer scene build, GPU-data build, graph setup/compile, record, submit, present, and idle waits using existing hooks only.
4. Add development/test-only launch controls for worker count, serial task mode, threaded renderer, parallel recording, and pipeline depth. Controls may be dormant until later prompts but must have one owner and no shipping UI.
5. Regenerate the legacy-concurrency ledger from the current tree. For every thread/future/mutex/atomic/condition variable/Qt or native wait/detach/device-idle hit, record file/call path, owner, users, invariant, blocking/affinity/lifetime policy, disposition, exact closing prompt, and falsifying test. New use of replacement-bound code is forbidden.
6. Classify every WaitForIdle call site—not only the wrapper API—as shutdown, device reinitialization, frame-slot/resource reuse, rare boundary, or data-path debt. Detect nested drains such as renderer → device services → swap chain and assign one owner/replacement.
7. Record lock-order and callback-under-lock findings. At minimum verify InputSystem callback dispatch, logger registry/sink, Streamline provider calls, D3D12/Vulkan queue synchronization, descriptor allocators, GPU allocation records, Vulkan diagnostics, and D3D12 linear allocator reset/publication.
8. Record reproducible baseline commands/cameras/workloads for tiny, Sponza, animation, RT/classic TLAS, PTLAS where supported, editor viewport, shader cook, and texture cook.
9. Record current feature-preservation status without claiming unsupported features.
10. Produce the initial MT-01–MT-44 hazard pre-mortem for the current architecture. Link every applicable hazard to a current source path, selected professional pattern, owning future prompt, and falsifying test/trace. Record non-applicable only with a concrete architectural reason.
11. Reserve truthful debugger/profiler role labels: `Sparkle.GameThread`, `Sparkle.EditorThread`, `Sparkle.RenderThread`, `Sparkle.Task.FrameCritical.N`, `Sparkle.Task.Background.N`, `Sparkle.Task.BlockingIo.N`, and `Sparkle.ToolMain`. Do not emit labels for roles that do not exist yet, especially `RHIThread`.

Validation:
- Build/run current D3D12 and Vulkan configurations with native validation as already supported.
- Capture the current CPU/GPU timeline through existing tools.
- Verify launch controls parse deterministically and do not alter default behavior.
- Run existing tests and representative current scenes.
- Exercise an InputSystem callback that unsubscribes itself under a watchdog or equivalent isolated harness; record the current failure/risk as before-state evidence without silently fixing it in this baseline-only prompt.
- Run git diff whitespace/include/dependency checks.

Acceptance gate:
- Current serial frame flow and owners can be explained from evidence.
- Baseline reproduction commands and essential captures exist in the existing artifact workflow.
- Every owned concurrency primitive, WaitForIdle call site, detach, and direct lifecycle callback has a classification and exact target prompt; no category is summarized only at wrapper level.
- No new observation product or public diagnostic API was created.
- Preservation and deletion ledgers are complete.
- The naming ledger distinguishes CPU tasks, OS threads, render control commands, RHI command recording, GPU queues, and completion proofs; every planned rename has one exact owning prompt and no future control/profiler label uses a rejected alias.
- The hazard ledger names current exposures—including raw game/render lifetime, callback-under-lock, polling/blocking waits, naive worker policy risk, shared allocation/queue state, nested device-idle, and measurement gaps—without implying later fixes already exist.

Positive patterns: evidence before design, owner assertions, narrow vocabulary, current tool reuse.
Forbidden: implementing the job system, moving renderer threads, speculative metrics infrastructure, broad cleanup.
~~~

## Prompt 01 — Build the Serial Task Graph Contract

~~~text
Implement Prompt 01 from K_MultithreadedEngineImplementationPromptSeries.md only after Prompt 00 passes.

Objective:
Create SparkleTasks' deterministic serial foundation: task identity, immutable compiled topology, execution generations, prerequisites, fan-in, nested completion semantics, explicit failure, and bounded graph storage. Do not create worker threads yet.

Non-negotiable repository rules:
- Enforce J's canonical concurrency/rendering vocabulary and Rule 10: search canonical terms plus rejected aliases in the touched scope, use one responsibility per name, and delete temporary compatibility spellings before the gate.
- Search for existing task, graph, event, handle/generation, arena/pool, result/error, and test-runner counterparts before adding types.
- Reuse or refactor coherent Core facilities; do not create parallel handle/result/assertion families.
- Leave touched CMake/Core/test code cleaner and delete temporary duplicate scaffolding.
- Keep Tasks independent of GameFramework, Renderer, RHI, Editor, Assets, Platform windows, and product policy.
- Pre-mortem MT-03–06, MT-10, MT-13–15, MT-23, MT-29, and MT-44. Model the familiar prerequisite/subsequent and nested-completion semantics described by Epic Tasks and structured execution; do not copy a sample pool's weaker lifetime contract.

Required implementation:
1. Add the Tasks module with the minimal public contracts from J: `TaskName`/`TaskDesc`, builder-local `TaskNodeHandle`, `TaskGraphBuilder`, immutable `CompiledTaskGraph`, `TaskExecution`/`TaskExecutionContext`, `TaskResult`, and serial `TaskExecutor`. Do not add `Job`, `WorkItem`, `Future`, `TaskRun`, or a public `TaskManager` alias.
2. Use generation-validated tokens/handles; no public raw TaskRecord pointers.
3. Compile and reject cycles, self/duplicate/foreign edges, stale tokens, capacity overflow, and invalid completion policies before execution starts.
4. Implement deterministic topological serial execution with prerequisites, WhenAll, continuation, and nested/group unfinished-count semantics.
5. Define failure propagation: first failure retained, normal dependents cancelled, explicit cleanup/finally nodes settle.
6. Bound per-execution task/edge storage; overflow returns controlled failure without partial execution.
7. Keep queue/record/topology implementation private and add no timing/reporting product surface.

Validation:
- Unit tests: single, chain, fan-out/fan-in, diamond, nested completion, failure, cleanup, reusable graph with separate contexts, stale handles, all invalid graph cases, configured overflow.
- Run every test repeatedly through the serial executor.
- Dependency-boundary test proves Tasks has no forbidden module include/link.
- Allocation/lifetime tests prove a run outlives all nodes/results it still needs and releases cleanly.
- Exhaustively enumerate small DAGs against a simple reference topological executor; inject failure/cancellation at each node and prove every accepted node/run has one terminal accounting path.

Acceptance gate:
- Serial graph output is deterministic and all counters settle exactly once.
- Invalid graphs fail before a task body executes.
- No worker, wait, lane, profiler, ECS, or renderer integration exists yet.
- Public concepts have a stated future consumer and private internals remain private.

Positive patterns: serial reference first, immutable topology, generation handles, bounded storage, explicit result policy.
Forbidden: std::future as task identity, fire-and-forget lifetime, worker threads, busy waiting, unbounded heap fallback.
~~~

## Prompt 02 — Add the Fixed Worker Executor

~~~text
Implement Prompt 02 only after Prompt 01 passes.

Objective:
Execute the same compiled task contract on a fixed worker set with local ready queues, external injection, work stealing, sleep/wake, and safe repeated startup/shutdown. Preserve exact serial semantics.

Non-negotiable repository rules:
- Enforce J's canonical concurrency/rendering vocabulary and Rule 10: search canonical terms plus rejected aliases in the touched scope, use one responsibility per name, and delete temporary compatibility spellings before the gate.
- Search for repository thread wrappers, naming/priority helpers, semaphore/condition-variable utilities, allocators, and queue implementations before adding counterparts.
- One executor owns workers; do not add feature/subsystem threads or a second pool.
- Apply daily refactoring to touched thread lifecycle utilities and remove duplicate naming/join code.
- Queue implementation is private and may begin correctness-oriented; do not expose it in public APIs.
- Pre-mortem MT-13–25 and MT-41–44. A fixed pool, work stealing, and parking are recognizable mechanisms, but thread count, lock-free queues, padding, and priority require Sparkle evidence rather than résumé value.

Required implementation:
1. Create a configured fixed worker set; support worker counts 0/1/2/N, where 0 uses serial execution.
   The host-facing configuration type is `TaskExecutorConfig`; do not add `TaskSchedulerConfig`, a public `TaskScheduler`, or a second pool configuration family.
2. Give each worker a local double-ended ready queue and implement opposite-end stealing; provide a synchronized external injection path.
3. Use semaphore/condition-variable parking with a lost-wakeup-safe predicate/protocol; no idle spin loop.
4. Completion atomically resolves dependent prerequisite counts and schedules newly ready nodes exactly once.
5. Use allocation-conscious task records/run arenas and pad/separate hot counters where measurements/layout inspection justify it.
6. Name workers through existing platform/debugger mechanisms. Priority/affinity remain conservative and private.
7. Define executor states Accepting, Draining/Cancelling, Stopping, Stopped; reject late submission deterministically and join every worker.
8. Retain the serial executor as the reference mode using the same task bodies/contracts.

Validation:
- Run Prompt 01 tests at 0/1/2/N workers with randomized yields/delays.
- Stress millions of dependency transitions, external submissions racing parking, stealing, queue wrap/growth policy, repeated executor construction/destruction, and shutdown with idle/queued/running work.
- Verify no task executes twice and every accepted `TaskExecution` settles.
- Use ThreadSanitizer-capable configuration or the strongest available equivalent; report limitations.
- Measure enqueue, local, stolen, and wake latency without creating a report framework.
- Compare fewer-than-physical, physical, logical, and oversubscribed worker settings where the machine permits; trace runnable threads/context switches and use a deliberately skewed workload. Inspect adjacent worker hot fields for false sharing.

Acceptance gate:
- Parallel results equal serial results for all deterministic tests.
- Idle workers park; no routine polling or worker blocking on child tasks.
- Repeated shutdown has no leak, stranded run, lost wakeup, or callback-after-destroy.
- Queue internals do not appear in public headers or editor diagnostics.

Positive patterns: fixed ownership, work-first scheduling, stealing, parked idle state, serial parity.
Forbidden: detached threads, one thread per system, busy-yield waits, silent exception swallowing, lock-free rewrite without reclamation proof.
~~~

## Prompt 03 — Complete Structured Task-Runtime Semantics

~~~text
Implement Prompt 03 only after Prompt 02 passes.

Objective:
Complete the production SparkleTasks contract: `TaskScope` hierarchy, cooperative cancellation, `TaskEvent`, `ParallelFor`, FrameCritical/Background/BlockingIo lanes, host joins, failure/finally semantics, private profiler events, and ordered shutdown. This is the engine job system, but `Job` is not a parallel API vocabulary.

Non-negotiable repository rules:
- Enforce J's canonical concurrency/rendering vocabulary and Rule 10: search canonical terms plus rejected aliases in the touched scope, use one responsibility per name, and delete temporary compatibility spellings before the gate.
- Search for cancellation, events, scoped handles, process/file completion, profiler scopes, lane/thread configuration, and shutdown orchestration counterparts.
- Do not repurpose Core Event as a cross-thread callback bus; its current callbacks remain owner-thread-affine.
- One SparkleTasks executor family serves engine/tools with host policy; no new pools.
- Refactor direct touched futures/cancellation helpers only when their consumer migrates in this prompt or Prompt 04.
- Resolve LC-05 now: InputSystem must declare registration/dispatch affinity and must never invoke a subscriber while holding `m_CallbackMutex`. Do not paper over re-entry with a recursive mutex.
- Pre-mortem MT-05–24 and MT-41–44. Explicitly reject generic busy-wait/task-help loops: Epic documents spinning, unrelated-task latency, circular dependency, and recursive stack failure. Sparkle uses dependencies, nested completion, and predicate parking.

Required implementation:
1. Add parent/child TaskScope for Application, World/Document, AssetGeneration, Frame, and ToolInvocation lifetimes.
2. Cancellation flows downward; scope completion flows upward. Owner destruction before settlement asserts in development.
3. Add cooperative cancellation checkpoints and exactly-once result settlement. Cancellation callbacks must not run arbitrary user code synchronously on the requesting thread.
4. Add one-shot generation-safe TaskEvent for external completion; no worker polling.
5. Add ParallelFor with explicit grain/serial threshold and exclusive range contract.
6. Add FrameCritical, Background, and BlockingIo policies so file/process waits cannot occupy FrameCritical workers and Background CPU work cannot consume all interactive capacity.
7. Detect/prohibit normal worker `TaskExecution::Wait`. Allow bounded owner-thread joins only at declared phase boundaries.
8. Emit task name/lane/run/worker/timing/status into existing profiler hooks privately; no public snapshot/panel/log stream.
9. Implement the shutdown order defined in J and stress cancellation/failure racing start/completion.
10. Refactor InputSystem dispatch to snapshot eligible callbacks under a narrow lock and invoke after unlock, or prove and enforce complete owner-thread affinity and remove the unnecessary lock. Preserve immediate/deferred ordering, handle identity, focus/layer routing, and define whether unsubscribe during a dispatch affects the current or next snapshot.

Validation:
- Scope tree cancellation/settlement tests, owner-destroy negative tests, stale event/double-trigger tests, cancelled prerequisite/finally behavior, ParallelFor partition coverage/non-overlap, invalid cross-lane dependency checks.
- Starvation test: sustained background/I/O work while frame tasks meet an explicit latency budget.
- Shutdown with event-blocked, cancelled, failed, and nested work.
- Parent-on-every-worker child-spawn test, empty-queue CPU-use test, lost-wakeup notify-before/after-park test, and proof that BlockingIo/native wait stacks never appear on frame workers.
- Input callback self-unsubscribe, unsubscribe-other, subscribe-during-dispatch, nested dispatch, deferred dispatch, and slow callback tests; none runs user code under the registry lock.
- Profiler capture shows names/lanes/dependencies without changing shipping public surface.

Acceptance gate:
- Every accepted `TaskExecution` settles exactly once under cancellation/failure/shutdown races.
- Raw owner capture beyond scope lifetime is prohibited by design/tests.
- Frame and blocking lanes cannot starve one another.
- No second executor/pool, task UI, public task diagnostics, or new runtime reporting system exists.
- LC-05 is closed, including a stated re-entrancy/snapshot semantic and no callback-under-lock.

Positive patterns: structured concurrency, cooperative cancellation, continuations, bounded host waits, private instrumentation.
Forbidden: detached work, arbitrary cancellation callbacks, worker waits, polling futures, priority used as correctness.
~~~

## Prompt 04 — Prove SparkleTasks in Real Tool Workflows

~~~text
Implement Prompt 04 only after Prompt 03 passes.

Objective:
Replace ad-hoc/as-serial application and tool concurrency with SparkleTasks in coarse, useful pilots: shader recook process coordination, launcher operations/process I/O, texture request cooking, and safe shader cook nodes. Preserve deterministic transactional output and explicit external-process lifetime.

Non-negotiable repository rules:
- Enforce J's canonical concurrency/rendering vocabulary and Rule 10: search canonical terms plus rejected aliases in the touched scope, use one responsibility per name, and delete temporary compatibility spellings before the gate.
- Search each tool for existing planners, batches, compiler sessions, COM/thread setup, process readers, caches, registries, publication, status/progress, and cancellation.
- Extend existing plans rather than creating alternate Async cookers or parallel pipelines.
- One task runtime with tool host policy; no std::async, detached reader threads, or tool-specific pools after migration.
- Refactor duplicated process/result/publication logic in the touched path and delete the old execution route in this prompt.
- Close LC-01 through LC-04 and LC-17 for the migrated call paths. Qt `startDetached` may remain only when launching a deliberately independent product; it may not substitute for scoped operation lifetime.

Required implementation:
1. Replace ShaderRecookCoordinator std::async/future with scoped Background/BlockingIo tasks and owner-thread structured result consumption.
2. Preserve latest/queued request behavior through explicit request/publication generations; failure keeps old shader packages active.
3. Move blocking child-process pipe handling to BlockingIo; cancellation signals child policy, drains/closes pipes, retains diagnostics, settles once.
4. Replace LauncherBackend's QThread-per-operation path with the scoped operation service and immutable progress/result delivery queued to the Qt owner thread. Migrate ProcessRunner's reader `std::thread`, 50 ms child polling, and standalone atomic cancellation into structured BlockingIo completion. Window/QObject access remains on the GUI owner.
5. Audit AssetCookerToolProcess's infinite process wait. Keep a synchronous adapter only for the outer CLI host; any scheduled caller uses BlockingIo, cancellation, handle cleanup, diagnostics, and exactly-once settlement.
6. Preserve intentionally independent launcher/restart process behavior, but replace timed handoff assumptions where an owned readiness/exit protocol is possible. Record the owner of every remaining detached process.
7. Parallelize texture requests only after auditing COM/decoder/cooker safety. Use per-task context and a weighted decompressed-byte limiter.
8. Execute safe shader cook plan nodes with per-worker/session constraints and internal-compiler-thread limits.
9. Collect diagnostics by stable asset/package key and emit packages/registries only at deterministic fan-in.
10. Write generation-temporary output, validate complete set, atomically publish; cancelled/failed generations never become active.

Validation:
- Serial/parallel outputs byte-identical wherever formats promise determinism; otherwise stable semantic/package/reflection identity comparison.
- Cancellation at launch, pipe read, texture decode, shader compile, fan-in, and pre-publication.
- Launcher operation start/cancel/restart/application-close stress; all UI progress/results arrive on the Qt owner, no QThread per operation remains, and child handles/pipes settle exactly once.
- Asset-cooker synchronous-host and scheduled BlockingIo paths have equivalent exit code/diagnostic behavior; no SparkleTasks worker performs an infinite native wait.
- One node failure leaves previous publication active and no partial registry accepted.
- Interactive run proves tool work does not starve the FrameCritical lane.
- Memory stress with multiple 4K/8K/HDR textures respects weighted limit.

Acceptance gate:
- Shader recook future/std::async, LauncherBackend operation QThreads, migrated reader threads/poll loops, and duplicate cancellation paths are deleted.
- No duplicate cooker/compiler/publication pipeline exists.
- Every remaining `startDetached`, native child wait, or Qt handoff delay has an explicit independent-process/outer-host justification and no task-lifetime responsibility.
- DXIL/SPIR-V, reflection/layout, IDs, registry order, and diagnostics match serial contract.
- Tool throughput evidence includes peak memory and explains internal compiler/compressor threads.

Positive patterns: coarse tasks, task-local contexts, deterministic fan-in, transactional publication, bounded memory.
Forbidden: parallel publish-as-finished, shared unsafe importer/compiler instance, unbounded texture jobs, UI callbacks from I/O thread.
~~~

## Prompt 05 — Build the Serial ECS Identity and Storage Kernel

~~~text
Implement Prompt 05 only after Prompt 03 passes. Prompt 04 may proceed independently but must use the same SparkleTasks contracts.

Objective:
Replace the unused owning Entity model with the private serial ECS foundation: generational EntityId, registry, per-type sparse-set component storage, stable schema IDs, and invariant tests. Do not parallelize systems yet.

Non-negotiable repository rules:
- Enforce J's canonical concurrency/rendering vocabulary and Rule 10: search canonical terms plus rejected aliases in the touched scope, use one responsibility per name, and delete temporary compatibility spellings before the gate.
- Search Entity, Component, SceneCameras/Meshes/Lighting/Animations, handles, registries, sparse/dense containers, type IDs, serialization schemas, and editor selection before adding types.
- ECS internals remain GameFramework-private; do not add a standalone ECS SDK/module or expose storage to Renderer/Editor.
- Keep current scene behavior working; no second mutable source of truth may survive a converted slice.
- Apply daily refactoring to the old Entity/Component path and remove only behavior made redundant by this prompt.

Required implementation:
1. Add opaque EntityId(slot,generation), invalid value, validation, free-list/reuse, and explicit generation wrap policy.
2. Add EntityRegistry with create/destroy/is-alive and one private type-erased storage registry with typed call sites.
3. Add ComponentStorage<T>: sparse slot-to-dense lookup, dense EntityId array, dense T array, add/remove/replace/get/contains, swap-remove bookkeeping, reserve/bulk insert.
4. Separate stable serialized ComponentSchemaId/version from compiler-local runtime type identity.
5. Define component storage/query version counters for later stale-view checks.
6. Add initial data component definitions only: LocalTransform, WorldTransform, MeshInstance handles/state, Visibility, Camera, Light, AnimationState, Morph/Skinning state handles, Name/EditorMetadata. Heavy assets/services/pointers are forbidden inside hot components.
7. Delete the unused owning Entity orchestration once no call site requires it. Retain/simplify legacy Component only where current camera/mesh compatibility still consumes it, with a deletion target in Prompt 07.

Validation:
- Randomized reference-model tests for entity create/destroy/reuse and component add/remove/replace.
- Verify sparse[denseEntity[i].slot] == i and every live sparse mapping round-trips after swap removal.
- Stale entity IDs fail after slot reuse.
- Bulk insertion order and schema serialization identity deterministic across runs/configurations where promised.
- Non-movable/owning component policy explicitly rejected or safely supported according to the chosen bounded contract.

Acceptance gate:
- Registry/storage invariants pass long randomized stress.
- Renderer/Editor cannot include ECS storage headers.
- No asset ownership, callbacks, mutexes, virtual Update/Render behavior, or raw scene pointers were introduced into data components.
- Old Entity container is deleted or has a concrete Prompt 07 blocker with no new consumers.

Positive patterns: opaque generations, packed per-type iteration, stable schema/runtime separation, private storage seam.
Forbidden: archetype chunks, reflection framework, prefab system, public registry, storing dense indices across mutation.
~~~

## Prompt 06 — Add ECS Queries, Structural Epochs, and Entity Commands

~~~text
Implement Prompt 06 only after Prompt 05 passes.

Objective:
Make ECS iteration and structural mutation safe for future jobs: typed read/write queries, include/exclude filtering, frozen structural epochs, task-local EntityCommandBuffer, deterministic playback, and temporary entity remapping. Execute serially.

Non-negotiable repository rules:
- Enforce J's canonical concurrency/rendering vocabulary and Rule 10: search canonical terms plus rejected aliases in the touched scope, use one responsibility per name, and delete temporary compatibility spellings before the gate.
- Search existing view/span/range, command, change-list, transaction, type-access, and deterministic-sort facilities before adding counterparts.
- Do not add a generic query language, runtime reflection DSL, or editor-facing raw registry API.
- Refactor duplicate create/destroy/set operations in touched scene paths toward one command/commit owner.
- Preserve current scene behavior through serial tests before any system parallelism.

Required implementation:
1. Add typed Query<Read<T>,Write<U>...> with include/exclude sets and explicit access metadata.
2. Single-component queries iterate the dense pool directly. Multi-component queries select the smallest included pool and resolve others through sparse lookup.
3. Add StructureFrozen epoch after owner commit. Query/view captures registry/storage versions and asserts on structural invalidation.
4. Permit value writes only through declared writable component access; prohibit create/destroy/add/remove during frozen iteration.
5. Add task-local EntityCommandBuffer commands: create temporary entity, destroy, add/remove/replace/set component.
6. Give each command deterministic system/phase/partition/local sequence keys. Merge/playback must not depend on recording completion order.
7. Remap temporary entity tokens only within their originating buffer; reject escape/cross-buffer use.
8. Produce a typed commit result for stale target, missing/present component conflicts, and successful mapping.

Validation:
- Query results match a slow reference model for randomized compositions and include/exclude combinations.
- Const query cannot obtain writable access at compile time/API boundary.
- Frozen-epoch structural mutation and stale view/version negative tests assert/reject.
- Shuffled command-buffer arrival produces identical EntityId/component state after deterministic merge.
- Temporary create plus subsequent add/set/destroy works; cross-buffer token use fails.
- Conflicting writes require explicit policy and never silently use task completion order.

Acceptance gate:
- Structure remains unchanged for the full query epoch.
- Commands settle deterministically and preserve registry invariants.
- No direct structural mutation path remains in any scene path converted during this prompt.
- No scheduler dependency is inferred from captured pointers; access metadata is explicit.

Positive patterns: typed const/write access, frozen epochs, per-task buffers, deterministic playback.
Forbidden: shared command buffer across jobs without partition protocol, mutation during iteration, stored query iterators across commit, incidental last-writer-wins.
~~~

## Prompt 07 — Convert Existing Scene Instance Data to ECS Components

~~~text
Implement Prompt 07 only after Prompt 06 passes.

Objective:
Move current camera, mesh-instance, visibility, light, transform, animation playback, and cold editor metadata instance state into ECS component pools while keeping GameScene as the coherent world facade. Compatibility facades may remain only as non-owning single-source views.

Non-negotiable repository rules:
- Enforce J's canonical concurrency/rendering vocabulary and Rule 10: search canonical terms plus rejected aliases in the touched scope, use one responsibility per name, and delete temporary compatibility spellings before the gate.
- Audit every field/call site in SceneCameras, SceneMeshes, SceneLighting, SceneAnimations, MeshComponent, CameraComponent, Transform, GameScene snapshot/loading, Showcase, editor inspectors, and renderer extraction.
- For every field classify: ECS component, immutable asset resource, world service/resource, derived output, editor-only metadata, or obsolete.
- Never copy the same mutable field into both legacy object and ECS storage.
- Refactor touched containers/factories/translators so construction and mutation converge on one path; delete duplicated state immediately per converted type.

Required implementation:
1. Make GameScene own the private EntityRegistry and expose narrow stable EntityId/world command/read contracts, not raw storage.
2. Translate mesh instances to MeshInstance + LocalTransform + Visibility (+ animation/skinning handles when applicable). Mesh resource ownership moves/remains in immutable asset registry/service and components store handles.
3. Translate cameras to Camera + LocalTransform + Visibility; remove lazy derived direction as authoritative state.
4. Translate lights to Light (+ LocalTransform where meaningful) and separate editor labels/cold metadata.
5. Translate animation playback to per-entity AnimationState; keep clip/skeleton definitions immutable resources.
6. Make existing Scene* types temporary facades over ECS queries/commands or replace call sites directly. A facade owns no parallel vector copy.
7. Convert scene/payload factories and current level initialization to deterministic component insertion.
8. Preserve authored/source identity separately from runtime EntityId.

Validation:
- Serial current-level visual and behavioral parity for cameras, mesh visibility/material, lighting, animations/morphs, Showcase motion, save/capture-to-level behavior.
- Component pool counts/values compare to a temporary reference extraction from the old path during tests only.
- Object deletion/reordering no longer invalidates durable selection/targets based on vector position.
- Asset unload/replacement cannot leave a component with an unvalidated raw resource pointer.

Acceptance gate:
- ECS is the sole mutable instance-state source for every converted type.
- No new legacy Component/Entity consumer exists.
- Compatibility facades are read/command adapters with explicit deletion in Prompt 10/11, not storage owners.
- Serial behavior matches baseline before later task scheduling.

Positive patterns: data/resource separation, hot/cold components, one source of truth, incremental vertical migration.
Forbidden: giant GodComponent, raw Mesh ownership in ECS, duplicate Scene* vectors, generic reflection/serialization detour.
~~~

## Prompt 08 — Make Transform/Derived State Explicit and Publish the Change Journal

~~~text
Implement Prompt 08 only after Prompt 07 passes.

Objective:
Remove write-on-read transform/camera caches, evaluate derived world state in explicit serial systems, and create the bounded sequenced world change journal/read publication that renderer/editor will consume.

Non-negotiable repository rules:
- Enforce J's canonical concurrency/rendering vocabulary and Rule 10: search canonical terms plus rejected aliases in the touched scope, use one responsibility per name, and delete temporary compatibility spellings before the gate.
- Search all Transform and camera direction/matrix getters, dirty flags, bounds calculations, snapshot extraction, editor reads, temporal history, and change notifications.
- Reuse existing math/identity/scene-generation types; do not create duplicate matrix conventions.
- Refactor every touched lazy getter into a true read or explicit evaluation owner.
- Do not build transform hierarchy scheduling until current runtime hierarchy data has an actual consumer and cycle contract.

Required implementation:
1. Keep LocalTransform as owner-mutated TRS input. Add explicit TransformEvaluationSystem writing WorldTransform (and inverse/derived data required by current consumers).
2. Add explicit CameraDerivedState evaluation or compute immutable camera direction/view data in a declared phase; const reads never mutate.
3. Compact dirty entity IDs/ranges deterministically and evaluate only required outputs in serial reference mode.
4. Add bounded WorldChangeJournal with sequence number, entity generation, typed create/destroy/component/value changes, and consumer acknowledgements.
5. Define lagging-consumer recovery: request a full immutable baseline and resume from a sequence; never read reclaimed journal storage.
6. Publish WorldReadView generation with only current editor/game query fields. Use release/acquire publication and generation-pinned storage/reclamation.
7. Drive initial render-delta extraction and editor model update inputs from the committed journal, not extra full mutable scans where avoidable.

Validation:
- Concurrent-read stress proves no lazy transform/camera write remains.
- Serial evaluated matrices/directions match baseline within defined floating-point tolerance.
- Dirty/no-change frames emit expected journal work; static world produces no structural churn.
- Journal replay and full baseline + deltas produce identical world/read state.
- Delayed reader falls outside retention, resynchronizes, and never accesses reclaimed memory.

Acceptance gate:
- Published component/read data is immutable for its generation.
- All current transform/camera consumers use explicit evaluated outputs.
- Journal/read publication has bounded storage and tested reclamation.
- No general event-sourcing framework or public diagnostics snapshot was added.

Positive patterns: explicit derived-data phase, dirty ranges, sequenced typed journal, RCU-like bounded publication.
Forbidden: mutable const cache, full shared-scene mutex, unbounded history, editor/renderer direct registry traversal.
~~~

## Prompt 09 — Implement Transactional Asynchronous Scene Loading

~~~text
Implement Prompt 09 only after Prompts 06 and 08 pass.

Objective:
Replace destructive synchronous level loading with a scoped read/decode/validate/assemble pipeline that produces immutable EntityBlueprint packages and commits atomically while preserving the old scene on failure/cancellation.

Non-negotiable repository rules:
- Enforce J's canonical concurrency/rendering vocabulary and Rule 10: search canonical terms plus rejected aliases in the touched scope, use one responsibility per name, and delete temporary compatibility spellings before the gate.
- Audit LevelManager, LevelRegistry/Asset, SceneAssetManager/Registry, manifest/payload loaders/appenders/translators, file utilities, lifecycle events, editor level menu, asset residency, and save behavior.
- Extend existing load translators and registries; do not create Async duplicates or a second asset database.
- One SparkleTasks runtime, explicit BlockingIo/Background lanes, World/Document task scope.
- Refactor touched synchronous mutation/callback code and delete clear-before-load behavior.

Required implementation:
1. Capture immutable level request, catalog generation, request ID, world/document generation on the owner thread.
2. Load/publish immutable registry/catalog generation before concurrent ID resolution.
3. Read manifests/files on BlockingIo; decode/translate per asset in task-local contexts with weighted CPU/pending-byte limits.
4. Produce deterministic EntityBlueprint component records with stable authored identity and component schema versions.
5. Fan-in, resolve cross-asset references, assign deterministic final order/handles, and validate a complete immutable SceneLoadPackage.
6. Marshal completion to owner. Reject stale request/document/world/catalog generation.
7. Bulk commit ECS entities/components through world commands only after validation; old scene remains active until acceptance.
8. Lifecycle events/results dispatch on owner thread after invariants hold. Workers never call UI, transaction, GameScene mutation, or arbitrary Event callbacks.
9. New request cancels older unpublished work; published/GPU-enqueued resources follow generation retirement rather than immediate free.
10. Expose bounded/coalesced operation progress and typed diagnostics through EditorOperationService-compatible result contracts.

Validation:
- Serial/parallel package IDs/order/content/diagnostics parity.
- Cancel before read, during read/decode, fan-in, after package publish, while commit queued, document close, newer request wins.
- Missing/corrupt registry/manifest/mesh/material/animation/skeleton keeps active scene and renderer state.
- Repeated load/cancel/reload stress with memory limit and delayed workers.
- Editor/application remains responsive and the FrameCritical lane meets budget during load.

Acceptance gate:
- No clear-before-success and no accepted partial scene.
- No mutable shared SceneAssetPayload/registry state across workers.
- Old scene survives every failed/stale/cancelled pre-commit path.
- Synchronous compatibility path is deleted after parity or has an exact Prompt 22 deletion gate with no new use.

Positive patterns: isolated construction, immutable package, generation validation, owner commit, failure containment.
Forbidden: nested synchronous loads from worker, UI/global callbacks during decode, task-completion-order IDs, unbounded decoded memory.
~~~

## Prompt 10 — Build the ECS-Aware Game System Graph and Parallel Animation

~~~text
Implement Prompt 10 only after Prompt 08 passes; use Prompt 09 contracts where loading affects target generations.

Objective:
Replace arbitrary GameSceneController mutable access with an ECS-aware GameSystemGraph and prove the first real gameplay parallel workload through movement, animation pose, morph, skinning, transform, and extraction dependencies.

Non-negotiable repository rules:
- Enforce J's canonical concurrency/rendering vocabulary and Rule 10: search canonical terms plus rejected aliases in the touched scope, use one responsibility per name, and delete temporary compatibility spellings before the gate.
- Audit GameSceneController, GameCameraController, ShowcaseSceneController, SceneAnimations/evaluators/samplers, SceneMeshes morph application, skeleton data, transform/extraction phases.
- Search for graph/access/resource-domain counterparts before adding system descriptors.
- Use SparkleTasks; do not add an ECS scheduler, controller pool, animation threads, or worker waits.
- Refactor whole-scene mutable APIs and delete migrated controller paths in this prompt.

Required implementation:
1. Define GameSystem descriptors: stable ID/name, phase, typed component query Read/Write access, non-ECS resource access, explicit prerequisites, execution/grain policy.
2. Compile component/resource hazards: read/read may overlap; conflicting access orders by phase/dependency or rejects ambiguity/cycles.
3. Execute the complete graph serially and prove state/render parity.
4. Convert camera input/movement and Showcase motion to narrow ECS systems over stable EntityId targets.
5. Pre-resolve clip-to-skeleton and morph targets per asset generation. Preallocate stable pose/morph/skinning output slots.
6. Partition leading component/query ranges. Each task writes exclusive slots/ranges; no per-task shared vector push or asset mutation.
7. Deterministically merge by system/entity/partition key, commit outputs, then run transform/derived state and extraction phases.
8. Add small-work serial thresholds and reusable/per-execution arenas; remove steady-state nested target scans and per-joint heap churn where measured/touched.
9. Delete GameSceneController::Update(GameScene&) and the temporary legacy adapter after current camera/Showcase consumers migrate.

Validation:
- Graph compile rejects cycle, duplicate system, undeclared/conflicting access, unavailable phase resource.
- Fixed input sequences match at serial, 1/2/N workers, randomized completion/delay.
- Camera navigation, Showcase PTLAS motion, animation pose, morph weights, skinning matrices, transforms, extraction all match defined tolerance/order.
- Negative tests prove worker cannot mutate structure, broadcast events, call UI/renderer/RHI, or retain component view.
- Animation-heavy benchmark shows crossover and critical path; tiny scene stays within overhead budget.

Acceptance gate:
- Current systems use typed narrow access; no arbitrary mutable GameScene controller execution remains.
- Parallel output is deterministic/serial-equivalent.
- Work uses SparkleTasks and explicit dependencies, not waits or extra pools.
- Removed scans/allocations/controller paths are listed in deletion ledger.

Positive patterns: query-driven systems, access hazards, exclusive outputs, deterministic merge, grain threshold.
Forbidden: parallel virtual Entity::Update, shared morph/pose pushes, mutable asset data, completion-order merge, ECS auto-scheduling hidden from task graph.
~~~

## Prompt 11 — Convert the Editor to Immutable Models and Semantic Commands

~~~text
Implement Prompt 11 only after Prompts 09 and 10 pass.

Objective:
Remove live GameScene pointer/index mutation from editor panels. Make editor main own ImGui/selection/transactions, consume immutable WorldReadView-derived EditorSceneModel, submit stable EntityId semantic commands, and manage background workflows through one EditorOperationService.

Non-negotiable repository rules:
- Enforce J's canonical concurrency/rendering vocabulary and Rule 10: search canonical terms plus rejected aliases in the touched scope, use one responsibility per name, and delete temporary compatibility spellings before the gate.
- Audit UI host services, SceneObjectSelection/Actions/Presentation, outliner entries/panel, all inspectors, material variants, level menu, viewport, shader recook, preview/search/save/package workflows.
- Before adding model/command/operation types, search for existing selection, transaction, status, request/result, preview, and restart/recook services; consolidate rather than duplicate.
- No editor thread pool, raw ECS registry access, live GameScene pointer in panels, task debugger, or renderer-cache browser.
- Refactor each migrated panel completely: read model + command + result + undo/redo; delete direct mutation helper.

Required implementation:
1. Build generation-pinned EditorSceneModel incrementally from WorldReadView/journal, with full resync fallback.
2. Replace type+vector-index selection with EntityId and validate on model generation change.
3. Convert camera/light/sky/mesh transform/visibility/material-variant edits into semantic commands committed at the next world boundary.
4. Return typed accepted/stale/rejected results. Panels retain draft widget values only; no component pointers/spans.
5. Implement main-thread transaction ownership and deterministic undo/redo through inverse semantic commands/before-after values. Coalesce continuous drags into bounded transactions.
6. Add one private EditorOperationService over SparkleTasks scopes for current open/reload, recook, import/cook, preview, search, save, package validation.
7. Use latest-generation-wins for search/preview/reload, bounded/coalesced progress, immutable results applied in UI/application update.
8. Ensure document/application close cancels/scopes operations, rejects late result, then destroys models.

Validation:
- All current inspector/outliner actions round-trip through commands and undo/redo.
- Deletion/reorder/reload invalidates/preserves selection correctly without vector-index aliasing.
- Pinned model read remains valid while next generation publishes; delayed reader reclamation tested.
- Close document during every operation stage; no callback-after-close or stale application.
- Editor responsive under load/cook/search/preview; background cannot starve frame.

Acceptance gate:
- UI/panels no longer retain GameScene*, MeshComponent*, light/camera pointers, mutable spans, or durable indices.
- Direct SceneObjectActions mutations and superseded host service are deleted.
- One operation service/runtime exists and exposes workflow state, not scheduler internals.
- Current UI behavior, transactions, viewport request behavior, and error reporting remain usable.

Positive patterns: immutable UI model, semantic commands, stable identity, main-thread transactions, scoped operations.
Forbidden: mutex around panels/scene, worker ImGui calls, raw registry in Editor, unbounded progress queue, duplicate editor pool.
~~~

## Prompt 12 — Establish the Immutable Game/Render Data Contract

~~~text
Implement Prompt 12 only after Prompts 08 and 10 pass.

Objective:
Replace GameSceneSnapshot/raw mesh pointer/direct lifecycle coupling with stable RenderObjectId, immutable asset handles, sequenced RenderWorldDelta, RenderFrameDynamicData, versioned frame metadata, and a headless-replayable renderer input contract. Keep renderer execution serial.

Non-negotiable repository rules:
- Enforce J's canonical concurrency/rendering vocabulary and Rule 10: search canonical terms plus rejected aliases in the touched scope, use one responsibility per name, and delete temporary compatibility spellings before the gate.
- Audit GameSceneSnapshot/MeshSnapshot, Renderer facade/SystemRoot, RenderSceneDataBuilder, SceneRenderStateCoordinator, temporal inputs, RT instances, providers, capture, level callbacks, asset handles.
- Reuse existing handle/packet/arena/math/frame-generation types; do not create a second scene schema or broad renderer snapshot API.
- Renderer must not depend on ECS or dereference GameScene after conversion.
- Refactor and delete each old read/callback path when its packet equivalent lands.

Required implementation:
1. Define RenderObjectId generation separate from EntityId and explicit extraction mapping.
2. Replace raw Mesh*/game pointers with immutable/versioned mesh/material/texture/skeleton/animation handles whose lifetime spans packet/proxy use.
3. Define structural RenderWorldDelta create/update/destroy with SceneGeneration + SequenceNumber and packet-owned/arena storage.
4. Define dynamic frame arrays for transforms, skinning/morph, lights/camera/view data, visibility and current feature inputs.
5. Include exact temporal/provider metadata: FrameId, jitter/sample, exposure, resolution, motion/depth conventions, camera cut/teleport/history reset, provider/frame-generation tags.
6. Implement serial RenderWorld/RenderProxyRegistry applying checked deltas and resolving immutable assets.
7. Record/replay packet streams in a headless renderer test without GameScene lifetime.
8. Convert level begin/end into ordered generation data; remove renderer direct level-event subscription/cache clearing.

Validation:
- Packet contains only owned values or stable immutable handles; poison/reset after acknowledgement catches retention.
- Create/update/destroy replay deterministic; stale/duplicate/out-of-order delta handling tested.
- Delayed render consumer while levels/assets change has no stale pointer.
- Serial visual/feature parity for raster, classic TLAS/PTLAS inputs, reservoir light identity, path/reference reset, temporal/provider tags, capture requests.
- Packet bytes/build time measured; no broad public diagnostics added.

Acceptance gate:
- Renderer consumes a recorded stream with GameScene destroyed.
- MeshInstanceSnapshot raw pointer and old full snapshot boundary are deleted.
- RendererSystemRoot has no GameScene access and no direct lifecycle callbacks.
- Serial output matches baseline before render threading.

Positive patterns: stable separate identities, immutable handles, structural/dynamic split, replayable packet.
Forbidden: shared scene mutex, raw object pointer, renderer ECS query, array index temporal identity, dual packet/snapshot path.
~~~

## Prompt 13 — Add RenderThread Ownership and the Bounded RenderFrameQueue

~~~text
Implement Prompt 13 only after Prompt 12 passes.

Objective:
Move RendererSystemRoot, FramePipeline, mutable RHI ownership, submit/present, and renderer resource creation/destruction to one `RenderCoordinator` running on `RenderThread`. Connect GameThread/EditorThread producers through bounded frame slots and sequenced control commands. Keep renderer preparation/recording serial inside the coordinator initially.

Non-negotiable repository rules:
- Enforce J's canonical concurrency/rendering vocabulary and Rule 10: search canonical terms plus rejected aliases in the touched scope, use one responsibility per name, and delete temporary compatibility spellings before the gate.
- Audit runtime/editor loops, renderer facade/host phases, window/swapchain resize, minimize/restore, provider affinity, capture, shutdown/device loss, RHI creation/destruction and submission.
- Audit LC-08 through LC-10 and LC-16/LC-18: Streamline global locking, D3D12 queue submission/CPU-wait mutexes, Vulkan native queue external synchronization, every renderer/RHI idle wrapper and native wait. A mutex-protected queue is not a substitute for one submission owner.
- Reuse SparkleTasks and existing frame-resource/timeline primitives; no second scheduler or RHI thread.
- Refactor host Prepare/Record/Submit routes into one coherent submission facade; delete direct product path after parity.
- Preserve serial consumer and threaded zero/one-ahead modes using the same packet contract.

Required implementation:
1. Add bounded `RenderFrameQueue` slots with explicit Free/Writing/Ready/Rendering/Retired transitions, release/acquire publication, close/wake, acknowledgement/reuse. The owning type is `RenderCoordinator`; its physical thread role is `RenderThread`.
2. Add `RenderControlCommandQueue` with sequenced commands for resize, settings, capture, shader generation, viewport, shutdown, and necessary provider/present actions. Do not call these CPU-owned control records command lists, command buffers, or GPU queue submissions.
3. Create/adopt RHI and renderer root on the coordinator; assert all mutable renderer/RHI owner methods.
4. Implement serial-consumer, threaded zero-ahead, and threaded one-ahead modes. No unbounded queue or hidden packet allocation fallback.
5. Define slow-render backpressure and minimize/resize behavior; producer parks only at explicit slot acquisition.
6. Implement ordered shutdown: stop `RenderControlCommandQueue` producers, close `RenderFrameQueue`, settle accepted frames/tasks, final GPU drain, destroy RHI on RenderThread, join the coordinator, stop executors last.
7. Ensure device-loss/fatal paths settle `RenderFrameQueue` slots and editor waiters without callback-after-destroy.
8. Make D3D12 submit and Vulkan submit/present coordinator-only. Retain a native queue mutex only for a documented provider/API shared-entry boundary; otherwise ownership assertions replace multi-owner permission. GPU waits never execute on task workers.
9. Split Streamline/provider lifecycle from frame/device/presentation calls: application lifecycle owns initialization/shutdown, coordinator owns render-facing calls, and no engine mutex is held across external SDK work without an explicit re-entry proof.

Validation:
- Serial/threaded image and packet-consumption parity D3D12/Vulkan.
- Artificial slow render proves bounded queue/memory and expected producer backpressure.
- Repeated resize/minimize/restore/level change/capture/shutdown/device-loss policy stress.
- Owner-thread assertions trigger on deliberate wrong-thread calls.
- Deliberate worker submit/present/GPU-wait/provider entry fails fast; provider enabled/disabled/failure shutdown proves no lock-order cycle or callback-after-destroy.
- Capture shows GameThread N+1 / RenderThread N overlap without a routine GameThread wait and uses the canonical thread labels.

Acceptance gate:
- Mutable RendererSystemRoot/RHI has one `RenderCoordinator` owner on `RenderThread`.
- Queue depth is fixed and no accepted slot is stranded.
- Direct host-thread Prepare/Record/Submit product path is deleted; serial mode uses same consumer.
- No separate RHI translation thread or worker submission exists.
- Queue/provider locks that remain have a documented invariant and API reason; they do not create a second submission owner.

Positive patterns: single owner, bounded frame queue, sequenced `RenderControlCommandQueue`, explicit backpressure, ordered shutdown.
Forbidden: generally thread-safe renderer root, unbounded queue, main routine render wait, device idle per frame, detached render thread.
~~~

## Prompt 14 — Convert Editor UI, Viewport, and Capture Across the Render Boundary

~~~text
Implement Prompt 14 only after Prompts 11 and 13 pass.

Objective:
Make all editor-to-render data owned/versioned: copied ImGui draw packets, viewport requests/products, rendering settings commands, preview/capture requests, and narrow completion results. Remove live editor/renderer pointer sharing.

Non-negotiable repository rules:
- Enforce J's canonical concurrency/rendering vocabulary and Rule 10: search canonical terms plus rejected aliases in the touched scope, use one responsibility per name, and delete temporary compatibility spellings before the gate.
- Audit UI NewFrame/Build/Render, ImDrawData lifetime, RhiImGuiRenderer, viewport panels/contracts, settings panels, preview geometry, diagnostics providers, screenshot/BMP path, resize/docking.
- Search existing packet/viewport/capture handles and reuse/consolidate them.
- Editor main owns ImGui; render coordinator owns GPU UI/viewport/capture resources.
- No broad renderer/task/cache snapshot or editor task debugger.

Required implementation:
1. At UI frame end, copy/convert draw vertices, indices, clip rects, texture handles, and commands into packet-owned EditorRenderPacket; no ImDrawData pointer crosses.
2. Convert texture IDs to stable renderer handles with generation/lifetime policy.
3. Publish viewport request with dimensions, format/color-space, feature/view settings, generation; render publishes product ID/descriptor generation/frame/lifetime token.
4. Editor releases products explicitly or by bounded retirement rule; it never queries a live cache pointer.
5. Route rendering setting changes, preview requests, and capture through sequenced render commands.
6. Implement bounded nonblocking capture pipeline: request → render copy/readback → GPU token → background encode/write → narrow result to editor/tool.
7. Preserve existing product-owned capability/memory diagnostics only where they have a current UI consumer; remove superseded broad access.

Validation:
- Delay render thread beyond next ImGui frame and verify copied draw correctness/lifetime.
- Dock/resize/minimize/viewport recreate stress on D3D12/Vulkan.
- Stale viewport generation rejected; old descriptor/product remains valid until release/retirement.
- Capture normal/resized/minimized/cancelled/bounded-queue; no routine device idle.
- Editor close while capture/preview in flight has no late UI callback.

Acceptance gate:
- No live ImGui/editor pointer crosses to render coordinator.
- Viewport/capture products have stable versioned ownership.
- Direct editor renderer mutation and redundant diagnostics routes are deleted.
- UI, viewport, settings, preview, screenshot/BMP remain functional.

Positive patterns: copied transient UI data, versioned products, sequenced commands, narrow result.
Forbidden: live descriptor/cache pointer, UI worker callback, WaitForIdle capture, unbounded capture queue.
~~~

## Prompt 15 — Build the Persistent Render/GPU Scene

~~~text
Implement Prompt 15 only after Prompt 13 passes.

Objective:
Stop rebuilding/uploading unchanged scene-wide arrays. Make RenderWorld proxies and GPU-scene slots persistent, apply structural deltas, update dirty dynamic ranges, and prepare token-based removal/retirement while preserving raster and RT identity.

Non-negotiable repository rules:
- Enforce J's canonical concurrency/rendering vocabulary and Rule 10: search canonical terms plus rejected aliases in the touched scope, use one responsibility per name, and delete temporary compatibility spellings before the gate.
- Audit RenderSceneDataBuilder, BuildRenderSceneGpuData, mesh/material/texture caches, structured buffers, uploads, raster/RT instance planning, BLAS/classic TLAS/PTLAS paths, allocator/budget data.
- Refactor existing caches/builders into one persistent owner; do not add a parallel GpuScene2 or retain full rebuild as product path.
- Remove avoidable work before parallelizing it.
- Preserve frame graph as update/copy/barrier authority and RHI as explicit resource owner.

Required implementation:
1. Give render proxies stable slots for mesh/material/transform/light/skinning/morph/RT metadata and asset generations.
2. Apply create/update/destroy deltas with sequence validation; static asset payloads upload once per generation.
3. Track dirty dynamic values/ranges and coalesce deterministic upload/update plans.
4. Use frame upload rings/pages for transforms, lights, skinning/morph, dirty materials; bound bytes and overflow policy.
5. Maintain separate raster, classic TLAS, and PTLAS instance identities/paths. Asset/BLAS replacement creates a new generation safely.
6. Remove logical proxies immediately at accepted delta but retire GPU allocations/resources by last-use completion token.
7. Integrate existing allocator/memory-budget pressure for growth/eviction/compaction decisions; no unbounded growth.
8. Delete full-scene structured-buffer creation/upload and direct level cache clear after parity.

Validation:
- Static scene after warm-up emits no structural delta/full static upload.
- Single transform/light/material change updates only expected ranges.
- Add/remove/reuse/asset replacement under delayed GPU completion has no stale slot/use-after-free.
- Raster/classic TLAS/PTLAS build/update/trace parity where supported, both backends.
- Measure upload bytes, resource creates, dirty ranges, memory/fragmentation before/after.

Acceptance gate:
- Persistent slots/generations are authoritative and full rebuild product path is deleted.
- GPU resources outlive all referencing frames and eventually reclaim.
- CPU reduction does not increase GPU time/barriers/descriptors/memory beyond justified budget.
- No routine WaitForIdle for scene changes.

Positive patterns: persistent state, delta application, dirty ranges, generation identity, token retirement.
Forbidden: parallel full rebuild, frame-wide reupload, vector-position identity, immediate GPU free, hidden backend cache ownership.
~~~

## Prompt 16 — Complete Runtime Residency and Generation-Based Reload/Retirement

~~~text
Implement Prompt 16 only after Prompts 04 and 15 pass.

Objective:
Connect asynchronous CPU asset generations, render uploads, shader package replacement, readiness, fallback, eviction, and deferred retirement without worker waits or routine device idle.

Non-negotiable repository rules:
- Enforce J's canonical concurrency/rendering vocabulary and Rule 10: search canonical terms plus rejected aliases in the touched scope, use one responsibility per name, and delete temporary compatibility spellings before the gate.
- Audit current mesh/texture/material/animation/shader load/reload, resource caches, upload service, frame graph copy work, shader package/reflection/runtime/PSO ownership, WaitForIdle reload paths.
- Reconcile LC-01, LC-08, LC-14, and LC-16. Trace idle calls through wrappers to native drains and remove duplicate nesting, not merely the highest-level spelling.
- Extend one asset-generation/residency contract; do not create per-type incompatible async state machines where common states suffice.
- Refactor touched reload/cache code and delete old idle/swap route after parity.
- Preserve shader ABI, providers, RT tables/pipelines, both backends, and previous active generation on failure.

Required implementation:
1. Implement bounded states Unloaded/Reading/Decoding/ReadyForUpload/Uploading/Resident/Evicting/Retired/Failed per asset generation with type-specific payloads.
2. Blocking reads/decode use I/O/background lanes and publish immutable payload; workers never wait for residency.
3. Render coordinator schedules upload/copy through frame graph/RHI; Resident only after completion token.
4. Components/packets/proxies carry stable handles/generations and use existing fallback/omit policy until ready.
5. Bound decoded CPU bytes, pending upload bytes, resident budget, and request backlog. Cancellation discards unpublished CPU data; GPU-enqueued data retires normally.
6. Materialize immutable shader runtime generation after package/reflection validation; swap at safe frame boundary.
7. Keep old PSOs/layouts/shader tables/provider resources alive through last-use tokens. Failure leaves old generation operational.
8. Remove routine renderer.WaitForIdle from shader reload/scene asset replacement.
9. Replace scene invalidation, image-provider refresh, frame-execution refresh, and resize/recreate drains with generation/token retirement where supported. If swap-chain or backend reinitialization temporarily requires a drain, exactly one coordinator-owned site performs it and the deletion/tokenization blocker is recorded; FramePipeline → device service → swap-chain must not issue repeated drains for one resize.
10. Harden D3D12/Vulkan allocation record and pending-release synchronization: mutate on the render owner where possible, snapshot diagnostics under a narrow lock and consume after unlock, never return an unowned mutable record across the lock, and retire only after last-use tokens.

Validation:
- Read/decode/upload completion reordered/delayed; stale generation never becomes active.
- Cancel/fail at every state; old generation/fallback behavior correct.
- Shader HLSL/Slang DXIL/SPIR-V/reflection/layout/parameter verification parity.
- Reload during classic TLAS/PTLAS, temporal/provider, capture, viewport work with delayed GPU completion.
- Profile/assert the exact native idle count for reload, scene invalidation, provider refresh, resize, and shutdown; ordinary paths are zero and one resize cannot recursively drain through multiple layers.
- Budget/backpressure/eviction stress and eventual retirement.

Acceptance gate:
- No frame worker waits for I/O/upload/residency.
- One coherent generation visible per packet/recording run.
- Previous valid generation survives all failure paths.
- Routine reload/replacement uses tokens, not device idle.
- Every retained idle is final shutdown, unrecoverable device reinitialization, or one documented swap-chain boundary with a named later deletion condition; allocation diagnostics cannot race retirement or hold their lock while formatting/calling external code.

Positive patterns: staged state machine, immutable generation, later-frame readiness, fallback, deferred retirement.
Forbidden: synchronous load in frame path, mixed shader generations, raw resource pointer in packet, immediate unload after cancellation.
~~~

## Prompt 17 — Decompose Renderer Preparation into a Task DAG

~~~text
Implement Prompt 17 only after Prompts 10 and 15 pass.

Objective:
Replace monolithic mutable renderer preparation with pure/coarse task nodes over immutable packet/render-world inputs and task-private outputs: transforms/bounds, visibility, batching, lighting, skinning/morph, material classification, RT planning, deterministic merge.

Non-negotiable repository rules:
- Enforce J's canonical concurrency/rendering vocabulary and Rule 10: search canonical terms plus rejected aliases in the touched scope, use one responsibility per name, and delete temporary compatibility spellings before the gate.
- Audit RenderSceneDataBuilder and all called builders/caches/scratch vectors, pass setup dependencies, temporal/lighting/RT planners, profiler scopes.
- Search existing frame-graph/task graph structures but do not make frame graph schedule non-render CPU policy or create a second task runtime.
- Refactor one builder responsibility at a time; delete shared scratch and old entry point as consumers migrate.
- Keep frame-graph setup/compile serial initially unless existing evidence justifies bounded private topology reuse.
- Pre-mortem MT-02–06, MT-13, MT-19–31, MT-36, MT-39, and MT-41–43. Do not turn one monolithic builder into many tiny tasks that still contend on the same caches/vectors; the target is pure ranges and earliest-start dependencies.

Required implementation:
1. Define immutable run inputs and task-local/preallocated output ranges for each preparation responsibility.
2. Produce transformed bounds and previous-transform ranges, then build per-view visibility/relevance/LOD results with stable `RenderObjectId` tie-breaks. Do not leave “visibility” as an unimplemented placeholder node.
3. Produce visible-light classifications/compact records with stable light identity. Where the current shadow path consumes raster casters, build per-light/view shadow frusta and task-private caster lists; merge in deterministic light/view/object order. If it does not, record the exact first consumer and do not add an unused caster system.
4. Produce skinning/morph ranges, raster pass/material eligibility, and distinct BLAS input plus classic TLAS/PTLAS plans. Preserve build/update/refit/rebuild/compaction decisions supported by the current RHI rather than flattening RT work into one list.
5. Reconcile current `MeshDraw`, `MeshRenderItem`, `MeshInstanceBatch`, and `MeshInstanceBatchBuilder` before adding a draw representation. Separate cacheable view-independent draw state from dynamic per-view state only where existing consumers require it; define invalidation and referenced-resource generation lifetime.
6. Build stable pass buckets, draw sort keys and compatible instance batches. Preserve explicit transparent ordering and report why each key field affects correctness or state-change cost.
7. Build explicit prerequisite DAG and joins; no task mutates renderer global caches or pushes into shared vectors.
8. Partition coarse ranges with serial thresholds; use per-execution arenas/reusable buffers and deterministic keys.
9. Pre-resolve/materialize any lazy resource/PSO/layout state needed by preparation/recording, and preassign per-view/pass constant/descriptor/upload requirements.
10. Merge/sort visibility, caster, batch, light, material, GPU-scene dirty-range and RT results deterministically independent of completion order.
11. Keep mutable frame-graph builder commit serial. Only measured expensive setup producers may emit private declaration records for stable owner merge; do not concurrently mutate `FrameGraphBuilder`.
12. Preserve current/previous transforms, sample/reset semantics, reservoir light identity/order, classic TLAS/PTLAS distinction, path/reference accumulation, provider tags.
13. Emit private existing-profiler scopes for critical path and queue delay; no public DAG report/UI.
14. Delete monolithic mutable builder routes and shared scratch after parity.

Validation:
- Serial DAG and 1/2/N workers produce equivalent scene arrays/plans/images.
- Randomized delays and task order do not alter per-view visible sets, light packets, shadow caster order, pass buckets, transparent order, batches, histories, RT instances, or provider metadata.
- Tiny/editor workload selects serial/grouped/dynamic path; large multi-view animation/draw/light/shadow/RT workload shows positive preparation critical-path result.
- No global cache/data race under stress and native validation remains clean.
- Report merge/scratch overhead and Amdahl limits, not only worker utilization.

Acceptance gate:
- Preparation dependencies and exclusive outputs are explicit and tested.
- Every applicable renderer-front-end row in K's Renderer/RHI Use-Case-to-Prompt Coverage ledger has real output and proof; unavailable shadow/draw/RT cases have a named non-applicability record, not an implied implementation.
- No worker wait/global mutable scratch/lazy cache creation remains in task bodies.
- Feature preservation matrix passes serial/parallel.
- Speedup is positive on representative large case without unacceptable small-case/GPU regression.

Positive patterns: pure task functions, preplanned ranges, deterministic fan-in, serial threshold, measured critical path.
Forbidden: tiny task per object, shared vector push/mutex, parallel lazy cache fill, claim speedup from utilization alone.
~~~

## Prompt 18 — Add D3D12 Worker Recording Contexts

~~~text
Implement Prompt 18 only after Prompt 17 passes.

Objective:
Make D3D12 command recording ownership safe for future frame-graph fan-out: per-worker/per-frame/per-queue allocator/list contexts, explicit lease lifetime, worker-local or preassigned transient upload/descriptor allocation, and token-based reset/reuse. Do not enable general parallel pass recording yet.

Non-negotiable repository rules:
- Enforce J's canonical concurrency/rendering vocabulary and Rule 10: search canonical terms plus rejected aliases in the touched scope, use one responsibility per name, and delete temporary compatibility spellings before the gate.
- Audit D3D12CommandContext, allocator/list slot selection, command queues/tokens, frame-resource rotation, upload allocator, descriptor heaps, resource barriers, PassBinder, PipelineStateManager, debug names/markers.
- Reconcile LC-09, LC-11, LC-13, LC-14, and LC-18 explicitly: `D3D12LinearAllocator` atomics, `D3D12DescriptorAllocator` mutex, queue submit/CPU-wait locks, frame-resource native waits, and GPU allocation-record synchronization are inputs to the redesign, not approved final recording primitives.
- Refactor existing context ownership; do not add an unrelated command system or D3D12-only public renderer API.
- Shared RHI contract must remain implementable by Vulkan without hiding native rules.
- Render coordinator remains sole reset coordination/submission owner.
- Pre-mortem MT-09–10, MT-19, MT-25–27, and MT-32–43. Follow AMD's allocator-per-recording-thread-per-frame rule as a minimum ownership shape, then bound Sparkle's actual frames × contexts × queues memory and preserve coordinator submission.

Required implementation:
1. Define public move-only `RhiCommandRecordingLease` expressing `ERhiQueueType`, frame slot, actual worker/context identity, one-task ownership, command list, upload/descriptor pages, and `RhiSubmissionToken` retirement. The backend-private owner is `D3D12CommandRecordingContext`; do not expose allocator/list pointers as a substitute lease.
2. Allocate/reset D3D12 command allocator/list state by buffered-frame × worker/context × queue policy. Never reset until GPU token proves prior use complete.
3. Ensure one allocator/list is recorded by at most one task and one lease closes/returns exactly once.
4. Provide worker-local/preassigned upload constant pages and transient descriptor blocks; remove shared hot cursor contention for leased recording.
5. Materialize PSOs/root signatures/pass runtime before lease fan-out; assert no lazy creation reachable.
6. Preserve command-list naming, PIX events, validation, fatal HRESULT handling.
7. Keep submission API coordinator-only and in compiled order.
8. Remove the leased recording path's shared atomic bump cursor. Give each lease a disjoint page/slice and encode reset eligibility with its GPU completion token; do not assume `m_Offset` acquire/release makes reset safe.
9. Separate persistent descriptor allocation from transient recording allocation. Persistent allocator locking may remain under the render owner; no per-draw/per-dispatch leased work contends on `D3D12DescriptorAllocator::m_mutex`.
10. Restrict infinite fence waits to coordinator/outer-host frame-slot reuse, explicit flush, or shutdown. Development waits identify queue/token/elapsed time and follow the device-hang/loss policy; task workers return dependencies instead of waiting.

Validation:
- Lease acquire/release/reset misuse and wrong-thread/concurrent-use negative assertions.
- Compile-time/API checks prove `RhiCommandRecordingLease` is move-only, does not expose a raw allocator/pool as ownership, and cannot be confused with `RenderCommandContext` or `RhiSubmissionToken`.
- Random worker migration/delay over many buffered frames and queue types.
- D3D12 debug layer + GPU-based validation with existing serial recording through leased contexts.
- Upload/descriptor page overflow and retirement tests.
- Shared-cursor contention comparison, disjoint-range proof, deliberate reset-before-token failure, and repository check that no SparkleTasks worker reaches `WaitForSingleObject`/GPU wait.
- Memory scaling documented for frames × contexts × queues and bounded by configuration.

Acceptance gate:
- No concurrent allocator/list use or premature reset is possible through supported API.
- Public code contains `RhiCommandRecordingLease`; backend code contains `D3D12CommandRecordingContext`; rejected `RecordingContextLease`/`WorkerRecordingContext` aliases and filenames are absent.
- Existing serial renderer records correctly through new context contract.
- No shared hot upload/descriptor cursor in leased path.
- Remaining queue/descriptor/accounting locks have a non-recording invariant and owner; `D3D12LinearAllocator` is removed from the concurrent leased hot path or made owner-local with proven reset semantics.
- No worker submission, lazy PSO creation, marker loss, or D3D12-only policy leaked above RHI.

Positive patterns: exclusive lease, token retirement, worker-local transient allocation, prewarm.
Forbidden: allocator mutex around concurrent record, reset by worker without token, submit from task worker, device idle reset.
~~~

## Prompt 19 — Add Vulkan Worker Recording Contexts

~~~text
Implement Prompt 19 only after Prompt 17 passes. Match the common lease semantics proven by Prompt 18 without copying D3D12 internals.

Objective:
Make Vulkan command recording ownership safe: per-worker/per-frame/per-queue-family command-pool/buffer contexts, external synchronization compliance, transient descriptor/upload ownership, reset/retirement, and preserved debug/validation behavior. Do not enable general parallel pass recording yet.

Non-negotiable repository rules:
- Enforce J's canonical concurrency/rendering vocabulary and Rule 10: search canonical terms plus rejected aliases in the touched scope, use one responsibility per name, and delete temporary compatibility spellings before the gate.
- Audit VulkanCommandContext, command pools/buffers, queue families/timeline semaphores, frame resources, descriptor pools/sets, uploads, pipeline/layout creation, debug labels.
- Reconcile LC-10, LC-12, LC-14, LC-15, LC-16, and LC-18 explicitly: native queue mutex, the broad VulkanDescriptorAllocator mutex, allocation/pending-release records, validation-message ingestion, command-context/swap-chain device-idle calls, and timeline waits.
- Implement the same public `RhiCommandRecordingLease` responsibilities as D3D12 with backend-private `VulkanCommandRecordingContext` pool details.
- Refactor existing mutable slot/pool path; no second Vulkan command abstraction or duplicate context tree.
- Render coordinator remains submission/reset authority; frame graph remains state/order authority.
- Pre-mortem MT-09–10, MT-19, MT-25–27, and MT-32–43. NVIDIA/AMD guidance agrees on exclusive pool ownership, meaningful command-buffer size, reuse, limited submits, and application-owned parallelism; backend validation and measurements decide the exact Sparkle policy.

Required implementation:
1. Allocate command pools with explicit thread/context and queue-family ownership; command buffers are leased exclusively.
2. Reset/recycle pool/buffer only after all submitted buffers from the relevant frame/context are GPU-complete.
3. Provide worker-local/per-frame transient descriptor pool blocks and upload pages or preassigned slices.
4. Precreate pipeline/layout/pass runtime before fan-out and assert no concurrent lazy mutation.
5. Preserve debug labels, object names, validation/synchronization diagnostics, timeline token behavior.
6. Keep coordinator-only ordered submission and explicit cross-queue semaphore/timeline edges.
7. Remove the single shared mutable pool/slot assumption after serial parity.
8. Split descriptor responsibilities by lifetime. Recording leases use worker/frame-local pool blocks or preassigned tables; persistent registered descriptors and retirement remain render-owned. Do not hold the broad allocator mutex across `vkUpdateDescriptorSets`, pool allocation, formatting, callbacks, or other unbounded/native work when a snapshot/two-phase design can avoid it.
9. Preserve `VulkanNativeQueue::SubmissionMutex` only as Vulkan-required external synchronization for an explicitly shared native queue; coordinator ownership remains the engine contract. Validation callback ingestion appends bounded owned messages and owner-side processing occurs after unlocking.
10. Replace context/resource-reuse device-idle calls with relevant timeline completion. Final backend/swap-chain destruction may use one owner drain; one resize must not drain in FramePipeline, device services, and VulkanSwapChain.

Validation:
- Vulkan validation and synchronization validation under worker migration/delay and many frames.
- API/name parity check proves D3D12 and Vulkan expose the same `RhiCommandRecordingLease` while only backend-private code names `VulkanCommandRecordingContext` and Vulkan command pools/buffers.
- Deliberate concurrent pool/reset misuse triggers engine assertion/validation in negative test.
- Descriptor pool exhaustion/overflow and command buffer retirement stress.
- Descriptor allocation contention and lock-held-native-call audit; concurrent validation callback/drain; exact idle-count resize/recreate test; provider/native queue sharing test.
- Serial renderer uses lease contract with image/state parity.
- Compare common RHI semantics to D3D12 while documenting backend-specific ownership differences.

Acceptance gate:
- No Vulkan command pool is concurrently accessed through supported paths.
- Rejected `RecordingContextLease`/`WorkerRecordingContext` aliases and filenames are absent; no D3D12 allocator/list noun leaks into the Vulkan/common contract.
- Pool reset/reuse is completion-token safe.
- Shared lease contract is backend-neutral without erasing Vulkan queue-family/pool rules.
- No worker submit, lazy pipeline/layout creation, or marker loss.
- No recording worker contends on the persistent descriptor registry, processes validation callbacks, waits for device idle, or gains submit/present authority through the native queue mutex.

Positive patterns: thread-local pool ownership, backend-private implementation, timeline retirement, paired validation.
Forbidden: one pool shared with mutex as final design, D3D12 assumptions copied blindly, pool reset at CPU frame end, worker queue submit.
~~~

## Prompt 20 — Compile and Execute Parallel Frame-Graph Recording Groups

~~~text
Implement Prompt 20 only after Prompts 18 and 19 pass.

Objective:
Use frame-graph dependencies to compile eligible pass recording groups, lease worker contexts, record concurrently, deterministically aggregate closed native command objects, and submit measured batches in unchanged compiled order with explicit entry/exit resource-state contracts on both backends. Keep preparation, native recording, software translation, aggregation, submission batching, and queue submission as distinct concepts.

Non-negotiable repository rules:
- Enforce J's canonical concurrency/rendering vocabulary and Rule 10: search canonical terms plus rejected aliases in the touched scope, use one responsibility per name, and delete temporary compatibility spellings before the gate.
- Audit FrameGraphCompiler/Execution/Submission, queue batches/waits, barrier planner, pass side effects, immediate command-list use, provider/present/readback islands, pass markers.
- Extend the existing frame graph; do not create a second render graph or let SparkleTasks decide GPU resource/queue ordering.
- Do not add an Unreal-style software RHI command stream or `RhiThread` in this prompt. Sparkle currently records `RenderCommandList` calls directly into backend command objects; retain that direct architecture and add an ADR entry pointing to J's five-condition translation gate.
- Every pass is serial by default and opts in only after an explicit safety audit.
- Refactor execution traversal so serial and parallel modes consume the same compiled plan; delete duplicate executor path.
- Pre-mortem MT-13, MT-19–21, MT-27, MT-29, and MT-32–43. Preserve the single-thread command/barrier/submission order as Epic and AMD's deterministic render-list examples do; parallel completion order has no GPU semantic authority.

Required implementation:
1. Add private compiled `RecordingGroup`/`RecordingPlan` with pass range, queue, prerequisites, initial/final resource states, context requirement, estimated command/recording cost, submission position, serial-island reason. Define `RecordingGroup`, `RecordingChunk`, `SubmissionBatch`, `aggregation`, and `translation` exactly as J Tutorial 16; do not use “merge command lists” unqualified.
2. Preserve frame-graph barrier/aliasing/cross-queue authority. Emit inter-group barriers/primary/coordinator work where independent lists cannot infer transitions.
3. Add pass parallel-safety declaration/audit: immutable inputs, prewarmed runtime, local transient allocations, no hidden side effects, provider/native affinity documented.
4. Begin with independent coarse compute/copy/draw passes whose CPU recording cost exceeds threshold. Combine adjacent tiny compatible groups for recording; never assume one pass equals one task/list/submission.
5. Submit recording tasks through SparkleTasks, lease backend contexts, begin/record/end on the worker, close into a preassigned result slot, then aggregate by `SubmissionOrderKey` independent of completion order.
6. Keep provider/frame-generation/present, screenshot/readback coordination, unaudited interop, and required primary barriers as explicit serial islands.
7. Add deterministic serial/parallel recording control using same plan and barrier comparison.
8. On D3D12, use separate direct lists as the baseline and submit ordered arrays through the existing submission service. Treat bundles as a measured reuse option, not the parallel-recording abstraction. Never concurrently record one list/allocator.
9. On Vulkan, select independent primary buffers for suitable pass groups or ordered secondary buffers for measured intra-rendering-scope work. Record required inheritance; execute secondaries from coordinator/primary work. Do not make secondary buffers mandatory in the common RHI.
10. Compile `SubmissionBatch` boundaries separately from recording groups. Sweep list count and batch size; allow early batches only at existing graph-safe submission points. Preserve queue waits/tokens and one coordinator submit owner.
11. Record draws/dispatches/build inputs per list, task ready/record/close time, aggregation time, command objects per batch, native submit calls/time, first-work availability, GPU gaps, descriptor/upload use, and transient native memory through existing profiling hooks.
12. Keep barrier/render-target/viewport/rendering-scope preamble and postamble in coordinator/primary work by default. Move state into worker lists only when the backend contract, compiled state plan, and validation prove it safe.

Validation:
- Serial/parallel images, pass order, queue waits, barrier entry/exit states, histories, and submission tokens match.
- D3D12 GPU validation and Vulkan synchronization validation under randomized recording delays/migration.
- Marker/object/timestamp continuity across split groups.
- Tiny graph remains grouped/serial; draw/pass-heavy graph measures record critical-path improvement.
- Sweep group/chunk/batch sizes. Track list/buffer count, barriers, descriptor/upload pages, transient memory, aggregation/submit CPU time, first GPU work and CPU/GPU/latency impact.
- A deliberate worker-completion reorder still yields identical closed-list order and submission batches.
- Architecture audit shows no software RHI replay objects, translate thread, raw native-buffer concatenation, or worker queue submission was introduced.

Acceptance gate:
- Only audited passes run parallel and native validation is clean on both backends.
- GPU submission order/resource semantics remain frame-graph-defined.
- Recording-group and submission-batch policies are independently measured; fewer submit calls are not accepted if they create material GPU starvation or latency regression.
- No lazy state, shared transient cursor, or worker submission in parallel execute.
- Performance gain does not buy unjustified GPU/list/memory regression.

Positive patterns: opt-in audit, compiled state contract, worker recording/coordinator submit, same serial plan.
Forbidden: parallelize all passes flag, infer barriers inside independent list, execute-time resource creation, second frame graph.
~~~

## Prompt 21 — Add Measured Intra-Pass Scaling and Close Advanced-Feature Preservation

~~~text
Implement Prompt 21 only after Prompt 20 passes.

Objective:
Add intra-pass chunked recording where pass-level parallelism is insufficient and close the full serial/threaded/parallel preservation matrix for raster, classic TLAS, PTLAS, reservoir lighting, reference path tracing, temporal/providers, shader ABI, capture, and both backends.

Non-negotiable repository rules:
- Enforce J's canonical concurrency/rendering vocabulary and Rule 10: search canonical terms plus rejected aliases in the touched scope, use one responsibility per name, and delete temporary compatibility spellings before the gate.
- Profile first to identify one or two expensive draw/dispatch planning/recording passes; do not add generic chunking to every pass.
- Reuse RecordingPlan/leases and existing pass/batch structures; no special per-feature thread systems.
- Apply daily refactor to selected pass data preparation and delete redundant serial-only batching after parity.
- Do not alter GPU algorithm/queue policy merely to make CPU task graphs look busier.

Required implementation:
1. Profile depth/GBuffer or equivalent opaque mesh work, transparent work, shadow passes, and RT build/trace-input work. Select at least two materially different real consumers when available—one raster draw-heavy path and one shadow-view, dispatch-heavy, or RT-input path. Record non-applicability rather than create fake work.
2. Choose chunk key/range (draw batches, instances, shadow views/caster ranges, dispatch ranges, RT build inputs) with exclusive recording output and deterministic order. Partition by estimated cost and bound chunks by context/native-memory budgets.
3. Precompute immutable shared pass state and per-chunk ranges; tasks record D3D12 direct lists/bundles only as measured or Vulkan primary/secondary buffers according to the backend plan. Do not call every backend result a “secondary stream.”
4. Join and aggregate chunks in deterministic pass-defined order, then feed Prompt 20's `SubmissionBatch` policy. Bound chunk count and use a serial threshold.
5. For shadow work, keep light/view order and transparent-sensitive caster/draw order stable. Keep pass barriers, render targets, viewport/scissor and rendering-scope setup in primary/coordinator work unless backend validation proves the selected alternative.
6. Audit classic TLAS and PTLAS initial/update/add/remove/asset replacement/trace identity and lifetime separately, including BLAS scratch/result/compaction inputs where currently supported.
7. Verify reservoir light ordering/IDs, reference path sample seed/accumulation/reset, temporal motion/depth/jitter/exposure/history tags.
8. Verify provider supported/enabled/operational/failure state, resource tags, thread affinity, resize/reload, frame generation/present sequencing.
9. Verify shader package/reflection/layout/generation coherence and capture/debugger continuity.
10. Record negative/small-work cases where chunking is disabled or slower and retain policy.

Validation:
- Full advanced-feature test matrix in J on D3D12/Vulkan where capability reports support.
- Serial/threaded/parallel recording reference images and deterministic plan identity.
- Delayed GPU completion plus level/asset/shader reload stress for RT/provider/capture paths.
- CPU record speedup, command-list/descriptor/transient/GPU-time and submission-latency impact for every selected heavy pass.
- Tiny/normal cases do not exceed accepted overhead budget.

Acceptance gate:
- Intra-pass work is measured, bounded, deterministic, and uses common ownership.
- No feature path is silently disabled/demoted to claim multithreading success.
- Both native backends pass applicable validation and capability truth remains honest.
- CPU gain and GPU/resource costs are reported separately.

Positive patterns: evidence-selected chunking, feature identity preservation, capability-gated tests, serial fallback.
Forbidden: task per draw, completion-order draw submission, provider-owned scheduling, CPU gain used to hide GPU regression.
~~~

## Prompt 22 — Reliability, Tools, Packages, and Deletion Closure

~~~text
Implement Prompt 22 only after Prompts 14, 16, and 21 pass.

Objective:
Close cross-subsystem reliability, deterministic cook/package workflows, editor lifecycle stress, public-surface review, and every compatibility/deletion ledger so one coherent architecture remains.

Non-negotiable repository rules:
- Enforce J's canonical concurrency/rendering vocabulary and Rule 10: search canonical terms plus rejected aliases in the touched scope, use one responsibility per name, and delete temporary compatibility spellings before the gate.
- Audit every remaining std::thread/jthread/async/future/mutex/shared_mutex/condition variable/atomic/memory-order operation, standard/Qt/native wait or detach, TaskExecutor instance, WaitForIdle call site, GameScene snapshot/pointer, controller, Scene* facade, host-render path, direct lifecycle callback, default report/artifact, launcher command/package kind. Search Engine, Tools, and Projects owned sources; exclude generated/external code but include wrapper policy and third-party worker counts.
- Search before adding any reliability helper; consolidate into current owner and delete obsolete mechanisms.
- Daily refactor is mandatory for every retained legacy hotspot touched.
- No new feature, framework, report system, task UI, or package kind is allowed in closure.

Required implementation:
1. Run and fix repeated stress: viewport recreate/dock/minimize, play/edit transition if owned, level load/cancel/reload, shader/asset reload, capture, provider resize/failure, delayed GPU, document/application shutdown.
2. Complete deterministic transactional asset/shader/texture cook fan-out from owned project catalogs/manifests.
3. Preserve curated in-repo levels and optional heavyweight content-pack policy; no default recursive uncataloged heavy-content scan.
4. Validate only owned runtime/editor/symbols/development-tools/optional-content packages; delete package/launcher/inspection surfaces without consumers.
5. Consolidate evidence into existing profiler/allocator/frame-graph/debugger hooks and delete superseded diagnostics/default timing artifacts.
6. Delete all old paths listed in prior prompts: futures/pools, old Entity/controller/object storage, duplicate Scene* state, raw snapshot/render host path, direct UI/renderer mutations, idle reload, full GPU rebuild, single-context-only recording path.
7. Audit public headers: no worker queues, ECS storage, caches, task timings, recording pools, or broad observation APIs.
8. Run include/dependency boundary checks and reduce unnecessary module/public dependencies.
9. Reconcile every J LC ledger row and every newly discovered primitive. Retained items require file-local owner/invariant/lock-order/blocking documentation and a falsifying stress test; replaced items have no call sites; deferral past this prompt is forbidden except expert hardening of an already-correct retained protocol in Prompt 24.
10. Prove no arbitrary callback, Qt/UI access, provider/driver call, blocking I/O, GPU wait, logging/formatting, or destruction occurs under an engine lock. Enumerated API-mandated exceptions require a narrow critical section, re-entry analysis, and test.
11. Recheck the concrete legacy hotspots: InputSystem callback registry; Logger/Timer; ShaderRecook; LauncherBackend/ProcessRunner/AssetCookerToolProcess; Streamline; D3D12/Vulkan queue, descriptor, upload, allocation-record, validation-message, fence/timeline, swap-chain, and idle paths.
12. Run the repository-wide naming reconciliation from J. Classify semantic uses of `Job`, `WorkItem`, `TaskRun`, bare `TaskNode`, `RenderFrameMailbox`, service-type `RenderThread`, `RenderThreadCommands`, `RecordingContextLease`, `WorkerRecordingContext`, misleading `RHIThread`, and unqualified cross-module `Queue`/`Context`. Delete rejected planned aliases; retain unrelated third-party/native terms only outside Sparkle ownership or behind an accurately named wrapper.

Validation:
- Long randomized stress repeatedly clean of deadlock/leak/stale callback/native validation/race symptom.
- All task/system/renderer modes 0/1/2/N, serial/threaded/parallel, D3D12/Vulkan applicable paths.
- Tool outputs deterministic and failed/cancelled generation never published.
- Package content and optional packs reproducible from documented commands.
- Repository-wide searches prove no unauthorized legacy counterpart remains.
- Repository-wide concurrency inventory has zero unclassified hits and zero obsolete LC consumers; retained primitive tests include re-entry, contention, cancellation/shutdown, and wrong-thread use as applicable.
- Repository-wide canonical/rejected-alias searches have zero unclassified owned-source hits; filenames, symbols, tests, CMake, comments, profiler labels, and thread labels agree.

Acceptance gate:
- One SparkleTasks runtime, one ECS world source, one packet/render path, one frame-graph authority, one owner per mutable subsystem.
- Every deletion ledger closed; no indefinite compatibility route.
- Every synchronization/blocking primitive is either a tested private implementation of the coherent architecture or deleted. Zero-unclassified—not zero-mutex—is the gate.
- Default workflow produces product artifacts, not research reports/log streams.
- Public surface and package/tool set are smaller/coherent and have real consumers.

Positive patterns: audit-to-zero, transactional products, owned workflows, public API reduction, repeated lifecycle stress.
Forbidden: keep old path “for safety,” new diagnostic subsystem, unowned package variant, test-only production API.
~~~

## Prompt 23 — Initial Full-System Performance Characterization and Tuning

~~~text
Implement Prompt 23 only after Prompt 22 passes.

Objective:
Tune the completed base architecture with representative evidence and establish the full-system reference captures required by the expert-hardening prompts. Do not declare the multithreading program or portfolio complete in this prompt.

Non-negotiable repository rules:
- Enforce J's canonical concurrency/rendering vocabulary and Rule 10: search canonical terms plus rejected aliases in the touched scope, use one responsibility per name, and delete temporary compatibility spellings before the gate.
- Use existing profiler/debugger/allocator/timestamp/benchmark launch hooks and manually maintained concise results.
- Search and consolidate existing docs rather than adding another architecture/policy document. Update J, K, and the owned concise product overview only where necessary.
- Performance tuning must not restore duplication, bypass ownership, or demote advanced features.
- Refactor touched tuning knobs/configuration into one coherent owner and delete experiments that lose.
- Pre-mortem MT-17–28, MT-35, MT-38, and MT-41–44. NVIDIA and AMD explicitly warn that more logical-core workers can reduce game performance through cache/chiplet/SMT contention, locks/atomics, false sharing, context switches, and power behavior; no worker-count formula passes without a workload/topology matrix.

Required implementation:
1. Tune worker count, FrameCritical/Background budgets, task grains, ECS query ranges, `RenderFrameQueue` depth policy, recording group/chunk size, upload/descriptor page sizes, and residency budgets using tiny and representative large scenes.
2. Run matrix: D3D12/Vulkan, serial/threaded zero/threaded one-ahead, serial/parallel recording, 0/1/2/N workers, cold/warm caches, validation/profile separately.
3. Collect CPU/GPU p50/p95/p99, critical paths, backpressure, task overhead/utilization, ECS query/storage/churn, upload/resource churn, lists/barriers/descriptors/transient memory, RT timings, tool throughput/peak memory, input-to-present.
4. Attribute gains separately to removed work, CPU task parallelism, game/render pipeline, GPU queue behavior, and shader/GPU changes.
5. Retain serial/small-work policies where parallel overhead loses; document negative results.
6. Record hardware metadata available through existing platform/profiler surfaces: physical/logical processor counts, relevant cache/topology facts, GPU/driver, memory, OS, power profile, build and third-party worker settings.
7. Identify concrete candidates for Prompts 24-28: one atomic protocol, one scheduler/topology pathology, one useful reduction/scan/partition workload, one streaming/PSO hitch path, and one GPU queue/latency experiment.
8. Perform preliminary teach-back on memory publication, task scheduling, ECS/DOD, render pipeline, GPU lifetime and native recording; record weak areas for the later expert prompts.

Validation:
- Reproducible commands/settings and machine/core/backend/configuration recorded.
- Independent rerun produces comparable results within stated variability.
- Native validation, correctness images, deterministic outputs, stress suite remain clean after tuning.
- Every public API/task primitive/tool/package has a current consumer.
- No research-only graph/data/panel/log/report enabled in shipping defaults.

Acceptance gate:
- Reference evidence shows correctness, tradeoffs, limitations, and causality—not only FPS or thread count.
- Representative large workloads improve where expected; tiny cases remain within budget.
- CPU gains do not cause unjustified GPU/memory/latency/backend regressions.
- Every Prompt 24-28 candidate names an existing Sparkle path and falsifiable success/failure criteria; no résumé-only subsystem is proposed.

Positive patterns: causal measurement, reproducibility, honest limits, negative-result retention, topology/configuration metadata.
Forbidden: premature portfolio-complete claim, cherry-picked FPS, one-backend claim, generated report product, tuning by thread count alone, unsupported “production parity” language.
~~~

## Prompt 24 — Harden Atomic Protocols and Scheduler Failure Modes

~~~text
Implement Prompt 24 only after Prompt 23 passes.

Objective:
Turn the task/packet runtime's real atomic and sleeping protocols into explainable state machines with reference implementations, lifetime proof, and adversarial failure tests. Improve current production owners; do not add a generic lock-free library.

Non-negotiable repository rules:
- Enforce J's canonical concurrency/rendering vocabulary and Rule 10: search canonical terms plus rejected aliases in the touched scope, use one responsibility per name, and delete temporary compatibility spellings before the gate.
- Inspect every atomic, condition variable, semaphore, queue state, generation counter and wait predicate before adding a primitive; record use/extend/replace/add decisions.
- Revisit retained legacy synchronization from LC-06/07/09/10/13-15/18: Logger publication, Timer pause flag, ProcessRunner only if any atomic remains, D3D12 linear/high-water state only if retained, queue submission values, Vulkan validation messages, allocation retirement, and native wait serialization. `relaxed` must name the independent invariant it serves.
- Apply daily refactoring to converge duplicated flags/wake paths and remove obsolete synchronization exposed by the audit.
- Keep protocols private to their owner. No public queue internals, generic concurrent containers, hazard-pointer framework, or test-only shipping API.
- Preserve serial/threaded modes, all lifecycle behavior and both rendering backends.

Required implementation:
1. Audit SparkleTasks ready/injection/worker-sleep/execution-completion state, `RenderFrameQueue` slots, change-journal cursors, generation handles, and retirement queues. For each atomic, document protected invariant and memory-order edge.
2. Select at least one real publication/reuse protocol (prefer `RenderFrameQueue`) and provide a mutex/condition-variable oracle plus a sequentially consistent atomic reference in tests.
3. Make the production acquire/release or locked protocol behaviorally equivalent. Draw publish, claim, acknowledge, reuse and close/shutdown transitions; keep GPU retirement separate.
4. Exercise compare-exchange expected-value behavior, generation wrap policy and ABA/stale-handle rejection. If a raw reclaimable pointer exists in a lock-free path, replace it with owned/indexed storage or stop for an explicit reclamation design.
5. Make every condition-variable wait predicate-based; prove notification cannot be lost when submission races parking or shutdown. Avoid spinning except for an already measured bounded handoff.
6. Inject worker-wait/nested-child exhaustion, cancellation-versus-start/complete, close-while-full/empty, thundering-herd wake, long-task starvation and priority-inversion scenarios. Fix ownership/dependency/lane policy rather than hiding them with recursive locks or arbitrary priority.
7. Add concise developer assertions for illegal state transitions, double consume/reuse, stale generation and worker blocking.
8. Teach back atomicity, modification order, synchronizes-with, relaxed/acquire/release/SC, progress guarantees, ABA and reclamation using the implemented protocol.
9. For each retained mutex/atomic pair, verify the atomic does not appear to publish lock-protected adjacent state and that lock-free readers cannot observe a torn logical snapshot. Remove atomics with no real cross-thread caller rather than preserving speculative thread safety.

Validation:
- Oracle, SC reference and production protocol deliver identical sequence/result/close behavior across randomized delays and millions of tiny wraparound transitions.
- 0/1/2/N workers; producers/consumers delayed at every state; cancellation and shutdown repeated under sanitizer-supported and optimized configurations.
- No lost/duplicate packet or task, deadlock, leaked scope, stale access, busy idle loop or validation regression.
- Repository search shows no duplicate state flag/wake protocol or new unowned concurrent primitive.
- Timer pause and Logger initialization/level behavior have explicit writer/reader/lifetime contracts; any retained allocator/queue atomic has reset/submission ownership and wrap policy tested.

Acceptance gate:
- Every production atomic has a stated invariant and sufficient memory-order/lifetime proof.
- No atomic remains solely because old code once anticipated concurrency; no atomic flag is treated as publication of unrelated fields.
- Reference implementations can falsify the optimized protocol and remain in the existing test surface.
- Scheduler failure modes settle predictably and leave no compatibility synchronization path.

Positive patterns: explicit state machines, reference-first weakening, bounded generations, predicate waits, fault injection, ownership repair.
Forbidden: “works on x64,” relaxed-by-default, raw-pointer lock-free reclamation by hope, busy waiting, public queue snapshots, generic lock-free framework.
~~~

## Prompt 25 — Characterize CPU Topology, Worker Policy, Contention, and False Sharing

~~~text
Implement Prompt 25 only after Prompt 24 passes.

Objective:
Make SparkleTasks scale from low-core machines to high-core/SMT/chiplet/heterogeneous systems through evidence-driven worker policy, while keeping topology handling private and OS-neutral by default.

Non-negotiable repository rules:
- Enforce J's canonical concurrency/rendering vocabulary and Rule 10: search canonical terms plus rejected aliases in the touched scope, use one responsibility per name, and delete temporary compatibility spellings before the gate.
- Inspect existing platform CPU queries, launch settings, profiler metadata and external-library thread controls before adding topology code or configuration.
- Use NVIDIA's thread-limiting and AMD's Ryzen/core-detection guidance as hypothesis sources, not universal constants. Pre-mortem MT-17–28 and MT-41–44, including SMT siblings, cache/chiplet traffic, false sharing, context switches, nested workers, power/frequency, and profiling perturbation.
- Apply daily refactoring so worker count, lane budgets and overrides have one owner and one documented precedence.
- Let the OS schedule by default. Affinity, CPU Sets, QoS and priority are diagnostic experiments unless repeated traces prove a product policy.
- Do not add a public topology service, affinity manager, benchmark-report generator or per-subsystem pool.

Required implementation:
1. Record available physical/logical processor, processor-group, SMT/cache-domain and heterogeneous-core metadata in existing benchmark/capture context. Gracefully mark unavailable facts.
2. Replace any unconditional `hardware_concurrency()-k` assumption with one private WorkerPolicy that supports serial, explicit count and measured capped automatic policy. Preserve a user/developer override through the existing settings mechanism.
3. Run compute-heavy, cache/gather-heavy, mixed critical-chain and foreground-plus-compiler/decode workloads at 0/1/2, physical-core-like, logical-core-like and selected capped N counts.
4. Capture task ready/running time, utilization, steals/failures, context switches, migrations, wakeups, p50/p95/p99 frame time, throughput, CPU frequency where available and background completion.
5. Detect and repair scheduler/`RenderFrameQueue` false sharing using layout/counter evidence. Separate true contention, bandwidth saturation, cache-domain traffic, and scheduling/preemption hypotheses.
6. Inventory third-party compiler/compression/provider worker counts and enforce existing lane/concurrency budgets so foreground work is not oversubscribed.
7. Run bounded affinity/QoS/priority experiments only to diagnose a captured problem; retain them only if cross-machine evidence and fallback behavior justify the complexity.
8. Document negative scaling and select defaults/thresholds per workload class without hard-coding one machine's optimum.

Validation:
- Matrix on at least one low/moderate-core and one high-core or logically constrained configuration; if hardware is unavailable, use OS core limits and state the limitation.
- Optimized build system trace through WPA/ETW, AMD uProf, Nsight Systems or an equivalent primary scheduler view; engine scopes correlate with threads/tasks.
- Idle engine does not produce excessive wakes; critical owner threads are not delayed by known background lock holders.
- Selected policy improves or preserves p95/p99 versus the old automatic choice on representative workloads and does not starve background completion.

Acceptance gate:
- Worker policy responds to measured workload/topology rather than maximizing logical threads.
- Contention/false-sharing claims have counter or controlled-layout evidence.
- No permanent hard affinity or priority policy remains without a documented cross-machine win and safe fallback.

Positive patterns: topology metadata, worker sweeps, ready-time analysis, capped budgets, neutral defaults, negative-result retention.
Forbidden: logical-core worship, pin-everything policy, priority as correctness, synthetic-only tuning, one-machine constants, hidden third-party oversubscription.
~~~

## Prompt 26 — Integrate Reduction, Scan/Compaction, Partition, and Deterministic Merge

~~~text
Implement Prompt 26 only after Prompt 25 passes.

Objective:
Demonstrate parallel-algorithm depth beyond parallel_for by improving real Sparkle data paths with serial oracles, deterministic contracts and measured crossover thresholds.

Non-negotiable repository rules:
- Enforce J's canonical concurrency/rendering vocabulary and Rule 10: search canonical terms plus rejected aliases in the touched scope, use one responsibility per name, and delete temporary compatibility spellings before the gate.
- Search ECS queries/commands, GPU-scene dirty updates, renderer preparation and cooker package/layout code for existing reductions, append loops, sorts, prefix offsets and merge helpers.
- Extend or replace existing algorithms; do not add a generic parallel-algorithm framework or duplicate buffers/helpers.
- Apply daily refactoring to remove shared append state, repeated sorting/offset logic and allocations exposed in the selected paths.
- Preserve byte/state determinism where required and declare explicitly where floating-point equivalence, rather than bitwise identity, is acceptable.

Required implementation:
1. Select at least three current consumers covering: a reduction, a compaction/scan or task-local list merge, and stable bucket/partition/merge. Prefer transformed bounds, GPU-scene dirty ranges, renderer draw preparation, ECS command merge and cooker record offsets/keys.
2. Build/retain a simple serial oracle for each consumer and define empty, capacity, ordering, floating-point and cancellation semantics.
3. Implement task-local partial reduction with a defined merge tree/order. Do not contend on one atomic accumulator for normal data.
4. Implement compaction using either task-local lists plus ordered merge or count/prefix-scan/scatter. Justify the choice by size/skew/allocation evidence.
5. Implement stable key/tie-break partition/bucketing/merge so work-stealing completion order cannot change packages, entity-command commit or render identity.
6. Test 0/1, just below/above grain, all/none selected, duplicate keys, skewed buckets, maximum capacity, randomized inputs and cancellation between phases.
7. Measure scheduling overhead, allocations, bytes moved, cache behavior where available, critical path and crossover. Keep serial paths for small or losing workloads.
8. Teach back map, reduction, scan, filter/compaction, partition, DAG and pipeline patterns; explain determinism and associativity costs.

Validation:
- Randomized property/reference tests compare every parallel result with its serial oracle across 1/2/N workers and repeated steal-order perturbation.
- Cooker products remain byte-deterministic; ECS command playback and render IDs remain stable; floating-point tolerance/order is documented where applicable.
- D3D12/Vulkan images and feature matrix remain correct for affected renderer/GPU-scene paths.
- Performance evidence shows why each retained parallel algorithm beats or usefully complements the serial path at representative scale.

Acceptance gate:
- At least three real paths demonstrate distinct parallel patterns without a new framework or shipping demo subsystem.
- Output ownership and merge order are explicit; no hot shared append/accumulator is left by convenience.
- Losing variants are deleted and nearby duplicate algorithms are converged.

Positive patterns: serial oracle, task-local partials, stable keys, explicit scan phases, randomized properties, measured crossover.
Forbidden: atomic append everywhere, nondeterministic package order, assuming floating-point associativity, benchmark-only consumer, parallelism below crossover.
~~~

## Prompt 27 — Complete Staged I/O and Cold-Cache PSO/Resource Hitch Control

~~~text
Implement Prompt 27 only after Prompt 26 passes.

Objective:
Turn current loading, cooking, shader reload and render resource creation into one bounded staged pipeline that distinguishes I/O completion, decode/build, shader compilation, native pipeline/resource creation, upload, owner commit and readiness; control first-run PSO/resource hitches without introducing a second cache or blocking recording.

Non-negotiable repository rules:
- Enforce J's canonical concurrency/rendering vocabulary and Rule 10: search canonical terms plus rejected aliases in the touched scope, use one responsibility per name, and delete temporary compatibility spellings before the gate.
- Inspect scene/asset/texture/shader reads, ProcessRunner, recook/reload, residency, PipelineStateManager, native device creation and all synchronous waits before adding a stage or cache.
- Inventory D3D12/Vulkan graphics/compute/RT pipeline creation, buffer/image/view creation, memory allocation/binding, descriptor writes/updates, acceleration-structure allocation/build inputs and all driver-facing locks. A vendor recommendation to parallelize does not override native external-synchronization or Sparkle owner contracts.
- Reuse the existing task runtime, asset/shader catalogs, renderer generation owner and upload/residency path. No second streaming service, PSO cache or I/O pool.
- Apply daily refactoring to converge duplicate read/decode/key/cache/reload code and delete accepted-path WaitForIdle/idle polling.
- DirectStorage/overlapped I/O is an optional backend only after the portable staged contract and measurements pass; it may not fork asset identity or publication.

Required implementation:
1. Express request → bounded I/O → decode/decompress → validate/build → shader compile if required → key discovery/deduplication → native create/allocation/bind/write/upload → owner commit → GPU-complete readiness as explicit states with capacity, cancellation, error and progress policy. Skip inapplicable states without collapsing their ownership.
2. Keep blocking file/process operations on the bounded I/O lane; CPU transforms return to normal/background task lanes. Do not occupy frame workers while waiting for I/O or child process pipes.
3. Carry request/document/world/asset/shader generations through every stage; late completion cannot replace newer state and failure leaves the prior generation usable.
4. Audit the current shader compiler backend contract. Use bounded worker processes when compiler/library global locks or crash isolation make in-process threads ineffective; otherwise use bounded task nodes. Preserve deterministic DXIL/SPIR-V/reflection/layout/package output and capture compiler-process count in the shared background-work budget.
5. Inventory runtime-required graphics/compute/RT pipeline and resource keys from owned pass/proxy data. Sort/deduplicate before native creation, use one in-flight request per key in the existing render-owned generation cache, and materialize before parallel recording.
6. Separate pipeline key discovery, shader readiness, native PSO creation and cache publication. Add cold/warm hit, miss, duplicate, too-late, failure and not-ready policies. Test loading-barrier, delayed draw/proxy and bounded fallback choices on their real products.
7. For RT pipelines, evaluate backend-native pipeline library/collection mechanisms only where capability and cold-cache evidence justify them. Keep common keys/results backend-neutral and delete a losing experiment.
8. Separate buffer/image descriptor creation from native object creation, memory allocation/suballocation from bind, view creation from descriptor publication, CPU upload preparation from copy recording, and GPU completion from generation readiness.
9. For every native operation, record allowed concurrent calls, exclusive input/output, required device/allocator/cache synchronization, owner commit, cancellation semantics and device-loss behavior. Parallelize independent object creation only after this audit.
10. Split descriptor work by lifetime: persistent descriptor registry/update remains render-owned or uses a proven two-phase snapshot/commit; transient tables/pages remain recording-lease-local. No resource task races an already-recording command list's descriptor visibility.
11. Define separate shader-compile, PSO-create, resource-create, decode and I/O concurrency/memory budgets for loading and gameplay. Account for third-party internal threads, driver serialization, duplicate similar pipeline compilation and large per-request peak memory; do not use one global “background threads” number blindly.
12. Define per-product “not ready” policy: loading barrier, delayed proxy/draw, bounded fallback, or explicit failure. Never surprise-block inside draw/dispatch recording.
13. Add existing-hook counters/scopes for cold/warm hit, miss, duplicate, too-late, failure, native-create time, driver-lock/queue time where observable, stage queue time, peak stage memory and foreground interference; no new report subsystem.
14. If measurements justify a platform asynchronous-I/O backend, hide it behind the current internal read-completion seam and prove identical cancellation/publication/package behavior.

Validation:
- Cold and warm caches, empty/corrupt/missing files, cancellation at each stage, supersession, process failure, full queues, shutdown and repeated reload.
- Shader/PSO cold cache is actually cleared or version-isolated; D3D12/Vulkan compile/create paths and fallback policies are exercised.
- Cold-cache graphics/compute and applicable RT pipelines; parallel duplicate/similar keys; buffer/image/view/allocation/bind/descriptor-update failures; delayed upload/GPU completion; native validation on both backends.
- Foreground p95/p99, total load/cook throughput and peak memory are measured across multiple worker/I/O/compile budgets.
- Sweep compile/create concurrency independently. Record driver serialization/negative scaling, peak memory, first-ready time and frame interference; retain serial or capped policies where they win.
- Accepted load/reload performs no routine device idle; parallel recording sees no lazy PSO/layout/resource mutation.

Acceptance gate:
- I/O, CPU work, native creation, upload and publication are distinct owned stages with bounded backpressure.
- First-run/cold-cache behavior is observable, deterministic where applicable and has an explicit user-visible policy.
- One asset/shader/PSO generation authority remains; superseded paths and caches are deleted.
- Every applicable RHI resource-work row in the coverage ledger has a safety contract and proof; RT pipeline-library/collection and parallel native-creation variants are retained only with a real consumer and measured benefit.

Positive patterns: staged ownership, bounded capacity, generation rejection, cold-cache testing, deduplication, memory-aware concurrency.
Forbidden: “async” blocked frame worker, warm-cache-only proof, PSO creation in recording, unbounded compile fan-out, second asset/cache system, DirectStorage résumé port.
~~~

## Prompt 28 — Prove GPU Queue Concurrency, Frame Pacing, and Correlated Latency

~~~text
Implement Prompt 28 only after Prompt 27 passes.

Objective:
Correlate CPU tasks, render submission, GPU graphics/compute/copy execution and presentation; retain only queue overlap and pipeline depth that improve a measured product objective without violating provider ownership or latency.

Non-negotiable repository rules:
- Enforce J's canonical concurrency/rendering vocabulary and Rule 10: search canonical terms plus rejected aliases in the touched scope, use one responsibility per name, and delete temporary compatibility spellings before the gate.
- Inspect existing `FrameId`/sequence markers, frame-graph queue assignment, RHI timelines, present path, provider/frame-generation integration, `RenderFrameQueue`, and profiler hooks before adding fields or events.
- Keep the frame graph as queue/dependency/barrier authority and the render coordinator as submit/present owner.
- Apply daily refactoring to converge duplicate frame IDs, waits, pacing settings and provider queue state.
- Do not make the architecture vendor-specific. Vendor latency APIs are optional adapters through existing provider boundaries; engine markers and fallback remain authoritative.

Required implementation:
1. Carry one correlated produced/rendered/presented frame identity through input sample, simulation start/end, packet publish/consume, recording, queue submit, GPU timestamps and present start/end.
2. Measure serial, threaded zero-ahead and one-ahead modes in CPU-bound and GPU-bound workloads; record CPU lead, backpressure, throughput, frame pacing and input-to-present stages.
3. Select real async-compute and copy candidates from declared frame-graph dependencies. Model graphics/compute/copy ownership, cross-queue signal/wait, resource-family/state transitions and lifetime.
4. Compare serial-queue and multi-queue execution using the same logical passes. Measure overlap, queue idle gaps, synchronization/submission CPU cost, bandwidth contention and GPU critical path.
5. Audit D3D12/Vulkan provider/frame-generation queue requirements including shared versus exclusive queues, present/acquire behavior, callbacks, shutdown and fallback. Assert illegal concurrent submission/ownership where possible.
6. Batch submissions only when it reduces CPU cost without delaying required GPU work or inflating latency; record the tradeoff.
7. Establish bounded pacing/backpressure policy for GPU-bound overload, VSync/VRR/present modes available to the engine and provider enabled/disabled paths.
8. Remove async/copy assignments or extra frame lead that lose. Preserve feature correctness and temporal frame semantics.

Validation:
- D3D12/Vulkan serial/parallel recording, graphics-only/multi-queue, zero/one-ahead and supported present/provider modes.
- Queue timelines from PIX/GPUView/RGP/Nsight/RenderDoc or appropriate backend tools correlate with engine frame/pass markers.
- Repeated minimize/resize/recreate, provider toggle/failure, frame-generation, capture and shutdown show no deadlock, stale frame ID or illegal queue use.
- Report CPU/GPU p50/p95/p99, pacing variance, stage latency and queue overlap with exact hardware/driver/configuration.

Acceptance gate:
- Every overlap claim identifies the two timelines, enabling dependency/fence and measured benefit.
- Pipeline depth/backpressure has an explicit throughput-versus-latency product policy.
- Provider queue ownership is documented and enforced; no shared queue can deadlock through uncontrolled submission.

Positive patterns: correlated frame identity, graph-derived queues, measured overlap, bounded CPU lead, explicit provider ownership, negative-result deletion.
Forbidden: two queues imply overlap, FPS-only latency claim, vendor-only frame identity, provider-owned renderer scheduling, unbounded frames ahead, gratuitous fence/submission traffic.
~~~

## Prompt 29 — Production Forensics, Expert Defense, and Final Portfolio Release

~~~text
Implement Prompt 29 only after Prompt 28 passes.

Objective:
Prove the completed architecture can be diagnosed, defended and reproduced at an AMD/NVIDIA graphics/systems interview bar, then release a concise honest portfolio without adding an interview-only product surface.

Non-negotiable repository rules:
- Enforce J's canonical concurrency/rendering vocabulary and Rule 10: search canonical terms plus rejected aliases in the touched scope, use one responsibility per name, and delete temporary compatibility spellings before the gate.
- Use existing engine instrumentation and external primary tools; do not add a crash service, telemetry/report framework, task panel or benchmark product.
- Search/consolidate J, K and the existing concise product overview. Do not create another architecture/policy document.
- Apply daily refactoring to remove injected-failure hooks from shipping paths, consolidate markers/settings and close every remaining deletion ledger.
- Do not claim AMD/NVIDIA employment equivalence, proprietary-engine knowledge, unavailable hardware validation or “production ready” without defined scope.

Required implementation:
1. Create three reproducible incidents in existing tests/developer launch surfaces: race/lifetime or stale generation; deadlock/stall/priority inversion; and scaling/pacing/latency regression.
2. Diagnose each timeline-first: exact reproduction, classification, serial comparison, system/native capture, critical path/owner, falsifiable hypotheses, injected delay, root cause, fix, regression and post-fix measurement.
3. Use appropriate evidence among WPA/ETW, Visual Studio, AMD uProf/RGP, Nsight Systems/Graphics, PIX/GPUView, RenderDoc, Vulkan/D3D12 validation, sanitizers and crash dumps. State tool/hardware gaps.
4. Complete J's 30-question bank, whiteboard and coding drills live: bounded-queue atomic protocol, DAG critical path, deadlock repair, deterministic compaction, native recording review, and trace diagnosis.
5. Produce final portfolio artifacts: ownership/frame diagrams, task/ECS graphs, atomic state machine, topology scaling curve, algorithm walkthrough, staged streaming/PSO trace, D3D12/Vulkan recording, GPU queue/latency timeline, incident reports and limitations.
6. Attribute gains separately to removed work, data layout, CPU tasks, game/render overlap, native recording, GPU queue scheduling, and shader/GPU changes. Retain negative results.
7. Re-run the complete serial/thread/backend/feature/cold-cache/stress/performance matrix and independently reproduce representative results.
8. Verify every public API, tool, task primitive, package, setting and retained diagnostic hook has a current product consumer; delete temporary or duplicate surfaces.
9. Rerun the exact repository-wide legacy-concurrency audit from Prompt 00. Explain every retained standard/Qt/native primitive from owner to falsifying test, verify no old LC replacement path reappeared during tuning, and include the zero-unclassified result in the portfolio review.
10. Reconcile MT-01 through MT-44 against the final code. For every ID, link the concrete preventing owner/pattern and test/trace, or mark it non-applicable with a reviewable reason. Run an adversarial review that attempts to reintroduce raw cross-owner state, worker wait, tiny work, false sharing, native allocator sharing, premature GPU reuse, nondeterministic merge, and misleading performance claims.
11. Close every row in `Renderer/RHI Use-Case-to-Prompt Coverage` with code/test/capture evidence or a source-backed non-applicability decision. Teach back light/shadow/mesh preparation, shader/PSO/resource staging, direct versus software-translated RHI recording, D3D12/Vulkan list ownership, the qualified meanings of merge, submit batching, retirement, and the CPU-task versus GPU-queue distinction. Distributed light baking and an RHI translation thread remain defer decisions unless separately approved through their product/ADR gates.
12. Update release-facing Current/validated status and limitations only after evidence passes; close J's Definition of Done and every K gate.

Validation:
- All 30 prompt reports exist; exact commands/configurations/hardware are recorded and representative reruns are comparable within stated variability.
- Three incident regressions fail before their fixes or fault injection and pass after; native validation and sanitizer-supported runs remain clean.
- Portfolio claims link to code/tests/captures and separate CPU/GPU/latency causality.
- A mock adversarial review can alter a scenario or trace and still receive a reasoned answer, not a memorized script.
- The renderer/RHI coverage ledger contains no “could be parallel” closure; each row is implemented/proven, explicitly non-applicable, or assigned to a separately approved future renderer program.

Acceptance gate:
- The owner can design, code, debug, measure and teach the core engine concurrency concepts under questioning.
- Sparkle solves real loading, framework, editor, renderer, RHI and tools problems with one coherent ownership architecture.
- The final product has no interview-only subsystem, hidden duplicate path or unsupported vendor claim.
- The final product has no unclassified thread, lock, atomic, wait, detached lifetime, callback-under-lock, multi-owner queue, or routine device-idle path.
- Every MT atlas hazard has executable evidence or a concrete non-applicability argument; no closure rests only on a vendor example or absence of an observed failure.

Positive patterns: incident-based evidence, tool selection, live reasoning, causal attribution, reproducibility, honest limitations, deletion closure.
Forbidden: memorized definitions, fabricated capture, cherry-picked FPS, generated report product, unbounded résumé feature, unsupported company/parity claim.
~~~

## Final Series Gate

The series is complete only when:

- all 30 prompt reports exist and every gate is PASS;
- J's Definition of Done is satisfied by executable evidence;
- all compatibility/deletion ledgers are closed;
- the current repository-wide concurrency scan has zero unclassified hits and each retained primitive has an owner, invariant, blocking/affinity policy, and falsifying test;
- MT-01 through MT-44 have concrete final-code prevention evidence or reviewable non-applicability, and each implementation report contains its applicable hazard pre-mortem/closure;
- the repository has one coherent ownership/data path per responsibility;
- the implementation remains understandable in serial mode;
- the owner can explain and defend every major design, tradeoff, failure mode, and measurement without relying on this document as a script.

If any condition is false, the multithreading program is still in progress. Do not relabel a partial infrastructure milestone as completion.

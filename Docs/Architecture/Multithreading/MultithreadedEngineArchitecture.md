# J. Multithreaded Engine Architecture

Status: canonical target architecture and decision record; not proof of current implementation

Last consolidated: 2026-08-02

Scope: SparkleTasks, GameFramework publication, render coordination, frame-graph recording, RHI ownership, editor/tool background work, cancellation, and shutdown

## Purpose and Authority Boundary

This document owns the multithreaded target design and the reasons behind its major choices. It does not own:

- current repository inventory — use [D. Whole Repository Architecture Map](../WholeRepositoryMap.md) and inspect code;
- general implementation rules — use [Engineering Standards](../../Engineering/Standards/README.md);
- Renderer/RHI dependency direction — use the canonical [Renderer and RHI Architecture Boundary](../RendererRhiBoundary.md);
- ordered work packages — use [K. Multithreaded Engine Implementation Plan](ImplementationPlan.md);
- scene/performance acceptance — use [I. Acceptance Workloads](../../Engineering/BistroAndSanMiguelWorkloads.md);
- principal capability/evidence definitions — use [A. Requirements](../../Strategy/Requirements.md).

K's prompt narratives and historical records do not prove current implementation. Before acting on this target, inspect the current owners, tests, and replacement history.

## Executive Decision

Sparkle uses one bounded engine task runtime, explicit owner-thread commits, immutable cross-thread publication, a dedicated render coordinator, a bounded frame queue, persistent render/GPU state, frame-graph-owned rendering dependencies, exclusive backend recording contexts, and single-owner submission/presentation.

The architecture adds concurrency only after ownership, lifetime, serial behavior, failure, and memory bounds are explicit. Task completion order is never product semantics.

The desired result is not maximum parallelism. It is predictable frame work with:

- one authority for every mutable state;
- independently writable work ranges;
- deterministic fan-in;
- bounded queues and memory;
- cancellation and shutdown settlement;
- D3D12/Vulkan parity;
- causal CPU/GPU performance evidence.

## Goals and Non-Goals

### Goals

- Reuse `SparkleTasks` for owned CPU work across runtime, renderer preparation, editor operations, and tools.
- Keep GameFramework, Renderer, frame graph, RHI, and Editor authorities distinct.
- Move data between owners as immutable values, stable handles, explicit deltas, packets, tokens, or leases.
- Overlap useful CPU work without moving correctness into priorities, timing, or worker waits.
- Record independent frame-graph groups concurrently while preserving one submission/present owner.
- Reuse persistent renderer/GPU data and retire it by completion token.
- Make tiny workloads remain near serial cost.
- Preserve supported rendering/provider/capture behavior across both backends.

### Non-Goals

- one permanent thread or pool per subsystem;
- a second ECS, renderer, frame graph, GPU scheduler, or cancellation runtime;
- worker-side waits, detached tasks, or unbounded producer queues;
- lock-free cross-module structures without a measured need and reclamation proof;
- shared mutable world/renderer graphs protected by broad locks;
- backend-native types in Renderer;
- automatic RHI dependency discovery competing with the frame graph;
- a general job-system SDK, general ML runtime, or diagnostics product;
- concurrency added solely to increase worker utilization.

## System Topology

```text
Application / Editor owner
    input, UI, semantic commands, world commit
                |
                | immutable requests / accepted deltas
                v
GameFramework owner --------------------------+
    ECS + resources + systems                 |
    structural commit                         |
    immutable structural/dynamic publication  |
                |                             |
                v                             |
         bounded RenderFrameQueue             |
                |                             |
                v                             |
RenderCoordinator owner                       |
    apply render deltas                        |
    persistent render/GPU scene               |
    build/compile frame graph                  |
    dispatch recording groups ---- SparkleTasks fixed workers
    submit / present                           |
                |                              |
                v                              |
       public neutral RHI contracts            |
          /                 \                   |
 backend-private D3D12   backend-private Vulkan
                |                              |
                +---- completion tokens -------+
```

`SparkleTasks` is shared execution infrastructure, not a mutable service locator. Each subsystem retains its domain state and commit boundaries.

## Owner and Thread Roles

| Role | Owns | May publish | Must not do |
| --- | --- | --- | --- |
| Application/Editor owner | lifecycle, input, ImGui/widget state, semantic commands, accepted UI models | immutable requests and commands | expose live UI/world pointers to workers or Renderer |
| GameFramework owner | world/ECS authority, resources, structural mutation, deterministic commit | generation-pinned world/render inputs | let workers structurally mutate storage or Renderer query the world |
| SparkleTasks workers | execution of dependency-ready bounded work | task-local/exclusive results and completion | wait for sibling tasks, retain borrowed epochs, mutate unrelated owners |
| RenderCoordinator | render-world state, frame graph execution, RHI mutation, submission, present | frame products, completion, narrow results | query ECS, invoke UI, or share submission authority |
| RHI backend | native objects, encoding, capabilities, queues, swap chain | neutral handles/tokens and explicit interop | own renderer policy or infer a second frame graph |
| GPU queues | submitted command execution | completion/fence/timeline progress | define CPU ownership or task dependencies |

Thread names describe ownership, not a guarantee that every stage has a permanent OS thread. Lane and priority are policy only.

## Frame Lifecycle

One logical frame follows this order:

1. Application/Editor accepts input and semantic operations.
2. GameFramework applies structural commands at its owner boundary.
3. Systems evaluate against a frozen world epoch, using a serial path or dependency graph.
4. The owner deterministically commits task-local results.
5. GameFramework publishes immutable structural and dynamic render input for one generation.
6. The bounded frame queue transfers ownership to `RenderCoordinator`.
7. Renderer applies deltas to persistent render-owned state.
8. Renderer builds and compiles the frame graph serially.
9. Dependency-independent recording groups may record through exclusive RHI leases.
10. `RenderCoordinator` submits batches in compiled order and presents.
11. Completion tokens retire CPU/GPU storage and unblock bounded reuse.

Frames may overlap only where these owner/lifetime edges permit. CPU task concurrency, render pipelining, command recording, GPU queue overlap, and frames in flight are separate mechanisms and measurements.

## SparkleTasks Contract

### Public Concepts

- `TaskDesc` describes work and policy.
- `TaskNodeHandle` identifies topology nodes.
- `CompiledTaskGraph` is immutable reusable topology.
- `TaskExecution` is one graph run with its own counters, cancellation, failure, and result state.
- `TaskExecutionContext` contains transient task-run access, not domain services.
- `TaskExecutor` owns the fixed worker runtime.
- `TaskScope` binds asynchronous work to owner lifetime and settles before destruction.
- `TaskEvent` is an explicit host/task completion primitive, not a general callback bus.
- `ParallelFor` partitions a bounded range above a measured serial threshold.
- `TaskLane` expresses scheduling policy; dependencies express correctness.

### Execution Rules

- Compile validates topology, dependency counts, cycles, bounds, and deterministic node identity.
- One execution allocates bounded state and publishes ready work to the fixed workers.
- Workers take dependency-ready tasks, read immutable input, and write exclusive or task-local output.
- Completing a task releases dependents; fan-in becomes ready only when all prerequisites settle.
- Finalization publishes success, failure, or cancellation exactly once.
- Worker tasks never wait on other task handles.
- Owner destruction first stops publication, then cancels/scopes work, settles it, and only then destroys captured state.
- Third-party worker counts are included in oversubscription policy.

The reference implementation begins serially with the final graph/data contract. Parallel execution must preserve its result and failure semantics.

## GameFramework and ECS Publication

### World Authority

GameFramework owns private ECS storage. `EntityId` is generational runtime identity; dense indices and component addresses remain epoch-local. Heavy immutable assets use typed handles rather than component-owned mutable objects.

Systems declare phase, typed component reads/writes, non-ECS resource access, prerequisites, and grain policy. Access declarations derive hazards: read/read may overlap; write conflicts require ordering or graph rejection.

### Structural Epoch

Queries execute against a frozen structural epoch. Workers cannot create/destroy entities, add/remove components, or resize stores. They emit typed task-local commands; the GameFramework owner merges them by a stable key and commits them at the structural boundary.

### Published Streams

Rendering consumes two explicit derived streams:

- **Structural** — identity, mesh/material/resource associations, creation/removal, topology, and other infrequent changes.
- **Dynamic** — transforms, previous-frame values, lights, animation outputs, visibility inputs, and other frame-varying values.

Each publication carries stable identity, generation/sequence, deterministic order, and owned lifetime. Incremental application has stale-sequence rejection and a full-resynchronization path. Renderer never dereferences `GameWorld` or retains component views.

Sparse-set storage is the initial default for packed iteration plus stable entity lookup. Archetype/chunk complexity requires workload evidence that the simpler design loses.

## Render Coordination

### Bounded Frame Queue

`RenderFrameQueue` transfers owned immutable frame input between the producing owner and `RenderCoordinator`. Capacity is explicit and the full-queue policy is documented as backpressure, replacement, or rejection—never unbounded growth.

Shutdown closes publication, wakes blocked host boundaries, settles queued/in-flight frames, retires completion-owned resources, and joins the coordinator before dependency destruction.

### Persistent Render and GPU State

Renderer resolves stable `RenderObjectId` values into persistent slots. Structural updates change topology/resources; dynamic updates touch bounded dirty ranges. Static resources use immutable handles and residency. Capacity growth publishes replacement storage at a frame boundary and retires old storage by GPU token.

Full-scene rebuild/upload is not the default. Measure packet bytes, dirty/upload bytes, descriptors, resource churn, memory high-water, RT build/update cost, and retirement backlog.

### Preparation and Frame Graph

Renderer preparation may use `SparkleTasks` only for independent data transforms with explicit dependencies and exclusive outputs. The deterministic join produces inputs for one frame graph.

The frame graph remains the authority for pass order, resource lifetime intervals, aliasing, barriers, queue ownership, and recording eligibility:

- setup declares all reads, writes, history, queue preference, and imported/persistent resources;
- compile resolves dependency and lifetime policy;
- execute records only compiled work.

Setup/compile remains serial until its own measured cost justifies a bounded redesign.

## Parallel Command Recording

The compiled frame graph may form `RecordingGroup` or `FrameGraphSubmissionBatch` units that are dependency-independent and large enough to amortize scheduling/allocator cost.

Each recording task receives one move-only `RhiCommandRecordingLease` backed by an exclusive D3D12 allocator/list or Vulkan command pool/buffer plus preassigned transient upload/descriptor storage. A recording worker may encode commands; it may not create/destroy global resources, mutate caches, submit, present, or wait.

`RenderCoordinator` collects completed groups in compiled order, applies required cross-group/queue dependencies, submits, and presents. Recording completion order never changes render order.

An additional GPU queue is accepted only when correlated timelines show useful overlap after synchronization, ownership transfer, bandwidth, and pacing cost. Parallel CPU recording alone is not evidence of GPU concurrency.

## Asset Loading, Editor, and Tools

### Loading and Publication

Read, decode, transform, validate, and publication are distinct stages. Background work produces an immutable package; the GameFramework owner transactionally accepts it or retains the previous accepted world. Cancellation and failure never publish a partial scene.

Runtime loading remains cooked-only. Weighted memory limits account for decoded scenes/textures, compiler processes, third-party workers, and publication backlog.

### Editor

Editor main owns ImGui, selection, transactions, draft widget values, document/application lifetime, and application of immutable results. Panels consume immutable models and submit semantic commands using stable identity.

One private `EditorOperationService` uses `SparkleTasks` scopes for owned background operations. Requests/results are owned values, progress is bounded/coalesced, latest-generation-wins is explicit where appropriate, and close cancels/settles before model destruction.

UI draw data and viewport/capture products cross the render boundary as owned packets/handles/tokens, never live ImGui or editor pointers.

### Tools

Cookers, shader compilation, import, launcher operations, and packaging reuse the same task runtime with host-specific lane and memory policy. Outputs publish deterministically and transactionally. Tools do not create second pools, asset databases, cancellation systems, or runtime authorities.

## Failure, Cancellation, and Shutdown

Every asynchronous operation has a named owner, terminal states, and exactly-once settlement. Cancellation means “stop accepting/starting unnecessary work and settle safely,” not “abandon captured state.”

Required shutdown order is:

1. stop new external requests/publication;
2. close bounded queues and wake host waiters;
3. cancel owned scopes/executions;
4. let running tasks reach defined cancellation points;
5. settle results and reject late generations;
6. drain/retire GPU-token-owned resources as required;
7. join coordinator/workers;
8. destroy captured owners, allocators, devices, and platform state.

Routine operation does not use device idle. Final teardown may use a documented bounded device-idle boundary where the backend contract requires it.

## Architecture Decisions

| Decision | Selected design | Rejected alternative | Revisit when |
| --- | --- | --- | --- |
| CPU runtime | one fixed `SparkleTasks` executor | per-subsystem pools, `std::async`, detached workers | product isolation cannot be expressed with scopes/lanes/bounds |
| Correctness order | explicit DAG dependencies | priority, timing, or worker waits | never; policy cannot replace correctness |
| Task lifetime | scopes and exactly-once settlement | raw-owner callbacks and fire-and-forget | never for owned work |
| World storage | private sparse-set ECS first | public ECS SDK or immediate archetype framework | measured workloads show storage/iteration loss |
| Structural mutation | deferred deterministic owner commit | mutation during queries | never |
| Game/render boundary | immutable structural + dynamic streams | shared scene graph/mutex or renderer world queries | only if a new product contract disproves ownership split |
| Render state | persistent slots and dirty ranges | full rebuild/upload each frame | only for bounded tiny/reference paths |
| Render schedule | frame graph authority | renderer/RHI competing graphs | never while frame graph owns resources/barriers |
| Recording | exclusive backend leases, parallel eligible groups | shared command contexts | only if APIs and evidence support a safer owner model |
| Submission/present | one render-coordinator owner | worker submission or permanent RHI thread | measured submission bottleneck with preserved ownership |
| Editor work | one scoped operation service | per-panel workers/callbacks | product requirements need a distinct real owner |
| Publication | bounded, generation-aware, transactional | partial state or unbounded queues | never |

## Verification and Performance

The applicable [Concurrency](../../Engineering/Standards/Concurrency.md), [Data-Oriented Design](../../Engineering/Standards/DataOrientedDesign.md), [graphics](../../Engineering/Standards/GraphicsEngineering.md), and [validation/evidence](../../Engineering/Standards/ValidationPerformanceAndEvidence.md) standards own test and measurement rules.

Architecture acceptance requires, as applicable:

- serial equivalence and deterministic repeated output;
- 1/2/N worker and randomized-completion stress;
- cancellation, owner-destruction, bounded-queue, and shutdown tests;
- D3D12/Vulkan native validation and delayed GPU retirement;
- tiny and representative Tier 1 workloads;
- memory high-water and oversubscription accounting;
- correlated CPU/GPU timelines and before/after critical-path evidence;
- feature-preservation checks for supported raster, RT, temporal/provider, capture, and packaging paths.

## Source and Rationale Trail

[E. External Renderer Repository Comparison](../ExternalRendererComparison.md) owns the broad vendor/framework comparison. The architecture also relies on primary API/language authorities for memory ordering, queue synchronization, resource lifetime, and backend behavior.

External sources are used narrowly:

- task/DAG systems support dependency-driven bounded execution, not copying another engine's public job API;
- Epic's game/render split and MassEntity patterns support derived render state, typed access, transient views, and deferred structural mutation, not Unreal's framework scale;
- NVIDIA Donut/NVRHI and AMD/GPUOpen renderers support explicit backend resources, persistent indexed data, and clear feature boundaries, not a general gameplay architecture;
- D3D12, Vulkan, and C++ specifications control correctness; sample repositories are design precedent only.

Record exact source revisions in research or a focused decision when a new choice depends on them. Do not turn vendor prevalence into a Sparkle rule.

## Current-State Use

Before implementing or resuming a stage:

1. inspect current code and tests;
2. classify prior criteria as enduring, transitional, or superseded;
3. use this document for target ownership and decisions;
4. use K only for bounded sequence;
5. apply current Engineering Standards and workload gates;
6. report current evidence rather than repeating historical completion text.

The architecture is complete only when code, tests, captures, and measurements prove it. This document is the map, not the proof.

# J. Multithreaded Engine Architecture

Status: canonical target architecture and decision record; not proof of current implementation

Last narrowed: 2026-08-11

Scope: task execution, thread ownership, cross-thread publication, bounded queues, render coordination, parallel command recording, cancellation, and shutdown

## Purpose and Authority Boundary

This document owns SparkleEngine's multithreaded target topology and the reasons for its concurrency decisions. It describes where work may run, who may mutate state, how results cross thread boundaries, and how asynchronous work settles.

It does not own domain data models, renderer features, content pipelines, product capability targets, schedules, or general implementation and validation rules. Use their owners directly:

- [D. Whole Repository Architecture Map](../WholeRepositoryMap.md) and current code for implemented structure;
- [Renderer and RHI Architecture Boundary](../RendererRhiBoundary.md) for Renderer/RHI responsibilities and frame-graph authority;
- [Engineering Standards](../../Engineering/Standards/README.md) for binding implementation rules;
- [F. Principal Graphics Roadmap](../../Strategy/Roadmap.md) for concurrency, feature, and portfolio sequencing;
- [A. Requirements](../../Strategy/Requirements.md) and [I. Acceptance Workloads](../../Engineering/BistroAndSanMiguelWorkloads.md) for capability and workload evidence.

Before using this target, inspect the current owners, producers, consumers, tests, waits, queues, and replacement history. This document does not prove that a target mechanism is implemented.

## Decision

Sparkle uses one bounded `SparkleTasks` runtime for owned CPU work. Mutable domain state remains on its named owner thread. Workers consume immutable or exclusively leased input, produce task-local or exclusively writable output, and publish only through explicit owner-thread commit boundaries.

A bounded queue transfers frame input to one render coordinator. The coordinator alone mutates render/RHI state, submits work, and presents. Dependency-independent frame-graph groups may record commands concurrently through exclusive backend recording leases; completion order never determines submission order.

Concurrency is accepted only when serial behavior, ownership, lifetime, deterministic fan-in, failure, cancellation, queue bounds, and a useful measured crossover are explicit.

## Concurrency Invariants

- Every mutable object graph has one thread or task owner.
- Cross-thread data is immutable, transferred, exclusively leased, or governed by one documented synchronization protocol.
- Dependencies express correctness; lanes and priorities express policy only.
- Workers never wait for sibling work and never retain borrowed epochs beyond their task.
- Task completion order is not observable product semantics.
- Queues, executions, progress, scratch memory, and frames in flight are bounded.
- Every asynchronous operation reaches exactly one terminal state.
- Owner destruction settles its work before captured state is destroyed.
- Tiny work uses the serial path; retained parallel work has a measured crossover.

## Target Topology

```text
Application / Editor owner
    input, UI, lifecycle, semantic commands
                |
                | immutable requests and accepted results
                v
GameFramework owner --------------------------+
    world mutation and deterministic commit   |
    frozen inputs for eligible system tasks   |
                |                             |
                | owned immutable frame input |
                v                             |
         bounded RenderFrameQueue             |
                |                             |
                v                             |
RenderCoordinator owner                       |
    renderer/RHI mutation                      |
    frame-graph setup and compile              |
    dispatch recording groups ---- SparkleTasks fixed workers
    ordered submission and present             |
                |                              |
                v                              |
       public neutral RHI contracts            |
          /                 \                  |
 backend-private D3D12   backend-private Vulkan
                |                              |
                +---- completion tokens -------+
```

`SparkleTasks` supplies execution capacity. It does not own GameFramework, Renderer, RHI, Editor, or tool state and is not a mutable service locator.

## Thread Roles

| Role | Owns | Cross-thread contract | Forbidden work |
| --- | --- | --- | --- |
| Application/Editor owner | lifecycle, UI state, commands, acceptance of UI results | immutable requests, packets, and narrow results | exposing live UI or owner pointers to workers |
| GameFramework owner | world mutation and structural commit | frozen task inputs, task-local results, immutable render input | worker-side structural mutation or shared live world access |
| `SparkleTasks` workers | execution of dependency-ready bounded work | exclusive output ranges or task-local results | sibling waits, owner mutation, submission, presentation |
| Render coordinator | mutable Renderer/RHI state, submission, presentation, render lifecycle | frame input, recording leases, completion tokens | world/UI queries or shared submission authority |
| RHI backend | native recording objects and queue operations | move-only recording leases and neutral completion tokens | renderer policy or a competing scheduler |
| GPU queues | submitted command execution | fence/timeline completion | CPU task or owner semantics |

Thread names describe ownership. They do not require one permanent OS thread per subsystem. Lanes and priority do not change ownership.

## Frame Concurrency Lifecycle

One logical frame observes these concurrency boundaries:

1. Application/Editor applies input and accepted operation results on its owner thread.
2. GameFramework commits owner-only mutations and freezes the inputs required by eligible system work.
3. A serial path or dependency graph evaluates those inputs; workers write only exclusive ranges or task-local results.
4. GameFramework deterministically joins results and performs owner-only commit.
5. The producer transfers one owned immutable render input through the bounded frame queue.
6. The render coordinator consumes the input and performs serial frame-graph setup/compile under the canonical Renderer/RHI boundary.
7. Dependency-independent recording groups may run on workers through exclusive RHI recording leases.
8. The render coordinator collects results in compiled order, submits, and presents.
9. Completion tokens retire or release storage whose lifetime crossed CPU/GPU execution.

CPU task concurrency, producer/render pipelining, command recording, GPU queue overlap, and frames in flight are distinct mechanisms. Each needs its own ownership proof and measurement.

## `SparkleTasks` Execution Contract

### Public Concepts

- `TaskDesc` describes work and scheduling policy.
- `TaskNodeHandle` identifies topology nodes.
- `CompiledTaskGraph` is immutable reusable topology.
- `TaskExecution` holds the counters, cancellation, failure, and result state for one run.
- `TaskExecutionContext` exposes transient run state, not domain services.
- `TaskExecutor` owns the fixed worker runtime.
- `TaskScope` binds asynchronous work to owner lifetime and settles before destruction.
- `TaskEvent` is a host/task completion primitive, not a callback bus.
- `ParallelFor` partitions a bounded range only above a measured serial threshold.
- `TaskLane` expresses policy; graph dependencies express correctness.

### Execution

- Graph compilation validates topology, dependency counts, cycles, bounds, and stable node identity.
- One execution allocates bounded state and publishes dependency-ready work to fixed workers.
- Completing a task releases dependents; fan-in becomes ready only after every prerequisite settles.
- Finalization publishes success, failure, or cancellation exactly once.
- Owner destruction stops publication, cancels or closes owned scopes, settles them, and then destroys captured state.
- Blocking I/O/process work uses the configured lane or existing narrow platform operation.
- Third-party worker counts participate in the oversubscription budget.

The reference execution uses the final graph and data contract serially. Parallel execution must preserve its output and terminal-state semantics.

## Owner-Thread Commit and Publication

### GameFramework Work

The [GameFramework and ECS standard](../../Engineering/Standards/GameFrameworkAndEcs.md) owns world storage, identities, systems, and extraction. When that work runs concurrently, this architecture adds only these constraints:

- the owner freezes the input epoch before dispatch;
- declared access and prerequisites determine which tasks may overlap;
- workers cannot structurally mutate the world or publish directly to consumers;
- task-local changes merge by a stable key and commit on the owner;
- views and references cannot outlive the frozen epoch.

### Render-Input Transfer

The producer publishes an owned immutable frame product with stable identity and generation/sequence information. `RenderFrameQueue` has an explicit capacity and a documented full policy: backpressure, replacement, or rejection. It never grows without a bound.

The render coordinator rejects stale input according to the owning domain contract. The Renderer never dereferences live world or editor storage across the boundary.

### Editor and Tool Work

The [Editor and Tools standard](../../Engineering/Standards/EditorAndTools.md) owns UI, import, cooking, capture, and tool behavior. Their background work follows the same threading shape:

- the owner creates an immutable request and a scope;
- workers produce bounded progress and an immutable result;
- the owner accepts or rejects the result by stable identity/generation;
- close cancels and settles the scope before model or application destruction;
- workers do not invoke UI callbacks or create private worker pools.

## Render Recording and Submission

The frame graph remains the sole authority for pass order, resource dependencies, barriers, queue ownership, and recording eligibility. Its compiled output may form dependency-independent recording groups when they are large enough to amortize scheduling and allocator cost.

Each recording task receives one move-only `RhiCommandRecordingLease` backed by an exclusive D3D12 allocator/list or Vulkan command pool/buffer and preassigned transient storage. A recording worker may encode its compiled group. It may not create or destroy global resources, mutate shared caches, submit, present, or wait.

The render coordinator joins completed groups in compiled order, applies the compiled cross-group and queue dependencies, submits, and presents. Worker completion order cannot change render order.

Additional GPU queue or frame-pipeline overlap is a separate concurrency decision. It is retained only when correlated timelines show a benefit after synchronization, ownership-transfer, bandwidth, pacing, and input-latency cost.

## Cancellation, Failure, and Shutdown

Every asynchronous operation has a named owner and exactly-once transition to success, failure, or cancellation. Cancellation stops unnecessary publication or work from starting; it never abandons captured state.

Shutdown order is:

1. stop accepting external requests and new publication;
2. close bounded queues and wake host waiters;
3. cancel owned scopes and executions;
4. let running tasks reach defined cancellation points;
5. settle results and reject late generations;
6. retire completion-token-owned work required by the active backends;
7. join the render coordinator and workers;
8. destroy captured owners, allocators, devices, and platform state.

Routine operation does not use device idle as a synchronization shortcut. Final teardown may use a documented bounded device-idle boundary when the backend contract requires it.

## Architecture Decisions

| Concern | Selected design | Rejected alternative |
| --- | --- | --- |
| CPU execution | one fixed `SparkleTasks` executor | subsystem pools, `std::async`, detached workers |
| Correctness order | explicit DAG dependencies | priority, timing, polling, or worker waits |
| Task lifetime | scopes and exactly-once settlement | raw-owner callbacks or fire-and-forget work |
| Mutable state | named owner-thread commit | broad locks around world, renderer, editor, or caches |
| Worker output | exclusive ranges or task-local results with deterministic join | concurrent mutation whose order changes results |
| Cross-thread transfer | owned immutable values, stable handles, or exclusive leases | borrowed live object graphs |
| Frame transfer | bounded queue with an explicit full policy | unbounded producer backlog |
| Render mutation | one render coordinator | shared worker mutation or permanent competing RHI thread |
| Command recording | compiled groups with exclusive backend leases | shared command contexts |
| Submission/present | render-coordinator ownership | worker submission or presentation |
| Background UI/tool work | scoped work on the shared executor | panel/tool-specific pools and callback runtimes |

## Concurrency Evidence

The [Concurrency](../../Engineering/Standards/Concurrency.md) and [Validation, Performance, and Evidence](../../Engineering/Standards/ValidationPerformanceAndEvidence.md) standards own the reusable test and measurement rules. Evidence for a retained mechanism includes, as applicable:

- serial, 1-, 2-, and N-worker equality;
- randomized completion and deterministic fan-in;
- cancellation, failure, owner-destruction, and shutdown stress;
- bounded-queue full-policy and memory-ceiling tests;
- wrong-thread, expired-epoch, and recording-lease misuse rejection;
- delayed completion-token retirement;
- tiny-work crossover and representative critical-path measurements;
- separate CPU-task, render-pipeline, command-recording, and GPU-queue timelines.

Backend or feature validation belongs to its domain owner. It is required here only where a concurrency mechanism crosses that backend or feature path.

## Current-State Use

Before implementing or resuming a stage:

1. inspect current code and tests;
2. inventory thread owners, queues, waits, atomics, locks, scopes, and captured lifetimes;
3. distinguish already-implemented behavior from target behavior;
4. use this document only for concurrency topology and decisions;
5. use the Roadmap for broader sequencing; no separate K implementation plan remains;
6. report current evidence rather than historical completion text.

The target is complete only when current code and tests prove these concurrency contracts. This document is the decision map, not the proof.

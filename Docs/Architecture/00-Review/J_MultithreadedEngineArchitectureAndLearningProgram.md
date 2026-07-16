# J. Multithreaded Engine Architecture and Learning Program

Status: proposed target architecture and implementation program
Date: 2026-07-16
Scope: runtime, renderer, RHI, editor, asset and shader tools, learning evidence, and portfolio presentation
Builds on: [D. Whole Repository Architecture Map](D_WholeRepositoryArchitectureMap.md), [E. External Renderer Repository Comparison](E_ExternalRendererRepositoryComparison.md), [G. Advanced Graphics Engine Executive Summary](G_AdvancedGraphicsEngineExecutiveSummary.md), [H. Advanced Graphics Engineer Persona](H_AdvancedGraphicsEngineerPersona.md), and [I. GameFramework / Renderer / RHI Responsibility Executive Summary](I_GameFrameworkRendererRhiResponsibilityExecutiveSummary.md)

## Executive Decision

Sparkle should introduce multithreading as an engine-wide ownership and data-flow redesign, not as a collection of isolated thread launches.

The target is:

- a small engine-owned task runtime used by the runtime, renderer, editor, and offline tools
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
- frame-graph dependency and resource-state reasoning
- editor and hot-reload integration rather than a renderer-only demo
- offline content-pipeline parallelism and deterministic publication
- profiling, testing, debugging modes, and honest performance reporting

The work should remain renderer-first. The existing GameFramework → Renderer → RHI responsibility direction is sound. A new Tasks foundation module is not a fourth rendering abstraction layer; it is shared execution infrastructure below the systems that use it.

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

## External Repository Study

All GitHub source observations below are tied to the listed commit rather than a moving branch. The goal is to learn mechanisms and constraints, not clone any one engine’s architecture.

### Source Matrix

| Source | Inspected revision | Relevant mechanisms | Sparkle lesson |
|---|---|---|---|
| NVIDIA Donut | bc1ea24b0486f1c00d89327fe16c0b4dd11c5937 | Small FIFO thread pool used for asynchronous scene/texture loading | Useful background-content pattern; insufficient for a renderer dependency graph |
| NVIDIA NVRHI | 8e8c36e37558acec333204619b95d9d2fcdc4a79 | Concurrent command-list recording and explicit state assumptions across lists | Parallel recording requires explicit entry/exit or permanent resource states |
| AMD FidelityFX SDK Cauldron2 | 60f4ea81909200d8542eca14dccb2628b763a9a3 | Task manager used mainly for content loading; fan-in completion callback | Continuations are useful; a loading pool is not a general frame scheduler |
| AMD Cauldron (legacy) | b92d559bd083f44df9f8f42a6ad149c1584ae94c | Simple global FIFO worker pool | Good historical sample, not the target scheduler architecture |
| Epic Unreal Engine documentation | UE 5.8 documentation, accessed 2026-07-16 | Tasks DAG, game/render/RHI separation, render proxies, RDG parallel execute | Strong ownership and dependency model; do not add a separate RHI thread without need |
| O3DE AzCore + Atom | 68683f23fb747380d3efa2424bd5f30242e9c5a2 | Retained task graph, task executor, jobified frame scheduler, thread-local command pools | Closest open-source structural reference for scheduler-to-renderer integration |
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

Direct Unreal source is access-controlled. This document therefore treats the official public documentation as the inspected authority and does not imply an audit of private engine source.

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
      TaskSchedulerConfig.h
      TaskDiagnostics.h
    Private/
      TaskRecord.h
      TaskPool.*
      Worker.*
      WorkStealingDeque.*
      InjectionQueue.*
      TaskExecutor.*
      TaskGraph.*
      TaskDiagnostics.*
    Tests/

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

### Thread Roles

| Role | Owns | May access | Must not do |
|---|---|---|---|
| Main/game thread | Window pump, input, GameScene mutation, level/editor commands, packet production | Immutable asset registry, published renderer diagnostics | Touch mutable renderer/RHI state; block for routine render completion |
| Editor main thread | ImGui/UI model, editor transactions, viewport requests | Game state at tick boundary, immutable renderer snapshots | Give live ImGui or editor pointers to render thread |
| Render coordinator | RendererSystemRoot, FramePipeline coordination, RenderWorld, GPU caches, RHI creation/destruction, submit/present | Published frame packets and task results | Dereference GameScene; execute blocking tool/file work |
| Frame workers | Data-pure game/render tasks and command recording with leased local contexts | Immutable inputs, disjoint output slices, concurrent task runtime | Wait on other tasks, submit queues, mutate global caches, destroy RHI resources |
| Background/IO workers | File reads, child processes, imports, compression/compilation where safe | Explicit task input and output publication objects | Consume latency-sensitive frame-worker capacity |
| GPU queues | Execute compiled command streams | RHI resources covered by barriers/fences | Be described as CPU task concurrency |

Each engine-created thread must have a stable name visible in PIX, RenderDoc/Nsight CPU tracks, Visual Studio, and Tracy or the eventual CPU profiler. The initial names can be Sparkle.Main, Sparkle.Render, Sparkle.Frame.0…N, and Sparkle.Background.0…M.

### Frame Pipeline

The normal mode is one CPU frame of game/render decoupling:

~~~text
time ───────────────────────────────────────────────────────────────►

Main/game       Update N+1 ── publish packet N+1 ── Update N+2
                         │
Render thread   Prepare N ─ task graph ─ record N ─ submit N
                                                   │
GPU             Execute N-1 ─────────────────────── Execute N
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

## SparkleTasks Design

### Why an Engine-Owned Task Runtime

An engine-owned implementation is justified here because it is part of the learning and portfolio goal, it needs integration with render thread roles and diagnostics, and it will be reused by tools. It should still be deliberately bounded. The engine does not need to invent fibers, a coroutine runtime, a distributed build system, or a lock-free scheduler research project.

The first production-capable version should provide:

- a fixed worker set configured once per host
- local worker queues plus stealing
- a thread-safe external injection path
- prerequisites and continuation scheduling
- fan-out and fan-in
- nested task creation without waiting
- parallel_for with grain-size and serial threshold
- cancellation and explicit failure reporting
- foreground/frame and background/blocking lanes
- deterministic serial execution
- names, priorities, timing, queue depth, and dependency diagnostics
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

Do not count the OS, shader compiler’s own internal threads, D3D/Vulkan driver threads, compression-library threads, and child processes as free capacity. Thread counts must be configurable and reported.

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

The task diagnostic stream should record:

- task name/category/lane
- enqueue, start, and finish timestamps
- worker ID
- prerequisite count
- parent/run ID
- cancelled/failed/succeeded status
- queue delay
- execution duration

This is both a debugging tool and portfolio evidence.

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
    FramePacketDiagnostics diagnostics;
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
- immutable diagnostic snapshot read
- explicit boundary wait used only by shutdown, resize policy, or tests

Example render commands:

- ResizeSwapchain
- ReplaceShaderPackageGeneration
- ChangeRenderFeatureSettings
- BeginSceneGeneration
- EndSceneGeneration
- RequestCapture
- RequestDeviceDiagnostics
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

Compile topology should be cached by a FrameGraphKey covering resolution classes, feature configuration, view count, backend capabilities, and debug/capture modes. Resource sizes and imported handles can be rebound per frame. This reduces serial compile cost before attempting to parallelize it.

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
- a coordinator-owned overflow and diagnostics path
- GPU-token-based page retirement

Report:

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

## GameFramework Integration

### First Principle

GameFramework parallelism is not required to ship the initial render thread, but the packet boundary must allow it. GameScene remains main-thread owned while tasks operate on immutable component snapshots or explicitly partitioned arrays.

Candidate later jobs:

- animation evaluation per independent skeleton/instance
- controller phases where dependencies are declared
- skinning matrix generation
- morph weight evaluation
- bounds updates
- transform hierarchy subtrees after parent dependencies
- render packet extraction by archetype/component array

### Phase Graph

An eventual update graph may be:

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

The editor sends versioned commands and packet data. It reads immutable RenderDiagnosticsSnapshot instances published back through a small double/triple-buffered channel.

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

## Tools and Content Pipeline

### One Runtime, Different Host Policy

Offline tools should link SparkleTasks but configure it for throughput:

- no render coordinator
- adopted main thread may help execute compute tasks
- most available cores assigned to compute/background lane
- small blocking-I/O lane
- stable deterministic publication order
- progress events marshalled to one console/UI owner

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

## Architecture Decision Record

### Decision Matrix

| Question | Chosen direction | Rejected/deferred alternative | Reason |
|---|---|---|---|
| General concurrency primitive | Engine-owned bounded task runtime | More std::async calls | No prerequisites, lanes, diagnostics, worker control, or consistent shutdown |
| Scheduler model | Work stealing plus explicit task DAG | Only a global FIFO pool | Frame workloads are nested and irregular; a FIFO becomes a contention/load-balance limit |
| Worker waiting | Dependencies and continuations | Busy-yield or routine worker Wait | Avoids wasted CPU and pool deadlock; makes critical path visible |
| Blocking operations | Separate limited I/O lane | Run on frame workers | Prevents pipe/file waits from starving a frame |
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
| Engine-owned task runtime | Deep learning, unified diagnostics, exact lane/render integration | Scheduler maintenance and subtle concurrency correctness burden |
| Work stealing | Adapts to irregular animation/culling/recording workloads | Execution order is nondeterministic; deque and shutdown logic are harder |
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
- benchmark and test it against a reputable library later
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

### Stage 0 — Invariants, Baseline, and Vocabulary

Goals:

- freeze current visual/performance baselines
- add thread-role assertions and FrameId/SceneGeneration/SequenceNumber types
- document which thread owns existing systems
- add runtime toggles that future stages will preserve

Code areas:

- Engine/Application
- Engine/Renderer/Public/Renderer.*
- Engine/Renderer/Private/RendererSystemRoot.*
- Engine/RHI diagnostics
- benchmark/test launch scripts

Work:

1. Add CPU timing zones for game update, snapshot capture, scene build, GPU-data build, graph setup, compile, record, submit, present, and idle waits.
2. Capture p50/p95 CPU timings and GPU timings for representative scenes on both backends.
3. Record upload bytes, structured buffer allocations, pass count, command-list count, and queue waits.
4. Add owner-thread assertions to current renderer/RHI mutators.
5. Define toggles:
   - tasks worker count
   - threaded renderer
   - parallel recording
   - pipeline depth
   - deterministic serial scheduler
6. Save reference images and deterministic scene/camera scripts.

Exit criteria:

- baseline data is checked into documentation/artifacts policy
- both D3D12 and Vulkan validation runs are clean
- every important WaitForIdle site is classified as shutdown, boundary, or data-path debt

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

Delete:

- the ShaderRecookCoordinator std::future/std::async execution path
- any temporary second ad-hoc pool introduced during the pilot

Exit criteria:

- ThreadSanitizer-capable platform or equivalent stress/diagnostic runs pass
- outputs match serial byte-for-byte where formats are deterministic
- worker counts 0/1/2/N work
- failed/cancelled cooks never publish partial registries
- interactive shader recook cannot starve the frame lane

Learning evidence:

- scheduler design note and dependency trace
- throughput scaling table and grain/concurrency discussion
- failure/cancellation demonstration

### Stage 2 — Render Data Contract

Goals:

- create the real game/editor-to-render ownership boundary
- remove all raw game-object/mesh pointers from render packets
- separate structural deltas from dynamic frame data

Code areas:

- Engine/GameFramework scene and snapshot extraction
- Engine/Renderer/Public scene handoff types
- Engine/Renderer/Private/SceneData
- Asset handle/registry types
- level lifecycle code

Work:

1. Add RenderObjectId and stable immutable asset handles.
2. Build RenderWorldDelta and RenderFrameDynamicData in packet-local arenas.
3. Create RenderWorld/RenderProxyRegistry and apply sequence-checked deltas.
4. Move previous transform/skinning identity from vector positions to stable IDs.
5. Publish level begin/end generations as data.
6. Keep Renderer serial while consuming the new packet contract.

Delete:

- MeshInstanceSnapshot::mesh raw pointer
- RendererSystemRoot reads of GameScene
- renderer cache invalidation through direct GameFramework event callbacks
- the old full GameSceneSnapshot-to-renderer boundary after parity

Exit criteria:

- renderer can consume a recorded/replayed packet stream without GameScene existing
- level unload/reload stress produces no stale-handle access
- serial visual output matches baseline
- packet memory and build time are measured

Learning evidence:

- ownership/lifetime state diagram
- acquire/release publication test
- stale generation rejection demo

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
5. Publish immutable diagnostic snapshots back to editor.
6. Implement threaded one-ahead, threaded zero-ahead, and serial-consumer modes.

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
- task diagnostics/profiler integration

Work:

1. Split monolithic builders into pure task functions with explicit inputs/outputs.
2. Parallelize transforms/bounds, visibility, lighting, skinning, mesh classification, and RT instance planning at coarse measured grains.
3. Merge into stable sort/batch order.
4. Keep graph setup/compile serial initially and cache topology by key.
5. Add serial thresholds and workload-size telemetry.

Delete:

- monolithic mutable builder entry points once all call sites use graph outputs
- shared scratch vectors in renderer system objects

Exit criteria:

- outputs match serial mode
- no task mutates renderer global caches
- speedup is positive on representative large scenes and does not regress small scenes beyond budget
- critical-path and worker-utilization traces are readable

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
2. Add render diagnostic snapshots for tasks, frame graph, GPU scene, uploads, and retirement.
3. Finish deterministic transactional cook publication.
4. Add cancellation/progress UI.
5. Audit every remaining thread, mutex, future, and WaitForIdle.
6. Remove legacy host-render and snapshot paths.

Exit criteria:

- no dual architecture remains
- all background operations use declared lanes
- all mutable cross-thread objects have documented ownership
- automated stress suite runs repeatedly without deadlock, leak, race symptom, or validation error

### Stage 8 — Performance and Portfolio Release

Goals:

- tune only with representative evidence
- present the design and its limits professionally

Work:

1. Tune worker count, task grain, recording groups, upload page sizes, and pipeline depth.
2. Capture CPU/GPU timelines and p50/p95 data across backends and core counts.
3. Write a concise public architecture overview linked to this detailed study.
4. Record a deterministic demo and source walkthrough.
5. Publish limitations and future work.

Exit criteria:

- results are reproducible from documented commands/settings
- gains are reported per subsystem, not as one unexplained FPS number
- correctness modes and backend parity are visible

## Recommended Change Sets

The stages can be organized into reviewable change sets:

1. Task runtime core and tests
2. Task diagnostics, serial mode, and lanes
3. Texture/shader cooker pilots
4. Stable render IDs and immutable asset handles
5. Render world delta and dynamic packet
6. Serial renderer conversion to new data contract
7. Bounded mailbox and render coordinator
8. Editor UI/viewport packet conversion
9. Persistent GPU scene
10. Deferred scene/shader generation retirement
11. Preparation DAG
12. D3D12 worker recording contexts
13. Vulkan worker recording contexts
14. Frame-graph recording groups and first parallel passes
15. Draw-heavy intra-pass parallelism
16. Reliability, metrics, and legacy deletion

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

Performance:

- enqueue/execute overhead
- local versus stolen task cost
- idle wake latency
- parallel_for crossover grain
- task-record pool high-water mark

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

### Concurrency Tooling

Use complementary tools:

- compiler/thread sanitizers where the platform/toolchain supports them
- Visual Studio concurrency and CPU profiling
- PIX CPU/GPU events for D3D12
- RenderDoc and Vulkan validation/synchronization validation
- Nsight Graphics/Systems where useful
- Application Verifier, guard allocators, and intentional packet poisoning
- a CPU task timeline in the engine profiler or Tracy integration

Native graphics validation cannot find a C++ data race. Thread sanitization cannot validate GPU barriers. Both are required.

## Performance Evaluation

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

### The Story

Present the work as five connected problems:

1. The old frame was serial and the scene snapshot was not a true ownership boundary.
2. Sparkle introduced a tested task DAG and bounded execution lanes.
3. Game/editor state became immutable packets, deltas, stable IDs, and render proxies.
4. The render thread, persistent GPU scene, and frame graph exposed safe parallel work.
5. D3D12/Vulkan worker-local command contexts enabled parallel recording with serial equivalence and measured results.

### Artifacts

Include:

- one architecture diagram
- one ownership table
- one before/after CPU/GPU timeline
- one preparation DAG capture
- D3D12 and Vulkan command-context diagrams
- serial/parallel toggles in the demo
- validation and stress-test output
- benchmark tables with machine/core/backend/configuration
- short sections on failed ideas and tradeoffs
- pinned primary-source study links

### Live Demonstration

A strong demo sequence:

1. run serial reference mode and show the CPU trace
2. enable render thread and show game/render overlap
3. enable preparation tasks and show the DAG/worker utilization
4. enable parallel command recording and show ordered multi-list submission
5. switch D3D12/Vulkan
6. trigger shader recook/reload without device idle
7. reload a level with frames in flight
8. change worker count and show stable output
9. show a deliberately invalid pass rejected by parallel-safety assertions

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
| Cross-thread lifetime bug | Rare crash on unload/reload | Stable handles, packet poison, generations, retirement stress |
| One-frame latency is unacceptable | Input feels delayed | Bounded depth, zero-ahead mode, measure input-to-present |
| Parallel tasks increase small-scene time | More overhead than work | Serial thresholds and grouping |
| Driver dislikes many command lists | CPU/GPU regression | Measured group size; backend policy; serial fallback |
| PSO/shader cache races | Hitches/crashes in record tasks | Prewarm immutable runtime view |
| Upload/descriptor contention | Workers serialize on allocator | Local pages and high-water diagnostics |
| Tool memory explosion | OOM on HDR/scene batch | Weighted concurrency budgets |
| Nondeterministic output | Flaky tests/cache misses | Stable IDs/sorts/merge/publication |
| Oversubscription | Low utilization and high latency | Lane budgets, report external/internal threads |
| Render thread waits on workers too long | Critical path unchanged | DAG profiling, load balance, larger task grains |
| Vulkan/D3D behavior diverges | Backend-only bugs | Common recording contract, native validation, paired milestones |
| Legacy path remains indefinitely | Two architectures drift | Explicit deletion gates |

## Definition of Done

This program is complete when all of the following are true:

### Architecture

- GameScene and editor mutable state are never dereferenced by the render thread.
- RenderFramePacket contains only packet-owned values or stable immutable handles.
- RendererSystemRoot and mutable RHI state have one render-coordinator owner.
- The packet queue is bounded and supports serial, zero-ahead, and one-ahead modes.
- Scene structure uses sequenced deltas and stable generational IDs.
- The GPU scene persists static state and updates dirty/dynamic ranges.

### Tasks

- SparkleTasks supports prerequisites, fan-in, nested work, cancellation, failure, lanes, diagnostics, and serial execution.
- Normal worker tasks do not block on other tasks.
- Frame and blocking-I/O workloads cannot starve each other.
- Scheduler tests pass at 1/2/N workers and during repeated shutdown stress.

### Rendering

- Frame preparation contains measured task-parallel work.
- Frame-graph recording groups have explicit state and deterministic submission contracts.
- D3D12 allocators/lists and Vulkan command pools/buffers are worker-local and frame-retired.
- Pass runtime/PSO state is prewarmed before parallel recording.
- Upload and transient descriptor allocations have parallel-safe ownership.
- Both backends pass native and engine validation in serial and parallel modes.

### Editor and Tools

- ImGui and viewport data cross through owned/versioned packets.
- Shader reload and level changes do not routinely wait for device idle.
- shader, texture, and asset cook paths use task graphs where safe.
- tool publication is transactional and deterministic.
- cancellation and progress are observable and safe.

### Evidence

- before/after profiler captures and benchmark tables exist
- correctness parity and stress results are reproducible
- CPU task concurrency, render pipelining, GPU queue concurrency, and frames in flight are explained separately
- limitations and tradeoffs are documented
- all replaced legacy paths are removed

## Immediate Next Implementation Slice

The first code change after accepting this design should be Stage 0 plus the smallest part of Stage 1:

1. add thread-role/FrameId/SceneGeneration vocabulary and CPU timing points
2. record current D3D12/Vulkan baselines
3. scaffold SparkleTasks with serial executor and dependency graph tests
4. add the fixed worker executor, cancellation, and diagnostics
5. replace ShaderRecookCoordinator’s std::async as the first integration

Do not start by moving RendererSystemRoot to a new thread. The task runtime and cross-thread data contract need independent tests before renderer lifetime becomes concurrent.

## Local Repository Audit Trail

These are the principal Sparkle sources inspected for this study. They should be revisited at the corresponding migration gate.

| Source | What it establishes |
|---|---|
| [RuntimeApplication.cpp](../../../Engine/Application/Private/RuntimeApplication.cpp) | GameScene update and Renderer::OnRender are sequential on the application thread |
| [EditorApplication.cpp](../../../Engine/Application/Private/EditorApplication.cpp) | Renderer prepare/record, editor UI, and submit are one ordered host-thread path |
| [GameScene.cpp](../../../Engine/GameFramework/Private/Scene/GameScene.cpp) | Controller, animation, morph, and snapshot phases are currently serial |
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

### Microsoft

- [DirectX 12 Multithreading sample overview at 357ade6](https://github.com/microsoft/DirectX-Graphics-Samples/blob/357ade6ec6ff0d9dcadc48f35c7a28e37c0cdf7a/Samples/Desktop/D3D12Multithreading/readme.md)
- [D3D12Multithreading FrameResource at 357ade6](https://github.com/microsoft/DirectX-Graphics-Samples/blob/357ade6ec6ff0d9dcadc48f35c7a28e37c0cdf7a/Samples/Desktop/D3D12Multithreading/src/FrameResource.cpp)

## Final Recommendation

Approve the architecture as one program with three non-negotiable ordering rules:

1. ownership and packet lifetime before the render thread
2. persistent data before parallelizing scene rebuild work
3. native per-worker recording ownership and pass audits before parallel frame-graph execute

Sparkle already has the rendering abstractions and explicit GPU scheduling needed to make this credible. The pivotal step is to turn implicit serial ordering into explicit ownership, generations, dependencies, and retirement—and then show, with serial equivalence and measurements, that the resulting concurrency is both correct and useful.

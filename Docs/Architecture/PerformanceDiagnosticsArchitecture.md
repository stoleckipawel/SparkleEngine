# Performance Diagnostics Architecture

Status: target proposal; not proof of current implementation

Last source reconciliation: 2026-08-11

Scope: editor and game frame timing, CPU owner/thread attribution, GPU queue/pass timing, process RAM, GPU memory, bounded live presentation, benchmark evidence, and external-profiler correlation

## Purpose And Authority Boundary

This document owns the target system design for answering four questions:

1. Is the observed frame limited by host/game/editor CPU work, render CPU work, GPU work, presentation, or an unresolved interaction?
2. What are Sparkle's explicitly owned threads and task lanes doing?
3. How much process RAM and GPU memory is used, allocated, budgeted, resident, or awaiting retirement?
4. Which external capture should be taken next to establish cause?

It does not claim that the target is implemented, set performance targets for a particular scene, or replace a profiler.

- Current implementation truth remains in code and executable configuration.
- [Validation, Performance, and Evidence](../Engineering/Standards/ValidationPerformanceAndEvidence.md) owns measurement and claim rules.
- [Concurrency](../Engineering/Standards/Concurrency.md) owns thread, task, wait, and publication rules.
- [Graphics Engineering](../Engineering/Standards/GraphicsEngineering.md) owns graphics profiling and hardware-specific evidence rules.
- [Editor and Tools](../Engineering/Standards/EditorAndTools.md) owns editor presentation and cross-thread UI products.
- [J. Multithreaded Engine Architecture](Multithreading/MultithreadedEngineArchitecture.md) owns the target concurrency topology and frame concurrency lifecycle.
- [Renderer and RHI Architecture Boundary](RendererRhiBoundary.md) owns renderer, frame-graph, and RHI authority.
- [I. Acceptance Workloads](../Engineering/BistroAndSanMiguelWorkloads.md) owns `MAP-00`, scene routes, warm-up/sample policy, and portfolio gates.
- [A. Principal Graphics Engineering Requirements](../Strategy/Requirements.md) owns the `PGE-05`, `PGE-06`, `PGE-10`, and reviewer-evidence meaning advanced by this system.

The product owner for the live view is Editor. Application owns cross-domain session orchestration and benchmark export. Each engine domain remains the authority for its own measurements. This current product ownership is what justifies a compact performance panel; it does not justify a general task browser, allocation explorer, or trace-viewer product.

## Executive Decision

Sparkle will provide three complementary diagnostic layers:

| Layer | Answers | Product shape |
| --- | --- | --- |
| Live orientation | Which domain is consuming the frame budget right now? Is memory growing or under pressure? | A compact viewport summary plus one bounded Editor performance window. |
| Reproducible measurement | What are the distributions and high-water marks for a declared run? | An explicit `MAP-00`/benchmark session with a manifest and raw samples. No default report files. |
| Causal investigation | Why is a CPU stage, GPU pass, wait, allocation path, or queue expensive? | PIX, RenderDoc, Nsight Graphics, Windows Performance Recorder/Analyzer, and hardware-appropriate vendor tools using Sparkle's stable names and markers. |

The live layer is a compass, not a verdict. It must expose validity, frame identity, configuration, and likely limiting domain without pretending that utilization, FPS, summed scope time, or a single captured frame proves cause.

## Goals And Non-Goals

### Goals

- Separate application frame interval, editor work, gameplay/world work, render-thread CPU work, task-worker work, GPU work, presentation waits, process RAM, and GPU memory.
- Preserve the difference between a physical OS thread and a logical phase running on that thread.
- Correlate delayed and pipelined CPU/GPU samples with the existing `FrameId`.
- Show current values, stable distributions, high-water marks, and data age with explicit units.
- Keep all queues, buffers, label sets, and exports bounded.
- Reuse the existing thread names, `SparkleTasks` ETW events, frame-graph GPU scopes, RHI timestamps, and allocator diagnostics.
- Make D3D12 and Vulkan measurements semantically comparable while exposing backend limitations.
- Make the next profiler action obvious and produce evidence a portfolio reviewer can audit.
- Measure the observer cost of the diagnostics themselves.

### Non-Goals

- A home-grown sampling profiler, flame graph, GPU counter suite, allocation call-stack recorder, or trace viewer.
- A second task runtime, task history browser, per-subsystem thread pool, or diagnostics event bus.
- Per-entity, per-resource, per-draw, or arbitrary string instrumentation in the live path.
- Inferring GPU milliseconds from GPU utilization.
- Calling a sum of overlapping/nested GPU scopes "GPU frame time."
- Calling aggregate CPU usage "game-thread cost."
- Treating Editor gameplay execution as a separate OS game thread when it is not one.
- Always-on detailed GPU timestamps or default disk reports in normal product runs.
- Optimizing Sponza before a reproducible baseline identifies its limiting path.

## Current Source-Backed Starting Point

The following is an observation of the 2026-08-11 worktree, not a completion claim:

| Surface | Current behavior | Consequence for the target |
| --- | --- | --- |
| Viewport statistic | `ViewportTopPanel` shows ImGui FPS and the latest ImGui delta. | It cannot separate host, gameplay, render CPU, GPU, presentation, or memory cost. |
| Host clock | `Timer` records unscaled tick-to-tick delta and a monotonically increasing frame count. | This is a useful starting identity/interval, but the benchmark definition must explicitly be unscaled application begin-to-begin time. |
| Thread names | Editor, Game, Render, Tool main, and every task lane worker call `SetCurrentThreadRole`; Windows receives the same thread description. | External CPU tools can already distinguish Sparkle-owned OS threads. |
| Task profiling | `TaskProfiler` emits ETW `TaskDependency`, `TaskBegin`, and `TaskEnd` events under the `SparkleTasks` provider. | WPA can inspect named task work; live UI should not duplicate a full task trace viewer. |
| Render topology | Threaded rendering defaults on with pipeline depth 1. A serial renderer is also supported. | Samples from simultaneous wall-clock columns may refer to different `FrameId` values. Mode and depth must always be visible. |
| Editor/game ownership | `EditorApplication::Tick` calls `RuntimeApplication::UpdateRuntime` on `Sparkle.EditorThread`. Standalone runtime calls it on `Sparkle.GameThread`. | "Gameplay" is a logical phase on the current host owner, not a permanent editor-side OS thread. |
| GPU markers | Frame-graph passes and selected detailed operations emit backend GPU diagnostic scopes. | Existing stable pass labels should be the profiler correlation vocabulary. |
| GPU timestamps | D3D12 and Vulkan timestamp queries resolve into private `ResolvedGpuTiming` values when a frame slot retires; `r.Diagnostics.GpuTiming` defaults off. | The renderer needs a bounded immutable frame result; Editor must never read renderer-private state. |
| GPU memory | D3D12MA/VMA-backed snapshots track used/allocated bytes, categories, API usage/budget, transient allocation, and Vulkan delayed destruction. | The foundation is useful, but device-local and non-local heaps must be presented separately before anything is labeled "VRAM." |
| Memory polling identity | `FramePipeline` currently passes the wrapping RHI frame-in-flight slot index to `RendererMemoryMonitor`, whose interval logic expects a monotonic value. | Periodic refresh cannot be treated as reliable until polling uses logical `FrameId` or monotonic time and is covered by a wrap test. |
| RAM | No production process-memory sampler exists in the engine. | Working set and private committed bytes are required; engine CPU allocation categories are a later measured need, not an initial fiction. |
| Editor memory route | A renderer memory provider reaches `UI`, but no current editor panel consumes it. | The target should replace this broad/synchronous presentation route with one immutable diagnostics model published by Application. |

Primary code landmarks for revalidation:

- `Engine/Application/Private/RuntimeApplication.cpp`
- `Engine/Application/Private/EditorApplication.cpp`
- `Engine/Core/Private/Time/Timer.cpp`
- `Engine/Core/Private/Threading/ThreadOwnership.cpp`
- `Engine/Tasks/Private/Profiling/TaskProfiler.cpp`
- `Engine/Renderer/Private/Concurrency/Coordinator/RenderCoordinator.cpp`
- `Engine/Renderer/Private/FramePipeline/FramePipeline.cpp`
- `Engine/Renderer/Private/Diagnostics/FrameExecutionDiagnostics.cpp`
- `Engine/Renderer/Private/Diagnostics/RendererMemoryMonitor.cpp`
- `Engine/RHI/Public/Memory/RhiMemoryDiagnostics.h`
- `Engine/Editor/Private/Panels/ViewportTopPanel.cpp`

## Measurement Vocabulary

Every visible metric must have one owner, unit, interval, correlation identity, validity state, and sampling mode. A blank or `N/A` value is correct when the engine cannot establish the measurement.

### Clock And Identity Rules

`FrameId` remains the shared logical correlation identity. Diagnostics may add a benchmark `RunId`, sample-window identity, OS thread ID, queue type, and `RhiSubmissionToken`, but none becomes a second frame authority.

CPU clocks use one monotonic source. GPU durations use backend timestamp periods. CPU and GPU absolute timestamps are not placed on one time axis unless the backend provides and the implementation validates clock calibration. Until then:

- CPU phases correlate to `FrameId` and use CPU-relative time;
- GPU scopes correlate to `FrameId` and use per-queue GPU-relative time;
- submit-to-GPU and input-to-present latency remain unavailable rather than estimated;
- multiple GPU queue spans remain separate unless calibrated overlap is proven.

### Required Top-Level Definitions

| Display name | Definition | Includes | Excludes / caution |
| --- | --- | --- | --- |
| Frame interval | Unscaled application begin-to-begin wall duration for the host frame. | Active host work, blocking/backpressure, presentation effects that delay the next begin, and scheduling delay. | Not "CPU execution time"; one thread may sleep for much of it. |
| Host phase wall | Non-overlapping instrumented wall duration on EditorThread or GameThread for that `FrameId`. | Input/host, editor, gameplay, extraction, UI packet build, render enqueue, and waits inside those phases. | Not sampled CPU execution time; does not include worker execution and shows uninstrumented time separately. |
| Gameplay wall | Caller duration around world edit application and game-system execution. | Caller waits for the system DAG to settle. | Do not add task-worker CPU duration to it; that double counts parallel work. |
| Render CPU wall | Render owner duration from consuming a frame packet through render-frame retirement. | Renderer preparation, frame graph, recording coordination, submission, present, and classified waits. | In serial mode it runs on the host thread and must be labeled `Inline`, not RenderThread. |
| Render queue wait | Time the producer blocks for a reusable frame slot or the render consumer waits for input. | Backpressure or starvation at the bounded frame queue. | A symptom; inspect the downstream/upstream critical path before assigning cause. |
| Worker occupancy | Task-body wall duration and ready-to-start delay by named `SparkleTasks` lane for the selected interval. | FrameCritical, Background, and BlockingIo task work. | Not OS CPU running time. Lane is policy, not subsystem ownership; run/ready/wait causality belongs in ETW/PIX. |
| GPU graphics span | Earliest begin to latest end of the outer valid graphics-queue frame scope for one `FrameId`. | Queue execution enclosed by the frame scope. | Does not include CPU queueing, present scan-out, or work outside the scope. |
| GPU pass duration | Timestamp duration for one named pass scope on its queue. | Work enclosed by that scope. | Nested/overlapping pass durations are not additive. |
| Process working set | Current physical pages resident for the process. | Shared and private resident pages as reported by the OS. | Not ownership and not total committed memory. |
| Process private commit | Private committed virtual memory charged to the process. | Private committed pages, resident or paged out. | Not the same as current physical RAM. |
| GPU tracked used | Sum of live resource/allocation payload bytes tracked by Sparkle's GPU allocator. | Engine-owned tracked allocations. | Not driver allocations and not necessarily resident. |
| GPU allocator blocks | Bytes reserved in allocator blocks/heaps. | Internal fragmentation and unused capacity within blocks. | Not identical to resource payload or adapter usage. |
| Local API usage / budget | Backend/API report for the device-local memory segment or heaps. | Scope declared by the backend snapshot. | Must not be combined with host-upload/non-local heaps and called VRAM. |
| Retirement backlog | Bytes whose logical owner released them but whose GPU completion token has not retired them. | Deferred/delayed destruction tracked by the backend. | `N/A` when the backend cannot report it. |

### Validity States

Every domain result carries one of these states:

| State | Meaning |
| --- | --- |
| `Pending` | Frame exists but a delayed GPU or publication result is not available yet. |
| `Valid` | Source, unit, interval, and `FrameId` are known. |
| `Unavailable` | Backend, platform, build, or mode cannot provide the metric. |
| `Disabled` | The capability exists but the selected diagnostics mode does not collect it. |
| `Stale` | The last valid sample is older than its declared polling interval. |
| `Invalid` | Correlation, nesting, timestamp resolution, or sample completeness failed; exclude it from aggregation. |

The UI shows the state and sample age. It never substitutes zero for missing data.

## Current And Target Execution Topology

With threaded rendering and pipeline depth 1, work may overlap as follows:

```text
wall time  -------------------------------------------------------------->

Sparkle.EditorThread   Frame 102: host + gameplay + extract + UI + enqueue
Sparkle.RenderThread            Frame 101: prepare + graph + record + submit
Task.FrameCritical.*                 F101 recording/system tasks
GPU.Graphics                              Frame 100 pass execution
Present                                           Frame 100 presentation

correlation              FrameId, never visual column position
```

Comparing Editor frame 102 directly with GPU frame 100 as if they were the same logical frame can produce a false diagnosis. The aggregator joins results by `FrameId` and allows older records to be completed when delayed GPU timestamps resolve.

### Explicit Sparkle Thread Roles

| Thread role | Exists when | Current responsibility | Required presentation |
| --- | --- | --- | --- |
| `Sparkle.EditorThread` | Editor product | Window/input host, editor operations, gameplay update/extraction, ImGui model/build, render packet production. | One physical lane with logical phase colors; never relabel gameplay as a separate OS thread. |
| `Sparkle.GameThread` | Game product | Window/input host, gameplay update/extraction, runtime UI/console, render packet production. | One physical lane with the same gameplay phase vocabulary. |
| `Sparkle.RenderThread` | `r.ThreadedRenderer=true` | Renderer/RHI mutable ownership, frame graph, recording coordination, submission, present. | Active stages, classified waits, queue latency, current `FrameId`. |
| `Sparkle.Task.FrameCritical.N` | Parallel task runtime has workers | Dependency-ready game-system, render-preparation, or command-recording tasks. | Occupied %, ready delay, task count, and current stable task label; detailed spans in ETW. |
| `Sparkle.Task.Background.N` | Parallel task runtime has workers | Bounded asset preparation and editor/tool background CPU work. | Occupied %, ready delay, task count, and current stable task label. |
| `Sparkle.Task.BlockingIo.N` | Parallel task runtime has workers | Bounded blocking file/process operations. | Occupied %, task count, and current stable task label; OS traces distinguish running from blocked. |
| `Sparkle.ToolMain` | Standalone tools | Tool orchestration. | Outside the editor frame panel; available in external CPU traces and tool-specific evidence. |

Driver, OS, SDK, and profiler threads are not Sparkle-owned. External tools may show them by their provider names or as unlabeled external threads; Sparkle does not rename them or attribute their CPU time to an engine subsystem without call-stack evidence.

If `task.SerialExecution=true`, no task-worker lanes are invented; task work is attributed to the caller thread with task scopes still available in a trace. If `r.ThreadedRenderer=false`, render phases appear nested on EditorThread/GameThread and RenderThread is `Unavailable`.

### Stable Logical CPU Phase Vocabulary

The initial live taxonomy is deliberately small and orchestration-shaped:

```text
Application.Frame
|-- Application.BeginFrame
|   |-- Platform.Events
|   `-- Level.CommitPending
|-- Editor.Operations                    [Editor only]
|-- Gameplay.Frame
|   |-- Gameplay.ApplyEdits
|   |-- Gameplay.Systems
|   `-- Gameplay.ExtractRenderInput
|-- Editor.BuildUi                       [Editor only]
|-- Renderer.Enqueue
|   `-- Renderer.FrameQueueBackpressure
`-- Application.Unattributed

Renderer.Frame
|-- Renderer.ConsumeInput
|-- Renderer.BeginFrame
|-- Renderer.SetupFrame
|-- Renderer.PrepareFrame
|-- Renderer.FrameGraph.Setup
|-- Renderer.FrameGraph.Compile
|-- Renderer.CommandRecording
|-- Renderer.Submit
|-- Renderer.Present
|-- Renderer.Wait
`-- Renderer.Unattributed
```

These labels describe ownership and work. They do not mirror every function. A new top-level phase requires a distinct diagnostic question and current consumer. Dynamic entity names, resource paths, pointer values, and per-frame formatted labels are forbidden in this taxonomy.

Task names retain their existing bounded `TaskName`. A task trace also carries lane, graph-run generation, task index, lane worker index, outcome, and, when the submitting domain has one, `FrameId` plus a fixed owner category such as `Gameplay`, `Renderer`, `Editor`, or `Asset`.

### GPU Scope Vocabulary

GPU markers remain frame-graph/pass owned:

```text
Renderer.Frame                         [FrameId is record metadata]
|-- Queue.Graphics
|   |-- Renderer.FrameGraph/<StablePassName>
|   `-- Renderer.PresentUi
|-- Queue.Compute
`-- Queue.Copy
```

- Pass names come from compiled frame-graph diagnostic names.
- `FrameId` is scope-record metadata or a capture bookmark where the tool supports it; it is not a per-frame formatted pass label.
- Queue is explicit metadata, not encoded only in display text.
- Detailed internal scopes are capture-time diagnostics and are disabled in the basic live mode.
- Draw/dispatch/resource detail belongs in PIX, RenderDoc, Nsight, or RGP.
- The engine preserves a top-level queue span even when detailed pass timings are disabled.

## System Ownership And Data Flow

```text
Core monotonic clock + minimal CPU trace sink
        |                         SparkleTasks ETW events
        v                                  |
Application diagnostics session <---------+
  host phase sample + process RAM
        |          ^
        |          | immutable results keyed by FrameId
        |          |
        |     Renderer diagnostics producer
        |       render CPU stages
        |       frame-queue waits
        |       resolved GPU scopes
        |       RHI GPU memory
        |          ^
        |          |
        |     RHI timestamps/allocator truth
        |
        +----> bounded joined frame history
                    |
                    +----> immutable Editor performance model
                    |
                    `----> explicit benchmark exporter
```

### Owners

| Responsibility | Owner | Contract |
| --- | --- | --- |
| Monotonic CPU timestamp and minimal platform trace emission | Core diagnostics bootstrap | No domain policy, sample history, UI, or file output. |
| Task dependency/begin/end trace events | Tasks | Existing bounded task identity and lane semantics; provider work is skipped when disabled. |
| Host frame identity, diagnostics mode, process RAM polling, cross-domain join, rolling windows, explicit export | Application | One session owner; immutable model/result publication. |
| Gameplay/update/extraction wall scopes | Application around GameFramework owner calls | Initial top-level measurement needs no public ECS diagnostic snapshot. Detailed systems stay in task traces. |
| Renderer CPU stages, frame queue, GPU timing resolution | Renderer | Immutable `FrameId`-keyed performance result; no ECS or Editor types. |
| GPU timestamp, queue type, allocator, and budget facts | RHI | Backend-neutral values with backend capability and scope declared. No bottleneck policy. |
| Live presentation and user intent | Editor | Reads one immutable model and submits semantic start/stop/reset/export requests. No renderer/RHI pointers. |
| Benchmark artifacts | Application evidence session | Written only for an explicit bounded request under the acceptance-workload artifact root. |

### Publication Rules

- Producers write owner-local frame builders; there is no global mutable timer registry.
- Completed results are immutable and keyed by `FrameId`.
- GPU results may update an older pending frame after its frame-in-flight slot retires.
- The Application join never blocks EditorThread/GameThread waiting for RenderThread or GPU data.
- The Editor reads a published model no more than once per UI frame and never synchronously requests a renderer snapshot.
- A dropped or overwritten result increments a bounded loss counter and invalidates the affected aggregate; it does not grow a queue.
- Reset starts a new sample-window generation. Results from the previous generation cannot enter new aggregates.
- Shutdown stops publication, settles producers, drains or invalidates pending joins, then destroys the session.

### Diagnostics Data/Access Inventory

| Product | Producer / mutation point | Consumers | Frequency and expected cardinality | Publication, lifetime, and overflow |
| --- | --- | --- | --- | --- |
| Host frame builder | EditorThread or GameThread at fixed Application scopes | Application join; CPU trace sink | One builder for the current `FrameId`; fixed phase columns | Owner-local until end-frame publish, then immutable; missing phase closes as invalid rather than escaping the frame epoch. |
| Renderer frame result | RenderThread/inline render owner; delayed GPU resolver completes GPU fields | Application join; external marker correlation | At most the configured frame-pipeline capacity plus delayed retirements | Fixed mailbox capacity; oldest unconsumed result is rejected with a loss counter rather than blocking or growing. |
| Joined frame ring | Application joins by `FrameId` and sample generation | Editor model; explicit benchmark exporter | 1,024 fixed summaries | Overwrites oldest live-only summary; a benchmark consumer streams accepted samples and marks any loss invalid. |
| Detailed GPU ring | Renderer resolves stable pass tokens and durations | Editor GPU detail view; explicit benchmark exporter | Eight frames, at most 256 scope records per frame across queues | Fixed storage; overflow invalidates detailed data for that frame, increments one loss counter, and preserves the independent top-level queue span where valid. |
| Memory ring | Application samples process memory; Renderer/RHI publishes GPU memory | Editor memory view; benchmark exporter | 256 samples at default 1 Hz | Overwrites oldest live sample; each sample owns fixed categories and explicit age/validity. |
| Editor performance model | Application derives a read-only projection after joining | Editor panels only | One latest immutable model plus fixed plotted windows | Replaced atomically/by owner publication; Editor retains no producer pointers or mutable spans. |
| Evidence stream | Application session serializes accepted joined summaries | Workload-owned artifact package | Exactly the declared bounded benchmark request | Fixed-size staging/chunk buffer; write failure stops the run, preserves prior accepted evidence, and reports the incomplete artifact. |

Hot records contain scalar fields, fixed arrays, stable label tokens, and typed identities only. They do not own strings, vectors, callbacks, service pointers, locks, or editor objects. Detailed records resolve tokens to display names outside collection. The storage budget is governed by:

```text
joined summary bytes
  = joined-frame capacity * fixed summary size
  + detailed-frame capacity * detailed-scope capacity * fixed scope-record size
  + memory-sample capacity * fixed memory-sample size
  + producer mailbox capacity * fixed result size
  + fixed export staging bytes
```

The implementation record must publish exact `sizeof` values and prove the total remains within the 4 MiB live-history cap. Changing frame-pipeline depth or worker count does not multiply live diagnostic storage implicitly; any per-worker producer scratch is separately fixed and included in that proof.

## Collection Modes And Cost Budget

| Mode | Intended use | CPU scopes | GPU scopes | Memory | Disk |
| --- | --- | --- | --- | --- | --- |
| `Off` | Shipping/default performance control where configured | Thread names only; no live aggregation. | Existing external markers follow their own build/CVar policy. | Product-required memory policy only. | None. |
| `LiveBasic` | Editor orientation | Fixed top-level host/render phases and bounded lane aggregates. | One top-level span per active queue; no detailed internal scopes. | Process sample at 1 Hz; RHI sample at its bounded cadence. | None. |
| `LiveDetailed` | Short interactive diagnosis | Top-level plus selected fixed subphases. | Frame-graph pass scopes and selected fixed detailed scopes. | Same cadence; optional high-water reset. | None. |
| `Benchmark` | `MAP-00` and declared routes | Exact raw per-frame summary for the bounded sample request. | Valid top-level and per-pass values required by the workload. | Current and high-water values over the run. | Explicit manifest, raw timing, and summary artifacts only. |
| `ExternalCapture` | PIX/RenderDoc/Nsight/WPA/RGP | Stable trace markers and symbols; live UI may be hidden. | Stable backend markers; internal timestamp collection may be disabled to avoid observer overlap. | Tool-specific capture plus a matching manifest bookmark. | Native profiler artifact by explicit user action. |

Instrumentation acceptance targets for the first implementation are:

- zero per-frame heap allocations in `LiveBasic` after initialization;
- at most 4 MiB total bounded live-history storage with a documented exact layout;
- less than 1% change in CPU frame p50 and less than 2% change in CPU frame p95 between `Off` and `LiveBasic` on both Empty and Sponza, using identical valid runs;
- less than 1% or 0.1 ms, whichever is larger, change in GPU p95 from the top-level timestamp pair;
- measured and reported overhead for `LiveDetailed` and `Benchmark`; results from different modes are never compared silently.

If a target cannot be met, the expensive collector becomes explicit capture-only. Diagnostics do not receive a hidden exception from the performance standard.

## Bounded Data Model

### Joined Frame Summary

The live ring stores at most 1,024 joined frame summaries. A summary contains fixed fields, not variable vectors:

- `FrameId`, sample generation, product mode, renderer mode, pipeline depth;
- begin/end CPU timestamps and frame interval;
- host phase durations and host unattributed duration;
- render phase durations, queue wait/backpressure, and render unattributed duration;
- task-lane occupied/count/ready-delay aggregates;
- per-queue top-level GPU span plus validity;
- presentation mode/wait classification;
- render/output/client extents, backend, render path, validation state;
- counts needed to compare work: frame-graph passes, draws, dispatches, submitted batches, visible/render instances, and upload bytes where already owned;
- diagnostics loss/invalid counters.

Detailed GPU pass records are retained only for the latest eight resolved frames while `LiveDetailed` is active, with at most 256 fixed scope records per frame across all queues. Overflow invalidates only that frame's detailed view and preserves an independently valid top-level queue span. The benchmark writer streams a declared bounded run to its explicit artifact rather than retaining an unbounded in-memory history.

### Memory History

Memory is slow-changing relative to frame scopes. A separate ring stores at most 256 samples at a default 1 Hz cadence:

- process working set, peak working set, private commit, and peak private commit;
- engine-tracked GPU used bytes and allocator block bytes;
- device-local API usage and budget;
- non-local/shared API usage and budget;
- transient bytes and retirement backlog;
- fixed GPU categories: texture, mesh, ray tracing, transient, upload, readback, constant buffer, other;
- sampled `FrameId`, monotonic timestamp, backend, validity, and source scope.

The target RHI memory contract must stop using a combined local plus non-local `TotalBudgetBytes` as the UI's "VRAM budget." The UI presents local and non-local segments separately. Backend differences in what API usage covers are visible in the model/tooltip and manifest.

### Aggregation Rules

- The live headline shows latest valid, p50, and p95 over the last 120 valid joined frames and displays the sample count and time span.
- Benchmark windows use the acceptance workload's current readiness, warm-up, sample-count, and run-count policy and record the resolved values in the manifest.
- Invalid, pending, stale, and disabled values do not enter a percentile.
- The initial percentile algorithm is nearest-rank over durations in integer nanoseconds; conversion/rounding happens only for presentation.
- Worst frame is preserved with its `FrameId`; a percentile is never reconstructed from displayed rounded values.
- High-water marks reset only on explicit session reset/start and record the `FrameId` at which they occurred.
- Nested CPU or GPU scopes are not summed into a parent. Parent wall duration and child contribution are displayed separately.
- Cross-thread task durations are not added to host/render wall time. Busy worker time and wall critical path are different views.

## Bottleneck Classification

The live UI may show a `Likely` hint only when all required signals are valid and correlated. It must also show the strongest supporting signals. `Confirmed` is reserved for a completed causal experiment/capture outside the live classifier.

| Hint | Required correlated observations | Next evidence |
| --- | --- | --- |
| `Likely GPU-limited` | GPU top-level span is at/over budget, non-wait host/render phases are below it, presentation throttle does not explain it, and producer/render backpressure is consistent with downstream GPU completion. | PIX Timing/GPU Capture on D3D12; Nsight Graphics or RGP GPU trace; RenderDoc for state/work inspection. |
| `Likely render-CPU-limited` | One non-wait render phase or render CPU wall is at/over budget, the GPU span is lower, and host/game work is not the critical path. | WPA/PIX Timing with symbols; inspect frame setup/compile/record/submit call stacks. |
| `Likely host/game/editor-limited` | A non-wait host/game/editor phase is at/over budget, render/GPU spans are lower, and frame-queue backpressure is not dominant. | WPA CPU sampled plus precise scheduling; compare DevelopmentGame and DevelopmentEditor. |
| `Likely present-limited` | Present/throttle wait dominates and the recorded VSync/frame-latency policy explains the interval. | PIX Timing or WPA/DWM/GPUView evidence with presentation configuration. |
| `Likely task scheduling-limited` | Ready delay, caller join, worker imbalance, or oversubscription is material to the critical path. | WPA with `SparkleTasks` ETW dependency/begin/end events and CPU scheduling. |
| `Likely memory-pressure/churn` | Local budget pressure, allocation growth, retirement backlog, upload/eviction activity, or process private commit rises with hitches. | PIX memory/timing capture, allocator records, vendor memory tool, and a controlled load/unload route. |
| `Unknown` | Required values are missing, disagree, belong to different frames, or do not isolate the limiting resource. | Fix validity/correlation or take a wider CPU/GPU system trace. |

CPU utilization alone is never sufficient for a CPU-limited hint. GPU utilization alone is never sufficient for a GPU-limited hint. A blocked producer can have low CPU utilization while the frame is GPU-limited; a GPU can show high utilization while an earlier CPU stage still controls latency.

## Editor Presentation

### Viewport Summary

The existing top-right FPS text becomes a compact diagnostic summary. Milliseconds lead; FPS remains a derived convenience.

```text
Frame 164.7 ms p50 / 181.2 p95 (6.1 FPS) | Host work 8.1 + wait 151.2 | Render work 18.6 | GPU 159.3 | RAM 4.12 GiB | VRAM local 4.02 / 7.36 GiB
Likely GPU-limited | D3D12 | 5120x1392 render | Threaded depth 1 | 120 frames / 19.8 s
```

Rules:

- A value without enough valid samples shows `warming up (N/120)`.
- GPU, RAM, and VRAM show `N/A`, `Disabled`, `Pending`, or age when appropriate.
- The resolution shown is render resolution, not inferred desktop/window size; output and client extent are in the details view.
- Color indicates budget state, not generic "good/bad": neutral below the configured budget, warning above it, critical above twice it, gray when invalid.
- Clicking the summary opens the Performance window at the correlated latest valid frame.
- FPS is `1000 / p50 frame interval ms` and is labeled as derived; it is not averaged from instantaneous FPS samples.

### Performance Window

One window serves the current Editor product owner. It is not a dockable general diagnostics platform.

```text
+ Performance ---------------------------------------------------------------+
| Run: Live  Frame: 18422  D3D12  DevelopmentEditor  Threaded depth 1        |
| Budget: 16.67 ms  Window: 120 valid frames / 19.8 s  Lost: 0  Invalid: 0   |
+----------------------------------------------------------------------------+
| Frame       latest 168.4 | p50 164.7 | p95 181.2 | worst 194.8 #18391      |
| Likely GPU-limited: GPU 159.3 ms; host waits 151.2 ms for frame capacity    |
+ CPU owners ----------------------------------------------------------------+
| EditorThread  [Host 1.0][Gameplay 2.7][Extract 1.1][UI 3.3][Wait 151.2]     |
| RenderThread  [Setup 5.2][Graph 3.6][Record 8.4][Submit 0.9][Present 0.5]   |
| Task FC 0     occupied 37%  ready p95 0.18 ms  Renderer.RecordChunk            |
| Task BG 0     occupied  4%  ready p95 0.09 ms  Idle                            |
| Task IO 0     occupied  0%                       Idle                            |
+ GPU queues ----------------------------------------------------------------+
| Graphics      [GBuffer 7.8][Lighting 132.4][Composite 11.0][UI 3.2] 159.3  |
| Compute       N/A (no submitted compute work)                               |
| Copy          1.1 ms; do not add to Graphics                                |
+ Memory --------------------------------------------------------------------+
| RAM working set 0.72 GiB | private commit 4.12 GiB | run high 4.19 GiB      |
| GPU tracked 3.56 GiB | local API 4.02 / 7.36 GiB | retire 0.18 GiB          |
| Texture 2.40 | Mesh 0.51 | RT 0.43 | Transient 0.17 | Other 0.05 GiB        |
+ Next action ---------------------------------------------------------------+
| Capture PIX Timing (D3D12 range), then inspect Lighting in a GPU capture.   |
+----------------------------------------------------------------------------+
```

All numbers above are illustrative only; they are not measurements of the supplied Sponza screenshot or evidence of a Sparkle bottleneck.

### Interpretation Examples

These patterns are also illustrative. They demonstrate the reasoning the system must support, not expected Sponza results.

| Observed correlated pattern | Honest orientation | Required confirmation |
| --- | --- | --- |
| Frame p95 `181 ms`; host non-wait phases `8 ms`; render non-wait phases `19 ms`; graphics span `172 ms`; producer backpressure `153 ms`; present throttle off. | Likely GPU-limited. Gameplay and editor UI are not large enough to explain the interval. | PIX Timing plus one PIX/Nsight/RGP GPU trace; rank pass and verify queue occupancy/synchronization. |
| Frame p95 `129 ms`; renderer frame-graph compile `94 ms`; graphics span `9 ms`; producer backpressure `0 ms`. | Likely render-CPU-limited in graph compilation. | WPA/PIX CPU samples and a compile-scope call tree; compare a serial/reference topology control. |
| Frame p95 `43 ms`; gameplay systems `35 ms`; render CPU `7 ms`; graphics span `10 ms`; FrameCritical ready delay `8 ms`. | Likely gameplay/task critical path with scheduling delay. | WPA precise scheduling plus `SparkleTasks` dependencies; test 1/2/N and serial controls. |
| Frame timing is stable; private commit rises every load/unload; GPU tracked used returns to baseline but allocator blocks remain high. | Possible CPU leak plus expected/uncertain GPU pool retention; do not call both leaks. | CPU allocation call-stack capture and a repeated post-retirement route; inspect allocator reuse/fragmentation separately. |

The window has four fixed views:

| View | Shows | Does not show |
| --- | --- | --- |
| Overview | Budget, frame distribution, likely domain, current configuration, next profiler action. | Per-function or per-draw detail. |
| CPU | Physical Sparkle thread lanes, fixed logical phase wall time, known owner waits, and task-lane occupied/ready-delay aggregates. | OS run/ready/wait scheduling, a flame graph, every task, or arbitrary external thread attribution. |
| GPU | Per-queue top-level spans and the latest bounded pass hierarchy. | Hardware counters, shader ISA, resource contents, or uncalibrated cross-queue total. |
| Memory | RAM definitions/trend/high-water plus local/non-local GPU memory and fixed categories. | Allocation call stacks or a permanent largest-resource browser. |

### Required Configuration Banner

Every screenshot or exported summary includes:

- commit/build identity and `DevelopmentGame`/`DevelopmentEditor` configuration;
- D3D12/Vulkan, adapter, device ID where available, driver, and validation state;
- scene/level, camera route, readiness state, render path/provider/fallback;
- render/output/client resolution, VSync/presentation mode, dynamic-resolution state;
- threaded/serial renderer, pipeline depth, task serial/worker counts;
- diagnostics mode, warm-up frames, measured frames, run count, and collection overhead result.

Without this banner, a number is orientation only and cannot be promoted into portfolio evidence.

## Baseline Experiments Before Optimization

The first Sponza investigation uses matched camera, resolution, renderer settings, content, and warm state. It records each mode separately:

| Comparison | Question | Important control |
| --- | --- | --- |
| `DevelopmentGame` vs `DevelopmentEditor` | Is the poor frame rate inherent to rendering/gameplay or specific to editor UI/composition? | Same render/output resolution and scene route; do not compare a maximized editor viewport with a smaller game window. |
| D3D12 vs Vulkan | Is the cost distribution backend-specific? | Same adapter, shader path, render settings, route, warm state, and validation state. |
| Threaded depth 1 vs threaded depth 0 | Does overlap help, or does backpressure/input latency dominate? | Compare distributions and input-to-present only when that latency becomes measurable. |
| Threaded vs serial renderer | Is render coordination/queueing involved? | Serial is a causal control, not an intended faster architecture. |
| Normal tasks vs `task.SerialExecution=true` | Does the task graph provide useful parallelism or scheduling overhead? | Same final result; record worker policy and tiny-work crossover. |
| GPU timings off vs basic vs detailed | What is instrumentation overhead? | Never merge samples collected with different timing scope sets. |

The screenshot's visible approximately 6 FPS is enough to justify measurement, but not enough to decide that Sponza is GPU-bound, editor-bound, or renderer-bound. Fixed resolution and active render path are first-order controls because the current editor can run at a maximized client extent.

## External Profiler Workflow

### Tool Selection Matrix

| Question | Primary tool | API/hardware | Sparkle correlation |
| --- | --- | --- | --- |
| Which CPU thread/function runs, waits, is ready but unscheduled, or wakes another thread? | Windows Performance Recorder + Windows Performance Analyzer | Windows, both graphics APIs | OS thread descriptions, symbols, `SparkleTasks` ETW provider, `FrameId` bookmarks. |
| How do CPU and GPU work overlap across many D3D12 frames? | PIX Timing Capture | D3D12 | CPU/GPU event hierarchy, thread names, frame markers, queue submits. |
| What D3D12 API state/resource/pipeline/event produced one frame? | PIX GPU Capture | D3D12 | Existing D3D12 GPU event scopes and frame-graph pass names. |
| What D3D12/Vulkan API state, resources, descriptors, draws, and dispatches produced one frame? | RenderDoc | D3D12 or Vulkan | Backend debug labels/object names and frame-graph pass names. |
| Which NVIDIA GPU units, shaders, barriers, queues, or RT work limit the frame? | Nsight Graphics GPU Trace; graphics capture/debugger where supported | NVIDIA D3D12/Vulkan | Frame/pass markers, queue names, shader debug data, exact driver/hardware. |
| Which AMD GPU waves, barriers, queues, or synchronization limit the frame? | Radeon GPU Profiler | AMD D3D12/Vulkan | PIX/Vulkan user markers, queue submits, exact driver/hardware. |
| Which CPU allocation call stack or lifetime grows RAM? | PIX Timing/Memory capture or a focused native heap tool | Windows process | Frame/phase events, symbols, explicit load/unload window. |

RenderDoc is a frame debugger, not the CPU profiler for this design. Nsight Graphics/RGP hardware conclusions apply only to the captured vendor architecture. PIX GPU evidence is D3D12-specific. WPA remains the cross-backend Windows CPU scheduling truth.

### Common Investigation Loop

1. Reproduce the issue on a fixed route and configuration after the published readiness signal.
2. Use `LiveBasic` only to choose the likely domain and select one representative `FrameId` or hitch range.
3. Write a falsifiable hypothesis before taking the detailed capture.
4. Capture the narrowest tool artifact able to distinguish competing causes.
5. Confirm symbols, stable thread/pass markers, API validation status, and capture overhead.
6. Record the critical path, not merely the busiest unit or largest duration.
7. Make one scoped change or one controlled configuration experiment.
8. Re-run the complete warm-up/sample protocol and correctness/quality checks.
9. Compare distributions and memory high-water; explain regressions and rejected alternatives.
10. Preserve only the small reviewed evidence package and tool-native capture needed for reproduction.

### CPU Playbook: WPA

Use the existing `SparkleTasks` TraceLogging provider GUID and a checked-in WPR profile that combines:

- sampled CPU stacks;
- precise CPU scheduling/context-switch data for ready/wait analysis;
- process/thread lifetime and image/symbol information;
- the `SparkleTasks` provider;
- the future minimal Application/Renderer frame marker provider.

For a slow editor frame:

1. Filter to the editor process and the fixed route interval.
2. Group sampled CPU by `Sparkle.EditorThread`, `Sparkle.RenderThread`, and named task lanes.
3. Inspect `CPU Usage (Precise)` for running, ready, and waiting intervals on the critical thread.
4. Join task begin/end/dependency events by run/task identity and `FrameId` where present.
5. Identify whether frame-queue backpressure, task join, present, file I/O, lock contention, preemption, or active call stacks own the delay.
6. Save the WPA profile/view and one annotated screenshot with the ETL artifact.

A high RenderThread wall duration with low sampled CPU can be a wait. A worker at 100% does not prove it is on the frame critical path. Ready time may expose oversubscription even when task bodies are individually fast.

### D3D12 GPU Playbook: PIX

Use a Timing Capture first when the question spans several frames, CPU/GPU overlap, queue latency, residency, or pacing. Use a GPU Capture second for a representative stable frame when the question concerns passes, events, API calls, resources, descriptors, pipeline state, shader cost, or barriers.

Required observations include:

- where the GPU becomes idle or work queues up;
- graphics/compute/copy submissions and synchronization;
- the expensive frame-graph pass and its child events;
- draw/dispatch/RT build cardinality and pipeline changes;
- resource state and descriptor correctness;
- timing/counter meaning and replay limitations;
- whether the profiler reproduces the engine's top-level timestamp within declared tolerance.

Native D3D12 debug layer/GPU validation proves API correctness separately. A successful PIX replay is not a substitute for validation.

### Vulkan / Cross-API GPU Playbook

Start with Vulkan validation and synchronization validation. Use RenderDoc to inspect the frame's API events, bound state, descriptors, resources, barriers, and outputs. On NVIDIA hardware, use Nsight Graphics GPU Trace for hardware limiter and shader/queue evidence. On AMD hardware, use RGP for queue, barrier, wave, and hardware evidence.

Compare D3D12 and Vulkan by semantic pass and route, not raw API call count alone. Record intentional differences in descriptor model, queue topology, allocator reporting, shader binary, barrier encoding, and provider capability.

### Memory Playbook

Use the live memory view to choose a controlled interval such as cold load, settled scene, camera route, scene switch, unload, and post-retirement. Record:

- process working set and private commit at every boundary;
- engine-tracked GPU used/block bytes;
- local/non-local API usage and budget;
- high-water frame identity;
- upload/eviction/residency events;
- retirement backlog and time to return to steady state.

If process private commit grows, take an allocation call-stack capture. If local GPU usage grows but tracked allocation does not, investigate driver/external-provider allocations and backend reporting. If tracked used shrinks but allocator blocks do not, investigate pooling/fragmentation before calling it a leak. If retirement backlog grows, correlate it with completion tokens and frames in flight.

## Benchmark And Portfolio Evidence

The live Editor screenshot is one portfolio artifact, not the result. Conceptually, a reviewer-ready performance case includes the following files under the workload-owned run directory; [I. Acceptance Workloads](../Engineering/BistroAndSanMiguelWorkloads.md) remains authoritative for exact names and placement:

```text
artifacts/validation/showcase-levels/<run-id>/<level-id>/
|-- manifest.json
|-- cook.log
|-- timings.csv
|-- summary.md
|-- launch.log
|-- <level>-<route>-diagnostics.png
|-- <level>-<route>-frame.png
`-- captures/
    |-- cpu-system-trace.<native-format>
    |-- d3d12-or-vulkan-frame.<native-format>
    `-- vendor-gpu-trace.<native-format>
```

This proposal requires that the manifest link every raw sample and capture to one run/configuration rather than creating a second evidence format. Native captures may remain in an accepted external location when their size or license requires it, but the manifest records the stable reference and provenance.
CPU, GPU, and sampled memory columns may share the workload-owned `timings.csv`; a separate raw file is added only if the acceptance-workload schema explicitly adopts it.

### Example Summary Table

This is a presentation template; values must come from real accepted runs.

| Metric | Baseline | Experiment | Delta | Interpretation |
| --- | ---: | ---: | ---: | --- |
| CPU frame p50 / p95 | `TBD` | `TBD` | `TBD` | Begin-to-begin, same declared route, window, and mode. |
| Host/game phase wall p50 / p95 | `TBD` | `TBD` | `TBD` | Physical owner plus logical phase breakdown; not sampled CPU execution time. |
| Render CPU p50 / p95 | `TBD` | `TBD` | `TBD` | Setup/compile/record/submit/present critical contribution. |
| GPU graphics p50 / p95 | `TBD` | `TBD` | `TBD` | Top-level graphics span; queue overlap stated separately. |
| Top GPU pass p50 / p95 | `TBD` | `TBD` | `TBD` | Stable semantic pass; no nested double count. |
| RAM working/private high-water | `TBD` | `TBD` | `TBD` | Both definitions reported. |
| GPU tracked/local high-water | `TBD` | `TBD` | `TBD` | Used/block/API/budget scope declared. |
| Quality/correctness | `TBD` | `TBD` | n/a | Identical image/reference or explained change. |

### Example Incident Narrative

A specialist-facing case should be readable in this order:

1. Result: "Sponza at the fixed route was limited by `<measured domain>`, not by `<competing hypothesis>`."
2. Configuration: commit, product, backend, hardware/driver, resolution, route, settings, validation, workers, and diagnostics mode.
3. Baseline: distributions and memory high-water, not one frame or one FPS number.
4. Competing hypotheses: for example editor UI, game systems, frame-graph compile, command recording, GPU lighting, presentation, or memory pressure.
5. Discriminating evidence: one joined diagnostics view plus the appropriate CPU/GPU capture.
6. Root cause: critical path with relevant call stack, pass, queue, resource, counter, or state.
7. Experiment/fix: one scoped change and the control that could falsify it.
8. Outcome: before/after distributions, image/validation result, memory, pacing, and limitations.
9. Adoption: how another engineer reproduces the route and opens the capture.
10. Deletion/negative result: instrumentation, path, or optimization rejected after evidence.

### Reviewer Paths

| Reviewer | Diagnostic evidence they should see |
| --- | --- |
| Recruiter, 60-90 seconds | One clean frame/diagnostics image and one sentence naming the measured bottleneck and outcome. No profiler wall of text. |
| Hiring manager, 10 minutes | Problem, configuration, baseline, likely-domain view, capture, one controlled experiment, before/after table, and limitation. |
| Graphics specialist, 45-60 minutes | Raw schema, CPU/GPU timing semantics, marker tree, PIX/RenderDoc/Nsight/RGP/WPA capture, API/backend difference, counters or disassembly where causal, and reproduction steps. |
| Adopter | Exact build/run/route, diagnostic mode, expected summary, capture trigger, fallback, raw files, and issue/reproducer template. |

This directly advances whole-system performance, hard-debugging, low-level concurrency, productization, and communication evidence. It does not advance those requirements to `E3` until the captures and measurements are reproducible.

## Failure And Edge Cases

- Minimized or invalid-size frames do not enter render/GPU aggregates.
- Loading frames remain visible but are tagged `Loading/Unsettled` and excluded from warm benchmark windows.
- Resize, render-path/provider change, shader reload, history reset, and capture frames are tagged discontinuities.
- A device-lost or failed timestamp resolution invalidates the frame and preserves the failure context; it never reports zero milliseconds.
- Timestamp pool exhaustion records one bounded diagnostic loss and disables affected detailed scopes for the frame.
- Timestamp wrap uses the backend valid-bit contract; cross-queue subtraction is forbidden without calibration.
- A GPU scope nesting mismatch invalidates the detailed frame and leaves the top-level result independent where possible.
- Queue overlap means per-queue spans are displayed in parallel; they are not summed.
- VSync, frame-latency throttling, occlusion, remote desktop, capture/replay, validation, and power/thermal changes are recorded in the manifest.
- Memory budget changes caused by other processes are retained as observed API budget changes, not attributed to Sparkle allocations.
- A synchronous diagnostics request from EditorThread to RenderThread is forbidden in the live frame path.

## Verification Contract

Implementation acceptance requires focused tests and measured runs, as applicable:

### Semantics And Correlation

- Synthetic nested scopes prove parent wall/child contribution without double count.
- Threaded depth 0/1/2 frames join CPU and delayed GPU data by `FrameId` under randomized resolution delay.
- Serial renderer/task modes publish honest physical-thread ownership and no phantom lanes.
- Missing/disabled/stale/invalid values never enter percentiles and never render as zero.
- Timestamp wrap, queue separation, sample-window reset, lost result, shutdown, and late publication are tested.
- Nearest-rank percentiles and high-water frame identity pass known-value tests.

### Backend And Product Matrix

- DevelopmentGame and DevelopmentEditor on D3D12 and Vulkan.
- Threaded and serial renderer controls.
- Normal and serial task controls.
- Validation on/off state recorded; native validation passes on supported routes.
- Empty establishes observer overhead; Sponza proves the `MAP-00` calibration path.
- Fixed render/output/client resolution, VSync, provider/fallback, and readiness are visible.

### Profiler Correlation

- An engine CPU phase matches the corresponding ETW/PIX interval within a declared tolerance.
- Engine top-level D3D12 GPU timing matches the corresponding PIX range within a declared tolerance.
- Engine Vulkan pass names and ordering are visible in RenderDoc and a supported vendor trace.
- Every Sparkle-owned OS thread appears with its stable name.
- The `SparkleTasks` ETW provider decodes task names, lanes, run/task identity, outcome, and dependencies.
- One artificial CPU stall, GPU stall, frame-queue backpressure case, and memory growth/retirement case is detected and classified; removing the injected defect clears the signal.

### Cost And Bounds

- Ring capacities, queue capacities, string/label storage, and export sizes are asserted/tested.
- `LiveBasic` performs no post-initialization per-frame heap allocation.
- `Off`, `LiveBasic`, `LiveDetailed`, and external-capture overhead are measured, not assumed.
- UI open/closed results do not materially change collection cost beyond the declared presentation cost.
- Export failure preserves the previous accepted evidence and reports one actionable error.
- `git diff --check`, applicable builds/tests, architecture boundary check, and exact unavailable hardware/tool paths are reported.

## Suggested Vertical Slices

This is architecture decomposition, not a schedule; the Roadmap and `MAP-00` own priority.

1. Freeze metric names, units, validity, `FrameId` join behavior, and a source-backed baseline trace using existing thread/ETW/GPU markers.
2. Add the bounded Application session, host phases, process RAM, Renderer CPU stages, frame-queue waits, and one top-level GPU queue span needed by `MAP-00`.
3. Complete the workload-owned `MAP-00` vertical slice: fixed resolution/readiness, explicit benchmark export and manifest integration, capture naming, and Sponza calibration.
4. Publish the same immutable session model to Editor, correct GPU memory segment semantics, and present RAM/VRAM trends and high-water marks in the fixed Performance window.
5. Add detailed pass timing only after basic-mode overhead and profiler correlation pass.
6. Check in the narrow WPR profile and profiler walkthrough; capture one D3D12 and one Vulkan specialist example.

Each slice must extend the existing owner, remove any presentation path it replaces, and leave detailed profiler data in profiler-native artifacts rather than widening engine public APIs.

## Decisions And Rejected Alternatives

| Decision | Selected | Rejected |
| --- | --- | --- |
| Product shape | Bounded live orientation + explicit evidence session + external tools. | General in-engine profiler/trace viewer. |
| Cross-domain owner | Application session joins immutable domain results. | Global Core profiler singleton or Editor reaching into renderer/RHI state. |
| Editor gameplay labeling | Logical `Gameplay.*` phases on `Sparkle.EditorThread`. | Invented editor-side `GameThread`. |
| CPU detail | Fixed orchestration scopes live; ETW/PIX call stacks for detail. | Per-function timers and an unbounded task history panel. |
| GPU frame value | Per-queue outer span with valid `FrameId`; detailed passes on demand. | Sum of pass timers or GPU utilization converted to milliseconds. |
| Memory | Working/private RAM and tracked/block/local/non-local/retirement GPU facts. | One ambiguous "RAM" and one combined "VRAM" number. |
| Publication | Delayed nonblocking join by `FrameId`. | Waiting for RenderThread/GPU so the newest UI row is complete. |
| Labels | Stable bounded ownership/pass vocabulary. | Dynamic resource/entity/path strings in timing scopes. |
| Files | Explicit workload-owned export. | Default per-run JSON/CSV/report emission. |
| Optimization start | Fixed, comparable DevelopmentGame/Editor and backend baselines. | Optimizing from the approximately 6 FPS screenshot alone. |

## Open Implementation Decisions

These choices require a bounded implementation record after measurement; they do not block the architecture:

- the exact minimal Core CPU trace surface and whether PIX CPU events are emitted in addition to ETW on Windows;
- the exact renderer result mailbox type and fixed capacity;
- whether top-level copy/compute timestamps meet the basic-mode overhead target on both backends;
- the platform abstraction for process working/private memory beyond the initial Windows evidence platform;
- the exact local/non-local heap representation needed to preserve D3D12/Vulkan semantics;
- the final fixed count set after the first Sponza trace proves which cardinalities distinguish the leading hypotheses;
- the benchmark CLI/request surface, which must integrate with `MAP-00` rather than create a second workflow.

## External Profiler References

- [PIX overview and capture selection](https://learn.microsoft.com/en-us/windows/win32/direct3dtools/pix/articles/general/pix-overview)
- [PIX D3D12 GPU capture analysis](https://learn.microsoft.com/en-us/windows/win32/direct3dtools/pix/articles/gpu-captures/pix-gpu-captures)
- [PIX Timing Capture memory layout](https://learn.microsoft.com/en-us/windows/win32/direct3dtools/pix/articles/timing-captures/layouts/pix-timing-captures-memory-layout)
- [Windows Performance Toolkit CPU analysis](https://learn.microsoft.com/en-us/windows-hardware/test/wpt/cpu-analysis)
- [Capturing and viewing TraceLogging data with WPR/WPA](https://learn.microsoft.com/en-us/windows-hardware/drivers/devtest/capture-and-view-tracelogging-data)
- [RenderDoc project and supported graphics APIs](https://github.com/baldurk/renderdoc)
- [RenderDoc Vulkan frame-debugging workflow](https://github.com/baldurk/renderdoc/wiki/Vulkan)
- [NVIDIA Nsight Graphics features](https://developer.nvidia.com/nsight-graphics-features)
- [NVIDIA GPU Trace workflow](https://developer.nvidia.com/blog/migrating-from-range-profiler-to-gpu-trace-in-nsight-graphics/)
- [AMD Radeon GPU Profiler manual](https://gpuopen.com/manuals/rgp_manual/)
- [RGP user marker integration](https://gpuopen.com/manuals/rgp_manual/user_debug_markers/)

These sources establish tool capability and workflow only. They do not define Sparkle ownership, metric semantics, or evidence acceptance.

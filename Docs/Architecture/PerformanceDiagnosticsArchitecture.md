# Performance Diagnostics Architecture

Status: target proposal; not proof of current implementation

Last source reconciliation: 2026-08-12

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
- [Diagnostics Product And UX Research](DiagnosticsUxResearch.md) records the Epic/NVIDIA/AMD product study and rejected UX options behind the selected presentation; it is research, not implementation authority.
- [External Performance Profiler Runbook](DiagnosticsProfilerRunbook.md) owns version-sensitive tool capabilities, capture preparation, marker interoperability, and operational playbooks; it does not define Sparkle metrics or prove a benchmark claim.

Application owns the presentation-neutral live diagnostics product, cross-domain session orchestration, active stat-view selection, and benchmark export. Editor owns its viewport menu/window presentation. DevelopmentGame owns a compact presenter through its existing runtime console/UI packet path. Each engine domain remains the authority for its own measurements. These concrete product consumers justify compact stat views; they do not justify a general task browser, allocation explorer, or trace-viewer product.

## Executive Decision

Sparkle will provide four complementary diagnostic layers:

| Layer | Answers | Product shape |
| --- | --- | --- |
| Live orientation | Which domain is consuming the frame budget right now? Is memory growing or under pressure? | Composable `Stat` overlays in Editor and DevelopmentGame, plus one bounded Editor performance window. |
| Focused GPU frame analysis | Which marked queue/pass region consumes a selected GPU frame, inclusively and exclusively? | An on-demand, frozen GPU captured-frame mode inside the Performance workspace, built from frame-graph-owned scopes. |
| Reproducible measurement | What are the distributions and high-water marks for a declared run? | An explicit `MAP-00`/benchmark session with a manifest and raw samples. No default report files. |
| Causal investigation | Why is a CPU stage, GPU pass, wait, allocation path, or queue expensive? | PIX, RenderDoc, Nsight Graphics, Windows Performance Recorder/Analyzer, and hardware-appropriate vendor tools using Sparkle's stable names and markers. |

The live layer is a compass, not a verdict. It must expose validity, frame identity, configuration, and likely limiting domain without pretending that utilization, FPS, summed scope time, or a single captured frame proves cause.

## Unreal Stat-System Precedent And Sparkle Adaptation

Status of this section: external research used to shape the target; it is not local implementation authority.

Unreal's strongest lesson is the diagnostic ladder, not any particular macro or subsystem name. Its official documentation presents stat commands as the quickest in-application view, `stat unit` as a frame/game/draw/GPU orientation surface, opt-in groups such as GPU, Memory, RHI, and SceneRendering, and deeper tools for causal analysis. Unreal's render graph also provides render-graph-owned scopes for both in-engine GPU stats and external-profiler events.

Sparkle adopts the following product behaviors:

| Unreal precedent | Sparkle adaptation | Deliberately not copied |
| --- | --- | --- |
| A short console command toggles an on-screen stat group. | One case-insensitive, autocomplete-capable `Stat` command uses Sparkle's existing Editor and runtime console registries. The Editor viewport also exposes the same fixed groups in a Stat menu. | A second console, hidden command parser, or presenter-specific command implementation. |
| `stat unit` rapidly compares frame, game, draw/render, RHI-thread, and GPU time. | `Stat Unit` compares frame interval, host/game phase wall, render wall, queue waits, presentation, and GPU queue span using honest Sparkle ownership labels. | Pretending Editor gameplay has its own OS thread, or treating the numerically largest pipelined column as proof by itself. |
| Named groups expose GPU, RHI, scene rendering, memory, gameplay, and hitch information. | A small fixed catalog exposes Fps, Unit, UnitGraph, Threads, Tasks, Gpu, GpuPasses, Render, Scene, Rhi, Memory, and Hitches views. Each group has a real producer and bounded consumer. | An open-ended global stat registry, arbitrary module macros, or hundreds of groups without an active product question. |
| Stat types distinguish cycle counters, per-frame counters, persistent accumulators, and memory values. | Every Sparkle row declares duration, count, bytes, ratio, state, high-water, sampling interval, and validity semantics. | Combining unlike kinds into a typeless number or assuming every value resets per frame. |
| Render-graph scopes feed both in-engine GPU stats and external markers. | Stable Sparkle frame-graph/pass tokens are shared by detailed GPU rows and PIX/RenderDoc/Nsight/RGP markers. | Separate UI-only pass names or per-frame formatted marker strings. |
| A focused GPU profile can expose a hierarchical event tree while realtime GPU stats stay quick and cumulative. | `Stat Gpu`/`Stat GpuPasses` remain live orientation; `ProfileGpu` takes one bounded frozen hierarchical capture with inclusive/exclusive views. | Running a full hierarchical query capture continuously or confusing the visualizer with hardware-counter attribution. |
| Captures complement live stats because the overlay is quick but not a complete profiler. | Live stats answer where to look and support simple controlled comparisons; external tools remain the authority for call stacks, scheduling causality, shader/hardware limits, and API-state investigation. | Claiming that a rich overlay eliminates the need to know profiler workflows. |

Primary precedent sources are Epic's [Stat Commands](https://dev.epicgames.com/documentation/unreal-engine/stat-commands-in-unreal-engine), [performance profiling introduction](https://dev.epicgames.com/documentation/unreal-engine/introduction-to-performance-profiling-and-configuration-in-unreal-engine), [Stats System overview](https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-stats-system-overview), [graphics programming overview](https://dev.epicgames.com/documentation/unreal-engine/graphics-programming-overview-for-unreal-engine), and [Render Dependency Graph profiling guidance](https://dev.epicgames.com/documentation/unreal-engine/render-dependency-graph-in-unreal-engine#performanceprofiling).

## Goals And Non-Goals

### Goals

- Separate application frame interval, editor work, gameplay/world work, render-thread CPU work, task-worker work, GPU work, presentation waits, process RAM, and GPU memory.
- Preserve the difference between a physical OS thread and a logical phase running on that thread.
- Correlate delayed and pipelined CPU/GPU samples with the existing `FrameId`.
- Show current values, stable distributions, high-water marks, and data age with explicit units.
- Keep all queues, buffers, label sets, and exports bounded.
- Reuse the existing thread names, `SparkleTasks` ETW events, frame-graph GPU scopes, RHI timestamps, and allocator diagnostics.
- Make D3D12 and Vulkan measurements semantically comparable while exposing backend limitations.
- Provide fast, composable `Stat` views in both DevelopmentEditor and DevelopmentGame without requiring an attached profiler.
- Provide an on-demand GPU captured-frame view with per-queue marker hierarchy, inclusive/exclusive cost, flat/coalesced views, and exact capture configuration.
- Make the next profiler action obvious and produce evidence a portfolio reviewer can audit.
- Measure the observer cost of the diagnostics themselves.

### Non-Goals

- A home-grown sampling profiler, flame graph, GPU counter suite, allocation call-stack recorder, or trace viewer.
- A second task runtime, task history browser, per-subsystem thread pool, or diagnostics event bus.
- A generic public stat-registration framework or unrestricted module-defined overlays.
- A replacement for RenderDoc/PIX resource inspection, pipeline-state debugging, shader ISA/counters, or vendor hardware analysis.
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
| Console surface | Core owns a case-insensitive command registry with help and autocomplete. Editor and DevelopmentGame already host separate console presenters over that registry. | Register one `Stat` command family through the existing composition path; do not create a diagnostics console. |
| Host clock | `Timer` records unscaled tick-to-tick delta and a monotonically increasing frame count. | This is a useful starting identity/interval, but the benchmark definition must explicitly be unscaled application begin-to-begin time. |
| Thread names | Editor, Game, Render, Tool main, and every task lane worker call `SetCurrentThreadRole`; Windows receives the same thread description. | External CPU tools can already distinguish Sparkle-owned OS threads. |
| Task profiling | `TaskProfiler` emits ETW `TaskDependency`, `TaskBegin`, and `TaskEnd` events under the `SparkleTasks` provider. | WPA can inspect named task work; live UI should not duplicate a full task trace viewer. |
| Render topology | Threaded rendering defaults on with pipeline depth 1. A serial renderer is also supported. | Samples from simultaneous wall-clock columns may refer to different `FrameId` values. Mode and depth must always be visible. |
| Editor/game ownership | `EditorApplication::Tick` calls `RuntimeApplication::UpdateRuntime` on `Sparkle.EditorThread`. Standalone runtime calls it on `Sparkle.GameThread`. | "Gameplay" is a logical phase on the current host owner, not a permanent editor-side OS thread. |
| GPU markers | Frame-graph passes and selected detailed operations emit backend GPU diagnostic scopes. | Existing stable pass labels should be the profiler correlation vocabulary. |
| GPU timestamps | D3D12 and Vulkan timestamp queries resolve into private `ResolvedGpuTiming` values when a frame slot retires; `r.Diagnostics.GpuTiming` defaults off. | The renderer needs a bounded immutable frame result; Editor must never read renderer-private state. |
| GPU hierarchy fields | Resolved timings contain label, begin/end ticks, duration, queue, and nesting depth. Frame-graph pass labels include pass kind/index/name; selected pass-internal RT scopes add child labels. | This is enough to prove the foundation, but not enough for a durable product: add `FrameId`, stable token, explicit parent, source kind, submission identity, counts, and validity. |
| GPU timing observer effect | `FrameGraphRecordingExecutor::ShouldRecordBatchInParallel` currently disables parallel command recording whenever `r.Diagnostics.GpuTiming` is enabled. | Current detailed timings alter CPU recording topology. A representative GPU Visualizer must preserve normal recording through preassigned task-local scope/query ranges or declare the capture non-representative. |
| GPU timing storage | Current scope recording uses per-scope strings, vectors, a mutexed completion stream, and completion-time depth reconstruction; query-pool exhaustion is fatal. | Replace this as the live product path with fixed records/tokens, deterministic merge, and bounded loss. Keep fatal errors only for owner/API invariant violations. |
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
- `Engine/Renderer/Private/Diagnostics/PassExecutionDiagnostics.cpp`
- `Engine/Renderer/Private/FrameGraph/Execution/FrameGraphRecordingChunkRecorder.cpp`
- `Engine/Renderer/Private/FrameGraph/Execution/FrameGraphRecordingExecutor.cpp`
- `Engine/Renderer/Private/Diagnostics/RendererMemoryMonitor.cpp`
- `Engine/RHI/Public/Memory/RhiMemoryDiagnostics.h`
- `Engine/Editor/Private/Panels/ViewportTopPanel.cpp`

## Measurement Vocabulary

Every visible metric must have one owner, unit, interval, correlation identity, validity state, and sampling mode. A blank or `N/A` value is correct when the engine cannot establish the measurement.

### Clock And Identity Rules

`FrameId` remains the shared logical correlation identity. Diagnostics may add a benchmark `RunId`, sample-window identity, OS thread ID, queue type, and `RhiSubmissionToken`, but none becomes a second frame authority.

CPU clocks use one monotonic source. GPU durations use backend timestamp periods. CPU and GPU absolute timestamps are not placed on one time axis unless the backend provides and the implementation validates clock calibration. Backend contracts are explicit:

- D3D12 timestamps are bottom-of-pipe samples. Frequency and support are queried per command queue; direct and compute support timestamps, while copy support is capability-gated.
- Vulkan timestamp records include the selected pipeline stage, queue-family `timestampValidBits`, device `timestampPeriod`, and the wrap decision used by the resolver. A Vulkan timestamp can occur at the requested stage or a logically later stage and is not silently treated as a D3D12 bottom-of-pipe sample.
- Queue dependencies and submission tokens are retained even when clocks are not calibrated. They prove ordering constraints, not elapsed overlap on a unified axis.

The initial product keeps CPU-relative and per-queue GPU-relative axes. A later calibrated view is admitted only when every participating domain has a current calibration, its uncertainty is below a declared purpose-specific threshold, and the calibration age is within a declared interval. D3D12 uses `ID3D12CommandQueue::GetClockCalibration`; Vulkan uses calibrated timestamps, records maximum deviation, rejects excessive deviation, and recalibrates rather than extrapolating indefinitely. Until those gates pass:

- CPU phases correlate to `FrameId` and use CPU-relative time;
- GPU scopes correlate to `FrameId` and use per-queue GPU-relative time;
- submit-to-GPU and input-to-present latency remain unavailable rather than estimated;
- multiple GPU queue spans remain separate unless calibrated overlap is proven.

### Timestamp And Measurement Provenance

Every timing or counter field carries one provenance value independent of validity:

| Provenance | Meaning and comparison rule |
| --- | --- |
| `NativeLiveTimestamp` | Timestamp recorded while the declared workload executed normally. Eligible for live and benchmark comparison when observer mode matches. |
| `ReplayTimestamp` | Timestamp measured during capture replay. Useful inside that replay; never silently compared with native benchmark timing. |
| `SampledHardwareMetric` | Statistical hardware/PMC/counter sample with architecture, interval, multiplexing, and sampling limitations attached. |
| `SystemTraceDerived` | Value derived from scheduler, presentation, API, or other system-trace events under a named derivation. |
| `Estimated` | Model-derived orientation only. The model and error bound are visible; it cannot satisfy a native timing or latency evidence gate. |
| `Unavailable` | No defensible source exists for the field in this capability/configuration. Validity carries the typed reason. |

The configuration banner, tooltips, raw schema, and comparison logic preserve provenance. A capture import may attach native, replay, sampled, and system-derived fields to one evidence case, but it may not coerce them into one homogeneous population.

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
| GPU scope inclusive | End timestamp minus begin timestamp for one valid scope instance on one queue. | Marked commands plus every valid nested child interval and any queue gap inside the scope. | Not shader-only active time; nested inclusive values are not additive. |
| GPU scope exclusive | Inclusive duration minus the interval union covered by valid direct children on the same queue. | Commands, waits/bubbles, and unmarked work inside the parent but outside child scopes. | Not a hardware-unit attribution; unavailable when hierarchy/containment is invalid. |
| GPU queue unaccounted | Synthetic queue-root inclusive duration minus the interval union of valid top-level recorded scopes. | Queue time not covered by a timed top-level marker. | A coverage signal, not automatically waste or idle time. |
| Process working set | Current physical pages resident for the process. | Shared and private resident pages as reported by the OS. | Not ownership and not total committed memory. |
| Process private commit | Private committed virtual memory charged to the process. | Private committed pages, resident or paged out. | Not the same as current physical RAM. |
| GPU tracked used | Sum of live resource/allocation payload bytes tracked by Sparkle's GPU allocator. | Engine-owned tracked allocations. | Not driver allocations and not necessarily resident. |
| GPU allocator blocks | Bytes reserved in allocator blocks/heaps. | Internal fragmentation and unused capacity within blocks. | Not identical to resource payload or adapter usage. |
| Local API usage / budget | Backend/API report for the D3D12 local segment or Vulkan device-local heap groups. | Process-estimated usage and mutable budget in the scope declared by the backend snapshot. | "Local" is not universally dedicated VRAM, especially on UMA/host-visible designs; never combine it with non-local heaps and call the sum VRAM. |
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

The UI shows the state, sample age, and typed reason. Capability reporting is `capability + reason + relevant limits`, not a boolean. It never substitutes zero for missing data.

### Sample Population And Cohort Contract

Each single-metric row uses its own field-valid population: all samples in the selected interval whose field is `Valid` and whose provenance/mode matches the view. Cross-domain comparisons, within-run paired deltas, and the live bottleneck classifier instead use the intersection of `FrameId` values for which every required field is valid and comparable. They never compare independently filtered CPU and GPU percentiles as if they described the same frames.

Every aggregate publishes:

- population kind: `FieldValid`, `CommonCorrelated`, or a named benchmark population;
- valid sample count, original interval count, elapsed span, sample generation, and mode/provenance;
- pending, disabled, stale, invalid, discontinuity, capacity-loss, and other excluded counts by reason;
- the selected interval/range identity and any minimum population requirement.

`UnitGraph` renders an invalid field as a gap for that field. A multi-domain hint is `WarmingUp` until its minimum common cohort exists and `Unknown` when correlation, provenance, or validity cannot support comparison.

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
| `Sparkle.RenderThread` | `r.ThreadedRenderer=true` | Renderer/RHI mutable ownership, frame graph, recording coordination, submission, present. | Selected completed `FrameId` stage breakdown, classified waits, and queue latency. |
| `Sparkle.Task.FrameCritical.N` | Parallel task runtime has workers | Dependency-ready game-system, render-preparation, or command-recording tasks. | Occupied %, ready delay, task count, and longest stable task label for the selected interval; detailed spans in ETW. |
| `Sparkle.Task.Background.N` | Parallel task runtime has workers | Bounded asset preparation and editor/tool background CPU work. | Occupied %, ready delay, task count, and longest stable task label for the selected interval. |
| `Sparkle.Task.BlockingIo.N` | Parallel task runtime has workers | Bounded blocking file/process operations. | Occupied %, task count, and longest stable task label for the selected interval; OS traces distinguish running from blocked. |
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

- Pass names come from compiled frame-graph diagnostic names. The same stable `ScopeToken` resolves both the visualizer name and the backend marker display path; a UI-only naming hierarchy is forbidden.
- `FrameId` is scope-record metadata or a capture bookmark where the tool supports it; it is not a per-frame formatted pass label.
- Queue is explicit metadata, not encoded only in display text.
- Detailed internal scopes are capture-time diagnostics and are disabled in the basic live mode.
- Owned draw/dispatch counts and marker-only annotations may accompany a timed scope. Per-draw timing, resource contents/state, shader analysis, and hardware attribution belong in PIX, RenderDoc, Nsight, or RGP.
- The engine preserves a top-level queue span even when detailed pass timings are disabled.

One private backend adapter fans each semantic scope out to Sparkle timing and supported native markers. The portable marker contract is:

- a generated or `constexpr` registry owns `ScopeToken`, stable display path, schema version, and backend-safe static string storage; build tests reject token/hash collisions and duplicate paths;
- aggregation identity never includes `FrameId`, pointer values, transient graph indices, resource paths, or per-frame formatted text;
- duration scopes are balanced RAII objects and remain inside one CPU task and one command-list/command-buffer recording lifetime; task-local and command-recording-local stacks prevent cross-thread or cross-command-buffer push/pop pairs;
- duration regions, point annotations, and resource/object names are distinct operations. A backend/tool may support only a subset without changing semantic scope identity;
- D3D12 PIX marker strings use the static/aligned storage required by the selected PIX event runtime path; Vulkan uses `VK_EXT_debug_utils` when available. Tool/version-specific fallbacks and limitations belong to the [profiler runbook](DiagnosticsProfilerRunbook.md#marker-interoperability-contract).

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
                    +----> immutable live performance model
                    |       |-- Editor Stat menu/overlays/window
                    |       `-- DevelopmentGame stat overlay
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
| Active stat groups and presentation-neutral row models | Application | Fixed catalog maps immutable diagnostic fields to bounded rows and required collection mode. No domain measurement authority. |
| Editor live presentation and user intent | Editor | Reads one immutable model and submits semantic group/reset/export requests from the viewport menu, console, or Performance window. No renderer/RHI pointers. |
| DevelopmentGame live presentation | Application runtime console/UI packet owner | Reads the same immutable model and submits the same semantic group requests. It does not gain Editor dependencies. |
| Benchmark artifacts | Application evidence session | Written only for an explicit bounded request under the acceptance-workload artifact root. |

### Target Responsibility Placement

The implementation extends existing modules and private production paths; these are responsibility boundaries, not permission to add a new top-level framework:

```text
Core
  monotonic clock + minimal platform trace emission only
Tasks
  existing SparkleTasks ETW/task identity
Application/Private/Diagnostics
  session/demand owner, typed domain mailboxes, FrameId join/generations,
  fixed rings/descriptors, immutable live model, explicit exporter
Renderer/Private/Diagnostics
  render CPU builder/waits, bounded GPU scope plan, delayed resolver,
  immutable FrameId-keyed renderer results
RHI/<backend>/Private/Diagnostics
  timestamp capability/calibration, memory facts, native marker/object-name fanout
Editor/Private
  Performance/Stat presenters and typed requests only
DevelopmentGame Application presenter
  compact rows through the existing runtime console/UI packet route
```

Core does not own a profiler service. Editor does not own engine measurements. RHI public contracts expose only the backend-neutral facts required by the Renderer/Application consumers; vendor SDK types and tool-specific state remain backend-private. A new file or public type requires a present consumer and cannot exist only to anticipate a future profiler.

### Publication Rules

- Producers write owner-local frame builders; there is no global mutable timer registry.
- Cross-domain transport uses bounded typed domain mailboxes selected by the Application session; there is no generic diagnostics event bus or unbounded MPSC trace stream.
- Completed results are immutable and keyed by `FrameId`.
- GPU results may complete an older pending frame after its frame-in-flight slot retires by publishing a new immutable model generation. Published objects are never mutated in place.
- The Application join never blocks EditorThread/GameThread waiting for RenderThread or GPU data.
- Each presenter reads a published model no more than once per UI frame and never synchronously requests a renderer snapshot.
- Editor menu actions and `Stat` console commands submit the same typed group-selection request; they do not parse or invoke one another's presentation path.
- Enabling or disabling a group starts a new sample-window generation at an owner commit boundary. Late results from the previous demand set cannot populate a newly enabled view.
- A dropped or overwritten result increments a bounded loss counter and invalidates the affected aggregate; it does not grow a queue.
- Reset starts a new sample-window generation. Results from the previous generation cannot enter new aggregates.
- Shutdown stops publication, settles producers, drains or invalidates pending joins, then destroys the session.
- Capture state and every asynchronous producer settle each request/generation exactly once. Generation checks reject late completion after reset, clear, replacement, shutdown, or device loss.

### Diagnostics Data/Access Inventory

| Product | Producer / mutation point | Consumers | Frequency and expected cardinality | Publication, lifetime, and overflow |
| --- | --- | --- | --- | --- |
| Host frame builder | EditorThread or GameThread at fixed Application scopes | Application join; CPU trace sink | One builder for the current `FrameId`; fixed phase columns | Owner-local until end-frame publish, then immutable; missing phase closes as invalid rather than escaping the frame epoch. |
| Renderer frame result | RenderThread/inline render owner; delayed GPU resolver completes GPU fields | Application join; external marker correlation | At most the configured frame-pipeline capacity plus delayed retirements | Fixed mailbox capacity; oldest unconsumed result is rejected with a loss counter rather than blocking or growing. |
| Joined frame ring | Application joins by `FrameId` and sample generation | Live performance model; explicit benchmark exporter | 1,024 fixed summaries | Overwrites oldest live-only summary; a benchmark consumer streams accepted samples and marks any loss invalid. |
| GPU scope plan | Frame-graph compile assigns stable scope tokens, parents, queues, submission order, and exclusive per-chunk record/query slices | Command recorders and delayed resolver | At most 256 scope instances and 512 timestamps for one detailed frame across queues | Immutable for the frame epoch; each task writes only its preassigned slice and merge order ignores completion order. |
| Detailed GPU ring | Renderer resolves stable pass tokens and durations | Editor GPU detail view; explicit benchmark exporter | Eight frames, at most 256 scope records per frame across queues | Fixed storage; overflow invalidates detailed data for that frame, increments one loss counter, and preserves the independent top-level queue span where valid. |
| Frozen GPU profile | Renderer validates and derives one resolved detailed frame | Application publication; Editor Performance/GPU captured-frame view; DevelopmentGame compact result | One armed request and one retained frozen capture | Capture request has generation/id; replacement or clear releases the prior result after presenter publication. No UI pointer crosses the boundary. |
| Memory ring | Application samples process memory; Renderer/RHI publishes GPU memory | Editor memory view; benchmark exporter | 256 samples at default 1 Hz | Overwrites oldest live sample; each sample owns fixed categories and explicit age/validity. |
| Active stat selection | Application applies typed requests from Editor or DevelopmentGame presenter | Collection-mode resolver and presenters | At most four compact groups plus one fixed Editor Performance view | Replaced at an owner commit; mode changes advance sample generation and never mutate producer authority. |
| Live performance model | Application derives a read-only projection after joining | Editor panels and DevelopmentGame stat presenter | One latest immutable model plus fixed plotted windows | Replaced atomically/by owner publication; presenters retain no producer pointers or mutable spans. |
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

Metric and view descriptors are private fixed `constexpr` tables owned by Application; modules do not register rows dynamically. The implementation record must publish exact `sizeof` values and prove the total remains within the 4 MiB live-history cap. Changing frame-pipeline depth or worker count does not multiply live diagnostic storage implicitly; any per-worker producer scratch is separately fixed and included in that proof.

## Collection Modes And Cost Budget

| Mode | Intended use | CPU scopes | GPU scopes | Memory | Disk |
| --- | --- | --- | --- | --- | --- |
| `Off` | Shipping/default performance control where configured | Thread names only; no live aggregation. | Existing external markers follow their own build/CVar policy. | Product-required memory policy only. | None. |
| `LiveBasic` | Editor orientation | Fixed top-level host/render phases and bounded lane aggregates. | One top-level span per active queue; no detailed internal scopes. | Process sample at 1 Hz; RHI sample at its bounded cadence. | None. |
| `LiveDetailed` | Short interactive diagnosis | Top-level plus selected fixed subphases. | Frame-graph pass scopes and selected fixed detailed scopes. | Same cadence; optional high-water reset. | None. |
| `GpuProfileCapture` | One focused GPU frame investigation | Only configuration/bookmark CPU markers required for correlation. | One armed eligible frame with the full bounded stable GPU scope plan; frozen after delayed resolution. | Captured as configuration context only. | None unless attached to an explicit evidence export. |
| `Benchmark` | `MAP-00` and declared routes | Exact raw per-frame summary for the bounded sample request. | Valid top-level and per-pass values required by the workload. | Current and high-water values over the run. | Explicit manifest, raw timing, and summary artifacts only. |
| `ExternalCapture` | PIX/RenderDoc/Nsight/WPA/RGP | Stable trace markers and symbols; live UI may be hidden. | Stable backend markers; internal timestamp collection may be disabled to avoid observer overlap. | Tool-specific capture plus a matching manifest bookmark. | Native profiler artifact by explicit user action. |

Instrumentation acceptance targets for the first implementation are:

- zero per-frame heap allocations in `LiveBasic` after initialization;
- at most 4 MiB total bounded live-history storage with a documented exact layout;
- less than 1% change in CPU frame p50 and less than 2% change in CPU frame p95 between `Off` and `LiveBasic` on both Empty and Sponza, using identical valid runs;
- less than 1% or 0.1 ms, whichever is larger, change in GPU p95 from the top-level timestamp pair;
- measured and reported overhead for `LiveDetailed` and `Benchmark`; results from different modes are never compared silently.
- measured and reported CPU/GPU disturbance for `GpuProfileCapture`, including whether normal parallel command recording remained enabled.

The CPU percentage target is evaluated together with a predeclared absolute noise floor measured on Empty; a near-zero baseline must not turn timer noise into an apparent percentage failure or pass. The implementation record owns the exact absolute threshold and measurement procedure before acceptance.

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

Detailed GPU pass records are retained only in eight fixed slots, with at most 256 fixed scope records per frame across all queues. `LiveDetailed` uses all eight as a rolling ring. `GpuProfileCapture` pins one slot for the latest frozen result and leaves seven rolling slots; a later accepted capture replaces that pinned slot. Overflow invalidates only that frame's detailed view and preserves an independently valid top-level queue span. The benchmark writer streams a declared bounded run to its explicit artifact rather than retaining an unbounded in-memory history.

### Memory History

Memory is slow-changing relative to frame scopes. A separate ring stores at most 256 samples at a default 1 Hz cadence:

- current process working set and private commit;
- OS-reported process-lifetime peak working set/private commit where the platform exposes them, labeled as process-lifetime values;
- Sparkle session sampled high-water and benchmark-run sampled high-water for working set/private commit, each with sample identity and cadence;
- engine-tracked GPU used bytes and allocator block bytes;
- device-local API usage and budget;
- non-local/shared API usage and budget;
- transient bytes and retirement backlog;
- fixed GPU categories: texture, mesh, ray tracing, transient, upload, readback, constant buffer, other;
- sampled `FrameId`, monotonic timestamp, backend, validity, and source scope.

The target RHI memory contract must stop using a combined local plus non-local `TotalBudgetBytes` as the UI's "VRAM budget." The primary vocabulary is local/non-local segment or heap, process estimated API usage/budget, engine-tracked allocation bytes, and allocator block bytes. "VRAM" is allowed only as a compact presentation alias when the adapter memory architecture and backend scope make it defensible; UMA, host-visible/device-local, and shared-memory systems retain the precise terms. API budgets are mutable implementation estimates that may change because of system activity.

`Stat Reset` resets only Sparkle session sampled high-water and its generation. It cannot reset OS process-lifetime peaks. A benchmark run owns a separate sampled high-water so an earlier editor/load spike cannot be presented as the run peak. All sampled high-water values expose cadence and may miss peaks shorter than that cadence.

### Aggregation Rules

- The live window is the newest 120 joined `FrameId` summaries in the current generation. Each headline shows latest valid, p50, and p95 from all field-valid entries inside that fixed interval and displays the full population metadata defined above. Cross-domain rows derive a common-correlated cohort over the identical interval.
- Benchmark windows use the acceptance workload's current readiness, warm-up, sample-count, and run-count policy and record the resolved values in the manifest. The workload-owned analysis computes per-run p50/p95/p99/worst as primary results, a clearly secondary combined view, run-to-run variation, its declared correlation-aware uncertainty method, and regression verdict from raw integer samples; these do not require more live UI columns.
- Invalid, pending, stale, and disabled values do not enter a percentile.
- The initial percentile algorithm is nearest-rank over durations in integer nanoseconds; conversion/rounding happens only for presentation.
- Worst frame and declared tail events are preserved with `FrameId`; a percentile is never reconstructed from displayed rounded values. Worst values are directly comparable only when sample counts are equal or the count sensitivity is explicitly modeled.
- High-water marks reset only on explicit session reset/start and record the `FrameId` at which they occurred.
- Nested CPU or GPU scopes are not summed into a parent. Parent wall duration and child contribution are displayed separately.
- Cross-thread task durations are not added to host/render wall time. Busy worker time and wall critical path are different views.

## Bottleneck Classification

The live UI may show a `Likely` hint only when all required signals are valid, provenance-compatible, and present in the same sufficiently sized common-correlated `FrameId` cohort. It shows `WarmingUp` until that cohort reaches its declared minimum and `Unknown` when correlation or capability fails. It must also show the strongest supporting signals. `Confirmed` is reserved for a completed causal experiment/capture outside the live classifier.

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

## Diagnostics Product Information Architecture

The accepted product shape is a progressive diagnostic ladder, not a collection of profiler windows. Every Sparkle-owned presentation reads the same immutable joined diagnostics product and preserves the same `FrameId`, range, semantic token, units, configuration, and validity.

### Product Surfaces And Depth Boundary

| Surface | Depth | Product role | Boundary |
| --- | --- | --- | --- |
| Viewport/runtime Stats | Glance and live orientation | Answer whether the frame is healthy and which top-level domain deserves attention. | No arbitrary tracks, history browser, or causal claim. |
| Editor Performance workspace | Trend, ranking, selected frame/range, and bounded capture | Correlate frame distribution, physical CPU owners/logical phases, task lanes, GPU queues/passes, process RAM, and precise GPU memory fields. | One fixed workspace; no generic trace store, symbol engine, or allocation explorer. |
| External-profiler handoff | API/state, scheduling, memory allocation/residency, shader/hardware, and crash cause | Carry exact configuration and stable selected identities into PIX, RenderDoc, Nsight, RGP/RMV/RGA, WPA, or equivalent tools. | Sparkle does not reproduce vendor/source-level analysis. |

The viewport and DevelopmentGame overlay own no independent data model. The Performance workspace is available once from `Windows > Performance`, the viewport summary, the existing console commands, and a selected hitch. A new diagnostic question does not automatically justify a new panel.

The GPU Visualizer is the `GPU / Captured frame` mode of the Performance workspace, not another permanent Editor window. Hitches are selections in the shared frame navigator, not a Hitch Browser. A future memory A/B snapshot is a Memory view mode only if an accepted workload justifies it. Existing Used Shaders, Used Meshes, and Used Textures remain resource/asset inspectors until a separately accepted migration can consolidate and remove an old path; they do not own whole-system performance truth.

### Shared Selection Model

Exactly one workspace selection is active:

| Selection | Meaning | Cross-view behavior |
| --- | --- | --- |
| `Live` | Follow the newest completed joined sample. | All views advance together; memory retains its independently displayed sample age. |
| `FrameId` | Freeze one completed application/renderer frame identity. | CPU/GPU values from another frame never substitute silently; delayed GPU data remains Pending. |
| Bounded frame range | Aggregate valid joined samples in the visible range. | Every percentile/table declares included, excluded, invalid, and discontinuity counts. |
| Typed object token | One physical thread, logical phase, task lane/family, GPU queue/scope, or memory category within the selected frame/range. | Main view highlights it and the Inspector shows producer, owner, identity, metrics, validity, and next action. |

Switching Overview/CPU/GPU/Memory preserves the frame/range. A typed object token follows only when the destination has an explicit semantic correlation; otherwise the object selection clears while the frame/range remains. Search/filter hides rows but does not alter captured totals, denominators, percentages, or the selection identity.

### Performance Workspace Layout

```text
+ Performance ----------------------------------------------------------------+
| LIVE | Frame 18422 | D3D12 | 5120x1392 | DevEditor | Basic | valid 120/120  |
| [Overview] [CPU] [GPU] [Memory]           [Freeze] [Capture GPU] [Export...]  |
+ Frame navigator -------------------------------------------------------------+
| budget 16.7 ms ........... ^ hitch #18391 ........ selected #18422 ......... |
+ Main view ------------------------------------------------+ Inspector --------+
| summary / lanes / timeline / marker tree / memory trend   | selected identity |
|                                                           | owner + definition |
|                                                           | metrics + validity |
|                                                           | correlation/action |
+ Status ----------------------------------------------------------------------+
| likely GPU-limited | lost 0 | memory age 0.4 s | GPU resolved 2 frames late  |
+------------------------------------------------------------------------------+
```

The composition is fixed:

- The context ribbon shows source mode, selected frame/range, build/backend/adapter, render extent, renderer topology, collection mode, and data quality.
- The fixed view tabs are Overview, CPU, GPU, and Memory.
- The frame navigator stays visible in every view. Click selects a frame, drag selects a range, and Previous/Next Hitch moves the same selection.
- The main view changes representation without changing truth: summary, physical thread lanes, queue/marker timeline and hierarchy, or memory trend/categories.
- The Inspector is selection-driven and never polls engine state independently.
- The status footer keeps validity, loss, observer mode, and the most honest next action visible.

Arbitrary dashboard tiles, plugin tracks, detachable subpanes, stored custom layouts, and a view-registration API are rejected for the initial product.

### Fixed View Contracts

| View | Primary composition | Selection detail | Explicit escalation boundary |
| --- | --- | --- | --- |
| Overview | Frame budget/distribution, top-level CPU work and waits, GPU queue span, present policy, process RAM/GPU memory current/high-water, likely-domain explanation, configuration. | Selected frame/range identity, supporting observations, missing data, next action. | No call stacks, API events, resource contents, or hardware cause. |
| CPU | Physical Sparkle thread lanes first; logical phase blocks within their real owner; task lanes separate; bounded aggregate phase/task table. | Thread name/ID, phase/task token, wall/wait/ready/count statistics, source and validity. | OS running/ready/preempted state, arbitrary threads, stack sampling, callers/callees, and flame graphs require WPA/PIX/Nsight Systems. |
| GPU Live | Per-queue spans/timelines and bounded recent pass ranking. | Queue/pass token, frame identity, inclusive/exclusive when valid, work counts, resolution delay. | API state, resources, pipelines, barriers, shader counters, wave/cache/bandwidth cause require PIX/RenderDoc/Nsight/RGP. |
| GPU Captured frame | Frozen per-queue timeline synchronized with Hierarchy, Flat Inclusive, Flat Exclusive, or Coalesced marker table. | Full marker path, explicit parent, queue/batch/chunk, ticks, duration, draws/dispatches, validity, copy path. | Marker timing cannot prove unmarked idle, cross-queue critical path, or hardware limitation. |
| Memory | Process RAM and GPU tracked/block/local/non-local definition rows, time trend, fixed categories, budget/pressure, retirement, current/high-water/age. | Category/heap token, current/high-water/delta when supported, definition, ownership, data age. | Allocation call stacks, arbitrary heap query, fragmentation/page map, residency events, and leak verdict require external tools. |

Overview uses aligned rows and a compact trend rather than a wall of decorative cards. The largest displayed number does not become the limiting-domain verdict automatically because pipelined domains overlap and waits may be downstream consequences.

CPU lanes always distinguish physical execution from logical work. For example, gameplay in DevelopmentEditor is shown as a `Gameplay` phase on `Sparkle.EditorThread` when that is the actual topology; it is not relabeled as a physical `Game Thread` to resemble another engine. `Sparkle.RenderThread` is shown because Sparkle currently owns it, not because a renderer is required to have one. Render work could legally execute on the caller, a render thread, tasks, or a hybrid topology; diagnostics names must follow the implemented owner.

GPU queue timelines are not summed. Hierarchy nesting and inclusive/exclusive columns remain adjacent. Flat-inclusive and coalesced sums carry a visible double-counting warning. `Unaccounted` means the union outside known child scopes within a valid queue root; it does not mean idle or wasted work.

Memory keeps these definitions separate:

- process working set versus process private commit;
- engine-tracked used bytes versus allocator block/committed bytes;
- engine-tracked GPU resources versus API/OS local and non-local usage/budget;
- current value versus run high-water versus pending-retirement value.

Categories need not sum to an OS/driver total. Differences remain visible and are never assigned to a fabricated category merely to make a chart close.

### Workspace Interaction Contract

| Action | Required behavior |
| --- | --- |
| Click viewport summary | Open Performance Overview following the newest valid joined frame. |
| Click/step a frame | Freeze that `FrameId` and update every view from the same joined identity. |
| Drag a frame range | Aggregate only valid samples and expose inclusion/exclusion counts. |
| Click a CPU phase, GPU marker, or memory category | Set one typed selection and populate the shared Inspector. |
| Double-click a GPU marker | Expand its hierarchy path and zoom the selected queue timeline to its interval. |
| Freeze / Live | Change follow state only; collection demand changes only through a visible collection control. |
| Capture GPU | Arm the bounded next-frame capture and show Armed/Recorded/Resolving/Ready/Invalid in the existing GPU view. |
| Copy marker path | Copy the same stable semantic path emitted to external GPU markers. |
| Export | Start an explicit bounded evidence action; never enable continuous file output. |

The console and UI issue the same typed requests. UI does not construct command strings; command handlers do not reach into ImGui panels. Exact keyboard bindings remain a presentation implementation detail, but frame stepping, hitch stepping, search focus, timeline fit, and selection clear must be possible without precision mouse work.

### Visual, Validity, And Accessibility Rules

- Milliseconds lead; derived FPS is secondary. Binary byte units and percent denominators are explicit.
- Stable domain/queue/phase colors are shared across overlay, Overview, lanes, and capture. Color conveys category or budget state, never validity/cause alone.
- Gray text plus hatching/icon/text distinguishes Invalid, Pending, Stale, Unsupported, Dropped, Filtered, and no submitted work.
- Budget coloring applies only to a value with a declared budget: neutral below, warning above, critical above twice budget. A high utilization percentage is not automatically red.
- Configuration, observer mode, sample count, loss, and selected identity remain visible in screenshots and while scrolling.
- Stable row order is used for Overview. Rankings show the active sort key. Search does not cause rows or totals to jump semantically.
- Narrow overlays may abbreviate, but the workspace uses full names and tooltips state producer, owner, interval, unit, inclusion rule, validity, and collection cost.

Required non-numeric states are first-class UX:

| State | Presentation requirement |
| --- | --- |
| Warming up | `N / required valid samples`; percentile and likely-domain fields remain unavailable. |
| GPU resolving | Selected `FrameId`, delayed nonblocking state, and age/frame delay. |
| No submitted queue work | Named text distinct from unsupported and measured zero. |
| Stale memory | Last sample age and cadence; it is not displayed as frame-correlated. |
| Cap/loss | Retained/lost counts and reason; incomplete views remain visibly incomplete. |
| Discontinuity | Resize/reload/device/route reason and distribution exclusion rule. |
| External capture | Tool/mode and observer warning; any internal timestamp suppression is explicit. |
| Device loss/shutdown | One settled invalid capture result; no hidden block or silent disappearance. |

### Low-Clutter Functional Gate

A proposed row, group, or mode is admitted only when it has all of the following:

1. a named user question tied to an acceptance workload or a core orientation need;
2. one authoritative producer and definition;
3. a bounded immutable presentation model and explicit cost class;
4. an existing surface slot in Stats or one of the four Performance views;
5. a validity/failure presentation and observer-cost check;
6. a clear point where an external profiler becomes the better tool.

This gate rejects instrumentation because it is merely interesting. Stable owner boundaries, RDG passes, RHI queues, allocator authorities, and the task scheduler should provide most measurements automatically. Feature code adds a child scope only when the parent cannot answer an accepted diagnostic question. No diagnostics-only scan of scene entities, resources, passes, descriptors, or tasks is permitted.

## Sparkle Stat Views And Commands

The Unreal-inspired surface is a set of fixed views over the existing joined diagnostics product. It is not a second collector. A stat group declares which already-owned fields it presents and the minimum collection mode it demands. Multiple visible groups share one sample.

### Stat Interaction Contract

The existing console grammar can support one root command in both Editor and DevelopmentGame:

| Command | Behavior |
| --- | --- |
| `Stat` | Prints active groups, collection mode, sample count, and compact usage. |
| `Stat List [filter]` | Lists the fixed available groups and their collection cost. Autocomplete uses the existing console registry. |
| `Stat <group> [On\|Off\|Toggle]` | Changes one group's visibility. Omitting the action toggles it. |
| `Stat Preset <Quick\|Cpu\|Gpu\|Memory\|Portfolio>` | Replaces the active group set with one fixed, documented preset. |
| `Stat None` | Hides all stat overlays and releases their demand at the next owner commit. Benchmark, external-capture, or open Performance-view demand remains independent. |
| `Stat Reset` | Starts a new live sample-window generation and clears live high-water/worst-frame presentation. It does not reset allocator or engine authority. |
| `Stat Dump [group]` | Prints one bounded immutable compact snapshot to the existing console output, capped at 64 rows across active groups. It does not create a file or begin a capture. |

Command names are displayed in canonical Sparkle casing but remain case-insensitive because the existing registry is case-insensitive. The Editor viewport Stat menu sends the same typed requests as the command handler; it does not construct command strings. Unknown groups, unavailable build capabilities, and invalid actions return one actionable console result.

External capture and workload benchmark start/stop remain separate explicit workflows. `Stat Dump` must never become an accidental unbounded `StartFile` equivalent.

### Fixed Group Catalog

| Group | Primary diagnostic question | Rows | Minimum collection |
| --- | --- | --- | --- |
| `Fps` | What is the derived display rate? | Derived FPS, latest frame interval, frame budget. | Existing host clock; orientation only. |
| `Unit` | Which top-level domain is consuming or stalling the frame budget? | Frame p50/p95, host/game phase wall, render work, producer/render waits, graphics GPU span, present policy, likely-domain hint. | `LiveBasic`. |
| `UnitGraph` | Is the limiting domain stable or spiking over time? | Fixed 120-frame graph for Frame, host non-wait work, render non-wait work, GPU graphics span, and budget line; invalid/pending samples have gaps. | Same `LiveBasic` data as Unit. |
| `Threads` | What did Sparkle's named physical threads do in the selected completed interval? | Editor/Game, Render, and fixed task-worker lanes; phase wall, classified wait, occupied ratio, ready delay, longest stable task label, and data age. | `LiveBasic`; OS running/ready/wait still requires a system trace. |
| `Tasks` | Is task scheduling, imbalance, or a named task family material? | Per-lane task count, occupied wall, ready-delay p95, caller-join wall, longest stable task label, failures/cancellations. | `LiveBasic` fixed aggregates; no task history table. |
| `Gpu` | Which GPU queue controls the current frame budget? | Graphics/compute/copy top-level span, validity, resolution delay, queue overlap notice, and presentation context. | `LiveBasic`. |
| `GpuPasses` | Which stable render passes consume the selected queue span? | Bounded live ranking from recent resolved frames with top-level inclusive/exclusive cost, shallow hierarchy, percent of queue span, owned draw/dispatch counts, and unaccounted span. It is not the frozen tree navigator. | `LiveDetailed`; latest eight frames, 256 scope-record cap per frame. |
| `Render` | Is renderer CPU orchestration or submitted workload unexpectedly large? | Setup/prepare/extract/cull/graph setup/compile/record/submit/present wall; recording groups, passes, submissions, draws, dispatches, pipeline/shader-package/RT-build counts, barriers/transitions, upload bytes, and rejected work where already produced. | `LiveBasic`; counters are admitted only at their production owner without a diagnostic rescan. |
| `Scene` | What scene cardinality reached each stage of the render path? | Extracted, accepted, visible, submitted, and rejected instances; meshes/materials/lights; triangle/index counts where meaningful; RT instances and BLAS/TLAS counts; dirty/upload counts. | `LiveBasic` after immutable owner counters exist. Never query live ECS or renderer caches from UI. |
| `Rhi` | What backend work and allocator pressure did Sparkle submit? | Backend/adapter, queue submissions, command-recording groups/lists, pipeline creations/cache state, descriptor and barrier counts where owned, upload/readback bytes, timestamp capacity/loss, tracked/block/local/non-local memory. | `LiveBasic`; neutral facts only, no native handle/type leakage. |
| `Memory` | Is RAM or GPU memory growing, near budget, fragmented, or awaiting retirement? | Working set/private commit, tracked used/allocator blocks, local/non-local usage/budget, categories, transient/RT scratch and result bytes, upload/eviction/residency/missing-event counters, retirement backlog, current/high-water/age. | Default 1 Hz memory sampling plus owned event deltas; allocation/residency detail remains external. |
| `Hitches` | Which recent frames exceeded a declared budget and which domain was implicated? | Last 16 qualifying `FrameId` values, interval, likely domain, worst phase/pass or compile/load/pipeline token, discontinuity, and validity. | `LiveBasic`; annotations only when an owner emitted them; no automatic file output. |

The catalog is intentionally much smaller than Unreal's because Sparkle does not yet own animation, audio, networking, streaming, or other production diagnostic products at comparable maturity. A group is added only with a present user question, authoritative producer, bounded row model, cost classification, and removal/review owner.

Presets are aliases for these exact sets:

| Preset | Groups | Intended question |
| --- | --- | --- |
| `Quick` | Unit | What should I inspect next? |
| `Cpu` | Unit, Threads, Tasks | Is host/render/task CPU work or waiting controlling the frame? |
| `Gpu` | Unit, Gpu, Render | Is GPU work dominant, and did renderer workload or CPU preparation change? |
| `Memory` | Unit, Memory, Rhi | Is process or GPU memory under pressure, growing, fragmented, or awaiting retirement? |
| `Portfolio` | Unit, Threads, Gpu, Memory | Produce one readable orientation screenshot; use the Performance window and captures for detail. |

No preset enables `GpuPasses` implicitly because detailed timestamp overhead must be a visible user choice.

### Delivery Tiers

The fixed catalog is a bounded destination, not permission to implement every group before the evidence harness. Delivery is gated by the first accepted consumer:

| Tier | Required product subset | Acceptance consumer | Deferred until evidence exists |
| --- | --- | --- | --- |
| A: Evidence spine | `Fps`, `Unit`, `UnitGraph`, minimal `Threads`, `Gpu`, `Memory`, Overview, configuration/validity, explicit benchmark export. | `MAP-00` on Sponza calibration. | Detailed pass tree, optional scene counters, RT structure detail, hardware counters. |
| B: Measured frame | Full CPU/GPU/Memory views; `GpuPasses`, `Render`, `Rhi`, `Hitches`; `ProfileGpu`; benchmark evidence families and native capture handoff. | `WL-04`, `CASE-02`, first bottleneck study. | A general trace store, arbitrary allocation explorer, embedded source/ISA, vendor SDK. |
| C: Workload-driven additions | `Scene`, task-family detail, RT and compilation/residency rows already justified by an accepted study; adoption guidance. | `CASE-03`, `CASE-04`, `CASE-05`, remaining studies. | Any row without an owner, falsifiable question, and measured usefulness. |

`Tasks` remains available in the fixed design but is not a Tier A blocker; the minimal CPU view can expose worker-lane aggregates needed for `MAP-00`. A group moves earlier only when the current measured bottleneck needs it. A late feature is deleted or left external when its implementation and observer cost exceed its diagnostic value.

### Row And Presentation Semantics

- Duration rows show latest, p50, p95, and maximum in milliseconds over the declared valid window.
- Per-frame count rows show latest, p50/p95 where useful, and maximum; they never carry an `ms` suffix.
- Byte rows show current, run high-water, budget/limit when defined, sample age, and binary units.
- Ratio rows name the denominator, such as occupied wall / sample-window wall or pass duration / same-queue outer span.
- State rows show stable enum text rather than encoding state as zero or color alone.
- Every row tooltip identifies producer, physical/logical owner, interval, unit, inclusion/exclusion rule, validity, and minimum collection mode.
- Parent duration and child contribution are distinct columns. Nested or overlapping CPU/GPU rows are never presented as an additive total.
- GPU unattributed span is the outer queue span minus the interval union covered by valid top-level scopes, never the outer span minus a sum of nested durations.
- A count is shown only when its production owner can increment it as part of normal work. The diagnostics path does not scan scene resources, passes, descriptors, or tasks merely to populate a stat.

### Demand, Cost, And Composition

- The active group set is bounded to four simultaneous compact overlays. The Editor Performance window can inspect every fixed view without placing all rows over the scene.
- Enabling a fifth compact group fails with the active set and `Stat None`/preset guidance; it never evicts a group implicitly. Presets replace the active set atomically.
- A compact group shows at most 16 rows. A bounded `+N hidden; open Performance` row replaces overflow. Stable priority precedes duration sorting so important validity/configuration rows do not jump.
- The union of overlay groups, the open Performance view, and benchmark requests determines the minimum collection mode. `GpuPasses` visibly promotes the session to `LiveDetailed`; releasing the last detailed request returns to `LiveBasic` after the generation boundary. `ExternalCapture` is an explicit override and may suppress overlapping internal timestamps as declared in the banner.
- `ProfileGpu` is an explicit one-shot demand outside the four-overlay limit. It pins one resolved detailed slot as the frozen product while armed/resolving and releases or replaces it only through the capture-state contract; it never makes continuous full-tree collection implicit.
- `UnitGraph` reuses the joined ring and cannot allocate another time-series history. Presets are fixed group sets, not stored user-authored layouts.
- Presenters may refresh every UI frame, but process/GPU memory retains its slower source cadence and visible age. Presentation never increases source polling silently.
- Overlay closed/open measurements are part of the observer-cost test. The banner always shows `Basic` or `Detailed`, sample count, invalid/lost count, and resolution.
- Shipping builds default to no live stat presentation. Exact build eligibility remains an implementation decision and must not make unavailable values look valid.

### Visual Design Handoff

The canonical `Unit` and `GpuPasses` presentations live in [Performance Diagnostics Visual Design And Tool Wireframes](PerformanceDiagnosticsAsciiWireframes.md#compact-stat-tools). Their values are illustrative. The pass hierarchy, counts, and timings must come from one correlated frame; the display must not combine rolling CPU values with an unrelated latest GPU frame without labeling both identities.

### What Built-In Stats Can Settle

| Question | Built-in stats can be sufficient when | Escalate when |
| --- | --- | --- |
| CPU-side, GPU-side, presentation, or memory pressure? | Unit/Threads/Gpu/Memory remain correlated and one controlled configuration change moves the expected domain. | Signals disagree, data is pending, or a wait/overlap controls the critical path. |
| Which render pass family dominates? | One stable top-level pass repeatedly consumes the valid queue span and disabling its feature removes comparable time without changing the workload unfairly. | The reason inside the pass requires shader, wave, cache, bandwidth, barrier, or API-state evidence. |
| Editor overhead or engine workload? | Matched DevelopmentEditor and DevelopmentGame runs isolate Editor/UI work at identical render extent and settings. | OS scheduling, DWM/compositor, driver, or third-party work remains plausible. |
| Too much submitted work? | Render/Scene/Rhi counts reveal a reproducible cardinality jump tied to a route or feature. | The generating call path, visibility error, command redundancy, or backend behavior is unclear. |
| Process/GPU memory growth or pressure? | Memory categories, tracked/block distinction, local/non-local budget, and retirement return-to-baseline behavior answer the lifecycle question. | Allocation call stacks, untracked driver memory, residency/eviction, or fragmentation cause is required. |

This is the intended reduction in profiler dependence: common orientation and controlled before/after questions stay in Sparkle. Specialized tools remain necessary when the question changes from "which domain or workload changed?" to "which call stack, scheduling edge, shader/hardware unit, allocation site, or API dependency caused it?"

## On-Demand GPU Visualizer

### Product Decision

Sparkle will provide a focused GPU Visualizer mode for one captured frame inside the Performance workspace. It reuses the same frame-graph scope vocabulary as `Stat GpuPasses` and external GPU markers, but freezes a richer immutable result so the user can navigate hierarchy, timeline, inclusive cost, exclusive cost, and repeated marker instances without an attached tool. `GPU Visualizer` is a recognizable capability name, not a separate panel or data owner.

This follows two useful Unreal patterns without copying its implementation wholesale:

- Epic documents realtime `stat gpu` as cumulative and non-hierarchical for quick monitoring.
- Epic's profile visualization surfaces support hierarchical, flat, coalesced, inclusive, and exclusive views.
- RDG owns scopes for both in-engine GPU statistics and external-profiler events.

The adopted behavior is a quick live view plus a deliberate detailed frame capture. The rejected behavior is an always-running full tree or a claim that timestamps provide resource state, shader counters, wave occupancy, cache/bandwidth attribution, or draw debugging. See Epic's [realtime GPU profiler guidance](https://dev.epicgames.com/documentation/unreal-engine/vr-performance-testing-in-unreal-engine#real-time-gpu-profiler), [ProfileVisualizer API](https://dev.epicgames.com/documentation/unreal-engine/API/Developer/ProfileVisualizer), and [RDG profiling scopes](https://dev.epicgames.com/documentation/unreal-engine/render-dependency-graph-in-unreal-engine#performanceprofiling).

### GPU Capture Interaction Contract

`ProfileGpu` is a separate command because a frozen query capture has different cost and lifetime from a live stat group:

| Command | Behavior |
| --- | --- |
| `ProfileGpu` or `ProfileGpu Capture` | Arms the next valid rendered frame, returns a `GpuProfileCaptureId`, and opens Performance/GPU in captured-frame mode when delayed results resolve. DevelopmentGame retains the result and prints a compact readiness summary. |
| `ProfileGpu Open` | Opens Performance/GPU on the latest retained frozen capture in Editor or shows its compact summary in DevelopmentGame. |
| `ProfileGpu Frame <FrameId>` | Freezes an already-resolved frame from the eight-slot detailed ring when available; it does not pretend an uncollected past frame can be recovered. |
| `ProfileGpu Cancel` | Cancels an armed request. If GPU work was submitted, resolution still retires safely but its product is discarded. |
| `ProfileGpu Clear` | Releases the retained frozen result after presenter publication and returns Performance/GPU captured-frame mode to its empty state. |

Only one request may be armed or resolving. A second request returns `Busy` with the active capture ID; it does not replace an explicit capture silently. A request fails immediately with a typed reason when timestamp queries are unavailable, the renderer has no valid output extent, an external capture owns the instrumentation mode, or the fixed scope plan cannot be created.

The Editor toolbar may expose `Profile GPU` and the Performance/GPU view may expose `Capture frame`; both submit the same typed request as the command. UI code does not construct command strings. No command writes a file by default.

### Capture State And Publication

```text
Idle
  | ProfileGpu -> CaptureId
  v
Armed --cancel------------------------------> Cancelled -> Idle
  | next valid rendered frame selects FrameId
  v
Recorded/Submitted --shutdown/device loss--> Invalid result -> Idle
  | frame slot and GPU completion retire
  v
Resolving --validation/query failure--------> Invalid result -> Idle
  | immutable result published
  v
Ready/Frozen --Clear or next accepted capture--------------> Idle
```

- Application owns user request identity and presentation state; Renderer owns frame selection, scope recording, delayed resolution, hierarchy validation, and derived timing values.
- `GpuProfileCaptureId`, `FrameId`, sample generation, backend, and renderer configuration all cross publication. A frame-in-flight slot is never capture identity.
- The producer never blocks EditorThread/GameThread for GPU completion. The result appears as `Armed`, `Resolving`, `Ready`, `Invalid`, or `Cancelled` with age and reason.
- The request captures the next frame with a valid render extent. Loading, resize, shader reload, validation, and other discontinuities remain visible tags; the visualizer never quietly substitutes a later "better" frame.
- Shutdown and device loss settle the request exactly once. Late resolution cannot republish after clear, replacement, or product destruction.

### Scope Identity And Fixed Record

Each frame-graph compile produces a bounded immutable GPU scope plan. The plan records the marker-schema version used to resolve tokens and display paths. The hot record uses capture-local indices for contiguous addressing and stable tokens for identity:

```text
GpuProfileScopeRecord
  ScopeToken                 stable semantic aggregation identity
  ParentScopeIndex           capture-local parent, never durable identity
  QueueType                  Graphics / Compute / Copy
  SourceKind                 QueueRoot / Batch / RecordingChunk / FrameGraphPass / PassDetail
  SubmissionBatchIndex       capture metadata
  RecordingChunkIndex        capture metadata
  LocalSequence              deterministic order inside the exclusive chunk slice
  BeginTicks / EndTicks      native queue timestamps
  InclusiveTicks             derived after validation
  ExclusiveTicks             derived after direct-child interval union
  DrawCount / DispatchCount  counts already produced inside this exact scope
  Validity                   explicit state/reason
```

Display names live in one bounded capture dictionary keyed by `ScopeToken`; hot records do not own strings. The identity policy is:

- synthetic queue roots use fixed tokens;
- structural batch/chunk instances use fixed kind plus capture-local submission metadata;
- frame-graph passes use a stable semantic pass token and retain pass index only as selected-frame metadata;
- pass-internal scopes use an owner-declared stable child token;
- repeated instances share an aggregation token but retain distinct capture-local instance indices and call ordinals;
- a runtime label that cannot resolve to a bounded stable token before recording appears under a fixed `Other` token or is marker-only; it does not create an unbounded dictionary entry.

The token registry is generated or compile-time declared from owner-local definitions. Its verification rejects collisions, missing paths, transient identity components, and schema drift without a version change. Captures preserve the schema version so a later build never resolves an old token silently to a different semantic region.

The current `FrameGraph/<Kind>/<Index>/<PassName>` event label remains useful for external navigation, but the numeric pass index is not the cross-frame aggregation identity because graph composition can change it.

### Preserving Parallel Command Recording

The accepted target must not reproduce the current hidden behavior where GPU timing disables parallel frame-graph recording.

```text
compiled FrameGraphPlan
        |
        v
bounded GpuScopePlan partitioned by (queue, batch, recording chunk)
        |
        +--> preassigned query pairs + exclusive record slice --> recording task 0
        +--> preassigned query pairs + exclusive record slice --> recording task 1
        `--> preassigned query pairs + exclusive record slice --> recording task N
                                                                  |
task join / normal submission order -------------------------------+
        |
        v
deterministic merge by (queue, batch, chunk, local sequence)
        |
GPU completion / frame-slot retirement
        |
        v
resolve ticks -> validate tree -> derive inclusive/exclusive -> publish
```

- Query pairs and fixed record ranges are assigned before recording tasks start.
- Each task writes only its chunk-owned slice and maintains a chunk-local parent stack. No task pushes into a shared vector or uses completion order as hierarchy.
- Parent indices are made explicit during deterministic merge; display depth is derived from the validated parent chain rather than treated as identity.
- Query allocation/reset occurs at its RHI owner before parallel recording. Recording tasks only write their preassigned query commands through their exclusive `RhiCommandRecordingLease`.
- The selected frame preserves the normal renderer mode, pipeline depth, recording policy, task worker policy, submission batches, and queue dependencies.
- If a backend cannot safely preserve that topology, representative `ProfileGpu` is `Unavailable` for that backend until fixed. An explicitly labeled serial/reference diagnostic may exist for debugging, but cannot be silently compared with normal runs.

The fixed 256-scope cap requires at most 512 timestamps across all queues for the detailed frame. The implementation still checks backend-specific per-queue capacity before arming because scope distribution, synthetic roots, and unavailable queue timestamp support matter more than the aggregate number.

### Inclusive And Exclusive Calculation

Calculations operate in integer ticks; milliseconds and percentages are presentation conversions.

For every valid scope instance `s` on one queue:

```text
inclusiveTicks(s) = unwrappedEndTicks(s) - unwrappedBeginTicks(s)

childCoverageTicks(s)
  = length of the interval union of valid direct-child intervals clipped to s

exclusiveTicks(s) = inclusiveTicks(s) - childCoverageTicks(s)
```

The resolver performs these steps per queue:

1. Validate timestamp support, period, valid-bit range, capture generation, and `FrameId`.
2. Unwrap native ticks relative to that queue's outer begin timestamp. Reject a span whose ordering cannot be proven across wrap.
3. Validate every explicit parent index: same capture, same queue, no cycle, and child interval contained by parent interval.
4. Sort direct-child intervals by begin tick and compute their union. Significant sibling overlap or child escape invalidates the affected subtree rather than producing negative exclusive time.
5. Derive inclusive/exclusive ticks, then convert using that query's timestamp period.
6. Create one synthetic root per active queue from the earliest valid begin to latest valid end. Root exclusive is labeled `Unaccounted`, not `Idle` or `Waste`.

For a valid leaf, exclusive equals inclusive. For a parent, a large inclusive value with small exclusive value means marked children explain most of the interval. A large exclusive value means the parent contains substantial unmarked commands, waits/bubbles, or work; it does not prove the shader body itself is expensive.

Graphics, compute, and copy produce independent trees. Sparkle does not subtract a compute child from a graphics parent, sum queue roots, or align queue-relative timelines on one axis without validated cross-queue clock calibration. Submission dependencies may be listed as metadata without inventing calibrated overlap.

### Views And Coalescing

The frozen capture supports four bounded views over the same records:

| View | Meaning |
| --- | --- |
| Hierarchy | Every scope instance under its explicit parent for the selected queue. Default sort is execution order. |
| Flat Inclusive | Individual instances sorted by inclusive duration. Nested rows can double count and carry a visible warning. |
| Flat Exclusive | Individual instances sorted by exclusive duration, useful for finding uninstrumented/self contribution. |
| Coalesced | Instances sharing `ScopeToken` are grouped with call count, sum/average/max inclusive, and sum/average/max exclusive. Coalesced sums are not a frame total. |

`Expand hot path` follows the largest valid child contribution from the selected node. Search filters display names without changing totals. Structural queue/batch/chunk rows can be hidden, but their timing remains part of the parent chain. Marker-only barrier/draw annotations may appear in details and counts but never receive fabricated milliseconds.

### GPU Visualizer Presentation

The captured-frame layout, columns, inspector, and navigation controls live in the [GPU Captured Frame wireframe](PerformanceDiagnosticsAsciiWireframes.md#gpu-captured-frame). The visual example does not establish Sponza's actual pass cost; the architecture in this section remains authoritative for record and calculation semantics.

### Relationship To Live Stats And External Tools

- `Stat Gpu` answers which queue is expensive over time.
- `Stat GpuPasses` shows a bounded live ranking from recent resolved detailed frames.
- `ProfileGpu` answers which marked region owns inclusive/exclusive time in one selected frame.
- PIX/RenderDoc answer which API events, resources, descriptors, pipelines, barriers, draws, and outputs produced the region.
- Nsight Graphics/RGP answer which shaders, hardware units, waves, occupancy, bandwidth, caches, or synchronization mechanisms explain the region on captured hardware.

Selecting a captured-frame node exposes `Copy marker path` so the same stable label can be located in PIX, RenderDoc, Nsight, or RGP. A Sparkle capture is sufficient for marker-level attribution and rapid feature controls; it is not causal proof below the marker boundary.

## Editor Presentation

### Viewport Summary

The existing top-right FPS text becomes the compact `Stat Unit` summary when that group is active. Milliseconds lead; FPS remains a derived convenience. A nearby Stat menu lists the same fixed groups and presets as the console command with visible checkmarks and collection-cost badges. The canonical [viewport control and compact `Unit` layouts](PerformanceDiagnosticsAsciiWireframes.md#shared-controls) live in the visual-design document.

Rules:

- A value without enough valid samples shows `warming up (N/120)`.
- GPU timing, process RAM, and GPU memory fields show `N/A`, `Disabled`, `Pending`, or age when appropriate.
- The resolution shown is render resolution, not inferred desktop/window size; output and client extent are in the details view.
- Color indicates budget state, not generic "good/bad": neutral below the configured budget, warning above it, critical above twice it, gray when invalid.
- Clicking the summary opens the Performance window at the correlated latest valid frame.
- FPS is `1000 / p50 frame interval ms` and is labeled as derived; it is not averaged from instantaneous FPS samples.

### Performance Window

One window serves the Editor presenter and expands the same immutable model used by compact stat groups. DevelopmentGame uses compact overlays only. The window is not a dockable general diagnostics platform and does not own a second group catalog. The visual-design document owns the [workspace shell and fixed view layouts](PerformanceDiagnosticsAsciiWireframes.md#performance-workspace-tools). All values in those layouts are illustrative only; they are not measurements of the supplied Sponza screenshot or evidence of a Sparkle bottleneck.

### Interpretation Examples

These patterns are also illustrative. They demonstrate the reasoning the system must support, not expected Sponza results.

| Observed correlated pattern | Honest orientation | Required confirmation |
| --- | --- | --- |
| Frame p95 `181 ms`; host non-wait phases `8 ms`; render non-wait phases `19 ms`; graphics span `172 ms`; producer backpressure `153 ms`; present throttle off. | Likely GPU-limited. Gameplay and editor UI are not large enough to explain the interval. | PIX Timing plus one PIX/Nsight/RGP GPU trace; rank pass and verify queue occupancy/synchronization. |
| Frame p95 `129 ms`; renderer frame-graph compile `94 ms`; graphics span `9 ms`; producer backpressure `0 ms`. | Likely render-CPU-limited in graph compilation. | WPA/PIX CPU samples and a compile-scope call tree; compare a serial/reference topology control. |
| Frame p95 `43 ms`; gameplay systems `35 ms`; render CPU `7 ms`; graphics span `10 ms`; FrameCritical ready delay `8 ms`. | Likely gameplay/task critical path with scheduling delay. | WPA precise scheduling plus `SparkleTasks` dependencies; test 1/2/N and serial controls. |
| Frame timing is stable; private commit rises every load/unload; GPU tracked used returns to baseline but allocator blocks remain high. | Possible CPU leak plus expected/uncertain GPU pool retention; do not call both leaks. | CPU allocation call-stack capture and a repeated post-retirement route; inspect allocator reuse/fragmentation separately. |
| Warm p50 is healthy but p99/worst coincide with new shader-package tokens and pipeline creations; the next traversal is stable. | Likely compilation/pipeline-cache hitch, not a steady GPU pass regression. | CPU/API timing trace with cold/warm cache states; repeat after cache invalidation and after a fully warm control. |
| RT build time and scratch/result high-water rise with instance count, while the selected ray dispatch also shows a traversal-pressure change. | RT structure/build policy and traversal are competing causes. | Record BLAS/TLAS build/update/compaction facts; inspect structure/traversal in RRA/PIX/Nsight/RGP and change one geometry/instance/build policy. |

The window has four fixed views:

| View | Shows | Does not show |
| --- | --- | --- |
| Overview | Budget, frame distribution, likely domain, current configuration, next profiler action. | Per-function or per-draw detail. |
| CPU | Physical Sparkle thread lanes, fixed logical phase wall time, known owner waits, and task-lane occupied/ready-delay aggregates. | OS run/ready/wait scheduling, a flame graph, every task, or arbitrary external thread attribution. |
| GPU | Per-queue top-level spans, latest bounded pass ranking, and Live/Captured-frame modes with one `Capture frame` action. | Hardware counters, shader ISA, resource contents, or uncalibrated cross-queue total. |
| Memory | RAM definitions/trend/high-water plus local/non-local GPU memory and fixed categories. | Allocation call stacks or a permanent largest-resource browser. |

### Required Configuration Banner

Every screenshot or exported summary includes:

- commit/build identity and `DevelopmentGame`/`DevelopmentEditor` configuration;
- D3D12/Vulkan, adapter, device ID where available, driver, and validation state;
- scene/level, camera route, readiness state, render path/provider/fallback;
- render/output/client resolution, VSync/presentation mode, dynamic-resolution state;
- threaded/serial renderer, pipeline depth, task serial/worker counts;
- diagnostics mode, warm-up frames, measured frames, run count, and collection overhead result.
- engine/content/configuration hashes and the reference/comparison-system role when the artifact is benchmark evidence.
- measurement provenance, valid/original/excluded population counts, elapsed span, and sample generation for every headline comparison;
- external capture tool/version and capture/replay/system-derived mode when an imported field or screenshot comes from a native tool;
- profiling-build identity, symbol/shader-debug package hashes, marker-schema version, and object-name eligibility for an external capture.

Without this banner, a number is orientation only and cannot be promoted into portfolio evidence.

## Acceptance-Workload Diagnostic Contract

The Performance workspace is a live diagnostic product. The acceptance-workload package is a stricter evidence product. They share metric definitions and immutable identities, but they are not the same schema and must not be forced into the same screen.

| Depth | Product | Required statistics | Retention/output | Claim strength |
| --- | --- | --- | --- | --- |
| Live orientation | Stats and Performance workspace | Latest, rolling p50/p95/max, sample count/span, current/high-water memory, validity and observer state. | Bounded in-memory rings; no file by default. | Chooses the next question; never proves causality. |
| Focused frame/range | Frozen frame/range and `ProfileGpu` | Selected-frame/range timing, inclusive/exclusive marker hierarchy, counts, validity, configuration. | One retained capture plus bounded recent detail. | Attributes marked time; does not prove hardware cause. |
| Benchmark evidence | `MAP-00` and accepted workload route | Per-run and combined p50/p95/p99/worst, run-to-run variance/uncertainty, high-water identity, event/count totals, comparisons and threshold verdict. | Explicit raw samples, summary, manifest, capture links under the workload owner. | Supports a reproducible performance claim after controls and quality checks. |
| Specialist capture | Native external tool | Scheduler/API/resource/counter/source/ISA/allocation/residency/crash facts. | Tool-native artifact linked from the same manifest. | Supports a scoped cause on the captured platform/configuration. |

### Benchmark Evidence Families

Each accepted run records the following families where the workload declares them applicable. `Unavailable`, `Unsupported`, and `NotInstrumented` are valid states; omission or zero is not.

| Family | Required measurements and identities | Primary presentation |
| --- | --- | --- |
| Frame, pacing, latency | CPU begin-to-begin and submitted-frame interval, GPU per-queue span/overlap, present/throttle classification, discontinuities, p50/p95/p99/worst. Input-to-present requires a real `InputSampleId -> simulation FrameId -> submitted PresentId -> displayed/presented result` chain; until that chain exists the field is `NotInstrumented`, never estimated from frame time. | Overview distribution/threshold plus raw samples; system/PIX timing trace for displayed latency and pacing cause. |
| CPU ownership | Host/game/editor, extraction, culling, frame setup, graph setup/compile, command recording, submit, present; physical thread, logical phase, wait/backpressure, tasks, ready delay, critical interval, serial/1/2/N worker policy. | CPU lanes/aggregates and workload table; WPA, Nsight Systems, or uProf for scheduling/stacks/microarchitecture. |
| GPU execution | Graphics/compute/copy queue spans, overlap, stable pass/child markers, inclusive/exclusive time, unaccounted span, synchronization identity, delayed-result validity. | GPU live/captured views; PIX/Nsight Graphics/RGP for API and hardware cause. |
| Workload cardinality | Passes, submissions/command lists, draws, dispatches, pipeline and shader-package counts, descriptor writes/binds where owned, barriers/transitions, instances/triangles, uploads, rejected work. | Render/RHI/selected marker tables and raw benchmark row. |
| CPU memory | Process working set and private commit, tracked allocator used/committed where owned, high-water identity, retirement/deferred-free state, load/unload checkpoints. | Memory trend/category table; allocation-stack tool for lifetime cause. |
| GPU memory and residency | Tracked used and allocator blocks, local/non-local usage/budget, committed/resident high-water, transient/upload/readback, uploads/evictions/mip-residency/missing-resource events, retirement. | Memory view and checkpoint table; PIX memory, RMV, or vendor trace for allocation/residency detail. |
| Ray tracing | BLAS/TLAS count and source geometry/instances, build/update/compaction time, scratch/result bytes, rebuild policy, traversal-sensitive route/configuration, RT dispatch and shader identity. | Selected Render/Memory rows and evidence table; RRA, PIX, Nsight, or RGP for structure/traversal cause. |
| Compilation and loading | Cold launch/load, warm load, time to first correct frame, shader compile/package-cache hit/miss, pipeline creation/cache state, asset/upload events, hitch `FrameId` and stable operation token. | Frame annotations/Hitches and evidence table; CPU/system/API capture for cause. |
| Concurrent/background work | Shader compilation/package work, asset I/O/upload, streaming/residency, editor composition, capture/tool activity, and other named engine workers active during the interval; explicit `Quiescent` only when the readiness contract proves it. | Context annotations and manifest; system trace for interference/critical-path cause. |
| Quality and comparability | Scene/route, reference image, settings, sample/reconstruction mode, dynamic resolution, output extent, validation, worker/topology, API, hardware/driver/build/content/config hashes. | Configuration banner, manifest, and before/after table. |

The live product admits a field only when a production owner can publish it within the cost/bounds rules. The benchmark schema may stream additional bounded event/count records during an explicit run. It still may not scan the ECS, renderer caches, descriptors, allocations, or native resources from the UI. Missing production facts remain a visible instrumentation gap and cannot be inferred from neighboring metrics.

### Input-To-Display Measurement Boundary

Input-to-display remains `NotInstrumented` until one bounded identity chain proves `InputSampleId -> simulation FrameId -> PresentId -> displayed result`. CPU begin-to-begin, queue submission, `Present` return, or average frame time are not substitutes. Presentation timing is a separate capability and observer mode because platform/API feedback may allocate queues, delay result availability, expose only some display stages, or require external instrumentation.

Candidate paths have different evidence strength:

| Path | Strength | Limitation / decision |
| --- | --- | --- |
| PresentMon/ETW system trace | Cross-API Windows pacing and displayed-event correlation without embedding a vendor SDK. | Vulkan accuracy/correlation may be reduced relative to DXGI paths; record provider and confidence. Initial external evidence option. |
| DXGI frame-latency/presentation data | Strong Windows D3D/DXGI integration and presentation identity. | D3D/DXGI-specific and still requires the input/simulation join. Initial D3D platform option. |
| Vulkan present-timing capability | API feedback can identify supported presentation stages and time domains. | Capability/surface dependent; the result queue is bounded and can perturb/fail when undersized. Admit only behind an explicit capability and observer mode. |
| Vendor latency SDK | May expose a deeper engine-to-display chain on supported hardware. | Vendor dependency, configuration coupling, and cross-vendor semantic gaps. Defer until an accepted workload needs it. |
| Optical/high-speed measurement | Measures photons and can validate the complete external path. | Specialized equipment and route automation; difficult to attribute internal stages. Use as validation evidence, not a live metric owner. |

Current tool/version caveats and source links live in the [profiler runbook](DiagnosticsProfilerRunbook.md#input-to-display-options).

### Comparison And Regression Contract

Every optimization result is a comparison between accepted runs, not between hand-picked screenshots. The workload-owned analysis must:

1. reject incompatible route, resolution, render settings, readiness, backend intent, diagnostic mode, or engine/content/configuration identity unless the changed field is the declared experiment variable;
2. retain all valid raw samples and per-run results, report exclusions/discontinuities, and calculate p50/p95/p99/worst plus the workload's selected uncertainty method;
3. state the predeclared hypothesis, competing cause, serial/control case, one scoped change, quality/correctness result, and architecture scope;
4. link the precise native capture and selected `FrameId`/marker/resource/shader identity that distinguishes the hypotheses;
5. apply a predeclared absolute/relative regression threshold and produce `Pass`, `Regression`, or `Inconclusive`, never a color-only verdict;
6. preserve useful negative results, including `Do not ship` or `Not worth the complexity`, with the rejected alternative and evidence.

The Reference System carries the primary distribution. A materially different GPU architecture is the Comparison System when available. Cross-machine results are not pooled into one percentile distribution.

Three runs of 300 warm valid frames are the current acceptance floor, not an automatic claim of statistical definitiveness. Frame samples are typically autocorrelated; p99 contains few tail observations at this floor, worst values are sample-count-sensitive, and pooling can hide run-to-run shifts. Therefore:

- per-run distributions and run-to-run variation are primary; the combined distribution is secondary and retains run identity;
- the analysis predeclares both an absolute and relative practical-effect band, not only a significance threshold;
- uncertainty uses a declared correlation-aware method such as a justified block bootstrap or permutation at an appropriate independent unit; the method, block/unit choice, confidence level, and assumptions are recorded before reading the outcome;
- a confidence interval that overlaps the practical-effect band produces `Inconclusive` even if the point estimate looks favorable;
- p99 and worst are accompanied by valid `N`, tail-event `FrameId` values, and exclusions. Worst-to-worst comparison requires equal `N` or an explicit sample-count model;
- a p-value, when a tool provides one, supplements rather than replaces effect size, confidence interval, run stability, quality, and the predeclared regression band.

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

## External Profiler Handoff Contract

Sparkle's built-in diagnostics orient the investigation and preserve stable identities; external tools own OS scheduling, call stacks, API/resource state, hardware counters, shader/ISA analysis, allocation maps, BVH inspection, and crash artifacts. The version-sensitive [External Performance Profiler Runbook](DiagnosticsProfilerRunbook.md) owns the question-to-tool map, current capability record, capture preparation, detailed playbooks, and operational tradeoffs.

Every external investigation must:

1. start from a fixed route, representative configuration, readiness signal, and falsifiable hypothesis;
2. carry stable thread, queue, pass, frame/range, shader, resource, and build identities as applicable;
3. distinguish native live timestamps, replay timestamps, sampled hardware metrics, system-trace-derived values, and estimates;
4. record tool/version, driver/runtime/SDK, hardware, OS, observer mode/cost, validation state, lost data, and unsupported capabilities;
5. identify the critical path rather than equating the busiest unit or largest duration with causality;
6. label topology-changing diagnostic controls `NonRepresentative` and keep them out of final benchmark claims; and
7. link the narrow native capture and selected evidence into the workload-owned manifest.

The architecture deliberately does not assert that a named tool/version supports the current adapter, API feature, marker encoding, or operating system. Revalidate the runbook before every external capture.

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
|-- <level>-<route>-gpu-profile.png
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
| CPU frame p50 / p95 / p99 / worst | `TBD` | `TBD` | `TBD` | Begin-to-begin, same declared route, window, and mode; uncertainty/run variation linked. |
| Host/game phase wall p50 / p95 / p99 | `TBD` | `TBD` | `TBD` | Physical owner plus logical phase breakdown; not sampled CPU execution time. |
| Render CPU p50 / p95 / p99 | `TBD` | `TBD` | `TBD` | Extract/cull/graph compile/record/submit/present critical contribution. |
| GPU graphics p50 / p95 / p99 / worst | `TBD` | `TBD` | `TBD` | Top-level graphics span; queue overlap stated separately. |
| Top GPU pass p50 / p95 / p99 | `TBD` | `TBD` | `TBD` | Stable semantic pass; no nested double count. |
| RAM working/private high-water | `TBD` | `TBD` | `TBD` | Both definitions reported. |
| GPU tracked/local high-water | `TBD` | `TBD` | `TBD` | Used/block/API/budget scope declared. |
| Compilation/pipeline hitches | `TBD` | `TBD` | `TBD` | Cold/warm state, cache state, event count, and worst correlated `FrameId`. |
| BLAS/TLAS build/update + scratch/result | `TBD` | `TBD` | `TBD` | Applicable RT mode only; structure/traversal capture linked. |
| Input-to-present p50 / p95 / p99 | `NotInstrumented` | `NotInstrumented` | n/a | Report only with an end-to-end input/present identity and supported measurement path. |
| Quality/correctness | `TBD` | `TBD` | n/a | Identical image/reference or explained change. |

The actual case-study table shows the predeclared regression threshold, result state, run count, valid/excluded samples, and uncertainty next to the headline delta. It may omit inapplicable rows but may not silently omit a required workload family.

### Example Incident Narrative

A specialist-facing case should be readable in this order:

1. Result: "Sponza at the fixed route was limited by `<measured domain>`, not by `<competing hypothesis>`."
2. Configuration: commit, product, backend, hardware/driver, resolution, route, settings, validation, workers, and diagnostics mode.
3. Baseline: distributions and memory high-water, not one frame or one FPS number.
4. Competing hypotheses: for example editor UI, game systems, frame-graph compile, command recording, GPU lighting, presentation, or memory pressure.
5. Discriminating evidence: one joined diagnostics view, one readable `ProfileGpu` tree when the question is marker attribution, and the appropriate external CPU/GPU capture when causality requires it.
6. Root cause: critical path with relevant call stack, pass, queue, resource, counter, or state.
7. Experiment/fix: one scoped change and the control that could falsify it.
8. Outcome: before/after distributions, image/validation result, memory, pacing, and limitations.
9. Adoption: how another engineer reproduces the route and opens the capture.
10. Deletion/negative result: instrumentation, path, or optimization rejected after evidence.

### Reviewer Paths

| Reviewer | Diagnostic evidence they should see |
| --- | --- |
| Recruiter, 60-90 seconds | One clean frame/diagnostics image and one sentence naming the measured bottleneck and outcome. No profiler wall of text. |
| Hiring manager, 10 minutes | Problem, configuration, baseline, likely-domain view, readable GPU Visualizer hot path, one controlled experiment, before/after table, and limitation. |
| Graphics specialist, 45-60 minutes | Raw schema, CPU/GPU timing semantics, inclusive/exclusive marker tree and invalidation rules, PIX/RenderDoc/Nsight/RGP/WPA capture, API/backend difference, counters or disassembly where causal, and reproduction steps. |
| Adopter | Exact build/run/route, diagnostic mode, expected summary, capture trigger, fallback, raw files, and issue/reproducer template. |

This directly advances whole-system performance, hard-debugging, low-level concurrency, productization, and communication evidence. It does not advance those requirements to `E3` until the captures and measurements are reproducible.

## Failure And Edge Cases

- Minimized or invalid-size frames do not enter render/GPU aggregates.
- Loading frames remain visible but are tagged `Loading/Unsettled` and excluded from warm benchmark windows.
- Resize, render-path/provider change, shader reload, history reset, and capture frames are tagged discontinuities.
- A device-lost or failed timestamp resolution invalidates the frame and preserves the failure context; it never reports zero milliseconds.
- A scope plan or timestamp requirement beyond fixed capacity rejects the profile request or invalidates detailed data with one bounded loss; the target product path never reaches the current fatal query-exhaustion behavior.
- Timestamp wrap uses the backend valid-bit contract; cross-queue subtraction is forbidden without calibration.
- An invalid parent, parent cycle, child outside parent, significant sibling overlap, or queue mismatch invalidates the affected GPU subtree and its exclusive values. The independent top-level queue result remains usable where valid.
- Draw, dispatch, resource, and barrier annotations without owned timestamp pairs remain marker-only. The visualizer never invents duration for them.
- A second `ProfileGpu` while one is armed/resolving returns `Busy`; cancellation, shutdown, device loss, clear, and late completion settle one capture ID exactly once.
- A capture that changes parallel command-recording policy is visibly `NonRepresentative` and cannot satisfy the representative profile acceptance path.
- Queue overlap means per-queue spans are displayed in parallel; they are not summed.
- VSync, frame-latency throttling, occlusion, remote desktop, capture/replay, validation, and power/thermal changes are recorded in the manifest.
- Memory budget changes caused by other processes are retained as observed API budget changes, not attributed to Sparkle allocations.
- A synchronous diagnostics request from EditorThread to RenderThread is forbidden in the live frame path.

## Verification Contract

Implementation acceptance requires focused tests and measured runs, as applicable:

### Semantics And Correlation

- Synthetic nested scopes with known ticks prove inclusive/exclusive calculation for leaves, nested children, sibling gaps, repeated tokens, and interval-union coverage without double count.
- Property/fuzz tests generate bounded valid and malformed hierarchies, timestamp-wrap cases, interval overlaps, generation changes, and state-machine action sequences; they assert deterministic merge, nonnegative derived values, bounded loss, and exactly-once settlement.
- Malformed parent cycles, child escape, sibling overlap, cross-queue parenting, and unprovable timestamp wrap invalidate the affected tree instead of producing negative or misleading exclusive time.
- Randomized recording-task completion produces the same merged hierarchy and order from preassigned `(queue, batch, chunk, local sequence)` records.
- `ProfileGpu` arm/capture/resolve/open/clear, busy, cancel, shutdown, failure, and late-publication transitions settle each capture ID once and preserve `FrameId` identity.
- Threaded depth 0/1/2 frames join CPU and delayed GPU data by `FrameId` under randomized resolution delay.
- Serial renderer/task modes publish honest physical-thread ownership and no phantom lanes.
- Missing/disabled/stale/invalid values never enter percentiles and never render as zero.
- Field-valid and common-correlated populations pass known-value tests with different missing fields, generations, provenance, discontinuities, and selection intervals; aggregates expose original/valid/excluded counts and reasons.
- Timestamp wrap, queue separation, sample-window reset, lost result, shutdown, and late publication are tested.
- D3D12 queue frequency/support and bottom-of-pipe semantics, Vulkan stage/period/valid-bit semantics, calibration age/deviation rejection, and dependency-only uncalibrated queue display pass backend-focused tests.
- Nearest-rank percentiles and high-water frame identity pass known-value tests.
- `Stat` parsing, case-insensitive group lookup, autocomplete, toggle/idempotent On/Off behavior, `None`, `Reset`, `Dump`, and every preset pass focused command tests.
- Editor menu and console requests produce the same active group set without one presentation path invoking the other.
- Group demand changes advance sample generation; late detailed results cannot populate a group after it is disabled and re-enabled.
- Unit/UnitGraph use the same joined samples, and correlated example data proves that invalid GPU frames render as gaps rather than zeros.
- GpuPasses live ranking, unaccounted span, queue separation, row overflow, and count/duration units pass known-value tests.
- GPU Visualizer hierarchy, flat inclusive, flat exclusive, coalesced call-count/sum/average/max, hot-path expansion, and marker-only rows pass known-value tests.
- The generated/static scope registry rejects token/path collisions and transient identity components, preserves schema-version decoding, emits balanced command-recording-local duration scopes under randomized task completion, and keeps point markers/resource names out of duration aggregation.
- OS process-lifetime peaks, Sparkle session sampled high-water, and benchmark-run sampled high-water remain distinct; reset changes only the session generation and sampled values expose cadence.

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
- D3D12 and Vulkan captured pass hierarchy and inclusive/exclusive values correlate with the same external marker ranges within a declared tolerance; differences and unavailable timing scopes are explained.
- Engine Vulkan pass names and ordering are visible in RenderDoc and a supported vendor trace.
- Every Sparkle-owned OS thread appears with its stable name.
- The `SparkleTasks` ETW provider decodes task names, lanes, run/task identity, outcome, and dependencies.
- One artificial CPU stall, GPU stall, frame-queue backpressure case, and memory growth/retirement case is detected and classified; removing the injected defect clears the signal.

### Workload Evidence And Investigation Method

- `MAP-00` proves fixed resolution, authoritative readiness, named screenshot, raw CPU/GPU samples, process RAM/GPU memory state, manifest identity, and a clean reproduction on Sponza before any flagship claim.
- The accepted measured route contains at least three runs of at least 300 warm valid frames, preserves per-run raw samples, and reports CPU/GPU p50/p95/p99/worst plus the declared uncertainty/run-variation method.
- A synthetic autocorrelated dataset proves the selected uncertainty implementation and `Inconclusive` overlap rule; tail reports preserve `N` and event identities, and unequal-`N` worst comparisons are rejected unless explicitly modeled.
- Cold launch/load, warm load, time to first correct frame, compilation/cache/pipeline hitches, and steady traversal are distinct phases and are never pooled into one warm distribution.
- Benchmark comparison rejects an intentionally mismatched resolution, route, settings, readiness, observer mode, or undeclared configuration change; accepted comparisons emit explicit threshold and `Pass`/`Regression`/`Inconclusive` state.
- Benchmark records cover applicable renderer-stage time, workload counts, process RAM and precise GPU memory high-water, upload/eviction/residency/missing events, BLAS/TLAS build/update/compaction/scratch/result data, and exact engine/content/configuration identity, or visibly mark the production fact unavailable.
- One CPU investigation demonstrates timeline classification, sampled-stack localization, a serial/1/2/N control, and a PMC/IBS or equivalent cache/branch/data-access hypothesis where it is causal; it reports CPU topology and does not generalize beyond measured systems.
- One GPU investigation demonstrates the top-down sequence from activity/queue behavior through unit pressure to selected marker and source/ISA or RT evidence where causal, then validates the predicted whole-route result.
- One memory investigation demonstrates `A Before`, `B Loaded/Warm`, and `C Unloaded/Retired`, reconciles engine totals with event trace/snapshot scope, and distinguishes retained pools/fragmentation from a leak claim.
- `CASE-02` captures the same semantic route on D3D12 and Vulkan and explains queue/resource/barrier/descriptor/pipeline/RT-build differences without requiring identical API event streams.
- At least three measured bottleneck studies satisfy the workload protocol; at least one preserves a justified `Do not ship`/`Not worth the complexity` result. One difficult incident includes competing hypotheses, minimal reproducer, native capture, root cause, scoped fix, regression gate, and limitation.
- `CASE-05` is attempted by another engineer or clean-environment adopter using only the documented route; deviations, missing capabilities, and capture failures become explicit evidence rather than being repaired silently by the author.

### Cost And Bounds

- Ring capacities, queue capacities, string/label storage, and export sizes are asserted/tested.
- Four-overlay and 16-compact-row limits, fixed presets, hidden-row count, hitch row limit, and detailed-scope capacity are asserted/tested.
- `LiveBasic` performs no post-initialization per-frame heap allocation.
- `Off`, `LiveBasic`, `LiveDetailed`, `GpuProfileCapture`, and external-capture overhead are measured, not assumed.
- Fps-only, Unit, four simultaneous basic groups, GpuPasses, ProfileGpu, and UI open/closed observer costs are measured independently.
- A representative ProfileGpu capture leaves parallel frame-graph recording enabled and preserves submission batches, queue dependencies, worker policy, command order, and output. CPU recording disturbance and GPU timing disturbance are reported.
- Scope planning and recording use fixed storage with no per-scope string allocation, shared-vector growth, or mutex contention between recording tasks; per-queue query capacity is checked before arming.
- Enabling groups with the same minimum mode does not duplicate collection, memory polling, GPU timestamp pairs, or history storage.
- Export failure preserves the previous accepted evidence and reports one actionable error.
- `git diff --check`, applicable builds/tests, architecture boundary check, and exact unavailable hardware/tool paths are reported.

## Suggested Vertical Slices

This is architecture decomposition, not a schedule; the Roadmap and `MAP-00` own priority.

1. Freeze metric names, units, validity, `FrameId` join behavior, and a source-backed baseline trace using existing thread/ETW/GPU markers.
2. Add the bounded Application session, host phases, process RAM, Renderer CPU stages, frame-queue waits, and one top-level GPU queue span needed by `MAP-00`.
3. Register the fixed `Stat` command through the existing Editor/DevelopmentGame console composition, publish `Fps`, `Unit`, and `UnitGraph` from the same model, and prove basic-mode observer cost.
4. Complete the workload-owned `MAP-00` vertical slice: fixed resolution/readiness, explicit benchmark export and manifest integration, capture naming, and Sponza calibration.
5. Publish Threads, Tasks, Render, and Memory views; correct GPU memory segment semantics; and expose the same groups through the Editor Stat menu and Performance window.
6. Add Gpu/GpuPasses only after top-level timing correlation and basic-mode overhead pass. Admit Scene/Rhi rows only from existing production-owner counters, never from diagnostic scans.
7. Add the `ProfileGpu` vertical slice: stable tokens and explicit parents, a fixed per-chunk scope/query plan that preserves parallel recording, delayed validation and inclusive/exclusive derivation, one frozen result, and the hierarchical/flat/coalesced Editor views.
8. Add bounded hitch selection/navigation to the shared frame navigator, check in the narrow WPR profile and profiler walkthrough, and capture one D3D12 and one Vulkan specialist example that correlates a GPU captured-frame node to the external marker tree.

Each slice must extend the existing owner and remove any presentation path it replaces. Sparkle may publish only the bounded marker-level GPU product described here; deeper API, shader, hardware, allocation, and scheduling data remains in profiler-native artifacts rather than widening engine public APIs.

## Decisions And Rejected Alternatives

| Decision | Selected | Rejected |
| --- | --- | --- |
| Product shape | Bounded live orientation + one focused frozen GPU marker capture + explicit evidence session + external tools. | General in-engine profiler/trace viewer. |
| Evidence statistics | Workload-owned raw samples, p50/p95/p99/worst, uncertainty, thresholds, and native capture links. | Expanding the live overlay into a benchmark/report application or treating a screenshot as the result. |
| Sample populations | Field-valid populations for one metric; common-correlated `FrameId` intersection for paired/cross-domain claims, with full exclusion metadata. | Comparing independently filtered percentiles or hiding missing frames. |
| Benchmark inference | Per-run primary results, combined secondary view, practical absolute/relative band, correlation-aware uncertainty, and `Inconclusive`. | Treating 3x300 as automatically definitive, pooling away run identity, p-value-only decisions, or unequal-`N` worst comparisons. |
| Cross-domain owner | Application session joins immutable domain results. | Global Core profiler singleton or Editor reaching into renderer/RHI state. |
| Stat interaction | One fixed `Stat` command family and Editor menu over the same typed group requests. | A second console, command-string-driven UI, or arbitrary module registration. |
| Stat composition | At most four compact groups sharing one demand-derived collection mode. | Duplicate collectors per overlay or every group displayed simultaneously. |
| Editor gameplay labeling | Logical `Gameplay.*` phases on `Sparkle.EditorThread`. | Invented editor-side `GameThread`. |
| CPU detail | Fixed orchestration scopes live; ETW/PIX call stacks for detail. | Per-function timers and an unbounded task history panel. |
| GPU frame value | Per-queue outer span with valid `FrameId`; detailed passes on demand. | Sum of pass timers or GPU utilization converted to milliseconds. |
| Timestamp provenance | Native/replay/sampled/system-derived/estimated provenance remains explicit and comparison-gated. | Mixing replay timings with native benchmark samples or hiding an estimate behind milliseconds. |
| Queue timelines | Separate queue-relative clocks plus dependency edges first; capability-gated calibrated axis only within a deviation/age contract. | Aligning or subtracting queue/CPU clocks by assumption. |
| GPU profile interaction | One typed `ProfileGpu` request and one retained immutable capture, separate from live stat demand. | Always-running full trees, silent capture replacement, blocking for GPU completion, or default capture files. |
| GPU hierarchy | Stable scope token + explicit capture-local parent; inclusive from ticks and exclusive from direct-child interval union per queue. | Completion-order/depth reconstruction, nested-duration sums, or cross-queue subtraction. |
| GPU capture topology | Preassigned per-chunk query/record slices preserve normal parallel recording and submission topology. | Silently serializing command recording to simplify profiling. |
| Memory | Working/private RAM and tracked/block/local/non-local/retirement GPU facts. | One ambiguous "RAM" and one combined "VRAM" number. |
| Memory peaks | OS process-lifetime peaks, Sparkle session sampled high-water, and benchmark-run sampled high-water remain distinct. | Claiming `Stat Reset` resets OS peaks or calling a sampled peak exact. |
| Publication | Delayed nonblocking join by `FrameId`. | Waiting for RenderThread/GPU so the newest UI row is complete. |
| Labels | Stable bounded ownership/pass vocabulary and bounded capture dictionary. | Dynamic resource/entity/path strings or runtime string interning in timing scopes. |
| Marker transport | Versioned generated/compile-time token registry with private backend fanout, balanced recording-local duration scopes, separate points and object names. | Vendor calls in feature code, cross-command-buffer scope stacks, or transient strings as aggregation identity. |
| Files | Explicit workload-owned export. | Default per-run JSON/CSV/report emission. |
| Hitch capture | Bounded in-memory hitch selection followed by an explicit `ProfileGpu`, external capture, or export action. | Automatic hitch-triggered disk/native captures before a measured bounded trigger, retention, privacy, and observer-cost contract exists. |
| Vendor counters | Native external capture first; a vendor SDK only as a later optional study adapter with a named consumer. | Making Nsight Perf SDK or another vendor runtime a core diagnostics dependency before workload proof. |
| External capture build | Fully optimized profiling build retaining symbols, shader debug/source packages, names, markers, and exact hashes. | A debug/validation build presented as representative or a profile switch that silently enables detailed internal collectors/serial recording. |
| Input-to-display | Remain `NotInstrumented` until the input/simulation/present/display identity chain and capability-specific observer path exist. | Estimating displayed latency from frame time or `Present` return. |
| Optimization start | Fixed, comparable DevelopmentGame/Editor and backend baselines. | Optimizing from the approximately 6 FPS screenshot alone. |

## Open Implementation Decisions

These choices require a bounded implementation record after measurement; they do not block the architecture:

- the exact minimal Core CPU trace surface and whether PIX CPU events are emitted in addition to ETW on Windows;
- the exact renderer result mailbox type and fixed capacity;
- whether top-level copy/compute timestamps meet the basic-mode overhead target on both backends;
- the exact CPU absolute observer-noise floor used with percentage overhead targets;
- the minimum common-correlated cohort required by each live multi-domain hint;
- the maximum calibration deviation, maximum calibration age, and recalibration cadence for each admitted unified time-axis purpose;
- the platform abstraction for process working/private memory beyond the initial Windows evidence platform;
- the exact local/non-local heap representation needed to preserve D3D12/Vulkan semantics;
- the end-to-end input-sample/simulation/submission/present/display identity and platform measurement path; input-to-present remains `NotInstrumented` until this exists;
- the final fixed count set after the first Sponza trace proves which cardinalities distinguish the leading hypotheses;
- the exact static token registry/hash representation and how owner-declared labels populate the bounded captured display dictionary;
- the initial allowlist of timed pass-internal scopes versus marker-only annotations, after query-cost measurement on D3D12 and Vulkan;
- exact Development/Shipping compilation and runtime eligibility for the Stat presenter and detailed collectors;
- the benchmark CLI/request surface, which must integrate with `MAP-00` rather than create a second workflow;
- the workload-owned correlation-aware uncertainty method, confidence level, practical-effect bands, and block/independent-unit choice for each benchmark family.

## Measurement Semantic References

Version-sensitive profiler sources and the current capability matrix live in the [External Performance Profiler Runbook](DiagnosticsProfilerRunbook.md#source-and-version-reconciliation). The stable API sources behind this architecture's timing, memory, and presentation semantics are:

- [D3D12 timing, queue frequency, bottom-of-pipe meaning, and calibration](https://learn.microsoft.com/en-us/windows/win32/direct3d12/timing)
- [`ID3D12CommandQueue::GetClockCalibration`](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12commandqueue-getclockcalibration)
- [Vulkan timestamp query semantics](https://docs.vulkan.org/spec/latest/chapters/queries.html#queries-timestamps)
- [Vulkan calibrated timestamps and maximum deviation](https://docs.vulkan.org/refpages/latest/refpages/source/vkGetCalibratedTimestampsKHR.html)
- [`VK_EXT_memory_budget` estimated heap usage and mutable budget](https://docs.vulkan.org/refpages/latest/refpages/source/VK_EXT_memory_budget.html)
- [DXGI per-process local/non-local video-memory usage and budget](https://learn.microsoft.com/en-us/windows/win32/api/dxgi1_4/nf-dxgi1_4-idxgiadapter3-queryvideomemoryinfo)
- [Vulkan Memory Allocator statistics scope](https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/statistics.html)
- [Vulkan present-timing stages, time domains, and bounded result queue](https://docs.vulkan.org/features/latest/features/proposals/VK_EXT_present_timing.html)
- [Windows process memory counters](https://learn.microsoft.com/en-us/windows/win32/api/psapi/ns-psapi-process_memory_counters_ex2)

These sources explain external API semantics. They do not define Sparkle ownership, metric naming, collection cost, or evidence acceptance.

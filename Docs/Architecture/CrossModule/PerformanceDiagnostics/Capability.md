# Performance Diagnostics Capability Inventory

Status: capability snapshot; dated, and not architecture, release approval, or executable evidence

Snapshot: 2026-08-28 at committed `master` revision `20814381`; source and executable build configuration were unchanged from implementation revision `99af6d5b`; refresh against the live tree before relying on an implementation claim

Scope: current timing, marker, memory, editor, benchmark-export, and attached-capture surfaces relevant to the target diagnostics system

Architecture authority: [Performance Diagnostics Architecture](README.md)

Delivery authority: [Performance Diagnostics Delivery Plan](../../../Plans/CrossModule/PerformanceDiagnostics.md)

Feature acceptance: [Performance Diagnostics — Acceptance](Acceptance.md)

This inventory records current source/build surfaces and the reconciliation gaps known when the diagnostics design was written. Code and executable build configuration remain authoritative.

## Source-Backed Snapshot

The following is a static observation reconciled with committed source on 2026-08-28, not a completion claim:

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
| Memory polling identity | `FramePipeline` passes its monotonic submission `FrameId` to `RendererMemoryMonitor`; the monitor uses checked frame-distance polling and retains the last sampled frame. | The earlier frame-slot wrap defect is closed in source. Poll cadence, unavailable-budget behavior, and long-run presentation still require executable validation. |
| RAM | No production process-memory sampler exists in the engine. | Working set and private committed bytes are required; engine CPU allocation categories are a later measured need, not an initial fiction. |
| Editor memory route | A renderer memory provider reaches `UI`, but no current editor panel consumes it. | The target should replace this broad/synchronous presentation route with one immutable diagnostics model published by Application. |
| Attached external capture | D3D12 emits PIX events when `WinPixEventRuntime.dll` is available, but Sparkle has no `-Pix`/`-RenderDoc`/`-Nsight` launch intent, capture-layer bootstrap, attached-provider state, or viewport capture action. | Add a bounded capability-gated provider-set path; the current marker runtime alone does not prove that PIX frame capture is available. |

Primary code landmarks for revalidation:

- `Engine/Application/Private/RuntimeApplication.cpp`
- `Engine/Application/Private/EditorApplication.cpp`
- `Engine/Core/Private/Time/Timer.cpp`
- `Engine/Core/Private/Threading/ThreadOwnership.cpp`
- `Engine/Tasks/Private/Profiling/TaskProfiler.cpp`
- `Engine/Renderer/Private/Concurrency/Coordinator/RenderCoordinator.cpp`
- `Engine/Renderer/Private/Frame/FramePipeline.cpp`
- `Engine/Renderer/Private/Diagnostics/FrameExecutionDiagnostics.cpp`
- `Engine/Renderer/Private/Diagnostics/PassExecutionDiagnostics.cpp`
- `Engine/Renderer/Private/FrameGraph/Execution/FrameGraphRecordingChunkRecorder.cpp`
- `Engine/Renderer/Private/FrameGraph/Execution/FrameGraphRecordingExecutor.cpp`
- `Engine/Renderer/Private/Diagnostics/RendererMemoryMonitor.cpp`
- `Engine/RHI/Public/Memory/RhiMemoryDiagnostics.h`
- `Engine/RHI/Private/D3D12/Diagnostics/D3D12PixEvents.cpp`
- `Engine/Editor/Private/Panels/ViewportTopPanel.cpp`

## Reconciliation Gaps

Revalidate this table with `rg` before implementation work and reconcile it into the mandatory authority/candidate ledger above. It records the 2026-08-28 route to extend, not permanent file-name policy.

| Responsibility | Current owner/path | Delivery decision |
| --- | --- | --- |
| Monotonic application identity and unscaled delta | `Timer`, used by `RuntimeApplication::BeginFrame` and `UpdateRuntime` | Reuse `Timer::GetFrameCount()` as the host `FrameId` source. Measure explicit begin-to-begin and phase boundaries in Application; do not create another clock singleton. |
| Runtime composition and lifetime | `RuntimeApplication` | Own the diagnostics session beside the existing Timer/Window/Renderer runtime. Create after host prerequisites, publish after available domain results, and destroy before those producers. |
| Editor composition | `EditorApplication` and `EditorUiFrameRenderer` | Adapt the Application-owned immutable snapshot and typed requests into Editor presentation. Do not make `SparkleEditor` depend upward on `SparkleApplication`. |
| Console parsing | Core `ConsoleCommandRegistry`, hosted by `EditorConsoleSystem` and `RuntimeConsoleOverlay` | Add one registration function used by both composition roots. The UI sends typed requests directly; it never formats a console command. |
| Renderer cross-thread control/publication | `RenderCoordinator`, `RenderControlCommandQueue`, and `PublishReadState` | Extend the existing bounded control/read-state route with diagnostics request/result products. Avoid synchronous per-frame queries and avoid a second mailbox framework. |
| Renderer execution measurements | `FramePipeline`, frame-graph executor, and existing diagnostic scopes | Instrument fixed owner boundaries and publish one bounded frame result keyed by `FrameId`. Do not scan the graph after execution merely to populate counters. |
| Content/feature authoring | Levels/content, gameplay systems, tasks, frame-graph passes, resources, and draws already declare real work to their owners | Derive facts at orchestration/compile/submit/allocator boundaries. Require zero diagnostics maintenance from content/feature authors; allow only a sparse static semantic token for a genuinely new owner boundary. Remove manual/dynamic instrumentation instead of copying it. |
| GPU timing | `FrameExecutionDiagnostics` and backend `RenderTimingDiagnostics` | Replace dynamic strings/vectors/mutex completion with stable tokens, fixed records, preassigned per-recording-chunk ranges, deterministic merge, and bounded loss for the accepted live/capture path. |
| Parallel command recording | `FrameGraphRecordingExecutor::ShouldRecordBatchInParallel` | Remove the `!CVarRendererDiagnosticGpuTiming` topology change before accepting `CAP-00`, `INV-03`, or `CAP-01`. A captured profile must not silently serialize normal recording. |
| GPU memory | RHI `RenderMemoryDiagnostics` and Renderer `RendererMemoryMonitor` | Reuse allocator facts and the corrected monotonic `FrameId` polling path. Preserve local/non-local and used/allocated/budget distinctions while adding only the selected neutral publication/history product. |
| Process RAM | No current production sampler | Add the smallest Platform-owned process snapshot required by Application, initially Windows-backed. Do not build an allocation tracker or put Win32 types in Application. |
| Task detail | `TaskProfiler` ETW provider and fixed task lanes | Reuse ETW for deep task traces. Live UI may publish bounded lane aggregates only when the executor already owns the counts/durations. |
| Viewport summary | `ViewportTopPanel::BuildRightControls` currently reads ImGui FPS/delta | Replace this source with the immutable diagnostics presentation. Do not retain two competing FPS truths. |
| Editor diagnostics providers | `UI` currently receives broad renderer snapshot callbacks | Migrate only overlapping performance responsibilities to the Application product and remove replaced callbacks. Asset inspector routes remain separate. |
| Viewport screenshot | `EditorViewportCaptureCoordinator` plus Renderer/RHI readback | Keep it as image capture and reuse its nonblocking lifecycle lessons only. External profiler capture is a different operation and must not overload `RhiCaptureService`. |
| Pre-device integrations | `RendererExternalRuntime` builds immutable `RendererBackendConfiguration` before `RenderCoordinator` creates the backend | Extend this existing process-facing owner with launch intent and capture bootstrap. Do not add a competing startup integration service. |
| Backend diagnostics | `RenderHardwareInterface::GetDiagnostics()` returns neutral RHI diagnostic services | Add only the narrow neutral capture capability/request/result needed above RHI. Native APIs, handles, DLLs, SDK state, and provider objects remain in D3D12/Vulkan private adapters. |
| Build membership | Canonical profiles already define `SPARKLE_BUILD_SHIPPING`; module `CMakeLists.txt` files glob owned Public/Private sources; RHI composes common/backend targets | Use the existing profile as one eligibility boundary: include diagnostic implementations/dependencies in Debug/Development and exclude them from Shipping target/link/package membership. Keep sparse owner seams compile-time empty and proven absent. Add optional SDK rules only inside eligible provider builds. Do not introduce a Diagnostics module or per-feature switches. |
| Tests | No active CTest registration exists; `ShaderCompilerCliValidation` is a focused custom target rather than a repository test suite. | Use the narrowest existing owner validation surface. If a selected diagnostics contract requires a new executable test, keep it focused in its owning module and do not build a generic diagnostics test framework. |

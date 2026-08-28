# External Performance Profiler Runbook

Status: version-sensitive operational research and runbook; not proof of current Sparkle implementation or tool support

Last external-source reconciliation: 2026-08-28

Scope: profiling-build preparation, current external-tool capabilities, marker interoperability, capture provenance, operational capture checks, input/display measurement options, and revalidation triggers

## Purpose And Authority Boundary

This document answers two operational questions: which external tool can currently distinguish a performance hypothesis, and what must be preserved so the resulting capture is representative and auditable?

It does not define Sparkle's metric meanings, owners, buffers, or product UX. Those belong to [Performance Diagnostics Architecture](PerformanceDiagnosticsArchitecture.md). It does not own benchmark gates, artifact names, or claim acceptance; [I. Acceptance Workloads](../../../Engineering/BistroAndSanMiguelWorkloads.md) and [Validation, Performance, and Evidence](../../../Engineering/Standards/ValidationPerformanceAndEvidence.md) remain authoritative. [Diagnostics Product And UX Research](DiagnosticsUxResearch.md) records precedent and adopted/rejected product ideas.

External tools, drivers, SDKs, and operating-system providers change independently of Sparkle. Every capture therefore records the installed tool version, driver, API/runtime/SDK, hardware, OS, build, observer mode, and relevant capability result. This runbook is revalidated before use; its date is not a support promise.

## Source Record And Revalidation Rule

Every external claim retained in this runbook has five parts:

1. **Claim supported** - the narrow capability or limitation used by the workflow.
2. **Source and section** - a primary manual, API specification, release page, or repository release.
3. **Version/date observed** - the tool/spec state reconciled above.
4. **Adopted / not inferred** - what Sparkle uses and what the source does not prove.
5. **Revalidation trigger** - tool, driver, SDK, API, hardware, OS, or marker-runtime change.

Release pages and current manuals establish current capability. Older blogs, tutorials, and product pages may establish a useful method, but they do not override a newer support matrix. Search snippets, screenshots, successful replay, and another engine's integration are never capability proof by themselves.

## Source And Version Reconciliation

| Product/source state observed on 2026-08-28 | Claim supported and adopted | Not inferred / revalidation trigger |
| --- | --- | --- |
| PIX [2603.25 main and 2606.18-preview download record](https://devblogs.microsoft.com/pix/download/) plus current Windows Performance Toolkit documentation | PIX Timing/GPU Capture are the primary D3D12 timing/frame paths; `PIXGpuCaptureNextFrames`, `PIXSetTargetWindow`, attachment query, and capture-open APIs support an editor-owned next-frame button when the GPU capturer was loaded/injected before D3D12 device creation. WPR/WPA provides Windows sampled/precise CPU and TraceLogging analysis. | WinPixEventRuntime markers alone do not mean GPU capture is attached. Use the main PIX release unless a required preview D3D feature justifies the preview, and record installed versions. A replay is not native timing; a collector can perturb the workload. Revalidate on PIX/WPT, Agility SDK, Windows, or capture-setting change. |
| RenderDoc [v1.45 release](https://github.com/baldurk/renderdoc/releases/tag/v1.45), 2026-07-02, and current in-application API | Current D3D12/Vulkan frame-debugging baseline; its dynamically queried API supports application-controlled capture without statically linking the injected module. | API/header versions must match and target device/window behavior must be smoke-tested. RenderDoc validation/replay does not establish CPU scheduling or hardware cause. Revalidate version, driver, API feature, attachment path, and replay result. |
| Nsight Graphics [2026.3](https://developer.nvidia.com/nsight-graphics/get-started), 2026-07-23, with NGFX SDK 0.9.0 beta documentation | GPU Trace and Graphics Capture cover current NVIDIA D3D12/Vulkan work; the beta SDK supports Graphics Capture initialization, next-delimiter request, native GUI-button use, and artifact-path queries. | The SDK is explicitly beta and permits only one activity per process. Sparkle `-Nsight` means Graphics Capture, not Systems or GPU Trace. Keep the button experimental and revalidate the SDK/API, feature matrix, GPU/driver, activity, artifact finalization, and observer effect. |
| Nsight Systems [2026.4.1](https://developer.nvidia.com/nsight-systems/get-started) current release and user guide | Current NVIDIA whole-system CPU scheduling, API, and GPU timeline path for supported Windows/Linux targets; current release notes include D3D12 and Vulkan graphics coverage. | Platform, API, trace-provider, driver, privilege, and collection overhead vary. Record exact collection settings and do not treat a system trace as shader or frame-replay proof. |
| Epic Unreal Engine 5.8 PIX and RenderDoc integration documentation | `-AttachPix`/`-AttachRenderDoc` request attachment; each successful integration adds its provider capture icon in the upper-right Level Viewport and the icon performs a single-frame capture. Sparkle adopts conditional provider-specific placement and allows multiple requested/detected actions to share a compact group. | This proves the individual Unreal UX precedents, not Sparkle implementation, identical flags, multi-provider compatibility, or successful replay. Revalidate when Epic integration guidance changes. |
| Radeon GPU Profiler [v2.7](https://gpuopen.com/rgp/), June 2026 | Current AMD queue/barrier/wave/event profiling baseline for supported D3D12/Vulkan platforms and RDNA hardware. | Counter conclusions are architecture/capture specific. Its extended/native PIX marker path currently calls for the Agility SDK 1.721 preview path and matching AMD developer-preview driver; this is not baseline support. Revalidate RGP/RDP, driver, OS, GPU, API, and marker path. |
| Radeon GPU Detective [v1.6.3](https://gpuopen.com/radeon-gpu-detective/), June 2026 | Current Windows 11 AMD D3D12/Vulkan crash-dump and marker-breadcrumb baseline on listed hardware/drivers. | A breadcrumb narrows location, not root cause. Point markers are ignored and cross-command-list/buffer scopes are not reliable in this version. Revalidate tool/driver/API and known issues. |
| AMD uProf [v5.3](https://www.amd.com/en/developer/uprof.html), 2026-06-17 | Current AMD x86 hotspot, call-stack, IBS/PMC, cache, power, and supported system-analysis baseline. | Sampling skid, counter availability, multiplexing, OS, and CPU-family limitations remain capture metadata. Revalidate version, OS, CPU, selected profile type, and counter set. |
| Radeon Memory Visualizer [v1.15](https://gpuopen.com/rmv/) and Radeon Raytracing Analyzer [v1.11](https://gpuopen.com/manuals/rra_manual/) current pages | RMV separates trace/current snapshot/comparison views for AMD memory investigation; RRA analyzes acceleration-structure layout/quality and traversal-oriented evidence. | A point snapshot is not an allocation event trace. RRA simulation/structure metrics are not interchangeable with RGP captured counters. Record installed tool/driver/GPU and revalidate before capture. |
| PresentMon [v2.4.1](https://github.com/GameTechDev/PresentMon/releases/tag/v2.4.1), 2026-01-16, and current repository documentation | Current Windows cross-API presentation, pacing, latency, and supported GPU-execution observations. | Vulkan/OpenGL commonly appear as `Other` with reduced presentation instrumentation, and HWS reduces GPU execution-metric accuracy. Record runtime, HWS, version, and affected fields. |
| Epic documentation wrapper current; realtime GPU page contains historical UE4-era guidance; ProfileVisualizer page is primarily an API index | The durable precedent is the progressive diagnostic ladder and graph-owned semantic scopes. | Do not infer current engine internals, performance, or tool support from historical/product pages. Revalidate when citing a specific Unreal feature; prefer current task/timing/memory/RDG documentation. |
| AMD RGP/RDP overhead article from 2022 | Retain the method: measure capture overhead and minimize unnecessary collection. | Do not use its tool versions or support claims as current capability. Revalidate against current manuals/releases. |

## External-Capture Build Contract

Representative performance captures use a fully optimized profiling build. It preserves production code generation and renderer topology while retaining correlation material:

- CPU symbols and exact binary/PDB or platform-symbol hashes;
- shader type, virtual source, entry point, compiler options, active map/code-library records, source/debug artifacts, intermediate/binary, and hashes needed by the selected tool;
- stable Windows thread descriptions and task/phase identities;
- stable D3D12 object names, Vulkan debug object names, queue names, and semantic duration markers;
- engine/content/configuration/marker-schema/compiler/shader-compiler hashes;
- the same task-worker policy, threaded/serial renderer choice, pipeline depth, command-recording policy, queue topology, presentation policy, and render settings as the declared experiment.

The profile-build switch does **not** silently enable assertions, D3D12 debug layer, Vulkan validation, detailed Sparkle timestamps, hardware counters, serial command recording, extra queue waits, capture files, an external capture provider, or a different allocator. Those are explicit observer/configuration dimensions. A correctness-validation run may use them separately; it cannot be presented as the representative performance run without a measured equivalence argument.

Keep three modes distinct:

| Mode | Purpose | Claim boundary |
| --- | --- | --- |
| Representative external capture | Optimized build, normal topology, stable markers/symbols; only the selected native collector is added. | May support a scoped performance cause after overhead and replay/provenance are declared. |
| Debug/replay investigation | Validation, shader replacement/debug compilation, replay, serialization, or force-barrier controls may be enabled. | Correctness and hypothesis isolation only; `NonRepresentative` for final timing. |
| Crash reproduction | Crash SDK/driver mode, breadcrumbs, dumps, validation where compatible, minimal reproducer. | Narrows the faulting interval and mechanism; does not by itself prove root cause or normal performance. |

## Marker Interoperability Contract

The stable Sparkle contract is a canonical `ScopeToken` and versioned semantic display path with private backend fanout. The frame graph/owning subsystem declares the scope once; feature code does not call PIX, RGP, Nsight, RenderDoc, Aftermath, or RGD directly.

Implementation and capture rules:

- generated or `constexpr` registry verification rejects token/hash collisions, duplicate paths, and transient identity; captures store the marker-schema version;
- duration scopes are balanced RAII and remain inside one task plus one command-list/command-buffer recording lifetime;
- task-local and recording-local stacks prevent push/pop pairs from crossing CPU threads, command lists, or Vulkan command buffers;
- `FrameId`, pointers, graph indices, resource paths, and runtime formatting are metadata, never aggregation identity;
- D3D12 PIX strings use backend-approved static/aligned storage; no temporary format buffer outlives the call contract;
- Vulkan prefers `VK_EXT_debug_utils`; the older debug-marker extension is a capability fallback, not a second semantic vocabulary;
- duration scopes, point annotations, and resource/object names are separate operations because tools preserve them differently;
- the same semantic token may resolve to a tool-safe display string, but tool-specific truncation/encoding never changes the token.

| Backend marker path | Intended consumers | Current caveat |
| --- | --- | --- |
| D3D12 native PIX3 duration events | PIX; RGP/RGD only when their current Agility SDK and driver requirements pass; Nsight/RenderDoc where supported | RGP v2.7's extended-marker route currently requires the 1.721 preview Agility path and matching AMD developer-preview driver. Capture the installed SDK/driver smoke-test result; do not infer ordinary 721+ support or add an engine-wide AGS path merely to bypass an unsupported setup. |
| Vulkan `VK_EXT_debug_utils` begin/end labels | RenderDoc, Nsight, RGP, RGD where supported | Keep the pair within one command buffer. Record extension availability. |
| Point marker | Bookmarks in tools that retain them | RGD v1.6.3 ignores D3D12 AGS/Vulkan point markers; a point never substitutes for a duration scope. |
| API resource/object name | State/resource/crash correlation | Separate from performance range identity; dynamic resource names stay bounded and off the live timing aggregation path. |

RGD v1.6.3 does not reliably handle a duration marker begun on one command list/buffer and ended on another. This reinforces the Sparkle command-recording-local scope rule even if another tool appears tolerant.

## Tool Choice And Tradeoffs

| Tool | Strongest evidence | Main limitation / anti-pattern |
| --- | --- | --- |
| WPR/WPA | Windows CPU sampled stacks plus precise running, ready, wait, preemption, wakeup, image/symbol, and `SparkleTasks` ETW correlation across D3D12/Vulkan. | Wall scopes and CPU samples answer different questions. Do not call the busiest thread the critical path without scheduling/dependency evidence. |
| PIX Timing Capture | Multi-frame D3D12 CPU/API/GPU/pacing/residency correlation and statistical range comparison. | Collector overhead and event volume matter. The Comparison layout's `N`, histograms, p-values, and low-sample warning are useful precedent, not Sparkle's regression policy. |
| PIX GPU Capture | One D3D12 frame's events, state, resources, descriptors, pipelines, shaders, and replay analysis. | Replay timing and queue behavior may differ from native execution. A successful replay does not replace the D3D12 debug layer/GPU validation. |
| RenderDoc | Cross-API D3D12/Vulkan event/state/resource/descriptor/draw/dispatch/output debugging. | It is a frame debugger, not CPU scheduler or vendor hardware-cause authority. Start with validation for correctness. |
| Nsight Graphics | NVIDIA GPU Trace, queue/hardware/shader/RT analysis; Shader Profiler on D3D12/Vulkan. | Results apply to captured NVIDIA architecture/driver. Live Shader Debugger is currently Vulkan-only and deliberately changes shader/debug execution. |
| RGP | AMD queue, barrier, event, wave, occupancy, and hardware-limit evidence. | Hardware/counter semantics and marker support depend on GPU, driver, API, SDK, and capture mode. Do not generalize to another architecture. |
| RMV | AMD allocation/residency event trace, point snapshots, and snapshot comparison. | Trace, snapshot, engine tracked total, and API budget cover different populations; reconcile instead of forcing equality. |
| RRA | BLAS/TLAS structure, geometry, instance, build-policy, and traversal-quality investigation. | Structure simulations/quality metrics are not RGP timing/counters and do not prove whole-route improvement. |
| AMD uProf | CPU sampled call stacks, IBS/PMCs, cache/branch/data-access and supported topology analysis. | Sampling skid and multiplexing can move/scale samples. Enable only counters that distinguish a declared mechanism. |
| Nsight Aftermath / RGD | GPU crash dump, page-fault/breadcrumb, in-flight marker/shader/resource context. | Last/in-flight marker is a search boundary, not proof that the named pass caused the crash. Require validation, minimal repro, and discriminating experiment. |
| PresentMon/ETW | Windows cross-API display/pacing/latency evidence. | PresentMon documents reduced instrumentation for Vulkan/"Other" runtimes and HWS-related GPU metric inaccuracy; record runtime/HWS and confidence. |

### Question-To-Tool Map

This map selects the narrowest likely evidence source. The capability record and smoke capture still decide whether the installed tool/version supports the current adapter, API feature, marker encoding, and operating system.

| Question | Primary tool | API/hardware | Sparkle correlation |
| --- | --- | --- | --- |
| Which CPU thread/function runs, waits, is ready but unscheduled, or wakes another thread? | Windows Performance Recorder + Windows Performance Analyzer | Windows, both graphics APIs | OS thread descriptions, symbols, `SparkleTasks` ETW provider, `FrameId` bookmarks. |
| How do CPU scheduling, API calls, and GPU queues interact across the system? | Nsight Systems on NVIDIA; PIX Timing/WPA on Windows | Supported platform/GPU/API | Thread descriptions, CPU/API/GPU markers, queue names, `FrameId` range. |
| Which CPU source path or microarchitectural behavior limits a selected workload? | AMD uProf on supported CPUs; platform PMC/IBS-equivalent profiler | CPU/platform-specific | Stable thread/phase/task identity, symbols, fixed range, serial/1/2/N policy, exact CPU topology. |
| How do CPU and GPU work overlap across many D3D12 frames? | PIX Timing Capture | D3D12 | CPU/GPU event hierarchy, thread names, frame markers, queue submits. |
| What D3D12 API state/resource/pipeline/event produced one frame? | PIX GPU Capture | D3D12 | Existing D3D12 GPU event scopes and frame-graph pass names. |
| What D3D12/Vulkan API state, resources, descriptors, draws, and dispatches produced one frame? | RenderDoc | D3D12 or Vulkan | Backend debug labels/object names and frame-graph pass names. |
| Which NVIDIA GPU units, shaders, barriers, queues, or RT work limit the frame? | Nsight Graphics GPU Trace; graphics capture/debugger where supported | NVIDIA D3D12/Vulkan | Frame/pass markers, queue names, shader debug data, exact driver/hardware. |
| Which AMD GPU waves, barriers, queues, or synchronization limit the frame? | Radeon GPU Profiler | AMD D3D12/Vulkan | PIX/Vulkan user markers, queue submits, exact driver/hardware. |
| Which shader instruction/resource pressure supports the selected GPU hypothesis? | Nsight Shader Profiler on NVIDIA; Radeon GPU Analyzer/RGP on AMD | Vendor/target-architecture specific | Pass -> pipeline -> shader type -> active map/code record -> entry point -> source/binary hash. |
| Which GPU allocation, heap, residency, fragmentation, or lifetime changed? | Radeon Memory Visualizer on AMD; PIX memory/residency or supported vendor equivalent | Tool/API/hardware specific | Named resource/category, A/B/C route bookmarks, local/non-local definitions, build/configuration. |
| Which BLAS/TLAS structure or traversal behavior explains RT cost? | Radeon Raytracing Analyzer on AMD; PIX/Nsight/RGP RT views where supported | Vendor/API specific | Route, TLAS/BLAS semantic token, geometry/instance/build policy, selected dispatch. |
| What evidence accompanies a GPU crash/page fault/device loss? | Nsight Aftermath on NVIDIA; Radeon GPU Detective on AMD; API diagnostics/validation | Vendor/API specific | Last completed/in-flight marker path, shader/resource/build identity, device/driver, reproducer. |
| Which CPU allocation call stack or lifetime grows RAM? | PIX Timing/Memory capture or a focused native heap tool | Windows process | Frame/phase events, symbols, explicit load/unload window. |

RenderDoc is a frame debugger, not the CPU profiler for this design. Nsight Graphics/RGP/uProf counter conclusions apply only to the captured architecture and conditions. PIX GPU evidence is D3D12-specific. WPA remains the cross-backend Windows CPU scheduling truth. Nsight Perf SDK is deliberately not a required runtime dependency; external Nsight captures answer the planned studies first.

## Attached Frame-Capture Provider Operations

The architecture selects the combinable Editor launch intents `-Pix`, `-RenderDoc`, and `-Nsight`; this runbook owns the changing provider mechanics, pairwise/multi-provider compatibility evidence, and readiness checks. These flags and the viewport icon group are target behavior, not proof that the current Sparkle executable implements them.

### Common Bootstrap And Capture Sequence

1. Parse a bounded provider set before graphics-device creation. Record the selected backend, reject provider/backend mismatches, and evaluate every requested combination against the measured compatibility matrix without hidden precedence or fallback.
2. Detect already injected providers and load each accepted capture layer through its documented pre-device path. Verify installation path, library identity/signature where supported, version/API negotiation, combination behavior, and clean unload/rollback behavior.
3. Initialize or query every provider's exact activity independently. Marker emission is not capture readiness: WinPixEventRuntime, Vulkan debug labels, or a loaded vendor library alone cannot enable that provider's icon.
4. Publish per-provider `Unavailable` with one actionable reason, or `Ready` with provider, activity, API/SDK version, backend, compatibility state, observer warning, and supported target semantics.
5. On a viewport icon click, bind the named provider and that viewport's native present target, enqueue one next-valid-frame request, and show `Armed`. Do not begin capture inside the UI event handler.
6. Observe `Capturing`/`Finalizing` only when that provider can report them; otherwise keep a truthful bounded pending state. Initially reject a second request through any provider and serialize against `ProfileGpu`, validation capture, or another provider even when several ready icons coexist.
7. On completion, preserve the provider-native artifact and its path when available. Open the tool or folder only through a visible user action unless usability evidence accepts automatic opening. Tag the captured Sparkle frame/discontinuity and observer mode where correlation is available.
8. On failure, timeout, resize, minimize, device loss, or shutdown, settle the request once, preserve the provider error, and prove that the next ordinary launch has no capture layer or stale callback.

### Provider Matrix

| Sparkle intent | Pre-device readiness | Viewport trigger | Target/artifact behavior | Primary limitation |
| --- | --- | --- | --- | --- |
| `-Pix` | Windows D3D12; load/inject `WinPixGpuCapturer.dll` before any D3D12 device/API creation; confirm GPU-capture attachment rather than marker runtime presence. | Set the clicked viewport's target window, then enqueue one frame with `PIXGpuCaptureNextFrames`. | `.wpix`; use the documented open-in-PIX API only after successful finalization. | D3D12/Windows only; capture/replay timing differs from native execution and the capture layer can perturb the workload. |
| `-RenderDoc` | D3D12 or Vulkan; discover the injected module and dynamically negotiate `RENDERDOC_GetAPI` with the matching header version. Do not statically link or invent a DLL search path outside configured/official locations. | Use the validated next-frame trigger or balanced start/end API with the selected device/window; smoke-test multi-window targeting on both backends. | RenderDoc owns the capture file/list and replay UI; retrieve/open only through supported API behavior. | Injection, API version, device/window selection, unsupported API features, and replay success vary by version/driver. |
| `-Nsight` | Supported NVIDIA D3D12/Vulkan; initialize the NGFX **Graphics Capture** activity and version every parameter struct. Only one NGFX activity may own the process. | Request one capture at the next Present or validated frame delimiter with `NGFX_GraphicsCapture_RequestCapture_*`. | Query finalized capture paths through NGFX artifact APIs; host/remote filesystem namespaces may differ. | NGFX SDK 0.9.0 is beta. Keep Sparkle support `Experimental`; do not reinterpret this intent as Nsight Systems or GPU Trace. |

Launching from provider-native UIs may inject one or more capture layers before Sparkle starts. Sparkle publishes an icon for every detected API that passes its provider, backend, and combination checks without requiring a matching flag. Passive detection never loads another provider. Simultaneously detected layers may coexist only when their exact versions/backends pass the compatibility matrix; affected icons otherwise remain `Conflict` until the user relaunches cleanly.

### Required Smoke Matrix

- no flag and no injection: no provider library load and no viewport icon;
- each requested provider missing, wrong version, wrong backend, and successfully ready;
- native-UI launch/attach detection without a Sparkle flag;
- every supported pair and three-provider combination: stable icon order, independent status, startup/rollback behavior, and explicit `Conflict` for unaccepted combinations;
- D3D12 multi-window target capture for PIX and RenderDoc, plus Vulkan target capture for RenderDoc;
- supported NVIDIA D3D12/Vulkan Nsight Graphics Capture with the exact SDK/driver/tool versions, while visibly `Experimental`;
- capture of the clicked viewport rather than the first unrelated Editor present;
- stable marker hierarchy and requested/captured `FrameId` correlation where the provider permits it;
- repeated single captures, clicking another ready provider while one is active, busy/conflict, resize/minimize, timeout, device loss, finalization, artifact opening, and shutdown/relaunch cleanup;
- internal detailed timing off versus provider capture on, then the explicitly declared combined mode only if later measured safe;
- Empty and Sponza observer-cost comparison against the same build/configuration without the provider.

## Capture Preparation And Provenance

Before any native capture:

1. Write the falsifiable hypothesis, competing cause, smallest discriminating tool/activity, expected signal, and failure interpretation.
2. Build the representative profiling configuration and archive CPU symbols plus shader source/debug/code artifacts and hashes.
3. Fix route, readiness, resolution, render settings, backend, validation state, worker/topology, VSync/presentation, power/thermal condition, background compilation policy, provider launch intent/activity, and target viewport.
4. Confirm stable thread, queue, pass, shader, resource, and `FrameId`/range identities in a short smoke capture.
5. Run API validation separately. Resolve correctness errors before performance attribution.
6. Disable overlapping Sparkle detailed timestamps/counters unless the experiment explicitly measures their interaction with the external collector.
7. Record tool, activity, version, driver/runtime/SDK, capture settings, privileges, replay mode, dropped/lost events, and measured observer effect.
8. Capture the narrowest representative interval. Preserve a wider system trace only when scheduling/pacing/context requires it.
9. Link the native artifact and selected range/marker/resource/shader identity into the workload-owned manifest.

Every imported measurement uses the architecture provenance vocabulary:

- native tool timeline over the live workload: `NativeLiveTimestamp` or `SystemTraceDerived` as appropriate;
- frame replay timing: `ReplayTimestamp`;
- sampled PMCs/GPU counters: `SampledHardwareMetric`;
- model-derived value: `Estimated`, never upgraded by presentation rounding.

## Common Investigation Loop

1. Reproduce the issue on a fixed route and configuration after the published readiness signal.
2. Use `LiveBasic` to choose the likely domain; use `Stat GpuPasses` only when a recent pass ranking would discriminate the next question.
3. Write a falsifiable hypothesis before taking the detailed capture.
4. Use `ProfileGpu` when marker hierarchy and inclusive/exclusive cost can distinguish the hypotheses; otherwise capture the narrowest external-tool artifact that can.
5. Confirm symbols, stable thread/pass markers, API validation status, and capture overhead.
6. Record the critical path, not merely the busiest unit or largest duration.
7. Make one scoped change or one controlled configuration experiment.
8. Re-run the complete warm-up/sample protocol and correctness/quality checks.
9. Compare distributions and memory high-water; explain regressions and rejected alternatives.
10. Preserve only the small reviewed evidence package and tool-native capture needed for reproduction.

## Operational Capture Checks

### WPA / ETW

- Use a checked-in narrow WPR profile with sampled CPU stacks, precise scheduling/context switches, process/thread/image data, `SparkleTasks`, and the minimal Application/Renderer frame provider when implemented.
- Confirm symbols resolve before interpreting source hotspots.
- Filter to the editor process and exact route interval, then group sampled CPU by `Sparkle.EditorThread`, `Sparkle.RenderThread`, and named task lanes.
- Inspect `CPU Usage (Precise)` for running, ready, and waiting intervals on the critical thread. Join task begin/end/dependency events by run/task identity and `FrameId` where present.
- Determine whether frame-queue backpressure, task join, present, file I/O, lock contention, preemption, or active call stacks own the delay.
- Save the ETL, WPA profile/view, selected range identity, and one annotated screenshot. Report lost buffers/events.

A high RenderThread wall duration with low sampled CPU can be a wait. A worker at 100% does not prove it is on the frame critical path. Ready time may expose oversubscription even when task bodies are individually fast.

### CPU Scheduling, Source, And Microarchitecture

Do not begin with every hardware counter enabled. Use the narrowest depth that can separate the hypotheses:

1. `Stat Unit`/CPU view identifies the physical owner, logical phase, wait versus active work, task imbalance, and representative fixed interval.
2. WPA, PIX Timing, Nsight Systems, or uProf thread timelines decide whether the critical delay is running, ready/preempted, blocked, sleeping, I/O, driver/API work, or a dependency/join.
3. Sampled call stacks rank source hotspots only inside the selected critical interval. Inclusive CPU samples and wall spans remain different measurements.
4. PMCs or IBS are enabled for a predeclared mechanism: cache/TLB/data-access locality, branch behavior, CPI/IPC, memory bandwidth, lock/atomic traffic, or false sharing. Report multiplexing/sampling limitations and the exact CPU topology.
5. Run serial, 1, 2, and `N` workers plus the declared renderer depth. Record physical/logical cores, SMT, P/E-core or chiplet/cache topology where applicable, context switches, ready time, task grain, memory high-water, and p50/p95/p99. Choose the smallest policy that wins the complete route reliably.
6. Change one cause—data layout, grain size, partition, affinity/QoS policy, lock/atomic design, or worker count—and remeasure the full route and correctness stress.

The conclusion is architecture-scoped. A result on one CPU topology does not justify a universal worker formula. A faster serial control does not automatically mean "remove threading"; it may expose task overhead, contention, oversubscription, or a tiny-work crossover.

### PIX

- Use Timing Capture first for multi-frame D3D12 CPU/GPU overlap, scheduling, pacing, queue, or residency questions.
- Use GPU Capture for one representative frame's API events, resources, descriptors, pipeline state, barriers, shaders, and outputs.
- Record whether a value is native or replay-derived. Correlate the same semantic marker and compare Sparkle top-level timestamps only within a declared tolerance.
- Use PIX Comparison as investigative assistance: preserve selected point count, ranges, histograms, and warnings. Sparkle still applies its workload-owned per-run, effect-band, and correlation-aware inference contract.

Record where the GPU becomes idle or work queues up; graphics/compute/copy submissions and synchronization; the expensive frame-graph pass and child events; draw/dispatch/RT build cardinality and pipeline changes; resource state and descriptor correctness; timing/counter meaning and replay limitations; and whether the profiler reproduces the engine's top-level timestamp within declared tolerance. Native D3D12 debug layer/GPU validation proves API correctness separately. A successful PIX replay is not a substitute for validation.

### RenderDoc And Cross-API Comparison

- Validate D3D12/Vulkan first; then verify launch/injection, frame boundary, marker hierarchy, object names, and representative output.
- Inspect events, state, descriptors, resources, barriers, draws/dispatches, and render result. Do not use replay milliseconds as the native benchmark distribution.
- Record RenderDoc version, driver, API feature use, replay success/fallback, and any unsupported state.

For Vulkan, start with validation and synchronization validation. Use RenderDoc for API evidence, Nsight Graphics GPU Trace on NVIDIA for hardware limiter and shader/queue evidence, and RGP on AMD for queue, barrier, wave, and hardware evidence. Compare D3D12 and Vulkan by semantic pass and route, not raw API call count alone. Record intentional differences in descriptor model, queue topology, allocator reporting, shader binary, barrier encoding, and provider capability.

### Nsight Graphics / RGP Top-Down Analysis

- Preserve exact GPU/driver/tool/API and counter/metric set. Report unavailable counters, sampling/multipass behavior, and other-process interference.
- Nsight Shader Profiler supports D3D12/Vulkan; the live Shader Debugger is a Vulkan-only debug activity in the reconciled version and is nonrepresentative for timing.
- For RGP, verify the current PIX3 or Vulkan marker path before capture. Do not assume a marker visible in PIX is visible under an older Agility SDK/AMD driver.

Use the same reasoning order on both vendors even though counter names and hardware differ:

1. Capture a stable representative range with fixed route/settings, warmed pipelines, declared validation, diagnostic mode, power/clock condition where available, and no unrelated background compilation.
2. Check GPU active, queue gaps, dependencies, and CPU submission. Low activity or starvation sends the investigation to the system timeline before shader analysis.
3. For a busy GPU, rank top-level unit/throughput pressure. A high metric is a hypothesis tied to that hardware, not a portable bottleneck label.
4. Select the stable Sparkle marker and correlate queue, barrier, workload count, pipeline, shader, and resource identities. Compare inclusive and exclusive cost without summing nested or overlapping work.
5. Inspect occupancy/waves, registers, divergence, bandwidth/cache, stalls, source/ISA, or RT traversal only when they can distinguish the competing mechanisms.
6. Predict what should improve if the suspected unit loses work, apply one scoped change, and confirm the selected range and whole-frame/route distributions. Record a transfer check on the other API and, when available, another GPU architecture.

Debug-only experiments may serialize queues, force full barriers, disable async/copy work, reduce render-path complexity, or pause background compilation to isolate a cause. They must be labeled `NonRepresentative`, cannot satisfy the final benchmark, and must never become the default renderer configuration merely because they simplify a capture.

### Memory / RMV

Use the live memory view to choose a controlled interval such as cold load, settled scene, camera route, scene switch, unload, and post-retirement. Record process working set/private commit, engine-tracked GPU used/block bytes, local/non-local API usage/budget, high-water frame identity, upload/eviction/residency events, and retirement return-to-steady-state behavior.

| Checkpoint | Required state | Primary question |
| --- | --- | --- |
| `A Before` | Process started or prior scene settled; captures/readiness recorded. | What is the retained baseline? |
| `B Loaded/Warm` | Target route fully resident, pipelines warm, retirement settled. | What did the workload add and where is pressure concentrated? |
| `C Unloaded/Retired` | Scene switched/unloaded and all declared GPU completion/retirement conditions passed. | What returned, what is intentionally pooled, and what appears leaked or fragmented? |

If process private commit grows, take an allocation call-stack capture. If local GPU usage grows but tracked allocation does not, investigate driver/external-provider allocations and backend reporting. If tracked used shrinks but allocator blocks do not, investigate pooling/fragmentation before calling it a leak. If retirement backlog grows, correlate it with completion tokens and frames in flight.

An event trace explains when allocations, uploads, residency changes, and frees occurred. A point snapshot explains current heap/resource structure. A sampled engine total shows trend. These products may disagree because they cover different providers and semantics; reconcile the difference instead of coercing them to the same number. Use RMV A/B comparison or an equivalent allocation/residency tool for suspected GPU leaks/fragmentation, and allocation call stacks for CPU private-commit growth.

### Ray Tracing / RRA And Crash Tools

- For a ray-tracing study, join route/pass/shader with BLAS/TLAS semantic identity, geometry/instance/build policy, compaction, scratch/result memory, build/update time, and the selected dispatch. Use RRA or supported PIX/Nsight/RGP RT views for structure quality/traversal behavior. Confirm a controlled change against both full-route timing and lighting quality; build time alone does not prove traversal improvement.
- For a fault, preserve exact build/backend/GPU/driver, validation state, last completed/in-flight stable markers, shader/resource identity, native dump, and minimal reproducer. Use Aftermath on supported NVIDIA paths, RGD on supported AMD paths, and API diagnostics/validation as applicable. Breadcrumb location narrows the search; it does not prove root cause.

## Input-To-Display Options

The architecture keeps latency `NotInstrumented` until Sparkle owns `InputSampleId -> simulation FrameId -> PresentId -> displayed result`. This runbook records the current measurement options; none may fabricate the missing identity chain.

| Option | Operational use | Tradeoff / required record |
| --- | --- | --- |
| PresentMon/ETW | Initial Windows cross-API displayed/pacing trace and external correlation. | Its documentation notes less presentation instrumentation and slightly reduced CPU-frame-derived latency accuracy for Vulkan/"Other" runtimes, plus HWS-related GPU metric limitations. Record runtime, HWS, provider/version, and affected columns. |
| DXGI latency/presentation path | Initial D3D12 Windows platform path for present identity and frame-latency behavior. | DXGI-specific and not optical display truth. Join to Sparkle input/simulation identity and record swapchain/presentation configuration. |
| `VK_EXT_present_timing` | Future capability-gated Vulkan path for supported present stages/time domains and past-present feedback. | Surface/device capability varies; internal result queues are explicitly sized and can return not-ready/full conditions. Record stages, time domains, queue size/loss, calibration deviation, and observer mode. |
| Vendor latency SDK | Later accepted-workload adapter when it supplies a missing supported identity/stage. | Vendor/runtime coupling, extra integration/validation, and cross-vendor gaps. Keep backend/private and optional. |
| Optical/high-speed measurement | Validate the final displayed response and systemic bias of software paths. | Requires equipment, automated stimulus, camera/display calibration, and many trials; limited internal attribution. Archive setup and synchronization evidence. |

## Embedded Vendor SDK Decision

Native external captures remain the production evidence path. A capture-control API is admitted only as an optional RHI-private adapter with a named user action, capability matrix, bounded request lifetime, observer-cost evidence, clean absence/fallback behavior, and no vendor types in Core, Application, Renderer public contracts, or Editor.

PIX and RenderDoc capture-control APIs satisfy a concrete consumer: the conditional viewport `Capture next frame` action. The NGFX Graphics Capture SDK addresses the same consumer but remains `Experimental` while its public documentation labels the SDK beta. This narrow admission does not admit counters, recommendations, or an always-on vendor collector.

Nsight **Perf SDK** remains deferred and is distinct from the NGFX Graphics Capture control API. Its current documentation describes one profiling session per GPU across the system, variable counter availability, system-global clock controls, D3D12 debug-layer incompatibility for range profiling, and Vulkan range limitations for secondary/simultaneous-use command buffers (including required behavior changes in some profiling paths). Those constraints conflict with a transparent cross-backend always-on collector and Sparkle's representative parallel-recording requirement. Reconsider only when external Nsight Graphics captures cannot answer an accepted study and a private adapter can preserve normal topology or label the experiment `NonRepresentative`.

## Primary Sources

### Microsoft And Windows

- [Epic: using PIX on Windows with Unreal Engine](https://dev.epicgames.com/documentation/en-us/unreal-engine/using-pix-on-windows-with-unreal-engine)
- [PIX current main and preview downloads](https://devblogs.microsoft.com/pix/download/)
- [PIX overview and capture selection](https://learn.microsoft.com/en-us/windows/win32/direct3dtools/pix/articles/general/pix-overview)
- [PIX GPU captures](https://learn.microsoft.com/en-us/windows/win32/direct3dtools/pix/articles/gpu-captures/pix-gpu-captures)
- [PIX programmatic capture APIs](https://devblogs.microsoft.com/pix/programmatic-capture/)
- [PIX Timing Comparison layout](https://learn.microsoft.com/en-us/windows/win32/direct3dtools/pix/articles/timing-captures/layouts/pix-timing-captures-comparison-layout)
- [Windows Performance Toolkit CPU analysis](https://learn.microsoft.com/en-us/windows-hardware/test/wpt/cpu-analysis)
- [TraceLogging capture with WPR/WPA](https://learn.microsoft.com/en-us/windows-hardware/drivers/devtest/capture-and-view-tracelogging-data)
- [WinPixEventRuntime](https://devblogs.microsoft.com/pix/winpixeventruntime/)

### RenderDoc

- [Epic: using RenderDoc with Unreal Engine](https://dev.epicgames.com/documentation/en-us/unreal-engine/using-renderdoc-with-unreal-engine)
- [RenderDoc project](https://github.com/baldurk/renderdoc)
- [RenderDoc releases](https://github.com/baldurk/renderdoc/releases)
- [RenderDoc in-application API](https://github.com/baldurk/renderdoc/blob/v1.x/docs/in_application_api.rst)
- [RenderDoc application API header](https://github.com/baldurk/renderdoc/blob/v1.x/renderdoc/api/app/renderdoc_app.h)
- [RenderDoc Vulkan wiki workflow](https://github.com/baldurk/renderdoc/wiki/Vulkan) - historical workflow aid; revalidate against the current release.

### NVIDIA

- [Nsight Graphics current release and system requirements](https://developer.nvidia.com/nsight-graphics/get-started)
- [Nsight Systems current release and system requirements](https://developer.nvidia.com/nsight-systems/get-started)
- [Nsight Graphics feature matrix](https://docs.nvidia.com/nsight-graphics/UserGuide/appendix.html)
- [GPU Trace overview](https://docs.nvidia.com/nsight-graphics/UserGuide/gpu-trace-overview.html)
- [Shader Profiler](https://docs.nvidia.com/nsight-graphics/UserGuide/shader-profiler.html)
- [Shader Debugger overview](https://docs.nvidia.com/nsight-graphics/UserGuide/shader-debugger-overview.html)
- [Application correlation configuration](https://docs.nvidia.com/nsight-graphics/UserGuide/configure-application.html)
- [Nsight Graphics SDK user guide and programmatic capture](https://docs.nvidia.com/nsight-graphics/UserGuide/sdk.html)
- [NGFX Graphics Capture API](https://docs.nvidia.com/nsight-graphics/NsightGraphicsSdk/group___n_g_f_x___a_p_i___c_o_r_e.html)
- [Nsight Systems User Guide](https://docs.nvidia.com/nsight-systems/UserGuide/index.html)
- [Nsight Perf SDK 2025.5 limitations](https://developer.nvidia.com/nsight-perfsdk/getting-started/release-note-v2025.5)
- [Nsight Aftermath](https://developer.nvidia.com/nsight-aftermath)

### AMD

- [Radeon GPU Profiler current release](https://gpuopen.com/rgp/)
- [RGP manual](https://gpuopen.com/manuals/rgp_manual/)
- [RGP user markers](https://gpuopen.com/manuals/rgp_manual/user_debug_markers/)
- [Radeon Memory Visualizer manual](https://gpuopen.com/manuals/rmv_manual/)
- [Radeon Raytracing Analyzer manual](https://gpuopen.com/manuals/rra_manual/)
- [Radeon GPU Detective current release](https://gpuopen.com/radeon-gpu-detective/)
- [RGD v1.6.3 help and known marker limitations](https://gpuopen.com/manuals/rgd_manual/help_manual/)
- [AMD uProf current release](https://www.amd.com/en/developer/uprof.html)
- [Historical RGP/RDP overhead method](https://gpuopen.com/learn/rgp-1-13-rdp-2-6/) - method only, not current capability.

### Presentation

- [PresentMon project and documented limitations](https://github.com/GameTechDev/PresentMon)
- [DXGI 1.4 frame-latency and presentation improvements](https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/dxgi-1-4-improvements/)
- [`VK_EXT_present_timing`](https://docs.vulkan.org/features/latest/features/proposals/VK_EXT_present_timing.html)

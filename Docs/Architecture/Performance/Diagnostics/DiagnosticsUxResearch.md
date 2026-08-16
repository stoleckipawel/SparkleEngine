# Diagnostics Product And UX Research

Status: research reference; external precedent and option analysis, not proof of current implementation

Research reconciliation: 2026-08-16

Scope: the visual and functional design of performance diagnostics for SparkleEngine, with emphasis on Epic Games, NVIDIA, and AMD products; data acquisition and implementation sequencing are intentionally secondary

## Purpose And Authority Boundary

This document maps the diagnostic product space before implementation. It asks what information a developer needs, at what depth, in which visual form, and how the views should connect without turning the engine or Editor into a profiler framework.

The selected Sparkle behavior belongs to [Performance Diagnostics Architecture](PerformanceDiagnosticsArchitecture.md). This document records precedent, alternatives, and reasons. It does not override:

- [A. Principal Graphics Engineering Requirements](../../../Strategy/Requirements.md), including `PGE-05`, `PGE-06`, `PGE-10`, and `PGE-13`;
- [Gap Assessment](../../../Strategy/GapAssessment.md), which owns the role-source/profile audit, current evidence grade, and principal-readiness gaps;
- [Validation, Performance, and Evidence](../../../Engineering/Standards/ValidationPerformanceAndEvidence.md), which owns measurement and claim discipline;
- [Editor and Tools](../../../Engineering/Standards/EditorAndTools.md), which rejects UI-owned engine truth and incidental public diagnostic APIs;
- [Graphics Engineering](../../../Engineering/Standards/GraphicsEngineering.md), which owns graphics evidence expectations;
- [I. Acceptance Workloads](../../../Engineering/BistroAndSanMiguelWorkloads.md), including `MAP-00`, reproducible captures, and reviewer routes.
- [External Performance Profiler Runbook](DiagnosticsProfilerRunbook.md), which owns version-sensitive tool capability, capture-build preparation, marker interoperability, and source revalidation.

The research deliberately separates product design from collection design. A compelling screen is not evidence that its fields can be measured cheaply or correctly. Every selected view therefore states its minimum semantic inputs and escalation boundary, but leaves concrete data structures and thread-safe publication to the owning architecture.

## Local Acceptance Lens

Sparkle does not need to clone Unreal Insights, Nsight Graphics, or Radeon GPU Profiler. It needs a coherent first-party ladder that satisfies the repository's actual use cases and hands causal questions to those tools with good correlation.

| Local need | Product consequence |
| --- | --- |
| `PGE-05`: CPU, GPU, memory, queues, residency, pacing, percentiles, high-water marks, causal experiments | One joined frame/session identity must connect summary, CPU, GPU, and memory views. Latest FPS alone is insufficient. |
| `PGE-06`: D3D12/Vulkan queues, commands, barriers, descriptors, memory, pipelines, shaders, presentation | Sparkle should orient and expose stable names; external frame/system profilers remain the detailed authority. Backend/configuration must always be visible. |
| `PGE-10`: counters, disassembly, concurrency evidence | The UI must make the handoff from a selected Sparkle marker to PIX/Nsight/RGP/RGA obvious. It must not invent hardware conclusions from timestamps. |
| `PGE-13`: bounded graphics-analysis tool and productization | A focused, polished GPU marker capture is valuable; a generic trace analysis application is out of scope. |
| `MAP-00`: fixed route, CPU/GPU p50/p95, process working/private memory, precise GPU memory fields, manifest, screenshot | The primary Performance workspace must be screenshot-readable and export the same definitions it displays. |
| Portfolio review | The first screen tells the bottleneck story in under 90 seconds; drill-down supports a 10-minute review; raw captures and methods support specialist audit. |
| Engineering standards | No UI-side scans, unbounded histories, arbitrary module registration, public provider sprawl, capture-time files by default, or diagnostic mode that silently changes renderer topology. |

## Requirements And Career-Evidence Audit

The selected product direction survives the repository-wide audit, but the audit changes what "done" means. A polished live profiler is not sufficient evidence for a principal graphics role. The product must lead to repeatable workload records, discriminating captures, controlled experiments, scoped conclusions, and an adoption path. Conversely, satisfying every portfolio field does not justify building an Insights-scale platform inside Sparkle.

### Canonical Requirement Traceability

| Requirement | Relationship to diagnostics | Required diagnostic contribution | What diagnostics cannot prove alone |
| --- | --- | --- | --- |
| `PGE-01` partner adoption | Enabler | Reproducible capture instructions, readable configuration, failure/fallback states, and an issue/reproducer template. | Cross-team discovery, review history, or external reproduction. |
| `PGE-02` ray/path tracing | Direct supporting evidence | BLAS/TLAS build/update/compaction, scratch/result memory, traversal-sensitive markers, sample/reconstruction cost, and quality links. | Correct lighting mathematics, convergence, temporal quality, or a real-time algorithm by itself. |
| `PGE-03` neural graphics | Future supporting evidence | Separate loading/inference phases, GPU markers, latency/memory distributions, fallback state, and selected shader identity. | A real model, training provenance, generalization, or product value. |
| `PGE-04` model-to-kernel | Future supporting evidence | Per-stage markers, dispatch/layout/count data, counter/disassembly handoff, and before/after distributions. | Operator derivation, numerical equivalence, or an optimized kernel implementation. |
| `PGE-05` whole-system performance | Primary | Correlated CPU, GPU, queues, memory, compilation, residency, streaming events, pacing, and measurable latency; p50/p95/p99/worst; high-water; comparison and regression evidence. | Causality without a controlled experiment and profiler evidence. |
| `PGE-06` workload analysis/debugging | Primary | Stable D3D12/Vulkan semantic identities, queue/pass hierarchy, API/configuration context, native capture links, incident workflow, and reduced-reproducer metadata. | Driver/hardware blame from engine timestamps or API correctness without validation. |
| `PGE-07` C++/Python engineering | Enabler | A bounded ownership model plus a narrow workload-owned analysis/export CLI contract. | Clean implementation, tests, sanitizers, review quality, or maintainability until implemented. |
| `PGE-08` mathematics/modeling | Enabler | Definitions, denominators, uncertainty, predicted bottleneck/cost fields, and experiments that can falsify a model. | The derivation or numerical analysis itself. |
| `PGE-09` APIs/shaders/ABI | Supporting | Backend parity context, pipeline/shader-package counts, barrier/descriptor/queue facts, shader identity, and D3D12/Vulkan capture correlation. | Correct ABI, reflection, compilation, synchronization, or fallback behavior. |
| `PGE-10` CPU/GPU architecture | Primary | Serial/1/2/N controls, scheduling and task traces, CPU PMCs/IBS workflow, GPU counter/source/ISA workflow, cache/bandwidth hypotheses, and architecture-scoped conclusions. | A low-level claim from utilization or duration alone. |
| `PGE-11` ML fundamentals | Outside this product | Preserve configuration links for later model evidence; do not widen the profiler for training concerns. | Training rigor, data separation, ablations, or model correctness. |
| `PGE-12` workload engineering | Future supporting evidence | Distinct startup/load/inference phases, batching/precision/layout configuration, memory and concurrency evidence. | Offline training behavior or deployment quality without the actual workload. |
| `PGE-13` productization/communication | Primary | Coherent progressive UX, transparent cost, documented decisions, a focused GPU capture, reviewer routes, and a deletion/negative-result path. | Principal-level productization until another engineer can use and reproduce it. |
| `PGE-14` platform breadth | Supporting | Windows D3D12/Vulkan capture recipes and a capability matrix; Linux evidence only after a native Linux path exists. | Platform breadth from a Windows-only capture. |
| `PGE-15` sustained influence | Enabler | Clear authority boundaries, reusable investigation method, teaching material, incident leadership template, and deliberate scope cuts. | Sustained influence, mentorship, or shipped impact from one tool. |

The direct acceptance center is therefore `PGE-05`, `PGE-06`, `PGE-10`, and `PGE-13`. Other rows are explicit consumers or boundaries, not excuses to turn diagnostics into a universal framework.

### Workload And Evidence Coverage Audit

| Gate/package | Required contribution from diagnostics | Design correction from this audit |
| --- | --- | --- |
| `MAP-00 Evidence Harness` | Fixed resolution/readiness, exact configuration, named screenshot, raw CPU/GPU samples, process RAM/GPU memory snapshot, manifest linkage. | Make this the first shipped subset; do not wait for every Stats group or the GPU Visualizer. |
| `WL-04 Measured Frame` | Three runs of at least 300 warm frames; CPU/GPU p50/p95/p99/worst; per-pass and queue overlap; render stages and workload counts; memory/residency/RT/compilation facts; paired API captures. | Add a benchmark-only evidence contract beyond the live 120-frame p50/p95 UI. The workload-owned Python analysis, not ImGui, computes uncertainty and comparisons. |
| Three bottleneck studies / `WL-07` | Predeclared hypothesis, serial/control, distinguishing trace/counter, scoped change, correctness, before/after distributions, rejected alternative, regression threshold. | Add study-state and capture links to evidence design, but no experiment manager to the Editor. One study must be allowed to conclude `Do not ship`. |
| `CASE-02 One Frame, Two APIs` | Same semantic route/pass across D3D12 and Vulkan; resource/barrier/descriptor/pipeline/RT-build comparison; difficult incident and reduced repro. | Make semantic marker paths and configuration hashes the join key; do not compare unlike API event names or assume identical implementation. |
| `CASE-03 Path-Traced Lighting Under Budget` | Reference/real-time modes, BLAS/TLAS and lighting cost, memory, time-to-quality, quality/performance frontier, failures. | Add an RT evidence family and RRA/Nsight/PIX/RGP handoff; timing hierarchy alone is insufficient. |
| `CASE-04 Model to Shader` | Per-stage inference profile, latency/memory budget, dispatch/layout/precision study, shader/source/ISA evidence, classical fallback. | Reserve stable phase/marker/configuration vocabulary without implementing ML-specific profiler panels now. |
| `CASE-05 Adoption Package` | Clean run/capture instructions, capability/fallback matrix, tuning guide, expected states, issue template, peer result. | Treat usability and failure states as acceptance data; require a novice reproduction route rather than only author screenshots. |

### Audit Verdict

Retain the progressive `Stats -> Performance workspace -> focused GPU capture -> external profiler` ladder. It is the right product shape. Close these gaps in the canonical architecture before implementation:

1. distinguish the live orientation window from the benchmark evidence schema, including p99, worst frame, uncertainty, run-to-run variance, regression thresholds, and raw-capture links;
2. include cold/warm loading, shader compilation, pipeline creation/cache state, upload/residency/eviction/missing-resource events, and measurable input-to-present identity rather than treating only steady rendering as the system;
3. represent BLAS/TLAS count, source geometry, build/update/compaction, scratch/result memory, and traversal experiments in workload evidence without crowding the default overlay;
4. add CPU scheduler, source-hotspot, and microarchitecture escalation paths, including serial/1/2/N worker studies, cache/branch/data-access evidence, contention, false sharing, and topology-aware conclusions;
5. add a top-down GPU investigation method that moves from GPU active/queue behavior to unit pressure, selected marker, shader/source/ISA, then validates a whole-frame change;
6. define controlled memory A/B/C checkpoints and keep event traces, current totals, committed blocks, residency, and point snapshots semantically distinct;
7. make incident and adoption deliverables first-class: exact environment, stable bookmarks, hypotheses, minimal reproducer, native artifact, fix/control, limitation, regression gate, and external reproduction;
8. stage delivery so `MAP-00` is not blocked by optional Stats groups, hardware-counter integration, a generic trace store, or vendor SDK embedding.

## The Option Space: Depth And Domain

Diagnostics vary vertically by investigative depth and horizontally by domain. Treating these as one giant table produces noise; treating them as unrelated tools destroys correlation.

### Vertical Depth Ladder

| Depth | User question | Interaction budget | Suitable Sparkle surface | External authority |
| --- | --- | --- | --- | --- |
| D0 Signal | Is the frame healthy? | A glance, always nearby | Viewport frame time/FPS and status affordance | None required |
| D1 Orientation | CPU, GPU, presentation, or memory pressure? | Seconds, live | `Stat Unit`, compact presets, Overview | External capture only when signals disagree |
| D2 Trend and ranking | Is it stable, a hitch, or a repeated dominant owner/pass/category? | Tens of seconds | Frame graph, named CPU lanes, top GPU passes, memory trend/high-water | System/GPU trace for causality |
| D3 Focused capture | What happened in this selected frame or range? | Minutes | Frozen GPU marker hierarchy; selected frame/range inspector; bounded comparison | PIX, RenderDoc, Nsight, RGP for deeper event/state data |
| D4 Trace and state | Which scheduling edge, API event, resource, barrier, allocation, or pipeline caused it? | Focused investigation | Stable correlation labels and launch/capture guidance, not an in-engine clone | Unreal Insights-style trace tooling, PIX, RenderDoc, Nsight Systems/Graphics, RGP, RMV |
| D5 Hardware/source/crash | Which instruction, wave, cache, register, page fault, or crash marker explains it? | Specialist analysis | Selected marker/shader/resource identity and evidence links | Nsight shader tools/Aftermath, RGA/RGP, RGD, vendor counters |

Sparkle should own D0-D2 broadly and one bounded D3 GPU-marker workflow. D4-D5 belong to specialist tools. This boundary is both a product decision and the main defense against diagnostic code clutter.

### Horizontal Domain Map

| Domain | Essential data | Best initial visualization | Useful grouping/hierarchy | Escalation |
| --- | --- | --- | --- | --- |
| Frame and pacing | interval, budget, p50/p95/max, discontinuity, present policy, frame ID | trend strip plus summary rows | run -> frame; hitch list by time | PIX Timing, WPA/GPUView, Nsight Systems |
| CPU ownership | physical thread, logical phase, busy/wait/ready wall, task counts and delay | physical thread lanes plus sortable aggregate table | process -> named thread -> phase; task lane separately | WPA/PIX CPU/System trace with symbols |
| GPU execution | queue span, marker intervals, dependencies, timestamps, validity | queue lanes synchronized with marker tree/table | queue -> submission/batch -> RDG pass -> bounded child marker | PIX GPU Capture, Nsight GPU Trace, RGP |
| GPU API/state | draws/dispatches, barriers, pipelines, descriptors, inputs/outputs | event list and state/resource inspector | marker -> command buffer -> event -> state/resource | PIX or RenderDoc |
| RAM | working set, private commit, tracked used, allocator blocks, category, high-water | trend and category table | process -> category -> owner; allocation site only externally | WPA/heap tool/Memory Insights equivalent |
| GPU memory/residency | tracked resources, heap/block usage, local/non-local usage/budget, retirement, paging | budget trend, category table, optional snapshot delta | segment/heap -> allocation -> resource | PIX memory, RMV, Nsight/driver tool |
| Shader/hardware | shader identity, duration association, ISA, occupancy, registers, stalls, bandwidth/cache | selected-event details plus source/ISA/counter views | marker/event -> pipeline -> shader stage -> function/instruction | Nsight Shader Profiler, RGP/RGA |
| Workload/cardinality | passes, submissions, draws, dispatches, instances, triangles, uploads | compact counters beside timing | renderer stage -> owned count | Frame debugger or owner-specific capture |
| Compilation/loading | compile/load wall, queueing, cache hit/miss, hitch correlation | event annotations on frame trend and bounded aggregate | operation family -> stable asset/shader token | CPU/system trace and artifact logs |
| Correctness/crash | validation state, rejected work, device loss, last markers | status banner and marker breadcrumb | frame -> queue -> last completed/in-flight marker | Aftermath, RGD, debug layers |
| Evidence/compare | manifest, raw samples, before/after distributions, capture links, caveats | aligned comparison table | study -> run -> sample/capture | Repository evidence package |

No single visualization works across all columns. The common connective tissue is session/frame/range selection, stable semantic identity, consistent units, and explicit validity.

## Epic Games Product Study

### Diagnostic Ladder

Epic exposes a deliberate depth ladder rather than one universal profiler:

| Product surface | UX pattern | Lesson for Sparkle |
| --- | --- | --- |
| Stat commands and Unit/UnitGraph/GPU groups | Immediate in-application overlays; fixed, question-oriented groups; milliseconds and rolling graphs for orientation | Keep activation fast and vocabulary fixed. An overlay should say where to look, not attempt full causal proof. |
| GPU profiling/Profile Visualizer | A focused marked-frame hierarchy with hierarchical, flat, and coalesced ways to inspect events | Sparkle's RDG markers justify one frozen marker capture with inclusive/`exclusive (uncovered)` and instance/coalesced modes; the latter is not shader self time. |
| Timing Insights | Frames overview, synchronized CPU/GPU/thread tracks, range selection, aggregate timers/counters, callers/callees, logs | Use overview -> timeline/ranking -> details and one selection context. Do not reproduce call-tree or arbitrary trace analysis in Editor. |
| Task Graph and Context Switch Insights | Optional relationships/critical path and OS core scheduling views | Thread lanes and logical phases must remain distinct. Dependency arrows are useful only on demand because a full graph overwhelms. |
| Memory Insights | Memory timeline, explicit A/B interval queries, grouping by tag/asset/class/callstack/heap, symbol status | Sparkle should show totals/categories/high-water and later snapshot delta; allocation callstacks and arbitrary queries remain external. |
| Render Resource Viewer | Snapshot-oriented searchable/sortable resource table with totals and selected details | Resource inventory is a snapshot tool, not a per-frame live overlay. Existing Sparkle mesh/texture tools should not become duplicated memory truth. |
| RDG Insights and GPUDump Viewer | Graph/pass hierarchy, resource lifetimes, pass inputs/outputs, explicit dump workflow | Preserve RDG semantic names across Sparkle and external tools. A future graph/resource dump is separate from timing and not part of the first slice. |
| Attached PIX and RenderDoc capture | Tool attachment is requested at launch; each successful integration adds a provider-specific icon in the upper-right Level Viewport and the icon captures a frame. | Use a compact icon group with one action per requested/detected provider; multiple provider actions may coexist. Keep per-provider setup, compatibility, state, target viewport, and observer effect explicit. |

Epic's [Timing Insights](https://dev.epicgames.com/documentation/en-us/unreal-engine/timing-insights-in-unreal-engine-5) uses a frame graph for trend discovery, thread/GPU tracks for temporal context, aggregate timer/counter tables for ranking, and selected-event relationships for detail. [Memory Insights](https://dev.epicgames.com/documentation/en-us/unreal-engine/memory-insights-in-unreal-engine) similarly separates the overview timeline from explicit allocation queries and hierarchical breakdowns. [Render Dependency Graph](https://dev.epicgames.com/documentation/en-us/unreal-engine/render-dependency-graph-in-unreal-engine) makes graph structure and resource lifetime first-class and owns profiler scopes near pass declaration/execution.

Epic's current [PIX integration](https://dev.epicgames.com/documentation/en-us/unreal-engine/using-pix-on-windows-with-unreal-engine) documents `-AttachPix`, an upper-right Level Viewport PIX icon after attachment, and an in-editor single-frame capture action. Its [RenderDoc integration](https://dev.epicgames.com/documentation/en-us/unreal-engine/using-renderdoc-with-unreal-engine) documents the parallel `-AttachRenderDoc` flow and RenderDoc Capture button. Sparkle adopts the conditional provider-action pattern, not those exact flags, plugin boundaries, or a claim that attachment guarantees a valid capture.

### What Not To Copy

- Sparkle has no need for Unreal's breadth of stat groups before corresponding systems and acceptance workloads exist.
- An Insights-scale trace store, query engine, symbol resolver, caller/callee analysis, and arbitrary dockable track system would conflict with the current product boundary.
- Per-subsystem macros scattered through gameplay are not the starting point. Instrument stable owner boundaries and derive existing facts first.
- A dedicated window per diagnostic noun makes the Editor harder to understand. Sparkle's much smaller scope benefits from one Performance workspace.

### Epic Engineering Practice To Preserve

Epic's published product ladder is also a lesson in choosing evidence depth. The [performance-profiling introduction](https://dev.epicgames.com/documentation/en-us/unreal-engine/introduction-to-performance-profiling-and-configuration-in-unreal-engine) calls Stats the fastest orientation path, Unreal Insights the robust trace path, and RenderDoc the detailed single-frame state path; it also warns that profiling changes the measured workload. [Timing Insights](https://dev.epicgames.com/documentation/en-us/unreal-engine/timing-insights-in-unreal-engine-5) makes a selected time range drive both temporal tracks and aggregate tables. [Task Graph Insights](https://dev.epicgames.com/documentation/unreal-engine/task-graph-insights-in-unreal-engine-5) keeps dependency relationships optional instead of drawing a permanent wall of arrows.

The public Trace architecture is instructive but intentionally too large for Sparkle. Epic separates high-frequency channelized recording, a UI-less trace server/store, analysis services/providers, and presentation. That separation validates Sparkle's immutable producer/session/presenter boundary; it does not justify copying a transport, trace database, symbol service, extensible track framework, or file lifecycle before a workload requires them. Epic Technical Developer Relations presentations such as [Profiling with Purpose](https://www.youtube.com/watch?v=C-AjCqjKRSs) and Epic's Ken Kuwano's [Unreal Insights performance-analysis case studies](https://www.youtube.com/watch?v=HQLYkwoDoT4) reinforce a problem-driven story: establish the question, isolate the critical interval, use the appropriate evidence, and explain the decision. Sparkle's portfolio should demonstrate that method, not merely familiarity with menus.

## NVIDIA Product Study

### Nsight Graphics

Nsight Graphics separates activities by question: frame capture/debugging, GPU Trace, shader profiling/debugging, and crash-dump inspection. Its GPU Trace presents multiple GPU queues and actions against hardware metric rows, then drives summary, metrics, shader, and information panes from a selected event or range. The trace can show queue hierarchy or flatter views, synchronize zoom and selection, hide/pin rows without discarding data, and explicitly marks ranges with no samples.

The strongest reusable patterns are:

- timeline and marker hierarchy are coordinated views of the same capture;
- selecting a marker or range updates details instead of opening unrelated windows;
- missing samples are visible, never drawn as zero;
- high-volume rows are focusable/hideable while headline rows stay pinned;
- analysis is based on a declared selection and exposes capture conditions;
- marker-based trace comparison can align semantically corresponding regions;
- hardware recommendations are clearly a deeper, capture-dependent layer.
- target-application capture can be triggered by a hotkey or, with the current beta Nsight Graphics SDK, a native application button; this remains a capability-gated external activity rather than a permanent Sparkle toolbar.

Nsight's [GPU Trace UI](https://docs.nvidia.com/nsight-graphics/UserGuide/gpu-trace-ui.html) describes queue/action/marker rows, metric graphs, event/details panes, range-driven summaries, marker-tree analysis, and trace comparison. Its [GPU Trace overview](https://docs.nvidia.com/nsight-graphics/UserGuide/gpu-trace-overview.html) makes multi-frame GPU-unit, synchronization, and under-utilization diagnosis the product boundary. Those are external-tool responsibilities for Sparkle, not arguments for embedding NVIDIA counters in the Editor.

### Nsight Systems And Aftermath

- Nsight Systems supplies the wider CPU/OS/GPU scheduling timeline when an engine-local phase view cannot explain ready time, preemption, driver activity, or WDDM queueing.
- Nsight Aftermath is a crash-oriented path: marker breadcrumbs, shader fault association, and GPU mini-dumps. Sparkle should preserve stable GPU markers and crash configuration, not implement its own crash-dump inspector.

The product lesson is explicit activity selection. A user investigating frame pacing should not be dropped into a shader source view; a user investigating a shader should enter from a selected event that carries shader identity.

### NVIDIA Engineering Practice To Preserve

NVIDIA's published material supplies two especially important methods for the portfolio:

- Louis Bavoil's [Peak-Performance-Percentage method](https://developer.nvidia.com/blog/what-is-limiting-your-rendering-performance/) begins with GPU active and top throughput metrics, identifies a likely saturated unit only when the counter evidence supports it, removes work from that unit, and validates the predicted speedup. A duration hierarchy selects the range; it does not substitute for unit counters.
- Jon Kennedy's [thread-count guidance for games](https://developer.nvidia.com/blog/limiting-cpu-threads-for-better-game-performance/) shows why `logical cores - 2` is not a universal policy. Cache topology, SMT, P/E cores, memory contention, locks/atomics, false sharing, scheduling, context switches, and power behavior can make fewer workers faster. Sparkle therefore needs a serial/1/2/N study and machine topology in evidence, not a decorative worker-utilization graph.

The practical NVIDIA escalation is top-down:

1. stabilize the workload and configuration; record clock/power state where the tool exposes it;
2. if GPU active is low or the queue is starved, use Nsight Systems or a platform system trace to examine CPU scheduling, API submission, synchronization, and driver gaps;
3. if GPU active is high, use Nsight Graphics GPU Trace to rank unit pressure and select the semantic range;
4. use shader profiling/source/ISA only after the selected marker and limiter justify that depth;
5. change one suspected cause and remeasure the whole route, not only the optimized event.

[Nsight Perf SDK](https://developer.nvidia.com/nsight-perf-sdk) is an option on the table because it demonstrates low-overhead in-application periodic counter/HUD and range-report workflows. It is not selected for Sparkle's core diagnostics: it is NVIDIA-specific, profiling sessions are a shared GPU resource, available counters vary, and embedding it would consume roadmap and portability budget before `MAP-00`. It may be evaluated later as an optional lab adapter only when a named NVIDIA study cannot be served by Nsight Graphics captures. Nsight Aftermath remains the NVIDIA crash path; stable markers and build/shader identity are Sparkle's integration obligation.

## AMD Product Study

### Radeon GPU Profiler

RGP uses an Overview first, then coordinated event-level panes:

- frame/profile summary establishes command-buffer timing and system activity;
- barrier, context-roll, render/depth-target, pipeline, and most-expensive-event views offer question-specific rankings;
- wavefront occupancy places events against GPU activity;
- event timing exposes a tree/grouped list;
- pipeline state, shader source/ISA, and instruction timing deepen the selected event;
- selections and navigation carry across views;
- nested user debug markers become the main application-owned hierarchy.

The [RGP Overview](https://gpuopen.com/manuals/rgp_manual/overview_windows/) is particularly effective at putting a small number of expensive-event and synchronization questions before specialist detail. The [Events views](https://gpuopen.com/manuals/rgp_manual/events_windows/) coordinate occupancy, timing, event grouping, pipeline state, source/ISA, and instruction timing around the same event selection. Sparkle should copy the continuity of selection and stable marker hierarchy, not AMD-specific hardware views.

### Radeon Memory Visualizer, GPU Analyzer, And GPU Detective

| Tool | Primary workflow | Sparkle handoff obligation |
| --- | --- | --- |
| Radeon Memory Visualizer | Trace overview, heap/resource/allocation views, point-in-time snapshots, snapshot comparison for oversubscription/fragmentation/leaks | Expose adapter/backend, local/non-local budget/usage, stable resource names where available, and a reproducible route. |
| Radeon GPU Analyzer | Shader source/disassembly and resource-pressure analysis per target GPU | Preserve shader/package/entry-point identity from selected pass to existing shader artifacts. |
| Radeon GPU Detective | Crash analysis with execution-marker hierarchy, command buffers, page-fault information, and machine-readable reports | Emit stable nested markers and retain exact build/backend/device metadata. |

RMV's [quick start](https://gpuopen.com/manuals/rmv_manual/quickstart/) reinforces the difference between a time-varying trace and explicit snapshots. Public GPUOpen repositories also make useful architecture visible in the [Radeon Memory Visualizer](https://github.com/GPUOpen-Tools/radeon_memory_visualizer), [Radeon GPU Analyzer](https://github.com/GPUOpen-Tools/radeon_gpu_analyzer), and [Radeon GPU Detective](https://github.com/GPUOpen-Tools/radeon_gpu_detective): capture/analysis backends, domain models, UI, CLI/export, and tests are separable concerns. This supports keeping Sparkle presentation independent of producer implementation and treating export as a deliberate product boundary.

### AMD Engineering Practice To Preserve

AMD's suite is strongest when treated as a set of distinct questions rather than one capture mode:

| Question | AMD authority | Portfolio use |
| --- | --- | --- |
| Where is GPU time and overlap going? | RGP queue/events/barriers/occupancy/pipeline views | Select a marked range, identify a competing limiter, and preserve the capture plus experiment. |
| Why is GPU memory growing or fragmented? | RMV event timeline and point snapshots | Compare `A: before load`, `B: settled/warm`, and `C: after unload + retirement`; explain allocation, residency, fragmentation, and retained-pool differences. |
| Is an RT structure expensive? | [Radeon Raytracing Analyzer](https://gpuopen.com/manuals/rra_manual/) | Inspect TLAS/BLAS counts, memory, instances, geometry/BVH structure, and ray dispatch behavior; join back to the same workload route. |
| What does the shader compile to? | RGA | Attach target-GPU ISA/resource-pressure evidence to a selected shader/package/entry point. |
| Why is CPU work slow? | [AMD uProf](https://docs.amd.com/r/en-US/57368-uProf-user-guide/uProf-User-Guide) | Separate thread timeline/concurrency and wait hotspots from source hotspots; use PMC/IBS data-access, cache, branch, CPI/IPC, and false-sharing evidence only for a predeclared CPU hypothesis. |
| Why did the GPU fault? | RGD | Preserve last marker hierarchy, page-fault context, machine/build identity, and the reduced reproducer. |

Chris Hesik's [RGP/Radeon Developer Panel guidance](https://gpuopen.com/learn/rgp-1-13-rdp-2-6/) explicitly recommends profiling-only mode unless simultaneous memory tracing is needed, because combining modes adds overhead. This becomes a general Sparkle rule: enable the minimum collector that can distinguish the current hypotheses, make observer mode visible, and never compare runs with silently different capture modes. AMD's RMV workflow also demonstrates that a memory trace and a point-in-time snapshot answer different questions; Sparkle must not flatten both into one ambiguous GPU-memory number.

## Integration Architecture Study

The public products differ internally, but their observable integration boundaries converge. This is the architectural pattern worth adopting; it avoids copying any vendor's private implementation.

```text
engine owners                     capture/session             analysis                  presentation
thread/task/runtime scopes ----> bounded event stream -----> immutable domain model --> overlay/timeline/tree
RDG semantic pass scopes ------> native GPU markers -------> external profiler DB ----> PIX/Nsight/RGP/RenderDoc
RHI timestamp/backend facts ---> delayed frame result ------> join by FrameId ---------> Performance workspace
allocator/OS memory facts -----> sampled snapshot/ring ----> definitions + aggregates -> Memory view/evidence
```

| Layer | Epic/NVIDIA/AMD precedent | Sparkle consequence |
| --- | --- | --- |
| Semantic annotation | Unreal RDG places scopes at graph/pass ownership. Nsight and RGP use application debug-marker hierarchies to correlate tool data with engine intent. RGP officially recommends native PIX3 events for D3D12 and accepts Vulkan debug markers. | Frame graph and RHI own automatic pass/queue markers. One semantic scope fans out to Sparkle timing and native tool markers; feature code does not call vendor tools directly. |
| Runtime collection | Trace channels, GPU trace/capture activities, and explicit memory traces/snapshots make expensive data opt-in. | `Off`, Basic, Detailed, frozen GPU capture, benchmark, and external capture are explicit demand modes with visible observer state. |
| Transport/artifact | Unreal traces, Nsight reports/captures, and Radeon trace/crash files separate runtime recording from later analysis. | Live rings and one frozen result remain bounded in-process; portfolio/capture files exist only through explicit workload-owned export. |
| Analysis model | Timing Insights aggregates a selected range; Nsight analysis is selection/range driven; RGP/RMV parse captures into event/memory models before UI navigation. | Producers publish facts, Application joins them, and presenters read an immutable model. ImGui code does not calculate ownership, resolve timestamps, traverse live renderer caches, or decide memory truth. |
| Presentation | Summary, timeline, hierarchy/table, and details are coordinated but replaceable views over one session/capture. | Compact Stats and Performance reuse one model and one selection. UI state is view/focus state, not diagnostic authority. |
| External specialization | Vendor tools own replay, counters, ISA, wave/occupancy, driver queues, crash dumps, call stacks, and resource state. | Stable names, `FrameId` bookmarks, shader/resource identity, and configuration make handoff cheap; those datasets are not pulled into the engine merely to imitate a profiler. |

Epic's [tracing guide](https://dev.epicgames.com/documentation/en-us/unreal-engine/developer-guide-to-tracing-in-unreal-engine) describes channelized event recording and later analysis, while AMD's [user-marker guidance](https://gpuopen.com/manuals/rgp_manual/user_debug_markers/) shows why standard API-native markers are a portability surface: one D3D12 PIX marker vocabulary can remain useful in PIX and RGP. This directly supports Sparkle's existing D3D12 PIX events and Vulkan debug-label path.

### Transparent Usage Contract

“Transparent” must mean low ceremony and shared ownership, not invisible cost:

- normal engine systems receive automatic top-level scopes from their owner boundary;
- the user asks questions through fixed views or one capture action, not by configuring a graph of collectors;
- the same stable pass name appears in overlay, frozen capture, and external tools;
- a deep mode visibly declares that it is collecting extra timestamps/counters or is under an external profiler;
- removing the UI does not remove the semantic engine ownership, and opening UI does not synchronously interrogate mutable renderer state;
- disabling all consumers releases collection demand; it does not leave an unnoticed background trace.

The smallest useful integration primitive is therefore an owner-level semantic token plus an RAII/timestamp/marker adapter, not a global profiler macro available everywhere. A marker is added because it defines a stable region a user can act on, not because a function happens to exist.

## Cross-Product UX Laws

The three ecosystems converge on the following design laws:

1. Start with a cheap summary, then deepen. A first screen should name the likely domain and the quality of the evidence.
2. Preserve one selection. Frame, time range, marker, thread, resource, and comparison selections must be explicit and must drive all visible details.
3. Pair temporal and aggregate views. A timeline explains overlap/order; a table/tree explains ranking and hierarchy. Neither substitutes for the other.
4. Keep hierarchy semantic. Queue/thread is the physical owner; pass/phase/task is the logical work. Nesting must not imply additive totals across overlapping work.
5. Make capture state visible. Live, armed, resolving, frozen, imported, invalid, stale, and externally captured are different modes.
6. Show absence honestly. Pending, unsupported, no submitted work, dropped, stale, and filtered are not `0`.
7. Keep configuration attached. Backend, adapter, resolution, VSync, validation, renderer mode, sample window, and observer mode belong in the viewport/workspace banner and exported evidence.
8. Filtering changes visibility, not totals. Hidden rows and search results must not silently redefine percentages or unattributed time.
9. Use stable names for navigation. An RDG pass selected in Sparkle should be findable by the same semantic path in PIX, RenderDoc, Nsight, or RGP.
10. Separate measurement from recommendation. A timestamp or utilization graph is an observation; a cause requires a controlled experiment or specialized capture.
11. Save/share only on explicit action. Live orientation must not create files. A frozen capture or benchmark can be deliberately exported with its manifest.
12. Reveal observer effects. Detailed timestamps, validation, hardware counters, replay, or multi-pass collection can perturb work and must be labeled.
13. Ask for intent, not collector configuration. `Quick Check`, `Investigate GPU`, and `Capture Evidence` are primary tasks; queue names, trace modes, counters, buffers, and cache mechanics are derived or advanced detail.
14. Make the recommended path the shortest path. Capability detection, minimum collection mode, correlation, default target/tool choice, and manifest population should be automatic and validated.
15. Use progressive disclosure. One summary and next action lead; selection-specific timelines/tables follow; raw events, hashes, manifests, compiler state, and expert overrides remain searchable details.
16. Prevent invalid setups. Hide impossible options, disable temporarily unavailable actions with the prerequisite beside them, and validate a complete operation before collection rather than failing after a long capture.
17. Preserve an expert escape hatch without making it the default. Advanced controls show their cost, capability scope, and difference from a named preset and can be reset in one action.
18. Preserve navigation context. A selected frame/range/marker/configuration must flow into the next view or external-tool checklist; users should not re-enter identities to continue one investigation.
19. Make external capture controls conditional and composable. Every requested or detected provider earns one compact action, so multiple icons may coexist; no provider means no icon group. Failed or conflicting setup remains disabled on the affected provider with remediation, and unrequested providers never become inert logos.

This is consistent with the reviewed production frontends. Epic exposes a saved-edit plus `recompileshaders changed` workflow rather than compiler-job construction, and Timing Insights moves from frame/range overview into selection-driven tracks and callers/callees. Nsight GPU Trace shows explicit collection state, frames/queues first, then event details and analysis; RGP provides an Overview with most-expensive events and context navigation into event, pipeline, occupancy, and ISA panes. Sparkle should adopt that task-to-detail progression, not their total pane count or vendor-specific datasets. [Epic Shader Development](https://dev.epicgames.com/documentation/en-us/unreal-engine/shader-development-in-unreal-engine), [Epic Timing Insights](https://dev.epicgames.com/documentation/en-us/unreal-engine/timing-insights-in-unreal-engine-5), [Nsight GPU Trace UI](https://docs.nvidia.com/nsight-graphics/UserGuide/gpu-trace-ui.html), [RGP Overview windows](https://gpuopen.com/manuals/rgp_manual/overview_windows/)

## Sparkle Product Options

| Option | Strength | Failure mode | Decision |
| --- | --- | --- | --- |
| FPS/frame-time only | Almost no UI or engineering cost | Cannot separate CPU/GPU/present, hides distributions and memory, creates guess-driven optimization | Reject |
| Many independent stat panels | Each feature can ship locally | Duplicated snapshots, provider sprawl, inconsistent selection/units, window clutter | Reject |
| External profilers only | Maximum causal depth with little in-engine UI | Poor everyday orientation, harder capture setup, weak portfolio first screen, no cross-tool definition authority | Reject |
| General in-engine Insights clone | One very powerful analysis environment | Large trace/query/symbol/UI platform outside roadmap; permanent instrumentation and maintenance burden | Reject |
| Vendor-counter HUD in engine | Hardware-limiter hints without leaving the app | Vendor coupling, counter availability/replay cost, cross-vendor semantic gaps, high validation burden | Defer/reject as a core surface |
| Separate standalone Sparkle profiler app | Keeps heavy UI out of Editor | Requires storage/protocol/tooling platform before core measurements are mature | Defer; reconsider only if a named workload demands D4 analysis |
| Hybrid diagnostics ladder | Cheap overlay + one joined Performance workspace + bounded GPU capture + external handoff | Requires disciplined shared identity and view scope | Select |

## Selected Direction And Canonical Handoff

The hybrid diagnostics ladder is the selected result of this research: cheap compact orientation, one joined Performance workspace, a bounded captured-frame GPU view, explicit evidence export, and stable handoff to specialized external tools. The detailed specification now lives with its durable owners instead of being repeated in this research record.

| Selected concern | Adopted boundary | Canonical owner |
| --- | --- | --- |
| Product depth and surfaces | Task-first Quick/CPU/GPU/Memory entry, an initially bounded compact-overlay set, one fixed Overview/CPU/GPU/Memory workspace, and external tools for causal depth. Raw stat groups remain expert customization, not the first menu. | [Product information architecture](PerformanceDiagnosticsArchitecture.md#diagnostics-product-information-architecture) |
| Physical/logical ownership and metric meaning | Physical CPU thread rows, logical phases inside their real owners, independent GPU queues, distinct memory definitions, explicit validity. | [Measurement and view contracts](PerformanceDiagnosticsArchitecture.md#measurement-vocabulary) |
| Selection and interaction | One shared frame/range/object selection; hitches are frame selections; Sparkle GPU capture stays inside the GPU view; an attached external provider may add one targeted viewport action; filtering never changes totals. | [Workspace interaction contract](PerformanceDiagnosticsArchitecture.md#workspace-interaction-contract) |
| Visual and failure behavior | Milliseconds lead, color is never the only signal, configuration remains visible, and pending/stale/unsupported/lost states never masquerade as zero. | [Visual and accessibility rules](PerformanceDiagnosticsArchitecture.md#visual-validity-and-accessibility-rules) |
| Concrete product presentation | Integrated graphical mockups, every compact group, fixed workspace views, evidence actions, keyboard baseline, and failure states. | [Performance Diagnostics Visual Design And Tool Wireframes](PerformanceDiagnosticsAsciiWireframes.md) |
| Resource inspector boundary | Existing shader, mesh, and texture tools remain asset inspectors unless an accepted migration removes the old route; they do not own system performance truth. | [Product surfaces and depth boundary](PerformanceDiagnosticsArchitecture.md#product-surfaces-and-depth-boundary) |

This handoff preserves the adopted decisions while keeping research focused on precedents, options, tradeoffs, and why the hybrid direction won. The following low-clutter principles remain here because they summarize the implementation consequences drawn from the research.

## Low-Clutter Integration Principles

The UX alone cannot prevent instrumentation clutter. The selected functionality imposes these implementation-facing constraints without designing the collector here:

- One presentation-neutral diagnostics product feeds overlays, Performance, benchmark summaries, and export.
- Normal entry points are user questions/presets; the fixed group catalog is available through contextual customization and the console, not displayed as twelve equal choices.
- UI panels consume immutable snapshots and issue typed semantic requests. They do not include renderer snapshots directly, synchronously query live engine state, or define measurement semantics.
- Instrument stable owner boundaries: application frame, named physical threads, scheduler lanes, renderer phases, frame-graph passes, RHI queue/timestamps, allocator/memory authority.
- RDG pass declaration/execution supplies semantic GPU scopes automatically. Feature code adds a child marker only when it answers an accepted workload question.
- Counts are incremented where normal work already occurs; no diagnostics-only scene/resource rescan.
- Hot records use fixed IDs/tokens and bounded storage. Strings are resolved outside capture paths.
- Deep modes are explicit and temporary. Closing the last consumer releases demand; no hidden always-on trace.
- Adding a view requires an authoritative producer, declared cost, bounded model, UX slot, validation plan, and an external escalation boundary.
- When a new route supersedes one of the current Editor diagnostic provider callbacks or standalone panels, remove the replaced path in the same accepted migration. Do not create a second compatibility system.

## Current Sparkle UX Reconciliation

The 2026-08-15 source reconciliation shows useful foundations but no joined performance product:

- `ViewportTopPanel::BuildPerformanceStats` displays ImGui's smoothed FPS and delta time only.
- `MainMenuBarPanel` opens Settings, Shaders, Meshes, and Textures utility windows and has an explicit viewport-capture action, but no Performance workspace.
- `UIWorkspace` has a clear fixed outliner/viewport/inspector composition and builds the current utility panels separately.
- `UI.h` publicly includes renderer diagnostic snapshot types and stores mesh, texture, and memory provider callbacks. This is evidence to avoid extending that callback pattern for CPU/GPU performance.
- Used Meshes and Used Textures refresh a captured snapshot when opened; Used Shaders has explicit refresh and artifact tabs. These are useful interaction precedents for snapshot/status/detail, not a whole-system diagnostics architecture.
- Renderer/RHI already have scoped GPU events/timers, frame-graph pass/chunk scopes, D3D12 PIX events, Vulkan debug labels, timestamp allocation/resolution, memory diagnostics, and task ETW profiling. The UX can reuse their semantic vocabulary after the ownership/publication design is accepted.

This reconciliation is not a request to refactor the current panels now. It establishes the integration pressure the later implementation plan must resolve.

## Portfolio And Reviewer Presentation

One implementation slice should intentionally produce four readable artifacts:

1. Hero: Sponza viewport plus compact `Stat Unit`/memory summary and exact configuration.
2. Triage: Performance Overview showing frame distribution, physical CPU owners, GPU queue/pass ranking, process RAM plus local/non-local GPU memory, likely domain, and next action.
3. Focused GPU: captured marker timeline/tree with inclusive/`exclusive (uncovered)` columns and a selected pass path found in PIX/Nsight/RGP.
4. Experiment: aligned baseline/change table with p50/p95/p99 or required percentiles, high-water values, capture links, and causal caveat.

The portfolio narrative should be `symptom -> reliable baseline -> Sparkle orientation -> external capture -> hypothesis -> one controlled change -> distribution/result -> limitations`. A beautiful profiler screen without a reproducible route and causal experiment is decoration, not engineering evidence.

## Adoption Order

This research recommends design validation in this order; it is not an implementation schedule:

1. Validate terminology, units, frame selection, configuration ribbon, and Overview wireframe against `MAP-00`.
2. Validate physical CPU thread/phase lanes and GPU queue/marker hierarchy against actual Sparkle topology.
3. Validate Memory definitions and category ownership before drawing any graph.
4. Prototype Performance interactions using fake immutable data: live/frozen, frame/range selection, pending/invalid, filters, inspector, GPU capture state.
5. Only after the product questions survive review, design producer/publication slices in the canonical architecture.

## Source Quality, Adoption, And Revalidation

The catalog below is evidence for precedent, not a flat list of equally current authorities. Use sources according to what they can actually support:

| Source kind | Appropriate use | Misuse to avoid |
| --- | --- | --- |
| Current API specification/reference | Timestamp stage/frequency/valid bits, calibration deviation, heap-budget scope, presentation stages, and other normative API semantics. | Inferring Sparkle ownership, cost, or tool UX from the API alone. |
| Current tool manual/support matrix/release page | Current API/OS/hardware capability, activity distinction, marker support, and known limitations. | Treating a successful capture on one system as universal support. |
| Current engine feature documentation | Product ladder, information architecture, terminology, and interaction precedent. | Copying engine internals or assuming the wrapper page's examples are current implementation truth. |
| Historical blog/tutorial/video | Investigation method, teaching pattern, and design rationale that remains useful. | Using its version, performance number, capability matrix, or screenshots as current proof. |
| API/class index | Discovering names and navigation entry points. | Treating an index as a semantic or UX specification. |

Specific reconciliations from this study:

- Epic's realtime GPU profiling page retains useful diagnostic-ladder ideas but contains historical UE4-era material; the current Timing Insights, Memory Insights, Stat Commands, and RDG documents are stronger current product sources.
- Epic's ProfileVisualizer page is mainly an API/class index. It supports vocabulary discovery, not a complete claim about current user workflow or measurement semantics.
- RenderDoc release/support material is current; its Vulkan wiki is an older workflow aid and must not anchor a current capability claim.
- Current NVIDIA material distinguishes GPU Trace/Shader Profiler support on D3D12 and Vulkan from the live Shader Debugger's Vulkan-only support. "Nsight supports this API" is too broad unless the activity is named.
- Current AMD material distinguishes RGP profiling, RMV trace/snapshot/comparison, RRA acceleration-structure analysis, and RGD crash breadcrumbs. Their metrics and artifacts are complementary, not interchangeable.
- The 2022 AMD capture-overhead article remains good method precedent, but its versions and support statements are historical.
- PIX Comparison is useful precedent for exposing selected sample `N`, histograms, p-values, and low-sample warnings. Sparkle adopts visibility of population and distribution, not a p-value-only regression policy; the canonical architecture owns practical effect bands, per-run results, correlation-aware uncertainty, and `Inconclusive`.

When a source changes, record the narrow claim, source section, version/date, adopted behavior, what was not inferred, and revalidation trigger. The [profiler runbook's current matrix](DiagnosticsProfilerRunbook.md#source-and-version-reconciliation) owns operational versions; this research keeps only the durable product lesson.

## Source Catalog

### Epic Games

- [Introduction to Performance Profiling and Configuration](https://dev.epicgames.com/documentation/en-us/unreal-engine/introduction-to-performance-profiling-and-configuration-in-unreal-engine)
- [Stat Commands](https://dev.epicgames.com/documentation/unreal-engine/stat-commands-in-unreal-engine)
- [Timing Insights](https://dev.epicgames.com/documentation/en-us/unreal-engine/timing-insights-in-unreal-engine-5)
- [Frames Panel](https://dev.epicgames.com/documentation/en-us/unreal-engine/using-the-frames-panel-in-unreal-insights-for-unreal-engine)
- [Memory Insights](https://dev.epicgames.com/documentation/en-us/unreal-engine/memory-insights-in-unreal-engine)
- [Task Graph Insights](https://dev.epicgames.com/documentation/unreal-engine/task-graph-insights-in-unreal-engine-5)
- [Context Switches](https://dev.epicgames.com/documentation/en-us/unreal-engine/context-switches-in-unreal-engine-5)
- [Render Dependency Graph](https://dev.epicgames.com/documentation/en-us/unreal-engine/render-dependency-graph-in-unreal-engine)
- [Shader Development and changed-shader iteration](https://dev.epicgames.com/documentation/en-us/unreal-engine/shader-development-in-unreal-engine)
- [Render Resource Viewer](https://dev.epicgames.com/documentation/unreal-engine/render-resource-viewer-in-unreal-engine)
- [GPUDump Viewer](https://dev.epicgames.com/documentation/en-us/unreal-engine/gpudump-viewer-tool-in-unreal-engine)
- [Using PIX on Windows with Unreal Engine](https://dev.epicgames.com/documentation/en-us/unreal-engine/using-pix-on-windows-with-unreal-engine)
- [Using RenderDoc with Unreal Engine](https://dev.epicgames.com/documentation/en-us/unreal-engine/using-renderdoc-with-unreal-engine)
- [ProfileVisualizer API](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Developer/ProfileVisualizer)
- [Developer Guide to Tracing](https://dev.epicgames.com/documentation/en-us/unreal-engine/developer-guide-to-tracing-in-unreal-engine)
- [Trace architecture overview](https://dev.epicgames.com/documentation/en-us/unreal-engine/trace-in-unreal-engine-5)
- [Profiling with Purpose: Performance Lessons from a Real Unreal Project](https://www.youtube.com/watch?v=C-AjCqjKRSs)
- [Mastering Performance Analysis with Unreal Insights](https://www.youtube.com/watch?v=HQLYkwoDoT4)

### NVIDIA

- [Nsight Graphics Features](https://developer.nvidia.com/nsight-graphics-features)
- [GPU Trace Overview](https://docs.nvidia.com/nsight-graphics/UserGuide/gpu-trace-overview.html)
- [GPU Trace UI](https://docs.nvidia.com/nsight-graphics/UserGuide/gpu-trace-ui.html)
- [Shader Profiler](https://docs.nvidia.com/nsight-graphics/UserGuide/shader-profiler.html)
- [Configuring applications for Nsight Graphics correlation](https://docs.nvidia.com/nsight-graphics/UserGuide/configure-application.html)
- [Nsight Graphics SDK in-application capture and trace control](https://docs.nvidia.com/nsight-graphics/UserGuide/sdk.html)
- [Nsight Systems User Guide](https://docs.nvidia.com/nsight-systems/UserGuide/index.html)
- [Peak-Performance-Percentage rendering analysis](https://developer.nvidia.com/blog/what-is-limiting-your-rendering-performance/)
- [Limiting CPU threads for better game performance](https://developer.nvidia.com/blog/limiting-cpu-threads-for-better-game-performance/)
- [Nsight Perf SDK](https://developer.nvidia.com/nsight-perf-sdk)
- [Nsight Aftermath](https://developer.nvidia.com/nsight-aftermath)

### AMD

- [Radeon GPU Profiler Quick Start](https://gpuopen.com/manuals/rgp_manual/quickstart/)
- [RGP Overview Windows](https://gpuopen.com/manuals/rgp_manual/overview_windows/)
- [RGP Events Windows](https://gpuopen.com/manuals/rgp_manual/events_windows/)
- [RGP User Debug Markers](https://gpuopen.com/manuals/rgp_manual/user_debug_markers/)
- [Radeon Memory Visualizer Quick Start](https://gpuopen.com/manuals/rmv_manual/quickstart/)
- [Radeon Memory Visualizer Snapshot views](https://gpuopen.com/manuals/rmv_manual/snapshot_windows/)
- [Radeon Raytracing Analyzer](https://gpuopen.com/manuals/rra_manual/)
- [Radeon GPU Analyzer](https://gpuopen.com/rga/)
- [AMD uProf User Guide](https://docs.amd.com/r/en-US/57368-uProf-user-guide/uProf-User-Guide)
- [RGP/Radeon Developer Panel capture-overhead guidance](https://gpuopen.com/learn/rgp-1-13-rdp-2-6/)
- [Radeon GPU Detective tutorial](https://gpuopen.com/learn/rgd_1_0_tutorial/)
- [Radeon Memory Visualizer repository](https://github.com/GPUOpen-Tools/radeon_memory_visualizer)
- [Radeon GPU Analyzer repository](https://github.com/GPUOpen-Tools/radeon_gpu_analyzer)
- [Radeon GPU Detective repository](https://github.com/GPUOpen-Tools/radeon_gpu_detective)

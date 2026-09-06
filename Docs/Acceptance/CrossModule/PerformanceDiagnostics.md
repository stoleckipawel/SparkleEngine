# Performance Diagnostics Acceptance Contract

Status: acceptance contract; not proof that diagnostics or workloads have passed

Scope: diagnostic evidence fields, baseline experiments, benchmark artifacts, reviewer paths, and verification gates for accepted workloads

Architecture authority: [Performance Diagnostics Architecture](../../Architecture/CrossModule/PerformanceDiagnostics.md)

Delivery authority: [Performance Diagnostics Delivery Plan](../../Plans/CrossModule/PerformanceDiagnostics.md)

Workload authority: [Graphics Workloads](../GraphicsWorkloads.md)

Engineering evidence rules: [Validation, Performance, and Evidence](../../Engineering/Verification/ValidationAndEvidence.md)

This contract owns the diagnostic evidence expected from acceptance workloads, baseline experiments, reviewer artifacts, and verification gates. Results must retain exact commands, configuration, environment, artifacts, and limitations.

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

Current tool/version caveats and source links live in the [profiler runbook](../../Engineering/Verification/ExternalProfiling.md#input-to-display-options).

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

## Benchmark And Portfolio Evidence

The live Editor screenshot is one portfolio artifact, not the result. Conceptually, a reviewer-ready performance case includes the following files under the workload-owned run directory; [Graphics Workloads](../GraphicsWorkloads.md) remains authoritative for exact names and placement:

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

## Verification Contract

Implementation acceptance requires focused tests and measured runs, as applicable:

### Authoring Isolation And Shipping Erasure

Every vertical slice passes the [authoring-isolation and Shipping-erasure](#authoring-isolation-and-shipping-erasure) checks in the same change. Reviewers inspect the real content/feature path as well as diagnostics files. Acceptance is blocked by a new authoring step, collector selection in production code, diagnostic-only state in a real-work owner, an optional profiler dependency in Shipping, or optimized Shipping object code/data that retains the diagnostic seam. Final hardening repeats this proof; it does not defer it.

### Frontend Workflow And Clutter

- Scenario tests drive the immutable presentation model through `Quick Check`, `Investigate CPU`, `Investigate GPU`, `Investigate Memory`, `Capture Evidence`, attached-provider ready/armed/finalizing/completed/unavailable states, cancellation, and failure without constructing console strings or mutating collectors/provider APIs from UI code.
- The normal route from viewport orientation to one selected domain requires no knowledge of stat-group names, collection modes, timestamp queries, counters, backend targets, hashes, manifests, or external-tool configuration. A test or structured UI audit rejects any normal path that makes one of those fields mandatory.
- The first-level viewport menu contains the task intents, current state/cost, `Customize Stats...`, and open/hide controls; raw group names remain behind customization. Workspace snapshots verify that contextual capture/export/recovery commands do not become a permanent wall of equal toolbar actions.
- Header snapshots verify zero providers, each single-provider state, multiple compatible providers visible together in stable order, mixed ready/unavailable providers, and a compact group containing only requested/detected providers. Every disabled icon exposes provider-specific remediation.
- Every unavailable or unsafe action is disabled with one visible prerequisite. The frontend cannot express contradictory collection requests, start a second exclusive capture, export invalid evidence, or silently enable a more perturbing mode.
- Switching views and launching a contextual investigation preserves the selected frame/range and follows a typed object only where correlation is valid. Failures preserve that context and show one root cause, one next action, and a details route to raw evidence.
- Advanced overrides are typed, validated, resettable, scoped to the current operation or saved preset, and rendered as a difference from the recommended configuration. Closing and reopening the workspace cannot silently promote them to defaults.
- Keyboard-only navigation reaches every task intent, view, selected row, Inspector action, configuration detail, and recovery action with visible focus. Accessible names and status text communicate validity/cost without relying on color.
- Representative engine users perform the orientation-to-investigation, failed-capture recovery, and evidence-export tasks from a clean UI state. Record completion, time, wrong turns, backtracking, and expert-setting exposure; unresolved repeated misconfiguration or terminology confusion blocks frontend acceptance.

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
- DevelopmentEditor external capture: PIX on D3D12; RenderDoc on D3D12 and Vulkan; Nsight Graphics Capture on its supported NVIDIA D3D12/Vulkan matrix while explicitly `Experimental`.
- For each provider, test no flag/no injection, requested but missing, passively attached, ready, next-frame capture of the clicked viewport, multi-window target selection, busy/conflict, resize/minimize, device loss, finalization, shutdown, artifact-open/path-unavailable, and clean relaunch without the provider. Test supported and rejected provider combinations separately, including independent icon state and global request arbitration.
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
- One-click PIX, RenderDoc, and supported Nsight Graphics captures start on the intended viewport's next valid frame, contain the same stable Sparkle marker path, and report the requested/captured identity or an explicit provider limitation; no test accepts a capture from another Editor window.
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
- Candidate four-overlay, 16-compact-row, hitch-row, and detailed-scope limits plus fixed presets and hidden-row behavior are asserted/tested before the calibrated values are frozen.
- `LiveBasic` performs no post-initialization per-frame heap allocation.
- `Off`, `LiveBasic`, `LiveDetailed`, `GpuProfileCapture`, and external-capture overhead are measured, not assumed.
- Fps-only, Unit, four simultaneous basic groups, GpuPasses, ProfileGpu, and UI open/closed observer costs are measured independently.
- A representative ProfileGpu capture leaves parallel frame-graph recording enabled and preserves submission batches, queue dependencies, worker policy, command order, and output. CPU recording disturbance and GPU timing disturbance are reported.
- Scope planning and recording use fixed storage with no per-scope string allocation, shared-vector growth, or mutex contention between recording tasks; per-queue query capacity is checked before arming.
- Enabling groups with the same minimum mode does not duplicate collection, memory polling, GPU timestamp pairs, or history storage.
- Export failure preserves the previous accepted evidence and reports one actionable error.
- `git diff --check`, applicable builds/tests, architecture boundary check, and exact unavailable hardware/tool paths are reported.

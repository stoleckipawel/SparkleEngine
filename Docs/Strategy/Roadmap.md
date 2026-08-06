# F. August 2026-January 2027 Principal Graphics Roadmap

Status: execution plan; gates are targets, not completion claims

Planning window: 2026-08-06 through 2027-01-31

Planning baseline: repository reviewed at `9cf7b3bd` on 2026-08-06; revalidate code, builds, tests, captures, and external state before acting

Governing requirements: [A. Principal Graphics Engineering Requirements](Requirements.md)

Dated assessment: [C. Candidate and Repository Gap Assessment](GapAssessment.md)

Canonical workload: [I. Bistro and San Miguel Acceptance Workloads](../Engineering/BistroAndSanMiguelWorkloads.md)

Multithreading execution detail: [K. Multithreaded Engine Implementation Plan](../Architecture/Multithreading/ImplementationPlan.md)

## Six-Month Outcome

By the end of January 2027, SparkleEngine should present one compact, reproducible body of evidence:

> A reviewer can acquire the declared workloads, build and run the engine, reproduce a measured D3D12/Vulkan path-tracing result, inspect one trained neural denoising feature running through Sparkle's shader path, compare it with a classical fallback, and understand the quality, latency, memory, failure, and ownership tradeoffs without private guidance.

Bistro exterior and wine interior are the narrative spine. San Miguel is the supported cross-scene and held-out generalization workload. Sponza remains the short regression loop. The six-month result is not a broader engine, a general ML framework, or a collection of unrelated effects.

The public package should contain:

- a clean reviewer path and known-good Windows configuration;
- deterministic workload acquisition, cook, launch, capture, and benchmark records;
- paired D3D12/Vulkan classical rendering evidence;
- a fixed-topology neural diffuse-indirect denoiser with a versioned artifact and classical fallback;
- three concise specialist case studies plus one adoption/reproduction record;
- exact limitations, negative results, and deleted or rejected alternatives.

## Current Planning Baseline

This table routes the next work; it does not replace the dated assessment or prove a gate complete.

| Area | Source-inspected state on 2026-08-06 | Planning consequence |
| --- | --- | --- |
| Renderer and execution foundation | D3D12/Vulkan, frame graph, task runtime, render coordination, persistent render data, capture/timestamp primitives, and bounded content work exist. | Measure and harden the existing path. Do not restart the renderer, scheduler, or concurrency architecture. |
| Tests | CMake currently registers eight focused tests across Core and Launcher. | Run them from a clean configuration, preserve them in CI, and add only tests required by the flagship proof. |
| Workload acquisition | Bistro and Modern Sponza acquisition/cook/launch smoke evidence exists; San Miguel acquisition is verified. | Treat smoke evidence as a starting point, not quality or performance acceptance. |
| Tier 1 readiness | Bistro material/camera/reference/performance gates remain open. San Miguel is source-ready but not runtime-ready because deterministic OBJ conversion/import is missing. | Close `WL-01` through `WL-04` before neural runtime work. |
| Evidence harness | Fixed resolution, a published settled signal, named capture sidecars, timing export, and a unified run manifest remain incomplete. | `MAP-00` is the first implementation milestone. No benchmark claim precedes it. |
| Reviewer trust | No root README or CI workflow exists and the license identity is still a placeholder. | Close the small trust surface in August; do not build a documentation portal. |
| Python and neural graphics | Narrow asset-conversion scripts exist, but no benchmark-analysis/training package, trained model, model artifact, or shader inference path was found. | Build one analysis/training toolchain and one fixed neural feature. |
| Platform breadth | Windows is the current product/evidence platform; both graphics APIs are targets on Windows. | Native Linux and second-hardware work remain after this six-month gate unless required to resolve a result. |

Any baseline row may be superseded only by current code and recorded evidence. Update this roadmap's status, not the historical assessment, when a planned gate changes.

## Capacity And Allocation

Plan against the conservative end of 12-15 focused hours per week: about 300 hours across the window. Keep approximately 10% unallocated for build, driver, content, and experiment failures.

| Workstream | Planned hours | Share | Boundary |
| --- | ---: | ---: | --- |
| Evidence spine and Tier 1 correctness | 75 | 25% | Clean build/test path, `MAP-00`, deterministic workloads, references, material/failure records. |
| Classical path tracing and workload analysis | 70 | 23% | Paired APIs, benchmark analysis, captures, incident, and causal bottleneck studies. |
| Neural model and GPU inference | 95 | 32% | Data, training, artifact, conformance, shader inference, ablations, and fallback. |
| Reproduction, writing, release, and application material | 30 | 10% | Reviewer routing, case studies, external review, release, CV/profile update. |
| Contingency | 30 | 10% | Unplanned correctness or environment blockers only. |

Editor, launcher, import, and general engine work share one rule: they enter the roadmap only when they directly block the current acceptance gate. Do not spend more than 10% of the six-month budget on usability or content breadth.

## Dependency And Work-In-Progress Rule

```text
clean baseline
    -> MAP-00 evidence harness
    -> correct Bistro + San Miguel workloads
    -> paired-API measured classical result
    -> frozen neural data + trained model
    -> versioned artifact + shader inference
    -> held-out evaluation + external reproduction + publication
```

Only one box is the primary implementation objective at a time. Learning, writing, and test maintenance may accompany it, but a later implementation box does not start while an earlier exit gate is red. Completion order is governed by evidence, not by elapsed calendar time.

## Roadmap At A Glance

| Month | Primary outcome | Workload target | Principal evidence focus | Exit artifact |
| --- | --- | --- | --- | --- |
| August 2026 | Trustworthy clean baseline and accepted evidence harness | `WL-01`, `MAP-00`, `MAP-01` | `PGE-07`, `PGE-09`, `PGE-13` | Known-good build/test record, CI, reviewer README, Sponza calibration evidence package |
| September 2026 | Correct deterministic Tier 1 content | `WL-02`, `WL-03`; sequential map review | `PGE-02`, `PGE-07`, `PGE-08`, `PGE-09` | Bistro/San Miguel inventories, frozen routes, material matrix, reference baseline |
| October 2026 | Measured classical result across both APIs | `WL-04` | `PGE-02`, `PGE-05`, `PGE-06`, `PGE-09`, `PGE-10` | Benchmark CLI, paired captures, ranked bottlenecks, incident report, case-study drafts |
| November 2026 | Reproducible neural training baseline | `WL-05` | `PGE-03`, `PGE-08`, `PGE-11`, `PGE-12` | Dataset manifest, model card, trained baseline, ablations, immutable export candidate |
| December 2026 | Correct model-to-shader runtime path | `WL-06` | `PGE-03`, `PGE-04`, `PGE-09`, `PGE-10`, `PGE-12` | Versioned artifact, conformance tests, D3D12/Vulkan inference, classical fallback |
| January 2027 | Frozen result, reproduction, and publication | `WL-07`, `WL-08` | `PGE-01`, `PGE-05`, `PGE-13`, `PGE-15` | Evidence release, three case studies, final video/report, peer reproduction record |

The `PGE-*` columns identify intended coverage, not promised evidence levels. Grade changes require the evidence defined in [Requirements](Requirements.md).

## August - Baseline And Evidence Spine

### Outcome

One documented Windows configuration can build, test, launch, settle, measure, and capture Sponza from a fresh checkout. A reviewer can find the result and limitations immediately.

### Work

1. Record the exact repository revision, Windows build, compiler/CMake/Ninja or Visual Studio versions, SDKs, GPU, driver, and selected D3D12/Vulkan configuration.
2. Configure and build from a clean checkout or clean worktree; run all registered tests, formatting checks, and the architecture boundary check. Fix only failures on the reviewer path.
3. Add a small root reviewer README, correct the license identity, add required notices, and add Windows CI for configure, non-GPU build/tests, formatting, and boundary checks.
4. Implement `MAP-00` exactly through the workload owner: fixed launch resolution, authoritative active/settled identity, named capture plus sidecar, CPU/GPU sample export, and a unified manifest.
5. Prove the harness on Sponza, then complete `MAP-01` without upgrading Sponza into Tier 1 evidence.
6. Reconcile Bistro and San Miguel provenance, archive identity, transformation warnings, and deterministic inspection output to close `WL-01`.

### Exit Gate - 2026-08-31

- Clean configure/build/test commands and results are recorded for one supported configuration.
- CI runs the non-GPU credibility gates from a fresh checkout.
- `MAP-00` and `MAP-01` are accepted with raw artifacts and an honest limitation list.
- The README routes a reviewer to build/run, architecture, workload, evidence, and limitations in under two minutes.
- Bistro and San Miguel provenance and loss inventories satisfy `WL-01`.

If this gate is red, September continues August work. Do not compensate with more launcher polish, maps, or neural scaffolding.

## September - Tier 1 Correctness And Deterministic Content

### Outcome

Bistro exterior/wine interior and San Miguel high/low travel through one deterministic content path and have frozen, reviewable correctness baselines.

### Work

1. Follow the canonical one-map review order and finish each checkpoint before opening the next. Reuse the accepted harness; do not create per-map evidence code.
2. Complete the Bistro material/texture inventory and classify every material as exact, converted, approximated, or rejected.
3. Fix transparency, lighting, camera, importer, or renderer behavior only where a frozen Tier 1 view proves it blocks correctness. Preserve one backend-neutral production path.
4. Add a pinned deterministic San Miguel OBJ/MTL/PNG-to-glTF conversion and before/after semantic inventory. Add direct OBJ import only if evidence proves it is simpler or more faithful.
5. Freeze the required Bistro and San Miguel cameras, seeds, reference settings, exposure policy, material/debug views, and high/low matched route.
6. Generate the first high-sample references and record unsupported material/lighting behavior without hiding fallbacks.

### Exit Gate - 2026-09-30

- `WL-02` and `WL-03` pass for both Tier 1 families.
- Bistro and San Miguel acquire, convert/import, cook, launch, settle, and capture deterministically.
- Frozen routes produce correct baseline images and explicit material/fallback matrices.
- San Miguel high/low share matched cameras and settings.
- No scene-specific renderer or shader fork exists.

If the full compatibility-map sequence consumes the month, cut extra cameras and polish before cutting Tier 1 correctness or San Miguel support.

## October - Classical Rendering And Workload Analysis

### Outcome

The existing path-tracing and renderer work becomes one reproducible, measured D3D12/Vulkan result rather than an architecture claim.

### Work

1. Emit one versioned benchmark record and build one narrow Python CLI that validates comparability, applies warm-up/sample policy, calculates p50/p95/p99 and uncertainty, and produces a comparison table plus two useful plots.
2. Capture the same frozen Bistro and San Miguel routes on D3D12 and Vulkan with native validation enabled where supported.
3. Record CPU/GPU timelines, frame pacing, memory high-water, resource/barrier/descriptor/queue state, pipeline/cache state, and BLAS/TLAS behavior defined by the workload contract.
4. Rank measured bottlenecks. Start the three required causal studies and finish at least one difficult incident with competing hypotheses, reduced reproducer, scoped fix, and regression gate.
5. Run K's `23`/`23A` characterization and value audit against current owners. Pull `24`-`29` forward only when a measured correctness or evidence blocker requires them.
6. Draft `CASE-01`, `CASE-02`, and `CASE-03` from captured evidence. Do not wait until January to reconstruct the experiment history.

### Exit Gate - 2026-10-31

- `WL-04` passes with paired captures and comparable records.
- The Python tool runs from a checked-in small sample and has fewer than five user-facing commands.
- At least one incident and one causal optimization or negative result are complete.
- Backend differences above the declared threshold are explained or explicitly open; none are hidden.
- The classical case has a reproducible configuration, quality result, latency distribution, memory result, and limitations.

## November - Neural Data And Training Baseline

### Outcome

A small fixed neural diffuse-indirect denoiser is trained reproducibly from declared data and evaluated against noisy and classical baselines without contaminating held-out San Miguel routes.

### Work

1. Freeze input/output meaning, tensor layout, normalization, color/exposure/demodulation rules, target sample count, and the spatial MVP topology before broad data generation.
2. Generate immutable training/validation/test identities. Keep final San Miguel cameras out of training and model selection.
3. Implement a deterministic PyTorch training path with a small public sample or documented generator, an overfit-one-batch check, loss curves, and exact environment lock.
4. Establish noisy and classical baselines, parameter/FLOP estimates, held-out metrics, and failure cases.
5. Run only decision-making ablations: input guides, width, loss, and precision proxy. Do not search architecture space broadly.
6. Produce the model card, dataset manifest, operator/math note, and immutable export candidate aligned with K's `30` and `31` outcomes.

### Exit Gate - 2026-11-30

- `WL-05` passes.
- Training is reproducible and train/validation/test identities are disjoint.
- The selected model beats the noisy input and has an evidence-backed path toward the classical quality/performance target.
- The topology and artifact contract are frozen for runtime integration.
- Weak inputs, layers, or losses are removed and retained only as concise negative results.

If the model does not justify runtime work, simplify inputs or width once. Do not respond by creating a larger topology, general training framework, or hand-picked test set.

## December - Model-To-Shader Runtime

### Outcome

The frozen artifact executes through Sparkle's existing shader cook/runtime ABI on D3D12 and Vulkan with numerical conformance, bounded memory, and a classical fallback.

### Work

1. Export weights and graph metadata into one minimal versioned artifact; reject incompatible version, shape, layout, operator, and precision combinations.
2. Add reference tensors and conformance checks across PyTorch, the export/reference runner, and GPU output.
3. Implement the fixed operators in HLSL or Slang and integrate them through the existing frame graph and renderer ownership path. Do not embed a general ONNX or tensor runtime.
4. Establish the FP32 correctness baseline, then evaluate FP16, layout, dispatch size, tiling, fusion, weight packing, bandwidth, register pressure, and occupancy only from profiles.
5. Run the same artifact on both APIs and both Tier 1 scene families. Preserve explicit capability failure, history invalidation where applicable, and the classical fallback.
6. Complete K's `32` runtime-inference outcome and begin `33` only after correctness and whole-frame cost are known.

### Exit Gate - 2026-12-31

- `WL-06` passes.
- Numerical tolerances are defined and pass on both backends.
- Inference latency, memory, cold start, and whole-frame interference are measured.
- Bistro quality/performance results and the classical fallback are available from the product path.
- Any backend or provider limitation is explicit and reproducible.

If the neural path loses at an honest budget, keep the classical path as default and preserve the neural work as a measured negative result. Do not tune away a correctness or generalization failure.

## January - Evaluation, Transfer, And Publication

### Outcome

The result is frozen, independently exercised, and packaged for recruiter, hiring-manager, graphics-specialist, and adopter review paths.

### Work

1. Freeze code, model, manifests, cameras, settings, hardware/driver record, benchmark schema, and capture versions by 2027-01-15.
2. Run final Bistro and held-out San Miguel evaluation: objective and perceptual quality, temporal errors, latency distribution, memory, operator timings, failure gallery, and quality/performance/memory frontier.
3. Complete all three measured bottleneck studies, including at least one rejected optimization where the evidence supports rejection.
4. Ask one graphics engineer who did not implement the feature to clone/build/run, reproduce one result row, switch classical/neural paths, locate the artifact/fallback contract, and record one technical criticism.
5. Fix adoption blockers without opening new infrastructure. Complete K's `34` handoff outcome.
6. Publish three concise specialist cases: content-to-correct-pixel, paired-API/path-tracing workload analysis, and model-to-shader. Link the adoption record as supporting evidence.
7. Produce one evidence release, an eight-to-twelve-page technical report or equivalent article series, a two-page integration guide, a model card, a short comparison video, and an honest support/limitations matrix.
8. Update the CV, public profile, and website only with claims supported by the frozen evidence.

### Exit Gate - 2027-01-31

- `WL-07` and `WL-08` pass, or each remaining miss is named with evidence and a bounded follow-up.
- Another engineer reproduces at least one result without private implementation guidance.
- The repository builds from the documented clean path and the release contains or links every required small artifact.
- Classical and neural outcomes state whether they won, lost, or traded quality, latency, and memory.
- A reviewer reaches the headline result, code, captures, data, reproduction steps, and limitations within the time budgets in [Requirements](Requirements.md#portfolio-review-contract).

## Monthly Tracker

Update a row only when its exit evidence is linked. `Implemented` without the required gate evidence remains `In progress`.

| Milestone | Target | Status | Evidence |
| --- | --- | --- | --- |
| `M0` Clean baseline and reviewer trust | 2026-08-31 | Not started | Pending |
| `M1` Accepted evidence harness and `WL-01` | 2026-08-31 | Not started | Pending |
| `M2` Tier 1 deterministic correctness | 2026-09-30 | Not started | Pending |
| `M3` Paired-API classical evidence | 2026-10-31 | Not started | Pending |
| `M4` Neural training baseline | 2026-11-30 | Not started | Pending |
| `M5` Runtime shader inference | 2026-12-31 | Not started | Pending |
| `M6` Reproduction and evidence release | 2027-01-31 | Not started | Pending |

Allowed status values are `Not started`, `In progress`, `Passed`, `Blocked`, `Deferred`, and `Rejected`. A blocked row names the blocker, owner, next decision date, and evidence gathered so far.

## Weekly Operating Rhythm

For a 12-hour week:

| Work | Hours |
| --- | ---: |
| Current gated implementation or experiment | 6 |
| Correctness tests, benchmark, capture, and evidence review | 3 |
| Targeted math/ML/GPU study for the next decision | 1.5 |
| Case-study notes and reviewer-path maintenance | 1 |
| Backlog review and deletion/retrospective | 0.5 |

Every week ends with:

- one demonstrable result or falsified hypothesis;
- exact commands/configuration and raw evidence where applicable;
- a green or explicitly blocked default path;
- one short record of what changed, what was learned, what was deleted, and what gate is next;
- at most one primary implementation item carried into the next week.

At month end, review the gate before planning the next month. Never mark a calendar phase complete merely because its month ended.

## Backlog And Scope Control

Keep one backlog of at most 20 items in three lanes: `Now`, `Next`, and `After January`. Every `Now` or `Next` item names:

- the current roadmap gate and `PGE-*` requirement it advances;
- the expected artifact or falsifiable result;
- its owner and estimated focused hours;
- prerequisite evidence;
- what will be deleted, rejected, or left unchanged.

Prioritize work that closes an absent/weak requirement, strengthens the three public stories, produces causal evidence, enables reproduction, or removes code. Penalize new subsystems, broad UI/tooling, unowned hardware/SDK dependencies, screenshot-only output, and duplicate mechanisms. Reject any task with no evidence output.

## Scope Cuts And Decision Dates

Never cut correctness, deterministic identity, both Windows graphics APIs, the classical fallback, San Miguel held-out evaluation, or honest limitations. If capacity drops, cut in this order:

1. extra cameras, videos, and presentation polish;
2. Modern Sponza compatibility polish beyond required regression coverage;
3. temporal neural inputs beyond the correct spatial MVP;
4. second-hardware characterization;
5. native Linux/Vulkan and upstream contribution work;
6. generalization of any tool, model topology, importer, or runtime interface.

Decision triggers:

| Date | Trigger | Response |
| --- | --- | --- |
| 2026-08-31 | `MAP-00` or clean baseline is red | Hold Tier 1 expansion; fix the evidence spine. |
| 2026-09-30 | San Miguel is not runtime-ready | Do not claim cross-scene or held-out readiness; close conversion/import before neural dataset freeze. |
| 2026-10-31 | `WL-04` is red | Continue classical measurement; do not hide missing baselines behind ML work. |
| 2026-11-30 | Model does not beat noisy input credibly | Simplify once or publish a training negative result; do not widen the model search. |
| 2026-12-31 | GPU conformance is red | Keep classical default and fix correctness before performance tuning or publication. |
| 2027-01-15 | New feature request appears after freeze | Move it to `After January` unless it blocks reproduction or corrects a material claim. |

The minimum honest six-month package is a correct measured paired-API classical result, one real fixed neural shader path with an honest win/loss result, and one external reproduction. If that package cannot be completed, publish the strongest passed gates and name the remaining gap; do not substitute breadth.

## After January - Direction, Not Commitment

Choose later work from measured adoption gaps, not from the old calendar:

- native Linux/Vulkan build-run-capture;
- a materially different GPU architecture study;
- one upstream graphics/compiler/tool contribution;
- public talk or technical-series submission;
- a second adopter and longer-lived maintenance evidence.

Physics, networking, audio, scripting, marketplace work, general ML runtime work, USD/virtual geometry, and broad editor expansion remain outside the strategy until the six-month evidence package passes and a real user demonstrates the need.

## Final Acceptance

The roadmap passes only when current evidence supports the outcome at the top of this document. A visually attractive image, a merged feature, an architecture document, or an elapsed deadline is a milestone, not completion.

At handoff, report:

1. passed, rejected, deferred, and blocked milestones;
2. exact build/test/capture/benchmark commands and configurations;
3. linked raw and reviewed evidence;
4. `PGE-*` and `WL-*` transitions justified by that evidence;
5. deleted alternatives and remaining limitations;
6. the first recommended post-January decision.

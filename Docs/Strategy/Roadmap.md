# F. Release-First Principal Graphics Roadmap

Status: execution plan; gates are targets, not completion claims

Planning baseline: repository and release surfaces statically reconciled on 2026-09-06 at `master` revision `f61cabc4` with 331 changed worktree paths; no build, cook, package, launch, capture, clean-machine, or performance result was added by this reconciliation

First-release acceptance authority: [First Release Acceptance Contract](../Engineering/FirstReleaseAcceptance.md)

Governing graphics requirements: [A. Principal Graphics Engineering Requirements](Requirements.md)

Dated graphics assessment: [C. Candidate and Repository Gap Assessment](GapAssessment.md)

Canonical graphics workload: [I. Bistro and San Miguel Acceptance Workloads](../Engineering/BistroAndSanMiguelWorkloads.md)

Concurrency architecture and execution contract: [J. Multithreaded Engine Architecture](../Architecture/Multithreading/MultithreadedEngineArchitecture.md)

## Roadmap Decision

SparkleEngine is release-first until `REL-10` passes for `v0.1.0`.

No new engine feature, render effect, backend, content family, general framework, or platform enters implementation while an earlier first-release gate is red. Existing capability work is not discarded: it is classified, completed with evidence, exposed as experimental with an honest boundary, or removed/excluded from the public product. The previously planned Bistro/San Miguel, paired-API, path-tracing, neural, and publication work remains in this roadmap after the first release gate.

This is a sequencing change, not a change to the long-term graphics plan. It closes the current product before expanding it.

## First-Release Outcome

The first release is complete when a person who did not build the engine can:

1. download a versioned Windows x64 archive and verify its checksum;
2. extract it outside the repository on a clean standard-user machine;
3. launch the `ShippingGame` Showcase runtime without build tools or network access;
4. run every shipped example map and advertised graphics/backend option end to end;
5. see PBR-correct, temporally stable images with no unresolved release-blocking artifacts;
6. receive at least 30 FPS on the declared minimum machine under the frozen measurement contract;
7. switch maps, resize/minimize/restore, repeat runs, and exit without crash, hang, device removal, stale content, or unbounded memory growth;
8. locate controls, requirements, licenses/notices, known issues, support, and crash-report instructions;
9. reproduce the recorded result from the release instructions and exact artifact hashes.

Only linked evidence can move a row to `Verified`, `Release candidate`, `Shippable`, or `Published`. Source presence, a successful compile, historical logs, a responsive process, an attractive screenshot, or average FPS cannot.

## Current Planning Baseline

The repository has substantial engine breadth. The release problem is closure and delivery evidence, not lack of features.

| Area | Current source-inspected state | Release-first consequence |
| --- | --- | --- |
| Engine foundation | Core, platform, tasks, application, world, editor, renderer, RHI, import, cooking, shader compiler, launcher, and Showcase owners exist. | Inventory current public surfaces and close or exclude each one. Do not start replacement frameworks. |
| Graphics | D3D12/Vulkan; raster/deferred PBR; debug views; exposure/tone/output; upscaling; ray/path and ReSTIR routes; scene/view/frame and GPU-scene infrastructure are present. | Source-present is the initial state. Each selectable mode needs feature, map, backend, failure, quality, and performance evidence. |
| Build | Six Debug/Development/Shipping editor/game profiles exist; Showcase editor/runtime and launcher targets exist. | Freeze `ShippingGame` as the runtime release product and prove a clean reproducible build. |
| Package | Runtime source recognizes a package manifest, and development dependency/artifact staging exists. | Add one owned Build-Cook-Stage-Package route. No repository `install()`/CPack contract or release archive is currently proven. |
| Content | The Showcase catalog has 16 level records; the workload audit describes 13 as runtime-supported and three as source-readiness-only. | Curate a smaller legally redistributable `ReleaseMapSet`; do not ship the whole catalog by implication. |
| Visual acceptance | Detailed map/PBR/reference/performance gates exist in the Bistro/San Miguel workload. | Reuse `MAP-A` through `MAP-H` and add release-package acceptance; do not create a second map-quality vocabulary. |
| Evidence | Level-selected launch, manual capture, native validation switches, and timing primitives exist. | Finish `MAP-00`: fixed resolution, authoritative settled identity, named sidecars, timing export, and unified manifest. |
| Verification | No active CTest registration, root CI workflow, or complete release matrix was found. | Record the validation policy and obtain explicit authorization before adding permanent submitted tests; automate approved build/static/package gates first. |
| Identity/trust | No root README, no declared CMake project version, placeholder license identity, and no Windows version resource were found. | Freeze identity, license, notices, version metadata, system requirements, support, and known issues before a candidate. |
| Worktree | The audit observed 331 changed paths. | Reconcile intended changes into a clean candidate commit; dirty-tree evidence cannot become final release evidence. |

Any row may be superseded only by current code, executable configuration, and recorded evidence. Update this roadmap's tracker when a gate changes; do not rewrite historical assessments to imply completion.

## Dependency And Work-In-Progress Rule

```text
REL-00 scope and freeze
    -> REL-01 identity, rights, and release maps
    -> REL-02 clean reproducible baseline
    -> REL-03 Build-Cook-Stage-Package spine
    -> REL-04 close or exclude every current feature
    -> REL-05 artifact-free PBR map acceptance
    -> REL-06 30 FPS and stability acceptance
    -> REL-07 native backend diagnostics
    -> REL-08 clean-machine candidate
    -> REL-09 independent approval
    -> REL-10 publish and verify
    -> retained advanced graphics roadmap
```

Only one gate is the primary implementation objective. Documentation, defect triage, evidence review, and preparation for the next gate may run alongside it, but later capability implementation does not start while an earlier gate is red.

Scope may shrink when a feature or map cannot meet the bar in reasonable time. The evidence threshold does not shrink. A release date is forecast only after `REL-03` produces a repeatable package and `REL-04` exposes the actual closure queue.

## Release Plan At A Glance

| Stage | Primary outcome | Exit artifact | Stop rule |
| --- | --- | --- | --- |
| 0. Define | Exact product, platform, support matrix, feature disposition, and non-goals. | Approved scope and inventory. | No implementation before the surface is finite. |
| 1. Trust | Real identity/license/notices, clean baseline, reviewer route, and executable checks. | Reproducible clean build record. | Do not package ambiguous ownership or rights. |
| 2. Package | One repeatable Build-Cook-Stage-Package route. | Install/stage manifest and first archive. | Do not validate a development-tree substitute. |
| 3. Close features | Every current capability is included, experimental, excluded, or removed. | Feature-closure matrix. | No unclassified selectable feature. |
| 4. Prove maps | Cleared examples are complete, PBR-correct, and artifact-free. | Per-map evidence packages. | One release-blocking visual/content defect stops the next map. |
| 5. Prove delivery | 30 FPS floor, stability, memory, both advertised APIs, and actionable failures. | Performance/native-validation/stability records. | No averaging away stalls, warnings, or backend failures. |
| 6. Candidate | Frozen package works on a clean minimum machine and for a non-author. | Signed go/no-go record. | Any package fix creates new bytes and invalidates affected gates. |
| 7. Publish | Immutable tag/assets/checksums, fresh-download smoke, and support ownership. | `v0.1.0` release record. | Do not announce before uploaded bytes are retrieved and verified. |

## Stage 0 - Define The Release

### Outcome

The first release is a bounded product rather than “everything currently in the repository.”

### Work

1. Create the `REL-00` scope record from the [acceptance contract](../Engineering/FirstReleaseAcceptance.md#release-scope-freeze): version, Windows/GPU/API support, minimum/reference machines, executable, default configuration, features, maps, tools, support, and non-goals.
2. Build a repository-wide feature inventory from current public UI/options, runtime configuration, map catalog, modules, executables, and documentation. Assign one owner and one release classification to every row.
3. Select `ShippingGame` Showcase runtime as the required public binary. Decide separately whether a developer archive containing launcher/editor/tools is worth delaying for; default to source-build documentation until its own gates pass.
4. Freeze the initial renderer surface. Every selectable raster/ray/path, lighting, upscaling, exposure/output, debug, and backend option is included, experimental, or excluded.
5. Write explicit first-release non-goals. Candidate defaults are Linux/macOS, installer/updater, marketplace/plugin ecosystem, new scene families, new render effects, and neural inference unless already release-ready by evidence.

### Exit Gate - `REL-00`

- The release scope and feature inventory are approved and version controlled.
- Every user-reachable current capability has an owner and classification.
- Minimum/reference machines and the 30 FPS profile are named.
- No feature is accepted from source inspection alone.

## Stage 1 - Identity, Rights, And Trustworthy Baseline

### Outcome

A clean checkout has a real product identity, legally reviewable content scope, and a reproducible Shipping build route.

### Work

1. Replace placeholder publisher/license data; declare project version; add executable/file version metadata; create root quick start, system requirements, support, third-party notices, privacy/crash-data disposition, and known-issues entry points.
2. Audit every candidate release map and dependency for source, archive hash, license text, attribution, modification, and redistribution permission. Freeze only the cleared `ReleaseMapSet`; keep uncleared large scenes as download recipes.
3. Reconcile the current worktree. Produce the baseline from a clean worktree/checkout and record commit, compiler, CMake/generator, Windows/SDK, dependency revisions, GPU, driver, and commands.
4. Run the smallest current credibility gates applicable to the release baseline: configure, Shipping target build, code-style/static checks, documentation/link checks, and architecture boundary checks. Record exact outcomes and unavailable checks.
5. Record whether the user explicitly authorizes a minimal permanent automated regression suite. The binding [submitted-test policy](../Engineering/Standards/ValidationPerformanceAndEvidence.md#submitted-test-code) remains in force; absence of authorization is not permission to add test scaffolding.
6. Add fresh-checkout automation only for approved deterministic gates. GPU/map/package evidence remains hardware-labelled and must not be represented by a compile-only job.

### Exit Gates - `REL-01` And `REL-02`

- Product/version/publisher/license/notices and `ReleaseMapSet` are real and reviewed.
- A clean checkout reproduces the named Shipping build and approved credibility checks.
- Every command, environment identity, raw result, failure, and limitation is retained.
- No package or map depends on unreviewed redistribution rights.

## Stage 2 - Build, Cook, Stage, Package

### Outcome

One command transforms the clean tagged source inputs into a self-contained, versioned, checksummed runtime archive.

### Work

1. Define the install/stage tree as a build-owned contract: executable, DLLs/runtime redistributables, cooked assets, cooked shaders, configuration, package manifest, notices, README, and licenses.
2. Make Build, Cook, Stage, and Package explicit steps with raw logs and failure propagation. The package MUST be generated from the stage/install manifest, not from an ad-hoc copy of development outputs.
3. Prohibit source assets, repository-relative paths, build caches, temporary files, authoring/training dependencies, private diagnostics, credentials, or unrelated executables from the runtime archive.
4. Generate the authoritative `manifests/sparkle-package-manifest.json`, per-file SHA-256, archive checksum, build identity, content identity, dependency list, and package size.
5. Extract the first archive into a non-repository path and prove offline default launch/map/exit before feature acceptance begins.

### Exit Gate - `REL-03`

- A clean run produces identical logical package membership; byte nondeterminism is explained.
- The archive starts without the repository, source tree, compiler, CMake, or network.
- Every runtime dependency and notice is present; every unexpected file is absent.
- Package discovery uses the staged manifest and documented writable locations.

## Stage 3 - Close Every Current Feature

### Outcome

There is no ambiguous gap between source-present capability and the product that users can select.

### Work

1. Populate the [subsystem feature-closure matrix](../Engineering/FirstReleaseAcceptance.md#subsystem-feature-closure-matrix) for Core/Platform, Tasks, Application/World, source import/cooking, shader delivery, D3D12, Vulkan, Renderer, Editor/Launcher if distributed, packaging, and documentation/support.
2. Trace each feature's owner, producers, consumers, lifetime, build membership, configuration/UI reachability, compatible maps, and failure path.
3. For every included feature, capture the smallest evidence that proves the feature and then its interaction in the staged product. Include negative/capability rejection, not only a happy path.
4. Fix release-blocking behavior within existing ownership. If closure would require a new subsystem or open-ended redesign, exclude the feature cleanly and document the limitation.
5. Remove or disable stale UI/configuration/documentation routes for excluded features. Do not leave a selectable path that silently falls back or fails later.
6. Finish `MAP-00` through the existing evidence owner before claiming any broad map or performance result.

### Exit Gate - `REL-04`

- Every current surface has one classification and linked evidence/disposition.
- Every included or experimental feature builds, runs, fails safely, and has a named supported matrix.
- Excluded features are unreachable from the release package and are not advertised.
- `MAP-00` provides fixed resolution, settled identity, named capture sidecars, timing export, and unified manifests.

## Stage 4 - Ship PBR-Correct, Artifact-Free Example Maps

### Outcome

The shipped examples are curated demonstrations a user can trust, not a menu of partially working test assets.

### Work

1. Run every `ReleaseMapSet` member through `MAP-A` to `MAP-H` from the staged package, one map at a time. Stop and classify each defect before advancing.
2. Capture frozen lit views and applicable albedo, normal, roughness, metallic, depth, motion, direct, indirect, and output views on every advertised backend.
3. Verify source-to-cooked material/texture semantics, geometry/transforms, tangent/normal orientation, UVs, alpha/double-sided behavior, skinning/animation, lights/shadows, exposure/tone/output, and temporal stability.
4. Compare against declared high-sample or publisher/reference views with identical camera/exposure/encoding assumptions. Keep raw captures, thresholds, observation sheets, and failure gallery.
5. Resolve every `S0`/`S1` artifact. An `S2` requires an explicit owner/impact/workaround/target waiver; a pleasant final screenshot does not erase a failed debug view or temporal route.
6. Repeat acquire/cook/stage/offline launch/load/settle/map switch/exit to prove the result is part of the delivered product.

### Exit Gate - `REL-05`

- Every release map has complete provenance, cook, package, visual/PBR, reference, temporal, and backend evidence.
- Zero unresolved `S0`/`S1` content or visual defects remain.
- No missing asset/shader, uncategorized warning, silent fallback, NaN/Inf output, stale history, or backend-only corruption remains.
- Review decisions and known limitations are public and map-specific.

## Stage 5 - Performance, Stability, And Native Backend Proof

### Outcome

The candidate sustains the promised experience and produces actionable evidence when it fails.

### Work

1. Measure every release map on the minimum and reference machines using the [30 FPS contract](../Engineering/FirstReleaseAcceptance.md#thirty-fps-performance-floor): 1920x1080, `ShippingGame`, fixed quality, VSync off, fixed route, 300 warm-up plus 300 measured frames, three runs, per backend.
2. Require presented/application frame-time p95 at or below 33.33 ms, record CPU and GPU p95 separately, and require neither to exceed 33.33 ms. Report p50/p95/p99, worst frame, one-percent low, load/warm-up, memory high-water, excluded samples, variance, and limitations.
3. Profile failures before optimizing. Remove unnecessary work and ownership/lifetime defects before adding parallelism or backend-specific complexity.
4. Run the [candidate stability matrix](../Engineering/FirstReleaseAcceptance.md#candidate-stability-matrix): repeat launch/load/exit, map-switch loop, soak, resize/minimize/restore/alt-tab, cold/warm caches, standard user, unusual paths, offline operation, and controlled corrupt/missing-file failures.
5. Run focused D3D12 debug/GPU validation and Vulkan core/synchronization/GPU-assisted/best-practices validation where supported. Resolve or classify every message; keep validation out of performance numbers.
6. Verify crash/device-removal evidence includes build ID, backend, adapter/driver, active map/mode, logs, and DRED or Vulkan diagnostic context as applicable.

### Exit Gates - `REL-06` And `REL-07`

- Every release map/backend meets the presented/application, CPU, and GPU p95 33.33 ms floors on the named minimum machine.
- No recurring hitch cluster, unbounded memory growth, crash, hang, device removal, stale scene, or shutdown leak remains.
- Native validation has zero uncategorized findings and all waivers have owners and user-impact records.
- Performance claims bind exact candidate, content, hardware, driver, settings, and raw data.

## Stage 6 - Candidate And Independent Approval

### Outcome

The exact bytes intended for publication work for a standard user who has no private repository knowledge.

### Work

1. Freeze a clean commit, version, content, dependencies, package manifest, archive, checksums, documentation, known issues, and release notes. Assign a candidate ID.
2. On a clean minimum-spec machine, extract to spaced/Unicode and non-system paths, run offline as a standard user, exercise default launch and every release map/backend, take a capture, verify logs/writable locations, exit, and remove the product.
3. Ask a non-author to repeat the shortest acceptance path using only packaged/public instructions. Record confusion, failure, elapsed time, and one technical criticism.
4. Triage all open defects. Zero `S0`/`S1`; every accepted `S2` waiver is explicit and user-visible where relevant.
5. Rerun every gate invalidated by a fix. Never patch a staged archive manually or reuse evidence from different bytes.

### Exit Gates - `REL-08` And `REL-09`

- Clean-machine and independent reproduction records bind the candidate hashes.
- Package contents, prerequisites, controls, support, notices, known issues, and removal are understandable without private help.
- All required gates are green, all unavailable checks are explicit, and the release owner signs the go/no-go record.

## Stage 7 - Publish And Stabilize

### Outcome

The public release is immutable, retrievable, supportable, and verified after upload.

### Work

1. Create the final signed tag and draft release with source snapshot, runtime archive, optional symbols/developer archive, checksums, manifest, release notes, notices, system requirements, known issues, and support link.
2. Upload the exact approved candidate bytes. Compare remote downloads against local SHA-256 and tag identity.
3. From a fresh location, follow the public quick start and run the shortest launch/default-map/exit smoke against the downloaded archive.
4. Announce only supported, evidenced features. Distinguish experimental and source-only work.
5. Open a bounded stabilization window. Classify reports against `S0`-`S3`, preserve reproduction data, and decide patch release versus known issue without reopening feature work.
6. Record lessons, actual gate duration, escapes, support load, and evidence gaps before scheduling the retained advanced roadmap.

### Exit Gate - `REL-10`

- Tag, uploaded bytes, checksums, manifest, notes, notices, and support route agree.
- Fresh-download verification passes on the published artifact.
- The release page makes supported platform/features/maps and limitations unambiguous.
- A post-release owner and patch/rollback decision path are named.

## First-Release Tracker

Update a row only when the required evidence is linked. `Implemented` without acceptance evidence remains `In progress` or `Blocked`.

| Gate | Status on 2026-09-06 | Evidence/blocker |
| --- | --- | --- |
| `REL-00` Scope and freeze | In progress | This roadmap and acceptance contract define the process; approved scope/feature inventory is still pending. |
| `REL-01` Identity, rights, and map set | Blocked | Placeholder license identity, no frozen version/publisher, and no cleared redistributable map set. |
| `REL-02` Clean reproducible baseline | Blocked | Audit observed 331 changed paths; no current clean Shipping build/check record. |
| `REL-03` Package spine | Blocked | No owned install/CPack/stage contract or produced release archive found. |
| `REL-04` Current feature closure | Blocked | No complete feature classification/evidence matrix; `MAP-00` remains open. |
| `REL-05` Map correctness and PBR | Blocked | No staged-package per-map artifact/PBR acceptance evidence. |
| `REL-06` Performance and stability | Blocked | No current named-hardware three-run 30 FPS package evidence. |
| `REL-07` Native backend diagnostics | Blocked | No current candidate-bound D3D12/Vulkan validation record. |
| `REL-08` Clean-machine candidate | Blocked | No candidate archive. |
| `REL-09` Independent approval | Blocked | Requires an accepted clean-machine candidate. |
| `REL-10` Publish and verify | Blocked | Requires signed approval and immutable artifacts. |

## Immediate Execution Queue

Do these in order; do not begin with renderer polish:

1. approve `v0.1.0` product/platform/support/non-goals and create the complete feature inventory;
2. choose the exact `ReleaseMapSet` after redistribution review;
3. settle publisher/license/version/root quick start/notices/support identity;
4. reconcile the dirty worktree and reproduce a clean `ShippingGame` build;
5. establish the package stage/install manifest and generate the first archive;
6. finish `MAP-00` and populate the feature-closure matrix from the packaged product;
7. close maps, then performance/stability, then native validation;
8. freeze, clean-machine test, independently reproduce, publish, and retrieve the exact approved bytes.

## Release Operating Rhythm

Keep one backlog of at most 20 items in `Now`, `Next`, and `After Release`. Every `Now` or `Next` item names the current `REL-*` gate, the claim and evidence it will produce, owner, estimate, prerequisites, and what remains unchanged or is excluded. Reject work with no release-gate effect.

For a 12-hour focused week:

| Work | Hours |
| --- | ---: |
| Current release-gate implementation or defect closure | 6 |
| Correctness, package, map, performance, or native-validation evidence | 3 |
| Focused study for the next gate decision | 1.5 |
| User/reviewer documentation and evidence routing | 1 |
| Backlog review, deletion, and retrospective | 0.5 |

Every week ends with one demonstrable result or falsified assumption, exact evidence and limitations, a green or explicitly blocked release path, one short decision record, and at most one primary item carried forward. Review the gate before planning more work; elapsed time does not change status.

## Retained Advanced Graphics Roadmap

The following plan is unchanged in intent. It starts only after `REL-10`; calendar dates are rebaselined from actual capacity at that point rather than pretending the missed August 2026 gate passed.

### Retained Outcome

A reviewer can acquire the declared workloads, build and run the engine, reproduce a measured D3D12/Vulkan path-tracing result, inspect one trained neural denoising feature running through Sparkle's shader path, compare it with a classical fallback, and understand quality, latency, memory, failure, and ownership tradeoffs without private guidance.

Bistro exterior and wine interior remain the narrative spine. San Miguel remains the supported cross-scene and held-out generalization workload. Sponza remains the short regression loop. The result is not a general ML framework or a collection of unrelated effects.

### Retained Capacity Envelope

After release, plan against about 300 focused hours and keep 10% unallocated:

| Workstream | Planned hours | Share | Boundary |
| --- | ---: | ---: | --- |
| Evidence spine and Tier 1 correctness | 75 | 25% | `MAP-00`, deterministic workloads, references, material/failure records. Reuse release evidence where still valid. |
| Classical path tracing and workload analysis | 70 | 23% | Paired APIs, analysis, captures, incident, and causal bottleneck studies. |
| Neural model and GPU inference | 95 | 32% | Data, training, artifact, conformance, shader inference, ablations, and fallback. |
| Reproduction, writing, and publication | 30 | 10% | Reviewer routing, case studies, external review, evidence release, profile updates. |
| Contingency | 30 | 10% | Unplanned correctness or environment blockers only. |

### Retained Dependency Sequence

```text
published v0.1.0 baseline
    -> MAP-00 evidence harness retained/extended
    -> correct Bistro + San Miguel workloads
    -> paired-API measured classical result
    -> frozen neural data + trained model
    -> versioned artifact + shader inference
    -> held-out evaluation + external reproduction + publication
```

### Phase A - Evidence And Tier 1 Content

- Revalidate `MAP-00` against the published product path; extend it only for requirements not already proven by release evidence.
- Close `WL-01`: Bistro/San Miguel provenance, archive identity, deterministic inspection, and complete loss/warning inventory.
- Close `WL-02` and `WL-03`: deterministic conversion/import/cook/load, frozen cameras/settings, material matrices, high-sample references, and explicit fallback lists.
- Keep Bistro flagship and San Miguel held-out/high-low roles. Do not create scene-specific renderer or shader forks.

Exit: correct deterministic Tier 1 content with complete reference and material evidence.

Primary requirements: `PGE-02`, `PGE-07`, `PGE-08`, `PGE-09`, and `PGE-13`.

### Phase B - Measured Classical Result

- Close `WL-04` with versioned benchmark records, warm-up/sample policy, p50/p95/p99 and uncertainty, paired D3D12/Vulkan captures, memory/high-water, frame/queue/barrier/descriptor/BLAS/TLAS records, and ranked bottlenecks.
- Build only the narrow analysis CLI required for comparability and useful plots.
- Complete at least one difficult incident and one causal optimization or measured negative result.
- Draft the three specialist cases from captured evidence rather than reconstructing them later.

Exit: reproducible paired-API classical configuration, quality result, latency distribution, memory result, bottleneck record, and limitations.

Primary requirements: `PGE-02`, `PGE-05`, `PGE-06`, `PGE-09`, and `PGE-10`.

### Phase C - Neural Data And Training

- Freeze one diffuse-indirect denoising input/output contract, tensor layout, normalization, exposure/demodulation rules, sample target, and small fixed topology.
- Generate immutable train/validation/test identities; keep final San Miguel views held out.
- Close `WL-05` with deterministic training, environment lock, overfit-one-batch check, noisy/classical baselines, model card, dataset manifest, operator note, ablations, metrics, and failure cases.
- Remove weak inputs/layers/losses. Do not grow a general training framework or broad architecture search.

Exit: reproducible selected model and immutable export candidate with an evidence-backed path to the quality/performance target.

Primary requirements: `PGE-03`, `PGE-08`, `PGE-11`, and `PGE-12`.

### Phase D - Model-To-Shader Runtime

- Export one minimal versioned artifact and reject incompatible version, shape, layout, operator, and precision combinations.
- Establish PyTorch/export/GPU reference-tensor conformance.
- Implement fixed operators through existing shader cook, frame graph, renderer, and backend ownership. Do not embed a general tensor runtime.
- Establish FP32 correctness, then profile FP16/layout/dispatch/tiling/fusion/packing/bandwidth/register/occupancy decisions.
- Close `WL-06` on D3D12/Vulkan and both Tier 1 families with explicit capability failure, history policy, latency, memory, cold start, whole-frame interference, and classical fallback.

Exit: correct bounded shader inference on both APIs, or a documented negative result with the classical path preserved as default.

Primary requirements: `PGE-03`, `PGE-04`, `PGE-09`, `PGE-10`, and `PGE-12`.

### Phase E - Evaluation, Reproduction, And Publication

- Freeze code/model/manifests/cameras/settings/hardware/driver/benchmark/capture identities.
- Close `WL-07` and `WL-08`: held-out San Miguel evaluation, quality/performance/memory frontier, temporal failure gallery, three bottleneck studies including a rejected optimization, and independent reproduction.
- Publish content-to-correct-pixel, paired-API/path-tracing analysis, and model-to-shader cases plus an evidence release, technical report, integration guide, model card, comparison video, and support/limitations matrix.
- Update public profiles only with frozen supported claims.

Exit: another engineer reproduces at least one result without private guidance and a reviewer can reach the headline code, evidence, artifacts, limitations, and adoption record within the time budgets in [Requirements](Requirements.md#portfolio-review-contract).

Primary requirements: `PGE-01`, `PGE-05`, `PGE-13`, and `PGE-15`.

### Retained Milestone Identities

The earlier milestone IDs remain valid so existing evidence and references do not lose meaning. Their old calendar targets were missed and are not silently rewritten; targets are re-estimated after `REL-10`.

| Milestone | Retained outcome | Current status | Prerequisite |
| --- | --- | --- | --- |
| `M0` | Clean baseline and reviewer trust | Not started | Reuse valid `REL-01`/`REL-02` evidence, then close graphics-reviewer-specific gaps. |
| `M1` | Accepted evidence harness and `WL-01` | Not started | `REL-10`; revalidate `MAP-00`. |
| `M2` | Tier 1 deterministic correctness | Not started | `M1`. |
| `M3` | Paired-API classical evidence | Not started | `M2`. |
| `M4` | Neural training baseline | Not started | `M3`. |
| `M5` | Runtime shader inference | Not started | `M4`. |
| `M6` | Reproduction and evidence release | Not started | `M5`. |

Allowed status values remain `Not started`, `In progress`, `Passed`, `Blocked`, `Deferred`, and `Rejected`. Any blocked row names blocker, owner, next decision, and evidence gathered so far.

### Retained Scope Cuts

Do not cut correctness, deterministic identity, both Windows graphics APIs, the classical fallback, San Miguel held-out evaluation, or honest limitations from the retained advanced program. If its capacity drops, cut in this order:

1. extra cameras, videos, and presentation polish;
2. Modern Sponza compatibility polish beyond required regression coverage;
3. temporal neural inputs beyond the correct spatial MVP;
4. second-hardware characterization;
5. native Linux/Vulkan and upstream contribution work;
6. generalization of any tool, model topology, importer, or runtime interface.

The minimum honest advanced package remains a correct measured paired-API classical result, one real fixed neural shader path with an honest win/loss result, and one external reproduction. If it cannot be completed, publish the strongest passed gates and name the remaining gap; do not substitute breadth.

### After The Advanced Program

Choose later work from measured adoption gaps: native Linux/Vulkan build-run-capture, a materially different GPU architecture study, one upstream graphics/compiler/tool contribution, a public technical submission, or a second adopter with longer-lived maintenance evidence. Physics, networking, audio, scripting, marketplace work, general ML runtime work, USD/virtual geometry, and broad editor expansion remain outside this strategy until the retained evidence package passes and a real user demonstrates the need.

## Scope Protection

Before `REL-10`, defer unless a release gate proves the work necessary:

- new render effects, algorithms, backends, platforms, scene families, editor products, or generalized tooling;
- Linux/macOS/native mobile support;
- installer, auto-updater, account, telemetry, marketplace, plugin ecosystem, or cloud service;
- new neural topology or runtime;
- broad UI polish, dashboard, benchmark suite, or content breadth;
- architecture refactors that do not remove a release blocker or measurable risk.

Do not defer release correctness, legal review, package ownership, safe failures, native validation, PBR/map quality, frame pacing, memory stability, clean-machine proof, or honest documentation as “polish.” Those are delivery work.

## Final Decision Rule

There are only three valid outcomes for a release candidate:

- **Go** — `REL-00` through `REL-09` pass for the exact candidate bytes; publish and complete `REL-10`.
- **No-go and fix** — a frozen-scope requirement failed; repair it and rerun every affected gate.
- **No-go and reduce scope** — exclude the feature/map/backend cleanly, update the approved scope and public claims, then rerun every gate affected by the change.

Time spent, code volume, a planned date, or enthusiasm cannot override failed evidence. The first trustworthy small release is preferable to a larger package containing unproven choices.

# First Release Feature Completion Reports

Status: acceptance contract; includes a source-audited initial registry whose entries are not completion claims

Responsibility: define the per-feature completion report, evidence, approval, and initial registry used to close first-release capability claims

Release target: SparkleEngine `v0.1.0`, Windows x64

Source snapshot: committed `master` revision `8414b5dc`, inspected 2026-09-06 while a separate documentation relocation was present in the worktree; no build, cook, runtime, capture, benchmark, or independent-adoption result was added by this report

Release authority: [First Release Acceptance Contract](FirstRelease.md)

Capability authority: [Principal Graphics Engineering Requirements](../Strategy/Requirements.md)

Operating model: [Advanced Graphics Engineer Persona](../Strategy/EngineerPersona.md)

Current owner map: [Whole Repository Architecture Map](../Architecture/WholeRepositoryMap.md)

Current feature-level source inventory: [Current Capability Inventory](../Architecture/Modules/README.md)

## Purpose And Boundary

This document defines the completion report required for every included or experimental first-release feature and seeds the report registry from current source. It answers two separate questions:

1. Is the feature polished and shippable for its declared consumer, configuration, and fallback?
2. What engineering/persona target does its evidence actually advance?

The first question is mandatory for release. The second prevents substantial engine work from becoming an unexplained toggle, but it does not force unrelated portfolio targets into `v0.1.0`. Code and executable build configuration prove implementation; retained runtime evidence proves behavior; [Requirements](../Strategy/Requirements.md) alone controls `PGE-*` evidence level.

A capability-inventory row says what the dated source scan found; an Architecture feature dossier defines what that feature must prove; a completion report says what the exact release candidate actually proved. They are not competing ledgers. Every included or experimental capability row and open capability-evidence plan item MUST map to exactly one primary `FCR-*` report, each report MUST list those IDs, and `REL-04` fails if either side has an orphan. One report may own several inseparable rows, but independently selectable modes, backends, providers, or materially different failure/performance routes require separate result subsections or separate reports.

Stable `AC-*`, `FM-*`, and `CHK-*` definitions belong with the feature under [Architecture](../Architecture/README.md). The candidate report references those definitions, records exact outcomes and artifacts, and may add candidate-specific challenges without becoming a duplicate feature contract.

A report is not accepted because it is long. Its job is to make the feature's outcome, complete execution path, quality, costs, failures, limitations, and adoption route reviewable without repository archaeology. When clearer code, one deletion, or an executable check communicates the truth better, do that work and keep the report short.

## Polish Is A Conjunction, Not A Score

A feature is polished only when every applicable dimension passes. An excellent image cannot average away broken shutdown; high FPS cannot average away wrong PBR; clean code cannot average away an undiscoverable consumer workflow. `N/A` requires a reason tied to frozen scope and reviewer approval.

| Dimension | Required completion question | Minimum evidence |
| --- | --- | --- |
| Product intent | What user problem does this solve, who can select it, and what is deliberately not promised? | One-sentence outcome, audience, classification, supported matrix, and non-goals. |
| Discoverability and control | Can the intended user find, understand, configure, reset, compare, and exit the feature without private knowledge? | Public workflow recording, controls/help, defaults, requested-versus-active state, and settings/reset result. |
| Functional correctness | Does the feature produce the specified state/output deterministically and reject invalid inputs? | Reference or invariant checks, repeated route, boundary cases, and negative checks. |
| Visual and PBR quality | Is the output physically/semantically correct and free of release-blocking artifacts? | Frozen cameras, debug views, reference comparison, image metrics, and expert observation record. |
| Temporal quality | Does motion, animation, history, disocclusion, reset, resize, and scene change remain stable? | Sequence/video, history/reset identity, temporal metrics where applicable, and failure gallery. |
| Performance and pacing | Does the whole delivered route meet startup, load, CPU, GPU, presented-frame, hitch, and latency budgets? | Frozen protocol, raw samples, p50/p95/p99/worst, timelines/captures, and causal analysis. |
| Memory and residency | Are CPU/GPU high-water, transient/history/AS/provider allocations, lifetime, and pressure behavior bounded? | Budget comparison, time series, allocation ownership, repeat/soak result, and pressure failure. |
| Concurrency and lifetime | Are publication, cancellation, synchronization, GPU completion, retirement, reload, and shutdown safe? | Owner/lifetime trace, 1/2/N or serial control where relevant, stress/fault result, and leak/late-work disposition. |
| API, shader, and backend | Are resource states, bindings/ABI, queues, capabilities, compiler output, D3D12, and Vulkan behavior explicit? | Shader-to-command trace, reflection/layout check, native validation, paired result or approved backend exclusion. |
| Fallback and failure | Is each unavailable, corrupt, unsupported, or failed state visible, bounded, honest, and recoverable? | Capability/failure matrix, injected fault, exact message/exit/recovery, and no silent false success. |
| Content and delivery | Does source/import/cook/stage/package preserve semantics and ship every required byte without private paths? | Provenance, deterministic inventory, cook/package manifest, clean-package run, and missing-content rejection. |
| Security, privacy, and compatibility | Are trust boundaries, mutable data, binary loading, signing, network/crash data, and version behavior controlled? | Threat/control record, signatures/SBOM, filesystem/network inventory, compatibility decision, and controlled abuse case. |
| Diagnostics and support | Can another engineer identify the active feature, capture the right evidence, reproduce a failure, and seek help? | Stable identity, markers/logs/capture route, support bundle, troubleshooting, and issue/reproducer route. |
| Code and ownership quality | Is there one owner and production path with narrow contracts, bounded copies/work, and no replaced residue? | Owner/producer/consumer map, public-surface review, cost model, deletion/simplification ledger, and applicable code/architecture checks. |
| Adoption and explanation | Can a non-author understand the math/data/API tradeoffs and reproduce or integrate the supported result? | Public how-it-works route, integration/tuning guide, expected output, limitations, and independent criticism/reproduction. |

## Iteration Traceability And Coverage

Every candidate report is also the control record for the iterations that close that feature. It applies the [Change Lifecycle identifier contract](../Engineering/Workflow/ChangeLifecycle.md#create-the-iteration-control-record) and begins with this linked header:

| Mark | Required feature-report value |
| --- | --- |
| North Star | Each applicable persona `NS-*` facet and `advance`, `preserve`, `not applicable`, or `blocked`. |
| Persona/capability | Each applicable `PGE-*`, evidence level before/after, and proof or reason no level moved. |
| Roadmap/acceptance | Current `ITER-*`, `REL-*`, `MAP-*`/`CASE-*`, `FCR-*`, capability row, and capability-evidence plan item. |
| Risk | Applicable roadmap `RISK-REL-*` plus feature `RISK-<feature>-NN` entries and current exposure. |
| Acceptance | Links to the owning Architecture feature dossier's binary `AC-<feature>-NN` criteria covering promised behavior and every applicable polish dimension, plus candidate results. |
| Failure | Links to its `FM-<feature>-NN` feature faults plus applicable common [`FM-REL-*`](FirstRelease.md#failure-mode-acceptance-matrix) rows, with exercised outcomes. |
| Checks | Links to `CHK-<feature>-NN` mappings that cover every criterion and failure through the [check/test design contract](../Engineering/Verification/ValidationAndEvidence.md#check-and-test-design-contract), with exact candidate evidence. |
| Decision | Current state, exact blocker/pass reason, evidence links, invalidation triggers, and next permitted roadmap step. |

Coverage is closed only when the report has no orphan target, risk, criterion, failure, check, artifact, defect, or waiver. A risk register without an observable trigger/retirement condition, acceptance prose without a binary oracle, or a test list without claim mappings is `Blocked`.

## Required Completion Report

Store the candidate-bound report at:

```text
artifacts/validation/releases/<version>/<candidate-id>/features/<feature-id>/completion.md
```

The feature registry and intentional summaries may be version controlled; raw captures, traces, dumps, and large images follow the release evidence retention policy. Every report uses this order:

1. **Result and traceability** — feature ID/name, iteration, `NS-*`, `PGE-*`, roadmap/gate/workload, mapped module capability, `WF-*`, and capability-evidence plan IDs, owner, audience, candidate/hash, state, and exact pass/block reason.
2. **Promise and boundaries** — supported platforms/backends/hardware/maps/modes, default and opt-in behavior, non-goals, dependencies, and compatibility promise.
3. **How it works end to end** — the complete path defined below, with links to current owners rather than a directory dump.
4. **Contracts and invariants** — semantic inputs/outputs, coordinate/color/units, mutability, identity, capacity, ordering, lifetime, and thread/queue ownership.
5. **Acceptance and check ledger** — every `AC-*` and its mapped `CHK-*`, predeclared oracle/threshold, matrix, result, evidence, and invalidation trigger.
6. **Algorithm, math, and reference** — equations/assumptions, known-value or CPU/reference behavior, stability/error bounds, and why the selected approach fits the product.
7. **User experience and polish** — selection, active-state feedback, controls, defaults, error/recovery, settings/reset, accessibility/display scope, and comparison path.
8. **Quality evidence** — correctness, PBR/debug-view/reference, temporal behavior, backend comparison, thresholds chosen before measurement, and artifact/failure gallery.
9. **Cost evidence** — CPU/GPU/presented latency, pacing, startup/load, memory/residency, compilation/cook/package cost, observer effect, bottleneck, and quality-performance-memory frontier.
10. **Risk assessment** — each `RISK-*` cause/event/consequence, likelihood rationale, impact, prevention, trigger, contingency, owner, state, and retirement evidence.
11. **Failure, fallback, and support** — every `FM-*`, capability rejection, injected fault, detection boundary, safe state, requested-versus-active result, recovery, severity, and support route.
12. **Validation record** — each `CHK-*` claim mapping, exact setup/action/oracle/command, environment, raw artifacts, defect-detection control, cleanup, unavailable checks, escalation, open defects, and waivers.
13. **Adoption and operations** — clean consumer/source-adopter route, integration and tuning instructions, diagnostics/capture workflow, expected result, removal/retirement, and non-author feedback.
14. **Simplification and decision ledger** — alternatives considered, what was rejected, what old code/config/UI/docs were removed, remaining maintenance cost, and the next delete/revisit trigger.
15. **Target crosswalk** — applicable `NS-*` and `PGE-*` requirements, persona pillars/behaviors exercised, evidence level before/after, and why the linked proof meets that requirement. No evidence means no level movement.
16. **Approval history** — feature owner, evidence reviewer, independent acceptor where required, decision/date, invalidations, accepted risks/waivers, and release-owner disposition.

### Required Coverage Ledgers

Use compact tables in the report; do not replace them with a checklist whose rows cannot be traced:

| Ledger | Minimum columns |
| --- | --- |
| Target coverage | `NS-*`/`PGE-*`/`REL-*`/`MAP-*`/`FCR-*`/`WF-*`/capability-plan ID; applicability; advance/preserve/block; evidence; reviewer. |
| Risk | `RISK-*`; cause/event/consequence; likelihood rationale; impact/severity; prevention; detection trigger; contingency; owner; state; retirement evidence. |
| Acceptance/check | `AC-*`; observable contract and matrix; threshold/oracle; mapped `CHK-*`; result; artifact/hash; invalidation trigger. |
| Failure/check | `FM-*`; controlled injection; detection boundary; safe state/recovery; severity; affected `RISK-*`/`AC-*`; mapped `CHK-*`; result/evidence. |

The highest-impact open risk, the criterion most likely to falsify the feature, and its cheapest discriminating `CHK-*` are marked as the next iteration's **key check**. A broader suite may follow only when that result passes or exposes a boundary that requires escalation.

### Required How-It-Works Trace

The explanation MUST let a reviewer follow these boundaries for the exact feature. Combine steps only when the same owner genuinely controls them:

```text
consumer/editor/config request
    -> validation and requested-versus-active selection
    -> world/content producer and semantic data
    -> immutable frame extraction/publication
    -> renderer scene/view/frame preparation
    -> frame-graph resource and pass topology
    -> shader parameters, math, and compiled program identity
    -> RHI resource/state/queue/native command lowering
    -> GPU execution, completion, history, and retirement
    -> composition/presentation/capture-visible output
    -> failure/fallback/recovery/support path
```

For tools and offline features, replace the frame path with source input -> validation -> import/plan -> bounded execution -> transactional publication -> runtime consumer, while keeping ownership, cancellation, failure, provenance, and adoption explicit.

The trace answers who owns mutable state, who produces and consumes each representation, when it becomes immutable, which copies/transfers occur, what invalidates cached/history state, how both backends differ, and what happens when any boundary fails.

## Engineering And Persona Crosswalk

| Target | What a completion report must expose | What does not count |
| --- | --- | --- |
| `PGE-01`, persona adopter-first behavior | Constraints, integration surface, fallback, handoff, review history, and non-author result. | Author-only screenshots or instructions repaired privately during acceptance. |
| `PGE-02`, renderer feature depth | Raster/ray/GI/path algorithm, realistic map/reference, temporal behavior, both supported APIs, latency/memory, and honest limitations. | A selectable mode, Sponza-only beauty image, or source reachability. |
| `PGE-05`, CPU/GPU architecture thinking | Whole-route percentiles, frame pacing, CPU/GPU timelines, memory high-water, concurrent work, causal experiment, and regression band. | Average FPS, one capture, or shifting cost outside the measured interval. |
| `PGE-06`, debugging/developer technology | Competing hypotheses, reduced reproducer, exact environment, native capture/validation, root cause, fix/control, and scoped conclusion. | Enabling a debug layer or listing profiler names. |
| `PGE-07`, build/review/communication | Cohesive ownership, narrow API, clean source route, applicable validation, standards result, and behavior-matching documentation. | File count, generated prose, or an unbuilt public header. |
| `PGE-08`, applied mathematics | Definitions, assumptions, derivation/reference values, numerical stability, predicted cost/quality, and falsifying measurement. | Equations disconnected from the shipped path or unexplained constants. |
| `PGE-09`, explicit API and shader/kernel craft | Source-to-bytecode-to-binding-to-command trace, reflection/ABI, state/synchronization, compiler diagnostics, capability/fallback, and paired backend result. | Backend-neutral vocabulary without native evidence. |
| `PGE-10`, low-level architecture/concurrency | Cache/bandwidth/register/occupancy or task/queue/lifetime model, serial/control case, counters/disassembly where useful, stress result, and rejected alternatives. | Parallelism or low-level terminology without measured causality. |
| `PGE-13`, research productization | Hypothesis, precedent, prototype-to-product/deletion decision, polished demo, tool/integration surface, report, limitations, and priority ledger. | Keeping scaffolding because it might become useful. |
| `PGE-14`, platform/ecosystem | Exact Windows/D3D12/Vulkan toolchain and workflows; only platforms actually built/run/captured are claimed. | Treating Vulkan-on-Windows as native Linux evidence. |
| `PGE-15`, principal judgment | End-to-end decision under constraints, incident/quality ownership, review/teaching value, reduced maintenance/public surface, and transferability. | Repository size, policy volume, or feature count. |
| `PGE-03`, `PGE-04`, `PGE-11`, `PGE-12` | Required only for a real included neural feature: data/model provenance, training, operator math, export, fixed GPU implementation, classical baseline/fallback, quality/latency/memory, and generalization. | Provider enums, DLSS integration, mock tensors, or documentation do not advance neural evidence. If neural work is excluded from `v0.1.0`, these targets remain open without blocking unrelated release scope. |

Every report also applies the persona's seven starting questions: owner, frame/pass, data, shader, API, adopter, and deletion. The report reviewer rejects a feature that cannot answer one of these at an applicable boundary.

## Initial Completion-Report Registry

All entries below are `Source present / Blocked` at this snapshot unless a candidate-bound report later links sufficient evidence. The “how it works” column is a source-navigation summary, not proof that the path builds or behaves correctly.

Current completion snapshot: 34 family rows identified; 34 blocked; zero candidate-bound feature reports found under the required release-artifact path; zero release-map reports because the `ReleaseMapSet` is not frozen. The family count is a navigation measure, not a feature-completeness score, and will increase when the mandatory result axes below are assigned independent verdicts.

### Product, Build, And Delivery

| ID | Surface and current how-it-works route | Required polish focus | Primary targets |
| --- | --- | --- | --- |
| `FCR-PROD-01` | Showcase runtime -> [`RunRuntimeApplication`](../../Projects/Showcase/Src/RuntimeMain.cpp) -> [`RuntimeApplication`](../../Engine/Application/Private/RuntimeApplication.cpp) -> level/world/renderer loop. | Intentional first run, example/mode selection, controls/help/settings/reset/quit, no development console leakage, startup/load/exit budgets. | `PGE-01`, `PGE-07`, `PGE-13`, `PGE-15` |
| `FCR-PROD-02` | Tagged source -> CMake options/profiles -> dependency fetch -> engine/tools/project targets -> cook -> Showcase runtime. | Root quick start, immutable prerequisites/dependencies, cold/warm build/cook, actionable failures, supported source API/ABI boundary, non-author adoption. | `PGE-01`, `PGE-07`, `PGE-13`, `PGE-14`, `PGE-15` |
| `FCR-PROD-03` | [`SparkleLauncher`](../../Tools/Launcher/SparkleLauncher) discovers repository/toolchains/content and composes typed sync/build/cook/run/clean operations into child processes. | Developer-only versus shipped classification, concise task UX, readiness truth, progress/cancel/failure/restart, no duplicate execution paths, clean-machine package if distributed. | `PGE-01`, `PGE-07`, `PGE-13`, `PGE-15` |
| `FCR-PROD-04` | [`EditorApplication`](../../Engine/Application/Private/EditorApplication.cpp) layers Editor UI, viewport, settings, capture, scene inspection, and transactions over the runtime host. | Exclude from runtime archive by default; if distributed, prove discoverability, save/settings ownership, viewport correctness, undo/transactions, failures, performance, and independent use. | `PGE-07`, `PGE-13`, `PGE-15` |
| `FCR-PROD-05` | CMake profiles and module/product targets build artifacts; no current install/CPack owner produces the public archive. | One Build-Cook-Stage-Sign-Verify-Package path, immutable stage, per-user state, allowlist/SBOM/signatures, logical reproducibility, archive size and clean-machine result. | `PGE-07`, `PGE-13`, `PGE-14`, `PGE-15` |
| `FCR-PROD-06` | Core/RHI logs and DRED/Vulkan diagnostics expose failures; public support/security/crash/patch operation remains a delivery contract. | Process crash/hang capture, consent/privacy, symbols, issue/security intake, severity clock, patch/withdraw/advisory, stabilization evidence. | `PGE-01`, `PGE-06`, `PGE-13`, `PGE-15` |

### Foundation, World, And Content

| ID | Surface and current how-it-works route | Required polish focus | Primary targets |
| --- | --- | --- | --- |
| `FCR-CORE-01` | [`SparkleCore`](../../Engine/Core) owns paths, filesystem helpers, logging, diagnostics, console, events, memory, strings, and math used by higher modules. | Standard-user/per-user paths, Unicode/length/root safety, bounded diagnostics, config corruption, no package mutation, clean error identity, narrow public contracts. | `PGE-07`, `PGE-15` |
| `FCR-PLAT-01` | [`SparklePlatform`](../../Engine/Platform) owns Win32 window, input, focus/routing, timing/platform integration consumed by Application/RHI. | DPI/monitor/resize/minimize/restore/alt-tab, focus/capture recovery, keyboard/mouse scope, standard user, unsupported environment, shutdown and handle hygiene. | `PGE-07`, `PGE-14`, `PGE-15` |
| `FCR-TASK-01` | [`SparkleTasks`](../../Engine/Tasks) provides the shared executor, scopes/events/parallel work used by runtime, renderer, world loading, and cook paths. | Serial and 1/2/N behavior, bounded queues, cancellation/destruction, exception/failure propagation, oversubscription, shutdown with work in flight, causal cost. | `PGE-05`, `PGE-07`, `PGE-10`, `PGE-15` |
| `FCR-WORLD-01` | [`LevelSession`](../../Engine/GameFramework/Private/Level/LevelSession.cpp) resolves a catalog level, load graph assembles cooked assets, and [`GameWorld`](../../Engine/GameFramework/Public/World/GameWorld.h) owns entities/components/systems. | No silent `Empty` success, deterministic activation/switch/reload, malformed/missing asset failure, cancellation, bounded memory, stable map identity and consumer selection. | `PGE-01`, `PGE-07`, `PGE-13`, `PGE-15` |
| `FCR-WORLD-02` | World systems and [`RenderFrameSubmissionExtractor`](../../Engine/GameFramework/Private/World/Extraction/RenderFrameSubmissionExtractor.cpp) publish structural deltas, dynamic data, resource tables, and view input without Renderer querying ECS storage. | Producer/consumer/lifetime proof, transform/light/material semantics, identity reuse, copy budget, deletion/reload, in-flight safety, extraction cost and determinism. | `PGE-05`, `PGE-07`, `PGE-10`, `PGE-15` |
| `FCR-WORLD-03` | Scene camera navigation/input, static and skeletal meshes, skinning/morph/animation, material variants, four light kinds, and sky data flow into world publication. | Authored-to-runtime semantic fidelity, coordinate/units, animation bounds/history, camera controls, temporal stability, invalid data, per-map coverage and PBR observation. | `PGE-02`, `PGE-07`, `PGE-08`, `PGE-13` |
| `FCR-CONT-01` | [`SourceImporters`](../../Tools/Import/SourceImporters) normalize glTF/GLB/FBX scenes into imported geometry/material/texture/light/camera/animation representations. | Provenance, deterministic inventory, coordinate/tangent/material/alpha/animation fidelity, missing/external/embedded texture policy, malformed/oversized/path-escape rejection. | `PGE-07`, `PGE-08`, `PGE-13`, `PGE-15` |
| `FCR-CONT-02` | [`AssetCooker`](../../Tools/Cooking/AssetCooker) orchestrates mesh/material/scene/texture products consumed by cooked-only GameFramework loaders. | Transactional publication, stale/incompatible product rejection, bounded parallel work/memory, complete dependency manifest, deterministic semantics, clean rebuild and package-relative load. | `PGE-05`, `PGE-07`, `PGE-10`, `PGE-13` |
| `FCR-CONT-03` | Engine-authored [`Assets`](../../Engine/Assets) provide shader sources/includes, semantic default textures, sky textures, and small mesh fixtures; shader and asset cook paths transform the required subset into package products consumed by Renderer and Showcase. | Per-file license/provenance, explicit release allowlist, semantic default correctness, missing/corrupt-default failure, source/debug-file erasure from runtime delivery, deterministic cooked identity, and no undeclared fixture dependency. | `PGE-07`, `PGE-09`, `PGE-13`, `PGE-15` |
| `FCR-SHDR-01` | Shader registration -> [`ShaderCompiler`](../../Tools/Shaders/ShaderCompiler) plan/backends/reflection -> global shader map/code library -> Renderer runtime materialization. | DXIL/SPIR-V parity, include/dependency identity, every selected job, ABI/reflection checks, transactional replacement/retirement, diagnostics, missing/corrupt shader rejection, cook/startup cost. | `PGE-06`, `PGE-07`, `PGE-09`, `PGE-13`, `PGE-15` |
| `FCR-TOOL-01` | [`ToolConsoleSupport`](../../Tools/Support/ToolConsoleSupport) supplies shared message, field, progress, list, summary, and path presentation used by the asset/texture/shader command-line tools. | Stable script-readable output, correct stdout/stderr and process result at each consumer, Unicode/path quoting, bounded progress, no false-success summary, and actionable diagnostics without leaking private paths. | `PGE-01`, `PGE-07`, `PGE-13`, `PGE-15` |

### RHI And GPU Execution

| ID | Surface and current how-it-works route | Required polish focus | Primary targets |
| --- | --- | --- | --- |
| `FCR-RHI-01` | Renderer requests [`RenderHardwareInterface`](../../Engine/RHI/Public/Device/RenderHardwareInterface.h) services for resources, descriptors, uploads, pipelines, interop, capture, diagnostics, presentation, and ray tracing. | Narrow neutral contracts, capability truth, invalid-handle/bounds behavior, ownership/release, allocation failure, native object names, and no renderer policy leakage. | `PGE-07`, `PGE-09`, `PGE-15` |
| `FCR-RHI-02` | [`RenderDeviceServices`](../../Engine/RHI/Public/Device/RenderDeviceServices.h) owns frame begin, recording leases, queue waits/submission tokens, present, completion, resize, and frames in flight. | Queue/state/synchronization correctness, batch ordering, serial control, cancellation/resize/shutdown, completion-driven retirement, frame latency/pacing, native validation. | `PGE-05`, `PGE-06`, `PGE-09`, `PGE-10` |
| `FCR-RHI-03` | Backend implementations lower common resource/binding/pipeline/command contracts into [`D3D12`](../../Engine/RHI/Private/D3D12) or [`Vulkan`](../../Engine/RHI/Private/Vulkan). | Paired feature/capability matrix, resource states/barriers, descriptor lifetime, swapchain differences, compiler/driver failures, debug-layer/validation cleanliness, scoped backend divergence. | `PGE-06`, `PGE-09`, `PGE-14`, `PGE-15` |
| `FCR-RHI-04` | Ray-tracing services create BLAS/TLAS resources, classic or partitioned TLAS operations, native pipelines/shader tables, inline queries, and `TraceRays` commands. | Geometry/instance/SBT identity, build/update/refit/PTLAS policy, scratch/result memory, synchronization/retirement, inline-versus-pipeline parity, unsupported-capability failure and traversal cost. | `PGE-02`, `PGE-05`, `PGE-06`, `PGE-09`, `PGE-10` |
| `FCR-RHI-05` | RHI timing, messages, memory, object naming/live-object reports, capture readback, and DRED/Vulkan failure data feed bounded Renderer/editor diagnostics. | Observer cost, capacity/drop state, timestamp validity, capture format/encoding, privacy, actionable device-loss context, external PIX/RenderDoc/Nsight/RGP handoff. | `PGE-05`, `PGE-06`, `PGE-13` |
| `FCR-RHI-06` | [Device Lifecycle and Failure Recovery](../Architecture/Modules/Engine/RHI/Features/DeviceAndResources/DeviceLifecycleAndFailureRecovery.md) composes and publishes one owner-thread backend aggregate, distinguishes swapchain recovery from device loss, settles queues, and destroys dependents before the device. | Partial-create cleanup, owner-thread/state legality, bounded settlement, resize/out-of-date generations, queue-wait failure, D3D12 DRED/Vulkan device-loss diagnostics, terminal non-recovery truth, leaks and post-loss use. | `PGE-05`, `PGE-06`, `PGE-09`, `PGE-10`, `PGE-13`, `PGE-15` |

### Renderer Features

| ID | Surface and current how-it-works route | Required polish focus | Primary targets |
| --- | --- | --- | --- |
| `FCR-REN-01` | Immutable world submission -> [`RenderCoordinator`](../../Engine/Renderer/Private/Concurrency/Coordinator/RenderCoordinator.cpp) serial/threaded control -> RendererHost -> [`FramePipeline`](../../Engine/Renderer/Private/Frame/FramePipeline.cpp). | Monotonic publication, bounded frame/control queues, requested/active settings, serial parity, in-flight invalidation, shutdown settlement, CPU cost and no stale frame. | `PGE-05`, `PGE-07`, `PGE-10`, `PGE-15` |
| `FCR-REN-02` | RenderScene/View preparation applies deltas and dynamic data, updates persistent GPU scene/mesh/material/light/ray bindings, and builds per-view camera/display/temporal state. | Scene-owned versus view-owned truth, handle/generation lifetime, copy/upload budget, dynamic deformation, reload/retirement, memory high-water, deterministic identity. | `PGE-05`, `PGE-07`, `PGE-09`, `PGE-10`, `PGE-15` |
| `FCR-REN-03` | [`BuildRenderFrameGraph`](../../Engine/Renderer/Private/Frame/Graph/BuildRenderFrameGraph.cpp) declares RT scene, GBuffer, lighting, reconstruction, post, debug, and presentation; compile/execution lowers dependencies and barriers to RHI commands. | Pass/resource ownership, culling/aliasing/transients/history, queue plan, invalidation/rebuild, failure propagation, CPU construction cost, markers and backend-equivalent output. | `PGE-05`, `PGE-06`, `PGE-07`, `PGE-09`, `PGE-10` |
| `FCR-REN-04` | Raster GBuffer mesh passes consume prepared scene geometry/materials and publish depth plus material attributes for shared lighting. | PBR channel/space/precision contract, culling/batching/skinning/morph/alpha/two-sided behavior, depth/normal/tangent correctness, motion/temporal data, draw cost and map coverage. | `PGE-02`, `PGE-05`, `PGE-08`, `PGE-09` |
| `FCR-REN-05` | Ray-traced GBuffer selects automatic/inline/native-pipeline traversal while sharing one semantic scene/material result consumed by lighting. | Requested/active route, inline/RGS equivalence, miss/hit/material/SBT mapping, alpha/two-sided/skinned geometry, capability failure, native validation, quality/cost crossover. | `PGE-02`, `PGE-05`, `PGE-06`, `PGE-09`, `PGE-10` |
| `FCR-REN-06` | [Direct Lighting](../Architecture/Modules/Engine/Renderer/Features/Lighting/DirectLighting.md) samples four analytic light kinds, reuses ReSTIR direct reservoirs, resolves inline/native visibility, and writes direct diffuse/specular/subsurface lobes. | BRDF/light units, all light kinds and limits, reservoir validity, shadow bias/leaks, inline/native parity, alpha/subsurface scope, debug separability, failure, and cost. | `PGE-02`, `PGE-08`, `PGE-09`, `PGE-13` |
| `FCR-REN-07` | [Indirect Lighting](../Architecture/Modules/Engine/Renderer/Features/Lighting/IndirectLighting.md) reprojects/reuses ReSTIR indirect reservoirs, traces secondary paths, writes indirect diffuse/specular lobes, and joins the sky/environment boundary through the shared composite. | Estimator/reservoir math, sample/PDF/weight validity, bounce behavior, history identity/reset, motion/disocclusion, fireflies/bias/noise, sky semantics, temporal failure gallery, and quality-time-memory frontier. | `PGE-02`, `PGE-05`, `PGE-08`, `PGE-09`, `PGE-10`, `PGE-13` |
| `FCR-REN-08` | Reference path-traced lighting accumulates a convergent comparison route using shared scene/material/ray data and a reference-specific history lifecycle. It is the first technical closure case, but its [research study](../Research/GraphicsArchitecture/OfflinePathTracerCompletion.md), [`PTD-00` discovery gate](../Architecture/Modules/Engine/Renderer/Features/Lighting/OfflinePathTracer/Discovery.md), and [feature acceptance contract](../Architecture/Modules/Engine/Renderer/Features/Lighting/OfflinePathTracer/README.md) currently prohibit calling it an unbiased oracle. | Pass every included `OPT-FS-*` row and `AC-OPT-*` criterion; exercise every applicable `FM-OPT-*`; then prove deterministic sample identity, raw accumulation/export, convergence/unbiasedness limits, reset, NaN/Inf/firefly behavior, robust ray spawn, material/light scope, analytic and independent references, backend parity, controlled failure, packaged operation, and release-map adoption. | `PGE-02`, `PGE-05`, `PGE-06`, `PGE-07`, `PGE-08`, `PGE-09`, `PGE-10`, `PGE-13`, `PGE-15` |
| `FCR-REN-09` | [Exposure](../Architecture/Modules/Engine/Renderer/Features/PostProcessing/DisplayPipeline/Exposure.md) meters scene luminance or applies manual intent, maintains bounded per-view adaptation history, and supplies one multiplier to reconstruction/tone mapping. | Metering equivalence, finite bounds, step response, adaptation direction/speed, cut/resize/mode/viewport reset, async scheduling correctness/overlap, and color-domain semantics. | `PGE-02`, `PGE-05`, `PGE-08`, `PGE-09`, `PGE-13` |
| `FCR-REN-10` | [Image Reconstruction and Upscaling](../Architecture/Modules/Engine/Renderer/Features/PostProcessing/ReconstructionAndGeneration/ImageReconstructionAndUpscaling.md) maps render extent to one output-extent result through Linear, NVIDIA DLSS Super Resolution, or ReSTIR-specific NVIDIA DLSS Ray Reconstruction. | Provider requested/active/fallback truth, version/signature/redistribution, input semantics/jitter/exposure/motion/guides, unsupported hardware/backend, missing DLL, reset, temporal quality, latency/memory and linear baseline. | `PGE-01`, `PGE-05`, `PGE-06`, `PGE-09`, `PGE-13` |
| `FCR-REN-11` | Lit/wireframe/GBuffer/lighting/GPU-scene view modes and viewport capture expose intermediate outputs through Renderer/RHI capture paths. | Exact view semantics, correct exposure/tone/encoding policy, unavailable-resource state, stable labels, capture sidecars/provenance, no private diagnostic leakage, observer cost and user interpretation. | `PGE-01`, `PGE-06`, `PGE-07`, `PGE-13` |
| `FCR-REN-12` | Classic/refit or partitioned TLAS policy converts persistent scene geometry/instances into the acceleration structure consumed by inline and pipeline effects. | Selection/capability truth, transform/instance contribution mapping, static/dynamic update policy, move/delete/reload, operation-buffer bounds, scratch/result memory, build/update/traversal evidence. | `PGE-02`, `PGE-05`, `PGE-06`, `PGE-09`, `PGE-10` |
| `FCR-REN-13` | [UI and Viewport Composition](../Architecture/Modules/Engine/Renderer/Features/ViewportAndDiagnostics/UiAndViewportComposition.md) replays immutable host/editor packets, resolves viewport textures, validates generations, and composes after graph execution before submission. | packet ownership, blend/color/DPI behavior, missing/stale/generation-mismatched products, texture-handle lifetime and bounds, resize/level-switch stability, and Shipping product behavior. | `PGE-01`, `PGE-05`, `PGE-07`, `PGE-13` |
| `FCR-REN-14` | [Tone Mapping](../Architecture/Modules/Engine/Renderer/Features/PostProcessing/DisplayPipeline/ToneMapping.md) applies exposure and exactly one of Reinhard, ACES approximation, or ACES fitted filmic to resolved HDR scene color. | Known ramps/extremes, finite output, alpha behavior, all operators, exposure interaction, scene/display-linear semantics, lack of bypass, backend equivalence, and numerical/colorimetric disposition. | `PGE-02`, `PGE-05`, `PGE-08`, `PGE-09`, `PGE-13` |
| `FCR-REN-15` | [Presentation and Output](../Architecture/Modules/Engine/Renderer/Features/PostProcessing/DisplayPipeline/PresentationAndOutput.md) encodes display-linear color and publishes it to the back buffer or a generation-qualified viewport product after optional debug replacement and tone mapping. | Every encoding/format pair, SDR/HDR scope, no double encoding/clipping/banding, exact-debug limitation, output/product identity, resize/DPI, capture interpretation, backend present, and UI boundary. | `PGE-01`, `PGE-02`, `PGE-05`, `PGE-06`, `PGE-08`, `PGE-09`, `PGE-13` |
| `FCR-REN-16` | [Pipeline Materialization and Typed Binding](../Architecture/Modules/Engine/Renderer/Features/ShaderRuntime/PipelineMaterializationAndTypedBinding.md) validates registered/cooked shader ABI, creates graphics/compute/ray layouts and pipelines, binds current resources, and replaces whole runtime generations. | Registration/build membership, every parameter/domain, complete graphics key, capability rejection, native backend validation, reload failure, in-flight retirement, and cache/retained high-water. | `PGE-05`, `PGE-06`, `PGE-07`, `PGE-09`, `PGE-10`, `PGE-15` |
| `FCR-REN-17` | [Mesh and Texture Residency](../Architecture/Modules/Engine/Renderer/Features/GeometryAndResources/MeshAndTextureResidency.md) moves immutable asset generations through bounded CPU work, GPU upload, activation, replacement, cancellation, and completion-safe eviction. | Exact/over budgets, 16-job concurrency, 256 backlog, task/decode/upload/token failures, placeholder/refusal policy, stale generation, all-queue lifetime, diagnostics, memory and streaming stability. | `PGE-05`, `PGE-07`, `PGE-09`, `PGE-10`, `PGE-13` |
| `FCR-REN-18` | [Temporal Sampling and History](../Architecture/Modules/Engine/Renderer/Features/FrameExecution/TemporalSamplingAndHistory.md) supplies one view-owned Halton jitter, previous-camera, common-validity, motion/reprojection, and reset contract to native and provider consumers. | Exact sequence/sign/extent, every invalidation reason, two-view isolation, static/camera/rigid/skin/morph/sky motion, ReSTIR/reference/provider agreement, backend parity, ghosting and stability. | `PGE-02`, `PGE-05`, `PGE-08`, `PGE-09`, `PGE-10`, `PGE-13` |
| `FCR-REN-19` | [Settings State and Persistence](../Architecture/Modules/Engine/Renderer/Features/RuntimeConfiguration/SettingsStateAndPersistence.md) restores, edits, saves, and transfers aggregate requested rendering state to serial/render-thread CVar and feature owners. | Field/name/UI/consumer closure, valid/malformed round trip, atomic durable error-reporting save, concurrent edit, package location, queue pressure/shutdown, command-line ordering, and requested/CVar/resolved/restart-active truth. | `PGE-01`, `PGE-05`, `PGE-07`, `PGE-13`, `PGE-15` |
| `FCR-REN-20` | [Latency Coordination](../Architecture/Modules/Engine/Renderer/Features/FrameExecution/LatencyCoordination.md) optionally joins host simulation and D3D12 render-submit/present identity through Streamline PCL and supported Reflex sleep. | Six-marker order/identity, host misuse, Streamline on/off and readiness, PCL/Reflex support, D3D12/Vulkan boundary, provider fault/shutdown, 64-to-32-bit token limit, packaging, and separately measured latency benefit. | `PGE-01`, `PGE-05`, `PGE-06`, `PGE-09`, `PGE-10`, `PGE-13` |
| `FCR-REN-21` | [Visibility and Draw Preparation](../Architecture/Modules/Engine/Renderer/Features/GeometryAndResources/VisibilityAndDrawPreparation.md) classifies per-view bounds/materials, validates candidates, preserves eligible authored groups, and deterministically sorts/batches raster work. | Analytic frustum cases, invalid identity rejection, authored/fallback grouping, opaque batching equivalence and benefit, transparent single-item order, task cancellation/failure, diagnostic reconciliation, and explicit absent occlusion/LOD/GPU-driven/stereo paths. | `PGE-05`, `PGE-07`, `PGE-09`, `PGE-10`, `PGE-13` |
| `FCR-REN-22` | [Resolution, Sampling, and Anti-Aliasing](../Architecture/Modules/Engine/Renderer/Features/PostProcessing/ReconstructionAndGeneration/ResolutionSamplingAndAntiAliasing.md) resolves output/render extents, shared jitter, provider route, active attachment sample count, resize invalidation, and negative AA/dynamic-resolution boundaries. | Exact extent/viewport/scissor/dispatch/product agreement, requested/active provider quality and actual ratio, history reset, active single-sample truth, no implicit resolve, and no false MSAA/TAA/FXAA/SMAA/dynamic-resolution claim. | `PGE-02`, `PGE-05`, `PGE-06`, `PGE-08`, `PGE-09`, `PGE-10`, `PGE-13` |

### Known Mandatory Decomposition

The family rows above keep the source map readable; they are not permission to hide unlike results in one verdict. The first `REL-04` reconciliation MUST expose at least these result axes:

| Family | Results that remain independently visible |
| --- | --- |
| Product and tools | Runtime consumer, source adopter, editor, launcher, each cooker/compiler command, package/sign/verify, support/crash/security operation. Excluded tools need erasure evidence rather than a runtime pass. |
| Content and world | glTF, GLB, and FBX import; each cooked product family; default assets; static, skeletal, morph, animation, camera, sky, and every light/material class; activation, reload, switch, and malformed/missing input. |
| RHI | D3D12 and Vulkan; graphics/compute/copy queues; persistent/transient resources; bindful/fixed-array material binding; raster/compute/ray pipelines; classic/PTLAS; present/resize; capture/diagnostics. A backend pass cannot mask the other's failure. |
| Pipeline/binding | Runtime versus contract-only registration membership; graphics/compute/ray materialization; every typed field and binding domain; every complete PSO-key input; cache hit/miss, reload success/failure, and all-queue retirement independently. |
| Residency | Mesh versus texture; queued/decoding/ready/uploading/resident/evicting/retired; exact and over every byte/backlog/concurrency bound; success/failure/cancel/replacement/reset/shutdown and both backends. |
| Temporal history | Per view and projection; every jitter sample/invalid extent; every invalidation reason; rigid/skin/morph/sky motion; ReSTIR/reference/provider consumer; D3D12/Vulkan and supported/unsupported provider cells. |
| Settings | Public state/editor/persisted/console-only/RHI-only/absent surfaces; startup/command-line/live/topology/restart; valid/malformed/unwritable/concurrent/package storage; serial/threaded and requested/resolved state. |
| Latency | Six marker kinds and host order; PCL/Reflex support/readiness; Streamline on/off; D3D12/Vulkan; token-wrap and provider-failure/shutdown; marker correctness and measured benefit receive separate verdicts. |
| GBuffer and ray execution | Raster, strict inline, strict native pipeline, and Automatic selection; opaque, alpha-tested, double-sided, static, skinned, and morphed geometry; requested-versus-active and unsupported-capability outcomes. |
| Direct lighting | Each supported light kind and hard limit, ReSTIR direct reservoir stages, direct visibility Inline/Pipeline/Automatic, direct diffuse/specular/subsurface lobes, BRDF scope, and absence of a non-ray shadow/lighting fallback. |
| Indirect lighting | ReSTIR indirect temporal/spatial/resolve, reference indirect contribution, bounce settings, sky/environment boundary, history reset, bias/convergence behavior, and absence of probe/lightmap fallback. |
| Volumetric lighting | Explicit Not Implemented/Excluded state for media, fog, transmittance/scattering, atmosphere, aerial perspective, products, selectors, and debug views; no candidate report exists until roadmap admission. |
| Exposure | Manual exposure, each automatic-metering method, async scheduling, bounds, adaptation, per-view overrides, and every reset/discontinuity. |
| Image reconstruction and upscaling | Linear upscale, each supported DLSS SR mode, DLSS ray reconstruction, provider/backend/package readiness, every input guide, requested/active/fallback, resize/reset, quality, cost, and latency. |
| Tone mapping | Reinhard, ACES approximation, and ACES fitted filmic independently; exposure interaction, numerical ramps/extremes, alpha, scene/display-linear semantics, and no-bypass limitation. |
| Color grading and chromatic aberration | Explicit Not Implemented/Excluded state for grading controls/LUTs/transforms and lens/channel distortion; no candidate report exists until roadmap admission. |
| Frame generation | Explicit Not Implemented/Excluded state for frame synthesis/provider, optical flow, generated-frame identity, UI, pacing, and present integration; Reflex/PCL and temporal reconstruction are independently identified as non-substitutes. |
| Presentation and output | Every output encoding/format, back-buffer and viewport-product route, resize/DPI, capture interpretation, exact-debug limitation, and supported SDR/HDR claim. |
| Diagnostics and views | Every reachable view mode, final/intermediate capture, frame/pass timing, mesh/texture/memory diagnostics, mesh preview, shader recook/reload, unavailable-resource display, and Shipping erasure of developer-only surfaces. |
| Maps | One report per accepted `ReleaseMapSet` member, then rows per advertised backend/mode/quality preset on that map. |

Use the detailed [RHI](../Architecture/Modules/Engine/RHI/CapabilityInventory.md), [Renderer](../Architecture/Modules/Engine/Renderer/CapabilityInventory.md), and [shader-compilation](../Architecture/Modules/Tools/ShaderCompiler/README.md) inventories for the source-level rows behind these axes, and the [capability evidence plan](../Plans/CapabilityEvidence.md) for the smallest next checks. A matrix subsection is sufficient when the owner/path is shared and each cell retains an independent verdict, artifact link, failure, and waiver; otherwise create separate reports.

### Release Maps

Create one additional feature report for every accepted `ReleaseMapSet` member. Its how-it-works trace starts at source provenance and catalog selection, crosses import/cook/load/world/extraction/render/presentation, and links the per-map `MAP-A` through `MAP-H` evidence. A map is not polished because it loads or because one final screenshot looks plausible: every applicable material, geometry, animation, lighting, debug-view, reference, temporal, performance, memory, failure, redistribution, and consumer-navigation row must pass.

## Report Closure And Review

At `REL-04`, every registry row is split further if it contains independently selectable modes with different capability, quality, performance, or failure behavior. Every resulting row has a report owner and complete source trace. Later gates fill the candidate evidence:

| Gate | Report closure added |
| --- | --- |
| `REL-04` | Finite scope, owner, implementation trace, contracts, requested/active/fallback, positive and negative focused integration. |
| `REL-05` | Per-map correctness, PBR, debug-view, reference, temporal, and artifact disposition. |
| `REL-06` | Startup/load/frame pacing, CPU/GPU, memory/residency, soak/repeat, and causal cost evidence. |
| `REL-07` | Native D3D12/Vulkan diagnostics, capture/debug route, device-loss and backend-difference disposition. |
| `REL-08` | Exact signed package, standard-user/read-only-install journey, integrity/security/filesystem/network and recovery proof. |
| `REL-09` | Independent consumer/source-adopter use, explanation quality, criticism, and final defect/waiver disposition. |
| `REL-10` | Published identity, immutable artifact links, public support/limitations, and fresh-download result. |
| `REL-11` | Escaped-defect/support history, patch/withdraw/advisory result, retrospective, and next simplification/revalidation action. |

No report may say `Verified`, `Shippable`, or `Published` until the corresponding evidence state in the [release acceptance contract](FirstRelease.md#completion-vocabulary) is satisfied. If the report cannot explain the complete feature in the graphics-specialist or adopter review budget, narrow the public feature, improve the navigation/code, or exclude it; do not compensate with more prose.

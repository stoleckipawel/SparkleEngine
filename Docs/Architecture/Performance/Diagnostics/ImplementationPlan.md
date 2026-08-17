# Performance Diagnostics Delivery Plan

Status: implementation plan; not proof of implementation or shipment

Last code and document reconciliation: 2026-08-17

Scope: staged, feature-selectable delivery of the performance diagnostics product defined by [Performance Diagnostics Architecture](PerformanceDiagnosticsArchitecture.md)

## Purpose And Authority Boundary

This plan turns the target architecture into independently reviewable vertical deliveries. It owns order, package dependencies, selection states, implementation prompts, and completion gates. It is deliberately the only performance-diagnostics implementation-plan document: phase notes, speculative class diagrams, and separate checklists should not be added unless they acquire a genuinely different owner and audience.

The following documents remain authoritative for their subjects:

- [Performance Diagnostics Architecture](PerformanceDiagnosticsArchitecture.md) owns metric meaning, identity, validity, data flow, product behavior, collection modes, bounded records, and rejected designs.
- [Performance Diagnostics Visual Design And Tool Wireframes](PerformanceDiagnosticsAsciiWireframes.md) owns layouts, interaction details, responsive states, and the multi-provider viewport icon group.
- [Diagnostics Product And UX Research](DiagnosticsUxResearch.md) owns external precedent, option analysis, and the reasons for the selected product depth.
- [External Performance Profiler Runbook](DiagnosticsProfilerRunbook.md) owns version-sensitive provider support, profiler operations, marker compatibility, and installed-tool revalidation.
- [Bistro And San Miguel Acceptance Workloads](../../../Engineering/BistroAndSanMiguelWorkloads.md) owns `MAP-00`, benchmark routes, evidence layout, sample policy, and acceptance claims.
- [Engineering Standards](../../../Engineering/Standards/README.md) are binding. In particular, apply the [Integration Style Guide](../../../Engineering/Standards/IntegrationStyleGuide.md), [Change Process](../../../Engineering/Standards/ChangeProcess.md), [Concurrency](../../../Engineering/Standards/Concurrency.md), [Data-Oriented Design](../../../Engineering/Standards/DataOrientedDesign.md), [Graphics Engineering](../../../Engineering/Standards/GraphicsEngineering.md), [Editor And Tools](../../../Engineering/Standards/EditorAndTools.md), and [Validation, Performance, And Evidence](../../../Engineering/Standards/ValidationPerformanceAndEvidence.md).

Code, CMake membership, tests, and observed evidence remain the authority for implemented behavior. Updating a package to `Accepted` in this plan is a useful ledger entry, not a substitute for those proofs.

## Zero-Duplicate-Authority Contract

Zero duplicate architecture is a mandatory package and release gate. In this plan, “zero duplication” means zero unjustified competing semantic authorities, mutable owners, production routes, histories, command meanings, export schemas, capture arbiters, or public facades for the same question. It does not mean forcing unrelated operations into one class or removing small local presentation differences. Such forced reuse can violate separation of concerns as severely as a parallel subsystem.

The operating rule is:

```text
one meaning -> one mutable owner -> one production path -> one joined history
            -> one request meaning -> one evidence schema
                                    -> any number of immutable readers/presenters
```

Two paths may remain separate only when they answer different questions, own different data or native lifetimes, and the boundary is named and tested. For example, Editor and DevelopmentGame may have different presenters over one Application snapshot. Viewport screenshot readback and native-profiler frame capture remain separate because one produces an engine-owned image while the other controls an external tool around a native API frame. They may share `FrameId`, viewport identity, and nonblocking lifecycle conventions; they must not share a misleading “capture manager” or duplicate each other's state.

### Authority tuple required before implementation

Before any package moves from `Selected` to `In progress`, search code, headers, tests, CMake, commands, CVars, launch flags, schemas, documentation, and uncommitted work. Record this tuple in the change description:

```text
Concept and exact user/engineering question:
Canonical semantic definition:
Canonical mutable owner and lifetime:
Authoritative producer(s) and physical clocks/resources:
Single typed request/control path:
Single immutable publication/history/schema:
Consumers and presenters:
Current overlap candidates and searches used to find them:
Disposition for every candidate:
Exact obsolete symbols/files/links/tests to remove:
Proof that only the intended production path remains:
```

Every candidate receives exactly one disposition:

| Disposition | Meaning |
| --- | --- |
| `Reuse` | Use the current owner and contract without adding another path. |
| `Extend` | Add the smallest selected behavior to the current owner and its production route. |
| `Merge` | Move genuinely identical responsibility into the canonical owner, migrate every consumer, then delete the redundant owner. |
| `ReplaceAndDelete` | Land the replacement and remove the old implementation, API, tests, CMake/dependency edge, flag, and documentation in the same accepted package. |
| `KeepSeparateWithBoundary` | Retain distinct responsibilities only with a named data/lifetime boundary and a test proving they cannot become competing authorities. |
| `RejectAndDelete` | Do not ship the candidate; remove its experiment, stub, flag, dependency, and documentation claim. |

`Coexist`, `temporary`, `deprecated`, `compatibility path`, and “remove later” are not final dispositions. A migration adapter may exist only inside the active change needed to move callers; it is not built or retained when that package is accepted. No two-way synchronization is allowed between old and new state. A shared abstraction is created only when current concrete consumers prove identical semantics and a real stable boundary; deduplication is not permission to invent a generic framework.

### Known candidates and mandatory disposition

This is the reconciled 2026-08-17 starting inventory. Revalidate it rather than treating it as permanent truth.

| Concept | Current overlap/candidate | Required outcome |
| --- | --- | --- |
| Frame interval and FPS | `ViewportTopPanel::BuildPerformanceStats` reads ImGui `Framerate`/`DeltaTime`, while `Timer` is the host timing authority. | `ReplaceAndDelete`: derive FPS only from the valid Application-owned unscaled interval and remove the ImGui-derived product path. ImGui may retain its internal timing for ImGui itself, never as Sparkle performance truth. |
| Diagnostics session and history | No accepted session exists yet; future groups, graphs, workspaces, and export could each be tempted to retain samples. | `Extend` Application once: one demand/generation/join/ring/snapshot authority. `UnitGraph`, workspace views, compact presenters, and export read that ring; none owns a second history. No Core global profiler singleton. |
| Console command semantics | Core `ConsoleCommandRegistry` is hosted separately by `EditorConsoleSystem` and `RuntimeConsoleOverlay`. | `KeepSeparateWithBoundary` for product-local registry/session/presentation lifetimes; `Merge` performance command registration and parsing into one Application-owned registration function that produces the same typed request for both. No UI-formatted command strings. |
| Renderer request and publication transport | `RenderCoordinator`, its bounded control queue, and `PublishReadState` already cross the Renderer thread boundary. | `Extend` the existing route. Reject a second diagnostics mailbox, synchronous query facade, event bus, or UI callback channel. |
| GPU events, markers, and timing | `FrameExecutionDiagnostics`, `PassExecutionDiagnostics`, and backend `RenderTimingDiagnostics` already form the production path; current timing uses dynamic labels/containers/completion and disables parallel recording when enabled. | `ReplaceAndDelete` inside this path: one fixed token catalog feeds both markers and timing, fixed records/ranges replace dynamic/mutexed completion, bounded loss replaces ordinary exhaustion fatality, and the topology-changing `!CVarRendererDiagnosticGpuTiming` path is removed before detailed timing is accepted. No second GPU profiler. |
| GPU memory facts | RHI `RenderMemoryDiagnostics`, Renderer `RendererMemoryMonitor`, and a synchronous Renderer-to-Editor snapshot callback expose related facts. | `Reuse` RHI allocator facts and `Extend` the monitor using logical `FrameId`/monotonic cadence. Application retains sampled history. `ReplaceAndDelete` the overlapping synchronous performance callback after its consumers migrate; do not add another poller or allocation tracker. |
| Process memory | No production sampler was found. | Add one narrow Platform-owned snapshot and let Application own cadence/high-water presentation. Reject duplicate OS samplers and any accidental allocation-profiler scope. |
| Editor diagnostic providers | `UI` accepts broad Renderer callbacks, including `MemoryDiagnostics`; mesh/texture asset inspectors have different questions. | `ReplaceAndDelete` only the performance callback with the immutable Application model. `KeepSeparateWithBoundary` for asset inspectors whose data, owner, and interaction are not performance-session truth. |
| Task detail | `TaskProfiler` already emits ETW task/dependency events from serial and scheduled executors. | `Reuse` ETW for deep traces. Live diagnostics may add bounded executor-owned lane aggregates, not another task-event store, dependency graph, timeline history, or scheduler instrumentation authority. |
| Screenshot versus profiler capture | `EditorViewportCaptureCoordinator` and Renderer/RHI `RhiCaptureService` own nonblocking image readback; external native-profiler capture is not implemented. | `KeepSeparateWithBoundary`: never overload or rename screenshot readback into a generic capture facade. Share only viewport identity, `FrameId`, and lifecycle conventions. |
| Pre-device external integration | `RendererExternalRuntime` already owns process-facing configuration before backend/device creation. | `Extend` it with immutable launch intent and backend configuration. Reject a second bootstrap/integration manager and late device injection. |
| Backend diagnostic composition | `RenderHardwareInterface::GetDiagnostics()` already exposes neutral backend diagnostic services. | `Extend` the existing composition with the narrow neutral capability/request/result needed by selected providers. Native SDK types and provider objects remain backend-private; reject a parallel vendor-facing RHI facade. |
| Internal versus external frame capture | `ProfileGpu` consumes Sparkle-owned timestamp records; PIX/RenderDoc/Nsight consume native tool captures. | `KeepSeparateWithBoundary`: share stable tokens, `FrameId`, viewport selection, and evidence correlation, but keep mechanisms/artifacts/lifetimes distinct. One global external-capture arbiter prevents simultaneous native captures unless a tested matrix later permits them. |
| Capture presentation state | Multiple viewport icons and the workspace need the same status, while provider adapters own native execution state. | `Extend` one neutral request/result model and one global arbitration owner; each icon is an immutable projection for one provider. Provider adapters translate native state but do not invent UI/session state machines. |
| Evidence and export | No current performance export implementation was found; this plan and the workload already define artifact ownership. | Add one Application serializer for the accepted versioned snapshot/history schema and one workload-owned orchestration/analysis path. Views never write alternative CSV/JSON meanings, and docs link to generated evidence rather than copying it. |
| Documentation | Architecture, wireframes, UX research, runbook, workload, and this plan intentionally own different questions. | `KeepSeparateWithBoundary` and link to the owner. Merge only documents with the same audience and authority; delete copied policy/status text instead of maintaining two truths. |

### Enforcement and acceptance

All implementation prompts and phase gates inherit this contract even where they do not repeat it. A package cannot become `Accepted` while its candidate ledger contains an unresolved row. Proof is proportional to the change and includes all applicable items:

- repository searches for old types, functions, commands, CVars, flags, schema keys, and include paths return no production consumer;
- public headers and module dependencies expose one route, and CMake/build output does not retain an obsolete implementation or optional dependency;
- deterministic tests prove all readers see the same identity/meaning and that an old route cannot be invoked;
- architecture boundary checks prove the canonical owner did not move into a convenient but invalid layer;
- the diff contains the required deletions, not only a new preferred path;
- generated evidence contains one schema and one requested/active identity source;
- owning documents link instead of restating one another, and the documentation link/anchor check passes.

Literal source repetition in backend-private API translation or genuinely different presenter layout may remain when extraction would create coupling or erase a necessary boundary. The acceptance claim is therefore precise: zero competing authorities and zero unjustified duplicate implementations, with every deliberate separation accounted for.

## Verification Comes First

Testing is part of each package definition, not a follow-up phase. Before editing code, the implementer must show how the user will invoke the selected package, what Sponza should prove, what every currently supported map should prove, and what data the user must interpret. If that cannot be stated concretely, the package is not ready to implement.

The understanding should deepen monotonically:

```text
P0 symptom inventory
       -> P1 trustworthy measurements
       -> P2 likely limiting domain
       -> P3 reproducible per-map distributions
       -> P4 expensive owner/pass/wait/memory contributor
       -> P5 native/internal capture corroboration
       -> P6 causal experiment and scoped conclusion
```

### Test card required before implementation

Every selected package begins with this card in the implementation response or change description. Do not create a separate test-plan document.

```text
Package and selected consumer:
User action after delivery:
Build / product / backend:
Collection mode and expected observer cost:
Authority tuple and repository searches:
Duplicate candidates and disposition for each:
Exact replacement/deletion proof:
Sponza shakedown procedure:
All-supported-map sweep procedure:
Expected valid, unavailable, stale, loss, and failure behavior:
Exact automated tests to add/run:
Data the user must read:
Questions the user must answer for acceptance:
Artifact directory and files:
```

A spine such as `FND-01`, `INV-00`, `CAP-00`, or `EXT-00` is not a standalone shipped feature. It can pass engineering tests, but `FIN-04` may retain it only when at least one selected user-facing presenter, exporter, or capture adapter consumes it and completes the map-verification contract.

### Canonical ways to run a map

Use the Sparkle Launcher as the normal user path: choose the Showcase project, the required `DevelopmentGame` or `DevelopmentEditor` configuration, the backend, and one level card, then select `Run`. The launcher owns acquisition, selection, build/cook prerequisites, and launch composition.

For a narrow developer repro, launch the same product with the catalog ID in `SPARKLE_STARTUP_LEVEL`. The concrete executable/configuration remains build-directory-specific:

```powershell
$env:SPARKLE_STARTUP_LEVEL = 'Sponza'
& '<configured ShowcaseGame or ShowcaseEditor executable>'
```

The evidence harness replaces manual repetition once `EVD-01` is accepted. It must use the same catalog authority and must record requested and actually active level IDs. Do not add a second map list to runtime code.

### Current supported-map sweep

At the start of every package, regenerate the sweep roster from [the Showcase catalog](../../../../Projects/Showcase/Levels.catalog): include each level with no asset pack or with a referenced asset pack whose `RuntimeSupported` value is true. Source availability and selection are different from runtime support. A required external pack that is not present must be acquired through the launcher; it is not a reason to silently omit the map.

The roster reconciled on 2026-08-16 contains 13 levels:

| Catalog ID | Diagnostic role in the sweep | What a useful feature should help distinguish |
| --- | --- | --- |
| `Empty` | CPU/presentation/no-scene control | Fixed host/render/present overhead, timer noise floor, baseline allocations, and provider overhead. |
| `Sponza` | First shakedown and architectural baseline | Basic raster workload, material/lighting cost, stable camera, and the first comparison against Empty. |
| `DamagedHelmet` | Compact PBR control | Fixed overhead versus a small material/texture workload without large-scene cardinality. |
| `CesiumMan` | Skinned/animated control | Animation/update work, dynamic uploads, and CPU/GPU changes relative to static compact scenes. |
| `DiffuseTransmissionPlant` | Alpha/transmission stress | Material/overdraw/transmission sensitivity and whether the feature exposes unsupported/fallback state honestly. |
| `ABeautifulGame` | Repetition and material variety | Instance, draw, descriptor, texture-residency, and material-variation scaling. |
| `LPSHead` | Dense character/head asset | Geometry, texture, skin/material, upload, and memory behavior on a focused external asset. |
| `CornellBox` | Lighting/reference control | Fixed geometry with lighting, ray/path/reference-mode, and presentation comparisons where supported. |
| `BistroExterior` | Large exterior scene | Geometry/material/texture scale, extraction/culling, command generation, GPU passes, and memory pressure. |
| `BistroInteriorWine` | Dense interior/material scene | Visibility, glass/material fallback, lighting, overdraw, and interior/exterior workload change. |
| `ModernSponza` | High-resolution composed scene | Texture/geometry residency, transparency, material variety, and large modern asset behavior. |
| `ModernSponzaCandles` | Emissive-density stress | Emissive/source-light workload, repeated content, lighting-pass cost, and memory change. |
| `ModernSponzaKnight` | Large scene plus animation | Modern Sponza baseline plus animation/skinning/update and temporal stability. |

`SanMiguelHigh`, `SanMiguelLow`, and `JungleRuins` remain source-readiness checks, not runtime performance sweeps, while their catalog asset packs are `RuntimeSupported = false`. Future Ivy, Trees, Flood, and Explosion content is likewise excluded until its declared blocker closes. If the catalog changes, update the workload roster in the same change and test the newly supported level from that package onward.

### Run ladder for every selected package

1. **Automated contract:** run deterministic unit/concurrency/serialization tests and deliberate invalid, overflow, stale-generation, and failure cases owned by the package.
2. **Sponza shakedown:** exercise the exact user action on Sponza first. Fix crashes, missing data, wrong units, stale identity, and unusable interaction before spending time on the full roster.
3. **All-map functional sweep:** use the action on every runtime-supported map. Prove the requested level became active, wait for settled state when available, and verify the feature publishes or explicitly explains unavailability. No map may disappear from the result table.
4. **Quantitative sweep:** for packages that report timing, distribution, memory, or comparison data, use the workload profile: fixed initial camera, fixed resolution once supported, VSync/presentation policy recorded, at least 300 warm-up frames after readiness, then at least 300 valid samples. One run validates wiring; at least three runs are required for a performance conclusion.
5. **Backend sweep:** use D3D12 for the first package shakedown. A Renderer/RHI/backend-neutral package runs the full supported-map sweep on D3D12 and Vulkan before its phase gate. Backend-specific provider packages run only their declared backend but must state the excluded cells.
6. **Editor/Game sweep:** measurements intended to represent the game use `DevelopmentGame`. Editor-only UI is exercised on every map in `DevelopmentEditor`; compare at least Empty, Sponza, and the worst measured map against DevelopmentGame before attributing editor overhead.
7. **Owner reading task:** review the generated all-map table and raw/capture evidence, answer the package questions below, and record the decision. Automated green checks alone cannot accept a diagnostic feature.

Package-verification artifacts belong under:

```text
artifacts/validation/performance-diagnostics/
  <package-id>/<run-id>/
    sweep-summary.md
    <level-id>/manifest.json
    <level-id>/timings.csv          # when the package produces timing samples
    <level-id>/capture-or-log.*     # only when explicitly requested
```

These are generated evidence, not new repository planning documents. Formal `MAP-*` content acceptance continues to use the workload-owned `showcase-levels` layout.

### Universal instrumentation acceptance criteria

A selected package passes its engineering AC only when all applicable statements are true:

- the user action is documented at the feature entry point and works without developer-only memory inspection;
- every supported map is present in the sweep with requested and active identity, backend, product, configuration, sample population, and result;
- there is no device removal, fatal diagnostic, deadlock, unbounded wait, unexplained process exit, or uncategorized error;
- timing runs retain at least 300 valid post-warm-up samples per run, or explicitly fail rather than silently reducing `N`;
- unavailable, stale, discontinuous, truncated, lost, and unsupported data are labeled and counted, never converted to zero;
- `FrameId`, generation, unit, interval, provenance, and inclusion rules match the architecture and the exported/raw record;
- repeated runs use identical declared settings; changed settings start a new cohort and cannot be pooled silently;
- `LiveBasic` satisfies the architecture's Empty/Sponza observer hypotheses: zero post-initialization per-frame heap allocations, under 4 MiB retained live history, CPU p50 change below 1%, CPU p95 change below 2% with the predeclared Empty noise floor, and GPU p95 disturbance below the larger of 1% or 0.1 ms;
- detailed, benchmark, internal-capture, and external-capture disturbance is measured and displayed even where no fixed budget is declared;
- the feature-specific expected result below passes on Sponza and remains semantically valid across the full map roster.

A slow map does not fail the diagnostics feature. Incorrect, incomplete, irreproducible, or misleading diagnostics fail it. Performance targets remain workload decisions.

### Owner reading and diagnosis AC

After each package, the implementer hands the user this task with the populated sweep. The package remains `In progress` until the user records an answer or explicitly delegates the review:

```text
1. Which maps are CPU-likely, GPU-likely, wait/presentation-likely, mixed, or
   still insufficient, and what exact fields support each label?
2. Which three maps have the worst p95/worst-frame behavior, and are the valid
   populations and configurations comparable?
3. What changed from Empty -> Sponza -> the worst map, and which owner/pass/wait/
   memory value changed with it?
4. Which map families share the same signature, and which map falsifies that
   grouping?
5. Are any conclusions weakened by stale/lost/unsupported data or observer cost?
6. What is the next discriminating package, capture, or controlled experiment?
7. Decision: Accept, Accept with named follow-up, Reject/delete, or Insufficient
   evidence. Give the evidence paths.
```

Use this confidence vocabulary in `sweep-summary.md`:

| Confidence | Meaning |
| --- | --- |
| `Observed` | A valid repeatable symptom/distribution exists. |
| `Correlated` | The symptom changes with a named owner/pass/wait/memory fact, but alternatives remain. |
| `Capture-confirmed` | An internal/native capture corroborates the same frame/token/marker and suspected mechanism. |
| `Causal` | A predeclared controlled change moves the predicted metric without unacceptable correctness/quality regression. |

“Fully understood” is reserved for the Phase-6 all-map diagnosis gate. It requires every supported map to have at least an `Observed` diagnosis, every material performance-signature cluster to have a `Capture-confirmed` explanation, and every top-priority problem to have a causal experiment or an explicit `Unresolved` limitation. Shipping a narrower selected feature set remains allowed, but it cannot claim complete performance understanding if that set leaves a required domain unmeasured.

## Delivery Rules

### Ordered phases, selectable packages

Phases close in order. Do not begin implementation from phase `P2` until the `P1` exit gate is closed, even when the desired feature appears independent. Within the active phase, any optional package may be selected, deferred, or rejected.

Each phase contains three kinds of work:

| Kind | Rule |
| --- | --- |
| Gate | Mandatory review and verification work. A phase with no selected optional package still performs its gate and closes as `No optional delivery`. |
| Spine | Shared production code required only when at least one dependent package is selected. Implement it once through its existing owner. |
| Feature | A user-visible capability or evidence product that may be selected independently when all hard dependencies are accepted. |

This distinction prevents two opposite failures: forcing every proposed feature into the engine, and creating empty frameworks for features that may never ship.

### Package states

Use exactly these states in the delivery ledger:

| State | Meaning |
| --- | --- |
| `Available` | Ready for an owner decision; no implementation is implied. |
| `Selected` | Approved for the current phase and all hard dependencies are accepted or selected. |
| `In progress` | One bounded change is active. At most one package should normally be in this state. |
| `Accepted` | The vertical slice and its required verification passed. |
| `Deferred` | Not being delivered now. No stub, dormant UI, placeholder flag, or unused public type is retained. |
| `Rejected` | Evidence says not to ship it. Record the reason and delete the experiment or superseded path. |
| `Blocked` | A named external prerequisite prevents progress; this is not a substitute for a decision. |

A phase closes only when no package in that phase remains `Available`, `Selected`, `In progress`, or `Blocked`. Deferring every optional feature is allowed; failing to make the decisions is not.

Moving `Selected -> In progress` additionally requires a complete test card and authority tuple. Moving `In progress -> Accepted` requires every duplicate candidate to have a final disposition and its required deletion/boundary proof. A package with an unresolved overlap remains `In progress` even when it builds and its new path works.

### Selection is not a feature-flag framework

Feature selection primarily controls what code is implemented and retained. Do not add a compile-time or runtime switch for every package.

- Fixed stat groups that ship are entries in one bounded catalog and use typed runtime demand.
- Build switches exist only for real distribution or dependency boundaries, such as whether a vendor capture adapter may be compiled and packaged.
- Launch options such as `-Pix`, `-RenderDoc`, and `-Nsight` express immutable process-start intent; they do not make unbuilt providers appear available.
- Deferred packages leave no compatibility layer, placeholder menu item, empty source file, or reserved public API.
- A prototype that fails its acceptance test is removed in the same package unless an explicitly selected follow-up consumes it.

### Vertical-slice rule

Every selected feature must reconcile current candidates and reach one real consumer in the same package:

```text
overlap search -> disposition -> typed demand -> authoritative producer
               -> bounded publication -> immutable model
               -> one presenter/exporter -> tests/evidence
               -> obsolete path/API/dependency/docs removed
```

Do not land a horizontal “diagnostics framework” with no accepted row, view, capture, or export. Do not add a second collector for a second presenter. Editor and DevelopmentGame may render differently, but they consume the same Application-owned truth and submit the same typed requests.

### What may be frozen

Freeze only a contract that crosses a real lifetime or artifact boundary:

- stable `FrameId` and generation semantics;
- metric units, inclusion rules, validity, and provenance;
- fixed tokens serialized into captures or evidence;
- bounded request/result shapes used across threads;
- an accepted evidence schema version.

Do not freeze speculative class names, directory trees, generic extension interfaces, arbitrary capacities, or provider abstractions before their selected consumer and measured bounds exist. Resolve the architecture's open capacity and timeout questions in the first package that needs each value, then record the calibrated decision in the owning architecture section or code test—not in a new decision document.

## Ship-Control Ledger

Copy this compact record into the active change description and update this table only when a package decision changes. `Accepted` requires an evidence link or exact command/result in the change handoff.

| ID | Package | Kind | Initial state | Hard dependencies |
| --- | --- | --- | --- | --- |
| `P0-GATE` | Reconcile contracts, baseline, and selected scope | Gate | Required | None |
| `FND-01` | Application session, demand, join, and immutable publication | Spine | Available | `P0-GATE` |
| `FND-02` | Host phase and frame-interval measurements | Feature | Available | `FND-01` |
| `FND-03` | Renderer CPU stages, queue waits, and top-level GPU queue spans | Feature | Available | `FND-01` |
| `FND-04` | Process working set and private commit | Feature | Available | `FND-01` |
| `FND-05` | GPU-memory polling identity and neutral segment model | Feature | Available | `FND-01` |
| `P1-GATE` | Foundation semantics, concurrency, and bound checks | Gate | Required | Selected `FND-*` packages |
| `ORI-01` | Shared typed `Stat` command registration | Spine | Available | `FND-01` |
| `ORI-02` | `Fps` compact group | Feature | Available | `FND-01`, `FND-02`, `ORI-01` |
| `ORI-03` | `Unit` compact group and limiting-domain hint | Feature | Available | `FND-01`–`FND-03`, `ORI-01` |
| `ORI-04` | `UnitGraph` from the joined history | Feature | Available | `ORI-03` |
| `ORI-05` | Editor `Quick Check` and viewport summary | Feature | Available | `ORI-03` |
| `ORI-06` | DevelopmentGame compact presenter | Feature | Available | `ORI-01` and one selected compact group |
| `P2-GATE` | First-product usability, cost, and stale-path checks | Gate | Required | Selected `ORI-*` packages |
| `EVD-01` | Deterministic evidence request, readiness, resolution, and viewport sidecar | Spine | Available | `FND-01` |
| `EVD-02` | Raw samples, manifest, and summary export | Feature | Available | `EVD-01`, `FND-02`–`FND-05` |
| `EVD-03` | `MAP-00` Sponza calibration run | Feature | Available | `EVD-02` |
| `EVD-04` | Repeated-run comparison and regression analysis | Feature | Available | `EVD-02`, `EVD-03` |
| `P3-GATE` | Evidence reproducibility and no-default-files check | Gate | Required | Selected `EVD-*` packages |
| `INV-00` | One fixed Performance workspace and shared selection | Spine | Available | `FND-01` |
| `CAP-00` | Fixed GPU token/query plan preserving parallel recording | Spine | Available | `FND-03` |
| `INV-01` | CPU/Threads view with physical and logical ownership | Feature | Available | `INV-00`, `FND-02`, `FND-03` |
| `INV-02` | GPU Live view | Feature | Available | `INV-00`, `FND-03` |
| `INV-03` | `GpuPasses` live detail | Feature | Available | `INV-02`, `CAP-00` |
| `INV-04` | Memory view | Feature | Available | `INV-00`, `FND-04`, `FND-05` |
| `INV-05` | Render and RHI owner counters | Feature | Available | `INV-00`, `FND-03` |
| `INV-06` | Task-lane aggregates | Feature | Available | `INV-00`, `INV-01` |
| `INV-07` | Scene cardinality | Feature | Available | `INV-00`; existing production-owner counters |
| `INV-08` | Bounded hitch list and shared-frame navigation | Feature | Available | `INV-00`, `ORI-04` |
| `P4-GATE` | Investigation value, bounds, and presenter checks | Gate | Required | Selected `INV-*`/`CAP-00` packages |
| `CAP-01` | One frozen `ProfileGpu` capture | Feature | Available | `CAP-00`, `INV-00`, `INV-02` |
| `EXT-00` | External-provider launch, capability, arbitration, and icon-group spine | Spine | Available | `FND-01` |
| `EXT-01` | PIX capture on D3D12 | Feature | Available | `EXT-00` |
| `EXT-02` | RenderDoc capture on D3D12 | Feature | Available | `EXT-00` |
| `EXT-03` | RenderDoc capture on Vulkan | Feature | Available | `EXT-00` |
| `EXT-04` | Nsight Graphics capture on D3D12, experimental | Feature | Available | `EXT-00`; runbook matrix passes |
| `EXT-05` | Nsight Graphics capture on Vulkan, experimental | Feature | Available | `EXT-00`; runbook matrix passes |
| `P5-GATE` | Capture lifecycle, topology, compatibility, and artifact checks | Gate | Required | Selected `CAP-01`/`EXT-*` packages |
| `FIN-01` | Product/backend/build matrix for the accepted set | Gate | Required | All accepted packages |
| `FIN-02` | Failure, device-loss, shutdown, and stale-generation audit | Gate | Required | All accepted packages |
| `FIN-03` | Observer cost, capacity calibration, and soak evidence | Gate | Required | All accepted packages |
| `FIN-04` | Replacement deletion, documentation, and final shipment ledger | Gate | Required | `FIN-01`–`FIN-03` |

The dependencies express capability, not schedule preference. For example, selecting `EVD-03` intentionally pulls in the complete evidence path needed by `MAP-00`. Selecting only `ORI-02` does not.

Example selections make the intended granularity concrete:

| Desired shipment | Select | Close without optional delivery |
| --- | --- | --- |
| Honest FPS only | `FND-01`, `FND-02`, `ORI-01`, `ORI-02`, then `FIN-*` | Remaining Phase 1 features and Phases 3–5 |
| `MAP-00` evidence harness without a large in-editor profiler | `FND-01`–`FND-05`, `EVD-01`–`EVD-03`, then `FIN-*`; add orientation packages only if wanted | Unselected Phase 2, 4, and 5 features |
| RenderDoc Vulkan viewport button only | `FND-01`, `EXT-00`, `EXT-03`, then `FIN-*` | Phase 1's other features and all optional Phase 2–4 packages |
| Internal GPU investigation | `FND-01`, `FND-03`, `INV-00`, `INV-02`, `CAP-00`, then `INV-03` and/or `CAP-01`, followed by `FIN-*` | Unrelated CPU, memory, evidence, scene, task, and external-provider packages |

In every example, the phase gates are still reviewed and closed in order. A package listed in a later phase is not implemented early merely because its dependencies are small.

## Current-Code Reconciliation

Revalidate this table with `rg` at the start of each phase and reconcile it into the mandatory authority/candidate ledger above. It records the 2026-08-17 route to extend, not permanent file-name policy.

| Responsibility | Current owner/path | Delivery decision |
| --- | --- | --- |
| Monotonic application identity and unscaled delta | `Timer`, used by `RuntimeApplication::BeginFrame` and `UpdateRuntime` | Reuse `Timer::GetFrameCount()` as the host `FrameId` source. Measure explicit begin-to-begin and phase boundaries in Application; do not create another clock singleton. |
| Runtime composition and lifetime | `RuntimeApplication` | Own the diagnostics session beside the existing Timer/Window/Renderer runtime. Create after host prerequisites, publish after available domain results, and destroy before those producers. |
| Editor composition | `EditorApplication` and `EditorUiFrameRenderer` | Adapt the Application-owned immutable snapshot and typed requests into Editor presentation. Do not make `SparkleEditor` depend upward on `SparkleApplication`. |
| Console parsing | Core `ConsoleCommandRegistry`, hosted by `EditorConsoleSystem` and `RuntimeConsoleOverlay` | Add one registration function used by both composition roots. The UI sends typed requests directly; it never formats a console command. |
| Renderer cross-thread control/publication | `RenderCoordinator`, `RenderControlCommandQueue`, and `PublishReadState` | Extend the existing bounded control/read-state route with diagnostics request/result products. Avoid synchronous per-frame queries and avoid a second mailbox framework. |
| Renderer execution measurements | `FramePipeline`, frame-graph executor, and existing diagnostic scopes | Instrument fixed owner boundaries and publish one bounded frame result keyed by `FrameId`. Do not scan the graph after execution merely to populate counters. |
| GPU timing | `FrameExecutionDiagnostics` and backend `RenderTimingDiagnostics` | Replace dynamic strings/vectors/mutex completion with stable tokens, fixed records, preassigned per-recording-chunk ranges, deterministic merge, and bounded loss for the accepted live/capture path. |
| Parallel command recording | `FrameGraphRecordingExecutor::ShouldRecordBatchInParallel` | Remove the `!CVarRendererDiagnosticGpuTiming` topology change before accepting `CAP-00`, `INV-03`, or `CAP-01`. A captured profile must not silently serialize normal recording. |
| GPU memory | RHI `RenderMemoryDiagnostics` and Renderer `RendererMemoryMonitor` | Reuse allocator facts. Correct the monitor to use logical `FrameId` or monotonic time rather than the wrapping frame-in-flight slot; preserve local/non-local and used/allocated/budget distinctions. |
| Process RAM | No current production sampler | Add the smallest Platform-owned process snapshot required by Application, initially Windows-backed. Do not build an allocation tracker or put Win32 types in Application. |
| Task detail | `TaskProfiler` ETW provider and fixed task lanes | Reuse ETW for deep task traces. Live UI may publish bounded lane aggregates only when the executor already owns the counts/durations. |
| Viewport summary | `ViewportTopPanel::BuildPerformanceStats` currently reads ImGui FPS/delta | Replace this source with the immutable diagnostics presentation. Do not retain two competing FPS truths. |
| Editor diagnostics providers | `UI` currently receives broad renderer snapshot callbacks | Migrate only overlapping performance responsibilities to the Application product and remove replaced callbacks. Asset inspector routes remain separate. |
| Viewport screenshot | `EditorViewportCaptureCoordinator` plus Renderer/RHI readback | Keep it as image capture and reuse its nonblocking lifecycle lessons only. External profiler capture is a different operation and must not overload `RhiCaptureService`. |
| Pre-device integrations | `RendererExternalRuntime` builds immutable `RendererBackendConfiguration` before `RenderCoordinator` creates the backend | Extend this existing process-facing owner with launch intent and capture bootstrap. Do not add a competing startup integration service. |
| Backend diagnostics | `RenderHardwareInterface::GetDiagnostics()` returns neutral RHI diagnostic services | Add only the narrow neutral capture capability/request/result needed above RHI. Native APIs, handles, DLLs, SDK state, and provider objects remain in D3D12/Vulkan private adapters. |
| Build membership | Module `CMakeLists.txt` files glob owned Public/Private sources; RHI composes common and backend-specific targets | Put files under the current owner and add explicit optional SDK/package rules only where provider eligibility requires them. Do not introduce a Diagnostics module. |
| Tests | Existing CTest executables are co-located under module `Tests` | Add small pure tests with the first testable contract in its owning module. Do not build a generic diagnostics test framework. |

## Target Shape

```text
                      Application / host owner
       +----------------------------------------------------+
Timer->| session + typed demand + FrameId join + bounded ring|
       |     |                 |                    |        |
       |     + host phases     + process RAM        + export |
       +-----------^----------------------^------------------+
                   | immutable results    | typed requests
          +--------+--------+     +-------+-----------------+
          | Renderer owner  |     | presenters              |
          | CPU stages/waits|     | Editor viewport/window  |
          | frame result    |     | DevelopmentGame overlay |
          +--------^--------+     +-------------------------+
                   |
       +-----------+-----------------------------------------+
       | RHI neutral diagnostics                             |
       | timestamps | memory | capabilities | capture request|
       +-----^---------------------------^-------------------+
             | backend-private             | pre-device config
       +-----+-----------+          +------+----------------+
       | D3D12 adapter   |          | Vulkan adapter         |
       | PIX/RenderDoc/  |          | RenderDoc/Nsight*      |
       | Nsight*         |          |                        |
       +-----------------+          +------------------------+

* only when explicitly built, requested/detected, and capability-tested
```

The data direction is always producer to immutable result to Application join to presenter/export. A UI request travels back through a typed bounded command. No panel reads renderer storage, waits for the RenderThread, receives a vendor handle, or owns capture state.

## Phase 0 — Reconcile And Choose

### Test it first

Before changing instrumentation, use the Launcher to run Sponza and then every current supported map in `DevelopmentGame`, D3D12, diagnostics `Off`, with one fixed initial camera and recorded presentation settings. Until deterministic readiness/resolution exists, record the actual extent and a manual settled observation; do not compare unlike extents or pretend this is benchmark evidence.

Sponza passes the baseline shakedown when it loads the requested level, remains responsive for the observation window, emits no fatal/device-removal error, and current thread names/ETW/GPU markers are discoverable. The full roster passes when all 13 requested IDs are actually active in turn and every failure/fallback is categorized.

Owner task: write an initial symptom matrix—visible stutter, approximate current FPS only as orientation, process-memory observation where available, warnings, and missing measurements. Mark every causal conclusion `Insufficient`; P0 establishes controls and blind spots, not root cause.

### Outcome

Close uncertainty before code without creating speculative frozen contracts. The deliverable is a selected package set, a current ownership map, and a reproducible baseline proving what the engine does before instrumentation.

### `P0-GATE` required work

1. Re-read the authority documents listed above and every standard selected by the touched packages.
2. Use `rg` to revalidate the current owners, producers, consumers, thread lifetimes, CMake membership, tests, and any overlapping uncommitted work.
3. Build the authority tuple and duplicate-candidate ledger for every selected concept. Search names, semantic equivalents, callers, commands, flags, schemas, tests, and dependencies; assign each candidate one allowed disposition and name the deletion or separation proof.
4. Mark every `FND-*` package `Selected`, `Deferred`, or `Rejected`. A later phase may promote a deferred package only by reopening dependency review.
5. Capture a source-backed control run with diagnostics timing disabled and the existing thread names/ETW/GPU markers. Record product, backend, build profile, pipeline mode/depth, resolution, VSync/presentation policy, adapter/driver, commit, and known invalid data.
6. For each selected foundation metric, write its producer, physical owner, logical phase, clock, unit, interval, inclusion/exclusion rule, `FrameId`, validity, and consumer in the implementation change description.
7. Size the first fixed record/ring from `sizeof`, update cadence, retention need, and a deliberate overflow test. Architecture capacities remain hypotheses until this check.

### Positive guardrails

- Prefer an experiment or existing trace when an open decision concerns cost, cardinality, or topology.
- Record a narrow decision next to the implementation or its owning architecture section.
- Preserve a serial control and threaded default for later observer comparisons.
- Keep one small change ledger: `add`, `modify`, `replace`, `delete`, `verify`.

### Negative guardrails

- No new module, registry, event bus, trace store, generic counter API, or empty presenter.
- No “final” capacity chosen from intuition alone.
- No optimization based on the current viewport FPS or one screenshot.
- No implementation of Phase 1 while metric meaning or owner remains ambiguous.

### Exit gate

- The selected foundation set and all hard dependencies are explicit.
- Every known and newly discovered overlap has one allowed disposition; no ambiguous owner or parallel route is approved for implementation.
- The baseline is reproducible and names invalid/unavailable observations.
- No code or document claims a proposed metric is implemented.
- `git diff --check` passes for any reconciliation edits.

### Ready-to-use implementation prompt

```text
Execute Performance Diagnostics phase P0-GATE only. Begin by filling the required
test card and showing the Sponza plus current all-supported-map baseline procedure,
expected criteria, artifact path, and owner reading task. Read the architecture,
wireframes, runbook, acceptance workload, AGENTS.md, and all applicable
engineering standards. Reconcile current code owners, consumers, lifetimes,
thread boundaries, CMake membership, tests, and dirty work with rg. Fill the
authority tuple and duplicate-candidate ledger; give every candidate one allowed
disposition and name exact deletion or boundary proof. I select
these foundation packages: <FND IDs>. Mark every other FND package Deferred or
Rejected with a reason; do not create stubs for it. Capture the smallest honest
serial/threaded baseline and define every selected metric's owner, clock, unit,
FrameId, validity, and inclusion rule. Resolve only decisions required by the
selected packages. Make no product feature and no generic diagnostics framework.
Run the baseline roster, give the user its symptom/blind-spot matrix to review,
and do not close P0 before that AC decision. Report exact evidence and update the
single delivery ledger.
```

## Phase 1 — Build The Bounded Data Spine

### Test it first

Fill one test card per selected `FND-*` package before editing. Add the named deterministic CTest or an equivalently narrow owner test; the exact test executable may contain multiple cases but must keep these names addressable through `ctest -R`.

| ID | Automated AC | Sponza and all-map user verification | Required reading task |
| --- | --- | --- | --- |
| `FND-01` | `performance_diagnostics_session`: generation, delayed/out-of-order join, overflow, stale rejection, mode release, shutdown. | Exercise through the first selected presenter/exporter on Sponza, then every map. `FrameId` is monotonic, active level/generation is current, missing domain results remain missing, and no wait is introduced. | Count valid/missing/stale/lost frames per map and explain every nonzero category. A spine with no selected consumer cannot ship. |
| `FND-02` | `performance_diagnostics_host_timing`: injected clock, begin-to-begin interval, phase nesting, pause/minimize/discontinuity. | Collect 300 valid samples per map. FPS derives from the same unscaled interval; physical owner and logical Gameplay/Editor phases are correctly labeled. | Rank maps by CPU frame p95 and host/game/editor phase p95; compare Empty and Sponza and flag any phase whose meaning is still ambiguous. |
| `FND-03` | `renderer_performance_diagnostics`: serial/threaded delayed publication, queue span, wrap, missing query, bounded loss. | Sweep every map on D3D12 and Vulkan. Each rendered frame has honest render/wait/present and supported queue-span state with its own `FrameId`; no panel synchronously queries Renderer. | Classify likely CPU-render, GPU, wait/present, mixed, or insufficient per map without adding pipelined columns as one total. |
| `FND-04` | `process_memory_diagnostics`: injected values, unsupported platform, cadence, high-water reset semantics. | Observe at least 60 settled seconds per map or the complete quantitative run. Working set/private commit are nonzero on Windows, sampled at the declared cadence, aged visibly, and never mislabeled as allocation detail. | Rank current and session-high-water values; identify growth between start/end and distinguish working set from private commit. |
| `FND-05` | `renderer_memory_monitor`: frame-slot wrap, monotonic cadence, unavailable budget, segment/category bounds. | Sweep all maps/backends. Tracked used does not exceed its corresponding allocated/block amount; local/non-local/budget/retirement fields are distinct or explicitly unavailable; polling survives frame-slot wrap. | Rank GPU current/high-water and pressure per map; compare Modern Sponza/Bistro families with Empty and name missing backend facts. |

Phase-1 instrumentation passes only if its Sponza and full-map records meet the universal AC and the selected metrics already expose a useful map-to-map difference or honestly show no difference. A test-only producer with no path to a selected Phase-2/3/4/5 consumer is not a shippable delivery.

### Outcome

Selected domains can publish correlated facts without blocking one another. Application owns one session and immutable read product; no UI is required to prove the spine.

### Package deliveries

#### `FND-01` Application session, demand, join, and publication

Value: establishes one authority for collection mode, generation, selection, delayed joins, history, and read snapshots so later features do not each invent state.

Implement one Application-owned session with bounded typed demand and fixed-capacity `FrameId` joins. Producers may arrive late or out of order. Publication never waits for a missing Renderer/GPU result, never relabels an old result as current, and never combines mismatched frame identities for a correlated claim. Keep the Application-to-Editor adaptation at the existing `SparkleApplicationEditor` composition boundary to avoid an Editor/Application dependency cycle.

Acceptance: deterministic tests cover generation changes, delayed/out-of-order results, stale rejection, missing fields, overflow/loss accounting, mode demotion, and shutdown with work in flight.

#### `FND-02` Host phases and frame interval

Value: separates begin-to-begin frame interval from actual Application-owned phase wall time and exposes the Editor/Game physical-owner distinction.

Instrument fixed orchestration boundaries in `RuntimeApplication`/`EditorApplication`; reuse `Timer` identity and a steady monotonic clock. Names must distinguish physical thread (`Sparkle.EditorThread` or `Sparkle.GameThread`) from logical `Gameplay.*`, Editor, wait, and presentation phases. Do not turn arbitrary scoped timers into a public macro system.

Acceptance: a controlled injected-clock test proves units and inclusion rules; serial and threaded runs preserve the same semantic labels.

#### `FND-03` Renderer CPU, waits, and top-level GPU spans

Value: supplies the minimum facts needed to distinguish host, render CPU, queue/pacing wait, and GPU-limited hypotheses.

Measure existing Renderer owner boundaries and publish a bounded immutable result through `RenderCoordinator::PublishReadState`. Attach logical `FrameId`, pipeline mode/depth, queue, source/provenance, validity, and loss. Top-level GPU value is an outer same-queue span, never a sum of passes. Keep detailed scopes off.

Acceptance: serial/threaded tests prove delayed correlation and no synchronous UI query; D3D12 and Vulkan smoke runs either produce semantically equivalent fields or an explicit capability state.

#### `FND-04` Process memory

Value: provides honest RAM growth and pressure orientation needed by `MAP-00` without pretending the engine owns an allocation profiler.

Add a narrow Platform process-memory snapshot returning working set and private committed bytes plus sample time/validity. The initial Windows implementation may use native APIs privately. Application samples at the declared slow cadence and owns session/run sampled high-water; OS process-lifetime peaks remain distinct.

Acceptance: supported/unsupported and injected-value tests; UI/export names never collapse working set, private commit, and sampled peak into “RAM.”

#### `FND-05` GPU memory identity and segments

Value: makes existing allocator diagnostics safe to consume over time and prevents a wrapping frame-slot index from corrupting the poll cadence.

Change `RendererMemoryMonitor` polling to logical `FrameId` or monotonic time and add wrap/regression tests. Publish used, allocated/block, local, non-local, budget, transient, delayed-retirement, age, and availability only where backend facts support them. Preserve the existing allocator owner; do not copy its live vectors into every frame.

Acceptance: frame-in-flight wrap test, cadence test, unavailable-budget test, and paired backend field audit.

### Positive guardrails

- Use fixed records or bounded containers on hot/cross-thread paths; publish loss and truncation explicitly.
- Reuse `RenderCoordinator` control/read publication and existing task runtime.
- Keep domain measurements in their producing owner and join only immutable results in Application.
- Make mode/generation changes atomic from the consumer's perspective.
- Add tests in the owning module with the first pure logic, using deterministic fake clocks/results.

### Negative guardrails

- No Editor reads of `RendererHost`, `FrameExecutionDiagnostics`, allocator storage, or live ECS state.
- No per-frame heap strings, vectors, locks, file I/O, or synchronous RenderThread/GPU waits in the selected data path.
- No fatal error for ordinary record/query exhaustion; report bounded loss. Fatal remains for broken owner/API invariants.
- No second scheduler, worker pool, mailbox framework, or global Core profiler singleton.
- No public type merely because a future package might use it.

### `P1-GATE` exit

- Every selected foundation feature has a real producer and deterministic consumer/test.
- Every authority tuple is complete; no selected metric has a second collector, mutable owner, request route, history, or public facade, and every replaced path is absent from the build.
- Owner/thread/lifetime assertions and overflow behavior are covered.
- `sizeof` and total retained bytes are recorded and within the calibrated bound.
- Serial and threaded behavior retain identical meanings; missing data stays visibly invalid.
- Selected module builds/tests pass; Renderer/RHI boundary changes pass `architecture_boundary_check`; `git diff --check` passes.

### Ready-to-use implementation prompt

```text
Implement only these selected phase-1 packages: <FND IDs>. Before editing, fill
one test card per package with its named deterministic test, Sponza shakedown,
13-map/backend sweep, expected validity/loss behavior, and owner reading AC. Fill
the authority tuple, search for semantic and name-level overlaps, assign every
candidate one disposition, and name the old symbols/dependencies to remove. The
P0 gate and all hard dependencies are accepted. Extend RuntimeApplication, the existing
RenderCoordinator control/read-state path, Renderer diagnostics, RHI diagnostics,
and Platform only where their current responsibility requires it. Application
owns session/demand/join/publication; domains own measurements. Use bounded fixed
records, explicit FrameId/generation/validity/provenance, nonblocking delayed
publication, and deterministic tests. Preserve the serial control and threaded
topology. Do not add UI, a generic stat registry, another task runtime, dynamic
hot-path labels, default files, or code for deferred packages. Remove any path
replaced by the selected packages. Hand the user the populated map matrix and
interpretation questions; keep the package In progress until reviewed. Report
exact build/test/boundary and evidence results.
```

## Phase 2 — Ship The First Orientation Surface

### Test it first

After automated command/model tests, run each selected action on Sponza in `DevelopmentEditor`, then repeat it on every map. Run `ORI-06` in `DevelopmentGame` as well. Leave each overlay open for the quantitative window and also measure it closed so the observer delta is available.

| ID | User action | Expected criteria on Sponza and every supported map | Required reading task |
| --- | --- | --- | --- |
| `ORI-01` | Enter `Stat` for help, then enable/disable every selected group; test case-insensitive names, autocomplete, unknown input, four-group limit, presets, and `Stat None`. | Editor and DevelopmentGame parse to the same typed requests; unavailable groups explain why; no silent eviction or presenter-specific behavior. | Confirm the command changes only demand/presentation and does not create a second measurement series. |
| `ORI-02` | `Stat Fps`. | Displayed FPS equals the valid unscaled interval derivative within presentation rounding, has visible sample/age state, and never substitutes ImGui FPS. | Rank FPS p50/p95-derived orientation across maps, then restate the ranking in milliseconds to avoid FPS-only reasoning. |
| `ORI-03` | `Stat Unit` or `Performance > Quick Check` when `ORI-05` is selected. | Milliseconds lead; frame, host/game/editor, render, wait/present, and GPU fields keep their own identity/validity. The hint is `Likely`, `Mixed`, `Waiting`, or `Insufficient`, never an unqualified cause. | Produce the first all-map limiting-domain matrix and cite the two fields supporting every classification. |
| `ORI-04` | `Stat UnitGraph`; select normal and tail frames. | The graph uses the joined ring, preserves gaps/invalid frames, and selected values match table/raw samples for the same `FrameId`. | Select the worst three frames per map and state whether the tail comes from CPU, GPU, wait/present, discontinuity, or insufficient correlation. |
| `ORI-05` | Open the viewport Performance menu, run `Quick Check`, customize selected stats, narrow the viewport, and use keyboard-only navigation. | One compact responsive surface, correct Basic/Detailed banner, no overlap, non-color state labels, bounded rows, and the old ImGui-derived text is gone. | Confirm the UI makes the next investigation action obvious on the worst map without changing the underlying result. |
| `ORI-06` | Open the DevelopmentGame console and issue the same selected `Stat` requests on every map. | Same semantics/data source as Editor, compact game presentation, no editor dependency, no output by default in Shipping. | Compare Empty, Sponza, and the worst map between DevelopmentEditor and DevelopmentGame; quantify rather than assume editor overhead. |

The owner accepts a package only after reading the all-map summary and deciding whether the orientation is sufficient to choose the next diagnostic action. A pretty overlay with no trustworthy map classification is rejected or returned for correction.

### Outcome

Deliver only the compact groups and presenters the owner selects. Every surface is a view over the Phase-1 product, not a new measurement path.

### Package deliveries

#### `ORI-01` Shared `Stat` command registration

Register one fixed, case-insensitive `Stat` command family into both existing console composition roots. Parse to a typed group/preset request, publish clear help/autocomplete, reject unknown/unavailable groups, cap four compact groups, and make `Stat None` explicit. Keep the fixed catalog closed; adding a group remains a code review, not runtime registration.

#### `ORI-02` `Fps`

Show FPS only as a derivative of valid unscaled frame interval, with population and age inherited from the snapshot. This is the smallest useful product and may ship without `Unit`.

#### `ORI-03` `Unit`

Show milliseconds first for frame interval, host/game/editor, render CPU, waits/presentation, and GPU outer span. Pipelined columns display their own `FrameId`/age when not common-correlated. The limiting-domain hint may say `Likely`, `Mixed`, `Waiting`, or `Insufficient data`; it never declares cause from the largest number alone.

#### `ORI-04` `UnitGraph`

Render the existing joined ring. Do not allocate another history, resample away invalid frames, or align CPU/GPU clocks by assumption.

#### `ORI-05` Editor Quick Check

Replace `ViewportTopPanel::BuildPerformanceStats` as the source of truth. Implement the task-first Performance menu and compact summary from the wireframes, including Basic/Detailed, sample/loss, and invalid/stale states. The UI receives an immutable presentation model and submits semantic requests.

#### `ORI-06` DevelopmentGame presenter

Render selected compact groups through the existing runtime console/UI packet path. It shares command registration and Application truth with Editor, while retaining its own presentation composition. Shipping builds remain off by default unless a later explicit eligibility decision says otherwise.

### Positive guardrails

- Implement each selected group end to end and delete the ImGui-derived FPS path it replaces.
- Keep fixed row priority, row caps, units, tooltips, validity, and keyboard behavior aligned with the wireframes.
- Derive collection demand from the union of active groups; presentation refresh never changes source cadence.
- Measure closed, `Fps`, `Unit`, and four-group overlay cost against the Phase-0 control.

### Negative guardrails

- No command strings issued from menu code and no presenter-specific command implementation.
- No twelve-group first-level menu, saved layout system, dockable diagnostics platform, or panel per metric.
- No silent group eviction, silent Detailed promotion, or zero used for unavailable data.
- No duplicate FPS, histories, aggregators, or render callbacks left behind.

### `P2-GATE` exit

- Selected commands autocomplete and behave identically in Editor and DevelopmentGame where eligible.
- Command registration, frame/FPS meaning, history, demand, and presentation inputs each have one authority; the replaced ImGui/product and performance-callback routes are deleted where their consumers migrated.
- Selected compact UI matches the corresponding wireframe states at normal and narrow widths.
- Keyboard-only access, non-color state labels, unavailable/stale/lost states, and overflow guidance pass.
- Observer cost is within the declared hypothesis or the package is rejected/trimmed.
- Exact builds/tests and `git diff --check` pass.

### Ready-to-use implementation prompt

```text
Implement only these phase-2 orientation packages: <ORI IDs>. Begin with the test
card: exact Stat/menu action, Sponza expected state, all-map Editor/Game sweep,
observer control, and the limiting-domain questions the user must answer. Fill
the authority/duplicate-candidate ledger first; prove that multiple presenters
share one model rather than retaining presenter-local measurements or histories. Use the
accepted Application diagnostics snapshot and typed request path; do not add collectors.
Register one Stat command family through the existing Editor and runtime console
composition. Replace overlapping ImGui FPS truth when ORI-05 is selected. Follow
the compact layouts, row caps, validity, keyboard, and responsive behavior in the
wireframes. Measure presenter/collection overhead against the P0 control. Do not
add a generic registry, command-string UI, saved layouts, duplicate histories,
silent Detailed mode, or code for deferred groups. Deliver tests, the populated
all-map orientation/FrameId table, stale-path deletion, and exact validation.
Keep the package In progress until the user accepts or rejects its interpretation.
```

## Phase 3 — Deliver Reproducible Evidence

### Test it first

Use `Capture Evidence...` (or the accepted typed benchmark entry) on Sponza first. Prove the harness with `MAP-00`; after it passes, run a package-verification sweep for all 13 supported maps. This sweep validates diagnostics on those maps but does not silently advance their formal `MAP-01`–`MAP-13` content-acceptance states.

| ID | User action | Expected criteria | Required reading task |
| --- | --- | --- | --- |
| `EVD-01` | Choose a map, fixed profile/resolution/camera, then start and cancel/complete an evidence request. | Requested and active IDs match; readiness precedes warm-up; resize is absent during measurement; screenshot is rendered viewport output with `FrameId`/config sidecar; prior catalog selection is restored after success/failure. | For every map, verify settle frame, warm-up start, capture frame, warnings/fallbacks, and restoration. Reject any “responsive means ready” shortcut. |
| `EVD-02` | Complete one explicit evidence run and open its manifest, timing CSV, summary, log, and image. | File set is complete only on success; manifest/raw populations and hashes agree; ordinary startup emits nothing; interrupted output cannot look accepted. | Recompute at least one map's CPU/GPU p50/p95 from raw samples and match the summary; audit every exclusion and unavailable field. |
| `EVD-03` | Run the workload's `MAP-00` sequence on Sponza. | `MAP-A`–`MAP-H` pass for the harness, with at least 300 warm-up and 300 valid sample frames, without claiming Sponza content/performance acceptance. | State what the harness can now prove, what remains uninstrumented, and which capacity/observer hypotheses were confirmed or falsified. |
| `EVD-04` | Run at least three identical runs for each map being compared, then invoke the accepted offline comparison. | Per-run results remain primary; p50/p95/p99/worst, practical bands, uncertainty, equal-`N` rule, and `Inconclusive` are honored. | Rank all maps by comparable p95/worst behavior, identify unstable runs, and choose the highest-value discriminating experiment rather than optimizing the loudest single frame. |

Phase-3 diagnosis AC is an auditable per-map distribution table with exact configurations and no unexplained omissions. It is acceptable for a map to be slow or inconclusive; it is not acceptable for its evidence to be incomparable or silently incomplete.

### Outcome

Turn selected live facts into an explicit, workload-owned run. Nothing writes by default, and the diagnostics product does not become a benchmark-management application.

### Package deliveries

#### `EVD-01` Evidence request and readiness spine

Add one explicit typed benchmark/evidence request integrated with the current project launch and viewport capture routes. It resolves fixed startup client extent, active level identity, scene/provider generation, renderer readiness, empty preparation queues, frozen camera/settings, warm-up start, sample range, and level-named viewport sidecar. Snapshot and restore workload selection state through the existing catalog authority.

#### `EVD-02` Raw export and manifest

Serialize the Application session's bounded valid/raw records into the workload-owned run directory. Emit schema version, commit/content/config hashes, product/profile/API, adapter/driver/CPU, resolution, presentation policy, provider/fallback state, warm-up/sample frames, per-field population/exclusions, CPU/GPU summaries, RAM/GPU-memory summaries, capture path, and loss/invalid metadata. Preserve per-run identity. The exporter is a consumer, not a second store.

#### `EVD-03` `MAP-00` calibration

Run `MAP-A` through `MAP-H` on the workload-specified Sponza calibration input. This accepts the harness, not Sponza quality or a performance target. Record every unavailable fact honestly and feed capacity/observer findings back into the owning code/architecture decision.

#### `EVD-04` Repeated-run comparison

Add only the analysis needed by a selected benchmark consumer: per-run distributions first, combined view second, p50/p95/p99/worst, equal-`N` or explicit worst model, practical absolute/relative band, correlation-aware uncertainty, and `Inconclusive`. Prefer a small offline tool over widening runtime code. Raw artifacts remain primary.

### Positive guardrails

- Reuse the viewport readback for evidence images; include rendered-frame/config sidecars.
- Start warm-up only after the published readiness contract is true.
- Retain raw samples, exclusions, run identity, and exact configuration.
- Keep evidence output under the acceptance workload's `artifacts/validation/showcase-levels/...` layout.
- Treat provisional performance gates as hypotheses until accepted runs calibrate them.

### Negative guardrails

- No default JSON/CSV/report emission during ordinary Editor/Game sessions.
- No desktop screenshots, responsive-process proxy for readiness, live resize during measurement, or project cook mislabeled as an isolated map cook.
- No independently filtered percentiles used for a correlated CPU/GPU claim.
- No pooled runs that hide run identity and no p-value-only pass/fail.
- No documentation screenshot presented as benchmark proof.

### `P3-GATE` exit

- Every selected evidence package is reproducible from a clean declared configuration.
- Runtime recording, artifact serialization, workload orchestration, and offline analysis each have one named owner; no view, harness, or report keeps an alternative sample store/schema.
- Manifest and raw artifacts agree on frame range, populations, hashes, and capture identity.
- A failed/interrupted run cannot masquerade as complete and restores prior catalog state.
- Ordinary startup creates no report files.
- Selected automation/tests pass and `git diff --check` passes.

### Ready-to-use implementation prompt

```text
Implement only these phase-3 evidence packages: <EVD IDs>. Begin with the test
card and show how the user starts, cancels, opens, recomputes, and reviews a
Sponza run before the full current supported-map sweep. Fill the authority tuple
and candidate ledger first; identify every existing artifact/readback/workload
route and assign each one a disposition before adding code. Integrate with the accepted
Application session, current level/catalog/readiness paths, viewport
readback, and the MAP-00 artifact contract. Add one explicit typed request; no
default files and no second sample store. Preserve raw records, FrameId,
populations, exclusions, per-run identity, exact configuration, and failure state.
Run only the workload gates required by the selected IDs and do not claim Sponza
acceptance from MAP-00. Keep analysis offline when it does not belong in runtime.
Do not implement deferred evidence features. Give the user raw and summarized
per-map data plus the comparison questions; do not accept on generated files alone.
Report exact artifact paths, commands, results, unavailable checks, and ledger updates.
```

## Phase 4 — Add Selected Investigation Depth

### Test it first

Open the selected workspace/view on Sponza, verify selection and validity against the compact/raw product, then sweep every supported map. For each view, the user must identify a map-to-map change that supports or falsifies a performance hypothesis. If the view cannot distinguish a current question, reject/delete it.

| ID | User action | Expected criteria | Required all-map interpretation AC |
| --- | --- | --- | --- |
| `INV-00` | Open Performance, switch selected views, select frames/ranges, return to Overview. | One workspace and one synchronized selection; no second history/catalog; banner/configuration and missing data remain visible. | Prove the same selected `FrameId`/range is shown across every selected view on Sponza and the worst map. |
| `CAP-00` | Enable Detailed through `GpuPasses` or arm the later `ProfileGpu`; compare threaded recording on/off diagnostics. | Fixed records/tokens/parents, bounded truncation, deterministic merge, no completion-order hierarchy, and normal parallel recording/submission topology preserved. | Measure Detailed disturbance on Empty/Sponza/worst map and reject live detail if it changes the topology or exceeds its justified value. |
| `INV-01` | `Performance > Investigate CPU` or `Stat Threads`. | Physical thread and logical phase are distinct; work/wait and nested values are not presented as additive totals. | Name the top CPU owner/phase and wait per map; cluster CPU-likely maps and identify the counterexample. |
| `INV-02` | `Performance > Investigate GPU` or `Stat Gpu`. | Separate queue spans/dependencies, explicit clock/calibration state, valid top contributors, no utilization-to-ms conversion. | Rank graphics/compute/copy behavior and GPU p95; explain whether queue overlap or missing calibration limits each conclusion. |
| `INV-03` | `Stat GpuPasses` and select the same frame in GPU Live. | Pass rows share one `FrameId`; hierarchy/flat values obey inclusive/exclusive rules; pass sum is never called GPU frame time. | Record top passes and unattributed outer span per map; compare Sponza/Bistro/Modern Sponza families and select a pass for capture. |
| `INV-04` | `Performance > Investigate Memory` or `Stat Memory`. | Working/private/tracked/allocated/local/non-local/budget/retirement and their high-water/age are distinct. | Rank memory growth/pressure; identify which maps scale textures, geometry, transient use, or retirement and which fact is unavailable. |
| `INV-05` | Enable `Stat Render` and/or `Stat Rhi`. | Counts come from production owners, are bounded and neutral, and do not trigger diagnostic rescans. | Correlate draw/dispatch/barrier/submission/upload/descriptor changes with timing changes; do not claim causality from count alone. |
| `INV-06` | `Stat Tasks` and CPU Tasks view. | Bounded lane/family aggregates match real task lanes; deep dependency/call-stack detail remains ETW/WPA. | Identify lane imbalance/starvation candidates per map and choose one ETW trace when task evidence is material. |
| `INV-07` | `Stat Scene`. | Extracted/accepted/visible/submitted/rejected counts originate at owners and retain generation/frame identity. | Test whether the suspected cost scales with instances, triangles, materials, lights, RT structures, or none; record the map that falsifies the simplest model. |
| `INV-08` | `Stat Hitches`; navigate each retained hitch to selected frame/export/capture. | Last 16 qualifying frames are bounded and keep budget/domain/validity; no automatic file/native capture. | Classify hitch frequency and worst frames per map, then select the most repeatable hitch for Phase-5 capture or a controlled experiment. |

The view passes when its data changes the investigation decision on at least one supported workload or supplies a useful negative result. “It displays numbers” is not acceptance evidence.

### Outcome

Add only views that answer a current measured question. `INV-00` is implemented once if any workspace feature is selected; if every investigation feature is deferred, it is not created.

### `INV-00` workspace spine

Implement the one fixed Performance workspace from the wireframes with Overview plus only selected views, one shared frame/range/object selection, synchronized navigation, configuration/validity banner, and contextual evidence/capture actions. It consumes the same immutable model and typed request path as compact stats. It is not a general dock/plugin host.

### `CAP-00` detailed GPU record spine

Implement this only when `INV-03` or the later `CAP-01` is selected. Replace the current dynamic completion stream with a preplanned fixed token/query layout per frame-graph recording chunk. Recording tasks write disjoint ranges; the owner merges deterministically after retirement. Records carry `FrameId`, token, explicit parent, source kind, queue, submission identity, begin/end ticks, counts, validity, and bounded label lookup. Ordinary exhaustion reports truncation/loss. Restore normal parallel recording and prove its topology before accepting the spine.

### Feature packages

| ID | Why it is useful | Production route and acceptance focus |
| --- | --- | --- |
| `INV-01` | Separates physical owner-thread wall time, logical phases, work, and waits. | Use fixed Application/Renderer phases and real thread roles. Prove no invented Editor GameThread and no additive nested totals. |
| `INV-02` | Shows graphics/compute/copy outer spans, dependency edges, overlap uncertainty, and top contributors. | Use correlated Renderer/RHI results. Keep queue clocks separate unless a capability-tested calibration exists. |
| `INV-03` | Answers which stable passes contribute to a live GPU frame. | Reuse `CAP-00` tokens/records at visible Detailed cost. Show bounded truncation and never call a pass sum GPU frame time. |
| `INV-04` | Distinguishes process RAM, allocator blocks, heap segment/budget, transient use, and retirement. | Use `FND-04/05`; retain sample cadence/age and independent high-water semantics. No allocation call stacks. |
| `INV-05` | Relates Renderer workload and RHI submission/allocator facts to timing shifts. | Admit counters only where existing production owners increment them. No diagnostic scene/pass/descriptor rescan and no native types. |
| `INV-06` | Reveals task-lane balance or starvation without recreating WPA. | Add bounded lane/family aggregates only if TaskExecutor owns them cheaply; keep dependency/call-stack detail in `SparkleTasks` ETW. |
| `INV-07` | Tests whether scene cardinality tracks a measured bottleneck. | Add extracted/accepted/visible/submitted/rejected counts at existing owners. Reject the package if it requires UI queries or a diagnostic traversal. |
| `INV-08` | Makes tail frames navigable and hands a specific `FrameId` to capture/export. | Retain the last 16 qualifying frames and owned annotations. Never auto-write or auto-launch a native capture. |

### Positive guardrails

- Select a feature because an accepted run has a falsifiable question it can distinguish.
- Reuse Overview, banner, selection, graphs, and tables rather than creating a window per feature.
- Keep aggregation definitions and `FrameId` visible near any cross-domain comparison.
- Coalesce only in presentation; raw fixed tokens/results remain auditable.
- Delete a counter/view when its measured value does not justify production and observer cost.

### Negative guardrails

- No trace viewer, flame graph, arbitrary query language, hardware-counter suite, allocation explorer, or task browser.
- No per-entity/resource/draw rows or dynamic timing labels in the live path.
- No UI-owned sorting/filtering that changes totals or sample population.
- No view implemented before its authoritative producer and bounded consumer.
- No Tier-C row added merely because a wireframe has space.

### `P4-GATE` exit

- Each selected view answers its declared question on a real workload and has a “not useful/reject” outcome available.
- Every view reads the one joined history and synchronized selection; every detailed GPU token, record, and marker comes from the one refactored production path, with superseded timing/storage/topology paths deleted.
- Fixed bounds, overflow UI, validity, selection synchronization, and observer cost pass.
- `CAP-00` preserves command-recording/submission topology when selected.
- Compact and workspace surfaces agree because they share the same model.
- External-only detail remains external; replaced callbacks/panels are removed.
- Exact builds/tests and `git diff --check` pass.

### Ready-to-use implementation prompt

```text
Implement only these phase-4 investigation packages: <INV IDs and CAP-00 if
required>. Fill the test card and state the measured question, Sponza expected
result, full supported-map sweep, falsification condition, and user reading AC for
each before editing. Fill and close the authority/duplicate-candidate ledger for
every selected view, counter, GPU token/record, and current diagnostic scope path.
If any selected
view needs INV-00, implement one fixed workspace and one shared selection over the
existing immutable Application model. Add counters only at current production
owners and detailed GPU rows only through accepted CAP-00 records. Match the
wireframes and architecture semantics. Reject/delete a feature if it requires
diagnostic rescans, unbounded history, UI access to live state, dynamic hot-path
labels, changed recording topology, or unjustified observer cost. Do not create a
general diagnostics platform or code for deferred views. Give the user the owner/
pass/wait/memory ranking and counterexample-map task. Report usefulness, bounds,
costs, tests, evidence, decision, and exact validation.
```

## Phase 5 — Add Selected Focused And External Captures

### Test it first

Take the first capture on Sponza, correlate it to the selected `FrameId`/stable markers, then repeat the selected capture path on every supported map. Captures are explicit and may be retained only for the package verification/evidence action. For large native artifacts, the sweep summary may link to external storage rather than committing them.

| ID | User action | Expected criteria | Required reading task |
| --- | --- | --- | --- |
| `CAP-01` | Select a Sponza frame and invoke `ProfileGpu`; repeat one frozen capture per map. | Exactly one `Idle -> Armed -> Submitted -> Resolving -> Frozen` result, correct viewport/frame, valid parent/token tree, inclusive/exclusive interval math, bounded failure/cancel/replace, no default file. | For each map, name top inclusive, top exclusive-uncovered, and unattributed regions; confirm or reject the `INV-03` suspected pass. |
| `EXT-00` | Launch with no provider, each selected provider alone, every selected pair, and the selected combined flag set; exercise each visible viewport icon and click another while one capture is active. | Per-provider capability/state is independent, only requested/detected capable icons appear, clicked provider/viewport identity is preserved, conflicts are explicit, global `Busy` arbitration works, and no native handle/provider API reaches Editor. | Review the provider/backend/version/combination matrix and decide which exact combinations are supported, unavailable, experimental, or rejected before accepting any adapter. |
| `EXT-01` | Launch D3D12 with `-Pix`, click the PIX mini icon for the desired viewport. | Icon appears only when requested/detected and capable; next valid target frame is captured; artifact opens/handoffs; absent PIX is honest and nonfatal where allowed. | Match Sparkle `FrameId`/markers to PIX events and state what PIX adds beyond built-in data for each map/signature cluster. |
| `EXT-02` | Launch D3D12 with `-RenderDoc`, click its icon. | Correct D3D12 device/swapchain/viewport, next-frame artifact, marker tree and API validation, clean absent-tool/failure behavior. | Inspect top suspected pass resources/pipeline state on each map; record capture-confirmed or falsified hypotheses. |
| `EXT-03` | Launch Vulkan with `-RenderDoc`, click its icon. | Correct Vulkan layer/bootstrap order and swapchain target, validation-clean capture, same neutral state semantics as D3D12. | Compare marker/pass/resource-state behavior with D3D12 for every map and explain material backend differences without vendor mythology. |
| `EXT-04` | Build eligible experimental D3D12 Nsight support, launch with `-Nsight`, click its icon. | Current SDK/tool matrix passes; experimental state is visible; ordinary packages remain independent; capture/failure/timeout/shutdown complete once. | Use counters/shader/hardware analysis only where the selected map question needs it; record architecture/hardware scope and alternatives. |
| `EXT-05` | Repeat `EXT-04` on Vulkan when the current matrix supports it. | Vulkan extension/layer/device path and provider compatibility pass; otherwise explicit rejection, not emulation. | Compare the same suspected mechanism across APIs or record why the matrix leaves it unresolved. |

For any selected provider combination, also launch the exact combined flags, confirm all capable icons coexist, click each provider in turn, and verify the non-active icons report global `Busy` rather than capturing simultaneously. A package is not accepted from icon visibility alone; at least one usable artifact per supported map and selected backend is required unless the provider explicitly reports a supported, tested unavailability.

### Outcome

Deliver bounded one-frame investigation paths. Internal `ProfileGpu` and native external providers share frame identity and stable markers, but remain separate capture mechanisms.

### `CAP-01` `ProfileGpu`

Implement one typed one-shot state machine over accepted `CAP-00`: `Idle -> Armed -> Submitted -> Resolving -> Frozen` with explicit failure/cancel/replace. Capture the next valid selected viewport frame, retain one immutable result, derive inclusive and `exclusive (uncovered)` by direct-child interval union on the same queue, and render hierarchical/flat/coalesced views from the wireframes. Do not write a file unless an explicit evidence/export action requests it.

### `EXT-00` external-provider spine

Implement this once only if an `EXT-01`–`EXT-05` adapter is selected.

- Parse the combinable immutable launch intent before Renderer/device creation.
- Extend `RendererExternalRuntime` and `RendererBackendConfiguration`; keep native provider bootstrap in RHI-private backend adapters.
- Publish a fixed provider set with independent `NotRequested`, `Unavailable`, `Ready`, `Armed`, `Capturing`, `Completed`, `Failed`, and `Busy` state plus bounded reason/artifact metadata.
- Render one far-right mini icon per requested or detected capable provider. Multiple icons may coexist in every renderable viewport.
- Each click sends a typed request naming provider, viewport selection token/generation, and expected frame identity; no native window/device/command-list handle reaches Editor.
- Serialize capture execution globally at first. Other capable icons remain visible and report `Busy`; icon coexistence never implies simultaneous capture safety.
- Resolve a checked compatibility matrix before loading more than one capture layer. Never use hidden provider precedence.
- Arm the named provider for the next valid frame of the named viewport and trigger at the backend's safe presentation boundary. Timeout, resize, device loss, viewport destruction, and shutdown complete or cancel exactly once.
- Keep profiler-native artifacts native. Sparkle stores only bounded status/provenance and an explicit handoff path.

### Provider feature packages

| ID | Delivery | Required proof before `Accepted` |
| --- | --- | --- |
| `EXT-01` | PIX capture, D3D12 | Profile-build startup, PIX capture capability distinct from marker-only runtime, selected viewport/next-frame correctness, artifact handoff, normal run without PIX installed, shutdown/failure, and runbook operation. |
| `EXT-02` | RenderDoc capture, D3D12 | Pre-device supported integration, correct present target, API validation, one artifact, normal run without RenderDoc, and PIX coexistence outcome when both are requested. |
| `EXT-03` | RenderDoc capture, Vulkan | Vulkan layer/bootstrap ordering, correct swapchain/present target, validation-clean smoke, artifact, and D3D12-semantic state parity. |
| `EXT-04` | Nsight Graphics, D3D12, experimental | Current runbook/SDK matrix revalidated, explicit experimental build eligibility, capture success/failure semantics, pairwise compatibility, and absence from ordinary packages when unsupported. |
| `EXT-05` | Nsight Graphics, Vulkan, experimental | Same as `EXT-04` plus Vulkan layer/extension and validation evidence. Reject rather than emulate if the installed/current SDK path is unsupported. |

### Positive guardrails

- Revalidate current primary provider documentation and installed versions through the runbook immediately before implementation.
- Keep markers stable across internal and external capture so one `FrameId`/token can be correlated.
- Treat each provider adapter as optional backend-private code with a narrow neutral service.
- Test one provider, pairwise requested sets, all requested providers where eligible, and normal launch without tool installations.
- Report optimized profiling-build configuration and the exact ways capture mode changes execution.

### Negative guardrails

- No late injection after device creation, vendor APIs above RHI private, static RenderDoc linkage, or mandatory beta SDK dependency.
- No permanent unrequested icons and no icon presented as `Ready` from launch intent alone.
- No simultaneous native captures until a tested compatibility entry explicitly permits the exact pair/backend/version.
- No capture of whichever window presents first, stale viewport generation, silent fallback provider, or blocking UI/RenderThread wait.
- No reuse of screenshot `RhiCaptureService` as the external profiler owner and no embedded replacement for the native profiler.

### `P5-GATE` exit

- Every accepted provider passes its supported backend/product/build matrix and an absent-tool launch.
- Launch parsing, provider capability, request/arbitration, marker identity, and presentation state each have one neutral authority; screenshot readback and internal/native capture separations have explicit passing boundary tests.
- Multiple provider icons render independently, request the clicked provider, and expose global Busy/arbitration honestly.
- Compatibility, timeout, resize, device-loss, viewport-loss, failure, artifact, and shutdown paths are bounded and exactly-once.
- Native validation, `architecture_boundary_check`, relevant tests/builds, observer evidence, and `git diff --check` pass.

### Ready-to-use implementation prompt

```text
Implement only these phase-5 capture packages: <CAP-01/EXT IDs>. Begin with the
test card: exact launch/action, Sponza capture criteria, one capture per supported
map/backend, marker/token correlation, compatibility cases, and the hypotheses the
user must confirm or falsify. Fill the authority/duplicate-candidate ledger for
launch parsing, pre-device bootstrap, capability, request/arbitration, provider
state, marker tokens, and both existing screenshot/internal capture paths.
Revalidate the current profiler runbook and primary
provider documentation first. Build CAP-01
only over accepted CAP-00 records that preserve parallel recording. For external
capture, extend RendererExternalRuntime's pre-device configuration and backend-
private RHI adapters; publish only neutral capability/state and typed provider+
viewport requests. Support multiple visible provider icons but globally serialize
capture unless the exact compatibility matrix proves coexistence. Do not expose
vendor handles, inject late, block the UI, overload screenshot readback,
auto-capture, silently choose a provider, or retain code for deferred/failed
adapters. Run the exact backend/product/provider and all-map matrix, boundary
check, native validation, and failure/shutdown tests. Hand the user capture links
and the correlation task; do not accept icon visibility alone.
```

## Phase 6 — Harden And Close The Selected Product

### Test it first

Run the final accepted set exactly as a user will use it. Perform the complete 13-map sweep in `DevelopmentGame` on D3D12 and Vulkan, plus the full Editor UI sweep where selected. Quantitative conclusions require at least three identical 300-warm-up/300-valid-sample runs per map. Repeat the `Off` control for Empty, Sponza, and every map used for a headline diagnosis so observer cost is part of the conclusion.

For each supported map, produce one diagnosis card inside the final generated `sweep-summary.md`:

```text
Level / run IDs / exact configuration:
CPU frame p50/p95/p99/worst and top owner/wait:
GPU p50/p95/p99/worst, top pass, queue/overlap state, unattributed span:
Working/private RAM and GPU used/allocated/local/non-local/high-water:
Hitch count and selected worst FrameIds:
Likely limiting domain and competing hypotheses:
Internal/native capture links and matching markers/tokens:
Controlled experiment, predicted result, observed result, quality/correctness check:
Confidence: Observed | Correlated | Capture-confirmed | Causal | Unresolved
Next action or accepted scoped conclusion:
```

Final user AC:

1. review every diagnosis card rather than only the worst map;
2. group maps by performance signature and justify every membership from comparable data;
3. identify the top three engine problems by impact across maps, not just absolute cost in one scene;
4. select one representative map per signature cluster for a capture-backed causal experiment;
5. verify the proposed change improves the predicted field and does not regress correctness, image quality, another backend, or a counterexample map;
6. mark remaining unknowns `Unresolved` with the missing package/tool/capability instead of calling them understood.

The selected implementation can ship with honest unresolved cards. The stronger claim that current map performance is fully understood passes only when every material signature cluster reaches `Capture-confirmed` and each top-priority problem reaches `Causal` or has an explicit owner-approved limitation.

| ID | Final verification action | Expected criteria and user AC |
| --- | --- | --- |
| `FIN-01` | Run the declared product/backend/build/serial-threaded/mode/provider matrix across the full current map roster. | Every applicable cell has evidence, every excluded cell has a reason, and the user can compare only genuinely equivalent cohorts. |
| `FIN-02` | Inject minimized/invalid window, resize, stale generation, overflow, late/drop, viewport loss, provider timeout/failure, device removal, interrupted export, and shutdown. | Every request succeeds, fails, cancels, or is superseded exactly once; every map remains recoverable or reports the scoped fatal boundary; the user reviews the failure table. |
| `FIN-03` | Repeat `Off`, Basic, maximum selected composition, Detailed, Benchmark, internal capture, and selected external-provider controls where applicable. | Basic meets fixed hypotheses; other disturbance is measured; topology changes are explicit; the user decides to retain, capture-gate, trim, or reject every costly collector. |
| `FIN-04` | Follow the docs as a fresh user on Sponza and the current worst map, then audit source/CMake/public headers/docs/artifacts and close the authority/candidate ledger. | Zero unresolved candidates, competing authorities, unjustified duplicate implementations, replaced paths, dead flags, unused public types, empty surfaces, broken links, or unowned packages remain; every deliberate separation has boundary proof and the final package/diagnosis ledgers match evidence. |

### Outcome

Finish every selected path end to end, delete superseded or unjustified code, and leave an explicit final shipment set. This phase does not force deferred features into the engine.

### `FIN-01` Product/backend/build matrix

For every accepted feature, test only applicable cells but make omissions explicit:

```text
                       D3D12                 Vulkan
DevelopmentEditor      selected surfaces     selected surfaces
DevelopmentGame        selected surfaces     selected surfaces
Profile build          evidence/captures     evidence/captures
Shipping*              absent/off/allowed    absent/off/allowed

* according to the explicit eligibility decision, never by accidental linkage
```

Include serial/threaded rendering, declared pipeline depths, VSync/presentation policy, normal and narrow UI, provider absent/present, and Basic/Detailed transitions where selected.

### `FIN-02` Failure and lifetime audit

Exercise minimized/invalid window, resize, stale `FrameId`, generation change, ring/query exhaustion, late GPU result, dropped Renderer result, viewport destruction, provider failure/timeout, device removal, interrupted export, and shutdown with work in flight. Every request completes, cancels, or is superseded once; every consumer can distinguish the outcome.

### `FIN-03` Observer cost and capacity calibration

Measure diagnostics off, Basic, each selected view/group, maximum accepted composition, Detailed, internal capture, and each external provider on the declared workload. Compare serial and threaded controls. Record CPU wall, GPU effect, allocations, waits, query/record loss, memory footprint, file size, and topology changes. Tighten capacities or reject features that miss their declared value/cost gate; do not normalize the overhead away.

### `FIN-04` Deletion, docs, and shipment ledger

- Remove replaced FPS/provider callbacks, dead CVars, experimental scaffolding, unused public types, unused icon assets, compatibility shims, and prototype dependencies.
- Repeat the repository-wide semantic overlap search across types, functions, commands, CVars, launch flags, marker tokens, capture states, histories, schema keys, serializers, tests, CMake targets, and dependency edges. Close every row with one allowed disposition and evidence.
- For each accepted concept, show one canonical owner, one mutable state, one production/request path, one history/schema where applicable, and all immutable consumers. A second presenter or backend adapter is acceptable only through its recorded boundary test.
- Search for every removed symbol and its semantic aliases. Production references, registrations, includes, build membership, compatibility aliases, deprecated commands, and copied documentation must be zero; test fixtures or migration notes that mention an old name must clearly be non-production.
- Ensure every accepted command/menu/icon/view has help, validity/failure behavior, test/evidence, and an owner.
- Ensure every deferred/rejected package has no shipped stub and a concise reason in the ledger.
- Update architecture only for accepted semantic decisions, wireframes only for shipped UX changes, runbook only for revalidated operations, and workload ledgers only for actual evidence.
- Do not create a completion report document. The change handoff, tests, artifacts, and this final ledger are sufficient.

Complete this matrix in the final change description; do not create another repository document for it:

```text
Concept | canonical owner/state | production + request route | readers
        | candidate dispositions | deletions | retained-boundary test
        | final rg/build/behavior proof
```

The audit must cover at least frame identity/time/FPS, collection demand, joined history, each metric producer, console/menu requests, GPU tokens/timing/markers, process/GPU memory, task detail, workspace selection, artifact schema/export, internal capture, screenshot readback, external launch/capability/arbitration/provider state, public APIs, CMake dependencies, and owning documentation. A fresh reviewer must be able to navigate from [Docs](../../../README.md) to this architecture and then find exactly one production owner for each accepted concept.

### Positive guardrails

- Prefer deleting a weak feature to weakening the product's ownership or cost model.
- Review public headers and CMake links as a deliberate compatibility surface.
- Validate a clean configured build where feasible, not only an incremental developer tree.
- Preserve raw evidence and state unavailable checks explicitly.
- Run `architecture_boundary_check` for every Renderer/RHI boundary change and `git diff --check` for every handoff.

### Negative guardrails

- No “phase complete” with untested cells silently omitted or failures relabeled as unsupported.
- No vestigial feature flags, empty panels, dead provider abstractions, or duplicate documentation.
- No two-way state synchronization, fallback-to-old route, shadow collector, presenter-local history, alternative export schema, or generic facade that hides two competing implementations.
- No performance or compatibility claim beyond the exact hardware, driver, backend, build, tool, and workload tested.
- No acceptance based only on compilation, screenshots, or one successful frame.

### Completion gate

The performance diagnostics delivery is complete for the chosen scope when:

1. every package is `Accepted`, `Deferred`, or `Rejected` and all accepted dependencies are accepted;
2. every accepted feature is a complete vertical slice with one authority, bounded cost/lifetime, real consumer, failure behavior, tests, and applicable evidence;
3. the final authority/candidate ledger has zero unresolved rows and proves zero competing semantic authorities or unjustified duplicate implementations; every retained separation has a named data/lifetime boundary and passing test;
4. every accepted package has passed its Sponza shakedown and current runtime-supported catalog sweep, with no map silently omitted;
5. the user has completed or explicitly delegated each package's reading task and the decision cites the populated sweep evidence;
6. all superseded and failed experimental paths, APIs, build links, flags, tests, aliases, and copied documentation are removed;
7. the selected product/backend/build matrix and observer-cost suite pass or record a scoped, honest limitation;
8. `MAP-00`/`WL-04`/case-study claims are made only if their own required packages and workload gates passed;
9. documentation links to evidence instead of duplicating it, and no extra planning/status files are needed;
10. exact verification commands/results and unavailable checks are in the final handoff.

Closing with deferred packages means the selected product is finished, not that every target proposal was implemented. A later decision to promote a deferred package reopens its phase dependency audit; it does not invalidate the accepted product.

### Ready-to-use implementation prompt

```text
Execute Performance Diagnostics phase 6 for the currently Accepted/Selected
ledger only. Begin with the final test card and enumerate all 13 current diagnosis
cards, three-run quantitative matrix, capture/experiment requirements, observer
controls, and the user's final reading AC. Do not promote deferred features. Run the applicable product,
backend, build, serial/threaded, collection-mode, provider, failure, shutdown,
capacity, and observer-cost matrix. Reconcile public headers, CMake links, tests,
architecture boundaries, and documentation with actual behavior. Repeat the full
authority/duplicate-candidate audit across semantic aliases, runtime routes,
commands, flags, histories, schemas, capture state, public APIs, and dependencies;
do not finish with an unresolved row. Remove every
superseded path, unused public type, failed prototype, dead flag, empty surface,
and unneeded dependency. Reject rather than retain a feature whose value does not
justify cost or complexity. Update only owning docs and actual evidence ledgers;
do not add status/completion documents. Give the user every diagnosis card and
signature-cluster task before claiming completion. Report exact commands/results,
artifacts, limitations, deletions, decisions, and the final package ledger.
```

## Per-Package Change Template

Use this inside the implementation change or pull request; do not create one Markdown file per package.

```text
Package: <ID and name>
State: Selected -> In progress -> Accepted | Deferred | Rejected
User question/value:
User action after delivery:
Hard dependencies and their evidence:
Current owner and path being extended:
Producer / consumer / lifetime / thread:
Metric or request semantics:
Fixed bounds and overflow behavior:
Authority tuple:
Overlap search terms, paths, commands, flags, schemas, and dependencies:
Duplicate candidates and consumers:
Disposition for every candidate:
Retained-separation boundary and test, if any:
Exact obsolete symbols/files/tests/CMake links/docs removed:
Proof that only the canonical production path remains:
Add / modify / replace / delete ledger:
Positive guardrails:
Negative guardrails:
Tests and exact commands:
Sponza shakedown and expected result:
Current catalog roster and all-map sweep result:
Product/backend/build evidence:
Observer cost versus control:
Unavailable checks or scoped limitations:
All-map findings and signature clusters:
User interpretation task and decision:
Decision and artifact links:
```

## Verification Command Baseline

Use the configured build directory and exact target/configuration selected by the change. Do not claim commands that were not run.

Record the package-specific overlap searches alongside these commands. Search both old symbol names and semantic aliases across implementation, public headers, tests, build files, and docs; a single preferred call site is not proof that the old route is gone.

```powershell
rg -n '<old-symbol>|<semantic-alias>|<old-command-or-flag>|<old-schema-key>' Engine Tools Projects Docs
cmake --build <build-dir> --config <DevelopmentEditor|DevelopmentGame> --target <smallest-touched-target>
ctest --test-dir <build-dir> -C <configuration> --output-on-failure -R <smallest-relevant-regex>
cmake --build <build-dir> --config <configuration> --target architecture_boundary_check
git diff --check
```

Add D3D12/Vulkan native validation, provider smoke commands, workload automation, formatting, and clean-build checks required by the selected standards and package. If the current generator or environment cannot run a check, report it as unavailable with the reason; do not translate that into a pass.

## Reference Map

| Implementation question | Owning reference |
| --- | --- |
| How is duplicate authority prevented and proven absent? | [Zero-Duplicate-Authority Contract](#zero-duplicate-authority-contract), [Integration Style Guide](../../../Engineering/Standards/IntegrationStyleGuide.md), and [Change Process](../../../Engineering/Standards/ChangeProcess.md) |
| What does a metric mean and who owns it? | [Measurement Vocabulary](PerformanceDiagnosticsArchitecture.md#measurement-vocabulary), [Owners](PerformanceDiagnosticsArchitecture.md#owners), and [Publication Rules](PerformanceDiagnosticsArchitecture.md#publication-rules) |
| What is the bounded data/cost model? | [Collection Modes And Cost Budget](PerformanceDiagnosticsArchitecture.md#collection-modes-and-cost-budget), [Bounded Data Model](PerformanceDiagnosticsArchitecture.md#bounded-data-model), and [Demand, Cost, And Composition](PerformanceDiagnosticsArchitecture.md#demand-cost-and-composition) |
| Which stat groups are candidates and what does Tier A/B/C mean? | [Fixed Group Catalog](PerformanceDiagnosticsArchitecture.md#fixed-group-catalog) and [Delivery Tiers](PerformanceDiagnosticsArchitecture.md#delivery-tiers) |
| How should the viewport, workspace, capture, and failure states look? | [Performance Diagnostics Visual Design And Tool Wireframes](PerformanceDiagnosticsAsciiWireframes.md) |
| Why this product depth and not a general profiler? | [Selected Direction And Canonical Handoff](DiagnosticsUxResearch.md#selected-direction-and-canonical-handoff) and [Low-Clutter Integration Principles](DiagnosticsUxResearch.md#low-clutter-integration-principles) |
| How do internal GPU capture records work? | [On-Demand GPU Visualizer](PerformanceDiagnosticsArchitecture.md#on-demand-gpu-visualizer) |
| How do viewport provider icons and requests work? | [Attached External Frame Capture](PerformanceDiagnosticsArchitecture.md#attached-external-frame-capture) and [Attached Profiler Capture Icons](PerformanceDiagnosticsAsciiWireframes.md#attached-profiler-capture-icons) |
| Which provider/tool is currently supported and how is it operated? | [External Performance Profiler Runbook](DiagnosticsProfilerRunbook.md) |
| What proves `MAP-00` and measured-frame readiness? | [Incremental Per-Level Verification Program](../../../Engineering/BistroAndSanMiguelWorkloads.md#incremental-per-level-verification-program), [Performance Contract](../../../Engineering/BistroAndSanMiguelWorkloads.md#performance-contract), and [Workload Gate Sequence](../../../Engineering/BistroAndSanMiguelWorkloads.md#workload-gate-sequence) |
| What is required for implementation and evidence quality? | [Engineering Standards Map](../../../Engineering/Standards/README.md#standards-map) |

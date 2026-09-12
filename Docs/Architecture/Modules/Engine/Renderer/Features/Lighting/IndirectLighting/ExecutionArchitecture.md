# Indirect Lighting And ReSTIR GI Execution Architecture

**Status:** target architecture pending `IND-D0`; current route is source-present but its estimator conformance is unproved

**Responsibility:** own the target feature enclosure, path/reservoir records, pass/resource graph, history, cross-feature boundaries, lifetime, failure, and integration hooks

**Authority boundary:** [Transport And Estimator](TransportAndEstimator.md) owns math; [Discovery](Discovery.md) freezes choices; [Plan](Plan.md) orders changes; code/build and `FCR-REN-07` own implementation/evidence

**Priority:** initial transport truth → explicit path/shift → GRIS reuse → temporal safety → reconstruction → admitted multi-bounce optimization → evidence

## Product Claim

One Lit View produces finite scene-linear `IndirectDiffuse` and `IndirectSpecular` over an explicit path domain. Initial samples and resampled paths agree statistically with independent references; motion and scene mutation cannot reuse invalid transport; unsupported paths and providers are visible; sparse results reconstruct within frozen quality/time/memory budgets.

## Current Versus Target

| Concern | Current | Target |
| --- | --- | --- |
| sample | pixel/sample/frame RNG identity packed through float | compact auditable path record with integer IDs and exact proposal/mapping state |
| reuse | rerun source seed at destination; normal/depth compatibility | ratified reconnection/replay shift, inverse/Jacobian/support, GRIS contribution weights |
| path accounting | shared path helper with unproved per-bounce contribution split | one frozen NEE/BSDF/emission/environment/MIS contract |
| traversal | Inline resolve | explicit supported provider/profile; shared ray semantics and visible exclusions |
| history | reservoir plus broad invalidation | classified path/light/sky/material/geometry/shader/provider generations per View |
| reconstruction | optional adjacent RR guides, no mandatory baseline | portable raw-first diffuse/specular baseline plus optional RR |
| evidence | source inventory | analytic/statistical/convergence/temporal/backend/workload evidence |

## Feature Enclosure

Proposed steady-state homes:

```text
Engine/Renderer/Private/Passes/Lighting/IndirectLighting/
Engine/Assets/Shaders/Passes/Lighting/IndirectLighting/
```

The capsule owns the initial indirect candidate, path sample/packing, shift mappings, GRIS reservoir, temporal/spatial reuse, feature history, confidence, lobe outputs, and feature-local status. It does not own authored materials/lights/sky, RenderScene/GPU-scene/TLAS, generic ray traversal, shader runtime, frame graph, Direct Lighting reservoirs, Reference Path Tracer sessions, volumetric transport, shared composite, exposure, or presentation.

Shared light/BRDF/ray semantics are called through their existing owners when independence permits. Acceptance records every shared-code dependency; no copy is made merely to appear independent.

## End-To-End Route

```text
immutable View + prepared surface + scene/material/light/environment generations
  -> GenerateIndirectPaths
       BSDF/lobe sample to secondary hit
       NEE/emission/environment technique accounting
  -> ResampleIndirectTemporal
       current-scene path shift + GRIS
  -> ResampleIndirectSpatial
       neighbor selection + path shift + GRIS
  -> EvaluateSelectedIndirectPath
       final visibility/material/light evaluation
  -> raw IndirectDiffuse / IndirectSpecular + hit distance/confidence
  -> ReconstructIndirectLighting (optional provider)
  -> existing LightingComposite
```

The unfused route is the validation shape. Optimization can replay suffixes, fuse passes, or terminate through an admitted cache only after exact results and intermediate states remain testable.

## Core Records

| Record | Semantic contents | Ownership/lifetime |
| --- | --- | --- |
| `IndirectLightingProfile` | path domain/depth, estimator/shift/reuse/reconstruction quality | immutable resolved View value; no raw CVar forwarding |
| `EnvironmentView` | mapping/rotation/radiance/PDF plus current/previous generation | borrowed frame-local view from Sky/environment owner |
| `IndirectPathSample` | source receiver, mapping anchors, directions/lobes, terminal sample, technique/probability, depth/termination | feature GPU value; compact only after round-trip proof |
| `IndirectReservoir` | selected path, generalized contribution weight/target/resampling sum/effective count/validity | per-pixel feature value/history |
| `IndirectHistoryKey` | View/extent/surface/geometry/material/light/environment/estimator/random/shader/traversal generations | feature-local immutable value |
| `IndirectLightingProducts` | raw/reconstructed lobe handles, hit distance, confidence | frame-graph handle aggregate |
| `IndirectLightingStatus` | requested/active domain/provider and finite reason | bounded per-View observation |

No record owns a duplicate scene, material table, environment texture, TLAS, or native backend handle. A path sample retains logical/generation identity; resource lifetime stays with its owner and GPU submission leases.

## Pass And Resource Plan

| Pass | Reads | Writes | Notes |
| --- | --- | --- | --- |
| `GenerateIndirectPaths` | GBuffer, material/geometry/light/environment, TLAS, profile | current path/reservoir | no history; supports analytic Stage-1 domain |
| `ResampleIndirectTemporal` | current reservoir, previous reservoir/surfaces/generations, motion | working reservoir/confidence | shifts in current scene or rejects |
| `ResampleIndirectSpatial` | working/current surfaces, neighbor distribution | final reservoir/confidence | decorrelated neighbors and exact selection probability |
| `EvaluateIndirectPaths` | final reservoir plus current scene generations | raw diffuse/specular, hit distance, counters | final expensive/visibility evaluation per frozen estimator |
| `ReconstructIndirectLighting` | raw lobes/guides/confidence/history | reconstructed lobes/current history | optional provider, separate state |

The Stage-1 baseline may evaluate an initial path directly without allocating persistent reservoirs. Stage 2 introduces the new path/reservoir layout and deletes the old seed payload; no dual schema remains.

### Resource Budget

`IND-D0-12` freezes exact formats and sizes. The budget includes:

- explicit/packed path vertex and terminal sample state;
- two or more reservoir generations required by the selected pass layout;
- previous surface/motion data not already owned by the GBuffer route;
- raw lobe and hit-distance/confidence products;
- reconstruction history and replacement overlap;
- light/environment distributions borrowed from Direct/Sky owners;
- transient ray queues only if the selected implementation actually uses them.

Fusing state into `float4` for convenience is rejected when it rounds identity or mixes lifetimes. Keeping full path vertices is rejected only after measured compact replay preserves semantics.

## History And Publication

One View owns one reservoir generation and one separate reconstruction generation. A frame reads immutable completed prior state and writes a distinct current state. Successful graph completion promotes current history; rejection, cancellation, device loss, shader failure, or invariant failure leaves prior state unchanged or explicitly resets according to the error contract.

History resources and the referenced scene/light/material/environment/shader generations remain alive until consuming submissions complete. Resize/profile/schema replacement allocates complete new state before activation and budgets coexistence. No history crosses viewport/View identity.

## Invalidation And Motion

| Change | Reservoir/path response | Reconstruction response |
| --- | --- | --- |
| camera cut, invalid motion, extent/jitter domain | reset | reset |
| rigid/deformed primary or secondary geometry | current-scene shift only if motion/generation contract supports it; otherwise reject | reset/reduce confidence |
| material/normal/roughness/lobe change | re-evaluate only inside mapping support; otherwise reject/reset | reset affected guide identity |
| light/environment change | translated/re-evaluated under frozen terminal technique or reject | confidence/reset according to raw change |
| TLAS/hit/material provider change | reset unless exact semantic compatibility proved | reset |
| estimator/path-record/random-layout/shader change | reset | reset |
| reconstruction-only setting/provider change | no reservoir reset | reset reconstruction only |

Disocclusion is not “missing history filled with neighbors” by default. It is a classified absence of a temporal proposal, followed by the admitted fresh/spatial/splat strategy and lower confidence.

## Cross-Feature Boundaries

### Direct Lighting

Indirect NEE may call the same immutable light inventory, distribution, and semantic light evaluator as Direct Lighting. It does not read Direct Lighting's screen-space reservoir unless a future unified estimator is separately researched, admitted, and proven. Each feature retains distinct histories and outputs.

### Sky And Atmosphere

The current image-based Sky owner publishes background/environment data. Indirect consumes an immutable `EnvironmentView`. Future Volumetric Lighting may synthesize an atmospheric environment generation; it owns atmospheric LUTs and camera-ray volume composition, while Indirect owns surface-path sampling/accounting of the published environment.

### Reference Path Tracer

The RPT supplies comparison artifacts only after its candidate is accepted. It does not become a callable library inside the interactive path, and the interactive feature cannot invoke the view-mode entry point directly. Shared semantic functions are disclosed in oracle independence analysis.

### Reconstruction

The selected reconstruction owner consumes typed products. It cannot reach into path reservoirs, rewrite estimator state, or become the only source of an indirect result. Optional RR and portable baseline report requested/active state through the existing provider vocabulary.

## Traversal And Backend Boundary

Indirect passes request semantic rays/trace results through the established Renderer ray owner. Inline and Pipeline execution remain distinct; the first product supports only the cells ratified by `IND-D0-01`. No backend-specific acceleration-structure representation, descriptor, pipeline, or SBT address enters feature records.

D3D12/Vulkan shader code shares the same registered semantic program contracts. Native validation and backend-specific evidence remain required. A working Inline implementation on both APIs does not prove Pipeline or higher material-feature cells.

## Quality Profiles

Exact values remain discovery output.

| Profile | Purpose | Shape |
| --- | --- | --- |
| `Initial` | analytic/statistical test | fresh one-bounce paths, no reuse/reconstruction |
| `Balanced` | default Lit product | frozen path depth, temporal/spatial candidates, correction, portable reconstruction |
| `Quality` | review/reference-near interactive | admitted additional paths/reuse/stronger correction within budget |
| `Unsupported`/`Degraded` | explicit missing domain/provider or reduced behavior | stable reason; never black-as-success |

Do not expose raw bounce/candidate/bias thresholds as permanent UI until workflow evidence shows a durable user decision.

## Integration-Hook Ledger

| Boundary | Permitted hook | Check |
| --- | --- | --- |
| shared transport semantics | exact evaluation/sampling functions used by interactive/reference as accepted | equation/independence test |
| GPU-scene/RenderScene | borrowed current/previous geometry/material/light generations required by paths | mutation/lifetime check |
| Sky environment | one immutable environment generation/view | mapping/accounting test |
| Lit frame recipe | one indirect feature invocation and typed products | graph/product trace |
| Lighting composite | unchanged two-lobe consumption and emission boundary | energy/accounting check |
| shader registration/cook | concrete feature shaders/dependencies | publication closure |
| selector/status/reconstruction | small resolved profile/provider boundary | fault/adoption matrix |
| capture/debug | existing raw product route | artifact identity |

Any other feature-named external edit requires an added row and defect-detecting check before code lands.

## Clean Break

The path-record stage updates passes, shaders, typed layouts, history allocation, captures, and docs together, then deletes `RandomPixel/SampleIndex/RandomFrameIndex` float payload and its seed-replay combination. No compatibility reader, internal version flag, dormant old shader, or second estimator stays. A bounded initial/no-reuse validation mode remains intentionally because it has a distinct oracle claim.

## Failure And Recovery

- Unsupported path/lobe/material/provider rejects or reports `Degraded` according to the frozen product table; it never silently reclassifies transport.
- Invalid shift/proposal/Jacobian/weight rejects the candidate; non-finite current output cannot publish history.
- Missing scene/material/light/environment generation fails before dereference/dispatch.
- Optional reconstruction failure follows explicit bypass/last-good/reset policy and never changes raw estimator status.
- Cancel/reload/shutdown drains or retires GPU generations; partial state cannot appear complete.

## Architecture Invariants

1. The resampled object is an explicit mathematical path/sample, not an unexplained seed.
2. Every path technique and contribution is counted once.
3. Every shift has a domain, inverse, support, and Jacobian.
4. GRIS contribution weight meaning survives packing, reuse, and reservoir combination.
5. Raw diffuse/specular precede reconstruction and composite.
6. Screen-space histories are per View and generation-safe.
7. Direct, indirect, reference, environment, and volumetric owners share only real semantic/infrastructure boundaries.
8. Optimization/caching follows a proved baseline and measured need.

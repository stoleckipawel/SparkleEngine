# Volumetric Lighting, Fog, Atmosphere, And Sky Discovery Gate

**Gate:** `VOL-D0`

**Status:** **Blocked by `REL-11` roadmap admission; discovery package open**. Research and bounded non-production experiments may prepare decisions, but no production feature implementation starts until release work is explicitly unlocked.

**Responsibility:** own roadmap/product, medium, content, reference, grid, lighting, atmosphere, reservoir, cloud, budget, evidence, and authorization decisions

**Authority boundary:** strategy/roadmap owns admission; research informs this page; this page freezes `VOL-D0`; semantics/architecture describe the target; the Plan cannot authorize itself; code and a future FCR own implementation/results

**Prepared:** 2026-09-12 at revision `8e4ffba225411965dc51c0b783e5f47a075c7e84`

## Gate At A Glance

| Question | Current answer | Required closure |
| --- | --- | --- |
| Is any volume feature implemented? | no; readiness remains 0/100 | preserve the negative claim until an admitted end-to-end slice exists |
| What ships first? | proposed: homogeneous/height fog + one-light single scattering + correct composition | roadmap and `VOL-D0` must ratify |
| Is Volumetric ReSTIR the base? | no | reference transport and froxel baseline precede its admission |
| Does existing Sky become atmosphere? | no | define an explicit image-sky versus physical-atmosphere mode and environment bridge |
| Are clouds/OpenVDB first scope? | no | later content-dependent gates |

## Iteration Control Record

| Field | Value |
| --- | --- |
| ID | `ITER-REN-VOL-00` |
| North Stars | `NS-REAL`, `NS-MATH-DATA`, `NS-EVIDENCE`, `NS-OWNERSHIP`, `NS-ADOPTION`, `NS-SIMPLIFY` |
| Graphics targets | `PGE-02`, `PGE-05` through `PGE-10`, `PGE-13`, `PGE-15` |
| Delivery target | post-`REL-11` admitted first volume slice, exact contracts, and assigned evidence owner |
| Principal risks | premature scope; nonphysical fog shortcut; composition error; temporal trails; duplicated sky/light/media state; ReSTIR without reference; volume asset pipeline bloat |
| Completion claim | documentation/research only; no capability/readiness increase |

## Required Decisions

### `VOL-D0-01` Roadmap, Product, And Platform Admission

After `REL-11`, freeze release/milestone, mandatory D3D12/Vulkan/device profile, runtime/editor reachability, and `Required/Deferred/Excluded` table for baseline fog, local media, atmosphere, heterogeneous volumes, Volumetric ReSTIR, and clouds. Assign an FCR/evidence owner before a production claim.

### `VOL-D0-02` World, Units, And Medium Semantics

Freeze metre-based world scale, `sigma_a`, `sigma_s`, `sigma_t`, albedo, emission units/basis, density multiplier, phase parameters, wavelength/RGB approximation, valid ranges, and scale behavior. Define friendly authoring conversions without creating a second renderer representation.

### `VOL-D0-03` Global, Height, And Local Media

Freeze global homogeneous/exponential height model, atmosphere-relative height/ground, local primitive shapes/transforms/falloff, overlap/blend/priority, capacity/overflow, and dynamic update identity. Decide whether local primitives are sphere/box only in the first unified-froxel slice.

### `VOL-D0-04` Content And Cooking

Freeze serializable components/assets, source import scope, cooked schema, dense 3D texture transform/channel/range/filter/compression, sparse OpenVDB/NanoVDB disposition, content hashes, reload/retirement, and missing/corrupt failure. No VDB ingestion before a renderer consumer and workload are admitted.

### `VOL-D0-05` Physical Reference And Analytic Oracles

Freeze Beer slab, homogeneous single scatter, phase normalization, height-density quadrature, and composition hand cases. Choose a CPU/reference volume integrator and an independent external/analytic cross-check. The surface Reference Path Tracer does not automatically support media and cannot be labeled the volume oracle without an admitted extension.

### `VOL-D0-06` Froxel Layout And Injection

Freeze view-space mapping, XY/Z resolution, linear/log/cascade depth, near/far behavior, jitter, world reconstruction, coefficient/density representation, local-volume raster/compute injection, overlap resolve, precision, clear/default state, and resize/multiview ownership.

### `VOL-D0-07` Direct Light And Shadow Injection

Freeze supported lights, selection/list/distribution, phase evaluation, light and view transmittance, geometry shadow provider, volume self-shadow/transmittance, per-froxel/pixel sampling, and light/medium generations. Compare deterministic small-light, clustered, and stochastic/reservoir approaches before choosing scale behavior.

### `VOL-D0-08` Integration And Composition

Freeze numerical integration, premultiplied in-scatter/transmittance format, opaque depth, sky/far boundary, surface equation, emission, pre-exposure, upsampling, transparency/transmission exclusions, pass order, and debug/raw capture. Prove `Lout=T*Lsurface+Lscatter` once.

### `VOL-D0-09` Temporal Reconstruction

Freeze sample jitter, reprojection domain/velocity, previous froxel or screen history, depth/medium boundary checks, confidence/responsive masks, neighborhood clamp, maximum accumulation, cuts/resize/dual-view, density/light/emission/atmosphere changes, and first-frame state.

### `VOL-D0-10` Atmosphere And Sky

Freeze planet/ground transform, Rayleigh/Mie/absorption profiles and phase, solar irradiance/disk, transmittance/multi-scatter/sky-view/aerial-perspective LUT parameterization, precision, generation/cache, background mode, atmosphere-light limit, altitude/space boundary, and interaction with image-based `SceneSkyDesc`.

### `VOL-D0-11` Environment Bridge

Freeze how image sky or physical atmosphere publishes one immutable background/environment radiance generation, directional PDF, rotation/time identity, and update transaction to Direct/Indirect Lighting. Define which owner samples surface environment and which integrates camera-ray atmosphere so neither is double counted.

### `VOL-D0-12` Heterogeneous Tracking And Acceleration

Freeze dense/sparse lookup, majorant/control field, fixed-march baseline, delta/ratio/residual tracking reference, empty-space skip, filter/LOD, animated transform/density, numerical limits, and failure when a majorant is violated. This decision gates heterogeneous production work.

### `VOL-D0-13` Volumetric ReSTIR Domain

Freeze whether the first reservoir use is per-froxel many-light injection, single-scatter camera paths, or multi-scatter path-space reuse. Specify path sample, free-flight/transmittance proposal, target, generalized contribution weight, shift/inverse/Jacobian/support, approximate candidate evaluation, unbiased/controlled final evaluation, correlation/bias mode, history identity, and reference tests.

### `VOL-D0-14` Clouds

Freeze separate admission only after atmosphere/heterogeneous foundations: weather/density authoring, procedural detail, bounds/LOD/empty-space skipping, direct/multiple-scatter approximation, ground/cloud shadows, atmosphere composition, temporal reconstruction, and close/fly-through acceptance. Keep clouds excluded from the first implementation plan unless explicitly promoted.

### `VOL-D0-15` Quality, Memory, Time, And Failure Budgets

Measure candidate layouts before freezing. Record render/upscale resolution, froxel dimensions/formats, light/volume capacities, steps/tracking events/rays/reservoir candidates, LUT sizes/update cost, history/replacement overlap, peak memory, queue placement, GPU time distribution, and quality metrics for clear air through extreme optical depth.

### `VOL-D0-16` Ownership, Adoption, Diagnostics, And Evidence

Freeze one feature home, authored/cooked/scene/GPU/frame/RHI integration hooks, requested/active modes and reason codes, default/quality profiles, raw diagnostics through existing infrastructure, acceptance fixtures/artifact schema/thresholds, D3D12/Vulkan cells, and the candidate report ID. No feature dashboard, generic volume framework, or speculative public type.

## Decision Closure Register

| Decision | Options that must be compared | Required retained evidence | Decision owner/reviewer | Status | Consequence while open |
| --- | --- | --- | --- | --- | --- |
| `VOL-D0-01` | admitted post-release tiers/platforms versus continued exclusion | `REL-11` closeout, product/persona/workload and FCR assignment | product/release owners | Open/Blocked | no production feature or selector |
| `VOL-D0-02` | scalar/RGB coefficient and photometric conversion policies | dimensional derivation, coefficient/scale/finite hand cases | volume semantics / math reviewer | Open | no medium record can publish |
| `VOL-D0-03` | global/height/local shapes and additive/priority/blend overlap | field/boundary/overlap/capacity/authoring study | content + volume owners / artist reviewer | Open | only smallest ratified global field may enter Stage 1 |
| `VOL-D0-04` | dense, sparse, procedural and deferred source formats | representative assets, rights, cook schema, memory/upload/error results | content/cook owner / provenance reviewer | Open | heterogeneous tier remains absent |
| `VOL-D0-05` | analytic, CPU/high precision, PBRT/tracker and external sky reference | oracle manifest, shared-code analysis, tolerances | verification / independent renderer reviewer | Open | no physical-quality verdict |
| `VOL-D0-06` | grid/depth/layout/format/representation/integration variants | forward/inverse tests and error/bandwidth/time/memory sweeps | froxel owner / backend reviewer | Open | no persistent production grid contract |
| `VOL-D0-07` | all-light/clustered/stochastic/reservoir plus visibility/transmittance | one/many-light raw source tests and scale curve | volume-light owner / Direct/RHI reviewers | Open | first tier limited to admitted light path |
| `VOL-D0-08` | segment integration, sky/surface/transparency ordering and early termination | analytic tuples, opaque/miss/depth/order captures | composition owner / lighting reviewer | Open | no scene-color composition hook |
| `VOL-D0-09` | raw/source/integrated history, reprojection/clamp/confidence | full camera/density/light/extent/dual-view matrix | temporal owner / motion reviewer | Open | no history or product stability claim |
| `VOL-D0-10` | Hillaire/Bruneton/direct-march LUT scope, atmosphere model and sky mode | raw LUT/query reference matrix and atomic generation failure | atmosphere owner / independent sky reviewer | Open | physical atmosphere remains absent |
| `VOL-D0-11` | background/sun/aerial/direct/indirect environment generation roles | mapping/radiance/PDF/generation and double-count tests | environment + lighting owners / cross-feature reviewer | Open | no physical-sky bridge |
| `VOL-D0-12` | fixed march and delta/ratio/residual tracking plus acceleration | majorant/reference statistics and density-frequency/empty-space A/B | heterogeneous owner / estimator reviewer | Open | no heterogeneous transport claim |
| `VOL-D0-13` | froxel-light reservoir versus camera/path-space ReSTIR domains | exact sample/shift/weight/final estimator and equal-time reference study | volume estimator / GRIS reviewer | Open | no reservoir code or ReSTIR label |
| `VOL-D0-14` | clouds excluded, deferred or separately admitted with exact scope | content/authoring/weather/lighting/temporal/workload budget | product/content owner / artist reviewer | Deferred | no cloud type, setting, pass or claim |
| `VOL-D0-15` | profile dimensions, samples/steps/rays, queues, time/memory and failure budgets | local target-device quality/time/memory/replacement curves | performance owner / platform reviewers | Open | no default profile or performance promise |
| `VOL-D0-16` | feature home, hooks, authoring/status/debug/automation and FCR schema | [UX](UserExperience.md) dry run, hook/removal ledger, candidate/check review | architecture/editor/verification owners | Open | no public workflow or implementation stage |

## Gate Resolution Protocol

The release owner first closes admission. Each admitted row then fixes options, fixtures, metrics and thresholds before a bounded prototype or reference run; retains exact source/content/build/backend/dirty identity and raw artifacts; records explicit accepted/rejected/deferred/excluded reasons; completes rights/provenance review where assets/code are involved; obtains the named independent review; and updates the single semantic/architecture/UX owner. Missing admission, reference, content rights, capability or target hardware yields `Blocked`. A paper or another engine cannot close a local row.

## Risk Register

| Risk | Leading indicator | Containment | Escalation trigger |
| --- | --- | --- | --- |
| cosmetic fog ships without physical contract | depth-color blend appears before coefficient/Beer tests | Stage 1 analytic medium and composition oracle | no inverse-metre representation or raw `T/Lscatter` |
| fog, atmosphere, sky and clouds become parallel systems | multiple environment generations/composition writes | one feature enclosure and environment bridge | second sky/background or medium state owner appears |
| froxel temporal stability hides integration bias | smooth result changes with grid/step in raw mean | current-only convergence plus separate history verdict | decreasing step/grid does not approach reference |
| majorant/content errors are clamped away | violations disappear but energy changes | terminal counters and invalid publication | any density exceeds claimed majorant in accepted run |
| Volumetric ReSTIR is only light selection | sample lacks volume path/free-flight/final estimator | exact domain naming and estimator ledger | product label exceeds stored/resampled object |
| external timings become Sparkle budget | plan/profile contains source hardware numbers | local workload/device measurement required | no Sparkle candidate measurement exists |
| content source lacks durable rights/schema | test asset cannot be redistributed or recooked | provenance manifest and synthetic fallback fixture | acceptance depends on inaccessible asset |

## Proposed Admission Matrix

| Capability | Proposed disposition | Dependency |
| --- | --- | --- |
| medium math + analytic/reference tests | Required first | post-`REL-11` approval |
| homogeneous/exponential height fog | Required first product | medium contract |
| one directional light single scattering | Required first vertical slice | Direct light semantic bridge |
| correct surface/sky composition | Required first vertical slice | frame recipe/scene-linear boundary |
| local box/sphere media | Required unified-froxel slice | authoring/scene/GPU schema |
| all admitted analytic lights + shadows | Required unified-froxel slice | light lists/distribution and visibility decision |
| portable temporal reconstruction | Required unified-froxel slice | raw static correctness |
| physical atmosphere/aerial perspective | Required major tier | LUT/reference/environment bridge |
| dense heterogeneous texture | Planned | content/cook/tracking gate |
| OpenVDB/NanoVDB | Deferred | accepted Volume workload and content pipeline |
| per-froxel reservoir light selection | Conditional | measured many-light problem |
| path-space Volumetric ReSTIR | Planned research/high-quality tier | reference integrator + heterogeneous media |
| clouds | Deferred separate admission | atmosphere + heterogeneous + authoring UX |
| transparent-surface medium nesting | Excluded initially | transmission/material/order-independent rendering contract |

## Required Experiments

| ID | Exercise | Decision |
| --- | --- | --- |
| `VOL-X-01` | CPU/shader Beer slab, albedo/emission, phase normalization, height integral | `02/05` |
| `VOL-X-02` | candidate froxel mappings at near/far, depth discontinuity, thin shafts, camera motion | `06/09/15` |
| `VOL-X-03` | one-to-thousands light injection comparison: exhaustive/clustered/stochastic | `07/13/15` |
| `VOL-X-04` | opaque surface + sky + volume + depth order with raw `T/Lscatter` | `08/11` |
| `VOL-X-05` | Hillaire/Bruneton reference sky and aerial-perspective parameter/altitude cells | `10/11` |
| `VOL-X-06` | dense volume fixed march versus tracking reference, majorant/empty-space stress | `04/12/15` |
| `VOL-X-07` | volumetric ReSTIR enumerated/homogeneous and heterogeneous path comparisons | `13` |
| `VOL-X-08` | temporal animated density/light/emission/atmosphere/cut/dual-view sequence | `09/15` |
| `VOL-X-09` | authoring/cook/load/reload/corrupt/missing/capacity workflow | `03/04/16` |

## Discovery Evidence Package

1. approved post-`REL-11` scope, owner, FCR ID, stage budget, and exact candidate boundary;
2. medium/phase/units and analytic/reference results;
3. authored/cooked/scene/GPU data schema and lifetime experiment;
4. froxel mapping/format/resolution/memory/quality study;
5. light/shadow injection and many-light comparison;
6. integration/composition proof with raw transmittance/in-scatter;
7. temporal reconstruction mutation results;
8. atmosphere/LUT/environment comparison and bridge design;
9. heterogeneous asset/tracking/majorant study;
10. Volumetric ReSTIR path/estimator experiment and explicit admission/rejection;
11. exact integration-hook and clean-break ledger, revised estimates, thresholds, backend cells, and independent review.

## Discovery Failure Modes

| ID | Failure | Gate response |
| --- | --- | --- |
| `FM-VOL-D0-01` | work begins before `REL-11` unlock/admission | stop production; retain research only |
| `FM-VOL-D0-02` | a fog screenshot substitutes for coefficient/composition proof | remain open; require raw analytic artifacts |
| `FM-VOL-D0-03` | image sky is mutated into atmosphere without mode/migration decision | reject ownership shape |
| `FM-VOL-D0-04` | Volumetric ReSTIR is selected before reference/tracking baseline | remain open; execute `VOL-X-06/07` |
| `FM-VOL-D0-05` | clouds/VDB/transparency inflate first slice | defer through admission matrix |
| `FM-VOL-D0-06` | source grid/step/timing becomes Sparkle default | measure locally and freeze budget |
| `FM-VOL-D0-07` | separate fog/atmosphere/cloud state or duplicate light store proposed | reject and redesign one medium/lighting boundary |
| `FM-VOL-D0-08` | no supported backend/content fixture/evidence owner exists | mark blocked; do not claim partial product readiness |

## Exit Criteria And Decision

`VOL-D0` passes only after `REL-11` explicitly unlocks new features, a roadmap owner admits the exact scope and assigns an FCR, all sixteen decisions and relevant experiments close, first-stage hooks/checks/budgets are reviewed, and negative-capability claims are updated in the same admitted change.

**Current decision: Blocked by roadmap; discovery open.** No production code is authorized by this package, and Volumetric Lighting remains 0/100.

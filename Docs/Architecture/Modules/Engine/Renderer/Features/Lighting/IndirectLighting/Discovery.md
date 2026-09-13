# Indirect Lighting And ReSTIR GI Discovery Gate

**Gate:** `IND-D0`

**Status:** **Open / production replacement blocked**; current code remains an unproved prototype until this gate and applicable release admission close

**Responsibility:** own indirect path-domain, estimator, shift, oracle, history, reconstruction, budget, risk, experiment, and authorization decisions

**Authority boundary:** research informs this gate; this page alone freezes `IND-D0`; semantics/architecture describe the target; the Plan cannot improvise unresolved choices; code and `FCR-REN-07` own implementation/results

**Prepared:** 2026-09-12 at revision `8e4ffba225411965dc51c0b783e5f47a075c7e84`

## Gate At A Glance

| Question | Current answer | Required closure |
| --- | --- | --- |
| Does Sparkle have indirect output? | yes, two lobe textures from an inline-ray seed-replay reservoir | prove the initial estimator and replace unratified reuse |
| Is it ReSTIR GI? | not established by inspected state/equations | select exact ReSTIR GI/GRIS domain, path record, shift, and weight |
| What is the oracle? | analytic/metamorphic cases now; Reference Path Tracer only after its own acceptance | freeze shared-code analysis and comparison statistics |
| Does sky mean physical atmosphere/IBL completeness? | no | freeze environment/background/path roles; volumetric atmosphere remains separate |
| Can implementation start? | only bounded discovery instrumentation | gate, release admission, and Stage-specific dependencies must pass |

## Iteration Control Record

| Field | Value |
| --- | --- |
| ID | `ITER-REN-IND-00` |
| North Stars | `NS-REAL`, `NS-MATH-DATA`, `NS-EVIDENCE`, `NS-OWNERSHIP`, `NS-ADOPTION`, `NS-SIMPLIFY` |
| Graphics targets | `PGE-02`, `PGE-05` through `PGE-10`, `PGE-13`, `PGE-15` |
| Delivery target | accepted indirect estimator domain and one bounded replacement slice |
| Principal risks | invalid seed replay; double-counted path techniques; correlated reuse; shared-oracle defects; temporal leaks; scope expansion into a second GI system |
| Completion claim | documentation/research only; no estimator, runtime, GPU, visual, convergence, performance, backend, or release proof |

## Required Decisions

### `IND-D0-01` Product And Release Domain

Freeze mandatory hardware/profile, Lit/view scope, first release milestone, bounce range, diffuse/glossy roughness support, dynamic geometry/light/sky requirements, and explicit delta/transmission/caustic exclusions. Decide whether only Inline traversal is a supported first product or Pipeline parity is required.

### `IND-D0-02` Transport Equation And Technique Accounting

Freeze pixel measurement, path-length/domain notation, throughput, emitted radiance, environment misses, next-event estimation, BSDF sampling, MIS, Russian roulette, direct/indirect split, and lobe split. Map every contribution to one technique so NEE, emissive hits, sky misses, and shared composite emissive cannot double count.

**Stop condition:** if the current `PathLighting` accounting cannot be proven on hand cases, replace it before any reservoir work.

### `IND-D0-03` Independent Oracles And Fixtures

Freeze CPU/analytic/metamorphic cases and external/reference-tracer artifacts. Name shared material/light/texture/ray code and the independent cross-check for each. Use Cornell Box as a controlled reference, Sponza as Tier 0 reachability, Bistro as flagship, and San Miguel as held-out content.

### `IND-D0-04` Initial Candidate Generator

Freeze the first one-bounce candidate: primary receiver, BSDF/lobe proposal, secondary vertex, NEE light proposal, terminal emission/environment handling, proposal densities/measures, throughput, and raw diffuse/specular classification. Decide candidate count and random-dimension layout only after baseline variance measurement.

### `IND-D0-05` Path Sample Record

Freeze every stored field, precision, packing, and reconstruction rule. Require integer storage for logical IDs/counts and exact round-trip tests. Decide the minimum explicit path vertices/factors required before any suffix replay optimization.

### `IND-D0-06` Shift Mapping

Select reconnection, random replay, or hybrid mapping for each admitted lobe/path class. Freeze domain, partial-bijection/inverse, support, determinant/Jacobian, visibility/retrace, geometry motion, normal/roughness criteria, and delta-chain behavior. A rejected mapping contributes no candidate mass.

### `IND-D0-07` GRIS/Reservoir Weight

Freeze target, generalized contribution weight, source proposal/technique identity, resampling MIS weights, stream update, effective sample count/cap, final estimator, repeated/correlated candidates, and declared convergence/bias mode. Map equations to CPU tests and shader functions.

### `IND-D0-08` Temporal/Spatial Reuse And Disocclusion

Freeze reprojection/splat direction, previous receiver and path motion, surface/material/object/light/sky generations, neighbor distribution, disocclusion classification, temporal/spatial order, sample duplication/correlation measurement, and reset policy. Evaluate 2025/2026 reservoir splatting or multilayer work only after the base route passes.

### `IND-D0-09` Ray, Material, And Animation Semantics

Freeze robust ray offsets, minimum/maximum segment, face/two-sided/alpha semantics, texture LOD, shading-normal correction, animated/skinned/deformed geometry identity, TLAS/hit-material generation, and provider failure. Reused paths cannot point to retired geometry/material/light generations.

### `IND-D0-10` Environment And Cross-Feature Boundary

Freeze environment mapping/rotation/radiance/PDF and its background, NEE, and terminal-miss roles. Define how a future atmosphere publishes an immutable environment generation and how camera-ray atmospheric transmittance stays outside surface GI. Define direct-lighting NEE reuse without coupling the two feature histories.

### `IND-D0-11` Reconstruction

Freeze vendor-neutral diffuse/specular baseline, optional RR, input radiance/hit-distance/pre-exposure, depth/normals/roughness/material/motion, confidence/disocclusion, history cap/reset, responsive behavior, and missing-provider fallback. Preserve raw products.

### `IND-D0-12` Quality, Time, Memory, And Path Budgets

Measure current and initial baselines. Freeze render/upscale resolution, candidates, ray/path counts, maximum depth, roulette, reservoir/path/guide formats, history generations, peak replacement memory, queue placement, and GPU timing statistic. Include static/motion/disocclusion, hard indirect visibility, glossy transport, emissive/environment, and alpha geometry.

### `IND-D0-13` Ownership, Clean Break, And Optional Architectures

Freeze the feature capsule and exact seed-replay deletions. Record dispositions for DDGI/probes, screen-space GI, Lumen-like surface cache, SHaRC, NRC, ReSTIR PG, ReSTIR PT Enhanced, and ReSTIR BDPT. None becomes a second live product without a separately admitted platform/user need.

### `IND-D0-14` Evidence, Adoption, And FCR

Freeze quality profiles, requested/active status, exclusions/degraded reasons, raw artifact schema, metrics/tolerances before observation, D3D12/Vulkan/device/driver/compiler/shader identity, capture windows, and `FCR-REN-07` ownership. Separate estimator, temporal, visual, denoiser, performance, memory, backend, and release verdicts.

## Decision Closure Register

| Decision | Options that must be compared | Required retained evidence | Decision owner/reviewer | Status | Consequence while open |
| --- | --- | --- | --- | --- | --- |
| `IND-D0-01` | one/multi-bounce, diffuse/rough-glossy, Inline/Pipeline, explicit exclusions | release/profile/path-domain table and workload need | Renderer owner / release owner | Open | no production domain is authorized |
| `IND-D0-02` | NEE/emission/environment/continuation/MIS and lobe split variants | technique ledger, analytic/metamorphic toggles and raw AOVs | transport semantics / independent math reviewer | Open | current path accounting cannot be trusted |
| `IND-D0-03` | analytic, external, accepted RPT and content fixtures | oracle manifest, shared-code analysis, metrics/tolerances | verification / independent reference reviewer | Open | no scene-level correctness claim |
| `IND-D0-04` | BSDF/lobe/light/environment candidate mixtures and counts | PDF/support/variance/ray-cost study | initial generator / estimator reviewer | Open | no initial reservoir implementation |
| `IND-D0-05` | explicit vertices/factors versus proved compact replay | logical record, pack/round-trip, mapping sufficiency and bandwidth | path record owner / data-lifetime reviewer | Open | seed payload cannot be grandfathered |
| `IND-D0-06` | reconnection, replay, hybrid, splatting by path class | domain/inverse/support/Jacobian/visibility hand and statistical tests | shift owner / independent estimator reviewer | Open | no cross-receiver reuse |
| `IND-D0-07` | exact GRIS target/contribution/resampling/bias/correlation modes | equation ledger and enumerated/distribution tests | reservoir owner / GRIS reviewer | Open | no ReSTIR GI/GRIS conformance claim |
| `IND-D0-08` | reprojection/backprojection/splatting, ordering/neighbors/disocclusion | mutation, duplication, autocorrelation, convergence and recovery artifacts | reuse owner / temporal reviewer | Open | no temporal/spatial product reuse |
| `IND-D0-09` | robust ray/material/alpha/animation semantics and provider matrix | ray hand cases, deformed motion, generation/lifetime faults | shared ray/material owners / RHI reviewer | Open | animated/path provider scope excluded |
| `IND-D0-10` | image/physical environment roles and direct-light sharing | mapping/PDF/generation and double-count tests | sky/environment + indirect owners / cross-feature reviewer | Open | environment claims remain partial |
| `IND-D0-11` | portable reconstruction and optional DLSS RR | raw/guide/history contract and motion/provider-fault A/B | reconstruction owner / product reviewer | Open | no reconstructed product claim |
| `IND-D0-12` | depth/rays/records/history/render scale/queue profiles | equal-quality/time/memory/replacement study by domain | performance owner / platform reviewers | Open | no default profile/budget |
| `IND-D0-13` | selected path-reservoir route versus separately admitted probes/caches/guiding | architecture A/B, feature-home/hook/deletion ledger | Renderer architecture / module owner | Open | no optional GI architecture may enter |
| `IND-D0-14` | settings/status, diagnostics, metrics, artifact schema and FCR cells | [UX](UserExperience.md) dry run, candidate manifest, check/FCR review | editor/verification / `FCR-REN-07` owner | Open | no public workflow or candidate verdict |

## Gate Resolution Protocol

Each row closes only after hypotheses and thresholds are fixed, the exact candidate/baseline/dirty boundary is recorded, the smallest differentiating experiment runs, raw artifacts and shared-code dependencies are retained, alternatives receive explicit `Accepted/Rejected/Deferred/Excluded` reasons, and the named reviewer signs the result. Accepted decisions are copied into their single semantic/architecture/UX owner before dependent plan stages unlock. New path classes or surprising convergence/motion results reopen the relevant row.

## Risk Register

| Risk | Leading indicator | Containment | Escalation trigger |
| --- | --- | --- | --- |
| seed replay looks plausible but has no mapped density | stable images with destination-dependent mean drift | explicit path records and shift distribution tests | current prototype cannot state source/destination probability |
| techniques double count emission/environment | energy changes incorrectly as NEE/depth toggles | executable technique ledger and metamorphic cases | predicted toggle relation fails |
| stored `M` overstates correlated diversity | `M` grows while unique paths plateau | duplication/autocorrelation/effective-diversity artifacts | convergence worsens with added reuse |
| broader glossy/depth scope hides base defects | failures appear only after multiple simultaneous changes | one lobe/depth/terminal increment per gate | Stage 1 one-bounce mean is not proved |
| shared reference repeats same defect | RPT and product agree but analytic/external case disagrees | shared-code disclosure and second oracle | all available references share questioned code |
| optional cache/guiding becomes second GI system | new scene state or lifetime appears in optimization | separate product admission and removal-on-fail | new generic owner/provider is required |

## Proposed Domain Disposition

| Capability | Proposed first disposition | Rationale |
| --- | --- | --- |
| one-bounce diffuse | Required | simplest useful estimator and shift validation |
| rough glossy | Required within frozen threshold | engine target requires separate indirect specular |
| multi-bounce diffuse | Required after one-bounce proof | flagship interior need |
| longer glossy chains | Planned ReSTIR PT increment | complexity exceeds first conformance slice |
| perfect delta reflection | Excluded initially or reference-only | reconnection/reconstruction domain risk |
| transmission/refraction | Excluded | material feature not currently admitted |
| caustics/ReSTIR BDPT | Research only | new bidirectional domain and large cost/state |
| environment miss and sampling | Required | existing sky route must have exact transport role |
| emissive terminal hit | Required | core path transport, with explicit double-count policy |
| emissive NEE inventory | dependent on Direct Lighting admission | avoid duplicate light extraction |
| portable reconstruction | Required | sparse GI product cannot depend only on vendor path |
| DLSS RR | Optional | acceleration behind common semantic inputs |
| DDGI/surface/radiance/neural caches | Deferred alternatives | no current platform tier justifies parallel systems |

## Required Experiments

| Experiment | Inputs | Decision it falsifies |
| --- | --- | --- |
| `IND-X-01` current seed replay audit | decoded source/destination seeds, paths, targets, weights over two distinct surfaces | whether current reuse has a valid mapping |
| `IND-X-02` technique accounting | analytic diffuse box, isolated emissive, environment, NEE toggle, path-depth sweep | `IND-D0-02/04` |
| `IND-X-03` shift round trip | known rough/glossy paths, mapping/inverse/Jacobian/support | `IND-D0-06` |
| `IND-X-04` reservoir statistics | enumerated/discrete path proposals, duplication/correlation, temporal/spatial chains | `IND-D0-07/08` |
| `IND-X-05` motion/animation | camera, disocclusion, rigid/deformed surface, light/sky/material mutation | `IND-D0-08/09/10` |
| `IND-X-06` reconstruction | raw/reconstructed static/motion cells with identical guides and exposure | `IND-D0-11` |
| `IND-X-07` workload baseline | Cornell/Sponza/Bistro/San Miguel at exact profile/backend | `IND-D0-12/14` |

## Discovery Evidence Package

1. exact source/candidate/build/shader/backend/content identities and clean working-boundary statement;
2. equation-to-current-code audit proving or rejecting seed replay as a valid shift;
3. frozen path-domain and technique-accounting table;
4. analytic/metamorphic and reference oracle manifests with shared-code analysis;
5. initial candidate and path-record round trips;
6. shift inverse/Jacobian/support results;
7. GRIS reservoir statistical/correlation results;
8. temporal/animation/failure matrix;
9. reconstruction guide/history study;
10. quality/ray/path/time/memory baseline, accepted hook/deletion ledger, revised estimates, and independent gate review.

## Discovery Failure Modes

| ID | Failure | Response |
| --- | --- | --- |
| `FM-IND-D0-01` | a random seed is assumed to encode a valid path proposal | remain open; require domain/probability/shift proof |
| `FM-IND-D0-02` | shared Reference Path Tracer output is treated as independent | disclose dependency and retain analytic/external cross-check |
| `FM-IND-D0-03` | denoised stability is used as estimator/convergence evidence | reject result; capture raw samples/history |
| `FM-IND-D0-04` | bounce count is increased before one-bounce accounting closes | stop and return to `IND-D0-02/04` |
| `FM-IND-D0-05` | multiple alternative GI systems enter the plan | require separate product/platform admission; keep one target |
| `FM-IND-D0-06` | paper timing/MSE becomes a Sparkle budget | leave local budget unresolved and measure |
| `FM-IND-D0-07` | release gate is closed to feature work | keep production stages blocked without changing capability status |

## Exit Criteria And Decision

`IND-D0` passes only when all fourteen decisions are explicit, every selected equation has an owner and falsifying test, path/shift/reservoir representations round-trip, the oracle independence is reviewed, budgets and failure states are frozen, seed-replay deletions are named, and the release owner admits the next stage.

**Current decision: Open / Blocked for production replacement.** The existing implementation remains source-present and unproved; this documentation does not change its capability score.

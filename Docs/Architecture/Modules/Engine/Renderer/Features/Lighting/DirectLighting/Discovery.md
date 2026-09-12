# Direct Lighting Discovery Gate

**Gate:** `DIR-D0`

**Status:** **Open / production replacement blocked**; research and source discovery may proceed, but Stage 1 implementation requires this gate plus the applicable release/admission decision

**Responsibility:** own unresolved direct-lighting decisions, experiments, risks, gate evidence, and the binary authorization for implementation

**Authority boundary:** research supplies precedent; this page alone freezes `DIR-D0`; semantic/architecture pages describe the candidate; the Plan cannot choose an unresolved decision; code and `FCR-REN-06` own implementation and results

**Prepared:** 2026-09-12 at revision `8e4ffba225411965dc51c0b783e5f47a075c7e84`

## Gate At A Glance

| Question | Current answer | Required closure |
| --- | --- | --- |
| Is there a direct-lighting implementation? | yes, a ray-dependent analytic-light reservoir route | preserve only behavior that passes the frozen conformance matrix |
| Is it known correct? | no; the user reports broken visuals and no numerical/runtime evidence was executed here | localize raw estimator, BRDF, visibility, identity, and reconstruction failures |
| Is the target “MegaLights” or RTXDI? | neither as a copied subsystem | one Sparkle-owned many-light product informed by both |
| Can production changes start? | only bounded diagnostic probes; no estimator rewrite | accept every `DIR-D0-*` decision and satisfy the release gate |
| What is the first implementation? | deterministic analytic/exhaustive baseline | reservoir replacement follows proof, not tuning |

## Iteration Control Record

| Field | Value |
| --- | --- |
| ID | `ITER-REN-DIR-00` |
| North Stars | `NS-REAL`, `NS-MATH-DATA`, `NS-EVIDENCE`, `NS-OWNERSHIP`, `NS-ADOPTION`, `NS-SIMPLIFY` |
| Graphics targets | `PGE-02`, `PGE-05` through `PGE-10`, `PGE-13`, `PGE-15` |
| Delivery target | accepted `DIR-D0` package and one authorized first production slice |
| Principal risks | misdiagnosed visual defect; biased reuse; unit/BRDF mismatch; stale light identity; denoiser masking; scope expansion; vendor lock-in |
| Completion claim | documentation/research only; no code, runtime, GPU, visual, performance, backend, or release result |

## Discovery Questions And Required Decisions

### `DIR-D0-01` Product And Release Scope

Freeze the minimum supported platform/profile, view modes, release milestone, and authored light classes. The proposed first product supports the existing four analytic lights; emissive triangles and environment candidates are planned capability increments, not assumed first-slice scope. Decide whether a non-ray fallback is a product requirement or an explicit unsupported cell.

**Exit artifact:** signed scope table with `Required`, `Deferred`, and `Excluded` dispositions and a link to the release owner.

### `DIR-D0-02` Units And Light Geometry

For each light class freeze authored units, RGB-to-radiometric conversion, range/attenuation law, cone interpolation, radius/angular extent, rectangle basis/sidedness, delta classification, near-field policy, and maximum finite input. Reconcile GameFramework descriptors, prepared GPU records, shaders, importers, and the Reference Path Tracer contract.

**Experiment:** CPU and shader hand cases at unit distance, inverse-square ratios, cone boundaries, rect area/solid-angle conversion, zero/degenerate values, and high dynamic range.

### `DIR-D0-03` Surface And Lobe Contract

Freeze Burley/GGX/Smith/Schlick formulas, perceptual roughness mapping, minimum roughness, Fresnel inputs, geometric versus shading normals, and diffuse/specular/subsurface energy allocation. Decide whether wrap subsurface remains a direct lobe, is replaced, or is excluded until a separate subsurface feature exists.

**Stop condition:** if simultaneous diffuse plus subsurface spends more energy than the intended base layer or evaluation/sampling disagree, do not proceed to reservoirs.

### `DIR-D0-04` Independent Baseline And Oracles

Define an exhaustive small-light GPU path and an independent CPU/closed-form evaluator. Freeze Cornell Box and analytic fixtures before observation. Name when the Reference Path Tracer may become an additional oracle and record shared-code independence. A shared BRDF/light defect cannot validate itself.

**Required products:** raw direct diffuse/specular/subsurface, selected light/sample PDF, visibility, and finite/error counters; no tone mapping.

### `DIR-D0-05` Light Inventory And Stable Identity

Freeze stable light identity across add/remove/reorder/update, current/previous lookup, tombstone/translation lifetime, emissive-triangle identity if admitted, and overflow policy. Decide whether the present broad scene hash remains only a conservative reset trigger or is replaced by classified generations.

**Faults:** remove the selected light, reorder each family, animate intensity/shape, recycle an object ID, exceed capacity by one, and mutate an emissive mesh generation.

### `DIR-D0-06` Initial Sampling

Freeze distributions and mixtures: uniform oracle, local-light power PDF, environment luminance-times-solid-angle PDF, analytic/delta family PMFs, and optional emissive/ReGIR sources. Every candidate records or can reconstruct its source PMF and conditional PDF in the correct measure.

**Admission rule:** ReGIR requires a measured quality/time/memory win in large distributed-light cells over global power PDF plus simple receiver-local filtering.

### `DIR-D0-07` Reservoir Estimator And Bias Policy

Freeze reservoir layout/precision, target function, streaming update, selected-sample replay, normalization, effective `M` caps, temporal/spatial combination, Jacobians/measure conversions, random dimensions, bias-correction modes, and visibility-reuse contract. Decide whether the base profile is unbiased, pairwise/basic corrected, or knowingly biased with bounded measured error.

**Required reference:** equations in [Sampling And Shading](SamplingAndShading.md) mapped to CPU test and exact shader function. Parameter names alone are insufficient.

### `DIR-D0-08` Reprojection, Compatibility, And Disocclusion

Freeze motion convention, previous pixel reconstruction, depth space, geometric/shading normal gates, material/roughness/lobe/object identity, hit/sky class, screen bounds, neighbor distribution, disocclusion classification, and reset generations. Decide which differences force invalidation versus reduced confidence.

**Experiment:** static convergence, subpixel camera motion, cut, thin geometry reveal, animated normal/roughness/alpha, moving occluder, moving emitter, resize, dual views, shader reload, and backend/provider switch.

### `DIR-D0-09` Visibility And Shadow Provider

Freeze segment endpoints, light-distance termination, origin offset, geometric-normal handling, backface/two-sided/alpha semantics, any-hit material access, and strict/automatic provider behavior. Decide whether Inline and Pipeline are both first-product requirements. Record shadow-map/VSM/screen-trace/OMM dispositions without implementing them.

### `DIR-D0-10` Reconstruction And Confidence

Select a vendor-neutral baseline and optional DLSS RR path. Freeze raw-signal representation, pre-exposure convention, motion/depth/normal/roughness/hit-distance inputs, confidence, responsive/disocclusion masks, history cap, clamp rules, and bypass/failure behavior. The denoiser cannot feed estimator history unless the estimator contract explicitly requires that dependency.

### `DIR-D0-11` Quality, Memory, And Time Budgets

Measure the current route first. Freeze resolution/upscaling cells, candidate and shadow-ray counts, lobe/reservoir/guide bytes, peak transient and persistent memory, queue placement, and GPU frame-time budget. Include one-light analytic, dense local overlap, broad distributed lights, motion/disocclusion, alpha foliage, and area-light scenes. Source-paper timings cannot fill these cells.

### `DIR-D0-12` Adoption, Diagnostics, And Failure UX

Freeze default/quality profiles, requested-versus-active algorithm/provider, unsupported and degraded reason codes, raw debug/capture path, and setting persistence. Prefer existing generic capture/debug routes. Do not add a feature dashboard, public inspection API, or convenience wrapper solely for evidence.

### `DIR-D0-13` Ownership And Clean Break

Freeze one feature home and an integration-hook ledger. Decide exact old reservoir/shader/pass files to replace. No legacy estimator, dual reservoir representation, compatibility reader, internal version switch, or duplicated RTXDI-style scene/light store may survive the replacement stage.

### `DIR-D0-14` Acceptance And Candidate Identity

Freeze every tolerance before candidate observation, exact D3D12/Vulkan configurations, driver/compiler/shader publication identities, fixture hashes, warmup/capture windows, raw artifact schema, performance statistics, and `FCR-REN-06` ownership. Separate numerical, temporal, visual, memory, time, failure, backend, and release verdicts.

## Proposed First-Scope Decision Matrix

| Capability | Proposed disposition | Reason |
| --- | --- | --- |
| four current analytic lights | Required | existing authoring surface and direct user value |
| exhaustive small-light resolve | Required | cheapest independent GPU truth path |
| power-weighted initial selection | Required | material variance reduction over uniform-only sampling |
| canonical temporal/spatial ReSTIR DI | Required | selected many-light scale path |
| Inline visibility | Required | existing lean semantic path |
| Pipeline visibility | Required where currently advertised ready | preserves current strict selector contract |
| vendor-neutral reconstruction | Required | portable product baseline |
| optional DLSS RR | Optional supported profile | acceleration, never sole semantics |
| emissive mesh lights | Planned; gate separately | necessary for broader many-light realism, but content/update ownership is large |
| environment direct candidates | Planned with sky/environment contract | avoids treating background fill as sampled lighting |
| ReGIR | Conditional | only if distributed-light evidence justifies state/cost |
| VSM/shadow maps | Deferred decision | no current non-ray product requirement is frozen |
| opacity micromaps | Deferred | depends on measured alpha traversal cost and content pipeline |
| new OpenPBR lobes | Excluded from this delivery | material feature expansion, not a direct-lighting repair |

## Discovery Evidence Package

The gate review receives:

1. exact revision, working-tree boundary, build options, shader compiler/generation, backend/device/driver, and fixture hashes;
2. current raw lobe/reservoir/visibility captures demonstrating the reported failure without presentation transforms;
3. unit/BRDF analytic results and exhaustive small-light comparison;
4. current estimator equation-to-code audit, including random dimensions and packed precision;
5. temporal mutation and stable-light-identity results;
6. visibility frontend parity/fault results;
7. reconstruction input/output and disocclusion study;
8. workload quality/time/memory baseline with uncertainty;
9. accepted decision table, revised stage estimates, hook/deletion ledger, and named owners;
10. independent review stating whether Stage 1 may begin.

## Discovery Failure Modes

| ID | Failure | Gate response |
| --- | --- | --- |
| `FM-DIR-D0-01` | tone-mapped screenshot is the only reproduction | remain open; require raw products and exact capture identity |
| `FM-DIR-D0-02` | RTXDI/MegaLights/source paper is treated as local proof | remain open; classify it as precedent |
| `FM-DIR-D0-03` | multiple estimator hypotheses change together | split experiments until one claim is falsifiable |
| `FM-DIR-D0-04` | Reference Path Tracer shares the questioned equation/code | mark comparison dependent and retain analytic/independent oracle |
| `FM-DIR-D0-05` | budgets are copied from another engine/GPU | leave budget unresolved and measure Sparkle |
| `FM-DIR-D0-06` | proposed architecture requires duplicate scene/light/material ownership | reject shape and redesign through existing owners |
| `FM-DIR-D0-07` | release gate does not admit feature work | keep plan staged and blocked; do not relabel research as implementation authority |

## Exit Criteria

`DIR-D0` passes only when all fourteen decisions have owners, immutable reviewed artifacts, and explicit dispositions; every `AC-DIR-*`, `FM-DIR-*`, and `CHK-DIR-*` maps to a stage; the first production slice has a bounded hook/deletion ledger; and the applicable release owner authorizes execution. Unanswered questions stay `Open`; unavailable evidence is `Blocked`, never assumed.

## Gate Decision

**Current decision: Open / Blocked for production replacement.** This documentation pass establishes the work required to close discovery. It does not authorize code changes or prove why the current picture is broken.

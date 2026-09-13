# Volumetric Lighting, Fog, Atmosphere, And ReSTIR Staged Plan

**Status:** implementation-ready conditional plan; all production stages are blocked until `REL-11` unlocks new features and `VOL-D0` is accepted

**Responsibility:** own the post-admission dependency order, stage scopes, estimates, non-goals, gates, copy-ready prompts, optional-tier decisions, and stop rules

**Authority boundary:** this plan consumes accepted Strategy/Discovery/Semantics/Architecture/User Experience and cannot admit or redefine them; code/build proves implementation and a future assigned FCR owns results

**Plan revision:** `VOL-PLAN-02`, deepened 2026-09-13 from repository revision `8b650c7450f8a59fb3bcc18edbb4d217a7b11ed5`

## Outcome

Move from a truthful 0/100 negative capability to one physically grounded, production-usable volume system in reviewable slices: analytic medium truth, basic fog composition, unified local/media/light froxels, temporal reconstruction, physical atmosphere, heterogeneous volumes, and only then a measured Volumetric ReSTIR tier. Clouds remain a separate admitted culmination.

## Mandatory Gate Order

```text
REL-11 close + explicit new-feature Go
  -> VOL-D0 + assigned FCR
  -> VOL-1 semantic core and first fog composition
  -> VOL-2 unified local media and direct lights
  -> VOL-3 temporal product
  -> VOL-4 physical atmosphere and environment bridge
  -> VOL-5 heterogeneous content and reference tracking
  -> VOL-6 Volumetric ReSTIR experiment/product decision
  -> VOL-7 clouds only if separately admitted
  -> VOL-8 backend/workload/adoption/FCR closure
```

Until Stage 1 creates an end-to-end admitted slice and updates inventories, `REN-E24` and `AC-VOL-NEG-*` remain authoritative.

## Planning Envelope

| Stage group | Provisional estimate after admission | Main uncertainty |
| --- | ---: | --- |
| Stage 0 discovery | 8–15 engineering days | product scope, content owner, budgets, reference tooling |
| Stage 1 first fog slice | 10–20 days | new authored-to-frame path and composition edge |
| Stages 2–3 unified froxel/temporal | 18–35 days | grid format, light/shadow scaling, motion quality |
| Stage 4 atmosphere | 15–30 days | LUT/reference/environment integration |
| Stage 5 heterogeneous | 15–35 days | cooker/storage/tracking/majorant/content fixture |
| Stage 6 ReSTIR volume | 20–45 days | path-space estimator, reuse, performance viability |
| Stage 7 clouds | 25–60+ days | authoring/weather/detail/lighting/temporal product scope |
| Stage 8 evidence | 10–25 days | backend/hardware/content/reference availability |

These ranges are deliberately broad and start only after `REL-11`. Stage 0 re-estimates from experiments. Volumetric ReSTIR or clouds may be rejected without invalidating a completed fog/atmosphere product.

## Universal Execution Contract

Every stage:

- verifies current revision/status, guidance, owners/producers/consumers/lifetime/build/package surfaces, and concurrent changes;
- follows Change Integration, Change Lifecycle, Renderer, Module Ownership, Data And Memory, Documentation Organization, and Validation And Evidence;
- preserves the existing release gate and unrelated/user-owned changes;
- keeps state/mechanism within one private Volumetric Lighting capsule and ledgers every authored/content/scene/frame/RHI/settings hook;
- uses existing GameFramework, asset/cook, RenderScene/GPU-scene, light, sky, ray, FrameGraph, shader, reconstruction, and presentation owners directly;
- adds no generic volume framework, duplicate light/scene/content state, compatibility reader, alias, legacy path, forwarding wrapper, or permanent diagnostic subsystem;
- updates code, headers, shaders, typed bindings, registration/cooking, build/package, tests, capability docs, and feature docs together;
- runs the cheapest claim-falsifying checks and never upgrades status from static/build/responsive-process evidence alone;
- stops when admission, math, units, content rights, backend capability, oracle, threshold, budget, or failure behavior is unresolved.

New test-only production additions may be local-only but must not be submitted; temporary probes are removed before handoff.

## Stage Delivery Contract Matrix

This matrix is normative. Stage 0 replaces owner surfaces with exact source/header/shader/CMake/registration/generated/content/package files and the accepted hook budget before any production stage.

| Stage | Prerequisites | Required production delta and deletion | Explicit non-goals | Retained deliverables | Smallest stage falsifier |
| --- | --- | --- | --- | --- | --- |
| 0 | approved `REL-11` closeout/new-feature Go; current negative audit; product/content/platform/FCR reviewers | discovery/reference prototypes only; no reachable production vocabulary; remove probes | authoring types, passes, shaders, selectors, readiness promotion | admission record, accepted decision/tier matrix, UX dry run, oracle/content-rights/profile/threshold table, exact hooks/files/build map, estimates/review | admission/FCR absent or any Stage-1 unit/composition/owner/UX/threshold decision open |
| 1 | Stage 0 authorizes `VOL-Q1`; accepted coefficients/phase/height/light/composition and first-use contract | add one authored global medium through serialization/world publication/RenderScene/preparation/feature pass/raw products/compose/status; update negative capability for exact slice | local media, history, atmosphere, heterogeneous assets, ReSTIR, clouds, generic framework | authoring round trips, CPU/shader analytic cases, raw `T/Lscatter`, composition/Off/invalid artifacts, hook/removal and package map | no exact Beer/phase/composition agreement, invalid edit partially publishes, or Off is not graph/compose identity |
| 2 | Stage 1 pass; frozen grid/format/overlap/capacity/light/shadow contracts | add one volume-owned froxel coefficient/light/integration graph and admitted local shapes through existing authoring/scene/light/ray hooks; delete temporary Stage-1 integration path if replaced | history, atmosphere, heterogeneous assets, volume path reservoirs | grid inverse/boundary, overlap/capacity, per-light/visibility/transmittance, convergence/scaling time/memory and raw slice artifacts | arbitrary volume/light loss, surface reservoir reuse, wrong segment semantics, or grid error exceeds threshold |
| 3 | Stage 2 static pass; accepted jitter/reprojection/history/confidence/UX and replacement-memory budget | add per-View immutable previous/current volume histories, reconstruct/publish/retire path and status; delete any temporary accumulation | atmosphere, heterogeneous assets, ReSTIR, learned provider | camera/density/emission/light/shadow/resize/dual-view/reload/cancel matrix, raw/reconstructed lag/leak/detail/time/memory | stale/cross-view/partial/non-finite history publishes or mutation recovery exceeds bound |
| 4 | Stage 3 pass; atmosphere model/LUT/reference/sky/environment decisions accepted; cross-feature hooks reviewed | add atmosphere authoring/preparation, atomic LUT group, physical sky/sun/aerial perspective, image-vs-physical mode and one environment generation; replace conflicting image-sky write in that mode | clouds, arbitrary planets/lights, heterogeneous volume, second sky owner | raw LUT/query and rendered reference cells, generation/failure tests, direct/indirect environment identity and double-count proof, UX | mixed LUT/environment generation, double sun/sky/transmittance, or reference cell misses threshold |
| 5 | Stage 4 pass where required; exact dense-or-sparse content schema/rights and tracker/majorant contracts | add one import/cook/runtime lease/upload/density sampling path, fixed-march product and stochastic reference; delete source runtime parser/duplicate canonical forms/failed acceleration | both dense and sparse canonical paths, ReSTIR, clouds | provenance/cook/load/reload/malformed results, transform/filter/range/majorant tests, march convergence/tracker statistics, memory/upload/retirement | majorant violation hidden, missing asset succeeds, representations diverge, or product does not approach reference |
| 6 | Stage 5 reference pass; exactly one froxel-light or path-space reservoir domain admitted with equal-time baseline | implement explicit volume sample/proposal/mapping/GRIS/history and final evaluation in volume owner; remove experiment completely if gate fails | borrowing surface reservoir, conflating light selection with path ReSTIR, clouds, unapproved multiple scattering | equation/code ledger, record/packing, shift/support/statistics, approximate-vs-final artifacts, mutation/correlation/error/time/memory A/B, adoption/rejection record | exact final estimator absent, false-zero support, no equal-time win, unsafe mutation, or required regression/budget fails |
| 7 | separately approved cloud product/content/UX/budget record; Stages 4/5 required foundations pass | extend the same atmosphere/medium/content/light/history/composition owners with one cloud tier; delete failed prototype/assets/settings | parallel cloud renderer/sky/fog state, unrelated surface-sky changes, cloud admission by visual appeal | weather/density authoring/cook, scale/LOD/empty-space/light/shadow/temporal cases, close/inside/horizon/flight A/B, UX/time/memory | no product/content owner, duplicate architecture, unacceptable fly-through/history behavior, or budget failure |
| 8 | exact included tier candidate, devices/backends/content/reference/rights and FCR owner ready | evidence/status/docs/package closure only; fixes require new candidate; remove stale negative or unsupported positive claims exactly | redesign/tuning during evidence, inheriting proof between tiers/backends, unrelated fixes | per-tier FCR, analytic/workload/raw/composed/reference, temporal/failure/backend/native validation, authoring/automation/accessibility/cook/package/time/memory/enclosure results | any included AC/FM/CHK lacks candidate-bound result or absent/deferred tier is presented as supported |

## Required Stage Handoff

Every stage reports start/end revision and dirty boundary; release/discovery prerequisite evidence; changed/deleted files by authored/content/scene/render/shader/build/package responsibility; hook and bounded-removal ledgers; semantic-rule mapping; exact commands/configurations/raw artifacts/results; content/code provenance; cleanup; every unrun check; remaining tier limits; and binary authorization for the next stage. Fog, atmosphere, heterogeneous, ReSTIR and cloud verdicts remain independent.

## Copy-Ready Prompt Contract

Every stage prompt is the local quotation plus this mandatory tail; copy both:

> NON-NEGOTIABLE: verify release/admission and the stage matrix prerequisites plus accepted Discovery/Transport/Architecture/User Experience contracts before editing. Keep all volume algorithms/state/content generations/LUTs/froxels/histories/reservoirs/diagnostics/failures in the frozen feature enclosure; generic owners only perform ledgered authoring, cook, scene, light/environment, frame, RHI, settings and package hooks. Preserve raw analytic/reference products, execute the listed clean break, and do not invent a unit, coefficient, composition order, grid, tracker, majorant, estimator, identity, lifetime, fallback, public control, threshold, budget, provider, content format, compatibility path, wrapper or tier.
>
> STOP: report `BLOCKED` before production mutation if `REL-11`/admission/FCR or any prerequisite/equation/oracle/threshold/owner/rights/capability is absent, evidence contradicts the contract, concurrent work overlaps the owned boundary, or scope exceeds the one stage. Handoff quotes every deliverable/exit gate with candidate-bound proof, lists hooks/deletions and exact checks/artifacts/results/provenance/cleanup/unrun work, and states whether the next stage is authorized.

## Stage 0 — Obtain Admission And Close `VOL-D0`

### Objective

Convert the research target into an approved post-release product slice with exact semantics, ownership, budget, and evidence identity.

### Work

1. Wait for approved `REL-11` closeout and explicit new-feature Go; record it.
2. Execute `VOL-X-01` through `VOL-X-09` as applicable using bounded prototypes/reference scripts, not production architecture.
3. Close `VOL-D0-01` through `16`, including exact `Required/Deferred/Excluded` scope.
4. Assign an FCR ID/owner and freeze candidate/reference/artifact/threshold/backend/workload contracts.
5. Freeze Stage-1 source files, external hooks, deletion/preservation ledger, checks, and revised estimate.
6. Obtain independent gate review.

### Exit Gate

- `REL-11` and explicit admission are evidenced.
- All decisions needed by Stage 1 are accepted; later cells may remain deliberately deferred.
- Negative capability docs are scheduled for update only with the end-to-end Stage-1 implementation.

### Ready-To-Use Prompt

> Execute Volumetric Lighting Stage 0 from `VolumetricLighting/Plan.md`. First verify that `REL-11` has an approved closeout and new-feature work is explicitly unlocked; if not, stop with a blocker and perform no production change. Run the bounded `VOL-X-*` discovery experiments, freeze every `VOL-D0-*` decision needed by the admitted first slice, assign the FCR owner, budgets, thresholds, backend/workload cells, and exact hook/deletion ledger, then obtain independent review. Do not raise readiness or implement the renderer feature.

## Stage 1 — Deliver Analytic Medium Truth And First Fog Composition

### Objective

Prove one end-to-end physical medium from authored intent to scene-linear composition with no temporal/history complexity.

### Work

1. Add the accepted minimal global homogeneous/height fog authored type, serialization/defaults, RenderScene delta/record, and prepared immutable value.
2. Implement one `MediumCoefficients` semantic kernel and CPU/shader Beer, homogeneous single-scatter, height-density, phase, finite/invalid tests.
3. Implement the smallest accepted per-pixel/froxel current-frame integration with one directional light and explicit geometry/medium visibility scope.
4. Publish raw premultiplied in-scatter/transmittance and compose once with opaque surface/image-sky background before display mapping.
5. Add requested/active/off/rejected status and raw capture through existing routes.
6. Update capability/readiness docs from negative only to the exact source-present stage without claiming visual/backend/release proof.

### Exit Gate

- `AC-VOL-01/02/03/06` and `CHK-VOL-01/05` pass for the frozen first domain.
- Off is exact graph omission/identity; invalid inputs reject before scene publication.
- No local volume, history, atmosphere LUT, heterogeneous asset, reservoir, cloud, or transparent-media claim exists.

### Ready-To-Use Prompt

> Implement Volumetric Lighting Stage 1 only after its gates are evidenced. Deliver the accepted minimal global homogeneous/height fog from GameFramework intent through RenderScene/preparation to one current-frame directional single-scatter integration and raw premultiplied in-scatter/transmittance composition. Add analytic Beer/phase/height and invalid-input checks, explicit Off/active/rejected status, and update exact capability docs. Do not add temporal history, local volumes, atmosphere, VDB, ReSTIR, clouds, or a generic volume framework.

## Stage 2 — Establish Unified Froxels, Local Media, And Direct Lights

### Objective

Extend the proved medium into one bounded froxel owner supporting local media and all admitted analytic lights/shadows.

### Work

1. Implement the frozen grid mapping/formats and inverse/boundary tests.
2. Add admitted local shape components, scene/prepared records, deterministic overlap, capacity/overflow, transforms, and update generations.
3. Inject resolved coefficients once; add deterministic/clustered/stochastic direct light strategy selected by `VOL-D0-07` with shared light semantics and own receiver-domain state.
4. Integrate geometry shadows and medium light transmittance under separate explicit contracts.
5. Preserve raw coefficient/density/light source/in-scatter/transmittance evidence using temporary or existing generic capture paths.
6. Measure memory/bandwidth/time as grid, volumes, optical depth, and lights scale.

### Exit Gate

- `AC-VOL-04/05/06`, `FM-VOL-03/08`, and `CHK-VOL-02/03/08` pass static cells.
- exact capacity and first-overflow behavior is visible;
- no surface reservoir or duplicated light store is used.

### Ready-To-Use Prompt

> Implement Volumetric Lighting Stage 2 from the accepted froxel/light contract. Add the tested grid mapping, local media shapes, deterministic coefficient overlap, all admitted analytic light injection, geometry shadow and medium transmittance, and bounded capacity/status. Reuse immutable existing light semantics/distributions only through the frozen bridge; keep volume receiver state local. Capture raw intermediates for tests, report scaling memory/time, and do not add temporal reconstruction, atmosphere, heterogeneous assets, or path-space ReSTIR.

## Stage 3 — Make The Froxel Product Temporally Stable

### Objective

Add per-View jitter/reprojection/reconstruction that responds correctly to camera, density, light, depth, and configuration changes.

### Work

1. Implement the frozen jitter and reprojection domain, confidence/responsive masks, depth/medium boundary validation, neighborhood clamp, and bounded history.
2. Maintain distinct immutable previous and current histories; publish transactionally and retire by submission.
3. Exercise cuts, subpixel/rapid motion, disocclusion, thin shafts, foreground edges, local volume transform/density/emission, light/shadow change, resize, dual Views, reload, profile/provider change, cancel, and device failure.
4. Compare raw/current-only and reconstructed transmittance/in-scatter; measure lag, leaks, detail, memory, and time.
5. Remove temporary diagnostic state before submission.

### Exit Gate

- `AC-VOL-07/12`, `FM-VOL-04/05`, and `CHK-VOL-04/05/10` pass for admitted cells.
- no cross-view, partial, stale, or non-finite history is published.
- first frame and Off transitions are deterministic.

### Ready-To-Use Prompt

> Implement Volumetric Lighting Stage 3 after static froxel correctness. Add the accepted per-View jitter, reprojection, compatibility/confidence, responsive handling, clamp, bounded history, transactional publication, and retirement. Run the full camera/depth/density/emission/light/shadow/resize/dual-view/reload/provider/cancel matrix and compare raw versus reconstructed `T/Lscatter`. Do not add atmosphere, heterogeneous assets, or ReSTIR in this stage.

## Stage 4 — Deliver Physical Atmosphere, Aerial Perspective, And Environment Bridge

### Objective

Replace the background-only gap with an explicit physical-atmosphere mode that shares one generation across sky, aerial perspective, sun, and surface environment sampling.

### Work

1. Add accepted atmosphere authored/scene/prepared records and validated physical parameter ranges.
2. Implement transmittance, multiple-scatter, sky-view, and aerial-perspective LUT generation with immutable all-or-nothing publication.
3. Add selected background mode and sun disk/atmosphere-light contract; retain image environment as an explicit separate mode.
4. Publish one `EnvironmentGeneration` mapping/radiance/PDF to Direct/Indirect Lighting without circular history or double counting.
5. Compose local fog/atmosphere/opaque depth/sky according to the frozen order.
6. Compare analytic and Bruneton/Hillaire reference cells over horizon, sun, altitude/space, ground, parameter changes, and exposure-independent raw values.

### Exit Gate

- `AC-VOL-08/09`, atmosphere portions of `CHK-VOL-05/10`, and mutation/failure cells pass.
- no mixed LUT generation, double sun/sky, or stale aerial perspective is observable.
- image and physical sky modes have exact requested/active behavior.

### Ready-To-Use Prompt

> Implement Volumetric Lighting Stage 4. Add the frozen atmosphere records and atomic LUT generation, physical sky/sun/aerial perspective, explicit image-versus-physical background selection, and one immutable environment generation consumed by Direct/Indirect Lighting. Prove raw radiance/transmittance against the accepted analytic/Bruneton/Hillaire cells, including horizon, ground, altitude/space, edits, and failure. Preserve one composition path and prevent sky/sun/transmittance double counting.

## Stage 5 — Add Heterogeneous Media And Reference Tracking

### Objective

Deliver the admitted dense/sparse density content path and a trustworthy stochastic reference for heterogeneous transport.

### Work

1. Implement the accepted source-import/cook schema (dense first unless sparse is explicitly admitted) with transform-to-metres, channel/range/filter/compression/bounds/majorant/content hash.
2. Add authored asset reference, RenderScene/prepared content-generation lease, GPU upload/reload, and missing/corrupt/unsupported behavior.
3. Implement fixed-march production sampling and accepted delta/ratio/residual tracking reference with majorant validation and statistical tests.
4. Add empty-space/hierarchical acceleration only after measured baseline.
5. Exercise thin/high-frequency density, empty space, boundary transforms, extreme optical depth, emission, moving volumes, reload/retirement, and the admitted VDB workload if available.

### Exit Gate

- `AC-VOL-10`, `FM-VOL-01/02/06`, and `CHK-VOL-02/06` pass.
- production marching error converges toward independent/tracking reference as resolution/steps increase;
- runtime owns no source parser or duplicate canonical dense/sparse copy.

### Ready-To-Use Prompt

> Implement Volumetric Lighting Stage 5 for the exact density representation admitted by `VOL-D0`. Add one validated cooked asset path, transform/units/channel/range/filter/bounds/majorant semantics, generation-safe scene/GPU lifetime, fixed-march product sampling, and the accepted stochastic tracking reference. Test missing/corrupt/reload, thin/high-frequency/empty/extreme/emissive/moving cases and majorant violations. Do not add both dense and sparse canonical forms or Volumetric ReSTIR yet.

## Stage 6 — Evaluate And, If Admitted, Deliver Volumetric ReSTIR

### Objective

Determine whether reservoir-resampled volume paths or per-froxel light reservoirs provide a justified quality/time improvement, then ship only the winning admitted domain.

### Work

1. Freeze A/B reference, path domain, light/media content, metrics, ray/tracking/step budget, and memory before implementation.
2. Build exact volume path sample, free-flight/transmittance proposal, shift/inverse/Jacobian/support, generalized contribution/resampling weight, target, effective count, and history generations.
3. Keep cheap approximate candidate evaluation distinct from accepted final selected-path evaluation.
4. Add temporal/spatial reuse with density/light/camera/majorant/content mutation and correlation measurement.
5. Compare equal-time error and motion behavior against Stage-5 tracking and Stage-3/5 product baselines for direct many-light, environment, emission, and admitted multiple scattering.
6. If the need is only many-light froxel selection, implement/name that smaller estimator and do not claim path-space Volumetric ReSTIR.
7. Remove rejected experiments entirely.

### Exit Gate

- `AC-VOL-11`, `FM-VOL-07`, and `CHK-VOL-07/08` pass for the exact claim;
- quality/time/memory improves the admitting workload without violating required regression cells;
- active bias/correlation/final-evaluation semantics are visible;
- a rejected experiment leaves no production residue.

### Ready-To-Use Prompt

> Execute Volumetric Lighting Stage 6 for the single estimator domain admitted by `VOL-D0-13`. Freeze equal-time A/B metrics and budgets first. Implement the exact volume path or froxel-light sample, proposal, shift/inverse/Jacobian/support, GRIS/reservoir weight, current generation identity, and separate approximate-candidate versus final evaluation. Run analytic/statistical/reference and camera/density/light/environment/emission/majorant mutation tests. Ship only if it beats the accepted baseline; otherwise remove it and record rejection.

## Stage 7 — Admit Clouds As A Separate Product Increment

### Objective

Only if `VOL-D0-14` is separately approved, extend the existing atmosphere/heterogeneous system to a production cloudscape without another volume architecture.

### Work

Freeze weather/density authoring, scale/coverage, procedural detail, bounds/LOD/empty-space, direct/multiple-scatter approximation, atmosphere/sun/environment coupling, ground/cloud shadows, temporal reconstruction, and close/fly-through workload. Integrate through existing medium/content/LUT/light/history/composition owners. Add exact authoring/adoption/failure and quality/time/memory evidence.

### Exit Gate

The separately assigned cloud acceptance set passes on approved workloads/backends and no duplicate sky/fog/light/history system appears. If no content/UX/budget owner exists, keep clouds deferred.

### Ready-To-Use Prompt

> Execute Volumetric Lighting Stage 7 only with a separately accepted cloud admission record. Extend the existing atmosphere and heterogeneous medium owners with the frozen weather/density/detail/LOD/lighting/shadow/temporal contracts; exercise close, inside, below, above, horizon, fast flight, time/light/weather change, and failure cells; and report raw/reconstructed quality, time, and memory. Do not create a parallel cloud renderer or change unrelated surface sky semantics.

## Stage 8 — Backend, Workload, Adoption, And Evidence Closure

### Objective

Validate every included tier against its exact product claim and publish the assigned FCR without inheriting proof across tiers.

### Work

1. Freeze candidate/build/shader/backend/device/driver/content/reference/settings/profile identities and thresholds.
2. Run analytic volume controls, Cornell slab/box, Sponza fog, Bistro exterior/interior light/atmosphere cells, San Miguel held-out, animated/failure/extreme optical-depth, heterogeneous/VDB, ReSTIR, and cloud cells only where included.
3. Run supported D3D12/Vulkan native validation and capability faults.
4. Retain raw coefficients/density, light source, transmittance, in-scatter, confidence/history, atmosphere LUT/environment identity, reconstructed/composed output, reference/error statistics, timing distributions, and peak/history/replacement memory.
5. Validate authoring/save/load/reload, requested/active/degraded/error UX, cook/package, generated surfaces, enclosure/hooks, stale links, scoped diff, and `git diff --check`.
6. Update the assigned FCR and readiness/capability maps only from observed evidence; retain excluded/deferred tiers explicitly.

### Exit Gate

- `AC-VOL-01` through `AC-VOL-15` have applicable explicit dispositions;
- every included tier has its own semantic, temporal, visual, performance, memory, backend, failure, adoption, and release evidence;
- absent/deferred features remain truthful and no negative/current claim is stale.

### Ready-To-Use Prompt

> Execute Volumetric Lighting Stage 8 as candidate validation for the exact included tiers. Freeze candidate/configuration/content/reference identities and metrics before runs. Exercise analytic/Cornell, Sponza, Bistro, San Miguel, motion/failure/extreme, and only the admitted heterogeneous/ReSTIR/cloud cells on supported D3D12/Vulkan profiles with native validation. Retain raw through composed artifacts and separate semantic, temporal, visual, performance, memory, backend, adoption, and release verdicts. Update the assigned FCR/readiness only from observed evidence.

## Stage Traceability

| Stage | Acceptance | Checks |
| --- | --- | --- |
| 0 | scope and `AC-VOL-15` prerequisites | `VOL-X-*`, discovery failures, `CHK-VOL-NEG-01` |
| 1 | `AC-VOL-01/02/03/06/12/14` foundation | `CHK-VOL-01/05/11` |
| 2 | `AC-VOL-04/05/06/12/14` | `CHK-VOL-02/03/08/11` |
| 3 | `AC-VOL-07/12` | `CHK-VOL-04/05/10` |
| 4 | `AC-VOL-08/09/12/14` | `CHK-VOL-04/05/10/11` |
| 5 | `AC-VOL-10/12/14` | `CHK-VOL-02/06/10/11` |
| 6 | `AC-VOL-11/12/13/14` | `CHK-VOL-07/08/09/10/11` |
| 7 | separately assigned cloud criteria | cloud workload/adoption/backend evidence |
| 8 | `AC-VOL-13/14/15` and all included closure | `CHK-VOL-09/10/11`, assigned FCR |

## Stop Rules

Stop when `REL-11`/admission is absent; coefficients/composition/reference are unresolved; a source asset lacks rights/schema/consumer; a required backend capability is missing; history/LUT/content cannot publish transactionally within memory; Volumetric ReSTIR lacks an exact final estimator; paper values substitute for local budgets; clouds/transparency expand the stage; or concurrent work overlaps the owned boundary. Preserve the last valid tier and report the blocker without relabeling it complete.

## Completion Rule

Only the assigned candidate report can close an included tier. The package may validly finish with fog/atmosphere accepted and Volumetric ReSTIR/clouds rejected or deferred. Documentation, a shader, an attractive fog screenshot, or a paper comparison alone is never completion.

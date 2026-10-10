# Direct Lighting Discovery Gate

**Gate:** `DIR-D0`

**Status:** **Open / production replacement blocked**; research and source discovery may proceed, but Stage 1 implementation requires this gate plus the applicable release/admission decision

**Responsibility:** own unresolved direct-lighting decisions, experiments, risks, gate evidence, and the binary authorization for implementation

**Authority boundary:** research supplies precedent; this page alone freezes `DIR-D0`; semantic/architecture pages describe the candidate; the Plan cannot choose an unresolved decision; code and `FCR-REN-06` own implementation and results

**Prepared:** 2026-09-12 at revision `8e4ffba225411965dc51c0b783e5f47a075c7e84`

## Gate At A Glance

**Additional source-use route:** [reference cards `NVR-02/03/17`](../../../../../../../Strategy/Research/RenderingReferenceExamples.md) pin the RTXDI surface/light bridge, temporal resampling, NRD guides and production/course precedents. Use them in `DIR-D0-05..08/10/11` to derive the actual local proposal/target/PDF/weight, current/previous identity, guide encoding and quality/cost falsifiers. Record exact file/assumption, differences and rights in the decision artifact. They do not close this gate, select RTXDI as the product, prove unbiasedness or authorize a new framework/dependency.

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

## Decision Closure Register

Every row is independently reviewed. `Proposed` means the documents contain a candidate, not that the decision is closed.

| Decision | Options that must be compared | Required retained evidence | Decision owner/reviewer | Status | Consequence while open |
| --- | --- | --- | --- | --- | --- |
| `DIR-D0-01` | first-release repair, later release, or excluded profiles; D3D12/Vulkan and traversal matrix | product/release admission and feature/profile table | Renderer owner / release owner | Open | no production replacement scope |
| `DIR-D0-02` | explicit photometric-to-working conversion and shape models | dimensional derivation, analytic light matrix, importer/authoring trace | lighting semantics / independent math reviewer | Open | no unit or reference comparison claim |
| `DIR-D0-03` | current versus corrected lobe energy, GGX sampling/energy variants | BRDF evaluation/PDF/furnace sweeps and per-lobe allocation | material-lighting semantics / RPT reviewer | Open | no reservoir target or lobe acceptance |
| `DIR-D0-04` | CPU analytic, exhaustive GPU, accepted RPT, external artifacts | oracle manifest, shared-code analysis, tolerances before results | verification owner / independent reviewer | Open | no visual-correctness root-cause verdict |
| `DIR-D0-05` | stable logical ID plus translation or full reset | add/remove/reorder/mutation captures and packing/round-trip tests | prepared-light owner / lifetime reviewer | Open | no temporal reuse across light changes |
| `DIR-D0-06` | uniform, power mixture, cluster/BVH, ReGIR, environment/emissive proposals | normalization/support/variance/time/memory study | Direct sampler owner / estimator reviewer | Open | initial proposal remains unratified |
| `DIR-D0-07` | reuse off, canonical correction modes, visibility-reuse choices, advanced pairwise/MCMC | equation-to-code table and enumerated/statistical reservoir tests | reservoir owner / independent estimator reviewer | Open | no ReSTIR conformance claim |
| `DIR-D0-08` | reprojection, compatibility, neighbor/disocclusion policies | mutation matrix, rejection reasons, diversity/correlation/recovery | reuse owner / motion reviewer | Open | surviving current history remains unproved |
| `DIR-D0-09` | Inline, Pipeline and any separately admitted non-ray provider | finite-segment/alpha/two-sided parity and injected capability failures | ray semantics / RHI reviewers | Open | provider support matrix cannot be promoted |
| `DIR-D0-10` | portable signal/filter plus optional DLSS RR | raw/guide contract, motion/disocclusion A/B and failure behavior | reconstruction owner / product reviewer | Open | no product-quality reconstructed claim |
| `DIR-D0-11` | measured profile budgets and queue placement | fixed workload quality/rays/time/memory/replacement curves | performance owner / platform reviewers | Open | no default profile or performance promise |
| `DIR-D0-12` | recommended/strict settings, status/reasons, debug/capture route | [UX](UserExperience.md) dry run, automation/accessibility/failure matrix | editor/runtime owners / artist and automation reviewers | Open | no durable public workflow |
| `DIR-D0-13` | private feature home, exact hooks/deletions, bounded oracle retention | source/build graph, hook ledger, bounded-removal and stale-symbol audit | Renderer architecture / module owner | Open | Stage 1 file scope not authorized |
| `DIR-D0-14` | metrics, masks, fixtures, hardware/backends and artifact schema | frozen candidate manifest, acceptance/check trace and FCR owner review | verification / `FCR-REN-06` owner | Open | no candidate may receive a pass verdict |

## Gate Resolution Protocol

### Debug-Controls Baseline Dependency

The Debug Views admission audit at `32660b5ca030dc5f46d1888d96f6aabcc5ec87e5` exposes two concrete `DIR-D0-03` failures. Their independent scalar counterexamples are retained in the candidate-bound `FCR-REN-11` admission report; they are not rendered-image or shader-execution results.

- `SurfaceLighting.hlsli::EvaluateDirectLightWithF0` implements `evaluateSubsurface = false` by passing strength zero into `BRDF::Direct::Evaluate`. The default `BRDF_SUBSURFACE_WRAP` interprets strength as wrap width, not amplitude. At `NoL = 1`, base color and subsurface color both `0.5`, its response is `0.25 / pi = 0.07957747154594767`, even with strength zero. A zero-strength argument is neither zero response nor skipped evaluation.
- The unchanged additive base layer fails even a conservative white-furnace bound. For white nonmetal, white subsurface, `F0 = 0`, `NoV = 1`, roughness zero and wrap width one, wrap hemispherical reflectance is `5/6`. Burley diffuse times Schlick's retained diffuse factor has reflectance at least `0.5 * (1 - (1 - sqrt(0.5))^5)`. Their sum exceeds `1.3322555869` before any specular contribution. This is a counterexample to the base-budget rule, not a full furnace sweep or an energy-conservation claim for other models.

**Required resolution:** material disablement must branch before subsurface evaluation; width cannot impersonate amplitude. Ratify the diffuse/subsurface allocation under [Surface Lobe Contract](SamplingAndShading.md#surface-lobe-contract), update the shared evaluator and every consumer together, and establish a corrected raw baseline before freezing Debug Views all-on equivalence. A feature-off action must not reassign the disabled authored lobe's budget to another lobe. Fixing these material semantics is owned by `DIR-1/2` in [Plan](Plan.md), not by registering a Show CVar or silently changing allocation inside DVP-4A-2.

**Bounded inspection/repair surface:** `BRDF/BRDF.hlsli`, `BRDF/Subsurface.hlsli`, `Lighting/SurfaceLighting.hlsli`; inspect their direct reservoir, direct resolve and shared hit-lighting callers. No new CVar, public API, frame state, provider, shader variant or feature framework is required to express the material decision. Any caller/parameter change must be ledgered before production admission. Existing `DIR-D0` gate requirements remain binding; this finding does not authorize the complete Direct Lighting replacement.

**Admitted bounded defect repair under the user's prerequisite-resolution and GBuffer predicate requests:** `Passes/GBuffer/GBufferUtils.hlsli` owns `HasSubsurface(GBufferData)`; `Lighting/DirectLightReservoir.hlsli::LoadSurface` and `Passes/Lighting/Direct/DirectLighting.hlsl` query it instead of repeating the material-presence expression. The generic `BRDF/Subsurface.hlsli` evaluator rejects nonpositive strength before model evaluation without depending on GBuffer; it remains usable for secondary material hits. Existing direct callers already treat positive strength and color as prerequisites; the existing boolean false path supplies zero strength. This restores that material-disabled contract without choosing a new positive-strength allocation, changing a GPU ABI, adding a feature control, replacing a reservoir or admitting `DIR-D0` as a whole. Required checks: actual evaluator execution against the independent scalar oracle for zero/positive strength and color; focused affected-shader cook; explicit shared-hit/Reference impact disclosure. Positive material inputs must preserve their previous response. The additive-energy and physical-lobe failures remain separate open decisions.

**Invalidation:** any evaluator repair invalidates affected direct all-on baselines and shared hit/Reference comparisons. Re-run the independent furnace/disabled-material cases and affected raw shader/runtime cells before DVP consumes the new baseline. `DIR-D0-03` remains open until that proof and the owning decision pass.

**Next bounded material-budget repair:** the user's continued prerequisite-resolution request admits the exact allocation in [Surface Lobe Contract](SamplingAndShading.md#surface-lobe-contract) in `BRDF/BRDF.hlsli` and its sole `Lighting/SurfaceLighting.hlsli` caller. Return a cohesive diffuse/specular/subsurface response instead of three output arguments; pass the already existing caller evaluation boolean independently of authored strength. No new CVar, CPU state, shader binding, graph hook or cooked variant. Prove fixed authored allocation across evaluation on/off, unchanged specular and no subsurface evaluation on the off path; rerun the existing counterexample with independent scalar integration. Full raw-GPU/provider and other BRDF conformance checks remain required, not inferred from the allocation proof.

**Bounded repair observation:** at the subsequent dirty candidate based on the same revision, the two-file allocation repair is source-present. An MSVC CPU adaptation of the authored direct evaluator passes aligned independent diffuse/specular/subsurface expectations `0.24/pi`, `0.16/pi`, `0.12/pi`, authored-strength cases `0`, `0.5`, `1`, and evaluation-off with no subsurface call. The original white wrap counterexample integrates to `0.833282829` with 100,000 midpoint samples (predeclared error `0.005` against `5/6`); a compute-then-mask negative control fails. The focused shared shader cook passes DXIL/SPIR-V. Exact inputs, adaptations and limits belong to the `FCR-REN-11` baseline report, not a whole `DIR-D0-03` pass. The preceding zero-strength-only paragraph describes the earlier repair; the current caller preserves authored strength and passes evaluation intent separately. Raw GPU outputs, broader BRDF conformance and provider/history behavior remain unproved.

**Native follow-up:** the unchanged authored evaluator also passes D3D12 GPU on/off/on using uploaded runtime material values, with independent raw FP32 RGB expectations at `2e-7` on the RTX 5070 Ti Laptop GPU. A temporary atomic branch witness is inserted only in the local flattened probe source; entries are `1/0/1`, and compiled DXIL keeps exclusive subsurface arithmetic behind the branch. A compute-then-mask negative still produces zero disabled subsurface but records one entry and fails. D3D12 debug-layer error/corruption checks pass for the normal run. This is actual GPU evaluator evidence, but not the engine's GBuffer decode, reservoir selection/weights, RGBA16F outputs, production machine-ISA/dispatch omission, provider or temporal result. The current `FCR-REN-11` native-prerequisite report binds source/probe/compiler/hardware identities. `DIR-D0-03` and DVP lighting admission are not closed by this narrower result.

**Engine raw-baseline follow-up, `ITER-DVP-SHOWFLAGS-14`:** at `d29d2351614a98f101537e028267da2dc99ee9ec` plus the bounded cached-frame queue-release repair, the actual raster GBuffer, direct reservoir, shadow signal and resolve produce independently non-zero direct diffuse/specular/subsurface on D3D12 and Vulkan. The 64x64 procedural aligned-light fixture retains raw GBuffer and RGBA16F lobe bytes; independent per-pixel double-precision references cover the central 16x16 ROI. Each backend passes 2,304 RGB comparisons at the already frozen `max(2e-5, 2 * binary16 ULP(reference))` tolerance; substituting zero subsurface fails. Decoded inputs are `127/255` for base, roughness, subsurface color/strength and `10/255` for F0, rather than the authored FP32 values. Native errors fail the fixture; the initial D3D12 queue-state defect is repaired at the existing graph compiler. Existing native warnings are retained, including Vulkan descriptor-pool undercapacity. Report: `artifacts/validation/releases/v0.1.0/d29d235-dvp-engine-prerequisites-20261004/features/FCR-REN-11/completion.md`. This proves the single-light all-on raw baseline, not feature-off, multi-light/reuse conformance, provider guides, other frontends or all of `DIR-D0-03`.

The Stage-2 through Stage-4 contracts and observations below record the 2026-10-05 candidate. The 2026-10-06 parameter refactor replaces their authored Direct uniform payload with direct `SHADER_PARAMETER` values and reads trivial CVar state directly. Current binding ownership is described by [Pipeline Materialization And Typed Binding](../../ShaderRuntime/PipelineMaterializationAndTypedBinding.md#parameter-and-binding-model). The earlier native results do not validate the later binding changes.

**Accepted Stage-2 execution contract, `ITER-DVP-SHOWFLAGS-15`:** retain the corrected authored allocation and light-proposal/source PDFs. The Direct owner reads `r.Lighting.Direct.Subsurface` directly through private IsEnabled/IsActive helpers; active admission uses the existing GBuffer subsurface input and direct subsurface output handles, not a forwarded CVar value or a new frame/settings holder. Disabled intent is inactive; enabled missing required input/output or program production is a defect, not a successful zero. The existing real-time feature entry point supplies mode enclosure; Reference neither calls these helpers nor receives their CVar. Material HasSubsurface stays a GBuffer predicate independent of feature intent.

Use one 16-byte GPU cbuffer with a uint evaluation word and three padding words, prepared directly at temporal, spatial and resolve owners. Gate material evaluation before subsurface response arithmetic in both reservoir target reevaluation and final resolve; retain shared geometry/Fresnel and active diffuse/specular. Do not reallocate authored subsurface energy to another lobe when evaluation is off. Omit the disabled exclusive UAV publication; the already scheduled current-frame target clear initializes the retained fixed five-target ABI. Composition reads that deliberate disabled zero, not a mask over computed subsurface. Enabled missing production must fail. No topology change or new shader permutation is required in Stage 2. One Direct-owned hash append reads enabled intent into the existing ReSTIR invalidation identity; six reservoir histories and provider reset are conservative dependencies, while exposure is unchanged.

Budget: new private `Passes/Lighting/Direct/DirectLightingControls.h/.cpp`, `DirectLightingUniformData.h`, and shader `Lighting/DirectLightingUniform.hlsli`; existing Direct temporal/spatial parameter definitions and shader headers, `DirectLighting.cpp/.h`, the three corresponding shaders and shared `Lighting/DirectLightReservoir.hlsli`. Integration hooks are the existing `RestirLightingInvalidation.cpp` append, the six diagnostic/publication files ledgered by Debug Views, and directly required build/cook discovery membership. No public control API, extra CVar registration, CPU policy copy, frame admission branch, compatibility route or variant is admitted. The diagnostic decision is owned by [Show execution contract](../../DebugViews/Controls/ShowFlags.md).

**Stage-2 bounded implementation result, 2026-10-05:** the default-enabled Direct Subsurface CVar and this uniform route now exist. The native toggle falsified existing target initialization on Vulkan: unbound attachment clears silently retained old output. Debug Views iteration `ITER-DVP-SHOWFLAGS-16` additionally admits only `LightingTargetClear.h/.cpp` and its existing `RealTimePathTracerPasses.cpp` call to bind actual targets/scissors for their declared extents. No new clear producer, guide substitute or RHI API is introduced. The repeated D3D12/Vulkan analytic, zero-off, diagnostic, missing-enabled-product and re-enable/history oracles pass; a D3D12 GPU evaluation witness rejects compute-then-mask. Exact identity, hooks, commands, failures and limits are owned by [Bounded Show evidence](../../../../../../../Acceptance/Renderer/DebugViews.md#earlier-execution-evidence) and its FCR-REN-11 report. Shared direct dispatch, reservoir and visibility costs remain; no GPU timing, active reconstruction, general multi-light/reuse estimator or Direct family omission result is inferred. This limited Debug Views slice does not authorize the broader Direct Lighting replacement below.

**Stage-3 bounded implementation result, 2026-10-05:** the same Direct owner now registers Diffuse and Specular, extends the unchanged 16-byte ABI to three evaluation words, and derives family admission at `AddRestirDirectLightingPasses` before allocation. Targets and direct reservoir history are declared in `DirectLightingResources`; indirect history stays with its own resource implementor. Exclusive early response/write gates use the same active sum in temporal/spatial/resolve, preserving proposal/source PDFs. The shared composition/visualization owners use exactly four real-product family-presence schemas. All-direct-off removes exclusive direct allocation/reservoir/shadow/resolve work; Scene delegates semantic topology identity, Frame stores only the built identity and uses existing retirement. The [Bounded Show evidence](../../../../../../../Acceptance/Renderer/DebugViews.md#earlier-execution-evidence) owns native mixed-lobe/all-off/re-enable, diagnostic/history, GPU work-witness and D3D12 held-submission evidence and limitations. This Stage-3 result did not itself authorize the broader Direct Lighting replacement, a shadow control, active-provider or GPU-timing claim.

**Stage-4 bounded primary-shadow result, 2026-10-05:** the separately queued Debug Views slice now owns `r.Lighting.Shadows.Direct` at Shadows. Optional signal allocation/production is separated from Direct-owned reservoirs. Exactly two final Direct schemas evaluate real signal visibility or fully visible primary response with no signal binding; the shadowed consumer fails missing or unproduced input. The unoccluded selection target, source/proposal PDFs and reservoir normalization remain unchanged. [Bounded Show evidence](../../../../../../../Acceptance/Renderer/DebugViews.md#earlier-execution-evidence) owns selected D3D12/Vulkan occluder/off/re-enable/retained-intent numeric, omission and history evidence and separate Reference findings. This is not Direct Lighting replacement, Indirect Shadows, full provider/backend coverage or GPU timing.

For each row: freeze hypotheses and thresholds; capture the committed baseline and dirty boundary; run the smallest differentiating experiment; retain raw inputs/results/configuration; record why each alternative was accepted, rejected, deferred or excluded; obtain the named independent review; then update the semantic/architecture/UX owner before marking `Accepted`. A surprising result reopens the row and every dependent row. Missing hardware, provider, content, rights or oracle marks `Blocked`, never `Accepted by assumption`.

## Risk Register

| Risk | Leading indicator | Containment | Escalation trigger |
| --- | --- | --- | --- |
| visually broken source has multiple causes | analytic, visibility and temporal artifacts change independently | isolate exhaustive/unshadowed/reuse-off/raw modes | no single defect class explains captured error |
| familiar ReSTIR formula is applied in wrong measure | analytic small set passes but finite-shape/statistical mean drifts | equation-to-measure ledger and enumerated tests | confidence interval excludes reference mean |
| denoising conceals bias or stale identity | reconstructed view stabilizes while raw mean/IDs fail | raw artifacts and separate verdicts mandatory | raw and reconstructed conclusions disagree |
| advanced method expands first slice | ReGIR/VSM/MCMC/OMM fields appear before base conformance | admission matrix and one-candidate A/B stages | new owner/resource/provider required |
| fixed budget silently drops light energy | selected diversity plateaus or important light never sampled | support/overlap sweep and degraded status | any contributing admitted light has zero proposal support |
| feature diffuses into generic owners | repeated settings/scene/RHI switches appear | per-stage hook ledger and bounded-removal check | any unledgered outside edit |

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

## Required Experiments

| ID | Controlled exercise and raw products | Primary decision falsified |
| --- | --- | --- |
| `DIR-X-01` | reproduce reported defect with direct lobes, unshadowed contribution, selected stable light/sample, PDFs, reservoir decode, visibility and presented image | whether one visual symptom identifies estimator, shading, visibility, reconstruction, or composition |
| `DIR-X-02` | CPU/closed-form plus shader-query directional/point/spot/rectangle unit, attenuation, cone, shape, sidedness and invalid-input matrix | `DIR-D0-02` |
| `DIR-X-03` | BRDF evaluation/sampling/PDF histograms and furnace/per-lobe energy over roughness/metallic/F0/subsurface/shading-normal extremes | `DIR-D0-03` |
| `DIR-X-04` | exhaustive small-light GPU resolve and finite-shape quadrature against analytic/external/accepted-RPT artifacts with shared-code disclosure | `DIR-D0-04` |
| `DIR-X-05` | uniform/power/mixture/environment/emissive candidate support, normalization, selection frequencies, raw mean/variance and equal-work cost | `DIR-D0-06/07` |
| `DIR-X-06` | light add/remove/reorder/disable/move/color/shape plus camera/disocclusion/material/object/alpha/resize/dual-view/reload mutation sequence | `DIR-D0-05/08` |
| `DIR-X-07` | paired Inline/Pipeline finite segment, self-hit, grazing, opaque/alpha/two-sided/area-light fixtures and injected capability/program/SBT/TLAS/material faults | `DIR-D0-09` |
| `DIR-X-08` | identical raw sequence through portable and optional reconstruction for static, motion, cut, disocclusion, thin geometry, light toggle and invalid guides/provider | `DIR-D0-10/12` |
| `DIR-X-09` | one-to-thousands overlap plus distributed-light, broad-light, alpha and motion workloads across candidate/ray/profile/render-scale cells | `DIR-D0-11/14` and ReGIR/provider admission |

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

**Current uniform-only review revision, 2026-10-05:** the user withdraws behavior-specific shader schemas. The implementation now uses one program per operation, with fixed initialized bindings and uniform evaluation policy. Prior bounded execution results below remain candidate-bound; their allocation/omission claims do not cover this revision. [Bounded Show evidence](../../../../../../../Acceptance/Renderer/DebugViews.md#earlier-execution-evidence) owns the precise retained-resource cost and revalidation boundary.

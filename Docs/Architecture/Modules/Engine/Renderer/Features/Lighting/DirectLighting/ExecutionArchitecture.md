# Direct Lighting Execution Architecture

**Status:** target architecture pending `DIR-D0`; current source facts are revision-pinned, while target names and stage boundaries do not prove implementation

**Responsibility:** own the target Direct Lighting feature enclosure, records, resources, execution, history, providers, lifetime, failure, and integration hooks

**Authority boundary:** [Sampling And Shading](SamplingAndShading.md) owns equations; [Discovery](Discovery.md) freezes choices; [Plan](Plan.md) orders changes; current code/build and `FCR-REN-06` own implementation/evidence

**Priority:** correctness baseline → canonical reservoir replacement → temporal/visibility safety → reconstruction → many-light expansion → evidence

## Product Claim

One Lit view resolves a declared Direct Lighting profile into finite scene-linear direct lobes for the supported light/material set. Small-light results agree with an independent analytic/exhaustive baseline. The scalable path spends a bounded candidate and visibility budget, reports its active strategy, survives motion and scene mutation without stale light, and provides portable reconstruction with optional accelerated providers.

## Current Versus Target

| Concern | Current source | Target |
| --- | --- | --- |
| owner | direct reservoir, shadow, lighting, and shared ReSTIR wrappers spread through lighting pass folders | one private `DirectLighting/` feature capsule with narrow scene/frame collaborators |
| initial candidates | four uniform analytic-light samples | exact uniform oracle plus power-weighted mixtures; environment/emissive only when admitted |
| reuse | normal/depth temporal and four-neighbor spatial reuse | canonical reservoir state, stable light translation, explicit correction mode, full compatibility/confidence |
| visibility | Inline/Pipeline ray path | preserve one semantic signal; prove providers and failure states |
| reconstruction | no dedicated portable direct path; optional adjacent RR route | raw-first classical baseline plus optional RR using identical declared guides |
| validation | source inspection only | analytic, statistical, temporal, paired-provider/backend, workload, memory, and time evidence |

## Ownership Boundary

The steady-state implementation home is proposed as:

```text
Engine/Renderer/Private/Passes/Lighting/DirectLighting/
Engine/Assets/Shaders/Passes/Lighting/DirectLighting/
```

The capsule owns candidate distributions, reservoir semantics/state, direct-light frame-graph passes, selected-sample shading, feature history, confidence, and feature-local status. It does not own:

- authored GameFramework light components or serialization;
- persistent RenderScene/GPU-scene storage and lifetime;
- canonical material evaluation beyond its direct-light use;
- TLAS, hit-material, shader-table, ray-query, or backend mechanisms;
- generic FrameGraph scheduling/resource lifetime;
- the shared five-lobe composite, exposure, tone map, debug presentation, or final output;
- Reference Path Tracer transport or volumetric transport.

Use direct calls and typed products. A helper/class exists only when it owns feature state, a real policy, a required lifetime boundary, nontrivial reusable math, or multiple meaningful consumers.

## End-To-End Route

```text
GameFramework light intent
  -> RenderScene stable logical lights
  -> prepared current/previous GPU light views + translation
  -> frame-local DirectLightingRequest/Profile

GBuffer + motion + light views + history
  -> initial candidates
  -> temporal resampling
  -> spatial resampling
  -> selected-sample visibility
  -> raw direct lobe evaluation + confidence/hit distance
  -> optional reconstruction
  -> DirectDiffuse / DirectSpecular / DirectSubsurface
  -> existing LightingComposite
```

The deterministic small-light baseline enters at the same GBuffer/light/material boundary and writes the same raw lobe semantics. It is enabled only by a bounded test/developer profile or temporary validation fixture and must not become a second general lighting architecture.

## Core Records

Names are target vocabulary subject to `DIR-D0`; ownership and single-truth rules are normative.

| Record | Contents | Lifetime/owner |
| --- | --- | --- |
| `PreparedDirectLightView` | immutable current and previous light spans, stable IDs/generations, translation, distributions, capacities | frame-local view into RenderScene/GPU-scene-owned data |
| `DirectLightingProfile` | requested algorithm, quality tier, candidate/reuse/visibility/reconstruction policy | immutable resolved per View; CVar consumers resolve locally rather than forwarding raw CVar values |
| `DirectLightingHistoryKey` | view/extent/surface/light/material/shader/provider/algorithm/random-layout generations | feature-local value; no pointers or texture handles |
| `DirectReservoir` | selected sample identity, shape parameters, target, weight sum, effective `M`, validity/bias metadata | GPU feature history with integer identity fields |
| `DirectLightingProducts` | raw/reconstructed lobe handles, visibility/hit distance/confidence where admitted | frame-graph handle aggregate; no resource ownership copy |
| `DirectLightingStatus` | requested/active profile and finite reason code | bounded per-view observation through existing status path |

Light data is not copied into a feature-owned mirror. Distribution/RIS buffers may cache derived immutable generations because they are a measured GPU boundary; their key and retirement follow the source light generation.

## Pass And Resource Graph

The unfused reference shape is preferred until each edge passes:

```text
PrepareDirectDistributions (only when source generation changes)
  -> GenerateDirectCandidates
  -> ResampleDirectTemporal
  -> ResampleDirectSpatial
  -> TraceDirectVisibility
  -> ResolveDirectLighting
  -> ReconstructDirectLighting (optional)
```

Pass fusion is a measured optimization stage. It may remove transient traffic only when raw intermediate validation remains reproducible through temporary probes or deterministic kernel tests. It cannot alter estimator order, random dimensions, barriers, or requested/active status silently.

### Resource Classes

| Resource | Persistence | Required contract |
| --- | --- | --- |
| light distributions/RIS data | light-generation cache | exact source generation, PDF build rule, backend-neutral layout contract |
| current working reservoir | transient or ping-pong member | typed layout/precision and valid extent |
| previous reservoir | per-view persistent history | immutable completed generation; never written while read |
| visibility/hit distance | transient unless correction requires prior state | selected sample and traversal generation identity |
| raw lobes | current graph generation | scene-linear semantics before reconstruction/composite |
| reconstruction history | per-view persistent, separate from reservoir | denoiser algorithm/guide/pre-exposure identity |
| confidence/diagnostic counters | transient/product only when required | bounded fields; no permanent feature dashboard |

Formats and bytes are frozen by `DIR-D0-11`. A separate full-resolution texture for every scalar is rejected unless measured access and lifetime justify it; pack only semantically aligned values with tested precision.

## State And Lifetime

Each View owns at most one active Direct Lighting history generation. A frame reads only completed previous state and writes a distinct current state. Publication happens after successful graph completion; cancellation, device loss, graph rejection, or non-finite invariant failure cannot publish partial history. Replaced buffers, light distributions, shaders, pipelines, and SBT generations retire after their last GPU submission.

Resize or algorithm/layout changes allocate a complete replacement before activation. Memory budgeting counts both old and replacement generations during overlap. Dual viewports never share screen-space reservoirs or reconstruction histories even if their scene is identical.

## Invalidation And Translation

| Change | Reservoir action | Reconstruction action |
| --- | --- | --- |
| camera cut/invalid motion/extent | reset | reset |
| light transform/intensity/shape update | translate identity and re-evaluate only if ratified; otherwise reset affected samples | lower confidence or reset affected pixels/history per contract |
| light removal/reorder | explicit current/previous mapping or reject sample | reject selected-light-dependent history |
| material/normal/roughness/alpha change | compatibility rejection/generation reset | reset/reduce history according to guide identity |
| occluder/geometry/TLAS update | correction/visibility policy decides reuse; stale visibility forbidden | confidence/reset policy |
| shader/random-layout/estimator change | reset | reset |
| Inline/Pipeline switch | reset unless parity and history compatibility are proved | reset provider-sensitive history |
| denoiser-only setting change | reservoir unaffected | reset reconstruction only |

The broad whole-scene hash may remain a conservative emergency reset, but it cannot be the sole design for light translation or fine-grained adoption behavior.

## Traversal Boundary

`DirectVisibilityRequest` carries only semantic inputs: receiver geometry, selected light sample/segment, alpha/two-sided material access, and selected strict/automatic policy. The existing ray owner resolves Inline or Pipeline readiness and returns the same typed visibility product plus active provider/status. Direct Lighting does not receive native handles, device addresses, pipeline objects, SBT addresses, or backend-specific descriptors.

No visibility provider may silently return unshadowed light. Automatic chooses only a ready provider according to the frozen resolver. A future VSM or screen-trace path would require a concrete second implementation, semantics matrix, integration-ledger update, and acceptance coverage before any abstraction expands.

## Reconstruction Boundary

Reconstruction consumes raw lobe and guide products without taking ownership of light/reservoir state. A vendor-neutral implementation remains available on the mandatory hardware profile. DLSS RR may resolve as an optional provider through the existing reconstruction architecture. Requested and active provider, history validity, and bypass/rejection reason are observable.

If lobe-separated reconstruction costs exceed budget, discovery may choose a packed diffuse/specular signal or reconstruct combined direct radiance, but raw acceptance artifacts must still recover the semantic lobe comparisons required by `AC-DIR-02/03/07`.

## Backend And Shader Contract

- Feature shaders use registered typed parameter contracts and graph-tracked SRV/UAV/acceleration-structure bindings.
- D3D12 and Vulkan lower the same semantic resources through existing RHI owners; no backend-specific shader identity or device-address frontend leaks into Direct Lighting.
- Inline and Pipeline remain distinct program/dispatch shapes with shared semantic HLSL kernels where real reuse exists.
- Shader publication identity participates in history keys. Reload swaps only complete generations and retires old ones by queue completion.
- CMake recursive membership does not remove the need to inspect generated registrations/publications and package inclusion.

## Quality Profiles

Discovery freezes exact values. The architecture supports a small closed set:

| Profile | Intent | Allowed variability |
| --- | --- | --- |
| `Analytic` | deterministic/exhaustive validation | bounded fixtures only; no temporal reuse/reconstruction |
| `Balanced` | default scalable many-light product | fixed candidate/reuse/ray/reconstruction budget |
| `Quality` | lower bias/noise for review | stronger correction/additional spatial or visibility work within frozen budget |
| `Unsupported`/`Degraded` | explicit inability or reduced provider | reason code and resolved behavior; never silent |

Do not expose every research constant as a CVar. Engineer-facing diagnostics can exist during discovery; the final settings surface contains only durable product decisions.

## Integration-Hook Ledger

Expected external hooks; `DIR-D0-13` must freeze exact files before Stage 1.

| Boundary | Permitted change | Detecting check |
| --- | --- | --- |
| GameFramework/RenderScene light records | stable logical identity or admitted light semantics only | light mutation/serialization check |
| GPU-scene preparation | current/previous view, translation, derived distribution generation | capacity/translation test |
| Lit frame recipe | one direct feature invocation and typed products | graph topology/product trace |
| shared lighting composite | consume unchanged direct lobe semantics | raw-before/after composite check |
| shader registration/cooking | concrete feature shaders and dependencies | registration/publication closure |
| runtime selector/status | small profile/provider selection and active reason | strict/automatic fault matrix |
| debug/capture | route existing raw products | artifact identity check |

Any new external feature-named edit needs a row, reason, and defect-detecting check. Forwarding wrappers and duplicate settings/state are rejected.

## Clean-Break Rule

When the canonical reservoir stage lands, update all callers, history layouts, shaders, registrations, captures, and docs together, then delete the replaced reservoir representation and obsolete pass wrappers in the same change. Do not retain current and target estimators behind an internal compatibility toggle. The exhaustive baseline is not “legacy”; it is a deliberately bounded oracle with a different claim.

## Failure And Recovery

- Invalid authored input rejects before GPU publication with light ID/family/field.
- Reservoir invariant failure invalidates the affected output/history and emits a bounded reason/counter; it does not clamp into plausible radiance without a defined rule.
- Missing strict traversal or reconstruction dependency rejects or uses the frozen explicit fallback.
- Shader reload/device loss/cancel never promotes partial current history.
- Quality-budget overload reports the active budget and degraded status; no important light is deterministically discarded without declared culling semantics.

## Detailed Pass Contracts

| Pass/product | Reads | Writes | Can omit when | Primary falsifier |
| --- | --- | --- | --- | --- |
| light preparation/distribution | immutable prepared analytic/emissive/environment facts and previous light table | current compact inventory, stable-ID translation, proposal tables, generation/status | never for an active profile; exhaustive oracle may omit proposal tables | add/remove/reorder/mutate light while decoding selected stable IDs |
| exhaustive oracle | receiver GBuffer, full admitted light inventory, current visibility mode | raw direct lobes and optional per-light breakdown | shipping profile only; retained in bounded checks | analytic four-light and finite-shape quadrature agreement |
| initial candidates | receiver GBuffer, proposal tables, current lights/materials, random layout | initial reservoir and rejection counters | no-reuse oracle may write direct samples instead | discrete selection/PDF/reservoir statistics |
| temporal reuse | immutable previous reservoir/surface/light translation, current receiver/lights | temporal current reservoir/confidence | first frame, cut, invalid generation or `Off` reuse | cut/disocclusion/light-reorder matrix |
| spatial reuse | temporal/current reservoirs, current receiver/lights, neighbor sequence | final current reservoir/confidence | diagnostic initial/temporal-only mode | deterministic neighbors, incompatibility and correlation curve |
| selected resolve | final reservoir, current lights/materials, traversal provider/TLAS | raw lobes, visibility/hit-distance/sample facts | never for active stochastic profile | forced visible/blocked finite segments and frontend parity |
| reconstruction | raw lobes/guides/confidence plus separate previous filter history | reconstructed lobes and current filter history/status | raw/debug or explicit accepted bypass | identical raw motion/disocclusion/provider-fault sequence |
| composition | authoritative direct products and existing lighting inputs | one lighting composite contribution | Direct Lighting `Off`/unavailable graph path | synthetic lobe sum and graph-omission identity |

The graph declares each read/write explicitly. No pass reaches into a feature object through hidden mutable globals; console/settings values are resolved into immutable per-frame configuration before scheduling.

## Resource Access, Barriers, And Replacement Memory

| Resource class | Lifetime | Access/order rule | Replacement accounting |
| --- | --- | --- | --- |
| prepared light inventory/translation/PDFs | scene or frame generation | built completely before sampling; read-only during frame | new and old generations both count until last GPU use retires |
| receiver/GBuffer/guides | frame | produced before lighting; immutable to all direct passes | shared input, not charged as feature-owned unless feature duplicates it |
| previous reservoir/surface metadata | per-View persistent generation | SRV only during current build; never overwritten in place | previous + current + retirement overlap at resize/reconfigure |
| initial/temporal/final reservoirs | frame/transient except published final | UAV producer then declared SRV consumer; alias only when live ranges and diagnostics permit | worst active diagnostic/profile path, not optimistic alias plan |
| raw lobes/visibility/confidence | frame products | resolve writes; reconstruction/capture/composition read | all simultaneously live consumers included |
| reconstruction history/output | per-View persistent/frame | provider owns history; raw estimator never reads filtered radiance | provider replacement and previous/current generations included |

UAV ordering, state transitions, queue ownership and cross-queue synchronization are expressed through FrameGraph dependencies. Manual backend barriers in the feature are rejected unless the RHI/FrameGraph contract proves no representation exists; such an exception requires a ledgered architecture decision and paired-backend check.

## Data Access And Divergence Budget

- Candidate generation groups work by receiver and proposal class; proposal tables are contiguous read-only data for the frame.
- Final shading evaluates one selected light per reservoir. Material/light dispatch uses existing typed shader data and explicit light-class branches measured on target workloads, not a feature-specific call framework.
- Alpha visibility is permitted to diverge by content but its material/texture access is the existing ray semantic path. Duplicating material decode into Direct Lighting is prohibited.
- Random dimensions derive from View/pixel/frame/stage/candidate/neighbor semantic IDs and are versioned; no hidden global RNG state crosses passes.
- Wave/subgroup operations, half precision, pass fusion and async compute are optimizations. They require scalar/portable equivalence or an explicit backend profile, and cannot change reservoir order/meaning invisibly.

## Architecture Fitness Gate

Every stage retains an outside-feature hook ledger with file, owner, reason, data direction, lifetime, deletion/reversal method and defect-detecting check. The gate fails on an unledgered `DirectLighting` field/switch in generic Scene/View/RHI/settings/frame orchestration, duplicate light/material/history state, a second shader-binding convention, or a wrapper that only forwards a call.

Bounded-removal proof must show that deleting the private feature home plus the listed hooks restores the previous graph without leaving selectors, build entries, shaders, generated surfaces, histories, docs or package assets. Search results for feature symbols and repeated policy switches are retained as artifacts. The hook budget is re-ratified at every stage; convenience is not justification.

## Architecture Invariants

1. One authored light has one stable logical identity and one active semantic record.
2. One estimator equation maps to one owner and at least one independent test.
3. Raw direct lobes precede reconstruction, composite, and presentation.
4. Reservoir and reconstruction histories are separate authorities and invalidated independently.
5. Screen-space history is per View; GPU resources retire by completed submissions.
6. Sampling, visibility, shading, and denoising remain composable but not duplicated.
7. Provider/backend differences cannot change semantic units or failure meaning.
8. Optimization follows proof and preserves a reproducible unfused oracle path.

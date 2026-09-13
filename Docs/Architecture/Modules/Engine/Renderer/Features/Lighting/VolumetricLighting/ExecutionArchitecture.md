# Volumetric Lighting, Fog, Atmosphere, And Sky Execution Architecture

**Status:** target architecture pending `REL-11` admission and `VOL-D0`; current Renderer has no volume feature

**Responsibility:** own the target feature enclosure, authored-to-GPU route, records, pass/resources, histories, sky/environment/content/backend boundaries, lifetime, failure, and integration hooks

**Authority boundary:** [Transport And Composition](TransportAndComposition.md) owns math; strategy and [Discovery](Discovery.md) admit/freeze the target; [Plan](Plan.md) orders work; code/build and a future FCR own implementation/evidence

**Priority:** semantic/reference core → first fog slice → unified froxel/local lights → temporal product → atmosphere/environment → heterogeneous media → Volumetric ReSTIR → clouds only if admitted

## Product Claim

One View resolves authored media and sky mode into immutable prepared state; computes finite scene-linear in-scattering and transmittance; composes them once with opaque surface/background radiance; reconstructs without stale density/light; and reports exact active quality/capability. Advanced heterogeneous, reservoir, and cloud tiers extend the same medium/transport owner rather than creating parallel fog systems.

## Current To Target Route

```text
CURRENT
surface Direct/Indirect -> LightingComposite -> image Sky fill -> exposure/presentation

TARGET
surface Direct/Indirect -> LightingComposite -> selected image/atmosphere background
                                       |                    |
prepared media + lights + depth -------+-> volume integrate + compose
                                                            |
                                             scene-linear SceneColor
                                                            |
                                             exposure/reconstruction/presentation
```

The exact surface-light reconstruction/upscaling edges are frozen by `VOL-D0-08`; the invariant is that volume composition operates in compatible scene-linear/pre-exposure units before display mapping and exactly once.

## Feature Enclosure

Proposed private homes after admission:

```text
Engine/Renderer/Private/Passes/Lighting/VolumetricLighting/
  Media/          coefficient resolution, local/global injection
  Fog/            froxel lighting, integration, temporal reconstruction
  Atmosphere/     LUTs, sky background, aerial perspective, environment bridge
  Restir/         only after VOL-D0-13 admission
  Clouds/         only after separate VOL-D0-14 admission

Engine/Assets/Shaders/Passes/Lighting/VolumetricLighting/
```

Subfolders represent real mechanisms under one feature owner; they do not get duplicate medium/light/history/status frameworks. Start with fewer files and introduce a sub-owner only when the admitted stage has state, policy, or multiple meaningful consumers.

The feature does not own generic assets/import, GameFramework ECS, RenderScene/GPU-scene, light records, TLAS/ray execution, FrameGraph, shader runtime, exposure, upscaling, presentation, or surface transport. It contributes narrow types/hooks to those owners only when a stage requires them.

## Authored-To-GPU Data Flow

```text
GameFramework
  Environment/Atmosphere intent
  Global/HeightFog component
  LocalMedium component + optional cooked density asset
         |
         v
monotonic RenderScene delta with stable logical IDs/generations
         |
         v
RenderScene medium/atmosphere records + content-generation leases
         |
         v
frame-local PreparedVolumetricScene
  bounded media spans
  selected sky/atmosphere generation
  borrowed prepared light/environment/TLAS views
         |
         v
VolumetricLighting frame-graph invocation and typed products
```

Authoring types express physical/product intent only. They do not contain GPU handles, froxel coordinates, history, shader settings, native descriptors, or importer state. Imported source data becomes one cooked asset schema; Renderer never parses OpenVDB/source formats at runtime.

## Core Records

Names are provisional until `VOL-D0`, but ownership is normative.

| Record | Contents | Owner/lifetime |
| --- | --- | --- |
| `MediumCoefficients` | finite `sigma_a/sigma_s/emission/phase` in frozen units | canonical semantic value shared by authoring conversion, CPU tests, prepared state |
| `GlobalFogDesc` | homogeneous/height profile and stable revision | GameFramework authored intent |
| `LocalMediumDesc` | shape/transform/falloff/coefficient or cooked density handle/revision | GameFramework authored intent |
| `AtmosphereDesc` | planet/ground, profiles, sun/sky parameters, mode/revision | GameFramework environment intent |
| `RenderMediumRecord` | stable object ID, immutable resolved intent/content generation | RenderScene persistent state |
| `PreparedVolumetricScene` | borrowed bounded medium/light/environment/TLAS views and generations | frame-local prepared scene |
| `VolumetricProfile` | grid/integration/light/reconstruction/atmosphere/reservoir selection | immutable resolved View value; no raw CVar forwarding |
| `VolumetricHistoryKey` | View/grid/depth/media/light/sky/LUT/content/shader/provider/profile generations | feature-local value |
| `VolumetricProducts` | raw coefficient/density if materialized, in-scatter/transmittance, composed output, confidence | frame-graph handle aggregate |
| `VolumetricStatus` | requested/active tier and finite unavailable/degraded/rejected reason | bounded existing observation path |

One mutable owner exists for each authored record and each GPU history generation. Prepared values are immutable views/derived copies justified by the world-to-render and CPU-to-GPU boundary; no “volume scene” mirrors the RenderScene.

## First Usable Slice

The first admitted production slice deliberately avoids a general framework:

1. one global homogeneous/exponential height medium from View/world state;
2. one directional light from the existing prepared light view;
3. deterministic coefficient evaluation and single scattering;
4. one froxel or bounded per-pixel integration path selected by `VOL-D0-06`;
5. raw in-scatter/transmittance output;
6. correct opaque depth and image-sky composition;
7. no temporal history, local volumes, atmosphere LUTs, heterogeneous textures, ReSTIR, or clouds.

It proves the end-to-end authoring-to-composition seam with Beer/analytic tests before state multiplies.

## Unified Froxel Graph

After the first slice passes:

```text
Resolve/UploadVolumetricSceneGeneration
  -> InjectMediumCoefficients
  -> InjectVolumetricLightsAndShadows
  -> IntegrateVolumetricScattering
  -> ReconstructVolumetrics
  -> ComposeVolumetrics
```

Atmosphere LUT generation runs only when `AtmosphereGeneration` changes and publishes atomically:

```text
GenerateAtmosphereTransmittance
  -> GenerateAtmosphereMultipleScatter
  -> GenerateSkyViewAndAerialPerspective
  -> PublishAtmosphereGeneration
```

The frame consumes the last complete eligible LUT generation or follows the frozen pending/failure policy. It never mixes new transmittance with old sky-view data.

Heterogeneous density injection and Volumetric ReSTIR add passes to the feature graph only at their stages. Pass fusion follows measured memory/bandwidth cost and keeps intermediate semantics reproducible.

## Resource And Format Classes

| Resource | Persistence | Identity/constraint |
| --- | --- | --- |
| medium records/content views | RenderScene/content generation | stable logical ID, bounded count, immutable during frame |
| coefficient/density froxels | transient or current working generation | grid mapping/format/profile/media generation |
| light/source froxels | transient | same grid + light/shadow generation |
| integrated in-scatter/transmittance | current frame, possibly history input | premultiplied composition semantics |
| temporal history | per View persistent | separate current/previous, full history key |
| atmosphere LUT set | persistent cache | all-or-nothing atmosphere/algorithm/shader generation |
| density texture/sparse grid | content-owned GPU generation | cooked asset/transform/filter/majorant identity |
| volume path reservoirs | per View only in admitted tier | path/medium/light/estimator generation |

`VOL-D0-15` freezes dimensions/formats/bytes. The budget counts transient aliasing barriers, previous/current histories, old/new LUT/content generations during replacement, and all views. FP16 is not assumed safe for optical depth, identity, or high-dynamic-range scattering until tests pass.

## Lifetime And Transactionality

- Scene deltas publish complete authored medium/atmosphere records with monotonic generations.
- Asset reload prepares a complete cooked/GPU density generation, swaps only on success, and retains the prior generation through its last submission.
- Froxel/history resources are per View; dual views never share screen-space history.
- Current frame reads completed previous history and writes separate current storage. Only successful completion promotes it.
- Atmosphere LUT sets publish as one generation. Cancellation, shader failure, device loss, or invalid parameter retains last-good or explicit fallback according to `VOL-D0`.
- Resize/profile/grid/schema change allocates replacements before activation; peak memory includes overlap.

## History Invalidation

| Mutation | Action |
| --- | --- |
| camera cut, invalid motion, projection/extent/grid change | reset View history |
| global/height coefficient/profile change | reset or responsive current-only according to frozen reconstruction |
| local medium transform/density/content change | invalidate affected history; full reset allowed as conservative first implementation |
| light/shadow change | confidence/reset according to light generation; stale illumination forbidden |
| atmosphere/environment/LUT generation change | reset dependent sky/aerial/fog history |
| shader/provider/profile/pre-exposure change | reset dependent history |
| denoiser-only change | reset reconstruction; medium/LUT caches remain |
| ReSTIR estimator/path/random-layout change | reset volume reservoirs and reconstruction |

Fine-grained invalidation is an optimization. A conservative whole-volume reset is acceptable first if it is correct, visible, and meets adoption/performance needs.

## Light, Ray, And Surface Boundaries

Volumetric Lighting borrows prepared light semantics/identity and, if justified, immutable light distributions from Direct Lighting/GPU-scene owners. It owns its own receiver-domain selection/reservoir state. Geometry shadow rays call the existing semantic ray path; medium transmittance remains the volume integrator's responsibility.

The surface result entering composition is an immutable scene-linear texture plus depth/validity. The volume feature does not mutate Direct/Indirect lobes or own their reconstruction. A future transparent-surface bridge requires explicit ordered products; no hidden sampling of the transparency pass.

## Sky And Environment Ownership

One View selects exactly one base background mode:

- `ImageEnvironment`: existing cooked HDR texture/multiplier semantics;
- `PhysicalAtmosphere`: active complete atmosphere generation;
- an explicitly designed combined mode only if `VOL-D0-10/11` admits it.

The Sky/environment owner publishes a typed `EnvironmentView` containing background evaluation and importance-distribution identity. Direct/Indirect Lighting consume it for surface paths. Volumetric Lighting owns camera-ray atmosphere/aerial perspective and composes local fog according to one order. No component independently adds another sun disk or sky fill.

## Heterogeneous Content Boundary

Source importer/cooker validates selected grid/channel/class, transform to metres, bounds, density range, compression, and majorant metadata. The cooked artifact owns a versioned content hash as a real external serialization boundary; runtime supports only the current clean-break schema. It does not keep source readers, migration aliases, or both dense and sparse canonical forms.

Renderer receives a cooked density asset handle and generation. Dense/sparse is an implementation/storage choice behind one semantic sample interface only after both are actually admitted consumers; do not create the abstraction while only dense textures exist.

## Volumetric ReSTIR Subgraph

Only after `VOL-D0-13` acceptance:

```text
GenerateVolumePathCandidates
  -> ResampleVolumeTemporal
  -> ResampleVolumeSpatial
  -> EvaluateSelectedVolumePathWithAcceptedTracking
  -> raw in-scatter/transmittance/confidence
```

Its reservoirs are per-View and feature-local. It may read medium/light/environment generations and the reference tracking kernel, but cannot reuse surface reservoirs or duplicate content/scene state. Approximate candidate evaluation and final selected evaluation are separate named functions/products so their claim cannot be confused.

If the admitted need is only many-light froxel injection, implement a Direct-style froxel light reservoir instead and name it accordingly; do not claim the multi-dimensional volume-path method.

## Backend And Capability Boundary

- Baseline fog/atmosphere prefers compute and typed 3D/2D textures supported by mandatory D3D12/Vulkan cells.
- Ray-traced geometry shadows or stochastic tracking request existing ray capability through semantic Renderer interfaces; no native backend handles leak upward.
- Required formats, filtering, atomics/wave operations, descriptor counts, and ray features are inventoried and preflighted before graph creation.
- Unsupported tier/provider rejects with exact requested/active status. A lower tier is selected only by the frozen fallback policy.
- Renderer/RHI boundary changes run `architecture_boundary_check` and paired native validation.

## Profiles And Failure State

The durable settings surface is small:

| State | Meaning |
| --- | --- |
| `Off` | no volume resources/passes; exact identity composition |
| `FogBalanced` | admitted unified fog/froxel/reconstruction profile |
| `AtmosphereQuality` | physical sky/aerial perspective with admitted fog tier |
| `HeterogeneousQuality` | content/tracking tier when supported |
| `ReservoirQuality` | admitted volume reservoir estimator |
| `Unavailable`/`Degraded`/`Rejected` | explicit stable reason and resolved behavior |

Do not expose grid dimensions, every march step, or paper constants as permanent product settings. Internal development controls are removed or deliberately classified before submission.

## Integration-Hook Ledger

`VOL-D0-16` freezes exact files per stage. Expected boundaries:

| Owner | Narrow hook | Detecting check |
| --- | --- | --- |
| GameFramework environment/world | authored fog/medium/atmosphere intent and serialization | round-trip/default/invalid-value check |
| importer/cooker/assets | admitted density source to one cooked representation | corrupt/missing/hash/reload check |
| RenderScene/GPU-scene | stable medium/atmosphere records and prepared views | delta/capacity/lifetime check |
| Direct/Sky environment | borrowed light/environment generation | unit/identity/double-count check |
| Lit frame recipe | one feature invocation and typed compose product | topology/order check |
| shader registration/cooking | concrete volume shaders and dependencies | publication closure |
| RHI | only capabilities/formats genuinely absent from current neutral contract | paired boundary/native validation |
| settings/status/debug | bounded profiles, reason codes, raw product routes | adoption/fault/artifact check |

Every additional feature-named edit outside the capsule is ledgered. A wrapper with no policy/state/lifetime/reuse is deleted.

## Failure And Recovery

- Invalid authored coefficients/assets reject before active scene publication and preserve the prior/default complete state.
- Resource/capacity/capability failure rejects the requested tier before partial graph execution or chooses only an explicit fallback.
- Non-finite volume math invalidates current output/history; it cannot publish NaN into `SceneColor`.
- LUT/content/shader reload is atomic and generation-safe.
- Device loss/cancel/shutdown drains/abandons current work and retires resources by completed submission.
- Quality overload reports degraded state/budget; it does not randomly drop media/lights without documented selection.

## Detailed Pass And Publication Contracts

| Producer/pass | Reads | Writes/publishes | Omission/stop | Primary falsifier |
| --- | --- | --- | --- | --- |
| authoring/cook validation | typed medium/atmosphere/asset intent | canonical authored/cooked value plus validation/provenance | current negative state exposes no unfinished type | invalid units/transform/content/rights and save/load round trip |
| scene extraction/preparation | immutable world publication and cooked leases | validated medium/atmosphere/content generations | no admitted medium means no volume scene state | add/remove/edit/reload/cancel and generation retirement |
| atmosphere LUT build | accepted atmosphere parameters and celestial-light identity | complete immutable LUT group/environment candidate | failed/partial group never publishes | parameter mutation/failure plus analytic/Hillaire/Bruneton query cells |
| froxel coefficient resolve | View/grid plus current medium/content generations | one coefficient/phase/emission representation and capacity status | absent/off graph omits it | analytic field, overlap, boundary, transform and overflow matrix |
| volume light source | coefficients, admitted light list/distribution, shadows/TLAS/environment | raw source radiance, visibility/transmittance facts | no admitted lights yields finite emission-only/zero source | one-light analytic, light count, shadow and medium-segment cases |
| front-to-back integrate | source/coefficient cells, opaque depth and grid mapping | premultiplied in-scatter plus transmittance | never for active fog tier | slab/height/convergence and synthetic composition cases |
| temporal reconstruction | raw current volume, depth/motion/generations, previous immutable history | reconstructed current volume/confidence/history | current-only/first frame/cut/incompatible state | camera/density/light/extent/dual-view matrix |
| volume ReSTIR subgraph | current medium/light/environment plus explicit volume paths | volume-owned path reservoir and raw final evaluation | omitted unless Stage 6 admission/conformance passes | analytic/tracking reference and equal-time reuse study |
| compose | selected raw/reconstructed pair, opaque surface and selected sky/environment | one scene-linear composition edge | Off is exact identity/graph omission | exact `T*surface+scatter`, depth/sky/transparency-order cells |

## Resource Access, Barriers, And Replacement Memory

| Resource class | Lifetime | Access/order | Budget requirement |
| --- | --- | --- | --- |
| authored/cooked medium assets | persistent content generation | immutable leases; no runtime parsing/mutation of source asset | source, cooked, upload and replacement overlap reported separately |
| prepared medium/light/environment views | scene/frame generation | complete before graph; read-only during frame | only feature-specific prepared state charged to feature |
| atmosphere LUT group | persistent generation | all textures build then publish atomically; readers use one generation | old + building + current + retirement overlap |
| coefficient/source/integrated froxel volumes | frame/transient or history input | explicit UAV producer/SRV consumer dependencies; aliases only after last read | worst admitted dimensions/formats, diagnostic overlap and resize replacement |
| previous/current volume history | per-View persistent | previous immutable, current unpublished until complete | dual views, current/previous and provider replacement included |
| density bricks/majorants/residency | content generation | asset upload publishes complete index/data/majorant set | sparse metadata, empty-space structures and worst replacement spike |
| volume path reservoirs | frame/per-View only for admitted ReSTIR | isolated from surface reservoirs and filter history | explicit logical/packed record, stages and correlation diagnostics |

FrameGraph owns backend resource states, UAV ordering, queue transfer and synchronization. Manual feature barriers/fences require an accepted missing-abstraction decision and paired-backend evidence. Compute/async work cannot outlive its View/content/LUT/TLAS leases.

## Capacity And Scaling Policy

Every bounded collection has a declared capacity, ordering/culling rule, first-overflow result, requested/active/degraded status and counter: local media, coefficient contributors, lights per froxel, density assets/bricks, atmosphere lights/LUT dimensions, tracking events, volume paths, histories and debug captures. Arbitrary iteration-order dropping is prohibited.

Scaling studies vary one axis at a time—render/froxel extent, depth slices, media overlap, light count, optical depth, density frequency, step/event count, path depth, history count—and record raw error, bandwidth, rays/events, GPU timing distribution, persistent/transient/replacement memory and first degraded state. Source-engine numbers never fill these fields.

## Architecture Fitness Gate

Every stage retains the feature-enclosure ledger across GameFramework authoring, level parser, source import/cook, world publication, RenderScene/preparation, Renderer frame graph, shared light/sky/ray semantics, RHI capabilities, settings/editor, tests and package/docs. Each hook names owner, direction, lifetime, reason, deletion and defect-detecting check.

The stage blocks on an unledgered fog/sky/cloud switch in generic orchestration, duplicate canonical medium/environment/light data, parallel sky or cloud renderer, runtime source parser, volume state on a surface reservoir, manual backend policy, forwarding-only abstraction, or public type with no end-to-end consumer. Bounded removal must restore the prior negative graph and eliminate selectors, types, parsers, assets, passes, shaders, histories, build members and documentation for the removed tier.

## Architecture Invariants

1. One physical medium contract feeds fog, atmosphere, heterogeneous, reservoir, and cloud tiers.
2. One mutable owner exists for authored state, content generation, prepared state, and each View history.
3. In-scatter/transmittance compose exactly once in scene-linear space.
4. Image sky, physical atmosphere, environment sampling, and camera-ray aerial perspective have explicit non-overlapping roles.
5. Surface and volume reservoirs never share receiver-domain state accidentally.
6. Volumetric ReSTIR follows a proved reference integrator and reports its exact estimator domain.
7. History/LUT/content publication is transactional and resources retire by GPU completion.
8. Every expansion is admitted by evidence and remains inside the feature enclosure.

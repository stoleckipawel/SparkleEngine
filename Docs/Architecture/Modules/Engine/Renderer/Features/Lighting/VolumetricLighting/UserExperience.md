# Volumetric Lighting, Fog, Atmosphere, And Sky User Experience Contract

**Status:** target experience only; current capability remains absent and production work remains blocked by `REL-11` and `VOL-D0`

**Responsibility:** define authoring, selection, first use, diagnosis, capture, failure, automation, and accessibility for the admitted volumetric tiers

**Authority boundary:** the [dossier](README.md) owns acceptance; [Transport And Composition](TransportAndComposition.md) owns physical/result meaning; [Execution Architecture](ExecutionArchitecture.md) owns runtime/content truth; [Discovery](Discovery.md) ratifies product choices; [Plan](Plan.md) owns delivery order

## Experience Principles

1. One environment story covers sky background, aerial perspective, global fog, local media, sun/direct lights, and clouds when admitted. Separate panels cannot create competing physical state.
2. The first product is understandable homogeneous/height fog with a recommended preset and physical expert controls. Volumetric ReSTIR and clouds do not block basic fog.
3. Authored intent, accepted prepared data, active renderer tier, and final published generation are distinct. A preview or stale LUT/history is never labeled current.
4. Units and composition are visible: inverse-metre coefficients, metres, dimensionless anisotropy, scene-linear radiance, transmittance, and `Lout = T * Lsurface + Lscatter`.
5. Unsupported tiers remain absent from selectors. Enabling an admitted but unsupported strict tier rejects before scene/frame publication and never falls back to cosmetic fog.

## Personas And Jobs

| Persona | Job | Required route |
| --- | --- | --- |
| environment artist | establish readable depth, atmosphere, shafts, and sky continuity quickly | Environment/Sky inspector with recommended presets and Lit preview |
| lighting artist | understand which lights affect media and why a light or shadow is missing | light inspector participation plus volumetric debug products |
| VFX/technical artist | author local/heterogeneous media, inspect density, bounds, filtering, and capacity | volume inspector, asset validation, slice/bounds views |
| rendering engineer | validate RTE, froxels, tracking, ReSTIR, composition, history, and backend | raw products, analytic/reference captures, counters, manifests |
| automation/release/support | reproduce one tier/profile/content generation and classify terminal state | structured authoring/settings/status/capture and support record |

## Current Experience Truth

The editor currently exposes an image-based Sky override with texture, color, brightness, and enabled state. No fog, participating medium, physical atmosphere, local volume, heterogeneous asset, cloud, volumetric setting, diagnostic product, or active status exists. The following contract deliberately does not make those controls reachable before Stage 1 closes the negative-capability gate.

## Progressive First Use

### `VOL-Q1` Fog

1. Select Environment/Sky and choose the admitted `Fog` mode or add the single admitted global medium owner.
2. Start from a named neutral preset whose exact coefficients are displayed and serializable.
3. Adjust density/extinction, albedo/color, height falloff/reference height, anisotropy, and enabled state within validated ranges.
4. The Lit view reports `Requested -> Active`, tier/profile, participating lights, reconstruction state, and current medium generation.
5. Toggle the raw transmittance and in-scatter views, then save/reload and verify the same authored values and result identity.

### `VOL-Q2` Local Media

Create one admitted bounded shape, edit world-space bounds/transform and coefficient contribution, then inspect bounds, density slice, overlap/priority result, capacity, and active generation. Invalid transforms or coefficient combinations remain unapplied and identify the field and volume.

### `VOL-Q3` Atmosphere

Choose `Image Environment` or `Physical Atmosphere` explicitly. The atmosphere inspector groups planet/ground, molecular/Rayleigh, aerosol/Mie, absorption/ozone, sun disk/light, aerial perspective, and quality. A compact `Earth-like` preset is convenience data, not a hidden unit system. Sky background, surface environment sampling, sun, and aerial perspective show the same environment generation.

### Later Admitted Tiers

Heterogeneous media require import/cook validation before selection. Volumetric ReSTIR appears only as a profile inside an admitted volume-lighting tier and discloses its estimator/bias domain. Clouds require a separate approved product increment and extend the same environment owner; they do not create another sky or fog panel.

## Authoring Contract

| Group | Public intent | Validation/status |
| --- | --- | --- |
| medium | extinction or density plus albedo/scattering, absorption, emission, anisotropy, unit scale | show derived `sigma_a/sigma_s/sigma_t`; reject negative/non-finite/invalid albedo or anisotropy |
| height fog | reference height, density, falloff and optional maximum extent | show metres/inverse metres and sampled profile preview |
| local medium | admitted shape, transform, boundary/falloff, priority/blend, coefficient contribution | bounds view, overlap rule, capacity index, invalid transform reason |
| physical atmosphere | planet radii, ground albedo, molecular/aerosol/absorption profiles, celestial light | parameter units, LUT state/generation, reference preset source |
| heterogeneous asset | cooked asset, channel, density scale, transform, filter, bounds, majorant policy | import/cook version/hash, residency, majorant validity, missing/corrupt state |
| cloud | admitted weather/density/detail/LOD/light/shadow parameters | tier/profile, memory/residency, temporal state, explicit deferred controls |

Controls write through existing transaction/undo/redo and level persistence. Invalid edits do not partially publish. Runtime state never leaks into authored files, and one canonical authored representation feeds one cooked/scene/prepared path.

## Selection, State, And Progress

| State | Meaning | Required behavior |
| --- | --- | --- |
| `Absent` | tier is not built/admitted | no selector or content vocabulary claims reachability |
| `Unavailable` | admitted request lacks capability/content/provider | reject before graph/publication; name blocker |
| `Preparing` | asset upload or atmosphere/froxel generation is building a new immutable generation | retain previous complete result, show new request is not active |
| `Active` | current completed generation produced the displayed tier/profile | identify medium/environment/content/history generations |
| `Degraded` | resolution, light/volume capacity, step/ray budget, reconstruction, or optional provider is below requested intent by accepted policy | identify actual active route, likely artifact, and remedy |
| `Failed` | validation, cook/load, majorant, non-finite, device, shader, or publication invariant failed | publish no partial generation/history and preserve prior valid result separately |

Preparation reports concrete units—assets validated/uploaded, LUTs completed, current generation, and memory—not an invented percentage. Cancellation retires unpublished work safely. A responsive viewport, partial LUT, or fog-colored fallback is not completion.

## Quality And Expert Controls

Named profiles own render/froxel resolution, depth mapping, sample/step/light budgets, temporal policy, and memory envelope after `VOL-D0-12`. Expert controls disclose those resolved values but do not create undocumented combinations. Correctness-defining changes create a new generation and reset affected histories/LUTs.

`Automatic` selects only accepted tiers/providers. Strict atmosphere, heterogeneous, cloud, ray-traced, or ReSTIR selections fail visibly when unavailable. Off removes the admitted graph contribution and is composition identity; it does not write black transmittance or a replacement sky.

## Diagnostic Views And Captures

| Product | Question | Required legend/identity |
| --- | --- | --- |
| medium coefficients/albedo | what physical medium was prepared? | units, range, global/local/source generation |
| density/bounds/slices | where is media present and how did overlap resolve? | world/froxel coordinates, shape/content ID, priority/capacity |
| light source/visibility | which lights and shadow/transmittance terms contribute? | stable light ID, geometry visibility, medium transmittance, selected estimator |
| transmittance | is optical depth/composition correct? | scalar/RGB range, segment endpoints, step/tracking method |
| in-scatter/emission | where does scene-linear radiance enter? | phase/light class, exposure state, sample/step budget |
| froxel/reprojection/confidence | is resolution/history causing the artifact? | slice mapping, jitter, motion/depth/generation gates, rejection reason |
| atmosphere LUT/environment | are sky, aerial perspective, sun, and path sampling coherent? | LUT kind/dimensions/generation, environment mapping/PDF generation |
| ReSTIR/path state | is an admitted reservoir estimator valid? | path/proposal/shift/weight revision, `M`, correlation/bias, approximate/final evaluation |
| composition | was `T * surface + scatter` applied exactly once? | opaque depth, sky/transparency order, raw input/output identity |

Captures bind candidate/dirty/build/shader/backend/device/driver, scene/camera/profile, authored/cooked/prepared generations, unit/coordinate conventions, content hashes/rights manifest, raw intermediates, counters, reference/metric, warmup/window, and final publication identity. Tone-mapped output is supplemental only.

## Failure, Recovery, Support, And Privacy

Every terminal failure reports category, field/volume/view/candidate identity, requested/active tier, first invalid capability/content/equation/invariant, safe state, retained/retired generations, cleanup, and next action. Correcting authoring/content or selecting an accepted tier starts a new generation; failed partial assets, LUTs, froxels, histories, or captures never replace the last successful product.

The support record contains logical IDs, hashes, profiles, dimensions, generations, budgets, counters, capability/provider status, and reason codes. Source asset payloads, user paths, and proprietary content are excluded unless the user deliberately attaches them. Asset provenance and redistribution rights travel with imported/cooked test content.

## Automation And Accessibility

Automation can create the same admitted authored records, apply profiles, query state/generations, wait for terminal preparation/frame/capture, request identical debug products, and observe the same validation/reason codes. UI, level parsing, scripts, and tests share semantic values; none owns a parallel fog or atmosphere schema.

All states include text, not color alone. Controls and diagnostics support keyboard navigation, search, UI scaling, numeric units/ranges, high-contrast invalid markers, and stable ordering. Temporal/flickering views support pause and frame stepping. Legends avoid relying solely on hue to encode density, transmittance, phase, or validity.

## Experience Acceptance

- `UX-VOL-01` — after Stage 1 admission, a first-time artist creates neutral fog, edits physical values, toggles Off, saves/reloads, and identifies the active generation without renderer terminology.
- `UX-VOL-02` — invalid coefficient/transform, overlap/capacity, missing/corrupt content, unsupported tier/backend/provider, majorant failure, and publication failure produce distinct safe states and recovery actions.
- `UX-VOL-03` — image environment and physical atmosphere are explicit modes; sky, sun, aerial perspective, fog, and surface environment report one coherent generation with no hidden double application.
- `UX-VOL-04` — raw, reconstructed, composed, stale, partial, failed, and final products are unambiguous; UI and automation produce equivalent manifests.
- `UX-VOL-05` — keyboard, text, contrast, scaling, pause/step, units, and non-color checks pass for every admitted authoring/debug route.

These criteria refine `AC-VOL-04`, `AC-VOL-07`, `AC-VOL-09`, `AC-VOL-10`, `AC-VOL-12`, `AC-VOL-14`, and `AC-VOL-15`; the future assigned FCR owns observed results.

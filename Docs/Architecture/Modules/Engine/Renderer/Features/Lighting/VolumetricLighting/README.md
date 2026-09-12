# Volumetric Lighting, Fog, Atmosphere, And Sky Dossier

**Status:** current negative capability plus target feature contract; research/discovery ready, production stages blocked by roadmap admission and `VOL-D0`

**Responsibility:** own the bounded volume/fog/atmosphere/sky promise, negative state, feature tiers, acceptance, failures, checks, and definition of done

**Authority boundary:** [Research](Research.md) owns precedent; [Discovery](Discovery.md) owns `VOL-D0`; [Transport And Composition](TransportAndComposition.md) owns math; [Execution Architecture](ExecutionArchitecture.md) owns system shape; [Plan](Plan.md) owns order; code/build and a future assigned FCR own implementation/results

**Verified:** 2026-09-12 against revision `8e4ffba225411965dc51c0b783e5f47a075c7e84`; no participating-media implementation was found

**Scope:** participating media, fog, direct volumetric lighting, transmittance, multiple scattering, aerial perspective, physical sky atmosphere, heterogeneous volumes, volumetric ReSTIR research, composition, reconstruction, diagnostics, and content adoption

**Current readiness:** **0/100** per [Current Feature Readiness](../../../../../../../Acceptance/CurrentReadiness.md#explicit-missing-or-not-yet-admitted-capabilities)

## Outcome

Sparkle needs one coherent participating-media system rather than separate cosmetic fog, light-shaft, and sky hacks. It must start from physically meaningful extinction/scattering/emission and the radiative transfer equation, deliver a practical froxel-based fog/atmosphere product, then evaluate reservoir-resampled heterogeneous/multiple-scattering paths where they materially improve quality.

Volumetric ReSTIR is an advanced estimator track inside this feature—not the prerequisite for basic fog. The first usable slice is deterministic homogeneous/height fog plus single-scattered light and exact composition. A ReSTIR volume implementation begins only after a reference transport baseline, medium representation, transmittance estimator, and workload budget exist.

The current roadmap excludes volumetric lighting from the first-release scope and blocks new feature production before the release gate. This package records the mandatory post-release target without changing that policy. See [Roadmap](../../../../../../../Strategy/Roadmap.md).

## Start Here

| Need | Owner |
| --- | --- |
| release admission, medium/sky/product decisions, and experiments | [Discovery](Discovery.md) |
| primary volume/atmosphere/cloud/ReSTIR research | [Research](Research.md) |
| radiative transfer, phase, transmittance, froxel, reservoir, and composition math | [Transport And Composition](TransportAndComposition.md) |
| scene/content owners, frame route, resources, histories, and integration hooks | [Execution Architecture](ExecutionArchitecture.md) |
| staged delivery from negative capability to accepted product | [Plan](Plan.md) |
| current image-based environment/background owner | [Indirect Lighting](../IndirectLighting/README.md) and current `Sky` source route |
| independent surface reference; volume support is not implied | [Reference Path Tracer](../ReferencePathTracer/README.md) |

No independent `UserExperience.md` is warranted while the feature is excluded and has no public workflow. The dossier owns the future product/status promise and the architecture owns proposed profiles; authoring UX must be added as an independent role when roadmap discovery admits it.

## Current Negative Capability

No owned path was found for:

- homogeneous, height, or heterogeneous participating media;
- global or local fog authoring/cooking/scene/GPU data;
- absorption, scattering, extinction, transmittance, phase functions, or volume emission;
- froxel/voxel medium injection, light injection, integration, or temporal reconstruction;
- volumetric shadows, light shafts, multiple scattering, or reservoir resampling;
- atmospheric Rayleigh/Mie/ozone scattering, aerial perspective, or ground/planet parameters;
- volumetric cloud representation, lighting, shadow, or weather authoring;
- volumetric debug products, selectors, backend requirements, or release evidence.

The existing `SceneSkyDesc` is an enabled flag, RGB multiplier/brightness, and cooked HDR texture reference. `Sky.cpp` fills background/environment radiance. Wrap subsurface, alpha masking, bloom/exposure, and discarded glTF transmission/volume vocabulary are not volume transport.

`REN-E24` remains the negative evidence owner until production admission. Creating this target package does not turn `REN-VOL-01` through `REN-VOL-03` into Planned-in-code, Partial, or Experimental.

## Feature Decomposition

| ID | Feature | First dependable contract | Advanced target | Current state |
| --- | --- | --- | --- | --- |
| `VOL-FS-01` | medium semantics | `sigma_a`, `sigma_s`, emission, phase, units, finite validation | wavelength/spectral model only if separately admitted | absent |
| `VOL-FS-02` | global/height fog | one atmosphere-relative homogeneous/exponential density field | layered weather profiles | absent |
| `VOL-FS-03` | local media | bounded sphere/box/ellipsoid density primitives with deterministic overlap | mesh/SDF and particle injection after content need | absent |
| `VOL-FS-04` | direct volume lighting | all admitted direct lights inject single scattering with transmittance/shadow policy | many-light reservoir assistance | absent |
| `VOL-FS-05` | froxel integration | camera-aligned medium/light representation and front-to-back radiance/transmittance | cascades/adaptive resolution if measured | absent |
| `VOL-FS-06` | temporal reconstruction | jittered sampling with velocity/depth/generation validity and confidence | learned reconstruction only after portable baseline | absent |
| `VOL-FS-07` | physical sky atmosphere | Rayleigh/Mie/absorption, sun disk, sky-view/aerial-perspective LUTs | ground-to-space and multiple celestial lights if admitted | absent |
| `VOL-FS-08` | heterogeneous assets | dense 3D texture first, explicit transform/filter/range | sparse OpenVDB/NanoVDB import/cook after workload admission | absent |
| `VOL-FS-09` | volumetric ReSTIR | reference single/multiple-scatter path and resampled path-space estimator | emission, complex environment, advanced tracking/guiding | absent/research |
| `VOL-FS-10` | clouds | authored density/weather field, lighting/shadow/composition and temporal quality | close/fly-through production cloudscape | absent/deferred |
| `VOL-FS-11` | diagnostics/adoption | medium coefficients, density, transmittance, in-scatter, slice/history views via existing tools | no separate dashboard | absent |

## Product Tiers

| Tier | Product promise | Explicit exclusions until later tier |
| --- | --- | --- |
| `VOL-Q0` semantic/reference | analytic homogeneous slabs, phase values, ratio/delta tracking references, composition | no visual product claim |
| `VOL-Q1` baseline fog | global height fog, one direct light, single scattering, opaque-depth composition | local heterogeneous media, atmosphere, ReSTIR |
| `VOL-Q2` unified froxel | local media, all analytic lights, shadows/transmittance, temporal reconstruction | multiple-scatter path truth and clouds |
| `VOL-Q3` atmosphere/sky | physical sky, sun, aerial perspective, environment bridge, ground-to-space cells | clouds and arbitrary planet systems unless admitted |
| `VOL-Q4` heterogeneous | dense/sparse volumes and local emission with content pipeline | production clouds until separate admission |
| `VOL-Q5` reservoir volume | measured ReSTIR assistance for complex lighting/media and optional multiple scattering | no blanket real-time/unbiased claim |

## Acceptance Criteria

- `AC-VOL-01` — medium coefficients use frozen inverse-metre units and satisfy `sigma_t=sigma_a+sigma_s`, non-negativity, finite range, albedo, and emission validation.
- `AC-VOL-02` — homogeneous and exponential-density slab/height cases match analytic transmittance and single-scattering values at predeclared tolerances.
- `AC-VOL-03` — phase functions are normalized over solid angle, reciprocal where claimed, finite at anisotropy bounds, and consistently parameterized.
- `AC-VOL-04` — global and overlapping local media resolve through one deterministic density/coefficient composition rule with explicit priority/blend behavior and bounded capacity failure.
- `AC-VOL-05` — every admitted direct light contributes correct volume in-scattering with light-distance, medium transmittance, geometry shadow, and light/medium identity; no surface-light reservoir is sampled as though it were a froxel distribution.
- `AC-VOL-06` — camera integration publishes premultiplied scene-linear in-scattering and transmittance so `Lout=T*Lsurface+Lscatter` is applied once at the correct depth/order.
- `AC-VOL-07` — temporal jitter/reprojection rejects camera cuts, disocclusion, depth/extent, medium/light/atmosphere, shader/provider, and quality changes; rapid density/emission changes do not leave unbounded trails.
- `AC-VOL-08` — physical sky/atmosphere passes analytic/independent sky radiance, transmittance, sun disk, aerial perspective, horizon, ground, altitude, and exposure-independent raw checks.
- `AC-VOL-09` — environment background and surface-path sampling consume one atmosphere/environment generation without double-applying sky radiance or atmospheric transmittance.
- `AC-VOL-10` — dense/sparse heterogeneous media define texture transform, density units/range, filtering, empty-space handling, content lifetime, and deterministic missing/malformed asset failure.
- `AC-VOL-11` — any Volumetric ReSTIR profile defines path domain, free-flight/transmittance estimators, target, contribution weight, shift, Jacobian/support, correlation/bias mode, and unbiased final evaluation; it passes analytic/statistical reference tests.
- `AC-VOL-12` — raw density/coefficient/transmittance/in-scatter and reconstructed/composed products remain separately inspectable with exact candidate identity.
- `AC-VOL-13` — D3D12/Vulkan supported cells pass identical semantic/failure checks with native validation; quality, temporal behavior, GPU time, and peak/history memory are separately reported.
- `AC-VOL-14` — one feature capsule owns volume mechanism/state; authored/cooked/scene/RHI hooks are ledgered and no fog/sky/cloud side system duplicates the transport.
- `AC-VOL-15` — the roadmap assigns an FCR/evidence owner before production acceptance; until then all target criteria remain open and current negative criteria remain authoritative.

## Current Negative Acceptance

- `AC-VOL-NEG-01` — no selector, component, imported/cooked field, or UI claims participating media, fog, atmosphere, aerial perspective, or clouds without a complete Renderer consumer.
- `AC-VOL-NEG-02` — no graph pass/resource, shader registration, history, debug product, backend capability, or package/release claim implies volume integration.
- `AC-VOL-NEG-03` — image sky, wrap subsurface, alpha masking, exposure/bloom, and ignored glTF volume vocabulary remain classified as non-volumetric.

## Controlled Failure Modes

| ID | Failure | Safe response | Check |
| --- | --- | --- | --- |
| `FM-VOL-01` | invalid/non-finite/negative coefficients, density, anisotropy, transform, or step budget | reject authoring/publication with field/volume identity | `CHK-VOL-01` |
| `FM-VOL-02` | extreme optical depth underflows or marching/tracking becomes non-finite | finite opaque limit or explicit invariant failure, never NaN history | `CHK-VOL-01/02` |
| `FM-VOL-03` | overlapping media use ambiguous blend/priority | deterministic frozen composition or reject conflicting content | `CHK-VOL-03` |
| `FM-VOL-04` | stale density/light/atmosphere history produces trails/light leaks | invalidate/reduce confidence and restart from current state | `CHK-VOL-04` |
| `FM-VOL-05` | surface, sky, transparency, and volume compose in wrong order | raw composition oracle fails; no presentation pass can hide it | `CHK-VOL-05` |
| `FM-VOL-06` | malformed/missing volume texture or sparse asset | reject/explicit fallback according to content contract | `CHK-VOL-06` |
| `FM-VOL-07` | reservoir shift/transmittance approximation changes estimator support/bias | reject candidate/profile and retain baseline | `CHK-VOL-07` |
| `FM-VOL-08` | too many lights/volumes or inadequate froxel resolution | visible degraded/capacity status with bounded behavior | `CHK-VOL-08` |
| `FM-VOL-09` | backend lacks required format/atomics/ray feature | reject unsupported profile before graph dispatch | `CHK-VOL-09` |
| `FM-VOL-NEG-01` | new volume vocabulary becomes reachable before consumer/evidence | fail negative audit and require roadmap/architecture update | `CHK-VOL-NEG-01` |

## Required Checks

| Check | Exercise |
| --- | --- |
| `CHK-VOL-01` | coefficient/phase/Beer slab analytic CPU and shader cases, invalid/extreme inputs |
| `CHK-VOL-02` | deterministic quadrature and tracking statistics for homogeneous/heterogeneous transmittance and single/multiple scattering |
| `CHK-VOL-03` | global/height/local overlap, boundary, capacity, transform, and authoring round trips |
| `CHK-VOL-04` | temporal camera/light/density/emission/atmosphere/extent/shader/provider mutation and dual-view matrix |
| `CHK-VOL-05` | opaque surface/sky/depth/transparency composition with raw `T` and in-scatter artifacts |
| `CHK-VOL-06` | dense/sparse texture import/cook/load/filter/missing/corrupt/lifetime matrix |
| `CHK-VOL-07` | volumetric ReSTIR path/shift/reservoir statistical tests versus volume reference integrator |
| `CHK-VOL-08` | light/volume/optical-depth/froxel-resolution/ray-step sweep with quality-time-memory curves |
| `CHK-VOL-09` | D3D12/Vulkan capability/native-validation matrix and provider faults |
| `CHK-VOL-10` | raw/reconstructed visual workload matrix: Cornell slab, Sponza fog, Bistro exterior/shafts, San Miguel held-out, volume asset/cloud fixtures when admitted |
| `CHK-VOL-11` | enclosure/hooks/build/cook/package/docs/status/FCR audit |
| `CHK-VOL-NEG-01` | repository-wide negative capability audit retained until Stage 1 admission |

## Definition Of Done

An included volumetric tier is done only after `REL-11` admission, accepted `VOL-D0`, an assigned FCR, passing mapped `AC-VOL-*`/`CHK-VOL-*`, controlled failures, supported backend/content/adoption/quality-time-memory evidence, and removal of stale negative claims for exactly that tier. Other tiers may remain deferred or excluded. There is no volumetric FCR identity today. Papers and commercial-engine examples prove precedent only; the 2021 Volumetric ReSTIR results are not a ready-made Sparkle real-time solution.

## Inspected Current Routes

- [`BuildRenderFrameGraph.cpp`](../../../../../../../../Engine/Renderer/Private/Frame/Graph/BuildRenderFrameGraph.cpp) has no volume stage.
- [`Lighting.cpp`](../../../../../../../../Engine/Renderer/Private/Passes/Lighting/Lighting.cpp) creates surface lobes, composite, Sky, and optional reconstruction only.
- [`LightingRenderTargets.cpp`](../../../../../../../../Engine/Renderer/Private/Passes/Lighting/LightingRenderTargets.cpp) defines no volume product.
- [`SceneSkyDesc.h`](../../../../../../../../Engine/GameFramework/Public/Scene/Sky/SceneSkyDesc.h) describes image-based sky intent, not atmosphere.
- [`GltfMaterialImporter.cpp`](../../../../../../../../Tools/Import/SourceImporters/Private/Gltf/GltfMaterialImporter.cpp) does not establish a medium/content path.

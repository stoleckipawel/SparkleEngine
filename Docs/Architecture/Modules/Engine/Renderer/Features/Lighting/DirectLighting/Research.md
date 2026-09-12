# Direct Lighting Completion Study

**Status:** primary-source research and decision input; precedent does not prove Sparkle implementation, suitability, performance, or release readiness

**Responsibility:** record current-source findings, primary external precedent, alternatives, failure lessons, permitted transfers, and forbidden inferences for `DIR-D0`

**Authority boundary:** this study informs [Discovery](Discovery.md) but cannot select the product, settle local math/architecture, authorize implementation, or pass `FCR-REN-06`

**Researched:** 2026-09-12; repository source audit at `8e4ffba225411965dc51c0b783e5f47a075c7e84`; GitHub references are revision-pinned where practical

## Research Decision

Sparkle should keep a small transparent renderer-owned ReSTIR DI implementation, but replace the current undocumented reservoir behavior through a correctness-led clean break. RTXDI is the principal executable specification and comparison implementation; it should not become an opaque subsystem or a second scene/material/RHI architecture. The product target is a fixed-work, many-dynamic-light path comparable in intent to MegaLights, with honest quality degradation and one independently testable fallback baseline.

No single shadow, sampler, or denoiser wins every cell. The target is a composed system:

1. analytic/exhaustive small-light truth;
2. power/environment/emissive candidate distributions with exact PDFs;
3. ReSTIR DI with explicit light translation and selectable bias correction;
4. ray-traced visibility through Sparkle's existing Inline/Pipeline semantic boundary;
5. a vendor-neutral reconstruction baseline plus optional DLSS Ray Reconstruction;
6. workload-driven admission for ReGIR, emissive meshes, virtual shadow maps, opacity micromaps, or advanced reuse.

## Method And Evidence Rules

The study uses papers/specifications, official SDK repositories, and official engine documentation. Reported images, speedups, timings, or light counts belong to their source's scenes, hardware, implementation, and error metric. They identify hypotheses and failure modes; none is a Sparkle budget or acceptance result. “State of the art” here means the relevant current design space was surveyed as of the research date, not that every published variant must ship.

## Sampling And Reservoir Findings

Original ReSTIR DI repeatedly resamples candidate lights across space and time so a small ray budget can represent very large light sets.[^1] The important transferable contract is not the acronym but the estimator state: the selected sample, its target density, accumulated weights, effective sample count, source distribution, and the transformations used when a sample moves between receivers.

RTXDI's current integration guide makes the renderer boundary explicit. The application still owns materials, scene data, GBuffer access, ray traversal, resources, and API integration; the library owns reusable sampling mathematics behind bridge functions.[^2] Its production sequence exposes concrete gaps to test in Sparkle: current/previous light data and index translation, neighbor offsets, local/environment PDF textures, presampled RIS candidates, optional ReGIR world-space candidates, reservoir arrays, final visibility/shading, confidence, and denoising.[^2]

RTXDI's bias guide distinguishes temporal stability from correctness. A target proportional to outgoing radiance, nonzero support, disocclusion-aware spatial work, valid previous surfaces/lights, and an explicit bias-correction mode all matter. Visibility reuse saves rays but can introduce bias unless its conservative assumptions and correction path are satisfied.[^3] Consequently, “it looks stable” is an invalid ReSTIR oracle.

MegaLights is useful product precedent: it spends a bounded number of stochastic samples/rays per pixel and unifies many direct lights, shadows, and denoising. Its own documentation also states the tradeoff—quality falls when many important lights overlap a pixel, and temporal reconstruction can blur or ghost the result.[^4] Sparkle should adopt the truthful budget-and-degradation model, not the marketing word “unlimited.”

| Candidate family | Strength | Characteristic failure | Sparkle disposition |
| --- | --- | --- | --- |
| exhaustive analytic lights | simple, deterministic, excellent oracle | work grows with affecting-light count | mandatory small-light baseline; not the scale path |
| uniform light selection | trivial and full support | catastrophic variance with uneven power/influence | retain only as oracle/debug distribution |
| power-weighted global PDF | cheap, stable, easy to validate | ignores receiver geometry/visibility | mandatory initial distribution |
| environment solid-angle/luminance PDF | correct importance for HDR texels | misses local visibility/material response | mandatory when environment enters direct candidates |
| tiled/clustered light lists | deterministic receiver locality | list construction/storage and large overlapping cells | compare during discovery; useful input, not automatically final sampler |
| ReGIR/world-space reservoir grid | helps reject distant irrelevant lights | rebuild/storage cost and low selectivity in locally dense cells | admit only if Bistro/San Miguel data beats simpler PDFs |
| ReSTIR DI screen-space reuse | high effective sample count at bounded rays | bias, correlation, stale identity, disocclusion noise | selected scale architecture after conformance work |
| pairwise/large-kernel/advanced ReSTIR | can improve robustness or reuse radius | added rays, state, theory, and implementation risk | watchlist; not first delivery slice |

## Light And Shading Findings

Sparkle's public light vocabulary already suggests physical units. Khronos' punctual-light contract provides useful interchange semantics: point and spot intensity are candela, directional intensity is lux, point irradiance follows inverse-square behavior, and spot falloff is defined between inner and outer cone angles.[^5] Rectangular emitters require an area-to-solid-angle PDF conversion and explicit one/two-sided policy; they are not covered by the punctual extension.

The glTF material model requires non-negative, energy-conserving metallic-roughness behavior and defines texture/channel/color-space semantics, while allowing implementation choices in its informative BRDF material.[^6] OpenPBR broadens the design space to layered coat, fuzz, transmission, subsurface, and anisotropy.[^7] Sparkle should freeze and prove its current diffuse/specular/wrap-subsurface allocation before adding those lobes. Expanding the material model while its current energy split is unknown would multiply sampling and denoising ambiguity.

PBRT's light-sampling treatment is the baseline for measure discipline: selection PMF and conditional shape/directional PDF are distinct; finite area sampling converts area density to solid-angle density with squared distance and emitter cosine; delta lights do not share the same continuous PDF handling.[^8]

| Shading concern | Required decision/test |
| --- | --- |
| direction convention | every `wi/wo`, light direction, normal, and handedness fixed once |
| units/color | photometric input conversion, Rec.709 working basis, exposure independence, and finite maximums |
| lobe energy | diffuse, metal specular, dielectric Fresnel, and wrap-subsurface allocation sum consistently |
| roughness | perceptual-to-alpha mapping, minimum roughness, GGX support, VNDF versus NDF sampling |
| shading normals | correction/clamp policy and geometric-normal visibility boundary |
| area lights | sampled point, normal, sidedness, area PDF, solid-angle conversion, and near-field behavior |
| emissive geometry | inventory identity, texture integration, animation/update policy, and double-count prevention |
| environment | background emission versus a selectable direct-light candidate, with one mapping/PDF identity |

## Visibility And Shadowing Survey

DirectX Raytracing and Vulkan define both inline ray-query and ray-pipeline mechanisms with different execution/binding machinery.[^9][^10] Sparkle already exposes this distinction, so the architecture should retain one semantic visibility product and make backend/provider lowering prove equivalence. It should not introduce a generic “shadow backend” framework ahead of a second admitted mechanism.

| Technique | Benefits | Failure/cost profile | Target decision |
| --- | --- | --- | --- |
| hard ray query | direct segment visibility, easy selected-sample coupling | traversal divergence, alpha/material access, self-hit robustness | required baseline |
| ray pipeline | alpha/hit extensibility and full RT stages | SBT/pipeline lifecycle and backend complexity | required parity where capability is ready |
| stochastic area-light rays | physically meaningful penumbra | high variance, reconstruction dependency | required for finite emitters |
| screen-space trace | cheap near-visible detail | off-screen/occluded geometry loss and view dependence | optional accelerator only, never sole truth |
| conventional shadow maps | broad hardware support and coherent filtering | per-light setup, bias, resolution, atlas/update cost | no current requirement; admission decision needed |
| virtual shadow maps | scalable cached high-resolution raster shadows | invalidation/cache/page cost and a large new subsystem | researched fallback candidate, not admitted[^11] |
| opacity micromaps | can reduce alpha any-hit cost | asset/build/update/capability complexity | defer until alpha traversal is measured |
| distance-field/cone visibility | inexpensive soft occlusion at scale | approximate geometry and leak/detail errors | reject as direct-light truth; possible future auxiliary term |

MegaLights can combine screen traces, hardware/software ray tracing, or virtual shadow maps, illustrating that sampling and shadow execution are separable decisions.[^4] Sparkle should preserve that separation but implement only the providers justified by its target hardware and release needs.

## Denoising And Reconstruction Survey

NRD provides API-independent spatiotemporal algorithms: REBLUR/RELAX for diffuse/specular radiance and SIGMA for per-light shadow signals. Its integration contract depends on correct depth, motion, normals, roughness, hit distance, exposure, and accumulation identity.[^12] AMD FidelityFX Denoiser documents a shadow path designed around a single jittered ray and a packed neighborhood mask, followed by temporal and spatial reconstruction.[^13] These are valuable vendor-neutral reference shapes even if Sparkle writes a smaller native implementation.

DLSS Ray Reconstruction is already an optional adjacent route, but it cannot be the only correctness or portability path. Every reconstructed result must retain access to the raw signal and exact guide identity. A denoiser can suppress variance while hiding bias, light loss, lag, or invalid reuse.

| Signal strategy | Use | Rejection condition |
| --- | --- | --- |
| no denoiser | analytic/statistical oracle and raw debugging | never used as product-quality proof at sparse sampling |
| feature-local temporal/spatial shadow filter | selected-light visibility or compact shadow signal | reject if it duplicates a whole reconstruction stack without measured need |
| RELAX/REBLUR-like classical radiance reconstruction | portable diffuse/specular baseline | require exact guide/roughness/hit-distance contract and license review before code adoption |
| SIGMA/FidelityFX-like shadow reconstruction | binary/stochastic shadow signal | require light-size/distance and history semantics compatible with Sparkle |
| DLSS Ray Reconstruction | optional high-end product route | capability-gated; cannot own base semantics or sole acceptance oracle |

## Current Sparkle Source Audit

| Observation | Evidence | Implication |
| --- | --- | --- |
| four uniform initial candidates | `RestirReservoirCommon.hlsli`, `DirectLightReservoir.hlsli` | insufficient for uneven/dense light sets; exact current estimator still needs tests |
| temporal cap `20`, spatial cap `32`, four fixed neighbors | same shader sources | constants are implementation facts, not justified quality settings |
| target is luminance of direct diffuse+specular+subsurface | `DirectLightReservoir.hlsli` | energy allocation and target support affect every reservoir decision |
| only packed normal and view distance gate reuse | temporal/spatial shaders and common include | material/object/roughness/disocclusion mismatch risk remains |
| sample identity is light type/index plus shape sample | reservoir pack/unpack | frame-to-frame light reordering needs translation or reset proof |
| Inline/Pipeline visibility exists | `DirectShadowSignal.cpp` and shader routes | useful semantic boundary; parity unproved |
| three full-resolution direct lobes | `LightingRenderTargets.cpp` | strong diagnostics but real bandwidth/memory cost must be measured |
| no dedicated direct denoiser/confidence | inspected lighting passes | product stability path is incomplete |

The user's report that the current image is broken raises priority but does not identify which row is defective. The first implementation stage must capture raw lobe, reservoir, light identity, and visibility evidence on analytic fixtures before changing estimator constants.

## Rejected Shortcuts

- Tune temporal caps, neighbors, or normal/depth thresholds until one scene looks acceptable.
- Integrate RTXDI wholesale while keeping a parallel Sparkle reservoir and duplicated scene/light representations.
- Add a denoiser before proving raw estimator support, units, and history validity.
- Call a fixed ray budget “unlimited lights” without an overlap stress curve.
- Make DLSS RR or NVIDIA hardware the only supported correctness path.
- Add shadow maps, ReGIR, emissive mesh extraction, opacity micromaps, or OpenPBR lobes in the first correctness slice without discovery admission.
- Use the in-engine Reference Path Tracer as truth until its owning dossier publishes an accepted candidate and independence analysis.

## Discovery Handoff

`DIR-D0` must freeze the light set, unit conversion, lobe allocation, deterministic baseline, reservoir equations/bias mode, stable light identity, history compatibility, visibility provider matrix, denoiser baseline, raw artifacts, quality tiers, and memory/time budgets. The recommended first vertical slice is one directional/point/spot/rect analytic matrix plus exhaustive GPU resolve and raw visibility; only then should reservoir replacement begin.

## Sources

[^1]: Bitterli et al., [Spatiotemporal Reservoir Resampling for Real-Time Ray Tracing with Dynamic Direct Lighting](https://research.nvidia.com/sites/default/files/pubs/2020-07_Spatiotemporal-reservoir-resampling/ReSTIR.pdf), ACM TOG 39(4), 2020.
[^2]: NVIDIA, [RTXDI integration guide](https://github.com/NVIDIA-RTX/RTXDI/blob/a6efab966b7c3b272da0461578eb56ac61c7cbff/Doc/Integration.md), revision `a6efab9`, accessed 2026-09-12.
[^3]: NVIDIA, [RTXDI noise and bias guide](https://github.com/NVIDIA-RTX/RTXDI/blob/a6efab966b7c3b272da0461578eb56ac61c7cbff/Doc/NoiseAndBias.md), revision `a6efab9`, accessed 2026-09-12.
[^4]: Epic Games, [MegaLights](https://dev.epicgames.com/documentation/unreal-engine/megalights-in-unreal-engine), accessed 2026-09-12.
[^5]: Khronos Group, [KHR_lights_punctual](https://github.com/KhronosGroup/glTF/blob/c18432787e6d545a1218c1926ccdcfaffd4c116b/extensions/2.0/Khronos/KHR_lights_punctual/README.md), revision `c184327`, accessed 2026-09-12.
[^6]: Khronos Group, [glTF 2.0 Specification](https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html), accessed 2026-09-12.
[^7]: Academy Software Foundation, [OpenPBR Surface](https://academysoftwarefoundation.github.io/OpenPBR/), accessed 2026-09-12.
[^8]: Pharr, Jakob, and Humphreys, [PBRT 4e: Light Sampling](https://pbr-book.org/4ed/Light_Sources/Light_Interface), 2023.
[^9]: Microsoft, [DirectX Raytracing Functional Specification](https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html), accessed 2026-09-12.
[^10]: Khronos Group, [Vulkan Specification—Ray Tracing](https://registry.khronos.org/vulkan/specs/latest-ratified/pdf/vkspec.pdf), accessed 2026-09-12.
[^11]: Epic Games, [Virtual Shadow Maps](https://dev.epicgames.com/documentation/unreal-engine/virtual-shadow-maps-in-unreal-engine), accessed 2026-09-12.
[^12]: NVIDIA, [NVIDIA Real-time Denoisers](https://github.com/NVIDIA-RTX/NRD/tree/bf877181058988ec5785f82c4189be0be75c1902), revision `bf87718`, accessed 2026-09-12.
[^13]: AMD, [FidelityFX Denoiser](https://gpuopen.com/manuals/fidelityfx_sdk/techniques/denoiser/), accessed 2026-09-12.

# Volumetric Lighting, Atmosphere, And ReSTIR Study

**Status:** primary-source research and discovery input; no implementation, budget, visual, or release claim

**Responsibility:** record primary volume/fog/atmosphere/cloud/ReSTIR precedent, current gaps, alternatives, failure lessons, transfer boundaries, and discovery implications

**Authority boundary:** this study informs [Discovery](Discovery.md); it cannot admit the feature, select local semantics/architecture, set Sparkle budgets, authorize code, or pass a future FCR

**Researched:** 2026-09-12; Sparkle negative audit at `8e4ffba225411965dc51c0b783e5f47a075c7e84`

## Research Decision

Use one participating-media semantic core with two execution tiers:

1. a production froxel path for global/height/local fog, direct single scattering, temporal reconstruction, physical sky, and aerial perspective;
2. a reference/stochastic path for heterogeneous media and multiple scattering, from which a volumetric ReSTIR profile may be admitted if it beats deterministic/tracking baselines in Sparkle's workloads.

Do not begin with reservoir resampling. Volumetric ReSTIR requires a correct medium, free-flight/transmittance estimator, path representation, final evaluation, and independent oracle. The 2021 paper demonstrates a promising path-space method but reports interactive rather than current game-frame costs in its examples; it is research precedent, not the first fog implementation.[^1]

Atmospheric sky is part of this dossier because sky radiance, sun attenuation, and aerial perspective share atmosphere parameters and LUTs. The existing HDR sky remains a separate image-based environment mode. Clouds remain a deferred subfeature with their own density/weather/content burden.

## Research Method

The survey prioritizes peer-reviewed papers/books, author project pages, official engine presentations, official SDK repositories, and official product documentation. Every external quality/timing number belongs to its source. Architecture comparisons identify mechanisms and failure modes; `VOL-D0` must measure Sparkle before choosing formats, resolution, step counts, budgets, or quality defaults.

## Physical Foundation

The radiative transfer equation separates extinction, out-scattering, in-scattering, and emission along a ray. PBRT's volume chapters provide the reference vocabulary for absorption `sigma_a`, scattering `sigma_s`, extinction `sigma_t`, transmittance, phase functions, majorants, free-flight sampling, null scattering, ratio tracking, and multiple-scattering estimators.[^2]

This foundation matters even for a raster/froxel implementation. “Fog color and density” can be a friendly authored layer, but the resolved renderer state must have explicit coefficients, distance units, phase, and emission so direct lighting, shadows, atmosphere, and path references agree.

| Physical concern | Common shortcut | Failure if unowned |
| --- | --- | --- |
| optical coefficients | arbitrary density/color | scale changes alter appearance unpredictably; energy/unit mismatch |
| Beer transmittance | linear opacity per step | step-count dependence and negative transmission |
| phase function | constant anisotropy tweak | non-normalized energy, instability near `g=±1` |
| heterogeneous tracking | fixed march only | missed thin features or excessive empty-space work |
| multiple scattering | ambient color | disconnected from lights/optical depth and double counting |
| emission | add color after integration | wrong attenuation and ordering |

## Real-Time Unified Fog/Froxel Architecture

Frostbite's unified volumetric presentation is foundational production precedent: represent participating media through a volume, inject different media sources, compute lighting/shadowing, and render them through one physically based framework instead of effect-specific fog hacks.[^3] Modern implementations commonly use a camera-aligned froxel grid to amortize density/light evaluation and integrate front to back.

| Design axis | Options | Recommended discovery comparison |
| --- | --- | --- |
| grid | linear view-z, logarithmic/exponential z, clip/cascade volumes | choose by near detail, far atmosphere, memory, and temporal stability |
| density injection | full-screen global/height, analytic local shapes, rasterized bounds, 3D texture | start global/height + bounded analytic shapes |
| light injection | per-light froxel loop, clustered lists, stochastic/reservoir selection | deterministic small-light first; reuse Direct light distributions only by exact semantic bridge |
| shadows | shadow map, ray query, volume shadow/transmittance map, none | one explicit geometry visibility plus medium transmittance policy per tier |
| integration | preintegrated froxels, per-pixel march, hybrid | froxel prefix/integration for product; pixel/tracking reference |
| temporal | jittered froxels, reprojection, neighborhood clamp, responsive mask | mandatory after static correctness |
| multiple scatter | heuristic ambient, diffusion/LUT, stochastic path | physically labeled approximation or reference path; never hidden tint |

MegaLights' current documentation is relevant because it extends bounded stochastic light sampling to shared volumetric-fog/translucency froxels with denoising.[^4] This shows a product direction for many volume lights, but the same fixed-sample limitation applies: overlapping important lights increase noise. Sparkle should first share immutable light identity/distributions, not Direct Lighting's screen-space reservoirs.

## Atmosphere And Aerial Perspective

Hillaire's 2020 sky-atmosphere method targets dynamic ground-to-space views with scalable LUTs and a multiple-scattering approximation.[^5] It is the recommended architecture precedent because it covers the product range Sparkle wants without requiring fully precomputed static atmosphere. Bruneton's precomputed atmosphere implementation remains a valuable independent reference with dimensional/unit tests and Earth-like comparisons.[^6]

The atmosphere owner must unite:

- planet/ground geometry and world-to-planet transform;
- Rayleigh density/scattering, Mie density/scattering/extinction/phase, and absorption profile;
- solar irradiance, disk/angular extent, and atmosphere-light direction;
- transmittance, multi-scattering, sky-view, and aerial-perspective LUT identities;
- background sky and surface/volume aerial perspective composition;
- environment radiance/PDF generation for Direct/Indirect surface transport.

Epic's Sky Atmosphere documentation provides current product precedent for Rayleigh, Mie, absorption, aerial perspective, atmosphere lights, and ground-to-space views.[^7] It is not a mathematical oracle.

| Atmosphere failure | Cause to test |
| --- | --- |
| horizon band/ringing | LUT parameterization/filtering/precision/boundaries |
| incorrect sun/sky exposure relation | mixed radiometric/photometric/pre-exposure units |
| double sun disk or sky | background and atmosphere/direct light counted separately |
| ground/space discontinuity | planet intersection and LUT domain mismatch |
| aerial perspective halo | depth/composition/reprojection mismatch |
| stale sky after parameter edit | LUT generation/publication not transactional |

## Heterogeneous Volumes And Tracking

Dense 3D textures are the smallest useful heterogeneous input. Sparse OpenVDB is an industry interchange structure; NanoVDB provides a GPU-friendly read-only representation.[^8] File support is not rendering support: importer/cooker must freeze units, grid class, transform, channel/range, background, filtering, majorants, bounds, compression, content hash, and missing/corrupt behavior.

For stochastic reference transport, compare:

| Estimator | Strength | Risk |
| --- | --- | --- |
| fixed-step ray march | simple deterministic baseline | biased/step dependent; wastes empty space |
| delta/Woodcock tracking | unbiased free-flight with majorant | null-collision cost when majorant is loose |
| ratio tracking | transmittance estimator with heterogeneous media | variance and negative/unstable variants if assumptions fail |
| residual/decomposition tracking | improved control-variate behavior | control-field/majorant construction complexity |
| hierarchical empty-space skipping | reduces sparse traversal | acceleration structure/build/lifetime cost |

The selected product integrator may remain biased within a declared error/budget, while the reference estimator must state its convergence conditions. They cannot share an unproved fixed step and validate one another.

## Volumetric ReSTIR

Lin, Wyman, and Yuksel extend spatiotemporal reservoir resampling to volumetric path space. Their method evaluates many reuse candidates with cheaper approximate scattering/transmittance and performs an unbiased final evaluation for the one selected path; it supports heterogeneous media, dynamic lighting, emission, and multiple scattering in the paper domain.[^1]

This yields specific architectural requirements:

- resampled state is a volume path with camera/sample domain, free-flight vertices, scattering directions, terminal light/emission, proposal factors, and tracking identity;
- shifts between pixels/frames define mappings, support, inverse/Jacobian, and current medium/light generations;
- approximate candidate transmittance is allowed only in the target/resampling formulation proved by the estimator;
- final selected-path transmittance/scattering evaluation remains the accepted unbiased/controlled estimator;
- reservoirs do not reuse surface-GBuffer compatibility as a substitute for volume-path validity;
- temporal reuse must account for moving density fields, transforms, lights, camera, and majorant/control-field generations.

| ReSTIR volume scope | First experiment | Admission bar |
| --- | --- | --- |
| single scattering, complex direct lights | homogeneous/heterogeneous slab with known reference | lower error at equal time without biased composition |
| environment lighting | rotating HDR environment and heterogeneous volume | correct mapping/PDF and motion response |
| volume emission | animated emissive density | no stale emission trails or double counting |
| multiple scattering | bounded 2–3-scatter reference cells | clear quality/time benefit within product budget |
| dense many-light froxels | local lights over fog/volume | compare against per-froxel deterministic and simple stochastic sampling |

The paper's example times—tens to over one hundred milliseconds—must not be used as Sparkle budgets.[^1] The likely first product use is reference/high-quality or reservoir-assisted light injection, followed by more aggressive profiles only if measurements justify them.

## Clouds

Production volumetric clouds combine authored weather/density fields, multi-scale procedural detail, ray marching, lighting/multiple-scattering approximations, ground/cloud shadows, atmosphere coupling, temporal reconstruction, and aggressive empty-space/LOD optimization. Guerrilla's Nubis work emphasizes close/fly-through quality and temporal-artifact control, demonstrating that clouds are a substantial product rather than a fog preset.[^9]

Epic documents a ray-marched cloud volume with approximate multiple scattering and a quality/performance choice between secondary ray marching and Beer shadow maps.[^10] Sparkle should admit clouds only after atmosphere and heterogeneous volume foundations pass; cloud authoring/weather UX and acceptance deserve a subordinate package or expansion at that point.

## Composition And Transparency

The authoritative form is premultiplied volume radiance and transmittance:

```text
Lout = T(camera, surfaceDepth) * Lsurface + Lscatter(camera, surfaceDepth).
```

Background sky uses the camera ray's atmosphere/medium interval to its defined far/planet boundary. Opaque depth bounds the integration for surfaces. Transparent surfaces require ordered surface/medium interaction and are not implied by a post-lighting fog blend. Epic's environmental-lighting overview demonstrates how fog, clouds, sky atmosphere, and lights interact in a production engine, but Sparkle must freeze its own pass order and supported transparency domain.[^11]

## Temporal Reconstruction And Denoising

Froxel grids are low resolution and normally jittered/reprojected. History validity must include View/extent/projection/jitter, depth, medium volumes/transforms/textures, lights/shadows, atmosphere/LUTs, algorithm/quality, shader/provider, and exposure convention. Density/emission changes need responsive confidence to avoid smoke/fog trails.

Volume reconstruction must measure:

- thin shafts and volume boundaries under motion;
- high-anisotropy highlights;
- animated lights/emission/density;
- camera cuts and rapid translation through media;
- disocclusion against foreground geometry;
- low-frequency bias versus high-frequency noise;
- upsample depth/normal edge leakage;
- raw versus reconstructed transmittance and radiance.

No neural/vendor denoiser is assumed. Begin with a transparent spatiotemporal filter and exact raw capture; advanced denoising enters only through measured admission.

## Optimization Portfolio

Apply only after static semantics pass:

- logarithmic/cascaded froxel depth and resolution scaling;
- analytic bounds and tiled/clustered medium/light culling;
- light distribution/RIS or reservoir sampling per froxel;
- density majorants, hierarchical occupancy, empty-space skipping;
- temporal sample rotation and confidence-driven accumulation;
- half precision only where analytic/error tests pass;
- async compute only with dependency/overlap/timing proof;
- LUT caching by immutable atmosphere generation;
- pass fusion only when intermediate oracle state remains reproducible;
- checkerboard/variable-rate sampling only with motion/detail acceptance.

## Current Sparkle Gap And Boundary

| Existing fact | Correct interpretation |
| --- | --- |
| image-based `SceneSkyDesc` and `Sky.cpp` | environment/background, not atmosphere or aerial perspective |
| direct/indirect surface lobes | input surface radiance to future volume composition, not volume lighting |
| ray query/pipeline support | possible visibility/tracking mechanism, not a volume feature |
| metre-based world/reference contract | useful unit foundation, still needs medium authoring/cook/scene contract |
| no volume import/GPU-scene/pass/shader/history | current feature remains 0/100 and negative |
| future OpenVDB workload is disabled | heterogeneous/sparse asset acceptance remains blocked by content admission |

## Rejected Shortcuts

- Add exponential fog as a post-tone-map color blend.
- Treat HDR sky, bloom, wrap subsurface, alpha, or light shafts without extinction as volume support.
- Build separate systems for height fog, local fog, atmosphere, clouds, and ReSTIR with duplicate media/light/history state.
- Start Volumetric ReSTIR before a volume reference integrator and transmittance tests exist.
- Copy paper step counts, grid sizes, or performance numbers into product defaults.
- Add OpenVDB parsing without a cooked/GPU representation and actual rendering consumer.
- Use a temporal filter to conceal incorrect transmittance/composition.
- Add clouds to the initial fog/atmosphere slice.

## Discovery Handoff

`VOL-D0` must obtain roadmap admission and freeze medium/content scope, units, overlap, phase, transmittance/reference estimator, froxel layout, direct-light/shadow strategy, temporal reconstruction, composition order, atmosphere/LUT/environment bridge, heterogeneous asset path, Volumetric ReSTIR domain, diagnostics, budgets, backend requirements, and evidence ownership.

## Sources

[^1]: Lin, Wyman, and Yuksel, [Fast Volume Rendering with Spatiotemporal Reservoir Resampling](https://graphics.cs.utah.edu/research/projects/volumetric-restir/volumetric_restir.pdf), ACM TOG 40(6), 2021; [project page](https://graphics.cs.utah.edu/research/projects/volumetric-restir/).
[^2]: Pharr, Jakob, and Humphreys, [PBRT 4e: Volume Scattering](https://www.pbr-book.org/4ed/Volume_Scattering) and [Volume Scattering Integrators](https://pbr-book.org/4ed/Light_Transport_II_Volume_Rendering/Volume_Scattering_Integrators), 2023.
[^3]: Hillaire, [Towards Unified and Physically-Based Volumetric Lighting in Frostbite](https://www.advances.realtimerendering.com/s2015/index.html), SIGGRAPH Advances in Real-Time Rendering, 2015.
[^4]: Epic Games, [MegaLights](https://dev.epicgames.com/documentation/unreal-engine/megalights-in-unreal-engine), accessed 2026-09-12.
[^5]: Hillaire, [A Scalable and Production Ready Sky and Atmosphere Rendering Technique](https://diglib.eg.org/items/8a3e5350-18b3-46bd-9274-3add5af88c75), Computer Graphics Forum 39(4), 2020.
[^6]: Bruneton, [Precomputed Atmospheric Scattering: implementation and tests](https://ebruneton.github.io/precomputed_atmospheric_scattering/), 2017 revision of Bruneton and Neyret 2008.
[^7]: Epic Games, [Sky Atmosphere](https://dev.epicgames.com/documentation/unreal-engine/sky-atmosphere-component-in-unreal-engine), accessed 2026-09-12.
[^8]: Academy Software Foundation, [OpenVDB documentation](https://www.openvdb.org/documentation/) and [NanoVDB](https://www.openvdb.org/documentation/doxygen/NanoVDB_MainPage.html), accessed 2026-09-12.
[^9]: Guerrilla Games, [Nubis Evolved: Real-Time Volumetric Clouds in Horizon Forbidden West](https://www.guerrilla-games.com/read/nubis-evolved), 2023.
[^10]: Epic Games, [Volumetric Cloud Component](https://dev.epicgames.com/documentation/unreal-engine/volumetric-cloud-component-in-unreal-engine), accessed 2026-09-12.
[^11]: Epic Games, [Environmental Light with Fog, Clouds, Sky and Atmosphere](https://dev.epicgames.com/documentation/unreal-engine/environmental-light-with-fog-clouds-sky-and-atmosphere-in-unreal-engine) and [Volumetric Fog](https://dev.epicgames.com/documentation/unreal-engine/volumetric-fog-in-unreal-engine), accessed 2026-09-12.

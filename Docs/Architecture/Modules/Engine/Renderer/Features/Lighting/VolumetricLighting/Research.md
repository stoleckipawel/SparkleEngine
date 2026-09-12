# Volumetric Lighting, Atmosphere, And ReSTIR Study

**Status:** primary-source research and discovery input; no implementation, budget, visual, or release claim

**Responsibility:** record primary volume/fog/atmosphere/cloud/ReSTIR precedent, current gaps, alternatives, failure lessons, transfer boundaries, and discovery implications

**Authority boundary:** this study informs [Discovery](Discovery.md); it cannot admit the feature, select local semantics/architecture, set Sparkle budgets, authorize code, or pass a future FCR

**Researched:** 2026-09-12; Sparkle source re-audit at `8b650c7450f8a59fb3bcc18edbb4d217a7b11ed5`; mutable reference repositories were checked against their listed HEAD revisions

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

## State-Of-The-Art Architecture Map

The target needs two related but independently provable estimators: a production real-time froxel system for fog/atmosphere and a stochastic volume reference/reservoir track for heterogeneous and multiple-scattering workloads. A single “volumetric lighting” label must not hide which one produced the image.

| Layer | Mature/advanced families | First Sparkle target | Later admission trigger |
| --- | --- | --- | --- |
| medium semantics | homogeneous, exponential height, analytic local primitives, dense/sparse fields | inverse-metre coefficients plus global/height medium | authored local and heterogeneous content need |
| camera integration | analytic slab, deterministic ray march, froxel integration | current-frame analytic/froxel single scattering | measured aliasing/performance justifies adaptive/cascaded layout |
| light selection | all lights, clustered/tiled lists, stochastic froxel selection, path-space reservoirs | deterministic/clustered admitted analytic lights | fixed-work many-light volume pressure survives clustering |
| transmittance | analytic, fixed quadrature, delta/ratio/residual ratio tracking, guided/control-variate variants | analytic plus deterministic production march and stochastic reference | heterogeneous quality/cost demonstrates a better estimator[^12][^13] |
| multiple scattering | empirical compensation, diffusion/ambient approximations, LUTs, probe/froxel final gather, path tracing | explicitly named approximation per tier | reference comparison and product budget admit broader transport |
| physical atmosphere | Bruneton precomputation, Hillaire scalable LUTs, direct ray march | Hillaire/Bruneton-comparable LUT route with independent hand cases | multiple planets/lights or spectral mode receives separate scope |
| path resampling | Volumetric ReSTIR with approximate candidate and unbiased selected evaluation | research track after reference transport | equal-time error win in admitted complex-media workload[^1] |
| sparse volume data | dense 3D texture, OpenVDB cook, NanoVDB GPU hierarchy, bricks/majorants | dense-first unless discovery proves sparse need | representative VDB content and memory/empty-space evidence |
| reconstruction | reprojection/clamp, variance-guided filtering, separated surface/volume learned denoising | portable transmittance/in-scatter temporal baseline | optional learned provider improves frozen cells without owning semantics[^19] |
| clouds | froxel/weather fields, procedural ray marching, shadow/ambient/multiple-scatter approximations | separately admitted extension of atmosphere/media | named open-world cloudscape content, authoring, and budget owner |

Frostbite's unified volumetric work, Unity HDRP's froxel implementation, Samurai Cinema's single compute-oriented haze path, and Lumen's froxel final gather show different production decompositions.[^3][^15][^16][^17] They are architectural precedent only. Sparkle must measure its own dimensions, formats, queues, sample counts, and budgets.

## Production Froxel Reference Pipeline

```text
author medium/atmosphere/local-volume intent
  -> validate, serialize, cook and publish immutable scene generations
  -> choose camera-relative froxel grid and depth mapping
  -> resolve coefficient fields once with deterministic overlap/capacity
  -> cull/select lights in the volume receiver domain
  -> evaluate geometry visibility and medium transmittance separately
  -> accumulate raw single-scatter source and emission per froxel
  -> integrate front-to-back into premultiplied in-scatter + transmittance
  -> reproject/reconstruct only with current depth/motion/generation validity
  -> compose once with opaque surface and chosen sky/environment
  -> expose raw products, status and capture identity
```

Each step owns one inspectable product on bounded fixtures. Production may fuse passes only after a diagnostic mode reproduces coefficient, light-source, transmittance, in-scatter, history/confidence, and composition products. Atmosphere LUT generation and heterogeneous asset upload publish atomically; mixed generations are invalid.

## Froxel Design Investigation

| Decision | Required comparison | Failure pressure |
| --- | --- | --- |
| XY resolution | full, half, tile-aligned and dynamic profiles at equal time/memory | edge halos, thin shaft loss, cache/bandwidth cost |
| Z mapping | linear, logarithmic/exponential, and bounded hybrid with exact inverse | near-camera banding versus far-horizon under-sampling |
| stored quantity | coefficients, source radiance, integrated radiance/transmittance, or normalized source | interpolation correctness, format range, composition and transparency consumers |
| format | FP16/FP32/R11G11B10-like candidates per semantic product | negative/overflow/underflow, precision at high optical depth, filtering support |
| light list | screen tiles, 3D clusters, flat bitset, stochastic selection | divergent loops, list overflow, globally large lights, build cost |
| integration | serial Z, wave/quad-swizzled, scan/prefix alternatives | synchronization, ordering, numerical drift, backend portability |
| history | integrated versus source/coefficient history, neighborhood clamp and confidence | camera motion, local density edits, moving shadows, disocclusion trails |

The Samurai Cinema presentation is a useful warning: coarse frustum volumes can alias horizon/thin haze, and storing a normalized in-scattered quantity can move opacity evaluation to the final pixel.[^16] This is a candidate representation, not a decision; Sparkle must prove interpolation and composition for its own analytic density fields.

## Heterogeneous Tracking Deep Dive

| Estimator | Required inputs | Strength | Failure/variance mode | Sparkle role |
| --- | --- | --- | --- | --- |
| fixed-step quadrature | density sampler, step rule, bounds | deterministic and GPU coherent | bias/aliasing from under-sampling; work in empty space | mandatory product baseline with convergence sweep |
| delta/Woodcock tracking | valid majorant and extinction samples | unbiased collision sampling under valid majorant | null-collision cost in loose/empty majorants | reference candidate |
| ratio tracking | majorant and per-channel residual weights | unbiased transmittance estimate | high variance or signed weights depending formulation and spectrum | reference candidate |
| residual ratio tracking | analytic/control extinction plus residual majorant | can reduce variance in structured media | control/majorant construction and violation handling | advanced reference candidate[^12] |
| guided/zero-variance-derived tracking | approximate transmittance/control model | principled direction for variance reduction | model/build cost and no free perfect control | research pressure, not first product[^13] |

Every stochastic tracker requires an explicit measure, channel policy, majorant proof/validation, collision/null-event probability, boundary rule, RNG dimension, weight update, termination, and finite behavior. A majorant violation is a terminal invariant failure for evidence; clamping density to hide it changes the estimator and is not recovery. Deterministic marching and stochastic tracking use the same world-to-medium transform and coefficient sampler so discrepancies isolate integration rather than content interpretation.

## Volumetric ReSTIR Deep Dive

The volume paper's pivotal architecture is asymmetric: many candidate paths may use cheap approximate scattering/transmittance for resampling, but the selected path is evaluated with the accepted unbiased estimator so candidate approximation does not directly become the final estimator.[^1] That transfer is valid only if Sparkle freezes:

1. integration domain: camera/media path length, direct versus multiple scattering, environment/emission and surface terminal rules;
2. sample record: scattering positions/directions, free-flight choices, medium/light/content generations, technique and probability facts;
3. candidate approximation: exact support, bounded/finite output, deterministic configuration and no false zero for contributing paths;
4. final evaluation: reference transmittance/collision estimator, visibility, contribution weight, target and normalization;
5. shift mapping: moved/reconnected vertices, inverse/support/Jacobian, medium-boundary and density-field change policy;
6. temporal/spatial proposal: receiver/path motion, density/light/majorant mutations, duplication/correlation and disocclusion;
7. product claim: path-space reservoir versus merely selecting lights for froxels.

| Possible domain | Benefit hypothesis | Blocking risk | Decision |
| --- | --- | --- | --- |
| per-froxel direct-light reservoir | bounded many-light source evaluation | receiver distribution differs from surfaces; visibility/transmittance cost | evaluate first if many-light fog alone is the need; do not call path-space ReSTIR |
| camera single-scatter path reservoir | reuse free-flight/light samples | medium motion/support/Jacobian and transmittance correlation | first plausible path-space experiment after reference tracker |
| heterogeneous multiple-scatter reservoir | major quality potential in dense complex lighting | large record, expensive shifts/final evaluation, extreme correlation | advanced conditional tier |
| emissive/environment volume paths | handles nonlocal source complexity | terminal-technique accounting and proposal support | separately admitted cells |

No volume reservoir work begins from a surface `DirectLightReservoir` or `RestirIndirectReservoir` representation. Those may donate immutable light/environment facts only; the receiver domain, path state, history, and estimator remain volume-owned.

## Atmosphere, Sky, And Environment Consistency

The Hillaire and Bruneton references both give testable sky/atmosphere implementations, but use different precomputation and parameterization tradeoffs.[^5][^6] The Hillaire source implementation is pinned here to make the comparison reproducible.[^18]

| Product | Authoritative generation | Prohibited double application |
| --- | --- | --- |
| sky background | physical-atmosphere or image-environment mode | separate image Sky fill after physical sky is already composed |
| aerial perspective | atmosphere generation plus camera/depth | reapplying atmosphere in presentation or fog composite |
| surface direct sun | shared celestial light identity and atmospheric attenuation policy | both pre-attenuated light and another atmosphere transmittance term |
| surface indirect environment | one mapping/radiance/PDF generation | background texture and atmosphere LUT sampled as two independent skies |
| local fog lighting | current medium/light/atmosphere generation | using surface lighting composite as incident radiance without defined split |

Required reference cells include zero atmosphere, pure absorption, Rayleigh-only, aerosol/Mie-only, absorption-band/ozone effect, ground albedo extremes, sun at zenith/horizon/below horizon, observer ground/high-altitude/space, planet shadow, parameter edits, and LUT failure. All comparisons occur in raw radiance/transmittance before exposure.

## Reconstruction And Composition Research

Surface denoisers depend on geometry guides that are not automatically valid inside media. Sparse-volume reconstruction research separates surface and volume layers, reconstructs transmittance/volume information, and combines them explicitly.[^19] Sparkle's first portable route need not be neural, but must respect the same separation:

```text
raw surface radiance/depth
raw volume transmittance + premultiplied in-scatter
volume history/confidence with medium/light generations
optional surface history/provider
one composition edge
presentation after composition
```

The core oracle is always `Lout = T * Lsurface + Lscatter`. Transparent objects need a separately ratified sampling/composition contract; sampling a camera-integrated froxel product at an arbitrary transparent fragment is not automatically correct. Clouds likewise compose through the same transmittance/radiance algebra and atmosphere generation rather than a special color blend.

## External Source And Provenance Ledger

| Source | Observed fact used | Permitted transfer | Forbidden inference | Provenance action before implementation |
| --- | --- | --- | --- | --- |
| Volumetric ReSTIR paper/project[^1] | approximate candidate plus accepted selected evaluation and path-resampling architecture | equations, test scenes/failure hypotheses | source timings, quality, or unbiased claim transfer to Sparkle | cite exact paper; audit any project code/license separately |
| PBRT 4e[^2] | RTE, medium interfaces, tracking/reference procedures | analytic/CPU reference concepts | real-time architecture or independent oracle if code is shared | cite edition and disclose shared code/equations |
| Frostbite/HDRP/Samurai/Lumen courses[^3][^15][^16][^17] | production froxel, lighting, temporal, and integration patterns | architecture/workload comparison | exact dimensions, timings or quality become budgets | citation only; record title/year and source-specific assumptions |
| MegaLights/Epic environment docs[^4][^7][^10][^11] | product interaction and current commercial-engine behavior | UX/failure/workload questions | parity or transferable implementation details | citation/date only unless source code is separately licensed |
| Hillaire/Bruneton atmosphere[^5][^6][^18] | production LUT and tested reference options | independent comparison, hand cases, possible code study | one implementation is automatically best or license-cleared | pin repo `183ead5`; retain LICENSE/notices and modifications before transfer |
| OpenVDB/NanoVDB[^8] | sparse asset/runtime structures | content schema and A/B reference | both formats must ship or source assets may parse at runtime | pin chosen code source, audit license/dependencies, record asset rights |
| Nubis/Epic clouds[^9][^10] | production cloud authoring/rendering precedents | workload and UX requirements | cloud scope is admitted or timings transfer | citation only until separate product/content decision |
| residual/zero-variance tracking[^12][^13] | unbiased/control-variate transmittance families | reference algorithms and adversarial tests | loose majorants or approximate controls are safe without proof | cite equations; independently implement; document majorant/control source |
| Pixar production volume course[^14] | production path/medium decomposition and tracking taxonomy | completeness checklist and reference cases | film architecture/cost fits real time | citation only unless code is separately sourced |
| sparse-volume reconstruction[^19] | surface/volume signal separation and learned reconstruction precedent | interface/failure design and A/B hypothesis | neural denoising is required or source quality transfers | citation first; code/model/assets need separate license and provenance review |

Git checks on 2026-09-12 confirmed `183ead5` as the Hillaire atmosphere repository HEAD and `6f0a32f` as OpenVDB HEAD. The current study references OpenVDB documentation rather than adopting HEAD; any code stage must choose and pin a reviewed release/commit.

## Adoption And Rejection Matrix

| Candidate | Disposition | Admission evidence | Rejection/removal evidence |
| --- | --- | --- | --- |
| analytic homogeneous/height fog | required first tier after roadmap gate | Beer/single-scatter/phase/composition conformance and first-use UX | only blocked by product admission or failure to define one coherent owner |
| unified froxel local-light product | proposed required | grid/inverse, overlap, light/shadow, motion and equal-quality budget evidence | cannot meet composition/history/backend contract |
| physical atmosphere | proposed distinct tier | Hillaire/Bruneton/analytic raw comparison and one environment-generation proof | double ownership with image sky or budget/product scope rejection |
| dense heterogeneous texture | proposed first content tier | representative content, transform/filter/range/cook and marching/reference convergence | no product content or memory/bandwidth failure |
| OpenVDB/NanoVDB | conditional | sparse workload beats dense including cook/upload/memory and rights | duplicate canonical data or insufficient target benefit |
| residual/guided tracking | reference/advanced | variance win with valid majorants/controls on accepted assets | no equal-work benefit or unsafe violation behavior |
| per-froxel light reservoir | conditional | many-light fog failure and equal-time improvement | clustering/all-lights baseline satisfies workload |
| path-space Volumetric ReSTIR | advanced conditional | exact estimator plus equal-time raw error/motion win | missing mapping/final estimator, excess memory/correlation, or no win |
| learned volume reconstruction | research only | portable baseline fails named workload and provider passes provenance/product gates | sole functional path, non-reproducible model, or hidden raw defects |
| clouds | separately admitted | content/authoring/weather/scale/budget owner and full workload | no product owner or parallel sky/fog system required |

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
[^12]: Novák, Selle, and Jarosz, [Residual Ratio Tracking for Estimating Attenuation in Participating Media](https://www.jannovak.info/publications/RRTracking/index.html), ACM TOG 33(6), 2014.
[^13]: d'Eon and Novák, [Zero-variance Transmittance Estimation](https://research.nvidia.com/labs/rtr/publication/deon2021zerovar/), EGSR 2021.
[^14]: Fong et al., [Production Volume Rendering](https://graphics.pixar.com/library/ProductionVolumeRendering/paper.pdf), SIGGRAPH Courses, 2017.
[^15]: Křivánek et al., [Real-time Volumetric Rendering in Unity High Definition Render Pipeline](https://www.advances.realtimerendering.com/s2018/Siggraph%202018%20HDRP%20talk_with%20notes.pdf), SIGGRAPH Advances in Real-Time Rendering, 2018.
[^16]: Patry, [Real-Time Samurai Cinema](https://advances.realtimerendering.com/s2021/jpatry_advances2021/index.html), SIGGRAPH Advances in Real-Time Rendering, 2021.
[^17]: Wright et al., [Lumen: Real-time Global Illumination in Unreal Engine 5](https://advances.realtimerendering.com/s2022/SIGGRAPH2022-Advances-Lumen-Wright%20et%20al.pdf), SIGGRAPH Advances in Real-Time Rendering, 2022.
[^18]: Hillaire, [UnrealEngineSkyAtmosphere reference implementation](https://github.com/sebh/UnrealEngineSkyAtmosphere/tree/183ead5bdacc701b3b626347a680a2f3cd3d4fbd), revision `183ead5`, accessed 2026-09-12.
[^19]: Hofmann et al., [Interactive Path Tracing and Reconstruction of Sparse Volumes](https://research.nvidia.com/labs/rtr/publication/hofmann2021volumerecon/), I3D 2021.

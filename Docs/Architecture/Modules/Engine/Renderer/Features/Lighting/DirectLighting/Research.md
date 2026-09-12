# Direct Lighting Completion Study

**Status:** primary-source research and decision input; precedent does not prove Sparkle implementation, suitability, performance, or release readiness

**Responsibility:** record current-source findings, primary external precedent, alternatives, failure lessons, permitted transfers, and forbidden inferences for `DIR-D0`

**Authority boundary:** this study informs [Discovery](Discovery.md) but cannot select the product, settle local math/architecture, authorize implementation, or pass `FCR-REN-06`

**Researched:** 2026-09-12; repository source re-audit at `8b650c7450f8a59fb3bcc18edbb4d217a7b11ed5`; mutable reference repositories were checked against their listed HEAD revisions

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

## State-Of-The-Art Algorithm Map

The useful comparison is a stack of separable decisions, not one branded renderer. Epic's current MegaLights documentation and SIGGRAPH presentation describe a fixed per-pixel stochastic sample/ray budget, importance sampling, screen and ray visibility options, temporal reconstruction, light-overlap limits, and a dedicated volume path.[^4][^14] RTXDI supplies a more explicit reusable ReSTIR DI/ReGIR implementation boundary.[^2] Neither determines Sparkle's light units, BRDF, frame graph, provider policy, or product budgets.

| Layer | Reference families surveyed | Best first Sparkle baseline | Advanced admission trigger |
| --- | --- | --- | --- |
| receiver/light pruning | exhaustive list, tiles/clusters, light BVH, ReGIR | exhaustive oracle plus power-weighted global distribution | measured misses/variance from globally distributed lights justify receiver/world-space structure |
| proposal mixture | uniform, power, solid-angle, BRDF-aware, environment/emissive distributions | full-support mixture with recorded technique PMF and conditional PDF | per-technique variance study proves another proposal repays state/build cost |
| reservoir reuse | RIS, canonical ReSTIR DI, pairwise MIS, MCMC decorrelation | canonical temporal then spatial ReSTIR with one frozen bias mode | persistent correlation, occlusion, or large-kernel failures survive correct base implementation[^15][^16] |
| visibility | exhaustive rays, selected rays, inline/pipeline RT, screen trace, shadow maps/VSM | selected finite-segment ray visibility with Inline/Pipeline parity where advertised | a non-ray tier or measured RT/alpha cost receives its own product and evidence admission |
| surface response | GGX NDF, GGX VNDF, multiple-scatter compensation/full multiple scatter, layered models | current admitted lobes with energy and sampling conformance | furnace/rough-metal error or new material requirement justifies a ratified model change[^17][^18] |
| reconstruction | raw accumulation, SVGF-like, NRD RELAX/REBLUR/SIGMA, FidelityFX, DLSS RR | raw oracle plus portable classical baseline | optional provider improves the same frozen workload without changing semantics[^19] |
| scale architecture | ReSTIR DI, ReGIR, MegaLights-like fixed work, ReSTIR-ranked shadow maps | renderer-owned ReSTIR DI and ray visibility | target hardware/content requires raster-shadow or distributed-light tier[^20] |

### Canonical Execution Reference

The first correctness target must be expressible as this auditable sequence; fusion is an optimization after equivalent artifacts exist:

```text
validate and compact current lights
  -> build stable logical-ID translation and proposal distributions
  -> generate N fresh full-support candidates per receiver
  -> reservoir stream update with exact target/source density
  -> temporal translation + compatibility + selected bias correction
  -> spatial neighbor proposal + compatibility + selected bias correction
  -> final selected-sample replay
  -> one finite visibility query
  -> raw lobe contribution + confidence/rejection products
  -> optional reconstruction
  -> ordinary lighting composition
```

Each arrow is a possible failure boundary. The implementation must be able to disable reuse, visibility, or reconstruction independently, and must compare the prefix result to the exhaustive baseline. A fused kernel is acceptable only if the same stage artifacts can be reproduced in a bounded diagnostic fixture.

### Proposal And Reuse Research Conclusions

- A selection PMF over lights and a conditional point/direction PDF must remain separate. Mixtures additionally store or reconstruct the selected technique and mixture density; a light index alone is insufficient.
- The initial distribution must retain nonzero support for every admitted contributing light. Power weighting is a baseline, not a guarantee: receiver geometry, visibility, emissive texture variation, and HDR environment peaks can still dominate variance.
- Temporal reuse requires previous-to-current light translation by stable logical identity. Reusing an array index after add/remove/reorder is a wrong-light defect, not merely extra noise.
- Spatial reuse changes the proposal population and correlation. Neighbor count cannot be treated as independent sample count, and large kernels require an explicitly ratified bias/MIS formulation.
- Pairwise MIS and MCMC mutation work are research candidates for difficult correlation/large-kernel cells, not patches to apply before canonical behavior is proved.[^15][^16]
- Final visibility belongs to the selected sample's current receiver/light segment. Visibility cached from a source receiver needs its own support and correction proof.

## PBR And Numerics Deep Dive

| Concern | Normative investigation | Failure oracle |
| --- | --- | --- |
| GGX sampling | compare NDF and visible-normal sampling using identical evaluation, measure, hemisphere, and roughness mapping | sampled directional histogram/PDF integral plus white-furnace energy sweep[^17] |
| microfacet energy | quantify single-scatter loss at rough conductors/dielectrics; choose full multiple scatter or an explicit compensation approximation | directional and white-furnace albedo over roughness/F0, compared to independent reference[^18] |
| shading normals | freeze geometric-normal support and any adjoint/non-symmetry correction; clamp policy must not create energy | bent-normal furnace, grazing view/light, backface/two-sided and normal-map extremes |
| finite emitters | preserve area sample, emitter normal/sidedness, distance, cosine, and area-to-solid-angle Jacobian | analytic disk/rectangle far-field and near-field numerical quadrature |
| photometry | document conversion from lux/candela/cd per square metre into working radiometric RGB assumptions | distance, cone, area, exposure, and scale metamorphic tests |
| lobe allocation | decide whether subsurface-wrap replaces or adds to diffuse and how metal/dielectric energy is reserved | white furnace and per-lobe sum over base color/metalness/roughness/subsurface |
| extremes | define zero target/PDF, denormals, near-zero distance, grazing cosine, maximum radiance, `M`, and weight caps | finite-value sweep and counter assertions; no clamp may silently bias an oracle case |

## Shadowing And Reconstruction Failure Taxonomy

| Symptom | Likely class | Required isolation |
| --- | --- | --- |
| missing or wrong light | proposal support, identity translation, capacity | exhaustive unshadowed comparison plus selected stable-ID view |
| detached or lagging shadow | stale reservoir, stale visibility, motion/disocclusion, denoiser history | freeze camera/light separately; compare raw visibility and reconstructed sequence |
| bright/dark edge halo | normal/depth compatibility, ray offset, guide convention, filter clamp | raw lobe/visibility/guide edge crops with geometric and shading normals |
| sparkling area light | shape replay/PDF/Jacobian, insufficient candidates, random dimensions | fixed receiver distribution test and deterministic seed replay |
| alpha foliage noise/cost | any-hit semantics, texture LOD, traversal divergence, reconstruction | opaque versus mask paired fixture with identical geometry and ray count |
| stable but biased image | target/normalization/bias correction or denoiser lag | long raw mean against exhaustive/reference; stability is not the oracle |
| lights vanish under overlap | fixed-budget saturation or proposal starvation | power/range/overlap sweep with eligible/selected diversity and error curve |

SVGF demonstrates the classical structure of temporal accumulation, variance estimation, and spatial filtering, but also documents motion/low-light failure modes.[^19] NRD and FidelityFX provide current product-shaped input/output precedents. Sparkle must decide signal type first: a binary/per-light shadow signal, a selected-sample radiance signal, and lobe-separated radiance do not share an interchangeable denoiser contract.

## External Source And Provenance Ledger

| Source | Observed fact used here | Permitted transfer | Forbidden inference | Provenance action before implementation |
| --- | --- | --- | --- | --- |
| ReSTIR DI paper[^1] | RIS reservoir and spatiotemporal direct-light reuse family | equations, experiments, failure hypotheses | current Sparkle reservoir conforms or inherits paper quality | cite paper/equation; independently derive and test code |
| RTXDI `a6efab9`[^2][^3] | current official integration, buffer/bridge, ReGIR, bias structures | executable comparison, layout/sequence review, differential fixtures | SDK ownership or performance automatically fits Sparkle | retain commit, LICENSE, notices, configuration, and modification record before copying code |
| MegaLights docs/presentation[^4][^14] | fixed-work product model, overlap limits, visibility/reconstruction architecture | workload/UX/failure requirements | feature parity, “unlimited” lights, or transferable timings | citation only; record engine/version/date |
| Khronos light/glTF specs[^5][^6] | interchange units and material data contract | conformance fixtures and import semantics | full BRDF or renderer equivalence | retain spec revision and test vector provenance |
| OpenPBR[^7] | modern layered material taxonomy | future decision vocabulary | scope admission for new lobes | citation/spec license review if schemas/code are reused |
| PBRT 4e[^8] | measure/PDF/reference procedures | independent hand cases and CPU reference concepts | real-time architecture or independent proof when code is shared | cite edition; disclose any shared equations/code |
| DXR/Vulkan specs[^9][^10] | backend traversal semantics/capabilities | native validation matrix | frontend parity without execution | retain spec/version, backend/device/driver identity |
| NRD `bf87718` and FidelityFX[^12][^13] | denoiser signals, guide and history precedents | interface comparison and optional provider study | correctness, license approval, or local quality | pin source, review license/notices, capture exact integration configuration |
| advanced ReSTIR/VNDF/SVGF/shadow-map work[^15][^16][^17][^18][^19][^20] | targeted solutions and known failure pressures | A/B hypotheses after base conformance | mandatory inclusion or source-reported gains | citation first; code-bearing sources require separate rights review |

Git checks on 2026-09-12 confirmed `a6efab9` as RTXDI HEAD and `bf87718` as NRD HEAD. That verifies the mutable source identity used by this study, not suitability or license clearance for code transfer.

## Adoption Decision Matrix

| Candidate | Decision state | Evidence needed to admit | Evidence that rejects/removes it |
| --- | --- | --- | --- |
| exhaustive small-light resolve | required oracle | analytic agreement and bounded fixture cost | never removed as test oracle; may remain non-shipping |
| power/full-support proposal mixture | proposed required | PDF normalization, support, variance versus uniform | inconsistent measure or no improvement at equal work |
| canonical ReSTIR DI | proposed scale path | CPU/discrete reservoir tests, raw GPU agreement, motion/identity matrix | normalization/identity/bias cannot be bounded |
| ReGIR/light BVH | conditional | distributed-light workload wins quality/time including build/memory | no equal-time benefit or duplicate light ownership |
| pairwise/MCMC reuse | research only | named failing correlation/large-kernel case and independent A/B | base algorithm solves case or added rays/state exceed budget |
| shadow maps/VSM/ReSTIR-ranked maps | deferred product alternative | non-RT hardware/content requirement and full cache/invalidation budget | ray profile is sufficient or new subsystem violates ownership budget |
| portable reconstruction | required product layer | frozen signal/guide/history contract and raw-versus-filtered motion suite | hides bias, cannot meet portability, or duplicates an existing provider owner |
| DLSS RR | optional provider | same semantic inputs/status and measured supported-profile gain | sole correctness path, hidden fallback, or provider changes raw semantics |

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
[^14]: Epic Games, [MegaLights: Stochastic Direct Lighting](https://advances.realtimerendering.com/s2025/content/MegaLights_Stochastic_Direct_Lighting_2025.pdf), SIGGRAPH Advances in Real-Time Rendering, 2025.
[^15]: Hedstrom et al., [Stochastic Pairwise MIS](https://research.nvidia.com/labs/rtr/publication/hedstrom2026stochastic/hedstrom2026stochastic.pdf), I3D 2026.
[^16]: Sawhney et al., [Decorrelating ReSTIR Samplers via MCMC Mutations](https://research.nvidia.com/labs/prl/sawhney2024decorrelating/restirmcmc2024.pdf), ACM TOG, 2024.
[^17]: Heitz, [Sampling the GGX Distribution of Visible Normals](https://jcgt.org/published/0007/04/01/), JCGT 7(4), 2018, revised 2019.
[^18]: Heitz et al., [Multiple-Scattering Microfacet BSDFs with the Smith Model](https://eheitzresearch.wordpress.com/240-2/), ACM TOG 35(4), 2016.
[^19]: Schied et al., [Spatiotemporal Variance-Guided Filtering](https://research.nvidia.com/labs/rtr/publication/schied2017spatiotemporal/), HPG 2017.
[^20]: Zhang et al., [Many-Light Rendering Using ReSTIR-Sampled Shadow Maps](https://research.nvidia.com/labs/rtr/publication/zhang2025many-light/), Computer Graphics Forum 44, 2025.

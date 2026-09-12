# Indirect Lighting And ReSTIR Path-Resampling Study

**Status:** primary-source research and discovery input; no cited method is local implementation or acceptance evidence

**Responsibility:** record current-source findings, primary path-resampling/GI precedent, alternatives, failure lessons, permitted transfers, and forbidden inference for `IND-D0`

**Authority boundary:** this study informs [Discovery](Discovery.md); it cannot label the current prototype ReSTIR GI, select local product/math/architecture, authorize code, or pass `FCR-REN-07`

**Researched:** 2026-09-12; Sparkle audit at `8e4ffba225411965dc51c0b783e5f47a075c7e84`; repository sources revision-pinned where practical

## Research Decision

Replace Sparkle's seed-replay reservoir with an explicitly derived path-resampling implementation. Start with the narrower ReSTIR GI-style domain—secondary illumination from a real initial path sample and a proved reconnection/shift contract—then admit generalized ReSTIR PT only when glossy/multi-bounce requirements and budgets justify it. The 2026 ReSTIR PT Enhanced work is the forward architecture watchpoint, but it is not a reason to begin with a unified direct+global reservoir.

Probe grids, surface caches, radiance caches, and neural caches solve different product problems. They are comparisons and possible future tiers, not parallel implementations to build alongside the selected path-resampling route.

## Method And Claim Discipline

This study prioritizes papers, pinned official SDK code/documentation, and official engine technical documentation. Source-reported MSE improvements and timings are scoped to their test scenes/hardware/algorithms. Sparkle must reproduce value on its own Cornell Box, Sponza, Bistro, San Miguel, motion, and failure workloads with raw scene-linear artifacts.

## ReSTIR GI, GRIS, And ReSTIR PT

ReSTIR GI resamples multi-bounce indirect paths across pixels and frames. Its reported one-sample-per-pixel improvements demonstrate potential on the paper's scenes, not a universal error bound.[^1] The reusable lesson is that the resampled object is a path/sample with a mathematically defined contribution and mapping, not merely a seed rerun at another receiver.

GRIS extends RIS to correlated samples, varied domains, unknown PDFs, and explicit shift mappings. It also explains why iterative ReSTIR does not automatically retain ordinary RIS convergence guarantees: spatial/temporal reuse introduces correlation and mapping assumptions.[^2] A Sparkle implementation must therefore state:

- integration domain and path representation;
- target and contribution weight;
- source proposal and generalized MIS weight;
- partial-bijection shift, inverse, support, and Jacobian;
- effective sample accounting and correlation/bias mode;
- which vertices are stored, reconnected, moved, or replayed;
- visibility and material evaluation at source and destination.

RTXDI's current ReSTIR PT integration is useful executable precedent for a renderer-owned path tracer: it requires a path-tracer context, path/sample state, temporal and spatial passes, and renderer application bridges rather than hiding scene/material/traversal ownership.[^3] The original public ReSTIR PT code provides an inspectable reference for hybrid shift mappings and path-reservoir data, but must be license-reviewed and treated as a reference implementation rather than pasted into Sparkle.[^4]

ReSTIR PT Enhanced reports reciprocal neighbor selection, footprint-based reconnection criteria, duplication maps for correlation, and a unified direct/global reservoir with improved speed and robustness in its evaluation.[^5] Those ideas enter the watchlist after a basic implementation passes. Unifying direct and indirect state too early would violate Sparkle's current independent product/evidence boundaries and greatly enlarge the first clean break.

| Family | Best fit | Main complexity/failure | Sparkle disposition |
| --- | --- | --- | --- |
| ReSTIR GI (2021) | diffuse/rough indirect paths, simpler real-time domain | limited shift validity on glossy/specular paths; temporal visibility | first target subject to `IND-D0` |
| GRIS/ReSTIR PT (2022) | multi-bounce diffuse and specular path reuse | contribution weights, shift maps, correlation, storage/retrace cost | target expansion after one-bounce foundation |
| ReSTIR PT Enhanced (2026) | robust/optimized broader path tracing | new algorithm, integration breadth, unified-reservoir migration | watchpoint and later optimization study |
| ReSTIR PG (2025) | better initial candidates from prior resampled paths | feedback/history/state and guiding update cost | conditional future increment[^6] |
| reservoir splatting/multilayer reuse | disocclusion and subpixel motion | extra layers/splat conflicts/MIS/memory | conditional motion increment[^7] |
| ReSTIR BDPT | hard caustics/light paths | bidirectional technique space, light vertices, ~non-real-time paper cost | excluded from first product; research only[^8] |

## Path Representation And Shift Findings

Reconnection reuses later path vertices while connecting a new primary/secondary receiver to a stored vertex. It is effective for rough transport but fails when the connection violates BSDF support, visibility, geometric orientation, or invertibility. Random replay can reproduce path decisions with less storage, but replay at another receiver is not a shift by itself: probability, domain, changed geometry, and inverse/Jacobian still matter.[^2]

Hybrid mappings select reconnection where roughness/footprint supports it and replay through specular chains. Every selection becomes part of the estimator. The recommended first path record is explicit enough to audit one secondary vertex and its light/BSDF sample; optimization may compact or replay suffixes only after round-trip tests.

| Record fact | Why it is needed |
| --- | --- |
| source primary surface and generation | defines original receiver/domain and motion |
| secondary vertex identity/position/normals/material | permits reconnection, visibility, and support evaluation |
| sampled directions/lobes and random dimensions | reconstructs proposal and delta/rough classification |
| terminal light/environment/emission identity | prevents stale or double-counted terminal contribution |
| forward/reverse proposal factors and technique identity | evaluates shift/MIS/contribution weight |
| throughput/contribution or reproducible factors | allows target at source/destination and finite validation |
| path depth/termination | preserves domain and roulette accounting |

The current Sparkle three-integer seed payload lacks these facts. It may regenerate a path, but the audit found no source-domain record or shift/PDF/Jacobian proof that makes neighbor replay a valid path proposal.

## Initial Sampling And Transport Findings

Initial candidate quality bounds what resampling can recover. ReSTIR PG explicitly targets poor/correlated candidates by deriving guiding distributions from previously accepted paths.[^6] That is later work; first delivery needs transparent next-event and BSDF proposals:

```text
camera primary surface
  -> sample BSDF to secondary surface
  -> evaluate direct illumination there with NEE/light selection
  -> handle emissive/environment terminal events with exact MIS policy
  -> continue admitted path depth with compensated roulette
```

The current `PathLighting.hlsli` appears to accumulate incident lighting at successive hits while tracing a BSDF path. Discovery must prove that direct lighting, emission, and sky are counted exactly once per admitted path technique. Familiar path-tracing structure does not establish correct throughput, PDFs, MIS, or lobe classification.

PBRT provides independent reference derivations for path sampling, multiple importance sampling, Russian roulette, environment emission, and robust rays.[^9] Sparkle's accepted Reference Path Tracer contract is the local intended oracle owner, but its output becomes evidence only after its own gates close.

## Alternative Real-Time GI Architectures

| Architecture | Strength | Known tradeoff | Decision |
| --- | --- | --- | --- |
| baked lightmaps | stable, cheap runtime diffuse | static content/lighting, bake pipeline, storage | excluded from this dynamic path-resampling feature |
| irradiance/DDGI probes | scalable diffuse world-space cache | placement, leaks, low-frequency/glossy limits, update latency | compare as future lower-tier/fallback; not parallel first delivery[^10] |
| screen-space GI | inexpensive reuse of visible data | off-screen loss, view dependence, disocclusion | optional candidate/accelerator only |
| signed-distance/voxel cone tracing | broad approximate world-space coverage | representation/update/leaks/detail loss | not selected |
| Lumen-style surface cache + traces | practical hybrid dynamic GI across hardware tiers | card/coverage/view-distance/update lag and a large scene-cache subsystem | product precedent only[^11] |
| sparse radiance cache (SHaRC) | reusable world-space radiance for path tracing | cache allocation/update/bias/leak and integration state | future measured supplement[^12] |
| neural radiance cache | learned online multi-bounce approximation | training stability, hardware/dependency, bias and determinism | research tier only[^13] |
| ReSTIR GI/PT | high-fidelity screen-space path reuse with RT geometry | sparse-ray noise, correlation, disocclusion, path storage/retrace | selected high-end target |

The persona favors understandable mechanisms and evidence. A second GI system should be added only for a real platform/product tier that ReSTIR cannot serve, not as speculative redundancy.

## Reconstruction Findings

Sparse indirect radiance needs a denoiser, but denoiser requirements constrain signal design. NRD RELAX/REBLUR expect stable depth, motion, normals, roughness, radiance/hit distance, pre-exposure, and history behavior.[^14] DLSS Ray Reconstruction is an optional adjacent provider. The mandatory path must remain vendor-neutral and preserve raw lobe/path evidence.

Correlation often produces blotchy low-frequency error that ordinary denoisers handle poorly. Conditional resampling and newer ReSTIR work explicitly address correlation/denoiser interaction.[^15] Sparkle should measure color noise, duplicated path identity, effective sample diversity, and disocclusion noise rather than adding more temporal frames blindly.

## Environment, Sky, And Emission

The image-based sky has three separable roles:

1. visible background emission on a camera miss;
2. an infinite light sampled by direct/secondary NEE;
3. terminal emission when a transported path misses.

They use one environment mapping, rotation, radiance generation, and importance PDF, but distinct path-technique accounting. A future physical atmosphere may publish the environment radiance generation; atmospheric transmittance/aerial perspective remains volumetric. Emissive surfaces likewise separate “visible/emissive hit” from “enumerated light sampled by NEE.” MIS or explicit technique exclusion prevents double counting.

## Current Sparkle Gap Analysis

| Current fact | Consequence to investigate |
| --- | --- |
| sample stores pixel/sample/frame seed in `float4` | insufficient documented path state and exact-integer risk after `2^24` |
| current-surface replay of source seed | no proved shift mapping, inverse, Jacobian, or source-domain probability |
| scalar luminance target/weight/`M` only | no visible generalized contribution-weight or technique metadata |
| normal/view-distance compatibility only | material/object/roughness/path/light/sky mismatches can survive |
| fixed four spatial neighbors and shared caps | correlation/disocclusion policy is unexplained |
| inline-only path resolve | no independent Pipeline/front-end parity path; product policy unresolved |
| max bounce eight | validation limit, not proof that all depths are correct/useful |
| sky available to misses; emissive joined in composite | potential technique/accounting ambiguity requires raw path tests |
| no dedicated portable indirect denoiser | product-quality path incomplete |

## Recommended Architecture To Test

1. Establish one-bounce diffuse/glossy initial estimator with explicit NEE/BSDF/emission/environment technique accounting.
2. Store an auditable compact path record; do not optimize to seed replay until mapping/probability round trips pass.
3. Implement reconnection for its valid rough domain and an explicit rejection path outside support.
4. Apply GRIS temporal then spatial reuse with integer identity, bounded effective sample count, and declared correlation/bias mode.
5. Reconstruct raw diffuse/specular through a portable baseline; retain optional RR.
6. Extend to multi-bounce/hybrid ReSTIR PT only after stage-level evidence and budget.
7. Evaluate Enhanced reciprocal neighbors/footprint criteria/duplication maps, ReSTIR PG, or reservoir splatting as measured optimizations, not foundational requirements.

## Rejected Shortcuts

- Rename the current shader to ReSTIR GI without equation/conformance evidence.
- Keep seed replay as a compatibility path after the replacement.
- Compare only against the in-engine reference when it shares material/light/path code under test.
- Judge convergence through tone-mapped screenshots or one temporal endpoint.
- Add a Lumen-like surface cache, probes, SHaRC, and neural cache together “for completeness.”
- Increase bounce count, temporal cap, or denoiser strength before proving one-bounce accounting.
- Claim caustics, transmission, perfect mirrors, atmosphere, or volumes from generic multi-bounce traversal.

## Discovery Handoff

`IND-D0` must freeze the supported path domain, initial techniques, path record, shift mappings, GRIS contribution weight/bias mode, visibility/animation policy, environment/emission accounting, history identity, reconstruction, oracles, metrics, and budgets. The current source route remains a prototype until those decisions and checks pass.

## Sources

[^1]: Ouyang et al., [ReSTIR GI: Path Resampling for Real-Time Path Tracing](https://research.nvidia.com/publication/2021-06_restir-gi-path-resampling-real-time-path-tracing), Computer Graphics Forum, 2021.
[^2]: Lin et al., [Generalized Resampled Importance Sampling: Foundations of ReSTIR](https://research.nvidia.com/labs/rtr/publication/lin2022generalized/), ACM TOG 41(4), 2022.
[^3]: NVIDIA, [RTXDI ReSTIR PT integration](https://github.com/NVIDIA-RTX/RTXDI/blob/a6efab966b7c3b272da0461578eb56ac61c7cbff/Doc/RestirPT.md), revision `a6efab9`, accessed 2026-09-12.
[^4]: Lin et al., [ReSTIR PT reference implementation](https://github.com/DQLin/ReSTIR_PT/tree/8d12332228eb64bc234e27c6f7e0913a926285ab), revision `8d12332`, accessed 2026-09-12.
[^5]: Lin, Kettunen, and Wyman, [ReSTIR PT Enhanced](https://research.nvidia.com/labs/rtr/publication/lin2026restirptenhanced/), I3D 2026.
[^6]: Zeng et al., [ReSTIR PG: Path Guiding with Spatiotemporally Resampled Paths](https://research.nvidia.com/labs/rtr/publication/zeng2025restirpg/), SIGGRAPH Asia 2025.
[^7]: Liu et al., [Reservoir Splatting for Temporal Path Resampling and Motion Blur](https://research.nvidia.com/labs/rtr/publication/liu2025splatting/), SIGGRAPH 2025; Hong et al., [Multi-Layer Reservoir Splatting for Temporal Reuse under Disocclusion](https://graphics.cs.utah.edu/research/projects/multi-layer-restir/), SIGGRAPH 2026.
[^8]: Hedstrom et al., [ReSTIR BDPT: Bidirectional ReSTIR Path Tracing with Caustics](https://research.nvidia.com/labs/rtr/publication/hedstrom2025restir/), ACM TOG, 2025.
[^9]: Pharr, Jakob, and Humphreys, [PBRT 4e: A Better Path Tracer](https://pbr-book.org/4ed/Light_Transport_I_Surface_Reflection/A_Better_Path_Tracer), 2023.
[^10]: Majercik et al., [Dynamic Diffuse Global Illumination with Ray-Traced Irradiance Fields](https://jcgt.org/published/0008/02/01/), JCGT 8(2), 2019; Majercik et al., [Scaling Probe-Based Real-Time Dynamic Global Illumination for Production](https://jcgt.org/published/0010/02/01/), JCGT 10(2), 2021.
[^11]: Epic Games, [Lumen Technical Details](https://dev.epicgames.com/documentation/unreal-engine/lumen-technical-details-in-unreal-engine), accessed 2026-09-12.
[^12]: NVIDIA, [Spatially Hashed Radiance Cache integration guide](https://github.com/NVIDIA-RTX/SHARC/blob/4e21b585c33c83d723ca9a1e11bbb1090d145793/docs/Integration.md), revision `4e21b58`, accessed 2026-09-12.
[^13]: Müller et al., [Real-Time Neural Radiance Caching for Path Tracing](https://research.nvidia.com/labs/rtr/publication/muller2021nrc/), ACM TOG, 2021.
[^14]: NVIDIA, [NVIDIA Real-time Denoisers](https://github.com/NVIDIA-RTX/NRD/tree/bf877181058988ec5785f82c4189be0be75c1902), revision `bf87718`, accessed 2026-09-12.
[^15]: Kettunen et al., [Conditional Resampled Importance Sampling and ReSTIR](https://research.nvidia.com/labs/rtr/publication/kettunen2023conditional/), ACM TOG, 2023.

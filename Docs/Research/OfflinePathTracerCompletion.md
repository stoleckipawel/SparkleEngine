# Offline Path Tracer Completion Study

Status: source-backed research and initial discovery report; not an architecture decision, implementation plan, feature-completion claim, or runtime proof

Responsibility: compare primary NVIDIA path-tracing precedents with SparkleEngine's current reference-lighting route and identify the decisions and evidence required before planning an offline unbiased path tracer

Authority boundary: the [roadmap](../Strategy/Roadmap.md#offline-reference-truth-first) owns priority, the [discovery acceptance contract](../Acceptance/Renderer/OfflinePathTracerDiscovery.md) owns `PTD-00` pass/fail, current code owns implemented behavior, and a later plan may own delivery only after `PTD-00` passes

Research snapshot: NVIDIA sources and SparkleEngine source were inspected on 2026-09-06; local source baseline is committed `master` revision `8414b5dc` with concurrent documentation relocation in the worktree

Non-claims: no shader was compiled, no renderer was launched, no image was captured, no estimator was numerically tested, and no D3D12/Vulkan parity, convergence, unbiasedness, performance, or feature-completion result was produced by this study

## Research Decision

SparkleEngine should make the offline path tracer the first **technical feature-closure case**, but should not yet call the current `ReferencePathTraced` mode an unbiased renderer or correctness oracle.

The first action is `PTD-00`: freeze what “offline,” “unbiased,” “reference,” “converged,” and “feature complete” mean; audit every dependency; derive the estimator; define falsifying fixtures and statistics; and decide the smallest trustworthy target. Only an accepted `PTD-00` report authorizes creation of `PTD-01`, the implementation plan. Implementation remains the first feature slice of `REL-04` and does not bypass release identity, reproducible-build, or package prerequisites.

This ordering is deliberate. A wrong oracle can make every later PBR, lighting, ReSTIR, map, and denoising comparison look authoritative while preserving the same error.

## Completion Vocabulary

| Term | Required meaning in SparkleEngine | What is insufficient |
| --- | --- | --- |
| Offline | A bounded, reproducible render job over a frozen scene/camera/configuration snapshot that may take longer than frame time and produces retained linear-HDR evidence. | A slow interactive CVar mode tied to frame scheduling. |
| Reference | A comparison route with an explicit dependency ledger and known authority limits. Shared dependencies are named, and each shared leaf has another oracle. | A mode named `ReferencePathTraced`. |
| Unbiased | For the declared transport domain, the estimator derivation shows that its expected value equals the declared integral. Every selection probability, PDF/Jacobian, MIS weight, Russian-roulette probability, and delta event is accounted for. | Noise, visual plausibility, Russian roulette alone, or agreement at one sample count. |
| Consistent | The estimate approaches the declared target as independent sample count increases under stated assumptions. | Two nearby high-sample images or a decreasing average error in one crop. |
| Converged | A predeclared statistical stop rule passes for every required region/quantity, with uncertainty and independent replicate evidence. | “Looks clean,” a fixed arbitrary SPP, or a denoised image. |
| Feature complete | Every included camera, geometry, material, texture, light, environment, alpha/sidedness, transform, backend, output, failure, and operational row has a verdict and evidence; excluded rows are unreachable and not advertised. | Supporting diffuse and GGX on a hero scene. |
| Ground truth | A result whose scope and uncertainty are sufficient for the specific comparison and whose shared dependencies cannot mask the defect being tested. | Any one renderer used as universal truth. |

Empirical tests can falsify a derivation or implementation, but cannot prove mathematical unbiasedness by themselves. Sparkle therefore needs both a reviewed derivation and defect-detecting executable evidence. If a deterministic bounce cap, contribution clamp, firefly filter, denoiser, biased adaptive stop, or approximate cache changes the expected value, the output is labeled with that bias and is not the raw unbiased oracle.

## Primary NVIDIA Source Study

The sources below are precedent, not Sparkle authority. Links are pinned to the revisions inspected where source identity matters.

| NVIDIA source | Inspected implementation surface | Relevant finding | Sparkle consequence |
| --- | --- | --- | --- |
| [Falcor Path Tracer guide](https://github.com/NVIDIAGameWorks/Falcor/blob/eb540f6748774680ce0039aaf3ac9279266ec521/docs/usage/path-tracer.md) and [`PathTracer`](https://github.com/NVIDIAGameWorks/Falcor/tree/eb540f6748774680ce0039aaf3ac9279266ec521/Source/RenderPasses/PathTracer) | path generation, surface-specific bounce limits, BSDF sampling, analytic/environment/emissive light sampling, NEE, MIS, nested dielectrics, outputs | Falcor explicitly calls the pass unbiased, records the transport strategies, and exposes ray/path counters and guide outputs. It still takes a primary V-buffer, so even a mature reference tracer has a named primary-visibility dependency. | Define unbiasedness against a frozen transport domain and publish the dependency boundary; do not infer whole-renderer independence from estimator quality. |
| Falcor [`MinimalPathTracer`](https://github.com/NVIDIAGameWorks/Falcor/blob/eb540f6748774680ce0039aaf3ac9279266ec521/Source/RenderPasses/MinimalPathTracer/MinimalPathTracer.rt.slang) | deliberately naive path tracer with fixed path length, direct sampling, emissive and environment hits | NVIDIA keeps a simpler, slower implementation because reviewability and a differently shaped oracle are valuable. It documents the fixed-length boundary and omits nested dielectric support. | Keep a minimal reviewable oracle or analytic layer separate from the optimized implementation. Complexity in MIS or material breadth must not be the only route to truth. |
| Falcor [`AccumulatePass`](https://github.com/NVIDIAGameWorks/Falcor/tree/eb540f6748774680ce0039aaf3ac9279266ec521/Source/RenderPasses/AccumulatePass) and [`ErrorMeasurePass`](https://github.com/NVIDIAGameWorks/Falcor/tree/eb540f6748774680ce0039aaf3ac9279266ec521/Source/RenderPasses/ErrorMeasurePass) | single, compensated-single, and double-precision accumulation; reset/overflow behavior; source/reference/difference and L1/L2 measurement | Accumulation precision, reset identity, and error measurement are first-class components rather than incidental temporal history. | Give raw accumulation, sample identity, numerical error, comparison, and export explicit contracts. |
| Falcor [path-tracer image tests](https://github.com/NVIDIAGameWorks/Falcor/tree/eb540f6748774680ce0039aaf3ac9279266ec521/tests/image_tests/renderpasses) | default/minimal, adaptive sampling, standard materials, material types, nested dielectrics, light leaks, alpha test, scene reload | A mature tracer is checked by targeted semantic and lifecycle workloads, not one beauty image. | Build a small analytic/metamorphic fixture ladder, then material/light/lifecycle and representative-map evidence. |
| [NVIDIA RTX Path Tracing](https://github.com/NVIDIA-RTX/RTXPT/tree/f08d1c739071e0faad0c7c274d861124c511abab) | pure path tracer without rasterization; reference and real-time modes; analytic/emissive/environment lighting; NEE; low-discrepancy sampling; ray cones; Russian roulette; volumes/nested dielectrics | RTXPT separates the pure path from rasterization and makes reference versus real-time behavior explicit. Its source also exposes optional reference firefly filtering and screenshot denoising. A “reference” preset is therefore not automatically an unbiased raw estimator. | Prefer camera-ray independence for the Sparkle oracle, record every biasing switch, and retain raw unclamped/undenoised output separately from presentation output. |
| NVIDIA [self-intersection analysis](https://developer.nvidia.com/blog/solving-self-intersection-artifacts-in-directx-raytracing/) and [sample implementation](https://github.com/NVIDIA/self-intersection-avoidance) | conservative error bounds for reconstructed/transformed triangles and ray origins; connection-ray endpoint handling | A fixed normal offset is a scene-scale heuristic. Too little causes self-hits; too much skips nearby geometry and leaks light. | Replace “tune `NormalBias` until the image looks good” with scale/transform/grazing fixtures and a justified robust spawn-point contract. |
| [OptiX Applications](https://github.com/NVIDIA/OptiX_Apps) | advanced path tracers, NEE, environment importance sampling, emissive mesh lights, material systems, multi-GPU accumulation | Production breadth is modular and configuration-dependent; path length and direct-light strategies are user-visible contracts. | Inventory feature and configuration semantics explicitly; do not import OptiX/RTXPT breadth into the first Sparkle target without a release need. |

### Precedent To Adopt

- distinguish a minimal reviewable oracle from an optimized feature-rich tracer;
- freeze a transport/material/light domain instead of using “PBR” as an unlimited promise;
- generate stable per-pixel, per-sample, per-dimension identities independent of wall-clock frame scheduling;
- treat NEE, BSDF sampling, emissive/environment hits, MIS, delta events, and Russian roulette as one derivable estimator;
- make raw linear radiance, sample count, variance/error, ray count, path length, rejection reason, and invalid-value counts inspectable;
- make accumulation precision, reset, maximum count, checkpoint/resume, and overflow behavior explicit;
- test materials, light leaks, alpha, scene reload, determinism, and failure behavior independently;
- separate unbiased raw evidence from denoised, filtered, tone-mapped, or otherwise presentation-oriented output.

### Precedent Not To Copy By Default

- RTXDI/ReSTIR, denoisers, neural reconstruction, stable planes, caches, SER, opacity micromaps, volumes, spectral transport, multi-GPU distribution, or a general material framework;
- a vendor-specific traversal architecture when one Sparkle semantic contract can serve D3D12 and Vulkan;
- a firefly clamp or biased adaptive termination in the raw oracle;
- a large framework merely because an external reference has one.

Those capabilities enter only if the frozen first-release transport domain or a measured correctness blocker requires them.

## Current Sparkle Source Trace

All rows are source-inspected (`S`) and unexecuted for this report.

| Stage | Current implementation | Consequence for discovery |
| --- | --- | --- |
| Selection | [`Lighting.cpp`](../../Engine/Renderer/Private/Passes/Lighting/Lighting.cpp) selects `LightingMode::ReferencePathTraced`, allocates RGBA32F lighting lobes, runs reference producers, common composite/sky, then reference accumulation. | The route exists, but the shared presentation graph is not an offline job boundary. |
| Primary visibility and surface | [`GBufferPathSurface.hlsli`](../../Engine/Assets/Shaders/RayTracing/GBufferPathSurface.hlsli) reconstructs world position and loads base color, shading normal, roughness, metallic, and dielectric F0 from the existing GBuffer. | It cannot independently detect primary raster/ray-GBuffer visibility, interpolation, encoding, precision, alpha, material, or normal defects. |
| Direct lighting | [`PathTracedDirectLighting.hlsl`](../../Engine/Assets/Shaders/Passes/RayTracing/PathTracedDirectLighting.hlsl) loops over every directional, point, spot, and rect light for every sample and uses shared area-light, BRDF, and shadow visibility code. | PDF/unit/visibility behavior needs derivation and fixtures. This path shares bugs with production lighting. |
| Indirect path | [`PathTracedIndirectLighting.hlsl`](../../Engine/Assets/Shaders/Passes/RayTracing/PathTracedIndirectLighting.hlsl) launches BSDF paths from the GBuffer surface, divides results into first diffuse/specular lobes, and writes valid samples. | There is no independent camera segment. Classification by first lobe is an output convention that must not alter energy. |
| Sampling and termination | [`PathSampling.hlsli`](../../Engine/Assets/Shaders/RayTracing/PathSampling.hlsli) chooses diffuse or stochastic GGX reflection, accounts for selected-lobe probability in throughput, and applies compensated Russian roulette after bounce two. Random values use interleaved-gradient noise salted by `FrameIndex`, pixel, bounce, and per-frame sample index. | The math still needs review and executable checks. Frame-index coupling is not yet a stable offline sample identity, and no MIS is present. |
| Hit lighting | [`PathLighting.hlsli`](../../Engine/Assets/Shaders/RayTracing/PathLighting.hlsli) adds emissive plus sampled direct analytic-light radiance at a hit and sky radiance at a miss for each bounce. | Emissive triangles and the environment are reached through BSDF paths only; analytic lights use NEE. Completeness, double-counting exclusions, and variance implications need proof. |
| Surface domain | [`PathSurface.hlsli`](../../Engine/Assets/Shaders/RayTracing/PathSurface.hlsli) carries position, shading normal, view direction, base color, roughness, metallic, and dielectric F0. [`RayTracingMaterialHit.hlsli`](../../Engine/Assets/Shaders/RayTracing/RayTracingMaterialHit.hlsli) can reconstruct emissive, alpha, subsurface, normal-map, skinning, morph, and transform data, but the subsequent path surface does not retain the whole hit domain. | The supported transport/material envelope is narrower than all reconstructed material data and has no transmission, IOR/interior stack, volume, or physical subsurface path. Scope must be explicit. |
| Ray robustness | [`PathTrace.hlsli`](../../Engine/Assets/Shaders/RayTracing/PathTrace.hlsli) uses a configurable shading-normal offset, grazing multiplier, fixed `MinT=0.001`, maximum distance, back-face culling, and alpha-tested inline ray query. | Fixed heuristics can create self-hit or leak bias across scale, transforms, thin gaps, and shading/geometric-normal disagreement. |
| Accumulation | [`ReferenceLightingAccumulation.hlsl`](../../Engine/Assets/Shaders/Passes/RayTracing/ReferenceLightingAccumulation.hlsl) stores running-average radiance plus sample count in RGBA32F and reuses history only when global history is valid and per-pixel motion is near zero. [`ReferenceLightingInvalidation.cpp`](../../Engine/Renderer/Private/Passes/Lighting/Reference/ReferenceLightingInvalidation.cpp) hashes lighting scene state, path settings, view mode, and camera matrices. | It is frame history, not yet a frozen offline accumulation record. Precision drift, counter range, all mutation invalidations, resize/restart/checkpoint behavior, and deterministic resume need proof. |
| Limits | [`PathTracedLightingSettings.cpp`](../../Engine/Renderer/Private/RayTracing/Effects/PathTracedLighting/PathTracedLightingSettings.cpp) clamps 1-4096 samples per frame and 1-16 bounces; defaults are 64 SPP, 8 bounces, `NormalBias=0.01`, and `MaxDistance=100000`. | A deterministic path cap and distance cutoff define a truncated target unless the accepted claim says otherwise. They cannot be hidden behind the word “unbiased.” |
| Backend/front end | The reference shaders are compute passes containing inline ray queries. The current [graphics matrix](../Architecture/CrossModule/GraphicsCoverageMatrix.md#renderer-feature-and-backend-matrix) records no native RT-pipeline adapter and no non-ray fallback. | Discovery must decide whether one inline semantic route on both APIs is the intended offline product or whether another traversal route is required. Traversal choice is separate from estimator correctness. |
| Output | Common composite, sky, exposure, upscale, debug, tone-map, encode, and present continue after reference accumulation; the workload calls for high-sample comparisons. | The raw linear estimator needs its own export/provenance boundary so post-processing cannot contaminate oracle comparisons. |

## Initial Missing-And-Unknown Ledger

“Known gap” means the required completion surface is absent from the inspected route. “Unknown” means source presence is insufficient and discovery must produce math or executable evidence. Neither status is a defect verdict by itself.

| ID | Status | Question or missing evidence | Required resolution before planning |
| --- | --- | --- | --- |
| `PTD-Q-01` | Decision required | Is the target unbiased for the full supported surface-transport integral, or unbiased only for an explicitly finite path-length domain? | Freeze the equation, measures, path domain, units, exclusions, and label. A silent hard cap is forbidden for a full-transport claim. |
| `PTD-Q-02` | Known gap | The current route begins from GBuffer data rather than an independently generated camera ray/primary hit. | Select camera-ray independence or downgrade the route to a lighting-only reference; map every shared dependency to another oracle. |
| `PTD-Q-03` | Unknown | Diffuse/GGX lobe selection, PDFs, delta limits, analytic area-light PDFs, NEE composition, and Russian roulette have not been derived end to end. | Produce a notation-consistent estimator derivation and adversarial numeric cases. |
| `PTD-Q-04` | Known gap | No MIS joins BSDF sampling with emissive/environment light sampling, and no importance sampler exists for emissive triangles or the environment. | Decide whether slow BSDF-only sampling remains unbiased and usable for the first domain or whether NEE/MIS is required for bounded convergence. |
| `PTD-Q-05` | Decision required | The first-release material/light domain is not frozen. | Classify opaque, alpha-tested, two-sided, normal-mapped, emissive, metallic-roughness, subsurface, blend/transmission, volumes, animation, sky, and every light kind as included or excluded. |
| `PTD-Q-06` | Known gap | The scattering state cannot represent transmission, media/interior state, or physical subsurface transport. | Exclude these from the oracle claim and release maps or add them to the later accepted plan; do not use appearance-only handling as transport proof. |
| `PTD-Q-07` | Unknown | Shading-normal use, back-face culling, alpha evaluation, normal maps, deformed geometry, and current/previous transforms have no path-reference evidence. | Define geometric-versus-shading-normal rules and focused fixtures for sidedness, alpha, motion snapshot, and deformation. |
| `PTD-Q-08` | Known gap | Fixed normal bias, `MinT`, and max distance are scene-scale heuristics. | Select a robust spawn-point and endpoint contract, then design scale/translation/shear/grazing/thin-gap tests. |
| `PTD-Q-09` | Known gap | Sample identity depends on render `FrameIndex`; there is no offline job/sample manifest or deterministic checkpoint/resume contract. | Define seed, sample ordinal, dimension allocation, stream version, job ID, interruption, resume, and duplicate/skip detection. |
| `PTD-Q-10` | Unknown | RGBA32F running-average precision, sample-count representation, invalid samples, overflow, reset, and history invalidation are unproven. | Choose summation/precision policy and prove static accumulation, mutation reset, resize, restart, checkpoint, and terminal count behavior. |
| `PTD-Q-11` | Known gap | No raw HDR export with machine-readable scene/camera/settings/shader/asset/backend/sample provenance was found on the reference route. | Define immutable output, metadata, hashes, AOV/diagnostic files, atomic completion marker, and partial-job handling. |
| `PTD-Q-12` | Known gap | No estimator diagnostics expose variance, standard error, ray/path counts, path length distribution, rejection reasons, NaN/Inf/negative/overflow counts, or cap hits. | Freeze the minimal diagnostic payload needed to detect false convergence and numerical failure without becoming a general dashboard. |
| `PTD-Q-13` | Unknown | D3D12/Vulkan results, capability rejection, device loss, validation messages, and compiler optimization sensitivity are untested. | Define paired-backend numeric tolerances, strict capability behavior, native validation, and failure artifacts. |
| `PTD-Q-14` | Known gap | No analytic fixture set, independent minimal oracle, cross-renderer interchange manifest, or statistical convergence protocol exists. | Design an oracle ladder and predeclare tolerances, replicate counts, regions, stop rules, and escalation. |
| `PTD-Q-15` | Decision required | Offline execution has no frozen CLI/UI job lifecycle, time/VRAM budget, progress, cancellation, watchdog, or recovery contract. | Select the smallest consumer/developer workflow and bounded failure behavior. |
| `PTD-Q-16` | Unknown | The current mode has no runtime, image, performance, or package evidence at this snapshot. | Execute only the checks authorized by the accepted discovery record; source inspection cannot close the feature. |

## Recommended Target Shape To Test In Discovery

This is a hypothesis to evaluate, not accepted architecture.

```text
frozen scene + camera + transport-domain manifest
    -> deterministic camera/sample generator
    -> primary ray and hit reconstruction from canonical scene data
    -> reviewable path-integrator contract
         -> BSDF/emission/environment evaluation
         -> NEE/light sampling and MIS when selected
         -> compensated Russian roulette
         -> robust ray spawn and visibility endpoints
    -> raw high-precision accumulation + diagnostics
    -> atomic linear-HDR/AOV/provenance export
    -> analytic/minimal/external/statistical comparison
    -> separately labeled display transform or denoised preview
```

### Dependency Rule

Canonical cooked/source asset identity, immutable scene geometry/material buffers, texture storage, and low-level traversal may be shared when duplication would create a second scene authority. A shared component cannot validate itself. Every shared decode, transform, intersection, material, BRDF, light, or texture rule therefore needs an analytic, metamorphic, independent minimal, or cross-renderer oracle capable of exposing its failure.

The offline reference must not consume a production GBuffer, ReSTIR reservoir, denoised signal, temporal reconstruction history, exposure result, tone-mapped value, or encoded presentation surface as transport input. Those are comparison subjects, not oracle inputs.

### Proposed First-Release Transport Domain

Discovery should begin with the current release material envelope rather than external-engine breadth:

- perspective pinhole camera at frozen resolution; depth of field and motion blur excluded unless an accepted map requires them;
- triangle geometry with static, skinned, and morphed frozen snapshots only where the release advertises them;
- opaque and alpha-tested sidedness, UVs, base color, metallic, roughness, dielectric F0, normal maps, emissive, and the exact texture/color-space rules used by shipped maps;
- directional, point, spot, rect/area, emissive-triangle, and environment lighting only when their units and sampling contracts are frozen;
- surface reflection and emission in scene-linear radiometric units;
- no blended transparency, physical transmission, participating media, spectral transport, physical BSSRDF, caustic promise, or arbitrary procedural material unless explicitly admitted by scope freeze.

If an excluded feature remains visible in a release map or public selector, scope freeze must remove the feature/map from the promise or discovery must expand before implementation planning.

## Oracle Ladder

| Level | Purpose | Example evidence | Limitation it prevents |
| --- | --- | --- | --- |
| 0. Algebra and analytic fixtures | Validate PDFs, weights, units, transforms, simple visibility, and known integrals. | constant environment over Lambertian surface; black/white/emissive limits; inverse-square and area-light cases; probability normalization; energy bounds. | Two implementations sharing the same conceptual error. |
| 1. Minimal reviewable tracer | Exercise camera-to-light paths with fewer strategies and branches. | deterministic tiny scenes and per-path event logs compared with hand calculations. | Optimized integrator complexity masking a wrong event or weight. |
| 2. Sparkle production offline tracer | Produce the intended high-sample reference with diagnostics and provenance. | raw HDR, variance/error, path/ray histograms, invalid counters, manifest, and repeat records. | Beauty-image-only acceptance. |
| 3. Independent renderer | Cross-check interchange scenes after proving camera/material/light equivalence. | pinned Falcor Minimal/PathTracer or another accepted primary reference, with conversion manifest and source/output hashes. | Sparkle shared-code bugs and self-consistent wrong results. |
| 4. Candidate comparisons | Judge real-time Sparkle modes and release maps. | FLIP/difference/regions, convergence curves, temporal sequences, and reviewed failure crops. | Promoting the oracle before it has earned authority. |

No level replaces the others. Cross-renderer disagreement is `Inconclusive` until scene semantics are shown equivalent; agreement is supporting evidence, not a proof of unbiasedness.

## Discovery Work Sequence

| Slice | Work | Required output | Stop condition |
| --- | --- | --- | --- |
| `PTD-D0` semantics and scope | Freeze claimant, audience, transport equation/domain, feature matrix, exclusions, unbiasedness label, and oracle authority. | reviewed terminology, equation, support matrix, and dependency policy | Any ambiguous path-length, unit, material/light, camera, output, or shared-dependency claim. |
| `PTD-D1` current-route audit | Trace owner, producers/consumers, build membership, scene/view/resource lifetime, shaders, PDFs, RNG dimensions, numerical guards, accumulation, invalidation, output, and both backends. | line-linked dependency graph, derivation draft, gap/unknown ledger, and risk triggers | An untraced contribution, probability, clamp, cutoff, fallback, or mutation. |
| `PTD-D2` oracle and experiment design | Define analytic/metamorphic fixtures, minimal oracle, external interchange cases, sample statistics, fault injections, tolerances, artifacts, and resource bounds. | executable check specifications mapped to every criterion/failure mode | A check cannot detect its named defect or has no predeclared oracle. |
| `PTD-D3` target-shape decision | Compare camera-ray versus GBuffer-seeded routes, megakernel versus bounded wavefront execution, light-sampling/MIS options, accumulation precision, export/job workflow, and backend strategy. | decision record selecting the smallest adequate target and rejected alternatives | Selection rests on familiarity, external precedent alone, or unmeasured performance. |
| `PTD-D4` planning handoff | Split accepted work by owner/dependency, identify clean breaks and deletions, define evidence order and estimates, and reconcile downstream `FCR`/map dependencies. | `PTD-00` completion report and authorization decision for `PTD-01` | Any criterion, risk treatment, failure check, owner, dependency, or scope decision remains open. |

The discovery owner executes the cheapest claim-falsifying work first. It does not start with a full engine build, representative map render, or large external integration. Focused local probes may be temporary only under the repository's submitted-test policy.

## Downstream Dependencies

Until `FCR-REN-08` eventually passes its candidate acceptance:

- Sparkle reference images are labeled **candidate comparisons**, not ground truth;
- `FCR-REN-06` direct/composite lighting and `FCR-REN-07` ReSTIR correctness cannot use the current reference mode as their sole oracle;
- `MAP-A` through `MAP-H` may gather source/configuration evidence, but reference-dependent PBR/lighting verdicts remain blocked;
- denoiser or neural targets cannot be trained/evaluated against Sparkle output represented as unbiased truth;
- the 30 FPS real-time target remains independent of offline render speed, while the offline job still needs maximum duration, progress, cancellation, and resource bounds.

## Research Handoff

Current discovery decision: **BLOCKED**. This report establishes the initial precedent, source trace, target hypothesis, and question ledger, but it does not contain reviewed estimator mathematics, executable fixture results, accepted scope, or an independent reproduction.

Next permitted work is the evidence-gathering sequence in the [offline path tracer discovery acceptance contract](../Acceptance/Renderer/OfflinePathTracerDiscovery.md). Creating an implementation plan, estimating implementation slices, or renaming the current route “unbiased” before that gate passes is a process failure.

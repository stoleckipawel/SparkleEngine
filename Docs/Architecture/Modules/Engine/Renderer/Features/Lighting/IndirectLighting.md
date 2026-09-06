# Renderer Indirect Lighting

Status: current feature dossier; source-backed, not a convergence proof, unbiased-oracle claim, numerical validation, performance result, or release approval

Verified: 2026-09-06 against committed `master` revision `d236da11`; `Engine/Renderer` is unchanged from the earlier `8414b5dc` source audit

Scope: indirect-light portions of `REN-PBR-05`, `REN-LGT-01`, and `REN-LGT-03` through `REN-LGT-07`; distinguishes bounced surface transport, environment background, accumulation, and reconstruction inputs

## Feature Promise

Indirect lighting produces `IndirectDiffuse` and `IndirectSpecular` scene-linear radiance from secondary surface transport. Sparkle currently provides a real-time ReSTIR branch and an accumulating reference branch. Both are seeded by the primary GBuffer and use inline ray traversal for secondary paths.

Sky is documented here because it is the current environment/background boundary, but background sky fill is not proof of image-based diffuse/specular lighting, probes, atmospheric scattering, or volumetric transport.

## Current Producers

| Lighting mode | Algorithm | History | Current boundary |
| --- | --- | --- | --- |
| ReSTIR path-traced | Clear/seed working reservoirs, temporal reuse, spatial reuse, then inline-ray indirect resolve. Configured bounce count is clamped to 8. | Previous indirect reservoir plus surface/motion/history validity; optional reconstruction-guide products are emitted for DLSS RR. | Inline only; bias, correlation, disocclusion, multi-bounce stability, backend behavior, and cost remain unproved. |
| Reference path-traced | `PathTracedIndirectLighting` produces one inline secondary-path sample into the indirect lobes; direct and indirect lobes are composited with sky into an RGBA32F reference sample and accumulated. | Validity- and motion-aware reference history; configured samples per pixel clamp to 4096 and path bounces to 16. | Candidate comparison path, not an accepted unbiased or independent ground truth. |

ReSTIR indirect order:

```text
GBuffer + TLAS/material/sky + previous indirect reservoir
  -> clear/seed working reservoirs
  -> temporal reuse
  -> spatial reuse
  -> inline secondary-ray resolve
  -> IndirectDiffuse / IndirectSpecular
```

Reference order:

```text
GBuffer + TLAS/material/lights/sky
  -> direct sample + indirect sample
  -> five-lobe composite + emissive + sky into ReferenceLightingSample
  -> validity/motion-aware accumulation into SceneColor
```

## Surface And Environment Scope

- Indirect output is split into diffuse and specular products for composite, debug, and reconstruction.
- Split-sum-style BRDF/specular helpers and Jimenez multibounce Af exist where used; the current default has no specular-occlusion feature.
- Secondary rays share scene geometry, material lookup, alpha-mask decisions, and texture bindings with the rest of the ray-tracing system.
- Emissive is added by the common lighting composite. This does not establish emissive-light sampling or a general light-transport solution for emissive geometry.
- Sky fills background pixels and supplies environment data to relevant path shaders. No probe baking, irradiance volume, reflection-capture system, atmosphere model, or broad IBL pipeline is claimed.

## History And Invalidation

Temporal correctness depends on frame, scene, camera/view, motion, surface, settings, extent, shader, provider, and graph-topology identity as applicable. Scene reset, camera discontinuity, resize, mode/provider/shader changes, and affected topology changes invalidate relevant histories. The presence of hashes and reset signals is not evidence that every editor mutation is covered.

Reference accumulation shares the primary GBuffer, scene, materials, lights, ray traversal, and presentation with the real-time renderer. Its precision and accumulation make it useful for comparison, but do not make it independent. The [Offline Path Tracer Discovery](OfflinePathTracer/Discovery.md) owns the estimator/domain/oracle decision.

## Inputs, Outputs, And Ownership

- Inputs: primary GBuffer, scene depth, TLAS, hit geometry/materials/textures, sky, view/motion data, mode settings, and applicable histories.
- Outputs: `IndirectDiffuse`, `IndirectSpecular`, ReSTIR reservoir history, optional reconstruction guides, reference sample/history, and finally shared scene color through composite/accumulation.
- The indirect producers do not own direct visibility, final presentation, or volumetric media.

## Failure, Diagnostics, And Evidence

- Missing inline-ray capability means current indirect lighting cannot activate; no raster/probe fallback exists.
- Invalid mode or stale required bindings must fail or reject the graph rather than manufacture black as a successful result.
- Indirect lobe debug views are available, but current presentation may alter their values.
- Evidence: `REN-E09` for ReSTIR indirect, `REN-E10` plus `PTD-00` for the reference path, `REN-E18` for debug products, and `RHI-E04`/ray checks for required formats and traversal.

## Acceptance Criteria

- `AC-IND-01` — ReSTIR indirect produces finite, scene-linear `IndirectDiffuse` and `IndirectSpecular` for canonical one-bounce and multi-bounce fixtures with requested bounce count clamped/reported exactly as documented.
- `AC-IND-02` — temporal and spatial reservoir reuse rejects incompatible surface, motion, scene, view, settings, extent, shader, provider, and topology identity without stale contribution or cross-view history.
- `AC-IND-03` — motion, disocclusion, camera cut, resize, mode switch, material/light/sky mutation, scene reload, and shader/provider change reset only affected ReSTIR/reference histories and converge from a documented first-frame state.
- `AC-IND-04` — reference accumulation uses stable sample/count identity, finite RGBA32F state, deterministic reset, and monotonic sample accounting; current output is labeled candidate comparison rather than accepted independent oracle.
- `AC-IND-05` — emissive contribution and sky background/environment roles remain distinct: no test or UI claims emissive-light sampling, probes, IBL breadth, atmosphere, or volumetric transport from their presence.
- `AC-IND-06` — unavailable inline traversal, TLAS, hit/material/texture/sky binding, or required format rejects the lighting mode before dispatch rather than publishing black as a successful result.
- `AC-IND-07` — direct/indirect/composite and optional RR guide products retain one scene/view/frame identity, expected extent/format, and inspectable producer.
- `AC-IND-08` — D3D12 and Vulkan satisfy predeclared raw-lobe/history tolerances for applicable ReSTIR/reference cells with native validation; quality/convergence/performance remain separately reported.

## Controlled Failure Modes And Checks

| Failure ID | Injection and safe state | Detecting check |
| --- | --- | --- |
| `FM-IND-01` | remove inline-ray/TLAS/material/format prerequisite; mode activation fails before graph execution | `CHK-IND-01` |
| `FM-IND-02` | mutate one history identity dimension without reset; verifier detects stale reuse and candidate fails | `CHK-IND-02` |
| `FM-IND-03` | invalid bounce/sample/bias/distance value or NaN/Inf radiance; clamp/reject is reported and persistent history never becomes non-finite | `CHK-IND-01`, `CHK-IND-03` |
| `FM-IND-04` | cancel/reload/shutdown with accumulation in flight; partial work cannot appear complete and resources retire by queue completion | `CHK-IND-02`, `CHK-IND-03` |
| `FM-IND-05` | compare final tone-mapped previews or the shared reference route as independent truth; evidence review marks the result inconclusive | `CHK-IND-04` |

| Check | Exercise and oracle | Covers |
| --- | --- | --- |
| `CHK-IND-01` | canonical diffuse/specular/sky cases over bounce/bias/distance boundaries and removed capabilities; decode raw lobes/guides | `AC-IND-01`, `AC-IND-05`–`AC-IND-07`; `FM-IND-01`, `FM-IND-03` |
| `CHK-IND-02` | temporal mutation matrix over every named identity, dual viewports, interruption/reload, and first-frame restart; inspect reservoir/sample/reset state | `AC-IND-02`–`AC-IND-04`; `FM-IND-02`, `FM-IND-04` |
| `CHK-IND-03` | long accumulation/stress sequence with finite checks, sample accounting, cancellation, completion drain, and retained-generation bounds | `AC-IND-03`, `AC-IND-04`; `FM-IND-03`, `FM-IND-04` |
| `CHK-IND-04` | paired-backend raw linear comparison using predeclared analytic/metamorphic cases and explicitly dependency-aware reference comparisons | `AC-IND-04`, `AC-IND-08`; `FM-IND-05` |

This contract is **defined but unproved**. `REN-E09` can close the interactive ReSTIR cells; credibility of the reference branch remains bounded by `PTD-00`. Noise reduction, plausible images, or agreement with a path sharing the same defect cannot by itself pass the contract.

## Primary Source Routes

- [`RestirIndirectLighting.cpp`](../../../../../../../Engine/Renderer/Private/Passes/Lighting/Restir/RestirIndirectLighting.cpp), [`RestirIndirectTemporal.cpp`](../../../../../../../Engine/Renderer/Private/Passes/Lighting/Restir/RestirIndirectTemporal.cpp), [`RestirIndirectSpatial.cpp`](../../../../../../../Engine/Renderer/Private/Passes/Lighting/Restir/RestirIndirectSpatial.cpp), and [`RestirIndirectResolve.cpp`](../../../../../../../Engine/Renderer/Private/Passes/Lighting/Restir/RestirIndirectResolve.cpp)
- [`PathTracedIndirectLighting.cpp`](../../../../../../../Engine/Renderer/Private/Passes/Lighting/PathTracedIndirectLighting.cpp)
- [`ReferenceLighting.cpp`](../../../../../../../Engine/Renderer/Private/Passes/Lighting/Reference/ReferenceLighting.cpp) and [`ReferenceLightingAccumulation.cpp`](../../../../../../../Engine/Renderer/Private/Passes/Lighting/Reference/ReferenceLightingAccumulation.cpp)
- [`LightingComposite.cpp`](../../../../../../../Engine/Renderer/Private/Passes/Lighting/LightingComposite.cpp) and [`Sky.cpp`](../../../../../../../Engine/Renderer/Private/Passes/Lighting/Sky/Sky.cpp)
- [`Engine/Assets/Shaders/Lighting`](../../../../../../../Engine/Assets/Shaders/Lighting) and [`Engine/Assets/Shaders/RayTracing`](../../../../../../../Engine/Assets/Shaders/RayTracing)

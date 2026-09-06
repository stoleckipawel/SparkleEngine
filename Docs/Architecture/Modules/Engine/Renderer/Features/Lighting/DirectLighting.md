# Renderer Direct Lighting

Status: current feature dossier; source-backed, not a numerical oracle, native-parity result, visual validation, performance result, or release approval

Verified: 2026-09-06 against committed `master` revision `d236da11`; `Engine/Renderer` is unchanged from the earlier `8414b5dc` source audit

Scope: direct-light portions of `REN-PBR-01` through `REN-PBR-04`, `REN-PBR-06` through `REN-PBR-10`, `REN-LGT-01`, `REN-LGT-02`, `REN-LGT-04`, and `REN-LGT-06`

## Feature Promise

Direct lighting evaluates one selected analytic light against the primary GBuffer surface and produces three scene-linear products: `DirectDiffuse`, `DirectSpecular`, and `DirectSubsurface`. The current light inventory is directional, point, spot, and rectangular area lights. Visibility is ray traced; Sparkle has no shadow-map or non-ray direct-lighting fallback.

## Current Producers

| Lighting mode | Candidate and visibility path | Surface resolve | Current boundary |
| --- | --- | --- | --- |
| ReSTIR path-traced | Temporal and spatial direct-light reservoir reuse select a light sample; `DirectShadowSignal` resolves visibility through Inline or native Pipeline traversal. | `DirectLighting` evaluates the active BRDF terms into the three direct lobes. | Capability-gated; reuse bias, disocclusion, parity, and performance evidence are open. |
| Reference path-traced | `PathTracedDirectLighting` traces the direct sample inline using the shared surface/light/material contract. | Writes the same three semantic lobes at reference precision. | Candidate comparison path; shared dependencies prevent treating it as an independent oracle without `PTD-00`. |

ReSTIR direct order:

```text
GBuffer + current lights + previous direct reservoir
  -> temporal reservoir reuse
  -> spatial reservoir reuse
  -> selected-light shadow visibility
  -> direct BRDF resolve
  -> DirectDiffuse / DirectSpecular / DirectSubsurface
```

## Light Types And Limits

| Light | Evaluated data | Inspected hard capacity |
| --- | --- | ---: |
| Directional | direction, intensity, sampling identity, shadow eligibility | 2 |
| Point | position, range, intensity, sampling identity, shadow eligibility | 1024 |
| Spot | position, direction, range, cone, intensity, sampling identity, shadow eligibility | 1024 |
| Rect | position, orientation, dimensions, intensity, sampling identity, shadow eligibility | 1024 |

These are validation limits in the inspected payload builder, not recommended content budgets or measured performant counts. Overflow, exact-boundary rendering, degenerate light data, and truncation visibility remain `REN-E07` work.

## Active Surface Model

| Lobe | Current compiled default | Explicit non-claim |
| --- | --- | --- |
| Direct diffuse | Burley diffuse | Lambert, Oren-Nayar, and Chan implementations are dormant source vocabulary, not registered feature choices. |
| Direct specular | Cook-Torrance with GGX distribution, Smith GGX correlated geometry, and Schlick Fresnel | No clearcoat, sheen, anisotropy, transmission, or IOR-authored lobe is claimed. |
| Direct subsurface | Wrap-lighting approximation consuming GBuffer subsurface color and strength | This is a surface approximation, not subsurface volume transport or participating-media scattering. |

Alpha masking affects ray candidates, but general transparent/transmissive direct lighting is not implemented.

## Traversal And Selection

`r.RayTracing.Shadows.Execution` resolves independently from the GBuffer traversal selection:

- Inline runs the compute/ray-query adapter.
- Pipeline runs ray-generation, miss, closest-hit, and alpha-mask any-hit stages through the scene shader table.
- Automatic chooses a supported ready frontend according to the current resolver.
- Strict requests must fail visibly when unavailable rather than fabricate an unshadowed or alternate result.

The two traversal frontends are intended to produce the same `DirectShadowSignal`; semantic and backend parity remain `REN-E08`, `RHI-E09`, and `RHI-E10` obligations.

## Inputs, Outputs, And Ownership

- Inputs: primary GBuffer, scene depth, GPU-scene light records, TLAS/hit/material bindings, current/previous surface identity, motion, direct reservoir history, and selected execution plan.
- Outputs: direct reservoir history, visibility signal, `DirectDiffuse`, `DirectSpecular`, and `DirectSubsurface`.
- The GPU scene owns light/material records; the view and graph generation own selection and temporal identity; the frame graph owns transient/history resources and synchronization.
- Lighting composite consumes the three direct lobes; direct lighting does not own final color, exposure, tone mapping, or presentation.

## Failure, Diagnostics, And Evidence

- Invalid light indices, unavailable strict traversal, stale scene/SBT identity, and missing required bindings must not silently produce plausible light.
- Direct lobe debug modes expose each result, subject to the current debug-presentation limitation.
- Evidence: `REN-E06` for BRDF behavior, `REN-E07` for light kinds/capacities, `REN-E08` for ReSTIR direct and visibility, `REN-E10`/`PTD-00` for reference-direct credibility, and RHI ray-tracing parity items.

## Acceptance Criteria

- `AC-DIR-01` — directional, point, spot, and rect fixtures produce finite direct diffuse, specular, and subsurface lobes matching predeclared analytic/reference values for the active BRDF defaults.
- `AC-DIR-02` — exact documented light capacities render and the first excess light is rejected before GPU-scene publication with family/count/capacity diagnostics.
- `AC-DIR-03` — ReSTIR direct temporal/spatial reuse preserves stable light identity, rejects invalid history at disocclusion/cut/reset, and does not reuse stale reservoirs.
- `AC-DIR-04` — Inline and Pipeline direct-shadow frontends agree on miss, opaque occlusion, alpha-mask rejection, double-sided geometry, light distance, and normal-bias edge cases within a declared tolerance.
- `AC-DIR-05` — Automatic exposes its resolved traversal; strict Inline/Pipeline rejects when unavailable and never substitutes unshadowed output or another frontend.
- `AC-DIR-06` — all three direct lobes remain scene-linear and independent until the one lighting composite; debug/capture can identify each producer without treating presentation output as raw evidence.
- `AC-DIR-07` — ReSTIR and reference-direct modes consume the same scene/light/material identity and produce the same semantic lobe units while retaining explicit algorithm/precision differences.
- `AC-DIR-08` — representative D3D12 and Vulkan results satisfy the numerical/parity tolerance with native validation enabled; performance claims include light count, traversal, resolution, and diagnostic state.

## Controlled Failure Modes And Checks

| Failure ID | Injection and safe state | Detecting check |
| --- | --- | --- |
| `FM-DIR-01` | invalid/stale light index or capacity overflow; reject before reservoir/lighting dispatch | `CHK-DIR-01` |
| `FM-DIR-02` | strict traversal lacks capability, program, SBT, or material table; graph selection fails and names the requirement | `CHK-DIR-02` |
| `FM-DIR-03` | camera cut, disocclusion, light add/remove/reorder, scene reload, or shader/topology change; invalidate affected reservoir history | `CHK-DIR-03` |
| `FM-DIR-04` | NaN/Inf/degenerate light, extreme roughness/F0, zero range, or invalid cone/rect basis; reject or produce the documented finite boundary result | `CHK-DIR-01` |
| `FM-DIR-05` | alpha-mask/bias/distance edge differs by traversal/backend; parity check fails and holds the affected cell | `CHK-DIR-02`, `CHK-DIR-04` |

| Check | Exercise and oracle | Covers |
| --- | --- | --- |
| `CHK-DIR-01` | analytic single-light/material matrix, exact capacity boundaries, degenerate/extreme inputs, and decoded lobe capture | `AC-DIR-01`, `AC-DIR-02`, `AC-DIR-06`; `FM-DIR-01`, `FM-DIR-04` |
| `CHK-DIR-02` | Inline/Pipeline/Automatic visibility matrix over miss/opaque/alpha/double-sided/bias/distance with capability faults | `AC-DIR-04`, `AC-DIR-05`; `FM-DIR-02`, `FM-DIR-05` |
| `CHK-DIR-03` | temporal sequence covering motion, disocclusion, cut, light mutation/reorder, resize, reload, and topology change; inspect reservoir identity/reset | `AC-DIR-03`, `AC-DIR-07`; `FM-DIR-03` |
| `CHK-DIR-04` | paired-backend native-validation run at declared light/material/resolution cells; compare decoded lobes and record frame-time only under identical diagnostics | `AC-DIR-07`, `AC-DIR-08`; `FM-DIR-05` |

This contract is **defined but unproved**. Passing requires the candidate report to retain raw lobe and visibility evidence, requested/active traversal, backend/capability records, thresholds chosen before observation, and controlled failure results.

## Primary Source Routes

- [`RestirDirectLighting.cpp`](../../../../../../../Engine/Renderer/Private/Passes/Lighting/Restir/RestirDirectLighting.cpp)
- [`DirectLightReservoir.cpp`](../../../../../../../Engine/Renderer/Private/Passes/Lighting/Direct/DirectLightReservoir.cpp) and the temporal/spatial shaders under [`Passes/Lighting/Direct`](../../../../../../../Engine/Assets/Shaders/Passes/Lighting/Direct)
- [`DirectShadowSignal.cpp`](../../../../../../../Engine/Renderer/Private/Passes/Lighting/Shadows/DirectShadowSignal.cpp) and [`DirectShadowSignalCommon.hlsli`](../../../../../../../Engine/Assets/Shaders/Passes/Lighting/Shadows/DirectShadowSignalCommon.hlsli)
- [`DirectLighting.cpp`](../../../../../../../Engine/Renderer/Private/Passes/Lighting/Direct/DirectLighting.cpp) and [`DirectLighting.hlsl`](../../../../../../../Engine/Assets/Shaders/Passes/Lighting/Direct/DirectLighting.hlsl)
- [`PathTracedDirectLighting.cpp`](../../../../../../../Engine/Renderer/Private/Passes/Lighting/Direct/PathTracedDirectLighting.cpp)
- [`RenderGpuLightingPayloadBuilder.cpp`](../../../../../../../Engine/Renderer/Private/Scene/GpuScene/RenderGpuLightingPayloadBuilder.cpp)

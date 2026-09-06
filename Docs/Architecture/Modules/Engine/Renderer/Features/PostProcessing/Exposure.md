# Renderer Exposure

Status: current feature dossier; source-backed, not colorimetric, temporal-response, performance, or release evidence

Verified: 2026-09-06 against source revision `d236da11`; `Engine/Renderer` is unchanged from the earlier `8414b5dc` source audit

Scope: `REN-POST-01` through `REN-POST-03`; manual and automatic exposure, metering, adaptation, history, per-viewport overrides, and asynchronous scheduling

Parent family: [Post Processing](README.md)

## Feature Promise

Sparkle produces one bounded 1x1 exposure multiplier from resolved per-view settings and the scene-linear lighting result. Exposure is measured before debug visualization can replace scene color, so diagnostic selection does not itself drive eye adaptation.

| Feature | Selector/current choices | Current behavior |
| --- | --- | --- |
| Manual exposure | `r.Exposure.Mode=Manual`, `r.Exposure.Manual`, compensation, min/max | produces a bounded linear multiplier |
| Automatic exposure | `r.Exposure.Mode=Automatic` | meters scene luminance toward target 0.18 by default and applies compensation/bounds |
| Metering | `r.Exposure.MeteringMethod` | parallel reduction or downsample pyramid |
| Adaptation | speed-up 3 EV/s, speed-down 1 EV/s defaults | history-aware asymmetric approach to target |
| Scheduling | graph queue preference/capability | may execute on asynchronous compute; useful overlap is unmeasured |

Default exposure is Automatic with ParallelReduction. Default multiplier bounds are `0.000001` and `65536`. Per-viewport overrides can replace mode, method, manual value, compensation, target, bounds, and speeds in resolved display settings.

## Ownership And Frame Placement

- `RenderView` owns resolved display intent for the frame; global settings are not a second runtime authority.
- The exposure pass reads the original scene-linear lighting result after lighting and before reconstruction/upscaling/debug/presentation consumers.
- Current/prior 1x1 textures own adaptation history. Scene/view/topology discontinuities invalidate affected history.
- Exposure is an input to image providers and tone mapping; it does not own either feature.

## Failure, Tradeoffs, And Evidence

- Invalid bounds, extreme luminance, camera cuts, resize, and mode/viewport changes need finite and reset evidence.
- Async scheduling centralizes dependency/barrier ownership in the frame graph, but source presence is not proof of overlap or speedup.
- Automatic metering improves adaptation but adds temporal behavior that can flicker or lag; manual mode is deterministic but requires authored intent.
- `REN-E13` owns controlled luminance steps, camera cuts, resize, viewport overrides, bounds, and adaptation. `REN-E17` separately owns how the resulting value participates in tone mapping.
- Stable proof obligations are defined below; their execution remains pending.

## Acceptance Criteria

- `AC-EXP-01` — Manual mode resolves the requested multiplier plus compensation into the documented min/max range and remains invariant for fixed settings across frames and scheduling modes.
- `AC-EXP-02` — ParallelReduction and DownsamplePyramid automatic metering produce finite values within predeclared tolerance for uniform, split, black, bright, NaN/Inf-contaminated, and high-dynamic-range fixtures.
- `AC-EXP-03` — adaptation follows the declared asymmetric EV-per-second rates under controlled luminance steps and converges monotonically without overshoot outside tolerance.
- `AC-EXP-04` — per-viewport overrides resolve once into `RenderView`; two views with different settings do not share or contaminate exposure history.
- `AC-EXP-05` — camera cut, scene/view discontinuity, resize, mode/metering change, and relevant topology generation reset history to the documented first-frame result.
- `AC-EXP-06` — graphics-queue and async-compute execution produce the same exposure/history values and correct dependencies; queue assignment is not called a speedup without measurement.
- `AC-EXP-07` — the exposure producer reads pre-debug scene-linear lighting and supplies exactly one multiplier to provider/tone-mapping consumers; debug-view selection alone does not remeter or reset it.

## Controlled Failure Modes And Checks

| Failure ID | Injection or cause | Required safe behavior | Detecting check |
| --- | --- | --- | --- |
| `FM-EXP-01` | min greater than max, non-finite setting, zero/negative target, or extreme luminance | resolve rejects or clamps by documented policy and never publishes non-finite history | `CHK-EXP-01` |
| `FM-EXP-02` | camera cut/resize/mode/view identity changes without reset | history oracle detects stale adaptation and candidate fails | `CHK-EXP-02` |
| `FM-EXP-03` | async queue unavailable or cross-queue dependency omitted | graph selects supported queue or rejects; no stale/uninitialized exposure reaches consumers | `CHK-EXP-03` |
| `FM-EXP-04` | one viewport changes exposure while another renders concurrently | resolved state/history remain isolated by viewport identity | `CHK-EXP-02` |

| Check | Exercise and oracle | Covers |
| --- | --- | --- |
| `CHK-EXP-01` | CPU/reference evaluation plus 1x1 readback over manual/automatic methods, luminance ramps, invalid/extreme inputs, bounds, compensation, and timestep series | `AC-EXP-01`–`AC-EXP-03`; `FM-EXP-01` |
| `CHK-EXP-02` | dual-viewport temporal sequence over overrides, cuts, resize, mode/metering switches, debug-mode changes, and scene reload | `AC-EXP-04`, `AC-EXP-05`, `AC-EXP-07`; `FM-EXP-02`, `FM-EXP-04` |
| `CHK-EXP-03` | same fixture on graphics and async-compute scheduling with capability unavailable/available, plan/barrier inspection, decoded value comparison, and native validation | `AC-EXP-06`; `FM-EXP-03` |

This contract is **defined but unproved**. `REN-E13` owns candidate execution; tolerances, timestep, luminance domain, and first-frame reset value must be declared before results are viewed.

## Primary Source Routes

- [`Exposure.cpp`](../../../../../../../Engine/Renderer/Private/Passes/PostProcessing/Exposure.cpp)
- [`ViewportDisplayCVars.cpp`](../../../../../../../Engine/Renderer/Private/View/ViewportDisplayCVars.cpp)
- [`RenderViewBuilder.cpp`](../../../../../../../Engine/Renderer/Private/View/RenderViewBuilder.cpp)

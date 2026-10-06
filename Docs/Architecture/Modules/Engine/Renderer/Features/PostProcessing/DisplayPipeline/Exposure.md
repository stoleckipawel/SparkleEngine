# Renderer Exposure

**Status:** current feature dossier and acceptance contract; candidate results remain separate

**Verified:** 2026-10-07 against the exposure repair worktree based on `b4f189a9`; executable evidence is candidate-bound

**Scope:** `REN-POST-01` through `REN-POST-03`; manual and automatic exposure, metering, adaptation, history, per-viewport overrides, and asynchronous scheduling

**Parent family:** [Post Processing](../README.md)

**Current readiness:** the portfolio row remains **45/100**; this repair does not promote whole-family, colorimetric, performance, or release acceptance. See [Current Feature Readiness](../../../../../../../Acceptance/CurrentReadiness.md#renderer) and the [`FCR-REN-09` result route](../../../../../../../Acceptance/FeatureCompletionReports.md#initial-completion-report-registry).

## At A Glance

| Mode or boundary | Current behavior | Main limitation |
| --- | --- | --- |
| Manual | fixed requested multiplier plus compensation and bounds | requires authored intent; no colorimetric acceptance |
| Automatic / histogram | meters the percentile-trimmed arithmetic mean of luminance bins | fixed log range and quantization; percentiles express a local quality policy |
| Automatic / downsample pyramid | meters the full-frame geometric mean | dark/black regions can drive a high exposure multiplier |
| Adaptation | asymmetric exponential response in log2 multiplier space | rates are inverse seconds; cuts and discontinuities require valid history reset |
| Async scheduling | may run on a capable non-graphics queue | queue assignment does not prove overlap or benefit |

Exposure is measured from pre-debug scene-linear color and resolved per view. Its single multiplier then feeds reconstruction providers and tone mapping, so stale or cross-viewport history can affect several downstream stages even when their own code is correct.

## Feature Promise

Sparkle produces one bounded 1x1 exposure multiplier from resolved per-view settings and the scene-linear lighting result. Exposure is measured before debug visualization can replace scene color, so diagnostic selection does not itself drive eye adaptation.

| Feature | Selector/current choices | Current behavior |
| --- | --- | --- |
| Manual exposure | `r.Exposure.Mode=Manual`, `r.Exposure.Manual`, compensation, min/max | produces a bounded linear multiplier |
| Automatic exposure | `r.Exposure.Mode=Automatic` | meters scene luminance toward target 0.18 by default and applies compensation/bounds |
| Metering | `r.Exposure.MeteringMethod` | `0=Histogram`, `1=DownsamplePyramid`; deliberately different statistics |
| Adaptation | speed-up `3/s`, speed-down `1/s` defaults | history-aware exponential approach to target |
| Scheduling | graph queue preference/capability | may execute on asynchronous compute; useful overlap is unmeasured |

Default exposure is Automatic with Histogram. Default multiplier bounds are `0.000001` and `65536`. Per-viewport overrides can replace mode, method, manual value, compensation, target, bounds, and rates in resolved display settings. [Exposure Semantics](ExposureSemantics.md) owns equations, finite-value policy, histogram layout/percentiles, history payload, and the end-to-end color-domain ledger.

## Ownership And Frame Placement

- `RenderView` owns resolved display intent for the frame; global settings are not a second runtime authority.
- The exposure pass reads the original scene-linear lighting result after lighting and before reconstruction/upscaling/debug/presentation consumers.
- Current/prior 1x1 textures own adaptation history. Scene/view/topology discontinuities invalidate affected history.
- Exposure is an input to image providers and tone mapping; it does not own either feature.

## Failure, Tradeoffs, And Evidence

- Non-finite settings use engine defaults; finite settings and final adaptation are bounded by the semantic contract. Invalid HDR samples cannot poison metering/history. Camera cuts, resize, and mode/viewport changes still require candidate-bound reset evidence.
- Async scheduling centralizes dependency/barrier ownership in the frame graph, but source presence is not proof of overlap or speedup.
- Automatic metering improves adaptation but adds temporal behavior that can flicker or lag; manual mode is deterministic but requires authored intent.
- `REN-E13` owns controlled luminance steps, camera cuts, resize, viewport overrides, bounds, and adaptation. `REN-E17` separately owns how the resulting value participates in tone mapping.
- The implementation-ledger and bounded checks for `ITER-EXPOSURE-01` are retained locally under `build/exposure-investigation.txt`; results do not replace the broader proof obligations below.

## Acceptance Criteria

- `AC-EXP-01` — Manual mode resolves the requested multiplier plus compensation into the documented min/max range and remains invariant for fixed settings across frames and scheduling modes.
- `AC-EXP-02` — Histogram agrees with an independent CPU bin/CDF reference, and DownsamplePyramid agrees with the finite full-frame geometric mean, within predeclared tolerance for uniform, split, black, bright, NaN/Inf-contaminated, odd-size, and high-dynamic-range fixtures. Their outputs need not match each other.
- `AC-EXP-03` — adaptation follows the declared asymmetric exponential rates in inverse seconds under controlled luminance steps and converges monotonically without overshoot outside tolerance, including multipliers below `0.0001`.
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

`REN-E13` owns candidate execution; tolerances, timestep, luminance domain, and first-frame reset value must be declared before results are viewed. Bounded repair checks do not prove concurrent dual-view isolation, every discontinuity, queue overlap, display calibration, or the full release workload.

## Primary Source Routes

- [Exposure Semantics](ExposureSemantics.md), including NVIDIA/AMD precedent and its limits
- [`ExposureAdaptation.cpp`](../../../../../../../../Engine/Renderer/Private/Passes/PostProcessing/Exposure/ExposureAdaptation.cpp) and [`ViewportDisplaySettings.cpp`](../../../../../../../../Engine/Renderer/Private/View/ViewportDisplaySettings.cpp)
- [`ViewportDisplayCVars.cpp`](../../../../../../../../Engine/Renderer/Private/View/ViewportDisplayCVars.cpp)
- [`RenderViewBuilder.cpp`](../../../../../../../../Engine/Renderer/Private/View/RenderViewBuilder.cpp)


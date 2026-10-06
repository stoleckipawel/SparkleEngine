# Exposure Semantics

**Status:** implemented contract; source and numerical evidence have separate owners

**Verified:** 2026-10-07 against the exposure repair worktree based on `b4f189a9`; inspect the candidate report for executable evidence

**Owner:** [Exposure](Exposure.md); these equations and color-domain rules are authoritative for that feature

## One Scene-Linear Multiplier

Lighting produces scene-linear RGB. Exposure meters that signal before debug visualization, ray reconstruction, or upscaling. It does not multiply the lighting buffers or modify authored lights/materials. Rec.709 luminance uses positive RGB and weights `(0.2126, 0.7152, 0.0722)`.

The producer publishes a 1x1 RGBA32F texture: `(current multiplier, metered luminance, target multiplier, exposure mode)`. Providers and tone mapping consume `.r`; the fourth component tags the history's mode. Both current output and history receive the same payload. There is no pass-specific uniform-buffer payload or CPU exposure-history mirror.

## Settings And Bounds

Renderer resolves global defaults and enabled viewport overrides into the current `RenderView`. Non-finite numeric values use the corresponding engine default before clamping. Manual exposure is non-negative, compensation is limited to `[-16, 16]` stops, target luminance is at least `0.0001`, and adaptation rates are non-negative. Multiplier bounds satisfy `0 <= minimum <= maximum <= 65536`; the upper limit keeps finite RGBA16F scene color within the supported tone curves' arithmetic range.

Let `C = 2^compensation`, `K = target luminance`, `L = metered luminance`, and `[Emin, Emax]` be the resolved multiplier bounds:

```text
manual target    = clamp(manual multiplier * C, Emin, Emax)
automatic target = clamp(K / max(L, 0.0001) * C, Emin, Emax)
```

Manual exposure is a linear multiplier, not an EV100 photographic-camera setting. Compensation is a stop offset for either mode. A zero manual multiplier is valid when the minimum is zero. The luminance floor has luminance units and must never become an exposure floor.

## Metering Methods

`r.Exposure.MeteringMethod=0` selects Histogram, the default. One full-resolution scan builds 512 integer bins: black occupies bin zero; 511 positive bins span log2 luminance `[-16, 16]`. Finite positive samples outside the range use its endpoint bins. Positive bin luminance is evaluated at the midpoint. Invalid RGB pixels are excluded entirely.

Resolve retains the middle 10th through 90th percentiles of valid sample counts. Boundary bins contribute only their fractional overlap with that count interval. The result is the arithmetic mean of retained bin luminances, floored at `0.0001`. Black pixels participate in the distribution; isolated dark regions and extreme highlights cannot dominate the whole-frame statistic. The histogram range covers the current finite RGBA16F lighting target. For uniform luminance at or above the metering floor within that range, midpoint quantization error is at most approximately 2.2%; numerical acceptance declares a 2.3% bound.

`r.Exposure.MeteringMethod=1` selects DownsamplePyramid. Its 2x2 stages sum `(log(max(luminance, 0.0001)), valid sample count)` through ceil-divided extents to 1x1. Out-of-bounds and invalid RGB samples contribute neither log luminance nor count. Resolve computes `exp(sumLog / count)`: the full-frame geometric mean. Dark valid pixels contribute the luminance floor, so a substantial black region can increase exposure significantly. These methods deliberately measure different statistics; equivalence is not a correctness requirement.

An entirely invalid automatic frame retains a valid, bounded prior multiplier. Without valid prior history it uses bounded neutral exposure `1`. A valid all-black image still meters at the luminance floor and resolves the bounded automatic target. This protects exposure state; it does not repair an invalid lighting producer.

## Temporal Response And History

Manual mode, missing/invalid history, a mode-tag mismatch, or a non-positive/non-finite prior multiplier resolves directly to target. Automatic mode with valid positive history adapts in log2 multiplier space:

```text
rate  = target > previous ? speedUp : speedDown
alpha = 1 - exp(-max(unscaledDeltaSeconds, 0) * rate)
E     = 2^(log2(previous) + (log2(max(target, 1e-20)) - log2(previous)) * alpha)
E     = clamp(E, Emin, Emax)
```

Rates have units of inverse seconds. They are exponential response coefficients, not a fixed EV-per-second velocity. Defaults are `3/s` up and `1/s` down; stored or overridden values remain authoritative. Zero delta or zero rate preserves the prior value within the current bounds. Increasing the linear multiplier means brightening the displayed image. The tiny positive target in the logarithm is numerical protection, not a published minimum.

The existing frame-graph history ring owns GPU lifetime and preceding-frame ordering. View identity, scene/shader/provider generation, graph topology, camera cuts/teleports/projection discontinuities, and resize invalidate history through their current view/graph owners. Changing metering method rebuilds graph settings. Mode changes are rejected by the GPU payload tag without adding generic view state. Ordinary manual values, compensation, targets, bounds, and rates are fresh per-frame parameters.

## Consumers And Transfer Functions

| Boundary | Required domain and behavior | Current source owner |
| --- | --- | --- |
| Material input | Cooked color textures use sRGB views for hardware decoding; scalar/normal data remain linear. Authored factors are unchanged. | Texture cook format policy, Renderer texture factory, material shaders |
| Lighting | Sky, direct/indirect components, and emissive accumulate scene-linear radiance; sky-disabled pixels preserve the initial black clear. | RealTimePathTracer passes and LightingComposite |
| Debug selection | Meter original lighting first; diagnostic replacement then follows its own presentation policy. | SceneRenderingPasses and PresentationPolicy |
| DLSS/DLAA | HDR input, tagged engine 1x1 exposure, provider auto exposure disabled, pre-exposure/exposure-scale defaults `1`; output remains in the original HDR domain. | StreamlineDlssOptions, resource tags, evaluation |
| Ray reconstruction | Same engine exposure texture and HDR convention; no second application of exposure in Renderer. | Streamline ray-reconstruction options/tags/evaluation |
| Tone mapping | Multiply resolved HDR by `.r` exactly once, then apply Reinhard, ACES approximation, or ACES filmic fit. Output is display-linear RGBA16F. | ToneMapping pass/shader |
| Encoding/presentation | Encode sRGB in the output shader for a linear UNorm swapchain, or leave linear for hardware sRGB conversion. Explicit output-encoding selection remains an override. The encoded intermediate is stored in a linear format. | OutputEncoding and presentation/UI backend |

The current pipeline does not pre-expose the lighting buffers. If that domain changes, metering must undo pre-exposure and every provider/tone-mapping consumer must be reconciled together. An exposure texture is a scale input for reconstruction, not proof that its output has already been exposed.

## Primary Precedent And Local Decisions

- [NVIDIA Donut exposure shader](https://github.com/NVIDIA-RTX/Donut/blob/main/shaders/passes/exposure_cs.hlsl) uses histogram percentile selection, a retained arithmetic luminance statistic, bounded adaptation, and a separate tone-mapping consumer. Sparkle's 10%-90% policy, midpoint bins, fractional boundary weights, and multiplier history are local decisions; they are not Donut's defaults or payload.
- [AMD FSR2 luminance pyramid](https://github.com/GPUOpen-Effects/FidelityFX-FSR2/blob/master/src/ffx-fsr2-api/shaders/ffx_fsr2_compute_luminance_pyramid.h) measures log luminance after undoing pre-exposure and uses a delta-time exponential response. Its photographic exposure conversion is not substituted for Sparkle's declared target-luminance multiplier.
- [NVIDIA Streamline DLSS guide](https://github.com/NVIDIA-RTX/Streamline/blob/main/docs/ProgrammingGuideDLSS.md) and [ray-reconstruction guide](https://github.com/NVIDIA-RTX/Streamline/blob/main/docs/ProgrammingGuideDLSS_RR.md) define the HDR/exposure tags and pre-exposure conventions. Check the pinned SDK in the build for the candidate's actual defaults and supported options.

## Source Routes

- [Exposure passes](../../../../../../../../Engine/Renderer/Private/Passes/PostProcessing/Exposure/ExposurePasses.cpp), [histogram declaration](../../../../../../../../Engine/Renderer/Private/Passes/PostProcessing/Exposure/ExposureMeteringPasses.cpp), and [adaptation parameters](../../../../../../../../Engine/Renderer/Private/Passes/PostProcessing/Exposure/ExposureAdaptation.cpp)
- [Histogram resolve](../../../../../../../../Engine/Assets/Shaders/Passes/PostProcessing/Exposure/ExposureHistogramResolve.hlsl), [shared exposure math](../../../../../../../../Engine/Assets/Shaders/Display/Exposure.hlsli), and [history resolve](../../../../../../../../Engine/Assets/Shaders/Passes/PostProcessing/Exposure/Exposure.hlsl)
- [View settings resolution](../../../../../../../../Engine/Renderer/Private/View/ViewportDisplaySettings.cpp) and [frame placement](../../../../../../../../Engine/Renderer/Private/Passes/Scene/SceneRenderingPasses.cpp)
- [Tone mapping](../../../../../../../../Engine/Assets/Shaders/Passes/Presentation/Display/ToneMapping.hlsl) and [output encoding](../../../../../../../../Engine/Renderer/Private/Passes/Presentation/Display/OutputEncoding.cpp)

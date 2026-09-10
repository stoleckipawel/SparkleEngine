# Chromatic Aberration Semantics

**Status:** proposed semantic contract; blocked until `CHRD-00` accepts the model and constants

**Responsibility:** define coordinate spaces, resolution-scaled displacement, channel sampling, filtering, edge behavior, alpha, identity, and independent reference evaluation

**Authority boundary:** [Discovery](Discovery.md) freezes choices; [Execution Architecture](ExecutionArchitecture.md) maps them to execution; this page does not prove a shader conforms

**Current readiness:** **0/100 — target only**.

## Claim Boundary

For one accepted finite settings value, active output extent/subrect, and target-linear input image, the intended claim is that Sparkle computes the frozen per-channel source coordinates and bilinear clamp samples within the declared tolerance, copies alpha from the unshifted pixel, applies the effect exactly once at the accepted stage, and omits all effect work at neutral strength.

This contract does not claim physical lens accuracy, calibrated wavelength dispersion, sensor response, spectral rendering, artifact removal, antialiasing, or correctness of tone/gamut/encoding. It defines a stylized renderer effect whose intentional output must remain distinguishable from incidental fringing.

## Notation And Direction Convention

`P = (px,py)` is an integer pixel coordinate inside the active viewport rectangle. `O = (ox,oy)` is that rectangle's origin in the backing resource and `E = (W,H)` its positive active extent. Pixel-center active-local normalized coordinates are:

```text
uv = ((P - O) + 0.5) / E
```

`center = (cx,cy)` is normalized in this active-local space, not window, monitor, DPI, or full backing-resource coordinates. Positive radial displacement samples farther from the center. RGB names mean storage/output components of the target-linear image; they do not mean spectral primaries or wavelengths unless `CHRD-05` selects the wavelength-inspired candidate explicitly.

CPU and shader manifests record rectangle origin/extent, resource extent, coordinate origin convention, center, settings semantic revision, and output frame/View identity. A fullscreen `uv` convention cannot silently replace active-subrect coordinates.

## Input And Output

The candidate consumes output-resolution target-linear RGB after the selected SDR or HDR tone/gamut mapping and before transfer encoding and UI. It preserves extent, viewport product identity, and alpha. `CHRD-02` must ratify the exact SDR/HDR domains before implementation.

## Coordinate Candidate

Let `uv` be the normalized coordinate of the output pixel center, `c` the normalized authored center, and `(W,H)` the active output extent. Form an aspect-aware pixel vector:

```text
p = (uv - c) * (W, H)
r = length(p)
d = (r > 0) ? p / r : (0, 0)
```

`CHRD-03` must freeze the radius normalization used by the authored start offset. The preferred candidate divides `r` by the distance from `c` to the farthest active-viewport corner along `d`, so the start control is stable for arbitrary centers and aspect ratios. A simpler half-height normalization may be accepted only with explicit corner/aspect behavior.

For normalized radius `rn` and start offset `a` in `[0,1)`, form:

```text
t = saturate((rn - a) / (1 - a))
falloff = t * t
```

The quadratic is a candidate, not an accepted fact. Discovery may choose linear or another fixed polynomial only with updated fixtures and controls.

The preferred farthest-corner normalization must be written without a direction-dependent divide at the center. One candidate computes the maximum distance from `center` to all four active-local corners in aspect-aware pixel space:

```text
cornerRadius = max(length((corner_i - center) * E) for i in four corners)
rn = (cornerRadius > 0) ? r / cornerRadius : 0
```

This makes the farthest corner `rn = 1`; nearer edges/corners start below one when the center is off-center. If discovery instead chooses the ray-to-rectangle boundary along `d`, the meaning differs and must get separate hand cases. The two candidates may not share the label “normalized radius.”

## Resolution-Scaled Strength

Authored strength `s1080` is the maximum channel displacement in pixels at an output height of 1080:

```text
s = s1080 * H / 1080
deltaPixels = d * (s * falloff)
deltaUv = deltaPixels / (W, H)
```

This preserves pixel-proportional authored intent across resolution. `H == 0`, non-finite values, invalid center/start, or strength outside the accepted range cannot execute the pass.

## Channel Candidate

The preferred first-release model uses three samples:

```text
R = sample(input, uv + redScale   * deltaUv).r
G = sample(input, uv + greenScale * deltaUv).g
B = sample(input, uv + blueScale  * deltaUv).b
RGBout = (R, G, B)
```

The candidate scales are `(+1, 0, -1)`. `CHRD-05` must freeze signs and weights after reviewed pattern images; a storage/channel permutation is a detectable defect. All samples use the accepted bilinear clamp-to-edge convention at texel centers.

Alpha is copied from the unshifted input pixel. It is never filtered from one of the displaced coordinates.

## Fixed-Model Alternatives

`CHRD-01/05` must select one exact fixed model before Stage 2:

| Model | Channel coordinate rule | Benefit | Cost/risk |
| --- | --- | --- | --- |
| `CHR-MODEL-A` symmetric | scales `(+1,0,-1)` times the same `deltaUv` | simple, transparent, centered green | arbitrary spacing/sign until product review; visible R/B symmetry can look synthetic |
| `CHR-MODEL-B` wavelength-inspired | derive three fixed magnification factors from frozen wavelength/dispersion constants, normalize their maximum displacement to authored strength | grounded in a concrete AMD precedent while retaining three reads | more constants/derivation and license/provenance review; still not physical calibration |
| `CHR-MODEL-C` multisample spectral | sample a radial segment and combine through a spectral curve/LUT | smoother/high-quality precedent | excluded from first release unless product scope, asset, quality tier, cost, and evidence are reopened |

For either admitted fixed model, Stage 0 writes the exact constant values, normalization, sign, and maximum-displacement proof. A shader literal, vendor default, or visual tweak after evidence capture cannot become the semantic authority.

## Bilinear Clamp Reference

The independent reference samples a source image of extent `(W,H)` at active-local `sampleUv`. Convert to texel space with the frozen center convention:

```text
texel = sampleUv * E - 0.5
i0 = floor(texel)
f = texel - i0
i1 = i0 + 1
j0 = clamp(i0, (0,0), E - 1)
j1 = clamp(i1, (0,0), E - 1)
sample = lerp(lerp(I[j0.x,j0.y], I[j1.x,j0.y], f.x),
              lerp(I[j0.x,j1.y], I[j1.x,j1.y], f.x), f.y)
```

Coordinates index the active product, with the resource/subrect translation specified by the graph binding. Clamp happens to integer taps; `f` remains the fraction from the unclamped texel coordinate so repeated edge texels produce the expected clamp-to-edge value. Hardware sampling is accepted only when it matches this reference within the frozen representation tolerance. Address-mode defaults are not semantic proof.

## Target Domain And Order

The same coordinate model can operate on SDR and HDR target-linear products only if both input contracts are explicitly named and filtering their values is semantically admitted. The feature does not convert primaries or transfer functions. Its graph edge is:

```text
scene grade -> target tone/gamut mapping -> chromatic aberration -> output transfer encoding -> UI -> publication
```

If the HDR architecture uses a different UI/encoding order, `CHRD-02/08` must reconcile that product route rather than assume this chain. Exact diagnostic products bypass the effect unless their owning Debug Views contract explicitly selects a display-intent path.

## Invalid And Exceptional Values

| Condition | Required Stage-0 disposition |
| --- | --- |
| non-finite strength/center/start or derived coordinate | reject requested state before graph publication; identify field/reason |
| negative strength | either reject or admit signed inward/outward displacement explicitly; hidden absolute/clamp is forbidden |
| center outside admitted normalized range | reject or clamp according to one documented rule; preserve authored versus effective value if sanitized |
| start `< 0` or `>= 1` | reject/clamp with exact boundary rule; avoid denominator instability |
| strength above maximum | reject/clamp before active publication; never rely on sampler bounds alone as capacity policy |
| `W == 0` or `H == 0` | no effect dispatch; follow normal minimized/zero-frame result without division |
| subrect outside backing resource | graph/frame admission failure; never manufacture coordinates |
| missing/stale input/product generation | graph failure or frame omission through existing owner; never sample unrelated memory |
| source RGB NaN/Inf | follow the accepted target-linear pipeline finite policy; effect does not privately sanitize only displaced taps |
| pipeline/binding unavailable | explicit requested-versus-active unavailable result; no mislabeled intentional fringe |

## Precision And Tolerance

The CPU oracle uses binary64. Stage 0 freezes parameter serialization precision, frame constant representation, shader arithmetic expectations, source texture format/filter precision, raw capture format, and per-case absolute/relative/ULP plus image thresholds. Analytic coordinate error and filtered-color error are recorded separately. A tolerance may cover representation/filtering differences but cannot hide swapped channels, wrong scale/aspect, wrong stage, wrap, alpha mutation, or a retained pass at zero.

## Identity And Omission

Strength zero is exact identity and omits the pass. A start offset at or beyond the active pixel's normalized radius yields zero displacement. Center pixels have zero displacement. The implementation may not run a nominally neutral pass and call approximate equality identity.

## Rules

| ID | Rule |
| --- | --- |
| `CHR-MATH-01` | active extent and viewport subrect define all coordinates; window/DPI size does not |
| `CHR-MATH-02` | strength scales from the 1080-line reference using active output height |
| `CHR-MATH-03` | center, aspect, radius, start offset, falloff, signs, and channel weights are one frozen contract |
| `CHR-MATH-04` | all texture coordinates are bounded by clamp-to-edge and use one texel-center convention |
| `CHR-MATH-05` | output alpha and product identity equal the unshifted input's |
| `CHR-MATH-06` | zero strength creates no pass, history, persistent resource, or changed pixel |
| `CHR-MATH-07` | non-finite or invalid controls never reach shader-active state |
| `CHR-MATH-08` | no temporal history or frame-dependent sample sequence participates |
| `CHR-MATH-09` | exact debug products and UI follow their owners' bypass/composition decisions |

## Independent Reference Procedure

A small double-precision CPU evaluator produces expected source coordinates and bilinear channel values from a manifest. It must not reuse shader helpers. Required inputs include impulse grids, vertical/horizontal/diagonal lines, RGB channel markers, alpha sentinels, checkerboards, gradients, edge/corner colors, odd extents, ultrawide/portrait aspect, off-center centers, every start boundary, zero/max strength, and matched 720p/1080p/1440p/4K cases.

Defect controls swap R/B, omit resolution scaling, use window instead of output extent, remove aspect correction, use wrap, sample alpha at a displaced coordinate, execute before tone mapping or after encoding, and retain the pass at zero. Each must cause a predeclared check failure.

## Retained Hand Cases

| ID | Setup | Required observation |
| --- | --- | --- |
| `CHR-HAND-01` | zero strength, asymmetric RGB/alpha image | exact product identity, no pass/resource, alpha unchanged |
| `CHR-HAND-02` | pixel at center and its four nearest pixel centers | zero/near-zero direction is finite and symmetric under the frozen convention |
| `CHR-HAND-03` | centered square extent; axes and diagonals at equal normalized radius | aspect/radius/falloff and channel signs match analytic coordinates |
| `CHR-HAND-04` | 16:9, ultrawide, portrait, odd extent, and nonzero subrect origin | active extent/subrect—not window/backing/DPI—drives coordinates |
| `CHR-HAND-05` | same normalized pattern at 720p/1080p/1440p/2160p | maximum displacement follows the exact reference-height scaling rule |
| `CHR-HAND-06` | start offset just below/at/above a selected radius | zero/start transition and denominator rule match the frozen polynomial |
| `CHR-HAND-07` | RGB-coded impulse/lines with channel-asymmetric model | R/G/B source positions and reconstruction are unmistakable |
| `CHR-HAND-08` | half-texel gradient sample | bilinear texel-center weights match the CPU reference |
| `CHR-HAND-09` | high strength at all edges/corners | repeated edge values show clamp; no wrap, non-finite, or out-of-bounds access |
| `CHR-HAND-10` | alpha checker with color displacement | alpha is copied from the unshifted output pixel exactly |
| `CHR-HAND-11` | invalid/non-finite/zero extent and missing pipeline | no active pass; requested/active reason is exact and recoverable |
| `CHR-HAND-12` | camera motion over static high-frequency fixture | no hidden history/frame sequence; identical frame inputs give identical output |

## Rule-To-Code And Evidence Ledger

Stage 0 replaces each role description with the exact accepted current owner/symbol before production.

| Rule | Intended owner | Independent check | Seeded defect |
| --- | --- | --- | --- |
| `CHR-MATH-01` | Stage-0-selected View/graph active-rectangle publication owner | subrect/aspect coordinate table | window/backing extent substitution |
| `CHR-MATH-02` | Stage-0-selected resolved shader-constant owner | matched-resolution displacement | omit/invert reference-height scaling |
| `CHR-MATH-03` | Stage-0-selected single semantic coordinate-helper owner | axes/diagonals/start hand cases | wrong radius/falloff/channel constant |
| `CHR-MATH-04` | Stage-0-selected fixed sampler/binding owner | edge and half-texel CPU/GPU cases | wrap or texel-edge shift |
| `CHR-MATH-05` | Stage-0-selected pass output-contract owner | alpha/product sentinel captures | displaced alpha or wrong stage/product |
| `CHR-MATH-06` | Stage-0-selected View/graph neutral-predicate owner | graph/resource trace | neutral pass/resource remains |
| `CHR-MATH-07` | Stage-0-selected settings/View validation owner | invalid-value state matrix | NaN/clamped secret default activates |
| `CHR-MATH-08` | Stage-0-selected stateless pass owner | repeated-frame/camera deterministic check | frame index/history leaks into output |
| `CHR-MATH-09` | Stage-0-selected Debug Views/UI/capture join owners | product topology and markers | exact debug/UI distorted or mislabeled |

## Common Mathematical Failure Points

- using full swapchain/window or DPI-scaled size instead of the View's active output rectangle;
- treating normalized UV radial distance as aspect-correct without a pixel/aspect convention;
- dividing by radius or `1-start` at a degenerate center/boundary;
- scaling normalized displacement twice, or not scaling reference-height strength at all;
- changing channel sign/order because of texture/storage/display-primary assumptions;
- using hardware sampler defaults that wrap or disagree with the CPU texel-center convention;
- filtering alpha at a displaced coordinate or transforming premultiplication semantics unintentionally;
- applying the effect to encoded values, before target tone/gamut mapping, or after UI;
- retaining a nominally neutral pass and accepting approximate image equality as identity;
- using a single natural image whose existing colored edges mask model defects.

## `CHRD-00` Ratification Checklist

- [ ] accepted model family, constants, normalization, signs, weights, and rights disposition;
- [ ] SDR/HDR target-linear domain identities and exact tone/gamut/encoding/debug/UI/capture edges;
- [ ] active rectangle, pixel-center, center, aspect, radius, start, falloff, and zero-radius equations;
- [ ] reference-height strength units, admitted range/sign, maximum displacement, and zero/minimized behavior;
- [ ] bilinear reference, sampler/address/texel-center rule, edge bound, alpha, and source finite policy;
- [ ] settings precision, shader precision, capture format, numeric/image tolerances, and seeded defects;
- [ ] exact neutral predicate and no-work/resource/history invariant;
- [ ] rule-to-owner/source symbols and invalidation triggers.

Unchecked items keep Stage 1/2 blocked. Production code may not choose them implicitly.

## Primary Sources And Precedent

AMD, Unity, and Unreal sources are pinned and bounded in the [Research source ledger](Research.md#source-ledger). They establish credible alternatives and workflow/stage precedent, not this semantic contract. The accepted Sparkle equations, constants, tolerances, and evidence remain local.

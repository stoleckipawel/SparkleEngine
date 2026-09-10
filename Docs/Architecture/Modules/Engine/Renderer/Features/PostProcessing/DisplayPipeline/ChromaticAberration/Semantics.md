# Chromatic Aberration Semantics

**Status:** proposed semantic contract; blocked until `CHRD-00` accepts the model and constants

**Responsibility:** define coordinate spaces, resolution-scaled displacement, channel sampling, filtering, edge behavior, alpha, identity, and independent reference evaluation

**Authority boundary:** [Discovery](Discovery.md) freezes choices; [Execution Architecture](ExecutionArchitecture.md) maps them to execution; this page does not prove a shader conforms

**Current readiness:** **0/100 — target only**.

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


# HDR Display Output Semantics

**Status:** proposed semantic contract; blocked until `HDRD-00` freezes scene, target, UI-white, gamut, tone, and output-route decisions

**Responsibility:** define luminance/color domains, Rec.2020/PQ signal encoding, target mapping, UI reference white, alpha, finite behavior, metadata separation, and independent reference procedure

**Authority boundary:** [Discovery](Discovery.md) accepts decisions; [Execution Architecture](ExecutionArchitecture.md) maps them to Renderer/RHI; platform APIs do not own these image semantics

**Current readiness:** **0/100 — target only**.

## Domain Chain

```text
scene-working linear radiance/color
  -> accepted exposure and color grade
  -> HDR target luminance and gamut map
  -> linear Rec.2020 RGB in absolute-display intent
  -> ST 2084/PQ encoded Rec.2020 RGB
  -> SDR UI mapped from system/accepted reference white, then encoded consistently
  -> compatible native presentation tuple
```

`HDRD-02/04/05/06` must freeze every scale and transform. Until then, the chain is a target decomposition, not an executable formula.

## ST 2084 Candidate

For absolute luminance `Y` in cd/m², normalize `L = clamp(Y / 10000, 0, 1)`. The BT.2100 PQ opto-electronic transfer candidate is:

```text
m1 = 2610 / 16384
m2 = 2523 / 32
c1 = 3424 / 4096
c2 = 2413 / 128
c3 = 2392 / 128

Lm = pow(L, m1)
PQ(Y) = pow((c1 + c2 * Lm) / (1 + c3 * Lm), m2)
```

The independent oracle uses higher precision. The shader must define behavior for negative/non-finite input before calling `pow`; clamping at the PQ boundary cannot be used to hide an upstream invalid-value defect.

The inverse used for encoded-ramp validation is:

```text
E = clamp(encoded, 0, 1)
Ep = pow(E, 1 / m2)
L = pow(max(Ep - c1, 0) / (c2 - c3 * Ep), 1 / m1)
Y = 10000 * L
```

## Primaries And Gamut

The output signal uses Rec.2020 primaries and D65 white. The scene-to-Rec.2020 matrix must be generated or independently checked from the accepted scene-working chromaticities; no Rec.709-to-Rec.2020 matrix is valid unless `HDRD-02` establishes Rec.709/D65 scene working space.

Out-of-gamut handling belongs to the HDR target transform. `HDRD-06` must choose and document clipping, hue-preserving compression, or another bounded method plus thresholds. PQ encoding per component is not gamut mapping.

## Target Luminance And Tone Mapping

The existing candidate uses a 1000-nit peak. Current platform guidance favors mapping into the current display's reported range. `HDRD-04` must choose:

- **Fixed mastering:** content is mapped to a fixed 1000-nit target and display adaptation is external;
- **Display-adaptive:** content mapping consumes a bounded queried display peak/black range;
- **Hybrid:** a fixed creative target is remapped within explicit bounds to current display capability.

The chosen rule must define diffuse white, peak, minimum/black handling, roll-off, average-light policy, invalid/missing display data, and whether a monitor move changes only output mapping or invalidates any View-owned state. No metadata field substitutes for the pixel transform.

## SDR UI White

Windows defines nominal scene-referred SDR white at 80 nits and exposes a current user/system SDR white level. For self-composited linear UI, the candidate scale is:

```text
uiScale = acceptedSdrWhiteNits / 80
```

`HDRD-05` decides whether `acceptedSdrWhiteNits` is the current system value, a user-controlled value initialized from the system, or a bounded fallback such as 200 when unavailable. The final UI samples must enter the same Rec.2020/PQ encoding exactly once. Alpha/blend semantics must be defined in the linear composition domain.

## Metadata Is Not Pixel Semantics

Static metadata records primaries/white and mastering/content luminance values when the accepted native route uses it. It does not select color space, encode PQ, tone map, prove monitor behavior, or guarantee transport to the display. Zero/unknown values and omission follow the accepted platform policy. Metadata values must agree with the actual target policy, but agreement is a consistency check rather than an active-HDR oracle.

## Semantic Rules

| ID | Rule |
| --- | --- |
| `HDR-MATH-01` | scene primaries, white, numeric units, exposure, and grade boundary are explicit before HDR mapping |
| `HDR-MATH-02` | target luminance/gamut mapping occurs once in Renderer and is distinct from PQ encoding |
| `HDR-MATH-03` | Rec.2020/D65 conversion is derived from accepted source chromaticities and independently checked |
| `HDR-MATH-04` | ST 2084 uses the exact accepted constants, 10000-nit normalization, finite policy, and precision |
| `HDR-MATH-05` | SDR UI white derives from the accepted current/fallback policy and 80-nit nominal reference |
| `HDR-MATH-06` | UI is composed in one linear target domain and PQ encoded once; alpha semantics remain explicit |
| `HDR-MATH-07` | metadata is consistent auxiliary state and never changes pixel interpretation |
| `HDR-MATH-08` | SDR fallback uses the existing SDR tone/encode contract, not PQ values copied into an SDR tuple |
| `HDR-MATH-09` | raw scene, target-linear, PQ-encoded, compositor capture, screenshot, and measured display artifacts remain distinctly labeled |
| `HDR-MATH-10` | no backend or profile changes semantic output without an explicit matrix cell and evidence rule |

## Independent Reference Procedure

A high-precision CPU tool or notebook, kept outside submitted production/test code unless separately authorized, consumes the accepted scene/target manifest and produces:

- PQ code values for black, sub-black policy, 0.0001, 0.01, 0.1, 1, 80, accepted SDR white, diffuse white, 100, 203, target peak, and 10000 nits;
- Rec.2020 primary/secondary/white matrices and round-trip error;
- scene ramps and saturated wedges through target/gamut mapping;
- SDR UI patches and alpha blends at current/fallback white levels;
- wrong constants, wrong 1000/10000 normalization, double PQ, omitted gamut transform, stale display peak, fixed-versus-system UI white, and metadata-only activation as defect controls.

Reference numbers, thresholds, output formats, capture points, and display-measurement interpretation are frozen before candidate execution. A shader screenshot is not the independent oracle.


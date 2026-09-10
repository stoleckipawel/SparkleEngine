# HDR Display Output Semantics

**Status:** proposed semantic contract; blocked until `HDRD-00` freezes scene, target, UI-white, gamut, tone, and output-route decisions

**Responsibility:** define luminance/color domains, Rec.2020/PQ signal encoding, target mapping, UI reference white, alpha, finite behavior, metadata separation, and independent reference procedure

**Authority boundary:** [Discovery](Discovery.md) accepts decisions; [Execution Architecture](ExecutionArchitecture.md) maps them to Renderer/RHI; platform APIs do not own these image semantics

**Current readiness:** **0/100 — target only**.

## Claim Boundary

For one accepted scene-working sample and output-policy/display-fact generation, Renderer should produce the independently specified target-linear and encoded signal values, UI composition, packing, and artifact identity; RHI should present that signal only through its matching active profile tuple. This page defines image semantics but does not prove native activation, OS composition, cable transport, panel luminance, calibration, or visual quality.

The HDR10 and any admitted scRGB profile are different claims. They may share upstream target mapping only where Stage 0 proves the domains agree; they never share an ambiguous `HDR encoded` label.

## Notation, Colorimetry, And Units

- RGB vectors are linear-light component tuples until an explicit transfer function is named.
- Chromaticities use CIE `x,y`; tristimulus vectors use `X,Y,Z`; matrices multiply column RGB/XYZ vectors.
- `Y_nits` means absolute luminance intent in cd/m². A unitless scene value has no absolute display meaning until the accepted exposure/reference-white scale is applied.
- `E_PQ` is the unitless ST 2084 encoded component in `[0,1]`; it is not linear relative light.
- `E_scRGB` is a signed/extended linear FP16 value under the exact Windows scRGB scale accepted by `HDRD-03`; it is not PQ or Rec.2020 code value.
- `SdrWhiteNits` is a queried or fallback UI/display policy value with validity and output generation, not a global constant.
- all semantic manifests name primaries, white, transfer, range, alpha, precision, profile, target policy, and display-fact generation.

The accepted scene contract must state primaries/white, linearity, relative or absolute scale, legal negative/HDR range, exposure/grade order, and reference diffuse white. The phrase `scene linear` is insufficient.

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

## Profile-Specific Output Chains

The discovery result must retain separate chains:

```text
HDR10 UINT10 profile:
  target-linear Rec.2020 nits -> component PQ -> optional deterministic dither
  -> packed UINT10 full-range RGB (+ frozen opaque/alpha bits)
  -> HDR10 ST2084 P2020 native tuple

FP16 scRGB profile, only if admitted:
  accepted target-linear intent -> explicit primaries/luminance-scale conversion
  -> signed/extended linear FP16 scRGB values (+ alpha/composition contract)
  -> scRGB/Advanced Color native tuple

SDR fallback:
  original accepted SDR tone/gamut -> existing SDR encoding
  -> known-good 8-bit SDR tuple
```

Renderer selects exactly one chain from the RHI active profile/result generation. PQ bytes in an SDR/scRGB tuple and scRGB linear values in an HDR10 tuple are semantic failures even if a display produces an image.

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

## Matrix Derivation Contract

For primaries `r=(xr,yr)`, `g=(xg,yg)`, `b=(xb,yb)` and white `(xw,yw)`, form unscaled XYZ columns with `Y=1`:

```text
X = x / y
Y = 1
Z = (1 - x - y) / y
P = [XYZr XYZg XYZb]
W = (xw/yw, 1, (1-xw-yw)/yw)
S = inverse(P) * W
M_RGB_to_XYZ = P * diagonal(S)
M_scene_to_2020 = inverse(M_2020_to_XYZ) * M_scene_to_XYZ
```

Stage 0 freezes chromaticity decimal/rational sources, adaptation rule if whites differ, matrix orientation/storage, calculation and shader precision, and tolerance. CPU tests include primaries, secondaries, white, gray, inverse/round-trip, negative values, and out-of-gamut wedges. Hard-coded matrices without derivation/provenance are not accepted.

## Target Luminance And Tone Mapping

The existing candidate uses a 1000-nit peak. Current platform guidance favors mapping into the current display's reported range. `HDRD-04` must choose:

- **Fixed mastering:** content is mapped to a fixed 1000-nit target and display adaptation is external;
- **Display-adaptive:** content mapping consumes a bounded queried display peak/black range;
- **Hybrid:** a fixed creative target is remapped within explicit bounds to current display capability.

The chosen rule must define diffuse white, peak, minimum/black handling, roll-off, average-light policy, invalid/missing display data, and whether a monitor move changes only output mapping or invalidates any View-owned state. No metadata field substitutes for the pixel transform.

The tone/gamut function must be a fully executable reference algorithm: input domain/units; negative and non-finite disposition; exposure/reference-white scale; luminance/chroma separation if any; shoulder/toe/black behavior; peak and full-frame constraints; gamut compression/clipping order; final component range; and effects of valid/invalid display facts. “Tone map to 1000 nits” is not an algorithm.

If display adaptation is admitted, output facts are immutable values tied to a display generation. A monitor move cannot mutate an already produced frame. Whether the adaptation invalidates only presentation-transform state or also View-visible creative state is explicit.

## SDR UI White

Windows defines nominal scene-referred SDR white at 80 nits and exposes a current user/system SDR white level. For self-composited linear UI, the candidate scale is:

```text
uiScale = acceptedSdrWhiteNits / 80
```

`HDRD-05` decides whether `acceptedSdrWhiteNits` is the current system value, a user-controlled value initialized from the system, or a bounded fallback such as 200 when unavailable. The final UI samples must enter the same Rec.2020/PQ encoding exactly once. Alpha/blend semantics must be defined in the linear composition domain.

## UI Composition And Alpha

An SDR UI sample is decoded by its owning UI texture/color contract into linear SDR-relative RGB, converted into the active target-linear primaries/scale, and multiplied by the accepted `SdrWhiteNits / 80` relation where applicable. Premultiplied or straight alpha is frozen before implementation. The blend executes once in the target-linear composition domain; only the composite is PQ-encoded or converted to the scRGB signal.

The HDR10 swapchain route is opaque unless `HDRD-03/07` explicitly proves a compatible alpha/composition cell. Its packed two alpha bits use a frozen constant and are not the semantic alpha of the UI blend. Raw capture retains scene/UI alpha only at products where it is meaningful. An overlay/interposer composition route must not cause the scene to be PQ-encoded twice or the UI to bypass its white conversion.

## UINT10 Quantization And Dither Candidate

For an HDR10 full-range RGB component `E_PQ`, the no-dither reference code candidate is:

```text
code10 = roundToNearestTiesRule(clamp(E_PQ, 0, 1) * 1023)
```

`HDRD-06` must freeze rounding/tie behavior, packed channel order, alpha bits, and whether deterministic spatial dither is required. If dither is admitted, it must define distribution, amplitude in code-value units, spatial coordinate, frame dependence, seed identity, mean/error bound, edge clamp, capture/oracle policy, and temporal-stability threshold. Random or frame-dependent noise cannot enter silently because it would destroy reproducibility and raw backend comparison.

The oracle tests code values around every selected rounding boundary and peak/black values. It records both floating PQ and packed integer results so transfer and quantization errors remain separable.

## scRGB Semantic Separation

If Stage 0 admits FP16/scRGB for an editor or composition profile, [Research](Research.md) supplies current Windows precedent, but this page must add exact scRGB primaries, linear scale relative to nominal SDR white, legal range, FP16 conversion/rounding, alpha/composition, tone/display-mapping responsibility, capture naming, and independent hand cases. The current HDR10 equations cannot be relabeled and reused.

Until that profile-specific section is ratified, scRGB is `Blocked/Ineligible`, not an automatic fallback. Fallback means the known SDR path unless the accepted product matrix explicitly defines another current profile.

## Metadata Is Not Pixel Semantics

Static metadata records primaries/white and mastering/content luminance values when the accepted native route uses it. It does not select color space, encode PQ, tone map, prove monitor behavior, or guarantee transport to the display. Zero/unknown values and omission follow the accepted platform policy. Metadata values must agree with the actual target policy, but agreement is a consistency check rather than an active-HDR oracle.

If metadata is admitted, the semantic manifest separately records mastering primaries/white, minimum/maximum mastering luminance, maximum content light level, maximum frame-average light level, value validity/source, quantization/unit conversion, native call disposition, and output/swapchain generation. `0`, unknown, omitted, and not-applicable are distinguished. Values cannot advertise a peak/FALL outside the actual target/pixel policy without an explicit product rationale.

## Invalid And Exceptional Values

| Condition | Required disposition |
| --- | --- |
| scene/UI input NaN or Inf | follow one display-pipeline finite policy before nonlinear functions; report named diagnostic/evidence result |
| negative scene RGB | preserve through accepted target/gamut stage or handle by its explicit rule; do not call PQ `pow` on invalid base |
| target luminance below/above accepted range | map/clamp/reject only at the frozen owner/step; record boundary behavior |
| invalid/missing/stale display peak/black/FALL or SDR white | use accepted bounded fallback or make profile ineligible; validity/reason remains observable |
| singular/invalid chromaticities or matrix | reject semantic/profile generation before active use |
| wrong/missing active presentation profile | Renderer emits existing SDR chain for coherent SDR result or withholds frame during bounded transition; never guess PQ |
| zero/minimized extent | no presentation frame; request remains known and revalidation occurs on restore |
| output/profile generation changes mid-frame | current frame cannot publish as active; commit only a generation-consistent transform/tuple pair |
| PQ/packing value outside representable range | exact clamp/quantization rule; no integer overflow/wrap |
| metadata invalid or call fails | follow separate `HDRD-09` policy; never mutate pixel encoding implicitly |

## Precision And Tolerance Contract

The independent oracle uses at least binary64 and records exact rational PQ constants before evaluation. Stage 0 freezes matrix/tone/PQ shader precision, contraction policy if relevant, target resource format, quantization/dither behavior, raw artifact precision, component and perceptual/colorimetric tolerances, black/peak/UI measurement uncertainty, backend comparison rule, and which differences are expected from OS/display tone mapping.

Thresholds are predeclared per evidence class. A physical display tolerance cannot excuse wrong raw PQ values; backend agreement cannot excuse a shared wrong matrix; screenshot similarity cannot excuse a wrong active tuple.

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

## Retained Hand Cases

| ID | Input/setup | Required observation |
| --- | --- | --- |
| `HDR-HAND-01` | PQ luminance list including zero, sub-black policy, `0.0001`, `0.01`, `0.1`, `1`, `80`, `203`, target peak, `10000` | forward/inverse values and monotonicity match high-precision reference |
| `HDR-HAND-02` | source/Rec.2020 primaries, secondaries, D65 white, gray | matrix orientation/white preservation and tolerance are exposed |
| `HDR-HAND-03` | negative, saturated, and out-of-gamut scene wedges | accepted target/gamut mapping is bounded and distinctly located before PQ |
| `HDR-HAND-04` | diffuse white and fixed/display-adaptive/hybrid peak cases with valid/invalid/stale facts | selected policy and generation dependence are explicit |
| `HDR-HAND-05` | SDR UI white at nominal 80, typical queried value, bounds, fallback; alpha 0/0.5/1 | scale, primaries conversion, blend, and exactly-once PQ are exposed |
| `HDR-HAND-06` | PQ values immediately around selected 10-bit half-code boundaries | rounding, packing, channel order, alpha bits, and dither policy are exposed |
| `HDR-HAND-07` | deliberate double PQ, omitted PQ, wrong 1000/10000 normalization, wrong matrix | every mutation fails the numeric/raw oracle |
| `HDR-HAND-08` | Renderer HDR request with RHI SDR active, and reverse mixed generations | no mismatched frame publishes; coherent SDR or bounded no-frame result |
| `HDR-HAND-09` | metadata omitted/success/failure/stale/inconsistent | pixel/active result remains governed by profile tuple; consistency state is separate |
| `HDR-HAND-10` | current output and SDR-white change across a monitor move | later generation changes mapping/state; prior frame/artifact remains immutable |
| `HDR-HAND-11` | scRGB values if profile admitted | separate linear-scale/FP16/alpha/capture contract matches its own oracle |
| `HDR-HAND-12` | SDR fallback reference content | existing SDR raw/published result is unchanged within its prior contract |

## Rule-To-Code And Evidence Ledger

Stage 0 replaces each role description with the exact accepted current owner/symbol and concrete check.

| Rule | Intended owner | Independent check | Seeded defect |
| --- | --- | --- | --- |
| `HDR-MATH-01` | Stage-0-selected View/display prepared-scene-contract owner | domain/scale manifest and scene hand cases | unnamed/wrong scene primaries/units |
| `HDR-MATH-02` | Stage-0-selected Renderer HDR target-transform owner | CPU target ramps/wedges | double/omitted tone map or wrong peak policy |
| `HDR-MATH-03` | Stage-0-selected Renderer matrix-constant owner | independent chromaticity-derived matrices | transpose/wrong source matrix |
| `HDR-MATH-04` | Stage-0-selected Renderer PQ-helper owner | high-precision forward/inverse table | wrong constants or 1000 normalization |
| `HDR-MATH-05` | Stage-0-selected platform-fact to Renderer UI mapping owner | 80/current/fallback UI patches | fixed/stale/wrong-unit SDR white |
| `HDR-MATH-06` | Stage-0-selected UI composition/output-shader owner | alpha/premultiplication and double-encode sentinels | UI PQ twice or wrong blend domain |
| `HDR-MATH-07` | Stage-0-selected presentation metadata-projection owner | consistency/native disposition audit | metadata changes profile/active result |
| `HDR-MATH-08` | Stage-0-selected Renderer/RHI profile-generation join owner | mixed-generation/fallback raw captures | PQ pixels in SDR tuple or stale HDR output |
| `HDR-MATH-09` | Stage-0-selected capture/support owners | artifact-classification challenge | screenshot/metadata labeled raw/display proof |
| `HDR-MATH-10` | Stage-0-selected backend/profile adapters | paired raw semantic manifests | backend-specific semantic drift |

## Common Mathematical Failure Points

- treating relative scene values as nits without a reference-white/exposure scale;
- using a remembered or transposed scene-to-Rec.2020 matrix;
- applying PQ to luminance only while claiming component Rec.2020/PQ, or applying it twice;
- normalizing PQ by a creative 1000-nit peak instead of the ST 2084 10000-nit definition;
- clipping to display peak before/after the wrong gamut/tone stage;
- hard-coding 200-nit UI and labeling it the current system value;
- blending already PQ-encoded UI or applying SDR transfer before target-linear composition;
- assuming R10G10B10A2 packing order/rounding/dither or meaningful swapchain alpha;
- calling FP16/scRGB and UINT10/PQ interchangeable HDR encodings;
- allowing metadata fields to select tone mapping or establish active state;
- comparing compositor screenshots or photographs as if they were raw PQ buffers;
- losing the existing SDR transform while building fallback.

## `HDRD-00` Ratification Checklist

- [ ] scene primaries/white/units/exposure/grade/reference-white/finite contract;
- [ ] exact product/profile matrix, including separate HDR10 and any scRGB chains;
- [ ] target peak/black/diffuse/FALL/adaptation and invalid-display-fact policy;
- [ ] matrix chromaticities/derivation/adaptation/orientation/precision and gamut algorithm;
- [ ] tone algorithm, PQ constants/domain/precision, quantization/packing/dither/alpha rules;
- [ ] current/fallback SDR-white query/units/bounds/events and UI decode/convert/blend/encode contract;
- [ ] metadata fields/units/validity/consistency/disposition and non-authority invariant;
- [ ] Renderer/RHI generation join, mixed-state and SDR fallback semantics;
- [ ] raw/native/screenshot/external-measurement artifact identities, tolerances, fixtures, and defect controls;
- [ ] rule-to-owner/source ledger and invalidation triggers.

Unchecked items keep production stages blocked. Neither shader code nor a successfully lit monitor may become the de facto semantic contract.

## Primary Sources

Normative and platform foundations are revisioned in the [Research source ledger](Research.md#source-ledger), including ITU-R BT.2100/BT.2408, Microsoft Advanced Color/SDR-white/metadata guidance, and Vulkan color-space/metadata contracts. Those sources constrain the candidate but do not choose Sparkle's scene domain, target mapping, profile matrix, fallback, or evidence thresholds.

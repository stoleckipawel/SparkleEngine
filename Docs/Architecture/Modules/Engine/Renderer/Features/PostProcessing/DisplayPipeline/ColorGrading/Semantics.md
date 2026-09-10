# Color Grading Semantics

**Status:** proposed semantic contract; blocked until `CGRD-00` accepts every open rule

**Responsibility:** define the exact color domain, parametric transform, 3D-LUT mapping, identity, invalid-value behavior, and independent reference procedure for first-release color grading

**Authority boundary:** [Discovery](Discovery.md) accepts or rejects these rules; [Execution Architecture](ExecutionArchitecture.md) maps accepted semantics to owners and execution; [Plan](Plan.md) orders implementation; this page does not prove conformance

**Current readiness:** **0/100 — target only**. No semantic implementation exists.

## Semantic Boundary

The proposed feature consumes one output-resolution reconstructed scene-color sample and produces one scene-referred graded sample. Both input and output use the same accepted working primaries and white convention. Tone/gamut mapping, transfer encoding, chromatic aberration, UI composition, and display activation remain downstream owners.

`CGRD-02` and `CGRD-03` must freeze whether exposure multiplication occurs before or after the grade. Until then, neither a shader equation nor a screenshot can define the grade's input units.

## Proposed Value Contract

| Value | Meaning | Open decision |
| --- | --- | --- |
| `c` | finite RGB input in the accepted scene-working space | legal negative and HDR range |
| `slope` | per-channel multiplier; identity `(1,1,1)` | admitted min/max |
| `offset` | per-channel additive value in the grade's numeric domain; identity `(0,0,0)` | units and range |
| `power` | per-channel exponent; identity `(1,1,1)` | positive lower bound and negative-base rule |
| `saturation` | scalar interpolation from working-space luminance to chroma; identity `1` | weights and extrapolation range |
| `domainMin/domainMax` | inclusive LUT input domain, each channel strictly increasing | accepted `.cube` directive mapping |
| `N` | cubic LUT dimension with exactly `N^3` RGB entries | admitted dimensions and memory ceiling |

## Parametric Candidate

For each channel, form:

```text
u = c * slope + offset
```

`CGRD-04` must select one power rule:

1. **Clamp style:** `v = pow(max(u, 0), power)`.
2. **Signed style:** `v = sign(u) * pow(abs(u), power)`.
3. **No-power-negative rejection:** negative `u` with non-identity power makes the requested generation invalid.

These alternatives differ materially for wide-range scene color. The implementation may not select one by convenience.

After the accepted SOP rule, compute saturation:

```text
Y = dot(v, acceptedWorkingLumaWeights)
parametric = lerp(Y.xxx, v, saturation)
```

The luma weights belong to the accepted working primaries. Hard-coded Rec.709 weights are invalid unless `CGRD-02/05` explicitly choose that space.

## LUT Candidate

The source parser emits a canonical representation:

```text
ColorGradeLutSource
  dimension N
  domainMin float3
  domainMax float3
  samples[N * N * N] in one frozen RGB-major order
  sourceContentHash
  semanticSchemaIdentity
```

For input `x`, map each channel to grid coordinate:

```text
q = clamp((x - domainMin) / (domainMax - domainMin), 0, 1) * (N - 1)
i0 = floor(q)
i1 = min(i0 + 1, N - 1)
f = q - i0
```

The CPU reference performs eight-corner trilinear interpolation in the frozen axis order. `CGRD-07` may select tetrahedral interpolation instead only by replacing this rule and its checks before implementation. Texture coordinates must address texel centers and reproduce the CPU indexing convention; normalized hardware sampling is not itself an oracle.

`CGRD-08` must freeze whether the LUT consumes the parametric result or vice versa. The current candidate is `LUT(parametric(c))`, with no separate LUT-contribution blend. Disabled LUT state is identity by omission, not an uploaded identity texture required for correctness.

## Identity And Alpha

The complete grade is identity only when slope, offset, power, and saturation are exact neutral values and no LUT is active. In that state the graph omits grading work. If the pass executes, alpha is copied bit-for-bit from its input and never participates in the RGB transform.

An identity LUT must reproduce its canonical grid points exactly at the accepted representation precision and stay within the frozen interpolation tolerance between grid points. “Looks unchanged” is not an identity oracle.

## Invalid And Exceptional Values

| Condition | Required candidate disposition |
| --- | --- |
| non-finite parameter, domain, or sample | reject the requested generation before active publication |
| `domainMax <= domainMin` in any channel | reject with channel/directive identity |
| unsupported directive, 1D table, mixed table, duplicate required field, or trailing sample count | reject unless `CGRD-06` explicitly admits it |
| dimension outside accepted set or `N^3` overflow/resource ceiling | reject before allocation |
| scene input NaN/Inf | follow the display pipeline's accepted finite-value policy and increment/retain the named diagnostic; do not silently repair only in grading |
| sample outside LUT domain | clamp to the boundary unless discovery selects an explicit alternative |
| stale completed source/cooked/runtime generation | discard without changing active state |

## Semantic Rules

| ID | Rule |
| --- | --- |
| `CGR-MATH-01` | one accepted working-space definition governs CPU, shader, LUT metadata, captures, and user labels |
| `CGR-MATH-02` | parameter order and negative behavior are identical in CPU and shader implementations |
| `CGR-MATH-03` | neutral parameters and absent LUT form an exact no-work identity |
| `CGR-MATH-04` | saturation luminance weights derive from the accepted working primaries |
| `CGR-MATH-05` | LUT domain, axis order, texel centers, interpolation, precision, and edge behavior are explicit |
| `CGR-MATH-06` | parser counts and allocation math are checked before multiplication/allocation |
| `CGR-MATH-07` | parametric/LUT composition happens exactly once in the accepted order |
| `CGR-MATH-08` | alpha and viewport/product identity are preserved |
| `CGR-MATH-09` | invalid requested data cannot masquerade as an active identity grade |
| `CGR-MATH-10` | SDR and HDR consume the same scene-referred graded result; target output transforms remain separate |

## Independent Reference Procedure

The discovery package must include a small double-precision CPU evaluator independent of shader helper code and cooked-layout conversion. It consumes a manifest containing working-space identity, parameters, LUT metadata/data, input samples, expected disposition, and tolerances. Required cases include:

- neutral, primary, secondary, gray, negative, HDR, and boundary samples;
- parameter-only, LUT-only, and composed grade;
- asymmetric known-cell LUTs capable of detecting every axis permutation and half-texel shift;
- dimensions at the minimum, typical, maximum, and one over limit;
- malformed, truncated, extra, non-finite, duplicate, unsupported, and hostile-count input;
- order mutations, wrong luma weights, wrong negative rule, and stale generation as defect controls.

The reference procedure becomes evidence only when the accepted semantic revision and defect controls are recorded. Reusing the shader's parser/layout/evaluator in the oracle is prohibited.


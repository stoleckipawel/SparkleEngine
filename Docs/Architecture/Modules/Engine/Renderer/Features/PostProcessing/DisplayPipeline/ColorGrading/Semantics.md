# Color Grading Semantics

**Status:** proposed semantic contract; blocked until `CGRD-00` accepts every open rule

**Responsibility:** define the exact color domain, parametric transform, 3D-LUT mapping, identity, invalid-value behavior, and independent reference procedure for first-release color grading

**Authority boundary:** [Discovery](Discovery.md) accepts or rejects these rules; [Execution Architecture](ExecutionArchitecture.md) maps accepted semantics to owners and execution; [Plan](Plan.md) orders implementation; this page does not prove conformance

**Current readiness:** **0/100 — target only**. No semantic implementation exists.

## Semantic Boundary

The proposed feature consumes one output-resolution reconstructed scene-color sample and produces one scene-referred graded sample. Both input and output use the same accepted working primaries and white convention. Tone/gamut mapping, transfer encoding, chromatic aberration, UI composition, and display activation remain downstream owners.

`CGRD-02` and `CGRD-03` must freeze whether exposure multiplication occurs before or after the grade. Until then, neither a shader equation nor a screenshot can define the grade's input units.

## Claim Boundary

This contract is intended to make the following claim decidable: for a finite scene-working RGB input, accepted parameters, and an optional accepted LUT generation, Sparkle produces the same graded RGB as the frozen independent evaluator within the declared representation tolerance, preserves alpha and product identity, and applies that transform exactly once at the accepted display-pipeline edge.

It does **not** claim colorimetric display accuracy, calibration, monitor profiling, ACES conformance, OCIO compatibility, camera-log decoding, artistic quality, or correctness of downstream tone/gamut/output transforms. Those claims require different owners and evidence.

## Notation, Direction, And Working Domain

All vectors are component tuples written `(r, g, b)`. Operations without an explicit matrix are component-wise. `lerp(a,b,t) = a + t(b-a)`. The LUT maps input grid coordinates to output RGB; no inverse transform is implied. Texture coordinates use the Renderer convention selected by `CGRD-07`, but the semantic oracle addresses integer grid cells directly and is independent of API texture orientation.

The accepted Stage-0 record must name:

- the scene-working RGB primaries and white chromaticity;
- whether values are linear with respect to light and whether exposure has already been applied;
- the reference-white/unit convention needed to interpret offset and LUT domains;
- legal finite input/parameter/output ranges and the pipeline-wide NaN/Inf policy;
- the exact point where conversion to target output primaries begins.

Names such as `linear`, `HDR`, `scene color`, or `Rec.709` are insufficient unless the chromaticities, white, encoding, and relative/absolute scale are recorded. CPU manifests, shader constants, captures, and UX labels must carry the same semantic revision.

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

## Candidate `.cube` Grammar And Canonicalization

`CGRD-06` must accept, narrow, or reject this candidate grammar before Stage 3:

```text
document       := trivia* statement (line-end trivia* statement)* trivia*
statement      := title | domain-min | domain-max | lut3d-size | sample | comment
title          := "TITLE" whitespace quoted-utf8-label
domain-min     := "DOMAIN_MIN" whitespace finite-number whitespace finite-number whitespace finite-number
domain-max     := "DOMAIN_MAX" whitespace finite-number whitespace finite-number whitespace finite-number
lut3d-size     := "LUT_3D_SIZE" whitespace unsigned-decimal
sample         := finite-number whitespace finite-number whitespace finite-number
comment        := "#" bounded-utf8-text
```

The parser candidate admits exactly one `LUT_3D_SIZE`, at most one `TITLE`, at most one `DOMAIN_MIN`, at most one `DOMAIN_MAX`, and exactly `N^3` samples after directives. Missing domain directives mean the accepted default only if Stage 0 freezes that default. It rejects `LUT_1D_SIZE`, mixed 1D/3D data, duplicate structural directives, non-finite syntax, locale-dependent decimals, hexadecimal values, trailing tokens, embedded NUL, invalid UTF-8 where text is required, and any directive outside the admitted set. Comments do not relax line/token ceilings.

Canonicalization is locale-independent and deterministic. It normalizes semantic values, not arbitrary source formatting: two sources may produce equal sample data while retaining different source-content identities. The cooked artifact records source-content hash, semantic-schema identity, accepted grammar revision, dimension, domain, frozen axis order, representation, data hash, and provenance. It never records a process-local path as runtime identity.

Before tokenization or allocation, the implementation checks source byte count, line length, token length, numeric-token count, dimension range, `N*N*N` overflow, sample-byte overflow, cooked-byte ceiling, and expected-versus-observed sample count. The Stage-0 budget supplies the numeric limits; “available memory” is not a valid limit.

## Axis, Storage, And Sampling Convention

The accepted representation must write one explicit formula that maps integer `(rIndex, gIndex, bIndex)` to canonical linear sample index. Candidate A, which still requires `CGRD-07` ratification, is:

```text
linearIndex = rIndex + N * (gIndex + N * bIndex)
```

For a native 3D texture with texel-center normalized coordinates, the candidate GPU coordinate is:

```text
uvw = (q + 0.5) / N
```

where `q` is the continuous grid coordinate from the domain mapping above. The implementation must prove that the API upload layout and shader coordinate convention reproduce the same eight cells and weights as the CPU oracle; neither file order nor hardware filtering is assumed correct. If the selected RHI representation uses a 2D strip, different axis order, manual interpolation, or half precision, this section must be replaced before implementation rather than patched by a hidden swizzle.

## Composition And Precision Contract

The current candidate complete transform is:

```text
gradedRgb = hasLut ? SampleLut(ApplyParametric(inputRgb)) : ApplyParametric(inputRgb)
gradedA   = inputA
```

`CGRD-08` must ratify the order. There is no strength blend, masked region, per-object selection, or second LUT in the admitted first release. CPU calculation uses binary64 for the independent oracle. Stage 0 must freeze source/cooked representation, runtime texture format, shader arithmetic expectations, per-case absolute/relative/ULP tolerances, and whether any backend-specific contraction is admitted. Tolerances describe accepted representation error; they may not hide wrong order, axis, domain, or edge behavior.

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

## Retained Hand Cases

These cases remain in semantic review even if automation later covers a larger corpus. Expected numbers are regenerated only from a ratified semantic revision and independently reviewed before candidate execution.

| ID | Input/setup | Required observation |
| --- | --- | --- |
| `CGR-HAND-01` | neutral parameters, no LUT, alpha sentinel | exact RGB/alpha identity and graph omission |
| `CGR-HAND-02` | asymmetric slope/offset/power on `(0.25, 0.5, 1.5)` | channel order and SOP order match the frozen equation |
| `CGR-HAND-03` | negative inputs crossing zero under non-identity power | accepted clamp/signed/reject rule is unmistakable |
| `CGR-HAND-04` | neutral gray and saturated primary under non-neutral saturation | frozen luma weights and extrapolation/clamp behavior are exposed |
| `CGR-HAND-05` | LUT exact corners and centers with asymmetric RGB-coded cells | axis order and integer cell indexing are exposed |
| `CGR-HAND-06` | point halfway among eight asymmetric cells | trilinear weights and texel-center mapping are exposed |
| `CGR-HAND-07` | samples one epsilon below/above each domain bound | boundary clamp and denominator behavior are exposed |
| `CGR-HAND-08` | parametric transform and LUT that do not commute | composition order and exactly-once application are exposed |
| `CGR-HAND-09` | minimum/maximum dimension and one-over-limit source | checked capacity decision occurs before allocation/publication |
| `CGR-HAND-10` | duplicate, truncated, extra, non-finite, huge-token, and huge-count text | parser rejects deterministically with bounded diagnostics |
| `CGR-HAND-11` | older request completes after newer request | stale generation is discarded and active identity remains truthful |
| `CGR-HAND-12` | same grade in two views, then edit only one | shared immutable resources may deduplicate, but View selection and result never alias |

## Rule-To-Code And Evidence Ledger

Stage 0 replaces each role description with the exact accepted current owner/symbol before production work.

| Rule | Intended implementation owner | Independent oracle/check | Seeded defect that must fail |
| --- | --- | --- | --- |
| `CGR-MATH-01` | Stage-0-selected View-prepared semantic constants and grade binding owner | manifest/capture semantic identity audit | mismatched working-space revision |
| `CGR-MATH-02` | Stage-0-selected single HLSL parametric helper owner | binary64 evaluator over `CGR-HAND-02/03` | swapped SOP order or negative rule |
| `CGR-MATH-03` | Stage-0-selected View/graph omission predicate owner | graph/resource trace plus raw identity | hidden identity pass/texture |
| `CGR-MATH-04` | Stage-0-selected accepted luminance-constant owner | primary/gray CPU cases | hard-coded wrong luma weights |
| `CGR-MATH-05` | Stage-0-selected cook-layout/upload/sampler/binding owners | asymmetric cells and interpolation points | axis permutation or half-texel shift |
| `CGR-MATH-06` | Stage-0-selected source parser/cook owner | hostile corpus and allocation instrumentation | unchecked cube/count multiplication |
| `CGR-MATH-07` | Stage-0-selected grade shader composition owner | noncommuting hand case and stage captures | LUT-before-parameters or double application |
| `CGR-MATH-08` | Stage-0-selected graph product/copy-contract owner | alpha sentinel, subrect, two-view capture | alpha transform or wrong product identity |
| `CGR-MATH-09` | Stage-0-selected request/residency publication owner | delayed/reordered completion schedule | failed request labeled active identity |
| `CGR-MATH-10` | Stage-0-selected presentation join owner | identical pre-tone grade capture under SDR/HDR | output profile changes grade semantics |

## Common Mathematical Failure Points

- treating the file row order as self-describing and compensating with an undocumented shader swizzle;
- applying Rec.709 luma coefficients to a different working-primary set;
- evaluating `pow` on negative values before the selected policy, producing backend-dependent non-finite results;
- normalizing by `N` instead of `N-1`, or addressing texel edges instead of centers;
- quantizing domains or samples during cook without carrying the representation in the semantic identity;
- clamping intermediate HDR values because the downstream output is SDR;
- fusing grade and tone mapping before independent stage products exist;
- reusing production parsing/sampling code in the oracle and reproducing the same defect;
- using broad screenshot tolerance that lets channel, order, or double-application errors pass;
- treating prior-good fallback pixels as proof that the newly requested generation succeeded.

## `CGRD-00` Ratification Checklist

The semantic contract can be marked accepted only when the discovery record names one answer for each item:

- [ ] working primaries, white, linearity, units, legal range, and exposure relation;
- [ ] SOP order, negative-base behavior, parameter ranges, saturation weights, and extrapolation behavior;
- [ ] `.cube` grammar, defaults, duplicate/unsupported/trailing behavior, UTF-8/numeric rules, and diagnostic bounds;
- [ ] dimension set, axis/linear index, domain mapping, edge rule, texel centers, interpolation, source/cooked/runtime precision;
- [ ] parameter/LUT order, absence/neutral predicate, alpha behavior, and exactly-once stage placement;
- [ ] finite-value policy shared with the display pipeline;
- [ ] CPU oracle independence, fixtures, tolerances, defect controls, and artifact formats;
- [ ] rule-to-code owner names and semantic-revision invalidation triggers.

Any unchecked item keeps production stages blocked. An implementation is not permitted to become the de facto answer.

## Primary Sources And Precedent

The candidate choices above are informed by the revision-pinned OpenColorIO, Filament, Unity, and security sources in the [Research source ledger](Research.md#source-ledger). Those sources demonstrate that multiple valid pipelines exist; they do not choose Sparkle's working domain or prove this contract. The accepted decision and its evidence remain local.

# Direct Lighting Sampling And Shading Contract

**Status:** target mathematical contract with explicit discovery parameters; no implementation or conformance claim

**Responsibility:** own direct-light radiometry, BRDF/lobe, light sampling, reservoir, reuse, visibility, reconstruction-signal, precision, and equation-to-code rules

**Authority boundary:** `DIR-D0` ratifies unresolved parameters; [Execution Architecture](ExecutionArchitecture.md) owns system shape; [Plan](Plan.md) owns order; code/tests must conform; `FCR-REN-06` owns results

**Notation:** directions point away from the surface point; `ng` is the geometric normal, `ns` the shading normal, `wo` the direction to the camera/previous vertex, and `wi` the direction to the sampled light

## Claim Boundary

This page defines what a correct direct-light sample means before pass layout or optimization. It covers surface direct illumination only. Environment radiance may be an infinite-light candidate after admission; atmospheric and participating-media transport belongs to [Volumetric Lighting](../VolumetricLighting/README.md). Indirect paths belong to [Indirect Lighting](../IndirectLighting/README.md).

Cells labeled `DIR-D0-*` are intentionally unresolved until discovery. Code must not choose them accidentally through constants or packing convenience.

## Working Space, Units, And Safety

The proposed common contract matches the accepted Reference Path Tracer basis:

- distances and areas are metres and square metres;
- working RGB is scene-linear sRGB/Rec.709 primaries with D65 white;
- point/spot authored luminous intensity is candela; directional authored illuminance is lux; rectangular luminance is candela per square metre;
- transport converts these photometric RGB inputs to the one renderer radiometric-equivalent convention frozen by `DIR-D0-02` and the reference tracer;
- negative, NaN, infinite, invalid-direction, degenerate-basis, and out-of-domain values reject before active publication or produce an explicitly frozen finite boundary;
- exposure, tone mapping, encoding, and denoising do not alter the estimator's units.

Use the same luminous-efficacy RGB approximation only if `DIR-D0-02` ratifies it. Do not independently convert units in GameFramework, Renderer preparation, light sampling, shading, and reference transport.

## Direct-Lighting Measurement

For a non-delta emitter domain `Y`, direct outgoing radiance at surface point `x` is

```text
Ldirect(x, wo) = integral_Y f(x, wo, wi(y)) Le(y -> x)
                  V(x, y) G(x, y) dA(y)
```

where `G = abs(dot(ng_x, wi)) * abs(dot(n_y, -wi)) / distance^2` for a finite area emitter after sidedness tests. In solid angle at `x` the receiver cosine is part of the integrand and the emitter-area Jacobian moves into the PDF. Delta point/spot/directional lights use their discrete/delta evaluation and never pretend to have an ordinary solid-angle density.

For light class/index selection `l` and a conditional shape sample `y`, the source probability is

```text
q(l, y) = p_select(l) * p_shape(y | l)
```

in the measure used by the estimator. If area sampling is converted to receiver solid angle,

```text
p_omega = p_area * distance^2 / abs(dot(n_light, -wi)).
```

Zero emitter cosine, zero distance, invalid sidedness, or a sample outside support contributes zero without division.

## Light Contracts

| Light | Required semantic evaluation | Discovery parameter |
| --- | --- | --- |
| directional disk | constant incident direction distribution over admitted angular radius; illuminance normalization independent of distance | disk/cone sampling and exact sun-disk normalization |
| point sphere | inverse-square intensity outside finite radius model; finite segment to sampled point | sphere emission model and inside/near-source behavior |
| spot sphere/cone | point/sphere geometry multiplied by normalized inner/outer angular falloff | falloff curve and range cutoff |
| rectangle | uniform or importance-sampled point on oriented rectangle, explicit normal/sidedness, area-to-solid-angle conversion | one/two-sided policy and near-field sampling |
| emissive triangle | world-space triangle plus material emission generation and exact area/PDF identity | admission, texture integration, animation, and two-sided policy |
| environment | lat-long direction with luminance-times-texel-solid-angle distribution | admission, mapping, pole/seam, rotation, and zero-map fallback |

Range is a culling/product boundary, not permission to violate the frozen attenuation law silently. Any smooth cutoff must be documented in both analytic and reference evaluators.

## Surface Lobe Contract

The admitted surface response decomposes once:

```text
f = f_diffuse + f_specular + f_subsurface_approx
```

The decomposition determines both output lobes and the reservoir target. `DIR-D0-03` must freeze an energy allocation before implementation. The proposed base-layer rule is:

```text
baseBudget = 1 - metallic
diffuseWeight + subsurfaceApproxWeight <= baseBudget
specular uses dielectric F0 blended to baseColor by metallic
```

This is a policy constraint, not a final formula. The current implementation must be audited rather than grandfathered.

### Specular

For non-delta GGX microfacet reflection:

```text
f_specular = F(wo, h) * D(h) * G2(wi, wo) /
             max(4 * abs(dot(ns, wi)) * abs(dot(ns, wo)), epsilon)
```

`h = normalize(wi + wo)`. `DIR-D0-03` freezes perceptual-roughness-to-alpha, minimum alpha, Smith correlated masking-shadowing, Schlick versus exact dielectric Fresnel scope, multiple-scattering compensation, shading-normal correction, and the delta threshold. Evaluation, sampling, and PDF functions must share the same decisions.

### Diffuse And Subsurface Approximation

Burley diffuse, if retained, uses the same roughness and direction convention in CPU/reference/GPU code. The wrap-subsurface term is explicitly an empirical surface lobe, not BSSRDF or volume transport. Its energy comes from the base diffuse budget; it cannot simply be added on top of an unchanged diffuse response.

### Shading Normals

Visibility begins from the geometric surface and geometric-normal offset. Shading normals influence the BSDF only under a frozen correction/clamp. Backfacing configurations that violate the geometric hemisphere produce zero or follow the explicit two-sided material contract; flipping normals opportunistically to obtain light is prohibited.

## Initial Candidate Estimator

For candidate `y` with vector lobe contribution `C(y)` excluding final reconstruction, define the non-negative scalar target

```text
pHat_x(y) = target(C_x(y)).
```

The proposed target is Rec.709 luminance of the sum of admitted non-negative direct lobes, with a documented positive floor only where required to preserve support. Whether candidate visibility is included is a `DIR-D0-07` decision because it trades extra rays against bias/noise. Chromatic robustness tests must prevent saturated low-luminance samples from being starved.

For an independently drawn candidate from source density `q(y)`:

```text
w(y) = pHat_x(y) / q(y).
```

Weighted reservoir update for accumulated weight `Wsum` is:

```text
Wsum' = Wsum + w
select y with probability w / Wsum'
M' = M + m
```

where `m=1` for a fresh candidate and is the accepted effective sample multiplicity for a reused reservoir. Invalid/zero-support candidates contribute neither weight nor multiplicity unless the ratified bias formulation requires otherwise.

After streaming, the selected-sample normalization is conceptually

```text
W = Wsum / (M * pHat_x(y_selected)).
estimate = W * C_final_x(y_selected).
```

This familiar expression is valid only under the assumptions frozen for the active resampling mode. Code must not label it unbiased merely because the algebra appears.

## Temporal And Spatial Reuse

A source reservoir represents candidates from another receiver/domain. Re-evaluate the selected sample at current receiver `x` and transform it with the ratified generalized RIS/bias-correction rule. At minimum the operation needs:

- exact prior selected sample and previous light generation;
- current-to-previous light identity translation;
- prior target and weight state with bounded effective `M`;
- source and destination receiver data required by the correction;
- visibility state only when its reuse assumptions are satisfied;
- a deterministic random dimension distinct from initial sampling and every neighbor;
- rejection before accessing stale resources.

The base delivery supports named modes, not a boolean “unbiased” flag:

| Mode | Contract | Claim |
| --- | --- | --- |
| `Off` | no cross-receiver reservoir reuse | analytic/debug only |
| `Uncorrected` | `1/M`-style normalization under restricted compatibility | explicitly biased; debug/research only unless bounded by product decision |
| `Basic` | canonical/basic MIS correction using available source surfaces | target candidate for normal product profile |
| `RayTraced` | conservative source visibility plus stronger correction | high-quality candidate if added rays and history are justified |

The exact equations and mode names must be ratified against the pinned RTXDI/GRIS reference during `DIR-D0-07`; this page deliberately does not invent a partial correction.

## Compatibility And Invalidation

Reuse requires all of the following to be valid or deliberately accounted for:

```text
view generation + extent + sample pattern + motion convention
surface hit class + object/primitive generation + material generation
geometric/shading normal + depth/distance + roughness/lobe support
light generation + current/previous translation + environment generation
estimator settings + shader publication + traversal provider semantics
```

Thresholds are quality parameters, not identity substitutes. A close normal and depth do not prove the same material, object, or light. Camera cuts, invalid motion, out-of-bounds reprojection, disocclusion, topology/provider change, and incompatible generation reject before reuse. Neighbor offsets are decorrelated and resolution-aware; repeated fixed directional patterns require artifact evidence.

## Visibility Contract

For finite lights, trace the half-open segment from a robustly offset receiver to before the sampled emitter point. Infinite lights use the frozen far interval. The semantic result is `0` blocked or `1` visible for the base opaque/alpha-mask surface model; partial transmission is not silently approximated.

Inline and Pipeline providers must agree on:

- instance/primitive identity and transforms;
- front/back-face and two-sided policy;
- alpha texture, UV, address/filter/LOD, and cutoff;
- ray flags, culling, minimum/maximum `t`, and closest/any-hit semantics;
- self-intersection offset and near-contact behavior;
- missing/stale material or SBT failure.

An optional screen trace can only return an acceleration hint plus confidence/fallback status; it cannot turn an unresolved off-screen ray into visible. Opacity micromaps, shadow maps, and VSM remain separately admitted provider designs.

## Reconstruction Signal Contract

The raw direct estimate is authoritative. Reconstruction receives immutable same-frame or explicitly previous-frame products:

| Input | Required identity |
| --- | --- |
| diffuse/specular/subsurface radiance and hit distance | estimator, exposure basis, extent, lobe semantics |
| depth and geometric/shading normal | projection/depth decode and normal-space generation |
| roughness/material/lobe class | material generation and decode |
| motion | current-to-previous convention, jitter treatment, validity |
| disocclusion/reservoir confidence | reuse decision and source generation |
| selected-light size/type where required | translated light generation |

Reconstruction history has its own validity and cap. It cannot write back altered radiance, hit distance, or confidence into reservoir history unless a future accepted estimator explicitly owns that feedback loop.

## Precision And Numerical Rules

- Stable IDs, frame/sample indices, and counts use integer storage; do not round long-lived identities through binary32.
- Reservoir weights/targets must define overflow/underflow behavior, finite checks, maximum `M`, and reset response.
- Random dimensions are versioned and unique by stage/candidate/neighbor; changing the layout invalidates history and comparisons.
- Zero candidate count, zero lights, black environment, and fully occluded pixels produce a finite zero result with valid empty state.
- Every division has a support precondition; epsilon cannot manufacture contribution outside support.
- Accumulation/reconstruction uses pre-exposure only under one frozen convention and records it in history identity.

## Equation-To-Code Ledger

| Contract | Intended owner | Required test |
| --- | --- | --- |
| units/light geometry | one shared lighting semantic include plus CPU oracle | `CHK-DIR-01` |
| BRDF evaluation/sampling/PDF | one shared material-lighting semantic owner | `CHK-DIR-01` |
| initial distribution/mixture PDF | Direct Lighting feature sampler | `CHK-DIR-02` |
| reservoir stream/final normalization | Direct Lighting reservoir owner | `CHK-DIR-02` |
| temporal/spatial correction | Direct Lighting reuse owner | `CHK-DIR-03` |
| light translation/history compatibility | prepared lighting state plus feature history | `CHK-DIR-03` |
| visibility segment/alpha semantics | shared ray semantic kernel, provider adapters | `CHK-DIR-04` |
| reconstruction inputs/history | selected reconstruction owner | `CHK-DIR-06` |

## Ratification Checklist

`DIR-D0` must replace every unresolved policy with an exact equation, enum/value domain, precision, error rule, source pin, CPU hand case, shader mapping, and acceptance threshold. If a source paper/SDK exposes multiple estimators, the chosen one and rejected alternatives must be named. No implementation stage may infer missing math from existing code.

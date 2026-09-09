# Offline Path Tracer Transport And Estimator Contract

**Status:** proposed mathematical implementation contract and `PTD-00` decision template; no equation, parameter, feature disposition, or oracle claim on this page is accepted until the discovery report ratifies this exact revision

**Responsibility:** define one notation, estimator, event algorithm, probability-measure contract, PBR material boundary, numerical policy, equation-to-code ledger, and mathematical failure checklist for `SurfaceTransportReference` and `FinitePathDiagnostic`

**Authority boundary:** [Discovery](Discovery.md) owns ratification and implementation authorization, [Execution Architecture](ExecutionArchitecture.md) owns system ownership and lifetime, [User Experience](UserExperience.md) owns the human and automation workflow, the [feature dossier](README.md) owns acceptance, [Research](Research.md) owns precedent, and the [staged plan](Plan.md) owns delivery order and prompts

**Prepared:** 2026-09-09 against committed `master` revision `a91d13c5`; the current shader route was inspected, but no equation was implemented or executed

**Non-claims:** this specification does not prove that the current or future implementation is unbiased, energy conserving, numerically robust, converged, backend-equivalent, or usable as an oracle

> [!IMPORTANT]
> An implementer may not select a missing constant, BRDF variant, normal treatment, PDF measure, invalid-sample rule, or threshold while writing code. `PTD-00` must first fill every decision slot below, record the reviewing experts, and bind the result to one immutable report revision.

## Claim Boundary

The proposed full product is a unidirectional, camera-originating Monte Carlo estimator of **scene-linear RGB surface radiance** for the accepted opaque reflective domain. It includes camera-visible emission, environment emission, direct-light next-event estimation (NEE), indirect surface reflection, and compensated Russian roulette. It excludes transmission, participating media, physical subsurface transport, spectral transport, and any undeclared approximation.

Two products remain mathematically distinct:

| Product | Target | Ordinary termination | Evidence use |
| --- | --- | --- | --- |
| `SurfaceTransportReference` | The full supported surface-reflection integral over paths admitted by the frozen scene/material/light/camera domain. | Absorption, escape, zero physical throughput, or compensated Russian roulette. A watchdog or safety-depth event fails the sample/job; it is not a zero contribution. | Candidate oracle only after every applicable `AC-OPT-*` criterion passes. |
| `FinitePathDiagnostic(D)` | The finite sum containing at most the positive integer `D` admitted surface vertices, where `x_0` counts as the first. Camera-visible environment/emission remains observable, emission and NEE are evaluated at every admitted surface, and no BSDF continuation occurs after `x_(D-1)`. | Declared deterministic depth; roulette is disabled. `D`, event semantics, and the finite label enter artifact identity. | Hand cases, path-event isolation, and semantically matched external comparisons; never substituted for the full product. |

“Unbiased” applies only to the declared mathematical target. It does not mean noiseless, photorealistic, physically complete, spectrally exact, or independent of all shared code. Consistency and finite-sample uncertainty are separate evidence claims.

## Notation And Direction Convention

| Symbol | Meaning and required measure |
| --- | --- |
| `x_k` | Surface vertex at scattering depth `k`; the first camera-visible surface is `x_0`. |
| `y` | Sampled point on an area or emissive light. |
| `omega_o` | Unit direction leaving `x_k` toward the preceding vertex or camera. |
| `omega_i` | Unit direction leaving `x_k` toward the next vertex/light, from which incident radiance arrives. |
| `n_g`, `n_s` | Oriented unit geometric and shading normals. Geometric orientation owns sides, visibility, and ray spawning; the accepted effective BSDF owns shading-normal behavior. |
| `L_o`, `L_i`, `L_e` | Outgoing, incident, and emitted radiance in the frozen scene-linear RGB working space. |
| `f*` | Complete effective reflective BSDF for the accepted material and shading-normal policy, expressed per unit solid angle. |
| `c_g(omega)` | `abs(dot(n_g, omega))` after the accepted side test. One-sided surfaces reject the back side; a two-sided surface first orients `n_g` by the frozen rule. |
| `beta_k` | RGB path throughput on arrival at `x_k`, before emission or NEE at that vertex. `beta_0 = 1`. |
| `p_B^omega` | Full BSDF-strategy density for a continuous direction, with respect to solid angle at `x_k`. |
| `p_L^omega` | Complete light-strategy density for a continuous direction, including light-selection PMF, with respect to solid angle at `x_k`. |
| `pi_l`, `alpha_j` | Discrete probability of selecting light `l` or BSDF lobe `j`. |
| `V(x,y)` | Binary visibility under the accepted alpha, sidedness, endpoint, and occlusion contract. |
| `H_a` | MIS weight assigned to technique `a`; proposed baseline is the power heuristic with exponent two. |

Implementation types and event records must preserve whether a probability is a discrete mass, an area density, a solid-angle density, or a delta distribution. A bare field named `Pdf` is insufficient at a cross-owner or diagnostic boundary.

## Radiometry, Color, And Units

Raw beauty is scene-linear RGB radiance in one frozen set of primaries and white point. The manifest must name those chromaticities or a stable repository-owned color-space identity. RGB is an explicitly three-channel approximation; no spectral-accuracy claim is permitted.

All light inputs are converted once, before sampling, into the radiometric quantity consumed by their light contract. Directional-light illuminance, point/spot luminous intensity, area/emissive radiance, exposure-like artistic multipliers, and environment texels are not interchangeable values. Stage 0 must provide a per-light dimensional analysis and known-value conversion fixture.

The raw path must not apply exposure, tone mapping, gamut mapping, output transfer functions, denoising, temporal reconstruction, contribution/firefly clamps, or lossy encoding. OpenEXR metadata records the color space and channel semantics; it does not define them.

## Mathematical Contract

### `MATH-01` — Pixel Measurement

For pixel footprint `A_p`, normalized reconstruction filter `W_p`, and camera sample `u ~ q_p(u)`, the target pixel value is

```text
P_p = integral[A_p] W_p(u) L_o(r(u)) du / integral[A_p] W_p(u) du

X_p(u) = W_p(u) L_o(r(u))
         / (q_p(u) integral[A_p] W_p(v) dv)
```

The arithmetic mean of valid `X_p` samples estimates `P_p`. The expected first slice is a pinhole camera and a normalized box filter with uniform subpixel sampling, but `PTD-00` must freeze raster-to-film mapping, pixel-center convention, crop semantics, projection, handedness, ray differentials/texture LOD policy, and zero-width edge behavior. A camera ray generated from jittered NDC without this correspondence is not accepted.

### `MATH-02` — Supported Surface Transport

At an accepted reflective surface,

```text
L_o(x, omega_o) = L_e(x, omega_o)
  + integral[H_g(x)] f*(x, omega_i, omega_o)
                       L_i(x, omega_i)
                       c_g(omega_i) d omega_i
```

`H_g(x)` is the side domain established by the oriented geometric normal and material sidedness. `f*` includes the complete accepted shading-normal treatment. This formulation prevents an implementer from mixing a shading-normal cosine with an uncorrected geometric-measure derivation.

### `MATH-03` — BSDF Mixture

Let the selectable BSDF strategies be partitioned into continuous lobes `C` and delta lobes `D`, with selection masses

```text
sum[j in C union D] alpha_j = 1, alpha_j >= 0
```

For a continuous direction,

```text
f*_C(omega_i, omega_o) = sum[j in C] f*_j(omega_i, omega_o)
p_B^omega(omega_i)     = sum[j in C] alpha_j p_j^omega(omega_i)
```

After choosing a continuous `j ~ alpha` and sampling `omega_i ~ p_j`, evaluate the **complete continuous** `f*_C` and **complete unconditional mixture density** `p_B^omega` for that direction. The continuation update is

```text
beta_(k+1) = beta_k f*_C(x_k, omega_i, omega_o) c_g(omega_i)
             / p_B^omega(omega_i)
```

For a realized delta event `e`, its complete probability mass and update are

```text
P_B(e)       = sum[j in D that generate e] alpha_j P_j(e)
beta_(k+1)   = beta_k W_delta(e) / P_B(e)
```

`W_delta(e)` is the summed analytic throughput numerator of every compatible accepted delta component, including its Fresnel and transport factors under the frozen convention; each conditional deterministic delta choice normally has `P_j(e) = 1`. Stage 0 must hand-derive the exact smooth branches rather than treating this placeholder as a formula. Lobe-selection mass must not be multiplied twice or omitted. Exact delta reflection is not approximated by clamping roughness to a small continuous GGX value. Delta probability mass and continuous solid-angle density remain different measures and never enter one numeric sum.

### `MATH-04` — Light Selection And Measure Conversion

The complete density of a continuous light sample is

```text
p_L^omega(omega_i) = pi_l(x) p_l^omega(omega_i | x)
sum[l eligible at x] pi_l(x) = 1
```

If light `l` samples point `y` with area density `p_l^A(y)`, then

```text
p_l^omega(omega_i | x)
  = p_l^A(y) ||y - x||^2 / abs(dot(n_g(y), -omega_i))
```

The conversion is invalid at zero distance, zero projected emitter cosine, a non-finite term, or a side rejected by the light contract. It is not repaired with an epsilon. Point and ideal directional lights are delta-direction techniques and use their explicitly derived discrete estimator branch.

For a latitude-longitude environment parameterization with `phi = 2 pi u` and `theta = pi v`,

```text
d omega = 2 pi^2 sin(theta) du dv
p_env^omega = p_env^uv(u,v) / (2 pi^2 sin(theta))
```

Pole handling, texel solid angle, texture addressing, and the mapping between `omega` and `(u,v)` must be exact. The sampling distribution may approximate importance for efficiency only if its recorded PDF matches the distribution actually sampled and every nonzero radiance direction retains support. Raw radiance evaluation uses the accepted full-resolution texture and declared filtering/LOD policy.

### `MATH-05` — Next-Event Estimation

For one valid continuous light sample at vertex `x_k`,

```text
C_NEE,k = beta_k
          f*_C(x_k, omega_i, omega_o)
          L_i(x_k, omega_i)
          c_g(omega_i)
          V(x_k, y)
          H_L(p_L^omega, p_B^omega)
          / p_L^omega
```

The light-selection PMF, conditional density, area-to-solid-angle Jacobian, receiver cosine, emitter sidedness, attenuation/radiance conversion, and visibility occur exactly once. If the sampled light is delta and the BSDF strategy cannot sample the same event, `H_L = 1`; a fabricated finite competitor PDF is forbidden.

For an ideal point or directional light selected with PMF `pi_l`, the conditional delta event has unit probability mass and the discrete branch is

```text
C_delta,k = beta_k f*_C(x_k, omega_l, omega_o) L_i(x_k, omega_l)
            c_g(omega_l) V / pi_l
```

Any additional discrete choice multiplies the denominator exactly once. A finite-radius point/spot/directional approximation is instead an area/solid-angle technique and must use the continuous formula above; the implementation cannot switch interpretations without changing input identity.

### `MATH-06` — MIS

For `n_a` samples from each comparable continuous technique and exponent `gamma = 2` in the proposed baseline,

```text
H_a = (n_a p_a)^gamma / sum[t] (n_t p_t)^gamma
```

The first implementation uses one BSDF and one light sample where eligible, so `n_B = n_L = 1`. Both PDFs must describe the same generated direction, event, and solid-angle measure. A zero competitor PDF gives weight one to the supported technique. Delta events bypass continuous-density MIS. `PTD-00` must either ratify the power heuristic or replace this section and every hand case before Stage 1.

### `MATH-07` — Emissive And Environment Hits

When a BSDF continuation reaches emissive geometry or escapes to the environment,

```text
C_hit,k = beta_k L_e H_B(p_B, p_L)
```

Use `H_B = 1` for the camera ray, after a delta BSDF event, or whenever no eligible light-sampling technique could generate the same event. Otherwise compute `p_L` at the **previous** vertex as light-selection PMF times the light's solid-angle PDF for the realized direction and apply `MATH-06`. This branch is the BSDF half of direct-light MIS; adding unweighted emission after NEE double counts light.

### `MATH-08` — Compensated Russian Roulette

After updating throughput for an accepted continuation and after the frozen minimum depth, choose survival probability

```text
s_k = clamp(g(beta_(k+1), path state), s_min, s_max)
0 < s_min <= s_max < 1
```

The initial candidate is `g(beta) = max(beta.r, beta.g, beta.b)` for the RGB reflective-only domain. If `xi_rr >= s_k`, terminate with no later contribution; on survival,

```text
beta_(k+1) <- beta_(k+1) / s_k
```

`PTD-00` freezes minimum depth, bounds, comparison convention, and the exact state observed by `g`. The upper bound below one guarantees an eventual stochastic termination opportunity even on unit-throughput paths. Transmission would require an explicit refractive scaling analysis and is excluded from this formula. A deterministic shader-loop ceiling is operational containment only: reaching it invalidates the affected sample/job and cannot return black as though the full target were evaluated.

### `MATH-09` — Stateless Sample Identity

Every random value is a pure function

```text
U = ToUnitFloat(CounterGenerator(
      key     = (JobSeed, ReplicateId),
      counter = (PixelX, PixelY, SampleOrdinal, DimensionId)))
```

The exact generator, rounds, integer packing, endianness, overflow policy, and open/closed interval conversion are frozen and recorded. Conceptual dimensions are assigned by name and stable offset, including film, lens/time if admitted, per-depth light selection, light shape, lobe selection, BSDF direction, alpha decision if stochastic behavior is accepted, and roulette. Branches do not shift later dimensions. A layout or generator change invalidates checkpoints rather than adding a legacy stream reader.

### `MATH-10` — Accumulation And Uncertainty

For RGB sample `X_n`, update the arithmetic mean and centered sum of squares with the stable online recurrence

```text
delta  = X_n - mean_(n-1)
mean_n = mean_(n-1) + delta / n
M2_n   = M2_(n-1) + delta * (X_n - mean_n)

sampleVariance_n = M2_n / (n - 1), n > 1
standardError_n  = sqrt(sampleVariance_n / n)
```

Two independently accumulated complete batches `A` and `B` merge as

```text
delta = mean_B - mean_A
n     = n_A + n_B
mean  = mean_A + delta n_B / n
M2    = M2_A + M2_B + delta^2 n_A n_B / n
```

The concrete GPU representation and precision require a maximum-SPP error study against a higher-precision oracle. `sum(X^2) - sum(X)^2/n` is not accepted without an error proof because cancellation can destroy small variance. Per-pixel standard error is descriptive only when sample assumptions hold; uncertainty for low-discrepancy or correlated rendering uses independently randomized replicates and the predeclared regional protocol. Variance never decides an adaptive stop in the first reference product.

### `MATH-11` — Robust Surface And Connection Rays

For a reconstructed world-space point `p` with conservative absolute error vector `Delta_p` and oriented unit geometric normal `n_g`, the candidate origin bound is

```text
d     = dot(abs(n_g), Delta_p)
sigma = -1 if dot(w, n_g) < 0, otherwise +1
po    = p + sigma d n_g
```

Each nonzero component of `po - p` is then rounded to the next representable float away from `p`. `Delta_p` must include object-space reconstruction, object-to-world transformation, and traversal uncertainty derived for Sparkle's formats and compiler/backend behavior. Connection rays apply corresponding bounds at both endpoints and reject collapsed intervals.

This is a derivation shape, not permission to copy constants. Stage 0 must reconcile the NVIDIA DXR analysis and PBRT error-bound method with Sparkle's row-vector transforms, instance packing, triangle formats, inline/pipeline routes, and D3D12/Vulkan behavior.

## PBR Material Contract

The path tracer judges lighting only if the material being integrated is itself explicit. `PTD-00` must freeze every row below and provide evaluation, sampling, PDF, limit, and energy evidence where applicable.

| Surface | Required decision before implementation | Non-negotiable failure boundary |
| --- | --- | --- |
| Base color | Source transfer function, factor semantics, working-space conversion, vertex-color multiplication, and legal range. | Sampling stored sRGB as linear or applying display conversion inside transport fails. |
| Metallic | Exact dielectric/metal mixture, intermediate-value meaning, and texture channel. | Metallic becomes only a lobe-selection probability or is multiplied twice. |
| Dielectric F0 | Frozen IOR/F0 mapping and whether material-authored F0 exists. | `0.04`, IOR `1.5`, or a current helper is assumed without the scene contract. |
| Diffuse | Exact Lambertian, Burley, or other formula and its interaction with Fresnel/metallic. | Real-time empirical diffuse is silently called physically exact; sample/eval/PDF disagree. |
| Rough specular | Isotropic GGX NDF, Smith masking-shadowing variant, Fresnel model, `roughness -> alpha` mapping, and visible-normal sampler are expected candidates. | Fast correlated approximations, roughness floors, or invalid-value clamps enter without a bounded claim. |
| Smooth limit | Exact delta reflection at the accepted zero-roughness boundary. | A small arbitrary roughness changes the target while retaining the same label. |
| Multiple scattering | Decide whether single-scatter GGX energy loss is accepted as the canonical material or compensated by a reviewed model. | White-furnace energy loss is hidden by exposure, clamp, or scene tuning. |
| Normal map | Tangent basis, handedness, decode, scale, normalization, hemisphere validity, effective-BSDF correction/model, and shadow-terminator policy. | Shading normal controls ray origin/sidedness, creates energy, or yields a different forward/backward contract without disclosure. |
| Alpha mask | Texture/factor ordering, transfer function, cutoff equality, mip/filter policy, and any stochastic coverage rule. Expected baseline is deterministic semantic cutout. | Raster alpha-to-coverage or a different any-hit rule changes visibility silently. |
| Two-sided | Geometric-normal orientation, emission side, and shading-frame reconstruction. | Flipping only the shading normal makes visibility and scattering disagree. |
| Emission | RGB space, radiance/unit conversion, sidedness, texture/factor behavior, and emissive-light registration. | A visible emissive surface is not sampled, is sampled twice, or uses arbitrary display color as radiance. |
| Ambient occlusion | Excluded from raw physical transport; it may remain a separately labeled real-time approximation subject. | Baked AO attenuates path-traced indirect light or the oracle shares the approximation it should judge. |
| Subsurface/transmission/media | Excluded by default under `OPT-FS-18` unless discovery expands the transport equation, state, and evidence. | A real-time appearance lobe is evaluated as if it were physical transport. |

The proposed reflective baseline is an energy-reviewed diffuse plus isotropic GGX reflection mixture with exact visible-normal sampling and matched full-mixture PDF. That is a candidate, not a frozen choice. Compatibility with the current Sparkle BRDF is evidence input; it is not permission to inherit its `saturate`, `max`, fast approximation, or sampling assumptions.

## Reference Algorithm

The semantic core must remain recognizable as this sequence. An optimization may reorder work only after proving identical event, probability, contribution, sample-identity, and diagnostic semantics.

```text
TraceSample(job, pixel, sampleOrdinal):
  filmSample <- Sample(MATH-09 film dimensions)
  ray        <- GenerateCameraRay(MATH-01, frozen View)
  beta       <- (1, 1, 1)
  radiance   <- (0, 0, 0)
  previous   <- CameraEvent

  for k = 0, 1, ...:
    hit <- TraceClosest(ray)

    if miss:
      Le <- EvaluateEnvironment(ray.direction)
      w  <- 1 if previous is camera/delta/unreachable-by-light
            else MIS_B(previous.bsdfPdfW,
                       LightPdfW(previous.vertex, environment, ray.direction))
      radiance += beta * w * Le
      return Valid(radiance, counters, events)

    surface <- ReconstructAndEvaluateFrozenSurface(hit)
    require finite, unit/side-consistent geometry and material state

    if surface emits toward previous vertex:
      w <- 1 if previous is camera/delta/unreachable-by-light
           else MIS_B(previous.bsdfPdfW,
                      LightPdfW(previous.vertex, surface.emitter, ray.direction))
      radiance += beta * w * surface.emittedRadiance

    lightSample <- None
    if surface has an NEE-eligible non-delta BSDF component:
      lightSample <- SampleOneEligibleLight(surface, named dimensions)
    if lightSample exists, is physically valid, and is visible:
      f      <- EvaluateCompleteContinuousEffectiveBSDF(surface, lightSample.direction)
      pB     <- ComparableBsdfPdfW(surface, lightSample.direction)
      pLight <- lightSample.completeProbability
                // tagged discrete mass for delta; solid-angle density otherwise
      w      <- 1 for a delta-only light, else MIS_L(pLight, pB)
      radiance += beta * f * lightSample.Li * GeometricCosine
                  * visibility * w / pLight

    if product is FinitePathDiagnostic and k + 1 == D:
      return Valid(radiance, counters, events)

    bsdfSample <- SampleCompleteBSDFMixture(surface, named dimensions)
    if bsdfSample has zero physical support:
      return Valid(radiance, counters, events)
    require valid event, direction, value, and probability measure

    if bsdfSample is continuous:
      beta *= bsdfSample.completeContinuousF * GeometricCosine
              / bsdfSample.completePdfW
    else:
      beta *= bsdfSample.completeDeltaWeight
              / bsdfSample.completeEventProbabilityMass
    require finite, non-negative beta under the accepted RGB contract

    previous <- (surface vertex, comparable BSDF pdf, delta flag)

    if product is SurfaceTransportReference and k >= rouletteStart:
      if roulette sample does not survive: return Valid(radiance, counters, events)
      beta /= survivalProbability

    if the implementation safety bound would be exceeded:
      return Failure(SafetyDepthReached, bounded event context)

    ray <- SpawnRobustContinuationRay(MATH-11, bsdfSample.direction)
```

Alpha rejection is traversal semantics, not a black surface event. The accepted any-hit/inline behavior must continue traversal using the same alpha input and sample identity. Every legitimate zero-support termination is distinct from invalid PDF, invalid normal, non-finite value, unsupported event, counter overflow, or endpoint collapse.

## Equation-To-Code Ledger

Stage 0 creates the first accepted revision; Stages 2–6 replace proposed symbols with exact types/functions and retained checks. No row may remain “implicit in shader code.”

| ID | Required implementation correspondence | Minimum falsifier |
| --- | --- | --- |
| `MATH-01` | Camera snapshot, film sample, raster/film mapping, ray generation, crop/filter weight. | Center/edge/corner/subpixel rays and constant-radiance pixel integral. |
| `MATH-02` | Effective BSDF, incident/emitted radiance, accepted side domain, geometric cosine. | Black/unit environment and one-/two-segment hand paths. |
| `MATH-03` | Lobe PMF, conditional samplers, full mixture evaluation/PDF, delta event representation. | Force each lobe and inject omitted/doubled selection mass. |
| `MATH-04` | Light PMF, conditional PDF, measure tag, Jacobian, environment mapping, delta branch. | Hand-computable rectangle, point, directional, emissive triangle, and constant environment. |
| `MATH-05` | NEE contribution and exact connection visibility endpoints. | Unit Lambertian under one light with PDF/Jacobian/visibility fault injection. |
| `MATH-06` | Comparable PDFs and MIS heuristic. | Equal-PDF weight, zero competitor, scale both PDFs, and wrong-measure injection. |
| `MATH-07` | Camera/delta/unweighted and non-delta/MIS-weighted emission/environment hit branches. | NEE on/off means and duplicate/missing-emission injection. |
| `MATH-08` | Roulette eligibility, survival probability, random comparison, compensation, safety failure. | Statistical tail mean plus missing-compensation and hard-cap injections. |
| `MATH-09` | Generator key/counter packing, float conversion, dimension constants, checkpoint identity. | Repeat/prefix/reorder/restart/backend dump and dimension-alias injection. |
| `MATH-10` | Complete-prefix mean/M2/count, batch merge, standard error, overflow, checkpoint. | Adversarial large-mean/small-variance sequence against higher precision. |
| `MATH-11` | Reconstruction/transform error bounds, geometric-normal offset, endpoint interval. | Scale/translation/shear/mirror/grazing/coplanar/thin-gap matrix. |

## Common Mathematical Failure Points

| Symptom | Likely defect | Required first check |
| --- | --- | --- |
| Image is uniformly too bright/dark | Missing/duplicated light or lobe PMF, cosine, unit conversion, or average division. | One-light unit Lambertian hand case; print tagged factors, not only final RGB. |
| Direct light is correct alone but too bright with indirect paths | Unweighted emissive/environment hit double counts NEE. | Compare NEE-only, BSDF-hit-only, and MIS-combined expected means. |
| Small area lights or HDR pixels create extreme fireflies | Wrong area-to-solid-angle Jacobian, missing light PMF, poor but valid sampling, or a near-zero PDF. | Retain factor/PDF/event trace; never begin with a clamp. |
| Rough metals darken in a white furnace | Single-scatter energy loss, mismatched GGX sampling/PDF, or shading-normal loss. | Per-lobe furnace sweep over roughness/view angle and flat-vs-normal-mapped surface. |
| Roughness zero explodes or changes brightness | Continuous GGX epsilon used instead of a delta event, or delta MIS is wrong. | Exact smooth-limit branch and zero/near-zero roughness sequence. |
| Normal maps create black fringes or excess energy | Geometric/shading hemisphere mismatch or unjustified correction. | Geometric-normal control, tangent-basis audit, grazing/terminator fixtures, reciprocal/energy review. |
| Point/directional lights disappear under MIS | Delta light was assigned a finite density or compared to continuous BSDF PDF. | Delta-only analytic light case with weight one. |
| Environment poles or seams brighten | Missing `sin(theta)` Jacobian, wrong direction mapping, address mode, or pole handling. | Constant-map integral and single-texel latitude sweep. |
| Noise pattern changes after pause or batch-size change | Stateful RNG, branch-dependent dimension consumption, or frame-index coupling. | Exact sample dump by `(pixel, ordinal, dimension)` before rendering. |
| Noise falls but mean remains wrong | Biased estimator, correlation, shared oracle defect, or local error hidden by aggregate metric. | Independent randomized replicates, analytic mean, and regional/crop statistics. |
| Variance becomes negative or zero at high means | Catastrophic cancellation or count/merge error. | Large-offset small-variance synthetic stream against double/extended precision. |
| Acne disappears only with a large bias, then thin gaps leak | Fixed epsilon or shading-normal ray offset. | Error-bound derivation plus transform/scale/grazing/thin-gap matrix. |
| NaN pixels look harmlessly black | Invalid-to-zero, `saturate`, or `max` masks the first bad factor. | First-invalid counter/event capture and analytic job failure. |
| CPU, Inline, Pipeline, D3D12, and Vulkan disagree | Different math path, contraction/precision, SBT/payload mismatch, or undefined values. | Same event trace and factor ledger before any beauty-image tolerance. |

## `PTD-00` Ratification Checklist

Before Stage 1, the accepted report must:

1. mark each `MATH-*` row `Accepted`, `Replaced`, or `Excluded`, with no open symbol capable of changing expectation;
2. fill the camera/filter, RGB space, light units, material formulas, shading-normal policy, lobe/light PMFs, MIS heuristic, roulette parameters, sampler, precision, maximum SPP, and safety behavior;
3. attach hand calculations for zero, unit, delta, two-technique, area/Jacobian, environment, roulette, finite-depth, invalid, and accumulation cases;
4. name one code owner and one executable defect-detecting check for every equation row;
5. record independent math and numerical review, including corrections and remaining limitations;
6. reconcile every choice with the feature matrix, release maps, workflow labels, artifact schema, and clean-break ledger;
7. keep `PTD-00 BLOCKED` if a reviewer must infer any probability, measure, unit, boundary case, or invalid-result disposition.

## Primary Sources

1. Eric Veach, [*Robust Monte Carlo Methods for Light Transport Simulation*](https://graphics.stanford.edu/papers/veach_thesis/), 1997 — path-space measures, Monte Carlo estimators, MIS, and non-symmetric scattering foundation.
2. Matt Pharr, Wenzel Jakob, and Greg Humphreys, [*Physically Based Rendering, 4th ed. — A Better Path Tracer*](https://pbr-book.org/4ed/Light_Transport_I_Surface_Reflection/A_Better_Path_Tracer) and [Light Interface](https://pbr-book.org/4ed/Light_Sources/Light_Interface) — NEE/MIS event flow, light PDF contract, emission-hit weighting, and roulette cross-check.
3. Khronos Group, [*glTF 2.0 Specification*](https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html) — source metallic-roughness, texture transfer, alpha, camera, and asset semantics where the accepted Sparkle content originates from glTF.
4. Eric Heitz, [*Sampling the GGX Distribution of Visible Normals*](https://jcgt.org/published/0007/04/01/paper.pdf), JCGT 7(4), 2018 — exact GGX VNDF sampling candidate and PDF review.
5. Eric Veach, [*Non-symmetric Scattering in Light Transport Algorithms*](https://graphics.stanford.edu/papers/non-symmetric/) and Schüssler et al., [*Microfacet-based Normal Mapping for Robust Monte Carlo Path Tracing*](https://cg.ivd.kit.edu/publications/2017/normalmaps/normalmap.pdf), 2017 — shading-normal correctness alternatives and limitations.
6. Carsten Wächter and Nikolaus Binder, [NVIDIA DXR self-intersection analysis and sample](https://developer.nvidia.com/blog/solving-self-intersection-artifacts-in-directx-raytracing/), plus PBRT's [Managing Rounding Error](https://pbr-book.org/4ed/Shapes/Managing_Rounding_Error) — robust spawn and endpoint derivation inputs.
7. John Salmon et al., [*Parallel Random Numbers: As Easy as 1, 2, 3*](https://www.thesalmons.org/john/random123/papers/random123sc11.pdf), SC11 — counter-based parallel sample-generator precedent.
8. Tony Chan, Gene Golub, and Randall LeVeque, [*Algorithms for Computing the Sample Variance: Analysis and Recommendations*](https://doi.org/10.1080/00031305.1983.10483115), 1983 — stable variance and pairwise accumulation rationale.
9. Academy Software Foundation, [OpenEXR Technical Introduction](https://openexr.com/en/latest/TechnicalIntroduction.html) — linear HDR channels, metadata, numeric types, and lossless-versus-lossy container behavior.

The vendor implementation ledger and exact pinned revisions remain in [Research](Research.md). These sources constrain and challenge the design; the accepted derivation, Sparkle code, and defect-detecting evidence remain the only local authority.

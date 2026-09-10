# Reference Path Tracer Transport And Estimator Contract

**Status:** Stage-0-frozen mathematical design candidate; all `MATH-01` through `MATH-11` rows are dispositioned below, but implementation evidence and the report-level `PTD-00` blockers remain open

**Responsibility:** define one notation, estimator, event algorithm, probability-measure contract, PBR material boundary, numerical policy, equation-to-code ledger, and mathematical failure checklist for `SurfaceTransportReference` and `FinitePathDiagnostic`

**Authority boundary:** [Discovery](Discovery.md) owns ratification and implementation authorization, [Execution Architecture](ExecutionArchitecture.md) owns system ownership and lifetime, [User Experience](UserExperience.md) owns the human and automation workflow, the [feature dossier](README.md) owns acceptance, [Research](Research.md) owns precedent, and the [staged plan](Plan.md) owns delivery order and prompts

**Prepared:** re-audited 2026-09-10 against committed `master` revision `669637cf23b9748f8b94635409e74159d31d0bc2`; the current shader route was inspected, but no equation was implemented or executed

**Naming reconciliation:** the 2026-09-09 working-tree clean break makes `ReferencePathTracer` the sole feature name; no mathematical decision or claim is thereby accepted.

**Non-claims:** this specification does not prove that the current or future implementation is unbiased, energy conserving, numerically robust, converged, backend-equivalent, or usable as an oracle

> [!IMPORTANT]
> An implementer may not select a missing constant, BRDF variant, normal treatment, PDF measure, invalid-sample rule, or threshold while writing code. `PTD-00` must first fill every decision slot below, record the reviewing experts, and bind the result to one immutable report revision.

## Claim Boundary

The frozen candidate full product is a unidirectional, camera-originating Monte Carlo estimator of **scene-linear RGB surface radiance** for the accepted opaque reflective domain. It includes camera-visible emission, environment emission, direct-light next-event estimation (NEE), indirect surface reflection, and compensated Russian roulette. It excludes transmission, participating media, physical subsurface transport, spectral transport, and any undeclared approximation.

Two products remain mathematically distinct:

| Product | Target | Ordinary termination | Evidence use |
| --- | --- | --- | --- |
| `SurfaceTransportReference` | The full supported surface-reflection integral over paths admitted by the frozen scene/material/light/camera domain. | Absorption, escape, zero physical throughput, or compensated Russian roulette. A watchdog or safety-depth event fails the sample/session; it is not a zero contribution. | Candidate oracle only after every applicable `AC-RPT-*` criterion passes. |
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
| `H_a` | MIS weight assigned to technique `a`; the frozen rule is the power heuristic with exponent two. |

Implementation types and event records must preserve whether a probability is a discrete mass, an area density, a solid-angle density, or a delta distribution. A bare field named `Pdf` is insufficient at a cross-owner or diagnostic boundary.

## Radiometry, Color, And Units

Raw beauty is scene-linear RGB radiance in one frozen set of primaries and white point. The manifest must name those chromaticities or a stable repository-owned color-space identity. RGB is an explicitly three-channel approximation; no spectral-accuracy claim is permitted.

All light inputs are converted once, before sampling, into the radiometric quantity consumed by their light contract. Directional-light illuminance, point/spot luminous intensity, area/emissive radiance, exposure-like artistic multipliers, and environment texels are not interchangeable values. Stage 0 must provide a per-light dimensional analysis and known-value conversion fixture.

The raw path must not apply exposure, tone mapping, gamut mapping, output transfer functions, denoising, temporal reconstruction, contribution/firefly clamps, or lossy encoding. OpenEXR metadata records the color space and channel semantics; it does not define them.

## Stage-0 Decision Freeze

The following values close formula-changing choices for planning. They describe the target implementation, not the current GBuffer-seeded shaders and not executable proof.

| Slot | Frozen decision |
| --- | --- |
| Products | Retain the two products in [Claim Boundary](#claim-boundary). `SurfaceTransportReference` has no valid deterministic depth/distance termination. `FinitePathDiagnostic(D)` requires `D >= 1`; `D=1` evaluates camera-visible emission and NEE at `x_0` and performs no continuation. A camera miss evaluates the environment for either product without consuming a surface depth. |
| Coordinates/camera/filter | Metres; left-handed world/view with `+X` right, `+Y` up, `+Z` forward; row-vector/row-major transforms; reversed-Z `[0,1]`; perspective pinhole only. For integer pixel `(x,y)` and `xi_x,xi_y in (0,1)`, raster UV is `((x+xi_x)/W,(y+xi_y)/H)`, NDC is `(2u-1,1-2v)`, and the homogeneous near point is `(ndc.x,ndc.y,1,1)`. Multiply row-vector by inverse projection, divide by `w`, transform its direction by inverse view, then normalize; origin is the canonical camera position. Full crop and a normalized box filter are the first product. Zero extent, non-finite/invertibility failure, non-perspective projection, or FOV outside `(0,pi)` or `0<NearZ<FarZ` rejects before sample zero. |
| Texture reconstruction | Base mip only, bilinear filtering with the frozen sampler address mode; no derivative-, cone-, or distance-selected MIP in v0.1 raw transport. The target is therefore the discrete base-level texture reconstruction recorded by the scene manifest, not an unknown continuous texture. |
| Working color/units | Linear sRGB/Rec.709 primaries, D65 white. Distances are metres. Environment/emissive/area values become RGB radiance-equivalent `W sr^-1 m^-2`; point/spot become RGB radiant-intensity-equivalent `W sr^-1`; directional becomes RGB irradiance-equivalent `W m^-2`. For a non-negative linear RGB chromaticity `c`, normalize by `Y=max(dot(c,(0.2126,0.7152,0.0722)),2^-24)` and convert a photometric scalar `q` to `q*c/(683*Y)`; black `c` yields zero. This is an explicit three-basis approximation, not spectral accuracy. |
| Material | glTF-style metallic-roughness: decoded linear base color `b`, `m in [0,1]`, `r in [0,1]`, `F0=lerp(0.04,b,m)`, `alpha=r^2` for `r>0`. Reflection is Lambertian `((1-m)b/pi)` plus isotropic single-scatter GGX reflection with Trowbridge-Reitz `D`, exact Smith masking-shadowing, and Schlick Fresnel; the diffuse term is multiplied componentwise by `(1-F)` for the realized half vector. `r=0` is exact delta reflection. Single-scatter energy loss is part of this named canonical v0.1 material and must be visible in furnace evidence; no compensation is inferred. |
| Normals/sides | `n_g` decides front/back, visibility, emitter side, and offset. Tangent-space normal maps use normalized MikkTSpace tangent/handedness, decode `2texel-1`, multiply decoded XY by authored `normalTexture.scale`, preserve decoded Z, then normalize; reject a non-finite/zero or opposite-`n_g` result. Radiance-mode Veach shading-normal correction is applied once to every accepted smooth or delta BSDF throughput: `C_sn=abs((wi·n_s)(wo·n_g)/((wi·n_g)(wo·n_s)))`; a zero denominator is zero physical support, not epsilon repair. |
| Alpha/two-sided | `MASK` only: base-color alpha times factor, base-mip bilinear decode, accept exactly when `alpha >= cutoff`. Rejected hits continue traversal and are not black events. `OPAQUE` ignores alpha. `BLEND` is unsupported. Two-sided surfaces orient `n_g` toward `omega_o` before the shading frame is built; emission uses the authored one-/two-sided flag. Alpha is deterministic and consumes no random dimension. |
| BSDF strategy | Select one active strategy class by the exact integer-partition categorical rule below: diffuse continuous iff every input is finite and at least one component of `(1-m)b` is strictly `>0`; rough GGX continuous iff `r>0` and at least one `F0` component is strictly `>0`; mirror delta iff `r=0` and at least one `F0` component is strictly `>0`. Invalid/negative input is rejected before class construction. The estimator uses the actual induced class mass, not idealized `1/N_active`. Continuous samples evaluate the complete continuous BSDF and unconditional mixture density. Mirror event numerator is `F*C_sn`; divide by its complete selection/event mass once. |
| Light strategy | One sample from the exact integer-partition PMF over the stable eligible list: each enabled supported analytic light whose converted RGB intensity/irradiance/radiance is finite, non-negative, and has a component `>0`; each accepted non-degenerate emissive triangle with such radiance, irrespective of current visibility; and one environment technique when any base-mip texel has such radiance. Ordering is `(kind, stable scene light/instance/primitive ID)`. Rectangle/triangle points are uniform in area. Point/spot and directional are ideal delta lights only in v0.1; any positive source radius/angular radius is unsupported rather than reinterpreted. The environment direction is uniform over the sphere with exact `p=1/(4pi)`; texture-importance sampling is deferred to avoid a second discrete-probability contract. A competing event includes emitter identity and, for finite emitters, its sampled endpoint; sum `pi_l p_l` only across duplicate techniques registered for that same event (normally exactly one), never unrelated collinear emitters. |
| MIS | Power heuristic, exponent `2`, one light and one BSDF sample. Only comparable continuous solid-angle densities enter. Delta events and an unavailable competitor receive weight one. |
| Roulette | After the third completed surface scattering event, compute `s*=clamp(max(beta),0.05,0.95)`, `T=clamp(roundTiesToEven(s* * 2^24),1,2^24-1)`, and the exactly representable `q=T*2^-24`. For `J=word>>8`, survive iff `J<T`, then divide `beta` by exactly `q`. Thus survival mass and compensation are identical. Safety-depth reach, non-finite state, overflow, or invalid probability fails the sample and session; it is never averaged as black. |
| Sampler | Philox4x32-10 with unsigned 32-bit `SessionSeed` and `ReplicateId` as the two key words. `DimensionBlock=floor(DimensionId/4)` and the four counter words are `((PixelY<<14)\|PixelX, SampleOrdinal, DimensionBlock, 0x52505431)`, requiring each coordinate `<16384`, ordinal `<1048576`, `DimensionId<=32775`, and unsigned checked arithmetic; lane is `DimensionId mod 4`. Convert one word to the open interval with `((word>>8)+0.5)*2^-24`. For `N` categorical outcomes and raw uint32 `R`, choose `i=floor((uint64(R)*N)/2^32)` and use its exact induced mass `(floor(((i+1)*2^32-1)/N)-ceil(i*2^32/N)+1)/2^32`; never substitute `1/N`. Integer words/checkpoints are canonical little-endian. Any bound violation fails preflight/session. |
| Dimensions | `0,1` film XY; `2..7` reserved for excluded lens/time/filter evolution. Per surface depth `k`, base `8+8k`: `+0` light choice, `+1,+2` light shape/environment, `+3` lobe choice, `+4,+5` BSDF direction, `+6` roulette, `+7` reserved. Branches never renumber later dimensions; deterministic alpha consumes none. `k` is uint32 and must be `<4096`; attempting surface vertex 4097 fails `SafetyDepthReached` before requesting dimension `32776`. |
| Accumulation | Per pixel: RGB binary64 mean, componentwise binary64 `M2`, exact uint64 count; cross-channel covariance is excluded. Complete sample ranges merge in ascending ordinal order with the stated pairwise recurrence. Maximum requested/committed SPP is `1,048,576`; count overflow is a hard failure. Viewport display may derive binary32 pixels from a committed binary64 prefix but cannot feed it back. |
| Arithmetic | Semantic path state, BSDF/light/PDF/MIS/roulette, endpoint bounds, and accumulation use IEEE-754 binary64, round-to-nearest ties-to-even, with contraction/fast-math/reassociation disabled and subnormals preserved; a backend/compiler unable to prove these controls is unsupported. RHI traversal remains binary32. Surface origins are cast then `nextafter`-rounded once more in the signed geometric-normal offset direction. An infinite ray uses `df=RN32(w)` without renormalization, `TMin=0`, `TMax=+Inf`. A finite connection casts the independently rounded endpoints to outward binary32 `p0f,p1f`, computes binary32 `df=p1f-p0f`, and traverses `p0f+t*df` over `TMin=0`, `TMax=nextafter32(1,-Inf)`; semantic BSDF/Jacobian factors still use original binary64 `(x,y,omega)`. Zero/non-finite `df` is invalid. Integer products use explicit uint64 promotion and checked overflow. Comparisons occur in binary64 except categorical/roulette integer thresholds. |
| Checkpoint/artifact | Checkpoint is little-endian schema `rpt-checkpoint-1`: fixed 8-byte magic `SPKRPT01`; uint32 schemaBytes, width, height, samplerVersion, dimensionVersion; uint64 committedBegin, committedEnd; 32-byte session digest; then tightly packed row-major binary64 `mean.R/G/B`, `M2.R/G/B`, and uint64 count planes. The 32-byte header hash field is zero while hashing the complete header and stores that SHA-256 afterward; each plane has uint64 byte size plus SHA-256 in a trailing directory. No padding or native structs are serialized. Raw OpenEXR is scanline, lossless ZIP, round-to-nearest-ties-even binary64-to-FLOAT. `beauty.exr` has `R,G,B`; `aov.exr` has `albedo.R/G/B`, world shading `normal.X/Y/Z`, `depth.Z` in metres (`0` on a miss with `hit=0`), `emission.R/G/B`, `direct.R/G/B`, `indirect.R/G/B`, and `hit`; all are FLOAT. Non-finite conversion fails publication. Counters/counts are separate canonical JSON decimal uint64 values. `manifest.json` uses RFC 8785 JSON Canonicalization Scheme, UTF-8 without BOM/newline, and is written last; every non-manifest file carries size/SHA-256, while the manifest digest is published in the terminal record rather than self-embedded. |

### Equation dispositions

| Row | Stage-0 disposition | Owner | Primary falsifier |
| --- | --- | --- | --- |
| `MATH-01` | Accepted with the camera/filter decision above. | Renderer View/camera-ray owner | `CHK-PTD-03`, then `CHK-RPT-03` |
| `MATH-02` | Accepted for the bounded reflective surface domain. | Renderer integrator owner | `CHK-PTD-03`, then `CHK-RPT-05` |
| `MATH-03` | Replaced by the exact integer-partition active-class selection and material/delta rules above. | Renderer BSDF owner | `CHK-PTD-03`, then `CHK-RPT-05` |
| `MATH-04` | Accepted with exact integer-partition eligible-light PMF and uniform-solid-angle environment sampling. | Renderer light-sampling owner | `CHK-PTD-03`, then `CHK-RPT-05` |
| `MATH-05` | Accepted; environment connections use an infinite directional visibility ray, finite lights use both robust endpoints. | Renderer integrator owner | `CHK-PTD-03`, then `CHK-RPT-05` |
| `MATH-06` | Accepted with power exponent two and one sample per eligible technique. | Renderer integrator owner | `CHK-PTD-03`, then `CHK-RPT-05` |
| `MATH-07` | Accepted with summed complete competing-light density and frozen emitter sidedness. | Renderer integrator owner | `CHK-PTD-03`, then `CHK-RPT-05` |
| `MATH-08` | Accepted with the exact third-scatter predicate and bounds above. | Renderer integrator owner | `CHK-PTD-03`, then `CHK-RPT-05` |
| `MATH-09` | Replaced by the Philox packing and dimension ledger above. | Renderer sampling owner | `CHK-PTD-06`, then `CHK-RPT-07` |
| `MATH-10` | Accepted with binary64 state, uint64 count, fixed merge order, and maximum SPP above. | Renderer accumulation owner | `CHK-PTD-07`, then `CHK-RPT-09` |
| `MATH-11` | Accepted with the exact baseline below; backend/compiler validation remains executable Stage-5/8 evidence. | Renderer geometry plus RHI traversal owners | `CHK-PTD-08`, then `CHK-RPT-08` |

## Mathematical Contract

### `MATH-01` — Pixel Measurement

For pixel footprint `A_p`, normalized reconstruction filter `W_p`, and camera sample `u ~ q_p(u)`, the target pixel value is

```text
P_p = integral[A_p] W_p(u) L_o(r(u)) du / integral[A_p] W_p(u) du

X_p(u) = W_p(u) L_o(r(u))
         / (q_p(u) integral[A_p] W_p(v) dv)
```

The arithmetic mean of every completely evaluated committed `X_p` sample estimates `P_p`. An invalid sample fails the session and is never silently omitted. The frozen first slice is the pinhole camera, normalized box filter, raster mapping, projection/handedness, base-mip texture policy, and rejection behavior in [Stage-0 Decision Freeze](#stage-0-decision-freeze). A camera ray generated from jittered NDC without this correspondence is not accepted.

For a progressive viewport session, every sample in one prefix uses the same canonical unjittered View camera/extent/filter identity. The film sample `u` supplies reference subpixel jitter; ordinary real-time temporal jitter does not. Primary visibility uses the camera clip interval: for unit world ray `w` and unit camera forward `f`, require finite `z=dot(w,f)>0`, set `TMin=NearZ/z` and `TMax=FarZ/z`, and reject a collapsed/non-finite interval. Any effective camera or image-domain change defines a different `P_p` and must invalidate before contributions mix. Switching to Lit changes scheduling/presentation, not `P_p`; a retained prefix may resume only after exact revalidation of every measurement and transport input.

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

`W_delta(e)` is the summed analytic throughput numerator of every compatible accepted delta component, including its Fresnel and transport factors under the frozen convention; each conditional deterministic delta choice normally has `P_j(e) = 1`. The exact smooth and delta branches are fixed immediately below. Lobe-selection mass must not be multiplied twice or omitted. Exact delta reflection is not approximated by clamping roughness to a small continuous GGX value. Delta probability mass and continuous solid-angle density remain different measures and never enter one numeric sum.

For the frozen material, all dot products in the formulas below use the valid oriented shading frame; geometric-side validity and the shading-normal correction remain outside these local terms. Let `mu_i=abs(n_s·omega_i)`, `mu_o=abs(n_s·omega_o)`, `h=normalize(omega_i+omega_o)`, `a=roughness^2`, and

```text
D_GGX(h) = a^2 / (pi ( (n_s·h)^2 (a^2 - 1) + 1 )^2)
G1(w)    = 2 / (1 + sqrt(1 + a^2 tan^2(theta_w)))
G2       = G1(omega_i) G1(omega_o)
F        = F0 + (1-F0) (1-abs(omega_i·h))^5

f_spec   = F D_GGX G2 / (4 mu_i mu_o)
f_diff   = (1-F) (1-metallic) baseColor / pi
```

All terms require strictly positive finite denominators; otherwise that strategy has zero support. The conditional diffuse sampler maps `(u1,u2)` to local `(sqrt(u1)cos(2pi u2),sqrt(u1)sin(2pi u2),sqrt(1-u1))` with `p_diff=mu_i/pi`. The conditional rough sampler is the exact Heitz 2018 isotropic GGX visible-normal algorithm using `u1,u2` without remapping or rejection, with `p_h=D_GGX(h) G1(omega_o) abs(omega_o·h)/mu_o` and reflection-direction density `p_spec=p_h/(4 abs(omega_o·h))`; the cited algorithm's stretch, orthonormal-basis, disk, warp, unstretch, and normalization operations are part of shader identity and receive a line-for-line CPU oracle before Stage 3. For `roughness=0`, `omega_i=reflect(-omega_o,n_s)`, `W_delta=F*C_sn`, and the complete event mass is the active mirror selection mass. These are the formulas the implementation and per-event log must expose; a real-time helper with a different `G`, roughness floor, clamp, or selected-component-only PDF is not equivalent.

### `MATH-04` — Light Selection And Measure Conversion

The complete density of a continuous light sample is

```text
p_L(event) = sum[l able to generate the same event] pi_l(x) p_l(event | x)
sum[l eligible at x] pi_l(x) = 1
```

If light `l` samples point `y` with area density `p_l^A(y)`, then

```text
p_l^omega(omega_i | x)
  = p_l^A(y) ||y - x||^2 / abs(dot(n_g(y), -omega_i))
```

The event contains direction, emitter identity, and a finite emitter endpoint when applicable. Consequently unrelated emitters that happen to be collinear do not enter one another's MIS density. The conversion is invalid at zero distance, zero projected emitter cosine, a degenerate rectangle/triangle, a non-finite term, or a side rejected by the light contract. It is not repaired with an epsilon. Point and ideal directional lights are delta-direction techniques and use their explicitly derived discrete estimator branch.

For the frozen Y-up latitude-longitude environment, `phi=2 pi u`, `theta=pi v`, and `omega(u,v)=(sin(theta)cos(phi), cos(theta), sin(theta)sin(phi))`. The seam is the `+X` half-plane with `u=0` and increasing `u` rotates from `+X` toward `+Z`; `u` wraps and `v` clamps. Evaluation maps `theta=acos(clamp(omega.y,-1,1))` and `u=fract(atan2(omega.z,omega.x)/(2pi))`, with the seam assigned to `u=0`. Thus

```text
d omega = 2 pi^2 sin(theta) du dv
p_env^omega = p_env^uv(u,v) / (2 pi^2 sin(theta))
```

The v0.1 sampler uses dimensions `+1,+2` directly: `phi=2pi u1`, `cos(theta)=1-2u2`, and `p_env^omega=1/(4pi)`. The environment is eligible iff a deterministic row-major binary64 scan of decoded base-mip texels finds at least one finite non-negative component `>0`; otherwise it is omitted. Raw radiance evaluation uses base-mip bilinear `u` wrap and `v` clamp. Texture importance sampling is an optimization requiring a new exact fixed-point selection/remapping/PDF review and is not part of this product.

The v0.1 native-light meanings are fixed as follows. An ideal point requires strictly positive finite distance and uses `Li=I/r^2` along its delta direction. An ideal directional light uses its converted normal-incidence irradiance as the delta-event numerator. An ideal spot is the point rule multiplied by `smoothstep(cosOuter,cosInner,dot(axis,-omega_i))`; when the cone cosines are equal the factor is `1` exactly when the dot product is greater than or equal to that boundary and `0` otherwise. Point/spot source radius and directional angular radius must be exactly zero; positive values are `UnsupportedDomain` before sample zero. A rectangle and emissive triangle emit constant one-sided radiance unless explicitly two-sided and sample `(u1,u2)` uniformly: rectangle affine coordinates use the two uniforms directly, while a triangle uses `s=sqrt(u1)`, barycentrics `(1-s,s*(1-u2),s*u2)`. Every accepted non-degenerate emissive triangle with nonzero radiance is registered once by stable instance/primitive ID, regardless of current visibility. Finite artistic range/attenuation coefficients are not physical reference semantics: a non-default finite-range request is unsupported rather than silently truncated. The event-specific competing PDF follows the equation above; unrelated collinear emitters are distinct events.

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

For `n_a` samples from each comparable continuous technique and the frozen exponent `gamma = 2`,

```text
H_a = (n_a p_a)^gamma / sum[t] (n_t p_t)^gamma
```

The first implementation uses one BSDF and one light sample where eligible, so `n_B = n_L = 1`. Both PDFs must describe the same generated direction, event, and solid-angle measure. A zero competitor PDF gives weight one to the supported technique. Delta events bypass continuous-density MIS. Changing the heuristic or exponent reopens this contract before implementation.

### `MATH-07` — Emissive And Environment Hits

When a BSDF continuation reaches emissive geometry or escapes to the environment,

```text
C_hit,k = beta_k L_e H_B(p_B, p_L)
```

Use `H_B = 1` for the camera ray, after a delta BSDF event, or whenever no eligible light-sampling technique could generate the same event. Otherwise compute complete event density `p_L=sum_l pi_l p_l(event)` at the **previous** vertex only over techniques able to generate the same emitter identity and endpoint/direction event and apply `MATH-06`. This branch is the BSDF half of direct-light MIS; adding unweighted emission after NEE double counts light.

### `MATH-08` — Compensated Russian Roulette

After updating throughput for an accepted continuation and after the frozen minimum depth, choose survival probability

```text
s*_k = clamp(g(beta_(k+1), path state), 0.05, 0.95)
T_k  = clamp(roundTiesToEven(s*_k 2^24), 1, 2^24-1)
q_k  = T_k 2^-24
```

The frozen rule is `g(beta) = max(beta.r, beta.g, beta.b)` for the RGB reflective-only domain. Let `J=word>>8`; if `J>=T_k`, terminate with no later contribution; on survival,

```text
beta_(k+1) <- beta_(k+1) / q_k
```

The decision freeze fixes eligibility after the third completed scattering event, bounds `0.05/0.95`, the integer comparison, exact induced mass `q`, and `g=max(beta)` after the accepted continuation update. The upper bound below one guarantees an eventual stochastic termination opportunity even on unit-throughput paths. Transmission would require an explicit refractive scaling analysis and is excluded from this formula. The safety ceiling is 4096 surface vertices; reaching it invalidates the affected sample/session and cannot return black as though the full target were evaluated.

### `MATH-09` — Stateless Sample Identity

Every random value is a pure function

```text
U = ToUnitFloat(CounterGenerator(
      key     = (SessionSeed, ReplicateId),
      counter = (PixelX, PixelY, SampleOrdinal, DimensionId)))
```

The exact generator, rounds, integer packing, endianness, overflow policy, and open/closed interval conversion are frozen and recorded. Conceptual dimensions are assigned by name and stable offset, including film, lens/time if admitted, per-depth light selection, light shape, lobe selection, BSDF direction, alpha decision if stochastic behavior is accepted, and roulette. Branches do not shift later dimensions. Renderer frame index, view-mode switches, batch size, and wall-clock timing never enter this function. A layout or generator change invalidates the active prefix and checkpoints rather than adding a legacy stream reader.

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

For a reconstructed world-space point `p` with conservative absolute error vector `Delta_p` and oriented unit geometric normal `n_g`, the origin bound is

```text
d     = dot(abs(n_g), Delta_p)
sigma = -1 if dot(w, n_g) < 0, otherwise +1
po    = p + sigma d n_g
```

Each nonzero component of `po - p` is then rounded with `nextafter` toward `+infinity` or `-infinity` according to the signed offset. For binary32 triangle vertices and arithmetic, use `c0=2^-24`, `c1=3*2^-24`, and `c2=2*2^-24`. With object-space barycentric reconstruction arranged as `v0 + b.x*edge1 + b.y*edge2`, `extent=max(abs(edge1)+abs(edge2)+abs(abs(edge1)-abs(edge2)))` componentwise and `Delta_obj=c0*abs(v0)+c1*extent`. Propagate absolute error through the row-vector object/world transforms using absolute matrix coefficients; add `c2*abs(translation)` for each object-to-world multiply and `c2*abs(linearTransform*p)` for the world-to-object traversal transform. Transform the geometric normal by inverse transpose, normalize it, and add the projected object and world bounds before directed rounding.

An infinite continuation/environment ray uses `TMin=0` from the source rounded along its outgoing direction and the binary32 adapter rule above. A finite connection first forms physical `omega=normalize(y-x)` from unoffset reconstructed receiver `x` to unoffset emitter point `y`; BSDF evaluation, cosines, distance-squared/Jacobian, and emitted radiance use this original binary64 `(x,y,omega)`. It rounds source `p0` from `x` along `omega` and light endpoint `p1` from `y` along `-omega`, converts them outward to binary32 `p0f,p1f`, computes unnormalized binary32 `df=p1f-p0f`, and traces `p0f+t*df` on `[0,nextafter32(1,-Inf)]`. No normalized traversal direction or offset distance may replace the semantic radiometric inputs. A zero/non-finite direction or collapsed interval is invalid. Primary rays use the camera interval in `MATH-01`, converted outward to the binary32 adapter interval. The constants and operation ordering are part of shader/compiler identity. Stage 5 must re-derive and numerically validate their conservatism for Sparkle's actual triangle formats, fused-operation settings, row-vector packing, Inline/RGS implementations, and D3D12/Vulkan compilers; a failing route is excluded rather than tuned per scene.

## PBR Material Contract

The path tracer judges lighting only if the material being integrated is itself explicit. The decision table above freezes every row below; implementation must provide evaluation, sampling, PDF, limit, and energy evidence where applicable.

| Surface | Required decision before implementation | Non-negotiable failure boundary |
| --- | --- | --- |
| Base color | glTF base-color texture RGB is decoded from sRGB to linear, multiplied componentwise by linear `baseColorFactor.rgb`, then by linear vertex color RGB when present; negative/non-finite inputs are unsupported, values above one remain HDR. Coverage alpha is linear texture alpha times factor alpha times vertex color alpha when present. Declared `KHR_texture_transform` applies scale, then rotation about the origin, then offset (`T*R*S` in the extension's column-vector notation) to the selected UV set. Missing sampler uses glTF `REPEAT`; declared `REPEAT`, `CLAMP_TO_EDGE`, or `MIRRORED_REPEAT` is honored with base-level bilinear reconstruction. | Sampling stored sRGB as linear or applying display conversion inside transport fails. |
| Metallic | Clamp is forbidden: finite `metallicFactor` and linear metallic-roughness texture blue channel must each already be in `[0,1]`; multiply once to form `m`. The same texture's green channel times finite `roughnessFactor`, each already `[0,1]`, forms `r`; both use the material texture UV transform/address/base-mip bilinear rule. | Metallic becomes only a lobe-selection probability or is multiplied twice. |
| Dielectric F0 | Fixed RGB `0.04`, corresponding to scalar IOR `1.5`; material-authored IOR/specular extensions are excluded. | Another F0 or current helper is assumed without the scene contract. |
| Diffuse | Lambertian `(1-F)(1-m)b/pi` using realized half-vector Schlick `F`, as frozen above. | Real-time empirical diffuse is silently called physically exact; sample/eval/PDF disagree. |
| Rough specular | Isotropic Trowbridge-Reitz GGX, separable exact Smith `G1_i*G1_o`, Schlick Fresnel, `alpha=roughness^2`, and the exact cited visible-normal sampler above. | Fast correlated approximations, roughness floors, or invalid-value clamps enter without a bounded claim. |
| Smooth limit | Exact delta reflection at the accepted zero-roughness boundary. | A small arbitrary roughness changes the target while retaining the same label. |
| Multiple scattering | Single-scatter GGX energy loss is accepted as part of the named v0.1 target and is measured, not hidden, in furnace evidence. | White-furnace energy loss is hidden by exposure, clamp, or scene tuning. |
| Normal map | MikkTSpace tangent and vertex handedness; sRGB-disabled RGB decode, scale decoded XY only, preserve decoded Z, normalize, require the geometric hemisphere, apply `C_sn` once; no shadow-terminator modification. | Shading normal controls ray origin/sidedness, creates energy, or yields a different forward/backward contract without disclosure. |
| Alpha mask | Linear base-color texture alpha times factor alpha times vertex color alpha when present, with the same UV transform/address/base-mip bilinear policy; accept iff `alpha>=cutoff`. It is deterministic and consumes no sample dimension. | Raster alpha-to-coverage or a different any-hit rule changes visibility silently. |
| Two-sided | Orient geometric normal toward `omega_o`, then rebuild tangent frame and shading normal; authored emitter sidedness remains a separate flag. | Flipping only the shading normal makes visibility and scattering disagree. |
| Emission | Decode emissive texture RGB from sRGB to linear, multiply linear emissive factor and admitted emissive-strength once, interpret the result directly as RGB radiance-equivalent `W sr^-1 m^-2`, apply declared side, and register each nonzero triangle exactly once. Negative/non-finite values are unsupported. | A visible emissive surface is not sampled, is sampled twice, or uses arbitrary display color as radiance. |
| Ambient occlusion | Excluded from raw physical transport; it may remain a separately labeled real-time approximation subject. | Baked AO attenuates path-traced indirect light or the oracle shares the approximation it should judge. |
| Subsurface/transmission/media | Excluded by default under `RPT-FS-18` unless discovery expands the transport equation, state, and evidence. | A real-time appearance lobe is evaluated as if it were physical transport. |

The frozen reflective baseline is the diffuse plus isotropic single-scatter GGX reflection mixture, visible-normal sampling, delta limit, shading-normal correction, and matched full-mixture PDF specified above. Compatibility with the current Sparkle BRDF is evidence input; it is not permission to inherit its `saturate`, `max`, fast approximation, or sampling assumptions.

## Reference Algorithm

The semantic core must remain recognizable as this sequence. An optimization may reorder work only after proving identical event, probability, contribution, sample-identity, and diagnostic semantics.

```text
TraceSample(session, pixel, sampleOrdinal):
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
      beta /= exactInducedSurvivalMassQ

    if the implementation safety bound would be exceeded:
      return Failure(SafetyDepthReached, bounded event context)

    ray <- SpawnRobustContinuationRay(MATH-11, bsdfSample.direction)
```

Alpha rejection is traversal semantics, not a black surface event. The accepted any-hit/inline behavior must continue traversal using the same alpha input and sample identity. Every legitimate zero-support termination is distinct from invalid PDF, invalid normal, non-finite value, unsupported event, counter overflow, or endpoint collapse.

## Retained Hand Cases

All RGB arithmetic below is componentwise. These paper oracles are exact under the frozen model and precede beauty-image evidence.

| Case | Construction | Required result and defect exposed |
| --- | --- | --- |
| Zero | No emitters and black environment; any valid reflective path. | `L=0` exactly. A nonzero value exposes uninitialized energy or duplicate state. |
| Isolated Lambert term | Standalone `f=1/pi` evaluator (not a realizable full glTF material), unit constant environment, no other emitters. Since `integral_H (1/pi) cos(theta) d omega=1`, the outgoing RGB is `(1,1,1)`. | Exposes missing cosine, `pi`, PDF, or mean division without pretending dielectric Fresnel is absent from the product material. |
| Camera center | Odd `W,H`, center pixel, `xi=(0.5,0.5)`, symmetric full crop. | NDC is `(0,0)` and the ray is canonical `+Z` transformed by inverse view. Corners use the exact half-open formula and never address `u=1`. |
| Delta mirror | Realizable white metallic, zero-roughness material (`b=1,m=1,r=0`), flat normal, unit environment. The mirror is the only active class. | `F=1`, `C_sn=1`, class mass `1`, and `beta'=1`. No continuous PDF or MIS comparison is evaluated. A separate isolated categorical oracle supplies two synthetic delta components and verifies division by each exact induced class mass. |
| Area Jacobian | Receiver at origin; four-square-metre emitter point at distance `2 m`, facing receiver; uniform area PDF `1/4 m^-2`. | `p^omega=(1/4)*4/1=1 sr^-1`. Omitting distance squared yields `1/4`; reversing emitter cosine rejects the sample. |
| Light units | Neutral `c=(1,1,1)` has `Y=1`: `683 cd` point/spot becomes `I=(1,1,1) W sr^-1` and at `r=2 m` gives `Li=(0.25,0.25,0.25)` in the delta numerator; `683 lux` directional becomes `(1,1,1) W m^-2`; `683 cd m^-2` rectangle/emissive/environment input becomes `Le=(1,1,1) W sr^-1 m^-2`. A hard-edge spot on/inside its boundary retains the point value and outside gives zero. | Exposes a missing `683`, luminance normalization, inverse square, cone boundary, or interchange of intensity, irradiance, and radiance. |
| Environment Jacobian | Uniform sphere via `phi=2pi u1`, `cos(theta)=1-2u2`; the induced lat-long density is `p_uv=(pi/2) sin(theta)`. | It integrates to one and gives `p^omega=1/(4pi)` away from poles; sampling uses the direct cosine mapping at the measure-zero pole limit and never divides by `sin(theta)`. |
| Equal-PDF MIS | `p_L=p_B=1/4 sr^-1`, one sample each. | `H_L=H_B=1/2`; multiplying both PDFs by the same positive constant leaves weights unchanged. If a competitor is zero, supported weight is one. |
| Emission hit | Realized non-delta hit with `p_B=1/4`, complete `p_L=1/2`. | BSDF-hit weight is `(1/4)^2/((1/4)^2+(1/2)^2)=1/5`; light weight is `4/5`. Camera/delta/unreachable hit instead has weight one. Adding an unweighted hit is detectable double counting. |
| Roulette | `beta=(0.2,0.1,0.05)` after eligibility gives target `s*=0.2`, integer threshold `T=3355443`, and exact `q=3355443/16777216`. | Exactly `T` of the `2^24` values survive and divide by `q`; expected post-test beta equals pre-test beta exactly in real arithmetic. Dividing by stored float `0.2` fails the hand case. |
| Finite depth | `FinitePathDiagnostic(1)` camera hits an emitting diffuse surface. | Add visible emission and eligible NEE at `x_0`, then stop before BSDF continuation. A camera miss returns environment. Neither output may carry the full-product label. |
| Invalid | A nonzero BSDF numerator with zero/non-finite PDF, invalid normal, endpoint collapse, NaN/Inf radiance, sampler/count overflow, or safety-depth reach. | Increment the exact first-invalid counter, retain bounded event context, fail the sample and session, and commit neither black nor a shortened prefix. |
| Variance | Scalar samples `1,3` (applied per RGB channel). | `mean=2`, `M2=2`, sample variance `2`, standard error `1`. Merging singleton batches produces the same values; cross-channel covariance is not reported. |

The numeric review additionally uses `(10^12-1,10^12+1)` in binary64 and rejects a representation that produces zero variance. The statistical review injects `+1%` mean bias into one predeclared region and duplicates every other sample; the former must fail the regional confidence/relative-error rule and the latter the stream-correlation/duplicate check.

## Equation-To-Code Ledger

`PTD-00-R0` freezes the candidate correspondence; Stages 2–6 bind the symbols to exact types/functions and retained checks after authorization. No row may remain “implicit in shader code.”

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
| NaN pixels look harmlessly black | Invalid-to-zero, `saturate`, or `max` masks the first bad factor. | First-invalid counter/event capture and analytic session failure. |
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
7. Khronos Group, [glTF 2.0 specification](https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html) and ratified [`KHR_texture_transform`](https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_texture_transform#overview) — base-color/vertex RGBA, metallic-roughness channels/factors, sampler defaults, and scale-rotate-offset UV semantics.
7. John Salmon et al., [*Parallel Random Numbers: As Easy as 1, 2, 3*](https://www.thesalmons.org/john/random123/papers/random123sc11.pdf), SC11 — counter-based parallel sample-generator precedent.
8. Tony Chan, Gene Golub, and Randall LeVeque, [*Algorithms for Computing the Sample Variance: Analysis and Recommendations*](https://doi.org/10.1080/00031305.1983.10483115), 1983 — stable variance and pairwise accumulation rationale.
9. Academy Software Foundation, [OpenEXR Technical Introduction](https://openexr.com/en/latest/TechnicalIntroduction.html) — linear HDR channels, metadata, numeric types, and lossless-versus-lossy container behavior.

The vendor implementation ledger and exact pinned revisions remain in [Research](Research.md). These sources constrain and challenge the design; the accepted derivation, Sparkle code, and defect-detecting evidence remain the only local authority.

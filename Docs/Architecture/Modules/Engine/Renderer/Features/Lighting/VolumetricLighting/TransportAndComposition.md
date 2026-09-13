# Volumetric Lighting Transport And Composition Contract

**Status:** target mathematical contract with `VOL-D0` parameters; no implementation or validation claim

**Responsibility:** own medium units, radiative transfer, phase, overlap, direct volume light, froxel integration, temporal signal, atmosphere, heterogeneous tracking, reservoir, composition, and numerical rules

**Authority boundary:** `VOL-D0` ratifies unresolved choices; [Execution Architecture](ExecutionArchitecture.md) owns system shape; [Plan](Plan.md) owns order; code/tests conform; a future assigned FCR owns results

**Convention:** camera rays parameterize distance `s` in metres; radiance is scene-linear; volume results are premultiplied in-scattering plus scalar or RGB transmittance as frozen by discovery

## Claim Boundary

This page defines participating-media transport, atmosphere, volume resampling, and scene composition. It does not define surface BRDFs, screen-space Direct/Indirect reservoirs, transparent refractive nesting, or cloud authoring. Target formulas are normative after `VOL-D0` ratification; exact RGB/spectral approximations, precision, and discretization remain discovery outputs.

## Medium State And Units

At world point `x` and wavelength/RGB basis:

```text
sigma_a(x) >= 0                 absorption coefficient [m^-1]
sigma_s(x) >= 0                 scattering coefficient [m^-1]
sigma_t(x) = sigma_a + sigma_s  extinction coefficient [m^-1]
albedo(x) = sigma_s / sigma_t   when sigma_t > 0
Le_volume(x,w) >= 0             emitted radiance density under frozen convention
```

All fields must be finite. Vacuum has `sigma_t=0`, transmittance one, and no scattering/emission. Author-facing “density” multiplies a typed base coefficient or resolves into coefficients once; different owners cannot apply it independently.

RGB coefficients are a three-basis approximation, not spectral transport. `VOL-D0-02` freezes whether transmittance is RGB or scalar for each tier and how photometric direct lights enter the radiometric-equivalent working basis.

## Radiative Transfer

Along ray direction `w`, radiance changes as

```text
dL(x,w)/ds = -sigma_t(x) * L(x,w)
             + sigma_s(x) * integral_sphere p(x, wi -> w) L(x,wi) dwi
             + Le_volume(x,w).
```

Transmittance from `a` to `b` is

```text
T(a,b) = exp(-integral_a^b sigma_t(x(s)) ds).
```

The camera-ray solution to distance `d` is

```text
Lcamera = T(0,d) * Lsurface_or_background
          + integral_0^d T(0,s) * [sigma_s(s) * Lin_scattered(s) + Le_volume(s)] ds.
```

This directly defines the composition contract. A post-process fog color mixed by arbitrary depth is not equivalent unless derived as the analytic homogeneous special case.

## Analytic Reference Cases

### Beer Slab

For constant extinction over distance `d`:

```text
T = exp(-sigma_t * d).
```

Tests cover vacuum, known optical depths, doubled distance/coefficient, very small depth, and the finite opaque limit.

### Homogeneous Single Scattering

For constant source term `S = sigma_s * Lin + Le_volume` and constant `sigma_t > 0`:

```text
Lscatter = S * (1 - exp(-sigma_t*d)) / sigma_t.
```

The `sigma_t -> 0` limit is handled analytically, not by unstable division. Directional-light tests include light-path transmittance and phase evaluation separately.

### Exponential Height Density

A proposed profile is

```text
rho(h) = rho0 * exp(-falloff * (h - h0))
```

inside the admitted world/planet domain. `VOL-D0-03` freezes clamping/bounds and analytic or high-precision quadrature references. Transforming world scale without transforming inverse-metre coefficients must predictably change optical depth; content scaling rules are explicit.

## Phase Functions

A phase function obeys

```text
p(wi -> wo) >= 0
integral_sphere p(wi -> wo) dwi = 1.
```

Isotropic phase is `1/(4*pi)`. The proposed anisotropic baseline is Henyey–Greenstein:

```text
p_HG(cosTheta,g) = (1-g^2) /
                   [4*pi*(1+g^2-2*g*cosTheta)^(3/2)].
```

`g` is bounded away from `±1` by a ratified numerical limit. Direction convention and sign are tested with forward/backward peaks; common literature conventions differ. Atmosphere Mie may use an accepted fitted/dual-lobe approximation, but its normalization and energy meaning must be independently checked. Rayleigh angular scattering is owned by the atmosphere semantic contract, not approximated by arbitrary HG without a decision.

## Medium Composition

Global, height, and local media resolve at `x` into one coefficient state before lighting/integration. Proposed physical overlap adds coefficients and emission:

```text
sigma_a_total = sum_i sigma_a_i
sigma_s_total = sum_i sigma_s_i
Le_total      = sum_i Le_i
```

If authored priority/override/blend modes are admitted, they are explicit closed semantics with deterministic ordering, not object iteration order. Phase mixtures are weighted by scattering coefficient:

```text
p_total = sum_i sigma_s_i * p_i / sigma_s_total.
```

Zero `sigma_s_total` has no phase query. Capacity overflow rejects publication or follows an explicit culling rule with visible degraded state; it never drops arbitrary volumes.

## Direct Volume Lighting

At scattering point `x`, single-scattered incident radiance from sampled light `y` contributes

```text
S_light(x) = sigma_s(x) * p(w_light -> w_camera)
             * Le(y -> x) * G_or_delta(y,x)
             * V_geometry(y,x) * T_medium(y,x) / p_sample(y|x).
```

Light selection PMF and shape/directional PDF use the same light semantics as Direct Lighting. Geometry visibility and medium transmittance are distinct multiplicative terms. A surface screen-space reservoir cannot be read at a froxel because its receiver domain/target/history differ. Shared immutable light distributions are permitted.

For deterministic small-light froxels, sum every admitted light. For stochastic or reservoir selection, record exact source probability, target, effective sample count, light generation, and correction. The result is evaluated in the medium, not copied from a surface BRDF lobe.

## Froxel Representation

A froxel maps integer `(i,j,k)` and subcell sample to a View-space frustum volume. `VOL-D0-06` freezes exact reconstruction. The Z mapping should concentrate resolution near the camera while reaching the declared far atmosphere; its inverse and boundary tests are mandatory.

Each cell stores the minimum semantic state required by the selected pipeline, for example:

```text
resolved sigma_t / sigma_s / emission / phase parameters
direct in-scattering source or integrated segment radiance
segment transmittance
history confidence/generation metadata where not derivable
```

Do not store density, coefficients, lighting, integrated radiance, and optical depth redundantly without a measured producer/consumer reason. One representation is authoritative at each stage.

## Discrete Integration

For ordered segments front to back, each segment `i` has transmittance `Ti` and premultiplied in-scatter `Li`. Composition is associative in this form:

```text
T_acc' = T_acc * Ti
L_acc' = L_acc + T_acc * Li
```

starting with `T_acc=1`, `L_acc=0`. After reaching opaque depth or the background boundary:

```text
Lout = L_acc + T_acc * Lsurface_or_sky.
```

Equivalently `Lout=T*Lbase+Lscatter`. Store/upsample `(L_acc,T_acc)` together under one extent/depth/generation identity. Alpha-like `1-T` alone loses colored transmittance if the tier admits it.

Step integration uses analytic constant-segment attenuation when possible. Fixed-step quadrature error is measured against analytic/tracking reference as resolution changes. Jitter changes sample positions, not the physical step length or coefficients.

## Geometry, Sky, And Transparency Order

- Opaque scene depth terminates the camera medium integral at the surface.
- Miss/background integrates to the atmosphere/volume far boundary, then applies remaining transmittance to the image or physical sky radiance according to the frozen atmosphere mode.
- A physical atmosphere can supply sky radiance already integrated through the planetary medium; composition must state whether local fog lies inside it and avoid a second atmosphere integral.
- Transparent/refractive surfaces and nested media are excluded initially. Later support requires ordered surface/medium boundaries and cannot be achieved by applying one full-screen fog result after transparency.
- Surface emissive is part of `Lsurface`; volume emission is inside `Lscatter` and attenuated by preceding medium.

## Temporal Reconstruction

The raw current volume sample is jittered under a versioned pattern. Reprojection maps either froxel world positions or final pixels to prior history under a frozen motion convention. Acceptance needs:

```text
View/extent/projection/jitter/depth mapping
medium volume/transform/density/texture generations
light/shadow/environment/atmosphere/LUT generations
algorithm/grid/quality/shader/provider/pre-exposure generations
```

History weight is bounded by confidence. Disocclusion, camera cut, invalid motion, medium boundary crossing, density/emission/light discontinuity, resize, and incompatible generation reject or sharply reduce history. Neighborhood clamps must preserve non-negative finite radiance/transmittance and cannot expand `T` outside `[0,1]` under the scalar contract.

## Atmosphere Contract

The atmosphere is a typed medium over a planet/ground domain. `VOL-D0-10` freezes:

- planet center/radii and View/world transform;
- Rayleigh scattering/density profile;
- Mie scattering/extinction/density/phase profile;
- absorption/ozone profile;
- ground albedo/emission boundary;
- solar irradiance/direction/angular radius and optional atmosphere-light count;
- transmittance, multiple-scattering, sky-view, and aerial-perspective LUT coordinates/formats/precision;
- numerical integration steps used to generate LUTs;
- cache key, atomic publication, invalidation, and fallback.

LUTs are derived immutable products of `AtmosphereGeneration`. A partial set never becomes active. Background sky, sun disk, aerial perspective, and environment sampling name the same generation. Image-based sky and physical atmosphere are explicit modes or a precisely defined composition—not two enabled writers to the same background.

## Heterogeneous Transmittance

For density field `rho(x)` and base coefficient `sigma_t_base`, extinction is `rho*sigma_t_base` under the frozen channel/unit convention. Dense texture transforms map world metres to texture coordinates exactly once.

Reference estimators define:

- majorant `mu(x) >= sigma_t(x)` over the queried domain;
- free-flight/null-collision sampling distribution;
- transmittance contribution weight for delta/ratio/residual tracking;
- response when density violates the majorant;
- maximum events/termination and finite behavior;
- independent random dimensions and statistical confidence.

A fixed-march production result is explicitly biased/discretized and must converge toward the reference as step size decreases. Sparse empty-space skipping may change work, never optical semantics.

## Volumetric ReSTIR Contract

The path integral may include camera free-flight/scattering vertices, subsequent medium/surface vertices, terminal light/environment/emission, and tracking/null events. `VOL-D0-13` selects a bounded domain.

Logical sample state includes:

| Fact | Purpose |
| --- | --- |
| source View/pixel/sample and camera ray | defines source integration domain |
| medium/transform/texture/majorant generation | validates free-flight and transmittance |
| real scattering vertices and directions | defines path and phase/geometry factors |
| null/tracking identity needed by estimator | preserves proposal/contribution weight |
| terminal light/environment/emission identity | supports current evaluation and mutation |
| proposal/technique/path depth/random layout | computes target and generalized weight |
| approximate candidate and final evaluation state | separates cheap reuse from accepted final estimator |

A spatiotemporal shift is a partial bijection between volume-path domains with an inverse, support, and Jacobian. It must handle camera motion, density transform/motion, changing medium boundaries, light/environment mutation, and visibility. Surface normal/depth compatibility alone is irrelevant for samples scattered inside a medium.

Candidate evaluation may use a cheap biased transmittance/scattering approximation only when the ratified estimator proves that its use in resampling does not bias the final selected contribution. The selected path receives the accepted final transmittance/scattering evaluation. This distinction is an acceptance invariant.

GRIS reservoir semantics follow the [Indirect estimator contract](../IndirectLighting/TransportAndEstimator.md#generalized-resampling-contract), specialized to the admitted volume path. Per-froxel light reservoirs use the simpler Direct-light domain and must not be mislabeled path-space Volumetric ReSTIR.

## Normative Reference Procedures

### Analytic Segment

For a segment with constant coefficients and source `S`, the stable implementation is conceptually:

```text
tau = sigma_t * distance
T = exp(-tau)
factor = distance                         when sigma_t approaches zero
         -expm1(-tau) / sigma_t          otherwise
Lsegment = S * factor
```

RGB extinction evaluates component-wise under an admitted RGB tier. The vacuum and opaque limits are explicit branches/results. Reference tests use higher precision than production formats.

### Deterministic March

```text
Tacc = 1; Lacc = 0
for each ordered segment before opaque depth/far boundary:
    sample/average coefficients and source by the frozen quadrature
    compute analytic constant-segment Ti and Li
    Lacc += Tacc * Li
    Tacc *= Ti
    terminate only under a predeclared transmittance/error rule
return (Lacc, Tacc)
```

Jitter alters quadrature locations but not coefficient units, segment length, bounds, or composition order. Decreasing maximum step size must approach the analytic or stochastic reference on the declared convergence fixtures.

### Ratio-Tracking Reference Shape

For a segment with valid scalar majorant `mu >= sigma_t(x)`, a transmittance reference samples exponential free flights under `mu` and multiplies the ratified null-event weight at each event. The exact spectral/RGB estimator and residual/control form are selected by `VOL-D0-11`; implementation must not infer one from this outline.

```text
t = segmentStart; weight = 1
while true:
    t += SampleExponential(mu, rng)
    if t >= segmentEnd: return weight
    sigma = EvaluateExtinction(t)
    require finite 0 <= sigma <= mu
    weight *= RatifiedNullWeight(sigma, mu, control)
    reject on non-finite weight, majorant violation or event-cap policy
```

The reference manifest records majorant construction/version, bounds, coefficient sampler, RNG sequence, event cap, channel policy, control function, confidence interval and violation count. A deterministic marcher and tracker share content decode but not integration logic.

## Froxel Mapping And Sampling Rules

`VOL-D0-06` produces exact forward and inverse functions:

```text
world position <-> View position <-> normalized screen/depth <-> froxel coordinate
```

Tests cover cell centers, all faces/corners, near/far planes, outside rejection, reversed or conventional depth, jitter, resize, projection change and round-trip error. A cell represents a frustum volume, not a point at its center; light/media integration states the spatial quadrature used inside it.

Filtering a field is legal only in a representation with defined interpolation semantics. Coefficients, optical depth, transmittance, normalized source, premultiplied in-scatter and integrated radiance are not interchangeable. If production stores a compressed/normalized representation, decode must reconstruct the semantic pair `(Lscatter,T)` and pass analytic interpolation/composition cases.

## Medium-Light Sampling And MIS

For a light technique selected with PMF `p_select` and conditional direction/shape density `p_conditional`, the volume source uses the combined density in the exact measure of the scattering integral. Phase sampling, light sampling and path continuation can share support; if more than one constructs the same path, `VOL-D0-07/13` freezes ordinary MIS or explicit exclusion before implementation.

| Technique | Required facts | Zero/rejection case |
| --- | --- | --- |
| deterministic all-light | eligible light inventory and exact per-light evaluation | capacity overflow is visible; no silent truncation |
| clustered light list | cell bounds, list construction/generation and overflow order | missing global/large light or list overflow rejects/degrades by policy |
| stochastic light | technique PMF, light PMF, conditional sample PDF, target and current light generation | contributing light with zero proposal support blocks the profile |
| phase-sampled path | normalized phase PDF, free-flight/vertex/path probabilities | singular anisotropy or direction outside support rejects |
| environment/emission | environment mapping/PDF or volume-emission proposal | black/empty source has explicit valid zero distribution |

Geometry visibility and medium transmittance are evaluated on the same segment endpoints but remain separate factors and failure counters. Reusing a surface light distribution is permitted only when its immutable PMF/conditional semantics have support for the volume receiver; its reservoir is never reused.

## Atmosphere LUT Reference Protocol

The selected LUT implementation defines for each texture:

- physical quantity and units;
- coordinate mapping and inverse/boundaries;
- dimensions/format/precision;
- integration domain, quadrature/sample count and approximation;
- source parameters and `AtmosphereGeneration` cache key;
- producer order and atomic publication group;
- consumers and whether they expect transmittance, irradiance, radiance, or multiple-scatter approximation;
- CPU/high-precision or external reference query for the acceptance cells.

Tests sample inside LUT texels and at boundaries, not only rendered skies. Changing any physical parameter builds a new complete set; a failed generation leaves the previous set active and visibly stale, or no atmosphere if none existed. The sun disk, sky view, aerial perspective, surface-light attenuation, and environment PDF cannot mix generations.

## Volume Reservoir Reference Procedure

After `VOL-D0-13` selects a path domain:

```text
GenerateVolumeCandidate(cameraRay, rng):
    sample admitted free-flight/scattering/terminal events
    store explicit path, technique probabilities and medium/content generations
    evaluate bounded approximate target for reservoir selection

MapVolumeCandidate(source, destination):
    transform admitted vertices/events
    validate support, inverse/Jacobian, boundaries, density/light/majorant generations
    reject with a deterministic reason or return generalized weight

ResolveVolumeCandidate(selected):
    re-evaluate the selected current path with the accepted final tracking/transport estimator
    emit raw transmittance/in-scatter/path/confidence independently of reconstruction/composition
```

Approximate evaluation may rank candidates but cannot replace the final estimator. A false-zero approximate target that removes a contributing region is a support defect. Tests compare fresh-only, temporal-only, spatial-only and combined modes for raw mean, variance, path duplication, autocorrelation, motion recovery, rays/events/time and memory.

## Error Budget And Tier Isolation

| Error class | Isolation | Required evidence |
| --- | --- | --- |
| coefficients/phase | CPU/shader point queries | analytic absolute/relative error, normalization and invalid-input result |
| deterministic integration | analytic slabs/height and decreasing step/grid size | convergence curve plus finite extreme-optical-depth output |
| froxel representation | identical analytic field sampled through grid/upsample | spatial/depth error, boundary leakage, format error and memory |
| light/shadow/transmittance | one light/froxel/segment at a time | source-factor breakdown and visibility/transmittance parity |
| temporal reconstruction | fixed raw sequences plus motion/mutation | lag, ghost/leak duration, detail loss and reset frames |
| atmosphere | raw LUT/query/reference cells | radiance/transmittance error by sun/altitude/ground/parameter cell |
| heterogeneous tracker | analytic/piecewise and independent stochastic runs | mean/confidence, variance, majorant/event statistics and convergence |
| volume reservoir | base tracker versus reuse at equal time | raw error, correlation/diversity, recovery, rays/events/time/memory |
| composition | synthetic `(surface, sky, T, Lscatter)` tuples | exact algebra/order before presentation |

Each tier can pass only its own rows. Fog acceptance cannot prove atmosphere, heterogeneous transport, ReSTIR, clouds, transparency, or release readiness.

## Numerical And Safety Rules

- Optical coefficients and density are non-negative finite; transmittance is finite and within the frozen physical range.
- Use stable `expm1`-style evaluation or series limit for small optical depth where required.
- Opaque-limit underflow becomes exact/finite zero transmittance, not NaN.
- HG/atmosphere anisotropy is bounded away from singularity.
- Froxel coordinates, texture transforms, and atmosphere LUT coordinates have inverse/boundary tests.
- Logical IDs, counts, grid dimensions, frame/sample indices, and reservoir multiplicity use integer storage.
- History/reference/resource generations publish transactionally and retire after GPU completion.
- Epsilon never creates medium support, hides a violated majorant, or adds light.

## Equation-To-Owner Ledger

| Contract | Intended owner | Check |
| --- | --- | --- |
| coefficients/phase/analytic volume | feature semantic kernel + CPU oracle | `CHK-VOL-01` |
| medium overlap | prepared medium scene view | `CHK-VOL-03` |
| light scattering/shadow/transmittance | feature light injection using existing light/ray owners | `CHK-VOL-02/08` |
| froxel mapping/integration | feature grid/integrator | `CHK-VOL-02/05` |
| temporal reconstruction | feature or selected reconstruction owner | `CHK-VOL-04` |
| atmosphere/LUTs/environment generation | atmosphere sub-owner inside feature | `CHK-VOL-05/10` |
| dense/sparse lookup/tracking | heterogeneous-medium sub-owner | `CHK-VOL-02/06` |
| volume path shift/reservoir/final evaluation | Volumetric ReSTIR sub-owner | `CHK-VOL-07` |

## Ratification Rule

`VOL-D0` must replace every unresolved formula, representation, unit, mapping, approximation, step/grid value, precision, failure response, and threshold with a reviewed decision before its production stage. Analytic/reference cases are written before optimized shaders; implementation convenience cannot define the physics retroactively.

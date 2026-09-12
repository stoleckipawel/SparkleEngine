# Indirect Lighting Transport And Estimator Contract

**Status:** target mathematical contract with discovery-gated path domains and mappings; no implementation/convergence claim

**Responsibility:** own indirect path measurement, technique accounting, path records, shifts, GRIS/reservoir weights, reuse, environment/emission, reconstruction-signal, and numerical rules

**Authority boundary:** `IND-D0` ratifies the selected domain/equations; [Execution Architecture](ExecutionArchitecture.md) owns system shape; [Plan](Plan.md) owns order; code/tests conform; `FCR-REN-07` owns results

**Notation:** a camera path is `x0 ... xk`, directions point away from a surface, `ng/ns` are geometric/shading normals, and all radiance products are scene-linear

## Claim Boundary

This page prevents three concepts from collapsing:

- path tracing generates an initial indirect sample;
- GRIS/ReSTIR transforms and resamples path samples between receiver domains;
- reconstruction filters sparse estimates using guides and history.

Correctness of one does not prove the others. The present Sparkle seed replay has not been shown to satisfy the path-transform/resampling contract below.

## Working Space And Transport Domain

Distances, light units, material semantics, texture decode, ray robustness, and RGB basis must match the accepted Reference Path Tracer and Direct Lighting contracts. `IND-D0-01/02` freezes the supported path domain:

```text
surface camera vertex x0
primary surface x1
one or more admitted non-delta/rough surface vertices x2 ... x(k-1)
terminal sampled emitter, emissive hit, or environment xk
```

Participating-media vertices, transmission/refraction, perfect-delta chains, caustic light tracing, lens effects, and atmosphere along camera/connection rays are excluded until explicitly admitted. A path outside the active domain is rejected or evaluated by another declared technique; it is never coerced into diffuse support.

## Path Measurement

The pixel value is a path-space integral:

```text
I = integral_Omega f(path) d(path).
```

`f` includes camera measurement, emission, BSDF factors, absolute geometry cosines, visibility, and emitted/environment radiance in the chosen measure. A conventional sampled surface path carries throughput

```text
beta_(j+1) = beta_j * f_s(xj, w_prev, w_next)
             * abs(dot(ns_j, w_next)) / p_bsdf(w_next)
```

plus any frozen shading-normal correction. Russian roulette after the admitted minimum depth continues with probability `p_continue` and divides surviving throughput by `p_continue`. Maximum depth is a product truncation policy and must be reported; it is not an unbiased infinite-bounce claim.

Every sampled value is finite or rejected before it enters a reservoir/history. Negative RGB due solely to numerical error follows one frozen clamp/failure policy; arbitrary clamping of valid high energy is prohibited.

## Initial One-Bounce Candidate

The first reviewable candidate contains:

```text
x1 primary receiver
  -- BSDF/lobe sample (p_bsdf) --> x2 secondary surface
x2 -- light selection/shape sample (p_light) --> emitter y
```

Its NEE contribution is conceptually

```text
C_nee = beta_1 * f_s(x1 -> x2) * geometry/visibility sampling factor
        * f_s(x2, x1, y) * Le(y -> x2) * V(x2,y)
        * abs(dot(ns_2, wi_y)) / p_light(y | x2)
```

with the exact measure factors placed once according to the sampled ray/area/solid-angle formulation. `beta_1` and the `x1 -> x2` BSDF sample already contain their PDF/cosine factors; code must not apply them twice.

When the BSDF ray hits an emitter or environment, the terminal contribution uses the frozen MIS policy against NEE where both techniques have support. Delta events follow their discrete technique. The common surface emissive composite must not add the same indirect terminal contribution again.

Initial candidates store enough facts to recompute their vector contribution `f(path)`, scalar target, source technique probability, and any shift. The baseline stores explicit `x2` identity/geometry/material plus terminal sample; a later compact replay representation must prove exact equivalence for the admitted mapping.

## Lobe Classification

`IndirectDiffuse` and `IndirectSpecular` classification is frozen at the first indirect scattering event or another explicit rule selected by `IND-D0-02`. The same physical path cannot contribute fully to both outputs. Roughness thresholds affect technique selection and reconstruction only after the material sampling domain is fixed.

## Environment And Emission Accounting

One immutable `EnvironmentGeneration` owns mapping, rotation, radiance basis, dimensions/content hash, and importance PDF.

| Event | Contribution rule |
| --- | --- |
| primary camera miss | background emission; later atmospheric composition may attenuate/add in-scattering |
| NEE environment direction | direct/indirect sampled-light technique with selection and directional PDF |
| secondary BSDF miss | terminal path emission with MIS when NEE shares support |
| emissive surface hit | terminal emission with material/light generation and MIS policy |
| emissive surface enumerated as a light | NEE technique; must share identity with hit emission to prevent duplication |

Black/empty environment produces an explicit zero distribution fallback without invalid PDF descent.

## Path Sample Record

The target logical record is independent of packing:

| Field | Requirement |
| --- | --- |
| source receiver | View/pixel/sample plus primary object/primitive/material/surface generation |
| explicit vertices | position, geometric/shading normal, object/primitive/material generation for every mapping anchor |
| directions/lobes | sampled direction, lobe/roughness/delta class, forward/reverse densities as required |
| terminal sample | light/environment/emission identity, shape/direction parameters, generation |
| technique | BSDF/NEE/MIS/path-length/random-layout identity |
| factors | throughput/contribution factors or exact reproducible state, target, contribution weight |
| termination | depth, roulette decision/probability, valid/failed reason |

IDs, indices, depths, sample counts, frame generations, and random dimensions use integer storage. Packing tests cover maximum live values and long runs. Binary32 is permitted for continuous quantities only where error bounds pass.

## Shift Mapping Contract

For input domain `Omega_i`, a shift is a partial bijection

```text
T_i : Omega_i -> Omega_target
```

with an inverse on its valid support and absolute Jacobian determinant `J_i`. If `x` has density `q_i(x)`, transformed `y=T_i(x)` has

```text
q_i_to_target(y) = q_i(x) / abs(J_i(x)).
```

A mapping is invalid if the inverse is ambiguous, determinant is zero/non-finite, path support changes without accounted technique, a required connection is occluded, a vertex/material/light generation is unavailable, or a delta constraint cannot be preserved.

### Reconnection

The baseline reconnects target primary receiver `y1` to a stored secondary vertex `x2`, retaining the suffix only if:

- both path domains and lobes admit the connection;
- geometric/shading hemispheres and roughness/footprint criteria pass;
- the new segment is visible under frozen ray semantics;
- source/destination BSDF, geometry, and proposal factors can be evaluated;
- the exact area/solid-angle Jacobian and inverse mapping are finite.

### Random Replay / Hybrid

Replay may regenerate a suffix from recorded random dimensions, especially through specular chains. It still requires source technique probability, destination probability, geometry/material motion handling, deterministic random layout, and a valid inverse/domain definition. “Run the same seed at a new pixel” alone is not a shift.

Hybrid selection between reconnection and replay is part of the sample technique; its probability and support enter contribution/MIS weights.

## Generalized Resampling Contract

Represent each candidate by `(Y_i, W_i)`, where `W_i` is its generalized contribution weight: for appropriate test functions it turns the random candidate representation into an unbiased estimate over the target domain under the ratified GRIS conditions. A newly sampled candidate normally begins from the reciprocal proposal density, including mapping Jacobian and technique MIS terms.

Choose a non-negative scalar target `pHat(Y)` proportional to magnitude of the vector contribution. For resampling MIS weights `m_i(Y)` that satisfy the selected partition/normalization rule, use conceptual resampling weight

```text
w_i = m_i(Y_i) * pHat(Y_i) * W_i.
```

Stream candidates proportional to non-negative finite `w_i`. If `Y` is selected, the output generalized contribution weight is conceptually

```text
W_out = sum_i(w_i) / pHat(Y).
estimate = W_out * f(Y).
```

The exact normalization may include reservoir multiplicity, canonical sample count, or mapping-specific MIS factors. `IND-D0-07` must transcribe and ratify the chosen GRIS formulation from its pinned source, then test enumerated distributions. The formula above defines the abstraction and prohibits the current shortcut `WeightSum/(M*Target)` from being assumed valid for arbitrary shifted/correlated path reservoirs.

### Reservoir Invariants

- selected sample is valid in the target domain;
- accumulated resampling weight, target, contribution weight, and effective count are finite and non-negative;
- effective count is bounded according to a documented rule, not allowed to grow without precision/history limits;
- zero target/support yields no selected contribution;
- a duplicated/correlated candidate follows the declared GRIS/MIS mode;
- reservoir-to-reservoir combination preserves source multiplicity and contribution-weight meaning;
- selected path/light/surface generations remain alive until consuming GPU work completes.

## Temporal And Spatial Reuse

Temporal reuse transforms a completed prior-frame path into the current receiver/scene. Rigid/deformed geometry motion, light/sky/material changes, and visibility are handled by the shift or invalidate the candidate. Backprojection, forward reservoir splatting, and multi-layer splatting are alternative proposal mechanisms; only the one frozen by discovery participates in the estimator.

Spatial reuse draws decorrelated neighbor reservoirs from the current compatible generation. Neighbor selection probability, screen bounds, receiver mapping, shift, and MIS weight are explicit. Large kernels and repeated spatial passes need correlation and convergence evidence.

Compatibility is a fast rejection layer, not a proof of valid mapping:

```text
view/extent/jitter/motion + hit/object/primitive/material generation
depth/position + geometric/shading normal + lobe/roughness
path/light/environment generation + estimator/random-layout/shader/provider generation
```

## Bias, Correlation, And Convergence Vocabulary

| Term | Allowed claim |
| --- | --- |
| unbiased initial estimator | demonstrated statistical agreement under its fixed finite path domain |
| asymptotically/convergently correct GRIS mode | only if the selected theory's conditions and local tests are satisfied |
| biased real-time mode | exact shortcut named and bounded against reference over frozen workloads |
| low variance | metric/confidence interval at stated samples/time, never visual smoothness alone |
| temporally stable | motion sequence metric plus artifact review; not correctness |

Correlation is measured through selected path duplication, effective diversity, temporal autocorrelation, and error convergence. Increasing `M` with repeated correlated samples cannot be advertised as equivalent independent sample count.

## Ray And Animation Semantics

Primary/connection/path rays share one geometric-normal offset, distance interval, face/two-sided/alpha, UV/texture, material-generation, and instance-motion contract with Direct Lighting and the Reference Path Tracer. Provider differences remain execution mechanics. Indirect may initially support Inline only, but its status must say so; Pipeline absence cannot be hidden as backend parity.

Animated geometry requires current/previous vertex identity or invalidation. A seed that retraces changed geometry is a new proposal whose probability/mapping must be accounted, not automatically the same historical path.

## Reconstruction Contract

Raw indirect diffuse/specular radiance and hit distance remain authoritative. Reconstruction receives exact motion/depth/normals/roughness/material, path/reservoir confidence, disocclusion, exposure, extent, algorithm, and shader generations. Its history is separate from path reservoirs, reset independently, and never feeds filtered radiance back as an unbiased path sample.

Static accumulation quality, motion response, disocclusion, high-frequency indirect shadow, glossy reflection, emissive toggle, and environment rotation are all evaluated raw and reconstructed.

## Numerical And Safety Rules

- Every PDF and Jacobian names its measure and support.
- No epsilon creates support or turns invalid mapping into finite weight.
- Throughput, target, resampling weight, contribution weight, radiance, and reconstruction inputs are checked for non-finite values at test boundaries and bounded runtime fault points.
- Maximum path depth and roulette probability are bounded; zero continuation terminates without division.
- Random dimensions and path-record schema are semantic generations; changing them resets history.
- Cancellation/device loss/reload cannot publish partially written reservoirs or reconstruction histories.

## Equation-To-Code Ledger

| Contract | Intended owner | Check |
| --- | --- | --- |
| initial path/NEE/BSDF/MIS/emission | shared focused transport semantics plus Indirect initial generator | `CHK-IND-01` |
| path record and packing | Indirect feature sample owner | `CHK-IND-02`, `CHK-IND-04` |
| reconnection/replay/hybrid shift | Indirect shift owner | `CHK-IND-02` |
| GRIS/reservoir stream/output weight | Indirect reservoir owner | `CHK-IND-02` |
| temporal/spatial proposal and compatibility | Indirect reuse owner | `CHK-IND-03` |
| environment generation/techniques | existing Sky/environment owner plus Indirect consumer | `CHK-IND-01`, `CHK-IND-03` |
| rays/material/animation | shared ray/material owners through narrow semantic functions | `CHK-IND-01`, `CHK-IND-07` |
| reconstruction | selected reconstruction owner | `CHK-IND-05` |

## Ratification Checklist

Before Stage 1 implementation, `IND-D0` must freeze path domain, techniques, exact equations, mappings/inverses/Jacobians, contribution weight, target, MIS/correlation/bias mode, record/precision, histories, safety, and test thresholds. Every equation receives a hand case and code owner. Anything unresolved stays out of the active product domain.

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

## Normative Reference Procedure

Once `IND-D0` ratifies the exact domain and equations, CPU/reference and shader paths follow the same semantic stages:

```text
GenerateInitialPath(receiver, rng):
    sample first admitted BSDF/lobe and trace the secondary vertex
    record forward/reverse densities and throughput in their exact measures
    select/evaluate one terminal technique according to frozen NEE/emission/environment/MIS policy
    record every vertex, technique, probability, generation and terminal fact needed by shifts
    compute vector contribution, scalar target and generalized source weight
    reject non-finite, unsupported or excluded paths before reservoir update

ShiftPath(sourcePath, destinationReceiver, mapping):
    validate source and destination generations/domains
    apply mapping and construct destination path
    evaluate inverse/support/Jacobian, current materials and required visibility
    return mapped path plus exact generalized contribution weight, or a reason-coded rejection

Resample(destination, freshAndMappedSamples):
    stream the ratified GRIS weights with bounded effective counts
    retain one coherent selected path and source/mapping metadata
    never reinterpret stored M as independent sample count

Resolve(destination, selectedPath):
    reevaluate the selected path in the current scene according to the final estimator
    classify its contribution once into admitted raw lobe products
    publish path/reservoir/confidence facts separately from reconstruction
```

The no-reuse reference calls `GenerateInitialPath` independently for each sample and never calls `ShiftPath` or reservoir merge code. The offline/reference tracer may be an additional oracle only after its acceptance and shared-code analysis; analytic and external comparisons remain required.

## Minimum Path Record By Proof Obligation

Packing is selected only after the logical facts are complete:

| Proof obligation | Minimum logical facts |
| --- | --- |
| re-evaluate source path | vertex positions, geometric/shading normals, materials/lobes, directions, terminal source, throughput factors |
| compute proposal/technique probability | technique IDs, selection PMFs, conditional PDFs, measures, forward/reverse probabilities, roulette |
| reconnect | source/destination receivers, anchor vertex, geometry factors, source/destination BSDF support and visibility |
| replay/hybrid | versioned random dimensions, every distribution/control-flow choice, explicit prefix events and divergence rule |
| mutate safely | View/object/primitive/material/light/environment/TLAS/shader/provider/content generations |
| classify outputs | first indirect lobe, path depth, terminal event, excluded/delta flags |
| reproduce failure | mapping family, rejection reason, finite/clamp state, source pixel/sample/frame integer identity |

A field may be removed only after a proof shows it is derivable from immutable current data without changing probability, mapping, lifetime, or diagnostics. Recomputing a previous material/light from current mutable indices is not derivation.

## Shift Proof Protocol

Every mapping is accepted independently for each path/lobe class:

1. Define source domain, destination domain, forward transformation and all unchanged/moved vertices.
2. State the density measure on both sides and derive the determinant/Jacobian or the generalized weight that replaces it.
3. Define the inverse or the theory that permits the chosen non-bijective/conditional form.
4. Enumerate zero-support, singular, occluded, delta, material/lobe, geometry-motion and generation rejection cases.
5. Implement deterministic hand paths with analytically known round trip and Jacobian.
6. Compare mapped-sample distributions and estimator means against independently generated destination samples.
7. Add the mapping to temporal/spatial reuse only after static proof passes.

No epsilon repairs zero support or a singular Jacobian. A mapping that is valid for diffuse paths can remain rejected for glossy/delta paths without blocking the narrower tier.

## Technique And MIS Ledger

For every path length and terminal event, one table generated during discovery must name:

| Fact | Required entry |
| --- | --- |
| construction technique | BSDF sequence, NEE/emitter/environment/continuation selection and discrete probabilities |
| path density | per-edge conditional density and conversion to the common path measure |
| alternate techniques | which could construct the same path and their densities |
| MIS/resampling role | ordinary path MIS, GRIS source weight, or explicit technique exclusion |
| output owner | indirect diffuse, indirect specular, direct lighting, camera-visible emissive/background, or excluded |
| reference case | analytic/metamorphic scene and expected change when this technique is disabled |

The ledger is executable input to tests, not prose copied into multiple shaders. Adding a bounce or terminal technique changes this ledger, the record, shift domain, random layout, acceptance cells, and history generation together.

## Statistical And Temporal Evidence Protocol

| Claim | Minimum experiment |
| --- | --- |
| initial estimator mean | independent seeds over analytic/Cornell cells with predeclared confidence interval against analytic/external reference |
| reservoir selection | enumerated discrete samples with known generalized weights and exact expected frequencies |
| shift validity | hand round trips plus destination-distribution/mean comparison, including rejected support |
| reuse value | fresh-only, temporal-only, spatial-only and combined equal-time runs |
| correlation control | unique selected paths, duplication map, temporal autocorrelation and effective diversity versus stored `M` |
| motion safety | camera, disocclusion, rigid/deformed object, material, light, sky, provider and shader changes with recovery frames |
| glossy/depth expansion | roughness-by-depth-by-terminal matrix with explicit unsupported cells |
| reconstruction | identical raw input/guide sequence comparing raw mean, filtered error/lag and detail retention |

The candidate manifest fixes seeds or sequence generator/version, samples/frames, warmup, region/mask, reference spp/artifact, exposure, metric, tolerance and invalid-sample handling before observation. Results from different path domains or histories are not pooled.

## Optimization Admission Rules

Wave compaction, half resolution, fused passes, compact replay, reciprocal neighbors, splatting, path guiding, caches and learned reconstruction enter one at a time. Each optimization must preserve the semantic artifacts above or provide a diagnostic reconstruction of them, and must beat the accepted predecessor on the named equal-time workload without exceeding any required error/lag/memory/backend cell. Rejected experiments are removed; they do not remain behind dormant switches.

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

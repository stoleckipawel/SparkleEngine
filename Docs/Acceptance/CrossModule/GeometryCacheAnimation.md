# Geometry Cache Animation Acceptance Contract

Status: acceptance contract; not proof that geometry-cache animation has passed

Scope: feature verification matrix and completion gates for geometry-cache animation

Architecture authority: [Geometry Cache Animation Pipeline](../../Architecture/CrossModule/GeometryCacheAnimation.md)

Delivery authority: [Geometry Cache Animation Delivery Plan](../../Plans/CrossModule/GeometryCacheAnimation.md)

Workload authority: [Graphics Workloads](../GraphicsWorkloads.md)

This contract owns the feature proof required after delivery. Results must retain exact commands, configurations, artifacts, and limitations.

## Traceability And Current Disposition

| Dimension | Binding target |
|---|---|
| North Star | `NS-REAL`, `NS-MATH-DATA`, `NS-EVIDENCE`, `NS-OWNERSHIP`, `NS-ADOPTION`, `NS-SIMPLIFY` |
| Persona targets | `PGE-02`, `PGE-05`–`PGE-10`, `PGE-13`, `PGE-15` |
| Roadmap target | Not in the `v0.1.0` source-present closure. `REL-11` and an explicit later roadmap admission are prerequisites to implementation. |
| Feature completion | No `FCR-*` candidate is permitted yet because the current source snapshot reports no geometry-cache runtime capability. Admission creates the feature report and copies the risk seeds below into its live risk ledger. |
| Release risk | `RISK-REL-12`; doing this work before release closeout is a scope/WIP failure. |

| Risk seed | Cause, event, and consequence | Likelihood / impact before implementation | Owner / gate | Prevention and detection; contingency and retirement evidence |
|---|---|---|---|---|
| `RISK-GCA-01` | Incorrect axis/unit/handedness, topology, material-slot, or time semantics produce plausible but wrong geometry and lighting. | High / Critical | Import/content owner; future correctness gate | Tools-only normalization/validation, source/cooked oracle, fixed-time overlays, rejection fixtures. Keep the accepted static asset if unresolved; retire only with source-reference evidence. |
| `RISK-GCA-02` | Streaming, cancellation, generation replacement, or retirement publishes stale/freed sample data or grows memory without bound. | High / Critical | Asset/Renderer owner; future lifetime/performance gate | One residency/lifetime owner, bounded budgets, delayed-consumer stress, high-water and stale-generation detection. Disable/defer playback rather than run unbounded. |
| `RISK-GCA-03` | Raster and ray paths consume different current/previous deformation or attributes, causing visibility, motion, or lighting divergence. | High / High | Renderer/RHI owner; future parity gate | One shared deformed product, readback oracle, fixed-time comparisons, paired native validation. Exclude unsupported consumers; retire only advertised rows. |
| `RISK-GCA-04` | Alembic/USD runtime dependencies, frame expansion, scene branches, duplicate caches, or permanent instrumentation make the feature unshippable. | Medium / High | Feature/build owner; future completion gate | Tools-only source boundary, native cooked asset, copy/owner budget, clean-break inventory. Revert/defer and retain static content if the bounded design cannot pass. |

The owning future iteration maps every `AC-GCA-*` and `FM-GCA-*` to the named `CHK-GCA-*`, artifacts, and results. These are planning claims only; no row is passed by this document.

## Verification Matrix

| Check | Contract | Narrow automated/static evidence | Runtime/capture evidence |
|---|---|---|---|
| `CHK-GCA-01` | Source normalization | axis/unit/handedness/transform fixture and Knight metadata inventory | fixed source-reference sample overlay |
| `CHK-GCA-02` | Topology profile | stable topology positive fixture; vertex/index/batch drift rejection | no partial or frozen rendering for rejected content |
| `CHK-GCA-03` | Material slots | face-set mapping round trip; missing/duplicate mapping rejection | correct Knight material separation and semantic texture channels |
| `CHK-GCA-04` | Cooked format | deterministic hash; corrupt/truncated/overflow/checksum checks; random-access sample oracle | bounded startup bytes and seek latency |
| `CHK-GCA-05` | Playback | pause/rate/offset/loop/seek/discontinuity checks | visible loop and scrub stability |
| `CHK-GCA-06` | Residency | budget, cancellation, generation replacement, starvation, and retirement checks | long playback and random-seek memory high-water |
| `CHK-GCA-07` | Shared deformation | CPU versus GPU current/previous readback | matching raster/ray geometry at fixed times |
| `CHK-GCA-08` | Raster | shader ABI, zero-content omission, bounds/culling checks | D3D12/Vulkan GBuffer, normals, materials, motion vectors |
| `CHK-GCA-09` | Ray tracing | generic BLAS update contract and resource-lifetime checks | primary, shadow, reflection, GI/path visibility and cost |
| `CHK-GCA-10` | Cleanup | architecture-boundary check, build membership, `git diff --check`, no Alembic runtime linkage | no scene-name branches, fallback pose, or permanent diagnostic clutter |

The Knight evidence record includes source and cooked bytes, track/sample/vertex/triangle counts, cook time and peak memory, compression ratio, read/decode/upload latency, resident CPU/GPU bytes, deformation pass time, BLAS build/update time and memory, total frame time, and fixed-camera captures. Use existing profiler/capture mechanisms. Do not add a permanent geometry-cache dashboard or periodic log stream.

## Failure Modes

| Failure ID | Controlled setup | Accepted behavior | Covering check |
|---|---|---|---|
| `FM-GCA-01` | Import changing topology, mismatched face/material slots, invalid transforms/time, or missing samples. | Cook rejects the asset with track/sample identity before runtime products exist; no partial/frozen fallback is published. | `CHK-GCA-01`–`CHK-GCA-03` |
| `FM-GCA-02` | Truncate/corrupt the copied cooked asset, alter offsets/counts/checksum, or request an invalid sample. | Load/seek rejects before out-of-bounds access or scene publication and identifies the failed contract. | `CHK-GCA-04` |
| `FM-GCA-03` | Cancel/reload during decode/upload, delay GPU completion, and force generation replacement. | Stale work cannot publish or free in-use ranges; owned work settles and memory returns within the declared bound. | `CHK-GCA-06`, `CHK-GCA-09` |
| `FM-GCA-04` | Pause, loop, reverse/rate-change if supported, and seek across a discontinuity. | Current/previous identity follows the stated reset rule with no temporal spike, one-frame static pose, or old-generation sample. | `CHK-GCA-05`, `CHK-GCA-07`, `CHK-GCA-08` |
| `FM-GCA-05` | Exercise long playback/random seek near the declared CPU/GPU memory budget. | Residency remains bounded or rejects work before system/device instability; no unbounded queue or cache growth. | `CHK-GCA-06` |
| `FM-GCA-06` | Run the frozen times on both backends and all advertised raster/ray consumers. | Geometry, attributes, motion, visibility, and lighting meet declared tolerances; unsupported ray capability is rejected before scheduling. | `CHK-GCA-07`–`CHK-GCA-09` |

## Completion Definition

Geometry-cache animation is complete only when all of the following are true:

- `AC-GCA-01` — Alembic is tools-only and produces a deterministic native cooked asset. Checks: `CHK-GCA-01`, `CHK-GCA-04`, `CHK-GCA-10`.
- `AC-GCA-02` — unsupported topology or material data fails explicitly at cook time. Checks: `CHK-GCA-02`, `CHK-GCA-03`.
- `AC-GCA-03` — GameFramework holds only playback state and immutable asset identity. Check: `CHK-GCA-10`.
- `AC-GCA-04` — sample bytes stream through one bounded residency/lifetime owner. Check: `CHK-GCA-06`.
- `AC-GCA-05` — raster, dynamic BLAS, and ray-hit attributes consume one current/previous deformed-geometry product. Checks: `CHK-GCA-07`–`CHK-GCA-09`.
- `AC-GCA-06` — raster and ray-traced primary visibility, reflections, GI, and path/reference lighting show the same accepted deformation. Checks: `CHK-GCA-07`–`CHK-GCA-09`.
- `AC-GCA-07` — D3D12 and Vulkan pass the recorded gates. Checks: `CHK-GCA-08`, `CHK-GCA-09`.
- `AC-GCA-08` — Modern Sponza Knight is correctly scaled, placed, material-bound, animated, and measured without a scene-specific branch or static fallback. Checks: `CHK-GCA-01`, `CHK-GCA-03`, `CHK-GCA-05`–`CHK-GCA-09`.
- `AC-GCA-09` — no USD runtime, Alembic runtime, pose bake, morph-frame expansion, duplicate cache, dormant mode, or permanent diagnostic clutter remains. Check: `CHK-GCA-10`.
- `AC-GCA-10` — documentation and workload status distinguish implemented evidence from future changing-topology and simulation work. Check: `CHK-GCA-10`.

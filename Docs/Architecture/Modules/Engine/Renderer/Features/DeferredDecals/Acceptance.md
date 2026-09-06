# Deferred GBuffer Decals — Acceptance

Status: feature-local acceptance contract; not proof that deferred decals have passed

Scope: validation fixture, producer/consumer matrix, controlled failures, and completion gates for deferred GBuffer decals

Feature architecture: [Deferred GBuffer Decal Composition Architecture](CompositionArchitecture.md)

Delivery authority: [Deferred GBuffer Decals Delivery Plan](../../../../../../Plans/Renderer/DeferredGBufferDecals.md)

Release/workload orchestration: [Graphics Workloads](../../../../../../Acceptance/GraphicsWorkloads.md)

This file is part of the Deferred Decals feature dossier and owns the proof contract required after delivery. Candidate results remain in the release-level completion report and must retain exact commands, configurations, artifacts, and limitations.

## Traceability And Current Disposition

| Dimension | Binding target |
|---|---|
| North Star | `NS-REAL`, `NS-MATH-DATA`, `NS-EVIDENCE`, `NS-OWNERSHIP`, `NS-ADOPTION`, `NS-SIMPLIFY` |
| Persona targets | `PGE-02`, `PGE-05`–`PGE-10`, `PGE-13`, `PGE-15` |
| Roadmap target | Not in the `v0.1.0` source-present closure. `REL-11` and an explicit later roadmap admission are prerequisites to implementation. |
| Feature completion | No `FCR-*` candidate is permitted yet because the current source snapshot reports no decal capability. Admission creates the feature report and copies the risk seeds below into its live risk ledger. |
| Release risk | `RISK-REL-12`; doing this work before the release closeout is a scope/WIP failure. |

| Risk seed | Cause, event, and consequence | Likelihood / impact before implementation | Owner / gate | Prevention and detection; contingency and retirement evidence |
|---|---|---|---|---|
| `RISK-DECAL-01` | Independent channel blending or duplicate material reconstruction corrupts GBuffer/PBR semantics and produces plausible but wrong lighting. | High / Critical | Material/Renderer owner; future feature gate | One composition contract, CPU oracle, raw-channel and reference checks. If unresolved, do not ship/enable decals. Retire across every producer and backend. |
| `RISK-DECAL-02` | Unbounded tile or receiver candidate growth causes overflow, memory spikes, or frame-time collapse in overlap-heavy views. | High / High | Renderer/performance owner; future performance gate | Explicit capacities/overflow, brute-force comparison, stress scaling, high-water and p99 evidence. If the budget fails, reduce supported density or defer the feature. |
| `RISK-DECAL-03` | Raster primary, ray primary, and secondary hits apply different projection, ordering, LOD, or receiver rules. | High / High | Renderer/RHI owner; future parity gate | Shared functions/data, producer comparisons, reflected fixture, paired backends. Exclude unsupported producers rather than fall back silently; retire only advertised rows. |
| `RISK-DECAL-04` | The new system introduces fallback paths, duplicated caches, or permanent diagnostics and grows maintenance cost beyond the feature value. | Medium / High | Feature owner; future completion gate | Clean-break review, zero-content proof, owner/copy budget, deleted-path inventory. Revert/defer the feature if one bounded authority is not achieved. |

The owning future iteration maps every `AC-DECAL-*` and `FM-DECAL-*` to the named `CHK-DECAL-*`, artifacts, and results. These are planning claims only; no row is passed by this document.

## Modern Sponza Validation Fixture

Intel's source package remains external and unmodified. Add a small repository-owned decal fixture under Showcase ownership and place it from the Modern Sponza level composition. Keep it visually intentional and small enough for review:

| Placement | Contract exercised |
|---|---|
| Broad wall damp/grime patch | BaseColor + Material, large screen coverage, soft depth edge |
| Chipped plaster/crack | Normal + roughness, grazing-angle facing fade |
| Faded painted mark | BaseColor alpha coverage and stable close inspection |
| Floor wet patch | Normal + roughness under reflected lighting |
| Two-layer repair/mark stack | Stable overlap order and all producer parity |
| Volume crossing an opted-out receiver | Exact box test and `ReceivesDecals` behavior |

The fixture must state its own texture license/provenance. Illustrative values or captures are not Intel reference measurements. Modern Sponza remains a compatibility workload under [Graphics Workloads](../../../../../../Acceptance/GraphicsWorkloads.md), not a replacement for its larger acceptance gates.

## Validation Matrix

| Surface | Raster primary | Ray primary | Ray reflection/GI hit |
|---|---:|---:|---:|
| Opaque static mesh | Required | Required | Required |
| Alpha-tested static mesh | Required | Required | Required |
| Receiver opted out | Unchanged | Unchanged | Unchanged |
| Sky/miss | Unchanged | Unchanged | Unchanged |
| Alpha-blended/transparent | Unsupported | Unsupported | Unsupported |
| Skinned or moving receiver | Static-frame appearance only; temporal support deferred | Same | Candidate update support deferred |

Minimum key-check coverage follows the [check and test design contract](../../../../../../Engineering/Verification/ValidationAndEvidence.md#check-and-test-design-contract). These identities do not authorize new permanent test code:

- `CHK-DECAL-01` — projection inside/outside, orientation, UV, edge/facing fade, negative/degenerate transform rejection;
- `CHK-DECAL-02` — composition masks, zero/one/intermediate coverage, normal normalization, receiver alpha preservation, and F0 preservation when the Material group is disabled;
- `CHK-DECAL-03` — stable ordering with equal and unequal sort values;
- `CHK-DECAL-04` — tile candidates against a brute-force CPU oracle, including near-plane and camera-inside cases;
- `CHK-DECAL-05` — secondary candidate spans against brute-force OBB/receiver overlap;
- `CHK-DECAL-06` — GBuffer pack/decode round trip for every affected field and receiver bit;
- `CHK-DECAL-07` — zero-decals graph omission and unchanged image;
- `CHK-DECAL-08` — D3D12/Vulkan shader ABI and image parity;
- `CHK-DECAL-09` — fixed-camera raster versus ray-primary comparison;
- `CHK-DECAL-10` — visible versus reflected/indirect appearance for the same decal.
- `CHK-DECAL-11` — create/update/destroy/reload with delayed completion, dirty-range retirement, zero-decal allocation/pass omission, and stale-generation detection.
- `CHK-DECAL-12` — owner/copy/build-membership and semantic-eradication review proving no fallback, DBuffer, separate decal-material cache, duplicate ray-material path, dormant flag, or status overclaim remains.

Performance evidence records existing frame time and the one pass duration, CPU plan time, upload bytes, active tiles, tile candidate indices, secondary candidate links, and shaded-hit candidate count. These belong in the existing capture/evidence workflow. Do not add a decal dashboard, periodic logging, or a new diagnostics subsystem.

## Failure Modes

| Failure ID | Controlled setup | Accepted behavior | Covering check |
|---|---|---|---|
| `FM-DECAL-01` | Supply a negative/degenerate transform, invalid material reference, or unsupported receiver. | Authoring/cook/readiness rejects the exact object before partial publication; no invisible success or default material substitution. | `CHK-DECAL-01`, `CHK-DECAL-06` |
| `FM-DECAL-02` | Exceed a declared tile/receiver candidate capacity and exercise camera-inside/near-plane volumes. | The selected overflow policy is bounded, deterministic, surfaced, and memory safe; it never writes outside capacity or silently loses arbitrary decals. | `CHK-DECAL-04`, `CHK-DECAL-05` |
| `FM-DECAL-03` | Compare raster primary, ray primary, and a reflected/indirect hit at the same authored state. | Projection, composition, ordering, and receiver behavior agree within the declared tolerance, with no double application. | `CHK-DECAL-02`, `CHK-DECAL-09`, `CHK-DECAL-10` |
| `FM-DECAL-04` | Use equal sort values, overlapping volumes, disabled material groups, and a receiver opted out. | Stable tie-breaking and preservation rules produce the predeclared result on both backends. | `CHK-DECAL-02`, `CHK-DECAL-03`, `CHK-DECAL-08` |
| `FM-DECAL-05` | Load, remove, and reload the final decal while testing a zero-decal scene. | Dirty data retires at the owned completion point and zero content leaves no pass, allocation, stale contribution, or diagnostic residue. | `CHK-DECAL-07`, `CHK-DECAL-11` |

## Completion Definition

Deferred decals are complete only when all of the following are true:

- `AC-DECAL-01` — authors place one decal kind that references one ordinary material kind. Checks: `CHK-DECAL-01`, `CHK-DECAL-06`.
- `AC-DECAL-02` — zero content has zero runtime pass cost. Checks: `CHK-DECAL-07`, `CHK-DECAL-11` and the recorded performance capture.
- `AC-DECAL-03` — raster and ray-traced primary visibility use the same post-GBuffer resolve. Check: `CHK-DECAL-09`.
- `AC-DECAL-04` — GI/reflection material hits use the same projection and composition functions. Check: `CHK-DECAL-10`.
- `AC-DECAL-05` — overlap order and receiver behavior match on D3D12 and Vulkan. Checks: `CHK-DECAL-03`, `CHK-DECAL-08`, `CHK-DECAL-09`.
- `AC-DECAL-06` — Modern Sponza fixture images and performance evidence satisfy the recorded gates. Checks: `CHK-DECAL-01`–`CHK-DECAL-11` and the recorded performance capture.
- `AC-DECAL-07` — no forward fallback, DBuffer, decal material cache, duplicate ray material path, or unowned diagnostic surface remains. Check: `CHK-DECAL-12`.
- `AC-DECAL-08` — the docs distinguish implemented behavior from future dynamic/transparent/mesh-decal work. Check: `CHK-DECAL-12`.

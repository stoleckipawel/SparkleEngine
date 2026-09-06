# Renderer Scene and View Preparation Acceptance

Status: feature-local acceptance contract; defined but unproved

Scope: shared identity, concurrency, capacity, failure, backend, reset, and completion requirements across scene preparation, view preparation, and GPU-scene publication

Authority boundary: [Scene Preparation](ScenePreparation.md), [View Preparation](ViewPreparation.md), and [GPU-Scene Publication](GpuScenePublication.md) own their mechanisms. [Mesh and Texture Residency](../GeometryAndResources/MeshAndTextureResidency.md) owns resource state/budgets; [Temporal Sampling and History](../FrameExecution/TemporalSamplingAndHistory.md) owns jitter/history semantics. This page owns only the joins needed for one coherent prepared publication. Candidate results and release disposition remain in [Acceptance reporting](../../../../../../Acceptance/FeatureCompletionReports.md).

## Horizontal Coverage

| Axis | Independently meaningful cells | Shared invariant |
| --- | --- | --- |
| execution | serial coordinator, render thread, reused task graph | one admitted submission produces at most one complete publication; partial task output never escapes |
| view | perspective/orthographic; swapchain/editor viewport; first/continuing frame | scene identity is shared while camera, extent, visibility, display intent, and temporal history remain view-owned |
| geometry | static, instanced, skinned, morphed, combined skin+morph | current/previous deformation, bounds, visibility, GPU payload, and ray work describe one primitive identity |
| residency join | resident versus unavailable/stale mesh and texture generation | preparation/publication consumes only the residency owner's active generation; detailed lifecycle proof stays in its dossier |
| lifecycle | add, update, remove, reset, cut, resize, reload, cancellation, shutdown | named generations invalidate only dependent history and completion owns reclamation |
| backend | D3D12 and Vulkan | prepared CPU semantics and GPU-scene layout agree; native mechanics remain RHI-owned |

## Acceptance Criteria

- `AC-SVP-01` — accepted submissions have strictly increasing frame identity; rejection leaves the prior scene/view state authoritative and publishes no partial replacement.
- `AC-SVP-02` — structural add/update/remove and moved dynamic arrays produce one coherent `RenderScene` generation whose primitives, lights, materials, textures, and ray records share source identities.
- `AC-SVP-03` — serial and threaded preparation produce identical ordered prepared data for static, instanced, skinned, morphed, and combined fixtures.
- `AC-SVP-04` — current/previous transforms, joints, and morph weights preserve continuity across ordinary frames and reset deterministically after discontinuity.
- `AC-SVP-05` — two simultaneous views differ independently in camera, projection, extent, visibility, display settings, batching, and partition plan without cross-talk or duplicated scene authority.
- `AC-SVP-06` — light capacities accept the documented maximum and reject the first excess element before GPU-scene publication with family/count/capacity identified.
- `AC-SVP-07` — preparation and GPU publication consume only the residency owner's active mesh/texture generation; unavailable or stale identities cannot enter a published scene/view binding.
- `AC-SVP-08` — reset, resize, cut, shader/provider/topology change, cancellation, and shutdown consume the temporal owner's invalidation result and retire scene/view slot and GPU-publication resources after completion.
- `AC-SVP-09` — D3D12 and Vulkan consume the same prepared scene/view semantics; backend differences do not alter IDs, counts, transforms, or declared capacity behavior.

## Controlled Failure Modes

| ID | Injection or cause | Safe state | Check |
| --- | --- | --- | --- |
| `FM-SVP-01` | duplicate/decreasing frame ID | reject before mutation; prior generation remains active | `CHK-SVP-01` |
| `FM-SVP-02` | preparation task failure or cancellation before merge | reset affected continuity, publish nothing partial, report failed stage | `CHK-SVP-01` |
| `FM-SVP-03` | one light beyond hard capacity | reject before upload with family/count/capacity | `CHK-SVP-03` |
| `FM-SVP-04` | pending/failed/evicted resource or stale completion | select documented placeholder/refusal or discard stale completion | `CHK-SVP-03`, `CHK-SVP-04` |
| `FM-SVP-05` | viewport identity reuse or concurrent views | invalidate mismatched view work/history; modify no other view | `CHK-SVP-02`, `CHK-SVP-04` |

## Checks And Completion

| Check | Cheapest claim-falsifying exercise | Covers |
| --- | --- | --- |
| `CHK-SVP-01` | submission/scene mutation harness comparing serial/threaded prepared records plus non-monotonic admission | `AC-SVP-01`–`AC-SVP-04`; `FM-SVP-01`, `FM-SVP-02` |
| `CHK-SVP-02` | two-view perspective/orthographic fixture with different extents, visibility, batching, cuts, resize, and reload | `AC-SVP-05`, `AC-SVP-08`; `FM-SVP-05` |
| `CHK-SVP-03` | capacity/boundary-plus-one lights, then consume the pending/fail/evict/stale resource states produced by `CHK-RES-01` through `CHK-RES-03` and inspect the preparation/publication join | `AC-SVP-06`, `AC-SVP-07`; `FM-SVP-03`, `FM-SVP-04` |
| `CHK-SVP-04` | D3D12/Vulkan capture of counts/IDs/transforms/GPU bindings with native validation and completion-drain shutdown | `AC-SVP-08`, `AC-SVP-09`; `FM-SVP-04`, `FM-SVP-05` |

Definition of done: every applicable criterion passes in the candidate report, every controlled failure reaches its safe state, and retained artifacts identify revision, execution mode, backend, scene/view generations, and fixture. Source review or one rendered frame cannot close this contract.

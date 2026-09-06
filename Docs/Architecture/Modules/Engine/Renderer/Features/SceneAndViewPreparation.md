# Renderer Scene and View Preparation

Status: current feature dossier; source-backed, not runtime, concurrency, visibility, memory, or release evidence

Verified: 2026-09-06 against committed `master` revision `d236da11`; `Engine/Renderer` is unchanged from the earlier `8414b5dc` source audit

Scope: `REN-SCENE-01` through `REN-SCENE-10` plus scene/view portions of `REN-OWN-02` through `REN-OWN-04`; defines how immutable world data becomes frame-local prepared scene/view state and GPU-scene bindings

## Feature Contract

GameFramework owns live world/ECS state and publishes `RenderFrameSubmission`. Renderer owns a persistent `RenderScene` mirror, but it never queries ECS storage. Each admitted frame derives a reusable-slot `PreparedRenderScene` and `RenderView`; the persistent `RenderGpuScene` then publishes the buffers and tables consumed by raster and ray passes.

```text
RenderFrameSubmission
  -> RenderScene::Apply(structural delta, moved dynamic data)
  -> RenderScenePreparation -> PreparedRenderScene
  -> RenderViewBuilder + RenderViewPreparation -> RenderView
  -> RenderScene::UpdateGpuScene -> RenderSceneGpuBindings
```

## Ownership And Representations

| Representation | Owner and lifetime | Contains |
| --- | --- | --- |
| `RenderFrameSubmission` | producer-owned immutable handoff until accepted/moved | increasing frame ID, structural scene delta, dynamic scene arrays, view input |
| `RenderScene` | persistent Renderer scene generation | render meshes/materials/textures/lights, continuity, GPU-scene and RT-scene state |
| `PreparedRenderScene` | one RHI frame-in-flight slot | resolved primitives, instance groups, current/previous deformation, prepared lights/sky, GPU-binding pointer |
| `RenderView` | same frame slot, one viewport/view identity | camera matrices/frustum, extents, viewport/scissor, display settings, temporal uniform, visible indices, batches, workload, RT partition plan |
| `RenderViewState` | persistent per active renderer view path | previous camera/temporal identity and invalidation state |
| `RenderSceneGpuBindings` | persistent plus frame-indexed GPU-scene storage | light buffers, instance tables, joint/morph current/previous data, hit geometry/material buffers, descriptor-table bindings |

Scene data and view data are intentionally separate. Frustum visibility, display settings, render/output extents, view mode, temporal identity, and partition planning are view-owned; geometry/material/light/resource identity remains scene-owned.

## Scene Preparation Work

`RenderScenePreparation` resolves inputs, maintains deformation continuity, and reuses a capacity-bucketed Tasks graph. The current graph has four independent producer families followed by merge and ray-plan work:

| Work | Serial threshold / grain / max partitions | Output |
| --- | --- | --- |
| Primitive transforms and bounds | 128 / 64 / 8 | prepared transforms, bounds, draw/material identity |
| Joint-matrix copies | 64 / 16 / 8 | current and previous joint matrices |
| Morph-weight copies | 64 / 16 / 8 | current and previous morph weights |
| Light preparation | 32 / 16 / 4 | directional, point, spot, and rect GPU-ready semantics |

Capacities round up to a power of two beyond the serial threshold so the compiled task graph can be reused. These are scheduling constants, not proven optimal workloads. If execution fails, continuity resets and no partial prepared scene is published; successful execution commits continuity before merged output moves to the frame slot.

## View Preparation Work

`RenderViewBuilder` resolves perspective or orthographic camera matrices under the engine coordinate/depth convention, sets render/output extents, viewport/scissor, display settings and debug mode, and asks `RenderViewState` for temporal data keyed by viewport/selection/kind plus frame, scene, shader, provider, and graph-topology generations.

`RenderViewPreparation` evaluates primitive frustum visibility in a reusable Tasks graph (threshold 128, grain 64, at most 8 partitions), classifies material alpha as opaque, alpha-tested, transparent, or rejected, produces visible raster indices, forms compatible instance batches when `r.MeshAutoBatching` is enabled, reports workload counts, and builds the view-relative RT partition plan. Transparency classification does not imply a completed transparent rendering feature.

## GPU-Scene Publication

The GPU scene updates after both scene and view preparation so it can publish one coherent set of:

- directional, point, spot, and rect light buffers;
- mesh instances and compact instance-slot indirection;
- current/previous joint matrices and morph weights;
- ray hit vertices, indices, skin influences, morph deltas, hit instances, and hit materials;
- material texture table/binding state shared by ray consumers;
- per-view RT instance/partition data and TLAS preparation inputs.

This ordering ensures GBuffer, lighting, and RT scene passes consume the same prepared scene identity. It does not prove raster/ray deformation parity or total residency bounds.

## History, Failure, And Limits

- Non-monotonic frame IDs are rejected before a new frame identity is published.
- Scene reset unloads scene textures and invalidates view/provider/frame history.
- Missing/not-yet-resident resources must resolve through explicit cache/placeholder policy; correctness and visual behavior need runtime evidence.
- Light payload hard limits are 2 directional and 1024 each for point, spot, and rect; overflow is a validation failure, not unlimited support.
- Auto batching groups compatible flat mesh instances; its draw-count benefit, ordering behavior, and CPU cost are unmeasured.
- Static BLAS reuse exists; deforming ray geometry rebuilds rather than claiming BLAS refit.
- Per-operation task partition limits do not establish total CPU memory, decoded-asset memory, or GPU residency bounds.

Primary evidence: `REN-E02`, `REN-E11`, `REN-E20`, `GF-E03`, and `GF-E05`. See [Geometry, Materials, and GBuffer](GeometryMaterialsAndGBuffer.md) for how prepared data becomes surface output.

## Horizontal Coverage

| Axis | Current cells that require independent treatment | Shared invariant |
| --- | --- | --- |
| Execution | serial coordinator, render thread, reused task graph | one admitted submission produces at most one published prepared scene; no partial task output escapes |
| View | perspective, orthographic; swapchain and editor viewport; first frame and continuing view | scene identity remains shared while camera, extents, visibility, display intent, and temporal history remain view-owned |
| Geometry | static, flat-instanced, skinned, morphed, combined skin+morph | current and previous deformation, bounds, visibility, GPU-scene payload, and ray work describe the same primitive identity |
| Residency | resident, pending, failed, evicted/reloaded; mesh and texture | placeholder/refusal behavior is explicit and never reuses stale resource identity as current |
| Lifecycle | add, update, remove, scene reset, camera cut, resize, level reload, shutdown | generations invalidate affected history and completion owns reclamation |
| Backend | D3D12 and Vulkan | prepared CPU data and semantic GPU-scene layout agree; backend resource mechanics remain RHI-owned |

Cells may share one check when they exercise the same owner and oracle. Static success cannot stand in for deformation continuity, multi-view isolation, capacity overflow, or reset/reload behavior.

## Acceptance Criteria

- `AC-SVP-01` — accepted submissions have strictly increasing frame identity; rejection leaves the prior scene/view state authoritative and publishes no partial replacement.
- `AC-SVP-02` — structural add/update/remove and moved dynamic arrays produce one coherent `RenderScene` generation whose prepared primitives, lights, materials, textures, and ray records refer to the same source identities.
- `AC-SVP-03` — serial and threaded preparation produce identical ordered prepared data for static, instanced, skinned, morphed, and combined skin+morph fixtures.
- `AC-SVP-04` — current/previous transforms, joints, and morph weights preserve motion continuity across ordinary frames and reset deterministically after discontinuity.
- `AC-SVP-05` — two simultaneous views can differ in camera, projection, extent, visibility, display settings, batching, and partition plan without cross-talk or duplicated scene authority.
- `AC-SVP-06` — light capacities accept the exact documented maximum and reject the first excess element before GPU-scene publication with the affected light family identified.
- `AC-SVP-07` — pending, failed, evicted, and reloaded mesh/texture resources follow a visible placeholder or refusal policy; no stale handle is reported as resident/current.
- `AC-SVP-08` — scene reset, resize, camera cut, shader/provider/topology change, cancellation, and shutdown invalidate only their owned histories and retire all slot/GPU resources after completion.
- `AC-SVP-09` — D3D12 and Vulkan consume the same prepared scene/view semantics for the representative matrix; backend differences do not alter IDs, counts, transforms, or declared capacity behavior.

## Controlled Failure Modes

| ID | Injection or cause | Detection boundary and safe state | Affected criteria |
| --- | --- | --- | --- |
| `FM-SVP-01` | duplicate or decreasing frame ID | admission rejects before mutation; prior generation remains active | `AC-SVP-01` |
| `FM-SVP-02` | a preparation task throws/fails or cancellation arrives before merge | preparation owner resets continuity, publishes nothing partial, and reports the failed stage | `AC-SVP-02`–`AC-SVP-04`, `AC-SVP-08` |
| `FM-SVP-03` | one more light than a hard payload capacity | validation rejects the frame before upload and names family/count/capacity | `AC-SVP-06` |
| `FM-SVP-04` | resource is pending, fails decode/upload, is evicted, or completes after scene generation changed | cache/generation check selects documented placeholder/refusal or discards stale completion | `AC-SVP-07`, `AC-SVP-08` |
| `FM-SVP-05` | viewport identity is reused across resize/reload or two views prepare concurrently | view generation mismatch invalidates history/work; no other view is modified | `AC-SVP-05`, `AC-SVP-08` |

## Checks And Completion

| Check | Cheapest claim-falsifying exercise | Covers |
| --- | --- | --- |
| `CHK-SVP-01` | focused submission/scene mutation harness comparing serial and threaded prepared records plus deliberate non-monotonic admission | `AC-SVP-01`–`AC-SVP-04`; `FM-SVP-01`, `FM-SVP-02` |
| `CHK-SVP-02` | two-view fixture spanning perspective/orthographic, different extents, visibility, batching, cuts, resize, and reload; compare identities and history validity per frame | `AC-SVP-05`, `AC-SVP-08`; `FM-SVP-05` |
| `CHK-SVP-03` | exact-boundary and boundary-plus-one light fixtures plus mesh/texture pending/fail/evict/stale-completion injections | `AC-SVP-06`, `AC-SVP-07`; `FM-SVP-03`, `FM-SVP-04` |
| `CHK-SVP-04` | focused D3D12/Vulkan runtime capture of prepared counts/IDs/transforms and GPU-scene bindings, with native validation and completion-drain shutdown | `AC-SVP-08`, `AC-SVP-09`; `FM-SVP-04`, `FM-SVP-05` |

This contract is **defined but unproved**. Completion requires all applicable criteria to pass in the candidate report, every controlled failure to be observed in its safe state, and retained artifacts to identify revision, execution mode, backend, scene/view generations, and exact fixture. A source review or one rendered frame cannot close it.

## Primary Source Routes

- [`FramePipeline::AcceptFrameSubmission` and `PrepareRenderFrame`](../../../../../../Engine/Renderer/Private/Frame/FramePipeline.cpp)
- [`RenderScenePreparation.cpp`](../../../../../../Engine/Renderer/Private/Scene/Preparation/RenderScenePreparation.cpp)
- [`RenderViewBuilder.cpp`](../../../../../../Engine/Renderer/Private/View/RenderViewBuilder.cpp)
- [`RenderViewPreparation.cpp`](../../../../../../Engine/Renderer/Private/View/RenderViewPreparation.cpp)
- [`RenderGpuScene.cpp`](../../../../../../Engine/Renderer/Private/Scene/GpuScene/RenderGpuScene.cpp)
- [`RenderFrameSubmission.h`](../../../../../../Engine/GameFramework/Public/Rendering/RenderFrameSubmission.h)

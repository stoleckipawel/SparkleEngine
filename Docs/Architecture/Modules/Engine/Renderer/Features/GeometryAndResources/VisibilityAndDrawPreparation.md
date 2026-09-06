# Renderer Visibility And Draw Preparation

Status: current feature dossier; source-backed, not visibility correctness, batching benefit, transparency, multi-view, or performance evidence

Verified: 2026-09-07 at committed `master` revision `c28b33bd`

Scope: `REN-VIS-01` through `REN-VIS-09`; per-view frustum visibility, material classification, candidate validation, authored-group preservation, opaque sorting/automatic batching, transparent ordering, workload publication, task capacity, and explicit absent visibility/draw-generation paths

Parents and consumers: [Scene and View Preparation](../SceneAndViewPreparation/README.md) owns scene/view construction; [Geometry, Materials, and GBuffer](GeometryMaterialsAndGBuffer.md) owns what accepted batches draw and publish

## Feature Promise And Motivation

For one prepared scene and one view, Renderer turns prepared primitives into a deterministic list of valid raster instance indices and compatible draw batches. It rejects invalid identity, removes bounds-frustum misses, preserves eligible authored grouping, reduces compatible opaque draw changes when enabled, and orders transparent candidates conservatively as single batches.

This boundary exists so scene preparation does not become view policy and the GBuffer pass does not rediscover visibility, material routing, or batching. It also gives ordering and rejected-work behavior an explicit proof contract; a lower draw count is not useful if it changes object/material identity or transparency order.

## Vertical Trace

```text
PreparedRenderScene primitives + instance groups
  + RenderView frustum/camera
  + r.MeshAutoBatching
  -> bounded parallel visibility/material classification
  -> discard rejected and frustum-missed items
  -> validate draw index, GPU mesh, group and material binding identity
  -> preserve compatible non-transparent authored/shared groups
  -> stable-sort remaining opaque/alpha-tested items by batch identity
  -> form compatible automatic batches or single batches
  -> stable-sort transparent candidates back-to-front and keep them single
  -> publish rasterPrimitiveIndices + meshInstanceBatches + workload
  -> raster GBuffer batch drawer
```

On visibility-task failure, view preparation clears raster indices, batches, and workload and publishes no partial list. Ray-tracing scene/planning has its own inclusion and acceleration-structure contract; a raster visibility decision is not automatically an RT instance-culling decision.

## Current Visibility And Classification Rules

| Concern | Current behavior | Important boundary |
| --- | --- | --- |
| primitive domain | one item per prepared primitive | input identity is the prepared scene generation |
| alpha classification | `0` opaque, `1` alpha-tested, `2` transparent, other rejected | classification is routing vocabulary; transparent draw preparation is not proof of a complete blended-transparency feature |
| spatial test | world AABB against six view-frustum planes using the positive vertex | no occlusion, portal, cluster, or hardware-query result participates |
| invalid bounds | conservatively visible; camera distance uses world translation | prevents accidental disappearance but can increase work and weaken transparent sorting |
| task graph | capacity at least 128, otherwise next power of two of bounded primitive count; grain 64, serial threshold 128, at most 8 partitions | values are current implementation limits, not measured optimal policy |
| task failure | clear raster indices, batches and workload | no partial result may reach the GBuffer drawer |

## Candidate Validation And Batch Identity

A candidate is raster-submittable only when its draw index names a prepared primitive, its prepared draw has a GPU mesh, any instance-group index is in range, its material binding exists when required, and its material classification is not rejected.

Two non-transparent candidates may share a batch only when all current batch-key fields agree:

- material classification and render-state key;
- material index and generation;
- GPU mesh handle;
- material slot;
- skeleton asset identity;
- mesh kind.

The object identity breaks ties after equal opaque keys, making the output deterministic for a fixed input. Any field newly consumed by the batch drawer or shader that changes per draw/instance must either enter this compatibility contract or be proven safely instance-addressed.

## Grouping, Sorting, And Publication

| Path | Ordering/grouping rule | Output consequence |
| --- | --- | --- |
| authored/shared group | compatible group with at least two visible non-transparent items is preserved as one batch | group source remains distinguishable as authored or preserved |
| incompatible group | not preserved; candidates remain available for ordinary sorting/batching | diagnostic counter exists when diagnostic collection is requested |
| remaining opaque/alpha-tested, auto batching on | stable sort by complete batch key then object; contiguous equal keys form one batch | instance indices are appended in sorted order and one batch records first/count |
| remaining opaque/alpha-tested, auto batching off | same deterministic sort, one instance per batch | selector changes grouping, not visibility/classification |
| transparent | stable far-to-near squared camera distance, then object; one instance per batch | transparent items never join preserved or automatic batches |

`RenderView` publishes raster primitive indices, mesh batches, counts of static/skinned visible instances and batches, and the separate RT partition plan. Diagnostic batch-building can count candidates, rejections, group sources, submitted instances, batch sizes, and estimated GBuffer draw calls saved, but the normal view-preparation path currently requests `CollectDiagnostics = false`. The existence of counters is not proof that production telemetry exposes them.

## Horizontal Coverage And Explicit Non-Claims

| Axis | Current cells | Required distinction |
| --- | --- | --- |
| material | opaque, alpha-tested, transparent, rejected | only opaque/alpha-tested have a complete deferred surface claim; transparent candidates remain a partial path |
| geometry | static, skeletal, morph-deformed prepared primitives | compatibility includes mesh kind/skeleton identity; deformation correctness belongs to geometry/GBuffer |
| grouping | authored, preserved shared-mesh reference, automatic, single | group preservation and auto batching need separate counts and equivalence oracles |
| bounds | valid inside/intersecting/outside; invalid | invalid is conservative visible, never silently culled |
| selector | auto batching on/off | must produce identical visible surface identity for eligible opaque work |
| view | current game/editor view identity, perspective/orthographic | state is per view; no active stereo/multiview family or view-family batching route was found |
| backend | common CPU preparation before D3D12/Vulkan lowering | identical lists are expected for identical prepared inputs; GPU results still need backend proof |

No current Renderer route was found for occlusion culling, HZB queries, portals, meshlet/cluster culling, LOD selection, GPU-driven draw generation, indirect draw/execute-indirect, multi-draw, stereo instancing, or multiview rendering. RHI or shader vocabulary must not be used to claim those features.

## Acceptance Criteria

- `AC-VIS-01` — analytic AABB/frustum cases, including all planes, boundary contact, large/small coordinates, transformed/deforming bounds, invalid bounds, and perspective/orthographic views, produce the declared visible set deterministically.
- `AC-VIS-02` — all material classifications and invalid draw/mesh/group/material identities produce the declared accepted/rejected set without out-of-range access or partial publication.
- `AC-VIS-03` — compatible authored and shared groups remain grouped; incompatible groups safely fall back without dropping, duplicating, or re-identifying instances.
- `AC-VIS-04` — automatic batching on/off produces identical opaque/alpha-tested GBuffer values, depth, motion and object/material identity while batch/draw counts change only as declared.
- `AC-VIS-05` — opaque sorting is deterministic from the complete batch key and object tie-break; changing any compatibility field cannot accidentally merge unlike work.
- `AC-VIS-06` — transparent candidates are stable far-to-near singles with deterministic equal-distance tie-breaks and never enter preserved/automatic batches; this criterion does not approve blended transparency.
- `AC-VIS-07` — task graph serial/parallel thresholds, partition counts, primitive counts at zero/one/127/128/129/power and large boundaries, cancellation, and executor failure yield equivalent complete results or a completely cleared failure result.
- `AC-VIS-08` — workload and optional batch diagnostics exactly reconcile candidates, rejections, submitted indices, batches, batch sources, instances, and estimated saved draws.
- `AC-VIS-09` — absent occlusion/LOD/GPU-driven/indirect/stereo/multiview paths remain unreachable and unadvertised until assigned their own owner, selector, fallback, criteria, and evidence.

## Controlled Failure Modes And Checks

| Failure ID | Injection or cause | Required safe behavior | Detecting check |
| --- | --- | --- | --- |
| `FM-VIS-01` | bounds plane/sign/transform defect or invalid bounds | analytic oracle identifies mismatch; invalid bounds remain conservative | `CHK-VIS-01` |
| `FM-VIS-02` | stale/out-of-range draw, mesh, material, or group identity | candidate rejected; no partial or aliased identity enters a batch | `CHK-VIS-02` |
| `FM-VIS-03` | compatibility key omits a shader/draw-varying field | auto-batching equivalence/product-identity oracle fails | `CHK-VIS-03` |
| `FM-VIS-04` | unstable/equal-key ordering or transparent item grouped | exact ordered index/batch ledger differs from oracle | `CHK-VIS-02`, `CHK-VIS-03` |
| `FM-VIS-05` | visibility task cancellation/failure/partition boundary | view publishes empty raster indices/batches/workload, never a prefix | `CHK-VIS-04` |
| `FM-VIS-06` | absent visibility/draw mode becomes reachable | capability/selector/source audit blocks claim until independently owned | `CHK-VIS-05` |

| Check | Cheapest claim-falsifying exercise | Covers |
| --- | --- | --- |
| `CHK-VIS-01` | pure CPU frustum/bounds table with perspective/orthographic and invalid/adversarial transforms | `AC-VIS-01`; `FM-VIS-01` |
| `CHK-VIS-02` | deterministic synthetic prepared-scene table spanning classifications, invalid identities, groups, equal keys and equal distances; compare exact index/batch ledger | `AC-VIS-02`, `AC-VIS-03`, `AC-VIS-05`, `AC-VIS-06`; `FM-VIS-02`, `FM-VIS-04` |
| `CHK-VIS-03` | render/capture the same eligible scene with batching on/off and compare raw GBuffer/depth/motion/identity plus draw/batch diagnostics | `AC-VIS-04`, `AC-VIS-05`, `AC-VIS-06`, `AC-VIS-08`; `FM-VIS-03`, `FM-VIS-04` |
| `CHK-VIS-04` | serial/parallel boundary and cancellation/failure injection; compare exact successful outputs or exact cleared state | `AC-VIS-07`, `AC-VIS-08`; `FM-VIS-05` |
| `CHK-VIS-05` | enumerate selectors, public contracts, passes, command calls and shader registrations for occlusion/LOD/indirect/stereo/multiview reachability | `AC-VIS-09`; `FM-VIS-06` |

This contract is **defined but unproved**. A lower CPU draw count, a visible scene, or deterministic source sort is not execution, visual-equivalence, or performance evidence.

Primary evidence destination: `REN-E32` in the [Capability Evidence Plan](../../../../../../Plans/CapabilityEvidence.md#renderer-capability-to-evidence-map).

## Primary Source Routes

- [`RenderViewPreparation.cpp`](../../../../../../../Engine/Renderer/Private/View/RenderViewPreparation.cpp)
- [`MeshInstanceBatchBuilder.cpp`](../../../../../../../Engine/Renderer/Private/View/MeshInstanceBatchBuilder.cpp)
- `Engine/Renderer/Private/View/RenderView.h`, `MeshInstanceBatchBuilder.h`, and prepared-scene contracts
- [`GBufferMeshBatchDrawer.cpp`](../../../../../../../Engine/Renderer/Private/Passes/GBuffer/GBufferMeshBatchDrawer.cpp)
- [Scene/View Acceptance](../SceneAndViewPreparation/Acceptance.md) and [Geometry, Materials, and GBuffer](GeometryMaterialsAndGBuffer.md)

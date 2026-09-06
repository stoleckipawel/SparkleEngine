# Renderer Scene and View Preparation

Status: current feature-family dossier; source-backed, not runtime, concurrency, visibility, memory, or release evidence

Verified: 2026-09-06 against committed `master` revision `d236da11`; `Engine/Renderer` is unchanged from the earlier `8414b5dc` source audit

Scope: `REN-SCENE-01` through `REN-SCENE-10` plus scene/view portions of `REN-OWN-02` through `REN-OWN-04`; routes the three distinct owners that turn immutable world data into prepared scene, prepared view, and published GPU-scene state

## Family Contract

GameFramework owns live world/ECS state and publishes `RenderFrameSubmission`. Renderer never queries ECS storage. One admitted submission updates the persistent scene, derives frame-slot scene and view state, then publishes one GPU-scene generation consumed by raster and ray passes.

```text
RenderFrameSubmission
  -> RenderScene::Apply
  -> RenderScenePreparation -> PreparedRenderScene
  -> RenderViewBuilder + RenderViewPreparation -> RenderView
  -> RenderScene::UpdateGpuScene -> RenderSceneGpuBindings
```

## Independent Owners

| Owner | Lifetime and result | Document |
| --- | --- | --- |
| `RenderScene` and `RenderScenePreparation` | persistent scene generation plus one frame-slot prepared scene; structural/dynamic inputs become resolved primitives, deformation continuity, lights, and references to active resources | [Scene Preparation](ScenePreparation.md) |
| `RenderViewBuilder` and `RenderViewPreparation` | one view/frame-slot result; camera/output intent and shared temporal input become matrices, visibility, batches, and RT partition plan | [View Preparation](ViewPreparation.md) |
| `RenderGpuScene` and `RenderSceneGpuBindings` | persistent plus frame-indexed GPU storage; prepared semantic data becomes coherent buffers/tables for raster and ray consumers | [GPU-Scene Publication](GpuScenePublication.md) |
| feature family | cross-owner identity, failure, backend, reset, capacity, and completion requirements | [Acceptance](Acceptance.md) |

These pages are separate because their state owners, lifetimes, outputs, and invalidation reasons differ. They remain one family because the publication invariant allows no mixed scene/view/GPU generation to escape.

## Shared Invariants

- Scene data and view data remain separate: geometry/material/light/resource identity is scene-owned; camera, frustum, extents, display intent, temporal identity, and partition planning are view-owned.
- A failed/cancelled preparation publishes none of its partial outputs. Successful scene and view preparation must agree on the admitted frame and scene generation before GPU publication.
- Scene reset, view discontinuity, resource completion, provider/shader changes, and graph-topology changes invalidate only their named histories/generations.
- GPU completion, not CPU scope exit, authorizes frame-slot and GPU-resource reuse.

Mesh/texture lifecycle is owned by [Mesh and Texture Residency](../GeometryAndResources/MeshAndTextureResidency.md). Per-view culling, classification, sorting, and batching semantics are owned by [Visibility and Draw Preparation](../GeometryAndResources/VisibilityAndDrawPreparation.md). Per-view sampling and history semantics are owned by [Temporal Sampling and History](../FrameExecution/TemporalSamplingAndHistory.md). This family consumes those active identities without duplicating their state machines or proof contracts.

See [Geometry, Materials, and GBuffer](../GeometryAndResources/GeometryMaterialsAndGBuffer.md) for the next semantic consumer and [Rendering a Sparkle Frame](../../RenderingASparkleFrame.md) for whole-frame order.

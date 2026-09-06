# Renderer View Preparation

Status: current feature dossier; source-backed, not visibility, batching, temporal, multi-view, or runtime evidence

Verified: 2026-09-06 against committed `master` revision `d236da11`

Scope: `REN-OWN-03`, `REN-SCENE-10`, and view-owned preparation; camera/output intent, temporal identity, culling, material visibility classes, raster batches, workload counts, and RT partition planning

## Feature Promise

For one prepared scene and one viewport/view identity, Renderer produces one frame-slot `RenderView` without mutating scene authority or another view. Camera, output, visibility, display intent, temporal history, batching, and RT partition decisions remain view-owned.

## Build And Prepare

`RenderViewBuilder` resolves perspective or orthographic matrices under the engine coordinate/depth convention, render/output extents, viewport/scissor, display settings, debug mode, and temporal data. `RenderViewState` keys continuity by viewport/selection/kind plus frame, scene, shader, provider, and graph-topology generations.

`RenderViewPreparation` evaluates frustum visibility through a reusable Tasks graph (threshold 128, grain 64, at most 8 partitions), classifies material alpha as opaque, alpha-tested, transparent, or rejected, produces visible raster indices, optionally forms compatible flat-instance batches through `r.MeshAutoBatching`, records workload counts, and builds a view-relative RT partition plan.

Transparency classification is routing vocabulary, not evidence of a completed transparent rendering feature. Automatic batching is a current path, but its draw-count benefit, ordering behavior, and CPU/GPU cost remain unmeasured.

## Ownership, Failure, And Invalidation

- Two views may share one scene generation while owning different cameras, extents, visibility, display settings, temporal state, batches, and partition plans.
- Camera cut, resize, view selection/kind change, scene reload, shader/provider generation, and graph topology invalidate their owned temporal work deterministically.
- Failed/cancelled culling or batching publishes no partial `RenderView`; another view is never modified.
- View preparation cannot promote missing scene resources, exceed scene-owned capacities, or redefine material/geometry identity.

Feature-family proof is owned by [Acceptance](Acceptance.md), especially `AC-SVP-03`, `AC-SVP-05`, `AC-SVP-08`, `AC-SVP-09`, and `CHK-SVP-02`/`04`.

## Primary Source Routes

- [`RenderViewBuilder.cpp`](../../../../../../../Engine/Renderer/Private/View/RenderViewBuilder.cpp)
- [`RenderViewPreparation.cpp`](../../../../../../../Engine/Renderer/Private/View/RenderViewPreparation.cpp)
- `Engine/Renderer/Private/View`, `Engine/Renderer/Private/Temporal`, and `Engine/Renderer/Private/Settings`

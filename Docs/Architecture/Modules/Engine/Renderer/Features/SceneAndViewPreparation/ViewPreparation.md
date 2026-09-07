# Renderer View Preparation

**Status:** current feature dossier; source-backed, not visibility, batching, temporal, multi-view, or runtime evidence

**Verified:** 2026-09-06 against committed `master` revision `d236da11`

**Scope:** `REN-OWN-03`, `REN-SCENE-10`, and view-owned preparation; camera/output intent, temporal identity, culling, material visibility classes, raster batches, workload counts, and RT partition planning

**Current readiness:** **45/100** — per-view camera/visibility/display/history preparation exists; multi-view isolation, edge cases, failure, capacity, equivalence, and cost evidence does not. See [Current Feature Readiness](../../../../../../Acceptance/CurrentReadiness.md#renderer).

## At A Glance

| View-owned input | Derived result | Kept separate from scene |
| --- | --- | --- |
| perspective/orthographic camera and world/view/projection convention | current matrices, frustum, viewport, and scissor | camera never mutates persistent geometry/material/light identity |
| requested render/output extent and display settings | resolved extent, display intent, and debug mode | output policy can differ between views sharing one scene |
| persistent per-view temporal state | jitter, previous matrices, history-validity input | one view cannot reuse another view's history |
| prepared scene primitives/groups | visible indices, material classes, deterministic batches, and workload counts | rejection/batching is per-view policy |
| ray-tracing capability and view plan | per-view RT partition/instance plan | RHI mechanisms and shared scene data retain their own owners |

The current route prepares one view at a time from shared scene state. It does not establish stereo, multiview, or view-family batching; those remain explicit absent capabilities.

## Feature Promise

For one prepared scene and one viewport/view identity, Renderer produces one frame-slot `RenderView` without mutating scene authority or another view. Camera, output, visibility, display intent, temporal history, batching, and RT partition decisions remain view-owned.

## Build And Prepare

`RenderViewBuilder` resolves perspective or orthographic matrices under the engine coordinate/depth convention, render/output extents, viewport/scissor, display settings, debug mode, and the temporal data produced by [Temporal Sampling and History](../FrameExecution/TemporalSamplingAndHistory.md). That dossier owns `RenderViewState`, jitter, previous matrices, history validity, and invalidation semantics; view preparation only consumes their current result.

`RenderViewPreparation` produces the visible raster indices, compatible batches, workload counts, and view-relative RT partition plan. [Visibility and Draw Preparation](../GeometryAndResources/VisibilityAndDrawPreparation.md) owns the exact frustum, classification, validation, authored-group, sort/batch, task-capacity, failure, and absent advanced-culling contract.

Transparency classification is routing vocabulary, not evidence of a completed transparent rendering feature. Automatic batching is a current path, but its draw-count benefit, ordering behavior, and CPU/GPU cost remain unmeasured.

## Ownership, Failure, And Invalidation

- Two views may share one scene generation while owning different cameras, extents, visibility, display settings, temporal state, batches, and partition plans.
- Camera cut, resize, view selection/kind change, scene reload, shader/provider generation, and graph topology invalidate their owned temporal work deterministically.
- Failed/cancelled culling or batching publishes no partial `RenderView`; another view is never modified.
- View preparation cannot promote missing scene resources, exceed scene-owned capacities, or redefine material/geometry identity.

## Design Decisions And Tradeoffs

| Decision | Benefit | Cost or risk |
| --- | --- | --- |
| Keep all camera/output/visibility state view-owned | Multiple editor/game views can share scene identity without contaminating each other | Every cache/history/batch needs a stable view key and invalidation rules |
| Consume prepared scene rather than live ECS | Rendering input is immutable and thread-safe | World changes appear only through the next admitted submission |
| Publish no partial view after task failure | Downstream passes never see a prefix of visibility/batches | A local preparation failure drops the whole view result |
| Keep visibility/batching in a dedicated child contract | Scene/view ownership stays understandable | Reviewers must join this page with the detailed visibility dossier |

Feature-family proof is owned by [Acceptance](Acceptance.md), especially `AC-SVP-03`, `AC-SVP-05`, `AC-SVP-08`, `AC-SVP-09`, and `CHK-SVP-02`/`04`. Exact temporal proof stays in [Temporal Sampling and History](../FrameExecution/TemporalSamplingAndHistory.md).

## Primary Source Routes

- [`RenderViewBuilder.cpp`](../../../../../../../Engine/Renderer/Private/View/RenderViewBuilder.cpp)
- [`RenderViewPreparation.cpp`](../../../../../../../Engine/Renderer/Private/View/RenderViewPreparation.cpp)
- `Engine/Renderer/Private/View`, `Engine/Renderer/Private/Temporal`, and `Engine/Renderer/Private/Settings`

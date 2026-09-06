# Renderer Scene Preparation

Status: current feature dossier; source-backed, not task, continuity, residency, capacity, or runtime evidence

Verified: 2026-09-06 against committed `master` revision `d236da11`

Scope: scene-owned portions of `REN-SCENE-02` through `REN-SCENE-09`; persistent `RenderScene` mutation, frame-slot preparation, deformation continuity, light preparation, resource state, and failure before publication

## Feature Promise

An admitted immutable submission updates one persistent Renderer scene generation, then produces one complete `PreparedRenderScene`. The owner commits deformation continuity only after all required work succeeds; failure or cancellation leaves no partial prepared scene visible.

## Ownership And Work

`RenderScene` consumes structural add/update/remove changes and moved dynamic arrays while retaining geometry, material, texture, light, GPU-scene, and ray-scene identity. `PreparedRenderScene` belongs to one RHI frame-in-flight slot and contains resolved primitives, instance groups, current/previous deformation, prepared lights/sky, and the eventual GPU-binding pointer.

`RenderScenePreparation` reuses a capacity-bucketed Tasks graph:

| Work | Serial threshold / grain / max partitions | Result |
| --- | --- | --- |
| primitive transforms and bounds | 128 / 64 / 8 | prepared transforms, bounds, draw/material identity |
| joint-matrix copies | 64 / 16 / 8 | current and previous joint matrices |
| morph-weight copies | 64 / 16 / 8 | current and previous morph weights |
| light preparation | 32 / 16 / 4 | directional, point, spot, and rect GPU-ready semantics |

Capacities round to a power of two beyond the serial threshold to reuse the compiled task graph. These are scheduling constants, not proven optimal workloads or total memory bounds.

## Failure, Capacity, And Lifetime

- Non-monotonic frame identity rejects before scene mutation/publication.
- Preparation failure/cancellation resets continuity and publishes no partial result; success commits continuity before moving merged output into the frame slot.
- Scene reset unloads scene textures and invalidates dependent scene/view/provider/frame history.
- Missing, pending, failed, evicted, or stale-completing mesh/texture resources follow explicit placeholder/refusal/generation policy.
- Light payload limits are 2 directional and 1024 each for point, spot, and rect; overflow rejects before GPU upload.
- Static BLAS reuse and deforming BLAS rebuild are downstream RT consequences; this page does not claim BLAS refit.

Feature-family proof is owned by [Acceptance](Acceptance.md), especially `AC-SVP-01` through `AC-SVP-04`, `AC-SVP-06` through `AC-SVP-08`, and `CHK-SVP-01`/`03`/`04`.

## Primary Source Routes

- [`FramePipeline::AcceptFrameSubmission` and `PrepareRenderFrame`](../../../../../../../Engine/Renderer/Private/Frame/FramePipeline.cpp)
- [`RenderScenePreparation.cpp`](../../../../../../../Engine/Renderer/Private/Scene/Preparation/RenderScenePreparation.cpp)
- [`RenderFrameSubmission.h`](../../../../../../../Engine/GameFramework/Public/Rendering/RenderFrameSubmission.h)

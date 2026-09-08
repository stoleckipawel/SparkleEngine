# Renderer Deferred Decals

**Status:** first-release target dossier plus source-backed absence record; not implementation proof or release approval

**Snapshot:** absence and extension seams rechecked 2026-09-06 against source revision `d236da11`; Renderer/RHI/shader source is unchanged from the earlier `8414b5dc` audit

**Scope:** `REN-DECAL-01` through `REN-DECAL-03`; current GBuffer, material, ray-hit, frame-graph, blend-state, and authored/runtime seams relevant to deferred decals, including the explicit absence of an implemented decal feature

**Target architecture:** [Deferred GBuffer Decal Composition Architecture](CompositionArchitecture.md)

**Delivery authority:** [Deferred GBuffer Decals Delivery Plan](../../../../../../Plans/Renderer/DeferredGBufferDecals.md)

**Feature acceptance:** [Deferred GBuffer Decals — Acceptance](Acceptance.md)

**Release admission:** mandatory `FCR-REN-23`; staged by [`GR-5`](../../../../../../Plans/FirstRelease/Renderer/GeometryAndRayTracing.md#gr-5--deferred-gbuffer-decals)

**Current readiness:** **0/100**, `Blocked` — release-admitted but no authored/cooked/scene/GPU decal data, GBuffer composition, or arbitrary ray-hit decal evaluation was found. See [Current Feature Readiness](../../../../../../Acceptance/CurrentReadiness.md#renderer).

## At A Glance

| Required capability | Current state | Existing seam is not the feature |
| --- | --- | --- |
| authored/imported/cooked decal data | Not found | ordinary materials and mesh components do not describe projected decal volumes |
| scene/GPU-scene decal publication | Not found | the existing material texture table carries base-material data only |
| primary raster/ray GBuffer composition | Not found | blend-state support cannot express per-field GBuffer composition by itself |
| arbitrary secondary-ray decal evaluation | Not found | current ray-hit material reconstruction evaluates the base material only |

The selected target is one material-overlay contract shared by raster and ray visibility, with composition before lighting. Release admission makes it mandatory work; until all four rows have real owners and reachability, this page remains a target plus negative-capability record rather than a support claim.

Code and executable build configuration remain authoritative. Reinspect every listed owner and absence before using this dated snapshot for implementation or release claims.

## Source-Backed Snapshot

Deferred decals are **not implemented**. The three inventory rows separate the absent user capability from useful extension seams:

| Capability | Current state | Meaning |
| --- | --- | --- |
| `REN-DECAL-01` authored and scene decal data | Not found | No component, imported/cooked decal record, render-scene table, GPU-scene payload, selector, or editor authoring path was found. |
| `REN-DECAL-02` primary deferred composition | Not found | No decal render/compute pass modifies the raster or ray-produced GBuffer before lighting. |
| `REN-DECAL-03` secondary-ray decal evaluation | Not found | Arbitrary ray hits evaluate the base material only; no decal candidate lookup or composition is present. |

The [composition architecture](CompositionArchitecture.md) is the selected first-release contract, not a fourth current rendering branch. Its plan is active in release sequencing, while its acceptance route remains unproved until implementation and candidate evidence exist.

The design extends the existing owner instead of adding a second renderer path:

- [`Passes/GBuffer/GBuffer.cpp`](../../../../../../../Engine/Renderer/Private/Passes/GBuffer/GBuffer.cpp) creates one `GBufferRenderTargets` set and selects either rasterization or the one immutable ray-tracing GBuffer execution plan. Inline and pipeline frontends share that target set and meet before sky motion vectors and device-depth linearization.
- [`GBufferFormats.h`](../../../../../../../Engine/Renderer/Private/Passes/GBuffer/GBufferFormats.h) defines the shared BaseColor, Normal, Material, Emissive, Subsurface, DeviceZ, and MotionVector products.
- [`GBufferPS.hlsl`](../../../../../../../Engine/Assets/Shaders/Passes/GBuffer/GBufferPS.hlsl) and [`RayTracingGBufferCommon.hlsli`](../../../../../../../Engine/Assets/Shaders/Passes/RayTracing/RayTracingGBufferCommon.hlsli) share [`GBufferPacking.hlsli`](../../../../../../../Engine/Assets/Shaders/Passes/GBuffer/GBufferPacking.hlsli) through the canonical `/Engine` virtual shader namespace. [`RayTracingGBufferInline.hlsl`](../../../../../../../Engine/Assets/Shaders/Passes/RayTracing/RayTracingGBufferInline.hlsl) and [`RayTracingGBufferPipeline.hlsl`](../../../../../../../Engine/Assets/Shaders/Passes/RayTracing/RayTracingGBufferPipeline.hlsl) are thin traversal/stage frontends over that common semantic owner.
- [`MaterialCache.cpp`](../../../../../../../Engine/Renderer/Private/Scene/Materials/MaterialCache.cpp) resolves semantic defaults, per-material raster tables, and one scene-wide material texture table beneath the persistent render-scene authority. The latter must remain a scene-material capability, not be described as ray-tracing-only.
- [`RayTracingMaterialHit.hlsli`](../../../../../../../Engine/Assets/Shaders/RayTracing/RayTracingMaterialHit.hlsli) is the central base-material reconstruction path for arbitrary ray hits. [`PathLighting.hlsli`](../../../../../../../Engine/Assets/Shaders/RayTracing/PathLighting.hlsli) is one current secondary-hit consumer.
- The frame graph already derives unordered-access allocation and barriers from declared use. Raster depth is shader-readable on both backends. No new public RHI operation is required by the selected primary path.
- Neutral graphics descriptors and both backends now lower explicit blend state, while the only current raster GBuffer producer requests opaque blending. Enabling ordinary source-alpha blending would still not solve the decal semantic problem: GBuffer alpha components contain independent material data, including receiver alpha and dielectric F0, so one hardware blend cannot express the required per-field preservation and normalized-normal composition.
- No decal component, cooked decal record, render-scene decal table, or decal shader exists today.

The exact code must be re-inspected at the start of each implementation phase because this document describes a target over a changing repository.

## Evidence Boundary

`REN-E25` owns the negative/reachability audit until implementation begins. The feature may be described as a mandatory target but must remain unavailable and never be claimed as implemented. Once code exists, the delivery plan must update the frame narrative, GBuffer dossier, shader-program catalog, inventories, selectors, and acceptance evidence together; this gap dossier then becomes the current implementation dossier rather than coexisting with a second truth.

### Current Negative Acceptance

- `AC-DECAL-NEG-01` — no authored/editor/imported/cooked decal data or Renderer scene/GPU-scene payload is reachable.
- `AC-DECAL-NEG-02` — no GBuffer composition pass/program/product changes primary raster or ray surfaces and no secondary-ray hit evaluates a decal.
- `AC-DECAL-NEG-03` — the target architecture, plan, blend-state support, and existing material/GBuffer seams are never described as implemented decal support.

| Failure mode | Required response | Check |
| --- | --- | --- |
| `FM-DECAL-NEG-01` — a decal-like component, selector, shader, pass, payload, or ray-hit branch appears | fail the negative contract and convert this dossier atomically to current implementation truth before advertising it | `CHK-DECAL-NEG-01` |
| `FM-DECAL-NEG-02` — target/plan/acceptance wording is presented as current support | correct the claim; current inventory stays Not found | `CHK-DECAL-NEG-01` |

`CHK-DECAL-NEG-01` is `REN-E25`: search authored/import/cook/editor surfaces, CMake and shader registrations, scene/GPU-scene, GBuffer and arbitrary-ray composition, selectors, diagnostics, packages, and documentation. It covers `AC-DECAL-NEG-01` through `AC-DECAL-NEG-03` and `FM-DECAL-NEG-01` through `FM-DECAL-NEG-02`. This current absence check is independent of the future [Acceptance](Acceptance.md) contract.

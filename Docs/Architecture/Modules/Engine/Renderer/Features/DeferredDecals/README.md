# Renderer Deferred Decals

Status: current feature-gap dossier; source-backed absence record, not release approval or executable evidence

Snapshot: absence and extension seams rechecked 2026-09-06 against source revision `8414b5dc`; current `HEAD` contains later documentation-only commits and no intervening Renderer/RHI/shader-source change

Scope: `REN-DECAL-01` through `REN-DECAL-03`; current GBuffer, material, ray-hit, frame-graph, blend-state, and authored/runtime seams relevant to deferred decals, including the explicit absence of an implemented decal feature

Target architecture: [Deferred GBuffer Decal Composition Architecture](CompositionArchitecture.md)

Delivery authority: [Deferred GBuffer Decals Delivery Plan](../../../../../../Plans/Renderer/DeferredGBufferDecals.md)

Feature acceptance: [Deferred GBuffer Decals — Acceptance](Acceptance.md)

Code and executable build configuration remain authoritative. Reinspect every listed owner and absence before using this dated snapshot for implementation or release claims.

## Source-Backed Snapshot

Deferred decals are **not implemented**. The three inventory rows separate the absent user capability from useful extension seams:

| Capability | Current state | Meaning |
| --- | --- | --- |
| `REN-DECAL-01` authored and scene decal data | Not found | No component, imported/cooked decal record, render-scene table, GPU-scene payload, selector, or editor authoring path was found. |
| `REN-DECAL-02` primary deferred composition | Not found | No decal render/compute pass modifies the raster or ray-produced GBuffer before lighting. |
| `REN-DECAL-03` secondary-ray decal evaluation | Not found | Arbitrary ray hits evaluate the base material only; no decal candidate lookup or composition is present. |

The [composition architecture](CompositionArchitecture.md) is a designed future contract, not a fourth current rendering branch. Its plan and acceptance documents remain inactive proof routes until implementation exists.

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

`REN-E25` owns the negative/reachability audit until implementation begins. The feature must remain absent from release-facing selectors and claims. Once code exists, the delivery plan must update the frame narrative, GBuffer dossier, shader-program catalog, inventories, selectors, and acceptance evidence together; this gap dossier then becomes the current implementation dossier rather than coexisting with a second truth.

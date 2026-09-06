# Deferred GBuffer Decals Capability Snapshot

Status: capability snapshot; not release approval or executable evidence

Snapshot: 2026-08-28 at committed `master` revision `20814381`; source and executable build configuration were unchanged from implementation revision `99af6d5b`

Scope: current GBuffer, material, ray-hit, frame-graph, blend-state, and authored/runtime decal surfaces relevant to the target decal system

Architecture authority: [Deferred GBuffer Decal Pipeline](DeferredGBufferDecals.md)

Delivery authority: [Deferred GBuffer Decals Delivery Plan](../../../../Plans/Renderer/DeferredGBufferDecals.md)

Acceptance authority: [Deferred GBuffer Decals Acceptance Contract](../../../../Acceptance/Renderer/DeferredGBufferDecals.md)

Code and executable build configuration remain authoritative. Reinspect every listed owner and absence before using this dated snapshot for implementation or release claims.

## Source-Backed Snapshot

The design extends the existing owner instead of adding a second renderer path:

- [`Passes/GBuffer/GBuffer.cpp`](../../../../../Engine/Renderer/Private/Passes/GBuffer/GBuffer.cpp) creates one `GBufferRenderTargets` set and selects either rasterization or the one immutable ray-tracing GBuffer execution plan. Inline and pipeline frontends share that target set and meet before sky motion vectors and device-depth linearization.
- [`GBufferFormats.h`](../../../../../Engine/Renderer/Private/Passes/GBuffer/GBufferFormats.h) defines the shared BaseColor, Normal, Material, Emissive, Subsurface, DeviceZ, and MotionVector products.
- [`GBufferPS.hlsl`](../../../../../Engine/Assets/Shaders/Passes/GBuffer/GBufferPS.hlsl) and [`RayTracingGBufferCommon.hlsli`](../../../../../Engine/Assets/Shaders/Passes/RayTracing/RayTracingGBufferCommon.hlsli) share [`GBufferPacking.hlsli`](../../../../../Engine/Assets/Shaders/Passes/GBuffer/GBufferPacking.hlsli) through the canonical `/Engine` virtual shader namespace. [`RayTracingGBufferInline.hlsl`](../../../../../Engine/Assets/Shaders/Passes/RayTracing/RayTracingGBufferInline.hlsl) and [`RayTracingGBufferPipeline.hlsl`](../../../../../Engine/Assets/Shaders/Passes/RayTracing/RayTracingGBufferPipeline.hlsl) are thin traversal/stage frontends over that common semantic owner.
- [`MaterialCache.cpp`](../../../../../Engine/Renderer/Private/Scene/Materials/MaterialCache.cpp) resolves semantic defaults, per-material raster tables, and one scene-wide material texture table beneath the persistent render-scene authority. The latter must remain a scene-material capability, not be described as ray-tracing-only.
- [`RayTracingMaterialHit.hlsli`](../../../../../Engine/Assets/Shaders/RayTracing/RayTracingMaterialHit.hlsli) is the central base-material reconstruction path for arbitrary ray hits. [`PathLighting.hlsli`](../../../../../Engine/Assets/Shaders/RayTracing/PathLighting.hlsli) is one current secondary-hit consumer.
- The frame graph already derives unordered-access allocation and barriers from declared use. Raster depth is shader-readable on both backends. No new public RHI operation is required by the selected primary path.
- Neutral graphics descriptors and both backends now lower explicit blend state, while the only current raster GBuffer producer requests opaque blending. Enabling ordinary source-alpha blending would still not solve the decal semantic problem: GBuffer alpha components contain independent material data, including receiver alpha and dielectric F0, so one hardware blend cannot express the required per-field preservation and normalized-normal composition.
- No decal component, cooked decal record, render-scene decal table, or decal shader exists today.

The exact code must be re-inspected at the start of each implementation phase because this document describes a target over a changing repository.

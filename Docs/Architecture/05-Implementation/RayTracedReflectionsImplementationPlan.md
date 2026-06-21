# Ray Traced Reflections Implementation Plan

Status: staged implementation plan  
Date: 2026-06-21  
Scope: full-resolution inline ray traced reflections, stochastic importance sampling, hit material and lighting evaluation, no denoiser yet

## Stage 0 Source Audit Note

Stage 0 was audited against current source on 2026-06-21. The existing renderer/RHI foundation is strong enough to start with an inline ray-query reflection pass, but real hit-point material and lighting requires one new renderer-owned shader data contract before Stage 4.

Already available to a reflection pass:

- `RenderRayTracingScene` owns capability-gated scene ray tracing and exposes TLAS availability, TLAS resource, TLAS GPU address, shader access mode, and instance count through renderer-facing methods.
- The current direct-lighting path already proves inline ray-query binding through `DirectLightingPass`, `DirectLightingShaders.cpp`, and `RayTracedShadows.hlsli`.
- Classic TLAS instances use `RhiRayTracingInstanceDesc::InstanceID = index`, where `index` is the render mesh instance index in `RenderSceneData::meshInstances`.
- `RayTracingBlasCache` builds BLAS data from `GPUMesh::GetRayTracingGeometry()`, which supplies vertex/index GPU addresses, vertex stride, vertex count, index count, and index format.
- `MeshInstanceFrameData` already uploads a `StructuredBuffer<MeshInstanceData>` with world matrix, previous world matrix, world inverse transpose, material slot, skinning flags, joint matrix offset, and packed ray tracing debug data.
- GBuffer/deferred shaders already expose reusable helpers for current-pixel surface data, world-position reconstruction, direct-light evaluation, light constants, BRDF helpers, and ray traced shadow sampling.
- `VertexData` contains position, UV, color, normal, and tangent, which is enough for triangle-hit interpolation if the renderer exposes shader-readable mesh fetch bindings.

Not yet available as a clean shader-visible reflection hit contract:

- a stable per-render-instance hit table mapping TLAS `InstanceID` to mesh fetch metadata, including vertex/index buffer descriptors or packed GPU addresses, vertex stride, index format, vertex/index counts, material slot, mesh kind, and skinning metadata
- typed pass parameters and shader declarations for arbitrary hit vertex/index fetch from a compute shader
- a GPU material-value buffer matching `MaterialData`/`PerObjectPSConstantBufferData` so hit shading can fetch material constants by material slot
- a texture access strategy for hit materials; current GBuffer material sampling is batch/draw-bound through one material texture table, not bindless per-hit material textures
- a decision for first hit-shading scope: material constants only, material constants plus texture arrays/bindless tables, or a fallback that samples only data packed in GBuffer for the primary surface
- alpha-mask/alpha-blend hit behavior, backface/two-sided material policy, and skinned mesh hit reconstruction policy

Stage order remains valid, but Stage 3 is not optional for "proper material & lighting to the hits." Stage 2 can produce mirror/debug ray hits using TLAS and GBuffer data. Stage 4 should wait until Stage 3 provides a renderer-owned `InstanceID -> hit geometry/material` shader contract.

## Production Hit-Shading Reference Note

Deep-dive updated on 2026-06-21 after checking NVIDIA, AMD, Epic, Khronos, and Microsoft ray tracing references. The current Stage 3 implementation is a useful first contract, but it should be treated as a bootstrap/debug contract, not the final production material-hit ABI.

Reference patterns to carry into the production solution:

- Microsoft DXR ray queries are appropriate for compute-stage inline tracing, but the application owns coherence, resource organization, and shading complexity. Avoid turning one inline compute shader into an unbounded material uber-shader.
- NVIDIA DXR/OptiX/Falcor references all separate intersection identity from material evaluation. A hit returns instance/primitive/barycentrics; engine-owned scene data then reconstructs geometric quantities, evaluates material, and returns radiance.
- NVIDIA Donut samples show two viable production directions: local-root/material records for ray tracing pipeline shaders, or bindless scene/material resources for ray tracing and raster parity. Sparkle's inline ray-query path should converge on the bindless/scene-data flavor because the pass is compute-owned and backend-neutral.
- AMD SSSR and Far Cry 6 reflection references emphasize roughness-aware reflection policy, GGX importance sampling, ray budgets, environment fallback, and clear integration into a later composite pass. The Sparkle plan should keep `IndirectSpecular` source-agnostic and expose mode/max-distance/debug controls without non-physical intensity or roughness cutoff knobs.
- Epic's real-time ray tracing documentation treats max roughness, max ray distance, per-material ray-traced shadow behavior, culling, and geometry class limitations as product controls, not hidden shader constants.
- Khronos hybrid ray tracing guidance calls out the same hit path: GBuffer depth/normal/material launch data, reflection ray based on roughness, environment on miss, and deferred composite integration.
- Falcor's texture LOD helpers are a reminder that production ray-hit material sampling needs an explicit texture LOD policy. Sparkle can start with constant material data, but textured hits need UV/tangent interpolation and either ray cones, ray differentials, or a conservative explicit mip strategy.

Production implications for Sparkle:

- `CommittedInstanceID()` and `CommittedPrimitiveIndex()` must be treated as keys into renderer-owned tables whose lifetime, ordering, and validation are tied to the TLAS build.
- The hit geometry ABI must include position, normal, tangent/sign, UV0, and index format/count metadata before textured material parity is considered complete.
- Material hit shading must reuse the same material packing semantics as `Material.hlsli` and the GBuffer path: base color, alpha, roughness, metallic, emissive, normal map policy, two-sided policy, alpha mode/cutoff, and texture flags.
- Hit material texture sampling eventually needs a renderer-owned texture table or bindless descriptor contract. Per-draw texture bindings from the raster GBuffer path are not enough for arbitrary ray hits, but this should be the final material-parity stage rather than a blocker for the first RT reflection introduction.
- Alpha-tested geometry requires an any-hit-equivalent policy. With inline `RayQuery`, that means either accepting opaque-only geometry for the first production milestone, or using candidate hit inspection plus alpha sampling before committing. Do not silently treat alpha-masked foliage/fences as fully opaque and call the result production.
- Skinned, morphed, instanced, and world-position-offset geometry must be explicitly classified as supported, frozen/static fallback, or excluded from the reflection TLAS/hit-data table.
- Reflection hit lighting should produce incident radiance at the hit, then the primary surface should apply the specular BRDF/PDF weight. Do not bake primary-surface Fresnel/roughness twice in both the trace pass and `LightingComposite`.
- Validation must include parity scenes where the same material is visible directly in GBuffer and indirectly through `RTIndirectSpecular`.

Payload model note:

- Unreal's ray tracing material paths commonly use DXR payload structs because ray generation, closest-hit, any-hit, and miss shaders need a shared packet of ray state and shading results.
- Sparkle's current `RTIndirectSpecular` path uses inline `RayQuery` in a compute shader, so there is no cross-shader DXR payload object. The equivalent concept is the local shader contract made of `RTIndirectSpecularTraceResult`, `RTIndirectSpecularHitSurface`, fallback reason bits, and the final incident-radiance/reflection contribution.
- Keep that local contract payload-shaped: small, explicit, versionable, and separated into trace identity, reconstructed surface, material sample, lighting result, and debug/fallback state.
- If Sparkle later adds a ray generation or closest-hit shader pipeline for reflections, this local contract should become the starting point for the real DXR payload rather than inventing a second material-hit representation.

## Goal

Add a renderer-owned ray traced reflection effect that uses the existing ray tracing scene and frame graph contracts. The first shipped shape should be:

- full-resolution compute pass
- inline ray queries rather than a ray generation pipeline
- stochastic GGX/specular importance sampling
- one reflection sample per pixel by default
- proper material and lighting evaluation at the ray hit
- deterministic fallback when ray tracing, TLAS, or required hit data is unavailable
- no temporal accumulation, spatial denoiser, or external provider integration in this stage

The effect should be reviewable as a clean renderer feature, not as a backend-specific experiment. Renderer code consumes frame-graph resources and `RenderRayTracingPassServices`; RHI remains the owner of acceleration structure creation, backend-native handles, and ray tracing capability facts.

## Existing Architecture Anchors

Use these existing contracts while implementing:

- RHI ray tracing ownership: [RHIContract.md](../02-Contracts/RHIContract.md)
- pass/resource ownership: [RendererFrameGraph.md](../02-Contracts/RendererFrameGraph.md)
- runtime scene boundary: [RuntimeSceneData.md](../02-Contracts/RuntimeSceneData.md)
- shader package ABI: [ShaderPipeline.md](../02-Contracts/ShaderPipeline.md)
- native/backend boundary rules: [BoundaryRules.md](../00-Review/BoundaryRules.md)

Relevant source entry points from the current tree:

- `Engine/Renderer/Private/Frame/Frame.cpp`
- `Engine/Renderer/Private/Frame/Lighting.cpp`
- `Engine/Renderer/Private/Frame/LightingRenderTargets.cpp`
- `Engine/Renderer/Private/Frame/Targets/FrameRenderTargets.h`
- `Engine/Renderer/Private/Passes/DirectLightingPass.h`
- `Engine/Renderer/Private/Passes/DirectLightingPass.cpp`
- `Engine/Renderer/ShaderRegistrations/DirectLightingShaders.cpp`
- `Engine/Renderer/ShaderRegistrations/RendererShaderPackages.h`
- `Engine/Assets/Shaders/Passes/Deferred/DirectLighting.hlsl`
- `Engine/Assets/Shaders/Passes/Deferred/DirectLightingVulkanAddress.hlsl`
- `Engine/Renderer/Private/Frame/RayTracingScene.cpp`
- `Engine/Renderer/Private/RayTracing/RenderRayTracingPassServices.h`

External references used to shape the production plan:

- Microsoft DXR Functional Spec: https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html
- NVIDIA, Introduction to NVIDIA RTX and DirectX Ray Tracing: https://developer.nvidia.com/blog/introduction-nvidia-rtx-directx-ray-tracing/
- NVIDIA, DX12 Raytracing Tutorial Part 2: https://developer.nvidia.com/rtx/raytracing/dxr/dx12-raytracing-tutorial-part-2
- NVIDIA Falcor Path Tracer documentation: https://github.com/NVIDIAGameWorks/Falcor/blob/master/docs/usage/path-tracer.md
- NVIDIA Falcor texture LOD helpers: https://github.com/NVIDIAGameWorks/Falcor/blob/master/Source/Falcor/Rendering/Materials/TexLODHelpers.slang
- NVIDIA Donut Samples: https://github.com/NVIDIA-RTX/Donut-Samples
- AMD FidelityFX Stochastic Screen-Space Reflections: https://gpuopen.com/manuals/fidelityfx_sdk/techniques/stochastic-screen-space-reflections/
- AMD Far Cry 6 Hybrid Ray Traced Reflections presentation: https://gpuopen.com/download/GDC_Performant_Reflective_Beauty_Hybrid_Ray_Traced_Reflections_In_Far_Cry_6.pdf
- Epic Real-Time Ray Tracing documentation: https://dev.epicgames.com/documentation/en-us/unreal-engine/real-time-ray-tracing
- Khronos Vulkan Ray Tracing Best Practices for Hybrid Rendering: https://www.khronos.org/blog/vulkan-ray-tracing-best-practices-for-hybrid-rendering

## Non-Goals For The First Version

- no denoiser
- no temporal accumulation
- no reflection history resources
- no checkerboard or half-resolution tracing
- no ray generation shader pipeline requirement
- no recursive reflections
- no backend-native renderer code
- no new provider interface unless the implementation proves a reusable contract is needed

## Technical Direction

The first implementation should add a new compute shader pass named `RTIndirectSpecular` before `LightingComposite`.

The pass should read:

- `GBufferBaseColor`
- `GBufferNormal`
- `GBufferMaterial`
- `GBufferDeviceZ`
- optional `GBufferMotionVector` only for future debug or reprojection diagnostics, not for denoising yet
- `SceneTlas`
- per-frame and per-view uniforms
- ray tracing reflection settings
- renderer-owned hit/material/lighting buffers needed to shade the ray hit

The first reflection implementation should remain constants-only for hit materials. Bindless or descriptor-indexed texture material parity is intentionally deferred to the last stage of this plan.

The pass should write:

- `IndirectSpecular`

The composite stage should keep consuming `IndirectSpecular` without caring whether that texture came from analytic indirect lighting, ray traced reflections, or a future blended strategy. Resource names describe lighting role, not implementation source.

## Resource Contract

Use the existing indirect specular output in `LightingRenderTargets`:

```cpp
FrameGraphTextureHandle IndirectSpecular;
```

Keep its stable debug/resource name as `"IndirectSpecular"`.

The first composite behavior should be:

- use existing `DirectSpecular` and `IndirectSpecular` as before
- let `RTIndirectSpecular` write `IndirectSpecular` when the feature is enabled and the pass produced valid output
- preserve the current indirect-specular path, or clear/write black according to renderer settings, when the pass is disabled or unavailable

Avoid introducing `RayTracedSpecular`, `ReflectionsSpecular`, or other source-specific frame-graph resource names for the main lighting product. If a temporary debug texture is later needed, name it as a debug artifact rather than as the lighting contract.

Avoid hidden pass-private resources. If the pass touches a texture, buffer, or acceleration structure at execute time, it must be declared during setup.

## Hit Shading Requirements

The initial implementation should not stop at visibility. It should compute reflection lighting at the ray hit using renderer-owned scene data. That means the implementation needs a clear hit-data path:

- instance id or geometry id from the ray query
- primitive id
- barycentrics
- world position
- interpolated normal
- material id
- base color / roughness / metallic / emissive or current engine equivalents
- direct light evaluation for the hit point
- optional environment/sky fallback for misses

If the current TLAS/BLAS build path does not expose enough geometry/material data to shaders, add a narrow renderer-owned GPU hit data contract first. Do not make the reflection shader scrape gameplay data or backend-native AS internals.

For production parity, the hit-data path must also answer:

- are the sampled hit material constants and textures identical in meaning to the raster GBuffer material path?
- which geometry classes are included in the reflection TLAS and which are deliberately excluded?
- how are alpha-tested hits rejected or accepted?
- how are two-sided surfaces oriented at a hit?
- which mip level is used for material textures when there are no screen-space derivatives?
- is the value written to `IndirectSpecular` incident reflected radiance, already-weighted primary BRDF contribution, or a deliberately documented intermediate?

## Sampling Requirements

Start simple, but keep the stochastic foundation correct:

- reconstruct world position from depth and camera data
- decode normal and material parameters from the GBuffer
- compute view vector and reflection basis
- sample a GGX visible-normal or half-vector distribution using roughness
- trace one ray per pixel by default
- shade the ray hit into incident radiance, then weight it by the primary surface BRDF/pdf throughput
- use Schlick Fresnel, GGX distribution, and the same geometry visibility convention as the engine BRDF helpers unless a deliberate difference is documented
- support the full material roughness range; use only numerical epsilons where required to avoid division by zero
- offset ray origin along the geometric normal to reduce self-intersection
- cap ray distance through a renderer setting
- return black for invalid depth, invalid normal, unsupported material, or missing TLAS
- use deterministic per-pixel/per-frame random seeds from frame index and pixel coordinate

Because there is no denoiser yet, expose review settings without changing physical energy:

- enable/disable ray traced reflections
- max ray distance
- sample mode: mirror/debug and stochastic GGX
- optional debug view for hit/miss or sample direction

## Stages

### Stage 0: Source Audit And Feature Contract

Goal: confirm the exact data already available to an inline ray-query shader and identify the smallest missing hit-data contract.

Status: completed by the Stage 0 source audit note above.

Implementation tasks:

- inspect `RenderRayTracingScene`, BLAS/TLAS build inputs, instance records, and pass-facing services
- inspect current material and mesh GPU buffers used by GBuffer/direct lighting
- document whether shader-visible hit reconstruction can access vertices, indices, instance transforms, and material ids
- add a short implementation note at the top of this file if source reality changes the stage order

Acceptance criteria:

- no code changes that cross module boundaries
- a clear list of existing shader-visible hit data
- a clear list of missing buffers or binding contracts required for proper hit material/lighting

Implementation prompt:

```text
Implement Stage 0 from Docs/Architecture/05-Implementation/RayTracedReflectionsImplementationPlan.md.

Audit the current SparkleEngine ray tracing scene, BLAS/TLAS build path, render scene data, material cache, mesh GPU buffers, and pass runtime services. Do not add the reflection effect yet. Produce a concise source-backed note in the plan that states which hit data is already shader-visible for inline ray queries and which renderer-owned buffers must be added before hit material and lighting can be evaluated. Keep Renderer/RHI/GameFramework boundaries from the architecture docs intact.
```

### Stage 1: Pass Skeleton And Black Output

Goal: add the reflection pass, shader package, frame-graph resources, settings gate, and diagnostics with no visual contribution beyond black output.

Status: implemented on 2026-06-21.

Implementation note:

- `RTIndirectSpecular` is registered as a compute shader package with `inline-ray-query` and `acceleration-structure` features.
- The pass writes the existing `LightingRenderTargets::IndirectSpecular` resource and is inserted after `IndirectLighting` and before `LightingComposite`.
- Stage 1 RGB output is black. The shader performs a minimal inline ray-query path and writes only a tiny alpha/debug signal so the declared acceleration structure, GBuffer, `PerFrame`, and `PerView` bindings remain reflected and validated.
- Descriptor-TLAS access is wired for this skeleton. Shader-device-address specialization should be handled deliberately in a later stage if Vulkan requires it for the active TLAS mode.

Implementation tasks:

- route the existing `LightingRenderTargets::IndirectSpecular` through the new pass ownership path
- add `RTIndirectSpecularPass` parameter struct and compute pass class
- add `AddRTIndirectSpecularPass(...)` frame assembly helper
- register an `RTIndirectSpecular` shader package using inline ray query and acceleration structure feature flags
- add `Engine/Assets/Shaders/Passes/Deferred/RTIndirectSpecular.hlsl`
- insert the pass after lighting inputs are available and before `LightingComposite`
- update `LightingComposite` to read the new reflection texture, initially black
- guard execution when ray tracing is unavailable or the scene TLAS is not bound
- add pass timing scope and stable pass labels

Acceptance criteria:

- shader registration validates
- shader package cooks for DXIL and, where supported, SPIR-V
- frame graph has no unresolved barrier warnings
- runtime output is visually unchanged with reflections disabled or black
- architecture boundary check still passes

Implementation prompt:

```text
Implement Stage 1 from Docs/Architecture/05-Implementation/RayTracedReflectionsImplementationPlan.md.

Add a renderer-owned full-resolution compute pass named RTIndirectSpecular that declares all frame-graph resources, registers an RTIndirectSpecular shader package, writes LightingRenderTargets::IndirectSpecular, and feeds that existing texture into LightingComposite. The shader should output black for now. Keep the pass backend-neutral, use existing shader/pass patterns from DirectLightingPass, and guard missing ray tracing/TLAS support with a deterministic no-op path. Run shader validation/cook checks and the architecture boundary check if available.
```

### Stage 2: Mirror Reflection Ray Query

Status: implemented on 2026-06-21.

Implementation note:

- `RTIndirectSpecular` now reconstructs world position from `GBufferDeviceZ`, decodes world-space GBuffer normals through shared GBuffer helpers, traces a mirror ray against the descriptor `SceneTlas`, and writes a visible temporary debug signal into `LightingRenderTargets::IndirectSpecular`.
- Debug mode is controlled by `r.RayTracing.Reflections.DebugMode`: `0=Off` uses stable hit-id color, `1=HitMask`, `2=HitDistance`, and `3=MirrorDirection`.
- Stage 2 intentionally keeps material-hit shading out of the pass; hit visualization is based on ray-query committed hit metadata until renderer-owned hit material buffers are introduced.

Goal: produce a visible full-resolution mirror reflection without stochastic roughness yet.

Implementation tasks:

- reconstruct world position from `GBufferDeviceZ` and camera matrices
- decode world-space normal from `GBufferNormal`
- compute mirror reflection direction from view vector and normal
- trace against `SceneTlas`
- shade misses with black or sky/environment fallback if a renderer-owned sky sample is available
- shade hits with a temporary debug color based on instance/primitive/hit distance if full hit material data is not ready
- expose debug modes: off, hit mask, hit distance, mirror direction

Acceptance criteria:

- reflective surfaces show a stable, full-resolution ray-query signal
- invalid depth and missing TLAS return black
- no direct backend-native access from Renderer
- debug output can prove rays are hitting scene geometry

Implementation prompt:

```text
Implement Stage 2 from Docs/Architecture/05-Implementation/RayTracedReflectionsImplementationPlan.md.

Turn RTIndirectSpecular from black output into a full-resolution mirror ray-query pass. Reconstruct world position from depth, use GBuffer normals, trace a reflection ray against SceneTlas, and write a visible debug reflection signal into IndirectSpecular. Add debug modes for hit mask and hit distance. Keep material hit shading out of this stage unless the required hit-data buffers already exist cleanly. Preserve the no-op path for missing ray tracing or missing TLAS.
```

### Stage 3: Shader-Visible Hit Data Contract

Status: implemented on 2026-06-21.

Implementation note:

- `RTIndirectSpecular` now has a renderer-owned hit-data contract made of packed static hit vertices, packed triangle indices, per-render-instance hit offsets, existing `MeshInstances`, and compact material constants.
- The frame hit-data upload is built from `RenderSceneData` after snapshot translation; the pass receives only renderer-owned GPU SRVs and does not read GameFramework scene state or backend-native acceleration-structure internals.
- Stage 3 supports static mesh hit reconstruction. Skinned meshes, missing retained mesh hit data, invalid material slots, or unavailable upload buffers are marked invalid and fall back to Stage 2 debug/black behavior with renderer diagnostics.
- The shader reconstructs hit normals from interpolated triangle attributes plus the instance world-inverse-transpose matrix, resolves material id through the hit instance table, and uses material base color/roughness as the temporary real-material hit signal.
- This is intentionally a bootstrap hit contract. It is not yet production material parity because it does not expose UVs, tangents, normal maps, material texture descriptors, explicit texture LOD, alpha-tested candidate-hit rejection, or skinned/deformed geometry support.

Goal: make ray hits shadeable using real mesh/material data instead of debug colors.

Implementation tasks:

- add or expose renderer-owned GPU buffers for reflection hit reconstruction
- include mesh vertex/index data needed for triangle interpolation
- include instance-to-material mapping or primitive material mapping
- include material parameters needed by the current BRDF
- include transforms needed to convert hit attributes to world space
- bind these buffers through typed pass parameters
- keep GameFramework data behind snapshots and renderer scene translation
- add validation for missing or mismatched hit-data buffers

Acceptance criteria:

- the reflection shader can reconstruct hit position, normal, and material id
- all buffers are declared in pass setup
- missing hit data maps to black/debug fallback with a diagnostic reason
- no GameFramework or backend-native ownership leaks into the pass

Implementation prompt:

```text
Implement Stage 3 from Docs/Architecture/05-Implementation/RayTracedReflectionsImplementationPlan.md.

Add the smallest renderer-owned shader-visible hit-data contract needed for RTIndirectSpecular to shade ray hits with real scene materials. Reuse existing RenderSceneData, material cache, GPU mesh cache, and ray tracing scene data where possible. Do not expose GameFramework mutable data or backend-native acceleration structure details to the pass. Bind all hit-data buffers through typed shader parameters and frame/pass runtime services, and add guarded fallback behavior when the data is unavailable.
```

### Stage 3.5: Production Hit Data ABI Hardening

Status: implemented on 2026-06-21.

Implementation note:

- `RTIndirectSpecularHitDataAbiVersion` is now `2`. The hit vertex ABI carries local position, local normal, tangent/sign, and `UV0`; shader reconstruction interpolates barycentrics as `(1 - x - y, x, y)`.
- Normal reconstruction transforms local normals by `MeshInstanceData::WorldInvTransposeMTX`. Tangents transform by `WorldMTX`; two-sided materials orient the reconstructed basis against the reflection ray before later normal-map work is added.
- The production policy for this stage is intentionally conservative: opaque static meshes are supported; alpha-tested, alpha-blended, skinned/deformed, missing mesh-hit-data, invalid material, invalid primitive, and invalid vertex-index cases are marked in the instance table with explicit fallback reasons.
- TLAS `InstanceID` continues to map to the render mesh instance index through the current classic TLAS path. Partitioned/PTLAS parity remains a validation item once the PTLAS compile drift is repaired.
- Debug modes now include hit UV, hit normal, material id, geometry class, fallback reason, and alpha policy so black/fallback hits can be diagnosed from shader output.

Goal: turn the Stage 3 bootstrap buffers into a production-ready ray-hit data ABI that can match raster material sampling.

Implementation tasks:

- define one versioned `RTIndirectSpecularHitVertex`/mesh layout that includes at least position, normal, tangent/sign, and UV0
- document coordinate conventions for barycentrics, tangent basis reconstruction, normal-map space, handedness, and non-uniform scale
- add explicit per-hit mesh metadata: vertex offset/count, index offset/count, material slot, geometry flags, alpha mode, two-sided flag, skinned/deformed support flag, and debug reason bits
- prove TLAS `InstanceID` ordering matches the hit instance table for every BLAS/TLAS build path, including classic and partitioned paths once PTLAS compiles again
- decide the first production alpha policy:
  - opaque-only supported, alpha-tested excluded from TLAS/hit table, or
  - inline `RayQuery` candidate-hit alpha test using UV/material texture before `CommitNonOpaqueTriangleHit`
- decide the first production skinned/deformed policy:
  - excluded/fallback with diagnostics, or
  - build/upload deformed ray-hit buffers from the same snapshot used by the TLAS
- add bounds and ABI validation for mismatched vertex/index/material counts, stale TLAS instance ids, missing texture descriptors, and unsupported geometry classes
- add debug visualizations for hit UV, hit normal, material id, geometry support class, alpha rejection, and fallback reason
- update Stage 3 source note with the exact supported/unsupported geometry and material classes

Acceptance criteria:

- the hit vertex ABI has all attributes required for textured PBR material sampling
- TLAS instance id to hit table mapping is validated and logged
- alpha-tested and skinned/deformed geometry behavior is explicit and testable
- unsupported geometry cannot silently shade as the wrong material
- debug modes can explain every black/fallback hit

Implementation prompt:

```text
Implement Stage 3.5 from Docs/Architecture/05-Implementation/RayTracedReflectionsImplementationPlan.md.

Harden the RTIndirectSpecular hit-data ABI from a bootstrap static-material path into a production ray-hit contract. Add UV0 and tangent/sign data, explicit mesh/material/geometry flags, TLAS InstanceID validation, unsupported-geometry diagnostics, and debug modes for UV/material/fallback reasons. Decide and implement the first alpha-tested and skinned/deformed geometry policy without leaking GameFramework or backend-native data into the pass. Keep all resources declared through typed pass parameters and preserve deterministic fallback behavior.
```

### Stage 4: Constants-Only Hit Material And Direct Lighting

Status: implemented on 2026-06-21.

Implementation note:

- `RTIndirectSpecular` now reconstructs constants-only hit material data: base color, alpha, roughness, metallic, dielectric F0, emissive, and subsurface constants.
- Hit shading evaluates directional, point, and spot lights from `ViewLighting` using the engine BRDF helpers and no secondary shadow rays. This produces outgoing/incident radiance from the reflected hit and includes hit emissive contribution.
- Material texture sampling, normal-map sampling, and alpha-tested candidate-hit rejection remain deferred to Stage 8. Textured materials still use material constants and existing debug/fallback diagnostics.
- Primary-surface Fresnel/roughness weighting remains outside this hit-lighting step, so the pass does not intentionally double-apply the primary surface specular response.

Goal: evaluate a plausible direct-lighting contribution at the reflection hit.

Implementation tasks:

- interpolate hit position, geometric normal, shading normal, tangent basis, UV0, and material data
- sample material constants through the same value semantics as `Material.hlsli`
- keep material texture sampling disabled in this stage; textured materials use constants-only values and diagnostics until the final bindless/material-texture stage
- carry UV/tangent data through the hit surface so later texture parity can be added without changing core hit reconstruction again
- apply alpha mode and alpha cutoff according to the Stage 3.5 policy
- apply two-sided material normal handling deliberately
- do not sample normal maps yet; validate tangent basis through debug views and reserve normal-map sampling for the final material-texture stage
- evaluate the same or intentionally matched BRDF terms used by direct lighting
- evaluate directional, point, and spot lights according to current renderer lighting data
- apply visibility policy deliberately:
  - first option: direct light without secondary shadow rays
  - later option: shadow ray from hit point to light when budget allows
- include emissive material contribution
- add sky/environment miss lighting if a renderer-owned source exists
- clearly separate hit-surface incident radiance from primary-surface specular weighting so Fresnel/roughness is not applied twice

Acceptance criteria:

- mirror reflections show material color and direct lighting from hit surfaces
- material roughness/metallic or current engine material parameters influence reflection color
- emissive hit surfaces contribute visibly
- textured materials use constants-only fallback with explicit diagnostics; full raster texture parity is deferred to the final bindless/material-texture stage
- normal-map, alpha-test, two-sided, and unsupported geometry behavior are intentional rather than accidental
- debug views still work

Implementation prompt:

```text
Implement Stage 4 from Docs/Architecture/05-Implementation/RayTracedReflectionsImplementationPlan.md.

Extend RTIndirectSpecular so ray hits are shaded with production constants-only material semantics and renderer lighting. Interpolate hit attributes including UV/tangent data, fetch material constants, apply the Stage 3.5 alpha/two-sided policy, evaluate the engine's current BRDF-compatible direct lighting, include emissive contribution, and use a deterministic miss fallback. Do not add bindless/material texture sampling in this stage; keep textured materials on explicit constants-only fallback diagnostics. Keep secondary shadow rays optional and disabled unless the existing ray tracing budget/settings make them clean. Keep LightingComposite source-agnostic by consuming IndirectSpecular.
```

### Stage 5: Stochastic GGX Importance Sampling

Status: implemented on 2026-06-21.

Implementation note:

- `RTIndirectSpecular` now has `r.RayTracing.Reflections.SampleMode`: `0=Mirror`, `1=StochasticGGX`. Stochastic GGX is the default, with mirror forced for the low-roughness limit.
- The shader uses deterministic per-pixel/per-frame interleaved gradient noise, samples a GGX half-vector with `alpha = roughness * roughness`, traces the sampled direction, shades hit/miss incident radiance separately, and applies explicit PDF/throughput weighting.
- Because `LightingComposite` still applies the primary-surface Fresnel and indirect-specular occlusion to `IndirectSpecular`, the Stage 5 estimator writes a Fresnel-free primary specular throughput. This is intentionally biased toward current pipeline compatibility rather than a fully standalone unbiased path.
- The full roughness range is supported. The shader does not apply roughness fade, an intensity multiplier, or contribution clamping; exact zero roughness takes the deterministic mirror path and nonzero roughness uses GGX sampling with numerical epsilons only.
- Debug modes now include sample direction, sample PDF, sample throughput, hit radiance, and final contribution.

Goal: move from mirror-only rays to rough-specular stochastic importance sampling.

Implementation tasks:

- add a reflection settings uniform
- generate deterministic per-pixel/per-frame random values
- importance sample the primary surface specular lobe from roughness, using a documented GGX half-vector or visible-normal distribution
- compute sample PDF and primary-surface BRDF throughput explicitly
- trace the sampled direction and evaluate hit/miss incident radiance separately from the primary-surface throughput
- keep mirror mode as a deterministic limit/debug path for very low roughness
- add mirror debug mode to compare against stochastic mode
- record whether the current estimator is unbiased, biased for current pipeline composition, or intentionally energy-normalized for one-sample/no-denoiser output

Acceptance criteria:

- smooth materials behave close to mirror mode
- rough materials produce stochastic spread
- sample weighting is energy-aware and does not obviously brighten/darken with roughness
- debug views can show sample direction, PDF, throughput, hit radiance, and final contribution separately
- no denoiser or history dependency is introduced

Implementation prompt:

```text
Implement Stage 5 from Docs/Architecture/05-Implementation/RayTracedReflectionsImplementationPlan.md.

Replace mirror-only sampling with a stochastic GGX importance-sampled reflection mode while keeping mirror as a debug mode. Sample the primary surface specular lobe across the full material roughness range, trace the sampled direction, shade hit/miss incident radiance, and apply an explicit BRDF/pdf throughput. Use deterministic per-pixel/per-frame random seeds, material roughness, and max ray distance. Do not add roughness fade, intensity scaling, contribution clamping, denoising, or temporal accumulation. Add debug views for sample direction, PDF, throughput, hit radiance, and final contribution. Keep the pass full resolution and inline ray-query based.
```

### Stage 6: Controls, Diagnostics, And Validation

Status: implemented on 2026-06-21.

Implementation note:

- Reviewer controls now use the renderer ray tracing settings path through `RTIndirectSpecularSettings` and `RenderRayTracingPassServices`.
- User-facing CVars are:
  - `r.RayTracing.Reflections.Enabled`: toggles the pass; disabled mode is a deterministic no-op that leaves the existing `IndirectSpecular` producer intact.
  - `r.RayTracing.Reflections.SampleMode`: `0=Mirror`, `1=StochasticGGX`.
  - `r.RayTracing.Reflections.MaxDistance`: physical ray length limit.
  - `r.RayTracing.Reflections.DebugMode`: `0=Off`, `1=HitMask`, `2=HitDistance`, `3=MirrorDirection`, `4=HitUV`, `5=HitNormal`, `6=MaterialId`, `7=GeometryClass`, `8=FallbackReason`, `9=AlphaPolicy`, `10=SampleDirection`, `11=SamplePdf`, `12=SampleThroughput`, `13=HitRadiance`, `14=FinalContribution`.
  - `r.RayTracing.Reflections.NormalBias`: ray origin bias; this is a geometric robustness control, not a lighting scale.
- The feature intentionally has no roughness cutoff/fade, intensity multiplier, or contribution clamp control.
- `RTIndirectSpecular` publishes status reasons through renderer smoke diagnostics: `disabled`, `unsupported`, `missing-tlas`, `missing-hit-data`, and `running`.
- Smoke diagnostics include enabled state, sample/debug modes, max distance, hit-data availability, hit instance/material counts, and the GPU timing label `RT Indirect Specular Ray Query`.
- Ray tracing frame timings now include `RTIndirectSpecularGpuMilliseconds` once timestamp results resolve.

Goal: make the feature easy to review, tune, and test.

Implementation tasks:

- add renderer settings for enable, mode, max distance, and debug visualization
- surface pass timing and basic counters in existing diagnostics
- include ray tracing reflection status in smoke capture metadata if that validation path is available
- add shader compiler inspection commands to documentation
- add a validation recipe for enabled, disabled, missing TLAS, and unsupported backend paths

Acceptance criteria:

- a reviewer can toggle the effect and see pass timing
- validation output identifies whether reflections ran, skipped, or fell back
- shader package inspection shows inline ray query and acceleration structure usage
- disabled mode is visually equivalent to the previous renderer output

Implementation prompt:

```text
Implement Stage 6 from Docs/Architecture/05-Implementation/RayTracedReflectionsImplementationPlan.md.

Add user/reviewer-facing controls and diagnostics for RTIndirectSpecular. Expose enable, sample mode, max ray distance, and debug visualization through the renderer settings path already used by similar features. Do not add non-physical roughness cutoff, intensity, or contribution clamp controls. Publish pass timing and a clear status reason for running, disabled, unsupported, missing TLAS, or missing hit data. Add smoke/validation metadata if that path exists, and document the shader compiler and runtime validation commands.
```

### Stage 7: Quality Cleanup Before Denoising

Status: implemented on 2026-06-21.

Implementation note:

- Ray origins now use a direction-aware normal bias plus a small ray-direction offset. Grazing rays scale the normal offset up to reduce immediate self-intersections without adding any lighting clamp or intensity control.
- Hit normal reconstruction now uses safe inverse-transpose normalization, tangent reconstruction is re-orthonormalized against the final shading normal, and degenerate tangents fall back to a generated basis.
- One-sided backface hits are now an explicit shader fallback reason (`OneSidedBackface`) instead of being accidentally shaded. Two-sided hits orient the normal and tangent toward the incoming ray before lighting.
- The stochastic path still supports the full material roughness range. Fireflies/noise are left visible for analysis and future denoising; no roughness fade, intensity scale, contribution clamp, temporal history, or denoiser resource was added.
- Blue-noise sampling is deferred. The current renderer has texture-manager and scene/default texture paths, but this pass does not yet have a clean renderer-owned imported blue-noise texture contract; hash/interleaved-gradient sampling remains the deterministic baseline.
- Known limitations before denoising: one sample per full-resolution pixel is noisy on rough materials, direct hit lighting has no secondary shadow rays, material textures and normal maps remain deferred to the bindless/material-texture stage, sky/environment miss lighting is still black unless a renderer-owned source is added, and unsupported geometry/material classes rely on fallback debug modes.

Goal: stabilize the stochastic base before any denoiser or temporal reuse is designed.

Implementation tasks:

- improve ray origin bias and normal handling
- investigate fireflies through diagnostics, sampling quality, and future denoising rather than material-aware contribution clamps
- handle backfaces and thin geometry deliberately
- preserve full roughness-range support
- add optional blue-noise texture input only if the engine already has a clean texture path for it
- record known noise limitations honestly

Acceptance criteria:

- the no-denoiser output is noisy but controlled
- debug modes remain useful
- known limitations are documented before denoising starts
- no hidden history resources have appeared

Implementation prompt:

```text
Implement Stage 7 from Docs/Architecture/05-Implementation/RayTracedReflectionsImplementationPlan.md.

Polish the no-denoiser stochastic RTIndirectSpecular baseline. Improve ray biasing, backface handling, sampling diagnostics, and debug views without adding history, denoising, roughness fade, intensity scaling, or contribution clamps. If a blue-noise resource can be integrated through existing renderer texture contracts, add it as an optional input; otherwise keep hash-based sampling. Update the docs with known quality limitations and recommended defaults.
```

### Stage 8: Bindless Material Texture Parity

Goal: add the renderer-owned descriptor-indexed or bindless material texture contract needed for ray-hit material sampling parity with the raster GBuffer path.

Implementation tasks:

- add a backend-neutral renderer material texture table indexed by material slot and `MaterialTextureSlots`/texture group
- expose capability checks for descriptor indexing/bindless resource access on D3D12 and Vulkan
- bind the material texture table and sampler set through typed pass parameters and frame/pass runtime services
- extend `RTIndirectSpecularHitMaterial` with stable texture indices or table offsets rather than per-draw texture bindings
- sample base color, roughness, metallic, emissive, and later normal maps at the ray hit through explicit texture LOD
- choose and document the first texture LOD policy:
  - conservative fixed/roughness-biased mip for the first version, or
  - ray-cone/ray-differential based mip if the required data is available
- add alpha-tested candidate-hit policy only after base-color alpha can be sampled through this texture table
- validate that textured materials visible directly in the GBuffer match the same materials through `RTIndirectSpecular`
- keep a constants-only fallback when bindless/material texture table support is unavailable

Acceptance criteria:

- ray-hit base color, roughness, metallic, and emissive texture sampling matches raster material semantics for supported material classes
- missing descriptor table, unsupported backend capability, missing texture descriptor, or invalid texture index falls back deterministically with diagnostics
- texture LOD policy is explicit and does not rely on implicit screen-space derivatives
- constants-only mode still works on platforms without the material texture table path
- architecture boundary check still passes with no backend-native renderer dependency

Implementation prompt:

```text
Implement Stage 8 from Docs/Architecture/05-Implementation/RayTracedReflectionsImplementationPlan.md.

Add the final material texture parity layer for RTIndirectSpecular. Build a renderer-owned descriptor-indexed or bindless material texture table keyed by material slot and MaterialTextureSlots, expose backend-neutral capability/fallback checks, bind the table through typed pass parameters, and sample supported material textures at ray hits with an explicit LOD policy. Keep constants-only fallback behavior for unsupported platforms or missing descriptors, and preserve Renderer/RHI/backend boundaries.
```

## Suggested Validation Commands

Exact command names can differ by local build output, so confirm paths from the current build tree before wiring these into automation:

```powershell
cmake --build build --target ShaderCompiler --config DevelopmentEditor
.\artifacts\dev\tools\ShaderCompiler\DevelopmentEditor\ShaderCompiler.exe list-shaders --validate
.\artifacts\dev\tools\ShaderCompiler\DevelopmentEditor\ShaderCompiler.exe inspect-shader RTIndirectSpecularCS
.\artifacts\dev\tools\ShaderCompiler\DevelopmentEditor\ShaderCompiler.exe cook --package RTIndirectSpecular --backend dxc --target DxilSm66
.\artifacts\dev\tools\ShaderCompiler\DevelopmentEditor\ShaderCompiler.exe cook --package RTIndirectSpecular --backend dxc --target SpirV16
.\artifacts\dev\tools\ShaderCompiler\DevelopmentEditor\ShaderCompiler.exe inspect-package <path-to-RTIndirectSpecular-cooked-package>
cmake --build build --target architecture_boundary_check --config DevelopmentEditor
cmake --build build --target SparkleRenderer --config DevelopmentEditor
```

Runtime validation should cover:

- D3D12 with ray tracing enabled
- Vulkan with ray tracing enabled if supported in the local build
- unsupported or missing ray tracing capability
- no scene TLAS / empty scene
- reflections disabled: set `r.RayTracing.Reflections.Enabled=0` and verify smoke status `disabled` plus visual equivalence to the previous renderer output
- missing TLAS: use an empty/no-traceable scene and verify smoke status `missing-tlas`
- unsupported descriptor-TLAS path/backend: verify smoke status `unsupported`
- missing hit data: force or reproduce missing RT hit buffers and verify smoke status `missing-hit-data` plus deterministic shader fallback
- mirror debug mode: set `r.RayTracing.Reflections.Enabled=1`, `r.RayTracing.Reflections.SampleMode=0`
- stochastic GGX mode: set `r.RayTracing.Reflections.Enabled=1`, `r.RayTracing.Reflections.SampleMode=1`
- status/timing metadata: capture smoke diagnostics and verify `RayTracing.RTIndirectSpecular.StatusReason`, `HitInstanceCount`, `HitMaterialCount`, and `FrameTimings.RTIndirectSpecularGpuMilliseconds`
- direct-vs-reflected material parity scene:
  - one material visible directly in GBuffer and through reflection
  - constants-only material
  - base-color textured material after Stage 8
  - roughness/metallic textured material after Stage 8
  - emissive material
- geometry policy scene:
  - opaque static mesh
  - two-sided surface
  - alpha-tested material
  - skinned/deformed mesh or explicit fallback object
  - non-uniform scale transform
- debug/fallback scene:
  - missing hit data
  - invalid material slot
  - unsupported geometry class
  - missing texture descriptor
  - TLAS instance id out of hit-table range

## First-Implementation Defaults

Recommended defaults for the first usable version:

- enabled: false until hit material shading is correct
- mode: mirror debug for Stage 2, stochastic GGX after Stage 5
- samples per pixel: 1
- max ray distance: 50 meters or current renderer world-unit equivalent
- miss lighting: black unless a renderer-owned sky/environment lookup is already cleanly available

## Known Risks

- Inline ray queries can prove the effect quickly, but hit shading requires shader-visible geometry/material data that may not be exposed yet.
- One sample per pixel without denoising will be noisy on rough materials. The plan intentionally does not hide that with roughness fade, intensity scaling, or contribution clamps; future quality work should address it through sampling and denoising.
- If Vulkan inline ray query support or acceleration structure binding differs from D3D12, keep that in capability checks and shader package features rather than adding renderer-native backend branches.
- Reflections can double count specular if the composite path does not clearly separate screen/local lighting from ray traced reflected lighting.
- If material data used by GBuffer and reflection hit shading diverges, visual mismatch will be obvious; prefer reusing the same renderer material packing where possible.
- Texture sampling at ray hits has no implicit screen-space derivatives. A naive `Sample` path can alias or pick inconsistent mips; require explicit LOD or ray-cone/ray-differential policy before claiming texture parity.
- Alpha-tested materials need an any-hit-equivalent policy with inline ray queries. Treating masked geometry as opaque can look acceptable in simple scenes and fail badly on foliage, fences, decals, and hair-like cards.
- Skinned/deformed geometry can desynchronize TLAS geometry, hit-data buffers, and raster output if not sourced from the same snapshot.
- Material/lights evaluated at the hit can become expensive quickly. Keep light loops bounded by existing renderer limits, expose diagnostics, and document when secondary shadow rays are disabled.

## Done Definition

The feature is done for this plan when:

- `RTIndirectSpecular` is a full-resolution inline ray-query compute pass
- it importance samples reflection directions from material roughness
- it shades ray hits with real renderer material and lighting data
- it reaches texture material parity through the final bindless/material texture table stage, or documents constants-only mode as the active platform fallback
- it composes into the lighting path through declared frame-graph resources
- it has deterministic no-op/fallback paths
- it exposes controls and diagnostics
- it has validation coverage for enabled, disabled, unsupported, and missing-data paths
- no denoiser, temporal history, or backend-native renderer dependency has been introduced

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
- Hit material texture sampling eventually needs a renderer-owned texture table or bindless descriptor contract. Per-draw texture bindings from the raster GBuffer path are not enough for arbitrary ray hits, but this should be the final material-parity stage rather than a blocker for the first ray-traced reflection introduction.
- Alpha-tested geometry requires an any-hit-equivalent policy. With inline `RayQuery`, that means either accepting opaque-only geometry for the first production milestone, or using candidate hit inspection plus alpha sampling before committing. Do not silently treat alpha-masked foliage/fences as fully opaque and call the result production.
- Skinned, morphed, instanced, and world-position-offset geometry must be explicitly classified as supported, frozen/static fallback, or excluded from the reflection TLAS/hit-data table.
- Reflection hit lighting should produce incident radiance at the hit, then the primary surface should apply the specular BRDF/PDF weight. Do not bake primary-surface Fresnel/roughness twice in both the trace pass and `LightingComposite`.
- Validation must include parity scenes where the same material is visible directly in GBuffer and indirectly through `IndirectSpecular`.

Payload model note:

- Unreal's ray tracing material paths commonly use DXR payload structs because ray generation, closest-hit, any-hit, and miss shaders need a shared packet of ray state and shading results.
- Sparkle's current `IndirectSpecular` path uses inline `RayQuery` in a compute shader, so there is no cross-shader DXR payload object. The equivalent concept is the local shader contract made of `IndirectSpecularTraceResult`, `IndirectSpecularHitSurface`, fallback reason bits, and the final incident-radiance/reflection contribution.
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

The first implementation should add a new compute shader pass named `IndirectSpecular` before `LightingComposite`.

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
- let `IndirectSpecular` write `IndirectSpecular` when the feature is enabled and the pass produced valid output
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

- `IndirectSpecular` is registered as a compute shader package with `inline-ray-query` and `acceleration-structure` features.
- The pass writes the existing `LightingRenderTargets::IndirectSpecular` resource and is inserted after `IndirectLighting` and before `LightingComposite`.
- Stage 1 RGB output is black. The shader performs a minimal inline ray-query path and writes only a tiny alpha/debug signal so the declared acceleration structure, GBuffer, `PerFrame`, and `PerView` bindings remain reflected and validated.
- Descriptor-TLAS access is wired for this skeleton. Shader-device-address specialization should be handled deliberately in a later stage if Vulkan requires it for the active TLAS mode.

Implementation tasks:

- route the existing `LightingRenderTargets::IndirectSpecular` through the new pass ownership path
- add `IndirectSpecularPass` parameter struct and compute pass class
- add `AddIndirectSpecularPass(...)` frame assembly helper
- register an `IndirectSpecular` shader package using inline ray query and acceleration structure feature flags
- add `Engine/Assets/Shaders/Passes/Deferred/IndirectSpecular.hlsl`
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

Add a renderer-owned full-resolution compute pass named IndirectSpecular that declares all frame-graph resources, registers an IndirectSpecular shader package, writes LightingRenderTargets::IndirectSpecular, and feeds that existing texture into LightingComposite. The shader should output black for now. Keep the pass backend-neutral, use existing shader/pass patterns from DirectLightingPass, and guard missing ray tracing/TLAS support with a deterministic no-op path. Run shader validation/cook checks and the architecture boundary check if available.
```

### Stage 2: Mirror Reflection Ray Query

Status: implemented on 2026-06-21.

Implementation note:

- `IndirectSpecular` now reconstructs world position from `GBufferDeviceZ`, decodes world-space GBuffer normals through shared GBuffer helpers, traces a mirror ray against the descriptor `SceneTlas`, and writes a visible temporary debug signal into `LightingRenderTargets::IndirectSpecular`.
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

Turn IndirectSpecular from black output into a full-resolution mirror ray-query pass. Reconstruct world position from depth, use GBuffer normals, trace a reflection ray against SceneTlas, and write a visible debug reflection signal into IndirectSpecular. Add debug modes for hit mask and hit distance. Keep material hit shading out of this stage unless the required hit-data buffers already exist cleanly. Preserve the no-op path for missing ray tracing or missing TLAS.
```

### Stage 3: Shader-Visible Hit Data Contract

Status: implemented on 2026-06-21.

Implementation note:

- `IndirectSpecular` now has a renderer-owned hit-data contract made of packed static hit vertices, packed triangle indices, per-render-instance hit offsets, existing `MeshInstances`, and compact material constants.
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

Add the smallest renderer-owned shader-visible hit-data contract needed for IndirectSpecular to shade ray hits with real scene materials. Reuse existing RenderSceneData, material cache, GPU mesh cache, and ray tracing scene data where possible. Do not expose GameFramework mutable data or backend-native acceleration structure details to the pass. Bind all hit-data buffers through typed shader parameters and frame/pass runtime services, and add guarded fallback behavior when the data is unavailable.
```

### Stage 3.5: Production Hit Data ABI Hardening

Status: implemented on 2026-06-21.

Implementation note:

- `RayTracingHitData::AbiVersion` is now `3`. The hit vertex ABI carries local position, local normal, tangent/sign, and `UV0`; shader reconstruction interpolates barycentrics as `(1 - x - y, x, y)`.
- Normal reconstruction transforms local normals by `MeshInstanceData::WorldInvTransposeMTX`. Tangents transform by `WorldMTX`; two-sided materials orient the reconstructed basis against the reflection ray before later normal-map work is added.
- The production policy for this stage is intentionally conservative: opaque static meshes are supported; alpha-tested, alpha-blended, skinned/deformed, missing mesh-hit-data, invalid material, invalid primitive, and invalid vertex-index cases are marked in the instance table with explicit fallback reasons.
- TLAS `InstanceID` continues to map to the render mesh instance index through the current classic TLAS path. Partitioned/PTLAS parity remains a validation item once the PTLAS compile drift is repaired.
- Debug modes now include hit UV, hit normal, material id, geometry class, fallback reason, and alpha policy so black/fallback hits can be diagnosed from shader output.

Goal: turn the Stage 3 bootstrap buffers into a production-ready ray-hit data ABI that can match raster material sampling.

Implementation tasks:

- define one versioned `RayTracingHitVertex`/mesh layout that includes at least position, normal, tangent/sign, and UV0
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

Harden the IndirectSpecular hit-data ABI from a bootstrap static-material path into a production ray-hit contract. Add UV0 and tangent/sign data, explicit mesh/material/geometry flags, TLAS InstanceID validation, unsupported-geometry diagnostics, and debug modes for UV/material/fallback reasons. Decide and implement the first alpha-tested and skinned/deformed geometry policy without leaking GameFramework or backend-native data into the pass. Keep all resources declared through typed pass parameters and preserve deterministic fallback behavior.
```

### Stage 4: Constants-Only Hit Material And Direct Lighting

Status: implemented on 2026-06-21.

Implementation note:

- `IndirectSpecular` now reconstructs constants-only hit material data: base color, alpha, roughness, metallic, dielectric F0, emissive, and subsurface constants.
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

Extend IndirectSpecular so ray hits are shaded with production constants-only material semantics and renderer lighting. Interpolate hit attributes including UV/tangent data, fetch material constants, apply the Stage 3.5 alpha/two-sided policy, evaluate the engine's current BRDF-compatible direct lighting, include emissive contribution, and use a deterministic miss fallback. Do not add bindless/material texture sampling in this stage; keep textured materials on explicit constants-only fallback diagnostics. Keep secondary shadow rays optional and disabled unless the existing ray tracing budget/settings make them clean. Keep LightingComposite source-agnostic by consuming IndirectSpecular.
```

### Stage 5: Stochastic GGX Importance Sampling

Status: implemented on 2026-06-21.

Implementation note:

- `IndirectSpecular` now has `r.RayTracing.Reflections.SampleMode`: `0=Mirror`, `1=StochasticGGX`. Stochastic GGX is the default, with mirror forced for the low-roughness limit.
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

- Reviewer controls now use the renderer ray tracing settings path through `IndirectSpecularSettings` and `RenderRayTracingPassServices`.
- User-facing CVars are:
  - `r.RayTracing.Reflections.Enabled`: toggles the pass; disabled mode is a deterministic no-op that leaves the existing `IndirectSpecular` producer intact.
  - `r.RayTracing.Reflections.SampleMode`: `0=Mirror`, `1=StochasticGGX`.
  - `r.RayTracing.Reflections.MaxDistance`: physical ray length limit.
  - `r.RayTracing.Reflections.DebugMode`: reflection-owned modes are `3=MirrorDirection`, `10=SampleDirection`, `11=SamplePdf`, `12=SampleThroughput`, `13=HitRadiance`, `14=FinalContribution`; shared ray-hit/material modes are `0=Off`, `1=HitMask`, `2=HitDistance`, `4=HitUV`, `5=HitNormal`, `6=MaterialId`, `7=GeometryClass`, `8=HitRejectionReason`, `15=MaterialBaseColor`, `16=MaterialRoughnessMetallic`, `17=MaterialEmissive`, `20=HitTangent`, `21=HitBitangent`, `22=HitNormalTangent`, `23=HitSampledNormal`, `24=AlphaAcceptedRejected`, `25=AlphaSample`, `26=AlphaCutoff`.
  - `r.RayTracing.Reflections.NormalBias`: ray origin bias; this is a geometric robustness control, not a lighting scale.
- The feature intentionally has no roughness cutoff/fade, intensity multiplier, or contribution clamp control.
- `IndirectSpecular` publishes status reasons through renderer smoke diagnostics: `disabled`, `unsupported`, `missing-tlas`, `missing-hit-data`, and `running`.
- Smoke diagnostics include enabled state, sample/debug modes, max distance, hit-data availability, hit instance/material counts, and the GPU timing label `RT Indirect Specular Ray Query`.
- Ray tracing frame timings now include `IndirectSpecularGpuMilliseconds` once timestamp results resolve.

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

Add user/reviewer-facing controls and diagnostics for IndirectSpecular. Expose enable, sample mode, max ray distance, and debug visualization through the renderer settings path already used by similar features. Do not add non-physical roughness cutoff, intensity, or contribution clamp controls. Publish pass timing and a clear status reason for running, disabled, unsupported, missing TLAS, or missing hit data. Add smoke/validation metadata if that path exists, and document the shader compiler and runtime validation commands.
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

Polish the no-denoiser stochastic IndirectSpecular baseline. Improve ray biasing, backface handling, sampling diagnostics, and debug views without adding history, denoising, roughness fade, intensity scaling, or contribution clamps. If a blue-noise resource can be integrated through existing renderer texture contracts, add it as an optional input; otherwise keep hash-based sampling. Update the docs with known quality limitations and recommended defaults.
```

### Stage 8: Bindless Material Texture Parity

Goal: add a renderer-owned descriptor-indexed or bindless material texture contract that can serve all material consumers, with `IndirectSpecular` as the first consumer that needs arbitrary material lookup after traversal.

Reference and source findings:

- Sparkle's current raster material path is bindful per draw: `MaterialCacheManager` resolves each material texture slot to a `RenderBindingSet`, and `GBufferMeshBatchDrawer` binds `TextureBaseColor`, `TextureNormal`, `TextureRoughness`, `TextureMetallic`, `TextureOcclusion`, `TextureEmissive`, `TextureSubsurfaceColor`, and `TextureSubsurfaceStrength` for the current material batch.
- `IndirectSpecular` cannot reuse per-draw texture bindings because a ray hit can land on any material after traversal. This is the first strong use case for a renderer material texture table, but the table itself must not be RT-owned.
- RHI already has early bindless metadata placeholders (`RhiBindlessBindingMetadata`) but no source-confirmed renderer pass parameter type for runtime-sized descriptor arrays yet. This means the final stage must start with RHI/compiler capability plumbing before shader sampling.
- DirectX Shader Model 6.6 dynamic resources allow shaders to index descriptor heaps directly, but require explicit root-signature/global flags and backend support.
- Vulkan descriptor indexing/bindless requires feature-gated descriptor arrays and non-uniform indexing semantics. Per-hit material indices are non-uniform by nature, so shader code must use the correct non-uniform indexing form once the cross-compile path supports it.
- NVIDIA Donut's relevant pattern is a scene-level bindless resource table populated from loaded scene resources. Sparkle should follow that shape: one renderer-owned material texture table plus compact per-material texture indices, not backend-native handles exposed to individual passes.

Design constraints:

- After Stage 8 material parity is enabled, every supported material texture table consumer path must be fully functional. Do not keep constants-only as a supported final fallback for textured materials.
- During bring-up substages, constants-only behavior is allowed only as a temporary disabled/not-yet-enabled state before texture sampling is wired. Once material texture table sampling is advertised as supported for a consumer, unsupported capability or invalid descriptor setup must fail closed with an explicit reason, not by silently shading textured materials as constants.
- Support renderer material binding modes deliberately:
  - `RaytracingOnly`: keep GBuffer/raster material bindings exactly as they are today and build a renderer material texture table that is bound only by ray tracing consumers such as `IndirectSpecular`. The table remains generic renderer material infrastructure; this mode only scopes its current consumers. This is the preferred bring-up and debug mode because direct raster material output stays a known-good comparison.
  - `Everything`: allow raster and RT to share the bindless material table once the table, shader compiler, RHI binding layouts, and validation scenes are stable.
- Do not add a `ConstantsOnlyFallback` final mode. If material texture table support is unavailable, texture sampling through that table is unsupported for that backend/configuration.
- Do not replace the existing GBuffer per-draw bindful path as part of table bring-up. Any raster bindless migration must be optional and separately switchable.
- Do not add roughness fades, intensity multipliers, contribution clamps, temporal history, or denoising as part of bindless material parity.
- Keep `LightingComposite` source-agnostic; it continues to consume `LightingRenderTargets::IndirectSpecular`.
- Keep GameFramework data behind snapshots and renderer scene translation. The shader sees compact renderer-owned material texture indices, not asset references or mutable scene objects.
- Texture sampling must use explicit LOD. Ray-hit shaders do not have reliable screen-space derivatives for arbitrary hit UVs.
- Texture parity should start with base color/roughness/metallic/emissive. Normal maps are a separate substage after tangent basis and LOD behavior are verified.

#### Stage 8.0: Bindless Capability And Shader Contract Audit

Goal: prove the backend/compiler/RHI contract needed for bindless material textures before touching the reflection shader.

Implementation note:

- Status: implemented as an audit/contract stage. `IndirectSpecular` does not sample material textures yet.
- Source-backed decision: use a fixed-capacity descriptor-indexed material texture array as the first cross-backend path. `ShaderTexture2D<T, ArrayCount>`, `PassParameterLayout::ArrayCount`, `PassParameterSet` array binding, and both D3D12/Vulkan binding layout compilers already carry fixed descriptor counts from shader reflection.
- Source-backed rejection for first pass: true runtime-sized bindless is not ready in the current source. `RhiBindlessBindingMetadata` exists as metadata only; D3D12 binding layout compilation does not set Shader Model 6.6 direct heap root signature flags or expose `ResourceDescriptorHeap`/`SamplerDescriptorHeap`; Vulkan binding layout compilation does not use descriptor-indexing layout flags such as variable descriptor count, partially bound, or update-after-bind; and the typed shader parameter system has fixed array counts rather than runtime-sized arrays.
- Active bring-up mode: `RaytracingOnly`. Raster/GBuffer keeps the current per-draw `MaterialCacheManager`/`GBufferMeshBatchDrawer` binding path while ray tracing consumers use the renderer-owned material texture table in later substages.
- `Everything` is available as a renderer material mode setting for future work, but the capability report currently marks it unsupported until raster has an explicit bindless path too.
- Capability reporting now exposes a generic `MaterialTextureTableCapabilityReport` with selected path, descriptor capacity, runtime-sized support, hybrid-mode support, full-bindless support, and a fail-closed reason. `RayTracingCapabilityReport` includes this generic material capability because ray tracing is the first consumer.
- Current fail-closed reasons include `backend-descriptor-model-unknown` and `shader-resource-descriptor-limit-unavailable`. Later table-building substages must add descriptor overflow and missing descriptor reasons before advertising texture parity as supported.
- No constants-only final material-texture mode is introduced. Constants-only behavior remains only an earlier-stage/not-yet-enabled RT material state, not a supported Stage 8 parity fallback.

Implementation tasks:

- inspect D3D12 binding-layout compilation for Shader Model 6.6 dynamic resources, descriptor heap flags, and resource descriptor heap indexing
- inspect Vulkan descriptor indexing support, descriptor array count limits, partially-bound/update-after-bind support, and non-uniform indexing codegen
- inspect shader reflection and `ShaderParameterStructBuilder` support for fixed-size texture arrays versus runtime-sized descriptor arrays
- add a renderer material binding mode enum/setting with at least `RaytracingOnly` and `Everything`
- decide the first supported cross-backend shape:
  - fixed-capacity descriptor array bound through typed pass parameters, or
  - true runtime-sized bindless descriptor heap/table
- make `RaytracingOnly` the first implementation target unless the audit proves `Everything` is already lower-risk
- add a renderer/RHI capability report field for material texture table support
- document exact fail-closed reasons for unsupported backend, unsupported compiler target, descriptor table overflow, and missing descriptors

Acceptance criteria:

- no reflection shader material sampling yet
- source-backed decision on fixed-array versus runtime-sized bindless path
- source-backed decision on active material binding mode, with `RaytracingOnly` available for debug comparisons
- no constants-only fallback is listed as a supported final material-texture mode
- D3D12 and Vulkan capability gates are explicit
- architecture boundary check still passes

Implementation prompt:

```text
Implement Stage 8.0 from Docs/Architecture/05-Implementation/RayTracedReflectionsImplementationPlan.md.

Audit and add the minimum backend-neutral capability contract for renderer material texture tables. Confirm whether Sparkle can safely expose a fixed-capacity descriptor array or true runtime-sized bindless table through existing shader reflection, binding layouts, D3D12, and Vulkan. Do not sample textures in IndirectSpecular yet. Add capability reporting and fail-closed unsupported reasons, then update this plan with the selected first path.

Keep raster bindful plus a renderer material texture table as the preferred bring-up/debug configuration. Add a material binding mode setting so later work can choose full bindless for raster and other consumers without rewriting the feature. Do not add constants-only as a supported final mode.
```

#### Stage 8.1: Renderer-Owned Material Texture Index Table

Goal: build a material-slot-indexed table of texture descriptor indices without changing reflection shading yet.

Implementation tasks:

- extend renderer material cache output with compact texture indices per `MaterialTextureSlots` entry
- keep per-draw `RenderBindingSet` creation for GBuffer unchanged in `RaytracingOnly`
- if `Everything` is selected later, add a separate raster binding path rather than deleting the bindful path
- create a renderer-owned `MaterialTextureTable` or equivalent owned by Renderer scene/material cache
- resolve missing material textures to the same default textures used by raster materials
- add table generation validation for material count, slot count, default fallback coverage, descriptor allocation failure, and overflow
- extend `RayTracingHitMaterial` with texture slot indices or an index-table base offset
- keep shader material-texture sampling disabled in this stage; do not advertise the RT material texture mode as supported yet

Acceptance criteria:

- every render material has stable texture indices for all `MaterialTextureSlots`
- existing GBuffer material rendering is unchanged in `RaytracingOnly`
- full bindless raster path, if enabled, can be toggled off for direct comparison
- RT hit material data carries texture indices but does not sample them yet
- missing textures resolve to raster-equivalent defaults
- descriptor allocation failure or overflow prevents enabling RT material-texture parity rather than falling back to constants

Implementation prompt:

```text
Implement Stage 8.1 from Docs/Architecture/05-Implementation/RayTracedReflectionsImplementationPlan.md.

Build the renderer-owned material texture index table. Reuse MaterialCacheManager and TextureManager resolution semantics so every material slot has stable indices for BaseColor, Normal, Roughness, Metallic, Occlusion, Emissive, SubsurfaceColor, and SubsurfaceStrength. Keep GBuffer's per-draw material binding path intact and keep IndirectSpecular material texture sampling disabled until the table is validated.

Implement this first for `RaytracingOnly`: raster keeps existing per-material bindful `RenderBindingSet` usage, while ray tracing consumers receive the new material texture index table. Do not remove or rewrite the bindful GBuffer path.
```

#### Stage 8.2: Bind Texture Table Through Typed Pass Parameters

Status: implemented on 2026-06-21.

Implementation note:

- The material texture table is still generic renderer scene/material infrastructure; `IndirectSpecular` is only the first table-aware consumer.
- `ShaderTexture2DTableSRV<N>` now represents a renderer-owned descriptor table through typed pass parameters without pretending the table is a frame-graph-owned texture array.
- The first concrete shader contract is a fixed-capacity descriptor array of 4096 `Texture2D` entries plus `MaterialTextureSampler`. Capability reporting now fails closed when the backend cannot support that fixed capacity.
- The shader package declares descriptor-indexing usage so DXC enables `SPV_EXT_descriptor_indexing` for SPIR-V cooks. Vulkan runtime support still fails closed until the Vulkan RHI reports descriptor-indexing capability explicitly.
- In `RaytracingOnly`, the table is bound only by `IndirectSpecularPass`; GBuffer/raster keeps the existing per-material bindful `RenderBindingSet` path.
- Production material texture shading is still disabled. `r.RayTracing.Reflections.DebugMode=15` samples hit base-color texture at hit `UV0` through the table to prove descriptor indexing by material slot and texture slot.
- Missing, invalid, empty, or overflowing table state prevents the RT material-texture path from running instead of silently shading textured hits as constants.

Goal: make the descriptor table visible to `IndirectSpecular` without changing final material output.

Implementation tasks:

- add typed shader parameter support for the selected descriptor table shape
- bind the material texture table and sampler through the first table-aware consumer, `IndirectSpecularPass`
- route binding through the active material binding mode:
  - `RaytracingOnly`: bind the table only for ray tracing consumers such as RT passes
  - `Everything`: bind the same table for RT and any opt-in raster path
- add shader-side helper declarations guarded by capability defines or table availability
- add debug-only shader reads that prove a descriptor can be addressed by material slot and texture slot
- fail closed when the table is unavailable, invalid, or unsupported; do not silently shade textured materials as constants in a supported material-texture mode
- avoid per-material branching explosions; indexing must be table-driven

Acceptance criteria:

- shader package validates and cooks for DXIL and supported SPIR-V target
- no backend-native handles leak into Renderer pass code
- debug view can prove table indexing without changing normal rendering
- missing table or unsupported capability reports an unavailable RT material-texture mode instead of producing a half-correct constants path

Implementation prompt:

```text
Implement Stage 8.2 from Docs/Architecture/05-Implementation/RayTracedReflectionsImplementationPlan.md.

Expose the renderer-owned material texture table to IndirectSpecular through typed pass parameters and backend-neutral pass services. Add shader helpers and debug-only table lookup validation, but keep production texture shading disabled until valid descriptor indexing is proven. Fail closed on unsupported backend, missing table, invalid material slot, invalid texture slot, or descriptor overflow; do not provide constants-only as a supported final fallback.

Honor the active material binding mode. In the preferred `RaytracingOnly` mode, do not bind or consume the table from GBuffer/raster passes.
```

#### Stage 8.3: Base Color, Roughness, Metallic, Emissive Texture Parity

Status: implemented on 2026-06-21.

Implementation note:

- `IndirectSpecular` now resolves base color, roughness, metallic, and emissive from the renderer material texture table during hit reconstruction before hit lighting.
- Value semantics match `Material.hlsli` for the first parity set: base color texture multiplies `BaseColor`, roughness and metallic use the texture red channel multiplied by their constants, and emissive texture RGB multiplies `EmissiveColor`.
- Explicit LOD policy for this stage is fixed mip 0 through `SampleLevel`. This avoids relying on unavailable ray-hit derivatives and keeps first texture parity deterministic; roughness-biased or cone/mip selection is left for a later quality stage.
- Untextured materials use their authored constants because that is their complete material. Textured material slots require valid descriptor indices before the material texture table is enabled; descriptor correctness is a renderer material-table contract, not a per-hit shader fallback.
- Debug modes now include resolved sampled base color, roughness/metallic, emissive, selected mip, and invalid descriptor visualization.

Goal: make ray-hit material constants match raster material value semantics for the first texture set.

Implementation tasks:

- sample base color, roughness, metallic, and emissive textures at hit `UV0`
- match current `Material.hlsli` multiplication semantics:
  - base color texture multiplied by material base color constant
  - roughness texture red channel multiplied by material roughness constant
  - metallic texture red channel multiplied by material metallic constant
  - emissive texture RGB multiplied by material emissive constant
- use explicit texture LOD; first policy should be fixed mip 0 or a conservative roughness-biased LOD, documented before implementation
- keep occlusion, subsurface texture, normal map, and alpha-tested candidate-hit behavior deferred unless trivial after base parity
- add debug views for sampled base color, sampled roughness/metallic, sampled emissive, selected mip, and invalid descriptor

Acceptance criteria:

- textured base color/roughness/metallic/emissive materials visible in mirrors and stochastic reflections match direct GBuffer material semantics closely
- invalid or unavailable texture table disables RT material texture parity with an explicit reason
- full roughness range remains supported
- no contribution clamp/intensity/roughness fade is introduced

Implementation prompt:

```text
Implement Stage 8.3 from Docs/Architecture/05-Implementation/RayTracedReflectionsImplementationPlan.md.

Use the IndirectSpecular material texture table to sample base color, roughness, metallic, and emissive textures at ray-hit UV0 with explicit LOD. Match Material.hlsli value semantics exactly for these slots and fail closed for unsupported or invalid texture access. Add debug views for sampled material values and selected mip. Do not add normal maps, alpha-tested candidate hits, denoising, roughness fades, contribution clamps, intensity scaling, or constants-only fallback mode.
```

#### Stage 8.4: Normal Map And Tangent-Space Parity

Status: implemented on 2026-06-21.

Implementation note:

- `IndirectSpecular` now samples the normal texture when the material normal-map flag is present, using the same `Material.hlsli` unpacking convention: `xy * 2 - 1`, reconstructed positive `z`, normalized.
- The hit tangent basis is reconstructed from the Stage 3.5 ABI as `T = orthonormalized hit tangent`, `B = tangentSign * normalize(cross(N, T))`, and `N = geometric world normal`. The tangent-space normal is transformed with the same `mul(normalTangent, float3x3(T, B, N))` convention as raster.
- Two-sided backface handling is applied after normal-map transformation: one-sided backfaces still fail closed, while two-sided hits flip the final sampled normal and debug basis toward the incoming ray.
- Normal-map descriptors use the same material-table contract as the other texture slots. Flagged normal-mapped materials must have valid descriptor indices before texture parity is enabled.
- Debug modes now expose hit tangent, hit bitangent, sampled tangent-space normal, and final sampled world normal.

Goal: add ray-hit normal map sampling after base material texture parity is stable.

Implementation tasks:

- sample normal texture using the same unpacking semantics as `Material.hlsli`
- reconstruct bitangent from hit normal, tangent, and tangent sign
- transform tangent-space normal to world space using the same convention as raster materials
- deliberately handle two-sided normal orientation after normal-map application
- add debug views for tangent, bitangent, normal-map tangent normal, and final world normal
- validate non-uniform scale and mirrored tangent sign with parity scenes

Acceptance criteria:

- normal-mapped materials reflect with the same lighting orientation as direct GBuffer surfaces
- tangent/sign and two-sided behavior are explicit and debug-visible
- degenerate tangent fallback remains deterministic

Implementation prompt:

```text
Implement Stage 8.4 from Docs/Architecture/05-Implementation/RayTracedReflectionsImplementationPlan.md.

Add normal-map sampling to IndirectSpecular material texture parity. Use the existing hit tangent/sign ABI, reconstruct bitangent, match Material.hlsli normal unpacking and tangent-to-world semantics, and keep robust fallback for degenerate tangents or missing normal descriptors. Add debug views for tangent basis and final sampled normal.
```

#### Stage 8.5: Alpha-Tested Candidate-Hit Policy

Status: implemented on 2026-06-21.

Implementation note:

- `IndirectSpecular` now uses inline `RayQuery` candidate-hit handling for alpha-tested geometry. The pass no longer uses `RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH`; non-opaque candidates reconstruct instance, primitive, barycentrics, UV0, and material before deciding whether to commit.
- Classic TLAS now has backend-neutral per-instance flags, mapped to D3D12/Vulkan force-non-opaque instance flags. Alpha-tested render instances set the non-opaque bit so candidate hits are visible to inline ray queries. Partitioned TLAS full builds and logical updates set the existing `ForceNoOpaque` flag for the same alpha-tested material class.
- Candidate alpha uses the same base-color alpha value semantics as Stage 8.3: base-color texture alpha multiplied by the material base-color alpha constant, sampled at explicit mip 0. Non-textured alpha-tested materials use their authored base-color alpha.
- Candidates with sampled alpha below `AlphaCutoff` are rejected and traversal continues. Candidates with invalid hit data, invalid material, unsupported blended alpha mode, or invalid required texture descriptors are committed deliberately so the final hit fails closed instead of silently revealing geometry behind it.
- Alpha-blended geometry remains unsupported and reports `UnsupportedAlphaMode`; it is not treated as cutout transparency.
- Debug modes now expose alpha accepted/rejected state, sampled alpha, cutoff, and candidate fallback reason.

Goal: support alpha-tested geometry only after base-color alpha can be sampled through the material texture table.

Implementation tasks:

- choose inline `RayQuery` candidate-hit handling for alpha-tested triangles
- during candidate hit, reconstruct hit UV/material enough to sample base-color alpha
- compare sampled alpha against material alpha cutoff
- commit or reject candidate hits deliberately
- keep alpha-blended geometry unsupported/fallback unless a separate transmission/refraction policy exists
- add debug views for alpha accepted/rejected, sampled alpha, cutoff, and candidate-hit fallback reason

Acceptance criteria:

- alpha-tested geometry no longer shades as opaque in reflections
- alpha rejection cannot silently select the wrong material
- alpha-blended geometry remains explicitly unsupported/fallback

Implementation prompt:

```text
Implement Stage 8.5 from Docs/Architecture/05-Implementation/RayTracedReflectionsImplementationPlan.md.

Add alpha-tested candidate-hit support to IndirectSpecular using inline RayQuery candidate hit handling. Reconstruct candidate UV/material, sample base-color alpha through the material texture table with explicit LOD, compare against alpha cutoff, and commit or reject the candidate hit. Keep alpha-blended materials unsupported with explicit fallback.
```

#### Stage 8.6: Material Parity Validation Scenes And Defaults

Status: implemented on 2026-06-21.

Implementation note:

- Added Showcase validation startup levels:
  - `IndirectSpecularMaterialParity_DamagedHelmet`
  - `IndirectSpecularMaterialParity_AlphaTest`
  - `IndirectSpecularMaterialParity_RoughnessRange`
- Added the validation recipe at [IndirectSpecularMaterialParityValidation.md](../03-Validation/IndirectSpecularMaterialParityValidation.md). It maps each material-parity requirement to a startup level, ray-traced reflection mode, debug mode, and backend path.
- `RaytracingOnly` remains the primary debug binding mode because raster/GBuffer stays bindful and acts as the direct-material baseline.
- `Everything` remains deferred until raster bindless opt-in exists and can be toggled against the bindful baseline.
- The current Showcase content does not include a dedicated synthetic constant-only swatch scene. The validation note documents that gap explicitly rather than claiming full synthetic material-grid coverage.

Goal: close the final reflection material-parity stage with repeatable scenes and documented defaults.

Implementation tasks:

- add validation scene coverage for direct-vs-reflected:
  - constants-only material
  - base-color textured material
  - roughness textured material
  - metallic textured material
  - emissive textured material
  - normal-mapped material
  - alpha-tested material after Stage 8.5
- validate mirror mode and stochastic GGX full roughness range
- validate D3D12 path and supported Vulkan path separately
- validate unsupported bindless capability disables RT material texture parity with an explicit reason
- validate `RaytracingOnly` against direct GBuffer output as the primary debug configuration
- validate `Everything` only after raster opt-in exists and can be toggled against the bindful baseline
- document known remaining differences from raster material evaluation

Acceptance criteria:

- reviewer can see mirror and importance-sampled reflections across the full roughness scale with proper lighting and material textures
- unsupported bindless/material-table configurations fail closed instead of presenting half-correct constants-only textured materials
- known mismatches are documented rather than hidden

Implementation prompt:

```text
Implement Stage 8.6 from Docs/Architecture/05-Implementation/RayTracedReflectionsImplementationPlan.md.

Add focused validation scenes and documentation for IndirectSpecular material texture parity. Cover mirror and stochastic GGX modes across the full roughness range, direct-vs-reflected material comparisons, textured material slots, normal maps, alpha-tested geometry if implemented, D3D12, supported Vulkan, and unsupported bindless fail-closed behavior. Keep this as validation coverage and documentation, not new rendering behavior.
```

## Suggested Validation Commands

Exact command names can differ by local build output, so confirm paths from the current build tree before wiring these into automation:

```powershell
cmake --build build --target ShaderCompiler --config DevelopmentEditor
.\artifacts\dev\tools\ShaderCompiler\DevelopmentEditor\ShaderCompiler.exe list-shaders --validate
.\artifacts\dev\tools\ShaderCompiler\DevelopmentEditor\ShaderCompiler.exe inspect-shader IndirectSpecularCS
.\artifacts\dev\tools\ShaderCompiler\DevelopmentEditor\ShaderCompiler.exe cook --package IndirectSpecular --backend dxc --target DxilSm66
.\artifacts\dev\tools\ShaderCompiler\DevelopmentEditor\ShaderCompiler.exe cook --package IndirectSpecular --backend dxc --target SpirV16
.\artifacts\dev\tools\ShaderCompiler\DevelopmentEditor\ShaderCompiler.exe inspect-package <path-to-IndirectSpecular-cooked-package>
cmake --build build --target architecture_boundary_check --config DevelopmentEditor
cmake --build build --target SparkleRenderer --config DevelopmentEditor
```

Runtime validation should cover:

- D3D12 with ray tracing enabled
- Vulkan with ray tracing enabled if supported in the local build
- material texture parity levels and commands from [IndirectSpecularMaterialParityValidation.md](../03-Validation/IndirectSpecularMaterialParityValidation.md)
- unsupported or missing ray tracing capability
- no scene TLAS / empty scene
- reflections disabled: set `r.RayTracing.Reflections.Enabled=0` and verify smoke status `disabled` plus visual equivalence to the previous renderer output
- missing TLAS: use an empty/no-traceable scene and verify smoke status `missing-tlas`
- unsupported descriptor-TLAS path/backend: verify smoke status `unsupported`
- missing hit data: force or reproduce missing RT hit buffers and verify smoke status `missing-hit-data` plus deterministic shader fallback
- mirror debug mode: set `r.RayTracing.Reflections.Enabled=1`, `r.RayTracing.Reflections.SampleMode=0`
- stochastic GGX mode: set `r.RayTracing.Reflections.Enabled=1`, `r.RayTracing.Reflections.SampleMode=1`
- status/timing metadata: capture smoke diagnostics and verify `RayTracing.IndirectSpecular.StatusReason`, `HitInstanceCount`, `HitMaterialCount`, and `FrameTimings.IndirectSpecularGpuMilliseconds`
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

- `IndirectSpecular` is a full-resolution inline ray-query compute pass
- it importance samples reflection directions from material roughness
- it shades ray hits with real renderer material and lighting data
- it reaches texture material parity through the final renderer-owned material texture table stage and fails closed when that table is unavailable
- it composes into the lighting path through declared frame-graph resources
- it has deterministic no-op/fallback paths
- it exposes controls and diagnostics
- it has validation coverage for enabled, disabled, unsupported, and missing-data paths
- no denoiser, temporal history, or backend-native renderer dependency has been introduced


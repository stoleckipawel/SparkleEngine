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

## Sampling Requirements

Start simple, but keep the stochastic foundation correct:

- reconstruct world position from depth and camera data
- decode normal and material parameters from the GBuffer
- compute view vector and reflection basis
- sample a GGX visible-normal or half-vector distribution using roughness
- trace one ray per pixel by default
- apply a minimum roughness clamp to avoid pathological fireflies
- offset ray origin along the geometric normal to reduce self-intersection
- cap ray distance through a renderer setting
- return black for invalid depth, invalid normal, unsupported material, or missing TLAS
- use deterministic per-pixel/per-frame random seeds from frame index and pixel coordinate

Because there is no denoiser yet, expose conservative settings:

- enable/disable ray traced reflections
- max ray distance
- roughness cutoff or fade
- intensity
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

### Stage 4: Proper Hit Material And Direct Lighting

Goal: evaluate a plausible direct-lighting contribution at the reflection hit.

Implementation tasks:

- interpolate hit normal and material data
- evaluate the same or intentionally matched BRDF terms used by direct lighting
- evaluate directional, point, and spot lights according to current renderer lighting data
- apply visibility policy deliberately:
  - first option: direct light without secondary shadow rays
  - later option: shadow ray from hit point to light when budget allows
- include emissive material contribution
- add sky/environment miss lighting if a renderer-owned source exists

Acceptance criteria:

- mirror reflections show material color and direct lighting from hit surfaces
- material roughness/metallic or current engine material parameters influence reflection color
- emissive hit surfaces contribute visibly
- debug views still work

Implementation prompt:

```text
Implement Stage 4 from Docs/Architecture/05-Implementation/RayTracedReflectionsImplementationPlan.md.

Extend RTIndirectSpecular so ray hits are shaded with real material data and renderer lighting. Interpolate hit attributes, fetch the hit material, evaluate the engine's current BRDF-compatible direct lighting, include emissive contribution, and use a deterministic miss fallback. Keep secondary shadow rays optional and disabled unless the existing ray tracing budget/settings make them clean. Keep LightingComposite source-agnostic by consuming IndirectSpecular.
```

### Stage 5: Stochastic GGX Importance Sampling

Goal: move from mirror-only rays to rough-specular stochastic importance sampling.

Implementation tasks:

- add a reflection settings uniform
- generate deterministic per-pixel/per-frame random values
- importance sample GGX from roughness
- compute sample PDF and BRDF weight correctly enough for one-sample stochastic output
- blend/fade reflections by roughness cutoff to avoid unusable noise on very rough materials
- add mirror debug mode to compare against stochastic mode
- clamp or otherwise control extreme contribution values

Acceptance criteria:

- smooth materials behave close to mirror mode
- rough materials produce stochastic spread
- sample weighting is energy-aware and does not obviously brighten/darken with roughness
- no denoiser or history dependency is introduced

Implementation prompt:

```text
Implement Stage 5 from Docs/Architecture/05-Implementation/RayTracedReflectionsImplementationPlan.md.

Replace mirror-only sampling with a stochastic GGX importance-sampled reflection mode while keeping mirror as a debug mode. Use deterministic per-pixel/per-frame random seeds, material roughness, a correct PDF/BRDF weighting path, max ray distance, roughness fade/cutoff, and contribution clamping. Do not add denoising or temporal accumulation. Keep the pass full resolution and inline ray-query based.
```

### Stage 6: Controls, Diagnostics, And Validation

Goal: make the feature easy to review, tune, and test.

Implementation tasks:

- add renderer settings for enable, mode, max distance, roughness cutoff, intensity, and debug visualization
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

Add user/reviewer-facing controls and diagnostics for RTIndirectSpecular. Expose enable, sample mode, max ray distance, roughness cutoff, intensity, and debug visualization through the renderer settings path already used by similar features. Publish pass timing and a clear status reason for running, disabled, unsupported, missing TLAS, or missing hit data. Add smoke/validation metadata if that path exists, and document the shader compiler and runtime validation commands.
```

### Stage 7: Quality Cleanup Before Denoising

Goal: stabilize the stochastic base before any denoiser or temporal reuse is designed.

Implementation tasks:

- improve ray origin bias and normal handling
- reduce fireflies with material-aware clamps
- handle backfaces and thin geometry deliberately
- tune roughness fade defaults
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

Polish the no-denoiser stochastic RTIndirectSpecular baseline. Improve ray biasing, backface handling, roughness fade, firefly control, and debug views without adding history or denoising. If a blue-noise resource can be integrated through existing renderer texture contracts, add it as an optional input; otherwise keep hash-based sampling. Update the docs with known quality limitations and recommended defaults.
```

## Suggested Validation Commands

Exact command names can differ by local build output, so confirm paths from the current build tree before wiring these into automation:

```powershell
ShaderCompiler list-shaders --validate
ShaderCompiler inspect-shader RTIndirectSpecularCS
ShaderCompiler cook --package RTIndirectSpecular --backend dxc --target DxilSm66
ShaderCompiler inspect-package <path-to-RTIndirectSpecular-cooked-package>
cmake --build <build-dir> --target ArchitectureBoundaryCheck
```

Runtime validation should cover:

- D3D12 with ray tracing enabled
- Vulkan with ray tracing enabled if supported in the local build
- unsupported or missing ray tracing capability
- no scene TLAS / empty scene
- reflections disabled
- mirror debug mode
- stochastic GGX mode

## First-Implementation Defaults

Recommended defaults for the first usable version:

- enabled: false until hit material shading is correct
- mode: mirror debug for Stage 2, stochastic GGX after Stage 5
- samples per pixel: 1
- max ray distance: 50 meters or current renderer world-unit equivalent
- roughness cutoff/fade: begin fading around 0.45, fully fade by 0.65
- intensity: 1.0
- miss lighting: black unless a renderer-owned sky/environment lookup is already cleanly available

## Known Risks

- Inline ray queries can prove the effect quickly, but hit shading requires shader-visible geometry/material data that may not be exposed yet.
- One sample per pixel without denoising will be noisy on rough materials; the plan handles this with roughness fade and conservative defaults.
- If Vulkan inline ray query support or acceleration structure binding differs from D3D12, keep that in capability checks and shader package features rather than adding renderer-native backend branches.
- Reflections can double count specular if the composite path does not clearly separate screen/local lighting from ray traced reflected lighting.
- If material data used by GBuffer and reflection hit shading diverges, visual mismatch will be obvious; prefer reusing the same renderer material packing where possible.

## Done Definition

The feature is done for this plan when:

- `RTIndirectSpecular` is a full-resolution inline ray-query compute pass
- it importance samples reflection directions from material roughness
- it shades ray hits with real renderer material and lighting data
- it composes into the lighting path through declared frame-graph resources
- it has deterministic no-op/fallback paths
- it exposes controls and diagnostics
- it has validation coverage for enabled, disabled, unsupported, and missing-data paths
- no denoiser, temporal history, or backend-native renderer dependency has been introduced

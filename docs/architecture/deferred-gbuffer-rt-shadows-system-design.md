# Deferred GBuffer With Ray-Traced Shadows System Design

Date: 2026-05-03

## Executive Summary

Sparkle should move the opaque renderer to a deferred architecture before pursuing full primary-ray lighting. The target frame sequence is:

```text
GBuffer -> Deferred Lighting with inline ray-query shadows -> CopySceneColorToBackBuffer -> Present
```

This design replaces the current `ShadowPasses -> ForwardOpaquePass -> CopySceneColorToBackBuffer` sequence with a conventional deferred path. It keeps ray tracing focused on shadow visibility first, where it gives immediate value without requiring a full `DispatchRays` pipeline, shader tables, or ray tracing state objects. A real BLAS/TLAS scene is still required, but the lighting shader can use inline ray queries instead of launching a separate ray tracing pipeline.

Tonemapping is intentionally deferred. The first milestone should keep `SceneColor` copy-compatible with the swapchain back buffer so the frame can present through the regular copy pass. HDR `SceneColor` should return with a dedicated tonemap/conversion pass later.

## Goals

- Build a PBR-ready opaque GBuffer.
- Add deferred direct lighting over the GBuffer.
- Add ray-traced shadow visibility through inline ray queries in deferred lighting.
- Replace `ForwardOpaquePass` as the normal opaque path.
- Remove raster shadow maps and cascaded shadow-map frame setup once RT shadows are validated.
- Keep materials bindful for this milestone.
- Keep all new GPU resources behind RHI/renderer-owned handles so D3D12MA and Vulkan can be introduced later.

## Non-Goals

- Full tonemapping/exposure pipeline.
- Full `DispatchRays` direct lighting, shader tables, or RT pipeline state objects.
- GI, path tracing, reflections, translucency, procedural geometry, or alpha-tested shadow correctness beyond a later follow-up.
- Skeletal/morph animation import.
- D3D12MA integration.
- Vulkan backend implementation.
- Async acceleration-structure build scheduling or BLAS compaction optimization.

## Implementation Status

Phase 6 is implemented in the current source tree. The normal opaque frame sequence is now:

```text
CreateSceneTargets
CreateGBufferTargets
AddGBufferPass
DeferredLightingPass
CopySceneColorToBackBuffer
Present
```

The active render pass runtime set is `GBufferPass`, `DeferredLightingPass`, and `ComputeClearPass`. `ForwardOpaquePass`, `ShadowOpaquePass`, `ForwardPasses`, `ShadowPasses`, `ShadowFrameBuilder`, `ShadowBuilder`, raster shadow-map shader registrations, and raster shadow-map constant data have been removed. General per-light `CastShadow` metadata remains because deferred inline ray-query shadows still use it.

Validation completed on 2026-05-03 for build, shader registry, and shader cook coverage. Runtime startup reaches the expected DXR capability gate on hardware without DXR tier 1.1 inline ray-query support; visual RT shadow validation still requires a DXR-capable machine.

## Implemented Architecture Snapshot

Sparkle now has the deferred and inline RT shadow foundations needed by this milestone:

- [Engine/Renderer/Private/FrameGraph/Builder/FrameGraphBuilder.cpp](../../Engine/Renderer/Private/FrameGraph/Builder/FrameGraphBuilder.cpp) owns the deferred frame sequence and wires GBuffer, deferred lighting, and presentation passes.
- [Engine/Renderer/Private/FrameGraph/Features/PresentationPasses.cpp](../../Engine/Renderer/Private/FrameGraph/Features/PresentationPasses.cpp) creates `SceneColor`, `BackBuffer`, and `MainDepth` and copies scene color to the back buffer.
- [Engine/Renderer/Private/FrameGraph/Features/GBufferPasses.cpp](../../Engine/Renderer/Private/FrameGraph/Features/GBufferPasses.cpp) creates the GBuffer products, including dedicated `GBufferDeviceZ`.
- [Engine/Renderer/Private/Passes/GBufferPass.cpp](../../Engine/Renderer/Private/Passes/GBufferPass.cpp) owns the opaque mesh/material draw loop for material attribute export.
- [Engine/Renderer/Private/Passes/DeferredLightingPass.cpp](../../Engine/Renderer/Private/Passes/DeferredLightingPass.cpp) owns compute deferred lighting, GBuffer reads, TLAS binding, and RT shadow constants.
- [Engine/Renderer/Private/Passes/PassUtilities.h](../../Engine/Renderer/Private/Passes/PassUtilities.h) provides pass binding helpers and full-screen triangle support.
- [Engine/Renderer/Public/ShaderParameters/ShaderParameterFields.h](../../Engine/Renderer/Public/ShaderParameters/ShaderParameterFields.h) and [Engine/Renderer/Public/ShaderParameters/PassParameterSet.h](../../Engine/Renderer/Public/ShaderParameters/PassParameterSet.h) already support render targets, depth targets, read textures, RW textures, buffers, uniforms, and samplers.
- [Engine/RHI/Private/D3D12/Pipeline/D3D12BindingLayout.cpp](../../Engine/RHI/Private/D3D12/Pipeline/D3D12BindingLayout.cpp) recognizes acceleration-structure shader semantics as SRV descriptor-table bindings.
- [Engine/Assets/Shaders/BRDF/BRDF.hlsli](../../Engine/Assets/Shaders/BRDF/BRDF.hlsli) and [Engine/Assets/Shaders/Passes/Deferred/DeferredLighting.hlsl](../../Engine/Assets/Shaders/Passes/Deferred/DeferredLighting.hlsl) contain the BRDF and deferred direct-lighting code used by the compute pass.

Remaining gaps:

- Depth SRV/typeless view handling is not yet explicit in the public format abstraction.
- HDR scene color, tonemapping, and exposure remain deferred to Phase 7.
- Visual RT shadow validation still requires DXR tier 1.1 inline ray-query hardware.

## Target Framegraph

```text
CreateSceneTargets
CreateGBufferTargets
AddGBufferPass
Wire DeferredLightingPass
CopySceneColorToBackBuffer
Present
```

The first stable version should validate deferred lighting with shadow visibility forced to `1.0`. After GBuffer packing, position reconstruction, and lighting match the current forward path closely enough, add RT scene construction and inline ray-query shadows.

## Proposed GBuffer Layout

| Texture | Initial Format | Contents |
| --- | --- | --- |
| `GBufferBaseColor` | `R8G8B8A8_UNorm` | Base color RGB, optional alpha/material coverage in A. |
| `GBufferNormal` | `R16G16B16A16_Float` | World normal XYZ, spare channel A. |
| `GBufferMaterial` | `R8G8B8A8_UNorm` | Metallic, roughness, occlusion, material flags. |
| `GBufferEmissive` | `R16G16B16A16_Float` | Emissive RGB, optional spare A. |
| `GBufferDeviceZ` | `R32_Float` | Raw device Z copied from raster depth for high precision deferred reads. |
| `MainDepth` | `D24_UNorm_S8_UInt` initially | Depth target for raster depth testing; not sampled by deferred lighting in this phase. |
| `SceneColor` | `R8G8B8A8_UNorm` initially | Copy-compatible deferred lighting output; HDR moves to the tonemapping phase. |

Deferred shader helpers treat `DeviceZ` as the raw depth-buffer value and reconstruct world position directly from it. Linear depth helpers and depth SRV/typeless support remain deferred until a pass actually needs them.

## System Components

### GBuffer Pass

`GBufferPass` replaces opaque material shading with material attribute export. It should reuse the `ForwardOpaquePass` mesh loop, per-object constants, material constants, and material texture table binding. It writes multiple render targets and `MainDepth`.

### Deferred Lighting Pass

`DeferredLightingPass` is a compute pass that reads GBuffer textures and `GBufferDeviceZ`, reconstructs position, evaluates direct lighting, applies emissive, and writes `SceneColor` through a UAV. It should start without ray tracing so GBuffer correctness is easy to debug.

### Ray Tracing Scene

`RayTracingScene` or `RayTracingSceneManager` owns BLAS/TLAS resources, scratch/result buffers, instance records, dirty tracking, and diagnostics. It should support moving-instance TLAS rebuilds and CPU-dirty mesh BLAS rebuilds. Skeletal/morph import remains outside this milestone.

### RT Shadows In Deferred Lighting

Deferred lighting gets the TLAS as an acceleration-structure parameter. For each shadow-casting directional light, the shader reconstructs the world-space surface point from GBuffer/depth, casts a shadow ray toward the light with bias, and multiplies direct light by visibility.

### Presentation Copy

Until tonemapping exists, keep `SceneColor` in the same format as the swapchain back buffer and present it with the regular copy pass. The deferred lighting shader writes into the copy-compatible target; any HDR-preserving path belongs in the later tonemapping phase.

## Implementation Phases

Each phase below is written as a prompt-ready implementation unit. Run them in order unless a phase explicitly says work can proceed in parallel.

### Phase 1: Format And Framegraph Products

**Objective**

Prepare the RHI and framegraph for a deferred renderer by adding GBuffer formats, GBuffer product handles, and a copy-compatible scene target.

**Prompt**

```text
Implement Phase 1 of docs/architecture/deferred-gbuffer-rt-shadows-system-design.md.

Goal: add the render formats and framegraph products needed for a deferred GBuffer pipeline.

Scope:
- Extend PixelFormat with the GBuffer/HDR formats needed by this document, at minimum R16G16B16A16_Float.
- Update D3D12 type conversions and resource/view format mapping for the new formats.
- Add RenderConfig::GBuffer constants for base color, normal, material, emissive, and SceneColor formats.
- Add FrameGraphGBufferTargets or equivalent product structs in FrameGraphProducts.h.
- Update scene target creation so SceneColor remains copy-compatible with BackBuffer for this phase.
- Keep presentation on the regular CopySceneColorToBackBuffer path.

Constraints:
- Keep changes backend-neutral at public RHI boundaries.
- Do not add tonemapping yet.
- Do not introduce D3D12MA.
- Keep the existing presentation path working for current callers.

Relevant files:
- Engine/RHI/Public/Formats/PixelFormat.h
- Engine/RHI/Private/D3D12/D3D12TypeConversions.cpp
- Engine/RHI/Public/Config/RenderConfig.h
- Engine/Renderer/Private/FrameGraph/Features/FrameGraphProducts.h
- Engine/Renderer/Private/FrameGraph/Features/PresentationPasses.cpp
- Engine/Renderer/Private/FrameGraph/Features/PresentationPasses.h

Acceptance checks:
- Project builds after adding formats and products.
- Existing framegraph still creates SceneColor, BackBuffer, and MainDepth.
- SceneColor and BackBuffer remain copy-compatible.
- No forward/deferred pass behavior is changed yet.
```

**Verification**

- Build `ShowcaseRuntime`.
- Build `sparkle_validation_check`.
- Inspect D3D12 debug output for invalid resource/view format errors.

### Phase 2: GBuffer Pass

**Objective**

Add the opaque GBuffer pass and shaders, reusing the existing forward opaque draw loop and material bindings.

**Prompt**

```text
Implement Phase 2 of docs/architecture/deferred-gbuffer-rt-shadows-system-design.md.

Goal: add GBufferPass as the main opaque material attribute pass.

Scope:
- Add Engine/Renderer/Private/Passes/GBufferPass.h/.cpp.
- Define GBufferPassParameters with render targets for base color, normal, material, emissive, and MainDepth.
- Define GBufferDrawParameters using existing per-object VS/PS constants and material texture table bindings.
- Reuse the ForwardOpaquePass mesh/material draw structure, but write material attributes instead of lighting.
- Add GBuffer shader registrations and HLSL under Engine/Assets/Shaders/Deferred or a similarly clear folder.
- Add GBufferPassRuntime and RenderPassPipelineTraits<GBufferPass> with multiple render target formats and depth writes enabled.
- Add FrameGraphFeatures::AddGBufferPass to allocate/bind GBuffer targets.

Constraints:
- Keep material access bindful, matching the existing material texture table approach.
- Do not add deferred lighting in this phase.
- Do not remove ForwardOpaquePass yet unless it is fully unused by this phase's framegraph wiring.
- Keep shader registrations cooked-only; do not reintroduce runtime shader compilation.

Relevant files:
- Engine/Renderer/Private/Passes/ForwardOpaquePass.h/.cpp
- Engine/Renderer/Private/Passes/GBufferPass.h/.cpp
- Engine/Renderer/Private/FrameGraph/Features/FrameGraphProducts.h
- Engine/Renderer/Private/FrameGraph/Features/ForwardPasses.cpp or new GBufferPasses.cpp
- Engine/Renderer/Private/FrameGraph/RenderPassRuntime.h
- Engine/Renderer/Private/Pipeline/RenderPassPipelineTraits.h
- Engine/Assets/Shaders/Deferred/*
- Engine/RHI/Private/Shaders/* shader registration files

Acceptance checks:
- GBuffer pass compiles and can be registered in the runtime registry.
- GBuffer HLSL cooks successfully.
- GBuffer outputs show sensible values in PIX/RenderDoc once wired.
```

**Verification**

- Build `ShaderCompiler`.
- Cook the GBuffer shader package for Showcase.
- Build `ShowcaseRuntime`.
- Capture a frame and inspect GBuffer textures.

### Phase 3: Deferred Lighting Without RT

**Objective**

Add deferred direct lighting over the GBuffer with shadow visibility forced to fully lit. This validates GBuffer packing, world-position reconstruction, BRDF reuse, and HDR output before introducing ray tracing.

**Prompt**

```text
Implement Phase 3 of docs/architecture/deferred-gbuffer-rt-shadows-system-design.md.

Goal: add DeferredLightingPass without RT shadows and wire the renderer sequence to GBuffer -> deferred lighting -> present.

Scope:
- Add Engine/Renderer/Private/Passes/DeferredLightingPass.h/.cpp.
- Implement a compute pass that dispatches one thread per output pixel.
- Add DeferredLightingPassParameters that read GBuffer textures and depth/depth-derived data, bind per-frame/per-view lighting constants, and write SceneColor as a UAV.
- Add deferred lighting HLSL that reconstructs world position, reads material values, evaluates direct diffuse/specular lighting, adds emissive, and writes SceneColor.
- Treat shadow visibility as 1.0 in this phase.
- Add DeferredLightingPassRuntime and RenderPassPipelineTraits<DeferredLightingPass>.
- Update FrameGraphBuilder to build CreateSceneTargets -> AddGBufferPass -> DeferredLightingPass -> CopySceneColorToBackBuffer.

Constraints:
- Do not add RT scene or ray queries yet.
- Do not add tonemapping.
- Keep old raster shadows out of the new deferred path.
- Prefer reuse of existing BRDF/Lighting shader includes.

Relevant files:
- Engine/Renderer/Private/Passes/DeferredLightingPass.h/.cpp
- Engine/Renderer/Private/Passes/PassUtilities.h
- Engine/Renderer/Private/FrameGraph/Builder/FrameGraphBuilder.cpp
- Engine/Renderer/Private/FrameGraph/Features/FrameGraphProducts.h
- Engine/Renderer/Private/FrameGraph/RenderPassRuntime.h
- Engine/Renderer/Private/Pipeline/RenderPassPipelineTraits.h
- Engine/Assets/Shaders/Deferred/*
- Engine/Assets/Shaders/BRDF/*
- Engine/Assets/Shaders/Lighting/LightEvaluation.hlsli

Acceptance checks:
- Deferred lighting renders opaque meshes using GBuffer data.
- Visual output roughly matches the existing forward path before shadows.
- SceneColor is written by compute and reaches the back buffer through the regular copy path.
- No ray tracing code is required to run this phase.
```

**Verification**

- Build `ShaderCompiler` and cook GBuffer/deferred packages.
- Build `ShowcaseRuntime`.
- Compare deferred output against old forward output before deleting old code.
- Inspect GBuffer and SceneColor in PIX/RenderDoc.

### Phase 4: RT Scene For Shadow Rays

**Objective**

Add the runtime ray tracing scene needed by inline ray-query shadows: DXR capability checks, BLAS/TLAS resource support, build commands, geometry metadata, dirty tracking, and diagnostics.

**Prompt**

```text
Implement Phase 4 of docs/architecture/deferred-gbuffer-rt-shadows-system-design.md.

Goal: build the D3D12/RHI and renderer runtime support for a TLAS usable by inline ray-query deferred shadows.

Scope:
- Add DXR capability reporting and require support for this branch/path.
- Add RHI resource/state support for acceleration-structure buffers, scratch buffers, instance buffers, and ResourceState::RayTracingAccelerationStructure.
- Add command-list support for BLAS/TLAS build or refit commands and required UAV barriers.
- Add renderer-side acceleration-structure parameter support: PassParameterValueKind, ShaderParameterFields type, PassParameterSet setter, and PassBinder descriptor-table binding.
- Add RayTracingScene or RayTracingSceneManager to own BLAS records, TLAS resources, scratch/result buffers, instance records, dirty tracking, diagnostics, and level lifecycle release.
- Extend GPUMesh/GPUMeshCache to expose native vertex/index resources and geometry metadata for BLAS builds.
- Support moving-instance TLAS rebuilds from RenderSceneData::meshDraws.
- Support CPU-dirty mesh reupload and BLAS rebuild; skeletal/morph import remains out of scope.

Constraints:
- Do not implement DispatchRays, shader tables, or RT state objects in this phase.
- Keep D3D12-specific details backend-private.
- Route all new resources through RHI/renderer-owned handles.
- Keep D3D12MA out of scope.

Relevant files:
- Engine/RHI/Public/Interop/RenderHardwareInterface.h
- Engine/RHI/Public/Interop/ResourceState.h
- Engine/RHI/Private/D3D12/D3D12Rhi.cpp
- Engine/RHI/Private/D3D12/D3D12RenderCommandList.h/.cpp
- Engine/RHI/Private/D3D12/D3D12TypeConversions.cpp
- Engine/Renderer/Public/ShaderParameters/PassParameterSet.h
- Engine/Renderer/Public/ShaderParameters/ShaderParameterFields.h
- Engine/Renderer/Private/Pipeline/PassBinder.cpp
- Engine/Renderer/Private/GPU/GPUMesh.h/.cpp
- Engine/Renderer/Private/GPU/GPUMeshCache.h/.cpp
- Engine/Renderer/Private/SceneData/Lifecycle/SceneRenderStateCoordinator.cpp

Acceptance checks:
- Runtime fails clearly when required DXR/inline ray query support is missing.
- BLAS/TLAS builds complete without GPU validation errors.
- Moving transforms update TLAS.
- CPU-dirty mesh data triggers GPU buffer update and BLAS rebuild.
- Level unload releases RT resources cleanly.
```

**Verification**

- Build `ShowcaseRuntime` and validation targets.
- Run on DXR-capable hardware with GPU validation enabled.
- Verify BLAS/TLAS build diagnostics: counts, sizes, build timings, and rebuild reasons.
- Check live object reports on shutdown/level unload.

### Phase 5: Deferred Lighting With Inline RT Shadows

**Objective**

Connect the RT scene to deferred lighting and use inline ray queries for directional-light shadow visibility.

**Prompt**

```text
Implement Phase 5 of docs/architecture/deferred-gbuffer-rt-shadows-system-design.md.

Goal: add inline ray-query shadows to DeferredLightingPass.

Scope:
- Extend DeferredLightingPass parameters with the TLAS acceleration structure and RT shadow constants.
- Bind the TLAS through the acceleration-structure pass parameter path added in Phase 4.
- Update deferred lighting HLSL to cast shadow rays from reconstructed world-space surface positions toward shadow-casting directional lights.
- Apply configurable bias/max distance to avoid self-intersection.
- Multiply direct diffuse/specular lighting by ray-traced visibility.
- Start with opaque triangle shadows only.
- Remove old shadow-map reads from deferred shader code if any were introduced during bring-up.

Constraints:
- Do not add DispatchRays.
- Do not add tonemapping.
- Do not solve alpha-tested shadowing unless it naturally fits without expanding scope.
- Keep the raster shadow path unused by the deferred renderer.

Relevant files:
- Engine/Renderer/Private/Passes/DeferredLightingPass.h/.cpp
- Engine/Assets/Shaders/Deferred/*
- Engine/Renderer/Public/ShaderParameters/PassParameterSet.h
- Engine/Renderer/Public/ShaderParameters/ShaderParameterFields.h
- Engine/Renderer/Private/Pipeline/PassBinder.cpp
- Engine/Renderer/Private/SceneData or GPU RayTracingScene files from Phase 4
- Engine/RHI/Public/Resources/RenderConstantBufferData.h if RT shadow constants need a shared struct

Acceptance checks:
- Deferred lighting changes when scene geometry occludes directional lights.
- Moving objects update shadow visibility through TLAS rebuilds.
- CPU-dirty mesh BLAS rebuilds affect shadow visibility.
- No severe self-shadowing or shadow acne with the initial bias defaults.
```

**Verification**

- Build/cook deferred shader package.
- Run Showcase on DXR-capable hardware.
- Validate GPU debug output for ray query/AS descriptor errors.
- Test camera movement, light movement, moving instances, and dirty mesh rebuilds.

### Phase 6: Remove Forward And Raster Shadow Paths

**Objective**

Make deferred rendering the normal opaque path and remove obsolete forward opaque and cascaded raster shadow code.

**Prompt**

```text
Implement Phase 6 of docs/architecture/deferred-gbuffer-rt-shadows-system-design.md.

Goal: remove the old forward opaque and raster shadow paths after GBuffer + deferred RT shadows are validated.

Scope:
- Remove ForwardOpaquePass as the normal opaque renderer.
- Remove raster shadow pass wiring: ShadowPasses, ShadowOpaquePass, shadow-map framegraph products, shadow-map shader registrations, ShadowFrameBuilder, and ShadowBuilder if they only serve cascaded raster shadows.
- Update FrameGraphBuilder so the normal sequence is GBuffer -> DeferredLighting -> CopySceneColorToBackBuffer -> present.
- Update RenderPassRuntimeRegistry and RenderPassPipelineTraits to include only the active pass set.
- Update shader package registrations and cook scripts/tasks to cook GBuffer and DeferredLighting packages and stop cooking removed forward/shadow packages.
- Clean up stale per-view shadow-map/cascade data after all users are gone.
- Keep general per-light CastShadow metadata because deferred RT shadows still use it.

Constraints:
- Do not remove code that is still needed by editor presentation or non-shadow lighting.
- If transparent/special-material forward rendering is needed later, leave it as a future explicit pass rather than keeping ForwardOpaque as a hidden fallback.
- Do not add tonemapping in this cleanup phase.

Relevant files:
- Engine/Renderer/Private/Passes/ForwardOpaquePass.h/.cpp
- Engine/Renderer/Private/Passes/ShadowOpaquePass.h/.cpp
- Engine/Renderer/Private/Frame/Shadow/*
- Engine/Renderer/Private/FrameGraph/Features/ShadowPasses.h/.cpp
- Engine/Renderer/Private/FrameGraph/Features/ForwardPasses.h/.cpp
- Engine/Renderer/Private/FrameGraph/Builder/FrameGraphBuilder.cpp
- Engine/Renderer/Private/FrameGraph/RenderPassRuntime.h
- Engine/Renderer/Private/Pipeline/RenderPassPipelineTraits.h
- Engine/RHI/Private/Shaders/* shadow/forward shader registrations
- Scripts/CookAssets.bat or related cook scripts/tasks

Acceptance checks:
- Normal rendering no longer references ForwardOpaquePass or raster shadow-map framegraph products.
- GBuffer/deferred packages are cooked and loaded.
- Validation checks pass after deleted pass/shader cleanup.
- Visual output still renders opaque scene with RT shadows.
```

**Verification**

- Run a text search for removed pass names and obsolete shadow-map package IDs.
- Build `ShaderCompiler`, cook assets, build `ShowcaseRuntime`, and build `sparkle_validation_check`.
- Run Showcase and check for missing cooked shader package errors.

### Phase 7: Tonemapping And Future Work

**Objective**

Track deferred work after the core GBuffer + RT shadow pipeline is stable.

**Prompt**

```text
Plan but do not implement Phase 7 from docs/architecture/deferred-gbuffer-rt-shadows-system-design.md unless explicitly requested.

Future work:
- Add proper HDR tonemapping/exposure: HDR SceneColor -> tonemap/conversion -> swapchain BackBuffer.
- Add full DispatchRays direct lighting, RT pipeline state objects, and shader tables if primary-ray rendering becomes a goal again.
- Add alpha-tested RT shadows after material lookup policy is stable.
- Add transparent/special-material forward pass if needed.
- Evaluate D3D12MA once RT scene allocation pressure is measurable.
- Start Vulkan only after renderer-facing deferred and RT scene contracts stabilize.

Constraints:
- Keep these as separate focused plans. Do not fold them into the initial deferred RT shadow milestone.
```

## Verification Matrix

| Area | Check |
| --- | --- |
| Formats | New formats map to valid DXGI resource and view formats. |
| GBuffer | Base color, normal, material params, emissive, and depth look correct in PIX/RenderDoc. |
| Deferred lighting | Output roughly matches old forward lighting before shadows. |
| Presentation | SceneColor reaches the back buffer through the regular copy path. |
| DXR capability | Startup fails clearly if required DXR/inline ray query support is missing. |
| RT scene | BLAS/TLAS builds without GPU validation errors. |
| Dynamic updates | Moving transforms update TLAS; CPU-dirty meshes rebuild BLAS. |
| RT shadows | Directional light occlusion responds to scene geometry and motion. |
| Cleanup | Forward/raster shadow pass names and package IDs are gone from normal runtime paths. |

## Build And Validation Commands

Use the existing serial MSBuild pattern for reliability on this workspace:

```text
msbuild build\Tools\ShaderCompiler\ShaderCompiler.vcxproj /p:Configuration=Debug /p:Platform=x64 /m:1 /p:UseMultiToolTask=false /p:TrackFileAccess=false /nodeReuse:false /nologo /clp:ErrorsOnly;Summary
```

Cook the new shader packages from `Projects\Showcase` using `ShaderCompiler.exe cook --shader <PackageName> --backend dxc --target DxilSm66 --no-cache`.

```text
msbuild build\Projects\Showcase\ShowcaseRuntime.vcxproj /p:Configuration=Debug /p:Platform=x64 /m:1 /p:UseMultiToolTask=false /p:TrackFileAccess=false /nodeReuse:false /nologo /clp:ErrorsOnly
```

```text
msbuild build\sparkle_validation_check.vcxproj /p:Configuration=Debug /p:Platform=x64 /m:1 /p:UseMultiToolTask=false /p:TrackFileAccess=false /nodeReuse:false /nologo /clp:ErrorsOnly
```

## Key Risks

- Depth SRV support may require typeless depth format handling in RHI/D3D12 conversion code.
- HDR SceneColor cannot use the raw copy path; reintroduce it only with a real tonemap/conversion pass.
- Position reconstruction bugs can look like lighting or RT shadow bugs; validate without shadows first.
- Inline ray-query shadows require a correct TLAS and acceleration-structure descriptor binding path.
- Removing forward/shadow paths too early can hide whether failures are deferred-specific or cleanup-specific. Complete validation before deletion.

## Final Recommendation

Implement the phases in order. The most important sequencing rule is: validate GBuffer and deferred lighting without RT shadows before adding the ray tracing scene. Once deferred lighting is trustworthy, add BLAS/TLAS and inline ray-query shadow visibility. Remove the old forward opaque and raster shadow systems only after the deferred RT shadow path is proven.

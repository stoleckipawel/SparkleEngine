# 05. RENDERING - Single Frame Modes Plan

Status: proposal for review
Date: 2026-07-09
Use this as: staged implementation plan for replacing the top-level realtime/reference frame fork with selectable GBuffer and lighting modes.

## Goal

Sparkle should have one frame pipeline. Users should be able to choose how key render products are produced:

- GBuffer: rasterized or raytraced
- Lighting: raytraced realtime lighting or path traced reference lighting
- Post-processing/output: owns exposure, debug visualization, and presentation from available frame products

There should be no special reference final-color pass family. A reference-style path traced view is achieved by selecting:

- `GBufferMode::Raytraced`
- `LightingMode::PathTraced`

The desired end shape is that `BuildFrame` always schedules the same conceptual frame skeleton:

```cpp
CreateFrameSceneResources(...);
AddRaytracingScenePasses(...);
AddGBufferPasses(...);
AddLightingPasses(...);
AddPostProcessingPasses(...);
```

The mode decisions move inside `AddGBufferPasses` and `AddLightingPasses`.

Deletion policy: when a stage replaces a frame-path concept, delete the replaced code/config in that same stage. Do not keep retired CVar/config names after Stage 8.

Rendering settings CVar contract:

- `EngineRenderingSettingsSection` is a config/editor facade over CVars, not a runtime settings owner.
- Every field in `EngineRenderingSettingsState` must capture from a `ConsoleVariable<T>::Get()` path.
- Persisted config must store CVar names and apply values through the CVar registry/parser.
- Individual editor/settings setters must write the owning CVar directly, then refresh and persist the section snapshot from CVars.
- Runtime render code should read CVars directly at use sites. Feature-local builders may group packing/clamping logic, but only for payloads the pass actually consumes.
- Do not add `Build*FromCVars` helpers.
- Do not use `Build*Settings().Enabled` for scheduling. Enabled gates are CVars and should be read directly at the branch that needs them.
- Feature settings structs should not carry broad scheduling flags such as `Enabled`; keep them to pass payload values such as bias, distance, sample counts, bounce counts, and debug modes.
- Do not route one-setting edits through a copied full settings state, mirrored globals, setter wrapper functions, or frame result fields.
- New renderer settings must add the CVar first, then add config/editor persistence on top of that CVar.
- Editor settings UI should keep feature-specific enum option tables in feature section files. The main rendering settings panel should orchestrate sections rather than own enum/index conversion details.

## Current Sparkle State

The top-level fork is still in `Engine/Renderer/Private/Frame/Core/Frame.cpp`. `ResolveFrameRenderPathFromSettings()` turns `r.RayTracing.ReferencePathTracing.Enabled` into `FrameRenderPath::PathTracedReference`, then `BuildFrame` either runs `AddReferenceRenderingPasses` or the realtime GBuffer plus lighting path.

Related dependency points:

- `Engine/Renderer/Private/Frame/Core/FrameRenderPath.*` owns the two-value frame path enum.
- `Engine/Renderer/Private/Frame/Core/FrameResolution.cpp` is already independent from `FrameRenderPath`; it resolves render extent from the upscaler quality CVar.
- `FrameBuildResult` is already resource-focused and no longer carries render path or mode state.
- `Engine/Renderer/Private/FramePipeline/FramePipeline.cpp` still rebuilds the graph and resets history when `FrameRenderPath` changes, but captures the current value directly from `ResolveFrameRenderPathFromSettings()`.
- `FramePipeline::RecordFrame` already derives ray tracing scene requirements from direct CVar reads for the temporary reference path and active RT effects.
- `Engine/Renderer/Private/Frame/RayTracing/RayTracingScene.*` owns the frame graph TLAS resource and scene build pass through `AddRaytracingScenePasses`.
- `Engine/Renderer/Private/Frame/Deferred/GBuffer.cpp` already centralizes GBuffer target creation and writes common viewport products.
- `Engine/Renderer/Private/Frame/Lighting/Lighting.cpp` already centralizes lighting composition and calls direct, indirect, sky, composite, and ray reconstruction passes.
- `Engine/Renderer/Private/Frame/Lighting/IndirectLighting.cpp` schedules indirect passes from direct enabled-CVar checks, not from bulk settings builders.
- `Engine/Renderer/Private/Frame/Reference/ReferencePathTracing.cpp` currently writes `ReferenceSceneColor` and copies it over scene color/final scene color. This path is deleted once raytraced GBuffer plus path traced lighting replaces its role.
- Current indirect diffuse/specular passes already path trace secondary rays from a GBuffer primary surface and output lighting/guide products.
- Feature-local settings builders such as `BuildIndirectDiffuseSettings`, `BuildIndirectSpecularSettings`, and `BuildReferencePathTracingSettings` are pass payload builders only. They must not own enabled/scheduling state.
- `RenderRayTracingPassServices` carries the ray tracing scene/capability data and ray-traced shadow payload. It does not carry indirect diffuse/specular settings payloads.

That means the refactor should not start by inventing a second frame graph. The frame graph is already the right scheduler; the ambiguity is the frame-wide render path switch.

## NVIDIA Reference Notes

Use these references for direction, not for direct architecture cloning.

- Donut keeps reusable render passes such as GBuffer fill as composable pieces in `donut_render`, with a `GBufferRenderTargets` product object containing depth, material buffers, and motion vectors. Reference: https://github.com/NVIDIA-RTX/Donut/blob/main/include/donut/render/GBuffer.h and https://github.com/NVIDIA-RTX/Donut/blob/main/src/render/GBuffer.cpp
- Donut's README describes `donut_render` as reusable passes, while ray tracing acceleration structure maintenance is application-specific. This supports keeping Sparkle's modes near the renderer frame graph and RT scene owner. Reference: https://github.com/NVIDIA-RTX/Donut
- RTXDI describes ReSTIR DI, ReGIR, ReSTIR GI, and ReSTIR PT as separable algorithms with shared state, and says integration is renderer-specific because material model, scene data, ray tracing, GBuffer access, and graphics API access are engine-owned. Reference: https://github.com/NVIDIA-RTX/RTXDI/blob/main/Doc/Integration.md
- RTXDI's ReSTIR PT documentation separates primary-surface data, motion vectors, secondary path resampling, and denoiser guide buffers. That maps directly to Sparkle's need for a stable GBuffer contract plus selectable lighting modes. Reference: https://github.com/NVIDIA-RTX/RTXDI/blob/main/Doc/RestirPT.md
- NRD states that denoising relies on per-pixel GBuffer guides such as normal, roughness, viewZ, and motion vector. Sparkle should therefore keep GBuffer products first-class even when lighting becomes path traced. Reference: https://github.com/NVIDIA-RTX/NRD/blob/master/README.md
- RTXPT is a pure path tracer with reference/realtime modes, guide-buffer generation, RTXDI, NRD, and DLSS-RR integration. Borrow its product discipline and mode vocabulary, but do not copy its monolithic sample shape into Sparkle. Reference: https://github.com/NVIDIA-RTX/RTXPT/blob/main/README.md
- Falcor's path tracer pass has optional inputs and outputs and is composed in render graphs. That is a good model for Sparkle render products: a pass writes a product contract, later passes consume the connected products. Reference: https://github.com/NVIDIAGameWorks/Falcor/blob/master/docs/usage/path-tracer.md and https://github.com/NVIDIAGameWorks/Falcor/blob/master/docs/getting-started.md

## Target Mode Vocabulary

Use exactly two mode enums:

```cpp
enum class GBufferMode : std::uint8_t
{
	Rasterized,
	Raytraced
};

enum class LightingMode : std::uint8_t
{
	Raytraced,
	PathTraced
};
```

Mode meanings:

- `GBufferMode::Rasterized`: current raster mesh pass writes the active GBuffer.
- `GBufferMode::Raytraced`: a primary-ray pass writes the same active GBuffer target contract.
- `LightingMode::Raytraced`: current realtime ray-traced lighting path.
- `LightingMode::PathTraced`: regular path tracing logic for secondary rays and reference-quality lighting from the active GBuffer.

## Product Contracts

The active GBuffer producer must write these products:

- `GBufferRenderTargets::BaseColor`
- `GBufferRenderTargets::Normal`
- `GBufferRenderTargets::Material`
- `GBufferRenderTargets::Emissive`
- `GBufferRenderTargets::Subsurface`
- `GBufferRenderTargets::DeviceZ`
- `GBufferRenderTargets::MotionVector`
- `GBufferRenderTargets::MainDepth`

The active lighting producer must write these products:

- `LightingRenderTargets::DirectDiffuse`
- `LightingRenderTargets::DirectSpecular`
- `LightingRenderTargets::DirectSubsurface`
- `LightingRenderTargets::IndirectDiffuse`
- `LightingRenderTargets::IndirectSpecular`
- `LightingRenderTargets::IndirectDiffuseDemodulatedRadiance`
- `LightingRenderTargets::IndirectSpecularDemodulatedRadiance`
- `LightingRenderTargets::IndirectDiffuseAlbedo`
- `LightingRenderTargets::IndirectSpecularAlbedo`
- `LightingRenderTargets::IndirectDiffuseSampleGuide`
- `LightingRenderTargets::IndirectSpecularSampleGuide`

The first implementation should match the current final output for the default settings:

- default GBuffer mode: `Rasterized`
- default lighting mode: `Raytraced`

## Stage 0 - Rename Ray Tracing Scene Frame Helper

Purpose: make frame code describe the real resource owner, not a vague infrastructure bucket.

Target files:

- `Engine/Renderer/Private/Frame/Core/Frame.cpp`
- `Engine/Renderer/Private/Frame/RayTracing/RayTracingScene.*`

Implementation notes:

- Rename the vague ray tracing infrastructure helper to `AddRaytracingScenePasses`.
- Keep `CreateRayTracingSceneFrameGraphResource` and `AddRayTracingSceneBuildPasses` as the lower-level helper names.

Acceptance:

- `Frame.cpp` calls `AddRaytracingScenePasses`.
- `rg "AddRayTracingInfrastructurePasses" Engine` finds no remaining references.

Ready-to-use prompt:

```text
Implement Stage 0 from Docs/Architecture/01-Implementation/05_RENDERING_SingleFrameModesPlan.md.

Goal: rename the ray tracing frame helper to AddRaytracingScenePasses.

Inspect:
- Engine/Renderer/Private/Frame/Core/Frame.cpp
- Engine/Renderer/Private/Frame/RayTracing/RayTracingScene.*

Requirements:
- Rename declaration, definition, and call site.
- Keep lower-level SceneTlas and build-pass helper names unchanged.

Verification:
- rg "AddRayTracingInfrastructurePasses" Engine
- Build Renderer.
```

## Stage 1 - Add Mode Settings As Source Of Truth

Purpose: introduce `GBufferMode` and `LightingMode` while keeping the default output stable.

Target files:

- `Engine/Renderer/Private/Frame/Core/FrameRenderPath.*`
- `Engine/Renderer/Private/Frame/Core/Frame.h`
- `Engine/Renderer/Private/FramePipeline/FramePipeline.*`
- `Engine/Renderer/Public/Debug/RendererCVars.h`
- `Engine/Renderer/Private/Debug/RendererCVars.cpp`
- `Engine/Renderer/Public/Settings/EngineRenderingSettings.h`
- `Engine/Renderer/Public/Settings/EngineRenderingRayTracingTypes.h`
- `Engine/Renderer/Private/Settings/EngineRenderingSettings.cpp`
- `Engine/Editor/Private/Panels/RenderingSettingsPanel.cpp`
- `Config/DefaultEngine.ini`

Implementation notes:

- Add `GBufferMode` and `LightingMode` to renderer settings/types.
- Add `CVarGBufferMode` and `CVarLightingMode`; CVars are the runtime source of truth.
- Rendering settings captures snapshots through `CVar*.Get()` and applies persisted config through CVar names.
- Any mode setter writes the mode CVar directly, then refreshes/persists the settings snapshot from CVars.
- Render code should read the mode CVars directly at the branch points that need them.
- Keep enabled feature gates as direct `CVar*.Get()` checks at scheduling and pass-entry points. Do not hide them inside settings payload builders.
- Leave `FrameRenderPath` removal to Stage 7, but stop adding new logic to it.
- Do not add any special reference final-color mode.
- Do not create a new route from `r.RayTracing.ReferencePathTracing.Enabled` into the new modes.
- Do not add mode fields to `FrameBuildResult` or `FrameGraphBuildResult`.
- Add editor/config fields only for modes that have implemented behavior. It is acceptable in Stage 1 to expose no UI and keep the resolver private.

Acceptance:

- Default rendering is unchanged.
- `GBufferMode` and `LightingMode` exist as the only new core render-mode enums.
- `CVarGBufferMode` and `CVarLightingMode` drive the runtime values.
- No duplicate renderer settings owner is introduced.

Ready-to-use prompt:

```text
Implement Stage 1 from Docs/Architecture/01-Implementation/05_RENDERING_SingleFrameModesPlan.md.

Goal: add GBufferMode and LightingMode as the renderer's mode vocabulary while keeping default output stable.

Inspect:
- Engine/Renderer/Private/Frame/Core/FrameRenderPath.*
- Engine/Renderer/Private/Frame/Core/Frame.h
- Engine/Renderer/Private/FramePipeline/FramePipeline.*
- Engine/Renderer/Public/Debug/RendererCVars.h
- Engine/Renderer/Private/Debug/RendererCVars.cpp
- Engine/Renderer/Public/Settings/EngineRenderingSettings.h
- Engine/Renderer/Public/Settings/EngineRenderingRayTracingTypes.h
- Engine/Renderer/Private/Settings/EngineRenderingSettings.cpp
- Engine/Editor/Private/Panels/RenderingSettingsPanel.cpp
- Config/DefaultEngine.ini

Requirements:
- Add only GBufferMode { Rasterized, Raytraced }.
- Add only LightingMode { Raytraced, PathTraced }.
- Add CVarGBufferMode and CVarLightingMode.
- Make rendering settings capture/apply persisted config through CVar Get/Set.
- Make any direct settings setter write the owning CVar first, not a copied full settings state.
- Do not add `Build*FromCVars` helpers or settings structs whose only purpose is moving copied CVar state.
- Do not put enabled flags into pass settings payloads when the enabled state can be checked directly from the owning CVar.
- Keep current default output stable.
- Do not connect r.RayTracing.ReferencePathTracing.Enabled to the new modes.
- Do not add a special reference final-color mode.
- Leave FrameRenderPath removal to Stage 7.
- Do not add GBufferMode or LightingMode fields to FrameBuildResult or FrameGraphBuildResult.
- Do not add UI for unimplemented behavior.

Verification:
- Build Renderer.
- Confirm only GBufferMode and LightingMode were introduced.
- Confirm mode runtime state is CVar-backed, not mirrored in custom globals.
- Confirm Frame.cpp default output is unchanged.
```

## Stage 2 - Split Rasterized GBuffer Into Dedicated Files

Purpose: make `AddGBufferPasses` an orchestrator and move the current rasterized path into dedicated files.

Target files:

- `Engine/Renderer/Private/Frame/Deferred/GBuffer.*`
- `Engine/Renderer/Private/Passes/Deferred/GBufferPass.*`
- `Engine/Renderer/ShaderRegistrations/GBufferShaders.cpp`
- `Engine/Assets/Shaders/Passes/Deferred/GBufferVS.hlsl`
- `Engine/Assets/Shaders/Passes/Deferred/GBufferPS.hlsl`

Implementation notes:

- Keep `CreateGBufferRenderTargets` in the common GBuffer orchestrator.
- Move `AddGBufferPass` to a new rasterized producer:
  - `Engine/Renderer/Private/Frame/GBuffer/RasterizedGBuffer.h`
  - `Engine/Renderer/Private/Frame/GBuffer/RasterizedGBuffer.cpp`
- Consider renaming the pass class to `RasterizedGBufferPass` only if the rename is mechanical and low risk. Otherwise leave the pass class as `GBufferPass` for this stage and only change frame-level file ownership.
- `AddGBufferPasses` should branch on `GBufferMode`. Only `Rasterized` is implemented in this stage.

Acceptance:

- Current rasterized GBuffer behavior is unchanged.
- `AddGBufferPasses` is now the GBuffer mode switch point.
- No shader package behavior changes.
- No duplicate target creation appears.

Ready-to-use prompt:

```text
Implement Stage 2 from Docs/Architecture/01-Implementation/05_RENDERING_SingleFrameModesPlan.md.

Goal: turn AddGBufferPasses into an orchestrator and move the current rasterized producer into dedicated RasterizedGBuffer files.

Inspect:
- Engine/Renderer/Private/Frame/Deferred/GBuffer.*
- Engine/Renderer/Private/Passes/Deferred/GBufferPass.*
- Engine/Renderer/ShaderRegistrations/GBufferShaders.cpp
- Engine/Assets/Shaders/Passes/Deferred/GBufferVS.hlsl
- Engine/Assets/Shaders/Passes/Deferred/GBufferPS.hlsl

Requirements:
- Keep CreateGBufferRenderTargets common.
- Add RasterizedGBuffer.h/.cpp and move the current AddGBufferPass scheduling there.
- Make AddGBufferPasses branch on GBufferMode.
- Only GBufferMode::Rasterized is active/implemented in this stage.
- Do not change render target formats.

Verification:
- Build Renderer.
- rg "AddGBufferPass" Engine/Renderer/Private/Frame
- Confirm viewport products SceneDepth, Normals, MotionVectors are still set from the active GBuffer.
```

## Stage 3 - Add Raytraced GBuffer Producer

Purpose: allow primary rays to fill the same GBuffer contract that rasterization fills.

Target files:

- `Engine/Renderer/Private/Frame/GBuffer/RaytracedGBuffer.*`
- `Engine/Renderer/Private/Passes/RayTracing/RaytracedGBufferPass.*`
- `Engine/Renderer/ShaderRegistrations/RaytracedGBufferShaders.cpp`
- `Engine/Assets/Shaders/Passes/RayTracing/RaytracedGBuffer.hlsl`
- `Engine/Assets/Shaders/RayTracing/RayTracingHitSurface.hlsli`
- `Engine/Assets/Shaders/RayTracing/RayTracingMaterialHit.hlsli`
- `Engine/Renderer/Private/RayTracing/Scene/RenderRayTracingPassServices.h`
- `Engine/Renderer/Private/FramePipeline/FramePipeline.cpp`

Implementation notes:

- The raytraced GBuffer pass should dispatch one thread per pixel and trace a primary ray against `SceneTlas`.
- It must write the common GBuffer textures. If motion vectors are not available for raytraced primary surfaces in the first implementation, write zero motion with an explicit TODO in the stage notes or implement camera-only reprojection if current temporal data supports it.
- Use existing hit/material shader helpers instead of duplicating material decode logic.
- If no hit is found, write sky/empty GBuffer values recognized by `IsSkyPixel`.
- `FramePipeline::RecordFrame` must require the ray tracing scene when `GBufferMode::Raytraced` is selected.
- The pass should have descriptor and device-address variants only if current pass infrastructure requires parity. Start with the existing descriptor TLAS path if that is the least risky.

Acceptance:

- `GBufferMode::Rasterized` remains default and unchanged.
- `GBufferMode::Raytraced` produces valid debug views for depth, normal, base color, roughness/metallic, emissive, and sky/miss pixels.
- Lighting passes consume the raytraced GBuffer without knowing which producer wrote it.
- Ray tracing scene is built only when selected modes/effects require it.

Ready-to-use prompt:

```text
Implement Stage 3 from Docs/Architecture/01-Implementation/05_RENDERING_SingleFrameModesPlan.md.

Goal: add a raytraced primary-surface producer that writes the existing GBufferRenderTargets contract.

Inspect:
- Engine/Renderer/Private/Frame/GBuffer/RasterizedGBuffer.*
- Engine/Renderer/Private/Frame/Deferred/GBuffer.*
- Engine/Renderer/Private/Passes/Deferred/GBufferPass.*
- Engine/Renderer/Private/Passes/Reference/ReferencePathTracingPass.*
- Engine/Assets/Shaders/Passes/Reference/ReferencePathTracing.hlsl
- Engine/Assets/Shaders/RayTracing/RayTracingHitSurface.hlsli
- Engine/Assets/Shaders/RayTracing/RayTracingMaterialHit.hlsli
- Engine/Renderer/Private/FramePipeline/FramePipeline.cpp

Requirements:
- Add RaytracedGBuffer frame helper, pass class, shader registration, and HLSL pass.
- Reuse existing ray tracing material/hit helpers.
- Write all GBuffer textures with valid sky/miss behavior.
- Update rayTracingSceneRequired to include GBufferMode::Raytraced.
- Keep rasterized mode default.
- Do not change lighting pass inputs.

Verification:
- Build Renderer and shader registrations.
- Cook/compile shaders if the local workflow supports targeted shader cook.
- Run visual debug modes for GBuffer outputs in rasterized and raytraced modes.
```

## Stage 4 - Make Lighting Mode Driven Internally

Purpose: make `AddLightingPasses` own lighting mode selection.

Target files:

- `Engine/Renderer/Private/Frame/Lighting/Lighting.*`
- `Engine/Renderer/Private/Frame/Lighting/IndirectLighting.*`
- `Engine/Renderer/Private/Frame/Lighting/IndirectDiffuse.*`
- `Engine/Renderer/Private/Frame/Lighting/IndirectSpecular.*`
- `Engine/Renderer/Private/RayTracing/Effects/IndirectDiffuse/*`
- `Engine/Renderer/Private/RayTracing/Effects/IndirectSpecular/*`
- `Engine/Assets/Shaders/Passes/Deferred/IndirectDiffuse.hlsl`
- `Engine/Assets/Shaders/Passes/Deferred/IndirectSpecular.hlsl`

Implementation notes:

- Treat existing direct lighting plus indirect diffuse/specular passes as the first `LightingMode::Raytraced` implementation.
- Move orchestration into clearer files such as `RaytracedLighting.*` if useful.
- `AddLightingPasses` or `AddIndirectLightingPasses` should branch on `LightingMode`:
  - `Raytraced`: current enabled-CVar behavior, with enabled checks read directly from the owning CVars.
  - `PathTraced`: no-op until Stage 5.
- Keep direct lighting behavior unchanged in this stage.
- Keep feature settings builders payload-only. For example, indirect diffuse/specular builders may prepare bias, distance, bounce count, intensity, sample mode, and debug mode, but must not own `Enabled`.

Acceptance:

- Existing direct and indirect raytraced lighting behavior is unchanged in default mode.
- The lighting mode switch lives under lighting, not frame core.
- Ray reconstruction still only runs when its required inputs exist.
- No `Build*Settings().Enabled` checks are introduced.

Ready-to-use prompt:

```text
Implement Stage 4 from Docs/Architecture/01-Implementation/05_RENDERING_SingleFrameModesPlan.md.

Goal: make AddLightingPasses select lighting by LightingMode.

Inspect:
- Engine/Renderer/Private/Frame/Lighting/Lighting.*
- Engine/Renderer/Private/Frame/Lighting/IndirectLighting.*
- Engine/Renderer/Private/Frame/Lighting/IndirectDiffuse.*
- Engine/Renderer/Private/Frame/Lighting/IndirectSpecular.*
- Engine/Renderer/Private/RayTracing/Effects/IndirectDiffuse/*
- Engine/Renderer/Private/RayTracing/Effects/IndirectSpecular/*
- Engine/Renderer/Private/Frame/PostProcessing/PostProcessing.cpp

Requirements:
- Add LightingMode branching inside lighting orchestration.
- Treat existing direct plus indirect raytraced passes as LightingMode::Raytraced.
- Leave LightingMode::PathTraced as explicit unimplemented/no-op until Stage 5.
- Read enabled CVars directly for pass scheduling and pass-entry gates.
- Keep indirect settings structs free of `Enabled` fields.
- Do not rename unrelated raytraced-lighting toggles in this stage.

Verification:
- Build Renderer.
- Test/default inspect: LightingMode::Raytraced still schedules current lighting passes.
- rg "Build.*Settings\\(\\)\\.Enabled|settings\\.Enabled" Engine/Renderer
```

## Stage 5 - Implement Path Traced Lighting

Purpose: use regular path tracing logic for lighting while keeping GBuffer production separate.

Target files:

- `Engine/Renderer/Private/Frame/Lighting/PathTracedLighting.*`
- `Engine/Renderer/Private/Passes/RayTracing/PathTracedLightingPass.*`
- `Engine/Renderer/ShaderRegistrations/PathTracedLightingShaders.cpp`
- `Engine/Assets/Shaders/Passes/RayTracing/PathTracedLighting.hlsl`
- `Engine/Assets/Shaders/RayTracing/PathTrace.hlsli`
- `Engine/Assets/Shaders/RayTracing/PathLighting.hlsli`
- `Engine/Assets/Shaders/RayTracing/PathSurface.hlsli`
- `Engine/Assets/Shaders/Passes/Reference/ReferencePathTracing.hlsl`

Implementation notes:

- Do not start from camera rays in this pass. Start from the active GBuffer primary surface.
- Reconstruct primary position and material from GBuffer. Then trace secondary bounces using the shared path tracing helpers.
- Output lighting products and guide products needed by ray reconstruction/denoising.
- Use `PathTracedLighting` naming for any settings introduced here. Existing reference path tracing code is source material, not the name of the new system.
- This stage may initially be expensive. Correctness and separation matter more than 60 fps.

Acceptance:

- `LightingMode::PathTraced` produces lighting through the normal lighting composite path.
- Rasterized GBuffer plus path traced lighting works.
- Raytraced GBuffer plus path traced lighting works.
- Post-processing/output remains shared.
- No special reference final-color pass is introduced.

Ready-to-use prompt:

```text
Implement Stage 5 from Docs/Architecture/01-Implementation/05_RENDERING_SingleFrameModesPlan.md.

Goal: add LightingMode::PathTraced as a lighting producer that starts from the active GBuffer surface.

Inspect:
- Engine/Renderer/Private/Frame/Lighting/Lighting.*
- Engine/Renderer/Private/Frame/Reference/ReferencePathTracing.*
- Engine/Renderer/Private/Passes/Reference/ReferencePathTracingPass.*
- Engine/Assets/Shaders/Passes/Reference/ReferencePathTracing.hlsl
- Engine/Assets/Shaders/Passes/Deferred/GBufferUtils.hlsli
- Engine/Assets/Shaders/RayTracing/PathTrace.hlsli
- Engine/Assets/Shaders/RayTracing/PathLighting.hlsli
- Engine/Assets/Shaders/RayTracing/PathSurface.hlsli

Requirements:
- Add PathTracedLighting frame helper, pass, shader registration, and HLSL.
- Reconstruct primary surfaces from GBuffer, then path trace lighting.
- Write LightingRenderTargets color and guide outputs.
- Do not replace the whole frame final color in this pass.
- Do not add a special reference final-color override.
- Reuse existing path tracing shader logic, but name any new settings after `PathTracedLighting`.

Verification:
- Build Renderer and shader registrations.
- Shader cook/compile targeted path tracing shaders.
- Compare Rasterized/Raytraced, Rasterized/PathTraced, Raytraced/Raytraced, and Raytraced/PathTraced mode combinations.
```

## Stage 6 - Collapse BuildFrame To One Skeleton

Purpose: remove the top-level `PathTracedReference` branch from `BuildFrame` after the mode combination exists.

Target files:

- `Engine/Renderer/Private/Frame/Core/Frame.cpp`
- `Engine/Renderer/Private/Frame/Core/Frame.h`
- `Engine/Renderer/Private/Frame/Core/FrameRenderPath.*`
- `Engine/Renderer/Private/Frame/Reference/ReferencePathTracing.*`
- `Engine/Renderer/Private/Frame/PostProcessing/PostProcessing.cpp`

Implementation notes:

- Change `BuildFrame` so it always calls `AddGBufferPasses` and `AddLightingPasses`.
- Do not add a special reference final-color helper.
- Remove the reference-frame CVar path. Reference-quality output is selected by:
  - `GBufferMode::Raytraced`
  - `LightingMode::PathTraced`
- Remove `AddReferenceRenderingPasses`.
- Keep `FrameBuildResult` resource-focused; do not add mode fields to it.

Acceptance:

- `Frame.cpp` has no `if (renderPath == FrameRenderPath::PathTracedReference)` branch.
- The default frame still writes GBuffer and lighting, then post-processing owns debug visualization and presentation.
- Reference-quality rendering is produced only by raytraced GBuffer plus path traced lighting.
- Post processing does not assume reference mode means missing GBuffer.

Ready-to-use prompt:

```text
Implement Stage 6 from Docs/Architecture/01-Implementation/05_RENDERING_SingleFrameModesPlan.md.

Goal: make BuildFrame a single conceptual render path without adding any special reference final-color pass.

Inspect:
- Engine/Renderer/Private/Frame/Core/Frame.cpp
- Engine/Renderer/Private/Frame/Core/Frame.h
- Engine/Renderer/Private/Frame/Reference/ReferencePathTracing.*
- Engine/Renderer/Private/Frame/PostProcessing/PostProcessing.cpp

Requirements:
- BuildFrame always schedules GBuffer and Lighting.
- Do not add any final-color reference override.
- Remove the reference-frame CVar path; use GBufferMode::Raytraced + LightingMode::PathTraced.
- Ensure FinalSceneColorProduced remains owned by normal post-processing/output paths.
- Remove AddReferenceRenderingPasses.

Verification:
- rg "AddReferenceRenderingPasses" Engine/Renderer
- rg "PathTracedReference" Engine/Renderer/Private/Frame/Core Engine/Renderer/Private/Frame/PostProcessing
- Build Renderer.
```

## Stage 7 - Replace FrameRenderPath Dependencies

Purpose: retire `FrameRenderPath` after `GBufferMode` and `LightingMode` own decisions.

Target files:

- `Engine/Renderer/Private/Frame/Core/FrameRenderPath.*`
- `Engine/Renderer/Private/Frame/Core/FrameResolution.*`
- `Engine/Renderer/Private/Frame/Core/Frame.h`
- `Engine/Renderer/Private/FramePipeline/FramePipeline.*`
- `Engine/Renderer/Private/FrameGraph/Builder/FrameGraphBuilder.h`

Implementation notes:

- Remove `FrameRenderPath` from frame build results after the single frame skeleton no longer needs it.
- Resolve `GBufferMode` and `LightingMode` from settings at the GBuffer, lighting, graph rebuild/history, and ray tracing scene decision points.
- `ResolveFrameResolutionExtents` is already independent from reference path tracing; keep it driven by upscaling/output policy, not frame path or lighting mode.
- Temporal history reset should happen where mode changes alter persistent graph resources or temporal inputs.
- `rayTracingSceneRequired` should be derived from selected modes plus active RT effects with direct CVar reads at the decision point:
  - `GBufferMode::Raytraced`
  - raytraced direct shadows
  - `LightingMode::Raytraced` when current RT lighting effects are enabled
  - `LightingMode::PathTraced`
- Delete `FrameRenderPath.*` in this stage after replacing its call sites.

Acceptance:

- `rg "FrameRenderPath|PathTracedReference|RealtimeDeferred" Engine/Renderer` finds no product code references.
- Frame graph rebuilds when selected modes alter graph resources.
- Resolution/upscaling behavior is explicit and still defaults to current output.
- Ray tracing scene is not built for pure rasterized/direct-only frames unless another selected mode or effect needs it.
- No extra frame-mode layer is introduced.

Ready-to-use prompt:

```text
Implement Stage 7 from Docs/Architecture/01-Implementation/05_RENDERING_SingleFrameModesPlan.md.

Goal: remove FrameRenderPath and make direct GBufferMode/LightingMode queries drive graph rebuild, resolution, and ray tracing scene requirements.

Inspect:
- Engine/Renderer/Private/Frame/Core/FrameRenderPath.*
- Engine/Renderer/Private/Frame/Core/FrameResolution.*
- Engine/Renderer/Private/Frame/Core/Frame.h
- Engine/Renderer/Private/FramePipeline/FramePipeline.*
- Engine/Renderer/Private/FrameGraph/Builder/FrameGraphBuilder.h

Requirements:
- Remove FrameRenderPath from FrameBuildResult after its branch is gone.
- Do not add mode fields to FrameBuildResult or FrameGraphBuildResult.
- Make resolution policy independent from reference path tracing.
- Derive rayTracingSceneRequired from selected modes and active RT effects with direct CVar reads for existing feature gates.
- Delete FrameRenderPath files after replacing their call sites.
- Keep temporal history resets on meaningful mode changes.
- Do not add extra frame-mode layers.

Verification:
- rg "FrameRenderPath|PathTracedReference|RealtimeDeferred" Engine/Renderer
- Build Renderer.
- Switch GBuffer and lighting modes and confirm graph rebuild/history reset behavior when the selected mode changes graph resources or temporal inputs.
```

## Stage 8 - Settings, UI, And CVar Cleanup

Purpose: expose implemented modes cleanly and remove confusing frame-path names.

Target files:

- `Engine/Renderer/Public/Settings/EngineRenderingSettings.h`
- `Engine/Renderer/Public/Settings/EngineRenderingRayTracingTypes.h`
- `Engine/Renderer/Private/Settings/EngineRenderingSettings.cpp`
- `Engine/Editor/Private/Panels/RenderingSettingsPanel.cpp`
- `Engine/Editor/Private/Panels/RenderingSettingsPanelUi.h`
- `Engine/Editor/Private/Panels/RenderingDisplaySettingsPanel.*`
- `Engine/Editor/Private/Panels/RenderingRayTracingSceneSettingsPanel.*`
- `Engine/Editor/Private/Panels/RenderingUpscalingSettingsPanel.cpp`
- `Engine/Editor/Private/Panels/RenderingRayReconstructionSettingsPanel.*`
- `Config/DefaultEngine.ini`
- `Engine/Renderer/Private/RayTracing/Effects/ReferencePathTracing/ReferencePathTracingCVars.*`

Implementation notes:

- Add settings UI rows only for implemented modes:
  - GBuffer: rasterized / raytraced
  - Lighting: raytraced / path traced
- Remove `ReferencePathTracing` CVar/config names from the public settings surface.
- Rename descriptions so "path traced reference" no longer implies a separate frame path.
- Store config values as readable strings, not magic integers, where the settings parser already supports it.
- Keep enum option mapping in feature-specific panel sections using typed option tables. Do not add `To*Index`/`From*Index` helper pairs to the main rendering settings panel.
- `RenderingSettingsPanel.cpp` should remain a section orchestrator.

Acceptance:

- Users can opt in/out from implemented modes through settings/config/CVars.
- No reference-frame CVar owns renderer behavior.
- Default config remains rasterized GBuffer plus raytraced lighting.
- UI does not expose unimplemented modes.
- `rg "To[A-Za-z0-9]+Index|From[A-Za-z0-9]+Index" Engine/Editor/Private/Panels` finds no enum/index conversion helpers.

Ready-to-use prompt:

```text
Implement Stage 8 from Docs/Architecture/01-Implementation/05_RENDERING_SingleFrameModesPlan.md.

Goal: expose GBufferMode and LightingMode through settings/config/UI and remove frame-path setting names.

Inspect:
- Engine/Renderer/Public/Settings/EngineRenderingSettings.h
- Engine/Renderer/Public/Settings/EngineRenderingRayTracingTypes.h
- Engine/Renderer/Private/Settings/EngineRenderingSettings.cpp
- Engine/Editor/Private/Panels/RenderingSettingsPanel.cpp
- Engine/Editor/Private/Panels/RenderingSettingsPanelUi.h
- Engine/Editor/Private/Panels/RenderingDisplaySettingsPanel.*
- Engine/Editor/Private/Panels/RenderingRayTracingSceneSettingsPanel.*
- Engine/Editor/Private/Panels/RenderingUpscalingSettingsPanel.cpp
- Engine/Editor/Private/Panels/RenderingRayReconstructionSettingsPanel.*
- Config/DefaultEngine.ini
- Engine/Renderer/Private/RayTracing/Effects/ReferencePathTracing/ReferencePathTracingCVars.*

Requirements:
- Add persisted settings for GBufferMode and LightingMode.
- Add editor controls only for implemented modes.
- Remove ReferencePathTracing CVar/config names from settings and UI.
- Use readable config strings where practical.
- Keep enum option mappings in feature-specific section files.
- Keep the main rendering settings panel free of feature enum/index conversion helpers.
- Keep defaults stable.
- Do not add extra frame-mode layers.

Verification:
- Build Renderer and Editor target if available.
- Edit config and confirm Capture/Apply path updates CVars/settings.
- Check UI labels do not mention a separate render path.
- rg "To[A-Za-z0-9]+Index|From[A-Za-z0-9]+Index" Engine/Editor/Private/Panels
```

## Stage 9 - Final Cleanup And Validation

Purpose: remove remaining frame-path leftovers and prove the single-frame model is real.

Target files:

- All files touched in Stages 1-8.
- Shader registrations for deleted/renamed passes.
- Documentation that still says "render path" when it now means "mode."

Implementation notes:

- Remove stale includes and retired render-path terminology.
- Remove the separate full-frame reference path once its role is covered by raytraced GBuffer plus path traced lighting.
- Ensure shader package names remain stable or migrate package names intentionally.
- Verify D3D12 and Vulkan expectations:
  - rasterized GBuffer works on both
  - raytraced GBuffer requires RT capability
  - path traced lighting requires RT capability
  - unsupported modes fail clearly or stay unavailable

Acceptance:

- One frame pipeline remains.
- GBuffer and lighting modes are independently selectable.
- Rasterized GBuffer plus raytraced lighting is possible.
- Rasterized GBuffer plus path traced lighting is possible.
- Raytraced GBuffer plus raytraced lighting is possible.
- Raytraced GBuffer plus path traced lighting is possible.
- No product code depends on `FrameRenderPath`.
- No product code contains a special final-color reference override.
- No product code contains an extra frame-mode layer.
- No product code contains `Build*FromCVars` helpers.
- No product code uses `Build*Settings().Enabled` for scheduling.
- Feature settings payload structs do not carry broad `Enabled` flags when a direct CVar gate exists.

Ready-to-use prompt:

```text
Implement Stage 9 from Docs/Architecture/01-Implementation/05_RENDERING_SingleFrameModesPlan.md.

Goal: remove remaining frame-path leftovers and validate the single-frame mode architecture.

Inspect:
- All files touched in Stages 1-8
- Engine/Renderer/ShaderRegistrations
- Engine/Assets/Shaders/Passes
- Docs/Architecture

Requirements:
- Remove includes, files, CVar names, and docs that imply a frame-wide render path.
- Remove the separate full-frame reference path when replaced by GBufferMode::Raytraced + LightingMode::PathTraced.
- Keep runtime branch decisions on direct CVar reads or direct mode CVar reads.
- Keep feature-local builders payload-only.
- Confirm shader package registrations match surviving pass files.
- Verify unsupported RT modes are unavailable or fail clearly.
- Do not add extra frame-mode layers.

Verification:
- rg "FrameRenderPath|PathTracedReference|RealtimeDeferred|ReferenceRenderingPasses" Engine Docs
- rg "FromCVars|Build[A-Za-z0-9]+Settings\\(\\)\\.Enabled|settings\\.Enabled" Engine/Renderer
- Build Renderer and Editor if available.
- Cook shaders.
- Test the mode matrix: Rasterized/Raytraced, Rasterized/PathTraced, Raytraced/Raytraced, Raytraced/PathTraced.
```

## Recommended Acceptance Order

Review and accept the stages in this order:

1. Stage 0: naming cleanup.
2. Stages 1-2: mode vocabulary and safe rasterized GBuffer reorganization.
3. Stage 3: raytraced GBuffer vertical slice.
4. Stage 4: lighting mode ownership.
5. Stage 5: path traced lighting.
6. Stage 6: remove the frame-wide reference path.
7. Stages 7-9: remove the frame-wide render path and polish settings.

If you want the smallest useful code batch first, accept Stages 1-2 together. If you want the first visible mode, accept Stage 3 after Stages 1-2 are merged.

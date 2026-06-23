# Indirect Diffuse Ray Tracing Implementation Plan

## Purpose

This document turns the evaluation in `Docs/Architecture/00-Review/IndirectDiffuseRayTracingIntroductionEvaluation.md` into staged implementation prompts.

Main goal:

- Add cosine-weighted stochastic ray-traced indirect diffuse lighting.

Non-negotiable constraints:

- No denoiser.
- No temporal reuse in the shipped path.
- No ReSTIR, DDGI, NRC, SHaRC, NRD, SVGF, or DLSS-RR integration in the first implementation.
- Every stage must be buildable and testable.
- Every stage must leave a small renderer/refactor improvement behind.
- Instrumentation must stay centralized and low noise.

## Target End State

At the end of this plan, SparkleEngine should have:

- a real pass-family hierarchy for frame composition
- explicit lighting target initialization
- current sky-ambient indirect diffuse isolated as fallback or legacy ambient
- a new ray-traced `IndirectDiffuse` pass
- `IndirectDiffuse` CVars, settings, and one centralized runtime status snapshot
- full-resolution one-sample stochastic diffuse ray tracing
- cosine hemisphere sampling only
- no-denoiser live output
- debug modes that make incorrect math obvious
- shared environment map binding/helper code
- shared ray tracing capability and TLAS access queries used by effect passes
- neutral pass-binding helpers for TLAS, hit data, material texture tables, and environment maps
- neutral shader libraries for ray-query tracing, hit reconstruction, incident lighting, path-sample data, random numbers, and sampling PDFs
- staged cleanup path for ray-query helpers and hit-data infrastructure

## Ray Tracing Architecture Contract

Ray tracing must be treated as a reusable renderer capability. `IndirectDiffuse` is the first new consumer in this plan, not the naming source for the architecture.

Layer ownership:

- `Engine/Renderer/Private/RayTracing/Scene/*` owns scene TLAS frame resources and shader access mode facts.
- `Engine/Renderer/Private/RayTracing/Acceleration/*` owns BLAS/TLAS build strategy and acceleration-structure lifetime policy.
- `Engine/Renderer/Private/RayTracing/RayTracingCapabilityReport.*` and any new ray tracing capability query files own backend feature facts.
- `Engine/Renderer/Private/RayTracing/RayTracingHitData.*` owns hit-data buffer contracts and rejection vocabulary.
- `Engine/Renderer/Private/Passes/Bindings/*` owns reusable pass parameter binding helpers.
- `Engine/Renderer/Private/RayTracing/Effects/<EffectName>/*` owns only effect settings, CVars, status snapshots, uniform data, and pass-data assembly.
- `Engine/Renderer/Private/Passes/Deferred/<EffectName>Pass.*` owns the leaf frame-graph pass wrapper and resource reads/writes for that effect.
- `Engine/Renderer/ShaderRegistrations/*Shaders.cpp` owns shader package registration and feature flags only.

Shader ownership:

- `Engine/Assets/Shaders/Common/*` owns general math, random numbers, and sampling primitives.
- `Engine/Assets/Shaders/BRDF/*` owns BSDF/BRDF functions.
- `Engine/Assets/Shaders/RayTracing/*` owns ray-query helpers, hit-surface reconstruction, hit lighting, rejection/debug helpers, and neutral path-sample structs.
- `Engine/Assets/Shaders/Lighting/*` owns sky/environment lighting helpers.
- `Engine/Assets/Shaders/Passes/Deferred/*` owns pass orchestration shaders that bind resources, call reusable libraries, and write pass outputs.

Naming rules:

- If a helper can be used by shadows, indirect specular, indirect diffuse, path tracing, or future GI, name it after the underlying concept, not the first caller.
- Files under `RayTracing/Effects/IndirectDiffuse` may mention indirect diffuse settings, CVars, debug modes, status reasons, and uniform layout.
- Files under generic `RayTracing`, `Passes/Bindings`, `Common`, `BRDF`, or `Lighting` must not depend on `IndirectDiffuse` settings, status, debug enums, or CVars.
- Effect passes translate generic capability facts into effect-specific status reasons. They must not each rediscover backend support through local backend-name branches.
- Shader package feature flags must describe actual resource use. Do not load a package variant that cannot bind its required TLAS/material resources.

Generalization gate for every new file:

1. If the file describes backend support, TLAS access, descriptor availability, hit data, material texture tables, or shader package capability, put it in a neutral ray tracing or binding layer.
2. If the file describes a sampling primitive, PDF, hit-surface layout, incident-radiance evaluation, or path-sample record, put it in a neutral shader library.
3. If the file describes an effect policy knob, debug mode, runtime status, or pass-specific uniform layout, keep it effect-specific.
4. A generic helper may serve one effect on the day it is introduced only when it represents a stable renderer concept. Its API must not accept effect-specific settings.
5. Do not create broad "renderer utility" buckets. Every shared helper must have a small, named responsibility.

## Estimator Contract

`IndirectDiffuse` texture stores incoming diffuse irradiance divided by pi, before applying primary-surface base color.

`LightingComposite.hlsl` remains responsible for:

```hlsl
indirectDiffuse = terms.IndirectDiffuse * response.DiffuseWeight * response.DiffuseColor * response.IndirectDiffuseOcclusion;
```

General estimator:

```hlsl
// w is the sampled incoming direction in world space.
// p(w) is the selected sampler PDF over the hemisphere.
// Li(w) is ray-hit or sky-miss radiance.
IndirectDiffuse = Li(w) * saturate(dot(N, w)) / (PI * max(p(w), 1.0e-6f));
```

Cosine-hemisphere baseline:

```hlsl
p(w) = saturate(dot(N, w)) / PI
IndirectDiffuse = Li(w)
```

Do not multiply `BaseColor` in the ray-traced diffuse pass.

## CVar Contract

Introduce these CVars exactly unless implementation finds an existing naming collision:

```text
r.RayTracing.IndirectDiffuse.Enabled
r.RayTracing.IndirectDiffuse.DebugMode
r.RayTracing.IndirectDiffuse.NormalBias
r.RayTracing.IndirectDiffuse.MaxDistance
r.RayTracing.IndirectDiffuse.Intensity
```

Initial enum values:

```text
DebugMode:
  0 = Off
  1 = HitMask
  2 = HitDistance
  3 = SampleDirection
  4 = SamplePdf
  5 = HitRadiance
  6 = FinalContribution
  7 = HitNormal
  8 = MaterialBaseColor
  9 = MissSkyRadiance
  10 = RejectionReason
```

Stage 4 baseline only needs `Enabled`, `DebugMode=0`, `NormalBias`, `MaxDistance`, and `Intensity`.

## Status Contract

Add `IndirectDiffuseStatusReason` with exactly:

```text
not-evaluated
disabled
unsupported
missing-tlas
missing-hit-data
missing-sky-texture
running
```

`unsupported` must be used when:

- the backend lacks ray tracing or inline ray query
- the TLAS access mode is unsupported by the current pass variant
- material texture table binding is required but unavailable

Do not use black output as the only signal for unsupported state.

## Instrumentation Budget

Hardening in this plan must not become logs and validation checks scattered across the renderer.

Allowed:

- one `IndirectDiffuseRuntimeStatus` snapshot
- editor overlay rows that read the same snapshot
- debug view modes in the indirect diffuse shader

Not allowed:

- per-frame logging from shaders, passes, frame graph, hit-data upload, or TLAS build just for this feature
- new feature-specific status logs
- new validation systems outside existing RHI/frame-graph/shader package validation surfaces
- duplicate status strings in multiple subsystems
- broad "just in case" counters
- extra editor panels for this feature
- source-backed validation levels or smoke-command suites for this feature

If a stage needs a new counter, it must be added to the centralized runtime status snapshot and named in that stage's acceptance criteria.

## Pass Family Contract

Sparkle's frame composition should be hierarchical. `FrameGraphBuilder` remains the graph authoring API, but renderer frame assembly should read as a set of pass families, not a flat list of unrelated effects.

Target hierarchy:

```text
BuildFrame
  GBuffer
  RayTracingInfrastructure
  Lighting
    DirectLighting
    IndirectLighting
    LightingComposite
    Sky
  PostProcessing
    UpscalerEvaluation
  Debug
  Presentation
```

Ownership rules:

- `Frame/*` files compose pass families and own ordering.
- `Passes/*` files implement leaf passes and own pass resources/execution.
- `ShaderRegistrations/*` files own shader package registration only.
- A family helper may call other family helpers.
- A leaf pass must not decide where it sits in the frame.
- Debug visualization belongs to `Debug`, not to lighting, post-processing, or presentation.
- Upscaler evaluation belongs to `PostProcessing`, not the frame root.
- Sky/environment scene-color resolve belongs to `Lighting` after lighting composite, not to post-processing.

Pass-family order:

- The hierarchy refactor sets this order:
  - GBuffer
  - ray tracing scene build passes
  - direct lighting
  - current ambient indirect lighting
  - indirect specular
  - lighting composite
  - sky
  - upscaler evaluation
  - visualize buffers
  - presentation

This intentionally places `Debug` after `PostProcessing`. Do not split debug by color-space labels unless the renderer later gains genuinely separate debug passes that require different placement.

## Stage 0 - Baseline Checkpoint

### Implementation Prompt

Inspect current indirect lighting, indirect specular, ray tracing scene, frame graph, and existing runtime status snapshot patterns. Do not edit source. Record the baseline files that will be touched by later stages.

### Required Source Reads

- `Engine/Renderer/Private/Frame/Lighting/Lighting.cpp`
- `Engine/Renderer/Private/Passes/Deferred/IndirectLightingPass.*`
- `Engine/Assets/Shaders/Passes/Deferred/IndirectLighting.hlsl`
- `Engine/Renderer/Private/Passes/Deferred/IndirectSpecularPass.*`
- `Engine/Assets/Shaders/Passes/Deferred/IndirectSpecular.hlsl`
- `Engine/Assets/Shaders/RayTracing/RayTracingMaterialHit.hlsli`
- `Engine/Assets/Shaders/RayTracing/RayTracingHitSurface.hlsli`
- `Engine/Assets/Shaders/RayTracing/RayTracingHitLighting.hlsli`
- `Engine/Assets/Shaders/RayTracing/RayTracingHitDebug.hlsli`
- `Engine/Assets/Shaders/Common/Sampling.hlsli`
- `Engine/Assets/Shaders/Common/Random.hlsli`
- `Engine/Renderer/Private/RayTracing/RayTracingCapabilityReport.*`
- `Engine/Renderer/Private/RayTracing/Scene/RayTracingSceneTlasShaderAccessMode.h`
- `Engine/Renderer/Private/RayTracing/Scene/RenderRayTracingPassServices.h`
- `Engine/Renderer/Private/Passes/Bindings/RayTracingHitDataPassBinding.h`
- `Engine/Renderer/Private/Passes/Bindings/MaterialTextureTablePassBinding.h`

### Acceptance

- No source edits.
- A short implementation note can name the current startup command and baseline indirect specular status line.

## Stage 1A - Pass Family Composition Hierarchy

### Implementation Prompt

Introduce pass-family composition helpers so the frame reads as `GBuffer`, `RayTracingInfrastructure`, `Lighting`, `PostProcessing`, `Debug`, and `Presentation`. Do not add new shader passes. Move the current visualization pass into the `Debug` family after post-processing. Keep sky inside the `Lighting` family after lighting composite.

### Files

Create:

- `Engine/Renderer/Private/Frame/Core/FrameSceneResources.h`
- `Engine/Renderer/Private/Frame/Core/FrameSceneResources.cpp`
- `Engine/Renderer/Private/Frame/Lighting/AmbientIndirectLighting.h`
- `Engine/Renderer/Private/Frame/Lighting/AmbientIndirectLighting.cpp`
- `Engine/Renderer/Private/Frame/Debug/Debug.h`
- `Engine/Renderer/Private/Frame/Debug/Debug.cpp`
- `Engine/Renderer/Private/Frame/PostProcessing/PostProcessing.h`
- `Engine/Renderer/Private/Frame/PostProcessing/PostProcessing.cpp`

Modify:

- `Engine/Renderer/Private/Frame/Core/Frame.cpp`
- `Engine/Renderer/Private/Frame/Deferred/GBuffer.h`
- `Engine/Renderer/Private/Frame/Deferred/GBuffer.cpp`
- `Engine/Renderer/Private/Frame/RayTracing/RayTracingScene.h`
- `Engine/Renderer/Private/Frame/RayTracing/RayTracingScene.cpp`
- `Engine/Renderer/Private/Frame/Lighting/IndirectLighting.h`
- `Engine/Renderer/Private/Frame/Lighting/IndirectLighting.cpp`
- `Engine/Renderer/Private/Frame/Lighting/Lighting.h`
- `Engine/Renderer/Private/Frame/Lighting/Lighting.cpp`
- `Engine/Renderer/Private/Frame/Presentation/Upscaling.h`
- `Engine/Renderer/Private/Frame/Presentation/Upscaling.cpp`

Read and call through without changing unless a compile-time signature mismatch proves otherwise:

- `Engine/Renderer/Private/Frame/Lighting/IndirectSpecular.h`
- `Engine/Renderer/Private/Frame/Lighting/Sky.h`
- `Engine/Renderer/Private/Frame/Debug/VisualizeBuffers.h`

Build-system note:

- `Engine/Renderer/CMakeLists.txt` currently uses recursive `CONFIGURE_DEPENDS` globs for renderer private sources and headers. Do not edit the renderer CMake file for this stage unless the new files are not picked up by the build.

### Required Behavior

- `AddIndirectLightingPasses(...)` calls the current ambient indirect lighting pass and indirect specular pass.
- `AddLightingPasses(...)` creates lighting targets, calls direct lighting, calls `AddIndirectLightingPasses(...)`, calls lighting composite, and then calls sky.
- `AddPostProcessingPasses(...)` calls `AddUpscalerEvaluationPass(...)` and assigns provider inputs through `CreateUpscalerProviderInputs(...)`.
- `AddDebugPasses(...)` calls `VisualizeBuffers`.
- `AddLightingPasses(...)` no longer includes visualization.
- `BuildFrame(...)` does not directly declare scene, GBuffer, lighting, ray tracing, or provider-input resources; those resources are owned by the family/helper that adds the related passes.
- `BuildFrame(...)` calls families in this exact order:
  1. `CreateFrameSceneResources(...)`
  2. `AddGBufferPasses(...)`
  3. `AddRayTracingInfrastructurePasses(...)`
  4. `AddLightingPasses(...)`
  5. `AddPostProcessingPasses(...)`
  6. `AddDebugPasses(...)`
  7. `AddPresentationPass(...)`
- Shader leaf pass names registered into the frame graph remain unchanged.
- The external-provider leaf pass is named for upscaling, not generic provider evaluation.
- The `VisualizeBuffers` leaf pass now runs after the current post-processing family.

### Acceptance

- No shader source changes.
- No shader recook required.
- `cmake --build build --target ShowcaseEditor --config DevelopmentEditor` succeeds.
- Runtime frame graph preserves existing shader leaf pass names; the upscaler external-provider leaf is named for upscaling.
- No new logs or diagnostics are added.

## Stage 1B - Explicit Lighting Target Initialization

### Implementation Prompt

Add a lighting target clear helper that clears all six lighting targets before any lighting producer runs. Use the frame graph's existing render-target clear path; do not add a shader package just to write zeroes. Then remove cross-target clearing responsibility from `IndirectLighting`.

### Files

Create:

- `Engine/Renderer/Private/Frame/Lighting/LightingTargetClear.h`
- `Engine/Renderer/Private/Frame/Lighting/LightingTargetClear.cpp`

Modify:

- `Engine/Renderer/Private/Frame/Lighting/Lighting.cpp`
- `Engine/Renderer/Private/Frame/Lighting/LightingRenderTargets.cpp`
- `Engine/Assets/Shaders/Passes/Deferred/IndirectLighting.hlsl`
- `Engine/Renderer/Private/Passes/Deferred/IndirectLightingPass.*`
- `Engine/Renderer/ShaderRegistrations/IndirectLightingShaders.cpp`

### Required Behavior

- Lighting targets use transparent black clear color.
- `AddLightingTargetClearPass(...)` clears these targets through `FrameGraphResourceCommands::ClearRenderTarget(...)`:
  - `DirectDiffuse`
  - `DirectSpecular`
  - `DirectSubsurface`
  - `IndirectDiffuse`
  - `IndirectSpecular`
  - `IndirectSubsurface`
- `AddLightingPasses(...)` calls clear before direct lighting.
- `IndirectLighting.hlsl` no longer writes `IndirectSpecularTexture`.
- `IndirectLighting.hlsl` writes only `IndirectDiffuseTexture`.
- `IndirectLightingPassParameters` no longer declares `IndirectSpecular` or `IndirectSubsurface`.
- `IndirectLightingShaders.cpp` no longer registers `IndirectSpecularTexture` or `IndirectSubsurfaceTexture` bindings for the ambient indirect pass.
- Disabled ray tracing effects can early-return without leaving stale transient contents.

### Acceptance

- `cmake --build build --target ShowcaseEditor --config DevelopmentEditor` succeeds.
- The changed `IndirectLighting` shader package cooks successfully.
- `Lit`, `DirectDiffuse`, `IndirectDiffuse`, and `IndirectSpecular` view modes still render.
- No new logs or diagnostics are added.

## Stage 2 - Environment Map Binding Helper

### Implementation Prompt

Extract sky/environment descriptor binding into a reusable renderer helper and shared HLSL include. Update `SkyPass`, `IndirectLightingPass`, and `IndirectSpecularPass` to use the helper.

### Files

Create:

- `Engine/Renderer/Private/Passes/Bindings/EnvironmentMapPassBinding.h`
- `Engine/Renderer/Private/Passes/Bindings/EnvironmentMapPassBinding.cpp`
- `Engine/Assets/Shaders/Lighting/SkyEnvironment.hlsli`

Modify:

- `SkyPass.*`
- `IndirectLightingPass.*`
- `IndirectSpecularPass.*`
- `Sky.hlsl`
- `IndirectLighting.hlsl`
- `IndirectSpecular.hlsl`

### Required Behavior

- CPU helper resolves `TextureId::SkyCubemap`, falling back to `TextureId::Checker`.
- CPU helper owns descriptor binding set caching.
- CPU helper returns empty binding if no texture manager or fallback texture exists.
- CPU helper is named as an environment map binding helper, not a sky-pass helper, because ray misses, ambient fallback, and sky resolve all consume the same environment source.
- HLSL helper exposes:
  - `float2 ComputeSkyEnvironmentUv(float3 worldDirection)`
  - `float3 ToneMapSkyEnvironment(float3 skyRadiance)`
  - `float3 SampleSkyEnvironment(Texture2D texture, SamplerState sampler, float3 worldDirection)`
- Existing visible sky and indirect specular miss colors remain visually consistent.

### Acceptance

- No duplicated `ResolveSkyTexture(...)` function remains in sky, indirect lighting, or indirect specular pass implementation files.
- `IndirectSpecular` still reports `reason=running`.
- The sky miss fix from indirect specular remains active.

## Stage 3 - Indirect Diffuse Settings And Status Snapshot

### Implementation Prompt

Add the indirect diffuse settings, CVars, one centralized runtime status snapshot, and editor overlay rows that read the same snapshot. Do not add ray tracing behavior yet. Do not add feature-specific logs.

### Files

Create:

- `Engine/Renderer/Private/RayTracing/Effects/IndirectDiffuse/IndirectDiffuseCVars.h`
- `Engine/Renderer/Private/RayTracing/Effects/IndirectDiffuse/IndirectDiffuseCVars.cpp`
- `Engine/Renderer/Private/RayTracing/Effects/IndirectDiffuse/IndirectDiffuseSettings.h`
- `Engine/Renderer/Private/RayTracing/Effects/IndirectDiffuse/IndirectDiffuseSettings.cpp`
- `Engine/Renderer/Private/RayTracing/Effects/IndirectDiffuse/IndirectDiffuseRuntimeStatus.h`
- `Engine/Renderer/Private/RayTracing/Effects/IndirectDiffuse/IndirectDiffuseRuntimeStatus.cpp`
- `Engine/Renderer/Private/RayTracing/Effects/IndirectDiffuse/IndirectDiffuseDebugMode.h`

Modify:

- `Engine/Renderer/Private/RayTracing/Scene/RenderRayTracingPassServices.h`
- `Engine/Editor/Private/Panels/ViewportRayTracingDebugOverlay.cpp`
- CMake lists under `Engine/Renderer` only if the existing renderer source/header globs do not pick up the new files

### Required Behavior

- Defaults:
  - `Enabled=false`
  - `DebugMode=Off`
  - `NormalBias=0.01`
  - `MaxDistance=100000.0`
  - `Intensity=1.0`
- Runtime status snapshot publishes `disabled` if disabled.
- Runtime status snapshot includes:
  - status reason
  - enabled
  - debug mode
  - max distance
  - intensity
  - hit-data availability
  - hit instance/material counts
  - GPU timing label string

### Acceptance

- Launch with `--cvar=r.RayTracing.IndirectDiffuse.Enabled=true`.
- Before the pass exists, the centralized status snapshot may remain `not-evaluated`, but code must compile and expose the CVar.
- No indirect specular status snapshot regressions.
- No new log line is emitted.

## Stage 3A - General Ray Tracing Capability And Binding Contracts

### Implementation Prompt

Introduce neutral capability and binding contracts that indirect diffuse will consume later. Do not add indirect diffuse ray tracing yet. Do not change indirect specular output. Do not add logs or validation systems.

### Files

Create if no equivalent abstraction already exists:

- `Engine/Renderer/Private/RayTracing/RayTracingPassCapabilityQuery.h`
- `Engine/Renderer/Private/RayTracing/RayTracingPassCapabilityQuery.cpp`
- `Engine/Renderer/Private/Passes/Bindings/RayTracingScenePassBinding.h`

Modify:

- `Engine/Renderer/Private/RayTracing/RayTracingCapabilityReport.*`
- `Engine/Renderer/Private/RayTracing/Scene/RenderRayTracingPassServices.h`
- `Engine/Renderer/Private/Passes/Deferred/IndirectSpecularPass.*`
- `Engine/Renderer/Private/RayTracing/Effects/IndirectSpecular/IndirectSpecularPassData.*`

Read and preserve:

- `Engine/Renderer/Private/Passes/Bindings/RayTracingHitDataPassBinding.h`
- `Engine/Renderer/Private/Passes/Bindings/MaterialTextureTablePassBinding.h`
- `Engine/Renderer/Private/RayTracing/Scene/RayTracingSceneTlasShaderAccessMode.h`

### Required Behavior

- A single neutral query object reports:
  - backend ray tracing availability
  - inline ray query availability
  - bound scene TLAS availability
  - scene TLAS shader access mode
  - descriptor TLAS support
  - device-address TLAS support
  - hit-data availability
  - material texture table availability and descriptor count
- Effect passes map these neutral facts to their own status reasons.
- Effect passes must not branch directly on backend API names when the neutral query can provide the answer.
- `RayTracingScenePassBinding` binds descriptor TLAS or device-address TLAS according to explicit access mode. It must not mention indirect diffuse, indirect specular, or shadows.
- `IndirectSpecularPass` may migrate to the neutral query and binding helper, but its status reasons and output must remain unchanged.
- Shader package selection continues to use package feature flags that match actual resource use.

### Acceptance

- `cmake --build build --target ShowcaseEditor --config DevelopmentEditor` succeeds.
- `IndirectSpecular` mirror mode still reaches `reason=running`.
- No new shader source is required for this stage.
- No new logs or diagnostics are added.
- No reusable file created in this stage contains `IndirectDiffuse` in its name.

## Stage 4 - Ray-Traced Indirect Diffuse Baseline

### Implementation Prompt

Add `IndirectDiffusePass` and `IndirectDiffuse.hlsl`. The pass must trace one cosine-hemisphere diffuse ray per non-sky GBuffer pixel and write raw one-sample indirect diffuse radiance. No denoiser. No temporal accumulation. No environment importance sampling. Reuse the neutral ray tracing capability, binding, hit-data, environment, sampling, and random helpers from earlier stages.

### Files

Create:

- `Engine/Renderer/Private/Passes/Deferred/IndirectDiffusePass.h`
- `Engine/Renderer/Private/Passes/Deferred/IndirectDiffusePass.cpp`
- `Engine/Renderer/Private/Frame/Lighting/IndirectDiffuse.h`
- `Engine/Renderer/Private/Frame/Lighting/IndirectDiffuse.cpp`
- `Engine/Renderer/ShaderRegistrations/IndirectDiffuseShaders.cpp`
- `Engine/Assets/Shaders/Passes/Deferred/IndirectDiffuse.hlsl`
- `Engine/Assets/Shaders/RayTracing/RayTracingPathSample.hlsli`
- `Engine/Renderer/Private/RayTracing/Effects/IndirectDiffuse/IndirectDiffuseUniformData.h`
- `Engine/Renderer/Private/RayTracing/Effects/IndirectDiffuse/IndirectDiffusePassData.h`
- `Engine/Renderer/Private/RayTracing/Effects/IndirectDiffuse/IndirectDiffusePassData.cpp`

Modify:

- `Engine/Renderer/Private/Frame/Lighting/Lighting.cpp`
- `Engine/Renderer/ShaderRegistrations/RendererShaderPackages.h`
- `Engine/Assets/Shaders/Common/Sampling.hlsli` only if cosine-hemisphere sampling is missing there
- CMake lists only if existing globs do not pick up new renderer files

Must reuse:

- `RayTracingPassCapabilityQuery`
- `RayTracingScenePassBinding`
- `RayTracingHitDataPassBinding`
- `MaterialTextureTablePassBinding`
- `EnvironmentMapPassBinding`
- `Common/Sampling.hlsli`
- `Common/Random.hlsli`
- `SkyEnvironment.hlsli`
- existing `RayTracing/*` hit-surface and hit-lighting includes

### Shader Inputs

The shader must bind:

- `RWTexture2D<float4> IndirectDiffuseTexture`
- `RaytracingAccelerationStructure SceneTlas`
- `PerFrame`
- `PerView`
- `ViewLighting`
- `IndirectDiffuseConstants`
- `GBufferBaseColor`
- `GBufferNormal`
- `GBufferMaterial`
- `GBufferDeviceZ`
- `SkyTexture`
- `SamplerLinearClamp`
- `RayTracingHitVertices`
- `RayTracingHitIndices`
- `RayTracingHitInstances`
- `RayTracingHitMaterials`
- `MeshInstances`
- `DirectionalLights`
- `PointLights`
- `SpotLights`
- `MaterialTextureTable`
- `MaterialTextureSampler`

### Shader Algorithm

For each pixel:

1. If outside viewport, return.
2. Load `GBufferData`.
3. If sky pixel, write `0.0f.xxxx` and return.
4. Reconstruct primary position and normal.
5. Generate a deterministic per-frame random sample from pixel coordinate and frame index using `Common/Random.hlsli`.
6. Build cosine-weighted hemisphere direction around the primary normal using a neutral `Common/Sampling.hlsli` helper.
7. Trace a ray from biased primary position.
8. If ray misses:
   - sample sky environment in ray direction
   - set `Li` to sky color/radiance consistent with `SkyEnvironment.hlsli`
9. If ray hits:
   - reconstruct `RayTracingHitSurfaceData`
   - set `Li` to `ShadeRayTracingHitIncidentRadiance(hitSurface, sampleDirection)`
10. For cosine hemisphere mode, write:
    - `IndirectDiffuseTexture[pixel] = float4(Li * Intensity, alphaSignal)`

`RayTracingPathSample.hlsli` must define neutral sample/result structs for direction, PDF, cosine term, hit/miss state, incident radiance, contribution, hit distance, and rejection reason. It must not include indirect diffuse settings, CVars, or debug enums.

### Runtime Gating

Return without dispatch and publish status if:

- disabled
- no bound TLAS
- TLAS access mode unsupported
- missing hit data
- material texture table unavailable
- sky texture unavailable

All of these checks must read from the neutral capability/query/binding helpers first, then map to `IndirectDiffuseStatusReason`.

### Required Feature Flags

The shader package must declare:

- `UsesInlineRayQuery`
- `UsesAccelerationStructure`
- `UsesDescriptorIndexing`

### Acceptance

- `ShaderCompiler.exe cook --package IndirectDiffuse --target DxilSm66 --target SpirV16 --no-cache` succeeds.
- `ShowcaseEditor` build succeeds.
- With `r.RayTracing.IndirectDiffuse.Enabled=true`, the centralized status snapshot and editor overlay show `reason=running`, `enabled=true`, and `debugMode=0`.
- `IndirectDiffuse` view mode shows stochastic traced lighting, not the old smooth sky ambient.

## Stage 5 - Debug Modes

### Implementation Prompt

Add indirect diffuse debug modes and make the existing editor overlay report the selected mode clearly.

### Files

Create:

- `Engine/Assets/Shaders/Passes/Deferred/IndirectDiffuseDebug.hlsli`

Modify:

- `IndirectDiffuse.hlsl`
- `IndirectDiffuseDebugMode.h`
- `IndirectDiffuseCVars.cpp`
- `ViewportRayTracingDebugOverlay.cpp`
- `Engine/Assets/Shaders/RayTracing/RayTracingPathSample.hlsli` only if a neutral field is missing
- `Engine/Assets/Shaders/RayTracing/RayTracingHitDebug.hlsli` only if a neutral rejection-color helper is missing

### Required Debug Output

Implement:

- `HitMask`: white for valid hit, blue for miss, red for rejected hit surface.
- `HitDistance`: normalized by `MaxDistance`.
- `SampleDirection`: direction mapped to `0.5 * dir + 0.5`.
- `SamplePdf`: visible scalar.
- `HitRadiance`: tone-mapped `Li`.
- `FinalContribution`: tone-mapped output after PDF and intensity.
- `HitNormal`: reconstructed hit normal.
- `MaterialBaseColor`: hit material base color.
- `MissSkyRadiance`: sky miss color only, black for hits.
- `RejectionReason`: stable categorical colors for ray tracing hit-data rejection reasons.

`IndirectDiffuseDebug.hlsli` may map the indirect diffuse debug enum to output colors. Shared rejection colors, hit/miss state fields, path-sample fields, and tone-map helpers must remain in neutral shader includes.

### Acceptance

- Debug mode 3 must not be used as final lighting.
- The centralized status snapshot and existing overlay must show `debugMode=<value>`.
- `IndirectDiffuse` view mode remains final contribution when `DebugMode=0`.
- Do not add a new editor panel.
- Do not add per-debug-mode log lines.
- Do not add generic ray tracing helpers to `IndirectDiffuseDebug.hlsli`.

## Stage 6 - Shared Ray Query Helper Refactor

### Implementation Prompt

Extract common ray query trace code from indirect specular and indirect diffuse after both paths exist. Keep shadow rays separate unless the same helper clearly fits. The helper must describe ray-query behavior, hit acceptance, and alpha-tested candidate handling, not indirect diffuse or indirect specular policy.

### Files

Create:

- `Engine/Assets/Shaders/RayTracing/RayTracingTraceQuery.hlsli`

Modify:

- `IndirectSpecular.hlsl`
- `IndirectDiffuse.hlsl`
- `Engine/Assets/Shaders/RayTracing/RayTracingPathSample.hlsli` only if trace results need one neutral field shared by both effects

### Required Helper API

Expose:

```hlsl
RayTracingTraceResult TraceRayQueryWithAlphaTest(
    RaytracingAccelerationStructure sceneTlas,
    float3 originWorld,
    float3 directionWorld,
    float tMin,
    float tMax,
    uint rayFlags,
    uint instanceMask)
```

The helper must:

- perform alpha-tested candidate resolution
- populate `RayTracingTraceResult`
- preserve alpha debug fields
- not know about diffuse or specular settings
- not know about diffuse/specular debug modes, CVars, intensity values, or output textures
- call existing hit-surface reconstruction helpers instead of duplicating material table reads

### Acceptance

- Indirect specular output is unchanged in mirror mode.
- Indirect diffuse output is unchanged in cosine mode.
- No duplicate alpha-tested candidate loops remain in diffuse/specular shaders.
- No file created in this stage contains `IndirectDiffuse` or `IndirectSpecular` unless it is the pass orchestration shader being modified.

## Stage 7 - TLAS Access Mode Contract

### Implementation Prompt

Make the first indirect diffuse release descriptor-TLAS only, matching current indirect specular, using the neutral ray tracing capability query from Stage 3A. Add explicit unsupported status for address-only TLAS access. Do not add a Vulkan address shader variant in this stage.

### Files

Likely modify:

- `IndirectDiffusePass.*`
- `IndirectDiffuseShaders.cpp`
- `RendererShaderPackages.h`
- `RenderRayTracingPassServices.h`

### Acceptance

- D3D12 runs.
- Vulkan descriptor mode runs if supported.
- Vulkan address mode reports `unsupported` with a precise reason.
- No shader package is loaded for a pass variant that cannot bind its TLAS.
- No effect pass branches on backend API name to decide TLAS binding. It reads the neutral TLAS access mode and capability facts.

## Stage 7B - Vulkan Address Variant

### Implementation Prompt

Add Vulkan address-mode support only after Stage 7 is validated. Mirror direct lighting's address variant structure, but keep package selection behind the neutral TLAS access mode/capability query.

### Required Behavior

- Add `IndirectDiffuseVulkanAddress` package.
- Required feature flags include `UsesInlineRayQuery`, `UsesAccelerationStructure`, `UsesAccelerationStructureDeviceAddress`, and `UsesDescriptorIndexing`.
- Use the same estimator contract as descriptor mode.
- Keep descriptor mode unchanged.
- The effect pass asks the neutral query for TLAS access mode and selects the matching package. It must not infer package choice from a hardcoded backend string.
- Shared HLSL stays in neutral includes. The Vulkan-address pass shader should contain only access-mode glue plus the same indirect diffuse orchestration.

### Acceptance

- D3D12 descriptor mode still runs.
- Vulkan address mode reaches `running`.
- Both packages cook for `SpirV16`.
- Descriptor and address variants produce comparable output for the same scene in cosine mode.

## Stage 8 - Hit Data Refinement

### Implementation Prompt

Improve shared hit-data infrastructure exposed by the diffuse work. Do not change diffuse math in this stage. The result must help all ray-traced hit shading consumers, including indirect specular and future effects, not only indirect diffuse.

### Ordered Sub-Stages

Stage 8A:

- Add rejection counters to the centralized runtime status snapshot for skinned, alpha-blended, missing mesh hit data, invalid material, invalid primitive, and invalid vertex index.
- Do not log individual rejected instances.

Stage 8B:

- Add persistent hit-data buffers keyed by mesh/material generation.
- Keep the old per-frame upload path behind a temporary fallback until persistent data is validated.

Stage 8C:

- Add texture mip selection for RT material sampling.
- Use ray cones or a documented explicit LOD heuristic; do not leave hit shading permanently locked to mip 0.

Stage 8D:

- Add skinned mesh hit-data support from the deformed geometry snapshot.
- Keep unsupported status for skinned meshes until BLAS geometry and hit buffers consume the same deformed data.

Stage 8E:

- Document alpha-blended geometry as unsupported for indirect diffuse, or implement a named approximation mode. Do not silently treat alpha-blended geometry as opaque.

### Acceptance

- Each sub-stage has one specific centralized status field before/after.
- Existing indirect specular material behavior remains unchanged.
- Indirect diffuse runtime status still reaches `running`.
- Hit-data API names remain effect-neutral.

## Stage 9 - Performance And Quality Controls

### Implementation Prompt

Add user-facing quality/performance controls without adding a denoiser.

### Allowed Controls

- `r.RayTracing.IndirectDiffuse.RayStride`
- `r.RayTracing.IndirectDiffuse.Checkerboard`
- `r.RayTracing.IndirectDiffuse.HalfResolution`

### Restrictions

- Do not add temporal accumulation.
- Do not add spatial filtering.
- Do not add history reprojection.

### Acceptance

- Default remains one ray per full-resolution shaded pixel.
- Any reduced-resolution mode has an explicit disabled default.
- Debug views still work at full resolution.

## Stage 10 - Future Work Not In First Integration

These are explicitly out of the first implementation:

- ReSTIR GI
- ReSTIR PT
- DDGI probe volumes
- SHaRC
- NRC
- NRD
- SVGF
- DLSS-RR denoising
- temporal GI accumulation
- mesh-light importance sampling
- emissive triangle light reservoir sampling

When revisiting these later, start from the working `IndirectDiffuse` baseline and preserve the estimator contract.

## Daily Working Rule

For each stage:

1. Read the files listed in the stage.
2. Classify every new file through the generalization gate: capability, scene/acceleration, binding, shader library, effect policy, or leaf pass.
3. Make only the stage's scoped change.
4. Build `ShowcaseEditor`.
5. Recook only changed shader packages unless the stage changes shared shader registrations.
6. Manually inspect the relevant view mode when the stage affects runtime behavior.
7. Update the implementation plan if behavior or commands change.

Do not start the next stage until the current stage has a clean build, clean shader cook, and explainable runtime behavior when the stage affects rendering.

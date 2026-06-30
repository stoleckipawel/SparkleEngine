# Indirect Diffuse Ray Tracing Introduction Evaluation

## Purpose

This document evaluates the current SparkleEngine renderer issues and integration challenges involved in introducing ray-traced indirect diffuse lighting.

The main goal is `IndirectDiffuse` integration with cosine-weighted stochastic ray tracing and no denoiser.

Supporting goals:

- improve ray tracing pass structure while adding the feature
- improve frame graph ownership of lighting targets
- refine acceleration-structure and hit-data contracts where the diffuse feature exposes weaknesses
- follow the everyday refactor principle: each feature stage should leave one local subsystem cleaner than it found it

This is not an implementation plan. The staged implementation prompts are in:

- `Docs/Architecture/02-Contracts/IndirectDiffuseRayTracingImplementationPlan.md`

## External References Reviewed

Reviewed on 2026-06-23.

- NVIDIA RTXGI-DDGI repository: https://github.com/NVIDIAGameWorks/RTXGI-DDGI
  - Relevant lesson: DDGI is scalable diffuse GI based on irradiance probes, but it is low frequency and intentionally not a high-detail per-pixel diffuse bounce replacement.
- RTXGI-DDGI algorithm notes: https://github.com/NVIDIAGameWorks/RTXGI-DDGI/blob/main/docs/Algorithms.md
  - Relevant lesson: ray-traced diffuse GI needs explicit visibility/occlusion handling and has known latency/detail tradeoffs when cached in probes.
- NVIDIA RTXDI repository: https://github.com/NVIDIA-RTX/RTXDI
  - Relevant lesson: ReSTIR GI/PT are structured as reusable shader libraries plus renderer bridge functions, not as one-off application shaders.
- RTXDI ReSTIR GI integration notes: https://github.com/NVIDIA-RTX/RTXDI/blob/main/Doc/RestirGI.md
  - Relevant lesson: first build a regular limited path-tracing sample that produces secondary-surface position/orientation, sampling PDF, and reflected/emitted radiance; use the no-resampling mode to validate math before adding reuse.
- NVIDIA Falcor path tracer usage: https://github.com/NVIDIAGameWorks/Falcor/blob/master/docs/usage/path-tracer.md
  - Relevant lesson: low samples-per-pixel path tracing is normal, but the renderer must expose sample count, path length, and sampling strategies clearly. Environment, analytic-light, and mesh-light sampling should be explicit strategies.
- NVIDIA RTX Path Tracing sample: https://github.com/NVIDIA-RTX/RTXPT
  - Relevant lesson: a clean path tracing integration has an obvious folder structure, a small BSDF model that is easy to extend, environment lighting support, and explicit performance/quality presets.
- NVIDIA Vulkan ray tracing tutorials: https://github.com/nvpro-samples/vk_raytracing_tutorial_KHR
  - Relevant lesson: ray tracing integration should move through progressive, compilable phases and keep focused samples for shadows, reflections, ray queries, opacity, and related features.
- NVIDIA RTXGI v2 repository: https://github.com/NVIDIA-RTX/RTXGI
  - Relevant lesson: modern GI integrations separate path tracing from radiance caching. Caches are accelerators for indirect bounce evaluation, not a substitute for proving the base ray sample.
- Unreal Engine Render Dependency Graph documentation: https://dev.epicgames.com/documentation/unreal-engine/render-dependency-graph-in-unreal-engine
  - Relevant lesson: high-level rendering code should be expressed through a graph that owns resource dependencies and pass scheduling. Feature code should declare dependencies instead of hiding them in command emission.
- Unity Scriptable Render Pipeline fundamentals: https://docs.unity3d.com/6000.4/Documentation/Manual/scriptable-render-pipeline-introduction.html
  - Relevant lesson: a render pipeline is a scheduled frame program; pass organization should expose the frame's major phases, not only individual effects.
- Unity URP render graph pass documentation: https://docs.unity3d.com/6000.4/Documentation/Manual/urp/render-graph-write-render-pass.html
  - Relevant lesson: pass setup declares inputs and outputs, while execution emits commands. Unused resources in pass data are discouraged.
- NVIDIA Falcor getting started documentation: https://github.com/NVIDIAGameWorks/Falcor/blob/master/docs/getting-started.md
  - Relevant lesson: render passes are reusable building blocks for render graphs. Pass libraries and graph composition should be distinct concepts.

## Decision From Reference Review

The first SparkleEngine implementation should not start with DDGI, ReSTIR GI, ReSTIR PT, NRC, SHaRC, NRD, SVGF, or a temporal denoiser.

The first implementation should be:

- full-resolution compute pass
- one stochastic diffuse ray per shaded pixel by default
- cosine-hemisphere sampling as the correctness baseline
- no denoiser and no temporal reuse in the shipped path
- deterministic debug controls for sample direction, PDF, miss/hit radiance, and final contribution

This mirrors the RTXDI ReSTIR GI recommendation to validate a basic limited path-traced sample before adding resampling, while keeping Sparkle's immediate goal smaller than a full path tracer.

## Current Renderer State

### Current Frame Assembly Shape

`Engine/Renderer/Private/Frame/Core/Frame.cpp` currently owns the top-level frame assembly:

1. creates scene, final scene, back buffer, and depth targets
2. creates GBuffer targets
3. adds `GBuffer`
4. creates and builds ray tracing scene resources
5. creates lighting targets
6. calls `AddLightingPasses(...)`
7. calls upscaler evaluation directly
8. assigns provider input handles
9. optionally adds presentation

The code already has useful folders:

- `Frame/Deferred`
- `Frame/Lighting`
- `Frame/Presentation`
- `Frame/RayTracing`
- `Frame/Debug`
- `Passes/Deferred`
- `Passes/Presentation`
- `Passes/Debug`

The issue is not total absence of organization. The issue is that the frame composition depth stops too early, so major pass families are not obvious from the top-level assembly.

### Lighting Pass Order

`Engine/Renderer/Private/Frame/Lighting/Lighting.cpp` currently assembles lighting as:

1. `AddDirectLightingPass(...)`
2. `AddIndirectLightingPass(...)`
3. `AddIndirectSpecularPass(...)`
4. `AddLightingCompositePass(...)`
5. `AddVisualizeBuffersPass(...)`
6. `AddSkyPass(...)`

Implications:

- `IndirectLighting` runs before `IndirectSpecular`.
- `LightingComposite` consumes all direct and indirect lighting targets.
- `Sky` writes scene color after lighting/composite for sky pixels.
- `VisualizeBuffers` can inspect `IndirectDiffuse` and `IndirectSpecular`.
- `VisualizeBuffers` is conceptually post-lighting/debug work, but it is currently called from the lighting group.
- Upscaler evaluation is conceptually post-processing work, but it is currently called directly from the frame root.

### Current Indirect Diffuse

`Engine/Assets/Shaders/Passes/Deferred/IndirectLighting.hlsl` is not ray traced indirect diffuse. It is a sky-ambient approximation:

- reads `GBufferNormal`
- reads `GBufferDeviceZ`
- samples `SkyTexture` in the normal direction
- mixes against a constant ground bounce
- writes `IndirectDiffuseTexture`
- writes `IndirectSpecularTexture` and `IndirectSubsurfaceTexture` to zero

The current pass name is broader than its actual behavior. This is one of the largest sources of confusion for future work.

### Current Indirect Specular

The current indirect specular path already provides most of the per-pixel ray-query scaffolding required by diffuse:

- inline ray query in `IndirectSpecular.hlsl`
- `SceneTlas` descriptor binding
- ray origin bias
- alpha-tested candidate handling
- `RayTracingHitSurfaceData` reconstruction
- material texture table lookup
- hit-surface direct lighting and emissive evaluation through `RayTracingHitLighting.hlsli`
- sky miss handling after the recent fix
- settings and CVars

The indirect diffuse integration should reuse this shape, then extract shared pieces as soon as duplication appears.

## Current Issues

### 1. Lighting Target Ownership Is Too Implicit

Current `IndirectLighting` writes zero to `IndirectSpecularTexture` and `IndirectSubsurfaceTexture`. That makes the ambient diffuse pass responsible for clearing targets it does not conceptually own.

Problem:

- If `IndirectSpecular` is disabled, early-returned, or unsupported, zeroing currently depends on a different pass.
- Adding ray-traced `IndirectDiffuse` would repeat this unclear ownership unless fixed first.

Required cleanup:

- Add an explicit `LightingTargetClear` or equivalent initialization pass.
- Make each producer own only the targets it produces.
- Keep disabled/unsupported producers from leaving stale transient contents.

### 1A. Pass Family Hierarchy Is Too Shallow

Current frame assembly uses helpers, but several helpers mix different frame families:

- `AddLightingPasses(...)` owns direct lighting, indirect lighting, lighting composite, debug visualization, and sky resolve.
- `BuildFrame(...)` directly calls external upscaler evaluation instead of routing it through a post-processing/presentation family.
- `IndirectLighting` names both a conceptual family and one current ambient approximation pass.

Problem:

- It is hard to know where a new pass belongs.
- Adding indirect diffuse risks making `Lighting.cpp` more crowded.
- Debug visualization and upscaling are not discoverable as post-lighting/post-processing stages.
- A future GBuffer expansion, such as depth prepass or material classification, has no obvious group boundary.

Required cleanup:

- Introduce pass-family composition helpers with an explicit `Debug` family after `PostProcessing`.
- Keep leaf shader pass classes separate from family composition helpers.
- Use folder and function names that expose the frame hierarchy:
  - `GBuffer`
  - `RayTracingInfrastructure`
  - `Lighting`
  - `DirectLighting`
  - `IndirectLighting`
  - `LightingComposite`
  - `PostProcessing`
  - `Debug`
  - `Presentation`
- Move `VisualizeBuffers` out of `Lighting.cpp` into a single `Debug` family after post-processing.
- Keep sky resolve inside the lighting family after lighting composite.
- Route upscaler evaluation through a post-processing family.
- Do not split debug into HDR/SDR families unless separate color-space debug passes actually exist.

### 2. Indirect Diffuse Has No Ray Tracing Settings Or CVars

There is no `IndirectDiffuseSettings` or `IndirectDiffuseCVars`.

Problem:

- The feature has no explicit control surface yet.
- Debug view output can be mistaken for final lighting, as happened with indirect specular.

Required cleanup:

- Mirror the indirect specular settings/CVar pattern without adding runtime status files.

### 3. Environment Map Binding Is Duplicated

The sky texture lookup and descriptor-cache pattern appears in:

- `SkyPass`
- `IndirectLightingPass`
- `IndirectSpecularPass`

Problem:

- Any texture fallback policy change must be copied.
- Any future ray miss or environment lighting policy change would need to rediscover the same texture source.

Required cleanup:

- Add a renderer-side helper such as `EnvironmentMapPassBinding`.
- Add shader-side `SkyEnvironment.hlsli` for shared equirectangular UV and tone mapping helpers.
- Keep pass-specific sampler names only when the sampler behavior really differs.

### 4. Ray Query Trace Logic Is Not Yet A Reusable Contract

Direct shadows and indirect specular both use ray queries, but the reusable boundary is incomplete:

- `RayTracingMaterialHit.hlsli` contains hit material and alpha-test helpers.
- `IndirectSpecular.hlsl` owns its own ray query loop and trace-result population.
- `RayTracedShadows.hlsli` owns a separate shadow-oriented trace path.

Problem:

- Indirect diffuse needs a third ray query loop.
- Alpha-tested candidate handling and debug rejection reasons can drift.

Required cleanup:

- Extract shared ray-query result structures and alpha-tested candidate trace helpers into a dedicated include after the first diffuse pass proves the exact needs.
- Do not over-generalize before the diffuse ray path exists.

### 5. Hit Data Is Correct Enough To Start, But Not Broad Enough To Declare Complete GI

`RayTracingHitDataFrameData` currently builds per-frame structured buffers for:

- vertices
- indices
- instances
- materials

Known limits:

- skinned meshes are rejected
- alpha-blended materials are rejected
- material texture sampling uses explicit mip 0 in RT hit shaders
- hit-data upload is rebuilt per frame rather than persistently cached
- hit data and BLAS/TLAS acceptance have related but separate rejection paths

Impact on indirect diffuse:

- Static opaque and alpha-tested scenes can work first.
- Animated/skinned and alpha-blended GI must be marked unsupported.
- Noise and aliasing from mip 0 can show up strongly on diffuse bounces.

Required cleanup:

- Keep hit-data rejection categories named in neutral hit-data code when a stage needs them.
- Do not hide unsupported materials by returning plausible-looking black.
- Plan persistent/cached hit data as a later infrastructure stage.

### 6. Current Frame Graph Has Enough Ray Tracing Support, But Variant Policy Is Uneven

Frame graph supports acceleration-structure resources and infers `ResourceState::RayTracingAccelerationStructure`.

Direct lighting has:

- no-ray-query variant
- D3D12 descriptor TLAS variant
- Vulkan address variant

Indirect specular currently uses only descriptor TLAS and gates out otherwise.

Problem:

- Indirect diffuse needs an explicit backend policy up front.
- If the first implementation is descriptor-only, Vulkan address mode must fail closed without plausible-looking output.
- If Vulkan support is required in the first milestone, the shader package and pass variants must be planned before implementation.

Required cleanup:

- Start with descriptor TLAS only if that matches current indirect specular.
- Add Vulkan address parity as a separate stage, not as an accidental omission.

### 6A. Reusable Ray Tracing Architecture Is Not Yet Explicit Enough

The renderer already has promising neutral areas:

- `Engine/Renderer/Private/RayTracing/Scene`
- `Engine/Renderer/Private/RayTracing/Acceleration`
- `Engine/Renderer/Private/RayTracing/Effects`
- `Engine/Renderer/Private/Passes/Bindings`
- `Engine/Assets/Shaders/RayTracing`
- `Engine/Assets/Shaders/Common`

Problem:

- It is still too easy for the next feature to place reusable facts under the first effect that needs them.
- Capability checks can drift if each effect asks backend questions directly.
- Shader helpers can become named after `IndirectDiffuse` even when they really describe ray queries, hit reconstruction, path samples, or sampling PDFs.
- Long-term ray tracing use outside indirect lighting becomes harder if TLAS access, hit data, material table binding, and capability policy are coupled to one pass.

Required cleanup:

- Treat ray tracing as a renderer capability that any pass family can consume.
- Put backend and TLAS access facts behind reusable ray tracing capability/query code.
- Keep reusable descriptor and frame-graph bindings in neutral `Passes/Bindings` or `RayTracing/*` files.
- Keep effect-specific settings, CVars, uniforms, and debug enums under `RayTracing/Effects/<EffectName>`.
- Keep reusable shader code under `Engine/Assets/Shaders/RayTracing`, `Engine/Assets/Shaders/Common`, `Engine/Assets/Shaders/BRDF`, or `Engine/Assets/Shaders/Lighting`.
- Do not create a reusable file whose name is derived from its first caller. If shadows, specular, diffuse, path tracing, or future GI could use it, name it after the underlying concept.

### 7. The Renderer Has No Shared "Path Sample" Vocabulary

Indirect specular has:

- sample direction
- PDF
- throughput
- mirror/stochastic sample mode

Diffuse needs:

- sample direction
- PDF
- cosine term
- miss/hit radiance
- final contribution

Problem:

- Without a shared vocabulary, diffuse and specular debug modes will diverge.
- Implementation prompts become ambiguous when they do not say whether the only active sampler is cosine-weighted hemisphere sampling.

Required cleanup:

- Use the authoritative lighting-target contract in `Docs/Rendering/PBR/01-PBR-Reference-Requirements.md#lighting-target-contract`.
- Define diffuse contribution as material-evaluated outgoing radiance from paths whose first primary event is the diffuse lobe.
- Keep `LightingComposite` as a direct sum; do not multiply base color or diffuse weights there.
- Add a neutral shader-side path-sample vocabulary before debug modes multiply across effects.
- For a general direction PDF `p(w)`, write:
  - `contribution = f_diffuse * diffuseEnergyWeight * Li(w) * saturate(dot(N, w)) / p(w)`
- For cosine hemisphere PDF `p(w) = cosTheta / PI`, this reduces to:
  - `contribution = f_diffuse * diffuseEnergyWeight * Li(w) * PI`

### 8. No-Denoiser Requirement Must Be Explicit

NVIDIA RTXPT, Falcor, RTXDI, and RTXGI all provide paths to denoising, temporal accumulation, resampling, probes, or caches. The user goal explicitly excludes denoising.

Problem:

- Without a written rule, feature work may drift into NRD/SVGF/TAA/reservoir reuse before the base stochastic path is correct.

Required cleanup:

- The shipped path must be raw single-frame stochastic lighting.
- Temporal reuse, spatial reuse, reservoirs, probes, and radiance caches are future features, not stage 1.

### 9. Debug Visibility Must Be Targeted, Not Noisy

Indirect diffuse can look acceptable while being wrong if:

- the output is sky ambient rather than traced
- ray misses are black
- PDF weights are missing
- cosine terms are double-applied
- albedo is applied in the pass and again in composite
- debug view is mistaken for final lighting

Required cleanup:

- Add debug modes for sample direction, PDF, hit/miss mask, hit radiance, final contribution.
- Do not add source-backed validation levels or smoke-command suites for this feature.
- Use existing levels and editor view modes for manual inspection.

Instrumentation rule:

- Do not add scattered logs, per-frame validation spam, or new ad hoc diagnostics panels.
- Keep runtime instrumentation limited to explicit debug view modes.
- Prefer deterministic debug views over additional logging.
- Do not add counters unless they directly drive a visible debug view or implementation decision.

## Recommended Architectural DirectionStage 2 - Environment Map Binding Helper
Implementation Prompt
Extract sky/environment descriptor binding into a reusable renderer helper and shared HLSL include. Update SkyPass, IndirectLightingPass, and IndirectSpecularPass to use the helper.

Files
Create:

Engine/Renderer/Private/Passes/Bindings/EnvironmentMapPassBinding.h
Engine/Renderer/Private/Passes/Bindings/EnvironmentMapPassBinding.cpp
Engine/Assets/Shaders/Lighting/SkyEnvironment.hlsli
Modify:

SkyPass.*
IndirectLightingPass.*
IndirectSpecularPass.*
Sky.hlsl
IndirectLighting.hlsl
IndirectSpecular.hlsl
Required Behavior
CPU helper resolves TextureId::SkyCubemap, falling back to TextureId::Checker.
CPU helper owns descriptor binding set caching.
CPU helper returns empty binding if no texture manager or fallback texture exists.
CPU helper is named as an environment map binding helper, not a sky-pass helper, because ray misses, ambient fallback, and sky resolve all consume the same environment source.
HLSL helper exposes:
float2 ComputeSkyEnvironmentUv(float3 worldDirection)
float3 ToneMapSkyEnvironment(float3 skyRadiance)
float3 SampleSkyEnvironment(Texture2D texture, SamplerState sampler, float3 worldDirection)
Existing visible sky and indirect specular miss colors remain visually consistent.
Acceptance
No duplicated ResolveSkyTexture(...) function remains in sky, indirect lighting, or indirect specular pass implementation files.
The sky miss fix from indirect specular remains active.

The feature should introduce a new ray-traced diffuse effect under:

- `Engine/Renderer/Private/RayTracing/Effects/IndirectDiffuse`
- `Engine/Assets/Shaders/Passes/Deferred/IndirectDiffuse.hlsl`
- `Engine/Renderer/Private/Passes/Deferred/IndirectDiffusePass.*`
- `Engine/Renderer/ShaderRegistrations/IndirectDiffuseShaders.cpp`

The old `IndirectLighting` pass should become an ambient fallback, not the long-term owner of indirect diffuse.

The frame composition should converge on this shape:

```text
BuildFrame
  Scene target creation
  GBuffer
    GBuffer target creation
    GBuffer raster pass
  RayTracingInfrastructure
    Scene TLAS resource reservation
    PTLAS logical update pass
    PTLAS native operation pack pass
    TLAS build pass
  Lighting
    Lighting target creation
    Lighting target clear
    DirectLighting
      direct lighting variants
      ray-traced shadow variants
    IndirectLighting
      ambient fallback
      ray-traced indirect diffuse
      ray-traced indirect specular
      future indirect subsurface
    LightingComposite
  PostProcessing
    sky resolve
    external upscaler provider
    future tone mapping / exposure / color grading
  Debug
    visualized buffer output
  Presentation
    present scene
```

Composition helpers own ordering. Leaf pass classes own pass resources and execution only.

The ray tracing architecture should converge on this ownership split:

```text
RayTracing
  Capability/query facts
  Scene/TLAS frame resources
  Acceleration build strategies
  Hit-data buffers and rejection vocabulary
  Effects
    Shadows
    IndirectSpecular
    IndirectDiffuse
Passes
  Bindings
    Ray tracing scene/TLAS binding
    Ray tracing hit-data binding
    Material texture table binding
    Environment map binding
Shaders
  Common
    random numbers
    hemisphere/cone/disk/sphere sampling
  RayTracing
    trace query helpers
    hit surface reconstruction
    hit lighting
    hit/debug reason colors
    path sample structs
  Lighting
    sky/environment sampling
  Passes/Deferred
    effect orchestration shaders only
```

In this split, `IndirectDiffuse` is a consumer of ray tracing architecture, not the owner of ray tracing architecture.

Acceptable naming options:

1. Keep `IndirectLighting` as fallback and add `IndirectDiffuseRayTracing`.
2. Rename current `IndirectLighting` to `AmbientIndirectLighting` and add `IndirectDiffuse`.

Recommendation:

- Use option 2 if the rename can be done in one clean mechanical stage.
- Use option 1 if preserving current cooked package names is more important during early development.

## Must Not Do

- Do not integrate NRD, SVGF, DLSS-RR, ReSTIR GI, ReSTIR PT, RTXGI DDGI, NRC, or SHaRC in the first indirect diffuse milestone.
- Do not add a hidden temporal accumulator and call it "not a denoiser".
- Do not multiply base color in `LightingComposite`; `IndirectDiffuse` is already a material-evaluated radiance contribution.
- Do not add diffuse ray tracing with scattered runtime instrumentation, noisy per-frame logs, or validation plumbing outside the existing CVar/debug-view pattern.
- Do not make a lighting producer pass write unrelated lighting targets to zero.
- Do not add more unrelated work to `Lighting.cpp`; create or use a family helper instead.
- Do not split debug into HDR/SDR families without a concrete color-space need.
- Do not add a shader package whose required feature flags are weaker than its actual resource use.
- Do not hide unsupported TLAS access modes behind black output.
- Do not put reusable ray tracing capability, binding, hit-data, sampling, or shader helper code under an effect-specific `IndirectDiffuse` path.
- Do not name reusable shader helpers after the first pass that uses them.
- Do not branch directly on backend API in an effect pass when a reusable capability or TLAS access query can answer the same question.

## Main Challenges

The technical challenge is not tracing one diffuse ray. SparkleEngine already has most of that machinery.

The real challenges are:

1. preserving lighting-target ownership
2. defining the diffuse estimator exactly
3. avoiding denoiser/resampling scope creep
4. extracting shared ray tracing binding and shader helpers at the right time
5. keeping backend capability failures diagnosable
6. building a real pass-family hierarchy before indirect lighting grows further
7. validating a noisy raw signal without relying on screenshots alone

The implementation plan should therefore be staged around correctness and contracts first, visual quality second, and performance/infrastructure refinements third.

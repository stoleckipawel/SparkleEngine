# PBR Implementation Action Plan

Date: 2026-06-29

This document is a staged implementation prompt plan. It starts from `Docs/Rendering/PBR/02-Sparkle-PBR-State-Audit.md` and aims at the requirements in `Docs/Rendering/PBR/01-PBR-Reference-Requirements.md`.

Scope:

- Direct lighting
- Indirect diffuse/specular ray-traced lighting
- Distant sky texture fallback
- NVIDIA denoiser/provider readiness

Out of scope for this plan:

- Volumetric lighting
- ReSTIR
- DDGI/probes
- Neural radiance caches
- Full material model expansion beyond the current metallic-roughness surface

Every stage has two equal acceptance surfaces:

- physical correctness
- code structure and pass architecture

A stage is not complete if the output looks correct but the implementation makes the next lighting problem harder to solve.

## Stage 0: Lock the Lighting Contract

Implementation prompt:

Document and assert that direct and indirect lighting targets store material-evaluated outgoing radiance contributions in linear HDR. Add comments near target declarations and composite code. Update or supersede older docs that describe `IndirectDiffuse` as raw irradiance.

Files:

- `Engine/Assets/Shaders/Passes/Deferred/LightingComposite.hlsl`
- `Engine/Renderer/Private/Frame/Lighting/LightingRenderTargets.cpp`
- Existing architecture docs that still describe old target semantics.

Acceptance criteria:

- `DirectDiffuse`, `DirectSpecular`, `IndirectDiffuse`, and `IndirectSpecular` have a written semantic contract.
- No doc claims `IndirectDiffuse` is raw irradiance unless the code is changed to match.
- `LightingComposite` remains a simple sum only because all inputs share the same radiance-contribution unit.

## Stage 0A: Lock Shader Module Boundaries

Implementation prompt:

Before changing lighting math, move reusable lighting concepts out of pass-specific shader folders. Do this as behavior-preserving refactors with shader-cook validation after each step.

Target dependency direction:

```text
Common -> Geometry -> Material -> BRDF -> Lighting -> RayTracing -> Passes
```

Files to create or move toward:

- `Engine/Assets/Shaders/Lighting/PunctualLights.hlsli`
- `Engine/Assets/Shaders/Lighting/SurfaceLighting.hlsli`
- `Engine/Assets/Shaders/Lighting/Visibility.hlsli`
- `Engine/Assets/Shaders/RayTracing/PathSurface.hlsli`
- `Engine/Assets/Shaders/RayTracing/PathSampling.hlsli`
- `Engine/Assets/Shaders/RayTracing/PathLighting.hlsli`
- `Engine/Assets/Shaders/RayTracing/Shadows/RayTracedShadowSignals.hlsli`
- `Engine/Assets/Shaders/RayTracing/Shadows/RayTracedShadowSampling.hlsli`
- `Engine/Assets/Shaders/RayTracing/Shadows/RayTracedShadowTrace.hlsli`
- `Engine/Assets/Shaders/RayTracing/Shadows/RayTracedShadowDenoiserInputs.hlsli`

Move candidates:

- light direction/falloff/cone helpers from `DirectLightingCommon.hlsli` to `Lighting/PunctualLights.hlsli`
- direct surface lobe evaluation from `DirectLightingCommon.hlsli` to `Lighting/SurfaceLighting.hlsli`
- shadow signal/sampling helpers from `Passes/Deferred` to `RayTracing/Shadows`
- duplicated indirect surface records into `RayTracing/PathSurface.hlsli`

Acceptance criteria:

- No file under `Engine/Assets/Shaders/RayTracing` includes `Passes/Deferred/*`.
- Generic lighting helpers do not live under `Passes/Deferred`.
- Deferred pass entrypoint files continue compiling and producing identical output after each move.
- `DirectLighting.hlsl`, `IndirectDiffuse.hlsl`, and `IndirectSpecular.hlsl` become thinner, not larger.
- The shader include graph has no cyclic conceptual dependency.

## Stage 0B: Make Pass Entrypoints Thin

Implementation prompt:

Refactor direct and indirect pass entrypoints so they own pass IO and output policy only. Sampling, light evaluation, visibility tracing, hit/miss resolve, and path throughput should live in reusable modules.

Target pass entrypoint shape:

```text
main:
    validate dispatch bounds
    load GBuffer / constants
    early out sky pixels
    build primary surface
    call one reusable lighting/path function
    choose debug or production output
    write target
```

Refactor targets:

- `Engine/Assets/Shaders/Passes/Deferred/DirectLighting.hlsl`
- `Engine/Assets/Shaders/Passes/Deferred/DirectLightingCommon.hlsli`
- `Engine/Assets/Shaders/Passes/Deferred/IndirectDiffuse.hlsl`
- `Engine/Assets/Shaders/Passes/Deferred/IndirectSpecular.hlsl`

Acceptance criteria:

- Pass entrypoint files do not define reusable BRDF, light falloff, ray-origin, lobe sampling, or path-throughput algorithms.
- Shared algorithms are named after concepts, not effects.
- Debug helpers stay pass-specific only when the debug view is effect-specific.
- Line count is reduced for `IndirectDiffuse.hlsl` and `IndirectSpecular.hlsl` by moving cohesive logic, not by hiding unrelated code in one new god include.
- New files each have one reason to change.

## Stage 1: Fix Linear HDR Sky Transport

Implementation prompt:

Split sky sampling into two functions: one that returns linear HDR radiance and one optional display transform for presentation/debug only. Use linear sky radiance for sky pixels, indirect ray misses, mirror reflections, and any future environment sampling. Remove pre-presentation tone mapping from `SampleSkyEnvironment`.

Files:

- `Engine/Assets/Shaders/Lighting/SkyEnvironment.hlsli`
- `Engine/Assets/Shaders/Passes/Deferred/Sky.hlsl`
- `Engine/Assets/Shaders/Passes/Deferred/IndirectDiffuse.hlsl`
- `Engine/Assets/Shaders/Passes/Deferred/IndirectSpecular.hlsl`
- `Engine/Assets/Shaders/Passes/Presentation/PresentScene.hlsl`

Expected shader shape:

```hlsl
float3 SampleSkyEnvironmentRadiance(Texture2D environmentTexture, SamplerState environmentSampler, float3 worldDirection)
{
    return max(environmentTexture.SampleLevel(environmentSampler, ComputeSkyEnvironmentUv(worldDirection), 0.0f).rgb, 0.0f.xxx);
}
```

Acceptance criteria:

- Sky pass writes linear HDR sky radiance to scene color.
- Presentation still tone maps final scene color exactly once.
- Indirect diffuse/specular miss rays use linear HDR sky radiance.
- Mirror-sky test: a perfect mirror reflection of the sky matches the sky pixel before presentation.
- Bright EXR sky values above 1.0 survive in scene color before presentation.

## Stage 2: Unify Primary and Ray-Hit Material Data

Implementation prompt:

Make deferred primary shading and ray-hit shading use the same dielectric F0 value and roughness convention. Add F0/reflectance to the GBuffer or derive it from a documented material constant that is available in direct lighting. Avoid using fixed `0.04` for primary surfaces when material F0 exists.

Files:

- `Engine/Assets/Shaders/Material/Material.hlsli`
- `Engine/Assets/Shaders/Passes/Deferred/GBufferPS.hlsl`
- `Engine/Assets/Shaders/Passes/Deferred/GBufferUtils.hlsli`
- `Engine/Assets/Shaders/Passes/Deferred/DirectLightingCommon.hlsli`
- `Engine/Assets/Shaders/RayTracing/RayTracingMaterialHit.hlsli`
- `Engine/Renderer/Private/Frame/Deferred/GBufferFormats.h`

Acceptance criteria:

- Direct primary lighting and ray-hit direct lighting use the same F0 for the same material.
- A dielectric material with non-default reflectance visibly affects primary and reflected lighting consistently.
- Roughness floor policy is documented and identical across direct, indirect diffuse throughput, indirect specular throughput, and ray-hit direct lighting.
- GBuffer format changes, if any, are reflected in visualization modes and shader binding metadata.

## Stage 3: Normalize BRDF Energy Behavior

Implementation prompt:

Create a small shared BRDF validation harness or debug shader mode for furnace tests. Use it to verify the current GGX/Smith/Schlick implementation, then decide whether the reference mode uses Lambert diffuse or Burley diffuse. Mark subsurface as non-reference until energy compensation is implemented.

Files:

- `Engine/Assets/Shaders/BRDF/*.hlsli`
- `Engine/Assets/Shaders/Passes/Deferred/DirectLightingCommon.hlsli`
- `Engine/Assets/Shaders/RayTracing/RayTracingHitLighting.hlsli`
- Test or validation docs under `Docs/Architecture/03-Validation` or `Docs/Rendering/PBR`.

Acceptance criteria:

- White-furnace tests pass for diffuse white, dielectric mixed, and metallic surfaces.
- No material returns NaN or infinity at grazing NoV/NoL.
- The meaning of `Geometry::EvaluateDirect` is documented as visibility `V` or raw geometry `G`.
- Subsurface contribution is either disabled in reference mode or energy compensated against diffuse.

## Stage 4: Fix Punctual Light Units and Falloff

Implementation prompt:

Define engine units for directional, point, and spot lights. Align glTF-imported point/spot intensity with candela and directional intensity with lux or a documented calibrated engine unit. Replace current range and cone attenuation with a glTF/Filament-compatible punctual falloff policy, or document a deliberate engine policy and convert imports into that policy.

Files:

- `Tools/Import/SourceImporters/Private/Gltf/GltfLightImporter.cpp`
- `Engine/GameFramework/Private/Scene/Lighting/Snapshots/SceneLightingSnapshotBuilder.cpp`
- `Engine/Assets/Shaders/Passes/Deferred/DirectLightingCommon.hlsli`
- `Engine/Assets/Shaders/RayTracing/RayTracingHitLighting.hlsli`
- `Engine/RHI/Public/Resources/RenderViewLightingData.h`

Recommended shader policy:

```hlsl
float DistanceFalloff(float distanceToLight, float range)
{
    float d2 = max(distanceToLight * distanceToLight, 1.0e-4f);
    if (range <= 0.0f)
    {
        return rcp(d2);
    }

    float x = distanceToLight / max(range, 1.0e-4f);
    float smooth = saturate(1.0f - x * x * x * x);
    return smooth * smooth * rcp(d2);
}

float SpotFalloff(float cosTheta, float innerCos, float outerCos)
{
    float t = saturate((cosTheta - outerCos) / max(innerCos - outerCos, 1.0e-4f));
    return t * t;
}
```

Acceptance criteria:

- Point and spot light falloff matches the selected unit policy.
- glTF `KHR_lights_punctual` imports do not need arbitrary intensity compensation.
- Primary direct and secondary hit direct use the same falloff and cone functions.
- A light calibration scene records expected luminance at known distances.

## Stage 5: Make Direct Shadows Physically Usable

Implementation prompt:

Route direct shadow rays through the same alpha-tested candidate handling used by indirect ray queries, or add an equivalent shadow-specific alpha-test resolver. Then split raw stochastic visibility from direct lighting accumulation so NRD SIGMA can consume it later.

Files:

- `Engine/Assets/Shaders/Passes/Deferred/RayTracedShadows.hlsli`
- `Engine/Assets/Shaders/RayTracing/RayTracingTraceQuery.hlsli`
- `Engine/Assets/Shaders/RayTracing/RayTracingMaterialHit.hlsli`
- `Engine/Assets/Shaders/Passes/Deferred/RayTracedShadowDenoiserInputs.hlsli`
- `Engine/Renderer/Private/FrameGraph/Resources/FrameGraphDenoiserRegistration.cpp`
- `Engine/Renderer/Private/RayTracing/Effects/Shadows/*`

Acceptance criteria:

- Alpha-tested foliage/card geometry casts cutout direct shadows.
- Hard shadows and one-sample soft shadows still work on descriptor TLAS and Vulkan address paths.
- Raw visibility and hit-distance targets can be written independently from lighting contribution.
- Existing direct-light visual output remains unchanged when denoiser mode is off.

## Stage 6: Add Shadow Denoiser Integration Point

Implementation prompt:

Implement the renderer-side pass boundary for NRD SIGMA without forcing all platforms to have NRD. The pass should consume raw visibility, normal, depth, motion vectors, and history, then output denoised visibility. Direct lighting should be able to consume either raw or denoised visibility through an explicit mode.

Files:

- Provider-neutral denoiser interface under renderer provider architecture.
- `FrameGraphDenoiserRegistration`
- Shadow settings and pass data.
- Direct lighting pass resource declarations.

Acceptance criteria:

- `r.RayTracedShadows.Denoiser=0` uses raw visibility.
- `r.RayTracedShadows.Denoiser=1` requests SIGMA if available and falls back with diagnostics if unavailable.
- History resources are persistent and reset on camera cut, resize, and feature toggles.
- Denoised visibility is inspectable in a debug view.

## Stage 7: Replace Split Bounce Logic With Unified BSDF Path Sampling

Implementation prompt:

Create a shared ray-traced surface path sampling module that can sample diffuse or specular lobes from one material using a single lobe-selection PDF. Use it for both indirect diffuse and indirect specular passes. Preserve current output split by assigning the path contribution to the buffer for the primary sampled lobe.

Files:

- New shared HLSL include under `Engine/Assets/Shaders/RayTracing` or `Engine/Assets/Shaders/Lighting`
- `Engine/Assets/Shaders/Passes/Deferred/IndirectDiffuse.hlsl`
- `Engine/Assets/Shaders/Passes/Deferred/IndirectSpecular.hlsl`
- `Engine/Assets/Shaders/BRDF/*.hlsli`
- `Engine/Assets/Shaders/RayTracing/RayTracingPathSample.hlsli`
- `Engine/Assets/Shaders/RayTracing/PathSurface.hlsli`
- `Engine/Assets/Shaders/RayTracing/PathSampling.hlsli`
- `Engine/Assets/Shaders/RayTracing/PathLighting.hlsli`

Target algorithm:

```text
surface = primary
throughput = 1
primaryLobe = none

for bounce:
    sample = SampleBSDF(surface, outgoingDirection, rng)
    if sample.invalid: break
    if bounce == 0: primaryLobe = sample.lobe

    throughput *= sample.f * sample.cosTheta / sample.pdf
    trace ray

    if miss:
        AddToPrimaryLobe(primaryLobe, throughput * SkyRadiance(ray.direction))
        break

    hit = ReconstructSurface(ray)
    AddToPrimaryLobe(primaryLobe, throughput * DirectLightingAtHit(hit, -ray.direction))
    AddToPrimaryLobe(primaryLobe, throughput * EmissiveAtHit(hit, -ray.direction))

    maybe russian roulette
    surface = hit
```

Acceptance criteria:

- Single-bounce output matches the previous diffuse/specular passes within expected sampling variance.
- Multi-bounce paths can include diffuse-after-specular and specular-after-diffuse.
- Debug views expose sampled lobe, PDF, throughput, hit distance, hit normal, and rejection reason.
- Reference mode can run more than one bounce without needing non-physical intensity multipliers.
- `IndirectDiffuse.hlsl` and `IndirectSpecular.hlsl` both call the same BSDF/path sampling code.
- No new path sampling code is named after only diffuse or only specular unless it truly supports only that lobe.

## Stage 8: Add Secondary Shadowing and Direct-Light Sampling at Hits

Implementation prompt:

Make `ShadeRayTracingHitIncidentRadiance` optionally shadow secondary-hit direct lights. Start with one shadow ray to the sampled/selected light direction, then add explicit light sampling if the path sampler supports next-event estimation with PDFs.

Files:

- `Engine/Assets/Shaders/RayTracing/RayTracingHitLighting.hlsli`
- `Engine/Assets/Shaders/Passes/Deferred/RayTracedShadows.hlsli`
- Shared light sampling include if introduced.

Acceptance criteria:

- Indirect lighting darkens correctly behind occluders at secondary hit points.
- Secondary shadowing can be disabled for performance with a visible debug flag.
- Shadow alpha-test behavior matches primary direct shadows.
- No double-counting of primary direct lighting occurs.

## Stage 9: Add Environment Importance Sampling

Implementation prompt:

Build an environment-map luminance distribution for the sky texture and sample it for indirect lighting. This is basic environment importance sampling, not ReSTIR. Combine with BSDF sampling using MIS when both strategies are active.

Files:

- Sky/environment texture manager or preprocess stage.
- New environment sampling data buffers.
- Indirect diffuse/specular shared path sampling code.

Acceptance criteria:

- HDR sky with small bright sun converges faster than cosine-only sampling.
- MIS weights are debug-visible.
- Energy matches brute-force high-sample cosine sampling.
- Miss rays still use the same `SampleSkyEnvironmentRadiance` function.

## Stage 10: Add Emissive Geometry Sampling

Implementation prompt:

Keep emissive-on-hit support, but add optional explicit emissive triangle/light sampling for high-variance emissive scenes. This can remain behind a reference or high-quality flag until acceleration structures for emitters are mature.

Files:

- Ray tracing hit data or scene light extraction.
- Shared light sampling include.
- Indirect path sampling code.

Acceptance criteria:

- An emissive panel lights nearby diffuse surfaces without requiring only random ray hits.
- High-sample reference matches hit-only estimation.
- Emissive geometry does not get double counted when directly visible to a path ray.

## Stage 11: Prepare DLRR or Other Indirect Reconstruction Inputs

Implementation prompt:

Extend the provider-neutral reconstruction contract for indirect lighting. Do not wire DLRR directly into shader code. First expose the resources and metadata a provider needs.

Required resources:

- noisy composed color or noisy indirect color at the provider-required stage
- depth and depth convention
- motion vectors and units
- normals
- roughness
- base color/diffuse albedo
- metallic/F0/specular albedo
- hit distance for specular and/or indirect rays
- exposure or auto-exposure state
- reset/history state

Files:

- `Engine/Renderer/Private/Upscaling/UpscalerInputContract.h`
- Provider contract docs
- Render product registration
- Indirect pass output resources
- Future Streamline DLRR provider implementation

Acceptance criteria:

- DLSS Super Resolution path still works unchanged.
- A DLRR-capable provider can be selected only when all required resources are available.
- Missing resources produce diagnostics, not silent fallback to an invalid reconstruction path.
- Raw indirect debug views remain available when reconstruction is enabled.

## Stage 12: Add a High-Sample Reference Mode

Implementation prompt:

Add a reference path tracing mode or offline accumulation path that uses the same material, light, sky, and alpha-test code as the real-time passes. The reference mode can be slow. It exists to define truth.

Files:

- New path tracing shader/pass, or a reference mode in existing indirect path infrastructure.
- Capture tooling for HDR/EXR or float target dumps.
- Validation docs.

Acceptance criteria:

- Captures can accumulate many samples per pixel deterministically.
- Same scene can output real-time direct/indirect and reference path-traced buffers.
- Difference images can be produced for direct, indirect diffuse, indirect specular, and composed scene color.
- The validation scenes from `01-PBR-Reference-Requirements.md` have expected captures.

## Recommended Implementation Order

Do these first:

1. Stage 0A: shader module boundaries.
2. Stage 0B: thin pass entrypoints.
3. Stage 1: linear HDR sky transport.
4. Stage 2: material F0 and roughness consistency.
5. Stage 4: light units/falloff.
6. Stage 5: alpha-tested shadows and raw visibility split.
7. Stage 7: unified BSDF path sampler.

Then:

8. Stage 8: secondary shadowing.
9. Stage 9: environment importance sampling.
10. Stage 6: NRD SIGMA shadow integration.
11. Stage 11: DLRR resource contract.
12. Stage 12: high-sample reference mode.

Stage 3 validation should run throughout. It is the guardrail, not a one-time cleanup.

## Definition of Done for "PBR Correct Enough to Build On"

The renderer is on a credible ground-truth path when all of these are true:

- Sky and lighting are linear HDR until presentation.
- Direct primary and ray-hit direct use the same BRDF, material F0, roughness, and light units.
- Punctual light falloff and spot cones are documented and calibrated.
- Alpha-tested geometry participates in direct shadows and indirect ray hits.
- Indirect throughput is always `BSDF * cosine / pdf`.
- Multi-bounce indirect can sample both diffuse and specular lobes.
- Secondary hit direct lighting can be shadowed.
- Raw denoiser inputs exist separately from final lighting targets.
- Validation scenes can compare real-time output against a high-sample reference.
- Reusable lighting/ray-tracing shader modules do not depend on pass-specific deferred files.
- Pass entrypoints are small orchestration layers over concept-named modules.
- New lighting features reuse shared light, surface, path, visibility, and sky code instead of forking effect-local copies.

# Sparkle PBR State Audit

Date: 2026-06-29

This document maps the current SparkleEngine PBR implementation against `Docs/Rendering/PBR/01-PBR-Reference-Requirements.md`.

The worktree already has modified lighting shaders at the time of this audit:

- `Engine/Assets/Shaders/Passes/Deferred/DirectLighting.hlsl`
- `Engine/Assets/Shaders/Passes/Deferred/DirectLightingCommon.hlsli`
- `Engine/Assets/Shaders/Passes/Deferred/IndirectDiffuse.hlsl`
- `Engine/Assets/Shaders/Passes/Deferred/IndirectSpecular.hlsl`
- `Engine/Assets/Shaders/Passes/Deferred/IndirectSpecularDebug.hlsli`
- `Engine/Assets/Shaders/Passes/Deferred/LightingComposite.hlsl`

This audit treats those files as the current starting point.

## Current Frame Shape

The lighting frame sequence is clear and usable:

1. `AddLightingTargetClearPass`
2. `AddDirectLightingPass`
3. `AddIndirectLightingPasses`
4. `AddLightingCompositePass`
5. `AddSkyPass`

Relevant code:

- `Engine/Renderer/Private/Frame/Lighting/Lighting.cpp:15` clears lighting targets.
- `Engine/Renderer/Private/Frame/Lighting/Lighting.cpp:16` adds direct lighting.
- `Engine/Renderer/Private/Frame/Lighting/Lighting.cpp:17` adds indirect diffuse and specular.
- `Engine/Renderer/Private/Frame/Lighting/Lighting.cpp:18` composites lighting.
- `Engine/Renderer/Private/Frame/Lighting/Lighting.cpp:19` fills sky pixels.

Lighting targets are created as `FrameRenderFormats::SceneColor`, currently `R32G32B32A32_Float`:

- `Engine/Renderer/Private/Frame/Core/FrameRenderFormats.h:7`
- `Engine/Renderer/Private/Frame/Lighting/LightingRenderTargets.cpp:25`
- `Engine/Renderer/Private/Frame/Lighting/LightingRenderTargets.cpp:26`

This is good for HDR transport.

## Current Target Semantics

`LightingComposite` simply adds all lighting target values and emissive:

- `Engine/Assets/Shaders/Passes/Deferred/LightingComposite.hlsl:34`
- `Engine/Assets/Shaders/Passes/Deferred/LightingComposite.hlsl:39`
- `Engine/Assets/Shaders/Passes/Deferred/LightingComposite.hlsl:44`
- `Engine/Assets/Shaders/Passes/Deferred/LightingComposite.hlsl:66`

Current effective semantics:

```text
DirectDiffuse = material-evaluated direct diffuse contribution
DirectSpecular = material-evaluated direct specular contribution
DirectSubsurface = material-evaluated direct subsurface contribution
IndirectDiffuse = material-evaluated indirect contribution from the diffuse first event
IndirectSpecular = material-evaluated indirect contribution from the specular first event
IndirectSubsurface = currently cleared, no active writer found in this pass family
SceneColor = sum(all lighting targets) + GBuffer emissive
```

This is internally consistent with the current composite. It conflicts with older architecture notes that described indirect diffuse as irradiance before base-color multiplication. The current implementation should either keep the material-evaluated convention or deliberately migrate, but it should not mix both.

## BRDF Library

Current BRDF pieces:

- Shading data: `Engine/Assets/Shaders/BRDF/ShadingData.hlsli:5` and `:19`
- GGX distribution: `Engine/Assets/Shaders/BRDF/Distribution.hlsli`
- Smith height-correlated visibility: `Engine/Assets/Shaders/BRDF/Geometry.hlsli:39`
- Schlick Fresnel: `Engine/Assets/Shaders/BRDF/Fresnel.hlsli`
- Burley diffuse default: `Engine/Assets/Shaders/BRDF/Config.hlsli`
- Direct BRDF split: `Engine/Assets/Shaders/BRDF/BRDF.hlsli:28` through `:33`
- Specular visibility convention: `Engine/Assets/Shaders/BRDF/Specular.hlsli:14` through `:32`

Positive:

- The implementation is a recognizable modern metallic-roughness BRDF.
- The correlated Smith branch appears to return the visibility form `G / (4 NoL NoV)` and `Specular::CookTorrance` correctly avoids dividing again for that branch.
- Direct primary lighting and secondary hit direct lighting both call `BRDF::Direct::Evaluate`.

Risks:

- The visibility convention is implicit. If another caller treats `Geometry::EvaluateDirect` as raw `G`, specular energy will be wrong.
- Burley diffuse is the default, but there is no visible white-furnace validation proving the current diffuse plus specular energy behavior.
- `BRDF::Direct::Evaluate` adds subsurface separately from diffuse. If subsurface remains active, this can exceed energy unless diffuse is reduced.

## Material and GBuffer State

Material sampling computes dielectric F0:

- `Engine/Assets/Shaders/Material/Material.hlsli:34`
- `Engine/Assets/Shaders/Material/Material.hlsli:74`
- `Engine/Assets/Shaders/Material/Material.hlsli:150`
- `Engine/Assets/Shaders/Material/Material.hlsli:201`

The GBuffer stores base color, normal, metallic, roughness, AO, alpha mode, emissive, subsurface, and device Z:

- `Engine/Assets/Shaders/Passes/Deferred/GBufferPS.hlsl:30`
- `Engine/Assets/Shaders/Passes/Deferred/GBufferPS.hlsl:32`
- `Engine/Assets/Shaders/Passes/Deferred/GBufferPS.hlsl:37`

But GBuffer does not store dielectric F0. Primary deferred direct lighting hardcodes dielectric F0 to `0.04`:

- `Engine/Assets/Shaders/Passes/Deferred/DirectLightingCommon.hlsli:73`

Ray-hit reconstruction does have material F0:

- `Engine/Assets/Shaders/RayTracing/RayTracingMaterialHit.hlsli:40`
- `Engine/Assets/Shaders/RayTracing/RayTracingMaterialHit.hlsli:389`

Issue:

- Primary and secondary material shading can disagree for dielectric reflectance. This violates PBR-R-002.

Texture color processing looks mostly correct:

- `TextureCookRequestBuilder` maps diffuse and emissive to sRGB, and roughness/metallic to data-linear textures.
- `Tools/Cooking/MaterialCooker/Private/TextureCookRequestBuilder.cpp:48`
- `Tools/Cooking/MaterialCooker/Private/TextureCookRequestBuilder.cpp:52`
- `Tools/Cooking/MaterialCooker/Private/TextureCookRequestBuilder.cpp:59`
- `Tools/Cooking/MaterialCooker/Private/TextureCookRequestBuilder.cpp:100`
- `Tools/Cooking/TextureCooker/Private/Pipeline/Stages/DecodeStage.cpp:46`

Open question:

- HDR sky texture import should be checked separately from material texture import. The sky must remain linear HDR.

## Direct Lighting State

Direct lighting shader:

- Loops directional, point, and spot lights in `DirectLighting.hlsl`.
- Writes separate `DirectDiffuse`, `DirectSpecular`, and `DirectSubsurface`.
- Uses ray-query shadows when supported, or unshadowed signals in the no-ray-query variant.

Relevant code:

- `Engine/Assets/Shaders/Passes/Deferred/DirectLightingCommon.hlsli:47`
- `Engine/Assets/Shaders/Passes/Deferred/DirectLightingCommon.hlsli:73`
- `Engine/Assets/Shaders/Passes/Deferred/DirectLightingCommon.hlsli:86`
- `Engine/Assets/Shaders/Passes/Deferred/DirectLightingCommon.hlsli:87`

Distance and cone attenuation:

- `Engine/Assets/Shaders/Passes/Deferred/DirectLightingCommon.hlsli:28`
- `Engine/Assets/Shaders/Passes/Deferred/DirectLightingCommon.hlsli:40`
- `Engine/Assets/Shaders/Passes/Deferred/DirectLightingCommon.hlsli:144`
- `Engine/Assets/Shaders/Passes/Deferred/DirectLightingCommon.hlsli:182`
- `Engine/Assets/Shaders/Passes/Deferred/DirectLightingCommon.hlsli:183`

Issues:

- Local light attenuation is `1 / distance^2 * (1 - distance / range)^2`. This is not the glTF punctual range falloff and dims much more aggressively across the range.
- Spot cone attenuation is linear in the cone ramp. glTF-compatible punctual lights normally square the cone attenuation.
- Source radius affects stochastic shadow sampling but not direct-light radiometry. This is a punctual-light-with-soft-shadow approximation, not a physically sampled area light.
- Direct primary roughness is clamped to `0.04`, while indirect specular allows much smaller roughness. This makes mirrors differ between direct and reflected paths.

## Shadow State

Direct shadows:

- `RayTracedShadows.hlsli` uses inline ray queries.
- Hard and one-sample soft area modes exist.
- Shadow settings contain an NRD SIGMA enum path.

Relevant code:

- `Engine/Assets/Shaders/Passes/Deferred/RayTracedShadows.hlsli:65`
- `Engine/Assets/Shaders/Passes/Deferred/RayTracedShadows.hlsli:98`
- `Engine/Assets/Shaders/Passes/Deferred/RayTracedShadows.hlsli:124`
- `Engine/Assets/Shaders/Passes/Deferred/RayTracedShadows.hlsli:166`
- `Engine/Renderer/Private/RayTracing/Effects/Shadows/RayTracedShadowSettings.h:21`
- `Engine/Renderer/Private/RayTracing/Effects/Shadows/RayTracedShadowCVars.cpp:8`

Issues:

- Direct shadow rays do not appear to run the alpha-tested material resolution path used by indirect rays. Alpha-tested geometry can cast opaque direct shadows.
- There is a `RayTracedShadowDenoiserInputs::PackShadowSignal` helper, but the direct lighting pass currently consumes visibility internally rather than writing raw visibility and denoised visibility resources.
- SIGMA is represented in settings/resources, but there is no complete NRD integration visible in this pass path.
- One-sample soft shadows use sampled light positions/directions for visibility, but direct BRDF/radiance is still evaluated with the original light direction. This is an approximation, not area-light integration.

## Indirect Diffuse State

Current `IndirectDiffuse.hlsl` is no longer only a sky ambient fallback. It traces ray-query paths:

- cosine-hemisphere direction sampling: `Engine/Assets/Shaders/Passes/Deferred/IndirectDiffuse.hlsl:57`
- throughput evaluation: `Engine/Assets/Shaders/Passes/Deferred/IndirectDiffuse.hlsl:108`
- ray-hit direct lighting: `Engine/Assets/Shaders/Passes/Deferred/IndirectDiffuse.hlsl:191`
- sky miss lighting: `Engine/Assets/Shaders/Passes/Deferred/IndirectDiffuse.hlsl:198`
- multi-bounce loop: `Engine/Assets/Shaders/Passes/Deferred/IndirectDiffuse.hlsl:228`
- contribution accumulation: `Engine/Assets/Shaders/Passes/Deferred/IndirectDiffuse.hlsl:246`
- intensity multiplier: `Engine/Assets/Shaders/Passes/Deferred/IndirectDiffuse.hlsl:297`

Positive:

- Throughput uses `diffuseBrdf * cosine / pdf`, which is the correct Monte Carlo shape.
- The pass can include direct lighting at secondary hits through `ShadeRayTracingHitIncidentRadiance`.
- It handles sky miss contribution.

Issues:

- It samples only diffuse-lobe continuation. Multi-bounce paths with specular-after-diffuse are missing until a unified BSDF sampler exists.
- It uses sky radiance after `SampleSkyEnvironment`, which currently tone maps the sky. That makes indirect diffuse non-linear and under-energized.
- It applies an arbitrary `IndirectDiffuseIntensity` multiplier. This is useful as a debug/art knob, but reference mode must default to `1.0` and validation must fail if it is used to compensate physical errors.
- It depends on hit-data and material texture table availability. If unavailable, the pass silently returns after clear. That is fine for fallback behavior, but reference captures must record the reason.

## Indirect Specular State

Current `IndirectSpecular.hlsl` traces mirror or stochastic GGX paths:

- GGX half-vector sampling: `Engine/Assets/Shaders/Passes/Deferred/IndirectSpecular.hlsl:79`
- reflection sample construction: `Engine/Assets/Shaders/Passes/Deferred/IndirectSpecular.hlsl:132`
- PDF calculation: `Engine/Assets/Shaders/Passes/Deferred/IndirectSpecular.hlsl:177`
- throughput evaluation: `Engine/Assets/Shaders/Passes/Deferred/IndirectSpecular.hlsl:183`
- `BRDF * NoL / pdf`: `Engine/Assets/Shaders/Passes/Deferred/IndirectSpecular.hlsl:208`
- ray tracing: `Engine/Assets/Shaders/Passes/Deferred/IndirectSpecular.hlsl:219`
- sky miss: `Engine/Assets/Shaders/Passes/Deferred/IndirectSpecular.hlsl:232`
- path loop: `Engine/Assets/Shaders/Passes/Deferred/IndirectSpecular.hlsl:265`
- contribution accumulation: `Engine/Assets/Shaders/Passes/Deferred/IndirectSpecular.hlsl:289`

Positive:

- Full BRDF-over-PDF weighting is present for stochastic GGX.
- Mirror mode returns Fresnel throughput.
- Hit surfaces are reconstructed with ray-hit material data.

Issues:

- Sampling uses the raw NDF instead of visible-normal GGX. This can be unbiased but will be much noisier at grazing angles than VNDF sampling.
- Multi-bounce paths sample only specular-lobe continuation. Diffuse-after-specular paths are missing until a unified BSDF sampler exists.
- The pass uses tone-mapped sky radiance on miss.
- Roughness handling differs from direct lighting.
- It outputs a fully material-evaluated contribution but does not output hit distance, roughness, normal, albedo, or demodulated radiance needed by a dedicated denoiser.

## Secondary Hit Direct Lighting

`ShadeRayTracingHitIncidentRadiance` computes direct lighting at ray hits:

- `Engine/Assets/Shaders/RayTracing/RayTracingHitLighting.hlsli`
- It adds emissive first.
- It loops directional, point, and spot lights.
- It calls `BRDF::Direct::Evaluate` through `AccumulateRayTracingHitDirectLight`.

Positive:

- This is the right conceptual shape for next-event estimation at indirect hit points.

Issues:

- It does not trace shadows from secondary hit points to lights. Secondary direct lighting is unshadowed.
- It repeats local-light attenuation and spot-cone behavior from primary direct lighting.
- It includes subsurface in the same additive way as primary direct lighting.

## Sky and Presentation State

`SkyEnvironment.hlsli` tone maps sampled sky radiance:

- `Engine/Assets/Shaders/Lighting/SkyEnvironment.hlsli:13`
- `Engine/Assets/Shaders/Lighting/SkyEnvironment.hlsli:18`
- `Engine/Assets/Shaders/Lighting/SkyEnvironment.hlsli:20`

`Sky.hlsl` writes that sky color into scene color:

- `Engine/Assets/Shaders/Passes/Deferred/Sky.hlsl`

`PresentScene.hlsl` applies Reinhard tone mapping again:

- `Engine/Assets/Shaders/Passes/Presentation/PresentScene.hlsl:24`
- `Engine/Assets/Shaders/Passes/Presentation/PresentScene.hlsl:33`

Issue:

- Sky pixels are tone mapped in the sky pass and then tone mapped again at presentation.
- Indirect diffuse and specular miss rays also receive tone-mapped sky, not linear HDR sky radiance.
- This violates PBR-R-001 and PBR-R-007 and is currently one of the highest priority correctness bugs.

## Denoising and Provider State

Existing DLSS integration:

- Current Streamline path queries and evaluates `sl::kFeatureDLSS`.
- Required resources are input color, depth, motion vectors, output color, native command list.
- Relevant code: `Engine/Renderer/Private/Upscaling/NvidiaDlss/StreamlineDlssRuntime.cpp:31`, `:343`, `:582`, `:583`, `:600`.

Provider contract docs already acknowledge future denoiser inputs:

- `Docs/Architecture/02-Contracts/RendererProviderContract.md:256`
- `Docs/Architecture/02-Contracts/RendererProviderContract.md:257`
- `Docs/Architecture/02-Contracts/RendererProviderContract.md:259`
- `Docs/Architecture/02-Contracts/RendererProviderContract.md:475`

Issues:

- DLSS Super Resolution is currently an upscaler, not a lighting denoiser.
- No visible DLRR provider path exists yet.
- No visible NRD SIGMA execution path exists yet.
- Indirect diffuse/specular do not currently provide denoiser-ready auxiliary buffers.

## Shader and Pass Structure Audit

The shader directory already has a useful top-level vocabulary:

```text
BRDF/
Common/
Debug/
Geometry/
Lighting/
Material/
Passes/
RayTracing/
Resources/
```

This is a good starting hierarchy. It lets a reader infer ownership without opening every file.

The largest shader files at the time of this audit are:

```text
359 Engine/Assets/Shaders/RayTracing/RayTracingMaterialHit.hlsli
322 Engine/Assets/Shaders/Passes/Deferred/IndirectSpecular.hlsl
275 Engine/Assets/Shaders/Passes/Deferred/IndirectDiffuse.hlsl
192 Engine/Assets/Shaders/Passes/Deferred/RayTracedShadows.hlsli
187 Engine/Assets/Shaders/Passes/Deferred/DirectLightingCommon.hlsli
174 Engine/Assets/Shaders/Material/Material.hlsli
154 Engine/Assets/Shaders/Passes/Deferred/DirectLighting.hlsl
```

Positive structure:

- BRDF primitives are already separated into distribution, geometry, Fresnel, diffuse, specular, subsurface, and occlusion files.
- CPU renderer passes are already split between `Frame/Lighting/*`, `Passes/Deferred/*Pass.*`, `Passes/Bindings/*`, and `ShaderRegistrations/*`.
- Material texture-table sampling is separated from material property sampling.
- Ray hit material reconstruction is centralized enough to avoid every effect reimplementing barycentric material decoding.
- Direct lighting has a pass entrypoint and a shared helper include instead of putting all light math inside the compute `main`.

Structural issues:

- `RayTracingHitLighting.hlsli` includes `Passes/Deferred/DirectLightingCommon.hlsli`. This is an inverted dependency: reusable ray-hit lighting depends on a deferred-pass helper because generic light math currently lives under `Passes/Deferred`.
- `DirectLightingCommon.hlsli` owns generic light direction, falloff, cone attenuation, and direct surface lighting. Those concepts should move to `Lighting/*` so primary direct lighting, secondary hit lighting, path tracing, and future probes/denoisers share them without depending on a deferred pass path.
- `RayTracedShadows.hlsli`, `RayTracedShadowSampling.hlsli`, `RayTracedShadowSignals.hlsli`, and `RayTracedShadowDenoiserInputs.hlsli` live under `Passes/Deferred`, but their concepts are reusable ray-tracing/visibility concepts. They should move or be wrapped by reusable `RayTracing/Shadows/*` or `Lighting/Visibility/*` modules before NRD SIGMA work expands them.
- `IndirectDiffuse.hlsl` currently owns pass IO, pass constants, surface records, random sampling, diffuse throughput, ray-origin policy, tracing, hit/miss resolve, path-loop logic, debug routing, and output writes. That is too many responsibilities for an entrypoint.
- `IndirectSpecular.hlsl` has the same issue for GGX sampling, path state, tracing, hit/miss resolve, debug routing, and output writes.
- `IndirectDiffuseSurface` and `IndirectSpecularSurface` are nearly the same concept. A shared `ShadingSurface` or `RayTracingPathSurface` should represent primary and hit surfaces.
- `IndirectDiffuse` and `IndirectSpecular` each own their first-lobe sample logic. That is acceptable for a first prototype, but it blocks a unified BSDF path sampler.
- Debug code is mostly separated, but debug-mode enums/constants are still pass-local in places where shared ray-hit debug modes already exist.

Recommended target layout:

```text
Engine/Assets/Shaders/Lighting/
  PunctualLights.hlsli
  SurfaceLighting.hlsli
  SkyEnvironment.hlsli
  Visibility.hlsli

Engine/Assets/Shaders/RayTracing/
  RayTracingTraceQuery.hlsli
  RayTracingHitSurface.hlsli
  RayTracingMaterialHit.hlsli
  RayTracingHitDebug.hlsli
  PathSampling.hlsli
  PathSurface.hlsli
  PathLighting.hlsli
  Shadows/
    RayTracedShadowSignals.hlsli
    RayTracedShadowSampling.hlsli
    RayTracedShadowTrace.hlsli
    RayTracedShadowDenoiserInputs.hlsli

Engine/Assets/Shaders/Passes/Deferred/
  DirectLighting.hlsl
  IndirectDiffuse.hlsl
  IndirectSpecular.hlsl
  LightingComposite.hlsl
  Sky.hlsl
  GBuffer*.*
  *Debug.hlsli
```

The pass files should become thin orchestration layers:

```text
load GBuffer / pass constants
build primary surface
call reusable lighting/path/shadow module
choose debug or production output
write target
```

CPU pass structure is in better shape than shader structure. The current split already resembles the desired model:

- `Frame/Lighting/*` schedules pass order.
- `Passes/Deferred/*Pass.*` owns resource declaration and dispatch.
- `Passes/Bindings/*` owns reusable binding policies.
- `RayTracing/Effects/*` owns settings and uniform-data construction.
- `ShaderRegistrations/*` owns shader package ABI.

Keep that shape. The main CPU-side improvement is to avoid growing pass classes into algorithm containers as denoisers, DLRR, or new lighting features arrive.

## Priority Issue List

P0 correctness blockers:

1. Sky/environment radiance is tone mapped before lighting and sky pixels are effectively tone mapped twice.
2. Primary direct lighting ignores material dielectric F0 while ray-hit lighting can use it.
3. Secondary hit direct lighting is unshadowed, so indirect bounces overestimate light in occluded regions.
4. Direct shadow rays do not appear to alpha-test foliage/material cutouts.
5. Light attenuation and spot falloff are not aligned with the imported glTF punctual light model.

P1 path-tracing convergence blockers:

1. Diffuse and specular indirect passes are separate lobe-continuation estimators instead of a unified BSDF path sampler.
2. Environment importance sampling is missing.
3. Emissive geometry is only found by random hit, with no explicit emitter sampling.
4. Source radius creates soft-shadow approximation but not physically integrated area-light radiance.
5. Roughness floor differs across direct and indirect paths.

P2 denoiser readiness blockers:

1. No raw shadow visibility target is wired into a SIGMA denoiser path.
2. No DLRR input/output provider path is visible.
3. Indirect passes do not output noisy/demodulated radiance, hit distance, and lobe metadata required for robust reconstruction.
4. Provider-neutral normals/roughness/albedo resources are not yet first-class denoiser inputs.

P3 validation gaps:

1. No visible white-furnace, mirror-sky, or glTF light calibration tests.
2. No high-sample reference path tracer mode.
3. No automated radiance statistics for lighting targets.
4. No EXR/HDR capture contract for pre-tonemap buffers.

P4 structural blockers:

1. Reusable ray-hit lighting depends on a deferred-pass include.
2. Generic light math is stored in `Passes/Deferred/DirectLightingCommon.hlsli`.
3. Shadow tracing/sampling/denoiser signal helpers are pass-local even though they are reusable visibility concepts.
4. Indirect diffuse/specular entrypoints mix pass IO, sampling, tracing, path state, resolve, and output.
5. Surface/path records are duplicated between diffuse and specular effects.

## Current Strengths

The repo is closer than a rough prototype:

- The frame graph has explicit lighting target ownership and clear pass order.
- Direct and indirect passes are split cleanly enough to instrument.
- The BRDF library has the core GGX/Smith/Schlick pieces.
- Texture cooking appears to distinguish color and data textures.
- Ray-hit material reconstruction is already substantial.
- The provider architecture has started isolating DLSS and future denoisers.
- The existing shader folders already make a clean modular layout achievable without a disruptive rename of the whole tree.

The main work is not to invent a new renderer. It is to lock the physics contract, remove non-linear sky transport, unify material/BRDF conventions, make indirect sampling converge toward a reference path tracer, and move generic lighting logic out of pass-specific files before the next feature wave hardens those boundaries.

# Sparkle PBR State Audit

Date: 2026-06-30

This document maps the current SparkleEngine PBR implementation against `Docs/Rendering/PBR/01-PBR-Reference-Requirements.md`.

The worktree already has modified lighting shaders at the time of this audit:

- `Engine/Assets/Shaders/Passes/Deferred/DirectLighting.hlsl`
- `Engine/Assets/Shaders/Lighting/PunctualLights.hlsli`
- `Engine/Assets/Shaders/Lighting/SurfaceLighting.hlsli`
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

The authoritative contract now lives in `Docs/Rendering/PBR/01-PBR-Reference-Requirements.md#lighting-target-contract`.

`LightingComposite` simply adds all lighting target values and emissive:

- `Engine/Assets/Shaders/Passes/Deferred/LightingComposite.hlsl:34`
- `Engine/Assets/Shaders/Passes/Deferred/LightingComposite.hlsl:39`
- `Engine/Assets/Shaders/Passes/Deferred/LightingComposite.hlsl:44`
- `Engine/Assets/Shaders/Passes/Deferred/LightingComposite.hlsl:66`

Current effective semantics match the PBR contract:

```text
DirectDiffuse = material-evaluated direct diffuse contribution
DirectSpecular = material-evaluated direct specular contribution
DirectSubsurface = material-evaluated direct subsurface contribution
IndirectDiffuse = material-evaluated indirect outgoing-radiance contribution from the diffuse first event
IndirectSpecular = material-evaluated indirect outgoing-radiance contribution from the specular first event
IndirectSubsurface = currently cleared, no active writer found in this pass family
SceneColor = sum(all lighting targets) + GBuffer emissive
```

This is internally consistent with the current composite. Older architecture notes that described indirect diffuse as irradiance before base-color multiplication have been superseded by the PBR contract.

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
- The rejected direct-light roughness classification helper has been removed. Direct primary and ray-hit lighting now use one BRDF evaluator again; Stage 2C keeps roughness as material data, leaves direct lighting on the shared BRDF path, and confines singularity guards to denominator/PDF expressions.

## Material and GBuffer State

Material sampling computes dielectric F0:

- `Engine/Assets/Shaders/Material/Material.hlsli:34`
- `Engine/Assets/Shaders/Material/Material.hlsli:74`
- `Engine/Assets/Shaders/Material/Material.hlsli:150`
- `Engine/Assets/Shaders/Material/Material.hlsli:201`

The GBuffer stores base color, normal, metallic, roughness, AO, dielectric F0, emissive, subsurface, and device Z:

- `Engine/Assets/Shaders/Passes/Deferred/GBufferPS.hlsl:30`
- `Engine/Assets/Shaders/Passes/Deferred/GBufferPS.hlsl:32`
- `Engine/Assets/Shaders/Passes/Deferred/GBufferPS.hlsl:37`

`GBufferMaterial.a` now stores dielectric F0. Primary deferred direct lighting and ray-hit direct lighting both build F0 through `SurfaceLighting::BuildF0`.

Ray-hit reconstruction does have material F0:

- `Engine/Assets/Shaders/RayTracing/RayTracingMaterialHit.hlsli:40`
- `Engine/Assets/Shaders/RayTracing/RayTracingMaterialHit.hlsli:389`

Remaining issue:

- Material roughness is transported as the authored/imported inclusive `[0, 1]` value. Direct, indirect diffuse, indirect specular, and ray-hit direct paths now share the Stage 2C roughness policy; capture-based validation remains Stage 13 work.

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

- `Engine/Assets/Shaders/Lighting/SurfaceLighting.hlsli`
- `Engine/Assets/Shaders/Lighting/PunctualLights.hlsli`
- `Engine/Assets/Shaders/Passes/Deferred/DirectLighting.hlsl`

Remaining issues:

- Local light attenuation is `1 / distance^2 * (1 - distance / range)^2`. This is not the glTF punctual range falloff and dims much more aggressively across the range.
- Spot cone attenuation is linear in the cone ramp. glTF-compatible punctual lights normally square the cone attenuation.
- Source radius affects stochastic shadow sampling but not direct-light radiometry. This is a punctual-light-with-soft-shadow approximation, not a physically sampled area light.
- The previous direct-lighting `0.04` roughness floor has been removed. Direct analytic lighting has no pass-local mirror/delta branch; singularity handling is localized to BRDF denominator/PDF safety, while finite-light and area-light policy remains Stage 4/4A work.

## Shadow State

Direct shadows:

- `RayTracedShadowTrace.hlsli` uses inline ray queries.
- Hard and one-sample soft area modes exist.
- Shadow settings contain an NRD SIGMA enum path.

Relevant code:

- `Engine/Assets/Shaders/RayTracing/Shadows/RayTracedShadowTrace.hlsli`
- `Engine/Assets/Shaders/RayTracing/Shadows/RayTracedShadowSampling.hlsli`
- `Engine/Assets/Shaders/RayTracing/Shadows/RayTracedShadowSignals.hlsli`
- `Engine/Assets/Shaders/RayTracing/Shadows/RayTracedShadowDenoiserInputs.hlsli`
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
- The previous diffuse-throughput `0.04` roughness floor has been removed in the current changelist.

## Indirect Specular State

Current `IndirectSpecular.hlsl` traces mirror or stochastic GGX paths through shared BRDF sampling helpers:

- GGX half-vector sampling: `Engine/Assets/Shaders/BRDF/SpecularSampling.hlsli`
- reflection sample construction: `Engine/Assets/Shaders/BRDF/SpecularSampling.hlsli`
- PDF calculation: `Engine/Assets/Shaders/BRDF/SpecularSampling.hlsli`
- throughput evaluation: `Engine/Assets/Shaders/Passes/Deferred/IndirectSpecular.hlsl`
- `BRDF * NoL / pdf`: `Engine/Assets/Shaders/Passes/Deferred/IndirectSpecular.hlsl`
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
- Mirror-vs-GGX sampling is no longer effect-local; `BRDF/SpecularSampling.hlsli` owns the current exact-mirror and finite-GGX reflection sample construction. Stage 7 still needs a unified BSDF path sampler, VNDF/MIS policy, and diffuse/specular lobe selection.
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

`SkyEnvironment.hlsli` samples sky radiance without applying a display transform:

- `Engine/Assets/Shaders/Lighting/SkyEnvironment.hlsli`

`Sky.hlsl` writes that linear HDR sky radiance into scene color:

- `Engine/Assets/Shaders/Passes/Deferred/Sky.hlsl`

Presentation now applies exposure, selectable tone mapping, and explicit output encoding through a display module:

- `Engine/Assets/Shaders/Display/ToneMapping.hlsli`
- `Engine/Assets/Shaders/Passes/Presentation/ToneMapping.hlsl`
- `Engine/Assets/Shaders/Passes/Presentation/OutputEncoding.hlsl`
- `Engine/Renderer/Private/Frame/PostProcessing/Exposure.cpp`
- `Engine/Assets/Shaders/Passes/PostProcessing/Exposure.hlsl`

Exposure state:

- A frame-graph `Exposure` texture exists as a 1x1 `R32G32B32A32_Float` resource.
- The production metering path uses a parallel reduction chain of log-luminance moments.
- The alternate metering path uses an explicit slower 2x2 downsample pyramid of the same moment payload.
- Exposure history is stored in persistent 1x1 frame-graph textures so automatic exposure adapts frame-to-frame instead of jumping instantly.
- The final resolve stores adapted exposure, average luminance, target exposure, and previous exposure in the exposure texture.
- Temporal exposure history is invalidated with the renderer temporal-history reset path, including resize and scene extent changes.
- The implementation is reference-backed by AMD FidelityFX FSR2/SPD, NVIDIA Falcor ToneMapper, and Microsoft MiniEngine exposure/luma shaders.

Resolved in Stage 0D:

- Sky pixels now stay linear HDR until presentation.
- Indirect diffuse and specular miss rays now receive the same linear HDR sky radiance used by the sky pass.
- Tone mapping is display-only for this path; remaining sky correctness work belongs to later HDR environment import, calibration, and validation stages.

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
- Raw ray-traced shadow signal packing and registered frame-graph resource formats need a contract audit. The shader helper packs visibility, hit distance, confidence, and max distance, while the current denoiser registration path uses single-channel visibility-style resources in places.
- Current ray-traced shadow queries use a first-hit visibility shortcut. That is valid for binary hard-shadow visibility, but it is not automatically a valid occluder-distance signal for NRD SIGMA-style denoising.

## RHI, Format, and Signal State

Current strengths:

- Scene lighting targets use floating-point HDR formats, so the frame graph has enough precision to carry physically scaled lighting before presentation.
- GBuffer normals are stored in a high-precision floating format.
- Motion vectors and device-Z are first-class GBuffer products.
- Presentation already has explicit tone-mapping and output-encoding code paths.

Issues:

- The material GBuffer now carries dielectric F0 for primary/ray-hit F0 parity. Stage 2A still needs to lock asset import for material extensions that can author non-default F0/specular behavior.
- Stage 0F now owns the typed renderer signal contract in `Docs/Rendering/PBR/04-PBR-Renderer-Signal-Contract.md`, including shadow, exposure, presentation, GBuffer, provider, and reserved indirect reconstruction signals.
- Packed raw shadow signal resources now match the shader `float4(visibility, hitDistance, confidence, maxDistance)` payload; scalar shadow visibility is explicitly named as denoised visibility or denoised visibility history.
- Motion-vector convention exists in provider code, but the PBR plan needs to lock units, jitter policy, camera-cut reset, depth convention, and normal space for all denoisers and reconstruction providers.

Required stages:

- Stage 0F: render-target, precision, and signal-surface contract.
- Stage 2 completed: material F0 parity between GBuffer and ray-hit material reconstruction.
- Stage 2C: full roughness range and reference roughness policy.
- Stage 2B: geometry, normal, depth, motion, and temporal signal contract.
- Stage 5 and Stage 6: shadow visibility/hit-distance resource split and NRD SIGMA integration boundary.
- Stage 11 and Stage 11A: provider-neutral DLRR/indirect denoiser resource contract and auxiliary buffers.

## Asset Import and Cooking State

Current strengths:

- Texture cooking distinguishes color usages from data usages. Base color, emissive color, and subsurface color are treated as color inputs, while normal, roughness, metallic, ambient occlusion, subsurface strength, and HDR color usages are treated as linear/data inputs.
- Normal maps have a dedicated compression path.
- HDR color inputs have an HDR compression path.
- The glTF importer maps base color, normal, AO, emissive, and packed metallic-roughness textures into material slots.
- The glTF light importer reads `KHR_lights_punctual` light color, intensity, range, cone angles, and direction.
- The repository already contains useful validation assets: EXR/HDR sky textures, glTF sample scenes such as DamagedHelmet/Sponza-style content, and Bistro-style BaseColor/Normal/Specular DDS texture sets.

Issues:

- The plan does not yet require deterministic asset-cook tests proving that each texture usage reaches the shader with the intended color-space and channel semantics.
- glTF material extension handling is incomplete for physically important controls such as specular/F0, IOR, and emissive strength. Unsupported extensions need diagnostics or deliberate implementation stages.
- Imported punctual light intensity units need a single documented conversion path into engine units. Otherwise light calibration can be accidentally fixed later by tone mapping or arbitrary intensity multipliers.
- HDR sky/environment texture import must be proven to stay linear HDR through sampling and presentation.
- Specular-workflow assets, such as Bistro `*_Specular` textures, need an explicit import policy. They should be converted into the engine's supported material model, used only by a declared specular workflow, or rejected/ignored with diagnostics.

Required stages:

- Stage 2A: asset color-space, material extension, and light-unit import contract.
- Stage 4: punctual light units and falloff.
- Stage 13: validation/review pack with asset-cook and glTF calibration cases.

## Geometry, Normal, and Temporal State

Current strengths:

- The renderer already produces motion vectors, normals, device-Z, TLAS data, and material texture tables.
- Provider contracts already have a place to describe motion-vector and depth conventions.
- Ray-hit material reconstruction is substantial enough to become the shared material/geometry decode path for validation and reference rendering.

Issues:

- Primary deferred shading and ray-hit shading do not yet have a documented shared policy for geometric normal, shading normal, face orientation, two-sided materials, normal-map orientation, and ray-origin offsets.
- Denoisers and DLRR need stable normal, depth/viewZ, motion-vector, jitter, and history-reset conventions. Those rules should be locked before signal buffers multiply.
- Alpha-tested geometry participates in indirect ray-query material handling but not in the direct shadow path.

Required stages:

- Stage 0A and Stage 0B: move generic shader concepts into reusable modules before adding more signal paths.
- Stage 2B: lock geometry/normal/depth/motion conventions.
- Stage 5: route direct shadows through alpha-test-aware visibility.
- Stage 11A: expose denoiser auxiliary buffers with agreed spaces and units.

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
174 Engine/Assets/Shaders/Material/Material.hlsli
154 Engine/Assets/Shaders/Passes/Deferred/DirectLighting.hlsl
```

Positive structure:

- BRDF primitives are already separated into distribution, geometry, Fresnel, diffuse, specular, subsurface, and occlusion files.
- CPU renderer passes are already split between `Frame/Lighting/*`, `Passes/Deferred/*Pass.*`, `Passes/Bindings/*`, and `ShaderRegistrations/*`.
- Material texture-table sampling is separated from material property sampling.
- Ray hit material reconstruction is centralized enough to avoid every effect reimplementing barycentric material decoding.
- Generic punctual-light helpers and direct surface evaluation now live under `Lighting/*`, with `DirectLighting.hlsl` kept as a pass entrypoint.

Structural issues:

- `IndirectDiffuse.hlsl` currently owns pass IO, pass constants, surface records, random sampling, diffuse throughput, ray-origin policy, tracing, hit/miss resolve, path-loop logic, debug routing, and output writes. That is too many responsibilities for an entrypoint.
- `IndirectSpecular.hlsl` has the same issue for GGX sampling, path state, tracing, hit/miss resolve, debug routing, and output writes.
- `RayTracingPathSurface` now represents primary and hit surface state, but diffuse/specular still keep effect-local path sample, throughput, and result payloads.
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

1. Sky/environment radiance now uses the Stage 1 HDR helper; HDR sky import/calibration and importance sampling remain later-stage work.
2. Material F0 parity is implemented for current primary and ray-hit shader paths; import extension coverage for non-default specular/F0 remains Stage 2A.
3. Secondary hit direct lighting is unshadowed, so indirect bounces overestimate light in occluded regions.
4. Direct shadow rays do not appear to alpha-test foliage/material cutouts.
5. Light attenuation and spot falloff are not aligned with the imported glTF punctual light model.
6. PBR-relevant asset import rules are not yet enforced by tests, so color-space, packed-channel, HDR sky, and light-unit regressions could silently invalidate lighting.

P1 path-tracing convergence blockers:

1. Diffuse and specular indirect passes are separate lobe-continuation estimators instead of a unified BSDF path sampler.
2. Environment importance sampling is missing.
3. Emissive geometry is only found by random hit, with no explicit emitter sampling.
4. Source radius creates soft-shadow approximation but not physically integrated area-light radiance.
5. Full roughness range is implemented for the current direct, indirect diffuse, indirect specular, and ray-hit direct shader paths: material roughness is not floored or reused as compensation, direct/ray-hit lighting use one BRDF path, and mirror-vs-GGX sampling lives in shared BRDF sampling code. Stage 13 still needs capture validation for roughness sweeps and mirror-sky behavior; Stage 4/4A still owns finite-light policy; Stage 11/11A still owns denoiser auxiliary roughness resources.
6. There is no shared lobe-energy budget for diffuse, specular, subsurface, transmission/future lobes, direct lighting, indirect sampling, and reference path tracing.

P2 denoiser readiness blockers:

1. No raw shadow visibility target is wired into a SIGMA denoiser path.
2. No DLRR input/output provider path is visible.
3. Indirect passes do not output noisy/demodulated radiance, hit distance, and lobe metadata required for robust reconstruction.
4. Provider-neutral normals/roughness/albedo resources are not yet first-class denoiser inputs.
5. Shadow hit-distance generation is not yet compatible with NRD SIGMA-style occluder-distance requirements.
6. Packed shadow signal formats and frame-graph resource formats are not yet reconciled.
7. Motion-vector, depth/viewZ, normal-space, jitter, and history-reset contracts need to be locked for all denoiser and reconstruction providers.

P3 validation gaps:

1. No visible white-furnace, mirror-sky, or glTF light calibration tests.
2. No high-sample reference path tracer mode.
3. No automated radiance statistics for lighting targets.
4. No EXR/HDR capture contract for pre-tonemap buffers.
5. No asset-cook/color-space validation pack for physically meaningful texture and light import.
6. No review bundle that maps each completed stage to references followed, captures produced, and known deviations.

P4 structural blockers:

1. Indirect diffuse/specular entrypoints mix pass IO, sampling, tracing, path state, resolve, and output.
2. Effect-local path sample, throughput, and result payloads are still split between diffuse and specular effects.
3. Denoiser/reconstruction resources now have a provider-neutral signal contract from Stage 0F; Stage 11, Stage 11A, and Stage 13 still need to allocate/debug/capture the future DLRR and indirect denoiser auxiliaries through that contract.
4. Asset import/cooking rules are not yet tied to renderer validation, which risks duplicate material assumptions between tools and shaders.

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

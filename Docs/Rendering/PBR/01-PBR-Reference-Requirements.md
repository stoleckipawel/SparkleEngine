# PBR Reference Requirements

Date: 2026-06-29

This document defines the target physically based rendering contract for SparkleEngine direct lighting, indirect ray-traced lighting, distant sky fallback, and denoiser-facing outputs.

The current target excludes volumetric lighting, ReSTIR, probe GI, DDGI, path guiding, neural radiance caches, and material systems beyond the current metallic-roughness plus subsurface fields. Volumetrics can be layered later after surface transport is correct.

## Reference Baseline

Sparkle should treat the following sources as the external baseline:

- Filament PBR documentation: <https://google.github.io/filament/Filament.md.html>
- Epic, "Real Shading in Unreal Engine 4": <https://cdn2.unrealengine.com/Resources/files/2013SiggraphPresentationsNotes-26915738.pdf>
- Unreal Engine physically based materials: <https://dev.epicgames.com/documentation/en-us/unreal-engine/physically-based-materials-in-unreal-engine>
- Unreal Engine path tracer: <https://dev.epicgames.com/documentation/en-us/unreal-engine/path-tracer-in-unreal-engine>
- NVIDIA RTX Path Tracing SDK: <https://github.com/NVIDIA-RTX/RTXPT>
- NVIDIA RTXDI integration guide: <https://github.com/NVIDIA-RTX/RTXDI/blob/main/Doc/Integration.md>
- NVIDIA NRD: <https://github.com/NVIDIA-RTX/NRD>
- NVIDIA Streamline DLSS Ray Reconstruction guide: <https://github.com/NVIDIAGameWorks/Streamline/blob/main/docs/ProgrammingGuideDLSS_RR.md>
- AMD FidelityFX SDK: <https://gpuopen.com/fidelityfx-sdk/>
- Unreal Engine shader development: <https://dev.epicgames.com/documentation/en-us/unreal-engine/shader-development-in-unreal-engine>
- glTF 2.0 material model: <https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#materials>
- glTF KHR_lights_punctual: <https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_lights_punctual>
- PBRT rendering equation reference: <https://pbr-book.org/4ed/Light_Transport_I_Surface_Reflection/A_Better_Path_Tracer>

The useful comparison is not "copy Unreal" or "copy Filament". The useful target is:

- Filament discipline for real-time PBR definitions, units, material parameter ranges, BRDF consistency, color management, and IBL math.
- Unreal discipline for artist-facing metallic-roughness materials, GGX/Smith/Schlick conventions, direct plus image-based split-sum approximations, and a path tracer as the reference-quality truth mode.
- NVIDIA discipline for path-traced signal contracts, temporal/spatial denoiser inputs, and SDK isolation.

## Target Rendering Equation

All lighting paths must be traceable back to the surface rendering equation:

```text
Lo(x, wo) = Le(x, wo)
          + integral_over_hemisphere f_s(x, wo, wi) Li(x, wi) abs(dot(N, wi)) dwi
```

The split real-time pipeline may write separate buffers, but each buffer must have a precise physical meaning. The split must not change the equation being estimated.

Target primary-surface decomposition:

```text
SceneLinearRadiance =
    DirectDiffuseContribution
  + DirectSpecularContribution
  + DirectSubsurfaceContribution
  + IndirectDiffuseContribution
  + IndirectSpecularContribution
  + IndirectSubsurfaceContribution
  + EmissiveRadiance
```

For the near-term scope, indirect subsurface may remain zero if subsurface is marked non-reference. The target direct and indirect diffuse/specular terms are already material-evaluated outgoing radiance contributions, not raw irradiance.

This convention intentionally matches the current `LightingComposite` sum model. If a future denoiser wants demodulated signals, those must be additional buffers, not a silent semantic change to the lighting targets.

## Requirement IDs

### PBR-R-001: Linear HDR Transport

All lighting, sky samples, emissive values, direct light values, and indirect ray contributions must remain linear HDR radiance or radiance-like values until the presentation pass, debug view, or a provider-specific display transform.

Forbidden before final display:

- Reinhard/ACES/tone mapping of sky radiance used by lighting.
- Gamma-space BRDF evaluation.
- Storing physically meaningful transport in LDR targets.
- Clamping radiance except for explicit debug visualizations or numerical safety values that are separately measured.

Required acceptance:

- A sky-only scene shows identical sky luminance whether viewed directly through the sky pass or sampled by a perfect mirror ray, before final presentation tone mapping.
- A bright HDR environment can exceed 1.0 in scene color and indirect buffers.

### PBR-R-002: Material Input Contract

The primary GBuffer and ray-hit reconstruction must expose the same material values to the same BRDF code:

- base color, linear
- opacity/alpha mode/alpha cutoff
- metallic in `[0, 1]`
- perceptual roughness in `[0, 1]`
- dielectric F0 or reflectance, with a documented mapping
- emissive radiance or clearly documented emissive factor
- normal in world space, normalized after normal mapping
- two-sided normal policy
- optional ambient/cavity occlusion, if used
- optional subsurface color/strength, if enabled

The material model must not use fixed primary-surface F0 while secondary ray hits use material F0. Primary and secondary rays must shade the same material the same way.

### PBR-R-003: BRDF Contract

Near-term reference BRDF:

- Metallic-roughness workflow.
- GGX/Trowbridge-Reitz normal distribution.
- Smith height-correlated visibility term.
- Schlick Fresnel.
- Lambert diffuse for the strict validation path.
- Burley diffuse may be supported as the production default after furnace tests prove the chosen energy behavior.

Required math convention:

```text
f_spec = D * V * F
```

where `V` may mean `G / (4 NoL NoV)` when using the height-correlated Smith visibility form. If a different geometry helper returns raw `G`, the caller must divide by `4 NoL NoV`. This convention must be explicit because both forms are common.

Energy rules:

- Dielectrics split energy between specular and diffuse with `kS = F` and `kD = (1 - F) * (1 - metallic)` or a more accurate directional albedo model.
- Metals have no diffuse lobe in the metallic workflow.
- Clearcoat, sheen, transmission, and subsurface are separate lobes that must have energy compensation before they are marked reference-correct.
- Additive subsurface on top of unchanged diffuse is not reference-correct unless the diffuse lobe is reduced accordingly.

Required validation:

- White furnace test for diffuse, metallic, and mixed materials.
- Smooth and rough conductor reflectance checks.
- NoL/NoV grazing tests for NaN, infinity, and energy spikes.
- CPU or offline shader unit tests for BRDF integral bounds.

### PBR-R-004: Light Unit Contract

Each light type must have a documented unit:

- Directional light intensity: illuminance in lux, or a documented engine unit that is calibrated to lux.
- Point light intensity: luminous intensity in candela if imported from `KHR_lights_punctual`.
- Spot light intensity: luminous intensity in candela if imported from `KHR_lights_punctual`.
- Light color: linear RGB chromatic multiplier.
- Source radius/angular diameter: geometric emitter size for sampling and shadow softness. It must not silently change total emitted energy unless the unit contract says so.

Direct punctual equation:

```text
Lo_direct += f_s(x, wo, wi) * Li_light(x, wi) * Visibility(x, light) * NoL
```

For a point light:

```text
Li_light = color * intensity_cd / max(distance^2, epsilon)
```

For a directional light:

```text
Li_light = color * illuminance_lux
```

This is acceptable if the engine treats directional intensity as irradiance on a surface perpendicular to the light direction and then multiplies by `NoL` in the rendering equation.

Finite area-light sampling should eventually use the sampled point or sampled direction in both visibility and BRDF evaluation:

```text
Lo += f_s * Le_or_I * geometry_term * visibility / pdf
```

Using source radius only to soften shadows is an approximation. It may remain as a punctual-light approximation, but it must be named and tested as such.

### PBR-R-005: Direct Lighting Requirements

Direct lighting must:

- Use the same BRDF functions as ray-hit direct lighting.
- Use the same material F0, roughness floor, normal policy, and alpha policy as ray-hit lighting.
- Keep direct visibility separate from radiance. Denoisers should receive raw visibility and hit-distance signals when available.
- Support alpha-tested shadow occlusion when alpha-tested geometry exists.
- Never tone-map or clamp light radiance before writing direct lighting targets.

Direct lighting may split contributions by first material lobe:

```text
DirectDiffuse = f_diffuse * Li * NoL * visibility
DirectSpecular = f_specular * Li * NoL * visibility
DirectSubsurface = f_subsurface * Li * NoL * visibility
```

The sum must match evaluating the same BSDF lobes together.

### PBR-R-006: Indirect Lighting Requirements

The reference estimator for indirect surface lighting is path tracing with next-event estimation:

```text
throughput *= f_s(previous, wo, wi) * abs(dot(N, wi)) / pdf(wi)
contribution += throughput * DirectLightingAtHit(hit, -wi)
contribution += throughput * EmissiveAtHit(hit, -wi)
miss contribution += throughput * SkyRadiance(wi)
```

Rules:

- Throughput must always be `BSDF * cosine / pdf`.
- Every sampled direction must carry its true PDF.
- Diffuse and specular path estimators must not double count the same first-event contribution.
- Multi-bounce paths should eventually sample all enabled lobes using one unified BSDF sampling policy, even if outputs are split by the primary event.
- Russian roulette should start only after a minimum bounce count and must compensate survival probability.
- No hidden temporal accumulation may be called "ground truth".

Near-term acceptable split:

- `IndirectDiffuse` stores path contributions whose first primary event is the diffuse lobe.
- `IndirectSpecular` stores path contributions whose first primary event is the specular lobe.
- Secondary bounces should be unified as soon as the implementation moves beyond single-bounce reference.

### PBR-R-007: Sky and Environment Fallback

The distant sky texture is a light source.

Required:

- Sample sky/environment as linear HDR radiance.
- Use the same sky radiance for sky pixels and ray misses.
- Apply exposure/tone mapping only after lighting composition or in provider-required output stages.
- Record the sky texture orientation convention.

Strongly recommended after baseline correctness:

- Add environment-map importance sampling. This is not ReSTIR. It is basic path tracing variance reduction and is needed for HDR skies with small bright regions.
- Support mip/roughness sampling for non-ray-traced fallback IBL if that path remains.

### PBR-R-008: Shadow and Visibility Denoiser Contract

Raw shadow rays should produce denoiser-ready signals:

- raw visibility
- hit distance or normalized hit distance
- confidence/sample count
- light id or pass id when needed
- depth
- normal
- motion vectors
- disocclusion/history validity

NRD SIGMA is the intended target for ray-traced shadow denoising. The direct lighting shader should not consume a future denoised visibility buffer through an undocumented path. The visibility source should be explicit:

```text
shadowVisibility = HardRay | RawStochasticRay | DenoisedVisibility
```

### PBR-R-009: DLSS, DLRR, and Indirect Denoising Contract

DLSS Super Resolution is not a general raw-lighting denoiser. It can hide some noise after composition, but it does not replace a denoiser contract for stochastic GI/reflection signals.

For DLSS Ray Reconstruction or any future provider, Sparkle must expose provider-neutral resources before provider-specific integration:

- noisy radiance or composed color at the expected stage
- depth with convention
- motion vectors with units and direction convention
- normals
- roughness
- diffuse albedo/base color
- specular albedo/F0 or material reflectance
- emissive mask or radiance if required
- hit distance for specular/indirect signals if required
- exposure or explicit auto-exposure mode
- reset/history/disocclusion metadata

Provider outputs must not redefine the physical meaning of internal lighting targets. If DLRR consumes composed noisy color, the renderer still needs internal debug views for raw direct, indirect diffuse, indirect specular, visibility, PDF, throughput, and hit distance.

### PBR-R-010: Validation Scenes

The implementation should ship or document deterministic validation scenes:

- White furnace: closed room with uniform white environment.
- Gray sphere matrix: metallic `{0, 0.5, 1}` by roughness `{0.02, 0.1, 0.5, 1}`.
- Mirror sky: perfect mirror under HDR sky; sky pixel and mirror ray luminance should match before tone mapping.
- glTF light calibration: point and spot lights with known candela and range behavior.
- Alpha shadow card: alpha-tested foliage casting direct shadows and indirect occlusion.
- Cornell/Sponza indirect: compare 1 spp denoised and high spp reference captures.
- Emissive panel: emissive geometry visible to indirect rays.

Required measurement outputs:

- Raw target captures before presentation.
- Average luminance and max luminance per target.
- Difference against high-sample reference.
- GPU feature path used: no RT, descriptor TLAS, device-address TLAS.

### PBR-R-011: Shader Architecture Contract

PBR correctness and shader architecture are equal requirements. A physically correct feature is not accepted if it arrives as a pass-specific tangle that cannot be reused by the next lighting problem.

Sparkle shader code should follow this dependency direction:

```text
Common
  -> Geometry
  -> Material
  -> BRDF
  -> Lighting
  -> RayTracing
  -> Passes
```

Rules:

- `Passes/*` files may depend on shared modules.
- Shared modules must not include `Passes/*` files.
- `BRDF/*` must not know about lights, GBuffer, TLAS, denoisers, debug modes, or pass CVars.
- `Lighting/*` may know about light records, radiance, visibility, environment sampling, and surface lighting equations.
- `RayTracing/*` may know about tracing, hit reconstruction, path state, path sampling, and ray-hit material decoding.
- `Passes/*` entrypoints own resource declarations, pass uniforms, pixel dispatch, GBuffer loading, debug-mode selection, and output writes.
- Effect-specific settings may live with the effect, but reusable sampling/tracing/lighting code must be named after the concept, not after the first pass that used it.

Expected reusable shader modules for the near-term lighting work:

- `Lighting/PunctualLights.hlsli`: directional, point, and spot light direction/radiance/falloff helpers.
- `Lighting/SurfaceLighting.hlsli`: lobe-evaluated direct lighting for a surface.
- `Lighting/SkyEnvironment.hlsli`: linear sky radiance, sky UVs, optional display-only transforms.
- `RayTracing/RayTracingTraceQuery.hlsli`: generic ray-query trace policies and alpha-test candidate handling.
- `RayTracing/RayTracingSurface.hlsli`: common primary and hit surface records.
- `RayTracing/PathSampling.hlsli`: direction samples, PDFs, lobe selection, throughput.
- `RayTracing/PathLighting.hlsli`: miss/hit radiance resolution and next-event lighting.
- `Passes/Deferred/*.hlsl`: compact pass entrypoints that wire inputs to reusable modules.

File-size is not a hard correctness rule, but it is a smell. Any shader file above roughly 250 lines should be reviewed for mixed responsibilities. A longer file can stay only if it owns one cohesive concept, such as full material hit reconstruction.

### PBR-R-012: CPU Pass Architecture Contract

Renderer pass code should mirror the shader separation:

- `Frame/*` files schedule pass order and high-level resources.
- `Passes/*Pass.*` files declare resources, bind parameters, check capability gates, and dispatch.
- `ShaderRegistrations/*` files define shader package metadata and parameter ABI.
- `RayTracing/Effects/*` files own effect settings, CVars, uniform data, and pass-data builders.
- `Passes/Bindings/*` files own reusable binding policies for lighting, environment, TLAS, hit data, and material texture tables.

Rules:

- Pass classes should not implement lighting algorithms.
- CVar/settings files should not leak into reusable shader modules.
- Capability gates should be explicit in CPU pass execution, not hidden behind shader zero-output behavior.
- New lighting features should add a narrow pass facade over reusable shader modules, not fork direct-light, indirect-light, or sky logic.

## Ground Truth Target

The long-term reference is a path tracer mode, not a visual guess:

- Same material decoder as the deferred path.
- Same BRDF code, or generated/shared code where possible.
- Same light units.
- Same sky radiance.
- Same alpha-test and two-sided policies.
- Multi-sample accumulation with deterministic seeds.
- A way to save EXR/HDR buffers for comparison.

Real-time direct and indirect passes should be judged by convergence toward that path tracer, not just by looking similar to an existing game renderer.

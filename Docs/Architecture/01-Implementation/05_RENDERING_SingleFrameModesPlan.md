# 05. RENDERING - Single Frame Modes

Status: implemented and hardened
Updated: 2026-07-10

## Outcome

Sparkle has one frame pipeline and two independent producer choices:

- `GBufferMode`: `Rasterized` or `Raytraced`
- `LightingMode`: `RestirPathTraced` or `ReferencePathTraced`

`BuildFrame` does not branch into a special reference renderer. It always builds the same frame skeleton:

```cpp
CreateFrameSceneResources(...);
AddRaytracingScenePasses(...);
AddGBufferPasses(...);
AddLightingPasses(...);
AddPostProcessingPasses(...);
```

The four supported combinations are therefore first-class:

| GBuffer | Lighting | Intended use |
| --- | --- | --- |
| Rasterized | ReSTIR path traced | Default realtime rendering |
| Raytraced | ReSTIR path traced | Raytraced primary visibility with realtime lighting |
| Rasterized | Reference path traced | Reference lighting over raster primary surfaces |
| Raytraced | Reference path traced | Fully raytraced primary visibility and reference lighting |

Post-processing, exposure, debug output, upscaling, and presentation remain shared. There is no reference final-color override.

## Mode Ownership

The runtime source of truth is CVar-backed:

- `r.GBuffer.Mode`
- `r.Lighting.Mode`

`EngineRenderingSettingsSection` and the editor are persistence/UI facades over those CVars. The frame graph is rebuilt and the relevant temporal histories are invalidated when a mode changes.

Mode decisions live with their owners:

- `Frame/Deferred/GBuffer.cpp` selects the active GBuffer producer.
- `Frame/Lighting/Lighting.cpp` selects the active lighting producer.
- Frame core owns neither mode policy nor a mirrored render-path enum.

Invalid `LightingMode` values resolve to `RestirPathTraced`, which is the default mode.

## GBuffer Contract

Both GBuffer producers write the same `GBufferRenderTargets` contract:

- base color
- world normal
- material parameters
- emissive
- subsurface data
- device depth
- motion vector

Rasterized depth remains a depth target. Raytraced depth is an `R32_Float` UAV containing device depth and is linearized by the same downstream frame helper.

The raytraced producer traces camera rays, reuses the shared material-hit path, writes camera/object motion, and writes the same sky values used by rasterized clear state. Its targets are cleared before the capability-gated dispatch, so an unavailable ray-query/material path cannot expose uninitialized frame products. One-sided instances cull back-facing primary hits; double-sided instances retain the TLAS cull-disable flag.

The current lighting/GBuffer shaders consume descriptor-bound TLAS resources. A requested partitioned TLAS provider is therefore selected only when that provider exposes a descriptor path; Vulkan NV partitioned TLAS currently falls back to classic descriptor TLAS instead of silently selecting an incompatible device-address path.

## Lighting Orchestration

`AddLightingPasses` is intentionally high level:

1. create and clear lighting products;
2. select one producer branch;
3. run the shared lighting composite and sky passes;
4. run the selected branch finalizer.

The branch orchestrators contain only named high-level operations. Pass resource declarations, capability checks, binding details, and dispatches live in their dedicated frame/pass files.

Common composition and sky ownership appears once in `Lighting.cpp`; it is not duplicated by the two branches.

### ReSTIR realtime branch

`LightingMode::RestirPathTraced` contains:

- ReSTIR-style direct-light reservoir generation, temporal reuse, spatial reuse, shadow visibility, and direct-light resolve;
- ReSTIR-style indirect path candidate generation, temporal reuse, spatial reuse, and indirect resolve;
- noisy radiance plus diffuse/specular albedo, material, and specular hit-distance guide products for NVIDIA DLSS Ray Reconstruction;
- generic post-processing scheduling of the configured ray-reconstruction provider only after its complete input contract exists.

Current and working indirect reservoirs are explicitly zeroed before capability-gated passes. History is marked valid only while the ReSTIR branch is active and its resource set exists.

This is Sparkle-owned reservoir sampling inspired by RTXDI integration patterns. It is not described as RTXDI SDK parity because the RTXDI SDK is not integrated.

### Reference branch

`LightingMode::ReferencePathTraced` contains separate direct and indirect producers:

- direct lighting samples the shared analytic light/material model and traces shadow visibility;
- indirect lighting reconstructs the active GBuffer primary surface, then traces secondary bounces through shared path surface, sampling, lighting, and material-hit helpers;
- shared composite and sky passes produce an FP32 reference sample;
- progressive FP32 history accumulation produces normal `SceneColor` for shared post-processing.

Reference history is rejected on motion and reset when camera, scene geometry, skinning, materials, lights, light budgets, environment, shadow policy, GBuffer mode, lighting settings, or resolution changes. Scene state is hashed field-by-field, so structure padding cannot create false resets. Non-finite samples cannot contaminate history.

The reference estimator converges for the configured path depth, maximum distance, and current Sparkle material/light model. Those finite settings are part of the estimator contract, so documentation should not claim mathematically unbounded transport.

## Shared Implementation Boundaries

The refactor keeps shared mechanisms narrow and policy-free:

- `RayTracedSurfaceLightingPassBinding` owns common TLAS, GBuffer, light, hit-data, material-table, environment, sampler, and capability binding.
- `RayTracedSurfaceLightingPassParameters` and its registration counterpart own the matching shared C++/shader parameter ABI.
- `IndirectLightingOutputPassBinding` owns the common C++ output declaration/binding contract.
- `IndirectLightingOutputs.hlsli` owns common indirect radiance and denoiser-guide writes.
- `RestirReservoirCommon.hlsli` owns shared reservoir limits, deterministic seeding, and surface-history compatibility.
- `GBufferPathSurface.hlsli` reconstructs the primary path surface once for reference and ReSTIR indirect lighting.
- `PathTrace.hlsli` owns common secondary-ray trace settings.
- `ReservoirHistoryResources` owns the repeated sample/weight/surface allocation, release, validation, and frame-graph binding lifecycle.

Settings builders remain payload-only. They may clamp sample counts, bounce counts, bias, and distance, but do not own scheduling flags such as `Enabled`.

## Shader and Pass File Rule

Each shader registration source owns exactly one `TGlobalShader` class and one registration. Each compute shader entry point has a dedicated `.hlsl` file and dedicated pass infrastructure. Shared declarations and algorithms live in shader-free C++ headers or `.hlsli` files.

Multi-stage raster packages may share a package identifier, but their vertex and pixel shader registrations still live in separate source files.

The ReSTIR direct reservoir, direct shadow variants, ReSTIR indirect temporal/spatial/resolve stages, reference direct/indirect stages, accumulation stage, exposure stages, and GBuffer shader stages follow this rule.

## Removed Architecture

The following retired concepts are intentionally absent:

- `FrameRenderPath`
- top-level realtime/reference frame branching
- full-frame `ReferencePathTracing` final-color override
- legacy independent indirect diffuse/specular pass families and their `Enabled` settings payloads
- special reference post-processing/presentation path

## Verification

Required static checks:

```text
rg "Build.*Settings\(\)\.Enabled|settings\.Enabled" Engine/Renderer
rg "FrameRenderPath|PathTracedReference|AddReferenceRenderingPasses" Engine/Renderer
ShaderCompiler.exe list-shaders --validate
```

Required builds and shader cooks:

```text
cmake --build build --target SparkleRenderer --config DevelopmentEditor
cmake --build build --target ShowcaseEditor --config DevelopmentEditor
cmake --build build --target ShaderCompiler --config DevelopmentEditor
ShaderCompiler.exe cook --package PathTracedDirectLighting --target DxilSm66 --target SpirV16 --no-cache
ShaderCompiler.exe cook --package PathTracedIndirectLighting --target DxilSm66 --target SpirV16 --no-cache
ShaderCompiler.exe cook --package ReferenceLightingAccumulation --target DxilSm66 --target SpirV16 --no-cache
ShaderCompiler.exe cook --package RestirIndirectTemporal --target DxilSm66 --target SpirV16 --no-cache
ShaderCompiler.exe cook --package RestirIndirectSpatial --target DxilSm66 --target SpirV16 --no-cache
ShaderCompiler.exe cook --package RestirIndirectResolve --target DxilSm66 --target SpirV16 --no-cache
```

Runtime inspection should cover all four GBuffer/lighting combinations and verify that reference accumulation resets after every state mutation listed above.

## External Design References

These projects informed product contracts and algorithm boundaries, not a copied renderer architecture:

- NVIDIA RTXDI: https://github.com/NVIDIA-RTX/RTXDI
- NVIDIA RTXPT: https://github.com/NVIDIA-RTX/RTXPT
- NVIDIA NRD: https://github.com/NVIDIA-RTX/NRD
- NVIDIA Donut: https://github.com/NVIDIA-RTX/Donut

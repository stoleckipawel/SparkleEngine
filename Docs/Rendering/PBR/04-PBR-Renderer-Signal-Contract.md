# Sparkle PBR Renderer Signal Contract

This is a compact semantic map for PBR-relevant renderer buffers. It is not a new format registry: code format constants stay with the effect that owns them.

Reference lineage:

- NRD and Streamline DLRR motivate explicit denoiser/provider signal names.
- Falcor and AMD FidelityFX motivate stable frame-graph ownership and temporal reset state.
- Sparkle's local contract keeps lighting and sky linear HDR until presentation.

Reuse/DRY audit:

- Reused existing homes: `FrameRenderFormats` for scene/lighting/presentation, `GBufferFormats` for GBuffer products, and `ShadowDenoiseContract` for shadow-denoiser formats.
- Reused existing resource owners: `FrameSceneResources`, `LightingRenderTargets`, `FrameGraphDenoiserRegistration`, `UpscalerInputContract`, and `ShadowDenoiseContract`.
- No renderer-wide alias namespace, provider shim, or new pass was added for this contract.

| Signal | Owner | Format | Essential contract |
| --- | --- | --- | --- |
| `SceneColor`, `FinalSceneColor` | Frame scene resources / upscaler fallback | `FrameRenderFormats::SceneColor` | Linear HDR pre-presentation scene color. |
| `DirectDiffuse`, `DirectSpecular`, `DirectSubsurface` | Direct lighting | `FrameRenderFormats::SceneColor` | Material-evaluated outgoing radiance contributions, not irradiance. |
| `IndirectDiffuse`, `IndirectSpecular`, `IndirectSubsurface` | Indirect lighting | `FrameRenderFormats::SceneColor` | Material-evaluated outgoing radiance contributions split by first primary lobe. |
| `GBufferBaseColor`, `GBufferNormal`, `GBufferMaterial`, `GBufferEmissive`, `GBufferSubsurface`, `GBufferDeviceZ`, `GBufferMotionVector` | GBuffer | `GBufferFormats::*` | Material, geometry, depth, and motion products. `GBufferMaterial` stores R metallic, G roughness, B ambient occlusion, A dielectric F0. |
| `Exposure` and exposure history | Exposure pass / frame pipeline | `R32G32B32A32_Float` | R adapted exposure, G average luminance, B target exposure, A previous exposure. |
| `ShadowVisibilitySignalRaw`, `ShadowVisibilitySignalScratch` | Shadow denoiser registration | `ShadowDenoiseContract::PackedVisibilitySignalFormat` | Packed `float4(visibility, hitDistance, confidence, maxDistance)`. |
| `ShadowVisibilityDenoised` | Shadow denoiser registration | `ShadowDenoiseContract::DenoisedVisibilityFormat` | Scalar denoised visibility output. |
| `PreviousDenoisedShadowVisibilityHistory`, `CurrentDenoisedShadowVisibilityHistory` | Frame scene resources / frame pipeline | `ShadowDenoiseContract::DenoisedVisibilityFormat` | Persistent previous/current scalar denoised visibility history, reset with renderer temporal history. |
| `ToneMappedSceneColor`, `EncodedSceneColor`, `BackBuffer` | Presentation | Existing presentation/backbuffer formats | Display-only outputs; they do not feed lighting. |
| `HistoryResetState` | Temporal frame state | Constant data | Shared reset signal for exposure, upscalers, and future denoisers. |

## Geometry And Temporal Conventions

| Signal or rule | Owner | Consumers | Convention | Reference mapping |
| --- | --- | --- | --- | --- |
| Primary GBuffer normal | `GBufferPS.hlsl`, `GBufferUtils.hlsli` | Deferred lighting, indirect primary rays, provider inputs | World-space unit shading normal after tangent-space normal-map evaluation; stored in `GBufferNormal.xyz`. | NRD/Streamline require provider-visible normal convention; Sparkle local decision is world-space normals until a provider asks for conversion. |
| Ray-hit normal and basis | `RayTracingMaterialHit.hlsli` | Indirect hit shading, future reference path | World-space geometric normal from interpolated mesh normal and inverse-transpose matrix; tangent is orthonormalized against the normal; bitangent is `tangentSign * cross(normal, tangent)`; sampled tangent normal is transformed through the same `Geometry/Basis.hlsli` path used by primary shading. | RTXPT/Falcor-style hit-surface records; PBRT separates geometric surface data from shading normal evaluation. |
| Face orientation and two-sidedness | Primary material sampling, ray-hit material reconstruction | GBuffer, indirect diffuse/specular, ray-hit direct lighting | One-sided ray hits reject backfaces; two-sided ray hits flip the evaluated shading basis on backfaces. Primary raster shading flips the final shading normal for `SV_IsFrontFace == false`; direct-shadow alpha/two-sided parity remains Stage 5. | RTXPT/Falcor hit-surface orientation with a documented local direct-shadow exception. |
| Normal-map handedness | `MaterialNormal.hlsli`, `Geometry/Basis.hlsli` | Primary and ray-hit material decode | Normal maps decode to tangent-space `[-1, 1]` XY with reconstructed positive Z; mesh tangent `.w` supplies bitangent sign. | glTF/Filament-style tangent-space normal-map workflow; Sparkle local import must keep tangent sign. |
| Ray-origin and self-intersection policy | Shadow, indirect diffuse, indirect specular ray tracing passes | Ray queries and future reference rays | Rays start from world position plus normal bias and a small positive `TMin`; indirect specular additionally nudges along ray direction for grazing safety. | PBRT motivates explicit ray-offset/self-intersection policy; Sparkle uses local numeric bias constants until Stage 7/13 replaces them with a shared reference/path policy. |
| Depth | `GBufferDeviceZ`, scene depth, `UpscalerInputContract` | Sky/deferred reconstruction, DLSS/provider inputs, future denoisers | Device depth in current projection convention; provider contract labels it `ReversedDeviceDepth`, and Streamline receives `depthInverted = true`. Linear viewZ is not stored globally; any provider needing viewZ must convert once in its builder. | NRD distinguishes device depth/viewZ inputs; Streamline consumes tagged depth plus inverted-depth metadata. |
| Motion vectors | `GBufferVS.hlsl`, `MotionVector.hlsli`, `UpscalerInputContract` | DLSS/provider inputs and future denoisers | 2D pixel delta, current minus previous, generated from unjittered current and previous clip positions; zero when history is invalid. Streamline conversion happens once through `BuildDlssMotionVectorScale`, which flips to provider direction and normalizes by render extent. | Streamline motion-vector scale/tagging; NRD temporal inputs require an explicit direction/unit convention. |
| Jitter | `PerTemporalConstantBufferData`, `GBufferVS.hlsl`, `StreamlineDlssRuntime.cpp` | Rasterization, DLSS/provider constants | Current clip position is jittered for rasterization; motion-vector endpoints are unjittered. Provider jitter is converted from NDC to pixels in the Streamline bridge. | Streamline jitter constants; Falcor-style temporal state handoff. |
| History reset | `PerTemporalConstantBufferData`, `TemporalFrameState`, frame pipeline/provider contracts | Exposure, DLSS, future denoisers | `HistoryValid == 0` resets motion vectors to zero and sets provider reset state; resize/camera-cut invalidation is represented by the shared temporal history state before provider evaluation. | NRD/Streamline temporal reset expectations; Sparkle local history state owns the reset bit. |

Future staged signals:

- Stage 11/11A owns allocation and provider contracts for noisy indirect radiance, demodulated indirect radiance, indirect hit distance, lobe id, confidence/variance, albedo, and specular/F0 guide resources.
- Stage 13 owns final cleanup of stale signal descriptions and any redundant provider-facing outputs.

Implementation notes:

- Keep format constants effect-local. Add a new constant only when multiple call sites in the same effect would otherwise duplicate a literal.
- Prefer deleting stale descriptions over adding bridge layers.
- Presentation samples only `FinalSceneColor` and `Exposure`; lighting and sky stay linear HDR before presentation.

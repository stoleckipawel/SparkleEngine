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
| `GBufferBaseColor`, `GBufferNormal`, `GBufferMaterial`, `GBufferEmissive`, `GBufferSubsurface`, `GBufferDeviceZ`, `GBufferMotionVector` | GBuffer | `GBufferFormats::*` | Existing material, geometry, depth, and motion products; Stage 2 still owns dielectric F0 parity. |
| `Exposure` and exposure history | Exposure pass / frame pipeline | `R32G32B32A32_Float` | R adapted exposure, G average luminance, B target exposure, A previous exposure. |
| `ShadowVisibilitySignalRaw`, `ShadowVisibilitySignalScratch` | Shadow denoiser registration | `ShadowDenoiseContract::PackedVisibilitySignalFormat` | Packed `float4(visibility, hitDistance, confidence, maxDistance)`. |
| `ShadowVisibilityDenoised`, `ShadowVisibilityDenoisedHistory` | Shadow denoiser registration | `ShadowDenoiseContract::DenoisedVisibilityFormat` | Scalar visibility only. |
| `ToneMappedSceneColor`, `EncodedSceneColor`, `BackBuffer` | Presentation | Existing presentation/backbuffer formats | Display-only outputs; they do not feed lighting. |
| `HistoryResetState` | Temporal frame state | Constant data | Shared reset signal for exposure, upscalers, and future denoisers. |

Future staged signals:

- Stage 11/11A owns allocation and provider contracts for noisy indirect radiance, demodulated indirect radiance, indirect hit distance, lobe id, confidence/variance, albedo, and specular/F0 guide resources.
- Stage 13 owns expanded debug/capture coverage for denoiser and reconstruction signals.

Implementation notes:

- Keep format constants effect-local. Add a new constant only when multiple call sites in the same effect would otherwise duplicate a literal.
- Prefer deleting stale descriptions over adding bridge layers.
- Presentation samples only `FinalSceneColor` and `Exposure`; lighting and sky stay linear HDR before presentation.

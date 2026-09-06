# Renderer Reconstruction And Generation

Status: Renderer post-processing feature-family index

Scope: route resolution reconstruction/upscaling and the independently absent generated-frame capability

| Document | Open it for |
| --- | --- |
| [Resolution, Sampling, And Anti-Aliasing](ResolutionSamplingAndAntiAliasing.md) | output/render extents, sample policy, provider selection, resize invalidation, and explicit absent AA/dynamic-resolution modes |
| [Image Reconstruction And Upscaling](ImageReconstructionAndUpscaling.md) | linear and provider-backed reconstruction, extent/history contracts, readiness, fallback, and output identity |
| [Frame Generation](FrameGeneration.md) | explicit negative capability boundary and the additional pacing, latency, UI, provenance, and presentation obligations |

Reconstruction produces one resolved image for the submitted frame. Frame generation would create additional presented frames and cannot be inferred from an upscaler or latency provider. The parent [Post Processing](../README.md) dossier owns shared stage order.

# Renderer Debug Views

Status: current feature dossier; source-backed, not release approval or executable evidence

Snapshot: implementation behavior rechecked 2026-09-06 against source revision `d236da11`; Renderer/RHI/shader source is unchanged from the earlier `8414b5dc` audit

Scope: `REN-DBG-01` through `REN-DBG-04` and `REN-POST-10`; current debug-visualization modes, source products, tone/output interaction, viewport resolution, limitations, and the boundary to the target presentation architecture

Target architecture: [View Modes And Show Flags](ViewModesAndShowFlags.md) and [Debug View Presentation Architecture](PresentationArchitecture.md)

Delivery authority: [Debug View Presentation Delivery Plan](../../../../../../Plans/Renderer/DebugViewPresentation.md)

Feature acceptance: [Debug View Presentation — Acceptance](Acceptance.md)

Code and executable build configuration remain authoritative. Reinspect every listed owner and behavior before using this dated snapshot for implementation or release claims.

## Source-Backed Snapshot And Problem

`r.ViewMode` exposes 16 current modes:

| Capability | Modes | Current product and boundary |
| --- | --- | --- |
| `REN-DBG-01` final/material | Lit, Wireframe | Lit uses the selected lighting path. Wireframe changes raster GBuffer fill and has no equivalent ray-GBuffer wireframe frontend. |
| `REN-DBG-02` GBuffer | Diffuse/Base Color, Normal, Roughness, Metallic, Emissive, Ambient Occlusion, Subsurface Color, Subsurface Strength | Reads one shared GBuffer product/channel and writes visualization color. |
| `REN-DBG-03` lighting | Direct Diffuse, Direct Specular, Direct Subsurface, Indirect Diffuse, Indirect Specular | Reads the independently documented [Direct](../Lighting/DirectLighting.md) or [Indirect](../Lighting/IndirectLighting.md) lobe. There is no volumetric-lighting debug product. |
| `REN-DBG-04` scene diagnostics | GPU Scene Instances | Visualizes GPU-scene instance identity rather than a lighting/material quantity. |

View mode is currently process/session state read by `RenderViewBuilder`; it is not part of the 27 persisted Renderer settings. Availability depends on the active frontend and produced resources. A selectable name is not evidence that its quantity is numerically correct or meaningful in every mode.

Sparkle currently has one unconditional presentation path:

```text
Scene or debug color
    -> exposure multiplication
    -> selected Reinhard/ACES tone curve
    -> output encoding
    -> viewport or back buffer
```

The path is visible in these current owners:

- [`Passes/PostProcessing/PostProcessing.cpp`](../../../../../../../Engine/Renderer/Private/Passes/PostProcessing/PostProcessing.cpp) schedules debug visualization and then presentation.
- [`Passes/Debug/VisualizeBuffers.cpp`](../../../../../../../Engine/Renderer/Private/Passes/Debug/VisualizeBuffers.cpp) overwrites `FinalSceneColor` for non-lit views.
- [`Passes/Presentation/Presentation.cpp`](../../../../../../../Engine/Renderer/Private/Passes/Presentation/Presentation.cpp) always schedules `ToneMappingPass` and `OutputEncodingPass`.
- [`Passes/Presentation/ToneMapping.hlsl`](../../../../../../../Engine/Assets/Shaders/Passes/Presentation/ToneMapping.hlsl) always multiplies by the exposure texture and applies the selected tone mapper.
- [`Passes/Debug/VisualizeBuffers.hlsl`](../../../../../../../Engine/Assets/Shaders/Passes/Debug/VisualizeBuffers.hlsl) maps HDR lighting and emissive values with `x / (1 + x)` before the global tone mapper runs.
- [`Viewport/ViewportContracts.h`](../../../../../../../Engine/Renderer/Public/Viewport/ViewportContracts.h) already separates view kind, selection, requested outputs, extent, and exposure; it has no `RenderFeatureFlags`, view-mode, or show-flag field.
- [`View/RenderViewBuilder.cpp`](../../../../../../../Engine/Renderer/Private/View/RenderViewBuilder.cpp) reads `CVarRenderViewMode` directly, so the current view mode is process-wide rather than resolved from a viewport request.

The producer-local HDR preview curve followed by unconditional exposure and tone mapping causes double mapping for HDR diagnostic views. The same global presentation step also changes bounded quantities and false colors: a roughness value, encoded normal, or instance-ID palette no longer reaches the display as the visualization shader authored it.

Exposure metering itself is already ordered usefully. It reads the original scene color before the debug pass overwrites final color. The implementation should preserve that ownership so diagnostics do not drive eye adaptation.

## Ownership, Failure, And Evidence

- The selected `RenderViewMode` is view intent; `VisualizeBuffers` owns conversion from the requested source product to visualization color; the shared presentation chain owns exposure, tone mapping, and output encoding.
- Missing or inapplicable source products must not be presented as a valid diagnostic result. Requested mode, resolved mode/product, viewport/frame/scene identity, and presentation transform need capture-visible provenance.
- Current `REN-POST-10` state is Partial because diagnostic color still passes through the common presentation transform. [Debug View Presentation Architecture](PresentationArchitecture.md) defines scene-referred HDR versus exact display-linear domains, while [View Modes And Show Flags](ViewModesAndShowFlags.md) owns viewport resolution; neither is implemented merely because this dossier links it.
- `REN-E18` owns representative output and transform checks for all modes. `REN-E21` owns product attribution and capture provenance. The acceptance contract owns the completion verdict.

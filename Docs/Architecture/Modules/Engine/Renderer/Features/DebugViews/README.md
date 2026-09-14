# Renderer Debug Views

**Status:** current feature dossier; source-backed, not release approval or executable evidence

**Snapshot:** implementation behavior rechecked 2026-09-15 against committed source `56048ce7e2764daa5bba9adce9feee7823fe7bd3` plus the current scoped working tree; the per-view visualization migration is source-present but uncompiled

**Scope:** `REN-DBG-01` through `REN-DBG-04` and `REN-POST-10`; current debug-visualization modes, source products, tone/output interaction, viewport resolution, limitations, and the boundary to the target presentation architecture

**Target architecture:** [View Modes And Show Flags](ViewModesAndShowFlags.md) and [Debug View Presentation Architecture](PresentationArchitecture.md)

**Delivery authority:** [Debug View Presentation Delivery Plan](Plan.md)

**Design precedent:** [Debug View Presentation Research](Research.md)

**Feature acceptance:** [Debug View Presentation — Acceptance](Acceptance.md)

**Current readiness:** **40/100** — debug modes and capture are reachable, but exact signal-domain presentation, per-viewport isolation, unavailable state, provenance, interpretation, and observer-cost proof remains open. See [Current Feature Readiness](../../../../../../Acceptance/CurrentReadiness.md#renderer).

## At A Glance

| You can inspect | Current behavior | Principal problem |
| --- | --- | --- |
| Lit and raster wireframe | selects the normal lit result or raster fill mode | wireframe has no ray-GBuffer equivalent |
| GBuffer channels | visualizes base color, normal, roughness, metallic, emissive, AO, and subsurface values | bounded/exact quantities still pass through exposure and tone mapping |
| lighting lobes | visualizes five direct/indirect products | HDR lobes can be mapped twice before display |
| GPU-scene instances | maps instance identity to diagnostic color | capture lacks the complete resolved-mode/presentation provenance contract |

The feature exists and is useful, but its presentation is only partial: source selection and visualization happen before the unconditional shared exposure, tone-mapping, and encoding chain. The target design separates scene-referred HDR diagnostics from exact display-linear diagnostics and resolves intent per viewport.

Code and executable build configuration remain authoritative. Reinspect every listed owner and behavior before using this dated snapshot for implementation or release claims.

## Source-Backed Snapshot And Problem

`Visualization` exposes exactly 16 concrete Renderer debug/final choices. The Editor's ordered view-mode list remains a UI concern. In the current working source, one exhaustive Editor viewport resolver maps every ordered mode to `ViewportRenderRequest::ActiveVisualization`; Reference maps to Lit and remains unavailable. `RenderViewBuilder` freezes that concrete value into `RenderView` and derives the existing shader scalar from it. The private `r.ReferencePathTracer` composition CVar remains transitional until the next clean-break stage.

| Capability | Modes | Current product and boundary |
| --- | --- | --- |
| `REN-DBG-01` final/material | Lit, Wireframe | Lit uses the selected lighting path. Wireframe changes raster GBuffer fill and has no equivalent ray-GBuffer wireframe frontend. |
| `REN-DBG-02` GBuffer | Diffuse/Base Color, Normal, Roughness, Metallic, Emissive, Ambient Occlusion, Subsurface Color, Subsurface Strength | Reads one shared GBuffer product/channel and writes visualization color. |
| `REN-DBG-03` lighting | Direct Diffuse, Direct Specular, Direct Subsurface, Indirect Diffuse, Indirect Specular | Reads the independently documented [Direct](../Lighting/DirectLighting/README.md) or [Indirect](../Lighting/IndirectLighting/README.md) lobe. There is no volumetric-lighting debug product. |
| `REN-DBG-04` scene diagnostics | GPU Scene Instances | Visualizes GPU-scene instance identity rather than a lighting/material quantity. |

Editor view-mode labels, ordering, and selection remain in `EditorViewportSession`. Its mode event stays inside Editor: `UI` resolves the preset and updates the `ViewportPanel`-owned request, which increments its existing generation when the visualization changes. Application and Renderer receive only that ordinary request. `Renderer::SubmitVisualization`, `VisualizationCommand`, `CVarVisualization`, and their callback/execution branch are removed. This completes the source shape of the visualization slice, not the show-flag architecture or build/runtime evidence.

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
- [`Viewport/ViewportContracts.h`](../../../../../../../Engine/Renderer/Public/Viewport/ViewportContracts.h) carries the concrete per-view `ActiveVisualization` beside view kind, selection, requested outputs, extent, and exposure; it does not yet contain `RenderShowFlagSet`.
- [`View/RenderViewBuilder.cpp`](../../../../../../../Engine/Renderer/Private/View/RenderViewBuilder.cpp) copies the request visualization into immutable frame View state and derives the focused shader scalar without consulting a process-global selection CVar.

The producer-local HDR preview curve followed by unconditional exposure and tone mapping causes double mapping for HDR diagnostic views. The same global presentation step also changes bounded quantities and false colors: a roughness value, encoded normal, or instance-ID palette no longer reaches the display as the visualization shader authored it.

Exposure metering itself is already ordered usefully. It reads the original scene color before the debug pass overwrites final color. The implementation should preserve that ownership so diagnostics do not drive eye adaptation.

## Ownership, Failure, And Evidence

- The selected `Visualization` value and `RenderShowFlagSet` are concrete per-view Renderer behavior; `VisualizeBuffers` owns conversion from the selected source product to visualization color; `BuildRenderFrameGraph` owns topology selection; the shared presentation chain owns exposure, tone mapping, and output encoding.
- Missing or inapplicable source products must not be presented as a valid diagnostic result. Requested mode, resolved mode/product, viewport/frame/scene identity, and presentation transform need capture-visible provenance.
- Current `REN-POST-10` state is Partial because diagnostic color still passes through the common presentation transform. [Debug View Presentation Architecture](PresentationArchitecture.md) defines scene-referred HDR versus exact display-linear domains, while [View Modes And Show Flags](ViewModesAndShowFlags.md) owns viewport resolution; neither is implemented merely because this dossier links it.
- `REN-E18` owns representative output and transform checks for all modes. `REN-E21` owns product attribution and capture provenance. The acceptance contract owns the completion verdict.

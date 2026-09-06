# Debug View Presentation Capability Snapshot

Status: capability snapshot; not release approval or executable evidence

Snapshot: 2026-08-28 at committed `master` revision `20814381`; source and executable build configuration were unchanged from implementation revision `99af6d5b`

Scope: current debug-visualization, tone-mapping, output-encoding, viewport-contract, and view-mode resolution surfaces relevant to the target presentation contract

Architecture authority: [Debug View Presentation Contract](DebugViewPresentation.md)

Delivery authority: [Debug View Presentation Delivery Plan](../../../../Plans/Renderer/DebugViewPresentation.md)

Acceptance authority: [Debug View Presentation Acceptance Contract](../../../../Acceptance/Renderer/DebugViewPresentation.md)

Code and executable build configuration remain authoritative. Reinspect every listed owner and behavior before using this dated snapshot for implementation or release claims.

## Source-Backed Snapshot And Problem

Sparkle currently has one unconditional presentation path:

```text
Scene or debug color
    -> exposure multiplication
    -> selected Reinhard/ACES tone curve
    -> output encoding
    -> viewport or back buffer
```

The path is visible in these current owners:

- [`Passes/PostProcessing/PostProcessing.cpp`](../../../../../Engine/Renderer/Private/Passes/PostProcessing/PostProcessing.cpp) schedules debug visualization and then presentation.
- [`Passes/Debug/VisualizeBuffers.cpp`](../../../../../Engine/Renderer/Private/Passes/Debug/VisualizeBuffers.cpp) overwrites `FinalSceneColor` for non-lit views.
- [`Passes/Presentation/Presentation.cpp`](../../../../../Engine/Renderer/Private/Passes/Presentation/Presentation.cpp) always schedules `ToneMappingPass` and `OutputEncodingPass`.
- [`Passes/Presentation/ToneMapping.hlsl`](../../../../../Engine/Assets/Shaders/Passes/Presentation/ToneMapping.hlsl) always multiplies by the exposure texture and applies the selected tone mapper.
- [`Passes/Debug/VisualizeBuffers.hlsl`](../../../../../Engine/Assets/Shaders/Passes/Debug/VisualizeBuffers.hlsl) maps HDR lighting and emissive values with `x / (1 + x)` before the global tone mapper runs.
- [`Viewport/ViewportContracts.h`](../../../../../Engine/Renderer/Public/Viewport/ViewportContracts.h) already separates view kind, selection, requested outputs, extent, and exposure; it has no `RenderFeatureFlags`, view-mode, or show-flag field.
- [`View/RenderViewBuilder.cpp`](../../../../../Engine/Renderer/Private/View/RenderViewBuilder.cpp) reads `CVarRenderViewMode` directly, so the current view mode is process-wide rather than resolved from a viewport request.

The producer-local HDR preview curve followed by unconditional exposure and tone mapping causes double mapping for HDR diagnostic views. The same global presentation step also changes bounded quantities and false colors: a roughness value, encoded normal, or instance-ID palette no longer reaches the display as the visualization shader authored it.

Exposure metering itself is already ordered usefully. It reads the original scene color before the debug pass overwrites final color. The implementation should preserve that ownership so diagnostics do not drive eye adaptation.

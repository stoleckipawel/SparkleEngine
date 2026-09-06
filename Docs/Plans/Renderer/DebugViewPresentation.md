# Debug View Presentation Delivery Plan

Status: implementation plan; not proof of implementation or acceptance

Scope: clean-break delivery of per-view show flags, presentation routing, editor controls, capture metadata, and focused verification

Architecture authority: [Debug View Presentation Contract](../../Architecture/Modules/Engine/Renderer/DebugViewPresentation.md)

Acceptance authority: [Debug View Presentation Acceptance Contract](../../Acceptance/Renderer/DebugViewPresentation.md)

This plan owns implementation slices and their delivery order. It does not redefine signal domains, show-flag semantics, display routing, or final acceptance.

## Delivery Plan

These are implementation workstreams. If delivered with the Scene/View/Frame refactor, they follow that document's atomic-migration contract rather than landing as dual old/new systems.

### Slice 1: establish show-flag ownership

1. Inventory the producer, consumer, disabled behavior, and graph impact of every proposed initial flag; remove any entry without a real first consumer.
2. Extend `ViewportRenderRequest` in one clean break with `RenderViewMode` and `RenderShowFlagOverrides`, keep selection and `RenderOutputFlags` in their existing categories, and stop using `CVarRenderViewMode` as normal renderer input.
3. Add the fixed `RenderShowFlag` enum, bitset operations, exhaustive editor metadata, kind baselines, and mode presets.
4. Resolve the immutable final set in `RenderViewBuilder`; expose only narrow values to graph keys, pass parameters, and `ViewUniformData`.
5. Add focused tests for precedence, non-overlapping overrides, mode selection, reset, invalid values, and two independent viewports.

### Slice 2: make presentation obey resolved flags

1. Replace the tone-map-only uniform with explicit exposure and tone-mapper application values derived from `RenderView.ShowFlags`.
2. Rename the pass, shader class, parameters, resource names, typed registration, and graph label together if the clean-break `DisplayMappingPass` rename is accepted. Generated map/library identity follows the shader type and registration; do not reintroduce authored cooked-package identity.
3. Preserve current scene-based exposure metering/history and keep output encoding unconditional.
4. Remove local HDR preview curves from emissive and lighting contribution modes.
5. Keep one exact mapping for scalar, normal, material-color, and instance-ID modes.
6. Delete the unused duplicate `Debug/ViewModes.hlsli` path if repository-wide search still proves it has no caller.
7. Make render-to-output coordinate selection explicit and deterministic for mismatched extents.

### Slice 3: wire scene, lighting, and editor flags

1. Wire each accepted initial flag to its named owner without per-pass CVar reads or duplicate booleans.
2. Classify its graph impact and update only the necessary topology key or narrow pass state.
3. Add the separate categorized Show menu, Reset Show Flags action, Custom indicator, and source tooltip.
4. Persist only per-viewport override deltas and publish them in the viewport request.
5. Extend capture metadata and replay input with the resolved set and force provenance.

### Slice 4: prove the complete contract

1. Add CPU tests for exhaustive mode presets, metadata coverage, flag precedence, invalid combinations, and per-viewport isolation.
2. Add shader/readback known-value tests for all four exposure/tone-mapper combinations and both output encodings.
3. Add D3D12 and Vulkan viewport smokes that switch among one HDR mode, one exact scalar mode, normal, instance IDs, and customized/reset presentation flags.
4. Verify the first scene/lighting/editor flags visually and through focused readback or pass-execution evidence.
5. Capture evidence with equal and unequal render/output extents and prove captured flags replay identically.

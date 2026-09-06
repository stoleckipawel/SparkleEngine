# Debug View Presentation Contract

Status: target proposal; design-only, not implementation proof
Date: 2026-08-18
Last source reconciliation: 2026-08-28 at committed `master` revision `20814381`; source and executable build configuration are unchanged from implementation revision `99af6d5b`
Responsibility: per-view show-flag resolution, presentation-domain classification, and display routing

## Decision

Sparkle should adopt an Unreal-like show-flag model at Sparkle's scale:

- `RenderViewMode` is the higher-level visualization preset.
- A typed `RenderShowFlagSet` is resolved for each viewport and copied into the immutable one-frame `RenderView`.
- Show flags control implemented rendering features for that view; they are not global renderer state and are not scalability settings.
- The initial presentation flags are `Exposure` and `Tonemapper`. They are independent because disabling the filmic curve alone does not make a visualization exact.
- Target output encoding is an invariant, not a show flag, and always runs.

Every view-mode preset must also declare the domain of the color it publishes:

- **Scene-referred HDR** is exposed and tone mapped exactly once, then output encoded.
- **Display-linear exact** bypasses exposure and the tone curve, then is output encoded.

Output encoding is mandatory for both domains. An sRGB swap chain still needs a linear-to-sRGB transfer for a display-linear diagnostic value to appear correctly; bypassing that transfer would not be a more exact visualization.

The stock view-mode preset owns the correct `Exposure` and `Tonemapper` defaults. A user may deliberately override an exposed flag for the current viewport, as in Unreal, but that creates a visibly customized view and the full resolved flag set must travel with captures. Selecting another mode reapplies that mode's presentation defaults. Correctness therefore never depends on a process-global editor toggle or a name heuristic.

This proposal does not change code. The current implementation remains authoritative until the delivery and acceptance work below is complete.

## Authority Boundary

This document owns the target show-flag semantics, signal-domain classification, and presentation routing for view modes. The [current renderer navigation overlay](WholeRepositoryMap.md#current-renderer-navigation-overlay) records the broader implemented placement of view mode, resolved display settings, view state, and narrow pass inputs. The generic resolved view-feature values become the resolved `RenderShowFlagSet`; during that refactor, place them in `RenderView` and focused pass parameters rather than preserving access through a broad runtime context.

[Editor Viewport Camera Architecture](EditorViewportCamera.md) continues to own per-viewport exposure overrides. [Renderer and RHI Architecture Boundary](RendererRhiBoundary.md) continues to own frame-graph and backend responsibility. Engineering requirements and evidence rules remain in the [Engineering Standards](../Engineering/Standards/README.md).

## Problem And Current Evidence

Sparkle currently has one unconditional presentation path:

```text
Scene or debug color
    -> exposure multiplication
    -> selected Reinhard/ACES tone curve
    -> output encoding
    -> viewport or back buffer
```

The path is visible in these current owners:

- [`Passes/PostProcessing/PostProcessing.cpp`](../../Engine/Renderer/Private/Passes/PostProcessing/PostProcessing.cpp) schedules debug visualization and then presentation.
- [`Passes/Debug/VisualizeBuffers.cpp`](../../Engine/Renderer/Private/Passes/Debug/VisualizeBuffers.cpp) overwrites `FinalSceneColor` for non-lit views.
- [`Passes/Presentation/Presentation.cpp`](../../Engine/Renderer/Private/Passes/Presentation/Presentation.cpp) always schedules `ToneMappingPass` and `OutputEncodingPass`.
- [`Passes/Presentation/ToneMapping.hlsl`](../../Engine/Assets/Shaders/Passes/Presentation/ToneMapping.hlsl) always multiplies by the exposure texture and applies the selected tone mapper.
- [`Passes/Debug/VisualizeBuffers.hlsl`](../../Engine/Assets/Shaders/Passes/Debug/VisualizeBuffers.hlsl) maps HDR lighting and emissive values with `x / (1 + x)` before the global tone mapper runs.
- [`Viewport/ViewportContracts.h`](../../Engine/Renderer/Public/Viewport/ViewportContracts.h) already separates view kind, selection, requested outputs, extent, and exposure; it has no `RenderFeatureFlags`, view-mode, or show-flag field.
- [`View/RenderViewBuilder.cpp`](../../Engine/Renderer/Private/View/RenderViewBuilder.cpp) reads `CVarRenderViewMode` directly, so the current view mode is process-wide rather than resolved from a viewport request.

The producer-local HDR preview curve followed by unconditional exposure and tone mapping causes double mapping for HDR diagnostic views. The same global presentation step also changes bounded quantities and false colors: a roughness value, encoded normal, or instance-ID palette no longer reaches the display as the visualization shader authored it.

Exposure metering itself is already ordered usefully. It reads the original scene color before the debug pass overwrites final color. The implementation should preserve that ownership so diagnostics do not drive eye adaptation.

## Terms And Invariants

The invariants in this section define unmodified stock view-mode presets. A deliberate presentation-flag override follows the custom behavior table later in this document and forfeits the stock HDR/exact claim until reset.

### Scene-referred HDR

The producer publishes linear scene color or a linear lighting contribution. Values may exceed `1.0`. Presentation applies the viewport's resolved exposure and selected tone curve once.

Invariants:

- no local Reinhard, ACES, or other display curve runs in the visualization producer;
- the scene's exposure state is used, allowing lighting contributions to be compared under the same exposure;
- manual exposure remains available through the existing viewport/display settings;
- changing the tone mapper is expected to change these views.

### Display-linear exact

The producer publishes the final bounded visualization in linear display space. For the current SDR path, RGB must be finite and in `[0, 1]` before output encoding. Examples include a scalar replicated to RGB, a decoded normal mapped from `[-1, 1]` to `[0, 1]`, and a stable false-color palette.

"Exact" means no content-dependent exposure, eye adaptation, color grading, or filmic curve changes that authored display-linear value. It does not mean copying linear numbers directly into an sRGB-encoded target. Quantization and target-format gamut remain physical output limitations.

Invariants:

- exposure and the tone curve are both bypassed;
- the visualization producer owns the one intentional mapping into `[0, 1]`;
- changing exposure or the selected tone mapper cannot change the decoded displayed result;
- output transfer encoding still runs once;
- values outside the displayable range must use an explicit visualization mapping or raw capture, not accidental clamping presented as exactness.

### Output encoding

Output encoding converts display-linear color to the transfer function required by the target. In the current implementation that is linear or sRGB. It is independent of whether scene tone mapping ran.

This separation must remain visible in the frame graph and resource names:

```text
selected view color -> display mapping -> DisplayLinearColor -> output encoding -> EncodedColor
```

## Show-Flag Contract

### Purpose and boundary

A show flag is a typed, per-view rendering switch with one real producer and one or more real consumers. It answers whether an implemented feature contributes to this view. It does not select an algorithm or quality tier, report hardware capability, request a render product, or enable an alternative architecture.

This follows the useful part of Unreal's contract: show flags live with the view, view modes are higher-level presets that manipulate them, and scalability remains a separate system. Sparkle should not copy Unreal's hundreds of flags, dynamic custom-flag registration, string mutation API, or shipping permutations before local workloads require them.

The target first set is deliberately small:

| Category | Flag | Meaning when disabled | First consumer |
| --- | --- | --- | --- |
| Scene | `Sky` | Do not composite the sky into this view. | Sky scheduling/composite |
| Lighting | `DirectLighting` | Publish zero direct-light contributions. | Direct-light scheduling/resolve |
| Lighting | `IndirectLighting` | Publish zero indirect-light contributions. | Indirect-light scheduling/resolve |
| Lighting | `Shadows` | Use fully visible shadow terms while retaining lighting. | Shadow planning/resolve |
| Post Processing | `Exposure` | Use a neutral `1.0` exposure multiplier in display mapping. Exposure metering/history may remain warm. | `DisplayMappingPass` |
| Post Processing | `Tonemapper` | Bypass the selected filmic curve and use linear display mapping. | `DisplayMappingPass` |
| Editor | `DebugOverlay` | Omit renderer debug overlays. | Debug-overlay scheduling |
| Editor | `GizmoOverlay` | Omit editor gizmos. | Editor-overlay scheduling |

`Exposure` is intentionally broader than Unreal's `EyeAdaptation` label: Sparkle's exact-view promise must bypass both automatic and manual exposure application. The automatic exposure mode and its tuning remain viewport display settings; the show flag only decides whether the resolved exposure affects this view.

Bulk menu entries such as **Lighting: All** or **Post Processing: All** operate on masks; they are not additional runtime flags. Add a new flag only when the same change supplies its owner, consumer, disabled behavior, UI metadata, and focused test. Do not add speculative flags for geometry classes or post effects Sparkle does not implement.

### Typed representation and metadata

Use a fixed enum plus bitset, with one exhaustive metadata table for editor name, category, and help text. The table is static renderer/editor integration data, not an extensible registry:

```cpp
enum class RenderShowFlag : std::uint8_t
{
	Sky,
	DirectLighting,
	IndirectLighting,
	Shadows,
	Exposure,
	Tonemapper,
	DebugOverlay,
	GizmoOverlay,
	Count,
};

struct RenderShowFlagOverrides final
{
	RenderShowFlagSet Enable;
	RenderShowFlagSet Disable;
};
```

`Enable` and `Disable` are sparse deltas and must not overlap. Storing deltas rather than a copied full mask lets new defaults take effect without rewriting every saved viewport. Runtime passes consume typed bits or focused booleans; no pass performs string lookup, iterates metadata, or reaches back into editor state.

### Resolution and ownership

The application/editor owns editable show-flag overrides for each stable `ViewportId`. The renderer owns default/preset resolution. `RenderViewBuilder` resolves them once for the submitted frame:

```text
RenderViewKind baseline
        |
        v
RenderViewMode preset set/clear masks
        |
        v
per-viewport enable/disable overrides
        |
        v
dependency and capability validation
        |
        +--> immutable RenderView.ShowFlags
        +--> focused graph key bits, pass parameters, and view uniforms
```

The order is normative:

1. `RenderViewKind` establishes Game, Scene, Preview, Thumbnail, or Debug defaults.
2. The exhaustive `RenderViewMode` preset establishes visualization and presentation defaults.
3. Explicit overrides for that viewport apply last.
4. The renderer validates parent/child dependencies and unavailable capabilities; it reports an invalid combination rather than silently mutating unrelated flags.
5. The final bitset is immutable for the frame and is the only value passes consume.

Selecting a new view mode clears overrides for the flags that mode explicitly owns, then applies the new preset; unrelated choices such as gizmo visibility remain. A later manual change to a mode-owned flag is allowed, but the viewport shows a **Custom** indicator and offers **Reset Show Flags**. This keeps the normal path deterministic while retaining Unreal-like expert control.

The resolved set is per view, never a process-global renderer singleton. Two viewports may therefore render the same scene with different flags. Console variables continue to own scalability, implementation selection, and developer forcing; if a CVar forces a show flag for diagnostics, that force is resolved before publication and is visible in diagnostics/capture metadata rather than read independently by passes.

### Editor experience

Keep the current view-mode dropdown task oriented and add a separate **Show** menu beside it, matching Unreal's useful separation between visualization presets and feature visibility. The first menu groups only implemented entries under **Scene**, **Lighting**, **Post Processing**, and **Editor**. Each entry is a checkbox backed by the current viewport's override delta.

The menu also provides:

- **Reset Show Flags**, which removes the viewport's explicit deltas and returns to kind/mode defaults;
- category-level **Show All** and **Hide All** actions that edit the same individual bits;
- a visible **Custom** marker on the toolbar whenever resolved mode-owned flags differ from the stock preset;
- a tooltip that identifies whether the current value came from the view-kind baseline, view-mode preset, viewport override, or diagnostic CVar force.

Do not expose raw bit indices, hexadecimal masks, CVar names, or graph-rebuild terminology in the normal UI. The View Mode menu remains the default workflow; Show is progressive disclosure for investigation and capture setup.

### View modes are presets over flags

Each `RenderViewMode` entry owns a preset containing its signal domain plus explicit set/clear masks. The stock presets set both presentation flags for scene-referred HDR modes and clear both for display-linear exact modes:

| Stock mode domain | `Exposure` | `Tonemapper` | Contract |
| --- | --- | --- | --- |
| `SceneReferredHdr` | On | On | Shared scene exposure and one selected tone curve |
| `DisplayLinearExact` | Off | Off | Producer-authored display-linear value |

This is a default contract, not a hidden hard-coded branch. If a user changes either presentation flag, the display-mapping pass follows the resolved flags and the view is marked customized:

| `Exposure` | `Tonemapper` | Display-mapping behavior |
| --- | --- | --- |
| On | On | Apply resolved exposure, then selected tone curve |
| On | Off | Apply resolved exposure, then linear mapping; HDR values may clip at the display boundary |
| Off | On | Apply selected tone curve with neutral exposure |
| Off | Off | Preserve producer-authored display-linear RGB |

Only the unmodified stock `DisplayLinearExact` preset may claim exact displayed diagnostics. Custom presentation flags are useful for investigation, but the **Custom** indicator and captured flag set prevent that result from being mistaken for the canonical view-mode contract. Output encoding remains unconditional in every row and is not exposed in the Show menu.

### Clean break from process-global view-mode state

The earlier generic `RenderFeatureFlags` representation no longer exists. `ViewportRenderRequest` already keeps selection and requested render products distinct from view kind, extent, and exposure. Do not recreate that removed mixed-purpose bitset when show flags are added.

The remaining clean break moves `RenderViewMode` from direct `CVarRenderViewMode` consumption into the viewport/view request and adds only the typed show-flag override value. The final request contains one mode, one show-flag override value, and one requested-output value with no compatibility alias or dual representation. A CVar may remain only as an explicit developer force resolved at the boundary, not as the renderer's normal source of truth. This aligns with the Scene/View/Frame target: editable intent crosses the viewport request boundary, while the complete resolved set lives only in the one-frame `RenderView`.

## Selected Architecture

The renderer keeps one presentation topology and resolves view-mode and show-flag policy before any pass executes:

```text
             active RenderViewMode + viewport show-flag overrides
                                      |
                                      v
                          RenderViewBuilder resolution
                                      |
                       immutable RenderView.ShowFlags
                                      |
                                      v
                         selected producer-domain color
                                      |
                                      v
            DisplayMappingPass(Exposure, Tonemapper, signal domain)
                                      |
                             DisplayLinearColor
                                      |
                             output encoding
                                      |
                          viewport / back buffer
```

### One preset and classification owner

Add one renderer-private, exhaustive view-mode preset resolver. Conceptually:

```cpp
enum class ViewModeSignalDomain : std::uint8_t
{
	SceneReferredHdr,
	DisplayLinearExact,
};

struct RenderViewModePreset final
{
	ViewModeSignalDomain SignalDomain;
	RenderShowFlagSet SetFlags;
	RenderShowFlagSet ClearFlags;
};

RenderViewModePreset ResolveRenderViewModePreset(RenderViewMode viewMode) noexcept;
```

The exact names may follow implementation review, but the responsibilities may not split:

- `RenderViewMode` remains the stable mode identity shared with the editor and shaders.
- The renderer-private resolver is the only mode-to-signal-domain and mode-to-show-flag-default table.
- Editor labels, mode menu categories, and the Show menu do not repeat view-mode preset policy.
- The display-mapping shader receives focused resolved booleans; it does not maintain a second mode-to-presentation-policy list.
- An unknown or `Count` value is rejected by the narrow resolver rather than silently becoming an exact diagnostic.

A polymorphic view-mode hierarchy, per-mode CVar, dynamic show-flag registry, and public presentation API are unnecessary for the current closed enums. The exhaustive preset table and fixed show-flag metadata table are the two static authorities: one owns rendering policy and one owns editor presentation.

### Display-mapping pass

Generalize the current `ToneMappingPass` into the one pass that produces display-linear color. A clean-break rename to `DisplayMappingPass` and `DisplayMapping.hlsl` is preferred because the pass can perform either scene mapping or identity mapping. Rename `ToneMappedSceneColor` to `DisplayLinearColor` in the same change; do not retain aliases.

The pass receives:

- selected view color;
- current exposure texture;
- selected tone mapper;
- resolved `ViewModeSignalDomain` for producer-contract validation/diagnostics;
- focused `ApplyExposure` and `ApplyTonemapper` booleans derived once from `RenderView.ShowFlags`.

Its stock behavior is closed and simple:

| Signal domain | Exposure | Tone curve | Output |
| --- | --- | --- | --- |
| `SceneReferredHdr` | Apply current scene exposure | Apply selected tone mapper once | Display-linear color |
| `DisplayLinearExact` | Bypass | Bypass | Producer-authored display-linear color |

The four explicit custom combinations are defined in the show-flag table above; there is no ambiguous `EnableToneMapping` boolean that also hides exposure behavior. `OutputEncodingPass` remains a separate unconditional consumer. Do not encode sRGB inside debug visualization shaders, and do not add a second exact-view copy path around output encoding.

### Visualization producer

Keep `VisualizeBuffers` as the single visualization producer for the current GBuffer and lighting modes. It should:

- output raw non-negative HDR values for modes classified as `SceneReferredHdr`;
- output one deliberate display-linear mapping for exact modes;
- contain no generic exposure or tone-mapper policy;
- use explicit source-to-output coordinate mapping when render and output extents differ;
- use point selection for exact buffer values so upscaling does not invent category IDs, material values, or false colors.

The currently unused `Debug/ViewModes.hlsli` duplicates preview mappings implemented by `Passes/Debug/VisualizeBuffers.hlsl`. If implementation-time search still finds no consumer of `ViewMode::Resolve` or its preview helpers, remove that duplicate and its broad include rather than updating two visualization authorities.

The existing post-reconstruction placement can remain for the first slice: lit output is reconstructed normally, then an active debug visualization overwrites it at output extent. This avoids temporal reconstruction, sharpening, or scene post effects changing exact views. The visualization pass must not assume its GBuffer and lighting inputs have the same extent as `FinalSceneColor`.

### Exposure state

Continue computing exposure from the original lit `SceneColor`, even while an exact diagnostic is visible. Exact modes ignore that exposure at display mapping, but keeping the history warm avoids a reset or brightness jump when the user returns to an HDR mode.

HDR contribution modes should use the same scene exposure rather than meter only the selected contribution. This makes direct and indirect components comparable. A user who needs fixed evidence can use the existing manual viewport exposure; a second debug-exposure system is out of scope.

### Frame-graph and capture integration

Show-flag resolution is per frame, but not every flag is a graph-rebuild key. `Exposure`, `Tonemapper`, `DebugOverlay`, and `GizmoOverlay` should flow through focused pass parameters or pass enable conditions. A scene/lighting flag enters the graph topology key only if changing it genuinely changes resource creation or pass lifetime; otherwise the graph remains stable and the resolved flag controls scheduling or contribution. The implementation inventory must classify each flag and prove that a switch takes effect on the next submitted frame.

Do not query show flags by name or branch on a broad bitset inside shader inner loops. Resolve them into the narrow booleans, zero-input bindings, or pass scheduling decisions owned by each consumer.

Every captured viewport product records:

- base `RenderViewKind` and `RenderViewMode`;
- the complete resolved `RenderShowFlagSet` and explicit override masks;
- `ViewModeSignalDomain` plus resolved exposure/tone-mapper application;
- exposure mode/value, tone-mapper selection, and output encoding;
- whether a diagnostic CVar force changed a flag.

This metadata makes stock and customized views distinguishable and permits deterministic capture replay. A capture labelled only `GBufferRoughness` is insufficient once show-flag overrides exist.

## Initial Mode Classification

This table classifies the signals produced by current shaders. If a producer changes meaning, its entry and tests must change in the same changelist.

| `RenderViewMode` | Domain | Stock presentation flags | Producer requirement | Rationale |
| --- | --- | --- | --- | --- |
| `Lit` | Scene-referred HDR | Exposure + Tonemapper | Publish composed linear scene color | Normal rendering needs exposure and the selected tone mapper. |
| `Wireframe` | Scene-referred HDR | Exposure + Tonemapper | Keep current lit wireframe shading | The current mode changes rasterization while retaining lit color. If it later becomes a fixed palette, reclassify it explicitly. |
| `GBufferDiffuse` | Display-linear exact | Neither | Saturated linear base color | Material input should not change with exposure or filmic contrast. |
| `GBufferNormal` | Display-linear exact | Neither | Normalize and map `[-1, 1]` to `[0, 1]` | Axis colors are the diagnostic encoding. |
| `GBufferRoughness` | Display-linear exact | Neither | Replicate the bounded scalar | A stored `0.5` must remain the `0.5` visualization value before output transfer. |
| `GBufferMetallic` | Display-linear exact | Neither | Replicate the bounded scalar | Same scalar contract. |
| `GBufferEmissive` | Scene-referred HDR | Exposure + Tonemapper | Publish raw non-negative emissive color | Emissive is an unbounded scene-light quantity; remove the local `PreviewHdr` curve. |
| `GBufferAmbientOcclusion` | Display-linear exact | Neither | Replicate the bounded scalar | Occlusion is diagnostic data, not scene luminance. |
| `GBufferSubsurfaceColor` | Display-linear exact | Neither | Saturated linear material color | Material input should remain stable. |
| `GBufferSubsurfaceStrength` | Display-linear exact | Neither | Replicate the bounded scalar | Same scalar contract. |
| `DirectDiffuse` | Scene-referred HDR | Exposure + Tonemapper | Publish raw non-negative contribution | It must receive the shared scene exposure and one tone curve. |
| `DirectSpecular` | Scene-referred HDR | Exposure + Tonemapper | Publish raw non-negative contribution | Same lighting-contribution contract. |
| `DirectSubsurface` | Scene-referred HDR | Exposure + Tonemapper | Publish raw non-negative contribution | Same lighting-contribution contract. |
| `IndirectDiffuse` | Scene-referred HDR | Exposure + Tonemapper | Publish raw non-negative contribution | Same lighting-contribution contract. |
| `IndirectSpecular` | Scene-referred HDR | Exposure + Tonemapper | Publish raw non-negative contribution | Same lighting-contribution contract. |
| `GpuSceneInstances` | Display-linear exact | Neither | Publish the stable hashed instance palette | IDs and false colors must not vary with exposure or tone mapper. |

The Show menu exposes `Exposure` and `Tonemapper` separately. These overrides are intentionally explicit and visibly custom; they do not create more view-mode enum values. A genuinely different producer interpretation, such as a future `EmissiveRangeHeatmap`, still needs a clearly named mode because a show flag must not silently change what source data means.

## Reference-Engine Findings

These sources are already in Sparkle's [external renderer reference set](ExternalReferences/ExternalRendererComparison.md). They are precedent, not local authority.

### Unreal Engine

Epic documents `FEngineShowFlags` as bits stored in the view family, intended for artists and developers to customize/debug rendering. The API explicitly states that view modes are higher level and can manipulate show flags before use, and that scalability belongs to console variables instead. `FSceneViewFamily` owns `EngineShowFlags`, while `FEditorViewportClient` owns current and previous show-flag sets for an editor viewport.

The editor presents View Mode and Show Flags as separate neighboring controls. Show flags are grouped by purpose, and the Post Processing group exposes Eye Adaptation and Tonemapper independently. Epic's buffer-visualization records also carry per-visualization `bApplyAutoExposure` intent rather than assuming every buffer uses the lit presentation path.

Transferable lesson:

- a view owns a resolved set of flags; passes do not read a mutable global editor state;
- view modes provide coherent presets above those individual feature switches;
- editor categories and a resettable Show menu make the power discoverable without crowding the task-oriented View Mode menu;
- exposure and the tone curve are distinct presentation decisions;
- show flags are not a substitute for scalability or backend capability policy.

Sparkle should adopt that architecture with a fixed local flag enum, immutable per-frame resolution, and only implemented consumers. It should not copy Unreal's material-driven visualization registry, dynamic custom flags, string mutation path, full category surface, or renderer scale.

Primary sources:

- Epic, [`FEngineShowFlags`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/FEngineShowFlags)
- Epic, [`FSceneViewFamily`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/FSceneViewFamily)
- Epic, [`FEditorViewportClient`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Editor/UnrealEd/FEditorViewportClient)
- Epic, [Viewport Toolbar: View Mode and Show Flag Options](https://dev.epicgames.com/documentation/en-us/unreal-engine/viewport-toolbar#viewporttoolbarviewmodeandshowflagoptions)
- Epic, [Viewport Show Flags](https://dev.epicgames.com/documentation/en-us/unreal-engine/viewport-show-flags-in-unreal-engine)
- Epic, [`FBufferVisualizationData`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/FBufferVisualizationData)
- Epic, [`FBufferVisualizationData::Record`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/FBufferVisualizationData/Record)
- Epic, [`FEngineShowFlags::EShowFlag`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/FEngineShowFlags/EShowFlag)

### NVIDIA RTXPT And Donut

RTXPT, built on Donut/NVRHI, names pre-tone-map and post-tone-map stages explicitly. Its tone-mapping pass accepts an `enabled` value; disabling it bypasses color grading and the tone curve. The shader applies auto exposure outside that `enabled` branch, however, so disabling only the curve is not sufficient for an exact visualization.

Transferable lesson:

- tone mapping should be an explicit presentation stage with a linear path;
- pre- and post-tone-map domains need clear names;
- exact mode policy must bypass both exposure and the curve, not reuse one ambiguous `EnableToneMapping` boolean.

Sparkle should not copy RTXPT's global UI checkbox or sample-level orchestration. Donut's reusable pass boundary is useful, while Sparkle needs the per-mode resolver that the sample does not provide.

Primary sources at reviewed revisions:

- NVIDIA RTXPT, [`Sample.cpp` at `f08d1c7`](https://github.com/NVIDIA-RTX/RTXPT/blob/f08d1c739071e0faad0c7c274d861124c511abab/Rtxpt/Sample.cpp#L2189-L2208)
- NVIDIA RTXPT, [`ToneMapping.ps.hlsli` at `f08d1c7`](https://github.com/NVIDIA-RTX/RTXPT/blob/f08d1c739071e0faad0c7c274d861124c511abab/Rtxpt/ToneMapper/ToneMapping.ps.hlsli#L133-L172)
- NVIDIA Donut, [`ToneMappingPasses.cpp` at `bfdebdd`](https://github.com/NVIDIA-RTX/Donut/blob/bfdebdd7dd5455c503b2737a1967a4ef651c145b/src/render/ToneMappingPasses.cpp)

### AMD Cauldron

Cauldron's reviewed tone-mapping shader has a linear operator that still applies exposure and a separate raw pass-through when exposure is negative. Its color-conversion shader is a separate stage that applies the target display transform and transfer function.

Transferable lesson:

- exposure, tone curve, and output conversion are separate concerns;
- a linear tone-mapper option is not the same as an exact pass-through when exposure still changes the signal;
- display conversion remains necessary after either mapping choice.

Sparkle should adopt the separation, not Cauldron's sentinel exposure value, numeric tone-mapper switch, or backend-specific duplication.

Primary sources at the revision already pinned by Sparkle's shader research:

- AMD Cauldron, [`Tonemapping.hlsl` at `b92d559`](https://github.com/GPUOpen-LibrariesAndSDKs/Cauldron/blob/b92d559bd083f44df9f8f42a6ad149c1584ae94c/src/DX12/shaders/Tonemapping.hlsl)
- AMD Cauldron, [`ColorConversionPS.hlsl` at `b92d559`](https://github.com/GPUOpen-LibrariesAndSDKs/Cauldron/blob/b92d559bd083f44df9f8f42a6ad149c1584ae94c/src/DX12/shaders/ColorConversionPS.hlsl)

## Rejected Alternatives

### Disable tone mapping for every non-lit mode

Rejected because emissive and lighting contributions are scene-referred HDR. A raw copy would clip or make their interpretation depend on the output target.

### Keep local `PreviewHdr` and disable only the global tone curve

Rejected because it hides the source magnitude behind a hard-coded curve and still leaves exposure as a possible second transform. HDR modes should use one owned scene-display mapping.

### Add a global "debug views bypass tone mapping" checkbox

Rejected because one process-global value cannot describe multiple viewports and makes captures ambiguous. The accepted alternative is two typed per-view show flags with stock mode defaults, a visible Custom state, reset behavior, and captured metadata.

### Implement show flags as CVars

Rejected because CVars are process-wide policy and already own scalability, algorithm selection, and developer forcing. View-local show flags must be resolved into `RenderView`; a diagnostic CVar may force a flag only through that resolver and must be reported as such.

### Copy exact modes after output encoding

Rejected because visualization shaders author linear values while encoded targets expect a transfer function. Writing linear values into an sRGB-encoded intermediate produces the wrong displayed result and couples debug code to back-buffer format.

### Maintain separate frame graphs for HDR and exact modes

Rejected for the current slice because view-mode switching is per-frame state and the existing graph can carry one resolved mapping mode. A topology split would add rebuild and lifetime complexity without improving the output contract.

### Add a dynamic visualization or show-flag registry

Rejected because the current closed enums have one renderer owner and a small number of implemented consumers. Exhaustive preset and metadata tables are easier to audit. Revisit extension only when a real module must contribute a flag without modifying the renderer; do not build that mechanism speculatively.

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

## Acceptance Contract

Implementation is accepted only when all of the following are demonstrated:

- every `RenderViewMode` other than `Count` has exactly one signal domain and one explicit show-flag preset;
- every `RenderShowFlag` other than `Count` has exactly one metadata entry, a real producer/consumer path, deterministic disabled behavior, and classified graph impact;
- no generic `RenderFeatureFlags` definition is reintroduced; selection, view-mode presets, requested outputs, and show flags each have one target representation;
- the active mode and show-flag overrides arrive through the viewport/view request; normal pass behavior does not read `CVarRenderViewMode` or another process-global substitute;
- `RenderViewBuilder` is the only resolver, and two viewports can resolve different show-flag sets without global-state races or cross-talk;
- stock `Lit`, `GBufferEmissive`, and direct/indirect lighting modes enable Exposure and Tonemapper; changing exposure compensation or tone mapper changes them;
- those stock HDR modes contain no producer-local display curve and are tone mapped once;
- stock roughness, metallic, ambient occlusion, subsurface strength, normals, diffuse/subsurface material colors, and instance palette views disable Exposure and Tonemapper; changing exposure compensation or tone mapper does not change their decoded display-linear pixels;
- toggling either presentation flag takes effect on the next submitted frame, marks the viewport Custom, and Reset Show Flags restores the stock mode result;
- all four `Exposure`/`Tonemapper` combinations match their specified behavior, including intentional linear-path clipping for HDR values;
- changing `Sky`, `DirectLighting`, `IndirectLighting`, `Shadows`, `DebugOverlay`, and `GizmoOverlay` affects only the current viewport and matches each flag's documented disabled behavior;
- sRGB and linear output configurations both produce the expected displayed values without double encoding;
- exact views do not pass through temporal reconstruction, sharpening, bloom, color grading, or future scene post effects;
- exposure continues to meter the lit scene while an exact view is active, and returning to `Lit` does not reset adaptation history;
- render/output extent mismatch has an explicit point-sampling result with no out-of-bounds reads;
- captured viewport-product metadata and replay record the view kind, mode, resolved flags, override deltas, presentation values, output encoding, and any CVar force; stock and Custom captures are distinguishable;
- no runtime pass reads editor state, resolves a flag by string, or independently consults a CVar for behavior already represented by the resolved show-flag set;
- the Show menu exposes only implemented flags, grouped by purpose, with working reset/category actions and no raw bit/CVar UI;
- documentation does not describe the encoded viewport image as a raw GBuffer dump;
- representative D3D12 and Vulkan results agree within the target format's quantization tolerance;
- `git diff --check`, the selected shader cook, focused tests, and the required backend smokes report exact commands and results.

## Non-Goals

- A raw GPU-resource inspector or lossless GBuffer export. Those require a separate typed capture product that preserves format, range, and metadata.
- HDR10/PQ or wide-gamut output implementation. The contract is compatible with future output encodings, but current code supports linear and sRGB.
- An Unreal-sized show-flag catalog, runtime custom-flag registration, string-based flag API, or plugin extension point.
- Replacing renderer scalability/quality CVars, capability reporting, requested outputs, or view modes with show flags.
- Debug-only exposure history or a second tone-mapper setting.
- Skipping normal scene rendering work while exact views are active. That optimization needs measurement and a separate graph-lifetime decision after correctness is established.

## Handoff Summary

The recommended fix is an Unreal-like but proportionate per-view show-flag system. `RenderViewMode` supplies the coherent preset, `RenderViewBuilder` resolves viewport overrides into immutable flags, and `DisplayMappingPass` applies Exposure and Tonemapper independently while output encoding always runs. Stock exact modes therefore preserve authored scalar/normal/ID values, stock HDR modes remain readable under the scene display transform, and deliberate expert overrides are visible, resettable, and reproducible in captures.

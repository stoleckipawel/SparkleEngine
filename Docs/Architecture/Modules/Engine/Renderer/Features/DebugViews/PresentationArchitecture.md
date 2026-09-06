# Debug View Presentation Architecture

Status: target architecture; design-only, not implementation proof
Date: 2026-08-18
Last source reconciliation: 2026-08-28 at committed `master` revision `20814381`; source and executable build configuration are unchanged from implementation revision `99af6d5b`
Responsibility: debug-view signal-domain classification, display mapping, output encoding, visualization producer requirements, and capture-visible presentation state

## Decision

Sparkle should classify every debug-view producer as either scene-referred HDR or display-linear exact, then route both domains through one explicit display-mapping and output-encoding path. The per-viewport [`RenderViewMode` and show-flag contract](ViewModesAndShowFlags.md) supplies the selected mode and the resolved `Exposure` and `Tonemapper` flags; this document owns what those inputs mean for presentation.

Every view-mode preset must also declare the domain of the color it publishes:

- **Scene-referred HDR** is exposed and tone mapped exactly once, then output encoded.
- **Display-linear exact** bypasses exposure and the tone curve, then is output encoded.

Output encoding is mandatory for both domains. An sRGB swap chain still needs a linear-to-sRGB transfer for a display-linear diagnostic value to appear correctly; bypassing that transfer would not be a more exact visualization.

The stock view-mode preset owns the correct `Exposure` and `Tonemapper` defaults. A deliberate override creates a customized view and the full resolved flag set travels with captures. Correctness therefore never depends on a process-global editor toggle or a name heuristic.

This proposal does not change code. The current implementation remains authoritative until the [delivery plan](../../../../../../Plans/Renderer/DebugViewPresentation.md) is implemented and the adjacent [feature acceptance contract](Acceptance.md) passes.

## Authority Boundary

This document owns signal-domain classification, presentation routing, display mapping, output encoding, and visualization producer requirements. [Debug View Modes And Show Flags](ViewModesAndShowFlags.md) owns view-mode intent, flag semantics, resolution, and editor controls. The [current renderer navigation overlay](../../../../../WholeRepositoryMap.md#current-renderer-navigation-overlay) records the broader implemented placement of view mode, resolved display settings, view state, and narrow pass inputs.

[Editor Viewport Camera Architecture](../../../../../Decisions/EditorViewportCamera.md) continues to own per-viewport exposure overrides. [Renderer and RHI Architecture Boundary](../../../../../Decisions/RendererRhiBoundary.md) continues to own frame-graph and backend responsibility. The [Debug View Presentation Delivery Plan](../../../../../../Plans/Renderer/DebugViewPresentation.md) owns implementation slices; the adjacent [feature acceptance contract](Acceptance.md) owns feature proof. Engineering requirements and evidence rules are routed by the [Engineering task map](../../../../../../Engineering/README.md#choose-by-task).

## Implementation Snapshot

The current presentation path, owners, and observed double-mapping problem live in the [Debug Views feature dossier](README.md). Refresh that dossier before beginning delivery; this architecture owns the target correction, not the current implementation claim.

## Terms And Invariants

The invariants in this section define unmodified stock view-mode presets. A deliberate presentation-flag override follows the custom behavior table in [View Modes And Show Flags](ViewModesAndShowFlags.md) and forfeits the stock HDR/exact claim until reset.

### Scene-Referred HDR

The producer publishes linear scene color or a linear lighting contribution. Values may exceed `1.0`. Presentation applies the viewport's resolved exposure and selected tone curve once.

Invariants:

- no local Reinhard, ACES, or other display curve runs in the visualization producer;
- the scene's exposure state is used, allowing lighting contributions to be compared under the same exposure;
- manual exposure remains available through the existing viewport/display settings;
- changing the tone mapper is expected to change these views.

### Display-Linear Exact

The producer publishes the final bounded visualization in linear display space. For the current SDR path, RGB must be finite and in `[0, 1]` before output encoding. Examples include a scalar replicated to RGB, a decoded normal mapped from `[-1, 1]` to `[0, 1]`, and a stable false-color palette.

"Exact" means no content-dependent exposure, eye adaptation, color grading, or filmic curve changes that authored display-linear value. It does not mean copying linear numbers directly into an sRGB-encoded target. Quantization and target-format gamut remain physical output limitations.

Invariants:

- exposure and the tone curve are both bypassed;
- the visualization producer owns the one intentional mapping into `[0, 1]`;
- changing exposure or the selected tone mapper cannot change the decoded displayed result;
- output transfer encoding still runs once;
- values outside the displayable range must use an explicit visualization mapping or raw capture, not accidental clamping presented as exactness.

### Output Encoding

Output encoding converts display-linear color to the transfer function required by the target. In the current implementation that is linear or sRGB. It is independent of whether scene tone mapping ran.

This separation must remain visible in the frame graph and resource names:

```text
selected view color -> display mapping -> DisplayLinearColor -> output encoding -> EncodedColor
```

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

### One Preset And Classification Owner

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

### Display-Mapping Pass

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

The four explicit custom combinations are defined by [View Modes And Show Flags](ViewModesAndShowFlags.md); there is no ambiguous `EnableToneMapping` boolean that also hides exposure behavior. `OutputEncodingPass` remains a separate unconditional consumer. Do not encode sRGB inside debug visualization shaders, and do not add a second exact-view copy path around output encoding.

### Visualization Producer

Keep `VisualizeBuffers` as the single visualization producer for the current GBuffer and lighting modes. It should:

- output raw non-negative HDR values for modes classified as `SceneReferredHdr`;
- output one deliberate display-linear mapping for exact modes;
- contain no generic exposure or tone-mapper policy;
- use explicit source-to-output coordinate mapping when render and output extents differ;
- use point selection for exact buffer values so upscaling does not invent category IDs, material values, or false colors.

The currently unused `Debug/ViewModes.hlsli` duplicates preview mappings implemented by `Passes/Debug/VisualizeBuffers.hlsl`. If implementation-time search still finds no consumer of `ViewMode::Resolve` or its preview helpers, remove that duplicate and its broad include rather than updating two visualization authorities.

The existing post-reconstruction placement can remain for the first slice: lit output is reconstructed normally, then an active debug visualization overwrites it at output extent. This avoids temporal reconstruction, sharpening, or scene post effects changing exact views. The visualization pass must not assume its GBuffer and lighting inputs have the same extent as `FinalSceneColor`.

### Exposure State

Continue computing exposure from the original lit `SceneColor`, even while an exact diagnostic is visible. Exact modes ignore that exposure at display mapping, but keeping the history warm avoids a reset or brightness jump when the user returns to an HDR mode.

HDR contribution modes should use the same scene exposure rather than meter only the selected contribution. This makes direct and indirect components comparable. A user who needs fixed evidence can use the existing manual viewport exposure; a second debug-exposure system is out of scope.

### Frame-Graph And Capture Integration

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

## External Precedent

[Debug View Presentation Precedent](../../../../../../Research/GraphicsArchitecture/DebugViewPresentationPrecedent.md) owns the NVIDIA RTXPT/Donut and AMD Cauldron findings behind the exposure, tone, and output separation. The decision, invariants, local type shape, and rejected presentation alternatives remain here.

## Rejected Alternatives

### Disable Tone Mapping For Every Non-Lit Mode

Rejected because emissive and lighting contributions are scene-referred HDR. A raw copy would clip or make their interpretation depend on the output target.

### Keep Local `PreviewHdr` And Disable Only The Global Tone Curve

Rejected because it hides the source magnitude behind a hard-coded curve and still leaves exposure as a possible second transform. HDR modes should use one owned scene-display mapping.

### Add a global "debug views bypass tone mapping" checkbox

Rejected because one process-global value cannot describe multiple viewports and makes captures ambiguous. The accepted alternative is two typed per-view show flags with stock mode defaults, a visible Custom state, reset behavior, and captured metadata.

### Copy Exact Modes After Output Encoding

Rejected because visualization shaders author linear values while encoded targets expect a transfer function. Writing linear values into an sRGB-encoded intermediate produces the wrong displayed result and couples debug code to back-buffer format.

### Maintain Separate Frame Graphs For HDR And Exact Modes

Rejected for the current slice because view-mode switching is per-frame state and the existing graph can carry one resolved mapping mode. A topology split would add rebuild and lifetime complexity without improving the output contract.

## Non-Goals

- A raw GPU-resource inspector or lossless GBuffer export. Those require a separate typed capture product that preserves format, range, and metadata.
- HDR10/PQ or wide-gamut output implementation. The contract is compatible with future output encodings, but current code supports linear and sRGB.
- Debug-only exposure history or a second tone-mapper setting.
- Skipping normal scene rendering work while exact views are active. That optimization needs measurement and a separate graph-lifetime decision after correctness is established.

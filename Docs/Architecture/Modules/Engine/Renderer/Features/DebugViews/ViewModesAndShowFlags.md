# Debug View Modes And Show Flags

Status: target architecture; design-only, not implementation proof

Date: 2026-09-07

Responsibility: per-viewport view-mode intent, typed show-flag semantics, preset and override resolution, editor controls, and immutable publication into `RenderView`

## Contract

Sparkle should adopt an Unreal-like show-flag model at Sparkle's scale:

- `RenderViewMode` is the higher-level visualization preset.
- A typed `RenderShowFlagSet` is resolved for each viewport and copied into the immutable one-frame `RenderView`.
- Show flags control implemented rendering features for that view; they are not global renderer state and are not scalability settings.
- View-mode presets own their default flags and presentation domain; deliberate per-viewport overrides produce a visibly customized view.

The [Debug View Presentation Architecture](PresentationArchitecture.md) owns scene-referred versus display-linear signal domains, display mapping, output encoding, and producer requirements. This document owns how a viewport selects and resolves the mode and flags that feed that presentation contract. The adjacent [acceptance contract](Acceptance.md) owns feature proof.

## Show-Flag Boundary

A show flag is a typed, per-view rendering switch with one real producer and one or more real consumers. It answers whether an implemented feature contributes to this view. It does not select an algorithm or quality tier, report hardware capability, request a render product, or enable an alternative architecture.

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

## Typed Representation And Metadata

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

## Resolution And Ownership

The application/editor owns editable show-flag overrides for each stable `ViewportId`. The renderer owns default and preset resolution. `RenderViewBuilder` resolves them once for the submitted frame:

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

Selecting a new view mode clears overrides for the flags that mode explicitly owns, then applies the new preset; unrelated choices such as gizmo visibility remain. A later manual change to a mode-owned flag is allowed, but the viewport shows a **Custom** indicator and offers **Reset Show Flags**. This keeps the normal path deterministic while retaining expert control.

The resolved set is per view, never a process-global renderer singleton. Two viewports may therefore render the same scene with different flags. Console variables continue to own scalability, implementation selection, and developer forcing; if a CVar forces a show flag for diagnostics, that force is resolved before publication and is visible in diagnostics and capture metadata rather than read independently by passes.

## Editor Experience

Keep the current view-mode dropdown task oriented and add a separate **Show** menu beside it. The first menu groups only implemented entries under **Scene**, **Lighting**, **Post Processing**, and **Editor**. Each entry is a checkbox backed by the current viewport's override delta.

The menu also provides:

- **Reset Show Flags**, which removes the viewport's explicit deltas and returns to kind and mode defaults;
- category-level **Show All** and **Hide All** actions that edit the same individual bits;
- a visible **Custom** marker whenever resolved mode-owned flags differ from the stock preset;
- a tooltip identifying whether the value came from the view-kind baseline, view-mode preset, viewport override, or diagnostic CVar force.

Do not expose raw bit indices, hexadecimal masks, CVar names, or graph-rebuild terminology in the normal UI. The View Mode menu remains the default workflow; Show is progressive disclosure for investigation and capture setup.

## View Modes As Presets

Each `RenderViewMode` entry owns a preset containing its signal domain plus explicit set and clear masks. The stock presets set both presentation flags for scene-referred HDR modes and clear both for display-linear exact modes:

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

Only the unmodified stock `DisplayLinearExact` preset may claim exact displayed diagnostics. Custom presentation flags are useful for investigation, but the **Custom** indicator and captured flag set prevent that result from being mistaken for the canonical view-mode contract. Output encoding remains unconditional and is not exposed in the Show menu.

## Clean Break From Process-Global State

The earlier generic `RenderFeatureFlags` representation no longer exists. `ViewportRenderRequest` already keeps selection and requested render products distinct from view kind, extent, and exposure. Do not recreate that removed mixed-purpose bitset when show flags are added.

The clean break moves `RenderViewMode` from direct `CVarRenderViewMode` consumption into the viewport and view request and adds only the typed show-flag override value. The final request contains one mode, one show-flag override value, and one requested-output value with no compatibility alias or dual representation. A CVar may remain only as an explicit developer force resolved at the boundary, not as the renderer's normal source of truth.

## External Precedent

[Debug View Presentation Precedent](../../../../../../Research/GraphicsArchitecture/DebugViewPresentationPrecedent.md) owns the Unreal Engine findings behind the selected view-mode and show-flag model.

## Rejected Alternatives

- **Global debug-view or show-flag state:** rejected because one process-global value cannot describe multiple viewports and makes captures ambiguous.
- **Show flags implemented as independently consumed CVars:** rejected because CVars are process-wide policy and already own scalability, algorithm selection, and developer forcing.
- **Dynamic flag registry:** rejected because the current closed enum has one renderer owner and a small implemented consumer set. Revisit only when a real module must contribute flags without modifying Renderer.
- **One flag per algorithm or quality choice:** rejected because those choices belong to rendering settings, selectors, and capability resolution rather than feature visibility.

## Non-Goals

- An Unreal-sized flag catalog, runtime custom-flag registration, string mutation API, or plugin extension point.
- Replacing Renderer scalability settings, capability reporting, requested outputs, or view modes with show flags.
- Defining visualization shader mappings or display transfer behavior; those remain in [Debug View Presentation Architecture](PresentationArchitecture.md).

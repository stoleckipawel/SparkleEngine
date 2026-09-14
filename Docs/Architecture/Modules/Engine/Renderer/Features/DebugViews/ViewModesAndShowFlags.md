# Editor View Modes And Renderer Show Flags

**Status:** target architecture; design-only, not implementation proof

**Date:** 2026-09-13

**Responsibility:** Editor-owned view-mode presets and UX, concrete per-view Renderer visualization and show flags, immutable View consumption, and strict exclusion of frontend identity from Renderer and RHI contracts

**Current readiness:** **40/100** for the existing debug-view feature; the per-view `Visualization` slice is source-present but uncompiled and unexercised, while show flags, presentation correction, optional overrides, and acceptance evidence remain unimplemented. See [Current Feature Readiness](../../../../../../Acceptance/CurrentReadiness.md#renderer).

## Decision

Sparkle adopts an Unreal-inspired separation without copying Unreal's scale:

- a **view mode** is a frontend-owned preset and label;
- `Visualization` is the concrete Renderer selection for the color/product visualization;
- `RenderShowFlagSet` is the concrete, typed set of rendering contributions enabled for one view;
- Renderer settings and console variables remain global algorithm, scalability, or developer policy and are not the ordinary transport for per-view selection;
- `RenderViewKind` identifies the producer of the view (`Scene`, `Game`, and so on), not how that view is rendered.

An Editor view-mode selection resolves once to a `Visualization` value plus a finite set of show-flag changes. Those concrete Renderer facts cross through the ordinary `ViewportRenderRequest`, are frozen into the corresponding `RenderView`, and are consumed by graph composition and passes. `EditorViewportViewMode`, labels, ordering, menu groups, icons, and selection identity never cross that boundary.

The Reference Path Tracer follows this same path. Its preset selects `Visualization::Lit` and enables `RenderShowFlag::ReferencePathTracer`. That flag is the sole semantic selector for the alternate middle-frame recipe. There is no `RenderViewMode` mirror, `r.ReferencePathTracer` selector, dedicated render command, or RHI representation in the target architecture.

The [Debug View Presentation Architecture](PresentationArchitecture.md) owns scene-referred versus display-linear signal domains, display mapping, output encoding, and producer requirements. This document owns how a viewport selects and resolves the mode and flags that feed that presentation contract. The adjacent [acceptance contract](Acceptance.md) owns feature proof.

## Why Show Flags Belong To The View

Show flags answer which implemented rendering contributions participate in a particular view. Two viewports may therefore use different modes and flags in the same process without racing over global state. This is the important part of the Unreal precedent: show flags are stored with view-family state, while higher-level view modes manipulate them. Epic explicitly separates show flags from scalability CVars.

The same split applies in Sparkle:

```text
Editor or Game viewport owner
    owns UI mode, labels, selection, and editable overrides
                    |
                    v
frontend preset resolution
    produces Visualization + RenderShowFlagSet
                    |
                    v
ViewportRenderRequest
    concrete per-view Renderer semantics only
                    |
          +---------+---------+
          |                   |
          v                   v
frame topology key       immutable RenderView
          |                   |
          v                   v
Lit / Reference choice   pass and shader consumption
          \___________________/
                    |
                    v
       shared viewport presentation
```

`ViewportRenderRequest` is available before frame-graph topology is refreshed, so the topology-affecting subset can be compared before `RenderView` construction. `RenderViewBuilder` copies the same accepted request values into the immutable View; it does not rediscover the Editor preset or read global selection CVars. The built graph retains only the resolved topology bits required to detect reconstruction.

## Show-Flag Boundary

A show flag is a fixed Renderer semantic with one owner, at least one production consumer, deterministic disabled behavior, and declared graph impact. Most flags enable or suppress a contribution. A small number may choose mutually exclusive composition when that is the natural per-view rendering semantic; `ReferencePathTracer` is the first such topology flag.

Show flags do not select backend APIs, hardware capabilities, quality tiers, sample counts, denoisers, ReSTIR algorithms, or other implementation policy. They do not report capability and they do not replace requested render products.

The implemented set grows only with real consumers. The Reference migration introduces exactly one flag; the remaining rows are frozen target vocabulary for later presentation work and must not exist in code before the same change wires their named consumer:

| Delivery | Category | Flag | Meaning when disabled | Graph impact / first consumer |
| --- | --- | --- | --- | --- |
| Reference selector clean break | Rendering | `ReferencePathTracer` | Use the ordinary Lit middle recipe. | Topology; `FramePipeline::BuildRenderFrameGraph` selects exactly one middle. |
| Later presentation stage | Scene | `Sky` | Do not composite the sky into this view. | Scheduling/composite. |
| Later presentation stage | Lighting | `DirectLighting` | Publish zero direct-light contributions. | Scheduling/resolve. |
| Later presentation stage | Lighting | `IndirectLighting` | Publish zero indirect-light contributions. | Scheduling/resolve. |
| Later presentation stage | Lighting | `Shadows` | Use fully visible shadow terms while retaining lighting. | Planning/resolve. |
| Later presentation stage | Post Processing | `Exposure` | Use neutral exposure in display mapping. | Pass parameter. |
| Later presentation stage | Post Processing | `Tonemapper` | Bypass the filmic curve and use linear display mapping. | Pass parameter. |
| Later presentation stage | Editor | `DebugOverlay` | Omit Renderer debug overlays. | Scheduling. |
| Later presentation stage | Editor | `GizmoOverlay` | Omit Editor gizmos. | Editor overlay scheduling. |

`Exposure` is intentionally broader than Unreal's `EyeAdaptation` label: Sparkle's exact-view promise must bypass both automatic and manual exposure application. The automatic exposure mode and its tuning remain viewport display settings; the show flag only decides whether the resolved exposure affects this view.

Add a flag only in the change that supplies its owner, consumer, disabled behavior, preset effect, topology classification, and focused defect-detecting check. Bulk UI actions such as **Lighting: All** are masks, not extra flags. Do not create flags for unimplemented features.

## Typed Representation And Metadata

Use a fixed enum plus compact bitset. At the Reference selector stage its complete vocabulary is deliberately only:

```cpp
enum class RenderShowFlag : std::uint8_t
{
	ReferencePathTracer,
	Count,
};
```

`RenderShowFlagSet` supplies only construction, equality, `Set`, and `IsEnabled` operations actually consumed by the request, topology comparison, and feature. It has no registry, reflection, string lookup, logging, serialization, shader-global mask, or metadata table. A later Show-menu change adds a flag and its Editor metadata only with the production consumer. Sparse `Enable`/`Disable` override deltas likewise do not enter production until the optional Show menu owns them.

The ordinary Renderer request carries the final resolved `RenderShowFlagSet`, never the Editor preset or saved deltas. This gives Renderer one immutable answer for the frame while allowing frontend defaults to evolve without adding UI identity to Renderer.

The concrete request surface is intentionally small:

```cpp
struct ViewportRenderRequest final
{
	// existing identity, kind, extent, selection, output, and display fields
	Visualization ActiveVisualization = Visualization::Lit;
	RenderShowFlagSet ShowFlags = DefaultRenderShowFlags();
};
```

These fields are not a UI leak: both are Renderer semantics consumed for that specific view. `EditorViewportViewMode` remains absent from Renderer, capture, frame-graph settings, shader constants, and RHI. Passes receive only the flag or focused boolean they consume; they do not receive the whole UI preset or perform string lookup.

## Preset Resolution And Precedence

The final target viewport owner applies the following order before submitting the request:

1. start from the baseline for the viewport kind;
2. apply the selected view-mode preset's explicit `Visualization`, enable mask, and disable mask;
3. apply saved per-viewport show-flag overrides;
4. submit the final `Visualization` and `RenderShowFlagSet` with the view request.

The initial Reference delivery stops after step 2 because no Show menu or sparse override storage exists yet. When overrides are implemented, selecting a new mode clears overrides only for flags explicitly owned by the old or new preset, then applies the new preset. Unrelated user choices such as gizmo visibility remain. A later manual change to a mode-owned flag marks the viewport **Custom** and **Reset Show Flags** removes the override deltas.

The initial implementation has no CVar force layer. If a future developer-force requirement is accepted, it must be a separate, visibly forced mask applied by Renderer after request admission, with explicit precedence and per-view isolation consequences. It may not silently become the normal view-mode transport.

## Reference Path Tracer Preset

The stock **Reference Path Tracer** view mode is defined by frontend data, not an imperative callback chain:

| Concrete setting | Value | Delivery |
| --- | --- | --- |
| `Visualization` | `Lit` | Visualization migration |
| `ReferencePathTracer` | enabled | Reference selector clean break |
| `Exposure` | enabled | Later presentation stage, when the flag gains its pass consumer |
| `Tonemapper` | enabled | Later presentation stage, when the flag gains its pass consumer |
| Lit-only estimator contributions | disabled by the mutually exclusive frame recipe, not by individually toggling every Lit flag | Reference selector clean break |

The corresponding Lit preset clears `ReferencePathTracer`. `BuildRenderFrameGraph` tests that one resolved flag and directly invokes either the existing Lit middle or the feature-local Reference middle. Provider, reconstruction, history, and resource decisions remain inside the selected composition. The shared frame shell and presentation tail contain no Reference-specific ternary.

The Reference session identity does not include the UI mode or the `ReferencePathTracer` flag. Entering Lit suspends the session; returning to Reference revalidates canonical camera/scene/transport identity before reuse. This preserves ordinary comparison behavior without allowing presentation state to become transport truth.

## Editor And Runtime Use

Editor owns the ordered View Mode menu and optional **Show** menu. Game/runtime may expose a different UI or semantic command, but it submits the same concrete per-view Renderer fields. Neither host writes Renderer CVars to select a view.

The optional Show menu provides:

- **Reset Show Flags**, which removes the viewport's explicit deltas and returns to kind and mode defaults;
- category-level **Show All** and **Hide All** actions that edit the same individual bits;
- a visible **Custom** marker whenever resolved mode-owned flags differ from the stock preset;
- a concise tooltip describing the flag's visible effect.

Do not expose raw bit indices, hexadecimal masks, CVar names, or graph-rebuild terminology in the normal UI. The View Mode menu remains the default workflow; Show is progressive disclosure. The Show menu is not a prerequisite for the Reference Path Tracer view mode.

## View Modes As Presets

Each `Visualization` has one Renderer-owned signal domain, while each Editor view-mode preset owns its explicit show-flag enable/disable masks. The exhaustive preset table sets both presentation flags for scene-referred HDR choices and clears both for display-linear exact choices:

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

## Clean-Break Migration

The staged migration removes, in the same change that replaces their final consumer:

- `Renderer::SubmitVisualization`, `VisualizationCommand`, and the Application callback used only to mutate view-selection CVars;
- `CVarVisualization` as the ordinary visualization authority;
- `CVarReferencePathTracer`, its private header, built-value cache, and every topology/composition read;
- any second visualization/show-flag copy in graph settings, feature sessions, capture payloads, or RHI;
- any Renderer-side `RenderViewMode` or UI enumeration.

There is no compatibility alias or period with two live selectors. The current CVar-command seam is transitional source state and must not be extended during the prerequisite stages.

## Frozen Migration Route

The clean break is intentionally split so no unused show-flag substrate lands:

1. Migrate `Visualization` first. The Editor viewport owner resolves every current ordered mode to the existing concrete Renderer `Visualization`, stores that value on its ordinary `ViewportRenderRequest`, and increments that request's existing generation when it changes. `RenderViewBuilder` freezes it into `RenderView` and derives the existing shader scalar from the View. The Reference menu row remains unavailable in this slice.
2. In the next slice, add the compact show-flag type and its first and only bit, `ReferencePathTracer`, together with all of that bit's consumers. The Editor Reference preset becomes selectable and submits `Visualization::Lit` plus the bit; Game/runtime may submit the same concrete request values without importing the Editor enum.
3. Add presentation, scene, lighting, and overlay flags later only alongside their production consumers and, where applicable, their optional Show-menu metadata and override storage.

`UI` is the Editor composition boundary that owns both the session and viewport panel. Its local view-mode-change handling resolves frontend preset data and updates the panel-owned request; the panel remains the sole owner that increments `ViewportRenderRequest::Generation`. The Application-level callback, Renderer command, and global selector writes disappear. This is a focused frontend event boundary, not a cross-module imperative callback chain.

Before `RenderView` exists, `FramePipeline` reads the already submitted request to resolve provider/render extent and compare the topology-affecting Reference bit with the built graph. `BuildRenderFrameGraph` contains the one execution branch that schedules Lit or Reference. After View construction, the Reference feature reads the frozen View bit for its selected/suspended lifecycle. These are observations of one accepted request/View value: no graph-settings copy, built selector CVar, command boolean, or independently resolved flag exists.

## External Precedent

[Debug View Presentation Precedent](Research.md) owns the Unreal Engine findings behind the selected view-mode and show-flag model.

## Rejected Alternatives

- **A Renderer-owned UI view-mode mirror:** rejected because it duplicates frontend taxonomy and couples rendering to UI ordering.
- **Process-global CVar selection:** rejected because it cannot represent two viewports independently and makes a UI action mutate global Renderer state.
- **Show flags implemented as independently consumed CVars:** rejected because that creates two authorities and repeats mutable global reads throughout the frame.
- **Dynamic flag registry:** rejected because the current closed enum has one renderer owner and a small implemented consumer set. Revisit only when a real module must contribute flags without modifying Renderer.
- **One flag per algorithm or quality choice:** rejected because those choices belong to rendering settings, selectors, and capability resolution rather than feature visibility.

## Non-Goals

- An Unreal-sized flag catalog, runtime registration, string mutation API, generic settings bag, or diagnostics framework.
- Moving Editor labels, order, selection identity, or override UI into Renderer.
- Moving show flags into RHI or shader-global state.
- Replacing Renderer scalability settings, capability reporting, requested outputs, or transport configuration with show flags.
- Defining visualization shader mappings or display transfer behavior; those remain in [Debug View Presentation Architecture](PresentationArchitecture.md).
- Implementing the Show menu, capture provenance, or unrelated debug tooling as a prerequisite for Reference Path Tracer delivery.

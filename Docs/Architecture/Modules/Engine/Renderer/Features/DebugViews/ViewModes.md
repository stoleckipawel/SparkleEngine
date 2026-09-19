# Render View Modes

**Status:** target architecture reconciled with the source-present Stage 7 route; not build, runtime, visual, or release proof

**Date:** 2026-09-15

**Responsibility:** the single per-view rendering-mode contract, its ownership path, and the boundary between rendering semantics and frontend presentation

## Decision

Sparkle has one view-mode authority: `RenderViewMode`.

It is a small Renderer contract because each value changes what Renderer produces. `ViewportRenderRequest::ViewMode` carries the selected value, `RenderViewBuilder` freezes it into `RenderView::viewMode`, and the pass that owns the affected behavior consumes it. Editor owns only how these values are presented and selected: labels, icons, menu grouping, shortcuts, and interaction state.

The mode is not decomposed into a visualization target plus show flags. That split created multiple names and owners for one user choice without providing independent controls. Sparkle also has no Editor mirror enum, preset translator, process-global selection CVar, generic render-settings bag, or RHI copy.

## Current Contract

The values are contiguous and stable within the current source contract:

| Value | Mode | Primary Renderer consumer |
| ---: | --- | --- |
| `0` | `Lit` | ordinary Lit middle and presentation |
| `1` | `ReferencePathTracer` | Lit-versus-Reference composition and feature lifecycle |
| `2` | `Wireframe` | raster GBuffer fill state |
| `3`-`10` | GBuffer views | GBuffer visualization family |
| `11`-`15` | direct/indirect lighting views | lighting visualization family |
| `16` | `GpuSceneInstances` | GPU-scene visualization family |
| `17` | `Count` | sentinel only; never submitted |

The C++ authority is [`RenderViewMode.h`](../../../../../../../Engine/Renderer/Public/Viewport/RenderViewMode.h). [`RenderViewModeConstants.hlsli`](../../../../../../../Engine/Assets/Shaders/Resources/RenderViewModeConstants.hlsli) mirrors only shader-consumed numeric values and must remain exactly aligned.

## Ownership Path

```text
Editor or Game/runtime viewport owner
        |
        | selects RenderViewMode
        v
ViewportRenderRequest::ViewMode
        |
        | accepted and frozen once
        v
RenderView::viewMode ----> focused View uniform index for debug shaders
        |
        +----> BuildRenderFrameGraph: Lit or Reference middle
        +----> raster GBuffer: filled or wireframe
        +----> SceneVisualizationPasses: selected family pass
```

The path has one semantic value, not several translated representations. Multiple consumers are permitted because composition, raster state, and debug resolve are distinct places where the selected mode has observable effect. None becomes a second authority.

`SceneVisualizationPasses` invokes the GBuffer, lighting, and GPU-scene family entry points in frame order. Each family privately uses an explicit enumerator switch or equality test and returns before declaring resources when inactive;
enum ordering is not an activation contract.Only the selected family binds resources
    and schedules its focused shader.

## Module Boundaries

### Renderer Public

Renderer Public contains only the generic `RenderViewMode` enum and the ordinary viewport request field. This is the minimum contract required by Editor and non-Editor viewport owners. It contains no Reference Path Tracer session, sampler, estimator, resources, UI labels, icons, preset tables, or feature configuration.

### Renderer Private

`RenderView`, the View builder, frame composition, raster state, and debug resolve are private implementation consumers. The Reference Path Tracer remains a private lighting feature. Outside that capsule its source-named integration is limited to:

- one generic mode enumerator;
- the existing `FramePipeline` lifetime member;
- one direct composition branch;
- shader registration/build membership;
- documentation and evidence.

### Editor

Editor uses `RenderViewMode` directly. `EditorViewportSession` owns the selected value, `ViewportTopPanel` owns labels/icons/menu layout, and `ViewportPanel` owns the request plus its generation. There is no `EditorViewportViewMode`, preset resolver, or Application callback translation.

### RHI

RHI has no view-mode type or field. It receives only neutral GPU resources, commands, synchronization, capabilities, presentation, and readback work.

## View Mode Versus Future Show Controls

Unreal exposes both a high-level runtime `EViewModeIndex` and lower-level `FEngineShowFlags`; the latter live with view-family state and may be manipulated by a mode. Sparkle currently needs only the higher-level mode contract. Copying both layers before users can independently control a contribution would create speculative state and duplicate selection authority.

A future per-view visibility or presentation control may be added only when all of these are true:

1. it has a real independent user or runtime use case;
2. it has a named production consumer and deterministic disabled behavior;
3. it is orthogonal to `RenderViewMode` rather than another encoding of a mode;
4. it is resolved below the mode-selection boundary and does not replace or compete with `ViewMode`;
5. it lands with its consumer, UX, and defect-detecting check in one change.

Examples could include independently hiding gizmos or a debug overlay. `Wireframe`, a GBuffer view, and Reference Path Tracer are not such controls: each is already a complete mutually exclusive view mode.

## Invariants

- One viewport request contains exactly one `ViewMode`.
- Two viewports may select different modes without process-global cross-talk.
- `ReferencePathTracer` is value `1`, immediately after Lit; remaining values are contiguous.
- Selection never travels through a CVar, command bridge, Editor mirror enum, target enum, show flag, generic settings record, or RHI field.
- The high-level frame builder contains one readable Lit-versus-Reference branch and no recipe/factory hierarchy.
- Feature mechanism and state remain in the private Reference Path Tracer capsule.
- The common frame shell, Scene/View ownership, frame graph, RHI submission, viewport products, and presentation remain shared.
- A mode without a current production consumer is not added.

## Clean Break

The following replaced paths are deleted rather than retained as aliases:

- `Visualization`, `VisualizationIndex`, and their global CVar/command selection route;
- the proposed `VisualizationTarget` and `RenderShowFlagSet` split;
- `EditorViewportViewMode` and `EditorViewportViewModePreset`;
- `CVarReferencePathTracer` and `r.ReferencePathTracer`;
- duplicate shader visualization helpers.

Historical plans may name these paths only in an explicit deletion or supersession record. They are not current architecture.

## Acceptance Focus

Source-shape checks must prove enum/shader numeric parity, contiguous values, one request/View field, absence of replaced authorities, focused consumers, no RHI leakage, and feature enclosure. Build/runtime/GPU checks remain separate evidence and must not be inferred from this document.

## References

- Epic, [`EViewModeIndex`](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/Engine/EViewModeIndex?application_version=5.5)
- Epic, [`FEngineShowFlags`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/FEngineShowFlags)
- Epic, [`UGameViewportClient`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/UGameViewportClient)
- Epic, [Viewport Modes](https://dev.epicgames.com/documentation/en-us/unreal-engine/viewport-modes-in-unreal-engine)


# Debug View Delivery Plan

**Status:** implementation plan; not proof of build, runtime, visual, backend, or release acceptance

**Architecture authority:** [Render View Modes](ViewModes.md) and [Debug View Presentation Architecture](PresentationArchitecture.md)

**Feature acceptance:** [Acceptance](Acceptance.md)

## Delivery At A Glance

| Stage | Outcome | Evidence still separate |
| --- | --- | --- |
| `DVP-0` | Freeze one per-view mode owner and clean-break scope | implementation |
| `DVP-1` | Migrate all existing modes to `RenderViewMode` and delete global selection | build/runtime/pixels |
| `DVP-2` | Add Reference Path Tracer as value `1` and delete its selector CVar | viewport UX and GPU correctness |
| `DVP-3` | Correct scene-referred HDR versus display-linear exact presentation | optional independent controls |
| `DVP-4` | Add only proved orthogonal per-view controls with their consumers | broad visual/backend proof |
| `DVP-5` | Retain the acceptance evidence | release acceptance until its report passes |

## DVP-0 - Freeze One Authority

Re-audit Editor/runtime viewport owners, request/View state, frame topology, raster/debug consumers, CVars, RHI, and capture. Freeze `RenderViewMode` as the sole per-view rendering choice and reject any parallel Editor enum, visualization target, show-flag encoding of a mode, CVar selector, command bridge, graph-settings copy, feature-settings copy, or RHI field.

The accepted value order is Lit `0`, Reference Path Tracer `1`, Wireframe `2`, current debug modes `3` through `16`, and Count `17`.

## DVP-1 - Migrate Existing Modes

1. Put `RenderViewMode` on the ordinary viewport request and immutable View.
2. Preserve a focused shader scalar derived from the View for existing debug shaders.
3. Let raster GBuffer consume Wireframe and `VisualizeBuffers` consume buffer/lobe/instance values.
4. Make Editor session and panel use the same type directly; keep labels/icons/menu grouping local to Editor.
5. Delete global visualization selection, command translation, Editor mirror enum/preset resolver, duplicate shader resolver, and orphan includes/APIs in one clean break.
6. Keep RHI and Renderer settings unaware.

This source shape is present in the current changelist. Compilation and runtime checks remain deferred.

## DVP-2 - Integrate Reference Path Tracer

1. Add `RenderViewMode::ReferencePathTracer` at value `1`.
2. Use the accepted request value for native-resolution topology and one direct Lit-versus-Reference branch in `FramePipeline::BuildRenderFrameGraph`.
3. Let the private Reference feature read the immutable View value for lifecycle activation.
4. Delete `CVarReferencePathTracer`, its built cache, and all selector aliases.
5. Keep the original frame shell and private feature ownership; do not add a recipe hierarchy, settings bag, diagnostics surface, or RHI state.

This source shape is present in the current changelist. The Editor row remains unavailable until the Reference UX stage connects the live product; source presence is not usable-path proof.

## DVP-3 - Correct Presentation Domains

1. Classify each mode as scene-referred HDR or display-linear exact.
2. Replace producer-local HDR preview curves with one owned display-mapping route.
3. Apply exposure and the tone curve once to HDR modes; bypass both for exact modes; always preserve output encoding.
4. Keep exposure history warm from the Lit scene and make render/output extent sampling explicit.
5. Do not introduce show flags merely to route stock mode defaults. Resolve stock presentation policy from the selected mode at its presentation owner.

## DVP-4 - Add Only Independent Controls

A per-view control may be added only with a real independent consumer and UX. Gizmo or overlay visibility may qualify. Wireframe, Reference Path Tracer, and debug products do not: they remain modes. Any added control must be orthogonal, resolved below mode selection, absent from RHI, and delivered with its disabled behavior and focused check.

## DVP-5 - Prove The Contract

Exercise enum/HLSL parity, every consumer, two-viewport isolation, Lit/Reference/Lit topology, exact/HDR numeric presentation, extent changes, output encoding, and advertised D3D12/Vulkan rows. Record only checks actually run in the owning completion report.

## Ready-To-Use Source Cleanup Prompt

```text
Reconcile the live Debug Views and Reference Path Tracer source to Docs/Architecture/Modules/Engine/Renderer/Features/DebugViews/ViewModes.md. Keep RenderViewMode as the sole host-independent per-view rendering choice on ViewportRenderRequest and immutable RenderView. Keep Editor labels/icons/menu layout local while using the same enum directly. Consume the value only at the owning frame-composition, raster, debug-resolve, and feature-lifecycle decisions. Delete parallel Editor enums, preset translators, visualization targets, mode-shaped show flags, selection CVars, command bridges, graph/feature settings copies, compatibility aliases, and RHI fields. Preserve ReferencePathTracer = 1 and contiguous values. Keep the Reference implementation private and the shared frame shell unchanged. Add no diagnostics, registry, generic settings bag, recipe hierarchy, or speculative controls. Run focused source checks, architecture_boundary_check, documentation link/anchor checks, and git diff --check; report builds and runtime checks as deferred unless actually run.
```

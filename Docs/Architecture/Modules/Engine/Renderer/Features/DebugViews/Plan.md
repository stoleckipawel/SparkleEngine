# Debug View Presentation Delivery Plan

**Status:** implementation plan; not proof of implementation or acceptance

**Scope:** clean-break delivery of per-view show flags, view-mode presets, frame-recipe selection, presentation routing, editor controls, and focused verification

**Architecture authority:** [View Modes And Show Flags](ViewModesAndShowFlags.md) and [Debug View Presentation Architecture](PresentationArchitecture.md)

**Feature acceptance:** [Debug View Presentation — Acceptance](Acceptance.md)

**Related current readiness:** **40/100** for the existing debug-view feature. The per-view presentation target in this plan is not implemented and adds no score; exact-domain, isolation, unavailable-state, provenance, and observer-cost proof remains open. See [Current Feature Readiness](../../../../../../Acceptance/CurrentReadiness.md#renderer).

This plan owns implementation slices and their delivery order. It does not redefine signal domains, show-flag semantics, display routing, or final acceptance.

## Delivery At A Glance

| Stage | Makes true | Must not claim yet |
| --- | --- | --- |
| `DVP-0` contract freeze | live ownership, staged flag vocabulary, preset precedence, graph impact, and clean-break target are explicit | implementation |
| `DVP-1` visualization migration | every current mode uses one per-view concrete visualization and no global visualization selector remains | show flags, Reference recipe integration, or pixel correctness |
| `DVP-2` Reference integration | the compact flag set and its first bit, `ReferencePathTracer`, land with all consumers and replace the global Reference selector | finished viewport UX or unrelated flags |
| `DVP-3` presentation flags | exposure and tone-mapping flags land with their production consumers and exact-domain routing | optional Show UX or backend/visual acceptance |
| `DVP-4` remaining flags and Show UX | each accepted scene/lighting/overlay flag lands with its consumer and optional per-viewport controls | backend/visual acceptance |
| `DVP-5` proof | isolation, topology, numeric presentation, interaction, and advertised backend evidence is retained | release completion until the feature report accepts it |

These stages are an architecture prerequisite for the next Reference Path Tracer UX slice. They do not add a second renderer, a Renderer view-mode enum, a generic settings framework, or a diagnostics surface. The current `SubmitVisualization`/CVar command path is transitional source state: do not extend it while this migration is pending.

## Stage DVP-0 - Freeze The Per-View Control Contract

### Work

1. Re-audit current Editor and runtime viewport owners, `ViewportRenderRequest`, `RenderViewBuilder`, `RenderView`, frame-topology timing, visualization consumers, CVar ownership, capture boundaries, and RHI exposure from the live tree.
2. Ratify the staged `RenderShowFlag` vocabulary. Every flag needs one meaning, deterministic disabled behavior, first production consumer, graph-impact class, preset ownership, delivery stage, and deletion consequence; only `ReferencePathTracer` may enter the first implementation.
3. Freeze preset resolution and override precedence, including Lit/Reference transitions, two simultaneous viewports, kind baselines, Custom/reset behavior, unavailable capability, and Shipping reachability.
4. Freeze the clean-break ledger for `CVarVisualization`, `CVarReferencePathTracer`, `SubmitVisualization`, `VisualizationCommand`, built topology state, and every producer/consumer.
5. Reject any design that carries `EditorViewportViewMode` into Renderer, copies show flags into RHI, makes CVars the normal per-view transport, or requires a generic registry/settings bag.

### Exit gate

- No open decision remains about control ownership, precedence, topology timing, or the CVar clean break.
- The ordinary request can expose concrete Renderer semantics before graph refresh without introducing UI taxonomy.
- Each proposed flag has a current or same-stage consumer; speculative flags are removed.
- No production code changes occur in this stage.

### Ready-to-use prompt

```text
Execute only Stage DVP-0 of Docs/Architecture/Modules/Engine/Renderer/Features/DebugViews/Plan.md. Re-audit the live tree and reconcile ViewModesAndShowFlags.md, PresentationArchitecture.md, Acceptance.md, the Reference Path Tracer architecture/plan/UX, and the Renderer/Editor/RHI engineering boundaries. Freeze one frontend-preset-to-concrete-request contract, exact flag vocabulary, override precedence, topology classification, and a no-orphan clean-break ledger. Do not change production code, add diagnostics, or invent a Renderer view-mode enum. End with PASS/BLOCKED, exact evidence, unresolved decisions, and authorization for DVP-1.
```

`DVP-0` is satisfied by the [Reference Path Tracer Stage-6A execution report](../Lighting/ReferencePathTracer/Plan.md#stage-6a-execution-result--2026-09-14), which binds the contract to the live source route and authorizes `DVP-1` without claiming implementation.

## Stage DVP-1 - Migrate Visualization And Delete Global Selection

### Work

1. Add the existing concrete Renderer `Visualization` value to `ViewportRenderRequest` and freeze it into `RenderView`. Keep the existing focused shader scalar derived from View state.
2. Add one exhaustive Editor-owned preset resolver for the existing contiguous `EditorViewportViewMode` values. The Reference row resolves to `Visualization::Lit` but remains unavailable until `DVP-2` supplies its execution flag.
3. Keep the panel-owned request as the sole request-generation owner. A local `UI` event connects the Editor session preset to that request; Application and Renderer receive only the ordinary request.
4. Migrate Game/runtime producers without importing the Editor enum. In one clean break delete `CVarVisualization` as normal authority, `Renderer::SubmitVisualization`, `VisualizationCommand`, the Application callback/translation chain, and orphan APIs/includes.
5. Do not add a show-flag type, override storage, registry, metadata table, diagnostics, capture schema, or RHI field in this stage.

### Exit gate

- Every existing selectable view mode produces its intended visualization through one ordinary per-view request.
- Two viewport requests may carry different visualizations without process-global cross-talk.
- UI mode identity remains absent from Renderer/RHI, and no show-flag substrate exists without a consumer.
- No normal selection path mutates `CVarVisualization`; no compatibility alias or second selector remains.

### Ready-to-use prompt

```text
Implement only Stage DVP-1 of Docs/Architecture/Modules/Engine/Renderer/Features/DebugViews/Plan.md after DVP-0 PASS. Put the existing concrete Visualization on ViewportRenderRequest and immutable RenderView, add one exhaustive Editor-owned resolver for the current contiguous view modes, and update the panel-owned request generation when its resolved visualization changes. Migrate Game/runtime producers without the Editor enum. In one clean break delete CVarVisualization as normal authority, Renderer::SubmitVisualization, VisualizationCommand, the Application callback/translation chain, and every orphan include/API. Preserve current shader behavior through the focused View-derived scalar. Keep Reference unavailable. Do not add RenderShowFlag, override storage, a Renderer view-mode enum, generic settings bag, registry, diagnostics, capture schema, Show menu, or RHI fields. Run focused compile/source checks, architecture_boundary_check, and git diff --check; report unrun runtime work honestly.
```

## Stage DVP-2 - Add The First Show Flag And Select The Reference Middle

### Work

1. Add the compact fixed `RenderShowFlagSet` and its first and only enum member, `RenderShowFlag::ReferencePathTracer`, at the narrow Renderer request/View boundary. Include only construction, equality, `Set`, and `IsEnabled` operations with current consumers.
2. Disabled means ordinary Lit composition; enabled means the feature-local Reference middle. The Editor Reference preset submits `Visualization::Lit` plus the bit; Lit clears it. Game/runtime may submit the same concrete values directly.
3. Use the submitted request bit for the necessary pre-View provider/render-extent and graph-topology observations. `BuildRenderFrameGraph` contains the sole execution branch and schedules exactly one middle. The Reference feature reads the frozen View bit for selected/suspended lifecycle state.
4. Clean-break `CVarReferencePathTracer`, its private header/definition, built CVar cache, renderer command boolean, and every remaining producer/consumer.
5. Keep feature mechanism in the Reference capsule, shared frame orchestration thin, and RHI/capture unaware. Add no unrelated flag, metadata registry, override store, Show menu, or diagnostics.

### Exit gate

- `RenderShowFlag::ReferencePathTracer` is the sole execution authority for both Scene and Game views.
- Lit/Reference/Lit changes rebuild topology safely, drive the same feature lifecycle, and never schedule both middles.
- No Reference CVar, built selector cache, dedicated command, Renderer view-mode enum, graph-settings copy, repeated execution branch, or RHI flag remains.
- Deleting the feature capsule plus the one branch and generic request/View bit leaves the ordinary Lit frame coherent.

### Ready-to-use prompt

```text
Implement only Stage DVP-2 of Docs/Architecture/Modules/Engine/Renderer/Features/DebugViews/Plan.md after DVP-1. Add the smallest fixed RenderShowFlagSet and only RenderShowFlag::ReferencePathTracer on ViewportRenderRequest and immutable RenderView. The Editor Reference preset submits Visualization::Lit plus the flag; Lit clears it; Game/runtime may submit the same concrete values. Use the request value for necessary pre-View provider/extent and topology observations, keep the sole Lit-versus-Reference execution branch in BuildRenderFrameGraph, and let the Reference feature read the frozen View bit for lifecycle state. Delete CVarReferencePathTracer, its private header/definition and built cache, the old command boolean, and every orphan consumer in one clean break. Do not add unrelated flags, metadata, overrides, diagnostics, capture fields, a Show menu, or RHI state.
```

## Stage DVP-3 - Add Presentation Flags With Their Consumers

### Work

1. Add `Exposure` and `Tonemapper` enum members only while making their application derive from immutable `RenderView.ShowFlags`.
2. Keep exposure metering/history, tone mapping, and output encoding under their existing owners; do not introduce a second presentation pipeline.
3. Remove producer-local HDR preview curves, preserve one exact mapping for scalar/normal/material/instance views, and make render/output extent sampling explicit.
4. Extend stock presets with the accepted presentation defaults. Do not add manual overrides or a Show menu yet.

### Exit gate

- Both presentation flags have real pass consumers and deterministic disabled behavior.
- Exact display-linear modes remain invariant under exposure/tone-mapper setting changes apart from declared output encoding.
- No producer-local preview curve, shader-global show-flag mask, or unrelated enum member lands.

### Ready-to-use prompt

```text
Implement only Stage DVP-3 of Docs/Architecture/Modules/Engine/Renderer/Features/DebugViews/Plan.md after DVP-2. Add Exposure and Tonemapper show flags together with their existing presentation-pass consumers and stock preset defaults. Preserve exposure metering/history and unconditional output encoding, remove producer-local HDR preview curves, and route exact views through one display-linear mapping with explicit extent sampling. Do not add unrelated flags, manual overrides, a Show menu, diagnostics, capture schema, or a second presentation pipeline.
```

## Stage DVP-4 - Add Remaining Accepted Flags And Optional Show UX

1. Add each accepted scene, lighting, and overlay enum member only in the same change that wires its named consumer and disabled behavior. No pass reads a CVar for behavior represented by a show flag.
2. Add the categorized Show menu, per-viewport sparse overrides, Custom indicator, and Reset Show Flags only for those implemented flags.
3. Keep the final resolved set on the request/View; override deltas and UI metadata remain Editor-owned.
4. Do not add capture provenance or diagnostics merely to justify the architecture; add evidence fields later only when a real capture/replay requirement owns them.

## Stage DVP-5 - Prove The Complete Contract

1. Exercise exhaustive preset/metadata coverage, override precedence, Lit/Reference topology, reset, unavailable capability, and two-viewport isolation.
2. Check the four exposure/tone-mapper combinations, both output encodings, and mismatched extents against fixed values.
3. Run the selected D3D12/Vulkan viewport workloads only when the acceptance claim requires them.
4. Retain exact commands/results in the owning `FCR-REN-11` report; source inspection or a plausible screenshot does not close the gate.

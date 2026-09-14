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
| `DVP-0` contract freeze | live ownership, every initial flag, preset precedence, graph impact, and clean-break target are explicit | implementation |
| `DVP-1` per-view substrate | one concrete visualization/show-flag request reaches immutable View state | migrated modes or pixel correctness |
| `DVP-2` mode migration | every current mode uses one frontend preset and no global visualization selector remains | Reference recipe integration or complete Show menu |
| `DVP-3` Reference integration | `ReferencePathTracer` is the sole per-view selector for the alternate middle recipe | finished viewport UX or acceptance |
| `DVP-4` presentation and Show UX | accepted flags affect their real consumers and the optional Show menu edits per-viewport overrides | backend/visual acceptance |
| `DVP-5` proof | isolation, topology, numeric presentation, interaction, and advertised backend evidence is retained | release completion until the feature report accepts it |

These stages are an architecture prerequisite for the next Reference Path Tracer UX slice. They do not add a second renderer, a Renderer view-mode enum, a generic settings framework, or a diagnostics surface. The current `SubmitVisualization`/CVar command path is transitional source state: do not extend it while this migration is pending.

## Stage DVP-0 - Freeze The Per-View Control Contract

### Work

1. Re-audit current Editor and runtime viewport owners, `ViewportRenderRequest`, `RenderViewBuilder`, `RenderView`, frame-topology timing, visualization consumers, CVar ownership, capture boundaries, and RHI exposure from the live tree.
2. Ratify the bounded `RenderShowFlag` set. Every flag needs one meaning, deterministic disabled behavior, first production consumer, graph-impact class, preset ownership, and deletion consequence.
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

## Stage DVP-1 - Establish The Per-View Show-Flag Substrate

### Work

1. Add the fixed `RenderShowFlag` and compact `RenderShowFlagSet` value types at the narrow Renderer public boundary shared by viewport producers and Renderer.
2. Add concrete `Visualization` and final `RenderShowFlagSet` values to `ViewportRenderRequest`. Do not add `RenderViewMode`, UI labels, override deltas, strings, capability state, or a generic settings aggregate.
3. Freeze those values into `RenderView`; use the request's topology-affecting subset during graph refresh so topology and View consumption observe one submitted value.
4. Add only the bitset operations and default baseline used by current production consumers. No dynamic registry, reflection, logging, readback, dashboard, or shader-global flag mask.
5. Prove two requests can carry different values without a process-global selector or RHI changes.

### Exit gate

- The per-view request and immutable View contain exactly one concrete visualization and show-flag set.
- The graph key can compare topology-affecting bits before View construction without a second authority.
- UI mode identity remains absent from Renderer/RHI and flags remain absent from RHI.
- No behavior changes yet beyond establishing and consuming the default-equivalent state.

### Ready-to-use prompt

```text
Implement only Stage DVP-1 of Docs/Architecture/Modules/Engine/Renderer/Features/DebugViews/Plan.md after DVP-0 PASS. Add the smallest fixed Renderer `RenderShowFlag`/`RenderShowFlagSet` contract, place concrete `Visualization` and the final flag set on `ViewportRenderRequest`, and freeze the same values into `RenderView`. Let frame-topology comparison read only the submitted topology bits it owns before View construction. Preserve current output with default-equivalent values. Do not migrate view modes yet; do not add a Renderer view-mode enum, generic settings bag, dynamic registry, diagnostics, UI, RHI fields, capture schema, or shader-wide flag mask. Run focused compile/source checks and architecture_boundary_check if the public Renderer boundary changes.
```

## Stage DVP-2 - Migrate Existing View Modes And Delete Global Selection

### Work

1. Define one exhaustive Editor-owned preset table mapping every `EditorViewportViewMode` to concrete `Visualization` plus explicit show-flag enable/disable masks. Keep numeric UI ordering contiguous.
2. Store only the selected frontend mode and sparse override deltas in the viewport owner; submit the final concrete values with each ordinary viewport request.
3. Migrate Application and Game/runtime producers to that request path. Game/runtime does not import the Editor enum and may submit the concrete values directly.
4. Clean-break `CVarVisualization` as the ordinary authority and delete `SubmitVisualization`, `VisualizationCommand`, the callback/translation chain, and their orphan includes/APIs.
5. Make `RenderViewBuilder` copy request visualization to the existing focused shader scalar. Passes consume View state or focused values, never Editor state or a global selection CVar.

### Exit gate

- Every existing view mode produces the same intended visualization through ordinary per-view request state.
- Two viewports can select different visualizations without cross-talk.
- No normal selection path mutates `CVarVisualization`, and no compatibility alias or second selector remains.
- No Reference Path Tracer topology migration is claimed until DVP-3.

### Ready-to-use prompt

```text
Implement only Stage DVP-2 of Docs/Architecture/Modules/Engine/Renderer/Features/DebugViews/Plan.md after DVP-1. Add one exhaustive Editor-owned preset table for the existing ordered view modes, submit its concrete Visualization and resolved RenderShowFlagSet through the ordinary viewport request, and make RenderViewBuilder consume that state. Migrate runtime producers without importing Editor vocabulary. In one clean break delete CVarVisualization as normal authority, Renderer::SubmitVisualization, VisualizationCommand, and the callback/translation plumbing that exists only for them. Preserve contiguous UI values and current visualization shader behavior. Do not integrate ReferencePathTracer yet, add a Show menu, diagnostics, capture metadata, RHI state, or a generic resolver framework.
```

## Stage DVP-3 - Select The Reference Middle With One Show Flag

### Work

1. Add `RenderShowFlag::ReferencePathTracer` with disabled meaning ordinary Lit composition and enabled meaning the feature-local Reference middle.
2. Make the Editor Reference preset set `Visualization::Lit`, enable `ReferencePathTracer`, and establish its presentation defaults. The Lit preset explicitly clears the flag.
3. Make `FramePipeline::BuildRenderFrameGraph` test the resolved per-view flag once and directly schedule exactly one middle recipe. Use the same flag in the topology key and Reference session active/suspended input; do not repeatedly branch elsewhere.
4. Clean-break `CVarReferencePathTracer`, its private header/definition, built CVar cache, renderer command boolean, and every remaining producer/consumer.
5. Keep feature mechanism in the Reference capsule, shared frame orchestration thin, and RHI unaware. Preserve the same Scene/Game View camera, Scene publication, graph execution, viewport product, progress, UI, and presentation owners.

### Exit gate

- `RenderShowFlag::ReferencePathTracer` is the sole execution selector for both Scene and Game views.
- Lit/Reference/Lit toggling rebuilds topology safely, suspends/revalidates the existing Reference session contract, and never schedules both middles.
- No `r.ReferencePathTracer`, selector command, Renderer view-mode enum, repeated flag switch, or RHI flag remains.
- The high-level frame names only the one semantic branch; provider, reconstruction, resource, and shader choices remain inside the selected route.

### Ready-to-use prompt

```text
Implement only Stage DVP-3 of Docs/Architecture/Modules/Engine/Renderer/Features/DebugViews/Plan.md after DVP-2. Add RenderShowFlag::ReferencePathTracer as the sole per-view selector for the existing alternate middle. The Editor preset selects Lit visualization and enables the flag; Lit clears it. BuildRenderFrameGraph reads the submitted topology flag once and directly selects exactly one Lit or feature-local Reference composition, while the same accepted value drives graph-key rebuild and Reference active/suspended state. Delete CVarReferencePathTracer, its built cache and private declaration, the old command boolean, and every orphaned producer/consumer in one clean break. Keep Scene/Game behavior identical, RHI unaware, and feature mechanism enclosed. Do not add diagnostics, a second renderer, recipe hierarchy, settings bag, Show menu, capture schema, or unrelated flags.
```

## Stage DVP-4 - Complete Presentation Flags And Optional Show UX

1. Make exposure and tone-mapper application derive from immutable `RenderView.ShowFlags`; keep exposure metering/history and output encoding under their existing owners.
2. Remove producer-local HDR preview curves, preserve one exact mapping for scalar/normal/material/instance views, and make render/output extent sampling explicit.
3. Wire only accepted scene, lighting, and overlay flags to their named consumers. No pass reads a CVar for behavior represented by a show flag.
4. Add the categorized Show menu, per-viewport sparse overrides, Custom indicator, and Reset Show Flags only after the underlying flags have real consumers.
5. Do not add capture provenance or diagnostics merely to justify the architecture; add evidence fields later only when a real capture/replay requirement owns them.

## Stage DVP-5 - Prove The Complete Contract

1. Exercise exhaustive preset/metadata coverage, override precedence, Lit/Reference topology, reset, unavailable capability, and two-viewport isolation.
2. Check the four exposure/tone-mapper combinations, both output encodings, and mismatched extents against fixed values.
3. Run the selected D3D12/Vulkan viewport workloads only when the acceptance claim requires them.
4. Retain exact commands/results in the owning `FCR-REN-11` report; source inspection or a plausible screenshot does not close the gate.

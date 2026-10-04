# Debug View Delivery Plan

**Status:** implementation plan; not proof of build, runtime, visual, backend, or release acceptance

**Responsibility:** own dependency order, change maps, integration ledgers, stop conditions, and exit gates for Debug Views delivery.

**Current readiness:** **Not applicable** to this plan; see the [Debug Views dossier](README.md) and central [Renderer readiness row](../../../../../../Acceptance/CurrentReadiness.md#renderer).

**Verified baseline:** 2026-10-04 at revision `26803f97` with an inspected dirty working tree; every implementation stage must re-audit its candidate.

**Architecture authority:** [Viewport Rendering Controls](Controls/README.md) and [Debug View Presentation Architecture](PresentationArchitecture.md)

**Implementation authorization:** [Discovery](Discovery.md)

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

The accepted value order is Lit `0`, Reference Path Tracer `1`, Wireframe `2`, current debug modes `3` through `17`, and Count `18`.

## DVP-1 - Migrate Existing Modes

1. Put `RenderViewMode` on the ordinary viewport request and immutable View.
2. Preserve a focused shader scalar derived from the View for existing debug shaders.
3. Let raster GBuffer consume Wireframe and let the independently activated GBuffer, lighting, and GPU-scene visualization families consume their own modes and products.
4. Make Editor session and panel use the same type directly; keep labels/icons/menu grouping local to Editor.
5. Delete global visualization selection, command translation, Editor mirror enum/preset resolver, duplicate shader resolver, and orphan includes/APIs in one clean break.
6. Keep RHI and Renderer settings unaware.
7. Partition debug resolve by GBuffer, lighting, and GPU-scene product families. Give each family an explicit mode predicate, pass parameter surface, and shader; never use enum ordering or a catch-all visualization shader as family membership.

This source shape is present in the current changelist. Compilation and runtime checks remain deferred.

## DVP-2 - Integrate Reference Path Tracer

1. Add `RenderViewMode::ReferencePathTracer` at value `1`.
2. Use the accepted request value to select presentation upscaling instead of ray reconstruction without overriding the chosen upscaler or quality, and for one direct Lit-versus-Reference branch in `AddSceneRenderingPasses` below `FramePipeline::BuildRenderFrameGraph`.
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

## DVP-4 - Add Lighting Show Flags

The [Renderer Show Flags](Controls/ShowFlags.md) target is delivered through two vertical slices. This plan orders work; it does not redefine the flag semantics or UI behavior owned there.

### DVP-4A - Existing Lighting And Shadow Controls

**Objective:** expose the five current direct/indirect lighting products plus Direct Shadows and Indirect Shadows through one per-viewport Show menu and one feature-owned global developer CVar gate per leaf, without changing graph topology or misrepresenting indirect-path visibility.

**Prerequisites:**

1. [Discovery](Discovery.md) records DVP-4A as **AUTHORIZED**. Its current decision is **BLOCKED**; implementation must not begin from this plan alone.
2. Re-audit the live five-product lighting composite, primary direct-shadow visibility signal, secondary-hit direct-light visibility in the Lit indirect estimator, viewport request/View publication, Editor session/panel route, CVar mutation thread, and Renderer control queue at the candidate revision.
3. Prove that live `SetCVar` writes reach Renderer through a sequenced owner-thread boundary. If the current generic CVar path cannot prove that, stop and repair the existing CVar delivery owner before adding feature CVars. Do not copy CVar values through viewport requests or graph settings as a workaround.
4. Confirm each selected shader-parameter representation can change every frame without graph reconstruction or a second mutable mask, and identify every indirect temporal state invalidated by an Indirect Shadows change.

**Work:**

1. Add `Renderer/Public/Viewport/RenderShowFlags.h` with the seven-leaf fixed enum/set and no reflection, registry, string API, serialization, or metadata table.
2. Add `ViewportRenderRequest::ShowFlags`, freeze it once into `RenderView::showFlags`, and restore all-enabled state in the existing reuse/reset path.
3. Register seven Renderer-private feature CVars: `r.Lighting.Direct.{Diffuse,Specular,Subsurface}`, `r.Lighting.Indirect.{Diffuse,Specular}`, and `r.Lighting.Shadows.{Direct,Indirect}`. Keep each beside its feature consumer; do not create an `r.ShowFlags` namespace.
4. Extend `EditorViewportSession` with the viewport-local set and one change notification. Let `ViewportPanel` remain the sole request-generation owner.
5. Add the separate hierarchical **Show** dropdown to `ViewportTopPanel`. Direct Lighting owns three children, Indirect Lighting owns two, and Shadows owns Direct Shadows and Indirect Shadows. Parent state is derived; one bulk action mutates the set and advances the request generation once.
6. Mask the five completed contributions only in `LightingComposite`. Preserve their producers, histories, reconstruction inputs, and raw lobe visualization.
7. Apply Direct Shadows only where primary direct lighting consumes the shadow visibility signal; substitute fully visible evaluation when disabled while preserving the raw signal and graph topology.
8. Apply Indirect Shadows only to direct-light visibility evaluated at secondary hits in the Lit indirect estimator. Preserve continuation intersections and Reference behavior, and route a semantic toggle through the existing per-view invalidation owner for every dependent temporal state.
9. Reconcile the Runtime Configuration catalog, Debug Views dossier, and directly affected source navigation. Add no disabled Indirect Subsurface control.

**Data/copy budget:**

| Value | Authority | Copy and boundary reason | Lifetime / invalidation |
| --- | --- | --- | --- |
| Editor selection | `EditorViewportSession` | copied into the ordinary viewport request for module/thread publication | viewport session; replaced by the next accepted edit |
| submitted selection | `ViewportRenderRequest::ShowFlags` | copied once into immutable `RenderView` because request and prepared-frame lifetimes differ | request generation to one prepared frame/frame slot |
| global CVar gate | Renderer-private feature owner | no request/View/settings copy; read at its narrow parameter-preparation consumer | current sequenced Renderer CVar value |
| effective shader booleans | lighting composite or direct/indirect lighting owner | derived pass parameters required by CPU/GPU ABI | one pass preparation/dispatch |
| indirect-shadow semantic generation | existing per-view indirect invalidation owner | retained only because temporal estimator state must not mix shadowed and unshadowed samples | changes when effective Indirect Shadows changes |

**Integration-hook ledger:**

| Surface | Classification | Justification | Defect-detecting check |
| --- | --- | --- | --- |
| Renderer Public viewport contract | new justified hook | Editor and Game/runtime need the same seven rendering semantics | `CHK-DVP-08` traces the only public type and request field |
| Renderer Private View | existing hook | immutable per-frame publication boundary | `CHK-DVP-08` proves one request-to-View copy and no writeback |
| Lighting composite C++/HLSL | existing consumer extended | narrow owner where all five completed products meet | `CHK-DVP-10` isolates every non-zero lobe |
| Direct-light visibility consumption | existing consumer extended | direct shadowing is already a separate signal at primary-surface shading | `CHK-DVP-10` proves unshadowed evaluation and unchanged raw signal |
| Lit indirect path-lighting evaluation and invalidation | existing consumers extended | secondary-hit direct-light visibility owns indirect shadowing; temporal state depends on it | `CHK-DVP-10` proves scope, reset, and no Reference cross-talk |
| Editor viewport session/panel/top panel | smallest presentation hook | viewport-local interaction and one request publication | `CHK-DVP-09` covers leaf, mixed parent, reset, and generation behavior |
| Application, generic frame graph, RHI | no hook permitted | no show-flag policy or transport belongs there | `CHK-DVP-08` stale-name/dependency search |

**Non-goals:** Indirect Subsurface, persistence, capture schema, producer/trace pruning, topology changes, global parent CVars, an `r.ShowFlags` namespace, a generic registry/settings bag, new diagnostics, Application translation, RHI state, disabling indirect continuation intersections, or changing Reference Path Tracer behavior.

**Stop conditions:** stop the slice if CVar mutation remains unsequenced; a contribution leaf lacks a non-zero isolatable product; either shadow leaf lacks a controlled occluder oracle; Indirect Shadows cannot invalidate every dependent temporal state; the Editor must read/write Renderer-private CVars; a second resolved mask is retained; request generation advances once per child; Reference behavior changes; continuation intersections are bypassed; or a required raw diagnostic product is lost.

**Exit gate:** `AC-DVP-17` through `AC-DVP-25` and `AC-DVP-27` pass through `CHK-DVP-08` through `CHK-DVP-11`. Record shader cook and decoded D3D12/Vulkan pixel checks as `PASS`, `BLOCKED`, or unrun; source inspection cannot close them.

**Current permitted prompt — discovery only:**

```text
Execute DVP-SF-P01 through DVP-SF-P04 from DebugViews/Discovery.md. Make no production-code changes. Re-audit the exact candidate, trace Editor/runtime console SetCVar mutation through serial and threaded Renderer execution, identify the owner thread/publication/order/shutdown contract, prove each narrow lighting consumer can observe its feature CVar and immutable View bit per frame without graph reconstruction, identify one independently non-zero analytic fixture for each of the five current lighting products, and freeze controlled direct/indirect shadow oracles plus the indirect-history invalidation set. Update only the owning Discovery/Plan/Acceptance facts needed to record AUTHORIZED or BLOCKED. Reject `r.ShowFlags` CVars, generic callback registries, copied CVar/settings bags, Application feature translation, parent state, no-op products, continuation-ray bypass, Reference cross-talk, and claims based only on source presence. Run documentation links/anchors, UTF-8, stale-path searches, and git diff --check; report every executable check as unrun.
```

**Implementation prompt — valid only after Discovery records `AUTHORIZED`:**

```text
Implement only DVP-4A from DebugViews/Plan.md against the exact Discovery-authorized CVar publication and shadow-invalidation routes. Add the seven-leaf RenderShowFlag/RenderShowFlagSet public viewport contract, publish one set through ViewportRenderRequest and immutable RenderView, combine feature CVar and per-view bits only at each named lighting consumer, and add the Direct Lighting, Indirect Lighting, and Shadows hierarchy with derived parents and one request-generation change per action. Preserve graph topology and raw direct-shadow signal; reset all dependent Lit indirect temporal state when effective Indirect Shadows changes; preserve continuation intersections and Reference behavior. Add no `r.ShowFlags` CVar, IndirectSubsurface surface, parent flag/CVar, persistence, registry, resolved-mask holder, Application/RHI state, producer/trace pruning, compatibility alias, permanent test file, or unledgered integration hook. Execute CHK-DVP-08 through CHK-DVP-11 in claim-driven order and stop on any failed prerequisite or enclosure check.
```

### DVP-4B - Indirect Subsurface

**Prerequisite:** `IND-D0-02` and the owning Indirect Lighting package must first define and authorize subsurface lobe classification, estimator/PDF/energy behavior, reconstruction/history consequences, a non-zero oracle, and the real `IndirectSubsurface` product. A UI request alone cannot satisfy this gate.

**Work:** land the product, producer, reconstruction and diagnostic consequences, composite binding, eighth enum value, global feature CVar, Editor leaf, Lighting-family documentation, and evidence together. The existing Indirect parent automatically expands from two to three available children; no compatibility bit or placeholder row remains.

**Non-goals:** reclassifying diffuse energy as subsurface, fabricated zero output, a UI-only flag, or opportunistic transmission/volume transport.

**Exit gate:** the eight-leaf target satisfies `AC-DVP-17` through `AC-DVP-27`, including a non-zero independent Indirect Subsurface oracle. If the transport decision remains open, record DVP-4B as `BLOCKED` and do not start implementation.

## DVP-5 - Prove The Contract

Exercise enum/HLSL parity, every consumer, two-viewport isolation, Lit/Reference/Lit topology, exact/HDR numeric presentation, extent changes, output encoding, and advertised D3D12/Vulkan rows. Record only checks actually run in the owning completion report.

## Ready-To-Use Source Cleanup Prompt

```text
Reconcile the live Debug Views and Reference Path Tracer source to Docs/Architecture/Modules/Engine/Renderer/Features/DebugViews/Controls/ViewModes.md. Keep RenderViewMode as the sole host-independent per-view rendering choice on ViewportRenderRequest and immutable RenderView. Keep Editor labels/icons/menu layout local while using the same enum directly. Consume the value only at the owning frame-composition, raster, debug-resolve, and feature-lifecycle decisions. Delete parallel Editor enums, preset translators, visualization targets, mode-shaped show flags, selection CVars, command bridges, graph/feature settings copies, compatibility aliases, and RHI fields. Preserve ReferencePathTracer = 1 and contiguous values. Keep the Reference implementation private and the shared frame shell unchanged. Add no diagnostics, registry, generic settings bag, recipe hierarchy, or speculative controls. Run focused source checks, architecture_boundary_check, documentation link/anchor checks, and git diff --check; report builds and runtime checks as deferred unless actually run.
```

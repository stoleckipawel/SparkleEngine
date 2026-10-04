# Debug View Delivery Plan

**Status:** implementation plan; not proof of build, runtime, visual, backend, or release acceptance

**Responsibility:** own dependency order, change maps, integration ledgers, stop conditions, and exit gates for Debug Views delivery.

**Current readiness:** **Not applicable** to this plan; see the [Debug Views dossier](README.md) and central [Renderer readiness row](../../../../../../Acceptance/CurrentReadiness.md#renderer).

**Verified baseline:** 2026-10-04 at revision `26803f97` with an inspected dirty working tree; every implementation stage must re-audit its candidate.

**DVP-4 design revision:** 2026-10-04 at `bbb9f7ed`; CVar-driven feature execution replaces the former per-view Show set/composite masking. This adds no executable evidence for DVP-0 through DVP-5.

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
| `DVP-4` | Add the Editor CVar frontend and feature-owned lighting/shadow execution controls | executable omission, pixels, GPU cost, and backend proof |
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

The [Lighting Show Menu And Feature Execution Controls](Controls/ShowFlags.md) target is delivered through feature-owned execution slices. This plan orders work; it does not redefine the activation, global scope, or UI semantics owned there.

### DVP-4A - Existing Lighting And Shadow Controls

**Objective:** make the hierarchical Editor Show menu edit the seven real feature CVars and remove disabled features' exclusive execution cost. Renderer sees only feature intent and prerequisites, never a Show bit set.

**Prerequisites:**

1. [Discovery](Discovery.md) records DVP-4A as **AUTHORIZED**. Its decision remains **BLOCKED**; this design revision does not authorize production edits.
2. Re-audit the five lighting products, shared direct/indirect estimators, direct shadow signal, secondary-hit visibility, CVar mutation/query route, cached graph lifecycle, reconstruction guides, and all dependent history owners at the candidate revision.
3. Prove sequenced console and Editor CVar mutation, including one parent/reset batch before frame admission and requested/applied observation. Repair the existing delivery owner if necessary; no private-header coupling, Application feature translator, request/View copy, or parallel settings authority is accepted.
4. Freeze a path-by-leaf execution ledger: exact feature-local `IsEnabled`/`IsActive` owner, exclusive/shared passes and math, supported runtime branch or cooked variant, active products/bindings, topology change mechanism, dependent invalidation, unavailable-state behavior, and remaining shared cost.
5. Supply independent non-zero lobe fixtures, primary/secondary occluder fixtures, required-product failure challenges, and negative controls that detect work still executing behind a hidden result.

**Delivery order:**

1. **Control publication:** register the seven feature-named CVars beside their existing owners. Establish the authorized sequenced mutation/query and batch route, with no Renderer show-flag API or request/View fields.
2. **Feature activation and lifecycle:** put `IsEnabled`/`IsActive` in the owning feature files; return at optional family entry points before exclusive resource/pass creation. Extend existing graph identity/retirement and affected-history invalidation only where topology or estimator semantics require it.
3. **Direct lighting:** bypass disabled lobe math/writes in the shared shader or select a bounded cooked variant. Omit the direct family and exclusive reservoir/shadow work when none of its lobes is active. Reconcile active-product bindings and reconstruction consumers in the same slice.
4. **Lit indirect lighting:** apply lobe intent before exclusive estimation/resolve, preserving valid sampling/PDF/target semantics for active lobes. Omit the indirect family when no active consumer requires it; prove guide ownership and resets. Do not infer that disabling primary specular output permits deleting specular continuation events.
5. **Shadows:** remove direct visibility production when no active consumer needs it and bypass secondary-hit direct-light shadow traces when Indirect Shadows is disabled. Preserve continuation intersections and Reference policy. Reset every dependent temporal state.
6. **Composition and diagnostics:** consume only active products; use intentional disabled-output initialization only for a justified fixed-output ABI. Remove masked finished-result suppression. A disabled raw diagnostic is unavailable, not secretly produced, stale, or zero-as-success.
7. **Editor frontend:** add the hierarchical Show dropdown using existing CVar query/mutation. Derive leaf and parent checks from CVar intent, reflect console edits, and submit one batch for parent/reset actions. Explain shared scope; add no session selection mirror or viewport-generation transport.
8. **Reconcile:** update affected runtime-CVar catalog, feature documentation, shaders/cooked membership, and navigation alongside implementation. Do not expose Indirect Subsurface prematurely.

Steps 3–5 are bounded vertical slices, each including product admission, parameters/shaders, history, and focused validation before moving on. Do not land a temporary composite-mask path as an intermediate implementation.

**Data/copy budget:**

| Value | Authority | Boundary / retained reason | Lifetime |
| --- | --- | --- | --- |
| feature intent | feature-owned CVar | existing sequenced control delivery; no request/View mirror | process / accepted control value |
| menu check state | CVar query | transient UI projection, not independently editable truth | current UI refresh |
| active pass/variant parameters | owning lighting feature | CPU/GPU ABI; one consistent accepted frame decision | pass / prepared frame |
| topology identity | existing graph lifecycle with feature contribution | retained only when accepted feature policy changes scheduled products/passes | graph generation and fence retirement |
| estimator semantic identity | existing lighting history invalidation owner | required to reject incompatible lobe/shadow samples | affected history generation |

**Integration-hook ledger:**

| Surface | Classification | Justification | Defect-detecting check |
| --- | --- | --- | --- |
| Existing Core console/control delivery | prerequisite hook, only if needed | sequenced live edits and batch acknowledgment | `CHK-DVP-08`, `CHK-DVP-09` trace serial/threaded ordering |
| Direct lighting/reservoir/shadow owners | existing feature owners extended | admission, lobe math, exclusive producer omission | `CHK-DVP-10`, `CHK-DVP-12` pixels plus pass/work omission |
| Lit indirect estimator/resolve/history owners | existing feature owners extended | shared estimator semantics and exclusive work/reset | `CHK-DVP-10`, `CHK-DVP-12` lobe, continuation, and history oracles |
| Lighting composition/products and dependent consumers | existing product boundary extended | no missing/stale reads after producer removal | `CHK-DVP-10`, `CHK-DVP-11` binding/native checks |
| Existing graph identity/retirement | narrowly justified lifetime hook | cached topology must match accepted feature execution | `CHK-DVP-08`, `CHK-DVP-11` generation/order/in-flight proof |
| Editor top-panel and existing control interface | presentation hook | CVar-driven hierarchy and atomic bulk intent | `CHK-DVP-09` console/UI/global-scope checks |
| Renderer Public viewport, request/View, Application, RHI | no Show hook permitted | no Editor show semantics or duplicate CVar transport | `CHK-DVP-08` enclosure/stale-name audit |

**Non-goals:** Indirect Subsurface, per-viewport feature overrides, persisted Editor selection, parent CVars, `r.ShowFlags`, generic feature manager/registry/settings bag, Application translation, RHI feature state, runtime shader compilation, indirect continuation bypass, or changing Reference behavior.

**Stop conditions:** unsequenced or mid-frame CVar mutation; a missing real product/oracle; requested unsupported work quietly disappearing; unresolved shared-estimator PDFs/target semantics; no accepted graph-retirement or dependent-history reset route; stale/missing resource reads; fabricated reconstruction guides; disabled diagnostics secretly keeping producers alive; frame/host activation sprawl; Reference cross-talk; or performance claims based only on hidden pixels.

**Exit gate:** `AC-DVP-17` through `AC-DVP-25` and `AC-DVP-27`, `AC-DVP-28` through `CHK-DVP-08` through `CHK-DVP-12`. Prove executable work omission separately from numerical correctness; measured savings require retained GPU evidence. Unrun checks remain unrun.

**Current permitted prompt — discovery only:**

```text
Execute DVP-SF-P01 through DVP-SF-P05 from DebugViews/Discovery.md. Make no production-code changes. Re-audit the exact candidate and freeze the Editor/console CVar sequencing and bulk-edit route, feature-local IsEnabled/IsActive owners, shared estimator semantics, cached-graph admission/rebuild/retirement, active-product bindings, invalidation scopes, and non-zero lobe/shadow fixtures. Map each leaf/path to omitted exclusive work and remaining shared cost. Update only the owning discovery/plan/acceptance facts needed for AUTHORIZED or BLOCKED. Reject Renderer show state, request/View feature copies, Application translation, parent CVars, composite masks, dummy required products, continuation bypass, Reference cross-talk, and performance claims without GPU evidence. Run scoped documentation checks and git diff --check; report executable probes not run.
```

**Implementation prompt — valid only after Discovery records `AUTHORIZED`:**

```text
Implement DVP-4A in its bounded feature-owned order using the authorized CVar publication, graph-lifecycle, estimator, product, and history routes. Keep Show an Editor-only CVar frontend. Put IsEnabled/IsActive in feature files and keep optional admission inside their Add...Passes entry points. Remove exclusive disabled lobe/shadow work via pass omission, early uniform branches, or justified cooked variants; preserve shared work only for real active consumers. Reconcile bindings/composition/guides/diagnostics and invalidate affected histories so there are no stale reads or silent required-product fallbacks. Add the CVar-driven hierarchy with one ordered parent/reset batch and global-scope explanation. Add no RenderShowFlag set, request/View show fields, parent CVar, r.ShowFlags namespace, generic feature manager, settings mirror, runtime shader compilation, or premature IndirectSubsurface. Preserve continuation intersections and Reference policy. Execute CHK-DVP-08 through CHK-DVP-12 proportionally and stop on failed prerequisites. No finished-result mask is an accepted intermediate slice.
```

### DVP-4B - Indirect Subsurface

**Prerequisite:** `IND-D0-02` and the Indirect Lighting owner authorize subsurface lobe classification, estimator/PDF/energy semantics, reconstruction/history consequences, and a real non-zero product.

**Work:** land producer, activation, disabled-work removal, reconstruction/diagnostic contract, active composite binding, feature CVar, and Editor leaf together. The existing Indirect parent expands from two to three real children; no flag enum or placeholder is required.

**Non-goals:** reclassifying diffuse energy as subsurface, fabricated output, UI-only enablement, or opportunistic transmission/volume transport.

**Exit gate:** applicable `AC-DVP-17` through `AC-DVP-28`, including a non-zero independent oracle and exclusive-work omission. Remain **BLOCKED** while the transport decision is open.

## DVP-5 - Prove The Contract

Exercise enum/HLSL parity, every consumer, two-viewport isolation, Lit/Reference/Lit topology, exact/HDR numeric presentation, extent changes, output encoding, and advertised D3D12/Vulkan rows. Record only checks actually run in the owning completion report.

## Ready-To-Use Source Cleanup Prompt

```text
Reconcile the live Debug Views and Reference Path Tracer source to Docs/Architecture/Modules/Engine/Renderer/Features/DebugViews/Controls/ViewModes.md. Keep RenderViewMode as the sole host-independent per-view rendering choice on ViewportRenderRequest and immutable RenderView. Keep Editor labels/icons/menu layout local while using the same enum directly. Consume the value only at the owning frame-composition, raster, debug-resolve, and feature-lifecycle decisions. Delete parallel Editor enums, preset translators, visualization targets, mode-shaped show flags, selection CVars, command bridges, graph/feature settings copies, compatibility aliases, and RHI fields. Preserve ReferencePathTracer = 1 and contiguous values. Keep the Reference implementation private and the shared frame shell unchanged. Add no diagnostics, registry, generic settings bag, recipe hierarchy, or speculative controls. Run focused source checks, architecture_boundary_check, documentation link/anchor checks, and git diff --check; report builds and runtime checks as deferred unless actually run.
```

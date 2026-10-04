# Debug Views Discovery And Implementation Authorization

**Status:** discovery gate; DVP-4 implementation is blocked pending the CVar publication decision

**Current readiness:** **Not applicable** to this gate. Debug Views readiness remains owned by the [Renderer readiness row](../../../../../../Acceptance/CurrentReadiness.md#renderer).

**Responsibility:** own the unresolved decisions, risks, probes, and binary authorization gate for the next Debug Views implementation slice.

**Authority boundary:** [Controls](Controls/README.md) and [Presentation Architecture](PresentationArchitecture.md) define target behavior; [Plan](Plan.md) orders authorized work; [Acceptance](Acceptance.md) defines proof; this page alone decides whether DVP-4 may start.

**Verified:** 2026-10-04 against revision `26803f97` and the inspected dirty working tree.

**Non-claims:** source inspection below does not prove compilation, thread safety, runtime behavior, pixels, backend parity, or release acceptance.

## Decision To Make

Authorize DVP-4A only when the five current lighting products, two shadow-evaluation seams, per-view publication boundary, focused Renderer consumers, Editor interaction owner, live CVar mutation path, and indirect-history invalidation form one coherent and race-free vertical slice.

DVP-4B is a separate authorization owned jointly with Indirect Lighting discovery because `IndirectSubsurface` changes transport and product semantics.

## Iteration Control Record

| Field | Record |
| --- | --- |
| Iteration identity | `ITER-DVP-SHOWFLAGS-01`; Debug Views owner; **BLOCKED**; documentation/discovery scope only; start revision `26803f97` with a dirty tree containing unrelated user work |
| Intended decision | authorize or block DVP-4A without changing production code |
| North Star | `NS-OWNERSHIP`: advance by freezing scope and publication owners; `NS-SIMPLIFY`: advance by rejecting parent/resolved duplicates; `NS-REAL`: blocked until a consumer-visible implementation exists; `NS-EVIDENCE`: preserve, with no executable claim added |
| Persona targets | `PGE-07`, `PGE-09`, and `PGE-13`: preserve target direction; no evidence-level advance from documentation |
| Delivery target | [DVP-4](Plan.md#dvp-4---add-lighting-show-flags), `RD-3`, `FCR-REN-11`; no release verdict |
| Acceptance | `AC-DVP-17` through `AC-DVP-27` in [Acceptance](Acceptance.md) |
| Failure modes | `FM-DVP-07` through `FM-DVP-12` in [Acceptance](Acceptance.md) |
| Checks | `CHK-DVP-08` through `CHK-DVP-11` in [Acceptance](Acceptance.md) |
| Current decision | **BLOCKED** for DVP-4A by `DVP-SF-D03`, `DVP-SF-D07`, and `DVP-SF-D08`; **BLOCKED** for DVP-4B by `DVP-SF-D05` |
| Next permitted step | execute `DVP-SF-P01` and `DVP-SF-P04` as read-only ownership/semantics audits, then complete the bounded parameter and fixture probes before production edits |

## Known Current State

| Area | Observed source state | Evidence boundary |
| --- | --- | --- |
| lighting products | `DirectDiffuse`, `DirectSpecular`, `DirectSubsurface`, `IndirectDiffuse`, and `IndirectSpecular` are allocated and read by `LightingComposite` | source present only |
| indirect subsurface | no product, producer, composite input, debug mode, or admitted indirect lobe was found | `Not found`; no control may advertise it |
| direct shadows | primary-surface direct lighting reads a separately produced `ShadowVisibilitySignal` | source-present seam; bypass and raw-signal preservation unproved |
| indirect shadows | Lit indirect evaluation calls `TraceSurfacePathWithRandomFrame`; secondary-hit direct lighting calls `TraceDirectLightSample`, while continuation intersections define the path | source-present seam; control scope, Reference isolation, and temporal invalidation unproved |
| per-view mode path | `ViewportRenderRequest` is frozen into `RenderView` and Editor publishes through `ViewportPanel` | source present; show flags absent |
| CVar storage | `ConsoleVariable<T>` stores and returns a plain `T`; `SetCVar` calls `TrySetValueFromString` directly | source shape does not prove cross-thread sequencing |
| Editor control | `ViewportTopPanel` owns view-mode presentation and `EditorViewportSession` owns local selection | no Show menu or show-flag state exists |
| Renderer/RHI boundary | lighting composition is Renderer-private and RHI has no feature selector | target must preserve this absence |

## Decision Register

| ID | Decision | Status | Required evidence / consequence |
| --- | --- | --- | --- |
| `DVP-SF-D01` | The first slice contains five current non-zero lighting contributions and the two source-present shadow-evaluation seams. | **Accepted** | live product/composite and visibility-consumer audit; no placeholder Indirect Subsurface surface |
| `DVP-SF-D02` | Direct Lighting, Indirect Lighting, and Shadows parents are derived Editor bulk actions, not flags or CVars. | **Accepted** | one mutable leaf set and parent truth table |
| `DVP-SF-D03` | Live feature CVar mutation uses a sequenced Renderer-owner publication path while each feature reads at its narrow owner. | **Open / blocking** | `DVP-SF-P01`; name exact mutation thread, queue/command, ordering, shutdown, and requested/active observation |
| `DVP-SF-D04` | Hidden contributions remain produced and are suppressed only by `LightingComposite`; shadow flags preserve graph topology and the raw direct-shadow signal. | **Accepted** | preserves contribution diagnostics and avoids conflating debug visibility with work pruning |
| `DVP-SF-D05` | Indirect Subsurface exists only after `IND-D0-02` defines and admits its path class and product contract. | **Open / blocks DVP-4B** | accepted Indirect Lighting decision, producer, reconstruction/diagnostic consequences, and non-zero oracle |
| `DVP-SF-D06` | Show flags are non-persisted viewport state; CVars are non-persisted global developer gates. | **Accepted** | no rendering-settings or Application persistence hook |
| `DVP-SF-D07` | Each of the five contribution leaves has a bounded non-zero independent fixture and each shadow leaf has a controlled occluder fixture before implementation. | **Open / blocking** | `DVP-SF-P03/04` fixture and oracle ledger |
| `DVP-SF-D08` | Direct Shadows bypasses only primary direct visibility; Indirect Shadows bypasses only secondary-hit direct-light visibility in Lit, preserves continuation intersections and Reference behavior, and resets every dependent indirect temporal state. | **Open / blocking** | `DVP-SF-P04`; exact shader ABI, call sites, invalidation owner/generation, and negative controls |

## Risk Register

| ID | Cause and event | Likelihood rationale / impact | Prevention / detection | Owner | Contingency and retirement |
| --- | --- | --- | --- | --- | --- |
| `RISK-DVP-SF-01` | console thread writes plain CVar storage while Renderer reads it | likely in threaded mode from the inspected direct setter; high correctness impact | block on `DVP-SF-P01` and `CHK-DVP-08` | Core console and Renderer concurrency owners | choose one sequenced owner route; retire with source trace plus threaded toggle evidence |
| `RISK-DVP-SF-02` | parent state is stored beside children | medium because hierarchical UI invites a parent boolean; medium maintenance/behavior impact | derived-only architecture; static state-holder audit | Editor viewport owner | delete parent field/CVar; retire through `CHK-DVP-08/09` |
| `RISK-DVP-SF-03` | suppression prunes producers | medium because disabled work suggests optimization; high history/diagnostic impact | composite-only first slice; raw-product negative check | Renderer lighting owner | restore producer continuity; retire through `CHK-DVP-10` |
| `RISK-DVP-SF-04` | Indirect Subsurface is inferred in UI/composite | high because it is explicitly requested but absent; high false-capability impact | hard DVP-4B gate | Indirect Lighting owner | remove every premature surface; retire only with `IND-D0-02` evidence |
| `RISK-DVP-SF-05` | per-view and global scopes are presented as one value | medium because both use the same leaf names; medium diagnosis impact | explicit AND rule, separate console/UI truth, two-viewport matrix | Renderer contract and Editor presentation owners | improve labeling or block; retire through `CHK-DVP-09/10` |
| `RISK-DVP-SF-06` | “Indirect Shadows” disables continuation intersections or leaks into Reference | medium because visibility and path intersection share ray-tracing helpers; high transport/reference impact | freeze the secondary-hit-only seam and add Reference/continuation negative controls | Indirect Lighting and Reference owners | restore the exact scope or remove the leaf; retire through `CHK-DVP-10/11` |
| `RISK-DVP-SF-07` | shadowed and unshadowed indirect samples mix in temporal reservoirs/reconstruction | high because the current indirect route reuses temporal state; high ghosting/bias impact | enumerate and reset dependent state on effective-toggle generation | Indirect Lighting owner | block implementation until the reset set is complete; retire through `CHK-DVP-10` |

## Required Probes

### `DVP-SF-P01` - CVar Mutation And Thread Ownership

Trace Editor and runtime console execution, `SetCVar`, Renderer serial/threaded modes, the control queue, frame/pass parameter preparation, shutdown, and existing live Renderer CVar consumers. Produce one bounded owner/sequence diagram and decide among only routes that satisfy the Renderer and Editor standards. The selected route must expose requested versus active state if application is delayed.

**Stop rule:** if satisfying the boundary requires a generic callback registry, copied settings bag, Application feature translation, or unbounded Core/Renderer refactor, DVP-4A remains blocked and the CVar requirement returns for scope review.

### `DVP-SF-P02` - Per-Frame Parameter Reachability

Prove that lighting-composite, primary direct-lighting, and Lit indirect-lighting parameters are prepared from the current immutable View and current accepted feature gate every frame without graph reconstruction. Record each exact producer, thread, cadence, and shader ABI surface.

### `DVP-SF-P03` - Five-Product Non-Zero Fixtures

Identify one bounded analytic fixture per existing lobe where the target product is non-zero and independently distinguishable. These fixtures become the `CHK-DVP-10` oracle; a zero-only path cannot authorize a flag.

### `DVP-SF-P04` - Shadow Scope And Invalidation

Trace `ShadowVisibilitySignal` into primary direct lighting and trace `RestirIndirectReservoir::EvaluateCandidate` through secondary-hit direct-light visibility. Identify the narrow shader parameters that can substitute visibility `1` without changing graph topology. Enumerate every temporal reservoir, reconstruction, accumulation, confidence, or generation whose meaning changes when effective Indirect Shadows changes, and name the existing per-view invalidation owner that resets it.

Freeze two controlled occluder fixtures: one where Direct Shadows changes only primary direct lighting while the raw shadow signal remains identical, and one where Indirect Shadows changes secondary-hit direct-light contribution while continuation-hit identity and Reference output remain unchanged. If the second oracle cannot distinguish shadow visibility from path continuation semantics, the Indirect Shadows leaf remains blocked.

## Exit Rule

DVP-4A becomes **AUTHORIZED** only when `DVP-SF-D03` is accepted with `DVP-SF-P01/02` evidence, all five `DVP-SF-P03` contribution fixtures and both `DVP-SF-P04` shadow fixtures are identified, `DVP-SF-D08` has an accepted invalidation route, every risk has a live prevention/check mapping, and the [Plan](Plan.md#dvp-4---add-lighting-show-flags) requires the resulting route without choosing new architecture during implementation.

DVP-4B remains **BLOCKED** until `DVP-SF-D05` passes in the owning Indirect Lighting package. Documentation length, a disabled UI row, or a zero texture cannot satisfy either gate.

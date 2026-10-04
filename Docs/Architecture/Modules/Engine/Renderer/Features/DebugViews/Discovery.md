# Debug Views Discovery And Implementation Authorization

**Status:** discovery gate; DVP-4 implementation is blocked pending CVar publication and feature-execution decisions

**Current readiness:** **Not applicable** to this gate. Debug Views progress remains owned by the [Renderer readiness row](../../../../../../Acceptance/CurrentReadiness.md#renderer).

**Responsibility:** own unresolved decisions, risks, probes, and binary authorization for the next Debug Views implementation slice.

**Authority boundary:** [Controls](Controls/README.md) and [Presentation Architecture](PresentationArchitecture.md) define behavior; [Plan](Plan.md) orders authorized work; [Acceptance](Acceptance.md) defines proof; this page decides whether DVP-4 may start.

**Verified:** 2026-10-04 against revision `bbb9f7ed` and the inspected dirty working tree.

**Non-claims:** source inspection does not prove compilation, thread safety, runtime behavior, pixels, GPU cost removal, backend parity, or release acceptance.

## Decision To Make

Authorize DVP-4A only when Editor and console can safely edit one feature-CVar authority, and each lighting/shadow owner can admit execution, remove exclusive disabled work, preserve valid shared work, and retire/reset its products and histories without a hidden fallback.

The revised [Show-menu design](Controls/ShowFlags.md) supersedes the former per-view bit set and composite-only masking target. It is an accepted design direction, not authorization or executable evidence. DVP-4B separately requires the Indirect Lighting transport/product decision.

## Iteration Control Record

| Field | Record |
| --- | --- |
| Iteration identity | `ITER-DVP-SHOWFLAGS-02`; Debug Views owner; **BLOCKED** for implementation; design revision only; start `bbb9f7ed` with unrelated shader/Launcher changes preserved |
| Intended outcome | replace hidden-result masking with feature-owned execution and CVar-driven Editor UI |
| North Star | `NS-OWNERSHIP` / `NS-SIMPLIFY`: advance target enclosure and single authority; `NS-REAL` / `NS-EVIDENCE`: preserve, no implementation or evidence-level advance |
| Persona targets | `PGE-07`, `PGE-09`, `PGE-13`: preserve direction; no runtime/performance claim |
| Delivery target | [DVP-4](Plan.md#dvp-4---add-lighting-show-flags), `RD-3`, `FCR-REN-11`; no release verdict |
| Acceptance / failures / checks | `AC-DVP-17`–`28`, `FM-DVP-07`–`14`, `CHK-DVP-08`–`12` in [Acceptance](Acceptance.md) |
| Current decision | DVP-4A **BLOCKED** by `DVP-SF-D03`, `DVP-SF-D07`, `DVP-SF-D08`, `DVP-SF-D09`; DVP-4B **BLOCKED** by `DVP-SF-D05` |
| Next permitted step | read-only ownership audits and bounded local probes `DVP-SF-P01`–`P05`; production edits require recorded authorization |

## Known Current State

| Area | Observed source state | Evidence boundary |
| --- | --- | --- |
| lighting products | five targets allocated, direct lobes share a shader, indirect lobes share estimator/resolve, composite reads all five | source only; no live lobe activation/removal proved |
| indirect subsurface | no admitted product/producer/composite input found | `Not found`; do not expose a control |
| direct shadows | direct family creates a separate signal and consumes it in primary lighting | source seam; producer omission and estimator consequences unproved |
| indirect shadows | Lit estimator reaches secondary-hit direct visibility through shared path helpers | source seam; branch placement and Reference isolation unproved |
| graph lifetime | `FramePipeline` caches graph construction and owns topology refresh/retirement | no proof that these proposed CVars invalidate/rebuild cached topology |
| histories | `RestirLightingInvalidation` hashes scene, shadow bias/distance, and bounce count | no proposed lobe/shadow activation values covered yet |
| guides and initialization | indirect resolve writes reconstruction guides; lighting targets have a shared clear pass | dependencies must be audited before omitting producers/writes |
| CVar storage | plain `ConsoleVariable<T>` storage and direct generic setter in the prior inspection | serial/threaded publication and safe UI query still unproved |
| Editor | top panel owns mode presentation; no Show menu exists | CVar-driven UI and ordered batch edits unimplemented |
| Renderer/RHI | no implemented Show contract | target preserves this absence; feature helpers remain private |

## Decision Register

| ID | Decision | Status | Required evidence / consequence |
| --- | --- | --- | --- |
| `DVP-SF-D01` | Initial scope is five real lobes and two controlled shadow seams. | **Accepted target** | confirm real non-zero products/oracles at the implementation candidate |
| `DVP-SF-D02` | Parent rows derive intent from child CVars and edit them in one batch. | **Accepted target** | no parent CVar or viewport-local selection mirror |
| `DVP-SF-D03` | Editor/console mutation and query use one sequenced control authority; frames see one accepted batch. | **Open / blocking** | `P01`: exact thread, publication, query, ordering, acknowledgment, shutdown |
| `DVP-SF-D04` | Feature-owned IsEnabled/IsActive removes exclusive work instead of masking completed products. | **Accepted target; execution probes open** | `P02/05`: omission and product/lifetime mechanism; old continuity/masking decision removed |
| `DVP-SF-D05` | Indirect Subsurface requires `IND-D0-02` and a real product. | **Open / blocks DVP-4B** | accepted owning transport decision, producer, and non-zero oracle |
| `DVP-SF-D06` | Show is an Editor CVar frontend with global scope, not persisted per-view rendering state. | **Accepted target** | no Show transport on request/View, no Editor selection persistence |
| `DVP-SF-D07` | Each leaf has non-zero pixel and executable-work oracles before implementation. | **Open / blocking** | `P03/04/05`: fixtures, pass/branch/ray omission, remaining shared-cost ledger |
| `DVP-SF-D08` | Lobe/shadow changes reset all dependent temporal state; indirect shadow bypass preserves continuation and Reference. | **Open / blocking** | `P04/05`: exact caller/ABI, affected histories, invalidation owners and negative controls |
| `DVP-SF-D09` | Each supported path has valid active products, sampling/estimator semantics, and a safe cached-graph admission/retirement route. | **Open / blocking** | `P02/05`: branches/variants, PDFs/targets, guides, bindings, topology identity and in-flight retirement |

## Risk Register

| ID | Failure | Prevention / check | Owner / retirement |
| --- | --- | --- | --- |
| `RISK-DVP-SF-01` | plain storage races or a frame observes half a parent edit | block on sequenced mutation/query and batch probe; `CHK-DVP-08`, `CHK-DVP-09` | existing console and Renderer control owners; retire with threaded evidence |
| `RISK-DVP-SF-02` | parent/UI mirror becomes a second authority | derive from CVars; source/state-holder audit | Editor owner; `CHK-DVP-08`, `CHK-DVP-09` |
| `RISK-DVP-SF-03` | producer omission leaves stale reads or enabled-required work silently disappears | active-product/required-failure oracles; no zero-as-success | lighting product owners; `CHK-DVP-10`, `CHK-DVP-11` |
| `RISK-DVP-SF-04` | absent Indirect Subsurface is advertised | hard owning transport/product gate | Indirect Lighting; remove premature surfaces, retire with `IND-D0-02` |
| `RISK-DVP-SF-05` | UI implies viewport-local state or active support from an enabled CVar | shared-scope UI and enabled/active/unavailable matrix | Editor and feature owners; `CHK-DVP-09`, `CHK-DVP-10` |
| `RISK-DVP-SF-06` | shadow bypass deletes continuation traces or leaks into Reference | Lit-caller policy and continuation/Reference negative controls | indirect/Reference owners; `CHK-DVP-10`, `CHK-DVP-11` |
| `RISK-DVP-SF-07` | old lobe/shadow state survives disable/re-enable | enumerate reset set and semantic identity | history owners; `CHK-DVP-10` |
| `RISK-DVP-SF-08` | hidden pixels are mistaken for removed cost; cached graph still dispatches disabled work | pass/ray/branch evidence and honest shared-cost accounting | feature/graph owners; `CHK-DVP-12` |
| `RISK-DVP-SF-09` | lobe pruning invalidates PDFs/reservoir targets or removes mandatory guides | estimator and reconstruction-owner review before execution changes | direct/indirect/provider consumers; `CHK-DVP-10`, `CHK-DVP-11` |

All risks remain open until the mapped checks have candidate evidence; target wording does not retire a risk.

## Required Probes

### `DVP-SF-P01` - CVar Mutation And Thread Ownership

Trace Editor/runtime console, direct SetCVar, serial/threaded Renderer, control queues, safe UI queries, frame admission, shutdown, and existing live feature CVars. Choose one existing-owner route for single edits and parent/reset batches. Name requested/applied observation when delivery is delayed.

**Stop rule:** a generic callback registry, settings bag, Application feature translation, private Renderer headers in Editor, or an unbounded cross-module refactor keeps DVP-4A blocked.

### `DVP-SF-P02` - Feature Activation And Cached Graph Reachability

For each leaf/path, locate the feature-local IsEnabled/IsActive owner, admission entry point, shader parameter/variant ABI, resources and consumers. Determine whether a runtime branch suffices or graph topology must change. Trace accepted CVar edits into existing graph identity/rebuild/retirement; prove admission, bindings, shader values, and invalidation observe one accepted frame state. Do not assume a per-frame parameter update removes a cached dispatch.

### `DVP-SF-P03` - Five-Product Non-Zero Fixtures

Identify one bounded analytic fixture per current lobe with independent non-zero energy. Freeze all-on and each-off references before changing implementation. Define a missing-prerequisite challenge for enabled work and a disabled diagnostic oracle. Record format tolerances and any shared-estimator sample/PDF consequences.

### `DVP-SF-P04` - Shadow Scope And Invalidation

Trace primary direct visibility and secondary-hit direct-light visibility, including every visibility-dependent reservoir target/weight. Name the exact bypass/producer omission route and affected temporal reservoir, reconstruction, accumulation, confidence, and guide owners.

Freeze primary/secondary occluder fixtures: Direct Shadows off matches fully visible primary lighting with exclusive signal work absent; Indirect Shadows off removes secondary-hit visibility traces while continuation-hit identity and Reference policy remain unchanged. Do not require the raw disabled shadow diagnostic to remain produced. If shared work genuinely needs visibility, freeze and disclose that dependency before claiming removal.

### `DVP-SF-P05` - Work Removal, Products, And Shared Estimators

Produce one bounded per-path execution ledger covering each leaf and all-off group:
exclusive and shared passes, trace categories, lobe math/writes, active-product bindings, bounded branches/permutations, sampling/PDF/reservoir semantics, guide production, history resets, and remaining cost.

Prefer no resource/read/write for inactive contributions. Justify intentional current-frame disabled-zero initialization only where a selected fixed-output ABI requires it; it must never satisfy missing enabled work or fabricate guides. Freeze pass/dispatch and shader/ray oracles plus the same-scene GPU timing protocol. These probes authorize implementation, not performance acceptance.

## Exit Rule

DVP-4A becomes **AUTHORIZED** only when `DVP-SF-D03`, `DVP-SF-D07`, `DVP-SF-D08`, `DVP-SF-D09` have accepted `P01`–`P05` evidence, the execution ledger has no unresolved transport/product/history/lifetime route, every risk has a live check mapping, and [Plan](Plan.md#dvp-4---add-lighting-show-flags) requires those routes without delegating architecture decisions to implementation.

DVP-4B stays **BLOCKED** until `DVP-SF-D05` passes in the Indirect Lighting package. Design edits, zero-only fixtures, hidden pixels, or an unchecked UI row cannot satisfy either gate.

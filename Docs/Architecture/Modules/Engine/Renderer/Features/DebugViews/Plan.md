# Debug View Staged Delivery Plan

**Status:** implementation plan; not proof of build, runtime, visual, backend, or release acceptance

**Responsibility:** own dependency order, change maps, integration ledgers, stop conditions, and exit gates for Debug Views delivery.

**Current readiness:** **Not applicable** to this plan; see the [Debug Views dossier](README.md) and central [Renderer readiness row](../../../../../../Acceptance/CurrentReadiness.md#renderer).

**Verified baseline:** 2026-10-04 at revision `26803f97` with an inspected dirty working tree; every implementation stage must re-audit its candidate.

**Planning iteration:** `ITER-DVP-SHOWFLAGS-03`; documentation-only stage refinement; start `bbb9f7ed` with pre-existing design, shader and Launcher edits preserved. `NS-OWNERSHIP`/`NS-SIMPLIFY` advance the target delivery discipline; `PGE-07`/`PGE-09`/`PGE-13` and runtime/GPU readiness are preserved, not advanced. The applicable proof rows remain owned by Discovery/Acceptance; no new candidate result.

**DVP-4 plan revision:** `DVP-SF-IP-01`, 2026-10-04; drafted from `bbb9f7ed`, reconciled at handoff with `84b7c5a4` after the prior design changes were committed. Concurrent Editor work is outside this planning slice. Ten bounded stages refine CVar-driven feature execution; required discovery and prerequisite repairs are the first delivery work whenever the selected stage is not yet ready. This adds no executable evidence for DVP-0 through DVP-5.

**Architecture authority:** [Viewport Rendering Controls](Controls/README.md) and [Debug View Presentation Architecture](PresentationArchitecture.md)

**Implementation authorization:** [Discovery](Discovery.md)

**Feature acceptance:** [Acceptance](Acceptance.md)

**Prerequisite-closure revision:** `ITER-DVP-PREREQ-04`, 2026-10-05 at `410d05ef`, with pre-existing Debug Views, lighting and shader edits preserved. Documentation-only: every selected stage includes automatic prerequisite closure; acceptance criteria and executable evidence are unchanged. Scope is this plan, not other feature plans.

**Execution-policy iteration:** 2026-10-04 at `d29d2351614a98f101537e028267da2dc99ee9ec`; preserve the two pre-existing Discovery edits. This documentation slice applies autonomous prerequisite repair/resumption to all 13 copy-ready prompts and removes manual Editor-opening/interaction gates through the Acceptance owner. No production/API/copy/hook/variant delta or new implementation verdict; retained automated AC/FM/CHK oracles are unchanged.

**Current shader-policy revision, 2026-10-05:** the user requires runtime-uniform behavior, superseding earlier family-presence, primary-shadow and guide-write schema alternatives. Each operation has one registration. Feature owners retain initialized fixed-ABI bindings: five radiance targets even with inactive families, a visibility target while Direct evaluates, and four guide targets while Indirect resolves. Inactive families still omit reservoirs/history and exclusive evaluation/tracing; inactive primary shadows omit tracing; unrequested guides omit guide writes. No allocation or GPU-time saving is inferred from these branches. Frame remains intent-based and consumes the existing semantic topology identity. Prior candidate-bound variant/omission results require revalidation for this source revision; see [Discovery](Discovery.md#current-candidate-evidence-and-permission).

## Stage Selection Includes Prerequisite Delivery

Selecting any stage in this plan selects its necessary dependency closure, even when predecessors or owning repairs were not separately queued. The executor must first establish missing prerequisites under the [Universal Execution Contract](#universal-execution-contract), then deliver and validate the requested stage. `After`, `Prerequisites`, `only`, `read-only discovery`, and a stage's non-goals constrain its own dependent work; they never prohibit the separately recorded prerequisite work needed to make that stage executable. This policy applies to DVP-0 through DVP-5, all DVP-4A/4B substages, and every copy-ready prompt. It takes precedence over a generic template instruction to end work solely because a prerequisite is missing.

An isolated request ends after the selected stage passes, not before its prerequisites are repaired. Later unrelated stages are not implicitly selected. Stage gates remain proof obligations: establish them, do not skip them or declare them passed from documentation alone.

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

**Plan identity:** `DVP-SF-IP-01`; staged refinement of the accepted [Show-menu design](Controls/ShowFlags.md), using the repository [staged-plan template](../../../../../../Engineering/Workflow/Templates/FeatureDeliveryPackage.md#scaffold-planmd--staged-delivery-and-copy-ready-prompts).

**Current permission:** DVP-4A-7 is user-selected. [Discovery's UI admission](Discovery.md#stage-7-ui-admission) records its bounded menu/control result and lighting-schema overlap requiring prerequisite reconciliation. Stage 8 is not admitted or queued until that reconciliation and the complete Stage-7 exit pass. The [accepted admission ledger](Discovery.md#accepted-admission-ledger) remains the estimator/product/guide/diagnostic/topology authority; candidate results remain in FCR-REN-11, not this plan.

The [2026-10-04 candidate audit](Discovery.md#dvp-4a-0-candidate-and-probe-record) stopped at the DVP-4A-0 blocking prerequisites. The subsequent bounded route admits delivery repair first; the selected stage's dependency closure requires executing that repair and returning with proof. Do not interpret the source dependency ledger as frozen lighting execution architecture or replay DVP-0 through DVP-3.

The existing DVP-0 through DVP-3 mode/presentation work is a baseline to reconcile, not a prerequisite instruction to replay. Do not reinstate superseded selectors or reimplement source-present mode controls. Numeric mode parity, presentation domains and Reference selection remain preservation obligations under their own owners.

#### Staged Sequence And Estimate Envelope

| Stage | Observable outcome | Dependency | Engineering envelope | Review/evidence envelope | Largest uncertainty |
| --- | --- | --- | --- | --- | --- |
| [`DVP-4A-0`](#dvp-4a-0---freeze-the-candidate-and-execution-decisions) | Freeze The Candidate And Execution Decisions | none; read-only discovery | 6–16 h | 4–8 h | CVar ordering and shared estimator/product contracts |
| [`DVP-4A-1`](#dvp-4a-1---prove-sequenced-cvar-delivery) | Prove Sequenced CVar Delivery | 0; prerequisite-only authorization if needed | 8–24 h | 4–8 h | existing console/query and Renderer control thread boundaries |
| [`DVP-4A-2`](#dvp-4a-2---deliver-direct-subsurface-as-the-first-vertical-slice) | Deliver Direct Subsurface As The First Vertical Slice | 1 proof; Discovery AUTHORIZED | 8–16 h | 4–8 h | shared direct shader output/binding strategy |
| [`DVP-4A-3`](#dvp-4a-3---complete-direct-lobes-and-all-off-admission) | Complete Direct Lobes And All-Off Admission | 2 | 12–24 h | 4–8 h | direct reservoir and shadow dependencies with no active lobe |
| [`DVP-4A-4`](#dvp-4a-4---remove-exclusive-direct-shadow-work) | Remove Exclusive Direct Shadow Work | 3 | 8–16 h | 4–8 h | visibility-dependent reservoir weighting |
| [`DVP-4A-5`](#dvp-4a-5---deliver-indirect-lobes-and-shared-estimator-admission) | Deliver Indirect Lobes And Shared-Estimator Admission | 4; indirect semantic ledger accepted | 24–48 h | 8–16 h | sampling/PDF/target and reconstruction-guide coupling |
| [`DVP-4A-6`](#dvp-4a-6---bypass-secondary-hit-shadow-visibility) | Bypass Secondary-Hit Shadow Visibility | 5 | 12–24 h | 4–8 h | shared path helper policy and temporal reuse |
| [`DVP-4A-7`](#dvp-4a-7---add-the-cvar-driven-editor-show-menu) | Add The CVar-Driven Editor Show Menu | 6; all seven consumers proved | 8–16 h | 4–8 h | safe query refresh and keyboard/mode UX |
| [`DVP-4A-8`](#dvp-4a-8---close-backend-provider-and-execution-evidence) | Close Backend, Provider And Execution Evidence | 7; frozen matrix and measurement protocol | 12–24 h | 8–16 h | hardware/tool access and provider-compatible products |
| [`DVP-4A-9`](#dvp-4a-9---adopt-the-controls-and-close-the-slice) | Adopt The Controls And Close The Slice | 8; conjunctive acceptance | 4–8 h | 4–8 h | orphan consumers and candidate/report consistency |

These are initial planning ranges, not commitments or measured throughput. They assume one experienced engine engineer, focused C++/shader build tools, controlled fixtures, existing GPU capture support and access to each advertised backend/provider cell. Waiting for independent control/concurrency, lighting-estimator, or graphics/evidence review is not included. Stage 0 revises ranges from its exact change map before production authorization. Stage 1 becomes a proof-only step if the current delivery route already satisfies the contract; no refactor is justified by an estimate.

The critical path is the table order. A selected stage can be split into smaller same-owner batches, but it cannot skip its coherent product/history/binding closure or authorize a later stage on source presence alone. If review finds a new architecture/transport/provider decision, return to Discovery and the owning contract; do not grow the current prompt's scope.

#### Universal Execution Contract

Every copy-ready prompt below incorporates this section and its selected stage's objective, prerequisites, work, non-goals, exit and stop rules.

**Mandatory prerequisite-first execution (all stages and copy-ready prompts):** a missing, failed, stale or contradicted prerequisite is the executor's next work item, never sufficient reason for a BLOCKED-only handoff. Before dependent implementation:

1. Re-audit the selected stage's exact candidate, prerequisite artifacts and owning decisions. Reuse current passing evidence; do not replay an unaffected predecessor.
2. Trace each unmet requirement to its owning stage or Architecture/Discovery/Acceptance subject and recursively establish its prerequisites in dependency order. Missing code, an unclosed design decision, failed checks, stale products and missing proof all require work, not a status-only response.
3. Record the smallest coherent owning repair, affected files/API/hooks/copies, risks and defect-detecting checks in the existing change/report owner. Establish missing semantic/design decisions there before production edits; then implement required in-scope changes through the existing production path. Revise the bounded repair ledger when new necessary work is exposed, rather than using its absence as a reason to stop. No routine approval is needed for this dependency closure.
4. Run the prerequisite's required positive and negative checks, regenerate affected disposable products, invalidate stale evidence and repair any falsified result. Acceptance thresholds, scope and independent-review requirements are not waived or manufactured to obtain a pass.
5. Update the owning decisions and candidate-bound evidence, return automatically to the selected stage, and deliver its outcome. Repeat this loop whenever a prerequisite or stage check fails. Continue to later stages only when user-queued.

Stop conditions suspend unsafe dependent edits, not the repair loop. A discovery-only or no-production-edit budget applies to that discovery activity; necessary production repair runs as a separately recorded owning prerequisite within the selected stage's dependency closure. Cross-owner work is not an instruction to ask another executor to do it or to wait passively. Keep working while safe dependency work remains.

Only an exhausted dependency that genuinely requires unavailable hardware/access, new external authority, or a material user-only product choice permits an actionable handoff. State the exact unresolved requirement, attempted safe remedies, remaining input and resume point. Such a limit never authorizes a false pass, a weaker oracle, silent product/transport expansion or a fallback. This documentation change does not itself execute or accept any production stage.

**Automated verification policy:** no stage requires a person to open the Editor, inspect screenshots, click widgets or perform manual keyboard/focus/layout checks. Use agent-run native fixtures, control-route probes, source/ABI checks and scoped builds/cooks for retained correctness obligations. Manual-only interaction/appearance checks are optional follow-up observations, not admission or completion gates; record them as outside this automated delivery scope, never as passed. Preserve the automated menu/console intent, batch, mode-isolation, product, GPU-execution, history and backend obligations in [Acceptance](Acceptance.md).

1. Read `AGENTS.md`, `Docs/README.md`, [Change Integration](../../../../../../Engineering/Workflow/ChangeIntegration.md), [Change Lifecycle](../../../../../../Engineering/Workflow/ChangeLifecycle.md), the [Engineering task map](../../../../../../Engineering/README.md#choose-by-task), and the exact Show/Discovery/Acceptance and prior-stage artifacts. Select Renderer, Editor, ownership/copy/naming/style/concurrency/validation rules according to the changed responsibility; do not copy their standards into implementation.
2. Record a small iteration control record in the existing change/report owner: stage ID, candidate revision and dirty boundary, prerequisite revisions, mapped AC/FM/CHK/RISK rows, intended outcome and permitted files/hooks. Preserve unrelated and concurrently appearing changes.
3. Before editing, audit current owners, producers, consumers, lifetime, public APIs, CMake/shader-cook/generated membership and the frozen execution ledger. Source paths below are inspection starting points, not permission for every listed file to change.
4. Work on one coherent selected stage or its recorded owning prerequisite repair at a time; apply the autonomous execution policy before handing off a failed gate. No new public type, state holder, helper, copy, configuration, variant or outside-feature hook without a current consumer, lifetime reason and accepted check. Register a feature CVar only in the stage delivering its real execution consumer.
5. Reuse existing owners; apply a clean break to replaced owned paths and reconcile all direct consumers immediately. Generic orchestration names semantic operations; feature admission, resources, shader bindings, estimator policy and failures stay at their narrow owners.
6. Every stage runs the applicable `CHK-DVP-08` architecture-fitness audit: changed public surface, state/copy inventory, dependency direction, definition-to-use placement, repeated CVar/predicate searches, every outside-feature occurrence with hook role, and bounded-removal reasoning. No permanent architecture-test framework or disposable submitted fixture is required. Run `architecture_boundary_check` when Renderer/RHI boundaries change.
7. Predeclare the cheapest claim-falsifying check, exact candidate/configuration/backend, oracle, tolerances/samples, artifact and escalation trigger. Use scoped formatting/source/compile/shader checks first; escalate only to the stage's required runtime/GPU cells. Do not replace an oracle with a full workspace build/cook.
8. Finish each batch with responsibility refinement and a scoped diff review. Keep functions/files cohesive; split genuinely independent policy/mechanism, not into forwarding-only wrappers or numbered fragments. Delete dead interim masks/holders/includes, not merely hide them.
9. Every prompt's `NON-NEGOTIABLE` paragraph is an exit gate. Quote each item with proof at its required level. Repair failed prerequisites through their owners and repeat affected checks before resuming; never use a failed or unavailable mandatory check as next-stage authorization. A repairable gate is a work item, not a terminal handoff.
10. Handoff records exact commands/results/artifacts, changed/deleted files by responsibility, public/API/copy/hook deltas, performance classification, cleanup, remaining risks, prerequisite validity and whether the named next stage is authorized. Stage results belong in the issue/candidate-bound `FCR-REN-11` owner, not an implementation diary in this plan.

#### Cross-Stage Invariants And Drift Stops

- `IsEnabled` is accepted feature intent; `IsActive` adds actual path/consumer/prerequisite support. Off/inapplicable is not broken; enabled but unavailable cannot silently succeed.
- Every stage preserves [AC-DVP-29](Acceptance.md#completion-criteria): direct feature-owned CVar Get/Set, no CPU value cache or forwarding chain, no per-leaf orchestration arguments or settings body hiding a long parameter list. Readability of the frame takes priority over avoiding repeated CVar reads. Thread semantics stay in Core/control ownership, not in FramePipeline; no thread-named accessor is added as an alias without a distinct enforced contract.
- Every stage preserves [AC-DVP-30](Acceptance.md#completion-criteria): clients receive narrow semantic capabilities, not implementation owners. Application binds console/menu control and owns frame orchestration; console clients neither take Renderer nor schedule rendering. Audit constructor/include/call/lifetime boundaries under CHK-DVP-08 before accepting a stage; no facade overload, forwarding service or service locator may retain the replaced coupling.
- The binding [System Core And Client Separation](../../../../../../Engineering/Foundations/ModuleOwnership.md#system-core-and-client-separation) standard drives every current and later stage, including generic Core/control work and client composition. AC-DVP-30 specializes that rule for this feature; it is not a separate engineering authority. A violated client/core boundary blocks the stage regardless of compilation or functional results.
- Show stays Editor presentation of process-global feature CVars. No Renderer Show type, request/View feature carrier, per-viewport mirror, parent CVar, generic manager or Application/RHI feature translation.
- One accepted batch/frame state feeds admission, topology, pass parameters and affected-history identity; no independently reread half-frame policy.
- Disabled exclusive evaluation/traces/writes do not run. Shared work requires a named active consumer; initialized disabled outputs require the frozen fixed-ABI justification, never enabled-missing-work or fabricated-guide substitution.
- Activation and semantic invalidation stay feature-owned. Cached graph changes use the existing lifecycle/retirement boundary; no unowned resources or stale history survive disable/re-enable.
- Active transport, Reference, emissive/sky and selected provider contracts stay valid. No continuation-as-shadow shortcut, silent provider substitution, new lobe meaning or post-result tolerance change.
- Each vertical slice reconciles producer, parameters/shader, output consumer, history, diagnostics, build/cook membership and documentation together. No interim finished-result mask is admissible.
- Temporary validation probes remain local-only and are removed before handoff; valid stage evidence may be reused only while its candidate inputs and oracle remain unchanged.

#### Source And Build Change Map

| Responsibility | Existing inspection home | Permitted shape |
| --- | --- | --- |
| CVar registration/parsing/query | `Engine/Core/{Public,Private}/Console` | extend existing generic authority only under the prerequisite's approved budget |
| Renderer sequencing/completion | `Engine/Renderer/Private/Concurrency/{Control,Coordinator}` | existing-owner delivery/lifetime hook; no lighting policy |
| Direct lobe evaluation/reservoirs | `Engine/Renderer/Private/Passes/Lighting/Direct` and corresponding shaders | feature-local controls/admission and focused bindings/evaluation |
| Primary visibility | `Engine/Renderer/Private/Passes/Lighting/Shadows` | direct shadow admission/resources/bindings |
| Lit indirect estimation/resolve | `Engine/Renderer/Private/Passes/Lighting/Restir/Indirect`, `RayTracing/Effects/RestirLighting` and existing shared shader owners | keep controls with their consuming feature; shared tracing gets narrow caller policy, not CVar reads |
| Lighting product/history consumers | existing Lighting targets/composite/invalidation, visualization and guide/provider consumers | only active-product and reset hooks required by the selected slice |
| Cached graph lifetime | `FramePipelineGraph.cpp` and existing graph identity/retirement owners | feature-contributed topology identity; no leaf policy or Show set in the frame shell |
| Editor presentation | `Engine/Editor/Private/Panels/ViewportTopPanel` and existing console/control access | widgets/labels only, no private Renderer headers or mutable feature truth |
| Build and generated products | owning CMake membership, shader registrations/cook products | update affected producer/consumer ABI and regenerate scoped disposable output together |

Do not pre-create any proposed control/settings/activation file. Reuse the cohesive existing owner when it can carry the responsibility; add a feature-local unit only after the stage's definition/usage audit proves it has independent policy/mechanism and real consumers.

**Data/copy budget:**

| Value | Authority | Boundary / retained reason | Lifetime |
| --- | --- | --- | --- |
| feature intent | feature-owned CVar | existing sequenced control delivery; no request/View mirror | process / accepted control value |
| menu check state | CVar query | transient UI projection, not independently editable truth | current UI refresh |
| active pass parameters | owning lighting feature | CPU/GPU ABI; one consistent accepted frame decision | pass / prepared frame |
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

### DVP-4A-0 - Freeze The Candidate And Execution Decisions

**Objective:** Freeze the exact execution design and validation controls so no implementation prompt has to invent architecture.

**Prerequisites to establish automatically:** Read the current Show-menu contract, Discovery, Acceptance, repository template, and owning Lighting contracts. Reconcile DVP-0 through DVP-3 against the live candidate; do not rerun their historical migration prompts merely because they precede DVP-4.

**Work:**

1. Execute DVP-SF-P01 through P05. Trace Core CVar parse/set/query, Renderer serial/threaded command ordering, cached graph construction/retirement, feature entry points, shared shaders/estimators, guide consumers, and history invalidation.
2. Freeze each leaf/path's owner, helper inputs, exclusive/shared work, uniform branch, resource/read/write disposition, disabled diagnostic result, estimator/PDF/target semantics, and reset scope. Choose exactly one admitted route per path; alternatives do not remain implementation-time decisions.
3. Freeze all-on/each-off/group-off fixtures, decoded-format and statistical tolerances, seeds/sample budgets, backend/provider cells, GPU measurement method, and negative controls before candidate results. Define mandatory guides and missing-enabled-product failure behavior.
4. Record the decision and exact revision in Discovery. If CVar delivery requires production repair, establish and execute the bounded prerequisite stage DVP-4A-1 with an explicit file/API/hook budget, validate it, and return automatically. Establish remaining owning prerequisites the same way before dependent lighting edits. Authorize DVP-4A only with the required probe evidence; retain proposals as proposals, not existing behavior.
5. Resolve falsified lighting baselines at their existing owning plans before freezing feature-control equivalence: [Direct baseline dependency](../Lighting/DirectLighting/Discovery.md#debug-controls-baseline-dependency) and [Indirect lobe accounting](../Lighting/IndirectLighting/Discovery.md#debug-controls-lobe-accounting-dependency). DVP-4A-0 may execute independent bounded discovery probes and prepare fixtures/protocols, but cannot repair material/transport semantics under its no-production-edit budget. A corrected lighting baseline precedes the all-on reference; never require preserving a known defect while simultaneously claiming the corrected owner contract. Return here with the exact repaired candidate, accepted owner decisions and new baseline artifacts. Do not replay DVP-4A-1 unless its production inputs changed.

**Non-goals:** No production edits, feature CVar registration, new public types, renderer-mode migration, external research expansion, or changed transport/product scope.

**Exit gate:** Discovery owns an accepted route/fixture ledger for every included cell, with no semantic, scope, lifetime, output, or evidence choice left to later prompts. CHK-DVP-08 source/ownership coverage is recorded; D03/07/08/09 dispositions identify what is accepted and what still needs the prerequisite repair.

**Stop conditions:** Any unresolved route, missing non-zero oracle, unbounded control refactor, unsupported guide contract, or need to change accepted semantics without returning to its owner. The shared architecture-fitness and clean-break gate is mandatory; source presence does not satisfy a required executable oracle.

**Risk/failure traceability:** `RISK-DVP-SF-01`, `RISK-DVP-SF-02`, `RISK-DVP-SF-03`, `RISK-DVP-SF-04`, `RISK-DVP-SF-05`, `RISK-DVP-SF-06`, `RISK-DVP-SF-07`, `RISK-DVP-SF-08`, `RISK-DVP-SF-09` in [Discovery](Discovery.md#risk-register); `FM-DVP-07`, `FM-DVP-08`, `FM-DVP-09`, `FM-DVP-10`, `FM-DVP-11`, `FM-DVP-12`, `FM-DVP-13`, `FM-DVP-14` in [Acceptance](Acceptance.md#failure-modes). The exit's AC/CHK rows and those owners define proof, not a second criterion set here.

**Ready-to-use discovery prompt:**

```text
AUTONOMOUS EXECUTION: Selecting this stage includes its necessary prerequisite closure, even if predecessors and owning repairs were not separately queued. First audit and recursively establish every missing, failed, stale or contradicted prerequisite at its owner; implement required in-scope repairs, run the required checks, refresh invalidated evidence, then automatically resume and deliver this stage. Do not hand off merely because a stage is BLOCKED. Apply the Universal Execution Contract; only an exhausted external dependency or material user-only choice permits an actionable handoff. Agent-run checks are required; manual Editor interaction is not a prerequisite. Preserve truthful evidence and stop after this stage unless later stages are user-queued.

Execute only DVP-4A-0 of Docs/Architecture/Modules/Engine/Renderer/Features/DebugViews/Plan.md. Apply its Universal Execution Contract. Keep discovery itself read-only; execute necessary production fixes as recorded owning prerequisite repairs, validate them and return. Execute Discovery probes P01–P05 against the exact candidate and freeze the seven-leaf ownership/execution/product/history ledger, sequenced CVar mutation/query/batch route, graph lifetime, uniform policy, shared-estimator semantics, and predeclared numeric/GPU oracles. Reconcile the current mode baseline without replaying obsolete migrations. Update Discovery and directly affected target/check facts only.

NON-NEGOTIABLE: No unresolved correctness/architecture/UX/evidence decision may be delegated to a production prompt. When publication is not yet proved, establish the bounded owning repair, execute it and prove publication before resuming discovery and dependent lighting work. Quote each gate with exact source/probe evidence, open decisions, and permitted next stage.

Do not bypass predecessor gates or silently expand the accepted hook/API/copy/variant budget. Apply non-goals and stop rules to dependent work; repair missing, stale or contradicted prerequisites at their owners, prove the repair and resume instead of choosing an unaccepted alternative.

Validate: Run scoped links/anchors, placeholder/ID/hook checks, UTF-8, and git diff --check; use only bounded local discovery probes required by a named decision. No broad build/cook or runtime acceptance claim. Retain the applicable CHK-DVP-08 hook/public-surface/definition-to-use audit and quote each required result; never claim an unrun check passed.

Handoff: report exact candidate/prerequisites, files by responsibility, deletions, copy/API/hook deltas, commands/configurations/results/artifacts, cleanup, performance classification, open risks and unavailable checks. State whether DVP-4A-1 is permitted; continue automatically when it is user-queued and its prerequisites pass; otherwise retain isolated-stage scope.
```

### DVP-4A-1 - Prove Sequenced CVar Delivery

**Objective:** A real existing Renderer CVar can be edited and queried without a race or partially applied bulk frame.

**Prerequisites to establish automatically:** DVP-4A-0 records the exact approved mutation/query/batch route. Discovery explicitly authorizes this prerequisite's file/API delta if delivery is broken. A correct existing route requires proof, not replacement.

**Work:**

1. Extend only the existing Core console/control and Renderer sequencing owners accepted at discovery. Core retains generic parse/registration; Renderer control retains ordering/completion. Do not move lighting names or policy into either generic owner.
2. Validate all batch entries before applying any; reject invalid/unregistered entries without partial mutation. Prove one accepted batch boundary before feature admission, safe requested/applied observation, and serial/threaded equivalence.
3. Exercise an already registered live Renderer CVar through the approved route, plus invalid input, failed batch, shutdown, and pending-query cases. Use temporary local probes where needed; do not register dormant lighting CVars to test infrastructure.
4. Remove any superseded direct mutation/query path in the scoped production route. Update Discovery with executable publication evidence and the explicit DVP-4A authorization decision; no later stage infers it from a successful build.

**Non-goals:** No lighting execution change, seven-CVar placeholder registration, Show UI, generic callback registry, feature manager, Application translator, global settings mirror, or repository-wide CVar refactor.

**Exit gate:** CHK-DVP-08/09 publication checks prove ordered serial/threaded edits/query and no partial batch. Discovery marks D03 accepted and DVP-4A AUTHORIZED only when all other blocking decisions are accepted. Existing unsupported/parse errors remain truthful.

**Stop conditions:** A second editable authority, data race, ownership/deadlock/shutdown ambiguity, exceeded prerequisite budget, or failed batch that mutates any entry. The shared architecture-fitness and clean-break gate is mandatory; source presence does not satisfy a required executable oracle.

**Risk/failure traceability:** `RISK-DVP-SF-01` in [Discovery](Discovery.md#risk-register); `FM-DVP-07`, `FM-DVP-08`, `FM-DVP-09` in [Acceptance](Acceptance.md#failure-modes). The exit's AC/CHK rows and those owners define proof, not a second criterion set here.

**Ready-to-use implementation prompt:**

```text
AUTONOMOUS EXECUTION: Selecting this stage includes its necessary prerequisite closure, even if predecessors and owning repairs were not separately queued. First audit and recursively establish every missing, failed, stale or contradicted prerequisite at its owner; implement required in-scope repairs, run the required checks, refresh invalidated evidence, then automatically resume and deliver this stage. Do not hand off merely because a stage is BLOCKED. Apply the Universal Execution Contract; only an exhausted external dependency or material user-only choice permits an actionable handoff. Agent-run checks are required; manual Editor interaction is not a prerequisite. Preserve truthful evidence and stop after this stage unless later stages are user-queued.

Implement or verify only DVP-4A-1 of Docs/Architecture/Modules/Engine/Renderer/Features/DebugViews/Plan.md after verifying DVP-4A-0 and Discovery's exact prerequisite authorization. Apply the Universal Execution Contract. Use the accepted existing Core console and Renderer control route to establish sequenced edits, safe query/acknowledgment, and validated bulk application before frame admission. Exercise an existing live Renderer CVar; add no dormant lighting registrations. Delete superseded scoped direct-write paths.

NON-NEGOTIABLE: One CVar authority, no partial failed batch, safe serial/threaded query and shutdown, and no lighting vocabulary in generic control owners. Do not invent a callback registry, Application translator, or settings mirror. Quote proof for each requirement and obtain Discovery's explicit lighting authorization before declaring DVP-4A-2 permitted.

Do not bypass predecessor gates or silently expand the accepted hook/API/copy/variant budget. Apply non-goals and stop rules to dependent work; repair missing, stale or contradicted prerequisites at their owners, prove the repair and resume instead of choosing an unaccepted alternative.

Validate: Run CHK-DVP-08/09's bounded publication probes and the smallest affected compile target; scope invalid-input/batch/shutdown controls to this route. Run architecture_boundary_check when a Renderer/RHI boundary actually changes, formatting and git diff --check. Retain the applicable CHK-DVP-08 hook/public-surface/definition-to-use audit and quote each required result; never claim an unrun check passed.

Handoff: report exact candidate/prerequisites, files by responsibility, deletions, copy/API/hook deltas, commands/configurations/results/artifacts, cleanup, performance classification, open risks and unavailable checks. State whether DVP-4A-2 is permitted; continue automatically when it is user-queued and its prerequisites pass; otherwise retain isolated-stage scope.
```

### DVP-4A-2 - Deliver Direct Subsurface As The First Vertical Slice

**Objective:** Console disabling Direct Subsurface removes its exclusive evaluation and publication while diffuse/specular remain valid.

**Prerequisites to establish automatically:** Discovery explicitly authorizes lighting implementation at the exact accepted candidate; stage 1 publication proof and stage 0 direct-lobe ABI/product/history/oracle decisions remain valid.

**Work:**

1. Register only r.Lighting.Direct.Subsurface at the direct feature owner and implement its feature-local IsEnabled/IsActive. Keep defaults enabled and unavailable-versus-disabled behavior distinct.
2. Apply the frozen uniform early uniform branch before subsurface response math and exclusive writes. Reconcile shared direct shader parameters, output initialization/bindings, composition, guides and diagnostic behavior in this same slice.
3. Wire the accepted activation into any affected graph identity and history invalidation without putting a per-leaf branch or Show value in FramePipeline/host/request/View.
4. Exercise enabled, disabled, disabled-diagnostic, missing-enabled-prerequisite and re-enable cases; remove any interim finished-result masking or stale-write assumptions immediately.

**Non-goals:** Other leaf registrations, Editor UI, parent controls, indirect transport changes, all-direct-off pruning not yet supported by all three controls, or new feature-control abstractions without a current consumer.

**Exit gate:** The independently non-zero subsurface fixture satisfies applicable AC-DVP-17/18/20/21/24/25/28 via CHK-DVP-08/10/12. Direct diffuse/specular stay within the frozen oracle; evidence locates skipped math/writes rather than only a dark pixel.

**Stop conditions:** Subsurface still computes behind a composite mask, retained targets lack current-frame semantics, a guide is fabricated, or the first helper requires new frame/host feature state. The shared architecture-fitness and clean-break gate is mandatory; source presence does not satisfy a required executable oracle.

**Risk/failure traceability:** `RISK-DVP-SF-03`, `RISK-DVP-SF-07`, `RISK-DVP-SF-08` in [Discovery](Discovery.md#risk-register); `FM-DVP-10`, `FM-DVP-13`, `FM-DVP-14` in [Acceptance](Acceptance.md#failure-modes). The exit's AC/CHK rows and those owners define proof, not a second criterion set here.

**Ready-to-use implementation prompt:**

```text
AUTONOMOUS EXECUTION: Selecting this stage includes its necessary prerequisite closure, even if predecessors and owning repairs were not separately queued. First audit and recursively establish every missing, failed, stale or contradicted prerequisite at its owner; implement required in-scope repairs, run the required checks, refresh invalidated evidence, then automatically resume and deliver this stage. Do not hand off merely because a stage is BLOCKED. Apply the Universal Execution Contract; only an exhausted external dependency or material user-only choice permits an actionable handoff. Agent-run checks are required; manual Editor interaction is not a prerequisite. Preserve truthful evidence and stop after this stage unless later stages are user-queued.

Implement only DVP-4A-2 of Docs/Architecture/Modules/Engine/Renderer/Features/DebugViews/Plan.md after explicit Discovery AUTHORIZED and stage 1 proof. Apply the Universal Execution Contract and freeze ledger. Deliver r.Lighting.Direct.Subsurface, feature-local IsEnabled/IsActive, and the accepted early shader route through actual direct evaluation, product binding/composition, affected guides/histories, and diagnostic unavailability. Register no later leaf.

NON-NEGOTIABLE: Skip exclusive subsurface evaluation/publication before the finished result; preserve active diffuse/specular semantics and valid downstream reads. No Renderer Show state, frame/host admission branch, dummy enabled product, stale output, or masked-only intermediate path. Quote candidate-bound pixel and work-removal proof ; repair missing proof or falsified prerequisites before resuming.

Do not bypass predecessor gates or silently expand the accepted hook/API/copy/variant budget. Apply non-goals and stop rules to dependent work; repair missing, stale or contradicted prerequisites at their owners, prove the repair and resume instead of choosing an unaccepted alternative.

Validate: Use CHK-DVP-08/10/12's single-lobe fixture and negative mask-only control, focused direct/composite shader cook, smallest affected C++ compile, selected runtime row, scoped formatting and git diff --check. Preserve evidence limits for unrun rows. Retain the applicable CHK-DVP-08 hook/public-surface/definition-to-use audit and quote each required result; never claim an unrun check passed.

Handoff: report exact candidate/prerequisites, files by responsibility, deletions, copy/API/hook deltas, commands/configurations/results/artifacts, cleanup, performance classification, open risks and unavailable checks. State whether DVP-4A-3 is permitted; continue automatically when it is user-queued and its prerequisites pass; otherwise retain isolated-stage scope.
```

### DVP-4A-3 - Complete Direct Lobes And All-Off Admission

**Objective:** All three direct lobes are independently controllable; all-off removes the exclusive direct family chain.

**Prerequisites to establish automatically:** Stage 2 passes its vertical-slice exit. Discovery's complete direct-family consumer/estimator/guide/topology ledger is accepted and unchanged.

**Work:**

1. Register Direct Diffuse and Direct Specular controls at the same cohesive direct owner and reuse the established helper/ABI pattern without a second mask or settings holder.
2. Derive direct-family admission from real active lobe consumers inside its existing Add...Passes entry point. When none is active, omit exclusive evaluation, reservoirs and shadow work; keep only dependencies with a named remaining consumer.
3. Reconcile direct resources, shared target clear, composition inputs, diagnostic product availability, and reconstruction requirements. Implement the approved cached-graph identity/retirement and direct temporal reset consequences atomically.
4. Prove each lobe, mixed combinations, all-direct-off and re-enable. Delete any now-dead unconditional direct-resource/pass path or superseded binding.

**Non-goals:** Direct Shadows toggle, indirect features, UI, a generic activation manager, new per-lobe orchestrator branches, or rewriting the estimator beyond the frozen active-lobe contract.

**Exit gate:** AC-DVP-17/18/20/21/24/25/28 pass for the direct family. CHK-DVP-08/10/12 prove all-off pass omission, correct active products/guides, generation consistency and fresh re-enable; shared retained work has an actual consumer.

**Stop conditions:** A disabled direct family still allocates/dispatches exclusive work, shadow/reservoir removal changes active indirect semantics, graph rebuilding reads a different policy frame, or bindings become conditional null fallbacks. The shared architecture-fitness and clean-break gate is mandatory; source presence does not satisfy a required executable oracle.

**Risk/failure traceability:** `RISK-DVP-SF-03`, `RISK-DVP-SF-07`, `RISK-DVP-SF-08`, `RISK-DVP-SF-09` in [Discovery](Discovery.md#risk-register); `FM-DVP-10`, `FM-DVP-13`, `FM-DVP-14` in [Acceptance](Acceptance.md#failure-modes). The exit's AC/CHK rows and those owners define proof, not a second criterion set here.

**Ready-to-use implementation prompt:**

```text
AUTONOMOUS EXECUTION: Selecting this stage includes its necessary prerequisite closure, even if predecessors and owning repairs were not separately queued. First audit and recursively establish every missing, failed, stale or contradicted prerequisite at its owner; implement required in-scope repairs, run the required checks, refresh invalidated evidence, then automatically resume and deliver this stage. Do not hand off merely because a stage is BLOCKED. Apply the Universal Execution Contract; only an exhausted external dependency or material user-only choice permits an actionable handoff. Agent-run checks are required; manual Editor interaction is not a prerequisite. Preserve truthful evidence and stop after this stage unless later stages are user-queued.

Implement only DVP-4A-3 of Docs/Architecture/Modules/Engine/Renderer/Features/DebugViews/Plan.md after stage 2 passes. Apply the Universal Execution Contract. Extend the existing direct control owner with Diffuse and Specular, then implement derived family admission and all-direct-off omission using the frozen consumer/estimator/product/graph/history route. Reconcile resources, clear, shader bindings, composition, diagnostics and guides together; delete superseded unconditional paths.

NON-NEGOTIABLE: Admission stays inside the direct feature entry point, parent intent is not stored, and no exclusive family work runs with all direct lobes inactive. Active indirect and mandatory-guide contracts stay valid; every remaining read has an admitted producer. No composite hiding, speculative manager, or frame/host feature branches. Quote per-lobe and group-off pixel/work/lifetime evidence ; repair missing proof or falsified prerequisites before resuming.

Do not bypass predecessor gates or silently expand the accepted hook/API/copy/variant budget. Apply non-goals and stop rules to dependent work; repair missing, stale or contradicted prerequisites at their owners, prove the repair and resume instead of choosing an unaccepted alternative.

Validate: Execute direct rows of CHK-DVP-08/10/12, focused changed shader/C++ checks and native resource validation. Exercise all-off/re-enable and in-flight topology retirement on the selected row; unrun backend/provider cells remain unproved. Retain the applicable CHK-DVP-08 hook/public-surface/definition-to-use audit and quote each required result; never claim an unrun check passed.

Handoff: report exact candidate/prerequisites, files by responsibility, deletions, copy/API/hook deltas, commands/configurations/results/artifacts, cleanup, performance classification, open risks and unavailable checks. State whether DVP-4A-4 is permitted; continue automatically when it is user-queued and its prerequisites pass; otherwise retain isolated-stage scope.
```

### DVP-4A-4 - Remove Exclusive Direct Shadow Work

**Objective:** Direct Shadows off gives fully visible primary direct lighting without unnecessary direct visibility production.

**Prerequisites to establish automatically:** Stage 3 passes. The primary visibility/reservoir dependence, shadow-signal resource contract, active consumers, history invalidation and unshadowed numeric oracle are frozen.

**Work:**

1. Register r.Lighting.Shadows.Direct at the direct-shadow owner with feature-local intent/activation; retained enabled intent is inactive when no relevant direct consumer exists.
2. Omit the direct visibility tracing producer when inactive. Keep its actual target initialized for the fixed Direct ABI; the primary lighting uniform branch uses visibility 1 without reading that signal; adjust only the approved visibility-dependent sampling/reservoir consequences.
3. Reconcile bindings, direct-shadow diagnostic unavailability, graph lifecycle and all affected temporal state. Preserve indirect visibility and Reference behavior.
4. Exercise occluder, shadow-off, all-direct-off, retained-shadow-intent/re-enable and missing-enabled-producer challenges.

**Non-goals:** Indirect Shadows, AO/GBuffer occlusion changes, continuation tracing, Reference policy edits, or retaining the shadow signal for a disabled raw diagnostic.

**Exit gate:** AC-DVP-18/21/24/25/27/28 through CHK-DVP-08/10/12: fully visible primary oracle, omitted exclusive shadow dispatch/traces, valid estimator weights and bindings, and affected-history reset. Any retained shared visibility cost is declared and justified.

**Stop conditions:** A shadowed target/PDF/weight remains inconsistent, signal tracing continues solely for display/debug continuity, the raw disabled diagnostic pretends to be produced, or unrelated transport changes. The shared architecture-fitness and clean-break gate is mandatory; source presence does not satisfy a required executable oracle.

**Risk/failure traceability:** `RISK-DVP-SF-03`, `RISK-DVP-SF-06`, `RISK-DVP-SF-07`, `RISK-DVP-SF-08`, `RISK-DVP-SF-09` in [Discovery](Discovery.md#risk-register); `FM-DVP-10`, `FM-DVP-12`, `FM-DVP-13`, `FM-DVP-14` in [Acceptance](Acceptance.md#failure-modes). The exit's AC/CHK rows and those owners define proof, not a second criterion set here.

**Ready-to-use implementation prompt:**

```text
AUTONOMOUS EXECUTION: Selecting this stage includes its necessary prerequisite closure, even if predecessors and owning repairs were not separately queued. First audit and recursively establish every missing, failed, stale or contradicted prerequisite at its owner; implement required in-scope repairs, run the required checks, refresh invalidated evidence, then automatically resume and deliver this stage. Do not hand off merely because a stage is BLOCKED. Apply the Universal Execution Contract; only an exhausted external dependency or material user-only choice permits an actionable handoff. Agent-run checks are required; manual Editor interaction is not a prerequisite. Preserve truthful evidence and stop after this stage unless later stages are user-queued.

Implement only DVP-4A-4 of Docs/Architecture/Modules/Engine/Renderer/Features/DebugViews/Plan.md after stage 3 and the frozen primary visibility/estimator ledger. Apply the Universal Execution Contract. Deliver the Direct Shadows CVar and feature-local activation, omit exclusive visibility production when inactive, and use the accepted fully visible primary evaluation without an absent-resource read. Reconcile sampling/reservoir implications, bindings, graph lifetime, diagnostics and histories.

NON-NEGOTIABLE: The control affects only primary direct visibility, disabled signal work is genuinely absent unless a frozen real shared consumer requires it, and enabled missing visibility remains a failure. Retain shadow intent across direct-family inactivity. Do not alter indirect visibility, AO, continuation intersections or Reference. Quote occluder and dispatch/trace/reset evidence ; repair missing proof or falsified prerequisites before resuming.

Do not bypass predecessor gates or silently expand the accepted hook/API/copy/variant budget. Apply non-goals and stop rules to dependent work; repair missing, stale or contradicted prerequisites at their owners, prove the repair and resume instead of choosing an unaccepted alternative.

Validate: Use primary-shadow rows of CHK-DVP-08/10/12, focused shadow/direct/reservoir shader cook, affected compile checks and native validation, including re-enable and missing-required-signal negative controls. Retain the applicable CHK-DVP-08 hook/public-surface/definition-to-use audit and quote each required result; never claim an unrun check passed.

Handoff: report exact candidate/prerequisites, files by responsibility, deletions, copy/API/hook deltas, commands/configurations/results/artifacts, cleanup, performance classification, open risks and unavailable checks. State whether DVP-4A-5 is permitted; continue automatically when it is user-queued and its prerequisites pass; otherwise retain isolated-stage scope.
```

### DVP-4A-5 - Deliver Indirect Lobes And Shared-Estimator Admission

**Objective:** Indirect diffuse/specular are independently controllable; all-indirect-off removes their exclusive trace/reservoir/resolve chain.

**Prerequisites to establish automatically:** Stages 0–4 pass. Indirect Lighting's owner accepts the exact primary-lobe classification, sampling/PDF/target/weight, emission/environment, guide, active-product and reset consequences; no implementation-time transport choice remains.

**Work:**

1. Register Indirect Diffuse and Indirect Specular beside the owning Lit indirect controls; introduce only the feature-local helpers required by current admission and shader parameters.
2. Implement the accepted disabled-lobe route before exclusive estimator/resolve work, preserving continuation events needed by remaining classified paths. Use the frozen runtime branch/variant and target/PDF semantics.
3. Own all-off admission inside the existing indirect family. Reconcile working/history reservoirs, temporal/spatial reuse, resolve, outputs, shared guides, composition, visualization and selected provider contracts in the same slice.
4. Wire feature topology identity and complete dependent reset scope. Validate all-on, each-off, both-off, provider-required guides, invalid prerequisites, rapid toggles and fresh re-enable; remove dormant/unconditional replaced paths.

**Non-goals:** Indirect Subsurface/transmission/volume, deleting every specular bounce when primary specular is off, provider substitution, indirect shadow toggle, estimator redesign or a second path-tracing implementation.

**Exit gate:** Applicable AC-DVP-17/18/20/21/24/25/28 pass using CHK-DVP-08/10/11/12. The frozen stochastic or numeric oracle validates remaining energy; group-off proves chain omission and honest guide/provider availability; incompatible history is not reused.

**Stop conditions:** Lobe filtering biases active transport outside the accepted contract, continuation events are mistaken for disabled output work, mandatory guides disappear without explicit rejection, or zero output hides a missing enabled producer. The shared architecture-fitness and clean-break gate is mandatory; source presence does not satisfy a required executable oracle.

**Risk/failure traceability:** `RISK-DVP-SF-03`, `RISK-DVP-SF-07`, `RISK-DVP-SF-08`, `RISK-DVP-SF-09` in [Discovery](Discovery.md#risk-register); `FM-DVP-10`, `FM-DVP-13`, `FM-DVP-14` in [Acceptance](Acceptance.md#failure-modes). The exit's AC/CHK rows and those owners define proof, not a second criterion set here.

**Ready-to-use implementation prompt:**

```text
AUTONOMOUS EXECUTION: Selecting this stage includes its necessary prerequisite closure, even if predecessors and owning repairs were not separately queued. First audit and recursively establish every missing, failed, stale or contradicted prerequisite at its owner; implement required in-scope repairs, run the required checks, refresh invalidated evidence, then automatically resume and deliver this stage. Do not hand off merely because a stage is BLOCKED. Apply the Universal Execution Contract; only an exhausted external dependency or material user-only choice permits an actionable handoff. Agent-run checks are required; manual Editor interaction is not a prerequisite. Preserve truthful evidence and stop after this stage unless later stages are user-queued.

Implement only DVP-4A-5 of Docs/Architecture/Modules/Engine/Renderer/Features/DebugViews/Plan.md after stage 4 and the accepted Indirect Lighting semantic/guide/reset ledger. Apply the Universal Execution Contract. Deliver both real indirect feature CVars and local activation through candidate generation, reservoir reuse, resolve, active products/composition and guide consumers. Remove exclusive inactive lobe work and omit the family with no active consumers through its own entry point; update graph lifetime and dependent histories coherently.

NON-NEGOTIABLE: Preserve the frozen path classification, sampling probabilities, PDFs, reservoir targets/weights, active-path emission/environment and mandatory guides. A disabled primary specular contribution does not disable all specular continuation events. No fabricated guides, provider fallback, missing/stale reads, mask-only implementation, Reference coupling, or premature Indirect Subsurface. Quote estimator, pixel, omission and reset proof ; repair missing proof or falsified prerequisites before resuming.

Do not bypass predecessor gates or silently expand the accepted hook/API/copy/variant budget. Apply non-goals and stop rules to dependent work; repair missing, stale or contradicted prerequisites at their owners, prove the repair and resume instead of choosing an unaccepted alternative.

Validate: Use indirect rows of CHK-DVP-08/10/11/12 with predeclared seeds/sample counts/tolerances; cook affected indirect/shared/composite variants and compile the smallest affected owner. Exercise selected reconstruction provider and all-off negative-guide cases; do not infer unrun cells. Retain the applicable CHK-DVP-08 hook/public-surface/definition-to-use audit and quote each required result; never claim an unrun check passed.

Handoff: report exact candidate/prerequisites, files by responsibility, deletions, copy/API/hook deltas, commands/configurations/results/artifacts, cleanup, performance classification, open risks and unavailable checks. State whether DVP-4A-6 is permitted; continue automatically when it is user-queued and its prerequisites pass; otherwise retain isolated-stage scope.
```

### DVP-4A-6 - Bypass Secondary-Hit Shadow Visibility

**Objective:** Indirect Shadows off removes secondary-hit direct-light visibility tests while keeping the active indirect path and Reference estimator valid.

**Prerequisites to establish automatically:** Stage 5 passes and P04 freezes the exact Lit caller/shared helper ABI, visibility-dependent reservoir semantics, history set and secondary occluder/continuation/Reference controls.

**Work:**

1. Register r.Lighting.Shadows.Indirect and its helpers in the Lit indirect owner; do not make shared ray/path helpers read this CVar.
2. Pass the accepted visibility policy from Lit to the narrow secondary direct-light evaluation. Use visibility 1 before the optional shadow trace; preserve geometry continuation, material hits, emitter/environment evaluation and active transport.
3. Reset every history whose meaning changes with this visibility evaluation; retain shadow intent when indirect lobes are inactive. Reference supplies its unchanged required visibility policy.
4. Prove secondary occluder results, shadow-ray omission, continuation-hit identity, Reference output, disable/re-enable, and rapid toggles.

**Non-goals:** Primary direct-shadow changes, shared global shadow switches, AO, altered bounce count/domain, Reference Show controls, or temporal warm-up by continuing disabled visibility work.

**Exit gate:** AC-DVP-18/24/25/27/28 through CHK-DVP-08/10/11/12: unshadowed secondary lighting, absent visibility rays, preserved continuation/Reference policy, and complete reset scope.

**Stop conditions:** The shared helper acquires the feature CVar, continuation rays are skipped, Reference changes, or stale visibility-dependent weights/history survive a toggle. The shared architecture-fitness and clean-break gate is mandatory; source presence does not satisfy a required executable oracle.

**Risk/failure traceability:** `RISK-DVP-SF-06`, `RISK-DVP-SF-07`, `RISK-DVP-SF-08`, `RISK-DVP-SF-09` in [Discovery](Discovery.md#risk-register); `FM-DVP-12`, `FM-DVP-14` in [Acceptance](Acceptance.md#failure-modes). The exit's AC/CHK rows and those owners define proof, not a second criterion set here.

**Ready-to-use implementation prompt:**

```text
AUTONOMOUS EXECUTION: Selecting this stage includes its necessary prerequisite closure, even if predecessors and owning repairs were not separately queued. First audit and recursively establish every missing, failed, stale or contradicted prerequisite at its owner; implement required in-scope repairs, run the required checks, refresh invalidated evidence, then automatically resume and deliver this stage. Do not hand off merely because a stage is BLOCKED. Apply the Universal Execution Contract; only an exhausted external dependency or material user-only choice permits an actionable handoff. Agent-run checks are required; manual Editor interaction is not a prerequisite. Preserve truthful evidence and stop after this stage unless later stages are user-queued.

Implement only DVP-4A-6 of Docs/Architecture/Modules/Engine/Renderer/Features/DebugViews/Plan.md after stage 5 and P04's accepted caller/ABI/reset ledger. Apply the Universal Execution Contract. Deliver Indirect Shadows intent/activation in the Lit feature, carry its narrow policy to secondary-hit direct-light evaluation, bypass disabled visibility tracing before it runs, and reset dependent histories. Keep Reference's required visibility argument unchanged.

NON-NEGOTIABLE: No CVar reads or Lit policy authority in shared tracing helpers; no continuation-intersection bypass, Reference behavior change, missing enabled products or stale reservoir/reconstruction state. Shadow intent remains enabled independently of whether indirect consumers are active. Quote secondary lighting, trace-category, continuation, Reference and reset oracles ; repair missing proof or falsified prerequisites before resuming.

Do not bypass predecessor gates or silently expand the accepted hook/API/copy/variant budget. Apply non-goals and stop rules to dependent work; repair missing, stale or contradicted prerequisites at their owners, prove the repair and resume instead of choosing an unaccepted alternative.

Validate: Run secondary-shadow rows of CHK-DVP-08/10/11/12, focused Lit/shared/Reference shader ABI/cook checks and the selected runtime/native-validation row. A bypass omitted or leaked into Reference must fail the negative controls. Retain the applicable CHK-DVP-08 hook/public-surface/definition-to-use audit and quote each required result; never claim an unrun check passed.

Handoff: report exact candidate/prerequisites, files by responsibility, deletions, copy/API/hook deltas, commands/configurations/results/artifacts, cleanup, performance classification, open risks and unavailable checks. State whether DVP-4A-7 is permitted; continue automatically when it is user-queued and its prerequisites pass; otherwise retain isolated-stage scope.
```

### DVP-4A-7 - Add The CVar-Driven Editor Show Menu

**Objective:** Users browse and edit all seven real controls through one hierarchical menu reflecting the console's shared feature intent.

**Prerequisites to establish automatically:** Stages 1–6 pass; seven real registrations and consumers exist. The control-query/batch acknowledgment and requested/applied UI contract are frozen.

**Work:**

1. Extend ViewportTopPanel presentation using the existing Core console/control surface. Place Direct Lighting, Indirect Lighting and Shadows groups beside Viewmode, with only their real children.
2. Derive checks and mixed parents from CVar intent; parent/reset mutations use one ordered batch. Reflect console edits without retaining an EditorViewportSession selection or changing viewport request generation.
3. Expose shared scope and mode limitations; make missing registrations visible as defects, not unchecked leaf defaults. Keep pending/applied state truthful and disabled raw diagnostics unavailable.
4. Exercise leaf/group/reset/console behavior through agent-run probes across multiple viewport requests and modes. Manual interaction, keyboard/focus and appearance checks are optional follow-up observations, not prerequisites. Update the runtime CVar catalog and user-facing navigation without duplicating feature semantics.

**Non-goals:** Viewport-local overrides, persistent Editor mirror, parent CVars, private Renderer includes, a feature registry, menu-to-Application translation or Indirect Subsurface advertising.

**Exit gate:** AC-DVP-17/19/22/23/25 via CHK-DVP-08/09: all seven leaves and parent/reset semantics, global UI/console parity, no partial applied frame, mode intent retention. Keyboard-accessible presentation remains a design obligation; manual interaction is not an exit gate.

**Stop conditions:** Menu check state becomes another mutable authority, bulk updates race frame admission, the UI claims requested work is already applied, or a Renderer-private symbol is imported. The shared architecture-fitness and clean-break gate is mandatory; source presence does not satisfy a required executable oracle.

**Risk/failure traceability:** `RISK-DVP-SF-01`, `RISK-DVP-SF-02`, `RISK-DVP-SF-05` in [Discovery](Discovery.md#risk-register); `FM-DVP-07`, `FM-DVP-08`, `FM-DVP-09`, `FM-DVP-10` in [Acceptance](Acceptance.md#failure-modes). The exit's AC/CHK rows and those owners define proof, not a second criterion set here.

**Ready-to-use implementation prompt:**

```text
AUTONOMOUS EXECUTION: Selecting this stage includes its necessary prerequisite closure, even if predecessors and owning repairs were not separately queued. First audit and recursively establish every missing, failed, stale or contradicted prerequisite at its owner; implement required in-scope repairs, run the required checks, refresh invalidated evidence, then automatically resume and deliver this stage. Do not hand off merely because a stage is BLOCKED. Apply the Universal Execution Contract; only an exhausted external dependency or material user-only choice permits an actionable handoff. Agent-run checks are required; manual Editor interaction is not a prerequisite. Preserve truthful evidence and stop after this stage unless later stages are user-queued.

Implement only DVP-4A-7 of Docs/Architecture/Modules/Engine/Renderer/Features/DebugViews/Plan.md after all seven execution consumers and control delivery are proved. Apply the Universal Execution Contract. Add the Show hierarchy in ViewportTopPanel, querying the existing CVar authority and submitting validated leaf/parent/reset edits through its sequenced batch route. Reflect console edits and global scope; preserve mode-driven CVar intent and truthful pending/unavailable states. Update directly affected user/CVar documentation.

NON-NEGOTIABLE: Editor owns labels and widgets only; no Renderer Show types, private CVar headers, session mirror, viewport-generation transport, stored parent state or premature leaf. One batch changes each parent/reset, all applicable viewports share the result, and checks cannot imply unsupported work is active. Quote menu/console/mode/batch/accessibility proof ; repair missing proof or falsified prerequisites before resuming.

Do not bypass predecessor gates or silently expand the accepted hook/API/copy/variant budget. Apply non-goals and stop rules to dependent work; repair missing, stale or contradicted prerequisites at their owners, prove the repair and resume instead of choosing an unaccepted alternative.

Validate: Execute CHK-DVP-08/09 with serial/threaded controls and multiple viewport requests, a missing-registration negative probe, automated menu-state/batch checks and source review of navigation/layout, smallest affected Editor compile, scoped formatting/links and git diff --check. No full engine build as a speculative UI check. Retain the applicable CHK-DVP-08 hook/public-surface/definition-to-use audit and quote each required result; never claim an unrun check passed.

Handoff: report exact candidate/prerequisites, files by responsibility, deletions, copy/API/hook deltas, commands/configurations/results/artifacts, cleanup, performance classification, open risks and unavailable checks. State whether DVP-4A-8 is permitted; continue automatically when it is user-queued and its prerequisites pass; otherwise retain isolated-stage scope.
```

### DVP-4A-8 - Close Backend, Provider And Execution Evidence

**Objective:** Every advertised candidate cell has independent correctness and work-removal evidence, with measured costs clearly separated from execution omission.

**Prerequisites to establish automatically:** Stages 2–7 have candidate-bound local evidence and no open production contract defect. Stage 0's backend/provider/mode/format/stochastic/GPU protocol is unchanged; prepare and verify required tooling automatically; exhaust safe local access/setup remedies before reporting genuinely unavailable hardware/access.

**Work:**

1. Cook every affected bounded shader combination and exercise the accepted D3D12/Vulkan cells with native validation. Include supported providers, all-off guide/product admission, serial/threaded edits and in-flight topology retirement.
2. Run CHK-DVP-10 numeric and failure oracles, then CHK-DVP-12 pass/dispatch/branch/write/trace inspections and repeated GPU timings outside rebuild/warm-up transients.
3. Report shared sampling/trace/guide costs, intentional disabled-output initialization, allocation/graph rebuild/reset latency and measurement variation. Do not equate a small or noisy timing delta with failed work omission, or omitted work with guaranteed speedup.
4. Repair only defects falsified in the stage that owns them; invalidate and repeat affected earlier evidence. No optimization, provider substitute or new thresholds are introduced to make the report pass.

**Non-goals:** New features/backends/providers, performance tuning beyond defect repair, adjusted tolerances after observation, whole-workspace validation, or a source-only pass for an unrun matrix cell.

**Exit gate:** CHK-DVP-11/12 and applicable AC-DVP-14/15/20/21/24/27/28 meet frozen oracles on each advertised cell. Unsupported/excluded cells are decided by the contract owner, not quietly omitted. Saved-cost claims have repeatable GPU evidence; unavailable mandatory checks block closure.

**Stop conditions:** Unavailable advertised hardware/probe, uncategorized native issue, mismatched candidate/oracle, incomplete variant bindings, hidden executing work, or a failed run disguised as an excluded row. The shared architecture-fitness and clean-break gate is mandatory; source presence does not satisfy a required executable oracle.

**Risk/failure traceability:** `RISK-DVP-SF-01`, `RISK-DVP-SF-03`, `RISK-DVP-SF-05`, `RISK-DVP-SF-06`, `RISK-DVP-SF-07`, `RISK-DVP-SF-08`, `RISK-DVP-SF-09` in [Discovery](Discovery.md#risk-register); `FM-DVP-07`, `FM-DVP-09`, `FM-DVP-10`, `FM-DVP-12`, `FM-DVP-13`, `FM-DVP-14` in [Acceptance](Acceptance.md#failure-modes). The exit's AC/CHK rows and those owners define proof, not a second criterion set here.

**Ready-to-use validation prompt:**

```text
AUTONOMOUS EXECUTION: Selecting this stage includes its necessary prerequisite closure, even if predecessors and owning repairs were not separately queued. First audit and recursively establish every missing, failed, stale or contradicted prerequisite at its owner; implement required in-scope repairs, run the required checks, refresh invalidated evidence, then automatically resume and deliver this stage. Do not hand off merely because a stage is BLOCKED. Apply the Universal Execution Contract; only an exhausted external dependency or material user-only choice permits an actionable handoff. Agent-run checks are required; manual Editor interaction is not a prerequisite. Preserve truthful evidence and stop after this stage unless later stages are user-queued.

Execute only DVP-4A-8 of Docs/Architecture/Modules/Engine/Renderer/Features/DebugViews/Plan.md after stages 2–7 and the frozen evidence protocol. Apply the Universal Execution Contract. Run CHK-DVP-10/11/12 on the advertised D3D12/Vulkan, provider and mode cells with native validation. Retain exact candidate, commands, hardware/driver, decoded outputs, graph/history observations, pass/branch/write/trace evidence, repeated timings and cleanup. Route falsified defects back to their owning stage and repeat affected evidence.

NON-NEGOTIABLE: No threshold/sample/matrix change after candidate observation, masked pixels as performance proof, inferred unrun backend/provider support, fabricated guides or provider fallback. Distinguish exclusive omission, remaining shared cost, initialization and graph/reset transients from measured steady-state savings. Quote independent cell evidence ; repair missing proof or falsified prerequisites before resuming.

Do not bypass predecessor gates or silently expand the accepted hook/API/copy/variant budget. Apply non-goals and stop rules to dependent work; repair missing, stale or contradicted prerequisites at their owners, prove the repair and resume instead of choosing an unaccepted alternative.

Validate: Run only the frozen focused cooks/workloads/captures needed by CHK-DVP-10/11/12 and architecture_boundary_check if affected boundaries changed. Use existing per-user evidence locations; do not submit temporary harnesses/probes. Retain the applicable CHK-DVP-08 hook/public-surface/definition-to-use audit and quote each required result; never claim an unrun check passed.

Handoff: report exact candidate/prerequisites, files by responsibility, deletions, copy/API/hook deltas, commands/configurations/results/artifacts, cleanup, performance classification, open risks and unavailable checks. State whether DVP-4A-9 is permitted; continue automatically when it is user-queued and its prerequisites pass; otherwise retain isolated-stage scope.
```

### DVP-4A-9 - Adopt The Controls And Close The Slice

**Objective:** Hand off one clean, feature-enclosed production route with no obsolete masking/Show state and a truthful feature-completion report.

**Prerequisites to establish automatically:** Stage 8 supplies complete evidence for every included advertised cell. All seven controls pass the applicable Acceptance criteria; no failed prerequisite is waived by this closure stage.

**Work:**

1. Re-audit definitions to all uses, public surface delta, feature-named outside hooks, conditional output consumers, cook/build membership and runtime CVar catalog. Prove bounded feature removal touches only its accepted hooks; no generic orchestrator knows the Editor hierarchy.
2. Delete scoped dead code/includes, interim masks/holders, replaced paths, temporary validation code and orphan generation entries. Regenerate directly affected disposable products; do not defer compatibility or cleanup.
3. Reconcile dossier/Discovery/Show contract and central readiness only from actual evidence. Record candidate results in the owning FCR-REN-11 report, not a status diary inside this plan.
4. List remaining explicitly blocked/excluded behavior, including Indirect Subsurface, and the next safe workflow. Supersede completed execution instructions only when they no longer have a live consumer.

**Non-goals:** Implementation of DVP-4B, unrelated repository cleanup, new diagnostic/progress machinery, package/release claims not exercised, or waiver of an unrun required criterion.

**Exit gate:** All included AC-DVP-17–25 and AC-DVP-27/28 pass conjunctively; CHK-DVP-08 enclosure/clean-break and CHK-DVP-09–12 evidence are current. The FCR owner records the result and limitations; when DVP-4B is selected, its owning transport prerequisite is mandatory delivery work before control integration.

**Stop conditions:** An orphan producer/consumer, hidden duplicate authority, unexplained outside hook, public feature mechanism, temporary submitted test, stale evidence identity, or required unrun result remains. The shared architecture-fitness and clean-break gate is mandatory; source presence does not satisfy a required executable oracle.

**Risk/failure traceability:** `RISK-DVP-SF-01`, `RISK-DVP-SF-02`, `RISK-DVP-SF-03`, `RISK-DVP-SF-04`, `RISK-DVP-SF-05`, `RISK-DVP-SF-06`, `RISK-DVP-SF-07`, `RISK-DVP-SF-08`, `RISK-DVP-SF-09` in [Discovery](Discovery.md#risk-register); `FM-DVP-07`, `FM-DVP-08`, `FM-DVP-09`, `FM-DVP-10`, `FM-DVP-11`, `FM-DVP-12`, `FM-DVP-13`, `FM-DVP-14` in [Acceptance](Acceptance.md#failure-modes). The exit's AC/CHK rows and those owners define proof, not a second criterion set here.

**Ready-to-use closure prompt:**

```text
AUTONOMOUS EXECUTION: Selecting this stage includes its necessary prerequisite closure, even if predecessors and owning repairs were not separately queued. First audit and recursively establish every missing, failed, stale or contradicted prerequisite at its owner; implement required in-scope repairs, run the required checks, refresh invalidated evidence, then automatically resume and deliver this stage. Do not hand off merely because a stage is BLOCKED. Apply the Universal Execution Contract; only an exhausted external dependency or material user-only choice permits an actionable handoff. Agent-run checks are required; manual Editor interaction is not a prerequisite. Preserve truthful evidence and stop after this stage unless later stages are user-queued.

Close only DVP-4A-9 of Docs/Architecture/Modules/Engine/Renderer/Features/DebugViews/Plan.md after stage 8's complete evidence. Apply the Universal Execution Contract. Audit enclosure, outside hooks, public APIs, copies, ownership, build/cook membership and all active-product consumers. Delete obsolete masks/state/paths/includes and temporary probes in one clean break, regenerate scoped products, reconcile affected documentation, and record candidate results in the FCR-REN-11 owner.

NON-NEGOTIABLE: No old/new dispatcher, compatibility alias, Show transport, deferred cleanup, dormant feature scaffolding or evidence-level upgrade from source alone. Every included criterion needs current proof; any missing mandatory proof blocks completion. Indirect Subsurface is not pulled into this slice. Quote closure evidence and report the exact remaining limitations/next permitted work.

Do not bypass predecessor gates or silently expand the accepted hook/API/copy/variant budget. Apply non-goals and stop rules to dependent work; repair missing, stale or contradicted prerequisites at their owners, prove the repair and resume instead of choosing an unaccepted alternative.

Validate: Run CHK-DVP-08 enclosure and no-stale-reference audit, directly affected documentation/ID checks, formatting, architecture_boundary_check when applicable, and git diff --check. Reuse valid evidence; do not run broad builds/cooks to replace an absent oracle. Retain the applicable CHK-DVP-08 hook/public-surface/definition-to-use audit and quote each required result; never claim an unrun check passed.

Handoff: report exact candidate/prerequisites, files by responsibility, deletions, copy/API/hook deltas, commands/configurations/results/artifacts, cleanup, performance classification, open risks and unavailable checks. State whether DVP-4A closure; DVP-4B establishes its own transport prerequisites when selected is permitted; continue automatically when it is user-queued and its prerequisites pass; otherwise retain isolated-stage scope.
```

### DVP-4B - Indirect Subsurface

**Prerequisite to establish automatically:** `IND-D0-02` and the Indirect Lighting owner authorize subsurface lobe classification, estimator/PDF/energy semantics, reconstruction/history consequences, and a real non-zero product.

**Work:** land producer, activation, disabled-work removal, reconstruction/diagnostic contract, active composite binding, feature CVar, and Editor leaf together. The existing Indirect parent expands from two to three real children; no flag enum or placeholder is required.

**Non-goals:** reclassifying diffuse energy as subsurface, fabricated output, UI-only enablement, or opportunistic transmission/volume transport.

**Exit gate:** applicable `AC-DVP-17` through `AC-DVP-28`, including a non-zero independent oracle and exclusive-work omission. If the transport decision is open, first execute DVP-4B-0 at the Indirect Lighting owner to establish the real transport/product contract and prerequisite work, then automatically proceed to DVP-4B-1 when its required proof passes.

#### DVP-4B-0 - Authorize The Owning Transport Slice

**Objective and prerequisites to establish automatically:** after establishing DVP-4A, work in the Indirect Lighting owner's discovery/plan to close `IND-D0-02` and establish its accepted staged transport/product plan. The Show-menu request is not authority to design that transport here.

**Work:** re-audit the absent product; freeze subsurface classification, energy/PDF/target semantics, real producer, guide/history effects, non-zero and disabled-work oracles, and exact integration handoff into this control system. Keep every premature CVar/UI/resource surface absent.

**Non-goals and stop rule:** no production change, diffuse relabeling, transmission/volume expansion, zero placeholder, or implied DVP-4B authorization from DVP-4A completion. Any unresolved owning transport decision triggers owning discovery and prerequisite closure before dependent integration; do not end with a blocked-status-only report.

**Exit gate:** Discovery and the Indirect Lighting owner jointly authorize a concrete transport/product slice with its exact prerequisites, semantic rules and checks. This plan does not duplicate that owner's delivery stages.

**Ready-to-use discovery prompt:**

```text
AUTONOMOUS EXECUTION: Selecting this stage includes its necessary prerequisite closure, even if predecessors and owning repairs were not separately queued. First audit and recursively establish every missing, failed, stale or contradicted prerequisite at its owner; implement required in-scope repairs, run the required checks, refresh invalidated evidence, then automatically resume and deliver this stage. Do not hand off merely because a stage is BLOCKED. Apply the Universal Execution Contract; only an exhausted external dependency or material user-only choice permits an actionable handoff. Agent-run checks are required; manual Editor interaction is not a prerequisite. Preserve truthful evidence and stop after this stage unless later stages are user-queued.

Execute only DVP-4B-0 of Docs/Architecture/Modules/Engine/Renderer/Features/DebugViews/Plan.md after DVP-4A closure. Apply its Universal Execution Contract. Keep transport discovery itself read-only; execute required production repairs through its separately recorded owning stages and return with proof. Work through the owning Indirect Lighting discovery/plan to close IND-D0-02 and freeze the real Indirect Subsurface producer, path classification, energy/PDF/target, product/guide/history, activation and non-zero/disabled-work proof. Record the exact transport-stage and control-integration prerequisites without duplicating that plan here.

NON-NEGOTIABLE: no CVar, Editor leaf, resource, fabricated zero or relabeled diffuse contribution is admitted before the owning transport/product decision. A Show request and prior seven-leaf completion do not authorize transport. Quote the accepted owner decisions and proof contracts ; repair missing proof or falsified prerequisites before resuming.

Validate documentation links/anchors, IDs, ownership/hook budget, UTF-8 and git diff --check. Report executable checks as unrun unless a bounded named discovery probe ran. Handoff the exact owning transport stage and whether DVP-4B-1 is permitted; continue only after its owning transport gate passes and it is user-queued.
```

#### DVP-4B-1 - Integrate The Authorized Real Contribution

**Objective and prerequisites to establish automatically:** integrate the eighth leaf only as part of the exact Indirect Lighting-authorized real product slice. Its producer and control contract must land coherently; an already accepted real producer may be extended, never replaced by a control-only placeholder.

**Work:** follow the owning transport stage; reuse the existing indirect activation/control/publication route, reconcile estimator/resolve/composite/guides/history/diagnostics, register the feature CVar beside its consumer, and expand the derived Editor parent by one real child. Run its non-zero and exclusive-work omission oracles, advertised-backend/native checks, and closure audit.

**Non-goals and stop rule:** no new Show types/settings/parent gates, semantics improvised in this integration step, compatibility path, producer-only/control-only intermediate state, or generic feature framework. Invalidated transport or product evidence requires automatically repairing and revalidating its owning stage, then returning to this integration.

**Exit gate:** `AC-DVP-26` and all applicable `AC-DVP-17` through `AC-DVP-28` pass with current `CHK-DVP-08` through `CHK-DVP-12` evidence. Update the owning candidate report and active feature/CVar documentation; no source-only completion.

**Ready-to-use implementation prompt:**

```text
AUTONOMOUS EXECUTION: Selecting this stage includes its necessary prerequisite closure, even if predecessors and owning repairs were not separately queued. First audit and recursively establish every missing, failed, stale or contradicted prerequisite at its owner; implement required in-scope repairs, run the required checks, refresh invalidated evidence, then automatically resume and deliver this stage. Do not hand off merely because a stage is BLOCKED. Apply the Universal Execution Contract; only an exhausted external dependency or material user-only choice permits an actionable handoff. Agent-run checks are required; manual Editor interaction is not a prerequisite. Preserve truthful evidence and stop after this stage unless later stages are user-queued.

Implement only DVP-4B-1 of Docs/Architecture/Modules/Engine/Renderer/Features/DebugViews/Plan.md within the exact Indirect Lighting-authorized transport/product stage recorded by DVP-4B-0. Apply the Universal Execution Contract. Reuse existing indirect feature controls and sequenced publication. Integrate the real Indirect Subsurface producer and activation/disabled-work path through estimator/resolve, composite, guides, history and diagnostics; add its feature CVar and one real Editor child in the same coherent slice.

NON-NEGOTIABLE: the owning transport rules and non-zero oracle are prerequisites, not implementation choices. No diffuse relabeling, fabricated product, UI-only registration, stale reads, masked-only output, duplicate settings/Show state, compatibility path or unledgered hook. Quote AC-DVP-26 and applicable execution/product/history/enclosure evidence ; repair missing proof or falsified prerequisites before resuming.

Validate the owning transport checks plus applicable CHK-DVP-08 through CHK-DVP-12, bounded shader/C++ checks, advertised-backend/native rows, affected documentation and git diff --check. Handoff exact prerequisite/candidate identity, files/deletions/copies/hooks, commands/artifacts, unrun checks and the owning report's closure disposition. Do not broaden transport or claim unrun performance proof.
```

## DVP-5 - Prove The Contract

Apply the autonomous execution and automated verification policies above. Reconcile every retained AC/FM/CHK row with exact candidate evidence, repair failed automated checks at their owners, repeat invalidated checks and continue through closure without routine approval. Manual Editor-opening checks are not prerequisites. Move the plan to done only when every retained required criterion passes; never infer completion from source presence or unavailable evidence.

Exercise enum/HLSL parity, every consumer, two-viewport mode isolation, shared feature-CVar UI/console parity, Lit/Reference/Lit topology, exact/HDR numeric presentation, extent changes, output encoding, and advertised D3D12/Vulkan rows. DVP-4A-8/9 own the lighting-control execution/evidence handoff; this family-wide closure does not repeat it or infer a missing row. Record only checks actually run in the owning completion report.

## Ready-To-Use Source Cleanup Prompt

This is a mode-baseline reconciliation prompt, not a DVP-4 implementation instruction. Use it only when the live mode audit identifies a directly scoped defect; do not replay historical migration or add feature controls through it.

```text
AUTONOMOUS EXECUTION: Selecting this stage includes its necessary prerequisite closure, even if predecessors and owning repairs were not separately queued. First audit and recursively establish every missing, failed, stale or contradicted prerequisite at its owner; implement required in-scope repairs, run the required checks, refresh invalidated evidence, then automatically resume and deliver this stage. Do not hand off merely because a stage is BLOCKED. Apply the Universal Execution Contract; only an exhausted external dependency or material user-only choice permits an actionable handoff. Agent-run checks are required; manual Editor interaction is not a prerequisite. Preserve truthful evidence and stop after this stage unless later stages are user-queued.

Reconcile the live Debug Views and Reference Path Tracer source to Docs/Architecture/Modules/Engine/Renderer/Features/DebugViews/Controls/ViewModes.md. Keep RenderViewMode as the sole host-independent per-view rendering choice on ViewportRenderRequest and immutable RenderView. Keep Editor labels/icons/menu layout local while using the same enum directly. Consume the value only at the owning frame-composition, raster, debug-resolve, and feature-lifecycle decisions. Delete parallel Editor enums, preset translators, visualization targets, mode-shaped show flags, selection CVars, command bridges, graph/feature settings copies, compatibility aliases, and RHI fields. Preserve ReferencePathTracer = 1 and contiguous values. Keep the Reference implementation private and the shared frame shell unchanged. Add no diagnostics, registry, generic settings bag, recipe hierarchy, or speculative controls. Run focused source checks, architecture_boundary_check, documentation link/anchor checks, and git diff --check; report builds and runtime checks as deferred unless actually run.
```

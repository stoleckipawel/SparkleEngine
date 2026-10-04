# Debug Views Discovery And Implementation Authorization

**Status:** discovery gate; bounded DVP-4A-1 prerequisite repair admitted below; lighting implementation remains blocked

**Current readiness:** **Not applicable** to this gate. Debug Views progress remains owned by the [Renderer readiness row](../../../../../../Acceptance/CurrentReadiness.md#renderer).

**Responsibility:** own unresolved decisions, risks, probes, and binary authorization for the next Debug Views implementation slice.

**Authority boundary:** [Controls](Controls/README.md) and [Presentation Architecture](PresentationArchitecture.md) define behavior; [Plan](Plan.md) orders authorized work; [Acceptance](Acceptance.md) defines proof; this page decides whether DVP-4 may start.

**Verified:** 2026-10-04 source audit against `bc26606c03766c540ca4959a3a18602ffc0443d7`; see the [candidate and probe record](#dvp-4a-0-candidate-and-probe-record). No executable publication, numeric, or GPU verdict.

**Non-claims:** source inspection does not prove compilation, thread safety, runtime behavior, pixels, GPU cost removal, backend parity, or release acceptance.

## Decision To Make

Authorize DVP-4A only when Editor and console can safely edit one feature-CVar authority, and each lighting/shadow owner can admit execution, remove exclusive disabled work, preserve valid shared work, and retire/reset its products and histories without a hidden fallback.

The revised [Show-menu design](Controls/ShowFlags.md) supersedes the former per-view bit set and composite-only masking target. It is an accepted design direction, not authorization or executable evidence. DVP-4B separately requires the Indirect Lighting transport/product decision.

### Authorization Boundaries For The Staged Plan

The [DVP-4A staged sequence](Plan.md#dvp-4a---existing-lighting-and-shadow-controls) begins with read-only `DVP-4A-0`. Its decision may authorize only `DVP-4A-1`, a bounded prerequisite repair of existing CVar delivery, while DVP-4A lighting implementation stays **BLOCKED**. Record the exact candidate, approved mutation/query/batch route, touched owners, public/API/copy/hook budget, negative controls, and executable exit evidence required from that repair. This limited decision does not authorize feature registration, activation, shaders, or Show UI.

If the existing route already satisfies the contract, `DVP-4A-1` proves it and makes no unnecessary production change. Only after its publication evidence and the other blocking decisions pass may this gate record DVP-4A **AUTHORIZED** for `DVP-4A-2`. Every subsequent stage separately requires its predecessor's exact exit evidence; global authorization is not permission to bypass a stage, choose an unresolved architecture, or run every prompt at once.

The initial DVP-4A-0 audit below did not authorize a prerequisite repair. The subsequent [DVP-4A-1 route reconciliation](#dvp-4a-1-bounded-prerequisite-route) admits only its listed delivery repair; publication proof remains required. A failed or exceeded repair scope returns here for review; it cannot expand into a generic control framework or an unbounded CVar rewrite.

## Iteration Control Record

| Field | Record |
| --- | --- |
| Iteration identity | `ITER-DVP-SHOWFLAGS-03`; Debug Views owner; `DVP-4A-0` source discovery; **BLOCKED** exit; candidate `bc26606c03766c540ca4959a3a18602ffc0443d7`; concurrent changes preserved |
| Intended outcome | freeze execution and evidence decisions without production edits; stop rather than delegate unresolved decisions to implementation |
| North Star | `NS-OWNERSHIP` / `NS-SIMPLIFY`: advance target enclosure and single authority; `NS-REAL` / `NS-EVIDENCE`: preserve, no implementation or evidence-level advance |
| Persona targets | `PGE-07`, `PGE-09`, `PGE-13`: preserve direction; no runtime/performance claim |
| Delivery target | [DVP-4](Plan.md#dvp-4---add-lighting-show-flags), `RD-3`, `FCR-REN-11`; no release verdict |
| Acceptance / failures / checks | `AC-DVP-17`–`28`, `FM-DVP-07`–`14`, `CHK-DVP-08`–`12` in [Acceptance](Acceptance.md) |
| Current decision | DVP-4A **BLOCKED** by `DVP-SF-D03`, `DVP-SF-D07`, `DVP-SF-D08`, `DVP-SF-D09`; DVP-4B **BLOCKED** by `DVP-SF-D05` |
| Next permitted step | only the bounded DVP-4A-1 delivery repair below; no lighting stage is permitted |

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

## DVP-4A-0 Candidate And Probe Record

### Candidate And Baseline Reconciliation

Inspection began at `c3863cfac9c9416599906328dacaa99124b56ad1`, with user-owned edits in `FeatureExecutionTraces.md` and `SettingsStateAndPersistence.md`. HEAD advanced to `bc26606c03766c540ca4959a3a18602ffc0443d7` during inspection. The intervening diff changes those documents, the Editor overview, exposure UI and PTLAS UI; it does not change the console, control queue, graph or lighting paths audited here. The working tree was clean immediately before this documentation edit. Those concurrent changes were not restored or modified.

`DVP-0` through `DVP-3` are reconciled as **source-present baseline, executable evidence unproved**, not rerun:

- `RenderViewMode.h` has Lit `0`, Reference `1`, Wireframe `2`, debug values `3`–`17`, Count `18`.
- `Passes/Scene/SceneRenderingPasses.cpp::AddSceneRenderingPasses` selects exactly one Reference or real-time middle, followed by exposure, visualization, denoising and upscaling. A GBuffer diagnostic label does **not** currently prove that lighting execution was omitted.
- `Passes/Presentation/PresentationPolicy.cpp` explicitly separates scene-referred HDR and display-linear exact modes. Reference selects linear upscaling in this candidate; do not reinstate the older intermediate provider-selection wording.
- Searches of `Engine`, `Tools` and `Projects` found no `RenderShowFlag`, `r.ShowFlags` or proposed `r.Lighting.Direct/Indirect/Shadows` registrations. Absence is a source result, not runtime proof or permission to add them now.

### P01 - Control Publication: Source Trace Complete, Required Oracle Blocked

Paths below are relative to the repository root; function names identify the inspected boundary.

| Boundary | Exact source evidence | Consequence |
| --- | --- | --- |
| storage and parse | `Engine/Core/Public/Console/CVar.h::Get`, `Set`, `GetValueAsString`, `TrySetValueFromString`: plain `m_value`; parse then immediate assignment | no synchronization or prepared all-or-nothing batch interface is present |
| command mutation/query | `Engine/Core/Private/Console/ConsoleBuiltinCommands.cpp::ExecuteSetCVar`, `ExecuteGetCVar`, `ExecuteListCVars`: direct registry lookup, setter or formatting | queueing only Set is insufficient; Get and List also read values on the caller |
| frontends | `Engine/Editor/Private/Console/EditorConsoleSystem.cpp` and `Engine/Application/Private/RuntimeConsole/RuntimeConsoleOverlay.cpp` register those builtins; `ConsoleSession.cpp::SubmitLine` executes the registry directly | both frontends need one host-composed route, not Editor-private Renderer includes or lighting translation in Application |
| existing ordered owner | `Engine/Renderer/Private/Concurrency/Coordinator/RenderCoordinatorControls.cpp::DispatchControl`, `SubmitThreadCommand`; `RenderCoordinatorThread.cpp::ExecuteThreadCommand` | serial executes on its context; threaded controls and frame tickets share monotonically checked command ordering |
| execution and replies | `RendererExecutionContext.cpp::ExecuteControl`; `Concurrency/Control/RendererExecutionControl.h`, `RenderControlCompletion.h` | no CVar command/result exists; extending this owner is a **proposal**, not implemented publication |
| shutdown | `RenderCoordinatorThread.cpp::SettleAbandonedWork` explicitly cancels reload/diagnostics completions | a new query/batch completion must settle on rejection, abandonment and shutdown; borrowing the queue alone does not prove this |
| adjacent live reader | `Engine/Renderer/Private/Renderer.cpp::CaptureRenderingSettings` directly calls `EngineRenderingSettingsRuntime::Capture`; that capture reads CVars, while `RenderSettingsChangedCommand` applies on the execution owner | console-only repair would leave a direct live read outside the proposed owner |
| startup setters | `Engine/Application/Private/Application.cpp::ConfigureProcessFromCommandLine` applies persisted settings and command-line CVars | distinguish initialization from live delivery; do not silently route an initialization call into an unavailable Renderer |

**Selected discovery direction, not an admitted API:** reuse the existing Renderer execution control queue for live renderer-CVar mutation, ordered queries and parent batches; Core retains parsing/storage authority, and host composition supplies the same operation to both consoles and eventual Show UI. Parse and validate every entry before any commit. Return applied values only after the owner accepts the operation; reject the whole invalid batch. Requests/replies may own moved strings across the thread boundary, but must not create a persistent policy mirror. Never wait on a completion from its own execution thread.

**Unclosed repair decisions:** classification of renderer-owned versus other process CVars and their live writers/readers; the concrete non-mutating preparation/commit API for the existing templated value types; startup/live lifetime binding; and the complete cancellation/reentrancy contract. These determine the file/API/copy budget. Neither an atomic per-variable setter nor sequential UI calls would by itself prove one frame-consistent parent batch.

**Authorization budget now:** zero production files, zero new public APIs/types, zero CVar registrations and zero control hooks. A later discovery revision must enumerate the exact Core parser/builtin, Renderer facade/control/completion, frontend-composition and adjacent-reader files before granting DVP-4A-1. This audit does not authorize an open-ended “fix CVars” prompt.

### P02, P04 And P05 - Seven-Leaf Dependency Ledger

This is a **source dependency ledger**, not an accepted execution ledger. No feature-local IsEnabled/IsActive implementation for these seven controls exists. Every row below needs the named decision closed before a production prompt may select a branch, cooked variant, binding or reset scope.

Renderer paths in this table are relative to `Engine/Renderer/Private`; shader paths are relative to `Engine/Assets/Shaders`.

| Leaf | Existing owner and helper inputs to retain | Exclusive/shared work and products | Blocking execution decision |
| --- | --- | --- | --- |
| Direct Diffuse | `Passes/Lighting/Direct`; accepted intent, applicable mode, GBuffer, lights, reservoir and visibility | `BRDF/BRDF.hlsli::Direct::Evaluate` diffuse math; DirectDiffuse UAV; shared shading geometry, Fresnel, reservoir and signal | `Lighting/DirectLightReservoir.hlsli::EvaluateTargetPdf` evaluates the sum of all direct lobes. Branching final resolve alone leaves disabled math in temporal/spatial target evaluation; active-target/reuse and output ABI are not frozen |
| Direct Specular | same direct owner and inputs | specular BRDF math; DirectSpecular UAV; Fresnel is also required by diffuse | must preserve the Fresnel term required by active diffuse while omitting exclusive specular math; same target/reuse and ABI gate |
| Direct Subsurface | same direct owner, additionally authored subsurface data | subsurface evaluation; DirectSubsurface UAV; shares reservoir selection | current BRDF adds a subsurface approximation alongside diffuse. [Direct semantics](../Lighting/DirectLighting/SamplingAndShading.md#surface-lobe-contract) leaves energy allocation at DIR-D0-03; this stage cannot silently choose a new allocation |
| Indirect Diffuse | `Passes/Lighting/Restir/Indirect`; accepted intent, mode, GBuffer, traversal, environment, reservoir, required consumers | path evaluation classified by `PrimaryLobe`; IndirectDiffuse UAV; shared proposal sampling, reuse, continuation and resolve | `Lighting/RestirIndirectReservoir.hlsli::EvaluateCandidate` replays `RayTracing/PathLighting.hlsli`; primary-lobe rejection, proposal PDFs, candidate counts and target/reuse consistency require owning estimator admission, not final-output masking |
| Indirect Specular | same indirect owner and inputs | IndirectSpecular UAV; specular-hit-distance guide depends on the replayed path | same estimator gate; disabling specular must not remove specular events at later vertices of an active diffuse-classified path; mandatory guide semantics remain unclosed |
| Direct Shadows | `Passes/Lighting/Shadows` producer with direct-family composition; retained shadow intent, active direct consumers, selected reservoir, traversal | `DirectShadowSignalCommon.hlsli` visibility trace and signal UAV; final direct shader reads visibility; reservoir working/history textures are packaged in `DirectShadowSignalResources` | target bypass is primary visibility = 1, not deletion of reservoirs. Resource ownership/binding must separate exclusive signal work from still-required reservoir data before selecting the no-signal ABI |
| Indirect Shadows | indirect caller policy at `RayTracing/PathLighting.hlsli` / `RayTracingHitLighting.hlsli`; retained intent, active indirect consumers and traversal | secondary-hit direct-light visibility traces; shares secondary material evaluation and indirect estimator | bypass only `TraceDirectLightSample`, retain continuation `TraceSurfaceRay` and secondary BSDF lobes. Shared-helper caller isolation, changed candidate targets and Reference negative proof remain required |

No row adds a new transport product. Indirect Subsurface remains excluded from this seven-leaf slice until IND-D0-02; that is the existing D05 gate, not a newly removed advertised cell.

**Group-off and resource disposition.** `Passes/Lighting/Restir/RestirLightingPasses.cpp` currently creates resources and invokes both families unconditionally. `Direct/RestirDirectLightingPasses.cpp` schedules reservoir temporal/spatial, shadow signal and direct resolve. The indirect family also initializes reservoir resources before its work. Family entry points must eventually own aggregate admission before exclusive allocation; no leaf switches belong in FramePipeline. However, `Passes/Lighting/LightingComposite.cpp` unconditionally binds all five outputs, and `LightingTargetClear.cpp` initializes lighting and guides. No omission-compatible output ABI has been accepted. This audit does **not** admit retained zero textures, null bindings, a powerset of variants or current-frame clears as an enabled-product substitute.

**Cached graph and retirement.** `Frame/FramePipelineGraph.cpp::RefreshGraphForTopology` compares provider key, frame settings, GBuffer algorithm, ray-tracing graph generation and shader generation; it has no proposed feature identity. `RetireFrameExecution` moves old graph/frames to `Frame/Retirement/FrameExecutionRetirementQueue.cpp`, which captures last submissions for all queues and polls token completion. This proves a source lifetime owner, not that a feature edit rebuilds safely. The missing family-owned topology identity and its narrow composition hook block producer-removal authorization; a uniform shader flag cannot remove a cached dispatch.

**Mandatory guides.** `Restir/Indirect/RestirIndirectResolve.cpp` binds all four guide UAVs. Its HLSL writes surface albedos/roughness and path-derived specular hit distance; `Lighting/RayReconstructionGuides.hlsli` defines those values. `Restir/Reconstruction/RestirRayReconstructionResources.cpp` binds them, together with depth, motion, exposure and normals, to the reconstruction consumer. All-indirect-off therefore cannot simply omit resolve and call cleared guides valid. A genuine independent guide producer and the selected provider's disabled-specular/all-off contract are unproved. Keep these provider cells **BLOCKED**, not silently switched to another provider or removed from scope.

**History and reset scope.** `RestirLightingInvalidation.cpp` currently hashes scene, normal bias, shadow distance and bounce count, not these controls. `Resources/History/FrameHistory.cpp::UpdateFrameHistory` checks this hash only when the direct reservoir Sample handle is valid; invalidation resets both reservoirs and provider history. That guard would miss an indirect-only topology if direct resources were omitted. Each reservoir has Sample/Weight/Surface current/previous state. `FramePipelineGraph.cpp::InvalidateViewHistory` also invalidates exposure/history, View state and providers. `RenderViewState.cpp::Invalidate` clears its lighting hash and resets its ray-tracing planner. Consequently neither “add seven hash bits” nor “reset everything” is an accepted scoped solution. The exact topology-change versus value-change reset set, including provider state, must be frozen at these owners; Reference accumulation must remain independent of Lit policy.

### P03 - Fixture And Evidence Freeze Stopped

No candidate-bound artifact establishing independently non-zero energy in each of the five products was produced in this audit. Showcase material demonstrations are not independent numeric oracles; source shader assignments are not such artifacts either. The non-zero stop condition is therefore triggered. No seed, sample budget, statistical tolerance or GPU result is marked accepted merely to fill a ledger.

Known format facts: `Frame/Graph/RenderFrameGraphFormats.h` selects RGBA16F scene color; `LightingRenderTargets.cpp` uses that format for the five contributions, RGBA16F albedo guides and R32F roughness/hit distance. Comparing displayed screenshots or requiring exact stochastic per-pixel subtraction would not prove active-lobe correctness. Decoded-format error and estimator variance need separate predeclared oracles.

Before resuming the freeze, the evidence owner must supply a retained manifest with actual fixture/material/light/camera identities, independently positive expected energy, all-on/each-off/direct-group-off/indirect-group-off/shadow-group-off/all-off and primary/secondary occluder cases. It must bind RNG frame/sample sequences, temporal-reuse settings, sample counts, decoded numeric/statistical thresholds, render/output extents, serial/threaded D3D12/Vulkan and each advertised GBuffer/traversal/provider cell. Unsupported or inaccessible cells stay visible as BLOCKED; a smaller convenient matrix is not authorization.

CHK-DVP-10/11 negative controls must detect an enabled missing producer, missing reset, stale diagnostic, continuation/Reference policy leak and unavailable mandatory guide. CHK-DVP-12 must detect the compute-then-mask implementation using pass/dispatch and trace-category evidence, independently of timing. Its capture manifest must freeze the GPU timestamp tool/range, warm-up and measurement counts, ordering, variance rule and graph-rebuild exclusion before results. These are **outstanding freeze prerequisites**, not decisions delegated to DVP-4A-2 through 8. No runtime, shader cook, GPU capture or broad build was run at this stop.

### CHK-DVP-08 Coverage And Gate Disposition

| Required check surface | Result at this candidate |
| --- | --- |
| definition-to-use / source ownership | traced Core parser/builtins/frontends, serial/threaded execution/control/completion, cached graph/retirement, seven leaf seams, reservoir targets, shared path helpers, guide consumer and history guard; findings above are source-only |
| public-surface / holder / copy / hook delta | zero production delta; only Discovery and Plan documentation edited; no state holder, public type/API, shader variant, CVar or feature-named integration hook added |
| clean break / feature enclosure | no old production path replaced, so no production deletion; no second authority, fallback, feature manager or compatibility path introduced. Later implementation still requires an accepted per-file hook budget and removal of the replaced route |
| executable enabled-missing / disabled / inapplicable negative probe | **NOT RUN / BLOCKED**: proposed controls and admitted output/guide routes do not exist. Source inspection does not satisfy this part of CHK-DVP-08 |
| Renderer/RHI architecture boundary execution | not applicable to this documentation-only delta; no dependency boundary changed. No claim that the executable target ran |

**D03:** existing ordered owner identified; publication/preparation/query/lifetime route and bounded repair budget **not accepted**. **D07:** non-zero fixture/oracle and numeric/GPU protocol **not accepted**. **D08:** current reset owners traced; policy/reset-scope and shadow negative proof **not accepted**. **D09:** cached lifetime owner traced; estimator, active-output and mandatory-guide routes **not accepted**. D04 remains an accepted target, not execution authorization. RISK-DVP-SF-01 through 09 remain open under their mapped CHK rows; no source finding retires a runtime risk.

**Verdict: DVP-4A-0 exit BLOCKED. DVP-4A-1 is NOT PERMITTED; DVP-4A-2 onward and DVP-4B remain BLOCKED.** Only continue discovery at the console/control, direct/indirect estimator, product/guide and evidence owners above. Do not resume a production prompt with these choices unresolved. A revised gate must bind the newly accepted decisions, exact candidate and prerequisite artifacts before changing permission.

## DVP-4A-1 Bounded Prerequisite Route

**Candidate:** `bc26606c03766c540ca4959a3a18602ffc0443d7` plus the scoped working changes; `ITER-DVP-SHOWFLAGS-04`. This reconciliation supersedes the initial audit's no-repair permission, not its unresolved lighting findings. The user requested autonomous prerequisite resolution and direct Get/Set access without cached CVar values.

**Admitted route:** keep one CVar value in Core. Scalar Get/Set use sequentially consistent atomic load/store; existing non-trivial value support uses a value-local mutex. There is no shadow value, requested/applied mirror, persisted cache, per-frame settings body or change-sink registry. Atomic access alone is not the parent-batch boundary: console and eventual UI control operations use the existing Renderer command sequence, applied entirely between execution of frame tickets. Renderer-owned direct writers continue to call Set on that owner; startup setters remain startup work.

Core owns a non-mutating validation operation using the same pure parser as its existing setter. A Set batch validates all names, duplicates and values before assigning any. Assignment reparses the immutable text through the existing setter rather than retaining type-erased parsed values. An impossible validation/commit mismatch is a fatal invariant defect, not a returned partial-success or rollback path. Query/List results are transient owned replies, consumed and discarded; features do not read them instead of the CVar. Batch size is bounded at 64 entries. Core registration must finish before concurrent live use, as in the existing static registrations.

One generic host-bound executor is injected into the existing builtin registration, not a callback registry. Both console frontends invoke the same Renderer facade operation. Application supplies that operation only at host composition; it names no feature or lighting policy. Renderer owns typed commands/completion; settings capture uses that same execution owner instead of direct caller-thread reads. Host objects destroy console bindings before Renderer destruction. The producer-thread assertion precedes every synchronous query, preventing execution-thread self-wait. A closed queue rejects admission; rejected or abandoned completion-bearing commands receive an explicit cancellation error. No waiter survives queue settlement.

**Production budget: 26 exact files, two of them new.** No unrelated call-site or registration rewrite is admitted:

| Owner / hook role | Permitted files relative to module root | Justification / required check |
| --- | --- | --- |
| Core storage/parser and console production (5) | `Public/Console/CVar.h`, `Public/Console/ConsoleBuiltinCommands.h`, `Private/Console/ConsoleBuiltinCommands.cpp`, new `Public/Console/CVarControl.h` and `Private/Console/CVarControl.cpp` | direct safe Get/Set, validate-before-commit and one control operation; scalar/non-trivial access, invalid/unregistered/duplicate/oversized-batch probes |
| Renderer delivery and lifetime (10) | `Public/Renderer.h`, `Private/Renderer.cpp`, `Private/Concurrency/Control/{RenderControlCompletion.h,RendererExecutionControl.h,RenderThreadCommandQueue.h,RenderThreadCommandQueue.cpp}`, `Private/Concurrency/Coordinator/{RenderCoordinator.h,RenderCoordinatorControls.cpp,RenderCoordinatorThread.cpp,RendererExecutionContext.cpp}` | facade/control/result, ordered query and settings capture, rejection/abandonment; serial/threaded sequence, batch/frame and shutdown probes |
| Editor consumer composition (5) | `Public/UI.h`, `Public/Console/EditorConsoleSystem.h`, `Private/UI.cpp`, `Private/UIInitialization.cpp`, `Private/Console/EditorConsoleSystem.cpp` | supply executor through existing host services, remove direct builtin registration; no private Renderer includes, no Show UI or selection mirror |
| Application host composition (6) | `Private/EditorApplication.cpp`, `Private/RuntimeApplication.cpp`, `Private/RuntimeConsole/{RuntimeConsoleHost.h,RuntimeConsoleHost.cpp,RuntimeConsoleOverlay.h,RuntimeConsoleOverlay.cpp}` | Application binds the executor and owns frame orchestration; console clients receive only the capability and return a stable UI packet, never Renderer; scoped consumer compilation and shutdown-order review |

**API/copy budget:** Core adds one control-operation enum, edit/value/request/result records, one executor alias, one owner-execution function and one non-mutating validation virtual. Renderer adds one public generic operation, two private commands and corresponding existing-completion result alternatives; queue admission reports rejection. Existing settings capture is sequenced, not replaced with a mirror. Public Editor host services add one generic executor; console constructors require that executor. Requests/replies own strings solely to cross the thread boundary and are moved through commands/completion; builtin closures retain only the host operation, never CVar values. No shader, RHI API, feature registration, mode field, graph state or lighting hook is admitted.

**Clean break:** remove direct Get/Set/List value access from builtin command execution and remove the direct settings-capture read path. Do not retain a default executor that falls back to direct registry mutation. Autocomplete may inspect immutable registration metadata, not values.

**Client boundary refinement:** AC-DVP-30 removes both the new Renderer constructor argument and the pre-existing RuntimeConsoleHost::TickFrame route. RuntimeConsoleHost receives the existing CVarControlExecutor and returns an optional UiRenderPacket after overlay update. RuntimeApplication alone binds Renderer, updates simulation, consumes/submits a visible console packet and renders, in the same order as before. No new service or public capability was added; no Renderer facade include, pointer/reference, capture or frame call remains in either console client. The stable Renderer UI packet remains a product contract, not access to its implementation. RuntimeApplication::Shutdown destroys the console host before Renderer; Editor host composition retains the corresponding UI-before-Renderer lifetime.

**Engineering authority refinement (same iteration):** the user elevated client/core separation to a general binding rule. [Module Ownership](../../../../../../Engineering/Foundations/ModuleOwnership.md#system-core-and-client-separation) now owns that rule; Engineering navigation, Change Integration and Code Review route/enforce it. Plan and AC-DVP-30 specialize it without duplicating authority. This is documentation-only beyond the 26-file production budget, adds no runtime API or package/ABI promise, and must not be read as a repository-wide compliance result.

The refinement advances NS-OWNERSHIP/NS-SIMPLIFY delivery discipline, preserves existing runtime evidence and has **no runtime exposure**. Its claim-falsifying check is owner/routing/criterion inspection plus strict UTF-8/local-link/anchor validation: **PASS**, 146 references in nine changed documents, and `git diff --check` **PASS**. No production rebuild was needed for this subsequent documentation-only change; the consuming build evidence below belongs to the earlier code refinement. General review enforcement is a binding contract, not a claim that every existing subsystem has been audited or is already independently packaged.

**Predeclared publication oracle:** exercise existing `r.Exposure.Compensation`, plus a second existing scalar Renderer CVar, using ordered operations in serial and threaded configurations. An invalid second entry, missing name, duplicate name or over-limit batch changes neither first nor second value. A following query returns the applied pair; a frame ticket between batches observes only a complete accepted pair. Use explicit queue barriers, not sleeps, for pending query, backpressure, close/rejection and abandonment cases. Retain exact probe command/build/candidate and deliberately broken validate/reset/order controls. Storage access stress alone, a queue model or a successful compilation is not full Renderer frame-publication proof.

### Bounded Delivery Evidence

**Candidate/configuration:** the exact revision above plus this 26-file production slice and directly affected documentation; Windows x64, MSVC 19.51, DevelopmentEditor libraries. Temporary Release probes link those actual libraries and production queue/completion sources. No probe source or target belongs to the submitted change.

| Command / artifact | Result and scope |
| --- | --- |
| `cmake --build build --config DevelopmentEditor --target SparkleApplicationEditor --parallel 4` | PASS, exit 0, including the refined RuntimeApplication/RuntimeConsoleHost consuming boundary. No shader cook or full product runtime claim. |
| `cmake -S build/validation/dvp-cvar-delivery-20261004 -B build/validation/dvp-cvar-delivery-20261004/out -G "Visual Studio 18 2026" -A x64`; `cmake --build build/validation/dvp-cvar-delivery-20261004/out --config Release --parallel 4` | PASS after correcting the local library-root path. The facade probe required the existing product's `sl.interposer.dll` alongside the executable; the initial missing-dependency launch failed and is not credited. |
| `build/validation/dvp-cvar-delivery-20261004/out/Release/DvpCVarProbe.exe` | PASS, exit 0. Real registered exposure CVars, actual Core parsing/builtin commands, invalid/unregistered/duplicate/oversized batches, applied query; actual bounded Renderer queue/completion with 2,000 ordered frame **markers**, closed admission, pending cancellation and 100,000 direct atomic Get/Set iterations. Markers read the expected newly edited pair, not just pair consistency. |
| Same executable with `--break-validation` | Expected FAIL, exit 1: failed batch partially mutated values. The local deliberately broken route applies the valid first entry before rejecting the second; the oracle detects it. |
| Same executable with `--break-cache` | Expected FAIL, exit 1: frame marker observed partial batch or out-of-order command. The local deliberately cached reads miss the expected newly edited pair; this falsifies the no-refresh/direct-read oracle. No production fault switch exists. |
| `build/validation/dvp-cvar-delivery-20261004/out/Release/DvpCVarFacadeProbe.exe` with `SPARKLE_SUPPRESS_CRASH_DIALOGS=1` | PASS, exit 0. Actual Renderer facade/coordinator/execution context, serial and threaded configurations, 32 batches each, invalid second-entry rejection, applied queries, ordered settings capture and destruction after queries. D3D12 owner initialization is exercised; no rendered-frame or GPU-work claim. Retained stdout: `build/validation/dvp-cvar-delivery-20261004/facade-result.log`. |

**Architecture-fitness audit:** all 26 production files remain within the admitted ledger. Core owns parse/storage, existing Renderer control owns sequence/completion, Application owns binding and frame orchestration, and clients own console presentation only. No lighting name, dormant registration, new callback registry, settings mirror, shadow storage, feature-value forwarding chain, frame-policy argument or variant was added. Request/reply strings are required transient thread-boundary ownership, not feature policy. Removed paths: builtin direct mutation/query, caller-thread settings capture and console-host frame orchestration. No Renderer/RHI responsibility or dependency boundary changed. Repeated direct Get cost is accepted for readability; no measured performance or saved-GPU-time claim is made.

**Scoped static checks/cleanup:** clang-format `--dry-run --Werror` passed for all 26 production files; `git diff --check` passed. A strict UTF-8/local-link/heading-anchor check passed for 86 references in the five changed documents. Constructor/include/call searches found no Renderer facade, TickFrame, OnRender or SubmitUiRenderPacket use in either console client. Temporary probe source files and their local CMakeLists were deleted after final rebuild/rerun; ignored binaries and the facade log remain as local evidence and are not submitted. No user assets or production files were deleted. Renderer/RHI boundary validation is not applicable because that boundary did not change. UI interaction, GPU frames and multi-viewport acceptance were not run.

**Disposition:** ordered real-facade edit/query/settings capture and component batch/marker/lifetime oracles are proved within DVP-4A-1. Actual rendered frame admission, multi-viewport menu/console publication and the complete CHK-DVP-08/09 remain **UNPROVED**; component markers do not upgrade those claims. D03's complete publication gate is therefore not yet accepted. D07/D08/D09 remain **BLOCKED**. DVP-4A-2 is not permitted; only bounded remaining prerequisite evidence/discovery may proceed. No lighting, GPU, visual, backend-parity or release acceptance credit is awarded.

## Exit Rule

DVP-4A becomes **AUTHORIZED** only when `DVP-SF-D03`, `DVP-SF-D07`, `DVP-SF-D08`, `DVP-SF-D09` have accepted `P01`–`P05` evidence, the execution ledger has no unresolved transport/product/history/lifetime route, every risk has a live check mapping, and [Plan](Plan.md#dvp-4---add-lighting-show-flags) requires those routes without delegating architecture decisions to implementation.

DVP-4B stays **BLOCKED** until `DVP-SF-D05` passes in the Indirect Lighting package. Design edits, zero-only fixtures, hidden pixels, or an unchecked UI row cannot satisfy either gate.

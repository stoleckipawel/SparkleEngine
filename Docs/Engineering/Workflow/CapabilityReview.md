# Capability Documentation Review

Status: operational review runbook

Scope: evaluate whether a SparkleEngine capability is discoverable, accurately bounded, traceable through its real owners, and ready to receive executable evidence

Authority boundary: module and cross-module Architecture documents own current source claims; Acceptance owns completion and release disposition; Plans own unanswered checks; this runbook owns only the review workflow

## Start From The Reader's Question

| Question | Owning route |
| --- | --- |
| What does this module currently contain? | [Module Architecture And Capability Inventory](../../Architecture/Modules/README.md) |
| How does a graphics feature differ across modes or backends? | [Graphics Feature Coverage Matrix](../../Architecture/CrossModule/GraphicsCoverageMatrix.md) |
| How does a graphics result travel from request to GPU retirement? | [Graphics Feature Execution Traces](../../Architecture/CrossModule/FeatureExecutionTraces.md) |
| Can a developer or user complete an end-to-end product journey? | Start with the [Whole Repository Architecture Map](../../Architecture/WholeRepositoryMap.md) and affected [module inventories](../../Architecture/Modules/README.md); record any missing dedicated cross-module route in the [Capability Evidence Plan](../../Plans/CapabilityEvidence.md). |
| Where does a non-graphics journey cross module boundaries? | Trace the affected [module inventories](../../Architecture/Modules/README.md) owner by owner; record an unowned trace as an explicit [Capability Evidence](../../Plans/CapabilityEvidence.md) gap rather than linking to a document that does not exist. |
| What evidence is still missing? | [Capability Evidence Plan](../../Plans/CapabilityEvidence.md) |
| Is the feature complete or allowed to ship? | [Feature Completion Reports](../../Acceptance/FeatureCompletionReports.md) and [First Release Acceptance](../../Acceptance/FirstRelease.md) |
| How should a change be implemented and reviewed? | [Change Lifecycle](ChangeLifecycle.md), [Change Integration](ChangeIntegration.md), and [Code Review](CodeReview.md) |

Do not answer a release question from an Architecture snapshot and do not answer an implementation question from a plan. Follow the owning route.

## Capability Dossier Contract

For each advertised capability or independently selectable mode, the documentation set must make every applicable dimension below discoverable. One document need not contain everything; links may cross to the owning module, decision, plan, or acceptance report.

| Dimension | Question the dossier must answer | If the answer is not known yet |
| --- | --- | --- |
| Identity and promise | What exact result is offered, to whom, and what is deliberately not promised? | Mark `Unknown` or `Pending`; add an inventory/evidence item before advertising it. |
| Reachability | Which executable, API, CLI argument, setting, CVar, editor control, catalog entry, or package surface selects it? | Add it to the selector audit; an unenumerated reachable control blocks scope freeze. |
| Owner and build membership | Which module/target owns policy and which targets compile/link the path? | Trace CMake and source membership; do not infer from directory names. |
| Inputs and outputs | What semantic data enters, what representation crosses each boundary, and what observable product exits? | Record the missing producer/consumer in the evidence plan. |
| State and identity | Who owns mutable state, stable IDs, generations, caches, history, and invalidation? | Mark the lifetime/identity boundary unresolved; do not claim reload or temporal correctness. |
| Concurrency and lifetime | Which thread/task lane/GPU queue owns each stage, when is data immutable, and what completion retires it? | Require a serial/control case plus stress evidence before concurrency claims. |
| Capacity and resource bounds | What fixed limits, queue depths, descriptor counts, memory leases, timeouts, and overflow policies apply? | Treat an unknown or silent bound as a release risk, not as unlimited support. |
| Algorithm and semantics | What algorithm, coordinate/color/unit convention, material model, numeric assumption, or ABI contract defines correctness? | Add a canonical oracle/reference requirement; source vocabulary is insufficient. |
| Platform/backend matrix | Which OS, compiler, API, GPU capability, vendor SDK, shader target, and feature combination is implemented? | Keep each unproven cell independently Pending or Excluded. |
| Requested versus active result | What was requested, what actually became active, and how is substitution or fallback reported? | Silent fallback blocks the capability claim. |
| Failure and recovery | Where is invalid input rejected, what safe state remains, can work be cancelled/retried, and what cleanup occurs? | Add a controlled `FM-*` and mapped check before approval. |
| Observability and support | Which logs, diagnostics, captures, counters, errors, and user-facing recovery hints prove the active path? | Add a diagnostic/evidence gap; absence of a signal prevents trustworthy support. |
| Quality and cost | What correctness, visual, temporal, latency, frame-time, memory, startup, cook, and package thresholds apply? | Predeclare the oracle and threshold before measurement. |
| Security and delivery | What paths, privileges, network inputs, provenance, package contents, writable state, and redistribution constraints apply? | Route to package/security acceptance; workspace success does not answer it. |
| Evidence and disposition | Which `INV-*`/module evidence, `FCR-*`, `AC-*`, `FM-*`, and `CHK-*` rows own proof and approval? | Keep the release disposition Pending; never embed an unreviewed pass claim in Architecture. |
| Change and retirement | What invalidates prior evidence, and when should the capability, fallback, or scaffolding be removed or reconsidered? | Add an invalidation/deletion trigger rather than retaining it indefinitely. |

Use four documentation answers consistently: **Answered**, **Partial**, **Unknown**, and **Not applicable**. `Unknown` is acceptable during inventory hardening only when it links to a named evidence or architecture action. Blank space, “should work,” and an uncited future intention are not states.

## Developer Workflow

1. Start at the nearest module inventory and select the exact capability row or create one if a reachable path has no row.
2. Enumerate every producer of selection/configuration and every runtime/tool consumer. Include CMake membership and generated products.
3. Walk the relevant vertical trace. At every boundary record representation, ownership, mutability, identity, lifetime, failure, and observability.
4. Compare all independently meaningful horizontal cells: backend, mode, shader target, material/geometry variant, product profile, map, and supported/unsupported hardware.
5. Check the dossier dimensions above. Mark unresolved dimensions explicitly and link the smallest claim-falsifying item in the evidence plan.
6. Create the iteration mapping required by the Change Lifecycle before implementation. Do not turn the inventory page into a work diary.
7. Update source, build membership, producers/consumers, documentation, checks, and replaced paths as one owned change.
8. Store executed results only in the candidate-bound completion report and artifacts; update Architecture only when the current implementation truth changed.

## Reviewer Workflow

1. Identify the exact user/developer promise and every reachable selector affected by the change.
2. Verify source and build membership rather than accepting the inventory text as proof.
3. Trace owner -> producer -> publication -> consumer -> failure/recovery -> retirement. Stop when a boundary is unowned or ambiguous.
4. Check the horizontal matrix for unlike backend/mode/content/product outcomes that cannot share one verdict.
5. Check requested-versus-active behavior, capacity/overflow, invalid inputs, cancellation, shutdown, diagnostics, and package isolation.
6. Confirm the selected checks can actually falsify the claim and that results identify revision, environment, command/workflow, oracle, observation, and artifacts.
7. Reject evidence-grade inflation: source is not build, build is not runtime, runtime is not native validation, and one screenshot is not correctness or performance.
8. Report orphan selectors, owners, representations, risks, criteria, failures, checks, or artifacts as actionable findings with the narrowest owner.

## Horizontal Evaluation

Create matrix cells only for dimensions that can change behavior or evidence. Typical axes are:

- product and profile: Editor/Runtime, Debug/Development/Shipping, workspace/package;
- platform and GPU: Windows version, D3D12/Vulkan, adapter tier/vendor, driver/SDK;
- execution: serial/threaded, graphics/compute/copy, raster/inline/native ray pipeline;
- content: source format, material/alpha type, static/instanced/skinned/morphed geometry, light type, map;
- presentation: resolution, resize/minimize, exposure, upscaler/reconstruction, view mode, output encoding;
- lifecycle: cold/warm start, first/repeat load, reload, cancellation, failure, recovery, shutdown.

Do not multiply cells that share the same implementation and observable outcome. Do split cells when capability gates, fallbacks, quality, cost, or failure behavior differ.

## Vertical Evaluation

For a runtime feature, trace:

```text
user/config request
  -> validation and requested-versus-active selection
  -> content/world producer
  -> immutable publication
  -> Renderer scene/view/frame preparation
  -> frame-graph pass and shader identity
  -> RHI resource/state/queue/native command
  -> GPU completion/history/retirement
  -> presentation, diagnostics, failure, and recovery
```

For an offline or developer workflow, trace:

```text
user/CLI/editor request
  -> prerequisite and path validation
  -> plan/import/compile/cook work
  -> bounded concurrency and cancellation
  -> transactional publication
  -> downstream invalidation and runtime consumer
  -> visible success/failure, recovery, cleanup, and retained evidence
```

Every arrow is a review boundary. A trace is incomplete when an owner, representation, lifetime, failure, or observable result is missing, even if both endpoint systems exist.

## Documentation Hardening Exit

The documentation pass is complete only when:

- every reachable surface has a module capability row or an explicit exclusion;
- module and cross-module indexes answer the common reader questions without duplicated authority;
- every vital product journey has horizontal coverage and a vertical trace or a named gap;
- every unresolved claim has a stable plan/report destination and a smallest next check;
- all local links/anchors, status metadata, tables, and source routes resolve;
- no documentation-only check is reported as executable evidence.

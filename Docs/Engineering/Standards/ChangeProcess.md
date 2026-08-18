# Change Process

Status: binding implementation and review process

Applies to: every material owned-code change; scale the record to risk and scope

This document owns the lifecycle of a change: preparation, implementation discipline, reconciliation, acceptance, and completion reporting. Architecture, coding, domain, and validation rules remain in their subject standards.

## Before Editing

### Select Applicable Authority

Use the [standards map](README.md#standards-map) to record:

- the integration contract and each touched subject standard;
- applicable [`PGE-01` through `PGE-15` requirements](../../Strategy/Requirements.md) only when the change affects or claims them;
- applicable [acceptance-workload gates](../BistroAndSanMiguelWorkloads.md) only when the change affects their proof surface;
- expected evidence and unavailable checks.

Classify each conditional gate as **advance**, **preserve**, **not applicable**, or **blocked**. “Not applicable” needs a short reason.

### Define the Outcome and Complexity Budget

Before adding a persistent concept, state:

- the user or engine outcome and authoritative production path;
- the current owner that will absorb the change;
- the essential state, branch, type, dependency, configuration, or public API being added;
- each material copy or new value holder and the boundary reason that prevents using a reference, view, handle, or move;
- the old path, repeated policy, dead surface, or unnecessary work being removed or simplified;
- the owned artifacts that will be regenerated and the internal versions, migrations, adapters, aliases, fallbacks, or dual paths that will be deleted rather than preserved;
- the performance classification and budgets that may move;
- explicit non-goals that bound cleanup and prevent a second architecture.

This is a reasoning ledger, not a target for negative line count. A change may add substantial implementation while remaining structurally reductive. If permanent complexity has no current consumer, owner, invariant, or evidence-backed need, do not add it.

### Reconcile Current State

Before relying on an earlier prompt, completion record, test, or criterion:

1. inspect the current owner, data path, consumers, lifetime, and replacement history;
2. classify the criterion as an enduring invariant, a still-relevant transition, or a superseded intermediate requirement;
3. prove enduring and still-relevant requirements against current code and evidence;
4. retain historical wording only as source trace, rationale, or regression context;
5. preserve the underlying correctness, ownership, lifetime, determinism, parity, and evidence invariant without restoring obsolete architecture.

### Find the Existing Responsibility

Search the complete owned repository with `rg` and `rg --files` before adding or renaming a concept. Search exact and rejected names, semantic counterparts, producers/consumers, services, queues, caches, packets, handles, build membership, tests, profiler labels, and thread labels.

For each counterpart decide **use**, **extend**, **refactor**, **replace and delete**, or **add because no owner exists**. Do not escape integration with `New*`, `*2`, vague concurrency suffixes, compatibility namespaces, or parallel directory trees.

### Map the Ownership Path

For each touched or proposed file record:

- target, module, subsystem, and `Public`/`Private` placement;
- primary type/operation and reason to change;
- each changed capability/invariant's authoritative definition and every material use classified as authority, composition, producer, consumer, or duplicate;
- mutable-state and lifetime owners;
- direct producers and consumers;
- final location and nearby instances of the same ownership defect.

Trace definitions to uses and changed uses back to their proper definition using the [definition and usage placement audit](RepositoryStructureAndOwnership.md#definition-and-usage-placement). Inspect the complete direct ownership path, not only prompt-named files. This is bounded reconciliation, not permission for unrelated cleanup.

### Reconcile the Touched Neighborhood

Review the changed unit, its direct owner and consumers, its folder boundary, and the build/test/documentation surfaces that express the same responsibility. The change MUST:

- remove superseded code, names, includes, flags, configuration, tests, comments, diagnostics, and documentation;
- correct the same local ownership or vocabulary defect where leaving it would create two conventions or preserve duplicate authority in the touched path;
- keep adjacent code consistent with the new invariant when that repair is small, low-risk, and validated with the change;
- stop or split an explicit prerequisite when the required reconciliation is too large to review safely.

Do not turn neighborhood cleanup into repository-wide reformatting, speculative decomposition, or unrelated debt collection. The boundary is coherence of the delivered ownership path.

### Keep Small Change Ledgers

Record preservation and deletion for every material change. Add the [data/access inventory](DataOrientedDesign.md#dataaccess-inventory) or a concurrency/hazard ledger only when those subjects are touched. Keep these in the change description or an existing implementation record, not in a permanent runtime reporting system.

Before parallelism or pipelining, define ownership, input/output identity, deterministic ordering, serial reference behavior, exclusive writable ranges, serial crossover, and failure/publication policy. Partial publication and ambiguous ownership are not policies.

### Plan the Fast Feedback Loop

Implement or generate one coherent owner/invariant-sized batch at a time. Before each batch, identify the files and production edge expected to change plus the cheapest check that could expose a mistake. Then:

1. locate the current owner and nearby accepted precedent with exact searches;
2. make the bounded implementation or generation pass;
3. inspect the scoped diff immediately for unintended files, duplicated policy or data, unjustified holders/copies, stale names, generated boilerplate, internal versioning/compatibility machinery, and ownership drift;
4. run cheap applicable formatting, static, schema, documentation, or architecture checks;
5. compile the smallest owning target only when the batch needs compilation evidence;
6. escalate runtime or broad validation only under the [claim-driven validation rules](ValidationPerformanceAndEvidence.md#claim-driven-validation-selection).

Do not accumulate a large AI-generated diff and use a full engine, game, editor, or workspace build as the first feedback signal. Broad builds, whole validation sets, all-content cooks, clean rebuilds, multi-backend runs, and acceptance workloads are final evidence only when the affected contract makes that breadth applicable. Uncertainty is resolved by inspecting the owner and choosing a more discriminating check, not by launching unrelated work.

## During Implementation

- Keep high-level workflow visible as orchestration and cohesive mechanism in capability implementations.
- Keep one mutable authority and one production path.
- Apply the [single-truth and copy budget](DataOrientedDesign.md#single-truth-and-copy-budget): prefer references, views, handles, and moves; copy only at a named lifetime or publication boundary and resolve that snapshot once per epoch.
- Spend the declared complexity budget only on current behavior; remove scaffolding that becomes unnecessary as the vertical slice closes.
- Apply the [current clean-break policy](IntegrationStyleGuide.md#current-clean-break-policy): update all producers and consumers, delete replaced paths and internal compatibility/versioning machinery in the same coherent change, and regenerate disposable local artifacts. Do not leave a later compatibility or deletion gate.
- Update implementation, headers, build membership, exports, removal of obsolete test registrations, and documentation atomically when ownership moves. Temporary validation tests are not submitted; follow [Validation, Performance, and Evidence](ValidationPerformanceAndEvidence.md#submitted-test-code).
- Re-check preservation and deletion when a new consumer appears.
- Keep generated and AI-assisted batches bounded, inspect their scoped diff immediately, and treat every changed line as untrusted until independently reviewed and proportionally validated.
- Stop when a product or architecture decision exceeds the task's authority.

## Review and Acceptance

Use the [SparkleEngine Code Review](../CodeReview.md) procedure and run the review section or acceptance rules in every selected subject standard; do not recreate those checklists in the change description. Then verify the integration itself:

- the requested outcome works through the authoritative production path;
- ownership, lifetime, publication, and failure behavior are explicit;
- moved, split, or deleted paths are reconciled with build membership, includes, exports, tests, and documentation;
- preservation and deletion records match the final implementation;
- old names and production references are gone where intended;
- no internal version, migration reader/writer, compatibility adapter, alias, fallback, old/new dispatch, or dual representation preserves the replaced Sparkle-owned contract;
- the final structure spends no unexplained complexity and leaves the touched ownership path easier to navigate;
- performance impact is classified and any material hot-path, memory, latency, loading, cooking, or build-time risk has proportional evidence;
- exact claim-to-check mappings, validation commands, configurations, backends, results, escalation, and applicable unavailable checks are recorded; unrelated broad checks are not reported as missing evidence;
- performance and AI-assisted claims meet [Validation, Performance, and Evidence](ValidationPerformanceAndEvidence.md);
- applicable `PGE-*` and workload gates are linked and classified without copying their contracts.

## Completion Report

Report only applicable sections:

1. outcome and user-visible behavior;
2. current-state reconciliation;
3. repository search and use/extend/refactor/replace/add decisions;
4. ownership, lifetime, publication, and failure contract;
5. files grouped by responsibility;
6. complexity budget, preservation, deletion, and touched-neighborhood cleanup;
7. structure/data/concurrency reconciliation;
8. performance classification plus exact validation and performance evidence;
9. naming and stale-reference audit;
10. limitations and unavailable evidence;
11. linked `PGE-*`/workload classification;
12. `PASS` or `BLOCKED` acceptance status.

## Stop Conditions

Stop for a product or architecture decision when a prerequisite has not passed; required behavior cannot survive the owner/lifetime model; the repository contradicts an assumption; a public abstraction has no real consumer; permanent machinery exceeds the stated complexity budget without an essential current invariant; exclusive outputs or deterministic order cannot be proven; serial parity differs without explanation; material performance exposure has no proportional evidence; native validation reports ownership/state errors; scope materially exceeds authorization; or the only justification is future use, familiarity, framework elegance, or unmeasured performance.

Do not hide a stop condition behind compatibility code, a broad mutex, retries, logging, or a temporary second architecture. Rejected technical patterns remain in the subject standard that owns their reason.

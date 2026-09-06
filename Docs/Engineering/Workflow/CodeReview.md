# SparkleEngine Code Review

Status: workflow index; code-review entry point whose owning standards control

Applies to: human or AI review of a SparkleEngine changelist

## Responsibility

This playbook makes repository review repeatable. It defines review order, evidence expectations, finding shape, and the compact prompt used to start an AI review. It does not create alternative implementation policy.

When a change adds, removes, advertises, or reclassifies a capability, also apply the [Capability Documentation Review](CapabilityReview.md) to its module dossier, horizontal matrix cells, vertical path, and evidence/report mappings.

The binding owners are:

- [L. Change Integration](ChangeIntegration.md) for integration invariants;
- [Change Lifecycle](ChangeLifecycle.md) for preparation, review, acceptance, and completion;
- the [Engineering task map](../README.md#choose-by-task) for structure, code, naming, data, concurrency, domain, and evidence rules;
- relevant canonical [Architecture](../../Architecture/WholeRepositoryMap.md) documents for accepted boundaries;
- code, CMake, formatter, static-analysis, compiler, and test configuration for implemented and executable behavior.

Review is read-only unless the user explicitly asks for fixes. Never modify the changelist merely because this file was referenced.

## Reusable AI Review Prompt

Use this prompt as written or append the intended outcome and comparison base:

```text
Review the complete SparkleEngine changelist using Docs/Engineering/Workflow/CodeReview.md.
Treat the review as read-only unless I explicitly ask for fixes. Inspect tracked,
staged, unstaged, and relevant untracked changes, then inspect the directly affected
owners, producers, consumers, lifetimes, build membership, tests, configuration, and
documentation. Apply ChangeIntegration.md, ChangeLifecycle.md, every applicable
subject standard, and the relevant canonical architecture. Code and executable build
configuration prove implemented behavior.

Keep review and validation claim-driven. Start from the diff, use exact searches and
bounded owner/consumer reads, and do not scan unrelated subsystems. Before running a
command, name the claim it can falsify and the condition that would justify escalation.
Do not run a full engine/game/workspace build, whole suite or cook, clean rebuild,
paired-backend smoke, or acceptance workload merely because it might be useful. Run a
broad check only when the affected contract, selected standard/gate, explicit request,
or an inconclusive narrower result requires that exact surface.

Find real defects and unmanaged complexity, not arbitrary size or style preferences.
Reject duplicate authority, incomplete replacement, speculative abstraction, hidden
lifetime or concurrency, orchestrators containing capability mechanics, god functions,
classes, files, or folders, needless diagnostics/logging, and performance claims or
material regressions without proportional evidence. Require bounded cleanup of the
complete touched ownership path while avoiding unrelated refactors.

Apply the current clean-break policy: Sparkle has no active compatibility obligation.
Require one current owned representation, update every producer and consumer, delete
the replaced path, and regenerate local artifacts. Reject new or extended internal
version fields, legacy paths, migration readers/writers, compatibility adapters/shims,
deprecated aliases, old/new dispatch, dual read/write, and fallbacks. Do not confuse
external API/tool/file-format versions or lifetime generations with legacy support.

For every changed capability or invariant, find its authoritative definition and every
material use. Classify each site as authority, composition, producer, consumer, or
duplicate. Verify that orchestrators own intent/order while capability owners contain
mechanics, and that callers do not repeat policy, transforms, validation, state, caches,
fallbacks, or backend decisions. Search semantic equivalents, not only matching names.

Apply the single-truth and copy budget to every new holder and material data copy. Prefer
references, views, handles, and moves. Accept a copied snapshot only for a named lifetime,
thread, publication, edit/commit, serialization, or ABI boundary; require one producer,
immutable consumption, an exact epoch, and one resolution per epoch. Reject convenience
mirrors, copied defaults/settings, forwarding DTOs, and parallel mutable truth.

Report actionable findings first, ordered by severity, with precise file:line evidence,
the failure scenario, violated owner or invariant, and the smallest coherent correction
boundary. Then report review scope, architecture/complexity/performance assessment,
exact validation run and results, residual risks or unavailable evidence, and final
PASS or BLOCKED status. If no actionable finding exists, say so explicitly; do not
invent one. Do not claim an unrun check passed.
```

## Review Objective

Accept a changelist only when it delivers the requested outcome through the authoritative production path and leaves the touched system at least as coherent, maintainable, performant, and easy to reason about as before.

The review surface is larger than the textual diff and smaller than the repository. It includes changed lines plus the direct owner, producers, consumers, lifetime/publication/failure edges, nearby code expressing the same invariant, build membership, tests, configuration, and documentation. This is the **touched ownership path**.

## Review Budget and Escalation

Review depth follows risk and unresolved questions, not repository size. Use four bounded passes:

1. **Orient** — inspect guidance, status, diff/rename/delete statistics, and changed-file build membership; produce an impact map before reading deeply.
2. **Trace** — follow exact symbols through the touched ownership path and selected authority. Read additional files only to answer a concrete ownership, lifetime, behavior, or replacement question.
3. **Challenge** — form specific failure hypotheses and verify them against code or executable policy. Stop searches that cannot produce an actionable finding or change acceptance.
4. **Validate** — map each remaining claim to the cheapest check that can falsify it, then follow the [claim-driven validation ladder](../Verification/ValidationAndEvidence.md#claim-driven-validation-selection).

Do not read whole subsystems, enumerate unrelated files, or execute broad checks to demonstrate effort. Reliability comes from closing the owner/producer/consumer/lifetime and failure path for each material change, then selecting discriminating evidence. A review may `PASS` without a full build when no applicable claim requires one and narrower evidence is sufficient. It may not `PASS` by silently classifying applicable evidence as optional.

Once a concrete P0–P2 finding already makes the review `BLOCKED`, skip expensive acceptance validation that cannot change that status. Continue only the bounded tracing needed to find other actionable defects, qualify the blocker, or satisfy an explicit validation request.

## Review Procedure

### 1. Establish Scope and Intent

- Inspect repository guidance, status, diff statistics, staged/unstaged changes, renames, deletions, and relevant untracked files.
- Build a changed-file and ownership impact map before opening broad context. Prefer exact symbol, path, build-membership, and rejected-name searches over subsystem-wide reading.
- State the intended user or engine outcome in one sentence.
- Identify preserved current behavior, explicit non-goals, comparison base, and the complete clean-break replacement/regeneration boundary.
- Separate the reviewed changelist from unrelated user work already present in the tree.
- Reject a diff that cannot explain why each changed file participates in one coherent result.

### 2. Select Authority

- Read Change Integration and Change Lifecycle.
- Select every applicable subject standard; do not enforce a candidate or personal preference as binding policy.
- Read the canonical architecture for any changed responsibility or dependency boundary.
- Use code and executable configuration to verify current behavior. Treat plans, snapshots, research, comments, and previous review text according to their declared status.

### 3. Trace the Production and Ownership Path

Trace input to output across success, rejection, cancellation, failure, publication, and retirement. Identify:

- mutable-state and lifetime owner;
- producers, consumers, thread or queue affinity, and stable identity;
- public/private and module boundaries;
- allocation, publication, reclamation, and stale-result behavior;
- build target, source membership, exports, configuration, tests, and documentation;
- the old path and every production reference that should disappear.
- any internal version, migration, compatibility, legacy, alias, fallback, or dual-representation mechanism in the touched path that should disappear rather than be extended.

Reading only changed lines is insufficient when their contract lives elsewhere.

### 4. Audit Definition and Usage Placement

For each changed capability, policy, transform, validation rule, and mutable fact, apply the [definition and usage placement audit](../Foundations/ModuleOwnership.md#definition-and-usage-placement):

1. name the authoritative definition, module, class/file owner, invariant, lifetime, and failure boundary;
2. search from the definition to every material use, then from every changed use back to the owner that should define its behavior;
3. classify each site as **authority**, **composition**, **producer**, **consumer**, or **duplicate**;
4. search semantic equivalents, copied branches/validation/error text, repeated switches, parallel state/caches, rejected names, and callers that reach around an owner facade;
5. verify high-level files express intent, ordering, lifecycle, and composition while lower capability owners contain algorithms, loops, parsing, encoding, allocation, synchronization, and backend mechanics;
6. apply the [single-truth and copy budget](../Foundations/DataAndMemory.md#single-truth-and-copy-budget) to every new holder and material copy, including its source, boundary reason, lifetime/epoch, producer, consumers, and invalidation;
7. apply the [mandatory orchestrator/implementor boundary](../Foundations/ModuleOwnership.md#mandatory-orchestratorimplementor-boundary) across the complete touched family: each independently changing payload/type/policy/backend behavior has a dedicated implementor, while the actual lifecycle owner only selects, sequences, and publishes it;
8. verify module dependency direction, `Public`/`Private` placement, build membership, and the ability to change or delete the capability through one bounded owner path.
9. for backend/provider sibling families, separate neutral invariants from native mechanics: search for repeated case lists, predicates, validation, and error meaning; require one neutral owner when the inputs and rule are common, while retaining local native translation, capability queries, and genuinely different failure contracts.
10. reject a helper that consumes only another owner's vocabulary unless the containing subsystem contributes a real local/native invariant; after moving authority, require exact searches to prove that mirrored types, redundant derived fields, duplicate converters, and old call paths are gone.

Report a placement defect when the same behavior or fact has more than one production authority, when logic lives in a caller because it was convenient rather than owned, or when feature work scatters knowledge across unrelated modules. Do not consolidate code whose semantics, lifetimes, failure contracts, or cost models are genuinely different merely because its syntax looks similar.

### 5. Review Outcome and Correctness

- Does the real production path deliver the requested behavior, or only scaffolding, a test double, a debug path, or an unused API?
- Are inputs, outputs, invariants, units, coordinate spaces, errors, fallback, and unsupported states explicit?
- Are edge cases and failure paths correct without preserving partially published state?
- Are D3D12/Vulkan, serial/parallel, runtime/editor, cold/warm, and feature variants preserved where applicable?
- Do tests exercise observable behavior and fail when the claimed invariant is deliberately broken?

### 6. Review Ownership, Decomposition, and Bloat

For every persistent concept ask: **Who owns it? Who consumes it now? Which invariant requires it? When does it die? What older complexity does it replace?**

| Unit | Healthy responsibility | Review failure signal |
| --- | --- | --- |
| Orchestrator | ordering, lifecycle, policy selection, capability composition, stage-level failure, publication | owns parsing, transforms, loops, caching, partitioning, backend mechanics, UI internals, or diagnostic formatting |
| Capability implementation | one cohesive algorithm, transform, state machine, policy, encoding, allocation, backend operation, or lifetime | selects unrelated product workflows or reaches through another owner's facade |
| Function | one named operation at one abstraction level | mixes high-level stages with several stages' loops, state transitions, allocation, synchronization, or failure detail |
| Class | one owner, invariant, lifetime, and reason to change | unrelated state, consumers, policies, modes, or ownership domains accumulate together |
| File | one primary type, cohesive operation, or private collaboration | becomes an edit hotspot for unrelated features or a relocation of an unchanged god unit |
| Folder | one durable subsystem, capability, lifetime, backend, or visibility boundary | catch-all naming, mixed owners, dependency shortcuts, or deep ceremony without a real boundary |

Do not use line counts as acceptance gates. Size is a prompt to inspect responsibility. Splitting is valid only when the result removes knowledge and gives each unit a real owner; wrappers, generic helpers, factories, interfaces, managers, and one-file directories can increase bloat while making the original flow harder to follow.

Review the **complexity budget**, not just additions:

- permanent types, services, state, modes, flags, configuration, queues, tasks, dependencies, public APIs, logging, diagnostics, tests, files, and folders added;
- obsolete paths, duplicate policy, dead state, compatibility, branches, includes, configuration, diagnostics, and files removed;
- whether the same outcome can use an existing owner with fewer concepts or a more direct data path;
- whether any abstraction exists only for hypothetical reuse, a possible backend, or design-pattern symmetry.

A code-positive change can be structurally reductive. A short change can still add an unmanaged second authority.

Review **change locality** as the scaling test: a capability should evolve and be removable through its owner, explicit composition point, tests, and documentation. If every feature requires edits to unrelated coordinators, generic utilities, global registries, backend internals, or scattered mode checks, the architecture is centralizing complexity rather than scaling.

### 7. Review Touched-Path Cleanup

Require cleanup when it is necessary to make the delivered path coherent:

- delete the replaced implementation, internal version field, migration reader/writer, compatibility adapter/shim, alias, flag, fallback, dual path, and stale name;
- remove dead includes, exports, build entries, tests, comments, logs, counters, configuration, and documentation;
- repair the same directly adjacent ownership or vocabulary defect when leaving it creates two conventions or preserves duplicate authority;
- update moved contracts, consumers, tests, build membership, and documents atomically.

Do not demand broad unrelated cleanup, repository-wide formatting, speculative frameworks, or a refactor whose validation cannot fit the changelist. If a large prerequisite is essential, split it explicitly and review it first; do not bridge it with an indefinite compatibility architecture.

### 8. Review Performance as Part of Delivery

Require one classification from [Performance Is a Delivery Property](../Verification/ValidationAndEvidence.md#performance-is-a-delivery-property): **improves**, **preserves**, **no runtime exposure**, or **blocked**.

Inspect the cost model relevant to the change:

- work cardinality, complexity, full scans/rebuilds, repeated lookup, copies, and uploads;
- allocation churn, memory high-water, locality, pointer chasing, branches, and data layout;
- task count, grain, contention, waits, atomics, barriers, queues, frames in flight, and critical path;
- CPU time, GPU time, queue behavior, latency, loading/cooking/startup, compile/link/package cost, and cold/warm behavior;
- quality, determinism, backend support, fallback, and memory traded for speed.

Prefer removal of work and data movement before caching, parallelism, custom allocators, lock-free code, or backend special cases. Require reproducible before/after evidence for optimization claims and proportional regression evidence for material exposure. Reuse existing profilers and validation; do not accept permanent logs, counters, dashboards, snapshot APIs, or report files created only to prove this change.

### 9. Review Evidence and Completion

- For a material owned change, verify the [iteration control record](ChangeLifecycle.md#create-the-iteration-control-record) connects the applicable `NS-*`, `PGE-*`, roadmap or plan target, `FCR-*`, `RISK-*`, `AC-*`, `FM-*`, and `CHK-*` identifiers. Any applicable orphan, missing oracle, or result without an artifact is an evidence defect; do not reconstruct the chain after implementation merely to obtain `PASS`.
- For each material claim, record the smallest check that can falsify it and the trigger for escalation. Availability alone does not make a check applicable.
- Start with scoped inspection and cheap deterministic checks. Compile the smallest owning target only when compilation evidence is required; add runtime, cook, backend, capture, or workload breadth only when the claim crosses that boundary.
- Do not run full product/workspace builds, whole suites/cooks, clean rebuilds, paired backends, or acceptance workloads as speculative confidence checks. Apply the binding selection rules in [Validation, Performance, and Evidence](../Verification/ValidationAndEvidence.md#claim-driven-validation-selection).
- Run `architecture_boundary_check` when Renderer/RHI boundaries change.
- Run `git diff --check` before acceptance.
- Record exact commands, configuration, backends/hardware where relevant, results, and unavailable checks.
- Inspect generated and AI-assisted output line by line; independently verify APIs, math, layouts, lifetimes, concurrency, tests, citations, provenance, and performance claims.
- Never infer runtime correctness from documentation checks, compilation from static inspection, or performance from traces, utilization, thread count, or FPS alone.

## Findings Contract

Report findings before summaries. A finding must be actionable, introduced or exposed by the changelist, and supported by a concrete failure, invariant violation, or binding standard.

Use these severities:

- **P0** — catastrophic data loss, security, corruption, or consistently unusable product path;
- **P1** — production correctness, ownership, lifetime, concurrency, backend, deterministic behavior, or major regression that must be fixed before acceptance;
- **P2** — material architecture, maintainability, complexity, performance-risk, cleanup, or evidence defect that must be fixed before acceptance;
- **P3** — bounded non-blocking improvement; label purely optional style or education as `Nit` instead of manufacturing a defect.

Use this compact shape:

```text
[P1] Imperative finding title — path/to/File.cpp:line
Failure: concrete input, state, workload, or sequence that demonstrates the problem.
Why: violated owner, invariant, contract, or evidence requirement.
Correction boundary: smallest responsibility or production path that must change; do not prescribe ceremony.
```

Avoid vague requests to “refactor,” “optimize,” “add validation,” or “add logging.” Explain the actual risk. Do not block on undocumented taste, unrelated pre-existing debt, or hypothetical future requirements.

## Required Review Output

Return:

1. **Findings** — ordered P0 to P3 with precise file/line evidence, or `No actionable findings.`
2. **Changelist assessment** — outcome, touched ownership path, applicable authority, preserved behavior, and old path removed.
3. **Architecture and complexity** — owner/orchestrator/implementation boundaries, complexity spent and deleted, public/dependency effect, and bounded neighborhood cleanup.
4. **Performance** — classification, affected budgets, evidence, and tradeoffs.
5. **Validation and traceability** — iteration identifiers, risk and acceptance coverage, claim-to-check mapping, exact commands and results, escalation used, and applicable unavailable evidence; do not list unrelated broad checks as missing.
6. **Residual risk** — only concrete unverified behavior or evidence gaps.
7. **Status** — `PASS` only when no P0-P2 finding remains and required evidence is sufficient; otherwise `BLOCKED` with the blocking items.

Keep the report findings-first and proportional to risk. Do not bury a production defect under a long summary. Do not invent findings to appear thorough.

## Fast Acceptance Questions

Before `PASS`, answer yes to every applicable question:

- Is there one real outcome, mutable authority, lifetime owner, and production path?
- Does every material copy or new holder have a real boundary reason, one producer, an exact lifetime, and no path to become stale mutable truth?
- Can every new concept and changed file justify its current consumer and reason to exist?
- Are orchestration and capability mechanics separated without wrapper ceremony?
- Does every shared invariant live with its neutral/domain owner, with sibling backends/providers containing only their genuinely native or capability-specific mechanics?
- Did the changelist remove the complete replaced path and directly exposed duplicate authority?
- Did it preserve exactly one current Sparkle-owned representation, with no internal versioning or legacy/migration/compatibility path and with disposable artifacts regenerated?
- Are functions, classes, files, and folders cohesive by ownership rather than arbitrary size?
- Can this capability evolve or be removed through a bounded ownership path without unrelated repository-wide edits?
- Is performance classified, with material risk or improvement measured proportionally?
- Are diagnostics, logging, comments, tests, and instrumentation narrow, necessary, and owner-placed?
- Are supported behavior, backends, failure, cancellation, publication, and retirement preserved?
- Does the iteration record have no applicable target, risk, acceptance, failure, check, result, or artifact orphan?
- Do exact validation results support the claims without implying unrun checks passed?
- Is the touched ownership path easier for the next engineer to navigate and change?

If any applicable answer is unknown, gather evidence or report the review as blocked.

## External Precedent Boundary

Google's official review guidance treats improving code health, complexity review, useful tests, and authority-backed rather than preference-backed comments as core review concerns. Epic's official coding and performance guidance emphasizes long-term readability and measurement of real frame costs. These are supporting precedents only; SparkleEngine's local standards and executable configuration remain authoritative.

- [Google Engineering Practices: The Standard of Code Review](https://google.github.io/eng-practices/review/reviewer/standard.html)
- [Google Engineering Practices: What to Look For](https://google.github.io/eng-practices/review/reviewer/looking-for.html)
- [Epic C++ Coding Standard for Unreal Engine](https://dev.epicgames.com/documentation/unreal-engine/epic-cplusplus-coding-standard-for-unreal-engine)
- [Unreal Engine Performance Profiling](https://dev.epicgames.com/documentation/en-us/unreal-engine/introduction-to-performance-profiling-and-configuration-in-unreal-engine)

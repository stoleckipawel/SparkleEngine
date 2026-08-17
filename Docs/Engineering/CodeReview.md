# SparkleEngine Code Review

Status: summary and code-review entry point; owning standards control

Applies to: human or AI review of a SparkleEngine changelist

## Responsibility

This playbook makes repository review repeatable. It defines review order, evidence expectations, finding shape, and the compact prompt used to start an AI review. It does not create alternative implementation policy.

The binding owners are:

- [L. Integration Style Guide](Standards/IntegrationStyleGuide.md) for integration invariants;
- [Change Process](Standards/ChangeProcess.md) for preparation, review, acceptance, and completion;
- the [standards map](Standards/README.md#standards-map) for structure, code, naming, data, concurrency, domain, and evidence rules;
- relevant canonical [Architecture](../Architecture/WholeRepositoryMap.md) documents for accepted boundaries;
- code, CMake, formatter, static-analysis, compiler, and test configuration for implemented and executable behavior.

Review is read-only unless the user explicitly asks for fixes. Never modify the changelist merely because this file was referenced.

## Reusable AI Review Prompt

Use this prompt as written or append the intended outcome and comparison base:

```text
Review the complete SparkleEngine changelist using Docs/Engineering/CodeReview.md.
Treat the review as read-only unless I explicitly ask for fixes. Inspect tracked,
staged, unstaged, and relevant untracked changes, then inspect the directly affected
owners, producers, consumers, lifetimes, build membership, tests, configuration, and
documentation. Apply IntegrationStyleGuide.md, ChangeProcess.md, every applicable
subject standard, and the relevant canonical architecture. Code and executable build
configuration prove implemented behavior.

Find real defects and unmanaged complexity, not arbitrary size or style preferences.
Reject duplicate authority, incomplete replacement, speculative abstraction, hidden
lifetime or concurrency, orchestrators containing capability mechanics, god functions,
classes, files, or folders, needless diagnostics/logging, and performance claims or
material regressions without proportional evidence. Require bounded cleanup of the
complete touched ownership path while avoiding unrelated refactors.

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

## Review Procedure

### 1. Establish Scope and Intent

- Inspect repository guidance, status, diff statistics, staged/unstaged changes, renames, deletions, and relevant untracked files.
- State the intended user or engine outcome in one sentence.
- Identify preserved behavior, explicit non-goals, comparison base, and any accepted migration.
- Separate the reviewed changelist from unrelated user work already present in the tree.
- Reject a diff that cannot explain why each changed file participates in one coherent result.

### 2. Select Authority

- Read the Integration Style Guide and Change Process.
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

Reading only changed lines is insufficient when their contract lives elsewhere.

### 4. Review Outcome and Correctness

- Does the real production path deliver the requested behavior, or only scaffolding, a test double, a debug path, or an unused API?
- Are inputs, outputs, invariants, units, coordinate spaces, errors, fallback, and unsupported states explicit?
- Are edge cases and failure paths correct without preserving partially published state?
- Are D3D12/Vulkan, serial/parallel, runtime/editor, cold/warm, and feature variants preserved where applicable?
- Do tests exercise observable behavior and fail when the claimed invariant is deliberately broken?

### 5. Review Ownership, Decomposition, and Bloat

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

### 6. Review Touched-Path Cleanup

Require cleanup when it is necessary to make the delivered path coherent:

- delete the replaced implementation, adapter, alias, flag, fallback, and stale name;
- remove dead includes, exports, build entries, tests, comments, logs, counters, configuration, and documentation;
- repair the same directly adjacent ownership or vocabulary defect when leaving it creates two conventions or preserves duplicate authority;
- update moved contracts, consumers, tests, build membership, and documents atomically.

Do not demand broad unrelated cleanup, repository-wide formatting, speculative frameworks, or a refactor whose validation cannot fit the changelist. If a large prerequisite is essential, split it explicitly and review it first; do not bridge it with an indefinite compatibility architecture.

### 7. Review Performance as Part of Delivery

Require one classification from [Performance Is a Delivery Property](Standards/ValidationPerformanceAndEvidence.md#performance-is-a-delivery-property): **improves**, **preserves**, **no runtime exposure**, or **blocked**.

Inspect the cost model relevant to the change:

- work cardinality, complexity, full scans/rebuilds, repeated lookup, copies, and uploads;
- allocation churn, memory high-water, locality, pointer chasing, branches, and data layout;
- task count, grain, contention, waits, atomics, barriers, queues, frames in flight, and critical path;
- CPU time, GPU time, queue behavior, latency, loading/cooking/startup, compile/link/package cost, and cold/warm behavior;
- quality, determinism, backend support, fallback, and memory traded for speed.

Prefer removal of work and data movement before caching, parallelism, custom allocators, lock-free code, or backend special cases. Require reproducible before/after evidence for optimization claims and proportional regression evidence for material exposure. Reuse existing profilers and validation; do not accept permanent logs, counters, dashboards, snapshot APIs, or report files created only to prove this change.

### 8. Review Evidence and Completion

- Run the smallest relevant build, tests, formatter/static checks, architecture checks, and evidence gates available for the touched targets.
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
5. **Validation** — exact commands and results; distinguish passed, failed, and unavailable.
6. **Residual risk** — only concrete unverified behavior or evidence gaps.
7. **Status** — `PASS` only when no P0-P2 finding remains and required evidence is sufficient; otherwise `BLOCKED` with the blocking items.

Keep the report findings-first and proportional to risk. Do not bury a production defect under a long summary. Do not invent findings to appear thorough.

## Fast Acceptance Questions

Before `PASS`, answer yes to every applicable question:

- Is there one real outcome, mutable authority, lifetime owner, and production path?
- Can every new concept and changed file justify its current consumer and reason to exist?
- Are orchestration and capability mechanics separated without wrapper ceremony?
- Did the changelist remove the complete replaced path and directly exposed duplicate authority?
- Are functions, classes, files, and folders cohesive by ownership rather than arbitrary size?
- Can this capability evolve or be removed through a bounded ownership path without unrelated repository-wide edits?
- Is performance classified, with material risk or improvement measured proportionally?
- Are diagnostics, logging, comments, tests, and instrumentation narrow, necessary, and owner-placed?
- Are supported behavior, backends, failure, cancellation, publication, and retirement preserved?
- Do exact validation results support the claims without implying unrun checks passed?
- Is the touched ownership path easier for the next engineer to navigate and change?

If any applicable answer is unknown, gather evidence or report the review as blocked.

## External Precedent Boundary

Google's official review guidance treats improving code health, complexity review, useful tests, and authority-backed rather than preference-backed comments as core review concerns. Epic's official coding and performance guidance emphasizes long-term readability and measurement of real frame costs. These are supporting precedents only; SparkleEngine's local standards and executable configuration remain authoritative.

- [Google Engineering Practices: The Standard of Code Review](https://google.github.io/eng-practices/review/reviewer/standard.html)
- [Google Engineering Practices: What to Look For](https://google.github.io/eng-practices/review/reviewer/looking-for.html)
- [Epic C++ Coding Standard for Unreal Engine](https://dev.epicgames.com/documentation/unreal-engine/epic-cplusplus-coding-standard-for-unreal-engine)
- [Unreal Engine Performance Profiling](https://dev.epicgames.com/documentation/en-us/unreal-engine/introduction-to-performance-profiling-and-configuration-in-unreal-engine)

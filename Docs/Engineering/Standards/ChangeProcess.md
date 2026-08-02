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
- mutable-state and lifetime owners;
- direct producers and consumers;
- final location and nearby instances of the same ownership defect.

Inspect the complete direct ownership path, not only prompt-named files. This is bounded reconciliation, not permission for unrelated cleanup.

### Keep Small Change Ledgers

Record preservation and deletion for every material change. Add the [data/access inventory](DataOrientedDesign.md#dataaccess-inventory) or a concurrency/hazard ledger only when those subjects are touched. Keep these in the change description or an existing implementation record, not in a permanent runtime reporting system.

Before parallelism or pipelining, define ownership, input/output identity, deterministic ordering, serial reference behavior, exclusive writable ranges, serial crossover, and failure/publication policy. Partial publication and ambiguous ownership are not policies.

## During Implementation

- Keep high-level workflow visible as orchestration.
- Keep one mutable authority and one production path.
- Delete replaced paths in the same change unless an accepted migration names a later owner and deletion gate.
- Update implementation, headers, build membership, exports, tests, and documentation atomically when ownership moves.
- Re-check preservation and deletion when a new consumer appears.
- Treat generated and AI-assisted output as untrusted until independently reviewed and validated.
- Stop when a product or architecture decision exceeds the task's authority.

## Review and Acceptance

Run the review section or acceptance rules in every selected subject standard; do not recreate those checklists in the change description. Then verify the integration itself:

- the requested outcome works through the authoritative production path;
- ownership, lifetime, publication, and failure behavior are explicit;
- moved, split, or deleted paths are reconciled with build membership, includes, exports, tests, and documentation;
- preservation and deletion records match the final implementation;
- old names and production references are gone where intended;
- exact validation commands, configurations, backends, results, and unavailable checks are recorded;
- performance and AI-assisted claims meet [Validation, Performance, and Evidence](ValidationPerformanceAndEvidence.md);
- applicable `PGE-*` and workload gates are linked and classified without copying their contracts.

## Completion Report

Report only applicable sections:

1. outcome and user-visible behavior;
2. current-state reconciliation;
3. repository search and use/extend/refactor/replace/add decisions;
4. ownership, lifetime, publication, and failure contract;
5. files grouped by responsibility;
6. preservation and deletion;
7. structure/data/concurrency reconciliation;
8. exact validation and performance evidence;
9. naming and stale-reference audit;
10. limitations and unavailable evidence;
11. linked `PGE-*`/workload classification;
12. `PASS` or `BLOCKED` acceptance status.

## Stop Conditions

Stop for a product or architecture decision when a prerequisite has not passed; required behavior cannot survive the owner/lifetime model; the repository contradicts an assumption; a public abstraction has no real consumer; exclusive outputs or deterministic order cannot be proven; serial parity differs without explanation; native validation reports ownership/state errors; scope materially exceeds authorization; or the only justification is future use, familiarity, framework elegance, or unmeasured performance.

Do not hide a stop condition behind compatibility code, a broad mutex, retries, logging, or a temporary second architecture. Rejected technical patterns remain in the subject standard that owns their reason.

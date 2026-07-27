# K. Multithreaded Engine Implementation Prompt Series

Status: required execution companion to J; no prompt implies implementation already exists
Date: 2026-07-24
Canonical naming authority: J's industry-grounded concurrency and rendering vocabulary; enforced by Rule 10
Architecture and tutorial source: [J. Multithreaded Engine Architecture and Learning Program](J_MultithreadedEngineArchitectureAndLearningProgram.md)
Governing requirements: [A. Principal Rendering Requirements](A_PrincipalRenderingRequirements.md), [E. External Renderer Repository Comparison](E_ExternalRendererRepositoryComparison.md), [G. Advanced Graphics Engine Executive Summary](G_AdvancedGraphicsEngineExecutiveSummary.md), [H. Advanced Graphics Engineer Persona](H_AdvancedGraphicsEngineerPersona.md), and [I. Bistro and San Miguel Acceptance Workloads](I_BistroAcceptanceWorkload.md)
Coding and integration contract: [L. SparkleEngine Integration Style Guide](L_SparkleEngineIntegrationStyleGuide.md)

## Canonical Workload Rule

Every new or resumed prompt that touches content, materials, rendering, RHI, ray tracing, neural reconstruction, residency, profiling, capture, or presentation must apply I:

- retain Sponza as the rapid regression tier;
- use Bistro as the primary Tier 1 completion workload;
- support San Miguel through the same generic path as the Tier 1 secondary workload;
- name the affected `BIS-*` and `SMG-*` routes, expected artifact, and performance/quality gate;
- reject a Sponza-only completion claim for a Tier 1 task;
- reject scene-specific engine/shader branches and heavyweight source-asset commits.

Completed prompt narratives remain historical evidence. This rule governs future execution and does not retroactively claim that unavailable scenes were tested.

## Purpose

This document converts every required target in J into an ordered series of implementation prompts. Use one prompt at a time. Each prompt defines what to inspect, what to implement, what nearby code to improve, what old path to delete, how to validate the result, and when work must stop.

J answers:

- why the architecture exists
- how the concurrency concepts work
- what tradeoffs were selected
- what you must understand and be able to explain

K answers:

- what code to study next
- what exact change to make
- what must not be duplicated or exposed
- what tests and captures prove completion
- what must be deleted before advancing

This is not a request to implement the whole program in one branch or one agent turn. Each prompt is a vertical slice with a hard gate. Do not begin the next prompt until the current gate passes or a documented blocker is accepted by the repository owner.

Every copy-ready prompt is incomplete unless its non-negotiable rules explicitly invoke L in full and its acceptance gate requires L's complete integration-style checklist to pass for the whole touched changelist, including directly affected neighboring ownership paths. Copying a prompt without those two requirements does not weaken or remove the contract.

## Non-Negotiable Rules for Every Prompt

These rules are repeated in compact form inside every copy-ready prompt. Their detailed meaning is defined here.

### 1. Inspect Before Adding

Before adding a file, class, function, concept, service, queue, allocator, handle, event, setting, diagnostic, or test fixture:

1. search the entire repository with `rg`/`rg --files` for names and semantic counterparts;
2. inspect public/private headers, CMake membership, ownership, call sites, tests, and shutdown paths;
3. record the counterpart decision:
   - **use** the existing abstraction unchanged;
   - **extend/refactor** it into the required abstraction;
   - **replace** it and delete the superseded path;
   - **add** only when no coherent counterpart exists;
4. do not create `*2`, `New*`, `Async*`, `ThreadSafe*`, or parallel directory trees as a way to avoid integrating with existing ownership;
5. if two counterparts overlap, converge them before adding a third.

The implementation report must list the searches performed and the use/extend/replace/add decision for every major new type.

### 2. Daily Refactor / Leave It Better

Every touched area must leave the repository more coherent than it was found:

- improve misleading names and comments in the directly touched ownership path;
- remove dead includes, stale members, duplicated helpers, obsolete branches, and unreachable compatibility code exposed by the change;
- split functions only when the split creates a real ownership/data boundary;
- consolidate repeated validation or lifetime logic into the existing owner;
- keep refactoring bounded to the touched subsystem and its direct dependencies;
- never use “refactor later” to leave two production paths after a stage gate.

This rule does not authorize unrelated cleanup. The refactor must reduce complexity required to understand or validate the current prompt.

### 3. One Coherent Product, Not Feature Piles

- Preserve `GameFramework → Renderer → RHI`; keep Tasks below its consumers.
- Keep frame-graph scheduling/barrier authority in the frame graph.
- Keep queue submission, presentation, and mutable RHI ownership on the render coordinator.
- Keep ECS storage private to GameFramework; editor and renderer consume stable contracts, not registry internals.
- Reuse one SparkleTasks runtime; do not add subsystem pools or ad-hoc futures.
- Reuse existing profiler/debugger hooks; do not create task panels, report frameworks, or public queue/cache snapshots.
- Preserve D3D12/Vulkan parity and the feature contracts for raster, classic TLAS, PTLAS, reservoir lighting, reference path tracing, temporal/provider paths, shader ABI, capture, and tools.

### 4. Serial Contract Before Parallel Execution

For every converted path:

1. express the new ownership/data contract;
2. run it through the serial executor/consumer;
3. prove behavior and deterministic output parity;
4. add the smallest useful parallel range;
5. retain serial mode and small-work thresholds as validation/product policy.

Never debug a new data model, new lifetime, and new parallel scheduling policy simultaneously if a serial intermediate can isolate them.

### 5. Capability-Preservation and Deletion Ledgers

Every prompt produces two explicit review ledgers:

- **Preserve:** current features, backends, packages, workflows, and observable behavior that must remain.
- **Delete:** old pointers, futures, waits, copies, caches, callbacks, APIs, compatibility facades, object paths, or diagnostics replaced by the prompt.

A deletion may move to a named later prompt only when simultaneous removal would make the current vertical slice impossible. It must have an owner, exact target prompt, and no new call sites.

### 6. Evidence Before Claims

- A class/API existing is not proof that it works.
- A multithreaded trace is not proof of speedup.
- No race observed is not proof of race freedom.
- One backend passing is not parity.
- FPS alone is not a performance result.
- A test listed but not run is not evidence.

Report exact commands, configurations, worker counts, backend, validation state, workload, and relevant before/after values. Mark unavailable evidence as unavailable; do not invent a new reporting system to fill the gap.

### 7. Stop Conditions

Stop and report rather than guessing when:

- the prerequisite prompt has not passed;
- a required current feature cannot be preserved within the proposed ownership model;
- repository state contradicts J's audit materially;
- a new public API has no necessary cross-module consumer;
- a supposedly independent range has no provable exclusive outputs;
- the serial reference differs and the difference is unexplained;
- native D3D12/Vulkan validation reports an ownership/state error;
- completion requires a broad new subsystem outside the prompt.

### 8. Existing Concurrency Has No Grandfather Clause

J's LC-01 through LC-18 ledger is binding, and Prompt 00 must extend it when the repository changes. For every existing or newly encountered thread, future, mutex, atomic, native/Qt wait, device-idle call, detached process, callback registry, queue lock, or allocator lock:

1. assign an owner and protected invariant;
2. classify it as **keep + harden**, **owner-only**, **replace**, **delete wait**, or **prove or remove**;
3. name the prompt that closes it and the evidence that proves closure;
4. prohibit new consumers of a mechanism already marked for replacement;
5. when touching its ownership path, migrate or narrow it in the same prompt unless doing so would violate a prerequisite; any deferral needs an exact later prompt and no expanded use;
6. never invoke arbitrary callbacks, UI, provider/driver, I/O, logging, or destruction work while holding an engine lock unless a documented API contract makes it unavoidable and a re-entry test exists;
7. do not replace a legitimate boundary mutex with tasks merely to reduce the primitive count. The goal is zero **unclassified** synchronization, not zero synchronization.

Each prompt reruns the concurrency search in its touched subtree and reconciles its results with the legacy ledger. Prompts 22 and 29 rerun it repository-wide.

### 9. Professional Hazard Pre-Mortem

Before editing for any prompt, read J's MT-01 through MT-44 Professional Multithreading Failure Atlas and record the applicable hazard IDs. For each applicable ID:

1. point to the NVIDIA, AMD/GPUOpen, Epic, language, or API precedent behind the chosen recognizable pattern;
2. state how the mistake could enter the exact Sparkle path being changed;
3. state the ownership/progress/lifetime/performance invariant that prevents it;
4. name an assertion, deterministic test, injected failure, native validation run, or timeline experiment capable of disproving the implementation;
5. execute that proof before closing the prompt; “not observed,” high utilization, one clean run, or a vendor sample doing something similar is insufficient;
6. if a source sample uses a lock, wait, lazy creation, fixed partition, or simple pool, copy neither mechanism nor name automatically—first prove that its scope and tradeoff match Sparkle's production path.

Newly discovered distinct hazards extend J and this traceability table, not a new document, runtime hazard registry, lint framework, or shipping diagnostics API.

### 10. Canonical NVIDIA/AMD/Epic-Grounded Vocabulary

J's **Canonical Concurrency and Rendering Vocabulary** is binding. Before adding or renaming a symbol, inspect the exact responsibility and the current repository's established convention, then use the canonical Sparkle term:

- CPU scheduled work is `Task`; a physical executor agent is a worker `Thread`; recorded RHI/GPU work is a `CommandList`/`CommandBuffer`; unqualified/native `CommandQueue` means GPU submission, while every CPU queue fully qualifies its payload.
- Use `TaskNodeHandle`, `CompiledTaskGraph`, `TaskExecution`, `TaskExecutionContext`, `TaskExecutor`, `TaskExecutorConfig`, `TaskScope`, `TaskEvent`, `ParallelFor`, and `TaskLane::{FrameCritical, Background, BlockingIo}` for the planned task runtime. “Job system” is the architectural category, not a second C++ type family.
- Use `GameThread`, `EditorThread`, and `RenderThread` for physical roles; use `RenderCoordinator` for the owning service. Reserve `RHIThread` until a real deferred RHI translation thread exists.
- Use `RenderFrameQueue` for bounded frame publication and `RenderControlCommandQueue` for ordered renderer control. Do not call either a GPU command queue.
- Use the existing `RenderCommandContext` for pass-facing recording, public `RhiCommandRecordingLease` for move-only exclusive borrowing, backend-private `<Backend>CommandRecordingContext` for native recording ownership, existing `ERhiQueueType` for GPU queue class, and existing `RhiSubmissionToken` for GPU completion/order.
- `Handle` identifies, `Token` proves, `Lease` borrows exclusively, `Context` supplies transient state, `Scope` owns asynchronous lifetime, `Execution` is one submission, and `Lane` is scheduling policy. A new name must obey the lifetime meaning it advertises.
- Do not introduce `Job`, `WorkItem`, bare `Context`, bare `Queue`, `*Manager`, `Async*`, `ThreadSafe*`, `LockFree*`, `MultiThreaded*`, `New*`, `*2`, or `MT*` as an escape from a precise responsibility. A justified established existing name is reconciled, not mechanically churned.

Every prompt searches for both the canonical term and rejected aliases in its touched scope. A rename updates filenames, C++ symbols, tests, profiler/debugger labels, comments, CMake references, and documents in one prompt; compatibility aliases are temporary within that prompt and deleted before its gate. The completion report must include a naming reconciliation even when the decision is to retain an accurate existing name.

### 11. Readable Product Code and Proportionate Validation

Concurrency correctness must be encoded at the narrowest reusable ownership boundary, not repeated as logging/assertion boilerplate throughout orchestration code. Follow the owner/context/lease separation used by the NVIDIA, AMD, Epic, and O3DE references in J:

- implementation types own mechanism and invariants; orchestrators express order and policy in short, readable calls;
- do not declare local classes or structs inside product function bodies. Lifecycle guards, runtime records, visitors, and policy objects belong at the narrowest file-private or owning-class scope where their complete invariant can be read and reused. Short algorithm/callback lambdas may remain local; extract them when they acquire multi-step lifecycle, synchronization, or policy behavior;
- prefer an owner-bound access type, scoped lease, typed token, or one gateway assertion over copying `AssertOwnerThread("method")` into every facade method;
- use generalized abstractions only when at least two real consumers share the same lifetime semantics; do not generalize feature policy into Core;
- place CVars/settings in the owning subsystem's dedicated `*CVars`/settings unit, never in a general parser, launch loop, or unrelated orchestrator;
- add only essential correctness assertions, failure diagnostics, and profiler labels with a concrete vendor/API precedent and a falsifying use; no per-item log spam, speculative counters, or diagnostic mirrors of ordinary state;
- prompt-specific unit tests, stress harnesses, fault injectors, and boundary-check scripts are disposable verification scaffolding: create them, run the required gate, preserve the command/result and invariant learned, then delete their source, CMake targets, fixtures, generated data, and dedicated test directories before handoff. Do not leave Sparkle with a parallel maintained test product. Existing reusable product validation or an owner-approved permanent test facility may be used, but a prompt does not create one implicitly;
- do not run a full repository build, all scenes, all backends, and heavyweight captures after every prompt or every edit. At the end of an ordinary code prompt, run the smallest affected target compile and focused deterministic test once. Full D3D12/Vulkan product builds and representative-scene matrices belong to integration gates 00, 05, 13, 20–22, and 29, or when the changed ABI/backend boundary specifically requires them;
- performance captures belong to measurement prompts 05, 23, 25, 28, and 29. Other prompts reuse the last compatible baseline and add a new capture only when their acceptance claim depends on timing;
- documentation-only prompts run link/search/format checks, not product builds.

The completion report must say why each validation action was proportionate and confirm that the touched product functions contain no local class/struct declarations. More logs, assertions, targets, and repeated full builds are not stronger evidence when they do not cross the changed invariant.

### 12. Reinforce Module, Folder, File, and Type Ownership

Every prompt must improve the physical repository structure of the architectural path it touches. Structure is part of the implementation contract, not optional cleanup after behavior works:

- before editing, map every touched and proposed file to its owning module, subsystem responsibility, public/private status, primary type or operation, and direct consumer;
- inspect the touched file's neighboring directory and semantic counterparts before choosing a destination. Extend an established coherent folder vocabulary; do not create synonymous trees, generic dumping grounds such as `Misc`, or a one-file directory without a demonstrated growth boundary;
- public folders contain stable cross-module contracts only. Runtime records, queues, native/backend mechanisms, policy implementation, and orchestration details remain private to their owner;
- a significant public class or contract has a filename that matches its canonical primary type. A `.cpp` normally matches the public/private type or clearly named operation it implements. Small tightly coupled private helpers may share an implementation unit; do not fragment one responsibility across ceremonial files;
- separate orchestration from mechanism when they have different owners or reasons to change. Folder and file boundaries must reveal that distinction without forcing readers through wrapper-only layers;
- when an existing touched file, class, or function is materially misplaced or misleadingly named, move or rename it in the same prompt. Update includes, forward declarations, exports, CMake membership/source groups, generated/package references, profiler labels, tests, and documentation atomically; leave no compatibility include or duplicate spelling after the gate;
- review nearby files that participate in the same changed ownership path. Reassign them when the prompt exposes the same structural defect, but do not authorize repository-wide cosmetic moves unrelated to the vertical slice;
- CMake targets and dependency direction remain the authority for module ownership. A folder move may clarify architecture but must never conceal an illegal link/include edge or create a convenience dependency;
- do not equate more folders/files with better architecture. Each boundary must communicate a stable responsibility, ownership/lifetime boundary, backend split, or independently changing implementation concern;
- before acceptance, print the touched subtree, search old paths/names, inspect target sources/includes, and confirm that filenames, primary types, namespaces, exports, and directory ownership tell one coherent story.

The completion report must contain a **structure reconciliation** listing files retained, moved, renamed, split, merged, or deliberately co-located, with the ownership reason for each decision. “Existing location” is not a justification by itself.

### 13. Evidence-Grounded Data-Oriented Design

Data-oriented design is binding wherever a prompt creates, migrates, publishes, stores, transforms, sorts, uploads, or repeatedly traverses data. An ECS, array, packet, or `SoA` name does not satisfy this rule by itself:

- before choosing a schema, record the real producer, consumer(s), type, frequency, quantity/cardinality, shape, value distribution/probability where relevant, mutation phase, lifetime, access order, stable key, and bandwidth/latency constraint;
- design the transformation and access pattern first. A generic container, reflection layer, object hierarchy, visitor, or template must not hide the concrete operation performed on the actual data;
- maintain one authoritative mutable source per domain. Editor models, render packets, render proxies, GPU tables, caches, and cooked products are explicitly sequenced or generation-tagged derived projections where stale rejection is required, with one-way publication and resync rules—not competing authorities;
- separate hot/cold, static/dynamic, structural/per-frame, CPU/GPU, and authoring/runtime data according to consumers and update frequency. Do not split cohesive fields that are always consumed together;
- choose AoS, SoA, AoSoA, sparse set, archetype chunk, indexed table, flat stream, or packed record from measured access. No layout is a universal DOD badge. Record the serial baseline and why rejected layouts lose for this workload;
- use stable generational IDs and immutable typed handles across storage and owner boundaries. Regenerated content republishes the newest immutable handle rather than adding content-version compatibility. Dense indices, pointers, iterators, spans, component references, and arena addresses never become durable identity;
- variable-length products use offsets/counts into bounded flat storage or another explicitly justified packed representation; hot records do not own vectors, strings, callbacks, mutexes, allocators, services, or heavyweight assets;
- process dense ranges/batches with explicit read/write columns and non-overlapping outputs. Structural mutation occurs at an owner commit through deterministic command buffers; queries/views are transient and cannot escape their epoch;
- prefer task-local or range-local outputs followed by stable-key compaction, bucketing, reduction, or deterministic merge. Completion order never defines IDs, packet rows, draw order, upload order, or serialized output;
- persistent renderer/GPU data is updated by stable slot and dirty range/page. Removing avoidable rebuild/copy/upload work precedes parallelizing what remains;
- validate with relevant bytes read/written, allocations, working-set/packet size, cache misses, bandwidth, branch behavior, iteration/extraction/apply time, dirty/upload bytes, and serial/parallel crossover. Keep only metrics that test the chosen access hypothesis;
- every material DOD choice cites an exact applicable source from J: Richard Fabian for data/access methodology, Epic MassEntity for data-only/query/deferred-structure practice, Epic game/render proxies for ownership separation, and NVIDIA/AMD renderer sources for persistent/indexed/access-specific renderer data. Do not attribute a general ECS to NVIDIA or AMD: the reviewed sources do not provide one. If no source and no measured Sparkle product need supports a new abstraction, do not add it.

The completion report must contain a **DOD reconciliation**: replaced object/pointer/full-copy path; authoritative and derived stores; data/access inventory; selected and rejected layouts; stable identity; mutation/publication phases; deterministic partition/merge; source trace; and measurements. “Cache friendly,” “ECS,” and “SoA” without this evidence are rejected claims.

### 14. Cohesive Files and Bounded Implementation Bodies

Every prompt must prevent touched implementation units, classes, and functions from accumulating unrelated reasons to change. SOLID is applied as an ownership/readability constraint, not as permission for interface ceremony:

- before adding behavior to a touched file or primary type, list its existing responsibilities and the new reason to change; if lifecycle/orchestration, data transformation, scheduling mechanism, backend policy, persistence, or diagnostics can evolve independently, give each a named owning boundary;
- a file is suspect when a reviewer cannot summarize its single responsibility without “and,” when its private types belong to several lifetimes, or when changes from several roadmap prompts repeatedly converge on it. Line count is a warning signal, not the decision by itself;
- split by cohesive behavior and shared invariant: facade, policy/orchestration, worker mechanism, per-execution accounting, backend implementation, serialization, and instrumentation are legitimate boundaries. Do not split chronological halves or create `Part1`, `Helpers`, `Common`, or one-method wrapper files;
- keep short operations with their owner. Do not turn every class into an interface, add dependency injection without a real substitution seam, or hide straightforward data flow behind factories solely to claim SOLID compliance;
- headers expose the minimum collaboration contract between implementation units. Private decomposition must not leak worker/queue/execution records into public contracts or create a second abstraction family;
- when a touched file is already a responsibility accumulator, the current prompt must split or reduce it before adding more behavior. A named later cleanup is acceptable only when the split would cross an unstarted prerequisite, and no further responsibility may be added meanwhile;
- the completion gate prints touched file sizes and responsibility summaries, then explains every retained large file in terms of one cohesive invariant. The goal is bounded reasoning cost as the repository grows, not a universal numeric line limit.

The completion report must include a **cohesion reconciliation**: responsibilities before/after, extracted owners, retained co-location, dependency direction, and why the result avoids both a god file and ceremonial fragmentation.

### 15. Narrow Capability APIs and Integration-Surface Containment

Multithreading is an implementation capability of the owning subsystem, not a second purpose imposed on every consumer. Every integration must preserve the receiving system's dominant intent:

- receiving orchestrators call a narrow domain capability such as start/submit/cancel/consume; they do not construct task graphs, choose lanes/worker counts, maintain prerequisite counters, bridge stop tokens, or implement deterministic fan-in unless scheduling is their explicit subsystem purpose;
- the subsystem that proves independence owns task partitioning, task-local state, lane policy, cancellation translation, result slots, merge order, and shutdown. Consumers provide domain inputs and consume domain results;
- public/cross-module headers expose stable requests, immutable results, handles/tokens, and explicit lifetime operations only. `TaskExecutionContext`, executor/queue/worker records, native handles, compiler sessions, COM setup, and transaction backups stay behind private implementation boundaries;
- use `std::stop_token` or a domain cancellation contract at a synchronous leaf seam; do not force an otherwise task-agnostic decoder, cooker, compiler, renderer, editor model, or RHI facade to include SparkleTasks types;
- callback/progress hooks carry owned domain values and document delivery affinity. A receiving UI must not become the owner of I/O, process, task, or cancellation mechanics merely to display progress;
- if integrating a feature adds more scheduling/lifetime code to a receiver than domain-policy code, stop and extract or strengthen the owning service API before acceptance. Moving the same leak into a helper lambda does not pass;
- private facades/PImpls are justified when they prevent volatile task/native mechanisms from entering stable headers. Wrapper-only layers with no invariant, policy, ownership, or substitution seam remain forbidden;
- audit transitive includes and links, not only symbol visibility. A private mechanism is still leaked when consumers must include its types, mirror its state machine, or link a dependency solely to satisfy an implementation detail.

The completion report must contain an **integration-surface reconciliation**: receiver purpose before/after, exact capability hook, mechanism types hidden, domain values exposed, cancellation/progress affinity, transitive include/link changes, and an explanation of why adding another operation does not require copying concurrency boilerplate.

### 16. Explicit Implementation Ownership and Declaration-Only Headers

Owned C++ source must make behavior ownership visible without anonymous linkage scopes or executable header clutter:

- anonymous/unnamed namespaces are forbidden in owned source. Do not satisfy this rule by renaming one to `Detail`, `Internal`, `Private`, `Local`, `Implementation`, or another arbitrary namespace;
- behavior that supports one owning type belongs to that type as a private member/static member or to a cohesive implementation capability type. Free behavior may live in an already established named domain namespace only when that namespace is the real API/operation owner;
- a file-local implementation type must name the capability, state, policy, encoding, formatting, or operation it owns. It must not become a generic bag of unrelated helpers or a second facade;
- headers are declaration and collaboration surfaces. Only templates and trivial field getters, setters, and direct accessors may have bodies in headers. Constructors, destructors, algorithms, transforms, parsers, formatters, orchestration, validation, factories, and non-template operators belong in the matching `.cpp`; deleted special-member declarations remain declarations of the type contract;
- never declare a class or struct inside a function body. Move lifecycle guards, records, visitors, callback state, and policies to the owning type or the narrowest cohesive implementation type. A short lambda is not a substitute when it acquires named state or multi-step policy;
- keep validation and diagnostics at the owning invariant: one actionable failure path or assertion is preferred over repeated logging, mirrored state, per-item traces, or permanent prompt-specific harnesses;
- before acceptance, scan all owned headers and touched sources for header bodies, function-local types, anonymous namespaces, invented namespace aliases, and diagnostic expansion. The anonymous-namespace scan is repository-wide and must be zero.

The completion report must include an **implementation-shape reconciliation**: header bodies moved, local types reassigned, former anonymous behavior owners, retained template/accessor bodies, diagnostics removed/retained, and the zero-result searches used by the gate.

### 17. Principal Graphics Engineering Traceability

Every prompt is reviewed against the canonical `PGE-01` through `PGE-15` requirements in A and the persona interpretation in H:

- identify applicable role requirements before editing and mark each **advance**, **preserve**, **not applicable**, or **blocked**;
- name the concrete code, test, capture, math/reference result, adoption artifact, or communication evidence expected from the prompt;
- preserve path tracing, neural-foundation, AI/ML, CPU/GPU architecture, driver, partner-adoption and communication boundaries even when a prompt does not advance them;
- do not add empty tensor/model types, AI managers, mock neural workloads, generic inference/training frameworks, speculative future-hardware APIs, permanent research graphs, or role-keyword scaffolding;
- material rendering/neural algorithm changes include coordinate/operator math, numerical assumptions, reference values and a performance model checked against measurements;
- AI-assisted code, shader, model, test, math, citation and design output is untrusted until independently inspected and validated under the same repository gate;
- hardware/driver conclusions record exact adapter, driver, OS, backend, compiler, capabilities and workload. A suspected driver defect requires a reduced reproducer and application-versus-driver analysis;
- feature boundaries are reviewed as partner adoption surfaces: prerequisites, owned inputs/outputs, failure/fallback, packaging, debugging and handoff must be narrow and explicit;
- training/offline preparation and runtime inference remain separate owners/workloads/packages. Runtime consumes deterministic validated immutable artifacts;
- quality and performance are reported together for path-traced/neural work, including a classical reference/fallback, visual failures, CPU/GPU latency, memory and frame pacing;
- a claimed new technique begins as a bounded hypothesis with a baseline and deletion condition; it enters product code only after correctness, quality, performance and ownership evidence wins;
- demo, whitepaper and conference-style artifacts are produced only by the designated closure prompts after the implementation passes; routine prompts do not grow documentation/reporting products;
- repository evidence never implies a degree, years of employment, proprietary experience, Linux support or driver-development history that was not actually earned and validated.

Prompts 00-29 establish the systems foundation and must preserve the role contract. Prompts 30-34 are the additive principal graphics engineering evidence sequence. Completing Prompt 29 alone does not close the `PGE-*` matrix.

The completion report must contain an **PGE reconciliation**: applicable IDs, advance/preserve/not-applicable/blocked status, exact evidence, honest gaps and proof that no role-only scaffolding was added.

## Required Prompt Completion Report

Every completed prompt returns this report:

1. **Outcome:** what now works; do not imply later stages.
2. **Repository audit:** counterpart searches and decisions.
3. **Ownership change:** owners, readers/writers, publication and lifetime edges.
4. **Files changed:** grouped by responsibility.
5. **Refactors:** nearby debt/duplication removed.
6. **Preservation ledger:** features/backends/workflows verified.
7. **Deletion ledger:** old paths deleted; remaining dated items.
8. **Validation:** commands and results, including serial/worker/backend modes.
9. **Performance evidence:** only metrics relevant to this prompt.
10. **Learning teach-back:** invariant, one rejected alternative, one failure test, one cost.
11. **Legacy-concurrency reconciliation:** LC IDs touched, retained-invariant proof, mechanisms deleted, newly discovered hits, and named deferrals.
12. **Hazard closure:** applicable MT IDs, source-backed pattern, falsifying evidence, and unresolved risk.
13. **Naming reconciliation:** canonical terms and rejected aliases searched, use/rename decisions, profiler/thread-label updates, and proof that no permanent synonym remains.
14. **Structure reconciliation:** module/folder/public-private ownership, filename-to-primary-type alignment, bounded moves/renames, updated includes/CMake/docs, and deliberate co-location decisions.
15. **DOD reconciliation:** data/access inventory, authority and derived projections, chosen/rejected layouts, stable identity, phase/lifetime, deterministic transforms, exact source trace, and measured evidence.
16. **Gate:** PASS or BLOCKED with concrete reason.
17. **Integration-surface reconciliation:** narrow hooks, hidden mechanism types, receiver responsibility before/after, dependency leakage removed, and scaling path for the next consumer.
18. **Implementation-shape reconciliation:** declaration-only header audit, function-local-type audit, anonymous-namespace ownership conversion, diagnostic reduction, and zero-result gate searches.
19. **PGE reconciliation:** applicable role IDs, status, concrete evidence, preserved contracts, exposed gaps, AI-assisted-work verification where used, and role-only scaffolding audit.

## Prompt Sequence and Dependencies

| Prompt | Deliverable | Depends on |
|---:|---|---|
| 00 | baseline, vocabulary, ownership and preservation inventory | current repository |
| 01 | serial task graph and task-execution contracts | 00 |
| 02 | fixed worker executor, work stealing, sleep/wake | 01 |
| 03 | dependencies, scopes, cancellation, lanes, shutdown, instrumentation | 02 |
| 03R | completed-work readability and physical-structure reconciliation | 03 |
| 04 | shader/texture/tool SparkleTasks pilots | 03R |
| 05 | ECS `EntityId`, registry, sparse-set component storage | 03R |
| 06 | ECS queries, structural epochs, deterministic entity commands | 05 |
| 07 | current scene data converted to ECS components/facades | 06 |
| 08 | explicit transform/camera derived-data system and change journal | 07 |
| 09 | transactional asynchronous level/scene loading | 06, 08 |
| 10 | ECS-aware system graph and parallel animation/morph/skinning | 08 |
| 11 | immutable editor scene model, commands, transactions, operations | 09, 10 |
| 12 | stable render IDs, immutable packet/delta contract, headless replay | 08, 10 |
| 12D | prove the Two Data Streams DOD extraction/render layout | 12 |
| 13 | dedicated render coordinator and bounded `RenderFrameQueue` | 12D |
| 14 | editor UI/viewport/capture packet conversion | 11, 13 |
| 15 | persistent render/GPU scene and dirty-range updates | 13 |
| 16 | generation-based shader/asset residency and deferred retirement | 04, 15 |
| 17 | renderer preparation task DAG | 10, 15 |
| 18 | D3D12 worker recording contexts and transient allocation | 17 |
| 19 | Vulkan worker recording contexts and transient allocation | 17 |
| 20 | frame-graph recording plan and first parallel passes | 18, 19 |
| 21 | advanced-feature preservation closure | 20 |
| 22 | tools/editor/reliability/package closure and legacy deletion | 14, 16, 21 |
| 23 | initial full-system performance characterization and tuning | 22 |
| 24 | atomic publication and wait-protocol correctness closure | 23 |
| 25 | conservative worker-budget and oversubscription policy | 23 |
| 26 | deterministic fan-in closure and evidence gate for parallel algorithms | 22, 23 |
| 27 | staged I/O and cold-cache PSO/resource-creation hitch control | 16, 22, 23, 25 |
| 28 | GPU queue concurrency, frame pacing, and correlated latency | 27 |
| 29 | production forensics, expert defense, and final portfolio release | 24, 26, 28 |
| 30 | path-tracing, mathematics, performance-model and partner-workload baseline | 29 |
| 31 | neural feature selection, data/model provenance, training/export and artifact contract | 30 |
| 32 | renderer-owned neural inference integration with classical fallback | 31 |
| 33 | neural model/kernel/system tuning and hardware/driver investigation | 32 |
| 34 | partner handoff, live demo, whitepaper/talk artifact and final role closure | 33 |

## J-to-K Scope Traceability

This ledger is a completeness check, not a substitute for reading J. A prompt may refine several concerns, but every major design area in J has a named implementation home and a final proof point.

| J design/learning area | Primary implementation prompts | Closure evidence |
|---|---|---|
| ownership, memory ordering, publication, lifetime, failure vocabulary | 00-03, 22, 24 | ownership ledger, targeted publication/wait stress, documented production invariants |
| engine-owned job system and structured concurrency | 01-04, 22, 24-25 | serial executor equivalence, 0/1/2/N stress, shutdown/cancellation tests, bounded worker budget |
| ECS/DOD foundations without framework sprawl | 05-08 | storage/query tests, deterministic command playback, scene conversion and change journal |
| game-system graph and useful simulation parallelism | 08-10 | dependency validation, serial/parallel equivalence, animation/skinning measurements |
| transactional loading and asset work | 04, 09, 16, 22, 27 | deterministic outputs, generation/cancellation/cold-cache tests, no partial publication |
| editor ownership, undo/redo, operations, UI affinity | 09-11, 14, 22 | immutable models, semantic commands, lifecycle and cancellation stress |
| immutable game/render boundary and bounded pipelining | 12-14, 24, 28 | packet replay, `RenderFrameQueue` publication/reuse tests, input-to-present and backpressure evidence |
| persistent render/GPU scene and lifetime-safe residency | 15-16 | dirty-range updates, generation retirement, delayed-GPU stress |
| renderer preparation task graph | 17 | serial/parallel equivalence, critical-path and granularity evidence |
| view visibility/LOD, light classification, shadow planning/caster lists, retained/dynamic draw preparation | 15, 17 | stable visible/light/caster/draw sets, cache invalidation, transparent order, large-scene critical path |
| D3D12 and Vulkan native recording ownership | 18-20 | native validation, token/reset misuse tests, worker migration stress |
| frame-graph authority, barriers, recording groups, submission | 20 | compiled plan inspection, serial/parallel execution of the same plan |
| command preparation versus native recording versus optional software translation versus aggregation/submission batching | 17-20, 28 | representation/owner ledger, direct-path parity, list/group/batch metrics, explicit RHI-thread ADR rejection |
| advanced graphics and vendor-neutral feature preservation | 16, 21-23, 27-29 | backend/feature matrix, image correctness, temporal/history, queue and capture tests |
| principal path-tracing/math/partner evidence | 30, 34 | derivation/reference tests, paired backend quality/performance, integration case and reproducible handoff |
| real neural graphics, training/export and inference | 31-34 | real artifact/model path, classical fallback, deterministic export, quality/performance frontier and final demo |
| hardware/driver diagnosis and current/future architecture reasoning | 23, 27-30, 33-34 | exact configuration, counters/captures, native validation, reduced reproducer, capability/fallback and scoped conclusions |
| AI fundamentals and independently verified AI-assisted engineering | 31-34 | provenance/model card, train/validation boundary, optimization decisions and code/math/source verification |
| principal communication, organization and prioritization | 29-30, 34 | adoption guide, priority/deletion ledger, incident/review record, live demo, whitepaper-quality note and talk outline |
| tools, packages, public-surface reduction, legacy deletion | 04, 22 | repository audit-to-zero, reproducible products/packages, closed deletion ledgers |
| deterministic task-local fan-in and evidence-gated parallel algorithms | 06, 10, 15, 22, 26 | stable keys/order, randomized completion, serial oracle only for a measured retained optimization |
| worker budget, OS scheduling and oversubscription | 03, 23, 25 | representative worker-count sweep, explicit override, third-party capacity inventory, conservative default |
| staged I/O, PSO/resource creation and first-run hitch policy | 04, 09, 16, 27 | bounded stage stress, cold/warm cache evidence, memory/late/miss/fallback metrics |
| shader workers, graphics/compute/RT pipeline creation, buffer/image/view creation, allocation/binding and descriptor writes | 16, 18-19, 27 | separate stage ownership, native safety audit, key deduplication, cold-cache memory/concurrency matrix |
| GPU queue overlap, presentation pacing and latency | 13, 20, 28 | correlated frame markers, graphics/compute/copy timelines, pacing and CPU-lead results |
| production concurrency diagnosis and interview defense | 22, 24-29 | injected incidents, exact tool evidence, regressions, coding/whiteboard/trace defense |
| reliability, determinism, performance, and portfolio teaching proof | every prompt, finalized by 22-29 | stress matrix, reproducible measurements, limitations, independent teach-back |
| industry-grounded professional failure prevention | every prompt, repository closure in 22 and 29 | MT-01–MT-44 pre-mortem, recognizable source-backed pattern, falsifying evidence, final non-applicability/closure review |
| industry-grounded canonical vocabulary | 00-03, 12-13, 18-20; enforced by every prompt | naming crosswalk, no CPU-task/GPU-command ambiguity, rejected-alias audit, truthful thread/profiler labels |

If a later repository discovery exposes a J responsibility with no row or prompt, update this ledger and the smallest owning prompt before implementing it. Do not hide the new obligation in a completion report.

## Renderer/RHI Use-Case-to-Prompt Coverage

This is the execution ledger for J's completeness audit. “Covered” means the named prompt must either retain a useful implementation with proof or record a source-backed non-applicability/defer decision. A general task runtime, one parallel pass, or one PSO future does not close the row.

| Renderer/RHI use case | Prompt owner(s) | Mandatory closure evidence |
|---|---:|---|
| render-proxy delta apply and persistent GPU scene | 12, 15 | stable ID/generation, dirty-only update, serial replay |
| transforms, bounds and previous-frame data | 08, 10, 17, 26 | exclusive ranges, reduction oracle, temporal parity |
| per-view visibility, relevance and LOD | 17 | stable visible set per view, 0/1/N parity, large/small crossover |
| light visibility/classification and compact light data | 17 | stable light IDs/order and reservoir/reference-lighting parity |
| shadow views/frusta, light-scene intersection and caster lists | 17 | per-light/view ownership, stable caster order, large-light proof when path exists |
| skinning and morph preparation | 10, 17 | world-to-render dependency, range ownership, image/pose parity |
| pass/material/pipeline eligibility | 17 | immutable lookup tables, no lazy runtime/cache mutation |
| retained/static and dynamic draw preparation | 15, 17 | current `MeshDraw`/`MeshInstanceBatch` reuse decision, invalidation and lifetime tests |
| sorting, state bucketing, instancing and draw merging | 17, 26 | stable key/tie-break, transparent order, measured batch improvement |
| per-pass/view constants and descriptors | 17-20 | preassigned/lease-local ranges, overflow/retirement tests |
| GPU-scene dirty compaction and upload plan | 15, 26 | scan/list-merge oracle, bounded dirty ranges, token retirement |
| BLAS inputs and build/update/refit/compaction decision | 16, 17, 21, 27 | asset/generation ownership, scratch/result lifetime, backend validation |
| classic TLAS and PTLAS planning | 17, 21 | distinct initial/update/add/remove/reload matrices |
| frame-graph setup and compile work | 17, 20, 23 | serial owner baseline; only measured private producers/algorithms retained |
| shader cook/compile workers | 04, 22, 27 | bounded processes/tasks, deterministic packages, third-party-thread budget |
| shader package/reflection/layout generation | 16, 27 | immutable generation publish, stale rejection, failure fallback |
| graphics/compute PSO precache/create | 27 | key discovery/dedup, cold-cache hit/miss/too-late, memory and fallback |
| RT pipeline/library/collection creation | 27 | backend capability decision; retain only useful measured path |
| buffer/image/view creation | 16, 27 | native thread-safety audit, exclusive output, owner commit |
| memory allocation/suballocation/binding | 15, 16, 27 | lifetime domain, contention/memory budget, native validation |
| persistent and transient descriptor allocation/update | 18, 19, 27 | lifetime split, no recording hot-lock, update visibility contract |
| decode/decompress/transcode/upload preparation | 04, 09, 16, 27 | staged states, no blocked frame worker, generation readiness |
| D3D12 allocator/list and Vulkan pool/buffer leases | 18, 19 | exclusive use, token-gated reset, worker migration stress |
| pass-level native command recording | 20 | same compiled plan/order/states, both native validation paths |
| intra-pass draw/dispatch/RT-input recording | future renderer program | defer unless Prompt 23 or later workload evidence identifies one pass as a material CPU-recording critical path; require a separately approved measured change |
| software RHI command stream and translation thread | 20, 29 | explicit know/defer ADR; implementation only after J's profiling/platform gate |
| recording-group aggregation and native submit batching | 20, 28 | order-key fan-in, list/group/batch counters, GPU starvation/latency tradeoff |
| barriers, preambles/postambles and queue ownership | 20, 28 | compiler authority, coordinator submit, state/wait equivalence |
| deferred resource/descriptor destruction | 15, 16, 22 | last-use token, delayed-GPU and reload/resize/shutdown tests |
| readback, screenshot, capture encode/write | 14, 21, 22 | staged GPU/CPU ownership, provider affinity, cancel/shutdown proof |
| validation callbacks, device loss and crash evidence | 19, 22, 29 | bounded callback intake, owner processing, injected failure narrative |
| offline distributed static-light build | 29 | know/defer record unless a real Sparkle static-light product is separately approved |
| GPU async queues and GPU-driven command generation | 28, future renderer program | explicitly not claimed as CPU multithreading; measured GPU proof only |

At Prompt 29, every row must point to tests/captures/code, a retained non-applicability decision, or an explicitly separately approved future program. “The job system could do this” is not closure.

## Failure-Atlas-to-Prompt Traceability

This is the minimum hazard pre-mortem scope. A prompt must add any other MT ID made applicable by the actual files it touches.

| Prompt(s) | Minimum J hazard IDs | Required recognizable proof family |
|---:|---|---|
| 00 | MT-01–12, MT-16–18, MT-23–27, MT-31–44 | repository ownership/blocking census, before-state failure reproductions, thread/idle/lock/allocator/native call-chain inventory |
| 01–03 | MT-03–06, MT-10, MT-12–25, MT-41–44 | serial DAG oracle, exactly-once settlement, predicate sleep/wake, nested-child exhaustion, no busy wait, grain/topology/false-sharing stress |
| 03R | MT-05, MT-10, MT-13–17, MT-23, MT-41–44 | ownership-to-file map, no hidden local lifecycle types, preserved serial/parallel contract, dependency-boundary scan, old-path/alias audit |
| 04 | MT-05–10, MT-16–19, MT-23–24, MT-27, MT-29, MT-31, MT-41–43 | scoped process/I/O lifetime, Qt affinity, bounded memory/concurrency, deterministic transactional fan-in, close/cancel/failure stress |
| 05–08 | MT-01–04, MT-11, MT-19–21, MT-25–31, MT-41–43 | stable generational identity, frozen structural epochs, exclusive query ranges, deterministic command playback, DOD/cache evidence |
| 09–11 | MT-01–10, MT-16, MT-23–24, MT-27, MT-29–31, MT-41–43 | isolated generation build/commit, immutable editor model, owner commands, undo/redo and document-close cancellation stress |
| 12–14 | MT-01–12, MT-23, MT-27, MT-29, MT-31, MT-33–40, MT-41–43 | packet replay, bounded `RenderFrameQueue`, owner affinity, delayed consumer, copied UI data, sequenced commands, latency/backpressure evidence |
| 12D | MT-01–04, MT-11, MT-19–21, MT-23, MT-25–31, MT-41–43 | data/access inventory, object/full-copy deletion, concrete stream layouts, stable-key extraction, dirty-range hypothesis, cache/bandwidth/bytes evidence |
| 15–16 | MT-01–10, MT-23, MT-26–31, MT-33, MT-36–39, MT-41–43 | persistent proxies, generation readiness, dirty ranges, delayed-GPU retirement, no ordinary idle, failure retains prior generation |
| 17 | MT-02–06, MT-09, MT-13, MT-19–31, MT-36, MT-39, MT-41–43 | immutable task inputs, task-private ranges, critical-path DAG, skew/grain tests, deterministic merge, prewarmed runtime |
| 18–20 | MT-09–10, MT-13, MT-19–21, MT-25–27, MT-29, MT-32–41, MT-43 | exclusive backend leases, token reset, native validation, same compiled order, group-size/list/submit/memory sweep, no worker submit/wait |
| 21 | MT-01–13, MT-19–21, MT-27, MT-29, MT-32–43 | full advanced-feature/backend parity matrix, deterministic identity/order, delayed-GPU lifetime, provider affinity, reload/resize/capture stress |
| 22 | MT-01–44 | repository audit-to-zero, closure of every applicable falsification test, no duplicate/legacy mechanism |
| 23 | MT-17–28, MT-35, MT-38, MT-41–44 | representative full-system characterization, crossover/negative scaling, profiler-overhead control |
| 24 | MT-01–15, MT-23, MT-25–27, MT-29, MT-41–44 | targeted publication/wait/reuse state proof, lost-wake/cancellation/shutdown stress, no speculative lock-free path |
| 25 | MT-17–19, MT-22, MT-24–28, MT-41–44 | conservative total worker budget, explicit override, third-party oversubscription check, evidence-gated contention diagnosis |
| 26–27 | MT-04–07, MT-13, MT-16, MT-19–31, MT-33, MT-36–37, MT-41–43 | deterministic fan-in verification, bounded staged I/O, cold-cache PSO/resource state machine, generation rejection |
| 28 | MT-17–18, MT-24–27, MT-32–43 | correlated CPU/task/queue/present identity, queue overlap proof, provider ownership, pacing/backpressure and input-latency matrix |
| 29 | MT-01–44 | injected incidents, independent reproduction, final zero-unclassified audit, live design/code/trace defense |
| 30 | MT-23, MT-28–29, MT-32–43 | deterministic path-tracing workload, math/reference oracle, paired API capture, exact hardware/driver baseline, partner reproduction |
| 31 | MT-01–07, MT-23–24, MT-27–31, MT-41–43 | bounded deterministic training/export, provenance, immutable artifact, no runtime training dependency, classical/reference oracle |
| 32 | MT-01–13, MT-19–21, MT-23, MT-25–40, MT-41–43 | immutable model lifetime, persistent GPU data, declared inference passes, offline/runtime parity, classical fallback, backend validation |
| 33 | MT-17–29, MT-32–44 | quality/performance ablation, topology/counter evidence, bounded memory, frame pacing, reduced driver repro, losing-path deletion |
| 34 | MT-01–44 | clean reproduction, adversarial adoption review, final role/deletion audit, live failure/fallback demonstration, evidence-backed communication |

The range notation is inclusive. It does not mean every hazard needs a bespoke test in every prompt; reuse the smallest existing proof that actually crosses the changed ownership path. It does mean no applicable hazard may be omitted because the happy path passed.

## Prompt 00 — Establish the Before-State and Invariants

Target CL Title: `Sparkle: Establish Concurrency Before-State and Invariants`

~~~text
Implement Prompt 00 from K_MultithreadedEngineImplementationPromptSeries.md.

Objective:
Create the verified before-state and invariant vocabulary required for every later multithreading change. This prompt changes no ownership architecture and introduces no new task/ECS/render framework.

Non-negotiable repository rules:
- Apply `Docs/Architecture/00-Review/L_SparkleEngineIntegrationStyleGuide.md` in full. Treat its ownership, implementation-shape, DOD, concurrency, rendering, naming, validation, structure, principal graphics engineering, and completion-report gates as acceptance criteria. Before editing, list the applicable `PGE-01` through `PGE-15` requirements from A, interpret them through H, and classify each as advance, preserve, not applicable, or blocked with expected evidence. Inspect the current repository, reconcile the whole touched ownership path, verify any AI-assisted work independently, delete replaced paths in the same change, reject role-only scaffolding, and report any justified exception explicitly with its owner, scope, evidence, and deletion or review gate.
- Apply Rule 13: audit every data source, transform, stream, packet, cache, table, upload, and hot traversal touched by this prompt; record the concrete access inventory, authoritative/derived ownership, layout decision, stable identity, deterministic transform, exact source precedent, and measured falsifier.
- Apply Rule 12: audit touched/new files for module and folder ownership, public/private placement, filename-to-primary-type alignment, and nearby misplaced counterparts; complete bounded moves/renames with includes, CMake, and documentation updated before the gate.
- Enforce J's canonical concurrency/rendering vocabulary and Rule 10: search canonical terms plus rejected aliases in the touched scope, use one responsibility per name, and delete temporary compatibility spellings before the gate.
- Search before adding; use/extend/replace existing timing, validation, launch-setting, and test facilities.
- Apply the daily-refactor rule to touched baseline/launch code and remove only directly exposed duplication/dead code.
- Do not add a profiler framework, task panel, runtime log stream, default JSON/CSV report, public diagnostics snapshot, or permanent feature-registry system.
- Preserve all product rendering paths and both backends.

Inspect first:
- RuntimeApplication and EditorApplication frame order.
- GameScene update/snapshot and level lifecycle.
- Renderer/RendererSystemRoot/FramePipeline stages.
- frame-graph compile/execute/submission.
- RHI frame resources, queue timelines, WaitForIdle sites, allocator/profiler hooks.
- current benchmark/test launch mechanisms and environment/config controls.
- current names, filenames, profiler labels, and thread labels for task/job/work, execution/run, graph/node, context/handle/token/lease, main/game/editor/render/RHI threads, queues, commands, frame publication, and GPU submission; classify semantic collisions rather than matching text alone.
- J's binding LC-01 through LC-18 legacy-concurrency ledger, then every owned-source hit for standard/Qt/native threading, locks, atomics, waits, detach, task-runtime, and device-idle mechanisms. Exclude generated/external trees from ownership, but record wrapped third-party worker behavior.
- A, E, G, H, and J governing documents.

Required implementation:
1. Adopt J's canonical naming crosswalk for `FrameId`, `SceneGeneration`, `SequenceNumber`, `Task`, `TaskExecution`, thread roles, packet lifetime, recording ownership, and GPU completion without duplicating existing strong types. Produce a before-state rename ledger with **keep**, **rename in owning prompt**, or **delete as alias** for every conflicting term. Dormant future controls use canonical labels from day one.
2. Add owner-thread assertions to current renderer/RHI mutators where an existing assertion mechanism can be reused.
3. Add or consolidate profiler-visible scopes for game update, snapshot/extraction, renderer scene build, GPU-data build, graph setup/compile, record, submit, present, and idle waits using existing hooks only.
4. Add development/test-only launch controls for worker count, serial task mode, threaded renderer, parallel recording, and pipeline depth. Controls may be dormant until later prompts but must have one owner and no shipping UI.
5. Regenerate the legacy-concurrency ledger from the current tree. For every thread/future/mutex/atomic/condition variable/Qt or native wait/detach/device-idle hit, record file/call path, owner, users, invariant, blocking/affinity/lifetime policy, disposition, exact closing prompt, and falsifying test. New use of replacement-bound code is forbidden.
6. Classify every WaitForIdle call site—not only the wrapper API—as shutdown, device reinitialization, frame-slot/resource reuse, rare boundary, or data-path debt. Detect nested drains such as renderer → device services → swap chain and assign one owner/replacement.
7. Record lock-order and callback-under-lock findings. At minimum verify InputSystem callback dispatch, logger registry/sink, Streamline provider calls, D3D12/Vulkan queue synchronization, descriptor allocators, GPU allocation records, Vulkan diagnostics, and D3D12 linear allocator reset/publication.
8. Record reproducible baseline commands/cameras/workloads for tiny, Sponza, animation, RT/classic TLAS, PTLAS where supported, editor viewport, shader cook, and texture cook.
9. Record current feature-preservation status without claiming unsupported features.
10. Produce the initial MT-01–MT-44 hazard pre-mortem for the current architecture. Link every applicable hazard to a current source path, selected professional pattern, owning future prompt, and falsifying test/trace. Record non-applicable only with a concrete architectural reason.
11. Reserve truthful debugger/profiler role labels: `Sparkle.GameThread`, `Sparkle.EditorThread`, `Sparkle.RenderThread`, `Sparkle.Task.FrameCritical.N`, `Sparkle.Task.Background.N`, `Sparkle.Task.BlockingIo.N`, and `Sparkle.ToolMain`. Do not emit labels for roles that do not exist yet, especially `RHIThread`.

Validation:
- Build/run current D3D12 and Vulkan configurations with native validation as already supported.
- Capture the current CPU/GPU timeline through existing tools.
- Verify launch controls parse deterministically and do not alter default behavior.
- Run existing tests and representative current scenes.
- Exercise an InputSystem callback that unsubscribes itself under a watchdog or equivalent isolated harness; record the current failure/risk as before-state evidence without silently fixing it in this baseline-only prompt.
- Run git diff whitespace/include/dependency checks.

Acceptance gate:
- L Sections 19-21 (reconciliation, acceptance checklist, and required completion report) and Section 24 (Principal Graphics Engineering review gate) pass for the entire touched changelist, including directly affected neighboring ownership paths; no undocumented exception remains.
- Current serial frame flow and owners can be explained from evidence.
- Baseline reproduction commands and essential captures exist in the existing artifact workflow.
- Every owned concurrency primitive, WaitForIdle call site, detach, and direct lifecycle callback has a classification and exact target prompt; no category is summarized only at wrapper level.
- No new observation product or public diagnostic API was created.
- Preservation and deletion ledgers are complete.
- The naming ledger distinguishes CPU tasks, OS threads, render control commands, RHI command recording, GPU queues, and completion proofs; every planned rename has one exact owning prompt and no future control/profiler label uses a rejected alias.
- The hazard ledger names current exposures—including raw game/render lifetime, callback-under-lock, polling/blocking waits, naive worker policy risk, shared allocation/queue state, nested device-idle, and measurement gaps—without implying later fixes already exist.

Positive patterns: evidence before design, owner assertions, narrow vocabulary, current tool reuse.
Forbidden: implementing the job system, moving renderer threads, speculative metrics infrastructure, broad cleanup.
~~~

## Prompt 01 — Build the Serial Task Graph Contract

Target CL Title: `SparkleTasks: Build the Deterministic Serial Task Graph Contract`

~~~text
Implement Prompt 01 from K_MultithreadedEngineImplementationPromptSeries.md only after Prompt 00 passes.

Objective:
Create SparkleTasks' deterministic serial foundation: task identity, immutable compiled topology, execution generations, prerequisites, fan-in, nested completion semantics, explicit failure, and bounded graph storage. Do not create worker threads yet.

Non-negotiable repository rules:
- Apply `Docs/Architecture/00-Review/L_SparkleEngineIntegrationStyleGuide.md` in full. Treat its ownership, implementation-shape, DOD, concurrency, rendering, naming, validation, structure, principal graphics engineering, and completion-report gates as acceptance criteria. Before editing, list the applicable `PGE-01` through `PGE-15` requirements from A, interpret them through H, and classify each as advance, preserve, not applicable, or blocked with expected evidence. Inspect the current repository, reconcile the whole touched ownership path, verify any AI-assisted work independently, delete replaced paths in the same change, reject role-only scaffolding, and report any justified exception explicitly with its owner, scope, evidence, and deletion or review gate.
- Apply Rule 13: audit every data source, transform, stream, packet, cache, table, upload, and hot traversal touched by this prompt; record the concrete access inventory, authoritative/derived ownership, layout decision, stable identity, deterministic transform, exact source precedent, and measured falsifier.
- Apply Rule 12: audit touched/new files for module and folder ownership, public/private placement, filename-to-primary-type alignment, and nearby misplaced counterparts; complete bounded moves/renames with includes, CMake, and documentation updated before the gate.
- Enforce J's canonical concurrency/rendering vocabulary and Rule 10: search canonical terms plus rejected aliases in the touched scope, use one responsibility per name, and delete temporary compatibility spellings before the gate.
- Search for existing task, graph, event, handle/generation, arena/pool, result/error, and test-runner counterparts before adding types.
- Reuse or refactor coherent Core facilities; do not create parallel handle/result/assertion families.
- Leave touched CMake/Core/test code cleaner and delete temporary duplicate scaffolding.
- Keep Tasks independent of GameFramework, Renderer, RHI, Editor, Assets, Platform windows, and product policy.
- Pre-mortem MT-03–06, MT-10, MT-13–15, MT-23, MT-29, and MT-44. Model the familiar prerequisite/subsequent and nested-completion semantics described by Epic Tasks and structured execution; do not copy a sample pool's weaker lifetime contract.

Required implementation:
1. Add the Tasks module with the minimal public contracts from J: `TaskName`/`TaskDesc`, builder-local `TaskNodeHandle`, `TaskGraphBuilder`, immutable `CompiledTaskGraph`, `TaskExecution`/`TaskExecutionContext`, `TaskResult`, and serial `TaskExecutor`. Do not add `Job`, `WorkItem`, `Future`, `TaskRun`, or a public `TaskManager` alias.
2. Use generation-validated tokens/handles; no public raw TaskRecord pointers.
3. Compile and reject cycles, self/duplicate/foreign edges, stale tokens, capacity overflow, and invalid completion policies before execution starts.
4. Implement deterministic topological serial execution with prerequisites, WhenAll, continuation, and nested/group unfinished-count semantics.
5. Define failure propagation: first failure retained, normal dependents cancelled, explicit cleanup/finally nodes settle.
6. Bound per-execution task/edge storage; overflow returns controlled failure without partial execution.
7. Keep queue/record/topology implementation private and add no timing/reporting product surface.

Validation:
- Unit tests: single, chain, fan-out/fan-in, diamond, nested completion, failure, cleanup, reusable graph with separate contexts, stale handles, all invalid graph cases, configured overflow.
- Run every test repeatedly through the serial executor.
- Dependency-boundary test proves Tasks has no forbidden module include/link.
- Allocation/lifetime tests prove a run outlives all nodes/results it still needs and releases cleanly.
- Exhaustively enumerate small DAGs against a simple reference topological executor; inject failure/cancellation at each node and prove every accepted node/run has one terminal accounting path.

Acceptance gate:
- L Sections 19-21 (reconciliation, acceptance checklist, and required completion report) and Section 24 (Principal Graphics Engineering review gate) pass for the entire touched changelist, including directly affected neighboring ownership paths; no undocumented exception remains.
- Serial graph output is deterministic and all counters settle exactly once.
- Invalid graphs fail before a task body executes.
- No worker, wait, lane, profiler, ECS, or renderer integration exists yet.
- Public concepts have a stated future consumer and private internals remain private.

Positive patterns: serial reference first, immutable topology, generation handles, bounded storage, explicit result policy.
Forbidden: std::future as task identity, fire-and-forget lifetime, worker threads, busy waiting, unbounded heap fallback.
~~~

### Prompt 01 completion record — 2026-07-18

Status: **passed**. `SparkleTasks` now contains only the serial contract owned by this prompt. Its public surface is `TaskName`/`TaskDesc`, generation-validated `TaskNodeHandle`, `TaskGraphBuilder`, immutable `CompiledTaskGraph`, `TaskExecution`/`TaskExecutionContext`, `TaskResult`, and `TaskExecutor`; topology, ready ordering, runtime records, and counters are private.

A transient `SparkleTasks.SerialContract` harness ran the eight contract groups three times. It covered the requested basic shapes, nested group completion and nested failure, first-failure/cleanup policy, reusable topology with separate contexts, result/callable lifetime, every public invalid-input class, builder/executor overflow with no body execution, stale/foreign result queries, and exhaustive four-node directed-graph comparison with failure/cancellation injected at each accepted node. A transient dependency scan proved the module had no Application, GameFramework, Renderer, RHI, Editor, Assets, or Platform dependency and contained no worker/wait primitive or noncanonical alias. Both focused checks passed in `DevelopmentEditor`; no wider engine or scene build was used. In accordance with Rule 11, the harness, CTest target, boundary script, and dedicated test directory were deleted after the gate; these paragraphs retain evidence, not a maintained test subsystem.

## Prompt 02 — Add the Fixed Worker Executor

Target CL Title: `SparkleTasks: Add the Fixed Worker Executor`

~~~text
Implement Prompt 02 only after Prompt 01 passes.

Objective:
Execute the same compiled task contract on a fixed worker set with local ready queues, external injection, work stealing, sleep/wake, and safe repeated startup/shutdown. Preserve exact serial semantics.

Non-negotiable repository rules:
- Apply `Docs/Architecture/00-Review/L_SparkleEngineIntegrationStyleGuide.md` in full. Treat its ownership, implementation-shape, DOD, concurrency, rendering, naming, validation, structure, principal graphics engineering, and completion-report gates as acceptance criteria. Before editing, list the applicable `PGE-01` through `PGE-15` requirements from A, interpret them through H, and classify each as advance, preserve, not applicable, or blocked with expected evidence. Inspect the current repository, reconcile the whole touched ownership path, verify any AI-assisted work independently, delete replaced paths in the same change, reject role-only scaffolding, and report any justified exception explicitly with its owner, scope, evidence, and deletion or review gate.
- Apply Rule 13: audit every data source, transform, stream, packet, cache, table, upload, and hot traversal touched by this prompt; record the concrete access inventory, authoritative/derived ownership, layout decision, stable identity, deterministic transform, exact source precedent, and measured falsifier.
- Apply Rule 12: audit touched/new files for module and folder ownership, public/private placement, filename-to-primary-type alignment, and nearby misplaced counterparts; complete bounded moves/renames with includes, CMake, and documentation updated before the gate.
- Enforce J's canonical concurrency/rendering vocabulary and Rule 10: search canonical terms plus rejected aliases in the touched scope, use one responsibility per name, and delete temporary compatibility spellings before the gate.
- Search for repository thread wrappers, naming/priority helpers, semaphore/condition-variable utilities, allocators, and queue implementations before adding counterparts.
- One executor owns workers; do not add feature/subsystem threads or a second pool.
- Apply daily refactoring to touched thread lifecycle utilities and remove duplicate naming/join code.
- Queue implementation is private and may begin correctness-oriented; do not expose it in public APIs.
- Pre-mortem MT-13–25 and MT-41–44. A fixed pool, work stealing, and parking are recognizable mechanisms, but thread count, lock-free queues, padding, and priority require Sparkle evidence rather than résumé value.

Required implementation:
1. Create a configured fixed worker set; support worker counts 0/1/2/N, where 0 uses serial execution.
   The host-facing configuration type is `TaskExecutorConfig`; do not add `TaskSchedulerConfig`, a public `TaskScheduler`, or a second pool configuration family.
2. Give each worker a local double-ended ready queue and implement opposite-end stealing; provide a synchronized external injection path.
3. Use semaphore/condition-variable parking with a lost-wakeup-safe predicate/protocol; no idle spin loop.
4. Completion atomically resolves dependent prerequisite counts and schedules newly ready nodes exactly once.
5. Use allocation-conscious task records/run arenas and pad/separate hot counters where measurements/layout inspection justify it.
6. Name workers through existing platform/debugger mechanisms. Priority/affinity remain conservative and private.
7. Define executor states Accepting, Draining/Cancelling, Stopping, Stopped; reject late submission deterministically and join every worker.
8. Retain the serial executor as the reference mode using the same task bodies/contracts.

Validation:
- Run Prompt 01 tests at 0/1/2/N workers with randomized yields/delays.
- Stress millions of dependency transitions, external submissions racing parking, stealing, queue wrap/growth policy, repeated executor construction/destruction, and shutdown with idle/queued/running work.
- Verify no task executes twice and every accepted `TaskExecution` settles.
- Use ThreadSanitizer-capable configuration or the strongest available equivalent; report limitations.
- Measure enqueue, local, stolen, and wake latency without creating a report framework.
- Compare fewer-than-physical, physical, logical, and oversubscribed worker settings where the machine permits; trace runnable threads/context switches and use a deliberately skewed workload. Inspect adjacent worker hot fields for false sharing.

Acceptance gate:
- L Sections 19-21 (reconciliation, acceptance checklist, and required completion report) and Section 24 (Principal Graphics Engineering review gate) pass for the entire touched changelist, including directly affected neighboring ownership paths; no undocumented exception remains.
- Parallel results equal serial results for all deterministic tests.
- Idle workers park; no routine polling or worker blocking on child tasks.
- Repeated shutdown has no leak, stranded run, lost wakeup, or callback-after-destroy.
- Queue internals do not appear in public headers or editor diagnostics.

Positive patterns: fixed ownership, work-first scheduling, stealing, parked idle state, serial parity.
Forbidden: detached threads, one thread per system, busy-yield waits, silent exception swallowing, lock-free rewrite without reclamation proof.
~~~

### Prompt 02 completion record — 2026-07-18

Status: **passed**. Prompt 02 proved the unchanged serial oracle at 0 or one executor-owned fixed set at 1/2/N; Prompt 03 subsequently split the host configuration into explicit FrameCritical/Background/BlockingIo worker counts without adding another executor family. Private worker deques use owner-end consumption and opposite-end stealing; external work uses a synchronized injection queue; condition-variable parking uses a mutex-protected epoch and rescan protocol. Atomic prerequisite, nested unfinished, schedule, and terminal transitions settle each accepted node once. `MaximumActiveExecutions` bounds concurrent run storage. Workers use Core's existing thread-role naming and add no priority/affinity policy.

The private lifecycle is `Accepting → Draining|Cancelling → Stopping → Stopped`. Admission and run registration are atomic under the lifecycle mutex; drain and cancel reject late submissions, cancellation prevents queued normal bodies while preserving cleanup, workers stop only after active runs settle, all threads join, and repeated shutdown is idempotent. Same-executor recursive worker submission rejects instead of blocking. Submission remains a settled host boundary until Prompt 03 introduces scopes.

A disposable harness passed Prompt 01 parity at 0/1/2/8 workers, more than one million dependency transitions, 400 concurrent external submissions, real opposite-thread stealing, active-run overflow, parked wakeup, recursive-submit rejection, 40 repeated lifecycle cycles, running-work drain, queued-work cancellation, cleanup, and late rejection. DevelopmentEditor passed three consecutive full runs and DebugEditor passed once. Eight idle workers used 0 ms process CPU over 200 ms. On the 16-core/32-thread host, the skewed batch measured approximately 10.2–11.8 ms at one worker, 2.0–2.4 ms at 16, 2.3–2.4 ms at 32, and 2.3–2.8 ms at 33; parked enqueue-to-start measured 128–195 µs. MSVC/Windows offered no supported ThreadSanitizer mode, and ETW context-switch, cache-traffic, percentile, and third-party nested-pool evidence remain unavailable rather than inferred. The harness and target were deleted after the gate.

## Prompt 03 — Complete Structured Task-Runtime Semantics

Target CL Title: `SparkleTasks: Complete Structured Runtime Semantics`

~~~text
Implement Prompt 03 only after Prompt 02 passes.

Objective:
Complete the production SparkleTasks contract: `TaskScope` hierarchy, cooperative cancellation, `TaskEvent`, `ParallelFor`, FrameCritical/Background/BlockingIo lanes, host joins, failure/finally semantics, private profiler events, and ordered shutdown. This is the engine job system, but `Job` is not a parallel API vocabulary.

Non-negotiable repository rules:
- Apply `Docs/Architecture/00-Review/L_SparkleEngineIntegrationStyleGuide.md` in full. Treat its ownership, implementation-shape, DOD, concurrency, rendering, naming, validation, structure, principal graphics engineering, and completion-report gates as acceptance criteria. Before editing, list the applicable `PGE-01` through `PGE-15` requirements from A, interpret them through H, and classify each as advance, preserve, not applicable, or blocked with expected evidence. Inspect the current repository, reconcile the whole touched ownership path, verify any AI-assisted work independently, delete replaced paths in the same change, reject role-only scaffolding, and report any justified exception explicitly with its owner, scope, evidence, and deletion or review gate.
- Apply Rule 13: audit every data source, transform, stream, packet, cache, table, upload, and hot traversal touched by this prompt; record the concrete access inventory, authoritative/derived ownership, layout decision, stable identity, deterministic transform, exact source precedent, and measured falsifier.
- Apply Rule 12: audit touched/new files for module and folder ownership, public/private placement, filename-to-primary-type alignment, and nearby misplaced counterparts; complete bounded moves/renames with includes, CMake, and documentation updated before the gate.
- Enforce J's canonical concurrency/rendering vocabulary and Rule 10: search canonical terms plus rejected aliases in the touched scope, use one responsibility per name, and delete temporary compatibility spellings before the gate.
- Search for cancellation, events, scoped handles, process/file completion, profiler scopes, lane/thread configuration, and shutdown orchestration counterparts.
- Do not repurpose Core Event as a cross-thread callback bus; its current callbacks remain owner-thread-affine.
- One SparkleTasks executor family serves engine/tools with host policy; no new pools.
- Refactor direct touched futures/cancellation helpers only when their consumer migrates in this prompt or Prompt 04.
- Resolve LC-05 now: InputSystem must declare registration/dispatch affinity and must never invoke a subscriber while holding `m_CallbackMutex`. Do not paper over re-entry with a recursive mutex.
- Pre-mortem MT-05–24 and MT-41–44. Explicitly reject generic busy-wait/task-help loops: Epic documents spinning, unrelated-task latency, circular dependency, and recursive stack failure. Sparkle uses dependencies, nested completion, and predicate parking.

Required implementation:
1. Add parent/child TaskScope for Application, World/Document, AssetGeneration, Frame, and ToolInvocation lifetimes.
2. Cancellation flows downward; scope completion flows upward. Owner destruction before settlement asserts in development.
3. Add cooperative cancellation checkpoints and exactly-once result settlement. Cancellation callbacks must not run arbitrary user code synchronously on the requesting thread.
4. Add one-shot generation-safe TaskEvent for external completion; no worker polling.
5. Add ParallelFor with explicit grain/serial threshold and exclusive range contract.
6. Add FrameCritical, Background, and BlockingIo policies so file/process waits cannot occupy FrameCritical workers and Background CPU work cannot consume all interactive capacity.
7. Detect/prohibit normal worker `TaskExecution::Wait`. Allow bounded owner-thread joins only at declared phase boundaries.
8. Emit task name/lane/run/worker/timing/status into existing profiler hooks privately; no public snapshot/panel/log stream.
9. Implement the shutdown order defined in J and stress cancellation/failure racing start/completion.
10. Refactor InputSystem dispatch to snapshot eligible callbacks under a narrow lock and invoke after unlock, or prove and enforce complete owner-thread affinity and remove the unnecessary lock. Preserve immediate/deferred ordering, handle identity, focus/layer routing, and define whether unsubscribe during a dispatch affects the current or next snapshot.

Validation:
- Scope tree cancellation/settlement tests, owner-destroy negative tests, stale event/double-trigger tests, cancelled prerequisite/finally behavior, ParallelFor partition coverage/non-overlap, invalid cross-lane dependency checks.
- Starvation test: sustained background/I/O work while frame tasks meet an explicit latency budget.
- Shutdown with event-blocked, cancelled, failed, and nested work.
- Parent-on-every-worker child-spawn test, empty-queue CPU-use test, lost-wakeup notify-before/after-park test, and proof that BlockingIo/native wait stacks never appear on frame workers.
- Input callback self-unsubscribe, unsubscribe-other, subscribe-during-dispatch, nested dispatch, deferred dispatch, and slow callback tests; none runs user code under the registry lock.
- Profiler capture shows names/lanes/dependencies without changing shipping public surface.

Acceptance gate:
- L Sections 19-21 (reconciliation, acceptance checklist, and required completion report) and Section 24 (Principal Graphics Engineering review gate) pass for the entire touched changelist, including directly affected neighboring ownership paths; no undocumented exception remains.
- Every accepted `TaskExecution` settles exactly once under cancellation/failure/shutdown races.
- Raw owner capture beyond scope lifetime is prohibited by design/tests.
- Frame and blocking lanes cannot starve one another.
- No second executor/pool, task UI, public task diagnostics, or new runtime reporting system exists.
- LC-05 is closed, including a stated re-entrancy/snapshot semantic and no callback-under-lock.

Positive patterns: structured concurrency, cooperative cancellation, continuations, bounded host waits, private instrumentation.
Forbidden: detached work, arbitrary cancellation callbacks, worker waits, polling futures, priority used as correctness.
~~~

Implementation evidence (2026-07-18): **passed with one external-tool limitation**. SparkleTasks now exposes one structured runtime family: parent/child `TaskScope`, downward cancellation and upward settlement, owned asynchronous contexts, bounded owner-thread joins, generation-safe one-shot `TaskEvent`, explicit-grain `ParallelFor`, and `TaskLane::{FrameCritical, Background, BlockingIo}`. The executor owns isolated capacity for each configured lane while retaining the zero-worker serial oracle; a graph is rejected if it requests an unconfigured lane or makes FrameCritical correctness depend on Background/BlockingIo work. Same-executor workers cannot submit or wait. Normal nodes cancel through dependency state, cleanup nodes still settle, and shutdown cancellation wakes event-blocked work through a private stop notification rather than polling or arbitrary user callbacks.

Private Windows TraceLogging events provide `TaskBegin`, `TaskEnd`, and `TaskDependency` records containing name, lane, run generation, task/worker index, duration, outcome, and dependency edges. They have no public callback, snapshot, log, CVar, or editor surface and do not read the clock when the provider is disabled. This environment could compile/link the provider but could not start an ETW capture session because `logman` requires administrator access; no in-engine diagnostic bypass was added.

LC-05 is closed as owner-only. Input registration and dispatch assert the existing `OwnerThread`; the obsolete callback mutex is removed. Each dispatch invokes a copied eligible-callback snapshot. Unsubscribe/subscribe during a dispatch affects the next snapshot; a nested dispatch takes a new current-registry snapshot; nested deferred-phase processing is suppressed while newly queued deferred events remain ordered in that phase. User callbacks run under no registry lock. The follow-up readability audit moved lifecycle guards and runtime records out of function bodies so orchestration remains policy-focused as the subsystem grows.

Final Prompt 03 ownership after the 03R gate: public contracts remain in `Tasks/Public`; graph construction is in `Private/Graph`; execution results and the serial oracle are in `Private/Execution`; the executor facade and worker mechanism are separated in `Private/Scheduling`; scopes/events are in `Private/Lifetime`; and private ETW instrumentation is in `Private/Profiling`. `TaskExecutorInternal.h` and `SerialTaskExecutor.cpp` were renamed to match execution-state and serial-operation ownership. The exact file map and deletion ledger are recorded in Prompt 03R evidence below.

A disposable harness passed twenty repeated DevelopmentEditor cycles covering scope settlement, exclusive/non-overlapping ParallelFor coverage, continuations, stale/double TaskEvent signalling, cancellation wakeup, cleanup/finally execution, invalid policy/cross-lane graphs, sustained Background plus BlockingIo load with FrameCritical progress under 200 ms, ordered drain/cancel shutdown, and Input self-unsubscribe/unsubscribe-other/subscribe-during-dispatch/nested/deferred behavior. A focused DebugEditor death case confirmed that destroying an explicitly unsettled scope owner terminates at the development assertion after safe cancellation/join. MSVC on this Windows configuration provides no supported ThreadSanitizer mode. The transient source, target, and capture artifacts were deleted after validation.

## Prompt 03R — Reconcile Completed Runtime Structure and Readability

Target CL Title: `SparkleTasks: Reconcile Runtime Structure and Readability`

~~~text
Implement Prompt 03R only after Prompt 03 passes and before Prompt 04 or Prompt 05 begins.

Objective:
Adversarially reconcile the completed Prompt 00–03 work with K Rules 2, 10, 11, and 12. Make SparkleTasks and the Prompt 03 InputSystem changes read like one production architecture through module ownership, folder placement, filenames, primary type names, public/private boundaries, and short policy-focused orchestration. Preserve the proven task-runtime behavior. This is a remediation and structure gate, not a feature prompt.

Non-negotiable repository rules:
- Apply `Docs/Architecture/00-Review/L_SparkleEngineIntegrationStyleGuide.md` in full. Treat its ownership, implementation-shape, DOD, concurrency, rendering, naming, validation, structure, principal graphics engineering, and completion-report gates as acceptance criteria. Before editing, list the applicable `PGE-01` through `PGE-15` requirements from A, interpret them through H, and classify each as advance, preserve, not applicable, or blocked with expected evidence. Inspect the current repository, reconcile the whole touched ownership path, verify any AI-assisted work independently, delete replaced paths in the same change, reject role-only scaffolding, and report any justified exception explicitly with its owner, scope, evidence, and deletion or review gate.
- Apply Rule 13: audit every data source, transform, stream, packet, cache, table, upload, and hot traversal touched by this prompt; record the concrete access inventory, authoritative/derived ownership, layout decision, stable identity, deterministic transform, exact source precedent, and measured falsifier.
- Apply Rule 12: audit the complete `Engine/Tasks` subtree and the directly touched InputSystem registration/dispatch path, including files introduced or modified by Prompts 00–03. Completed work has no grandfather clause.
- Enforce Rule 11: product function bodies contain no local class/struct definitions; lifecycle, synchronization, runtime-record, visitor, and policy types live at the narrowest file-private or owning-type scope. Short local lambdas remain only for genuinely inline algorithms/callbacks.
- Enforce J's canonical vocabulary and Rule 10 across filenames, types, functions, namespaces, profiler/thread labels, comments, includes, CMake, and documentation. Delete compatibility filenames/includes and rejected aliases before the gate.
- Search the repository for existing module/folder conventions and semantic counterparts before moving or splitting anything. Do not impose a generic enterprise layout or create one-file ceremonial directories.
- Keep `SparkleTasks` dependent only on Core and platform system libraries. Application, GameFramework, Renderer, RHI, Editor, Assets, Platform windows/input, and product policy remain outside the module.
- Preserve one executor family, the serial oracle, graph semantics, scope/event/cancellation behavior, lane isolation, shutdown, InputSystem routing/order, and private-only instrumentation.
- Do not introduce a new task feature, public diagnostic, settings surface, abstraction family, compatibility facade, permanent test target, or broad unrelated cleanup.

Required implementation:
1. Print and review the current `Engine/Tasks` tree and the touched InputSystem files. Produce a before/target responsibility map for every source/header and every significant class, struct, free function, and PImpl: owning module; subsystem responsibility; public/private status; primary type/operation; direct consumers; retain/move/rename/split/merge decision.
2. Keep stable cross-module contracts in `Engine/Tasks/Public`. Confirm each significant public contract has a canonical matching filename and contains no queue, worker-record, topology-storage, profiler-provider, native wait, or product-policy implementation detail. Merge or split headers only when the resulting contract has one clear consumer-facing responsibility.
3. Reconcile the private tree around real reasons to change. Evaluate, at minimum, coherent groups for graph/topology construction, execution/result state and the serial oracle, worker scheduling/executor mechanism, structured lifetime/external completion, and private profiling. Create a directory only when at least two cohesive files or a demonstrated backend/growth boundary justify it; otherwise retain deliberate file-private co-location.
4. Adversarially review `TaskExecutor.cpp`. Separate the public executor facade from fixed-worker/lane/run mechanism if their ownership and change reasons are currently obscured. Do not add wrapper-only layers. The worker/deque/injection/parking implementation remains private and no queue type appears in a public header.
5. Adversarially review `TaskExecutorInternal.h`. Its filename and placement must match what it actually owns. If it primarily contains execution result/state and serial-execution contracts, rename/move or split it accordingly; do not leave an `Executor` filename as a miscellaneous internal bucket.
6. Review `SerialTaskExecutor.cpp`, `TaskExecution.cpp`, `TaskGraph.cpp`, `ParallelFor.cpp`, `TaskScope.cpp`, `TaskEvent.cpp`, and `TaskProfiler.*` for filename-to-primary-type/operation alignment. Rename or co-locate deliberately; remove duplicated validation, lane naming, completion accounting, or lifetime helpers exposed by the reorganization without changing semantics.
7. Audit every Prompt 00–03 touched function for local type declarations and complex lifecycle/policy lambdas. Move local types to file-private or owning-type scope. Extract a lambda only when it contains multi-step lifecycle, synchronization, error policy, or reusable scheduling behavior; do not turn simple traversal predicates into ceremonial functions.
8. Review InputSystem's owner-thread and deferred-dispatch implementation. Keep callback snapshot semantics obvious in `ProcessDeferredEvents` and dispatch orchestration; retain a file-private re-entry/lifetime helper only if it is specific to InputSystem and clearer than a generalized Core utility. Do not create a generic guard unless at least two real consumers share the same invariant.
9. Update every moved/renamed path atomically: includes, forward declarations, export boundaries, CMake globs/source groups, module comments, documentation links, profiler labels when responsibility names change, and any generated/package references. Delete old files and compatibility includes in this prompt.
10. Re-run the Tasks forbidden-dependency and canonical/rejected-alias searches. Search old filenames and directories explicitly and require zero owned-source hits. Confirm CMake target membership follows the physical tree and no move changed dependency direction.
11. Update Prompt 03 implementation evidence with the final structure reconciliation: exact retained/moved/renamed/split/merged/co-located files and the ownership reason. Do not claim that a move improved architecture without naming the responsibility boundary it exposes.

Validation:
- Focused `SparkleTasks` and `SparklePlatform` compile after all moves/renames; do not build the full repository.
- Disposable contract smoke validation covers serial and 1/2/N execution, graph failure/cleanup, scope cancellation, TaskEvent wake, ParallelFor coverage, shutdown, and InputSystem self-unsubscribe/nested/deferred snapshot behavior when implementation—not only paths—changed. Delete its source, CMake target, fixtures, and generated data after the run.
- Dependency scan proves SparkleTasks includes/links no forbidden engine/product module.
- Touched-scope source audit proves no function-local class/struct declarations, no obsolete path or compatibility include, and no rejected concurrency alias.
- Public-header audit proves private queue/worker/run/profiler/native-wait implementation does not leak through the stable contract.
- Tree review proves every new directory has at least two cohesive files or a documented backend/growth boundary, and every deliberately flat/co-located file has a stated reason.

Acceptance gate:
- L Sections 19-21 (reconciliation, acceptance checklist, and required completion report) and Section 24 (Principal Graphics Engineering review gate) pass for the entire touched changelist, including directly affected neighboring ownership paths; no undocumented exception remains.
- A reader can locate graph construction, execution state, scheduling mechanism, structured lifetime/completion, algorithms, and instrumentation from folder and filenames without reading implementation bodies first.
- Public filenames and primary contracts agree; private filenames describe their actual responsibility; CMake and include paths agree with the physical architecture.
- `TaskExecutor` orchestration is not a dumping ground for unrelated graph, execution-result, profiling, event, or InputSystem policy.
- Completed Prompt 00–03 behavior and API meaning remain intact, with focused validation passing.
- No local product type declaration, compatibility path, duplicate helper family, generic dumping-ground folder, or unjustified one-file directory remains in the audited scope.
- The completion report contains the complete Rule 12 structure reconciliation and a deletion ledger for every old path/name.

Positive patterns: ownership-revealing tree, matching file/type names, private mechanisms, policy-focused orchestration, bounded atomic moves, deliberate co-location.
Forbidden: cosmetic folder churn, one class per file dogma, wrapper-only layers, generic `Common`/`Misc` dumping grounds, compatibility headers, feature work hidden inside cleanup.
~~~

Implementation evidence (2026-07-18): **passed**. The complete Prompt 00–03 Tasks subtree and InputSystem registration/deferred-dispatch seam were reviewed. Public contracts remain flat because each is a stable consumer concept: `TasksAPI.h` owns export policy; `TaskTypes.h` owns task values and policies; `TaskGraph.h` owns handles, validation, the builder, and immutable topology; `TaskExecutionContext.h` owns per-execution bindings/cancellation observation; `TaskExecution.h` owns the settled execution handle; `TaskExecutor.h` owns configuration and the single facade/PImpl; `TaskScope.h` owns structured lifetime; `TaskEvent.h` owns generation-safe external completion; and `ParallelFor.h` owns the range contract. No worker, queue, topology record, profiler provider, native wait, or product policy is public.

The private target map exposes five real change boundaries. `Graph/TaskGraph.cpp` and `TaskGraphInternal.h` own builder validation and compiled nodes/topology; `Graph/ParallelFor.cpp` is co-located because it expands ranges into that topology. `Execution/TaskTypes.cpp`, `TaskExecution.cpp`, and renamed `TaskExecutionInternal.h` own values, published result state, serial contracts, and worker-wait detection; renamed `SerialTaskExecution.cpp` is the deterministic oracle. Scheduling is deliberately decomposed further: `TaskExecutor.cpp` is host-facing orchestration; `TaskExecutorImplementation.*` adapts the stable PImpl to its runtime; `TaskExecutorRuntime.h` declares the private collaboration/state boundary; `TaskExecutorImplementation.cpp` owns configuration, submission, lifecycle, and shutdown policy; `TaskWorkerScheduling.cpp` owns workers, lane queues, stealing, parking, and join mechanics; and `ScheduledTaskExecution.*` owns one graph execution's prerequisite, nested-completion, result, and terminal accounting. `Lifetime/TaskScope.*`, `TaskScopeInternal.h`, and `TaskEvent.cpp` own scope trees and external completion. `Profiling/TaskProfiler.*` alone owns optional private ETW events. The PImpl-to-runtime indirection keeps the stable facade independent of worker/execution layout; it is not a second executor or operation family.

Input remains in Platform ownership. `InputSystem.h` retains callback/deferred storage and small snapshot templates because event type is compile-time dispatch policy; `InputSystem.cpp` retains owner-thread orchestration and one file-private `DeferredEventProcessingGuard`. That guard has one InputSystem consumer and was not generalized into Core. Registration changes affect the next copied snapshot, nested dispatch reads the current registry, and deferred-phase re-entry is suppressed without a type declaration inside `ProcessDeferredEvents`.

Rule 13 data audit:

| Data path | Access and authority | Layout, identity, and transform | Precedent and falsifier |
|---|---|---|---|
| builder → compiled graph | builder mutation is single-owner; compile validates and publishes the execution authority; executors only read it | bounded node array with per-node adjacency/nesting vectors; builder identity + generation validates handles; insertion and topological validation are deterministic | Epic prerequisite DAG/nested Tasks and AMD RPS graph/node IDs; replace only if graph-build/cache measurements show adjacency allocation or traversal dominates representative frames |
| graph → execution state | serial/parallel executors read immutable nodes; one run owns atomic prerequisite/unfinished/terminal state and separate results | task index is stable within the builder generation; transitions settle exactly once and result policy is deterministic although worker order is not | Epic Tasks, NVIDIA stdexec structured composition, and O3DE task-graph precedent recorded in J; repack only if layout inspection plus cache/false-sharing captures prove a repeatable bottleneck |
| ready scheduling stream | a run derives ready records; worker owns its local deque end, thieves use the opposite end, external readiness enters synchronized lane injection | compact `{run, task index}` records bounded by execution lifetime; lane and graph dependencies determine legal placement | Epic/O3DE scheduling mechanisms and NVIDIA worker-count guidance in J; change queue/padding only when steal, wake, runnable-thread, and cache evidence beats this correctness-oriented layout |
| scope/event completion | scope is lifetime authority; executions/events publish derived settlement/cancellation | generations reject stale tokens; weak links avoid ownership cycles; downward cancellation/upward settlement are deterministic | Epic nested Tasks/Task Events and NVIDIA stdexec scope/cancellation; revise if race validation finds a missing terminal path or a real consumer proves another boundary |
| Input callback/deferred streams | owner thread is registry/queue authority; dispatch consumes eligible callback copies; deferred queues retain event order | callback handle is stable registration identity; typed vectors match per-event traversal; snapshot and deferred transforms preserve insertion order | Epic owner-thread/delegate and async-loading restrictions summarized in J; change layout only if representative high-rate input captures prove copying material while preserving re-entry semantics |

Deletion and cohesion ledger: public files were retained; `Private/ParallelFor.cpp` → `Private/Graph/ParallelFor.cpp`; `TaskGraph.cpp`/`TaskGraphInternal.h` → `Graph/`; `TaskTypes.cpp` and `TaskExecution.cpp` → `Execution/`; `TaskExecutorInternal.h` → `Execution/TaskExecutionInternal.h` because it owns result state and serial contracts; `SerialTaskExecutor.cpp` → `Execution/SerialTaskExecution.cpp` because the operation is serial execution, not a second executor; `TaskScope.cpp`/`TaskScopeInternal.h` and `TaskEvent.cpp` → `Lifetime/`; `TaskProfiler.*` → `Profiling/`; and monolithic `Private/TaskExecutor.cpp` split into the seven focused Scheduling files named above. The formerly near-god mechanism body no longer combines host policy, worker mechanics, and per-graph settlement. Old paths were deleted with no compatibility headers. Every private directory has at least two cohesive files; public contracts and InputSystem-specific policy remain deliberately co-located.

A disposable DevelopmentEditor smoke executable passed ten cycles of serial and 1/2/4-worker execution, fan-out/fan-in, failure cancellation with cleanup, exact `ParallelFor` coverage, scope cancellation, `TaskEvent` wake, and drain shutdown. Prompt 03 already validated Input self-unsubscribe, nested dispatch, and deferred snapshots; 03R did not change those semantics. The temporary source and CMake target were deleted. Focused `SparkleTasks` and `SparklePlatform` builds pass; dependency, obsolete-path, rejected-alias, public-leakage, and function-local-type audits pass.

## Prompt 04 — Prove SparkleTasks in Real Tool Workflows

Target CL Title: `SparkleTools: Integrate SparkleTasks into Production Workflows`

~~~text
Implement Prompt 04 only after Prompt 03R passes.

Objective:
Replace ad-hoc/as-serial application and tool concurrency with SparkleTasks in coarse, useful pilots: shader recook process coordination, launcher operations/process I/O, texture request cooking, and safe shader cook nodes. Preserve deterministic transactional output and explicit external-process lifetime.

Non-negotiable repository rules:
- Apply `Docs/Architecture/00-Review/L_SparkleEngineIntegrationStyleGuide.md` in full. Treat its ownership, implementation-shape, DOD, concurrency, rendering, naming, validation, structure, principal graphics engineering, and completion-report gates as acceptance criteria. Before editing, list the applicable `PGE-01` through `PGE-15` requirements from A, interpret them through H, and classify each as advance, preserve, not applicable, or blocked with expected evidence. Inspect the current repository, reconcile the whole touched ownership path, verify any AI-assisted work independently, delete replaced paths in the same change, reject role-only scaffolding, and report any justified exception explicitly with its owner, scope, evidence, and deletion or review gate.
- Apply Rule 13: audit every data source, transform, stream, packet, cache, table, upload, and hot traversal touched by this prompt; record the concrete access inventory, authoritative/derived ownership, layout decision, stable identity, deterministic transform, exact source precedent, and measured falsifier.
- Apply Rule 12: audit touched/new files for module and folder ownership, public/private placement, filename-to-primary-type alignment, and nearby misplaced counterparts; complete bounded moves/renames with includes, CMake, and documentation updated before the gate.
- Enforce J's canonical concurrency/rendering vocabulary and Rule 10: search canonical terms plus rejected aliases in the touched scope, use one responsibility per name, and delete temporary compatibility spellings before the gate.
- Search each tool for existing planners, batches, compiler sessions, COM/thread setup, process readers, caches, registries, publication, status/progress, and cancellation.
- Extend existing plans rather than creating alternate Async cookers or parallel pipelines.
- One task runtime with tool host policy; no std::async, detached reader threads, or tool-specific pools after migration.
- Refactor duplicated process/result/publication logic in the touched path and delete the old execution route in this prompt.
- Apply Rule 15: receiving editor/launcher/cooker code invokes domain capabilities; task graphs, lane policy, cancellation bridging, task-local setup, and fan-in remain private to the owning execution service.
- Close LC-01 through LC-04 and LC-17 for the migrated call paths. Qt `startDetached` may remain only when launching a deliberately independent product; it may not substitute for scoped operation lifetime.

Required implementation:
1. Replace ShaderRecookCoordinator std::async/future with scoped Background/BlockingIo tasks and owner-thread structured result consumption.
2. Preserve latest/queued request behavior through explicit request/publication generations; failure keeps old shader packages active.
3. Move blocking child-process pipe handling to BlockingIo; cancellation signals child policy, drains/closes pipes, retains diagnostics, settles once.
4. Replace LauncherBackend's QThread-per-operation path with the scoped operation service and immutable progress/result delivery queued to the Qt owner thread. Migrate ProcessRunner's reader `std::thread`, 50 ms child polling, and standalone atomic cancellation into structured BlockingIo completion. Window/QObject access remains on the GUI owner.
5. Audit AssetCookerToolProcess's infinite process wait. Keep a synchronous adapter only for the outer CLI host; any scheduled caller uses BlockingIo, cancellation, handle cleanup, diagnostics, and exactly-once settlement.
6. Preserve intentionally independent launcher/restart process behavior, but replace timed handoff assumptions where an owned readiness/exit protocol is possible. Record the owner of every remaining detached process.
7. Parallelize texture requests only after auditing COM/decoder/cooker safety. Use per-task context and a weighted decompressed-byte limiter.
8. Execute safe shader cook plan nodes with per-worker/session constraints and internal-compiler-thread limits.
9. Collect diagnostics by stable asset/package key and emit packages/registries only at deterministic fan-in.
10. Write generation-temporary output, validate complete set, atomically publish; cancelled/failed generations never become active.

Validation:
- Serial/parallel outputs byte-identical wherever formats promise determinism; otherwise stable semantic/package/reflection identity comparison.
- Cancellation at launch, pipe read, texture decode, shader compile, fan-in, and pre-publication.
- Launcher operation start/cancel/restart/application-close stress; all UI progress/results arrive on the Qt owner, no QThread per operation remains, and child handles/pipes settle exactly once.
- Asset-cooker synchronous-host and scheduled BlockingIo paths have equivalent exit code/diagnostic behavior; no SparkleTasks worker performs an infinite native wait.
- One node failure leaves previous publication active and no partial registry accepted.
- Interactive run proves tool work does not starve the FrameCritical lane.
- Memory stress with multiple 4K/8K/HDR textures respects weighted limit.

Acceptance gate:
- L Sections 19-21 (reconciliation, acceptance checklist, and required completion report) and Section 24 (Principal Graphics Engineering review gate) pass for the entire touched changelist, including directly affected neighboring ownership paths; no undocumented exception remains.
- Shader recook future/std::async, LauncherBackend operation QThreads, migrated reader threads/poll loops, and duplicate cancellation paths are deleted.
- No duplicate cooker/compiler/publication pipeline exists.
- Every remaining `startDetached`, native child wait, or Qt handoff delay has an explicit independent-process/outer-host justification and no task-lifetime responsibility.
- DXIL/SPIR-V, reflection/layout, IDs, registry order, and diagnostics match serial contract.
- Tool throughput evidence includes peak memory and explains internal compiler/compressor threads.

Positive patterns: coarse tasks, task-local contexts, deterministic fan-in, transactional publication, bounded memory.
Forbidden: parallel publish-as-finished, shared unsafe importer/compiler instance, unbounded texture jobs, UI callbacks from I/O thread.
~~~

### Prompt 04 implementation record

The pilot now uses one SparkleTasks runtime per product/tool host. Editor shader recook is a scoped Background preparation plus BlockingIo compiler execution; request and publication generations are consumed only by the editor owner. Launcher operations execute as bounded BlockingIo tasks, while `LauncherOperationRequestMapping` owns Qt-to-domain translation and `LauncherOperationExecution` owns plan execution. The Qt backend only orchestrates, then queues immutable output/completion values through `QMetaObject::invokeMethod`. AssetCooker's synchronous CLI adapter and the launcher/editor scheduled paths share Core `Process::ChildProcess`; Windows process-tree ownership, overlapped pipe completion, stop-token cancellation, and handle cleanup live behind the platform facade. The former future, `QThread::create`, pipe-reader thread, 50 ms polling, atomic cancellation family, `_popen`, and infinite child wait were deleted.

Texture requests retain the existing request loader and cooker pipeline. A bounded set of Background tasks owns COM initialization and one decoder/cooker context per request. Results occupy request-indexed slots, diagnostics retain `TextureAssetId`, and a weighted limiter admits the decoded mip payload through the pipeline. No task publishes. The owner validates every result, then publishes the staged set through `Files::TryPublishFileSet`, which preflights the complete set and restores the prior generation if any replacement fails. Shader cooking likewise extends the existing plan: bounded Background nodes own backend sessions and cache-store adapters, write node-indexed results, and the owner merges stages in plan order. Packages, registry, and recook signal are all staged; the signal is the last transaction member and therefore remains the activation marker. A failed/cancelled node never reaches emission, and failed publication restores the previous package/registry/signal set.

Rule 13 access inventory:

| Touched stream | Authority and derived ownership | Layout, identity, deterministic transform | Source precedent and falsifier |
|---|---|---|---|
| child command, environment, stdout/stderr, exit | request value is authoritative; OS process/job/pipe handles are private execution state; `ChildProcessResult` is the sole derived result | ordered byte stream plus one exit/cancel/failure record; executable/argument order is stable; one job owns descendants | EPIC-TASK long/blocking-work separation and J's BlockingIo contract; cancel while a shell descendant owns the pipe must settle promptly with captured preceding output |
| launcher operation request/progress/result | Qt owner owns the request and UI; task owns an immutable request copy; queued Qt values own progress/result text | domain plan order is retained; `runId`/operation ID are stable identities; no QObject is dereferenced by process I/O | Epic task lifetime/continuation model and Qt owner-affinity inventory in J; close/cancel plus thread-affinity capture falsifies it |
| texture request/source/decoded mips/cooked output | request list and source file are authoritative; each task exclusively owns loader, mip vectors, pipeline state, staged file, diagnostic slot, and memory lease | vector-of-slices/mips remains the existing traversal layout; `TextureAssetId` and sorted request index are stable; fan-in/publish is request order | AMD-CPU thread-local/range-local guidance and NV-PAR bounded batch/serial guidance; byte comparison, overlap stress, and peak admitted bytes falsify it |
| shader registrations/plan/cache/node results/package registry | registrations and immutable plan are authoritative; task-local compiler session/store/result are derived; only owner mutates package contexts and counters | plan/node vectors remain contiguous; package key plus node index identify results; merge, diagnostic selection, registry order, and publication order are stable | EPIC-TASK prerequisite/fan-in model, AMD-RPS planned ranges, and NVIDIA bounded-thread guidance; serial/parallel DXIL/SPIR-V/package/reflection hashes falsify it |
| generation-temporary files | published paths are authoritative; staged and backup paths are transaction-private | destination path is stable identity; complete-set preflight, ordered replace, reverse rollback, and signal-last commit are deterministic | existing Sparkle atomic-file convention extended to a file set; injected missing/locked destination and old-generation hash checks falsify it |

Rule 12/14/15 cohesion record: Core's stable `Process/ChildProcess.h` facade is separated from `Private/Process/ChildProcessWindows.*`; generic multi-file publication remains beside Core file primitives. `ShaderRecookExecutionService` owns task graph/lane/scope/process execution so `ShaderRecookCoordinator` retains request, publication, and renderer-reload policy. `ShaderCookPlanExecutor` owns task partition/configuration and node failure selection so `ShaderPackageCooker` remains plan → deterministic merge → emit. `TextureCookBatchExecutor` owns task/COM/cancellation mechanics; `TextureAssetCooker` accepts only a standard stop token and remains a texture transformation, while the request processor retains load/diagnostic/publication policy. Launcher Qt mapping, domain execution, and scoped task lifetime are separate `Private/Gui/Operations/` owners; `LauncherBackend` now owns only preview/UI orchestration and owner-thread delivery. All scheduling types remain private, all cross-module surfaces are domain requests/results or the shared Core process/file contracts, and no compatibility path or second cooker/compiler/pool was retained.

Focused DevelopmentEditor builds pass for `SparkleCore`, `SparkleApplicationEditor`, `AssetCookerCore`, `TextureCooker`, `ShaderCompiler`, and `SparkleLauncher`; no whole-repository build was run. A disposable Core smoke executable proved output capture, cancellation of a shell descendant that inherited the pipe, sub-second process-tree settlement, and complete file-set replacement; it exposed the missing process-tree ownership, which was fixed with a kill-on-close Windows job, and the smoke source/target were then deleted. A real GBuffer cook with four plan nodes produced byte-identical serial and four-session package/registry outputs (`C64E5BD...EBD3` and `6C09747F...19AE`). A real two-request texture batch produced identical outputs and a 2,400,000-byte admitted peak. The required multiple-4K/8K/HDR memory run, interactive FrameCritical latency capture, forced failure at every publication rename, and full launcher UI cancel/restart stress still require the representative assets and interactive harness; they are explicit evidence gaps, not reported passes.

## Prompt 05 — Build the Serial ECS Identity and Storage Kernel

Target CL Title: `SparkleGameFramework: Build the Serial ECS Storage Kernel`

~~~text
Implement Prompt 05 only after Prompt 03R passes. Prompt 04 may proceed independently but must use the same SparkleTasks contracts.

Objective:
Replace the unused owning Entity model with the private serial ECS foundation: generational EntityId, registry, per-type sparse-set component storage, stable schema IDs, and invariant tests. Do not parallelize systems yet.

Non-negotiable repository rules:
- Apply `Docs/Architecture/00-Review/L_SparkleEngineIntegrationStyleGuide.md` in full. Treat its ownership, implementation-shape, DOD, concurrency, rendering, naming, validation, structure, principal graphics engineering, and completion-report gates as acceptance criteria. Before editing, list the applicable `PGE-01` through `PGE-15` requirements from A, interpret them through H, and classify each as advance, preserve, not applicable, or blocked with expected evidence. Inspect the current repository, reconcile the whole touched ownership path, verify any AI-assisted work independently, delete replaced paths in the same change, reject role-only scaffolding, and report any justified exception explicitly with its owner, scope, evidence, and deletion or review gate.
- Apply Rule 13: audit every data source, transform, stream, packet, cache, table, upload, and hot traversal touched by this prompt; record the concrete access inventory, authoritative/derived ownership, layout decision, stable identity, deterministic transform, exact source precedent, and measured falsifier.
- Apply Rule 12: audit touched/new files for module and folder ownership, public/private placement, filename-to-primary-type alignment, and nearby misplaced counterparts; complete bounded moves/renames with includes, CMake, and documentation updated before the gate.
- Enforce J's canonical concurrency/rendering vocabulary and Rule 10: search canonical terms plus rejected aliases in the touched scope, use one responsibility per name, and delete temporary compatibility spellings before the gate.
- Search Entity, Component, SceneCameras/Meshes/Lighting/Animations, handles, registries, sparse/dense containers, type IDs, serialization schemas, and editor selection before adding types.
- ECS internals remain GameFramework-private; do not add a standalone ECS SDK/module or expose storage to Renderer/Editor.
- Keep current scene behavior working; no second mutable source of truth may survive a converted slice.
- Apply daily refactoring to the old Entity/Component path and remove only behavior made redundant by this prompt.

Required implementation:
1. Add opaque EntityId(slot,generation), invalid value, validation, free-list/reuse, and explicit generation wrap policy.
2. Add EntityRegistry with create/destroy/is-alive and one private type-erased storage registry with typed call sites.
3. Add ComponentStorage<T>: sparse slot-to-dense lookup, dense EntityId array, dense T array, add/remove/replace/get/contains, swap-remove bookkeeping, reserve/bulk insert.
4. Separate stable serialized ComponentSchemaId/version from compiler-local runtime type identity.
5. Define component storage/query version counters for later stale-view checks.
6. Add initial data component definitions only: LocalTransform, WorldTransform, MeshInstance handles/state, Visibility, Camera, Light, AnimationState, Morph/Skinning state handles, Name/EditorMetadata. Heavy assets/services/pointers are forbidden inside hot components.
7. Delete the unused owning Entity orchestration once no call site requires it. Retain/simplify legacy Component only where current camera/mesh compatibility still consumes it, with a deletion target in Prompt 07.

Validation:
- Randomized reference-model tests for entity create/destroy/reuse and component add/remove/replace.
- Verify sparse[denseEntity[i].slot] == i and every live sparse mapping round-trips after swap removal.
- Stale entity IDs fail after slot reuse.
- Bulk insertion order and schema serialization identity deterministic across runs/configurations where promised.
- Non-movable/owning component policy explicitly rejected or safely supported according to the chosen bounded contract.

Acceptance gate:
- L Sections 19-21 (reconciliation, acceptance checklist, and required completion report) and Section 24 (Principal Graphics Engineering review gate) pass for the entire touched changelist, including directly affected neighboring ownership paths; no undocumented exception remains.
- Registry/storage invariants pass long randomized stress.
- Renderer/Editor cannot include ECS storage headers.
- No asset ownership, callbacks, mutexes, virtual Update/Render behavior, or raw scene pointers were introduced into data components.
- Old Entity container is deleted or has a concrete Prompt 07 blocker with no new consumers.

Positive patterns: opaque generations, packed per-type iteration, stable schema/runtime separation, private storage seam.
Forbidden: archetype chunks, reflection framework, prefab system, public registry, storing dense indices across mutation.
~~~

### Prompt 05 completion record — 2026-07-18

Prompt 05 **passes**. `EntityId` is the only new public world contract: an opaque slot/generation value with an invalid state and no public constructor for fabricated live identities. `EntityRegistry` owns allocation, validation, component cleanup, free-slot reuse, and the live count. Destroy increments the generation before reuse; exhausting the 32-bit generation retires that slot permanently, so wrap cannot make a reachable stale ID live again. The former public owning `Entity` and its virtual Initialize/Update/Render traversal were unused and are deleted. The still-consumed legacy `Component` now contains only polymorphic destruction and visibility; Prompt 07 owns its deletion after camera/mesh instance migration and no new consumer may be added.

`ComponentStorage<T>` is a GameFramework-private sparse set: sparse entity slot to dense index, dense `EntityId` array, and dense per-type value array. Add rejects an occupied slot even when presented with another generation; exact generation comparison protects reads/removes; swap removal repairs the moved entity's sparse entry. Bulk insertion validates the entire input, reserves all arrays, and then moves values in input order. Components must be copy-constructible and nothrow-movable/assignable; non-movable and owning `unique_ptr`-style component types fail the compile-time storage concept. Mutable pointers/spans are not exposed: value changes use `Replace`, which advances content identity, while structural changes advance both structure and content identity. `ComponentQueryVersion` captures those counters for Prompt 06 stale-view enforcement. Dense indices remain an implementation detail and are never durable identity.

Stable serialization identity is separate from compiler-local dispatch. `ComponentSchemaId` is the specified 64-bit FNV-1a hash of a canonical `sparkle.world.*` schema name plus an explicit schema version; fixed compile-time values guard accidental drift. `RuntimeComponentTypeId` is private, process-local, registration-order-dependent, and never serialized. `ComponentTypeRegistry` owns one type-erased storage map with typed calls; there is no public registry, ECS module, reflection system, archetype store, prefab path, or raw renderer/editor storage seam.

Rule 13 access inventory:

| Data path | Authority and access | Layout, identity, and deterministic transform | Exact precedent and falsifier |
|---|---|---|---|
| entity allocation/free stream | `EntityRegistry` is the sole authority; serial create/destroy writes slots/free list and readers validate values | packed slot table plus LIFO free-slot vector; `{slot,generation}` is stable runtime identity; destroy removes components, advances generation, then makes reuse available | EnTT registry at pinned `1333fa5` and J's ABA/generational-handle contract; randomized reference-model create/destroy/reuse and stale-ID probes falsify identity errors |
| per-type component table | registry structural calls are authoritative; future queries consume const dense spans; no current scene owns one yet | sparse slot-to-dense lookup plus separate dense identity/value arrays; insertion preserves request order and removal is swap-compact | EnTT sparse set at pinned `1333fa5`; dense-to-sparse pointer round trips after randomized swap removal falsify bookkeeping, while Prompt 07 cache/bandwidth comparison will falsify any performance claim |
| component value mutation/query freshness | `Replace` is the only exposed value writer; const reads capture storage/query versions | structure/content counters distinguish compaction from value replacement; exact entity generations reject stale slot reuse | Epic transient Mass entity-view lifetime contract and EnTT versioned pools; version-transition and future Prompt 06 stale-view negative tests are the falsifiers |
| schema/runtime type lookup | canonical schema names/versions are serialization authority; runtime IDs only index the private in-process map | fixed schema hashes are configuration-independent; runtime registration order cannot enter files, packets, editor identity, or renderer identity | Unity/EnTT typed component precedent and Sparkle cooked-schema practice; compile-time expected hashes plus run-time uniqueness checks falsify drift/collision in the initial catalog |
| initial world component columns | these are data definitions only; existing `SceneCameras`, `SceneMeshes`, lighting, and animations remain the sole mutable scene authority until Prompt 07 | hot transform/render/animation state is split by consumer; cold `Name`/`EditorMetadata` is separate; mesh/material/assets and morph/skinning state are handles, never owned resources or scene pointers | Epic Mass data-only fragments, Unity ECS samples, EnTT typed pools, and Fabian's access-driven DOD questions as recorded in J; Prompt 07 representative current-scene iteration/cache measurements may change a layout, and migration parity must delete each legacy owner |

Rule 12/14/15 reconciliation: public stable identity lives in `Public/World/EntityId.h`; all allocation/storage/schema mechanics live in `Private/World/ECS`; cohesive data definitions and their schema catalog live in `Private/World/ECS/Components`. Template storage remains in its matching header; the only non-template owner implementation is `EntityRegistry.cpp`. Existing `CookedAssetId`, `MaterialHandle`, `SceneMeshKind`, and `SceneLightKind` are reused instead of duplicated. The renderer and editor receive no integration code. `ArchitectureBoundaryCheck.cmake` now rejects private `World/ECS` includes from Renderer, Editor, and GameFramework public contracts.

Validation used a disposable DevelopmentEditor executable and removed its source/CMake target afterward. Five complete runs each exercised eight deterministic seeds × 50,000 randomized create/destroy/add/remove/replace/get operations (2,000,000 operations total), continuously compared against an independent map model, and checked every dense entity resolves to the component at the same dense address after compaction. It also proved slot reuse changes generation, stale identities cannot read/add, bulk order is exact, schema IDs are valid/unique/fixed, structure/content versions change under the intended operations, and owning/non-movable policy is rejected at compile time. A final disposable pass compiled every initial component/schema, inserted and verified 10,000 ordered values through the sorted-slot bulk path, and rejected a duplicate-slot batch without partial insertion. `cmake --build build --config DevelopmentEditor --target Prompt05EcsSmoke --parallel 8` built only the focused GameFramework dependency and probe; all runs returned 0. The architecture boundary script passed. No whole-repository build, performance claim, worker, query, system scheduler, or scene migration was performed.

## Prompt 06 — Add ECS Queries, Structural Epochs, and Entity Commands

Target CL Title: `SparkleGameFramework: Add ECS Queries, Structural Epochs, and Entity Commands`

~~~text
Implement Prompt 06 only after Prompt 05 passes.

Objective:
Make ECS iteration and structural mutation safe for future jobs: typed read/write queries, include/exclude filtering, frozen structural epochs, task-local EntityCommandBuffer, deterministic playback, and temporary entity remapping. Execute serially.

Non-negotiable repository rules:
- Apply `Docs/Architecture/00-Review/L_SparkleEngineIntegrationStyleGuide.md` in full. Treat its ownership, implementation-shape, DOD, concurrency, rendering, naming, validation, structure, principal graphics engineering, and completion-report gates as acceptance criteria. Before editing, list the applicable `PGE-01` through `PGE-15` requirements from A, interpret them through H, and classify each as advance, preserve, not applicable, or blocked with expected evidence. Inspect the current repository, reconcile the whole touched ownership path, verify any AI-assisted work independently, delete replaced paths in the same change, reject role-only scaffolding, and report any justified exception explicitly with its owner, scope, evidence, and deletion or review gate.
- Apply Rule 13: audit every data source, transform, stream, packet, cache, table, upload, and hot traversal touched by this prompt; record the concrete access inventory, authoritative/derived ownership, layout decision, stable identity, deterministic transform, exact source precedent, and measured falsifier.
- Apply Rule 12: audit touched/new files for module and folder ownership, public/private placement, filename-to-primary-type alignment, and nearby misplaced counterparts; complete bounded moves/renames with includes, CMake, and documentation updated before the gate.
- Enforce J's canonical concurrency/rendering vocabulary and Rule 10: search canonical terms plus rejected aliases in the touched scope, use one responsibility per name, and delete temporary compatibility spellings before the gate.
- Search existing view/span/range, command, change-list, transaction, type-access, and deterministic-sort facilities before adding counterparts.
- Do not add a generic query language, runtime reflection DSL, or editor-facing raw registry API.
- Refactor duplicate create/destroy/set operations in touched scene paths toward one command/commit owner.
- Preserve current scene behavior through serial tests before any system parallelism.

Required implementation:
1. Add typed Query<Read<T>,Write<U>...> with include/exclude sets and explicit access metadata.
2. Single-component queries iterate the dense pool directly. Multi-component queries select the smallest included pool and resolve others through sparse lookup.
3. Add StructureFrozen epoch after owner commit. Query/view captures registry/storage versions and asserts on structural invalidation.
4. Permit value writes only through declared writable component access; prohibit create/destroy/add/remove during frozen iteration.
5. Add task-local EntityCommandBuffer commands: create temporary entity, destroy, add/remove/replace/set component.
6. Give each command deterministic system/phase/partition/local sequence keys. Merge/playback must not depend on recording completion order.
7. Remap temporary entity tokens only within their originating buffer; reject escape/cross-buffer use.
8. Produce a typed commit result for stale target, missing/present component conflicts, and successful mapping.

Validation:
- Query results match a slow reference model for randomized compositions and include/exclude combinations.
- Const query cannot obtain writable access at compile time/API boundary.
- Frozen-epoch structural mutation and stale view/version negative tests assert/reject.
- Shuffled command-buffer arrival produces identical EntityId/component state after deterministic merge.
- Temporary create plus subsequent add/set/destroy works; cross-buffer token use fails.
- Conflicting writes require explicit policy and never silently use task completion order.

Acceptance gate:
- L Sections 19-21 (reconciliation, acceptance checklist, and required completion report) and Section 24 (Principal Graphics Engineering review gate) pass for the entire touched changelist, including directly affected neighboring ownership paths; no undocumented exception remains.
- Structure remains unchanged for the full query epoch.
- Commands settle deterministically and preserve registry invariants.
- No direct structural mutation path remains in any scene path converted during this prompt.
- No scheduler dependency is inferred from captured pointers; access metadata is explicit.

Positive patterns: typed const/write access, frozen epochs, per-task buffers, deterministic playback.
Forbidden: shared command buffer across jobs without partition protocol, mutation during iteration, stored query iterators across commit, incidental last-writer-wins.
~~~

### Prompt 06 completion record — 2026-07-18

Prompt 06 **passes**. `EntityRegistry` now alternates explicitly between structural commit and a move-only `StructureFrozenEpoch`. Create, destroy, reserve, add, remove, bulk add, and direct replace centrally reject while an epoch is frozen; releasing the epoch restores owner commit. Each successful structural operation advances the registry structure version, and each component storage retains separate structure/content versions. Frozen-epoch generations never wrap back to zero: generation exhaustion disables further epochs rather than reviving a stale query capability.

`Query<Read<T>, Write<U>, Exclude<V>...>` is GameFramework-private and carries explicit runtime access metadata without scheduler behavior. A single included component iterates its dense entity span directly. Multi-component queries choose the smallest included storage as the leading span, validate remaining included pools through sparse lookup, and reject excluded components through the same lookup. Query construction requires a current frozen-epoch token and captures registry plus per-storage structural versions. Queries are non-copyable/non-movable, expose no iterator or durable dense index, and reject use after the epoch ends or a captured structure version changes. Read callbacks receive `const T&`; mutable references are constructed only for declared `Write<T>` callbacks, and one conservative content-version advance records a writable traversal that reaches at least one entity. The query publishes type-level `ReadsComponent`, `WritesComponent`, and `ExcludesComponent` facts for later `GameSystemGraph` hazard compilation; it schedules nothing and infers nothing from captured pointers.

`EntityCommandBuffer` is a bounded, move-only, single-use task-local recorder. Its identity is the stable `{system, phase, partition}` producer key; every record adds a monotonically increasing local sequence. Temporary IDs contain the originating buffer identity plus local index, cannot be constructed externally, and are rejected immediately by another buffer. Create, destroy, add, remove, replace, and set records own typed values without storing registry/component pointers. Playback moves those values from the consumed buffer.

`EntityCommandCommit` preflights structure state, duplicate buffer IDs, overflow, prior consumption, and the required conflict policy before mutating the registry. It sorts all record pointers by `{system, phase, partition, local sequence}`, creates/remaps temporary entities in that order, and emits ordered typed command results plus successful temporary mappings. Add-present, remove/replace-missing, stale real/temporary target, capacity, and mapping failures are explicit statuses. The required `RejectLaterDeterministicKey` policy lets deliberate same-buffer sequences proceed but rejects a later cross-buffer write to the same entity/component or an entity-wide destroy conflict. Therefore neither input-buffer arrival nor future task completion order selects a writer. Buffers are marked consumed before playback and cannot be appended or committed again.

Rule 13 access inventory:

| Data path | Authority and access | Layout, identity, deterministic transform | Exact precedent and falsifier |
|---|---|---|---|
| structural epoch stream | registry owner is the only structure writer; frozen systems receive a scoped capability | registry version plus non-wrapping epoch generation; commit → freeze → release is one explicit state transition | Epic Mass transient-view/processing lifetime and Unity structural-change boundaries recorded in J; frozen mutation rejection and expired-query probes falsify it |
| typed query traversal | query reads immutable sparse/dense topology; only `Write<T>` callback arguments mutate values | tuple of typed storage pointers, captured structure versions, and a non-owning leading dense span; smallest included pool is selected deterministically | EnTT sparse-set/view behavior at pinned `1333fa5`, Epic Mass query batches, and Fabian access-driven layout rules; randomized slow-model include/exclude comparisons and future Prompt 07 cache measurements falsify correctness/performance claims |
| task-local command stream | one buffer/partition owns append order and component values; registry owner alone plays records | bounded contiguous records; producer identity plus local sequence is stable; merge sorts keys rather than completion order | Unity entity command-buffer deterministic sort keys and Epic Mass deferred commands recorded in J; reversed arrival, duplicate producer, overflow, and conflict scenarios falsify ordering/lifetime |
| temporary entity remap | originating buffer owns temporary namespace; commit owns real allocation/mapping | `{buffer ID, local index}` maps to generational `EntityId`; mapping order follows sorted create commands | Unity deferred-entity command precedent and Sparkle generational identity; create/add/set/destroy, retained mapping, foreign-buffer rejection, and stale-target probes falsify it |
| command result stream | commit owns ordered outcomes; callers receive values, never storage references | one result per accepted command plus successful temporary mappings; explicit semantic status replaces incidental last-writer-wins | existing Sparkle typed result practice plus Mass/Unity deferred playback; shuffled arrival must yield identical keys, statuses, mappings, entity generations, and final values |

Rule 12/14/15 reconciliation: query access vocabulary is isolated in `QueryAccess.h`; the templated traversal remains in `Query.h`. Epoch lifetime is owned by `StructureFrozenEpoch.*`, while mutation/version authority stays in `EntityRegistry.*`. Command value contracts, type-erased records, task-local recording, conflict ownership, and owner playback are separated into `EntityCommandTypes.h`, `EntityCommandRecord.h`, `EntityCommandBuffer.*`, `EntityCommandConflictTracker.*`, and `EntityCommandCommit.*`. No file combines query traversal, command recording, conflict policy, and playback. Everything remains under private `World/ECS`; Renderer, Editor, Tasks, RHI, and current scene containers receive no registry/storage API or integration logic.

Applicable hazards MT-05/06, MT-28–31, and MT-40 are closed for this serial slice through epoch-bound non-owning views, generation validation, access-specific packed traversal, stable command keys, single-use buffer lifetime, and one future migration authority. Prompt 06 does not claim the live scene is converted: `SceneCameras`, `SceneMeshes`, lighting, and animation remain the sole mutable product state, so no converted scene path or duplicate ECS authority exists yet. Prompt 07 must migrate each vertical slice and delete its old mutable owner.

Validation used one disposable DevelopmentEditor target and removed its source/CMake membership afterward. Twenty runs each built 2,000 randomized entity compositions and compared single-/multi-component include/exclude query counts with a slow reference model; writable visible/moving queries proved exact mutation coverage and content-version changes. Compile-time access facts proved `Read<Position>` has no write capability. Frozen create/destroy/add/remove/replace/reserve rejected; an expired query returned `InvalidEpoch`. Reversed command-buffer arrival produced identical sorted results, mappings, generations, and final values. Temporary create/add/set/destroy and retained mappings passed; foreign temporary use rejected; add-present, replace-missing, stale target, duplicate buffer ID, overflow, frozen commit, explicit cross-buffer conflict, and second-commit cases returned their typed statuses without hidden fallback. `cmake --build build --config DevelopmentEditor --target Prompt06EcsSmoke --parallel 8` built only GameFramework dependencies and the disposable probe; all twenty runs returned 0. The architecture boundary check and final source audits passed. No whole-repository build, worker execution, scene migration, ECS UI, reflection layer, or scheduler integration was added.

## Prompt 07 — Convert Existing Scene Instance Data to ECS Components

Target CL Title: `SparkleGameFramework: Convert Scene Instance Data to ECS Components`

~~~text
Implement Prompt 07 only after Prompt 06 passes.

Objective:
Move current camera, mesh-instance, visibility, light, transform, animation playback, and cold editor metadata instance state into ECS component pools while keeping GameScene as the coherent world facade. Compatibility facades may remain only as non-owning single-source views.

Non-negotiable repository rules:
- Apply `Docs/Architecture/00-Review/L_SparkleEngineIntegrationStyleGuide.md` in full. Treat its ownership, implementation-shape, DOD, concurrency, rendering, naming, validation, structure, principal graphics engineering, and completion-report gates as acceptance criteria. Before editing, list the applicable `PGE-01` through `PGE-15` requirements from A, interpret them through H, and classify each as advance, preserve, not applicable, or blocked with expected evidence. Inspect the current repository, reconcile the whole touched ownership path, verify any AI-assisted work independently, delete replaced paths in the same change, reject role-only scaffolding, and report any justified exception explicitly with its owner, scope, evidence, and deletion or review gate.
- Apply Rule 13: audit every data source, transform, stream, packet, cache, table, upload, and hot traversal touched by this prompt; record the concrete access inventory, authoritative/derived ownership, layout decision, stable identity, deterministic transform, exact source precedent, and measured falsifier.
- Apply Rule 12: audit touched/new files for module and folder ownership, public/private placement, filename-to-primary-type alignment, and nearby misplaced counterparts; complete bounded moves/renames with includes, CMake, and documentation updated before the gate.
- Enforce J's canonical concurrency/rendering vocabulary and Rule 10: search canonical terms plus rejected aliases in the touched scope, use one responsibility per name, and delete temporary compatibility spellings before the gate.
- Audit every field/call site in SceneCameras, SceneMeshes, SceneLighting, SceneAnimations, MeshComponent, CameraComponent, Transform, GameScene snapshot/loading, Showcase, editor inspectors, and renderer extraction.
- For every field classify: ECS component, immutable asset resource, world service/resource, derived output, editor-only metadata, or obsolete.
- Never copy the same mutable field into both legacy object and ECS storage.
- Refactor touched containers/factories/translators so construction and mutation converge on one path; delete duplicated state immediately per converted type.

Required implementation:
1. Make GameScene own the private EntityRegistry and expose narrow stable EntityId/world command/read contracts, not raw storage.
2. Translate mesh instances to MeshInstance + LocalTransform + Visibility (+ animation/skinning handles when applicable). Mesh resource ownership moves/remains in immutable asset registry/service and components store handles.
3. Translate cameras to Camera + LocalTransform + Visibility; remove lazy derived direction as authoritative state.
4. Translate lights to Light (+ LocalTransform where meaningful) and separate editor labels/cold metadata.
5. Translate animation playback to per-entity AnimationState; keep clip/skeleton definitions immutable resources.
6. Make existing Scene* types temporary facades over ECS queries/commands or replace call sites directly. A facade owns no parallel vector copy.
7. Convert scene/payload factories and current level initialization to deterministic component insertion.
8. Preserve authored/source identity separately from runtime EntityId.

Validation:
- Serial current-level visual and behavioral parity for cameras, mesh visibility/material, lighting, animations/morphs, Showcase motion, save/capture-to-level behavior.
- Component pool counts/values compare to a temporary reference extraction from the old path during tests only.
- Object deletion/reordering no longer invalidates durable selection/targets based on vector position.
- Asset unload/replacement cannot leave a component with an unvalidated raw resource pointer.

Acceptance gate:
- L Sections 19-21 (reconciliation, acceptance checklist, and required completion report) and Section 24 (Principal Graphics Engineering review gate) pass for the entire touched changelist, including directly affected neighboring ownership paths; no undocumented exception remains.
- ECS is the sole mutable instance-state source for every converted type.
- No new legacy Component/Entity consumer exists.
- Compatibility facades are read/command adapters with explicit deletion in Prompt 10/11, not storage owners.
- Serial behavior matches baseline before later task scheduling.

Positive patterns: data/resource separation, hot/cold components, one source of truth, incremental vertical migration.
Forbidden: giant GodComponent, raw Mesh ownership in ECS, duplicate Scene* vectors, generic reflection/serialization detour.
~~~

### Prompt 07 completion record - 2026-07-18

Target CL Title: `SparkleGameFramework: Convert Scene Instance Data to ECS Components`

`GameScene` now owns one private `ECS::SceneWorld`, which owns the private `EntityRegistry`, generation-validated mesh/deformation resource stores, animation resources, derived animation output, and world-level active-camera selection. The public boundary exposes stable `EntityId`, `IsEntityAlive`, `DestroyEntity`, and the remaining purpose-specific `Scene*` read/command surfaces; it does not expose registry, component storage, queries, or raw component pointers. `SceneAnimations` was deleted after the post-conversion consumer audit proved it had no external consumer, policy, invariant, or ownership: `GameScene` now invokes the world-private serial animation phase and payload attachment writes clips directly through the private appender seam. `SceneCameras`, `SceneLighting`, and `SceneMeshes` remain temporary anti-corruption adapters only while current Editor, Showcase, save, and diagnostic consumers still need synchronous focused access.

Camera/scene responsibility reconciliation completed after the initial conversion:

| Surface | One responsibility | Explicitly excluded |
|---|---|---|
| ECS `Camera`, `CameraMovement`, `LocalTransform`, `Visibility` | authoritative per-entity values in independently traversable columns | input callbacks, active-camera policy, renderer objects, save ownership |
| `SceneCameraView` | ephemeral generation-validated read/focused-write view bound to one `EntityId` | movement math, mouse/keyboard state, implicit rebinding to whichever camera is active, collection ownership |
| `SceneCameras` | enumerate cameras, create camera entries, and select the active camera | navigation behavior, duplicated camera fields, render extraction policy |
| `GameCameraController` | own input intent and first-person navigation policy, then commit one transform/settings edit through a view | camera storage, editor inspection, final render matrices |
| `CameraSnapshot` / Renderer `RenderCamera` | immutable game-to-render publication and renderer-owned derived view state | mutable GameFramework access and controller behavior |
| `SceneMeshView` / `SceneMeshes` | entity-bound mesh instance edits / mesh collection and snapshot boundary | animation-system execution; morph application is world-private |
| `SceneLighting` | light collection, focused light value edits, and snapshot/save boundary | lighting simulation, renderer light ownership, controller behavior |

Temporary facade deletion ledger:

| Temporary body | Why it still exists after Prompt 07 | Required replacement and deletion gate |
|---|---|---|
| `SceneCameras` / `SceneCameraView` | current controller, editor outliner/inspector, active-camera selection, and level save still need a narrow boundary; deleting it now would expose private ECS or inflate `GameScene` with camera forwarding | Prompt 08 supplies immutable world reads; Prompt 10 replaces controller writes with the camera system; Prompt 11 replaces editor/save reads and writes with model/semantic commands. Delete the facade after its final Prompt 11 consumer migrates |
| `SceneLighting` | current level construction/save and editor light model/actions are synchronous consumers | Prompt 09 construction package owns load insertion; Prompt 11 model/commands own editor/save interaction; Prompt 12 render packets replace snapshots. Delete after the last Prompt 12 snapshot consumer migrates |
| `SceneMeshes` / `SceneMeshView` | Showcase motion, editor mesh edits/material variants, payload attachment, and one renderer diagnostic still consume the focused seam | Prompt 09 owns construction, Prompt 10 owns motion/animation writes, Prompt 11 owns editor commands, and Prompt 12 owns renderer/diagnostic read packets. Delete after the last Prompt 12 consumer migrates |
| `SceneAnimations` | none | deleted in Prompt 07 reconciliation; it must not be reintroduced |

The `GameScene` friend declarations are therefore a bounded private migration seam, not the final architecture. A friend is permitted only for a class listed above with live consumers and an explicit deletion gate. Adding another `Scene*` friend or compatibility consumer is forbidden. After Prompt 08, `GameWorld` is the sole public gameplay-world owner; compiled systems use typed query/resource access, editor consumes `WorldReadView`-derived models and submits semantic commands, and renderer consumes immutable render streams. The final architecture does not retain a family of forwarding scene containers.

Final world/scene vocabulary decision:

`Scene` is not a generic suffix for anything associated with a level. Epic uses `UWorld` as the top-level gameplay sandbox and hosts Mass entity management at world lifetime; NVIDIA Donut and AMD Cauldron use `Scene` for a graphics/scene representation. Sparkle needs both ownership domains, so the final names make the distinction explicit:

| Current family | Final production role | Required fate |
|---|---|---|
| `GameScene` + private `SceneWorld` | one authoritative gameplay/ECS world owner | Prompt 08 renames the public root to `GameWorld` and folds/renames `SceneWorld` as private `GameWorldState`; there must not be two apparent world owners |
| `SceneCameras`, `SceneLighting`, `SceneMeshes`, `SceneCameraView`, `SceneMeshView` | temporary synchronous anti-corruption adapters | delete through Prompts 10-12 as listed above; systems use queries/resources, editor uses model/commands, renderer uses streams |
| `SceneSky` | mutable world-global authored environment | replace with typed `SkyEnvironment` world resource plus semantic command/read publication; do not manufacture a collection facade or fake per-frame snapshot owner |
| `SceneMaterials`, `SceneTextures`, `SceneSkeletons` | mutable vectors currently standing in for immutable asset resources | replace during Prompt 09/12 with generation/version-validated `MaterialResourceStore`, texture asset handles/residency, and `SkeletonResourceStore`; components and packets carry handles, never vector indices or mutable references |
| `SceneMaterialVariants` | authored variant definitions plus bindings and current world selection | split into immutable `MaterialVariantSet` resource and world-owned active-variant state/system command; bindings use stable entity/resource identity, not mesh vector position |
| `GameSceneController`, `GameCameraController`, `ShowcaseSceneController` | arbitrary callback/update compatibility | delete in Prompt 10 after compiled `GameSystem` conversion; camera input collection may remain a host service, but movement is a system |
| `GameSceneSnapshot`, `CameraSnapshot`, `MeshSnapshot`, `LightingSnapshot`, `MaterialSnapshot`, `TextureSnapshot`, `SceneAnimationSnapshot`, `SceneSkySnapshot` | temporary whole-scene/render compatibility copies | delete in Prompt 12 when `RenderWorldDelta` and `RenderFrameDynamicData` own the renderer boundary; do not add new snapshot fields meanwhile |
| `SceneAnimation*` and `SceneSkeleton*` runtime names | mixed resource definition and derived output vocabulary | Prompt 09/10 separates `AnimationClipResource`, `SkeletonResource`, `AnimationState`, and bounded pose/morph/skinning outputs; remove `Scene` prefix where the value is not a scene owner |
| `SceneCameraEntry`, `SceneLightDesc`, `SceneMeshKind`, `SceneMeshInstanceIndex` | compatibility translation/index vocabulary | Prompt 09 translates authored descriptions into typed entity blueprints/resources; Prompt 10/11 use `EntityId` and typed component/model values. Delete vector-index identity and remove `Scene` prefixes from values that are not scene owners |
| private `SceneAnimationSampler`, `SceneAnimationPoseEvaluator`, `SceneMorphWeightEvaluator`, `SceneLightingSnapshotBuilder` | implementation mechanisms named after their old container/snapshot path | Prompt 10 renames focused animation mechanisms to `AnimationSampler`, `AnimationPoseEvaluator`, and `MorphWeightEvaluator`; Prompt 12 deletes the snapshot builder in favor of extraction/packet builders |
| `SceneAssetPayload`, cooked/imported scene records | scene-asset ingestion format | source/cooked `Scene` terminology is valid at the asset boundary; Prompt 09 translates it into immutable `SceneLoadPackage`/entity blueprints and prevents it from becoming runtime authority |
| `EditorSceneModel`, editor scene inspectors/outliner | immutable editor projection and presentation | `EditorSceneModel` remains a valid final name because it is explicitly editor-owned; direct `SceneObjectActions` becomes semantic world commands and selection becomes stable entity selection |
| Renderer `RenderScene*`, ray-tracing scene, scene passes | renderer-owned representation or rendering stage | valid final terminology when prefixed by `Render` or scoped under Renderer; it must never alias or retain `GameWorld` storage |

Canonical end-state data flow:

`SceneAsset/CookedScene -> SceneLoadPackage -> GameWorldState(ECS + immutable resource handles) -> WorldReadView/WorldChangeJournal -> GameSystemGraph + EditorSceneModel + RenderWorldDelta/RenderFrameDynamicData -> RenderWorld/GPU scene`

This is a deletion plan, not permission to maintain dual APIs. Each replacing prompt must remove the old `Scene*` body and its `GameWorld` friendship as soon as its last consumer migrates. Compatibility aliases between `GameScene` and `GameWorld`, or between snapshots and render streams, are forbidden after the corresponding gate.

The split is intentionally adapted rather than copied wholesale. NVIDIA Donut documents that movement methods belong to derived application camera behavior while its engine scene camera is a separate type; Donut's `FirstPersonCamera` owns input state and performs navigation updates. Epic separates a camera component/view target from `PlayerController` ownership and `PlayerCameraManager` final-POV production. AMD Cauldron separates `CameraComponentData`, entity component management, and derived camera matrices, although its sample-oriented component also processes input. Sparkle keeps the recognizable roles while moving authoritative values into ECS columns and keeping input policy in `GameCameraController`, which is the better fit for the repository's DOD and future system scheduling requirements. Sources: [NVIDIA Donut Camera.h](https://github.com/NVIDIA-RTX/Donut/blob/main/include/donut/app/Camera.h), [NVIDIA Donut Camera.cpp](https://github.com/NVIDIA-RTX/Donut/blob/main/src/app/Camera.cpp), [Epic `APlayerCameraManager`](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/APlayerCameraManager), [AMD Cauldron `CameraComponent`](https://gpuopen.com/manuals/fidelityfx_sdk/fidelityfx_sdk-class_cameracomponent/), and [AMD Cauldron `CameraComponentMgr`](https://gpuopen.com/manuals/fidelityfx_sdk/fidelityfx_sdk-class_cameracomponentmgr/).

The old owning `Component`, `CameraComponent`, `MeshComponent`, `StaticMeshComponent`, `SkeletalMeshComponent`, mesh-component factory, and morph-weight applicator were deleted. The duplicate, uncompiled `ShowcaseSceneBehavior.cpp` predecessor was also removed; the compiled `ShowcaseSceneController` now retains generational mesh entities rather than vector indices. The replacement `SceneAssetMeshInstanceBuilder` performs deterministic payload-to-instance translation, after which only `SceneWorld::AddMesh` creates resources and component composition. Editor camera/light/mesh selections now carry `EntityId`; dense swap removal can reorder pools without retargeting durable selections.

Field classification and one-source result:

| Previous field/domain | Classification and authority after Prompt 07 | Derived/compatibility behavior |
|---|---|---|
| camera position/rotation/scale | `LocalTransform`; ECS is authoritative | direction is derived on snapshot/read and is never cached as authored state; `WorldTransform` is the explicit derived column prepared for Prompt 08 |
| camera projection, clip planes, aspect | `Camera`; ECS is authoritative | entity-bound `SceneCameraView` translates focused value commands; controller and collection policy remain separate |
| camera navigation tuning | `CameraMovement`; ECS is authoritative | controller reads a value and writes through the camera adapter |
| mesh transform/material/classification/resource/source | `LocalTransform`, `MeshInstance`, and `Visibility`; ECS is authoritative | `MeshInstance` holds a generation-validated resource handle, never `Mesh*` ownership; `SceneMeshView` is an ephemeral entity adapter |
| mesh geometry/assets | `SceneMeshResources`; world resource authority | static data is resource data; current skeletal mesh geometry is a renderer-facing derived cache |
| morph weights and skeleton binding | generation-validated deformation state referenced by `MorphState`; `SkinningState` carries binding identity | applying weights updates authoritative deformation state before refreshing derived skeletal geometry |
| light kind/photometry/shape/direction/shadow | `Light`; ECS is authoritative | `SceneLightDesc` is reconstructed at the API/save boundary, not stored beside ECS |
| light transform/visibility/name | `LocalTransform`, `WorldTransform`, `Visibility`, and `Name` | snapshot builders consume reconstructed value descriptions |
| animation time/rate/play/loop | `AnimationState`; ECS is authoritative | clip definitions live in `SceneAnimationResources`; pose/morph snapshots are explicitly derived output |
| names, authored source, editor flags | cold `Name`, `AuthoredIdentity`, and `EditorMetadata` columns | `{source asset, source object, kind}` remains distinct from runtime `{slot,generation}` |
| mesh-instance groups | immutable scene resource metadata | capture derives visible instance ranges; no mutable instance fields live in the group table |

Rule 13 access inventory:

| Data source / transform / hot traversal | Owner and layout decision | Stable identity and deterministic transform | Exact precedent and falsifier |
|---|---|---|---|
| level camera/light descriptions -> live world | `GameScene` owner inserts per-type packed components in authored order; hot state is separate from cold strings/editor flags | runtime `EntityId` is generational; `AuthoredIdentity` separately records source identity; desc/component conversion is value based | Epic Mass data-only fragments and EnTT sparse-set storage at pinned `1333fa5`; disposable level/capture comparisons falsify field loss or duplicate authority |
| scene payload -> mesh instances | builder validates asset indices and emits ordered construction records; world resource store owns meshes while packed `MeshInstance` stores handles | payload order, source asset ID, source node ID, and group offsets determine insertion/output; no task or pointer order participates | Epic Mass fragment/resource separation and Fabian access-driven layout rules recorded in J; two-instance payload parity and post-swap stable-entity checks falsify translation/identity errors |
| mesh pools -> `MeshSnapshot` -> renderer | dense `MeshInstance` traversal joins `Visibility`/`LocalTransform` by entity; snapshot is derived output and Renderer remains unaware of ECS storage | component dense order is deterministic between structural commits; group ranges are rebuilt from accepted visible instances | Epic game/render proxy separation plus EnTT packed traversal; focused Renderer compile and disposable snapshot value/count checks falsify the adapter |
| light pools -> `SceneLightDesc`/`LightingSnapshot` | packed hot light/transform/visibility columns plus cold name; descriptions are boundary values | light entity and authored identity survive unrelated dense reordering; kind-specific payload reconstruction is explicit | Epic Mass hot/cold fragments and existing Sparkle lighting snapshot contract; point-light name/transform/color/intensity/shape parity falsifies conversion |
| clip resources + `AnimationState` -> pose/morph output | clips are resource definitions, playback is a packed per-entity value, outputs are rebuilt derived vectors | insertion order and entity identity select resource handles; playback advances serially without object callbacks | Epic Mass data/resources and Unity ECS component/job separation recorded in J; clip count/update/capture and later Prompt 08 pose-journal tests falsify the split |
| editor/Showcase mutation | focused facade commands update exact component values; durable targets store `EntityId` rather than dense index | generation validation rejects stale targets and swap removal cannot silently retarget | EnTT generational identity and Epic editor/runtime separation; destroying the first of two mesh entities while continuing to mutate/read the second falsifies index coupling |
| resource resolution/unload | world-private mesh/deformation stores own slots; components store `{slot,generation}` handles | removal clears the resource, advances generation, and only then permits slot reuse; no component-facing raw pointer is durable | generational resource-handle practice required by J and NVIDIA/AMD resource-lifetime examples; destroyed-entity/resource-view rejection and stale generation resolution falsify dangling access |

Rule 12 structure: ECS storage and schemas remain in `Private/World/ECS`; world orchestration is split by domain across `SceneWorldCameras.cpp`, `SceneWorldMeshes.cpp`, `SceneWorldLighting.cpp`, and `SceneWorldAnimations.cpp`; conversion-only transform helpers live in `SceneWorldTransforms.*`; mesh, animation, and deformation ownership each have focused files under `Private/World/Resources`; payload translation is named `SceneAssetMeshInstanceBuilder.*`. No new god file, public ECS SDK, reflection system, or Renderer/Editor storage include was introduced.

Validation used one disposable `Prompt07SceneParitySmoke` target and removed its source and CMake membership afterward. One execution performed twenty complete world lifetimes covering level camera/light conversion, scene payload insertion, two mesh resources/instances, visibility mutation, animation resource/playback creation, camera switching, world snapshot/save-boundary capture, deletion of the first packed mesh, and continued access to the second by `EntityId`; all iterations returned success. Focused `SparkleGameFramework`, `SparkleEditor`, Renderer dependency, and `ShowcaseRuntime` DevelopmentEditor targets compiled successfully. No whole-repository build or permanent test/reporting surface was added.

## Prompt 08 — Make Transform/Derived State Explicit and Publish the Change Journal

Target CL Title: `SparkleGameFramework: Publish Explicit Derived State through the Change Journal`

~~~text
Implement Prompt 08 only after Prompt 07 passes.

Objective:
Remove write-on-read transform/camera caches, evaluate derived world state in explicit serial systems, and create the bounded sequenced world change journal/read publication that renderer/editor will consume.

Non-negotiable repository rules:
- Apply `Docs/Architecture/00-Review/L_SparkleEngineIntegrationStyleGuide.md` in full. Treat its ownership, implementation-shape, DOD, concurrency, rendering, naming, validation, structure, principal graphics engineering, and completion-report gates as acceptance criteria. Before editing, list the applicable `PGE-01` through `PGE-15` requirements from A, interpret them through H, and classify each as advance, preserve, not applicable, or blocked with expected evidence. Inspect the current repository, reconcile the whole touched ownership path, verify any AI-assisted work independently, delete replaced paths in the same change, reject role-only scaffolding, and report any justified exception explicitly with its owner, scope, evidence, and deletion or review gate.
- Apply Rule 13: audit every data source, transform, stream, packet, cache, table, upload, and hot traversal touched by this prompt; record the concrete access inventory, authoritative/derived ownership, layout decision, stable identity, deterministic transform, exact source precedent, and measured falsifier.
- Apply Rule 12: audit touched/new files for module and folder ownership, public/private placement, filename-to-primary-type alignment, and nearby misplaced counterparts; complete bounded moves/renames with includes, CMake, and documentation updated before the gate.
- Enforce J's canonical concurrency/rendering vocabulary and Rule 10: search canonical terms plus rejected aliases in the touched scope, use one responsibility per name, and delete temporary compatibility spellings before the gate.
- Search all Transform and camera direction/matrix getters, dirty flags, bounds calculations, snapshot extraction, editor reads, temporal history, and change notifications.
- Reuse existing math/identity/scene-generation types; do not create duplicate matrix conventions.
- Refactor every touched lazy getter into a true read or explicit evaluation owner.
- Do not build transform hierarchy scheduling until current runtime hierarchy data has an actual consumer and cycle contract.

Required implementation:
1. Keep LocalTransform as owner-mutated TRS input. Add explicit TransformEvaluationSystem writing WorldTransform (and inverse/derived data required by current consumers).
2. Add explicit CameraDerivedState evaluation or compute immutable camera direction/view data in a declared phase; const reads never mutate.
3. Compact dirty entity IDs/ranges deterministically and evaluate only required outputs in serial reference mode.
4. Add bounded WorldChangeJournal with sequence number, entity generation, typed create/destroy/component/value changes, and consumer acknowledgements.
5. Define lagging-consumer recovery: request a full immutable baseline and resume from a sequence; never read reclaimed journal storage.
6. Publish WorldReadView generation with only current editor/game query fields. Use release/acquire publication and generation-pinned storage/reclamation.
7. Drive initial render-delta extraction and editor model update inputs from the committed journal, not extra full mutable scans where avoidable.
8. Establish the final owner vocabulary: rename `GameScene` to `GameWorld`, replace the competing private `SceneWorld` name with private `GameWorldState` (or fold it into the `GameWorld` implementation), and update scopes, files, CMake, documentation, and consumers without a compatibility alias. `GameWorld` owns lifecycle/commit/publication; `GameWorldState` owns storage and private domain operations.
9. Move `SceneSky` authority into a typed `SkyEnvironment` world resource published through the same read/journal contract. Delete mutable pointer access and the `SceneSky` wrapper once editor compatibility is accounted for by Prompt 11 commands.

Validation:
- Concurrent-read stress proves no lazy transform/camera write remains.
- Serial evaluated matrices/directions match baseline within defined floating-point tolerance.
- Dirty/no-change frames emit expected journal work; static world produces no structural churn.
- Journal replay and full baseline + deltas produce identical world/read state.
- Delayed reader falls outside retention, resynchronizes, and never accesses reclaimed memory.

Acceptance gate:
- L Sections 19-21 (reconciliation, acceptance checklist, and required completion report) and Section 24 (Principal Graphics Engineering review gate) pass for the entire touched changelist, including directly affected neighboring ownership paths; no undocumented exception remains.
- Published component/read data is immutable for its generation.
- All current transform/camera consumers use explicit evaluated outputs.
- Journal/read publication has bounded storage and tested reclamation.
- No general event-sourcing framework or public diagnostics snapshot was added.
- `GameScene` and private `SceneWorld` no longer coexist as competing owner names; no new `Scene*` runtime facade was introduced.

Positive patterns: explicit derived-data phase, dirty ranges, sequenced typed journal, RCU-like bounded publication.
Forbidden: mutable const cache, full shared-scene mutex, unbounded history, editor/renderer direct registry traversal.
~~~

### Prompt 08 completion record

Prompt 08 is complete. `GameWorld` is now the only public gameplay-world owner name and private `ECS::GameWorldState` is the only storage/domain-operation owner. The old `GameScene` and private `SceneWorld` files, types, variables, includes, and compatibility spellings were removed together; current C++ search finds none. `GameWorld` owns level lifecycle, controller phases, commit, journal publication, and public read acquisition. `GameWorldState` owns the registry, world resources, dirty collection, explicit derived evaluation, and immutable generation construction. Legitimate scene-asset, editor-scene, and renderer `RenderScene*` names remain because those identify different domains rather than a competing world owner.

`Transform` is now a pure TRS value: its const matrix and inverse-transpose reads calculate without mutable caches. `TransformEvaluationSystem` and `CameraDerivedStateEvaluationSystem` are focused serial phase implementations under `Private/World/Systems`; they consume sorted/unique dirty `EntityId` values and write `WorldTransform` plus `CameraDerivedState` before publication. Camera, mesh, and light construction installs derived-output components but does not treat their placeholder values as authoritative. Local transform mutation only marks input dirty; the `GameWorld` commit boundary performs evaluation. No transform hierarchy was introduced because the runtime still has no hierarchy owner or cycle contract.

`WorldChangeJournal` is a private bounded mechanism under `Private/World/Publication`. Each committed record carries a monotonic `WorldSequence`, generational `EntityId`, `WorldChangeKind`, and `WorldDataKind`. Pending changes and dirty IDs are capacity-bounded; overflow collapses to one `WorldReset` and forces a full baseline rather than allocating an unbounded fallback or publishing a partial delta. The journal retains at most 64 immutable batches of at most 4,096 records, filters partially acknowledged batches, and protects only its narrow batch metadata with a mutex. Readers retain shared immutable batch storage, so eviction cannot reclaim an observed batch. A cursor older than the retained interval receives `ResyncRequired`, acquires the current full `WorldReadView`, acknowledges its sequence, and resumes from later deltas.

`WorldReadView` is a deliberately narrow public cross-module value contract containing current camera, light, mesh, and sky editor/game fields. `GameWorldState` copies the previous immutable generation and patches only changed stable entities; a reset, initial publication, or journal overflow rebuilds a full baseline. Publication uses `std::atomic<std::shared_ptr<const Storage>>` release/store and acquire/load. The shared generation pins reclamation without exposing registry/storage/query APIs. `GameWorld::CaptureSnapshot` now obtains active-camera data from the committed read generation, and the editor outliner builds its model from one pinned `WorldReadView` per UI build rather than directly enumerating mutable world storage. The remaining whole renderer snapshot and editor mutation facades are explicit Prompt 11/12 deletion targets, not new consumers of ECS internals.

`SkyEnvironment` is now authoritative typed world-resource data in `GameWorldState` and participates in the same journal/read publication. `SceneSky` remains only as the previously declared Prompt 11 non-owning command/read adapter: it stores no sky data, returns descriptions by value, and exposes no mutable pointer. This prompt did not create another `Scene*` runtime facade.

Preserve ledger:

- preserved level loading, default/active camera behavior, camera navigation, Showcase motion, mesh/material visibility, light editing, sky editing/capture, animation/morph updates, renderer snapshot compatibility, editor selection, and save-boundary capture;
- preserved existing matrix convention and generational `EntityId`; no parallel scheduler, hierarchy, event-sourcing framework, public diagnostics/reporting API, or renderer thread was added;
- preserved the temporary focused `SceneCameras`, `SceneLighting`, `SceneMeshes`, and `SceneSky` command adapters only for their named Prompt 10/11 consumers.

Delete ledger:

- deleted `GameScene`, `GameSceneController`, `GameSceneSnapshot`, private `SceneWorld`, `SceneWorld*` domain filenames, old includes, and all `gameScene`/`m_gameScene` variable spellings; their canonical replacements are `GameWorld`, `GameWorldController`, `GameWorldSnapshot`, `GameWorldState`, and `gameWorld`/`m_gameWorld`;
- deleted mutable transform matrix caching, write-on-read invalidation, and camera direction recomputation in camera capture;
- deleted `SceneSky` mutable pointer access and wrapper-owned mutable sky authority;
- the old full `GameWorldSnapshot` renderer path and remaining `Scene*` editor/game adapters are not silently accepted as final: Prompts 10-12 retain their existing exact deletion gates.

Rule 13 access inventory:

| Data source / transform / publication | Authoritative and derived ownership / layout | Stable identity and deterministic rule | Exact precedent and measured falsifier |
|---|---|---|---|
| owner-written `LocalTransform` -> `WorldTransform` | packed `LocalTransform` is authoritative TRS input; packed `WorldTransform` is derived matrix/inverse-transpose output | dirty generational `EntityId` values sort and deduplicate before one serial evaluation; overflow evaluates the complete dense local-transform pool | Epic Mass fragment/processor separation and Fabian access-driven layout in J; disposable validation checked translated matrices and static-frame non-publication |
| camera local rotation -> `CameraDerivedState` | camera direction is a derived packed column, never a mutable const cache or controller-owned duplicate | the same committed dirty order evaluates quaternion-rotated normalized +Z after transforms | NVIDIA Donut camera/application separation, AMD Cauldron scene-camera derived view data, and Epic component/controller/view separation recorded in J; disposable validation checked unit direction and generation-only visibility |
| component/resource mutations -> `WorldChangeJournal` | owner-thread pending records are bounded; journal batches are immutable retained publications | sequence assignment occurs only in commit order; records carry generational entity identity and typed kind/data, never pointer or completion order | Epic game/render dirty-proxy model (`EPIC-OWN`) and Donut persistent scene dirty-state interface at pinned `bc1ea24`; partial-ack replay and 70 delayed publications falsified duplication and retention errors |
| committed state -> `WorldReadView` | current immutable generation owns sorted camera/light/mesh columns plus optional sky; previous published generation is copied and patched by changed ID | full baselines sort by `EntityId`; incremental erase/upsert uses the same ordering; release/acquire publishes one complete generation | C++ release/acquire ownership publication and Epic proxy isolation in J; four concurrent readers plus retained old views falsified torn publication and early reclamation |
| sky edit/load -> `SkyEnvironment` | `GameWorldState` owns one typed world resource; `SceneSky` is a non-owning value/command adapter | resource changes sequence with component changes; absent/present state is copied into each published generation | Epic Mass subsystem/resource separation and AMD/NVIDIA scene resource separation in J; editor and level capture compile/behavior validation falsified a second mutable authority |
| published view -> editor/renderer compatibility | editor outliner uses one pinned generation; active render camera snapshot is built from the committed generation, while remaining renderer data stays on the named Prompt 12 bridge | stable `EntityId` replaces dense-position identity and a view sequence identifies its exact commit | Epic game/render proxy separation (`EPIC-OWN`); focused Editor/Renderer/Showcase build plus disposable camera publication checks falsified direct registry leakage and stale camera reads |

Rule 12 structure and naming reconciliation: stable cross-module contracts live in `Public/World/{GameWorld,GameWorldController,GameWorldSnapshot,WorldChange,WorldReadView,SkyEnvironment}.*`. Storage and orchestration remain private in `Private/World/GameWorldState.*` and domain-specific `GameWorld{Cameras,Lighting,Meshes,Animations}.cpp`. `Private/World/Systems` owns the two explicit derived evaluators; `Private/World/Publication` owns journal/read storage mechanics; `WorldTransformConversion.*` owns only public/ECS transform conversion. No renderer/editor module includes a private ECS header. The touched code contains no local class/struct declaration, new god file, compatibility header, or duplicate settings/diagnostics family.

Validation was proportionate to the changed boundary. One focused DevelopmentEditor build compiled `SparkleGameFramework`, its Renderer/Editor dependencies, and `ShowcaseEditor`. One disposable `Prompt08WorldPublicationValidation` target then checked static no-change frames, explicit transform/camera evaluation, immutable old-generation pinning, partial journal acknowledgement, four concurrent readers during 70 owner publications, bounded retention, and `ResyncRequired`; it returned success. Its source and CMake target were deleted and CMake was regenerated afterward. No full repository/backend matrix, permanent test product, report framework, or extra product logging was added.

## Prompt 09 — Implement Transactional Asynchronous Scene Loading

Target CL Title: `SparkleGameFramework: Implement Transactional Asynchronous Scene Loading`

~~~text
Implement Prompt 09 only after Prompts 06 and 08 pass.

Objective:
Replace destructive synchronous level loading with a scoped read/decode/validate/assemble pipeline that produces immutable EntityBlueprint packages and commits atomically while preserving the old scene on failure/cancellation.

Non-negotiable repository rules:
- Apply `Docs/Architecture/00-Review/L_SparkleEngineIntegrationStyleGuide.md` in full. Treat its ownership, implementation-shape, DOD, concurrency, rendering, naming, validation, structure, principal graphics engineering, and completion-report gates as acceptance criteria. Before editing, list the applicable `PGE-01` through `PGE-15` requirements from A, interpret them through H, and classify each as advance, preserve, not applicable, or blocked with expected evidence. Inspect the current repository, reconcile the whole touched ownership path, verify any AI-assisted work independently, delete replaced paths in the same change, reject role-only scaffolding, and report any justified exception explicitly with its owner, scope, evidence, and deletion or review gate.
- Apply Rule 13: audit every data source, transform, stream, packet, cache, table, upload, and hot traversal touched by this prompt; record the concrete access inventory, authoritative/derived ownership, layout decision, stable identity, deterministic transform, exact source precedent, and measured falsifier.
- Apply Rule 12: audit touched/new files for module and folder ownership, public/private placement, filename-to-primary-type alignment, and nearby misplaced counterparts; complete bounded moves/renames with includes, CMake, and documentation updated before the gate.
- Enforce J's canonical concurrency/rendering vocabulary and Rule 10: search canonical terms plus rejected aliases in the touched scope, use one responsibility per name, and delete temporary compatibility spellings before the gate.
- Audit LevelManager, LevelRegistry/Asset, SceneAssetManager/Registry, manifest/payload loaders/appenders/translators, file utilities, lifecycle events, editor level menu, asset residency, and save behavior.
- Audit and replace `SceneMaterials`, `SceneTextures`, `SceneSkeletons`, `SceneMaterialVariants`, and every resource vector/index exposed by the current world. They are not final ECS component containers.
- Extend existing load translators and registries; do not create Async duplicates or a second asset database.
- One SparkleTasks runtime, explicit BlockingIo/Background lanes, World/Document task scope.
- Refactor touched synchronous mutation/callback code and delete clear-before-load behavior.

Required implementation:
1. Capture immutable level request, catalog generation, request ID, world/document generation on the owner thread.
2. Load/publish immutable registry/catalog generation before concurrent ID resolution.
3. Read manifests/files on BlockingIo; decode/translate per asset in task-local contexts with weighted CPU/pending-byte limits.
4. Produce deterministic EntityBlueprint component records with stable authored identity and component schema versions.
5. Fan-in, resolve cross-asset references, assign deterministic final order/handles, and validate a complete immutable SceneLoadPackage.
6. Marshal completion to owner. Reject stale request/document/world/catalog generation.
7. Bulk commit ECS entities/components through world commands only after validation; old scene remains active until acceptance.
8. Lifecycle events/results dispatch on owner thread after invariants hold. Workers never call UI, transaction, `GameWorld` mutation, or arbitrary Event callbacks.
9. New request cancels older unpublished work; published/GPU-enqueued resources follow generation retirement rather than immediate free.
10. Expose bounded/coalesced operation progress and typed diagnostics through EditorOperationService-compatible result contracts.
11. Translate loaded materials, textures, skeletons, animation clips, and material variants into immutable generation/version-validated resource stores before entity commit. Entity components and variant bindings receive stable handles/EntityId mappings; the old `SceneMaterials`, `SceneTextures`, `SceneSkeletons`, and vector-index binding owners are deleted after parity.

Validation:
- Serial/parallel package IDs/order/content/diagnostics parity.
- Cancel before read, during read/decode, fan-in, after package publish, while commit queued, document close, newer request wins.
- Missing/corrupt registry/manifest/mesh/material/animation/skeleton keeps active scene and renderer state.
- Repeated load/cancel/reload stress with memory limit and delayed workers.
- Editor/application remains responsive and the FrameCritical lane meets budget during load.

Acceptance gate:
- L Sections 19-21 (reconciliation, acceptance checklist, and required completion report) and Section 24 (Principal Graphics Engineering review gate) pass for the entire touched changelist, including directly affected neighboring ownership paths; no undocumented exception remains.
- No clear-before-success and no accepted partial scene.
- No mutable shared SceneAssetPayload/registry state across workers.
- Old scene survives every failed/stale/cancelled pre-commit path.
- Synchronous compatibility path is deleted after parity or has an exact Prompt 22 deletion gate with no new use.
- No mutable `Scene*` resource vector or public resource reference remains in `GameWorld`; scene-asset payload types stop at the load-package translation boundary.

Positive patterns: isolated construction, immutable package, generation validation, owner commit, failure containment.
Forbidden: nested synchronous loads from worker, UI/global callbacks during decode, task-completion-order IDs, unbounded decoded memory.
~~~

Prompt 09 implementation record (2026-07-19):

The destructive transition has been removed. `LevelManager` now captures request, world, document, and catalog generations on the owner; `SceneLoadExecutionService` owns one Document `TaskScope`; `SceneLoadTaskGraph` assigns manifest/referenced-file reads to `BlockingIo` and cooked mesh/material/skeleton/animation decode plus blueprint construction to `Background`; and `SceneLoadPackageBuilder` validates references and assembles request-indexed partial packages in stable order. A superseding request cancels the unpublished scope and cannot clear or replace the active world. The owner consumes the settled package, rejects stale generations, constructs a complete staged `GameWorldState` plus resource generation, and swaps only after construction succeeds. Lifecycle events remain owner-thread calls and failed/cancelled work emits no unload transition.

`RuntimeApplication` now owns the one `ApplicationTaskRuntime`. Level loading and editor shader recooking receive that executor and parent scope; the shader recook service's private executor was deleted. `task.SerialExecution` remains the deterministic oracle. Until Prompt 25 supplies machine/workload measurements, the application uses one FrameCritical, one BlockingIo, and one Background worker; `task.WorkerCount` provides explicit 1/2/N Background experiments without baking in a `hardware_concurrency()-N` policy. No loader pool, feature thread, callback bus, task panel, or diagnostic stream was added.

The old public `SceneMaterials`, `SceneTextures`, `SceneSkeletons`, and `SceneMaterialVariants` owners were deleted. `GameWorldResourceStores` privately groups focused material, texture, skeleton, and material-variant stores. Materials now carry a store generation in both `MaterialHandle` and `MaterialSnapshot`; payload-local generation-zero indices are translated to generation-validated runtime handles during staged construction. Editor material-variant UI uses narrow `GameWorld` name/count/apply operations and cannot retain or mutate a resource vector. Renderer access remains immutable `GameWorldSnapshot` publication.

Rule 13 access inventory:

| Touched data/access path | Authority and concrete layout | Stable identity and deterministic transform | Exact precedent and falsifier |
|---|---|---|---|
| level request and registry/catalog | owner-copied `LevelDesc`; immutable ordered `SceneAssetCatalog` generation; workers receive shared const catalog storage | request ID plus world/document/catalog generations; catalog lookup occurs before graph construction and duplicate/empty IDs reject | Epic asynchronous-level negative contract in J; stale/newer request or missing catalog entry must leave active world unchanged |
| cooked manifest and referenced files | each asset index owns one `LoadedSceneManifest`, one immutable-after-read `CookedAssetFileSet`, one payload, and one blueprint vector; no shared append target | input request index is the merge key; BlockingIo reads bytes, Background decodes existing formats/translators, and fan-in iterates request order | NVIDIA Donut Scene/Texture background loading at `bc1ea24` and AMD Cauldron2 content-task fan-in at `60f4ea8`; serial/parallel package identity/schema order comparison and cancellation probe |
| blueprint/component records | AoS is selected because construction/validation consumes complete per-entity records once; schema arrays contain existing stable `ComponentSchemaId` and version, not compiler type identity | authored asset ID plus source node/domain ordinal; schema order is fixed by entity kind | Epic Mass deferred construction principles plus Sparkle Prompt 05/06 schema/command contracts; shuffled completion may not change entity or schema order |
| retained load data | task-local raw files are released immediately after successful decode; decoded payloads remain until fan-in; request count is capped at 256 and weighted retained bytes at 512 MiB return controlled failure | no task-completion-order append and no unbounded fallback | J MT-23/29 and AMD/NVIDIA bounded content-task guidance; oversized admission/failure must settle without partial publication |
| world/resource commit | staged `GameWorldState` and private `GameWorldResourceStores` are the sole prospective generation; the current generation remains authoritative until the swap | `GameWorld` generation plus generation-bearing material handles; existing stable cooked asset IDs and ECS authored identities survive translation | Epic game/render ownership, NVIDIA Donut scene/resource separation, and AMD Cauldron data/backend separation; any decode/reference/stale failure must preserve old world and renderer snapshot |
| progress and lifecycle | four scalar progress fields are coalesced atomic state; final diagnostic is one bounded operation result; owner alone invokes lifecycle events | request ID identifies progress/result and stale completions are discarded | AMD Cauldron completion/fan-in and Epic owner-finalization guidance; worker UI/Event calls and unload events on failed requests are repository-search failures |

Rule 12 reconciliation: public module surface is limited to the dedicated `LevelLoadOperation` progress contract, narrow `GameWorld` operations, snapshots, resource handles, and the cooked `SceneAssetRegistry` format deliberately shared with `SceneCooker`. Catalog, execution state, graph construction, package building, cooked byte staging, payload DTOs, and resource stores live in responsibility-named private folders/files. `SceneCooker` no longer receives GameFramework's private include directory. The one-method `SceneAssetManager` wrapper was deleted; `SceneLoadExecutionService` owns execution lifetime and retains the immutable catalog prerequisite directly, while `SceneAssetCatalog` owns lookup and construction from the existing registry. Five uncalled synchronous `AssetLoader::Load(path, ...)` compatibility entries were deleted, leaving `CookedAssetFileSet` as the sole file-read owner and byte-only `Decode` as the sole cooked-loader entry. The duplicate private `SceneLoadStage`/`SceneLoadProgress` family was folded into `LevelLoadOperation`. The write-only `SceneAssetPayloadDiagnostics` counters and unused count helpers were deleted; camera/light translation now has a responsibility-matching appender name. Staging and material-variant stores mutate private `GameWorldState` directly, so the public `SceneMeshes` facade no longer grants construction/resource-store friendship or owns hidden append methods. Staging-only `GameWorld` construction no longer invokes controllers or publishes after every asset; one owner-side finalization follows the atomic swap. Activated task controls live in `TaskRuntimeCVars.cpp`, separate from dormant renderer launch controls. No touched/new implementation file declares a local class/struct, and no compatibility `Async*`, manager, public payload DTO, resource-vector, or second executor family remains.

Second cleanup gate: the mixed `SceneAssetPayloadLoader` was deleted and replaced by `SceneAssetFileReader` under the BlockingIo phase and `SceneAssetPayloadDecoder` under the Background phase. `SceneLoadPackageBuilder::BuildAssetBlueprints` no longer claims to decode, `SceneMaterialVariantTranslator` no longer claims to apply, and `GameWorldSceneAssetCommitter` is the only decoded-asset-to-staged-world mutation owner. The obsolete caller-managed material-base accumulator, unused completed-read counter, generic private `Implementation` names in touched execution services, and no-op shader-recook preparation task were deleted. Raw `CookedAssetFileSet` storage now resets after successful decode and retained-byte accounting is decremented exactly by the released raw byte count.

Focused validation built `ShowcaseEditor` in `DevelopmentEditor`. A disposable `Prompt09Validation` executable loaded Sponza through both the zero-worker serial executor and a 1/2/1 FrameCritical/Background/BlockingIo executor, compared authored identity plus component schema ID/version order, then proved immediate scope cancellation settled as `Cancelled`; it returned success and its source/CMake target were removed. The host policy blocked launching the editor executable, so no interactive frame-latency or D3D12/Vulkan visual claim is made here. `git diff --check`, the architecture boundary target, and final canonical/legacy-owner searches close the non-interactive gate.

## Prompt 10 — Build the ECS-Aware Game System Graph and Parallel Animation

Target CL Title: `SparkleGameFramework: Add the ECS System Graph and Parallel Animation`

~~~text
Implement Prompt 10 only after Prompt 08 passes; use Prompt 09 contracts where loading affects target generations.

Objective:
Replace the legacy `GameWorldController` arbitrary mutable-world access with an ECS-aware `GameSystemGraph` and prove the first real gameplay parallel workload through movement, animation pose, morph, skinning, transform, and extraction dependencies.

Non-negotiable repository rules:
- Apply `Docs/Architecture/00-Review/L_SparkleEngineIntegrationStyleGuide.md` in full. Treat its ownership, implementation-shape, DOD, concurrency, rendering, naming, validation, structure, principal graphics engineering, and completion-report gates as acceptance criteria. Before editing, list the applicable `PGE-01` through `PGE-15` requirements from A, interpret them through H, and classify each as advance, preserve, not applicable, or blocked with expected evidence. Inspect the current repository, reconcile the whole touched ownership path, verify any AI-assisted work independently, delete replaced paths in the same change, reject role-only scaffolding, and report any justified exception explicitly with its owner, scope, evidence, and deletion or review gate.
- Apply Rule 13: audit every data source, transform, stream, packet, cache, table, upload, and hot traversal touched by this prompt; record the concrete access inventory, authoritative/derived ownership, layout decision, stable identity, deterministic transform, exact source precedent, and measured falsifier.
- Apply Rule 12: audit touched/new files for module and folder ownership, public/private placement, filename-to-primary-type alignment, and nearby misplaced counterparts; complete bounded moves/renames with includes, CMake, and documentation updated before the gate.
- Enforce J's canonical concurrency/rendering vocabulary and Rule 10: search canonical terms plus rejected aliases in the touched scope, use one responsibility per name, and delete temporary compatibility spellings before the gate.
- Audit GameWorldController, GameCameraController, ShowcaseSceneController, world-private animation evaluators/samplers, remaining SceneMeshes compatibility access, skeleton data, transform/extraction phases. `SceneAnimations` was already deleted and must not be recreated.
- Search for graph/access/resource-domain counterparts before adding system descriptors.
- Use SparkleTasks; do not add an ECS scheduler, controller pool, animation threads, or worker waits.
- Refactor whole-scene mutable APIs and delete migrated controller paths in this prompt.

Required implementation:
1. Define GameSystem descriptors: stable ID/name, phase, typed component query Read/Write access, non-ECS resource access, explicit prerequisites, execution/grain policy.
2. Compile component/resource hazards: read/read may overlap; conflicting access orders by phase/dependency or rejects ambiguity/cycles.
3. Execute the complete graph serially and prove state/render parity.
4. Convert camera input/movement and Showcase motion to narrow ECS systems over stable EntityId targets.
5. Pre-resolve clip-to-skeleton and morph targets per asset generation. Preallocate stable pose/morph/skinning output slots.
6. Partition leading component/query ranges. Each task writes exclusive slots/ranges; no per-task shared vector push or asset mutation.
7. Deterministically merge by system/entity/partition key, commit outputs, then run transform/derived state and extraction phases.
8. Add small-work serial thresholds and reusable/per-execution arenas; remove steady-state nested target scans and per-joint heap churn where measured/touched.
9. Delete GameWorldController::Update(GameWorld&) and the temporary legacy adapter after current camera/Showcase consumers migrate.
10. Delete `GameWorldController`, `GameCameraController`, `ShowcaseSceneController`, and migrated camera/mesh animation facade writes rather than retaining callback adapters. Input collection publishes intent; systems consume it through declared resource/component access.
11. Rename remaining animation mechanisms by their actual operation (`AnimationSampler`, `AnimationPoseEvaluator`, `MorphWeightEvaluator`, animation output storage) and remove the misleading `SceneAnimation*` implementation family. Do not replace it with one animation god subsystem.

Validation:
- Graph compile rejects cycle, duplicate system, undeclared/conflicting access, unavailable phase resource.
- Fixed input sequences match at serial, 1/2/N workers, randomized completion/delay.
- Camera navigation, Showcase PTLAS motion, animation pose, morph weights, skinning matrices, transforms, extraction all match defined tolerance/order.
- Negative tests prove worker cannot mutate structure, broadcast events, call UI/renderer/RHI, or retain component view.
- Animation-heavy benchmark shows crossover and critical path; tiny scene stays within overhead budget.

Acceptance gate:
- L Sections 19-21 (reconciliation, acceptance checklist, and required completion report) and Section 24 (Principal Graphics Engineering review gate) pass for the entire touched changelist, including directly affected neighboring ownership paths; no undocumented exception remains.
- Current systems use typed narrow access; no arbitrary mutable-world controller execution remains.
- Parallel output is deterministic/serial-equivalent.
- Work uses SparkleTasks and explicit dependencies, not waits or extra pools.
- Removed scans/allocations/controller paths are listed in deletion ledger.

Positive patterns: query-driven systems, access hazards, exclusive outputs, deterministic merge, grain threshold.
Forbidden: parallel virtual Entity::Update, shared morph/pose pushes, mutable asset data, completion-order merge, ECS auto-scheduling hidden from task graph.
~~~

Prompt 10 implementation record (2026-07-20):

`GameWorld::Update` now binds one immutable topology compiled by the private `GameSystemGraph` to the shared application `TaskExecutor`. Descriptors carry a stable FNV-1a system ID, canonical name, phase, typed `Query<Read/Write/...>` component metadata, coarse non-ECS resource access, explicit prerequisites, and range/grain policy. Compilation rejects empty identity/name, duplicate systems, duplicate or conflicting declarations, systems with no declared access, phase-invalid resources, missing/backward prerequisites, same-phase unordered write hazards, cycles, invalid grain policy, and task-graph rejection. Read/read nodes may overlap; phase edges and explicit prerequisites order all conflicts. The production topology is camera movement and Showcase oscillating motion; animation playback followed by independent pose and morph evaluation; skinning and morph commit; deterministic system-output commit; transform and camera-derived evaluation; mesh extraction and publication.

Input collection moved to Application-private `CameraInputIntentCollector`: platform events update a value intent, `RuntimeApplication` publishes that value before update, and the camera system consumes it against the stable active `EntityId`. `OscillatingMeshMotion` is a narrow component holding the fixed Showcase lane key/base transform; its system reproduces the PTLAS motion math without retaining mesh indices or calling `SceneMeshes`. `GameWorldController`, `GameCameraController`, `ShowcaseSceneController`, and their sources/CMake entries were deleted. There is no legacy controller registration or whole-world update callback.

Animation resources and outputs are separate responsibilities. Immutable `AnimationClipResource` and `SkeletonResource` live in generation-owned stores. `AnimationClipResourceStore::ResolveTargets` resolves skeleton handles and morph-channel indices once for an accepted resource generation. `AnimationOutputStorage::Prepare` runs only when the target generation or ECS structure version changes, sorts animation entities, preallocates pose-local/model/skinning arrays and fixed four-weight morph slots, builds O(1) animation-entity-to-work lookup, and pre-resolves morph targets plus skinning output handles. Authored scene-instance identity is distinct from the catalog asset ID: `SponzaPtlas.level` now gives its ten CesiumMan actors stable instance IDs that map to one catalog ID, `AuthoredIdentity` schema version 2 carries the stable instance hash, and pose/morph targets match instance plus skeleton/node rather than forming a duplicate-asset cross-product. Duplicate authored instance IDs still reject; immutable skeleton resources with the same cooked ID deduplicate in `SkeletonResourceStore`.

Range tasks write only preassigned dense/query spans or stable output slots. `GameWorldSystemRun` prepares storage content versions before submit, keeps the structure-frozen epoch alive through the settled host `Submit`, and exposes no scheduling operation to a system body. Camera, motion, playback, pose, morph, skinning, transform, camera-derived, and extraction work use fixed indices; no task appends to a shared vector or mutates an asset. Single-range commit nodes sort/deduplicate by `EntityId`; extraction sorts slots by entity before rebuilding group ranges. Pose and morph branches therefore remain scheduling-order independent. The graph is compiled once per world topology, per-execution vectors retain capacity, and the application worker override now applies to both FrameCritical and Background lanes of the same SparkleTasks executor.

Rule 13 access inventory:

| Touched data/access path | Authority, concrete layout, and hot access | Stable identity and deterministic transform | Exact precedent and measured falsifier |
|---|---|---|---|
| camera input intent | Application owns one scalar `CameraInputIntent`; GameFramework reads one captured value and writes only matching `Camera`, `CameraMovement`, and `LocalTransform` query rows | active generational `EntityId`; fixed input sequence and entity-sorted commit | J “Narrow System Views” and Epic Mass query-batch principle; SponzaPtlas serial/1/2/4 camera output matched within `1e-5` |
| system descriptors/topology | GameFramework-private AoS descriptors are cold compile data; immutable compiled edges/tasks are reused, while bindings are one-run callbacks/ranges | FNV-1a canonical system name; phase/prerequisite edges, never registration or completion order | J “ECS-Aware World System Graph” plus EPIC-TASK prerequisites/nested completion; duplicate/cycle/undeclared/conflicting/phase-invalid cases rejected |
| ECS component streams | authoritative sparse-set dense component/entity arrays; smallest included storage leads each typed query; write content versions are marked before fan-out | generational `EntityId`; leading-index partitions and entity-keyed merge | Epic MassEntity query batches and transient `FMassEntityView`, Unity/Mass deferred-structure precedent; frozen create/destroy/add rejected and a retained query returned `InvalidEpoch` |
| animation clip/skeleton targets | immutable resource-store entries; generation-time clip-to-skeleton handles and compact morph-channel index vectors replace frame scans | generation-bearing handles plus cooked asset ID and authored scene-instance ID | J “First GameFramework Parallel Workload” and data-only Mass resources; two duplicate asset instances produced two pose slots/two morph bindings, not four |
| pose and skinning work | stable AoS work slots with already-sized joint-local, model-space, and public skinning matrix vectors; one animation instance owns one slot/range | animation `EntityId`, authored instance, skeleton handle, output slot generation | AMD-CPU range-local data and J animation steps 3–6; 512 instances × 64 joints measured about `0.75–0.95 ms` serial versus `0.23–0.25 ms` at four workers |
| morph work and committed weights | generation-time sample/binding tables plus fixed-size per-instance `MorphWeightStorage`; sampled slots are exclusive, commit order is `(sample, target EntityId)` | animation entity + authored instance + source node + generation-bearing output handle | MT-26/29 exclusive output and deterministic fan-in; linear interpolation oracle produced four `0.5` weights and serial/worker output order matched |
| movement/transform/derived state | authoritative `LocalTransform` components; reusable fixed change slots merge to a sorted unique dirty-entity vector; `WorldTransform` and camera direction are derived outputs | stable `EntityId`; camera-system then motion-system order is explicit, transforms evaluate by sorted entity | J Transform Contract; actual PTLAS motion, transforms, camera-derived direction, and extraction matrices matched within `1e-5` at 0/1/2/4 workers |
| mesh extraction/publication | task ranges fill one preallocated `MeshSlot` per leading query row; deterministic commit rebuilds the existing immutable `MeshSnapshot` cache and instance-group ranges | entity sort, immutable mesh resource handle/cooked asset ID, group index only as extraction-local storage | AMD-PORT deterministic single-thread order and EPIC-OWN value separation; 113 SponzaPtlas mesh outputs matched serial and every worker count |
| level instance/catalog mapping | authored `SceneAssetId::value` is instance authority; optional `catalogValue` selects immutable cooked content; parser/writer preserve `instance|catalog` | duplicate instance text rejects, stable 64-bit instance hash is validated for collision, catalog generation remains Prompt 09 authority | Prompt 09 isolated-generation contract plus J authored/runtime identity separation; SponzaPtlas loaded ten CesiumMan actors without aliasing entity/output targets |

Rule 12 and naming reconciliation: `GameSystemGraph` and focused operation systems remain in `Private/World/Systems`, graph declarations are isolated under `Systems/Descriptors`, and phase-specific runtime capabilities are isolated under `Systems/Execution`; `GameWorldSystems.cpp` now prepares the reusable arena, binds those capabilities, and invokes the graph rather than implementing every phase. Query/epoch mechanics remain in `Private/World/ECS`; internal resource definitions, reusable animation operations, and output storage live in `Private/Animation`; resource stores remain in `Private/World/Resources`; extraction owns its private stable-slot cache; only renderer-facing animation output and cross-module input-intent value contracts are public. `SceneMeshResources.*` was renamed to filename/type-aligned `MeshResourceStore.*`. Runtime `SceneAnimation*`, `SceneMorph*`, and `SceneSkeleton*` implementation families were replaced by `AnimationSampler`, `AnimationPoseEvaluator`, `MorphWeightEvaluator`, `SkinningMatrixEvaluator`, `AnimationOutputStorage`, `AnimationClipResource`, and `SkeletonResource`; the remaining cooker implementation was subsequently renamed to `CookedAnimationAssetBuilder` and its manifest value to `CookedAnimationReference`. Searches find no `SceneAnimation*` implementation, old controller, old mesh-resource name, `Job`, `WorkItem`, controller adapter, animation thread/pool, or second scheduler. Prompt 11/12 then deleted the remaining `SceneMeshes` editor/snapshot bridge.

Deletion ledger:

- deleted `GameWorldController::{Update, OnSceneLoaded}` and both controller files; no adapter or registration vector remains;
- deleted `GameCameraController` input subscriptions, raw camera-view state, and whole-world update path; value-only input collection replaces it;
- deleted `ShowcaseSceneController`, its retained mesh-index list, per-frame facade traversal, and CMake entries; `OscillatingMeshMotionSystem` replaces the operation;
- deleted the private `SceneAnimationSampler`, `SceneAnimationPoseEvaluator`, `SceneMorphWeightEvaluator`, and `SceneAnimationDiagnostics` family plus public `SceneAnimation.h`;
- deleted `SceneAnimationResources` linear skeleton lookup and `SceneDeformationStateStore`; generation-resolved clip/skeleton stores and `MorphWeightStorage` replace them;
- deleted per-frame clip × channel × mesh morph target scans, mutable `SkeletalCookedMesh::SetMorphWeights`, and shared mesh-asset deformation writes;
- deleted pose-evaluator per-call joint/model vector construction; generation-preallocated joint arrays now survive steady-state frames;
- deleted legacy `SceneMeshResources` spelling after the bounded file/type rename.

Applicable failure-atlas closure: MT-01/02/04 use generational entity/resource/output identity and immutable extraction; MT-05/06 use settled `Submit` and epoch/run stack lifetime; MT-08/09 exclude Event/UI/renderer/RHI capabilities from worker system files; MT-10/17 retain the one application executor and its existing ordered shutdown; MT-11 adds no world mutex; MT-13/14 use graph prerequisites/nested completion and no worker wait/help loop; MT-16 runs no I/O on FrameCritical systems; MT-18 keeps explicit serial/1/2/N control; MT-19 uses measured serial thresholds/coarse ranges; MT-20/21 use SparkleTasks stealing and real pose/morph branch dependencies; MT-23/27 inherit Prompt 09 asset-count/byte bounds and allocate output strictly from accepted-generation counts; MT-26 eliminates task-side pushes/cursors; MT-28 keeps hot components and joint streams separate from cold editor/resource data; MT-29 sorts every semantic fan-in; MT-30 freezes registry structure and invalidates retained views; MT-31 deletes the controller/animation second owners. MT-41 remains limited by the Windows/MSVC sanitizer matrix, and MT-43 labels the timings as local characterization rather than a universal product claim.

Focused validation used disposable executables and removed their source/CMake targets afterward. The graph gate covered duplicate system, cycle, unordered hazard, conflicting declaration, undeclared access, unavailable phase resource, frozen structure, and retained-view rejection. Twelve randomized-yield seeds produced identical range output at serial, 1, 2, and 4 workers. A fixed 120-frame input sequence on the real `SponzaPtlas` level produced 113 meshes and 10 poses with camera, Showcase motion, playback, skinning matrices, transforms, and extraction matching serial at 1/2/4 workers within `1e-5`; synthetic morph sampling supplied the absent authored morph channel. Three repeated animation benchmarks measured tiny one-instance execution at about `0.0045–0.0050 ms` serial and `0.0317–0.0331 ms` on four workers, below the `2.5 ms` gate, while the heavy case crossed over as reported above. Two injected independent 20 ms animation branches joined in `25.3–35.5 ms`, demonstrating the graph critical path rather than a global serial barrier. A final focused fixture confirmed authored-instance pose/morph isolation. No interactive visual or GPU performance claim is made; the Renderer snapshot consumer was compiled for parity.

## Prompt 11 — Convert the Editor to Immutable Models and Semantic Commands

Target CL Title: `SparkleEditor: Adopt Immutable Models and Semantic Commands`

~~~text
Implement Prompt 11 only after Prompts 09 and 10 pass.

Objective:
Remove live `GameWorld` pointer/index mutation from editor panels. Make editor main own ImGui/selection/transactions, consume immutable `WorldReadView`-derived `EditorSceneModel`, submit stable `EntityId` semantic commands, and manage background workflows through one `EditorOperationService`.

Non-negotiable repository rules:
- Apply `Docs/Architecture/00-Review/L_SparkleEngineIntegrationStyleGuide.md` in full. Treat its ownership, implementation-shape, DOD, concurrency, rendering, naming, validation, structure, principal graphics engineering, and completion-report gates as acceptance criteria. Before editing, list the applicable `PGE-01` through `PGE-15` requirements from A, interpret them through H, and classify each as advance, preserve, not applicable, or blocked with expected evidence. Inspect the current repository, reconcile the whole touched ownership path, verify any AI-assisted work independently, delete replaced paths in the same change, reject role-only scaffolding, and report any justified exception explicitly with its owner, scope, evidence, and deletion or review gate.
- Apply Rule 13: audit every data source, transform, stream, packet, cache, table, upload, and hot traversal touched by this prompt; record the concrete access inventory, authoritative/derived ownership, layout decision, stable identity, deterministic transform, exact source precedent, and measured falsifier.
- Apply Rule 12: audit touched/new files for module and folder ownership, public/private placement, filename-to-primary-type alignment, and nearby misplaced counterparts; complete bounded moves/renames with includes, CMake, and documentation updated before the gate.
- Enforce J's canonical concurrency/rendering vocabulary and Rule 10: search canonical terms plus rejected aliases in the touched scope, use one responsibility per name, and delete temporary compatibility spellings before the gate.
- Audit UI host services, SceneObjectSelection/Actions/Presentation, outliner entries/panel, all inspectors, material variants, level menu, viewport, shader recook, preview/search/save/package workflows.
- Before adding model/command/operation types, search for existing selection, transaction, status, request/result, preview, and restart/recook services; consolidate rather than duplicate.
- No editor thread pool, raw ECS registry access, live `GameWorld` pointer in panels, task debugger, or renderer-cache browser.
- Refactor each migrated panel completely: read model + command + result + undo/redo; delete direct mutation helper.

Required implementation:
1. Build generation-pinned EditorSceneModel incrementally from WorldReadView/journal, with full resync fallback.
2. Replace type+vector-index selection with EntityId and validate on model generation change.
3. Convert camera/light/sky/mesh transform/visibility/material-variant edits into semantic commands committed at the next world boundary.
4. Return typed accepted/stale/rejected results. Panels retain draft widget values only; no component pointers/spans.
5. Implement main-thread transaction ownership and deterministic undo/redo through inverse semantic commands/before-after values. Coalesce continuous drags into bounded transactions.
6. Add one private EditorOperationService over SparkleTasks scopes for current open/reload, recook, import/cook, preview, search, save, package validation.
7. Use latest-generation-wins for search/preview/reload, bounded/coalesced progress, immutable results applied in UI/application update.
8. Ensure document/application close cancels/scopes operations, rejects late result, then destroys models.

Validation:
- All current inspector/outliner actions round-trip through commands and undo/redo.
- Deletion/reorder/reload invalidates/preserves selection correctly without vector-index aliasing.
- Pinned model read remains valid while next generation publishes; delayed reader reclamation tested.
- Close document during every operation stage; no callback-after-close or stale application.
- Editor responsive under load/cook/search/preview; background cannot starve frame.

Acceptance gate:
- L Sections 19-21 (reconciliation, acceptance checklist, and required completion report) and Section 24 (Principal Graphics Engineering review gate) pass for the entire touched changelist, including directly affected neighboring ownership paths; no undocumented exception remains.
- UI/panels no longer retain `GameWorld*`, legacy `GameWorld*`, mesh/light/camera facade pointers, mutable spans, or durable indices.
- Direct SceneObjectActions mutations and superseded host service are deleted.
- `SceneCameraView`, `SceneMeshView`, `SceneCameras`, and editor-facing `SceneLighting`/`SceneSky` access are deleted once semantic commands cover their final consumers; panels contain no compatibility facade references.
- One operation service/runtime exists and exposes workflow state, not scheduler internals.
- Current UI behavior, transactions, viewport request behavior, and error reporting remain usable.

Positive patterns: immutable UI model, semantic commands, stable identity, main-thread transactions, scoped operations.
Forbidden: mutex around panels/scene, worker ImGui calls, raw registry in Editor, unbounded progress queue, duplicate editor pool.
~~~

### Prompt 11 implementation and SOLID/DRY reconciliation record

The editor boundary now has four explicit private capabilities instead of one UI-to-world mutation path. `Scene/Model/EditorSceneModel` is an immutable generation value, `Scene/Model/EditorSceneModelBuilder` alone performs full/incremental publication, `Scene/Transactions/EditorTransactionManager` alone owns bounded inverse-command history, and `Scene/Commands/SceneObjectCommandFactory` performs pure model-to-command translation. Panels retain only widget drafts, `SceneObjectSelection` uses `EntityId`, and `WorldEditCommandQueue` validates, bounds, and applies commands at `GameWorld::Update` before systems execute. `EditorOperationService` owns the one document `TaskScope`, admission, cancellation, and immutable result slot; operation-specific shader compiler behavior moved to `EditorOperations/Operations/ShaderRecookOperation`.

Rule 13 access inventory:

| data / transform | authority and ownership | layout, stable identity, deterministic transform | source precedent and measured falsifier |
|---|---|---|---|
| `WorldReadView` + `WorldChangeBatch` -> `EditorSceneModel` | world publication is authoritative; builder-created `shared_ptr<const EditorSceneModel>` generations are derived and reader-pinned | camera/light/mesh rows stay sorted contiguous vectors keyed by generation-bearing `EntityId`; journal changes patch by lower-bound and reset/gap performs full rebuild | J `EPIC-OWN` game/editor read-model boundary; `ShowcaseEditor` DebugEditor link and no-`GameWorld*`/no-registry editor searches are current structural falsifiers |
| selection -> panel presentation | `UI` owns one selection; the immutable model validates it on every model/world generation change | `{kind, EntityId}` replaces vector position; deletion removes selection and world-generation replacement clears it deterministically | J MT-02/30 stable identity and pinned publication; `EditorSceneModel::Contains` falsifies dense-index aliasing |
| widget edit -> semantic command -> world boundary | panel draft is transient; command is an owned value; `WorldEditCommandQueue` is sole pending authority until application | typed variant payload, request ID, expected generation, fixed 4096-command bound; FIFO application reports accepted/stale/rejected | Epic command/transaction separation scoped in J; stale generation/full queue are explicit rejection paths; light categories now submit only when a widget changes, eliminating the per-frame command flood found by this audit |
| forward/inverse commands -> undo/redo | editor-main `EditorTransactionManager` owns history | bounded 256-entry vectors; coalescing replaces only the forward value while preserving the original inverse; world-generation change clears both stacks | J editor-main ownership and MT-03/27 bounds; executable link plus command-only inspector search are current gates; interactive drag timing remains a manual falsifier |
| shader recook request -> compiler result | `EditorOperationService` owns scope/execution/publication; `ShaderRecookOperation` owns compiler invocation only | one in-flight execution, immutable shared result, document-scope cancellation, owner-thread consumption | J structured-scope guidance; DebugEditor executable link proves the split, while close-at-each-stage stress remains an interactive gate |
| mesh preview | renderer immutable proxy/resource is authoritative for the diagnostic copy; editor owns only `MeshPreviewGeometry` | owned vertex/index vectors keyed by current renderer diagnostic identity | game/render proxy precedent in J; removing the empty callback and linking `ShowcaseEditor` falsifies the prior behavior regression |

Rule 12 placement record: public cross-module values remain under `GameFramework/Public/World` and `Renderer/Public/Diagnostics`; editor models, transactions, command factories, and panels are private under named `Scene/Model`, `Scene/Transactions`, `Scene/Commands`, and `Panels` folders. `EditorOperationService` and individual operations are Application-editor private. Public `SceneOutlinerPanel`/`SceneInspectorPanel` headers moved private, and renderer-owned `MeshPreviewGeometry` moved out of Editor. The editor host keeps orchestration only: its former monolithic build procedure is named as input routing, menu, outliner, center workspace, viewport/input registration, inspector, and utility-panel steps. Filenames match their primary type; CMake editor source grouping includes the new hierarchy.

Deletion ledger: `SceneObjectActions`, `SceneOutlinerEntries`, `ShaderRecookExecutionService`, public panel headers, `SceneCameraView`, `SceneCameras`, `SceneLighting`, `SceneMeshes`/`SceneMeshView`, and `SceneSky` were deleted with their direct mutation/read paths. No panel contains `GameWorld`, ECS registry/storage, facade pointer, mutable span, or durable vector index. The empty mesh-preview compatibility callback was deleted rather than retained.

Current evidence: `cmake --build build --config DebugEditor --target ShowcaseEditor -j 4` links the editor; `cmake --build build --config DebugGame --target ShowcaseRuntime -j 4` proves editor-private sources do not leak into the game target. The rejected-alias search finds no deleted controller/facade types. Full close-during-operation and interactive undo/redo/drag tests remain product interaction gates and are not inferred from compilation.

## Prompt 12 — Establish the Immutable Game/Render Data Contract

Target CL Title: `SparkleRenderer: Establish the Immutable Game-to-Render Contract`

~~~text
Implement Prompt 12 only after Prompts 08 and 10 pass.

Objective:
Replace GameWorldSnapshot/raw mesh pointer/direct lifecycle coupling with stable RenderObjectId, immutable asset handles, sequenced RenderWorldDelta, RenderFrameDynamicData, exact frame metadata, and a headless-replayable renderer input contract. Keep renderer execution serial.

Non-negotiable repository rules:
- Apply `Docs/Architecture/00-Review/L_SparkleEngineIntegrationStyleGuide.md` in full. Treat its ownership, implementation-shape, DOD, concurrency, rendering, naming, validation, structure, principal graphics engineering, and completion-report gates as acceptance criteria. Before editing, list the applicable `PGE-01` through `PGE-15` requirements from A, interpret them through H, and classify each as advance, preserve, not applicable, or blocked with expected evidence. Inspect the current repository, reconcile the whole touched ownership path, verify any AI-assisted work independently, delete replaced paths in the same change, reject role-only scaffolding, and report any justified exception explicitly with its owner, scope, evidence, and deletion or review gate.
- Apply Rule 13: audit every data source, transform, stream, packet, cache, table, upload, and hot traversal touched by this prompt; record the concrete access inventory, authoritative/derived ownership, layout decision, stable identity, deterministic transform, exact source precedent, and measured falsifier.
- Apply Rule 12: audit touched/new files for module and folder ownership, public/private placement, filename-to-primary-type alignment, and nearby misplaced counterparts; complete bounded moves/renames with includes, CMake, and documentation updated before the gate.
- Enforce J's canonical concurrency/rendering vocabulary and Rule 10: search canonical terms plus rejected aliases in the touched scope, use one responsibility per name, and delete temporary compatibility spellings before the gate.
- Audit GameWorldSnapshot/MeshSnapshot, Renderer facade/SystemRoot, RenderSceneDataBuilder, SceneRenderStateCoordinator, temporal inputs, RT instances, providers, capture, level callbacks, asset handles.
- Reuse existing handle/packet/arena/math/frame-generation types; do not create a second scene schema or broad renderer snapshot API.
- Renderer must not depend on ECS or dereference `GameWorld` after conversion.
- Refactor and delete each old read/callback path when its packet equivalent lands.

Required implementation:
1. Define RenderObjectId generation separate from EntityId and explicit extraction mapping.
2. Replace raw Mesh*/game pointers with immutable typed mesh/material/texture/skeleton/animation handles whose lifetime spans packet/proxy use. Content regeneration publishes the newest immutable handle; do not add per-handle content-version compatibility.
3. Define structural RenderWorldDelta create/update/destroy with SceneGeneration + SequenceNumber and packet-owned/arena storage.
4. Define dynamic frame arrays for transforms, skinning/morph, lights/camera/view data, visibility and current feature inputs.
5. Include exact temporal/provider metadata: FrameId, exposure, resolution, motion/depth conventions, camera cut/teleport/history reset, and provider/frame-generation tags. Jitter/sample selection is renderer-private and derives from FrameId.
6. Implement serial RenderWorld/RenderProxyRegistry applying checked deltas and resolving immutable assets.
7. Record/replay packet streams in a headless renderer test without `GameWorld` lifetime.
8. Convert level begin/end into ordered generation data; remove renderer direct level-event subscription/cache clearing.

Validation:
- Packet contains only owned values or stable immutable handles; poison/reset after acknowledgement catches retention.
- Create/update/destroy replay deterministic; stale/duplicate/out-of-order delta handling tested.
- Delayed render consumer while levels/assets change has no stale pointer.
- Serial visual/feature parity for raster, classic TLAS/PTLAS inputs, reservoir light identity, path/reference reset, temporal/provider tags, capture requests.
- Packet bytes/build time measured; no broad public diagnostics added.

Acceptance gate:
- L Sections 19-21 (reconciliation, acceptance checklist, and required completion report) and Section 24 (Principal Graphics Engineering review gate) pass for the entire touched changelist, including directly affected neighboring ownership paths; no undocumented exception remains.
- Renderer consumes a recorded stream with `GameWorld` destroyed.
- MeshInstanceSnapshot raw pointer and old full snapshot boundary are deleted.
- `GameWorldSnapshot` and all domain `*Snapshot` compatibility families listed in the Prompt 07 final-vocabulary table are deleted; only deliberately named renderer packets/read models remain.
- `RendererSystemRoot` has no `GameWorld` access and no direct lifecycle callbacks.
- Serial output matches baseline before render threading.

Positive patterns: stable separate identities, immutable handles, structural/dynamic split, replayable packet.
Forbidden: shared scene mutex, raw object pointer, renderer ECS query, array index temporal identity, dual packet/snapshot path.
~~~

### Prompt 12 implementation and subsystem-boundary reconciliation record

The renderer now consumes one owned `RenderInputFrame` composed of a sequenced structural `RenderWorldDelta` and per-frame `RenderFrameDynamicData`. Private GameFramework extraction is hierarchical: `RenderInputExtractor` sequences only, `RenderObjectIdentityMap` owns the sole `EntityId -> RenderObjectId` mapping, `RenderObjectDeltaExtractor` owns structural comparison, `RenderResourcePublisher` owns table/sky/group publication, and `RenderFrameDynamicDataExtractor` owns camera/light/object/deformation rows. Renderer admission has one serial pending slot rather than a transport queue; `RenderInputFrameValidator` composes focused metadata and dynamic-data validators, `RenderWorldDeltaValidator` owns structural/resource rejection, and `RenderWorld` owns only persistent mutation. Renderer projection now enters `RenderPreparationGraph`; serial `RenderPreparationInputResolver` owns lazy cache/materialization, pure range capabilities own transform/visibility/light/deformation work, and `RenderPreparationMerger` owns deterministic publication. `RendererSystemRoot` owns no `GameWorld`, `LevelManager`, ECS query, or level callback.

Rule 13 access inventory:

| data / transform | authority and ownership | layout, stable identity, deterministic transform | source precedent and measured falsifier |
|---|---|---|---|
| extracted ECS meshes/lights -> structural and dynamic rows | ECS/world resource stores are authoritative; `RenderInputExtractor` owns one generational `EntityId -> RenderObjectId` map for every extracted render identity; packets are derived | ordered entity maps give deterministic rows; complete static updates carry the same cohesive `RenderObjectStaticData` as creates; light rows retain stable IDs through dense-storage reorder; `RenderObjectId` remains separate from `EntityId` and GUID domains | J `EPIC-OWN`, `MT-01`, and `MT-04`; headless replay rejects duplicate/stale/gap/new-generation-without-reset input and exact projected-object coverage is validated |
| mesh/material/texture/skeleton/animation resources -> packet/proxy tables | generation stores own resources; mesh lifetime crosses only as `shared_ptr<const Mesh>` inside `ImmutableRenderMeshHandle`; all other renderer inputs are owned values or typed handles | mesh carries cooked ID + immutable shared ownership; existing `MaterialHandle` retains its table slot/generation contract; every texture-table row carries a slot-only `RenderTextureAssetHandle`; skeleton/animation provenance carries a typed cooked ID after pose/morph data is copied; regenerated content publishes a new immutable resource/table rather than a compatibility version | J `EPIC-OWN`, `MT-02`, and `MT-04`; validation poisons producer and acknowledged packet storage, then reads retained mesh geometry/material/texture state and rejects invalid table slots, asset classes, and paths |
| transforms/visibility/camera/lights -> dynamic frame | the committed extraction/read generation is frame authority; the packet owns every value | object rows carry stable object, matrix, inverse transpose, and visibility; camera/light arrays are values; lights are sorted by stable entity and carry `RenderObjectId`; raster, PTLAS/TLAS input, and lighting invalidation consume the same deterministic order, with light identity included in the history hash | J two-data-stream contract, `AMD-PORT`, and `MT-29`; DebugEditor/DebugGame links are parity build gates while interactive image tolerance remains a separate product run |
| animation output -> skinning/morph rows | generation-prepared animation output is authoritative until extraction; packet owns copied matrices/weights and typed source provenance | skinning and morph rows use resolved target `RenderObjectId`; skeleton and animation handles are validated; no renderer dereference of clip/skeleton storage occurs; mesh builder owns previous matrices per render object | Prompt 10 exclusive outputs plus J `MT-02`/`MT-04`; actors sharing a skeleton do not alias a joint offset and malformed provenance is rejected before world mutation |
| packet -> admission -> serial `RenderWorld` | packet sequence is input authority; renderer proxy/resource tables are one persistent derived projection | `RenderInputFrameValidator` checks frame/scene/provider metadata and exact dynamic coverage; `RenderWorldDeltaValidator` rejects duplicate/conflicting/unavailable operations before mutation; the serial consumer rejects overwrite of its single pending slot; apply order is reset/destroy/create/update/publication | J `EPIC-OWN`, `MT-03`, and `MT-23`; the headless gate proves atomic rejection for duplicate create/update/destroy, update+destroy conflict, stale/duplicate/gap sequence, incomplete reset, and incompatible resource generations |
| frame metadata -> temporal/providers/capture | accepted packet metadata is authoritative for logical frame identity; renderer resolution is finalized at submission; renderer owns buffered-frame slots separately | `FrameId`, scene/frame generation, provider generation, exact render/output extent, exposure, conventions, and reset causes are validated; `TemporalDataBuilder` derives its renderer-private jitter sample from `FrameId`; Streamline tokens and capture use the same frame identity; capture uses `ExpectedFrameId` and returns frame/provider/scene tags | J canonical `FrameId` vocabulary, `EPIC-RHI`, and `MT-04`; duplicate frame IDs and camera cuts without reset are rejected, provider-generation change forces reset, and editor/game/provider/capture paths compile with no ambiguous capture `FrameIndex` or app-level jitter field |
| `RenderWorld` + dynamic -> draw data | renderer projection is authoritative; `RenderPreparationInputResolver` resolves immutable/cache-backed inputs and `RenderDeformationPreparation` owns temporal deformation history | frustum-visible raster traversal, per-object joint/morph offsets, material resolution, stable group conversion, deterministic batch builder, distinct RT work plan | AMD/NVIDIA renderer data/batching guidance scoped in J; source reconciliation falsifies dependency breakage; GPU visual/performance evidence remains user-owned interactive validation |
| diagnostics -> editor preview | renderer proxy/resource is authoritative; diagnostics return owned read values | no proxy/resource pointer escapes; preview copies vertices/indices; deliberately named diagnostic snapshots remain UI read models | J diagnostic read-model exception; full editor link and removal of GameWorld preview access are structural falsifiers |

Rule 12 placement record: public packet/resource/capture contracts remain responsibility-split under their existing owning modules: `GameFramework/Public/Rendering`, `Renderer/Public/Viewport`, and `RHI/Public/Capture`. Private extraction capabilities are filename/type-aligned under `World/Extraction/{Identity,Structural,Resources,Dynamic}` behind the orchestration file. Renderer input validation is hierarchical under `SceneData/Input/Validation`; structural-delta validation is adjacent to its persistent owner under `SceneData/Validation`; `RenderWorld` remains the mutation boundary; and the headless executable delegates to fixture, metrics, and check capabilities under `Renderer/Validation/RenderWorldContract`. No validation policy moved into packet DTOs, no extraction policy moved into `RenderWorld`, and no RHI type acquired GameFramework knowledge. Game-system descriptors live under `World/Systems/Descriptors`; the extraction descriptor declares skeleton-resource reads used to construct the typed handle. No `SceneAnimation*` implementation spelling, content-version adapter, or compatibility capture field remains.

Deletion ledger: `GameWorldSnapshot`, `MeshSnapshot`, camera/lighting/material/texture/sky snapshot families, `RenderSceneSnapshot`, `SceneRenderStateCoordinator`, `SceneLightingSnapshotBuilder`, and `RenderMeshSnapshotAdapter` remain deleted. Direct renderer level subscriptions/cache clearing and all `RendererSystemRoot` world/level accessors remain absent. This reconciliation additionally deleted partial material-only proxy updates, scene-generation truncation as render-object generation, raw skeleton/animation IDs at the packet boundary, ambiguous absent-sky clearing, unconditional instance-group replacement, silent pending-packet overwrite, completion-order-free light array identity, provider-local timer frame identity, and capture `FrameIndex` compatibility spelling.

Validation evidence: `cmake --build build --config DebugEditor --target RenderWorldContractValidationRun -j 4` builds and runs a three-frame create/full-update/destroy recording without `GameWorld` or RHI lifetime. The current DebugEditor run measured 3,856 logical owned bytes for the fixture stream, 104 microseconds to construct it, and 46 microseconds to replay it into two worlds; these are smoke-falsifier measurements, not a performance claim. The gate compares complete render-world fingerprints, validates dynamic coverage/provenance/metadata, rejects operation conflicts atomically, preserves omitted sky state, clears explicitly published sky state, and poisons producer plus acknowledged packet storage before reading retained resources. `ShowcaseEditor` DebugEditor and `ShowcaseRuntime` DebugGame build with exact packet `FrameId` driving jitter/provider tokens and capture tags. Searches find no retired snapshot/controller family, renderer `GameWorld`/ECS access, `SceneAnimation*` implementation, or ambiguous capture `FrameIndex`. Interactive raster/classic TLAS/PTLAS/reservoir/path/reference image tolerance and representative real-world extraction profiling remain product/Prompt 12D gates and are explicitly not inferred from compilation or the synthetic headless timing.

### Repository-wide SOLID/DRY continuation record

The Prompt 10-12 boundary work was used as the pattern for a second, repository-wide hotspot audit. File size was only a locator: a large backend file was changed only when it mixed authorities or operations. The bounded cleanup selected level-light parsing, cooked-shader package loading/validation, editor mesh/texture diagnostics, generic text-file reads, and editor widget implementation. Large Vulkan command-list, device, GPU-allocation, descriptor-allocation, and launcher files were inspected but retained where the file still represents one backend capability; their size alone is not evidence for an ownership move.

Rule 13 access inventory:

| data / transform | authority and ownership | layout, stable identity, deterministic transform | exact precedent and measured falsifier |
|---|---|---|---|
| level text line -> light key -> typed light field -> `LevelDesc` | the level file is serialized authority during load; `LightFieldKeyParser` decodes identity, `LightFieldParser` mutates only the construction value, and `LightingSectionWriter` owns serialization | cohesive light descriptors remain AoS because construction/save consume all fields together; `(light kind, authored ordinal)` is the load-time key and source-line order is semantic | adjacent `CameraSectionParser`/level parsing conventions are the repository precedent; `LightingSectionParser.cpp` fell from 596 to 21 logical lines and both DebugEditor and DebugGame compiled every new parser/writer unit |
| cooked package file -> owned bytes/records -> contract checks -> admitted cache entry | the cooked file plus `CookedShaderPackageContract` are authoritative; `LoadedShaderPackage` owns immutable loaded data; reader, validators, diagnostics, and cache admission are separate derived operations | contiguous package storage and record arrays are retained; `ShaderPackageKey`, layout hash, codegen target, and stage are stable contract identity; named header, feature, layout, binding, reflection, and binary checks run in fixed fail-fast order | existing `BinarySpanReader`, package contract, and the NVIDIA Donut shader/resource separation recorded in J are the precedents; the former 1,102-line cache implementation is 150 logical lines, reflected diagnostics/rules have one definition, and editor/game links compile both D3D12 and Vulkan consumers |
| renderer diagnostic read model + cooked metadata -> presentation values -> panel draw | renderer diagnostic snapshots are immutable UI-input authority for a frame; metadata catalogs own read-only lookup caches; presentation owns formatting/preview math; panels own selection and ImGui interaction only | rows stay cohesive AoS read models; mesh metadata is keyed by `MeshAssetId`, texture metadata by the diagnostic key/cooked summary; iteration and aggregation preserve snapshot order | Prompt 11 immutable-model/presentation boundary and existing renderer diagnostic contracts are the precedents; direct metadata JSON/file/cache code is absent from both panels, and one shared byte formatter replaced panel-local copies |
| filesystem path -> text bytes -> caller-owned string | disk is authority; Core `Files` performs I/O and the caller owns the returned string/error | one contiguous `std::string`, exact byte order, path identity, no borrowed stream/view escapes | `Files::TryReadAllBytes` is the exact local precedent; repository search finds no remaining private `TryReadTextFile` copy and both diagnostics catalogs consume `Files::TryReadAllText` |
| public editor widget call -> category implementation -> ImGui result | editor main/ImGui retain UI-state authority; category implementation owns only drawing/edit behavior and returns values/events to its caller | the public `UiUtil` API and ImGui IDs remain stable; icon, panel, property, details, and shared primitive calls execute in the same immediate-mode call order | existing `UiUtil.h` is the contract precedent; the 907-logical-line mixed implementation was deleted and replaced by focused category units, each no larger than 273 logical lines, plus one private shared-primitives unit; `ShowcaseEditor` links after the split |

Rule 12 placement record: level lighting mechanisms are private under `GameFramework/Private/Level/Parsing/Lighting`, leaving `LightingSectionParser` as the section boundary. `LoadedShaderPackage` is a public RHI value type in a filename-matched header; binary reading, structural validation, reflected-binding validation/diagnostics/rules, and cache admission remain private RHI capabilities. Diagnostics catalogs, presentation, and shared panel formatting are private Editor panel capabilities. The stable `UiUtil` contract remains public while its responsibility-split implementations and shared drawing primitives remain private. Generic all-text file I/O lives beside the existing Core all-bytes capability. CMake glob regeneration discovered every new unit; no private header is included through a public path.

Deletion ledger: the mixed `UiUtil.cpp` implementation was deleted without an adapter; duplicate editor `TryReadTextFile` and byte-size formatting implementations were deleted; level key decoding, field mutation, and writing were removed from the section facade; package byte reading, loaded-package behavior, structural validation, reflected-binding diagnostics, and validation rules were removed from `CookedShaderPackageCache.cpp`. The misleading `SceneAnimation*` implementation family remains absent. No parallel old/new path or compatibility spelling was introduced.

Validation evidence: `ShowcaseEditor` (`DebugEditor`), `SceneCooker` (`DebugEditor`), `RenderWorldContractValidationRun` (`DebugEditor`), and `ShowcaseRuntime` (`DebugGame`) pass. Repository searches find no `UiUtil.cpp`, `TryReadTextFile`, `FormatBytes(`, `SceneAnimation*`, private-header leakage, or second `LoadedShaderPackage` definition. These builds and structural counts falsify dependency/placement regressions; interactive widget appearance, level-file round-trip corpus coverage, malformed shader-package diagnostics, and runtime performance remain behavior/benchmark gates and are not inferred from compilation.

### Repository-wide SOLID/DRY continuation record — round two

The second audit selected three untouched ownership crossings: Core process/root discovery versus cached asset-path configuration and lookup; texture-request identity versus token/line codecs and file transport; and AssetCooker plan dispatch versus stage execution, source import, cooked-scene publication, and default-texture policy. The public contracts and serialized request version remain unchanged; no bridge implementation was retained.

Rule 13 access inventory:

| data / transform | authority and ownership | layout, stable identity, deterministic transform | exact precedent and measured falsifier |
|---|---|---|---|
| executable/current directory + marker files + explicit project root -> discovered roots -> cached asset paths | process paths and marker files are bootstrap authority; an explicit `ConfigureProjectRoot` becomes project authority; `AssetPathState` alone owns the cached derived paths and output materialization | scalar path fields plus fixed `std::array<path, AssetType::Count>` typed roots; stable keys are `AssetType`, `PathRoot`, and normalized path; discovery preserves executable, working-directory, then workspace fallback order and asset resolution preserves cooked/project/engine order | existing `Paths::Normalize`, private package discovery, marker constants, and the public `FileSystemUtils` contract are exact local precedents; the 561-line `FileSystemUtils.cpp` was deleted, Core compiles four responsibility units, and editor/game/tool links exercise both bootstrap profiles |
| material/default texture references -> `TextureCookRequestSet` -> versioned request text -> loaded request vector | source materials and the fixed engine-default catalog produce requests; `TextureCookRequestSet` is the sole conflict authority for a `TextureAssetId`; the request file is transport, not a second mutable authority | map lookup enforces stable asset identity while the vector preserves collection data; serialization sorts by `(assetId, outputPath)`, loading sorts by `assetId`, and `TextureCookRequests|1` plus ten ordered fields remains the wire contract | the existing version-one request header, `Formatting::FormatHexUInt64`, and `Files::TryWriteAllText` are exact precedents; the mixed request implementation fell from 524 to 118 logical lines and the rebuilt `TextureCookShared`/`TextureCooker` resolves all policy-name, codec, and set symbols |
| `AssetCookerProjectCookPlan` -> ordered stage -> import/build/write or external tool -> output record | the plan owns stage/scene order; dispatcher owns only plan-level progress/failure; stage executor owns stage selection and output admission; imported-scene cooker owns COM/import/build/write lifetime; texture-plan/default builders own their respective request sources | stable identities remain plan step, project name, scene path/scene asset identity, texture asset ID, and output asset ID; stages and scenes execute in authored order, outputs append in the previous fixed order, and temporary request cleanup is scope-owned | existing `ProjectCookPlan`, `SourceSceneImporter`, `SceneCooker`, and `AssetCookerToolProcess` are exact local precedents; `AssetCookerDispatcher.cpp` fell from 594 to 44 logical lines, unused per-failure scene strings were replaced by a count, and `AssetCooker` links and reaches its CLI usage path |

Rule 12 placement record: Core state, resolution, accessors, and discovery now live under `Core/Private/Paths`; `FileSystemDiscovery.cpp` owns all executable/marker/package discovery and `AssetPathState.cpp` owns cached configuration/materialization. The obsolete root-level Core implementation was deleted, and Core globbing now uses the repository-standard `CONFIGURE_DEPENDS` so moves are reflected by CMake. Texture policy and request-line codecs plus request-set storage live under `TextureCooker/Private/Requests`; only the established request model/list API is public. AssetCooker import/build/request capabilities live under `Private/Cooking`, while stage selection/execution and the thin dispatcher live under `Private/Dispatch`. Every new filename matches its primary capability and all explicit TextureCooker/AssetCooker CMake source/header lists were updated.

Deletion ledger: `Engine/Core/Private/FileSystemUtils.cpp` was deleted; its anonymous cached state, directory materialization, discovery, getters, and lookup helpers have no compatibility copy. Texture policy parsing/name formatting and request conflict storage were removed from `TextureCookRequestList.cpp`; the list file now owns only deterministic file transport. COM import lifetime, default texture descriptors, scene build/write, texture-request collection, tool lookup/temp cleanup, and stage bodies were removed from `AssetCookerDispatcher.cpp`; it now owns only plan orchestration. The unused `failedScenes` string vectors and manual temporary-file cleanup branches were deleted in favor of counts and scope-owned cleanup.

Validation evidence: `SparkleCore`, `TextureCooker`, `AssetCooker`, `SceneCooker`, `ShowcaseEditor`, and `RenderWorldContractValidationRun` pass in `DebugEditor`; `ShowcaseRuntime` passes in `DebugGame`; both cooking executables load and print their usage contracts. `git diff --check` passes. Searches find no old `AssetCookerRun*`/default/request-collector helper family, root-level `FileSystemUtils.cpp`, private-header leakage, or missing explicit cooker CMake entries. A real project recook, request-file golden round trip, malformed-request corpus, and packaged-runtime path-discovery matrix remain behavioral gates and are not inferred from compilation or CLI startup.

## Prompt 12D — Prove the Two Data Streams Data-Oriented Boundary

Target CL Title: `SparkleEngine: Prove the Data-Oriented Game-to-Render Streams`

~~~text
Implement Prompt 12D only after Prompt 12 passes and before Prompt 13 begins.

Objective:
Make `RenderWorldDelta` and `RenderFrameDynamicData` a measured data-oriented transform from GameFramework's frozen ECS/read publication into renderer-owned persistent tables. Remove any remaining object-shaped/full-scene extraction, generic cosmetic SoA, redundant authority, pointer chasing, or per-frame static duplication before renderer threading begins. Preserve serial rendering and every current raster, RT, temporal, provider, editor-viewport, and capture input.

Non-negotiable repository rules:
- Apply `Docs/Architecture/00-Review/L_SparkleEngineIntegrationStyleGuide.md` in full. Treat its ownership, implementation-shape, DOD, concurrency, rendering, naming, validation, structure, principal graphics engineering, and completion-report gates as acceptance criteria. Before editing, list the applicable `PGE-01` through `PGE-15` requirements from A, interpret them through H, and classify each as advance, preserve, not applicable, or blocked with expected evidence. Inspect the current repository, reconcile the whole touched ownership path, verify any AI-assisted work independently, delete replaced paths in the same change, reject role-only scaffolding, and report any justified exception explicitly with its owner, scope, evidence, and deletion or review gate.
- Apply Rule 13: audit every GameFramework source column, extraction transform, packet field, RenderWorld proxy/table, and planned GPU upload consumed by the two streams. No field exists only because the old object carried it.
- Apply Rule 12: make GameFramework publication, extraction contracts, renderer input streams, and render-owned table files/folders reveal their ownership and data-flow direction.
- Trace every material decision to Richard Fabian's data/access methodology, Epic MassEntity or game/render proxy documentation, NVIDIA Donut renderer scene buffers/dirty state, or AMD Cauldron/Detroit/RDNA renderer data guidance. State the scope of the precedent. Do not claim NVIDIA/AMD provides Sparkle's GameFramework ECS.
- If no source and no measured Sparkle consumer supports a proposed abstraction, field, index, cache, or layout, do not add it.
- GameFramework ECS remains the only authoritative mutable world-instance source. Render packets and RenderWorld are sequenced one-way derived projections; renderer state never mutates ECS or editor authoring truth.
- Renderer does not query ECS, retain `GameWorld`, invoke entity/component behavior, or consume raw pointers/references/spans into GameFramework storage.
- Keep renderer execution serial. Do not begin RenderThread, frame queue, parallel extraction, persistent GPU-scene implementation, or command recording work owned by later prompts.

Required implementation:
1. Build a field-level data/access ledger for all current camera, view, transform/current-previous transform, bounds, visibility, mesh instance, material, light, animation, skinning, morph, RT instance, temporal/history, provider, editor viewport, and capture inputs. For each field record producer, consumer passes, cardinality, frequency, mutation phase, read grouping, stable key, lifetime, current bytes/allocation, and retain/move/split/delete decision.
2. Build an authority/projection ledger: authored/cooked source → ECS/runtime resource → frozen world view/change journal → packet stream → RenderWorld slot/table → later GPU table. Name the sole authority and generation/rejection/resync rule at every arrow. Delete any unowned duplicate mutable copy exposed by the audit.
3. Replace a generic `SoA<T...>` or object-shaped “packet per entity” with named concrete streams whose columns match real consumers. At minimum decide and document layouts for transforms/current-previous transforms, bounds/visibility, lights, skinning palettes, morph weights, and RT instance inputs. Keep cohesive AoS/AoSoA blocks when consumers read all fields together; use SoA only where access differs materially.
4. Encode variable-length skinning/morph/instance payloads as bounded flat arrays plus offsets/counts or another source-backed measured packed layout. Packet records contain no owning vector/string, allocator, callback, mutex, service, raw asset pointer, component reference, or nested polymorphic object.
5. Keep `RenderWorldDelta` as typed bounded operation batches with scene/sequence metadata. Choose AoS or columnar form per operation consumption and measured size; do not force cold structural records into SoA. Define stale, duplicate, gap, overflow, and full-resync behavior before application.
6. Make extraction one explicit serial bulk transform over a frozen world epoch using declared component/resource reads. Resolve `EntityId` to `RenderObjectId` through one mapping authority, produce stable-key task-partition-ready ranges, and never expose ECS dense indices as render identity.
7. Make the serial RenderWorld representation persistent and indexed: stable render slots/generations, packed per-consumer tables, free/reuse policy, asset-handle generations, and dirty columns/ranges. Prompt 15 owns persistent GPU allocation/upload/retirement, but this prompt must give it a concrete CPU-side source layout rather than another scene-wide builder.
8. Define current/previous/temporal ownership once. State exactly where previous transforms, camera cuts, history reset, exposure, resolution, and provider frame tags roll over; derive renderer-private jitter/sample state from `FrameId`; delete redundant rollover or app/game packet fields.
9. Define deterministic extraction and apply order with stable `(scene, sequence, render object, stream/local)` keys. Sorting, bucketing, compaction, and deduplication use packet/range-local outputs and deterministic merge; completion order is never semantic.
10. Preserve immutable static assets through typed handles and residency operations. Regeneration replaces the published immutable content rather than retaining compatibility versions. Mesh geometry, material definitions, textures, skeletons, animation clips, BLAS payload identity, and shader packages are not copied into per-frame dynamic streams.
11. Produce small, representative, high-instance, high-light, animated/skinned/morph, RT/PTLAS, structural-churn, and mostly-static workloads. Compare the replaced snapshot/object path with the new serial stream path for output identity and relevant packet bytes, allocations, extraction/apply time, bytes read/written, cache misses/bandwidth where available, dirty rows/ranges, and projected upload bytes.
12. Record rejected alternatives: full snapshot, one AoS object per renderable, universal SoA, immediate archetype conversion, renderer ECS query, hash lookup in every hot pass, and duplicated current/previous data. Keep an alternative only if a source-backed measured workload beats the selected layout.
13. Delete `GameWorldSnapshot`, raw `Mesh*` packet fields, converted parallel scene vectors, old full-scene extraction/build path, and temporary compatibility schema after parity. Repository search must find one game-to-render data path.
14. Update J's Two Data Streams section and the Prompt 12/15 field ledgers with the concrete accepted layouts and measurements; label unmeasured hardware claims as pending rather than inferring benefit.

Validation:
- Headless record/replay destroys or mutates `GameWorld`/ECS storage immediately after packet publication and delays consumption; output remains valid and deterministic.
- Field/ownership audit proves every packet byte has a named consumer and every consumer reads an owned immutable value or stable typed handle.
- Serial old/new comparison preserves raster draw identity/order, camera/view values, classic TLAS/PTLAS instance identity, reservoir-light identity, skinning/morph output, temporal resets, provider tags, viewport and capture inputs.
- Mostly-static scene changes zero objects for many frames, then one transform/light/material/object at a time; only the documented stream rows and RenderWorld dirty ranges change.
- Structural create/update/destroy replay covers stale/duplicate/gap/overflow/full-resync and stable-slot reuse generations.
- Variable-length offset/count fuzzing rejects overlap/out-of-bounds/stale generation and releases packet arenas without retained pointers.
- Representative layout comparison records packet bytes, allocations, extraction/apply time and available cache/bandwidth counters; unsupported counters are stated, not replaced with logging.
- Dependency scan proves Renderer includes no ECS storage/private GameFramework header and GameFramework has no Renderer/RHI dependency.

Acceptance gate:
- L Sections 19-21 (reconciliation, acceptance checklist, and required completion report) and Section 24 (Principal Graphics Engineering review gate) pass for the entire touched changelist, including directly affected neighboring ownership paths; no undocumented exception remains.
- The two streams are concrete named access-driven schemas, not generic containers or copied object graphs.
- GameFramework has one mutable world authority; renderer has one sequenced derived projection and no backward mutation path.
- Static/structural/dynamic/temporal data have explicit frequencies, owners, generations and consumers; variable data is flat and bounded.
- Renderer hot preparation can begin from stable render slots and dense per-consumer ranges without ECS joins, object polymorphism, raw asset traversal, or repeated scene-wide hash lookup.
- One-object changes do not rebuild or republish unrelated scene-wide data; full resync is an exceptional explicit recovery path.
- Old snapshot/object/parallel-vector paths are deleted, serial feature parity passes, and the DOD reconciliation contains exact source and measurement evidence.

Positive patterns: access-driven concrete streams, one-way derived projection, stable slots, flat bounded arrays, hot/cold and static/dynamic split, deterministic bulk transform, measured layout.
Forbidden: DOD by naming, universal SoA, renderer ECS access, per-entity virtual extraction, raw pointers, nested packet containers, duplicated authority, full rebuild disguised as parallel work.
~~~

## Binding Changelist Design Gate for Prompts 13-29

This gate is part of the acceptance criteria of every unfinished prompt from Prompt 13 through Prompt 29. A prompt does not pass merely because its feature works or its local validation bullets pass. The entire prompt changelist, including directly adjacent counterparts whose responsibilities are exposed by the work, must also pass this design gate:

- Produce a responsibility and placement inventory for every added, modified, moved, or deleted owned-source file. Record its owning module/subsystem, architectural layer, primary responsibility, authoritative or derived data, allowed dependencies, public/private status, and final folder. Audit nearby counterparts before adding a new descriptor, service, queue, cache, builder, validator, adapter, or utility.
- Separate orchestration from capability implementation hierarchically. Orchestration entry points may sequence phases, select policy, route typed inputs/results, and own lifecycle state; algorithms, validation, storage/cache mechanics, serialization/I/O, backend execution, and UI presentation belong to named capability files beneath the owning subsystem. An orchestration file must read as the system flow rather than contain the implementation of every phase.
- Enforce single responsibility at class and function level. A class has one reason to change and must not mix lifecycle/orchestration, policy/validation, state/storage/cache, transport/serialization, backend execution, and presentation. Decompose multi-stage, deeply nested, or branch-heavy functions into named behaviors even when a behavior has one call site; the name and boundary must expose a real invariant or operation, not merely move lines.
- Apply SOLID without speculative abstraction: extend behavior through the narrow owning capability; preserve substitutability of existing backend/provider contracts; keep interfaces consumer-specific and minimal; and make dependencies point toward stable contracts/owners rather than concrete peer internals. Do not add an interface, manager, service, or pass-through wrapper solely to satisfy a pattern or line-count target.
- Apply DRY to authority and behavior, not just syntax. One owner implements each transform, validation rule, lifecycle transition, cache lookup, upload plan, or merge policy. Consolidate repeated logic at the narrowest correct owner and delete superseded copies, callback adapters, compatibility aliases, boolean-mode forks, and parallel old/new paths before the gate.
- Audit god files, god classes, and god functions across the whole prompt changelist. File size and function length are locators, not verdicts; retained complexity requires a concrete cohesion reason, an explicit responsibility map, and a measured or testable falsifier. No touched hotspot may be waived as “pre-existing” when the prompt adds another responsibility to it.
- Complete Rule 12 placement work before validation: filename matches the primary type/capability; public headers expose only durable contracts; private mechanisms remain private; folders express the subsystem hierarchy; bounded moves/renames update includes, source groups/CMake, tests, documentation, and deletion ledgers. Reject generic dumping grounds such as new catch-all `Common`, `Utils`, `Helpers`, `Managers`, or flat `Systems` folders without a single documented owner.
- Record before/after structural evidence: responsibility/file map, hotspot disposition, dependency-direction and rejected-alias searches, moved/deleted path ledger, and relevant build/test/benchmark gates. The evidence must prove that the final tree is easier to navigate and reason about and that no temporary compatibility spelling or duplicate implementation remains.
- Apply Rule 16 across the complete prompt changelist and owned repository baseline: no anonymous namespace, no arbitrary renamed substitute, no function-local class/struct, no non-template/non-accessor implementation body in a touched header, and no diagnostic scaffolding beyond the owning invariant. The completion evidence includes repository-wide anonymous-namespace and touched-header searches.
- Apply Rule 17 and L to the complete prompt changelist: identify applicable `PGE-*` requirements, preserve path-tracing/neural/AI/math/hardware-driver/partner boundaries, verify AI-assisted work independently, and reject role-only scaffolding. This does not require a prompt to implement neural work before Prompt 30; it requires every prompt to leave a clean, measurable integration surface for the later evidence program.

This contract deliberately has no arbitrary maximum file length and does not require one file per function. A one-call helper or private capability is desirable when it names and isolates meaningful behavior; a content-free forwarding layer or cosmetic file split fails the gate.

## Prompt 13 — Add RenderThread Ownership and the Bounded RenderFrameQueue

Target CL Title: `SparkleRenderer: Add RenderThread Ownership and the Bounded Frame Queue`

~~~text
Implement Prompt 13 only after Prompt 12D passes.

Objective:
Move RendererSystemRoot, FramePipeline, mutable RHI ownership, submit/present, and renderer resource creation/destruction to one `RenderCoordinator` running on `RenderThread`. Connect GameThread/EditorThread producers through bounded frame slots and sequenced control commands. Keep renderer preparation/recording serial inside the coordinator initially.

Non-negotiable repository rules:
- Apply `Docs/Architecture/00-Review/L_SparkleEngineIntegrationStyleGuide.md` in full. Treat its ownership, implementation-shape, DOD, concurrency, rendering, naming, validation, structure, principal graphics engineering, and completion-report gates as acceptance criteria. Before editing, list the applicable `PGE-01` through `PGE-15` requirements from A, interpret them through H, and classify each as advance, preserve, not applicable, or blocked with expected evidence. Inspect the current repository, reconcile the whole touched ownership path, verify any AI-assisted work independently, delete replaced paths in the same change, reject role-only scaffolding, and report any justified exception explicitly with its owner, scope, evidence, and deletion or review gate.
- Apply Rule 13: audit every data source, transform, stream, packet, cache, table, upload, and hot traversal touched by this prompt; record the concrete access inventory, authoritative/derived ownership, layout decision, stable identity, deterministic transform, exact source precedent, and measured falsifier.
- Apply Rule 12: audit touched/new files for module and folder ownership, public/private placement, filename-to-primary-type alignment, and nearby misplaced counterparts; complete bounded moves/renames with includes, CMake, and documentation updated before the gate.
- Enforce J's canonical concurrency/rendering vocabulary and Rule 10: search canonical terms plus rejected aliases in the touched scope, use one responsibility per name, and delete temporary compatibility spellings before the gate.
- Audit runtime/editor loops, renderer facade/host phases, window/swapchain resize, minimize/restore, provider affinity, capture, shutdown/device loss, RHI creation/destruction and submission.
- Audit LC-08 through LC-10 and LC-16/LC-18: Streamline global locking, D3D12 queue submission/CPU-wait mutexes, Vulkan native queue external synchronization, every renderer/RHI idle wrapper and native wait. A mutex-protected queue is not a substitute for one submission owner.
- Reuse SparkleTasks and existing frame-resource/timeline primitives; no second scheduler or RHI thread.
- Refactor host Prepare/Record/Submit routes into one coherent submission facade; delete direct product path after parity.
- Preserve serial consumer and threaded zero/one-ahead modes using the same packet contract.

Required implementation:
1. Add bounded `RenderFrameQueue` slots with explicit Free/Writing/Ready/Rendering/Retired transitions, release/acquire publication, close/wake, acknowledgement/reuse. The owning type is `RenderCoordinator`; its physical thread role is `RenderThread`.
2. Add `RenderControlCommandQueue` with sequenced commands for resize, settings, capture, shader generation, viewport, shutdown, and necessary provider/present actions. Do not call these CPU-owned control records command lists, command buffers, or GPU queue submissions.
3. Create/adopt RHI and renderer root on the coordinator; assert all mutable renderer/RHI owner methods.
4. Implement serial-consumer, threaded zero-ahead, and threaded one-ahead modes. No unbounded queue or hidden packet allocation fallback.
5. Define slow-render backpressure and minimize/resize behavior; producer parks only at explicit slot acquisition.
6. Implement ordered shutdown: stop `RenderControlCommandQueue` producers, close `RenderFrameQueue`, settle accepted frames/tasks, final GPU drain, destroy RHI on RenderThread, join the coordinator, stop executors last.
7. Ensure device-loss/fatal paths settle `RenderFrameQueue` slots and editor waiters without callback-after-destroy.
8. Make D3D12 submit and Vulkan submit/present coordinator-only. Retain a native queue mutex only for a documented provider/API shared-entry boundary; otherwise ownership assertions replace multi-owner permission. GPU waits never execute on task workers.
9. Split Streamline/provider lifecycle from frame/device/presentation calls: application lifecycle owns initialization/shutdown, coordinator owns render-facing calls, and no engine mutex is held across external SDK work without an explicit re-entry proof.

Validation:
- Serial/threaded image and packet-consumption parity D3D12/Vulkan.
- Artificial slow render proves bounded queue/memory and expected producer backpressure.
- Repeated resize/minimize/restore/level change/capture/shutdown/device-loss policy stress.
- Owner-thread assertions trigger on deliberate wrong-thread calls.
- Deliberate worker submit/present/GPU-wait/provider entry fails fast; provider enabled/disabled/failure shutdown proves no lock-order cycle or callback-after-destroy.
- Capture shows GameThread N+1 / RenderThread N overlap without a routine GameThread wait and uses the canonical thread labels.

Acceptance gate:
- L Sections 19-21 (reconciliation, acceptance checklist, and required completion report) and Section 24 (Principal Graphics Engineering review gate) pass for the entire touched changelist, including directly affected neighboring ownership paths; no undocumented exception remains.
- The binding Prompt 13-29 changelist design gate passes for the entire prompt changelist, including touched neighboring counterparts.
- Mutable RendererSystemRoot/RHI has one `RenderCoordinator` owner on `RenderThread`.
- Queue depth is fixed and no accepted slot is stranded.
- Direct host-thread Prepare/Record/Submit product path is deleted; serial mode uses same consumer.
- No separate RHI translation thread or worker submission exists.
- Queue/provider locks that remain have a documented invariant and API reason; they do not create a second submission owner.

Positive patterns: single owner, bounded frame queue, sequenced `RenderControlCommandQueue`, explicit backpressure, ordered shutdown.
Forbidden: generally thread-safe renderer root, unbounded queue, main routine render wait, device idle per frame, detached render thread.
~~~

### Prompt 13 implementation record — 2026-07-23

Status: **implementation complete; native runtime acceptance pending**. `Renderer` now owns one private `RenderCoordinator`; serial, threaded zero-ahead, and threaded one-ahead modes all enter the same coordinator contract. Threaded modes create and destroy `RendererSystemRoot`, `FramePipeline`, and mutable RHI state on `Sparkle.RenderThread`. The producer publishes an owned `RenderFramePacket` through fixed slots, while resize, settings, viewport, reload, capture, idle, presentation, and shutdown operations use a separately bounded, sequenced `RenderControlCommandQueue`. D3D12 and Vulkan queue operations assert their creating owner, and Streamline process lifetime is separated from coordinator-owned render calls through an active-call lease that never holds the engine state lock across SDK entry.

Rule 13 access inventory:

| Path | Authority and access | Layout, identity, and deterministic transform | Exact precedent and measured falsifier |
|---|---|---|---|
| game/editor producer → render frame | producer alone writes an acquired slot; coordinator alone consumes and retires it | one slot in serial/zero-ahead, two in one-ahead; ticket is `{slot, SequenceNumber}`; `Free → Writing → Ready → Rendering → Retired → Free` is the only legal order | J `EPIC-RHI` bounded frame-lead precedent and its release/acquire publication rule; replace only if correlated throughput/latency captures show that fixed zero/one-ahead policies are insufficient |
| host → render control | producer assigns the sequence; coordinator applies commands in FIFO order | capacity 64, typed variant payload, optional narrow completion; no RHI command-list or GPU-submission identity is reused | J `EPIC-RHI`, Microsoft D3D12 queue ownership guidance, and the Vulkan threading/external-synchronization specification; change only if burst measurements prove a current product command cannot be bounded/coalesced |
| coordinator → published UI reads | coordinator is mutable authority; producer receives copied viewport/diagnostic results | narrow copied products under one read-state lock and atomic shader generation; no renderer-root or cache pointer escapes | J immutable-publication rule; falsifier is a delayed reader retaining a live renderer pointer or measurable read-state contention |
| coordinator → native queue | D3D12/Vulkan queue creator is the sole submit/present/wait owner | native fence/timeline values remain backend-private; owner assertion is checked before mutable queue entry | Microsoft D3D12 command-queue design and Vulkan external synchronization, cross-referenced by J LC-09/LC-10; falsifier is any required provider/native entry that proves a second queue participant |
| application integration → Streamline | application lifetime owns initialize/shutdown; coordinator call lease owns admitted render-facing calls | locked state and active-call count only; SDK call occurs after unlock; shutdown closes admission then waits for leases | NVIDIA Streamline lifecycle/re-entry guidance catalogued in J; falsifier is provider re-entry/failure stress exposing callback-after-close or a lock-order cycle |

Rule 12/16 placement reconciliation: durable mode configuration is public under `Renderer/Public/Concurrency`; fixed frame transport is private under `Concurrency/FrameQueue`; typed control transport and completion are private under `Concurrency/Control`; lifecycle, thread ownership, sequencing, and published read state are private under `Concurrency/Coordinator`; process-facing vendor lifetime is isolated under `Integrations`; backend bootstrap receives an immutable `RendererBackendConfiguration`; and `Renderer` remains the narrow application facade. Capability implementation is not embedded in the facade or coordinator. Anonymous namespaces were removed repository-wide without introducing `Detail`/`Internal` substitutes: cpp-local policies and transformations are owned by cohesive implementation types, or by an already established domain namespace. Behavioral header bodies touched by this work were moved to cpp files; templates and field accessors remain inline; function-local class/struct declarations are absent.

Deletion ledger: the direct `PrepareHostFrame`/`RecordHostFrame`/`SubmitHostFrame` facade path, renderer-root timer ownership, renderer-root final idle wait, backend-owned Streamline process lifecycle, D3D12 submission/CPU-wait mutex permission, Vulkan submission mutex permission, duplicate queue rejection/stale-ticket warning logs, and the disposable render-concurrency validation executable/target are deleted. No callback adapter preserves the old host phase path.

Focused evidence: DebugGame `SparkleTasks`, `SparkleGameFramework`, both RHI backends, and `SparkleRenderer` compile. The disposable CPU contract check proved fixed-slot backpressure, ticket publication/retirement, bounded FIFO control ordering, and shutdown settlement; an artificial 10 ms consumer delay produced 14.405 ms observed producer backpressure. The repository architecture gate passes, and searches over `Engine`, `Tools`, and `Projects` report zero anonymous namespaces and zero cpp function-local class/struct definitions. The disposable check was deleted after proof. D3D12/Vulkan image parity, resize/minimize/capture/device-loss stress, and native timeline capture remain required before changing this record to **passed**.

## Prompt 14 — Convert Editor UI, Viewport, and Capture Across the Render Boundary

Target CL Title: `SparkleEditor: Move Viewport and Capture Work across the Render Boundary`

~~~text
Implement Prompt 14 only after Prompts 11 and 13 pass.

Objective:
Make all editor-to-render data owned/versioned: copied ImGui draw packets, viewport requests/products, rendering settings commands, preview/capture requests, and narrow completion results. Remove live editor/renderer pointer sharing.

Non-negotiable repository rules:
- Apply `Docs/Architecture/00-Review/L_SparkleEngineIntegrationStyleGuide.md` in full. Treat its ownership, implementation-shape, DOD, concurrency, rendering, naming, validation, structure, principal graphics engineering, and completion-report gates as acceptance criteria. Before editing, list the applicable `PGE-01` through `PGE-15` requirements from A, interpret them through H, and classify each as advance, preserve, not applicable, or blocked with expected evidence. Inspect the current repository, reconcile the whole touched ownership path, verify any AI-assisted work independently, delete replaced paths in the same change, reject role-only scaffolding, and report any justified exception explicitly with its owner, scope, evidence, and deletion or review gate.
- Apply Rule 13: audit every data source, transform, stream, packet, cache, table, upload, and hot traversal touched by this prompt; record the concrete access inventory, authoritative/derived ownership, layout decision, stable identity, deterministic transform, exact source precedent, and measured falsifier.
- Apply Rule 12: audit touched/new files for module and folder ownership, public/private placement, filename-to-primary-type alignment, and nearby misplaced counterparts; complete bounded moves/renames with includes, CMake, and documentation updated before the gate.
- Enforce J's canonical concurrency/rendering vocabulary and Rule 10: search canonical terms plus rejected aliases in the touched scope, use one responsibility per name, and delete temporary compatibility spellings before the gate.
- Audit UI NewFrame/Build/Render, ImDrawData lifetime, RhiImGuiRenderer, viewport panels/contracts, settings panels, preview geometry, diagnostics providers, screenshot/BMP path, resize/docking.
- Search existing packet/viewport/capture handles and reuse/consolidate them.
- Editor main owns ImGui; render coordinator owns GPU UI/viewport/capture resources.
- No broad renderer/task/cache snapshot or editor task debugger.

Required implementation:
1. At UI frame end, copy/convert draw vertices, indices, clip rects, texture handles, and commands into packet-owned EditorRenderPacket; no ImDrawData pointer crosses.
2. Convert texture IDs to stable renderer handles with generation/lifetime policy.
3. Publish viewport request with dimensions, format/color-space, feature/view settings, generation; render publishes product ID/descriptor generation/frame/lifetime token.
4. Editor releases products explicitly or by bounded retirement rule; it never queries a live cache pointer.
5. Route rendering setting changes, preview requests, and capture through sequenced render commands.
6. Implement bounded nonblocking capture pipeline: request → render copy/readback → GPU token → background encode/write → narrow result to editor/tool.
7. Preserve existing product-owned capability/memory diagnostics only where they have a current UI consumer; remove superseded broad access.

Validation:
- Delay render thread beyond next ImGui frame and verify copied draw correctness/lifetime.
- Dock/resize/minimize/viewport recreate stress on D3D12/Vulkan.
- Stale viewport generation rejected; old descriptor/product remains valid until release/retirement.
- Capture normal/resized/minimized/cancelled/bounded-queue; no routine device idle.
- Editor close while capture/preview in flight has no late UI callback.

Acceptance gate:
- L Sections 19-21 (reconciliation, acceptance checklist, and required completion report) and Section 24 (Principal Graphics Engineering review gate) pass for the entire touched changelist, including directly affected neighboring ownership paths; no undocumented exception remains.
- The binding Prompt 13-29 changelist design gate passes for the entire prompt changelist, including touched neighboring counterparts.
- No live ImGui/editor pointer crosses to render coordinator.
- Viewport/capture products have stable versioned ownership.
- Direct editor renderer mutation and redundant diagnostics routes are deleted.
- UI, viewport, settings, preview, screenshot/BMP remain functional.

Positive patterns: copied transient UI data, versioned products, sequenced commands, narrow result.
Forbidden: live descriptor/cache pointer, UI worker callback, WaitForIdle capture, unbounded capture queue.
~~~

### Prompt 14 implementation record — 2026-07-26

Status: **implementation complete; native runtime acceptance pending**. Editor main owns ImGui frame construction and copies draw lists into `EditorRenderPacket`; the render owner resolves only `EditorTextureHandle` values and plays the packet after acquisition. `ViewportRenderRequest`, `ViewportRenderProducts`, settings state, and capture requests cross through owned values or sequenced control commands. Capture admission, GPU readback, completed readbacks, and editor publication are each bounded; BMP encoding/writing runs through the document-scoped `EditorOperationService`. Cancellation no longer waits for a GPU token. The unused public serial viewport-presentation facade and its backend-native texture ID were deleted, so descriptor resolution is private to `FramePipeline`.

PGE reconciliation: `PGE-01`, `PGE-05`, `PGE-07`, `PGE-09`, `PGE-10`, and `PGE-15` advance through the narrow integration contract, bounded nonblocking work, explicit ownership, paired backend implementation, and deletion of the superseded path. `PGE-02`, `PGE-06`, `PGE-13`, and `PGE-14` are preserved; native D3D12/Vulkan resize, minimized-window, capture, and delayed-render evidence is still blocked on an interactive run. `PGE-03`, `PGE-04`, `PGE-08`, `PGE-11`, and `PGE-12` are not applicable. No role-only scaffolding was added.

Rule 13 access inventory:

| Path | Authority, ownership, and layout | Stable identity and deterministic transform | Exact precedent and falsifier |
|---|---|---|---|
| ImGui draw data → `EditorRenderPacket` | editor main is the sole ImGui owner; packet vectors own converted vertices, 32-bit indices, commands, and list ranges | `UiFrameId`, viewport generation, and source list order; callbacks reduce to draw/reset commands before publication | NVIDIA Donut's `donut_app`/`donut_render` ownership split in E; falsified by any `ImDrawData*`/`ImDrawList*` crossing or by delayed playback differing from the source frame |
| ImGui texture value → renderer texture binding | editor copies a packed `EditorTextureHandle`; `EditorTextureRegistry` alone maps it to a backend-native ID | handle index/generation; lookup is independent of descriptor address and packet completion order | NVRHI handle/lifetime guidance linked in E; zero public `ViewportPresentationProduct`, native texture-ID facade, and `RegisterEditorTexture` occurrences is the current structural result |
| viewport request → product publication | producer owns request values; render owner owns GPU products and publishes copied descriptors | viewport/product generation plus render-product and editor-texture handles; stale packet generation is rejected before draw | J immutable publication and MT-31/MT-33; falsifier is resize/recreate with an old handle resolving after retirement |
| settings/preview/capture → render owner | typed `RenderControlCommand` payload, FIFO sequence, capacity 64 | control sequence plus viewport/capture ID; no editor CVar or renderer-cache mutation | J bounded render-control precedent; zero direct editor `CVar*.Set` writes and zero direct editor texture registration routes |
| capture copy → file result | frame pipeline admits at most three GPU readbacks, coordinator publishes at most three completed readbacks, and one editor operation owns encode/write | capture ID plus frame/provider generations; results apply only to the active document request | NVRHI token-lifetime model and J MT-23/MT-33/MT-37; falsifier is queue growth above three, a cancel-time GPU wait, late document application, or routine device idle |

Rule 12 and implementation-shape reconciliation: editor draw conversion remains under `Editor/Private/Rendering`; editor capture orchestration is under `Application/Private/Editor/Capture`; public renderer contracts contain values and stable handles only; packet playback, texture resolution, viewport capture, and backend readback stay in renderer/RHI private owners. `EditorUiFrameRenderer` remains a dedicated application integration file. Non-accessor header bodies touched by the work are in cpp files, no function-local class/struct exists, and repository searches report zero anonymous namespaces. Product diagnostics were retained only for existing panels; the public native viewport-presentation path, direct editor render-view CVar write, direct texture-registration facade, raw viewport texture storage, and blocking capture cancellation were deleted.

Focused evidence: fresh CMake generation succeeded; `ShowcaseEditor` passed a DebugEditor build; the architecture boundary target passed; ShaderCompiler CLI validation passed. Native delayed-render, dock/resize/minimize, cancellation-at-each-stage, and both-backend image/capture runs remain the explicit acceptance evidence gap.

## Prompt 15 — Build the Persistent Render/GPU Scene

Target CL Title: `SparkleRenderer: Build the Persistent Render and GPU Scene`

~~~text
Implement Prompt 15 only after Prompt 13 passes.

Objective:
Stop rebuilding/uploading unchanged scene-wide arrays. Make RenderWorld proxies and GPU-scene slots persistent, apply structural deltas, update dirty dynamic ranges, and prepare token-based removal/retirement while preserving raster and RT identity.

Non-negotiable repository rules:
- Apply `Docs/Architecture/00-Review/L_SparkleEngineIntegrationStyleGuide.md` in full. Treat its ownership, implementation-shape, DOD, concurrency, rendering, naming, validation, structure, principal graphics engineering, and completion-report gates as acceptance criteria. Before editing, list the applicable `PGE-01` through `PGE-15` requirements from A, interpret them through H, and classify each as advance, preserve, not applicable, or blocked with expected evidence. Inspect the current repository, reconcile the whole touched ownership path, verify any AI-assisted work independently, delete replaced paths in the same change, reject role-only scaffolding, and report any justified exception explicitly with its owner, scope, evidence, and deletion or review gate.
- Apply Rule 13: audit every data source, transform, stream, packet, cache, table, upload, and hot traversal touched by this prompt; record the concrete access inventory, authoritative/derived ownership, layout decision, stable identity, deterministic transform, exact source precedent, and measured falsifier.
- Apply Rule 12: audit touched/new files for module and folder ownership, public/private placement, filename-to-primary-type alignment, and nearby misplaced counterparts; complete bounded moves/renames with includes, CMake, and documentation updated before the gate.
- Enforce J's canonical concurrency/rendering vocabulary and Rule 10: search canonical terms plus rejected aliases in the touched scope, use one responsibility per name, and delete temporary compatibility spellings before the gate.
- Audit RenderSceneDataBuilder, BuildRenderSceneGpuData, mesh/material/texture caches, structured buffers, uploads, raster/RT instance planning, BLAS/classic TLAS/PTLAS paths, allocator/budget data.
- Refactor existing caches/builders into one persistent owner; do not add a parallel GpuScene2 or retain full rebuild as product path.
- Remove avoidable work before parallelizing it.
- Preserve frame graph as update/copy/barrier authority and RHI as explicit resource owner.

Required implementation:
1. Give render proxies stable slots for mesh/material/transform/light/skinning/morph/RT metadata and asset generations.
2. Apply create/update/destroy deltas with sequence validation; static asset payloads upload once per generation.
3. Track dirty dynamic values/ranges and coalesce deterministic upload/update plans.
4. Use frame upload rings/pages for transforms, lights, skinning/morph, dirty materials; bound bytes and overflow policy.
5. Maintain separate raster, classic TLAS, and PTLAS instance identities/paths. Asset/BLAS replacement creates a new generation safely.
6. Remove logical proxies immediately at accepted delta but retire GPU allocations/resources by last-use completion token.
7. Integrate existing allocator/memory-budget pressure for growth/eviction/compaction decisions; no unbounded growth.
8. Delete full-scene structured-buffer creation/upload and direct level cache clear after parity.

Validation:
- Static scene after warm-up emits no structural delta/full static upload.
- Single transform/light/material change updates only expected ranges.
- Add/remove/reuse/asset replacement under delayed GPU completion has no stale slot/use-after-free.
- Raster/classic TLAS/PTLAS build/update/trace parity where supported, both backends.
- Measure upload bytes, resource creates, dirty ranges, memory/fragmentation before/after.

Acceptance gate:
- L Sections 19-21 (reconciliation, acceptance checklist, and required completion report) and Section 24 (Principal Graphics Engineering review gate) pass for the entire touched changelist, including directly affected neighboring ownership paths; no undocumented exception remains.
- The binding Prompt 13-29 changelist design gate passes for the entire prompt changelist, including touched neighboring counterparts.
- Persistent slots/generations are authoritative and full rebuild product path is deleted.
- GPU resources outlive all referencing frames and eventually reclaim.
- CPU reduction does not increase GPU time/barriers/descriptors/memory beyond justified budget.
- No routine WaitForIdle for scene changes.

Positive patterns: persistent state, delta application, dirty ranges, generation identity, token retirement.
Forbidden: parallel full rebuild, frame-wide reupload, vector-position identity, immediate GPU free, hidden backend cache ownership.
~~~

### Prompt 15 implementation record — 2026-07-26

Status: **source-complete; runtime acceptance is user-owned**. Under the explicit integration policy that a stage whose only remaining evidence is runtime validation is complete for prerequisite purposes, Prompt 15 passes Prompt 17's source gate. `RenderWorld` applies sequenced deltas to persistent proxies and assigns token-retired `GpuSceneSlot` values. `PersistentRenderGpuScene` owns static RT topology and a frame-indexed ring for lights, mesh-instance values, raster slot indirection, joint matrices, current/previous morph weights, RT instances, and RT materials. `PersistentStructuredBuffer` keeps a CPU shadow, grows geometrically, compares by element, and writes deterministic contiguous dirty ranges; unchanged ring generations emit no writes after warm-up. Static RT topology is replaced only on structural/mesh change, while material-only changes update ringed instance/material ranges. Raster, classic TLAS, PTLAS, and skinned/morphed-BLAS keys use the same stable slot without conflating it with the current packed draw index.

Morph ownership now follows the real asset/instance boundary. `SkeletalCookedMesh` retains undeformed immutable geometry and target deltas; authored and evaluated weights remain authoritative in the entity-owned `MorphWeightStorage`. Extraction publishes every live morph slot in stable `RenderObjectId` order. `RenderDeformationPreparation` reuses per-object current/previous history storage, preassigns flat offset/count ranges, and exposes disjoint copy ranges to the renderer-preparation DAG. `GpuMorphTargetBuffer` uploads target-major immutable deltas once per mesh generation. Raster applies morph deltas before skinning and uses previous weights for motion. RT topology owns one global immutable delta table, hit reconstruction applies the same current/previous transform, and skinned BLAS input applies current morph positions before skinning. The former CPU asset-deformation evaluator was deleted rather than retained as a second product path.

PGE reconciliation: `PGE-02`, `PGE-05`, `PGE-07`, `PGE-08`, `PGE-09`, `PGE-10`, and `PGE-15` advance through persistent RT/raster identity, explicit morph-before-skinning math, dirty-range CPU/GPU data flow, paired shader/resource contracts, token retirement, and smaller ownership files. `PGE-01`, `PGE-06`, `PGE-13`, and `PGE-14` are preserved; native captures and performance measurements remain blocked evidence. `PGE-03`, `PGE-04`, `PGE-11`, and `PGE-12` are not applicable. AI-assisted edits were independently reconciled across the authoritative ECS store, immutable mesh upload, raster vertex path, RT hit reconstruction, BLAS input, frame-graph bindings, pass metadata, shader registrations, and accumulation invalidation; no portfolio-only type or unused morph buffer was retained.

Rule 13 access inventory:

| Path | Authority, ownership, and layout | Stable identity and deterministic transform | Exact precedent and falsifier |
|---|---|---|---|
| `RenderWorldDelta` → persistent proxy table | render owner validates and commits create/update/destroy; logical destroy is immediate | `RenderObjectId` maps to one `GpuSceneSlot`; minimum completed retired slot is reused deterministically | J Tutorial 6 and MT-31/MT-33; falsifier is stale/out-of-order acceptance, packed-vector identity, or slot reuse before every recorded queue token completes |
| immutable mesh handle → `GPUMeshCache` | cache owns one GPU mesh per asset generation and an O(1) handle lookup | `GpuMeshHandle` plus mesh asset generation; replacement removes the old lookup before publication | NVIDIA Donut persistent scene buffer/dirty interface at pinned commit `bc1ea24` and NVRHI handle guidance linked in E/J; falsifier is a steady-state linear handle scan or mutable asset pointer in `MeshDraw` |
| scene values → GPU-scene payloads | `RenderGpuScenePayloadBuilder` performs pure packing; `PersistentRenderGpuScene` owns storage/update policy | stable-slot sparse mesh/RT arrays plus packed raster indirection; source order is deterministic | J persistent GPU-scene design and MT-28/MT-29; zero `BuildRenderSceneGpuData`, `SourceInstanceIndex`, `GpuScene2`, and completion-order merge occurrences |
| dynamic payload → structured-buffer ring | current RHI frame index selects one of `FramesInFlight` owners; each owner keeps capacity and shadow bytes | element stride and ascending source index; adjacent changed elements coalesce into one write | D3D12 upload-resource and Vulkan host-visible range-update ownership described in J's RHI use-case inventory; falsifier is any unchanged post-warm-up write, overlapping frame-slot write, or whole-buffer replacement for one value |
| RT topology/material split | persistent static owner holds vertices/skin influences/indices; per-frame owners hold sparse hit instances/materials | structural/material/texture revisions trigger only their owned transformation; binding view is per in-flight frame | NVRHI lifetime tracking and J MT-33/MT-39; falsifier is a material edit recreating topology, a delayed frame observing a later CPU binding view, or invalid sparse instance bounds |
| morph authority → raster/RT deformation | entity `MorphWeightStorage` owns current weights; immutable mesh generation owns target-major deltas; render history owns previous weights; GPU scene owns in-flight packed ranges | `RenderObjectId` selects temporal history and `GpuSceneSlot` selects shader instance state; target/vertex and object ordering are deterministic; retained packing is `48 * vertex * target` CPU bytes plus one raster and one RT GPU copy, while dynamic ring bytes are `2 * 4 * summed targets * FramesInFlight` | J MT-27/MT-28/MT-29/MT-31 and the same immutable/static versus dynamic split used for skinning; falsified by instance weights mutating a mesh asset, per-frame delta upload, completion-order offsets, raster/RT disagreement, morph-after-skinning order, or representative morph assets exceeding the measured scene-memory budget |
| logical removal → GPU reclamation | slot allocator captures last submitted tokens; RHI resource services already defer physical release by recorded last use | queue-specific submission tokens, never arbitrary frame distance | NVRHI lifetime tracking plus J LC-08/MT-33; falsifier is delayed completion followed by early slot/resource reuse or failure to reclaim after all tokens complete |

Rule 12 and implementation-shape reconciliation: orchestration remains in `FramePipeline`; GPU-scene ownership, slot retirement, payload transformation, and structured-buffer dirty updates are separate files under `SceneData/GpuScene`; immutable per-mesh morph upload is in `Meshes/GpuMorphTargetBuffer.*`; temporal range planning and copying are in `SceneData/Preparation/RenderDeformationPreparation.*`; shader ABI data is in `ShaderData/MorphTargetShaderData.h`; backend offset writes remain in their D3D12/Vulkan resource services. `RenderSceneGpuData` is a non-owning frame binding view, not a second owner. `RenderSceneGpuData.cpp` lost the scene-wide build/upload implementation. Product mesh batching no longer collects hot-path diagnostic counters or emits the one-time compatibility warning; requested editor mesh diagnostics retain their existing on-demand collector. `GPUMeshCache::Resolve` no longer scans the cache. Touched headers contain only declarations, templates, setters/getters, and trivial predicates; `RaytracedGBufferPassParameters::Describe` and the touched ReSTIR constructors moved to cpp ownership. No function-local type or anonymous namespace was introduced.

Deletion ledger: frame-wide owning scene-buffer reconstruction, revision fingerprints, arbitrary delayed slot reuse, packed raster position as shader identity, duplicate PTLAS stable-index spelling, raw `GPUMesh*` in draw data, the public full-scene builder name, hot-path batch diagnostic collection/logging, the linear GPU-mesh handle scan, per-instance initial-weight mutation of `SkeletalCookedMesh`, and the superseded CPU `MeshMorphEvaluator` are deleted. No fallback full rebuild or second morph-deformation path remains.

Focused evidence before build execution was stopped at the user's request: fresh CMake generation succeeded; DebugEditor `ShowcaseEditor` passed before the final morph integration; DebugGame `ShowcaseRuntime` had previously passed before the final cleanup and a final incremental DebugGame run was intentionally terminated on request. The earlier ShaderCompiler GBuffer cook and architecture boundary target passed before this morph integration. Current static reconciliation reports zero anonymous namespaces, zero function-local type additions, zero renderer `GameWorld`/ECS references, zero superseded `MeshMorphEvaluator` references, and paired morph fields across producer, mesh upload, GPU-scene ring, raster pass, ray-traced GBuffer, path-traced indirect, and ReSTIR indirect paths. Per user instruction, no build, shader cook, test executable, or native validation was run after the morph source change. Compilation, DXIL/SPIR-V reflection, GPU upload-byte/resource-create/fragmentation captures, D3D12/Vulkan raster/classic/PTLAS parity, and delayed-completion stress remain in the user's manual runtime evidence ledger; they do not block the next prompt's source implementation.

## Prompt 16 — Complete Runtime Residency and Generation-Based Reload/Retirement

Target CL Title: `SparkleRenderer: Add Runtime Residency and Generation-Based Retirement`

~~~text
Implement Prompt 16 only after Prompts 04 and 15 pass.

Objective:
Connect asynchronous CPU asset generations, render uploads, shader package replacement, readiness, fallback, eviction, and deferred retirement without worker waits or routine device idle.

Non-negotiable repository rules:
- Apply `Docs/Architecture/00-Review/L_SparkleEngineIntegrationStyleGuide.md` in full. Treat its ownership, implementation-shape, DOD, concurrency, rendering, naming, validation, structure, principal graphics engineering, and completion-report gates as acceptance criteria. Before editing, list the applicable `PGE-01` through `PGE-15` requirements from A, interpret them through H, and classify each as advance, preserve, not applicable, or blocked with expected evidence. Inspect the current repository, reconcile the whole touched ownership path, verify any AI-assisted work independently, delete replaced paths in the same change, reject role-only scaffolding, and report any justified exception explicitly with its owner, scope, evidence, and deletion or review gate.
- Apply Rule 13: audit every data source, transform, stream, packet, cache, table, upload, and hot traversal touched by this prompt; record the concrete access inventory, authoritative/derived ownership, layout decision, stable identity, deterministic transform, exact source precedent, and measured falsifier.
- Apply Rule 12: audit touched/new files for module and folder ownership, public/private placement, filename-to-primary-type alignment, and nearby misplaced counterparts; complete bounded moves/renames with includes, CMake, and documentation updated before the gate.
- Enforce J's canonical concurrency/rendering vocabulary and Rule 10: search canonical terms plus rejected aliases in the touched scope, use one responsibility per name, and delete temporary compatibility spellings before the gate.
- Audit current mesh/texture/material/animation/shader load/reload, resource caches, upload service, frame graph copy work, shader package/reflection/runtime/PSO ownership, WaitForIdle reload paths.
- Reconcile LC-01, LC-08, LC-14, and LC-16. Trace idle calls through wrappers to native drains and remove duplicate nesting, not merely the highest-level spelling.
- Extend one asset-generation/residency contract; do not create per-type incompatible async state machines where common states suffice.
- Refactor touched reload/cache code and delete old idle/swap route after parity.
- Preserve shader ABI, providers, RT tables/pipelines, both backends, and previous active generation on failure.

Required implementation:
1. Implement bounded states Unloaded/Reading/Decoding/ReadyForUpload/Uploading/Resident/Evicting/Retired/Failed per asset generation with type-specific payloads.
2. Blocking reads/decode use I/O/background lanes and publish immutable payload; workers never wait for residency.
3. Render coordinator schedules upload/copy through frame graph/RHI; Resident only after completion token.
4. Components/packets/proxies carry stable handles/generations and use existing fallback/omit policy until ready.
5. Bound decoded CPU bytes, pending upload bytes, resident budget, and request backlog. Cancellation discards unpublished CPU data; GPU-enqueued data retires normally.
6. Materialize immutable shader runtime generation after package/reflection validation; swap at safe frame boundary.
7. Keep old PSOs/layouts/shader tables/provider resources alive through last-use tokens. Failure leaves old generation operational.
8. Remove routine renderer.WaitForIdle from shader reload/scene asset replacement.
9. Replace scene invalidation, image-provider refresh, frame-execution refresh, and resize/recreate drains with generation/token retirement where supported. If swap-chain or backend reinitialization temporarily requires a drain, exactly one coordinator-owned site performs it and the deletion/tokenization blocker is recorded; FramePipeline → device service → swap-chain must not issue repeated drains for one resize.
10. Harden D3D12/Vulkan allocation record and pending-release synchronization: mutate on the render owner where possible, snapshot diagnostics under a narrow lock and consume after unlock, never return an unowned mutable record across the lock, and retire only after last-use tokens.

Validation:
- Read/decode/upload completion reordered/delayed; stale generation never becomes active.
- Cancel/fail at every state; old generation/fallback behavior correct.
- Shader HLSL/Slang DXIL/SPIR-V/reflection/layout/parameter verification parity.
- Reload during classic TLAS/PTLAS, temporal/provider, capture, viewport work with delayed GPU completion.
- Profile/assert the exact native idle count for reload, scene invalidation, provider refresh, resize, and shutdown; ordinary paths are zero and one resize cannot recursively drain through multiple layers.
- Budget/backpressure/eviction stress and eventual retirement.

Acceptance gate:
- L Sections 19-21 (reconciliation, acceptance checklist, and required completion report) and Section 24 (Principal Graphics Engineering review gate) pass for the entire touched changelist, including directly affected neighboring ownership paths; no undocumented exception remains.
- The binding Prompt 13-29 changelist design gate passes for the entire prompt changelist, including touched neighboring counterparts.
- No frame worker waits for I/O/upload/residency.
- One coherent generation visible per packet/recording run.
- Previous valid generation survives all failure paths.
- Routine reload/replacement uses tokens, not device idle.
- Every retained idle is final shutdown, unrecoverable device reinitialization, or one documented swap-chain boundary with a named later deletion condition; allocation diagnostics cannot race retirement or hold their lock while formatting/calling external code.

Positive patterns: staged state machine, immutable generation, later-frame readiness, fallback, deferred retirement.
Forbidden: synchronous load in frame path, mixed shader generations, raw resource pointer in packet, immediate unload after cancellation.
~~~

## Prompt 17 — Decompose Renderer Preparation into a Task DAG

Target CL Title: `SparkleRenderer: Decompose Renderer Preparation into a Task DAG`

~~~text
Implement Prompt 17 only after Prompts 10 and 15 pass.

Objective:
Replace monolithic mutable renderer preparation with pure/coarse task nodes over immutable packet/render-world inputs and task-private outputs: transforms/bounds, visibility, batching, lighting, skinning/morph, material classification, RT planning, deterministic merge.

Non-negotiable repository rules:
- Apply `Docs/Architecture/00-Review/L_SparkleEngineIntegrationStyleGuide.md` in full. Treat its ownership, implementation-shape, DOD, concurrency, rendering, naming, validation, structure, principal graphics engineering, and completion-report gates as acceptance criteria. Before editing, list the applicable `PGE-01` through `PGE-15` requirements from A, interpret them through H, and classify each as advance, preserve, not applicable, or blocked with expected evidence. Inspect the current repository, reconcile the whole touched ownership path, verify any AI-assisted work independently, delete replaced paths in the same change, reject role-only scaffolding, and report any justified exception explicitly with its owner, scope, evidence, and deletion or review gate.
- Apply Rule 13: audit every data source, transform, stream, packet, cache, table, upload, and hot traversal touched by this prompt; record the concrete access inventory, authoritative/derived ownership, layout decision, stable identity, deterministic transform, exact source precedent, and measured falsifier.
- Apply Rule 12: audit touched/new files for module and folder ownership, public/private placement, filename-to-primary-type alignment, and nearby misplaced counterparts; complete bounded moves/renames with includes, CMake, and documentation updated before the gate.
- Enforce J's canonical concurrency/rendering vocabulary and Rule 10: search canonical terms plus rejected aliases in the touched scope, use one responsibility per name, and delete temporary compatibility spellings before the gate.
- Audit RenderSceneDataBuilder and all called builders/caches/scratch vectors, pass setup dependencies, temporal/lighting/RT planners, profiler scopes.
- Search existing frame-graph/task graph structures but do not make frame graph schedule non-render CPU policy or create a second task runtime.
- Refactor one builder responsibility at a time; delete shared scratch and old entry point as consumers migrate.
- Keep frame-graph setup/compile serial initially unless existing evidence justifies bounded private topology reuse.
- Pre-mortem MT-02–06, MT-13, MT-19–31, MT-36, MT-39, and MT-41–43. Do not turn one monolithic builder into many tiny tasks that still contend on the same caches/vectors; the target is pure ranges and earliest-start dependencies.

Required implementation:
1. Define immutable run inputs and task-local/preallocated output ranges for each preparation responsibility.
2. Produce transformed bounds and previous-transform ranges, then build per-view visibility/relevance/LOD results with stable `RenderObjectId` tie-breaks. Do not leave “visibility” as an unimplemented placeholder node.
3. Produce visible-light classifications/compact records with stable light identity. Where the current shadow path consumes raster casters, build per-light/view shadow frusta and task-private caster lists; merge in deterministic light/view/object order. If it does not, record the exact first consumer and do not add an unused caster system.
4. Produce skinning/morph ranges, raster pass/material eligibility, and distinct BLAS input plus classic TLAS/PTLAS plans. Preserve build/update/refit/rebuild/compaction decisions supported by the current RHI rather than flattening RT work into one list.
5. Reconcile current `MeshDraw`, `MeshRenderItem`, `MeshInstanceBatch`, and `MeshInstanceBatchBuilder` before adding a draw representation. Separate cacheable view-independent draw state from dynamic per-view state only where existing consumers require it; define invalidation and referenced-resource generation lifetime.
6. Build stable pass buckets, draw sort keys and compatible instance batches. Preserve explicit transparent ordering and report why each key field affects correctness or state-change cost.
7. Build explicit prerequisite DAG and joins; no task mutates renderer global caches or pushes into shared vectors.
8. Partition coarse ranges with serial thresholds; use per-execution arenas/reusable buffers and deterministic keys.
9. Pre-resolve/materialize any lazy resource/PSO/layout state needed by preparation/recording, and preassign per-view/pass constant/descriptor/upload requirements.
10. Merge/sort visibility, caster, batch, light, material, GPU-scene dirty-range and RT results deterministically independent of completion order.
11. Keep mutable frame-graph builder commit serial. Only measured expensive setup producers may emit private declaration records for stable owner merge; do not concurrently mutate `FrameGraphBuilder`.
12. Preserve current/previous transforms, sample/reset semantics, reservoir light identity/order, classic TLAS/PTLAS distinction, path/reference accumulation, provider tags.
13. Emit private existing-profiler scopes for critical path and queue delay; no public DAG report/UI.
14. Delete monolithic mutable builder routes and shared scratch after parity.

Validation:
- Serial DAG and 1/2/N workers produce equivalent scene arrays/plans/images.
- Randomized delays and task order do not alter per-view visible sets, light packets, shadow caster order, pass buckets, transparent order, batches, histories, RT instances, or provider metadata.
- Tiny/editor workload selects serial/grouped/dynamic path; large multi-view animation/draw/light/shadow/RT workload shows positive preparation critical-path result.
- No global cache/data race under stress and native validation remains clean.
- Report merge/scratch overhead and Amdahl limits, not only worker utilization.

Acceptance gate:
- L Sections 19-21 (reconciliation, acceptance checklist, and required completion report) and Section 24 (Principal Graphics Engineering review gate) pass for the entire touched changelist, including directly affected neighboring ownership paths; no undocumented exception remains.
- The binding Prompt 13-29 changelist design gate passes for the entire prompt changelist, including touched neighboring counterparts.
- Preparation dependencies and exclusive outputs are explicit and tested.
- Every applicable renderer-front-end row in K's Renderer/RHI Use-Case-to-Prompt Coverage ledger has real output and proof; unavailable shadow/draw/RT cases have a named non-applicability record, not an implied implementation.
- No worker wait/global mutable scratch/lazy cache creation remains in task bodies.
- Feature preservation matrix passes serial/parallel.
- Speedup is positive on representative large case without unacceptable small-case/GPU regression.

Positive patterns: pure task functions, preplanned ranges, deterministic fan-in, serial threshold, measured critical path.
Forbidden: tiny task per object, shared vector push/mutex, parallel lazy cache fill, claim speedup from utilization alone.
~~~

### Prompt 17 implementation record — 2026-07-26

Status: **source-complete; runtime acceptance is user-owned**. Prompt 10 is complete and Prompt 15 is source-complete under the explicit prerequisite policy above. `RenderPreparationGraph` replaces the deleted `RenderSceneDataBuilder` entry point and compiles one bounded-capacity SparkleTasks topology for transform/bounds, visibility/material classification, skinning copies, morph copies, light classification, deterministic merge/batching, and distinct RT work-plan publication. Cache, texture, material, mesh upload, and CVar resolution happen once on the render owner in `RenderPreparationInputResolver` before submission. Every range task reads the immutable run input and writes an exclusive preallocated span; no task waits, fills a lazy cache, mutates an asset, broadcasts an event, pushes into a shared vector, records RHI commands, or mutates `FrameGraphBuilder`. `RenderPreparationMerger` publishes stable raster order and the BLAS/classic-TLAS/PTLAS work records after the explicit join. The existing Tasks-private profiler records the named nodes' queue delay and execution interval, so no renderer DAG-report or diagnostics product was added.

PGE and workload reconciliation:

| Requirement | Classification | Prompt 17 evidence |
|---|---|---|
| `PGE-01` architecture and system design | preserve | renderer projection remains behind the immutable game/render contract; frame graph and RHI authority do not move into Tasks |
| `PGE-02` advanced rendering | preserve | raster, classic TLAS, PTLAS, ray-query shadow, path/reference, ReSTIR, skinning, and morph inputs retain their existing consumers |
| `PGE-03` neural rendering | not applicable | no neural inference/model boundary is touched |
| `PGE-04` ML training/inference optimization | not applicable | no ML workload is touched |
| `PGE-05` GPU/system performance | advance | off-frustum draws are removed from raster indirection/batches while RT retains its separate complete relevance plan |
| `PGE-06` hardware/API breadth | preserve | no backend-native policy or D3D12/Vulkan contract changes |
| `PGE-07` C++ ownership/debuggability | advance | serial lazy resolution, reusable execution arena, exclusive task ranges, explicit join, and deterministic publication replace shared mutable builder state |
| `PGE-08` CPU/GPU architecture | advance | avoidable O(n²) auto-batch scans and per-frame deformation lookup maps are removed before parallelism |
| `PGE-09` mathematics | advance | transformed world AABBs and six-plane frustum rejection are explicit and reused by PTLAS partition planning |
| `PGE-10` profiling/optimization | advance | coarse named SparkleTasks nodes expose queue delay/critical path through the existing private profiler; serial thresholds bound tiny-work overhead |
| `PGE-11` AI-assisted engineering | preserve | every generated edit was independently traced from extraction through raster/GPU-scene/classic/PTLAS consumers; no placeholder system remains |
| `PGE-12` driver development | not applicable | no native driver or backend command ownership changes |
| `PGE-13` communication | preserve | ownership, access, deletion, feature, non-applicability, and runtime evidence ledgers are recorded here |
| `PGE-14` prioritization | preserve | the change removes avoidable scans/allocations and exposes measured falsifiers before adding lower-level recording parallelism |
| `PGE-15` principal graphics judgment | advance | stateful/lazy owners stay serial, only pure ranges fan out, and raster visibility is deliberately separated from RT relevance |

Sponza is the small/regression and serial-threshold workload; Bistro is the large object/material/light/deformation crossover workload; San Miguel is the cross-scene identity, ordering, and memory-pressure workload. No source contains a scene-name branch, asset-ID special case, or scene-specific threshold. Image, timing, and backend evidence for those workloads is intentionally assigned to the user's later manual run.

Rule 13 access inventory:

| Data path | Authority, ownership, and layout | Stable identity and deterministic transform | Exact precedent and measured falsifier |
|---|---|---|---|
| `RenderWorld` + `RenderFrameDynamicData` -> resolved objects | render owner resolves immutable proxy/asset handles, material slots, GPU mesh handles, and previous transforms before tasks; `RenderPreparationRun` retains reusable vector capacity | inputs and retained previous transforms are sorted by `RenderObjectId` and joined by a linear merge, never vector-position identity | J renderer-preparation fan-out and MT-02/MT-19/MT-28/MT-31; falsified by a task reaching `GetOrUpload`, a completion-order append, or a different serial/worker object order |
| local bounds + transforms -> world bounds | `RenderObjectPreparation` reads one input range and writes the matching `PreparedRenderObject` range | eight-corner affine transform followed by min/max reduction in input index order | DirectXMath transform contract already used by PTLAS; falsified by transformed-bounds mismatch or PTLAS recomputing a different bound |
| world bounds + view -> raster relevance | visibility tasks read transformed bounds only after the transform group completes; output is one boolean/classification/distance per object | six frustum planes; opaque/alpha/transparent classification; `RenderObjectId` resolves equal sort keys | existing `Frustum::ExtractFromViewProjection`; falsified by an off-frustum raster slot, missing on-frustum object, or completion-order-dependent visible set |
| dynamic skinning/morph -> flat deformation arrays | extraction publishes object-sorted rows; serial preparation preassigns offsets; range tasks copy exclusive matrix/weight spans; history commits only after successful graph settlement | `RenderObjectId` merge walk, ascending offsets, current/previous arrays with identical layout | Prompt 10 animation output storage and Prompt 15 GPU-scene ring; falsified by joint/morph overlap, per-task allocation/push, asset mutation, or temporal mismatch |
| dynamic lights -> compact light collections | one prepared record per source row; workers classify exclusive slots; serial commit preserves source order | stable `RenderObjectId` accompanies directional/point/spot/rect records | existing `RenderLightCollection` reservoir identity contract; falsified by light order/identity drift between worker counts |
| prepared objects -> raster batches | `MeshInstanceBatchBuilder` owns sort/batch policy; result owns a raster draw-index indirection separate from all RT objects | classification, render-state key, material generation/index, mesh handle, material slot, skeleton ID, mesh kind, then `RenderObjectId`; transparent rows use descending camera distance then `RenderObjectId` | existing authored/shared group contract plus J batching guidance; material/mesh/skeleton keys protect binding/geometry compatibility, render state reduces state changes, and distance preserves transparent correctness; falsified by an incompatible batch or unstable transparent order |
| prepared objects -> RT work plan | merge continuation emits BLAS inputs and separate classic/PTLAS index arrays; classic builder, PTLAS planner/build, strategy capacity, and RT GPU payload consume them | `GpuSceneSlot` remains shader/TLAS identity; `MeshInstanceIndex` is only a bounded frame-local lookup | Prompt 15 persistent-slot contract and existing classic/PTLAS strategy split; falsified by a consumer reverting to packed raster position or frustum culling reflective/off-screen RT geometry |
| published scene -> persistent GPU scene/frame graph | `PersistentRenderGpuScene` remains the sole dirty-range/upload owner after the CPU DAG; frame-graph declaration/compile/recording stays serial | stable slot sparse payload plus raster indirection; dirty ranges remain ascending and owner-committed | Prompt 15 persistent GPU-scene precedent; falsified by a task mutating GPU scene, descriptors, RHI, or `FrameGraphBuilder` |

Concurrency and hazard inventory: `RenderPreparationGraph::Execute` is render-owner-only and `TaskExecutor::Submit` is the synchronous host boundary. Transform partitions conflict only with the later visibility group; lighting, skinning, and morph ranges can overlap transform/visibility and each other. The four completion groups feed one `WhenAll`, one deterministic merge, then one RT-plan continuation. The cached topology uses capacity buckets and policies `(objects: threshold 128, grain 64, max 8)`, `(lights: 32, 16, max 4)`, and `(skinning/morph ranges: 64, 16, max 8)`. The retained arena is `O(objects + lights + skinning ranges + morph ranges)` temporary records; published storage is `O(objects + lights + summed joints + summed morph weights)`. Input spans are cleared before `Execute` returns, so the reusable arena never retains a packet/component view. There is no per-partition vector or shared scratch. The remaining Amdahl serial region is cache/materialization, stable sort/batching merge, history commit, stateful PTLAS partition update, persistent GPU-scene dirty comparison, and frame-graph work; the user's Bistro/San Miguel capture must show that their cost does not erase range-task savings.

Rule 12 and implementation-shape reconciliation: `Frame/Builders/FrameContextBuilder.*` invokes the graph and owns frame-context composition; `SceneData/Preparation/RenderPreparationGraph.*` owns topology/orchestration; `RenderPreparationTasks.*` owns bounded range-callback plumbing; `RenderPreparationInputResolver.*` owns serial materialization; `RenderObjectPreparation.*`, `RenderLightPreparation.*`, and `RenderDeformationPreparation.*` own pure capabilities/history; `MeshInstanceBatchBuilder.*` owns batching policy; `RenderPreparationMerger.*` owns deterministic publication; `RenderPreparationRun.h` is the private collaboration arena; RT strategy files retain provider-specific state and build/refit policy. Substantive classes have matching file names, headers contain declarations/trivial accessors only, and no function-local type or anonymous/invented `Details`/`Operations` namespace was introduced. The misplaced `Frame/Core/FrameContext.cpp` implementation and free `BuildFrameContext` entry point were replaced by the filename-aligned builder. The GBuffer drawer now owns its operations directly and its repeated per-batch warning/assert instrumentation was removed. CMake already discovers renderer sources through the module's existing source collection; no parallel source list or target was added.

Non-applicability records: Sparkle currently has no mesh LOD representation or LOD-selecting raster consumer, so visibility publishes exact frustum relevance without adding an unused `LOD` field; the first future multi-LOD mesh consumer must extend `PreparedRenderObject` and the batch key together. Current shadows are ray-query consumers: `DirectShadowSignalPassCommon::SetRayQueryParameters` binds the scene TLAS and light buffers, so there is no raster shadow-caster list or shadow-frustum consumer to populate. The current raster mesh consumer is the GBuffer batch drawer, so classification-aware GBuffer buckets are the complete current pass-bucket surface; no unused depth/shadow bucket framework was added. Stateful PTLAS partition assignment remains serial in `RayTracingTopLevelScenePlanner` because mutating that history from a worker would violate task-private output ownership; the DAG produces the distinct immutable PTLAS input plan it consumes.

Preservation ledger: current/previous transforms, skinning/morph order and offsets, authored/shared instance groups, explicit transparent ordering, material alpha/two-sided state, stable reservoir light identity, sparse GPU-scene slot identity, classic TLAS refit selection, PTLAS partition policy, ray-query shadows, path/reference accumulation, temporal/provider metadata, capture, and serial frame-graph/RHI recording remain on their prior owners. Failure policy is no partial history commit: temporal history advances only after the complete graph reports success; topology failure clears preparation history and returns no task-produced ranges.

Deletion ledger: `RenderSceneDataBuilder`, `RenderMeshDrawBuilder`, `RenderMeshMorphBuilder`, their shared mutable scratch/entry paths, the old `SceneData/Builders` batch/light implementations, the O(n²) compatible-batch scan, per-frame transform/deformation lookup maps, packed raster position as RT identity, RT consumers that independently rescan all mesh instances, the GBuffer source-local operations pseudo-owner, repeated hot per-batch warning strings, and touched RT `Details`/`Operations` pseudo-owners are deleted. Rejected aliases and the old include paths have zero source consumers.

Static source reconciliation was performed without invoking a build, shader cook, test executable, or runtime, per user instruction. Native compilation, randomized completion/delay, serial versus 1/2/N equivalence, Sponza overhead, Bistro/San Miguel crossover/critical path, D3D12/Vulkan images, and RT/provider validation remain the user's manual evidence ledger and do not block source-stage completion under the stated policy.

## Prompt 18 — Add D3D12 Worker Recording Contexts

Target CL Title: `SparkleRHI: Add D3D12 Worker Command Recording Contexts`

~~~text
Implement Prompt 18 only after Prompt 17 passes.

Objective:
Make D3D12 command recording ownership safe for future frame-graph fan-out: per-worker/per-frame/per-queue allocator/list contexts, explicit lease lifetime, worker-local or preassigned transient upload/descriptor allocation, and token-based reset/reuse. Do not enable general parallel pass recording yet.

Non-negotiable repository rules:
- Apply `Docs/Architecture/00-Review/L_SparkleEngineIntegrationStyleGuide.md` in full. Treat its ownership, implementation-shape, DOD, concurrency, rendering, naming, validation, structure, principal graphics engineering, and completion-report gates as acceptance criteria. Before editing, list the applicable `PGE-01` through `PGE-15` requirements from A, interpret them through H, and classify each as advance, preserve, not applicable, or blocked with expected evidence. Inspect the current repository, reconcile the whole touched ownership path, verify any AI-assisted work independently, delete replaced paths in the same change, reject role-only scaffolding, and report any justified exception explicitly with its owner, scope, evidence, and deletion or review gate.
- Apply Rule 13: audit every data source, transform, stream, packet, cache, table, upload, and hot traversal touched by this prompt; record the concrete access inventory, authoritative/derived ownership, layout decision, stable identity, deterministic transform, exact source precedent, and measured falsifier.
- Apply Rule 12: audit touched/new files for module and folder ownership, public/private placement, filename-to-primary-type alignment, and nearby misplaced counterparts; complete bounded moves/renames with includes, CMake, and documentation updated before the gate.
- Enforce J's canonical concurrency/rendering vocabulary and Rule 10: search canonical terms plus rejected aliases in the touched scope, use one responsibility per name, and delete temporary compatibility spellings before the gate.
- Audit D3D12CommandContext, allocator/list slot selection, command queues/tokens, frame-resource rotation, upload allocator, descriptor heaps, resource barriers, PassBinder, PipelineStateManager, debug names/markers.
- Reconcile LC-09, LC-11, LC-13, LC-14, and LC-18 explicitly: `D3D12LinearAllocator` atomics, `D3D12DescriptorAllocator` mutex, queue submit/CPU-wait locks, frame-resource native waits, and GPU allocation-record synchronization are inputs to the redesign, not approved final recording primitives.
- Refactor existing context ownership; do not add an unrelated command system or D3D12-only public renderer API.
- Shared RHI contract must remain implementable by Vulkan without hiding native rules.
- Render coordinator remains sole reset coordination/submission owner.
- Pre-mortem MT-09–10, MT-19, MT-25–27, and MT-32–43. Follow AMD's allocator-per-recording-thread-per-frame rule as a minimum ownership shape, then bound Sparkle's actual frames × contexts × queues memory and preserve coordinator submission.

Required implementation:
1. Define public move-only `RhiCommandRecordingLease` expressing `ERhiQueueType`, frame slot, actual worker/context identity, one-task ownership, command list, upload/descriptor pages, and `RhiSubmissionToken` retirement. The backend-private owner is `D3D12CommandRecordingContext`; do not expose allocator/list pointers as a substitute lease.
2. Allocate/reset D3D12 command allocator/list state by buffered-frame × worker/context × queue policy. Never reset until GPU token proves prior use complete.
3. Ensure one allocator/list is recorded by at most one task and one lease closes/returns exactly once.
4. Provide worker-local/preassigned upload constant pages and transient descriptor blocks; remove shared hot cursor contention for leased recording.
5. Materialize PSOs/root signatures/pass runtime before lease fan-out; assert no lazy creation reachable.
6. Preserve command-list naming, PIX events, validation, fatal HRESULT handling.
7. Keep submission API coordinator-only and in compiled order.
8. Remove the leased recording path's shared atomic bump cursor. Give each lease a disjoint page/slice and encode reset eligibility with its GPU completion token; do not assume `m_Offset` acquire/release makes reset safe.
9. Separate persistent descriptor allocation from transient recording allocation. Persistent allocator locking may remain under the render owner; no per-draw/per-dispatch leased work contends on `D3D12DescriptorAllocator::m_mutex`.
10. Restrict infinite fence waits to coordinator/outer-host frame-slot reuse, explicit flush, or shutdown. Development waits identify queue/token/elapsed time and follow the device-hang/loss policy; task workers return dependencies instead of waiting.

Validation:
- Lease acquire/release/reset misuse and wrong-thread/concurrent-use negative assertions.
- Compile-time/API checks prove `RhiCommandRecordingLease` is move-only, does not expose a raw allocator/pool as ownership, and cannot be confused with `RenderCommandContext` or `RhiSubmissionToken`.
- Random worker migration/delay over many buffered frames and queue types.
- D3D12 debug layer + GPU-based validation with existing serial recording through leased contexts.
- Upload/descriptor page overflow and retirement tests.
- Shared-cursor contention comparison, disjoint-range proof, deliberate reset-before-token failure, and repository check that no SparkleTasks worker reaches `WaitForSingleObject`/GPU wait.
- Memory scaling documented for frames × contexts × queues and bounded by configuration.

Acceptance gate:
- L Sections 19-21 (reconciliation, acceptance checklist, and required completion report) and Section 24 (Principal Graphics Engineering review gate) pass for the entire touched changelist, including directly affected neighboring ownership paths; no undocumented exception remains.
- The binding Prompt 13-29 changelist design gate passes for the entire prompt changelist, including touched neighboring counterparts.
- No concurrent allocator/list use or premature reset is possible through supported API.
- Public code contains `RhiCommandRecordingLease`; backend code contains `D3D12CommandRecordingContext`; rejected `RecordingContextLease`/`WorkerRecordingContext` aliases and filenames are absent.
- Existing serial renderer records correctly through new context contract.
- No shared hot upload/descriptor cursor in leased path.
- Remaining queue/descriptor/accounting locks have a non-recording invariant and owner; `D3D12LinearAllocator` is removed from the concurrent leased hot path or made owner-local with proven reset semantics.
- No worker submission, lazy PSO creation, marker loss, or D3D12-only policy leaked above RHI.

Positive patterns: exclusive lease, token retirement, worker-local transient allocation, prewarm.
Forbidden: allocator mutex around concurrent record, reset by worker without token, submit from task worker, device idle reset.
~~~

### Prompt 18 implementation record — 2026-07-26

Status: **source-complete; runtime acceptance is user-owned**. Prompt 17 is source-complete under the explicit prerequisite policy above. The public RHI now uses move-only `RhiCommandRecordingLease` values for non-current command recording. A lease carries queue, buffered-frame slot, context ID, partition/task identity, command list access, the completed token that made the slot reusable, and bounded upload/descriptor page descriptions. It binds its actual recording thread on first command-list/page access, closes at most once, can move to the coordinator after close, and cannot submit itself. `RenderDeviceServices` remains the owner-thread-checked coordinator submission surface. The current graphics list remains an explicitly named coordinator-owned frame continuation; copy and compiled frame-graph batches record through leases and submit in compiled order.

PGE reconciliation:

| Requirement | Classification | Prompt 18 evidence |
|---|---|---|
| `PGE-01` architecture and system design | preserve | frame graph retains dependency/order authority, RenderDeviceServices retains submission authority, and RHI owns backend command resources |
| `PGE-02` advanced rendering | preserve | raster, compute, copy, classic RT, PTLAS, provider, UI, and capture command semantics are unchanged |
| `PGE-03` neural rendering | not applicable | no neural model or inference boundary is touched |
| `PGE-04` ML training/inference optimization | not applicable | no ML workload is touched |
| `PGE-05` GPU/system performance | advance | frame-global atomic constant-buffer allocation is replaced by disjoint recording pages and exact-token slot reuse |
| `PGE-06` hardware/API breadth | advance | one backend-neutral lease/submission contract is implemented by D3D12 and kept serially implementable by Vulkan without exposing native allocator/list types |
| `PGE-07` C++ ownership/debuggability | advance | move-only ownership, explicit close/consume, stable context identity, wrong-thread assertions, and dedicated capability files replace bare list-reference ownership |
| `PGE-08` CPU/GPU architecture | preserve | coordinator submission, explicit queue dependencies, native queue rules, and GPU completion tokens remain authoritative |
| `PGE-09` mathematics | advance | capacity is a concrete `frames × queues × contexts` bound and page offsets are aligned, monotonic, disjoint ranges |
| `PGE-10` profiling/optimization | advance | the contention hypothesis, memory bound, overflow policy, and runtime falsifiers are explicit rather than inferred from utilization |
| `PGE-11` AI-assisted engineering | preserve | generated changes were traced through frame setup, frame-graph batches, PassBinder, upload services, both backends, present, and shutdown; no unused parallel recorder was added |
| `PGE-12` driver development | not applicable | no driver implementation is introduced; D3D12 API ownership is engine-side |
| `PGE-13` communication | preserve | access, hazard, memory, lock, preservation, deletion, and manual evidence ledgers are recorded here |
| `PGE-14` prioritization | preserve | Prompt 18 establishes ownership and removes contention without prematurely enabling general parallel pass recording |
| `PGE-15` principal graphics judgment | advance | resources are precreated, workers may only record an exclusive lease, the coordinator alone submits, and backend-specific state stays private |

Rule 13 access inventory:

| Data path | Authority, ownership, and layout | Stable identity and deterministic transform | Exact precedent and measured falsifier |
|---|---|---|---|
| compiled submission batch -> recording lease | `FrameGraphSubmissionExecutor` selects the queue and dependency tokens; `RenderDeviceServices` owner thread acquires/consumes the lease; backend state remains private | identity is `(queue, frame slot, context ID)` plus partition/task identity; batches still execute and submit in compiled index order | J `AMD-RDNA`, `AMD-RPS`, `NV-VK`, LC-09, and MT-25–27/32–43; falsified by two live leases naming one context or completion-order submission |
| lease -> D3D12 allocator/list | `D3D12CommandRecordingContext` exclusively owns 48 precreated slots (`2 frames × 3 queues × 8 contexts`); each slot has one state machine: available, recording, closed, submitted | first access on the exclusive recording worker resets the completed allocator/list and binds the actual thread; native close remains on that thread and retains fatal HRESULT/debug-message handling | J's pinned Microsoft D3D12 multithreading sample and `AMD-RDNA` allocator reset rules; falsified by wrong-thread use not asserting, double close, or allocator reset while its token is incomplete |
| frame-slot reuse -> retirement wait/reset | the render coordinator visits only the reused buffered-frame row, coalesces the greatest outstanding token per queue, waits completion, and resets CPU-side slot pages/cursors; native allocator/list reset is deferred to first exclusive worker access | the lease exposes the completed prior retirement token; a new submission replaces the slot token | J LC-09/LC-18; falsified by deliberate reset-before-token success, a task-worker fence wait, or a routine device-idle reset |
| pass uniform data -> upload address | `PassBinder` supplies the active `RenderCommandList`; each D3D12 command list has one backend-private direct association with its exclusive 256 KiB page, so binding performs no slot scan, hash lookup, or shared allocation; Vulkan retains its serial backend allocator behind the same RHI signature | 256-byte aligned monotonic offsets within one lease page; the page checks its bound recording thread and no page is shared by live leases | pinned NVRHI command-list-local upload ownership, J LC-13, and the deleted atomic `D3D12LinearAllocator`; falsified by overlapping GPU addresses, a slot scan/shared CAS/mutex in the D3D12 leased path, or overflow escaping the bounded failure policy |
| lease -> transient descriptor range | every D3D12 slot owns a preassigned 256-descriptor shader-visible block; per-lease allocation increments only the slot-local cursor | base CPU/GPU handles plus deterministic ascending offset/count; allocator/pool pointers never cross the public API | J LC-11; falsified by a draw/dispatch entering `D3D12DescriptorAllocator::m_mutex`, overlapping live ranges, or allocation beyond 256 descriptors succeeding |
| persistent descriptor request -> global heap | `D3D12DescriptorHeapManager` and `D3D12DescriptorAllocator` remain the persistent registry owner; their mutex is used during owner-side allocation/free and one-time recording-block preassignment only | existing descriptor handle/index identity is preserved | existing descriptor service contract; falsified by leased recording allocating from the persistent registry or holding its mutex across command recording |
| authored shader pass -> pass runtime | `FrameGraphBuilder` asks `PipelineStateManager` to materialize each typed pass once while the persistent graph is built; frame execution performs read-only lookup and carries no materialization callback or per-frame prewarm traversal | pass C++ type remains the key; repeated pass registration is a no-op after first materialization and reload creates every registered replacement before publication | NVIDIA pipeline-precreation guidance, the existing `PipelineStateManager` reload contract, and MT-36; falsified by execute-time map insertion, PSO/root-signature creation, or a missing-runtime lookup not asserting |
| compiled batch -> recorded commands | `FrameGraphSubmissionExecutor` owns dependency-token resolution, lease selection, compiled-order submission, and no pass mechanism; dedicated `FrameGraphBatchRecorder` owns resource tracking, compiled barriers, scopes, and pass callbacks | compiled batch/pass indices remain the only semantic order; recording completion has no authority | AMD RPS range-recording versus ordered submission and pinned NVRHI command-list recording/submission separation; falsified by the recorder submitting, the executor discovering pass resources, or pass order diverging from the compiled plan |
| closed lease -> queue submission/token | only owner-thread-checked `RenderDeviceServices` consumes leases; D3D12 submits the closed native list and resolves tracked resources; Vulkan adapts the same neutral contract serially pending Prompt 19 | explicit wait-token span and compiled batch index determine order; returned `RhiSubmissionToken` is the retirement authority | J LC-09/LC-18; falsified by a worker reaching submit/present, marker loss, or queue order differing from the compiled plan |

Capacity and memory ledger: `RhiFrameConstants::FramesInFlight` is 2, `RhiQueueTypeCount` is 3, and `MaximumContextsPerFrameQueue` is 8. D3D12 therefore precreates 48 allocator/list contexts. Each owns a 256 KiB upload page and 256 transient descriptors: 12 MiB of upload capacity and 12,288 descriptors, plus native allocator/list objects. This replaces the deleted 8 MiB (`2 × 4 MiB`) frame-global constant-buffer pool, a bounded 4 MiB increase that buys disjoint recording ownership across all queue/context combinations. Context exhaustion, upload overflow, and descriptor overflow are bounded failures; slot vectors and native resources are built once, the current coordinator lease uses in-place optional storage, and acquisition/recording performs no heap growth.

Hazard and lock reconciliation: `D3D12CommandRecordingContext::BeginFrame` is the submitted-slot reuse authority and is called by the render coordinator. It coalesces and waits the greatest exact token per queue before authorizing slot reuse; a discarded, unsubmitted lease may reset only its CPU-local page/cursor without a GPU wait. Native allocator/list reset is deferred until first access by the next exclusive recording owner. No SparkleTasks callback invokes a GPU/OS wait. `D3D12CommandQueue` retains its owner-side queue/fence synchronization and reports queue name, submission token, elapsed milliseconds, and native wait result on development timeout; shipping infinite waits remain confined to owner frame-slot reuse, explicit idle/flush, or shutdown. `D3D12DescriptorAllocator::m_mutex` remains persistent bookkeeping and one-time pool initialization, never a leased recording primitive. D3D12 GPU allocation-record synchronization is paid while upload pages are precreated, not per constant binding. No shared atomic upload cursor remains.

Rule 12 and implementation-shape reconciliation: `Public/Commands/RhiCommandRecordingLease.h` owns the backend-neutral value vocabulary; `Private/Commands/RhiCommandRecordingLease.*` owns move/close/release mechanics; `Private/D3D12/Commands/D3D12CommandRecordingContext.*` owns slot lifecycle and retirement; `D3D12RecordingUploadPage.*` owns one exclusive mapped page; device services orchestrate creation/submission only; PassBinder supplies data to the active command list; `FrameGraphBuilder` owns typed registration/prewarm; `FrameGraphBatchRecorder.*` owns pass command recording; and `FrameGraphSubmissionExecutor.*` owns compiled waits and submission. Primary types match filenames, substantive behavior is in `.cpp` files, touched non-template headers contain only declarations/trivial accessors, and no function-local type, anonymous namespace, or invented `Details`/`Operations` owner remains. The descriptor manager's former nontrivial inline allocation bodies were moved to its `.cpp`, and its legacy member names were clarified.

Reference-backed frame-graph cleanup: the temporary general `ExecuteShaderPass`/`AddPassWithRuntime` surface was rejected. AMD RPS exposes graph-compiled command ranges and records those ranges into exclusive command buffers before ordered submission; NVRHI keeps command-list recording, resource lifetime, and device submission explicit; NVIDIA's command-buffer guidance calls for precreated pipelines, meaningful command-list granularity, and limited submits. Sparkle therefore retains its existing typed `Draw`/`Dispatch` vocabulary, adds only the constrained `DispatchIf` needed for frame-conditioned lighting variants, materializes their runtimes at graph construction, and uses the same compiled plan for serial recording. `LightingRayTracingPasses.h` now states only the ray-query/TLAS predicates instead of duplicating parameter setup, runtime lookup, and execution. `FrameGraphBuilder.h` is smaller than its pre-change form, and `FrameGraph.h` receives no generic runtime execution escape hatch.

Preservation ledger: D3D12 object names include queue/frame/context; native close still drains debug-layer messages and treats failed HRESULT as fatal; existing PIX/diagnostic scopes remain command-list behavior; resource tracking resolves on the returned submission token; graphics initialization, cross-queue waits, final graphics continuation, present, explicit flush, and capture paths retain coordinator authority. Vulkan implements the common acquire/consume surface serially without claiming command-pool worker safety; Prompt 19 remains responsible for its backend-private pool/page retirement.

Deletion ledger: `D3D12CommandContext.*`, `D3D12FrameResource.*`, `D3D12LinearAllocator.*`, the bare public `BeginCommandList`/`SubmitCommandList` ownership path, the frame-global atomic CAS upload cursor, per-binding recording-slot scans, heap-backed current-lease storage, the native infinite frame-resource wait, dynamic D3D12 slot creation during recording, role-only frame-graph `Operations` classes, duplicated conditional-lighting setup/execution, and nontrivial descriptor-manager header bodies are deleted. The temporary generic `ExecuteShaderPass`/`AddPassWithRuntime` API never reaches the final source. Source contains the canonical `RhiCommandRecordingLease` and `D3D12CommandRecordingContext`; rejected compatibility aliases and filenames are absent.

Static source reconciliation was performed without invoking a build, shader cook, test executable, or runtime, per user instruction. Compile-time move/copy assertions are present. Native compilation, D3D12 debug/GPU validation, wrong-thread/double-close/reset negative runs, randomized worker delay/migration, upload/descriptor overflow, serial image parity, and contention measurements remain the user's manual evidence ledger and do not block source-stage completion under the stated policy.

## Prompt 19 — Add Vulkan Worker Recording Contexts

Target CL Title: `SparkleRHI: Add Vulkan Worker Command Recording Contexts`

~~~text
Implement Prompt 19 only after Prompt 17 passes. Match the common lease semantics proven by Prompt 18 without copying D3D12 internals.

Objective:
Make Vulkan command recording ownership safe: per-worker/per-frame/per-queue-family command-pool/buffer contexts, external synchronization compliance, transient descriptor/upload ownership, reset/retirement, and preserved debug/validation behavior. Do not enable general parallel pass recording yet.

Non-negotiable repository rules:
- Apply `Docs/Architecture/00-Review/L_SparkleEngineIntegrationStyleGuide.md` in full. Treat its ownership, implementation-shape, DOD, concurrency, rendering, naming, validation, structure, principal graphics engineering, and completion-report gates as acceptance criteria. Before editing, list the applicable `PGE-01` through `PGE-15` requirements from A, interpret them through H, and classify each as advance, preserve, not applicable, or blocked with expected evidence. Inspect the current repository, reconcile the whole touched ownership path, verify any AI-assisted work independently, delete replaced paths in the same change, reject role-only scaffolding, and report any justified exception explicitly with its owner, scope, evidence, and deletion or review gate.
- Apply Rule 13: audit every data source, transform, stream, packet, cache, table, upload, and hot traversal touched by this prompt; record the concrete access inventory, authoritative/derived ownership, layout decision, stable identity, deterministic transform, exact source precedent, and measured falsifier.
- Apply Rule 12: audit touched/new files for module and folder ownership, public/private placement, filename-to-primary-type alignment, and nearby misplaced counterparts; complete bounded moves/renames with includes, CMake, and documentation updated before the gate.
- Enforce J's canonical concurrency/rendering vocabulary and Rule 10: search canonical terms plus rejected aliases in the touched scope, use one responsibility per name, and delete temporary compatibility spellings before the gate.
- Audit VulkanCommandContext, command pools/buffers, queue families/timeline semaphores, frame resources, descriptor pools/sets, uploads, pipeline/layout creation, debug labels.
- Reconcile LC-10, LC-12, LC-14, LC-15, LC-16, and LC-18 explicitly: native queue mutex, the broad VulkanDescriptorAllocator mutex, allocation/pending-release records, validation-message ingestion, command-context/swap-chain device-idle calls, and timeline waits.
- Implement the same public `RhiCommandRecordingLease` responsibilities as D3D12 with backend-private `VulkanCommandRecordingContext` pool details.
- Refactor existing mutable slot/pool path; no second Vulkan command abstraction or duplicate context tree.
- Render coordinator remains submission/reset authority; frame graph remains state/order authority.
- Pre-mortem MT-09–10, MT-19, MT-25–27, and MT-32–43. NVIDIA/AMD guidance agrees on exclusive pool ownership, meaningful command-buffer size, reuse, limited submits, and application-owned parallelism; backend validation and measurements decide the exact Sparkle policy.

Required implementation:
1. Allocate command pools with explicit thread/context and queue-family ownership; command buffers are leased exclusively.
2. Reset/recycle pool/buffer only after all submitted buffers from the relevant frame/context are GPU-complete.
3. Provide worker-local/per-frame transient descriptor pool blocks and upload pages or preassigned slices.
4. Precreate pipeline/layout/pass runtime before fan-out and assert no concurrent lazy mutation.
5. Preserve debug labels, object names, validation/synchronization diagnostics, timeline token behavior.
6. Keep coordinator-only ordered submission and explicit cross-queue semaphore/timeline edges.
7. Remove the single shared mutable pool/slot assumption after serial parity.
8. Split descriptor responsibilities by lifetime. Recording leases use worker/frame-local pool blocks or preassigned tables; persistent registered descriptors and retirement remain render-owned. Do not hold the broad allocator mutex across `vkUpdateDescriptorSets`, pool allocation, formatting, callbacks, or other unbounded/native work when a snapshot/two-phase design can avoid it.
9. Preserve `VulkanNativeQueue::SubmissionMutex` only as Vulkan-required external synchronization for an explicitly shared native queue; coordinator ownership remains the engine contract. Validation callback ingestion appends bounded owned messages and owner-side processing occurs after unlocking.
10. Replace context/resource-reuse device-idle calls with relevant timeline completion. Final backend/swap-chain destruction may use one owner drain; one resize must not drain in FramePipeline, device services, and VulkanSwapChain.

Validation:
- Vulkan validation and synchronization validation under worker migration/delay and many frames.
- API/name parity check proves D3D12 and Vulkan expose the same `RhiCommandRecordingLease` while only backend-private code names `VulkanCommandRecordingContext` and Vulkan command pools/buffers.
- Deliberate concurrent pool/reset misuse triggers engine assertion/validation in negative test.
- Descriptor pool exhaustion/overflow and command buffer retirement stress.
- Descriptor allocation contention and lock-held-native-call audit; concurrent validation callback/drain; exact idle-count resize/recreate test; provider/native queue sharing test.
- Serial renderer uses lease contract with image/state parity.
- Compare common RHI semantics to D3D12 while documenting backend-specific ownership differences.

Acceptance gate:
- L Sections 19-21 (reconciliation, acceptance checklist, and required completion report) and Section 24 (Principal Graphics Engineering review gate) pass for the entire touched changelist, including directly affected neighboring ownership paths; no undocumented exception remains.
- The binding Prompt 13-29 changelist design gate passes for the entire prompt changelist, including touched neighboring counterparts.
- No Vulkan command pool is concurrently accessed through supported paths.
- Rejected `RecordingContextLease`/`WorkerRecordingContext` aliases and filenames are absent; no D3D12 allocator/list noun leaks into the Vulkan/common contract.
- Pool reset/reuse is completion-token safe.
- Shared lease contract is backend-neutral without erasing Vulkan queue-family/pool rules.
- No worker submit, lazy pipeline/layout creation, or marker loss.
- No recording worker contends on the persistent descriptor registry, processes validation callbacks, waits for device idle, or gains submit/present authority through the native queue mutex.

Positive patterns: thread-local pool ownership, backend-private implementation, timeline retirement, paired validation.
Forbidden: one pool shared with mutex as final design, D3D12 assumptions copied blindly, pool reset at CPU frame end, worker queue submit.
~~~

### Prompt 19 source-completion record

Source-stage status: **pass**. Prompt 19's implementation and static reconciliation are complete. General parallel frame-graph recording remains disabled for Prompt 20. Native compilation, Vulkan validation-layer execution, synchronization validation, deliberate misuse runs, image parity, and timing measurements were not run by instruction; they remain the manual runtime evidence listed below and do not represent a source gap.

Principal Graphics Engineering classification:

| ID | Classification | Prompt 19 evidence |
|---|---|---|
| `PGE-01` | preserve | the existing backend-neutral lease and submission service remain the integration boundary; Vulkan pool, descriptor, upload, and native queue types remain private |
| `PGE-02` | preserve | classic TLAS, PTLAS, scratch-address resolution, resource tracking, and queue-token lifetime continue through the same command-list and frame-graph contracts |
| `PGE-03` | not applicable | no neural model/operator is introduced |
| `PGE-04` | not applicable | no model layout, precision, or ablation choice is introduced |
| `PGE-05` | advance | all recording resources have explicit bounds; the runtime ledger requires p95/p99 CPU recording time, queue interference, and overflow evidence before parallel recording is enabled |
| `PGE-06` | advance | engine ownership assertions, Vulkan validation/synchronization validation, native queue external synchronization, and exact driver/API configurations are separated in the evidence plan |
| `PGE-07` | advance | move-only leases, exclusive slots, immutable recording views, opaque lifetime tokens, owner-only reset/submit, and dedicated capability files express ownership directly |
| `PGE-08` | not applicable | no new rendering equation or numerical algorithm is introduced; alignment and capacity arithmetic are recorded under Rule 13 |
| `PGE-09` | advance | command pools, queue families, descriptor pools, mapped upload pages, image aspects, resource addresses, timeline semaphores, and capability-gated RT descriptor types are explicit |
| `PGE-10` | advance | the change removes shared registry traversal from recording and defines lock, allocation, submission-count, memory, and crossover falsifiers rather than claiming speedup from thread count |
| `PGE-11` | preserve | AI-assisted edits were independently traced through allocation, publication, recording, submission, retirement, resize, capture, and teardown; review corrected pending-resource republication, ambiguous mixed address lookup, and last-use publication after reference release |
| `PGE-12` | not applicable | no training/offline inference package is introduced |
| `PGE-13` | advance | the bounded hypothesis is now reviewable: exclusive Vulkan recording contexts should remove pool/registry contention without increasing submits; Prompt 20 must measure or reject that hypothesis |
| `PGE-14` | preserve | no Vulkan driver, Windows/Linux, or hardware result is claimed without a run; runtime claims remain explicitly manual |
| `PGE-15` | advance | the shared mutable context and frame-global upload allocator were deleted, recording lookup became a dedicated immutable table, and submission preparation moved to the narrow command-submission boundary |

Rule 13 access inventory:

| Data path | Authority, derived ownership, and layout | Stable identity and deterministic transform | Exact precedent and measured falsifier |
|---|---|---|---|
| compiled frame-graph plan -> recording preparation | `FrameGraph` remains order/state authority; `RhiCommandSubmissionService::PrepareCommandRecording` asks the backend owner to publish final descriptor/resource read views after transient materialization | one publication point precedes all batch recording; Vulkan publishes descriptor/resource views and D3D12 publishes its resource-retention view | AMD RPS compiled-range ownership and NVRHI explicit command-list preparation; falsified by execute-time registry mutation, lazy pipeline/layout creation, or a worker observing a newly mutated owner vector |
| queue/frame/owner request -> lease | `VulkanCommandRecordingContext` owns a fixed `frame slot * queue type * context index` matrix; the coordinator selects one available slot and publishes only the common move-only lease | `(queue type, frame slot, context ID)` plus partition/task identity; acquisition order is deterministic within the precreated row and the actual thread is bound separately on first use | Vulkan command-pool external-synchronization rules and NVIDIA command-buffer guidance; falsified by two live leases naming one context or a pool touched from two recording threads |
| lease -> command pool/buffer | each slot owns one queue-family-specific transient command pool, one primary command buffer, and one `VulkanRenderCommandList`; first use on the exclusive worker resets the completed pool, begins the primary buffer with `ONE_TIME_SUBMIT`, and binds the actual thread | slot state is `Available -> Recording -> Closed -> Submitted` or `Discarded`; the coordinator alone waits exact retirement and authorizes reuse, while native reset/begin/end stay on the exclusive recording worker | NVIDIA Vulkan dos/don'ts and AMD RPS per-thread command-buffer recording; falsified by CPU-frame-end reset, second-thread begin/close, or reset before the retirement token completes |
| lease -> transient descriptors | each slot owns one `VulkanRecordingDescriptorPool`; descriptor sets are allocated only by that exclusive command list and the pool is reset with its slot | fixed 256-set bound; supported descriptor kinds are capability-gated, including classic and partitioned AS descriptors; the common lease reports capacity while Vulkan keeps layout-dependent `VkDescriptorSet` allocation behind its command list rather than fabricating a D3D12-style descriptor range | Vulkan descriptor-pool external-synchronization rules; falsified by a worker entering `m_registryMutex`, two slots sharing one pool, unbounded growth, or exhaustion returning a usable set |
| persistent descriptors -> recording read view | `VulkanDescriptorAllocator` remains authoritative for registered descriptors/tables; copy-on-write entry arrays and generation records are published through an atomic immutable view | table generational handles and registered descriptor indices remain stable; recording reads perform bounds/generation/type checks before stack-chunked `vkUpdateDescriptorSets` | NVRHI explicit descriptor-table ownership and J LC-12; falsified by a persistent lock held across `vkUpdateDescriptorSets`, a worker mutation, or a stale table generation resolving |
| resource views -> image-aspect lookup | `VulkanDescriptorManager` owns live/retired image views; a sorted immutable `(VkImageView, aspect mask)` table is derived immediately before recording | native image-view value is the key; lower-bound lookup is deterministic and never traverses the mutable owner vector on a recording worker | Vulkan dynamic-rendering attachment metadata requirements; falsified by worker access to `m_resourceViewRecords`, mismatched depth/stencil aspects, or retirement before the frame-slot token completes |
| lease -> uniform upload | each slot owns one persistently mapped `VulkanRecordingUploadPage`; the active Vulkan command list routes constant allocation directly to its page | 256 KiB capacity, device minimum uniform alignment, monotonic offset, and at most 4096 stable allocation records; returned opaque address resolves only inside the owning page | NVRHI command-list-local volatile upload ownership and J LC-13/LC-14; falsified by overlapping ranges, shared cursor/lock contention, heap allocation per constant, or overflow escaping the bounded empty result |
| live GPU allocations -> recording metadata | `VulkanGpuMemoryAllocator` remains authoritative; `VulkanRecordingResourceTable` owns only immutable copied native metadata and opaque retained-use tokens | resource handle is the primary key and owns one full metadata copy; exact device addresses and buffer-base ranges use compact `(address, read-view index, publication order)` projections, exact lookup precedes range lookup, and pending releases are excluded from future publications | J LC-14/LC-18 and explicit NVRHI resource lifetime; falsified by a worker receiving a mutable allocation record, an AS address resolving through the wrong buffer range, a pending resource being republished, or destruction while a read view/use token exists |
| closed lease -> native queue submit | `VulkanCommandQueue` alone builds timeline waits/signals and submits; `VulkanNativeQueue::SubmissionMutex` covers only the Vulkan native call when logical queues share one `VkQueue` | compiled batch order and explicit wait tokens determine submission; each logical queue has a monotonic timeline value | Vulkan queue external-synchronization rules, AMD RPS ordered submission, and J LC-10/LC-18; falsified by worker submit/present, completion-order submission, a wait on an unsubmitted token, or the mutex spreading into engine scheduling |
| validation callback -> diagnostics consumer | the callback copies into a bounded 256-message deque under its dedicated mutex; `TryPopDiagnosticMessage` removes owned messages after unlocking from callback execution | FIFO order with oldest-message eviction at capacity | Vulkan debug-utils callback rules and J LC-15; falsified by logging/UI/callback execution while the callback mutex is held, unbounded growth, or a borrowed driver message pointer escaping |
| frame-slot reuse/resize/shutdown -> completion | command slots wait their exact queue timeline token before pool/page reset; resize waits the last graphics token and performs one graphics-queue drain for presentation ownership; device idle remains explicit final flush/shutdown only | retirement token belongs to the exact slot submission; no worker-visible wait exists | J LC-16/LC-18; falsified by routine `vkDeviceWaitIdle`, nested resize drains, a task-worker wait, or destruction of a submitted pool/page before completion |
| capture command pool -> capture submission | `VulkanCaptureService` retains a separate owner-thread-only pool per pending readback and submits through `VulkanCommandQueue`; it is not exposed to recording workers or folded into the frame-graph context pool | capture ticket and returned graphics timeline token retain the existing identity/lifetime | existing bounded capture contract; falsified by capture directly calling an unsynchronized shared queue, reusing a pending pool before its token, or a worker acquiring capture ownership |

Capacity and performance ledger: Vulkan precreates `2 frames * 3 queue types * 8 contexts = 48` exclusive command contexts. Each owns one 256 KiB upload page and one descriptor pool capped at 256 sets, for 12 MiB of mapped upload capacity plus 12,288 descriptor-set slots and native pool/buffer objects. The authoritative runtime falsifiers are context/pool/page overflow count, command-pool reset wait time, recording-view publication bytes/time, persistent-registry mutex contention during recording (target zero), native submit count (must not rise in Prompt 19), p50/p95/p99 CPU recording time, and GPU/frame-pacing parity. These are bounds and hypotheses, not unrun performance claims.

LC and MT reconciliation:

- `LC-10`: logical queues sharing a native `VkQueue` share exactly one `VulkanNativeQueue::SubmissionMutex`; queue methods remain coordinator-owned and workers gain no submit/present authority.
- `LC-12`: persistent descriptor mutations remain locked and copy-on-write; recording uses immutable views and slot-local native pools, and no native descriptor update occurs under `m_registryMutex`.
- `LC-14`: workers receive copied `VulkanRecordingResource` values and opaque use tokens, never allocation or pending-release records. Pending resources are excluded from the next publication, exact and ranged addresses cannot mask one another, and submission last-use state is published before the protecting reference is released.
- `LC-15`: validation ingestion is bounded and callback-minimal; consumption/logging remains outside callback-held synchronization.
- `LC-16`: context reuse uses timeline completion; resize has one presentation queue drain; final explicit flush/shutdown retains the only device-wide idle path.
- `LC-18`: only the coordinator waits exact timeline tokens, updates last-use state, resets slots, retires descriptors/resources, and submits queues.
- `MT-09/10/19/25-27/32-43`: pool ownership, task affinity metadata, immutable inputs, exclusive outputs, precreated pass runtime, coordinator aggregation/submission, bounded overflow, validation, and serial-first rollout are explicit. Prompt 19 intentionally creates no SparkleTasks work and enables no general parallel pass recording.

Rule 12 and implementation-shape reconciliation:

- `Public/Commands/RhiCommandRecordingLease.*` remains the only backend-neutral recording value contract.
- `Private/Vulkan/Commands/VulkanCommandRecordingContext.*` owns the slot state machine, native pool/buffer lifecycle, exact retirement, debug names, and lease callbacks.
- `Private/Vulkan/Descriptors/VulkanRecordingDescriptorPool.*` owns only one lease-local native descriptor pool.
- `Private/Vulkan/Resources/VulkanRecordingUploadPage.*` owns only one lease-local mapped page and its bounded allocation table.
- `Private/Vulkan/Memory/VulkanRecordingResource.*` defines copied recording metadata and the opaque use token; `VulkanRecordingResourceTable.*` owns publication, deterministic lookup, and temporary recording references.
- `VulkanDescriptorAllocator` owns persistent descriptor entries/read publication; `VulkanDescriptorManager` owns resource-view lifetime/aspect publication; `VulkanGpuMemoryAllocator` owns allocation and delayed destruction.
- `VulkanCommandQueue` owns logical timeline sequencing; `VulkanNativeQueue` owns only the Vulkan-required shared native-queue mutex; device services orchestrate initialization, publication, submission, presentation, and teardown.
- Primary types match filenames and folders. New substantive methods are defined in `.cpp` files. The touched source contains no function-local type, anonymous namespace, role-only `Implementation`/`Operations`/`Details`/`Helpers` owner, or nontrivial header body outside trivial accessors/setters/templates. CMake uses the existing `CONFIGURE_DEPENDS` recursive source ownership, so no explicit list duplication was added.

Preservation ledger: serial frame-graph batch order, explicit cross-queue timeline edges, debug labels, Vulkan object names, classic TLAS/PTLAS address handling, resource last-use tracking, validation message delivery, swap-chain presentation, capture submission, transient alias planning, and current D3D12 lease semantics remain intact. Pipeline/layout/pass runtimes continue to materialize through `FrameGraphBuilder`/`PipelineStateManager` before execution; Prompt 19 adds no lazy worker creation path and no second scheduler, thread pool, command abstraction, or submission path.

Deletion ledger: `VulkanCommandContext.*`, `VulkanLinearAllocator.*`, the single shared mutable command-pool/slot path, frame-global Vulkan uniform allocation, worker-time live allocation scans, recording-time persistent descriptor-registry traversal, the broad descriptor-pool responsibility inside `VulkanDescriptorAllocator`, role-only `VulkanCommandQueuePolicy`, and temporary broad hardware-facade recording preparation are deleted. The final source contains canonical `RhiCommandRecordingLease`, backend-private `VulkanCommandRecordingContext`, and narrow command-submission preparation; rejected `RecordingContextLease` and `WorkerRecordingContext` source aliases are absent.

Static validation and manual handoff:

- Static reconciliation checks cover stale deleted includes/types, common lease/backend naming, command-pool creators, native queue submit sites, wait/idle sites, descriptor locks/native calls, anonymous namespaces, function-local types, role-only ownership names, header bodies, and whitespace errors.
- Code inspection proves every supported recording slot has one pool/buffer/descriptor pool/upload page, worker operations have no submit/reset/wait entry point, resource/descriptor reads are immutable, and release/reset follows exact token completion.
- Manual runtime evidence remains: Vulkan validation and synchronization validation with delayed/migrated workers; deliberate second-thread/reset misuse; 1/2/N worker context exhaustion; descriptor/upload overflow; many-frame retirement; serial image/state/marker parity; resize/recreate idle counts; validation callback/drain concurrency; provider/native queue sharing; and p50/p95/p99 publication/recording/submit measurements.
- No exception or compatibility path is retained. Under the stated policy that runtime-only validation does not block source completion, Prompt 19 is complete and Prompt 20 may rely on its lease ownership contract after the manual validation ledger passes.

Required completion report:

1. **Outcome** - Vulkan now supplies the common move-only command-recording lease through exclusive frame/queue/context-owned pools, buffers, descriptor pools, and upload pages while submission remains serial and coordinator-owned.
2. **Repository audit** - the existing common lease, D3D12 semantics, Vulkan command context, descriptor allocator/manager, memory allocator, upload service, native queues, swap chain, capture path, and frame-graph execution boundary were inspected; the decision was extend the common lease, replace the Vulkan mutable context/linear allocator, and refactor the existing descriptor/resource owners rather than add a second abstraction.
3. **Ownership** - the render coordinator owns lease acquisition, retirement waits/reuse authorization, submit, presentation, publication, and reclamation; one recording thread exclusively owns native pool reset/begin/end for an active slot; immutable read views and retained-use tokens span recording; exact queue timeline tokens govern reuse and destruction.
4. **Files changed by responsibility** - common RHI files expose only submission preparation and the existing lease; Vulkan `Commands`, `Descriptors`, `Resources`, and `Memory` own their named mechanisms; device services orchestrate; frame-graph execution invokes the single publication boundary.
5. **Orchestration/capability refinement** - device services read as lifecycle orchestration, `VulkanCommandRecordingContext` owns the slot state machine, `VulkanRecordingResourceTable` separates read-view construction, address projection, lookup, and reference lifetime, and queue submission construction is split from the native call.
6. **SOLID/DRY reconciliation** - one context tree, descriptor registry, resource registry, upload path, queue path, and submission authority remain; backend-neutral contracts do not expose Vulkan pool/layout rules and Vulkan does not imitate D3D12 descriptor-range mechanics.
7. **Preservation ledger** - serial batch order, explicit cross-queue waits, markers/object names, raster/classic TLAS/PTLAS behavior, validation delivery, swap-chain presentation, capture, aliasing, and D3D12 lease behavior are retained.
8. **Deletion ledger** - `VulkanCommandContext.*`, `VulkanLinearAllocator.*`, shared frame-global recording allocation, live worker scans, broad recording-time registry locking, role-only queue policy, and rejected aliases are removed.
9. **Structure reconciliation** - new primary types match dedicated private backend files; the existing RHI recursive source discovery owns them; public exposure is limited to the pre-existing common lease/submission service.
10. **Implementation-shape reconciliation** - touched headers contain only declarations, templates, or trivial accessors/setters; no function-local type, anonymous namespace, invented ownership bucket, or new diagnostic framework remains.
11. **DOD reconciliation** - the Rule 13 inventory above records authority, access, compact projections, stable handles/context identity, publication, deterministic lookup, bounded capacities, lifetime, precedent, and falsifiers.
12. **Concurrency reconciliation** - SparkleTasks remains the only worker runtime; Prompt 19 creates no tasks; leases have exclusive outputs, workers have no submit/reset/wait/present entry point, native queue locking is limited to Vulkan external synchronization, callback ingestion is bounded, and shutdown settles exact timeline tokens.
13. **Validation** - source checks used `git diff --check`, canonical/rejected-name `rg` scans, anonymous-namespace and local-type scans, header-body inspection, and audits of command-pool, descriptor-native-call, queue-submit, wait, and idle sites. Runtime configurations and stress cases remain in the manual ledger above.
14. **Performance** - the fixed 48-context/12 MiB capacity equation and required contention, overflow, submit-count, publication-time, p50/p95/p99, pacing, and GPU-parity measurements are recorded; no unrun speedup is claimed.
15. **Naming audit** - source contains canonical `RhiCommandRecordingLease`, `VulkanCommandRecordingContext`, `VulkanRecordingDescriptorPool`, `VulkanRecordingUploadPage`, `VulkanRecordingResourceTable`, and `VulkanNativeQueue`; rejected Vulkan context/linear-allocator and lease aliases return zero production matches.
16. **Limitations and unavailable evidence** - no build, executable, validation-layer run, image comparison, timing capture, Linux result, or hardware/driver claim is reported; general parallel pass recording remains Prompt 20 work.
17. **Acceptance status** - **PASS** under the explicit source-complete/runtime-manual policy, with no partial implementation or undocumented exception.
18. **PGE reconciliation** - the classification/evidence table above advances `PGE-05/06/07/09/10/13/15`, preserves `PGE-01/02/11/14`, marks `PGE-03/04/08/12` not applicable, verifies the AI-assisted ownership path through source review, and introduces no role-only or unsupported platform/hardware claim.

## Prompt 20 — Compile and Execute Parallel Frame-Graph Recording Groups

Target CL Title: `SparkleRenderer: Execute Parallel Frame-Graph Recording Groups`

~~~text
Implement Prompt 20 only after Prompts 18 and 19 pass.

Objective:
Use frame-graph dependencies to compile eligible pass recording groups, lease worker contexts, record concurrently, deterministically aggregate closed native command objects, and submit measured batches in unchanged compiled order with explicit entry/exit resource-state contracts on both backends. Keep preparation, native recording, software translation, aggregation, submission batching, and queue submission as distinct concepts.

Non-negotiable repository rules:
- Apply `Docs/Architecture/00-Review/L_SparkleEngineIntegrationStyleGuide.md` in full. Treat its ownership, implementation-shape, DOD, concurrency, rendering, naming, validation, structure, principal graphics engineering, and completion-report gates as acceptance criteria. Before editing, list the applicable `PGE-01` through `PGE-15` requirements from A, interpret them through H, and classify each as advance, preserve, not applicable, or blocked with expected evidence. Inspect the current repository, reconcile the whole touched ownership path, verify any AI-assisted work independently, delete replaced paths in the same change, reject role-only scaffolding, and report any justified exception explicitly with its owner, scope, evidence, and deletion or review gate.
- Apply Rule 13: audit every data source, transform, stream, packet, cache, table, upload, and hot traversal touched by this prompt; record the concrete access inventory, authoritative/derived ownership, layout decision, stable identity, deterministic transform, exact source precedent, and measured falsifier.
- Apply Rule 12: audit touched/new files for module and folder ownership, public/private placement, filename-to-primary-type alignment, and nearby misplaced counterparts; complete bounded moves/renames with includes, CMake, and documentation updated before the gate.
- Enforce J's canonical concurrency/rendering vocabulary and Rule 10: search canonical terms plus rejected aliases in the touched scope, use one responsibility per name, and delete temporary compatibility spellings before the gate.
- Audit FrameGraphCompiler/Execution/Submission, queue batches/waits, barrier planner, pass side effects, immediate command-list use, provider/present/readback islands, pass markers.
- Extend the existing frame graph; do not create a second render graph or let SparkleTasks decide GPU resource/queue ordering.
- Do not add an Unreal-style software RHI command stream or `RhiThread` in this prompt. Sparkle currently records `RenderCommandList` calls directly into backend command objects; retain that direct architecture and add an ADR entry pointing to J's five-condition translation gate.
- Arbitrary callback passes are serial by default. Typed draw/dispatch pass families become compiler-eligible only after their shared execution contract is audited; eligibility is derived privately from execution shape and graph resources, never exposed as a `FrameGraphBuilder`/`AddPass` recording-policy parameter.
- Refactor execution traversal so serial and parallel modes consume the same compiled plan; delete duplicate executor path.
- Pre-mortem MT-13, MT-19–21, MT-27, MT-29, and MT-32–43. Preserve the single-thread command/barrier/submission order as Epic and AMD's deterministic render-list examples do; parallel completion order has no GPU semantic authority.

Required implementation:
1. Add private compiled `RecordingGroup`/`RecordingPlan` with pass range, queue, prerequisites, initial/final resource states, context requirement, estimated command/recording cost, submission position, serial-island reason. Define `RecordingGroup`, `RecordingChunk`, `SubmissionBatch`, `aggregation`, and `translation` exactly as J Tutorial 16; do not use “merge command lists” unqualified.
2. Preserve frame-graph barrier/aliasing/cross-queue authority. Emit inter-group barriers/primary/coordinator work where independent lists cannot infer transitions.
3. Add a compiler-owned parallel-safety classification/audit for pass execution families: immutable inputs, prewarmed runtime, local transient allocations, no hidden side effects, and documented provider/native affinity. Do not add a recording-policy argument to pass declarations; unaudited callback families remain serial.
4. Begin with independent coarse compute/copy/draw passes whose CPU recording cost exceeds threshold. Combine adjacent tiny compatible groups for recording; never assume one pass equals one task/list/submission.
5. Submit recording tasks through SparkleTasks, lease backend contexts, begin/record/end on the worker, close into a preassigned result slot, then aggregate by `SubmissionOrderKey` independent of completion order.
6. Keep provider/frame-generation/present, screenshot/readback coordination, unaudited interop, and required primary barriers as explicit serial islands.
7. Add deterministic serial/parallel recording control using same plan and barrier comparison.
8. On D3D12, use separate direct lists as the baseline and submit ordered arrays through the existing submission service. Treat bundles as a measured reuse option, not the parallel-recording abstraction. Never concurrently record one list/allocator.
9. On Vulkan, select independent primary buffers for suitable pass groups or ordered secondary buffers for measured intra-rendering-scope work. Record required inheritance; execute secondaries from coordinator/primary work. Do not make secondary buffers mandatory in the common RHI.
10. Compile `SubmissionBatch` boundaries separately from recording groups. Sweep list count and batch size; allow early batches only at existing graph-safe submission points. Preserve queue waits/tokens and one coordinator submit owner.
11. Record draws/dispatches/build inputs per list, task ready/record/close time, aggregation time, command objects per batch, native submit calls/time, first-work availability, GPU gaps, descriptor/upload use, and transient native memory through existing profiling hooks.
12. Keep barrier/render-target/viewport/rendering-scope preamble and postamble in coordinator/primary work by default. Move state into worker lists only when the backend contract, compiled state plan, and validation prove it safe.

Validation:
- Serial/parallel images, pass order, queue waits, barrier entry/exit states, histories, and submission tokens match.
- D3D12 GPU validation and Vulkan synchronization validation under randomized recording delays/migration.
- Marker/object/timestamp continuity across split groups.
- Tiny graph remains grouped/serial; draw/pass-heavy graph measures record critical-path improvement.
- Sweep group/chunk/batch sizes. Track list/buffer count, barriers, descriptor/upload pages, transient memory, aggregation/submit CPU time, first GPU work and CPU/GPU/latency impact.
- A deliberate worker-completion reorder still yields identical closed-list order and submission batches.
- Architecture audit shows no software RHI replay objects, translate thread, raw native-buffer concatenation, or worker queue submission was introduced.

Acceptance gate:
- L Sections 19-21 (reconciliation, acceptance checklist, and required completion report) and Section 24 (Principal Graphics Engineering review gate) pass for the entire touched changelist, including directly affected neighboring ownership paths; no undocumented exception remains.
- The binding Prompt 13-29 changelist design gate passes for the entire prompt changelist, including touched neighboring counterparts.
- Only compiler-classified, structurally audited pass families run parallel and native validation is clean on both backends; pass declarations contain no authored recording policy.
- GPU submission order/resource semantics remain frame-graph-defined.
- Recording-group and submission-batch policies are independently measured; fewer submit calls are not accepted if they create material GPU starvation or latency regression.
- No lazy state, shared transient cursor, or worker submission in parallel execute.
- Performance gain does not buy unjustified GPU/list/memory regression.

Positive patterns: execution-family audit, compiler-derived eligibility, compiled state contract, worker recording/coordinator submit, same serial plan.
Forbidden: pass-level recording-policy parameter, parallelize-all flag, infer barriers inside independent list, execute-time resource creation, second frame graph.
~~~

### Prompt 20 source-completion record — 2026-07-26

Status: **source-complete; runtime acceptance is user-owned**. Parallel recording is an internal frame-graph execution decision, not a pass-authoring feature. `FrameGraphBuilder` retains its established `AddPass`, `Draw`, `Dispatch`, `DispatchAsync`, and `DispatchIf` vocabulary with no recording-policy parameter. Arbitrary callback, external-provider, and presentation work compiles to coordinator serial islands. The audited typed shader execution family compiles to exclusive recording groups, adjacent small groups are combined into bounded chunks, and SparkleTasks records eligible chunks into preassigned lease slots. Serial mode, GPU-timing mode, tiny batches, and zero/one-worker configurations record the same plan into one native command object per submission batch.

Primary-source alignment:

- [Microsoft D3D12 recording guidance](https://learn.microsoft.com/en-us/windows/win32/direct3d12/recording-command-lists-and-bundles) requires exclusive command-allocator recording/reset, reuse only after GPU completion, and allocator pooling across recording threads and frame latency. Sparkle precreates a bounded frame/queue/context matrix, waits exact retirement on the coordinator, and performs allocator/list reset, recording, and close on the exclusive worker.
- [Microsoft D3D12 execution and synchronization guidance](https://learn.microsoft.com/en-us/windows/win32/direct3d12/executing-and-synchronizing-command-lists) keeps queue order and cross-queue synchronization explicit. Sparkle submits one ordered native-list array at each pre-existing compiled `SubmissionBatch` boundary and preserves the frame graph's queue waits and tokens.
- [Khronos Vulkan threading guidance](https://docs.vulkan.org/guide/latest/threading.html) and the [command-buffer usage sample](https://docs.vulkan.org/samples/latest/samples/performance/command_buffer_usage/README.html) require externally synchronized command pools, recommend exclusive pools per recording thread, prefer whole-pool reset/reuse, and warn against many tiny buffers. Each Sparkle partition lease has a distinct queue-family pool/buffer; only its bound worker resets/begins/ends it; `VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT` is used; individual buffer reset and `SIMULTANEOUS_USE` are absent.
- [NVIDIA command-buffer guidance](https://developer.nvidia.com/blog/advanced-api-performance-command-buffers/) and [CPU guidance](https://developer.nvidia.com/blog/advanced-api-performance-cpus/) require useful work per list, balanced worker recording, few native submissions, and a light submission thread. Sparkle groups small work, uses a serial crossover, bounds chunks at eight, removes per-frame task-name formatting/allocation, and leaves aggregation/submission on the coordinator. NVIDIA's approximate 5–10 `ExecuteCommandLists` calls per frame is treated as a profiling heuristic, not an API invariant.
- [AMD RDNA performance guidance](https://gpuopen.com/learn/rdna-performance-guide/) requires application-owned recording threads, allocator/context counts proportional to worker/frame concurrency, reuse rather than per-frame creation, sufficiently coarse command buffers, and batched submission. [AMD RPS range recording](https://gpuopen.com/learn/rps-tutorial/rps-tutorial-part4/) demonstrates parallel range recording followed by deterministic original-order submission with barriers retained in execution order. Sparkle follows that ownership/order model without importing RPS or adding a second graph. AMD's approximate ten-draw/dispatch granularity is retained as a runtime crossover falsifier because Sparkle's compile-time structural-cost estimate is not a draw count.

PGE reconciliation:

| Requirement | Classification | Prompt 20 evidence |
|---|---|---|
| `PGE-01` partner adoption | preserve | one backend-neutral lease/submission boundary and one frame-graph authoring vocabulary remain; no pass caller learns D3D12/Vulkan recording policy |
| `PGE-02` ray/path rendering | preserve | ray-query, classic TLAS, PTLAS, reservoir, reference, temporal, and provider passes retain compiled queue/resource order |
| `PGE-03` neural feature | not applicable | no model/operator path is changed |
| `PGE-04` model-to-kernel | not applicable | no model graph, tensor layout, precision, or kernel is changed |
| `PGE-05` whole-system performance | advance | task count, command-object count, native submits, descriptor/upload capacity, serial crossover, frame latency, and GPU starvation are one bounded measurement contract |
| `PGE-06` workload debugging | preserve | markers and native validation paths remain; exact PIX/GPUView/RenderDoc/RGP evidence is manual |
| `PGE-07` C++ engineering | advance | compiler, recorder, executor, submission service, and backend context owners remain separate; hot result storage is fixed and compatibility policy plumbing is absent |
| `PGE-08` applied mathematics | not applicable | only integer capacity/cost bounds are touched; no rendering or numerical method changes |
| `PGE-09` explicit APIs | advance | D3D12 allocator/list and Vulkan pool/primary-buffer lifecycle, queue synchronization, flags, and backend-private ownership are explicit |
| `PGE-10` CPU/GPU architecture | advance | parallel CPU recording is separated from GPU queue execution; oversubscription, useful-work crossover, driver submission, and frame-latency reuse are explicit falsifiers |
| `PGE-11` ML fundamentals/AI verification | preserve | no ML claim is made; AI-assisted changes were independently traced through compiler, execution, both backends, diagnostics, and submission |
| `PGE-12` training/inference workloads | not applicable | no training or inference workload is touched |
| `PGE-13` productization/communication | advance | primary-source decisions, rejected API policy, ownership inventory, limits, and runtime handoff are recorded here |
| `PGE-14` platform breadth | preserve | both Windows D3D12 and Vulkan source paths are paired; no native Linux, driver, or hardware result is claimed |
| `PGE-15` principal judgment | advance | the pass-policy API was rejected, backend lifecycle defects were fixed at their owners, small-work overhead was removed, and one graph/submission path remains |

Rule 13 access inventory:

| Data path | Authority, ownership, and layout | Stable identity and deterministic transform | Exact precedent and measured falsifier |
|---|---|---|---|
| registered pass -> execution classification | `FrameGraph` owns the cold registered-pass AoS; generic callbacks default to `Callback`, while the existing typed draw/dispatch construction path records `TypedShader` privately | registered pass index and compiled submission position; no caller-authored recording flag or compatibility spelling | AMD RPS compiled ranges and the repository's typed pass/runtime contract; falsified by any public pass-policy parameter or callback entering an exclusive chunk |
| compiled pass/resource declarations -> recording groups | `FrameGraphRecordingPlanCompiler` derives cold groups, resource-state contracts, prerequisites, queue, serial-island reason, and structural cost | pass/group indices follow compiled order; resource entry/exit state is derived from the authoritative barrier plan | AMD RPS ordered ranges and Microsoft explicit state/order rules; falsified by execute-time hazard inference or a group reordering compiled barriers |
| adjacent groups -> recording chunks | the compiler combines compatible small groups and caps a submission batch at eight chunks; batches without a useful parallel range collapse to one coordinator chunk | `SubmissionOrderKey {batch, position}`; chunk construction is a forward deterministic transform independent of workers | NVIDIA/AMD coarse-list guidance; falsified by a tiny graph producing multiple native objects or measured crossover showing the retained threshold regresses |
| chunk range -> SparkleTasks recording | `FrameGraphRecordingExecutor` owns one settled task graph per eligible range and fixed eight-slot result/aggregate arrays; each task writes one exclusive result lease | preassigned partition index plus task identity derived from submission order; completion order cannot select output position | SparkleTasks settled host submission and AMD ordered aggregation; falsified by shared push, completion-order append, worker wait, or per-frame result allocation |
| lease -> native recording context | the active backend owns a fixed `2 frames × 3 queues × 8 contexts` matrix; each slot owns native command state plus disjoint descriptor/upload storage | `(queue, frame slot, context ID)` is resource identity; partition identity is scheduling metadata and actual thread binding is checked separately | Microsoft allocator rules and Vulkan command-pool external synchronization; falsified by concurrent pool/allocator access or reset before exact retirement |
| compiled barriers/pass commands -> native object | `FrameGraphRecordingChunkRecorder` emits alias/resource barriers and pass commands into the chunk's exclusive command list/buffer; arbitrary providers/presentation remain coordinator islands | group/pass order inside each chunk is compiled order; native objects are never concatenated or replayed through a software RHI stream | Microsoft ordered list execution and AMD RPS barrier interleaving; falsified by barriers hoisted ahead of dependent worker work or native state inferred from completion |
| closed native objects -> queue submission | `FrameGraphSubmissionExecutor` aggregates fixed result slots and `RhiCommandSubmissionService` alone submits; D3D12 uses one `ExecuteCommandLists` array and Vulkan one `vkQueueSubmit` command-buffer array per compiled batch | array order is recording-plan order; wait tokens and batch index remain semantic authority | NVIDIA/AMD submit batching and Khronos queue ownership; falsified by worker submit, one native submit per chunk, or changed cross-queue token order |
| GPU timing/markers -> recording mode | existing marker scopes remain command-list local; enabling shared timestamp timing selects the one-command-object serial path so timestamp allocation/order is not raced | pass/chunk labels follow compiled identity; timing mode never silently loses scopes | NVIDIA profiling guidance and the existing diagnostic owner; falsified by parallel mutation of timing vectors/query allocation or missing timing scopes |

Capacity, memory, and critical-path ledger: each active backend precreates 48 contexts and 12 MiB of upload pages (`2 × 3 × 8 × 256 KiB`) plus 12,288 D3D12 descriptor entries or 12,288 Vulkan descriptor-set slots. Only the selected backend is resident. A submission batch owns at most eight result leases in fixed executor arrays, so recording introduces no per-batch result-vector growth. The compiler's structural-cost values `16` target and `32` parallel minimum are starting heuristics, not measured draw counts. The runtime falsifiers are p50/p95/p99 coordinator record time, task ready/record/close spans, command objects and native submits per frame, average draws/dispatches per object, first GPU work, GPU bubbles, frame latency, upload/descriptor high-water, and serial/1/2/N-worker crossover on representative tiny and heavy scenes.

Concurrency and lifetime reconciliation: SparkleTasks remains the only worker runtime and its settled `Submit` boundary joins recording before stack-owned executor state can retire. Workers receive immutable frame/plan/runtime references and one exclusive lease; they do not submit, present, wait for GPU/OS completion, mutate graph structure, or allocate from another lease's cursor. The coordinator acquires leases, waits only on buffered-frame reuse or final context destruction, deterministically aggregates, submits, and attaches the returned token to every submitted slot. D3D12 allocator/list reset and Vulkan pool reset/begin occur on the exclusive recording worker only after coordinator-authorized completion. Cross-queue waits, submission tokens, and GPU order remain compiled frame-graph data rather than task edges.

Rule 12/16 and SOLID/DRY reconciliation: no new public file or CMake surface was required. `FrameGraphRecordingPlanCompiler` owns classification/chunk construction, `FrameGraphRecordingChunkRecorder` owns native command emission, `FrameGraphRecordingExecutor` owns SparkleTasks recording and fixed aggregation, `FrameGraphSubmissionExecutor` owns batch waits/submission orchestration, and each backend recording context owns native resource lifecycle. Touched headers contain declarations, templates, and trivial accessors only; substantive new behavior is in `.cpp`; no anonymous namespace, function-local type, or role-only owner was introduced. Serial and parallel modes consume the same compiled plan and recorder.

Preservation and deletion ledger: raster/compute/copy, classic TLAS/PTLAS, reservoir/reference lighting, temporal state, providers, presentation, capture, markers, queue waits, and final graphics continuation retain their owners. The temporary pass-level recording-policy design and call-site arguments are deleted. The parallel path no longer disables GPU timing silently; timing selects serial recording. Per-frame formatted task names, heap-growing result/aggregate vectors, completion-order metadata duplication, repeated per-slot reuse waits, coordinator-side native reset/begin, and misleading `WorkerIndex` scheduling identity are removed. No bundle/secondary-buffer abstraction, RHI thread, second frame graph, worker submit, or software command replay stream was added.

Required completion report:

1. **Outcome** — eligible typed frame-graph groups record concurrently on D3D12 and Vulkan, then submit as one deterministic ordered array per compiled batch; pass declarations remain unchanged.
2. **Repository audit** — compiler, barrier planner, typed/callback pass construction, execution, diagnostics, task runtime, submission services, D3D12/Vulkan contexts/queues, providers, presentation, capture, and RT paths were traced; existing owners were extended/refined rather than duplicated.
3. **Ownership** — the frame graph owns GPU order/state, the executor owns CPU recording work, leases own exclusive native recording state, the coordinator owns waits/aggregation/submission, and exact tokens own reuse.
4. **Files changed by responsibility** — common RHI lease identity, D3D12/Vulkan recording lifecycle, renderer recording execution/diagnostics, and this integration record are the only touched groups.
5. **Orchestration/capability refinement** — high-level batch execution reads as resolve waits → record → aggregate → submit; native reset/record/close remains backend capability code.
6. **SOLID/DRY reconciliation** — one compiled plan, recorder, submission path, backend context tree, and authoring API remain; serial and parallel behavior do not duplicate pass traversal.
7. **Preservation ledger** — current rendering features, barriers, queue edges, markers, timing, presentation, capture, and both backend contracts are retained.
8. **Deletion ledger** — authored recording policy, timing suppression, dynamic result vectors, formatted task names, repeated waits, coordinator native begin/reset, and misleading worker-index naming are removed.
9. **Structure reconciliation** — all types stay in their established private/public module folders; filenames match primary types; no CMake change is needed.
10. **Implementation-shape reconciliation** — headers remain declaration-only except templates/accessors; no local type, anonymous namespace, role-only bucket, diagnostic framework, or logging expansion was added.
11. **DOD reconciliation** — the inventory above records authority, compact fixed layouts, identities, deterministic transforms, capacity, lifetime, precedents, and falsifiers.
12. **Concurrency reconciliation** — SparkleTasks records exclusive leases; task completion has no semantic order; workers never submit or wait; coordinator reuse and shutdown remain token-settled.
13. **Validation** — static diff/alias/namespace/local-type/header/lifecycle/submission searches were performed without a build; D3D12 GPU validation, Vulkan synchronization validation, randomized delays, image/state comparison, and worker sweeps remain manual.
14. **Performance** — source removes small-work multi-object overhead and per-batch allocations; vendor heuristics and the exact runtime crossover/critical-path/memory/GPU evidence are recorded without claiming an unrun speedup.
15. **Naming audit** — canonical recording group/chunk/batch/lease/partition terms remain; pass-policy, worker-index-as-thread, merge-list, RHI-thread, and secondary-stream compatibility spellings are absent.
16. **Limitations and unavailable evidence** — no build, executable, native validation layer, capture, hardware/driver comparison, image result, timing result, or Linux claim is reported.
17. **Acceptance status** — **PASS** under the explicit source-complete/runtime-manual policy, with no pass-level scheduling parameter and no known source-stage gap.
18. **PGE reconciliation** — `PGE-05/07/09/10/13/15` advance, `PGE-01/02/06/11/14` are preserved, and `PGE-03/04/08/12` are not applicable; AI-assisted work was independently source-traced and no role-only or unsupported performance/platform claim remains.

## Prompt 21 — Close Advanced-Feature Preservation

Target CL Title: `SparkleRenderer: Prove Advanced-Feature Preservation Across Recording Modes`

~~~text
Implement Prompt 21 only after Prompt 20 passes.

Objective:
Close the serial/threaded/parallel preservation matrix for raster, classic TLAS, PTLAS, reservoir lighting, reference path tracing, temporal/providers, shader ABI, capture, and both backends. This is a correctness and lifetime gate for the architecture already implemented in Prompts 14-20, not a new scaling feature.

Non-negotiable repository rules:
- Apply `Docs/Architecture/00-Review/L_SparkleEngineIntegrationStyleGuide.md` in full. Treat its ownership, implementation-shape, DOD, concurrency, rendering, naming, validation, structure, principal graphics engineering, and completion-report gates as acceptance criteria. Before editing, list the applicable `PGE-01` through `PGE-15` requirements from A, interpret them through H, and classify each as advance, preserve, not applicable, or blocked with expected evidence. Inspect the current repository, reconcile the whole touched ownership path, verify any AI-assisted work independently, delete replaced paths in the same change, reject role-only scaffolding, and report any justified exception explicitly with its owner, scope, evidence, and deletion or review gate.
- Apply Rule 13: audit every data source, transform, stream, packet, cache, table, upload, and hot traversal touched by this prompt; record the concrete access inventory, authoritative/derived ownership, layout decision, stable identity, deterministic transform, exact source precedent, and measured falsifier.
- Apply Rule 12: audit touched/new files for module and folder ownership, public/private placement, filename-to-primary-type alignment, and nearby misplaced counterparts; complete bounded moves/renames with includes, CMake, and documentation updated before the gate.
- Enforce J's canonical concurrency/rendering vocabulary and Rule 10: search canonical terms plus rejected aliases in the touched scope, use one responsibility per name, and delete temporary compatibility spellings before the gate.
- Reuse Prompt 20's recording plan, leases, serial islands, submission batches, and existing validation/profiling surfaces; no special per-feature thread systems or test-only production APIs.
- Do not add intra-pass draw/dispatch/build-input chunking, backend bundle/secondary-buffer policy, or another recording abstraction in this prompt. Prompt 23 may record a future candidate only if representative measurements identify one pass as a material CPU-recording critical path.
- Fix only defects exposed by the preservation matrix. Refactor or delete a touched faulty/duplicate path in the same change; do not add unrelated optimization work.
- Do not alter GPU algorithms, queue policy, feature settings, workload resolution, or capability reporting to make parity pass.

Required implementation:
1. Define one capability-gated matrix covering serial recording and every supported threaded/parallel recording mode on D3D12 and Vulkan. Use identical scene revision, camera, settings, seeds, shader generation, provider state, and workload duration for comparable rows.
2. Verify raster draw/batch order, material/pipeline selection, transparent-sensitive order, frame-graph plan identity, barriers, queue waits, submission order, markers, timestamps, and output/reference images.
3. Audit classic TLAS and PTLAS separately for initial build, update, add/remove, asset replacement, trace identity, delayed-GPU lifetime, and currently supported BLAS scratch/result/compaction inputs. Unsupported capability rows must report unavailable rather than silently falling back to another AS path.
4. Verify reservoir light ordering/IDs and weights; reference-path seed, sample count, accumulation, and reset; temporal motion/depth/jitter/exposure/history identity across recording modes.
5. Verify provider supported/enabled/operational/failure state, resource tags, required thread affinity, resize/reload, frame-generation/present sequencing, and classical fallback.
6. Verify shader package/reflection/layout/generation coherence through reload and failure, plus screenshot/readback, debugger capture, marker, and object-name continuity.
7. Stress delayed GPU completion, repeated level/asset/shader reload, resize/recreate, capture, provider failure/recovery, and shutdown. Prove that CPU task completion never substitutes for GPU last-use completion.
8. Record every matrix row as pass, fail, unsupported by honest capability state, or blocked by a named external prerequisite. Fix failures in the owning existing path; do not create feature-specific scheduling or compatibility paths.

Validation:
- Full advanced-feature test matrix in J on D3D12/Vulkan where capability reports support.
- Serial/threaded/parallel recording reference images, deterministic plan/order identity, and native validation.
- Delayed GPU completion plus level/asset/shader reload stress for RT/provider/capture paths.
- Capability/failure/fallback reports remain truthful; no unsupported row is counted as a pass.
- Existing Prompt 20 recording/list/descriptor/transient/GPU-time and submission-latency counters show that the validation fixes introduced no material regression. No speedup is required or claimed.

Acceptance gate:
- L Sections 19-21 (reconciliation, acceptance checklist, and required completion report) and Section 24 (Principal Graphics Engineering review gate) pass for the entire touched changelist, including directly affected neighboring ownership paths; no undocumented exception remains.
- The binding Prompt 13-29 changelist design gate passes for the entire prompt changelist, including touched neighboring counterparts.
- One preserved feature path and one common recording/submission architecture serve all supported modes; no feature-specific scheduler or duplicate serial implementation remains.
- No feature path is silently disabled/demoted to claim multithreading success.
- Both native backends pass applicable validation and capability truth remains honest.
- Feature identity, temporal state, GPU lifetime, provider affinity, shader generations, and capture/debugger continuity remain correct under stress.

Positive patterns: feature identity preservation, capability-gated matrix, same-plan serial oracle, defect fixes in existing owners.
Forbidden: intra-pass scaling, feature-specific scheduling, test-only production API, feature demotion, fake capability success, CPU task completion used as GPU lifetime.
~~~

### Prompt 21 integration record

Prerequisite disposition: Prompt 20 is source-complete under its recorded runtime-manual policy. Prompt 21 did not change the compiled plan, queue policy, GPU algorithms, feature settings, workload resolution, or recording granularity.

PGE classification:

| Requirement | Disposition | Evidence expected/produced |
|---|---|---|
| `PGE-01` rendering portfolio | preserve | raster, RT, reservoir/reference, temporal/provider, shader, and capture paths remain one product renderer; no feature was removed to make recording pass |
| `PGE-02` renderer/architecture ownership | advance | one capability matrix binds render-owner mode, recording mode, worker count, backend, feature capability, and identical input signature |
| `PGE-03` neural rendering/ML systems | preserve | provider and shader structured-resource seams remain; no ML runtime, task, or claim was introduced |
| `PGE-04` research publication | not applicable | no new algorithm or publication claim is made |
| `PGE-05` low-level graphics correctness | preserve | compiled barriers, queue waits, submission order, native validation obligations, markers, and timestamp gating retain their Prompt 20 owners |
| `PGE-06` advanced rendering | advance | classic TLAS/PTLAS, BLAS inputs, reservoir/reference identity, temporal state, and shader/provider preservation are traced separately |
| `PGE-07` C++ engineering | advance | hidden GPU-address lifetimes are attached to command-list resource use; D3D12 publishes an immutable recording lookup, retains through opaque tokens and narrow atomics, and keeps record mutation serialized |
| `PGE-08` applied mathematics | preserve | renderer-owned jitter/sample/reset and numerical settings are compared unchanged; no math method is altered |
| `PGE-09` explicit APIs | advance | D3D12/Vulkan resource lifetime and PTLAS capability selection are explicit; unsupported/blocked states do not masquerade as another AS path |
| `PGE-10` CPU/GPU architecture | advance | CPU recording completion remains distinct from queue-token last use for raster geometry and BLAS/TLAS input, scratch, operation, and result resources |
| `PGE-11` ML fundamentals/AI verification | preserve | no ML claim is made; AI-assisted changes were independently traced through resource creation, recording, aggregation, submission, release, and both backends |
| `PGE-12` training/inference workloads | not applicable | no training or inference workload is touched |
| `PGE-13` productization/communication | advance | J now defines exact modes, capability gates, frozen inputs, row dispositions, stress order, and unavailable evidence without adding a report framework |
| `PGE-14` platform breadth | preserve | D3D12 and Vulkan source paths are paired; no unrun hardware, driver, Linux, or provider result is claimed |
| `PGE-15` principal judgment | advance | the matrix produced bounded lifetime/capability fixes in current owners and rejected feature schedulers, test APIs, algorithm changes, and fake parity |

Rule 13 access inventory:

| Data path | Authority, ownership, and layout | Stable identity and deterministic transform | Exact precedent and measured falsifier |
|---|---|---|---|
| launch/CVar state -> execution mode | application launch owns serial/zero-ahead/one-ahead selection; renderer owns the one canonical parallel-recording control | `M0`-`M5` in J plus fixed worker-count row; the CVar changes recording only, not the compiled graph | existing `RendererExecutionConfig`, SparkleTasks controls, and Prompt 20 same-plan oracle; falsified by two controls selecting the same behavior or a mode changing plan/order |
| registered passes/resources -> compiled plan -> recording/submission | frame graph remains authoritative; the plan is cold ordered AoS and recording results use fixed chunk slots | pass/group/chunk/batch indices and `SubmissionOrderKey`; aggregation ignores task completion order | AMD RPS-style compiled ranges and D3D12/Vulkan ordered submission rules already recorded in J; falsified by mode-dependent plan/barrier/wait/submission identity |
| render proxies/lights -> prepared arrays -> reservoir/reference shaders | `RenderWorld` and deterministic preparation own stable `RenderObjectId` order; shader passes read immutable frame data | render-object ID plus prepared slot, frame/history identity, and reference invalidation hash | existing render-world/preparation and history owners; falsified by worker completion changing light order, reservoir weights, sample count, or reset |
| mesh handles -> raster vertex/index binding | `GPUMesh` owns immutable buffers and now binds both views and command-list resource use as one capability | `GpuMeshHandle` and underlying `RhiResourceHandle`; draw order remains batch order | D3D12/Vulkan pending-command-buffer resource lifetime rules and existing `RenderCommandList::TrackResource`; falsified by asset replacement reclaiming a bound buffer before its submission token |
| mesh geometry -> BLAS build | geometry desc owns vertex/index address plus resource identity; BLAS cache owns scratch/result and build decision | mesh/gpu-scene identity, geometry values, and BLAS resource handle; build inputs are tracked before native recording | DXR/Vulkan AS build input lifetime rules and existing BLAS cache; falsified by unload/rebuild reclaiming geometry, scratch, or result while recording/GPU work is pending |
| BLAS set -> classic TLAS build/update | classic builder owns deterministic instance array, instance upload, scratch/result, and refit decision | stable GPU-scene instance ID, BLAS handle, count, flags, and transform; every referenced BLAS is tracked even when reused | DXR/Vulkan TLAS build/update contracts; falsified by reused BLAS or instance/scratch/result release before the graphics token settles |
| partition plan/BLAS set -> PTLAS operation/build | PTLAS strategy owns instance writes and storage/scratch/operation buffers; backend owns native encoding | stable instance/partition IDs and provider capability; classic fallback is a different row, never PTLAS success | NVIDIA PTLAS operation/storage contracts and existing D3D12 NVAPI/Vulkan NV providers; falsified by provider mismatch, changed partition identity, or early operation/storage release |
| D3D12 live allocation records -> recording retention view | allocator mutation and pending-release ownership remain render-owner-only; `D3D12RecordingResourceTable` publishes a sorted immutable resource-to-record projection and exposes only opaque retained-use tokens to an exclusive command list | native resource identity is the sorted lookup key; pending releases are excluded from the next publication; publication and command-list references keep allocation/parent-heap records alive | existing Vulkan recording read-view pattern, J LC-14, and D3D12 deferred release; falsified by a worker entering `recordsMutex`, observing owner-vector mutation, republishing a pending record, or destruction while a read view/use token exists |
| worker command-list resource use -> deferred release | command list owns a private unique-resource vector paired with backend-private retained-use tokens; allocation/heap recording references are atomic, final last-use marking and destruction remain coordinator-owned | native resource identity plus exact `RhiSubmissionToken`; reference count is lifetime only, never ordering | existing Vulkan/D3D12 recording read views and deferred-release owners; falsified by reference underflow, mismatch between tracked resources and tokens, or release before token completion |
| backend feature query -> renderer TLAS selection | backend query owns supported/access truth; renderer strategy consumes the same descriptor-capable selection contract | backend/provider enum plus static capability reason and active build result | current capability-report/strategy owners; falsified by reporting classic while descriptor-capable PTLAS is selected, or counting classic fallback as PTLAS |
| shader packages -> pass runtimes -> retired generation | `PipelineStateManager` prewarms one coherent runtime generation before recording and retains the old generation by all-queue last use | shader package generation and pass type; failed replacement keeps current generation | existing immutable shader runtime publication; falsified by one recording run observing mixed layout/pipeline generations or early retirement |
| provider stack -> external-provider passes -> retired provider generation | render owner creates/evaluates/destroys providers; external callbacks remain coordinator serial islands; replaced stacks retain all-queue last use | provider selection key/generation and frame tag | provider API affinity contracts and existing retired-provider owner; falsified by worker provider call, wrong frame tag, or callback/destruction before GPU completion |
| viewport product -> capture request/readback/result | frame pipeline owns bounded three-entry requests; backend capture owns copy/readback token; editor/tool receives owned pixels/result | request ID, expected `FrameId`, frame/provider generation, artifact path | D3D12/Vulkan copy/readback completion contracts and existing capture service; falsified by device-idle routine path, stale-frame acceptance, unbounded queue, or late callback |

Defects fixed by the matrix:

1. Raster mesh bindings and ray-tracing geometry/build inputs used raw GPU addresses without consistently attaching every underlying allocation to the submitting command list. `GPUMesh::Bind`, BLAS build, classic TLAS build, and PTLAS build now track the precise vertex, index, BLAS, instance, scratch, operation, and result resources they use.
2. D3D12 recording-time resource retention still asserted render-owner access, entered the allocator record mutex from workers, and used non-atomic recording reference counts even though Prompt 20 records eligible command lists on workers. The render owner now publishes a sorted immutable recording view after transient materialization; workers perform lock-free lookup and retain opaque use tokens through narrow relaxed atomics. Last-use publication, release scans, resource destruction, and allocator mutation remain on the coordinator. Pending releases are excluded from later publications, aliasing resources participate in the same table, and discarding a closed unsubmitted list releases its retained uses immediately.
3. D3D12/Vulkan provider summaries retained obsolete “renderer selection not wired” states. Descriptor-capable requested PTLAS now reports selected; Vulkan native PTLAS without the advanced-feature descriptor path reports that named prerequisite and remains classic rather than claiming PTLAS.
4. The application still registered unused `r.ParallelCommandRecording` beside renderer-owned `r.FrameGraph.ParallelRecording`. The dormant alias is deleted and J names the one actual control.

Capacity, lifetime, and performance ledger: the change adds no scheduler, task, queue, command object, GPU resource, descriptor allocation, report buffer, or feature table. Each command list retains only the unique resources it already uses. D3D12 publishes one compact sorted entry per live non-pending allocation at the existing recording-preparation boundary; old views retire through shared immutable-view ownership. Each allocation and parent heap carries one atomic recording count, with increments once per publication entry and once per unique command-list use; final token marking occurs once at submission. Matrix falsifiers are Prompt 20 recording/list/descriptor/transient/GPU-time and submission-latency evidence plus publication bytes/time, zero worker contention on `recordsMutex`, resource-tracking count/cost, delayed-GPU release latency, and image/order parity. No speedup is required or claimed.

Rule 12/16 and implementation-shape reconciliation: `D3D12RecordingResourceTable.*` is a filename-aligned private Memory capability beside the existing Vulkan counterpart, and `D3D12RecordingResourceUseToken.h` owns its matching opaque value; the module's recursive private-source discovery requires no manual CMake list. `RenderCommandContext` remains the narrow renderer command capability; `GPUMesh` owns raster geometry binding; BLAS/classic-TLAS/PTLAS owners retain their build-specific lifetime declarations; backend RHI device owners retain capability selection; D3D12 device services publish, the allocator owns records/table, resource service adapts retention, and command lists own per-recording tokens. New substantive functions are declared in headers and defined in `.cpp`; no function-local type, anonymous namespace, nested feature scheduler, role-only class, or diagnostic/logging surface was added.

Preservation and deletion ledger: the same frame graph, pass callbacks, recording plan, leases, serial islands, SparkleTasks executor, submission batches, queues, shader algorithms, provider calls, temporal histories, presentation, and capture path serve every mode. Deleted items are the unused `r.ParallelCommandRecording` alias and obsolete PTLAS “selection not wired” capability spellings. No serial feature implementation, compatibility AS path, test API, bundle/secondary-buffer policy, feature worker system, intra-pass chunking, or new validation framework remains.

Required completion report:

1. **Outcome** — Prompt 21 is source-complete: J owns one capability-gated matrix, and the source defects it exposed in GPU lifetime retention, D3D12 worker recording, capability truth, and duplicate controls are fixed in existing owners.
2. **Repository audit** — renderer modes, task controls, frame-graph compiler/recording/submission, raster mesh binding, BLAS/classic-TLAS/PTLAS, reservoir/reference/temporal state, shader generations, providers, capture, deferred release, and both backend capability paths were traced.
3. **Ownership** — frame graph owns GPU order, command lists own recorded resource use, RHI allocation records own token retirement, feature owners own semantic identity, and capability reports own support truth.
4. **Files changed by responsibility** — J/K evidence, application control deletion, renderer command/mesh/RT lifetime declarations, D3D12 recording-view publication/retention, and D3D12/Vulkan TLAS selection are the only changed responsibility groups.
5. **Orchestration/capability refinement** — mode selection reads as launch mode plus canonical recording switch; feature build functions read as resolve inputs, track exact resources, encode native work, and publish existing results.
6. **SOLID/DRY reconciliation** — one resource-tracking mechanism and one recording/submission architecture serve all features/modes; common behavior was not copied into feature schedulers or backend-specific test paths.
7. **Preservation ledger** — raster order, RT providers, reservoir/reference lighting, temporal state, providers, shader ABI/reload, capture/debug, markers, timestamps through their serial capability gate, and both backends retain current product behavior.
8. **Deletion ledger** — unused parallel-recording control and stale PTLAS capability reasons are removed; no compatibility adapter was introduced.
9. **Structure reconciliation** — all modified types remain in their existing owning module/folder; the new private D3D12 recording table mirrors the existing Vulkan ownership split and filenames match primary owners; no move or explicit CMake update is warranted.
10. **Implementation-shape reconciliation** — headers contain declarations/data/accessors/templates only; substantive behavior is in `.cpp`; no local type, anonymous namespace, giant switchboard, diagnostics expansion, or noisy validation layer was added.
11. **DOD reconciliation** — the access inventory records authorities, resource identities, deterministic transforms, layout/capacity effects, lifetimes, precedents, and concrete falsifiers.
12. **Concurrency reconciliation** — SparkleTasks remains the only worker runtime; workers record exclusive leases, resolve immutable recording views without allocator locking, and retain resources atomically; only the coordinator publishes views, aggregates, submits, marks token last use, polls release, mutates records, and destroys.
13. **Validation** — static ownership/call-site/alias/capability/resource-lifetime searches and diff checks were performed without a build. The J matrix records every unrun native row as blocked or unsupported rather than pass.
14. **Performance** — no worker enters the D3D12 allocation-record mutex; immutable-view publication is the one bounded per-run derived-data cost and its bytes/time join the exact manual regression evidence, with no unrun gain claimed.
15. **Naming audit** — canonical `RenderCommandContext`, `RenderCommandList`, recording group/chunk/batch/lease, `RhiSubmissionToken`, classic TLAS/PTLAS, and `r.FrameGraph.ParallelRecording` remain; rejected duplicate/compatibility spellings are absent.
16. **Limitations and unavailable evidence** — no build, executable, native validation, debugger capture, image comparison, timestamp result, hardware/provider run, delayed-GPU stress, Bistro/San Miguel content, or Linux result is claimed. BLAS compaction, PTLAS source/update, and Vulkan advanced-feature descriptor PTLAS are explicitly unsupported/blocked rather than passed through rebuild/classic fallback. These named rows remain manual/external prerequisites in J.
17. **Acceptance status** — **PASS** under the user-authorized source-complete/runtime-manual policy. No known source-stage Prompt 21 gap remains; hardware rows keep honest blocked/unsupported dispositions until manually executed.
18. **PGE reconciliation** — `PGE-02/06/07/09/10/13/15` advance, `PGE-01/03/05/08/11/14` are preserved, and `PGE-04/12` are not applicable; no feature demotion, fake capability success, role-only scaffolding, or unsupported performance/platform claim remains.

## Prompt 22 — Reliability, Tools, Packages, and Deletion Closure

Target CL Title: `SparkleEngine: Close Multithreading Reliability, Tooling, and Legacy Deletions`

~~~text
Implement Prompt 22 only after Prompts 14, 16, and 21 pass.

Objective:
Close cross-subsystem reliability, deterministic cook/package workflows, editor lifecycle stress, public-surface review, and every compatibility/deletion ledger so one coherent architecture remains.

Non-negotiable repository rules:
- Apply `Docs/Architecture/00-Review/L_SparkleEngineIntegrationStyleGuide.md` in full. Treat its ownership, implementation-shape, DOD, concurrency, rendering, naming, validation, structure, principal graphics engineering, and completion-report gates as acceptance criteria. Before editing, list the applicable `PGE-01` through `PGE-15` requirements from A, interpret them through H, and classify each as advance, preserve, not applicable, or blocked with expected evidence. Inspect the current repository, reconcile the whole touched ownership path, verify any AI-assisted work independently, delete replaced paths in the same change, reject role-only scaffolding, and report any justified exception explicitly with its owner, scope, evidence, and deletion or review gate.
- Apply Rule 13: audit every data source, transform, stream, packet, cache, table, upload, and hot traversal touched by this prompt; record the concrete access inventory, authoritative/derived ownership, layout decision, stable identity, deterministic transform, exact source precedent, and measured falsifier.
- Apply Rule 12: audit touched/new files for module and folder ownership, public/private placement, filename-to-primary-type alignment, and nearby misplaced counterparts; complete bounded moves/renames with includes, CMake, and documentation updated before the gate.
- Enforce J's canonical concurrency/rendering vocabulary and Rule 10: search canonical terms plus rejected aliases in the touched scope, use one responsibility per name, and delete temporary compatibility spellings before the gate.
- Audit every remaining std::thread/jthread/async/future/mutex/shared_mutex/condition variable/atomic/memory-order operation, standard/Qt/native wait or detach, TaskExecutor instance, WaitForIdle call site, GameWorld snapshot/pointer, controller, Scene* facade, host-render path, direct lifecycle callback, default report/artifact, launcher command/package kind. Search Engine, Tools, and Projects owned sources; exclude generated/external code but include wrapper policy and third-party worker counts.
- Search before adding any reliability helper; consolidate into current owner and delete obsolete mechanisms.
- Daily refactor is mandatory for every retained legacy hotspot touched.
- No new feature, framework, report system, task UI, or package kind is allowed in closure.

Required implementation:
1. Run and fix repeated stress: viewport recreate/dock/minimize, play/edit transition if owned, level load/cancel/reload, shader/asset reload, capture, provider resize/failure, delayed GPU, document/application shutdown.
2. Complete deterministic transactional asset/shader/texture cook fan-out from owned project catalogs/manifests.
3. Preserve curated in-repo levels and optional heavyweight content-pack policy; no default recursive uncataloged heavy-content scan.
4. Validate only owned runtime/editor/symbols/development-tools/optional-content packages; delete package/launcher/inspection surfaces without consumers.
5. Consolidate evidence into existing profiler/allocator/frame-graph/debugger hooks and delete superseded diagnostics/default timing artifacts.
6. Delete all old paths listed in prior prompts: futures/pools, old Entity/controller/object storage, duplicate Scene* state, raw snapshot/render host path, direct UI/renderer mutations, idle reload, full GPU rebuild, single-context-only recording path.
7. Audit public headers: no worker queues, ECS storage, caches, task timings, recording pools, or broad observation APIs.
8. Run include/dependency boundary checks and reduce unnecessary module/public dependencies.
9. Reconcile every J LC ledger row and every newly discovered primitive. Retained items require file-local owner/invariant/lock-order/blocking documentation and a falsifying stress test; replaced items have no call sites. Prompt 24 may perform only targeted publication/wait/reuse proof or a concrete fix identified by this audit or Prompt 23; it is not permission to reopen closed utility protocols.
10. Prove no arbitrary callback, Qt/UI access, provider/driver call, blocking I/O, GPU wait, logging/formatting, or destruction occurs under an engine lock. Enumerated API-mandated exceptions require a narrow critical section, re-entry analysis, and test.
11. Recheck the concrete legacy hotspots: InputSystem callback registry; Logger/Timer; ShaderRecook; LauncherBackend/ProcessRunner/AssetCookerToolProcess; Streamline; D3D12/Vulkan queue, descriptor, upload, allocation-record, validation-message, fence/timeline, swap-chain, and idle paths.
12. Run the repository-wide naming reconciliation from J. Classify semantic uses of `Job`, `WorkItem`, `TaskRun`, bare `TaskNode`, `RenderFrameMailbox`, service-type `RenderThread`, `RenderThreadCommands`, `RecordingContextLease`, `WorkerRecordingContext`, misleading `RHIThread`, and unqualified cross-module `Queue`/`Context`. Delete rejected planned aliases; retain unrelated third-party/native terms only outside Sparkle ownership or behind an accurately named wrapper.

Validation:
- Long randomized stress repeatedly clean of deadlock/leak/stale callback/native validation/race symptom.
- All task/system/renderer modes 0/1/2/N, serial/threaded/parallel, D3D12/Vulkan applicable paths.
- Tool outputs deterministic and failed/cancelled generation never published.
- Package content and optional packs reproducible from documented commands.
- Repository-wide searches prove no unauthorized legacy counterpart remains.
- Repository-wide concurrency inventory has zero unclassified hits and zero obsolete LC consumers; retained primitive tests include re-entry, contention, cancellation/shutdown, and wrong-thread use as applicable.
- Repository-wide canonical/rejected-alias searches have zero unclassified owned-source hits; filenames, symbols, tests, CMake, comments, profiler labels, and thread labels agree.

Acceptance gate:
- L Sections 19-21 (reconciliation, acceptance checklist, and required completion report) and Section 24 (Principal Graphics Engineering review gate) pass for the entire touched changelist, including directly affected neighboring ownership paths; no undocumented exception remains.
- The binding Prompt 13-29 changelist design gate passes for the entire prompt changelist, including touched neighboring counterparts.
- One SparkleTasks runtime, one ECS world source, one packet/render path, one frame-graph authority, one owner per mutable subsystem.
- Every deletion ledger closed; no indefinite compatibility route.
- Every synchronization/blocking primitive is either a tested private implementation of the coherent architecture or deleted. Zero-unclassified—not zero-mutex—is the gate.
- Default workflow produces product artifacts, not research reports/log streams.
- Public surface and package/tool set are smaller/coherent and have real consumers.

Positive patterns: audit-to-zero, transactional products, owned workflows, public API reduction, repeated lifecycle stress.
Forbidden: keep old path “for safety,” new diagnostic subsystem, unowned package variant, test-only production API.
~~~

### Prompt 22 integration record

1. **Outcome** — Prompt 22 is source- and build-complete: the retained cook path is catalog-driven, task-backed, transactional, and deterministic; the replaced wrapper/status/version paths are deleted rather than adapted.
2. **Repository audit** — owned task runtimes, waits, launch/process operations, project catalogs, scene/material/texture/shader cooks, generation publication, diagnostics, packages, and current deletion-ledger aliases were searched across `Engine`, `Tools`, and `Projects`.
3. **Ownership** — project catalogs own the requested source set, cook plans own deterministic ordering, SparkleTasks owns fan-out, typed cook builds own results, and file publication owns generation commit/rollback.
4. **Files changed by responsibility** — catalog/scene registries, cook discovery and planning, typed scene outputs, material serialization, bounded cook execution, narrow diagnostics, and launcher synchronization remain in their existing owning modules.
5. **Orchestration/capability refinement** — discovery now reads as catalog resolution, scene collection, plan construction, task execution, and publication; detailed parsing, cooking, serialization, and UI option population are named subordinate operations.
6. **SOLID/DRY reconciliation** — `CookedSceneBuild` is the single scene result, registry entries have one parser, cook failures have one error channel, and launcher startup options have one population path.
7. **Preservation ledger** — curated in-repository levels, optional content, scene/material/texture/animation products, launcher workflows, and current product diagnostics retain their consumers and behavior.
8. **Deletion ledger** — the redundant imported-scene product/status/version wrappers, duplicated success state, nullable post-plan scene entries, and superseded discovery/control branches are removed; no compatibility route remains.
9. **Structure reconciliation** — public declarations contain only product contracts; discovery, batch execution, parsing, serialization, and launcher synchronization remain private; filenames continue to match their primary owner and no bounded move was warranted.
10. **Implementation-shape reconciliation** — substantive behavior is in `.cpp`, function-local types and anonymous namespaces are absent in the touched path, orchestration is decomposed by named stage, and no new report or validator product was added.
11. **DOD reconciliation** — catalog entries are authoritative identities, sorted plans are deterministic derived data, task slots are exclusive, staged files are generation-local, and publication is the only externally visible commit.
12. **Concurrency reconciliation** — one SparkleTasks executor performs bounded cook fan-out; workers write exclusive result slots and perform no UI access; the coordinator validates results and publishes in stable plan order.
13. **Validation** — DevelopmentEditor `AssetCooker`, `SparkleLauncher`, and `architecture_boundary_check` build successfully; a DevelopmentGame full cook completed twice with identical hashes for all 526 published product files and no staging residue.
14. **Performance** — the closure removes duplicated scene-result copying and repeated discovery work, retains bounded worker counts, and adds no default timing/log stream; full performance characterization is intentionally owned by Prompt 23.
15. **Naming audit** — `TaskRun`, cook plan, cook build, generation publication, scene asset registry, and project level catalog remain canonical; rejected future/pool/job/report and legacy product spellings have no owned consumers.
16. **Limitations and unavailable evidence** — Windows Smart App Control blocked a freshly built unsigned DevelopmentEditor cooker, so deterministic full-cook evidence used the DevelopmentGame tool binary; D3D12/Vulkan lifecycle stress and Linux execution remain manual/unavailable and are not claimed.
17. **Acceptance status** — **PASS** for source, build, deterministic tool output, ownership, and deletion closure. Runtime/backend stress rows retain their documented manual disposition under the user-authorized runtime-validation policy.
18. **PGE reconciliation** — `PGE-01/02/03/05/06/07/09/10/13/14/15` advance or are preserved by a smaller deterministic product path; `PGE-08/11` are preserved; `PGE-04/12` are not applicable to this closure; no role-only scaffold or unsupported parity claim was introduced.

## Prompt 23 — Initial Full-System Performance Characterization and Tuning

Target CL Title: `SparkleEngine: Characterize and Tune Full-System Multithreading Performance`

~~~text
Implement Prompt 23 only after Prompt 22 passes.

Objective:
Tune the completed base architecture with representative evidence and establish the full-system reference captures required by the expert-hardening prompts. Do not declare the multithreading program or portfolio complete in this prompt.

Non-negotiable repository rules:
- Apply `Docs/Architecture/00-Review/L_SparkleEngineIntegrationStyleGuide.md` in full. Treat its ownership, implementation-shape, DOD, concurrency, rendering, naming, validation, structure, principal graphics engineering, and completion-report gates as acceptance criteria. Before editing, list the applicable `PGE-01` through `PGE-15` requirements from A, interpret them through H, and classify each as advance, preserve, not applicable, or blocked with expected evidence. Inspect the current repository, reconcile the whole touched ownership path, verify any AI-assisted work independently, delete replaced paths in the same change, reject role-only scaffolding, and report any justified exception explicitly with its owner, scope, evidence, and deletion or review gate.
- Apply Rule 13: audit every data source, transform, stream, packet, cache, table, upload, and hot traversal touched by this prompt; record the concrete access inventory, authoritative/derived ownership, layout decision, stable identity, deterministic transform, exact source precedent, and measured falsifier.
- Apply Rule 12: audit touched/new files for module and folder ownership, public/private placement, filename-to-primary-type alignment, and nearby misplaced counterparts; complete bounded moves/renames with includes, CMake, and documentation updated before the gate.
- Enforce J's canonical concurrency/rendering vocabulary and Rule 10: search canonical terms plus rejected aliases in the touched scope, use one responsibility per name, and delete temporary compatibility spellings before the gate.
- Use existing profiler/debugger/allocator/timestamp/benchmark launch hooks and manually maintained concise results.
- Search and consolidate existing docs rather than adding another architecture/policy document. Update J, K, and the owned concise product overview only where necessary.
- Performance tuning must not restore duplication, bypass ownership, or demote advanced features.
- Refactor touched tuning knobs/configuration into one coherent owner and delete experiments that lose.
- Pre-mortem MT-17–28, MT-35, MT-38, and MT-41–44. NVIDIA and AMD explicitly warn that more workers can reduce game performance through oversubscription, cache pressure, synchronization, context switches, and power behavior; no automatic worker count passes without a representative Sparkle workload sweep.

Required implementation:
1. Tune worker count, FrameCritical/Background budgets, task grains, ECS query ranges, `RenderFrameQueue` depth policy, recording group/chunk size, upload/descriptor page sizes, and residency budgets using tiny and representative large scenes.
2. Run matrix: D3D12/Vulkan, serial/threaded zero/threaded one-ahead, serial/parallel recording, 0/1/2/N workers, cold/warm caches, validation/profile separately.
3. Collect CPU/GPU p50/p95/p99, critical paths, backpressure, task overhead/utilization, ECS query/storage/churn, upload/resource churn, lists/barriers/descriptors/transient memory, RT timings, tool throughput/peak memory, input-to-present.
4. Attribute gains separately to removed work, CPU task parallelism, game/render pipeline, GPU queue behavior, and shader/GPU changes.
5. Retain serial/small-work policies where parallel overhead loses; document negative results.
6. Record hardware metadata already available through existing platform/profiler surfaces: processor count, GPU/driver, memory, OS, power profile, build and relevant third-party worker settings. Do not add a topology subsystem to enrich the report.
7. Identify only measured correctness risks or material bottlenecks for Prompts 24-28. A later prompt may close with verification and no production code when its hypothesized problem is absent; do not manufacture one atomic experiment, scheduler pathology, parallel algorithm, streaming feature, or queue experiment per prompt.
8. Perform preliminary teach-back on memory publication, task scheduling, ECS/DOD, render pipeline, GPU lifetime and native recording; record weak areas for the later expert prompts.

Validation:
- Reproducible commands/settings and machine/core/backend/configuration recorded.
- Independent rerun produces comparable results within stated variability.
- Native validation, correctness images, deterministic outputs, stress suite remain clean after tuning.
- Every public API/task primitive/tool/package has a current consumer.
- No research-only graph/data/panel/log/report enabled in shipping defaults.

Acceptance gate:
- L Sections 19-21 (reconciliation, acceptance checklist, and required completion report) and Section 24 (Principal Graphics Engineering review gate) pass for the entire touched changelist, including directly affected neighboring ownership paths; no undocumented exception remains.
- The binding Prompt 13-29 changelist design gate passes for the entire prompt changelist, including touched neighboring counterparts.
- Reference evidence shows correctness, tradeoffs, limitations, and causality—not only FPS or thread count.
- Representative large workloads improve where expected; tiny cases remain within budget.
- CPU gains do not cause unjustified GPU/memory/latency/backend regressions.
- Every retained Prompt 24-28 change names an existing Sparkle path and falsifiable success/failure criteria; prompts without a current problem close without production-code expansion.

Positive patterns: causal measurement, reproducibility, honest limits, negative-result retention, configuration metadata.
Forbidden: premature portfolio-complete claim, cherry-picked FPS, one-backend claim, generated report product, tuning by thread count alone, unsupported “production parity” language.
~~~

## Prompt 24 — Close Atomic Publication and Wait-Protocol Correctness

Target CL Title: `SparkleTasks: Close Production Publication and Wait Protocols`

~~~text
Implement Prompt 24 only after Prompt 23 passes.

Objective:
Prove that Sparkle's existing task, frame-packet, generation-publication, and wait/reuse protocols are correct through cancellation, shutdown, delayed consumers, and object lifetime. Fix concrete defects in their current owners; do not create a lock-free research project or a second implementation of a working protocol.

Non-negotiable repository rules:
- Apply `Docs/Architecture/00-Review/L_SparkleEngineIntegrationStyleGuide.md` in full. Treat its ownership, implementation-shape, DOD, concurrency, rendering, naming, validation, structure, principal graphics engineering, and completion-report gates as acceptance criteria. Before editing, list the applicable `PGE-01` through `PGE-15` requirements from A, interpret them through H, and classify each as advance, preserve, not applicable, or blocked with expected evidence. Inspect the current repository, reconcile the whole touched ownership path, verify any AI-assisted work independently, delete replaced paths in the same change, reject role-only scaffolding, and report any justified exception explicitly with its owner, scope, evidence, and deletion or review gate.
- Apply Rule 13: audit every data source, transform, stream, packet, cache, table, upload, and hot traversal touched by this prompt; record the concrete access inventory, authoritative/derived ownership, layout decision, stable identity, deterministic transform, exact source precedent, and measured falsifier.
- Apply Rule 12: audit touched/new files for module and folder ownership, public/private placement, filename-to-primary-type alignment, and nearby misplaced counterparts; complete bounded moves/renames with includes, CMake, and documentation updated before the gate.
- Enforce J's canonical concurrency/rendering vocabulary and Rule 10: search canonical terms plus rejected aliases in the touched scope, use one responsibility per name, and delete temporary compatibility spellings before the gate.
- Start from Prompt 22's synchronization inventory and Prompt 23's measured risks. Re-audit only production protocols that publish payload, govern reuse/lifetime, park/wake workers or coordinators, or were touched by a required fix; identity-only counters and unrelated cold flags need a concise classification, not a new test framework.
- Prefer the simplest correct owner-local mechanism. A mutex and predicate condition variable are acceptable; atomics require a clear invariant and publication edge. Do not weaken or replace a correct locked protocol merely to demonstrate memory-order knowledge.
- Apply daily refactoring only where the audit exposes duplicate flags, wake paths, speculative atomics, or ambiguous ownership.
- Keep protocols private to their owner. No public queue internals, generic concurrent containers, hazard-pointer framework, formal model-checker integration, duplicate SC production path, or test-only shipping API.
- Preserve serial/threaded modes, all lifecycle behavior and both rendering backends.

Required implementation:
1. Audit the real publication/lifetime paths: SparkleTasks ready/sleep/completion state, `RenderFrameQueue` publish/consume/retire/reuse, render-control completion, shader/asset generation publication, and GPU retirement tokens. For each retained atomic or wait, state its owner, protected invariant, payload/lifetime edge, and why its memory order or lock is sufficient.
2. Verify `RenderFrameQueue` state transitions and payload publication: acquire, publish, consume, retire/cancel, reuse, close, and settle. Sequence identity must reject stale tickets, and CPU task completion must remain distinct from GPU last-use completion.
3. Verify every condition-variable wait uses owner state in a predicate and that notify-before-park, park-before-notify, close, cancellation, and shutdown cannot lose progress. Workers must not block on nested task completion, GPU completion, or ordinary file/process I/O.
4. Exercise targeted scheduler races already relevant to the runtime: cancellation versus start/settle, shutdown with queued/running work, empty/full frame queue with delayed producer/consumer, and repeated close. Use 0/1/2/N workers and randomized short delays.
5. Test stale generation/handle rejection and wrap policy only for actual reusable bounded identities. If a lock-free path reclaims raw pointers, replace it with owned/indexed storage or stop for an explicit reclamation design.
6. Remove speculative cross-thread atomics with no real concurrent caller. Add narrow private assertions only where they expose illegal state transitions, double consume/reuse, or stale identity.
7. Fix any failure in the existing owner and delete the replaced wake/flag path. If the targeted stress passes and the audit finds no defect, close with evidence and no production-code change.

Validation:
- Targeted state/result/close behavior remains deterministic across randomized delays and repeated 0/1/2/N runs.
- Cancellation, shutdown, delayed producer/consumer, stale ticket/generation, and notify/park order are stressed under sanitizer-supported and optimized configurations where available.
- No lost/duplicate packet or task, deadlock, leaked scope, stale access, busy idle loop or validation regression.
- Repository search shows no duplicate state flag/wake protocol introduced by the fixes and no new unowned concurrent primitive.

Acceptance gate:
- L Sections 19-21 (reconciliation, acceptance checklist, and required completion report) and Section 24 (Principal Graphics Engineering review gate) pass for the entire touched changelist, including directly affected neighboring ownership paths; no undocumented exception remains.
- The binding Prompt 13-29 changelist design gate passes for the entire prompt changelist, including touched neighboring counterparts.
- Every production publication, wait, and reuse protocol in scope has a stated invariant and sufficient memory-order/lock/lifetime proof.
- No atomic remains solely because old code once anticipated concurrency, and no atomic flag is treated as publication of unrelated fields.
- Targeted cancellation, shutdown, delayed-consumer, stale-identity, and lost-wake tests settle predictably.
- No duplicate reference protocol, generic concurrency framework, or compatibility synchronization path remains.

Positive patterns: explicit owner state, predicate waits, bounded identities, targeted race tests, simplest-correct mechanism, ownership repair.
Forbidden: “works on x64,” relaxed-by-default, lock-free rewrite for prestige, raw-pointer reclamation by hope, busy waiting, public queue snapshots, generic lock-free framework.
~~~

## Prompt 25 — Establish a Conservative Worker and Oversubscription Policy

Target CL Title: `SparkleTasks: Establish a Conservative Worker Budget`

~~~text
Implement Prompt 25 only after Prompt 23 passes. Prompt 24 is an independent correctness closure, not a worker-policy prerequisite.

Objective:
Choose one simple, bounded SparkleTasks worker budget that behaves well on current representative rendering/tool workloads, retains explicit serial/1/2/N overrides, and avoids obvious nested-pool oversubscription. Do not build a CPU-topology, affinity, or scheduler-tuning subsystem.

Non-negotiable repository rules:
- Apply `Docs/Architecture/00-Review/L_SparkleEngineIntegrationStyleGuide.md` in full. Treat its ownership, implementation-shape, DOD, concurrency, rendering, naming, validation, structure, principal graphics engineering, and completion-report gates as acceptance criteria. Before editing, list the applicable `PGE-01` through `PGE-15` requirements from A, interpret them through H, and classify each as advance, preserve, not applicable, or blocked with expected evidence. Inspect the current repository, reconcile the whole touched ownership path, verify any AI-assisted work independently, delete replaced paths in the same change, reject role-only scaffolding, and report any justified exception explicitly with its owner, scope, evidence, and deletion or review gate.
- Apply Rule 13: audit every data source, transform, stream, packet, cache, table, upload, and hot traversal touched by this prompt; record the concrete access inventory, authoritative/derived ownership, layout decision, stable identity, deterministic transform, exact source precedent, and measured falsifier.
- Apply Rule 12: audit touched/new files for module and folder ownership, public/private placement, filename-to-primary-type alignment, and nearby misplaced counterparts; complete bounded moves/renames with includes, CMake, and documentation updated before the gate.
- Enforce J's canonical concurrency/rendering vocabulary and Rule 10: search canonical terms plus rejected aliases in the touched scope, use one responsibility per name, and delete temporary compatibility spellings before the gate.
- Inspect existing worker CVars, lane counts, platform concurrency query and third-party-library controls before adding policy.
- Treat vendor topology guidance as a warning against maximizing thread count, not a requirement to detect SMT, cache/chiplet domains, processor groups, or heterogeneous cores.
- Refactor only as needed so the total CPU-worker budget, lane allocation and explicit override have one owner and documented precedence.
- Let the OS schedule. Affinity, CPU Sets, QoS, priority and per-workload worker pools are outside this prompt.
- Do not add a public/private topology service, affinity manager, false-sharing framework, benchmark-report generator or per-subsystem pool.

Required implementation:
1. Audit the current application configuration, including the fact that lane counts contribute to one total runnable-worker population. Define one private total CPU-worker budget with deterministic serial and explicit 1/2/N overrides; automatic mode uses a conservative cap derived from the already available processor count.
2. Allocate that budget across existing FrameCritical/Background lanes without silently multiplying the requested worker count per lane. Keep BlockingIo bounded separately because it may block, and do not create new lanes or pools.
3. Sweep serial, 1, 2, automatic and one larger explicit N on the fast regression scene plus one representative Tier 1 rendering or tool workload when available. Record frame/operation p50/p95/p99, task critical path/ready time, background completion and idle wake behavior through existing surfaces.
4. Inventory material third-party workers active during the selected compiler/decode/provider workload. Bound outer Sparkle concurrency only when simultaneous-work evidence shows oversubscription; do not wrap every library in a new control layer.
5. Select one default that is conservative across the measured workloads and retain the explicit override for investigation. Record negative scaling without adding per-machine or per-workload policy tables.
6. Investigate contention, false sharing, migrations or cache topology only when Prompt 23 or this sweep exposes a material unexplained regression. A captured issue may justify a focused fix; absence of such evidence closes those topics as not applicable.

Validation:
- Reproducible optimized-build sweep on the primary supported machine; a logically constrained or second machine is supporting evidence only, not a prerequisite for a local engine default.
- Idle engine does not produce excessive wakes; frame-critical work, background completion and shutdown remain live.
- Selected automatic policy improves or preserves representative p95/p99 versus the current default and avoids obvious total-worker multiplication.
- Explicit serial/1/2/N overrides remain deterministic and do not require topology or affinity APIs.

Acceptance gate:
- L Sections 19-21 (reconciliation, acceptance checklist, and required completion report) and Section 24 (Principal Graphics Engineering review gate) pass for the entire touched changelist, including directly affected neighboring ownership paths; no undocumented exception remains.
- The binding Prompt 13-29 changelist design gate passes for the entire prompt changelist, including touched neighboring counterparts.
- One private owner resolves the total worker budget, lane allocation and explicit override.
- The automatic policy is conservative and justified by representative Sparkle measurements rather than logical-core maximization.
- No topology service, affinity/priority policy, per-workload policy table, speculative padding, or per-subsystem pool is introduced.

Positive patterns: one total budget, small representative sweep, explicit override, bounded blocking lane, negative-result retention.
Forbidden: logical-core worship, topology framework, pin-everything policy, priority as correctness, synthetic-only tuning, per-machine constants, hidden worker multiplication.
~~~

## Prompt 26 — Close Deterministic Fan-In and Evidence-Gate Parallel Algorithms

Target CL Title: `SparkleEngine: Prove Deterministic Parallel Fan-In`

~~~text
Implement Prompt 26 only after Prompts 22 and 23 pass. Prompt 25's worker-budget choice is not required to verify deterministic fan-in.

Objective:
Prove that existing task-local results merge deterministically into ECS, renderer/GPU-scene and cooker outputs. Add or parallelize a reduction, scan/compaction or partition algorithm only when Prompt 23 identifies a material current bottleneck and the retained implementation wins representative measurements.

Non-negotiable repository rules:
- Apply `Docs/Architecture/00-Review/L_SparkleEngineIntegrationStyleGuide.md` in full. Treat its ownership, implementation-shape, DOD, concurrency, rendering, naming, validation, structure, principal graphics engineering, and completion-report gates as acceptance criteria. Before editing, list the applicable `PGE-01` through `PGE-15` requirements from A, interpret them through H, and classify each as advance, preserve, not applicable, or blocked with expected evidence. Inspect the current repository, reconcile the whole touched ownership path, verify any AI-assisted work independently, delete replaced paths in the same change, reject role-only scaffolding, and report any justified exception explicitly with its owner, scope, evidence, and deletion or review gate.
- Apply Rule 13: audit every data source, transform, stream, packet, cache, table, upload, and hot traversal touched by this prompt; record the concrete access inventory, authoritative/derived ownership, layout decision, stable identity, deterministic transform, exact source precedent, and measured falsifier.
- Apply Rule 12: audit touched/new files for module and folder ownership, public/private placement, filename-to-primary-type alignment, and nearby misplaced counterparts; complete bounded moves/renames with includes, CMake, and documentation updated before the gate.
- Enforce J's canonical concurrency/rendering vocabulary and Rule 10: search canonical terms plus rejected aliases in the touched scope, use one responsibility per name, and delete temporary compatibility spellings before the gate.
- Search ECS commands, GPU-scene dirty updates, renderer preparation and cooker publication for task-local outputs, shared append state, sorts, offsets and merge helpers.
- Verify or simplify existing algorithms first. Do not add a generic parallel-algorithm framework, algorithm quota, duplicate serial/parallel product paths, or buffers/helpers without a selected measured consumer.
- Apply daily refactoring only to remove nondeterministic/shared append state, repeated ordering logic or avoidable allocations exposed by the selected real path.
- Preserve byte/state determinism where required and declare explicitly where floating-point equivalence, rather than bitwise identity, is acceptable.

Required implementation:
1. Inventory existing parallel producers and their fan-in points: ECS structural commands, renderer preparation, GPU-scene dirty publication and deterministic cooker/package outputs. Record the authoritative ordering key, tie-break, capacity/failure behavior and cancellation boundary for each applicable path.
2. Re-run existing serial/parallel equivalence and deterministic-output tests with randomized task delays and 0/1/2/N workers. Completion order must not change entity commands, render IDs/draw order, dirty ranges, RT identities or cooked bytes.
3. Fix any shared append, completion-order merge, unstable tie-break or duplicate ordering helper in its existing owner. Prefer task-local results plus one straightforward ordered merge.
4. If Prompt 23 selected a measured reduction/scan/compaction/partition bottleneck, prototype the smallest owner-local alternative with a serial oracle, explicit empty/capacity/cancellation semantics and a measured small-work threshold. Retain it only if representative critical-path improvement justifies its allocation/code cost; otherwise delete it.
5. If no such bottleneck exists, record the algorithm optimization as not applicable and make no production-code addition. Knowing where scan/reduction would fit is sufficient; Sparkle does not need one of each pattern.

Validation:
- Existing deterministic products compare across 0/1/2/N workers and repeated completion-order perturbation.
- Cooker products remain byte-deterministic; ECS command playback, render IDs/draw order, GPU-scene dirty publication and RT identity remain stable where applicable.
- D3D12/Vulkan images and the advanced-feature matrix remain correct for any affected renderer/GPU-scene path.
- Any newly retained parallel algorithm has a serial oracle and representative crossover/critical-path evidence; otherwise no new algorithm is required.

Acceptance gate:
- L Sections 19-21 (reconciliation, acceptance checklist, and required completion report) and Section 24 (Principal Graphics Engineering review gate) pass for the entire touched changelist, including directly affected neighboring ownership paths; no undocumented exception remains.
- The binding Prompt 13-29 changelist design gate passes for the entire prompt changelist, including touched neighboring counterparts.
- Applicable production fan-in points have explicit output ownership, stable ordering and deterministic cancellation/failure behavior.
- No task completion order controls product identity, and no hot shared append/accumulator remains in a measured path by convenience.
- There is no parallel-pattern quota. Losing prototypes are deleted, and an absent measured candidate closes with no production-code expansion.

Positive patterns: task-local results, one ordered fan-in, stable keys, randomized completion, serial oracle for measured changes, honest non-applicability.
Forbidden: algorithm collection for prestige, atomic append everywhere, nondeterministic package order, assuming floating-point associativity, benchmark-only consumer, parallelism below crossover.
~~~

## Prompt 27 — Complete Staged I/O and Cold-Cache PSO/Resource Hitch Control

Target CL Title: `SparkleEngine: Add Staged I/O and Cold-Cache Hitch Control`

~~~text
Implement Prompt 27 only after Prompts 16, 22, 23, and 25 pass. Prompt 26 is an independent deterministic-fan-in closure, not an I/O/PSO prerequisite.

Objective:
Turn current loading, cooking, shader reload and render resource creation into one bounded staged pipeline that distinguishes I/O completion, decode/build, shader compilation, native pipeline/resource creation, upload, owner commit and readiness; control first-run PSO/resource hitches without introducing a second cache or blocking recording.

Non-negotiable repository rules:
- Apply `Docs/Architecture/00-Review/L_SparkleEngineIntegrationStyleGuide.md` in full. Treat its ownership, implementation-shape, DOD, concurrency, rendering, naming, validation, structure, principal graphics engineering, and completion-report gates as acceptance criteria. Before editing, list the applicable `PGE-01` through `PGE-15` requirements from A, interpret them through H, and classify each as advance, preserve, not applicable, or blocked with expected evidence. Inspect the current repository, reconcile the whole touched ownership path, verify any AI-assisted work independently, delete replaced paths in the same change, reject role-only scaffolding, and report any justified exception explicitly with its owner, scope, evidence, and deletion or review gate.
- Apply Rule 13: audit every data source, transform, stream, packet, cache, table, upload, and hot traversal touched by this prompt; record the concrete access inventory, authoritative/derived ownership, layout decision, stable identity, deterministic transform, exact source precedent, and measured falsifier.
- Apply Rule 12: audit touched/new files for module and folder ownership, public/private placement, filename-to-primary-type alignment, and nearby misplaced counterparts; complete bounded moves/renames with includes, CMake, and documentation updated before the gate.
- Enforce J's canonical concurrency/rendering vocabulary and Rule 10: search canonical terms plus rejected aliases in the touched scope, use one responsibility per name, and delete temporary compatibility spellings before the gate.
- Inspect scene/asset/texture/shader reads, ProcessRunner, recook/reload, residency, PipelineStateManager, native device creation and all synchronous waits before adding a stage or cache.
- Inventory D3D12/Vulkan graphics/compute/RT pipeline creation, buffer/image/view creation, memory allocation/binding, descriptor writes/updates, acceleration-structure allocation/build inputs and all driver-facing locks. A vendor recommendation to parallelize does not override native external-synchronization or Sparkle owner contracts.
- Reuse the existing task runtime, asset/shader catalogs, renderer generation owner and upload/residency path. No second streaming service, PSO cache or I/O pool.
- Apply daily refactoring to converge duplicate read/decode/key/cache/reload code and delete accepted-path WaitForIdle/idle polling.
- DirectStorage/overlapped I/O is an optional backend only after the portable staged contract and measurements pass; it may not fork asset identity or publication.

Required implementation:
1. Express request → bounded I/O → decode/decompress → validate/build → shader compile if required → key discovery/deduplication → native create/allocation/bind/write/upload → owner commit → GPU-complete readiness as explicit states with capacity, cancellation, error and progress policy. Skip inapplicable states without collapsing their ownership.
2. Keep blocking file/process operations on the bounded I/O lane; CPU transforms return to normal/background task lanes. Do not occupy frame workers while waiting for I/O or child process pipes.
3. Carry request/document/world/asset/shader generations through every stage; late completion cannot replace newer state and failure leaves the prior generation usable.
4. Audit the current shader compiler backend contract. Use bounded worker processes when compiler/library global locks or crash isolation make in-process threads ineffective; otherwise use bounded task nodes. Preserve deterministic DXIL/SPIR-V/reflection/layout/package output and capture compiler-process count in the shared background-work budget.
5. Inventory runtime-required graphics/compute/RT pipeline and resource keys from owned pass/proxy data. Sort/deduplicate before native creation, use one in-flight request per key in the existing render-owned generation cache, and materialize before parallel recording.
6. Separate pipeline key discovery, shader readiness, native PSO creation and cache publication. Add cold/warm hit, miss, duplicate, too-late, failure and not-ready policies. Test loading-barrier, delayed draw/proxy and bounded fallback choices on their real products.
7. For RT pipelines, evaluate backend-native pipeline library/collection mechanisms only where capability and cold-cache evidence justify them. Keep common keys/results backend-neutral and delete a losing experiment.
8. Separate buffer/image descriptor creation from native object creation, memory allocation/suballocation from bind, view creation from descriptor publication, CPU upload preparation from copy recording, and GPU completion from generation readiness.
9. For every native operation, record allowed concurrent calls, exclusive input/output, required device/allocator/cache synchronization, owner commit, cancellation semantics and device-loss behavior. Parallelize independent object creation only after this audit.
10. Split descriptor work by lifetime: persistent descriptor registry/update remains render-owned or uses a proven two-phase snapshot/commit; transient tables/pages remain recording-lease-local. No resource task races an already-recording command list's descriptor visibility.
11. Define separate shader-compile, PSO-create, resource-create, decode and I/O concurrency/memory budgets for loading and gameplay. Account for third-party internal threads, driver serialization, duplicate similar pipeline compilation and large per-request peak memory; do not use one global “background threads” number blindly.
12. Define per-product “not ready” policy: loading barrier, delayed proxy/draw, bounded fallback, or explicit failure. Never surprise-block inside draw/dispatch recording.
13. Add existing-hook counters/scopes for cold/warm hit, miss, duplicate, too-late, failure, native-create time, driver-lock/queue time where observable, stage queue time, peak stage memory and foreground interference; no new report subsystem.
14. If measurements justify a platform asynchronous-I/O backend, hide it behind the current internal read-completion seam and prove identical cancellation/publication/package behavior.

Validation:
- Cold and warm caches, empty/corrupt/missing files, cancellation at each stage, supersession, process failure, full queues, shutdown and repeated reload.
- Shader/PSO cold cache is actually cleared or version-isolated; D3D12/Vulkan compile/create paths and fallback policies are exercised.
- Cold-cache graphics/compute and applicable RT pipelines; parallel duplicate/similar keys; buffer/image/view/allocation/bind/descriptor-update failures; delayed upload/GPU completion; native validation on both backends.
- Foreground p95/p99, total load/cook throughput and peak memory are measured across multiple worker/I/O/compile budgets.
- Sweep compile/create concurrency independently. Record driver serialization/negative scaling, peak memory, first-ready time and frame interference; retain serial or capped policies where they win.
- Accepted load/reload performs no routine device idle; parallel recording sees no lazy PSO/layout/resource mutation.

Acceptance gate:
- L Sections 19-21 (reconciliation, acceptance checklist, and required completion report) and Section 24 (Principal Graphics Engineering review gate) pass for the entire touched changelist, including directly affected neighboring ownership paths; no undocumented exception remains.
- The binding Prompt 13-29 changelist design gate passes for the entire prompt changelist, including touched neighboring counterparts.
- I/O, CPU work, native creation, upload and publication are distinct owned stages with bounded backpressure.
- First-run/cold-cache behavior is observable, deterministic where applicable and has an explicit user-visible policy.
- One asset/shader/PSO generation authority remains; superseded paths and caches are deleted.
- Every applicable RHI resource-work row in the coverage ledger has a safety contract and proof; RT pipeline-library/collection and parallel native-creation variants are retained only with a real consumer and measured benefit.

Positive patterns: staged ownership, bounded capacity, generation rejection, cold-cache testing, deduplication, memory-aware concurrency.
Forbidden: “async” blocked frame worker, warm-cache-only proof, PSO creation in recording, unbounded compile fan-out, second asset/cache system, DirectStorage résumé port.
~~~

## Prompt 28 — Prove GPU Queue Concurrency, Frame Pacing, and Correlated Latency

Target CL Title: `SparkleRenderer: Prove GPU Queue Concurrency, Frame Pacing, and Latency`

~~~text
Implement Prompt 28 only after Prompt 27 passes.

Objective:
Correlate CPU tasks, render submission, GPU graphics/compute/copy execution and presentation; retain only queue overlap and pipeline depth that improve a measured product objective without violating provider ownership or latency.

Non-negotiable repository rules:
- Apply `Docs/Architecture/00-Review/L_SparkleEngineIntegrationStyleGuide.md` in full. Treat its ownership, implementation-shape, DOD, concurrency, rendering, naming, validation, structure, principal graphics engineering, and completion-report gates as acceptance criteria. Before editing, list the applicable `PGE-01` through `PGE-15` requirements from A, interpret them through H, and classify each as advance, preserve, not applicable, or blocked with expected evidence. Inspect the current repository, reconcile the whole touched ownership path, verify any AI-assisted work independently, delete replaced paths in the same change, reject role-only scaffolding, and report any justified exception explicitly with its owner, scope, evidence, and deletion or review gate.
- Apply Rule 13: audit every data source, transform, stream, packet, cache, table, upload, and hot traversal touched by this prompt; record the concrete access inventory, authoritative/derived ownership, layout decision, stable identity, deterministic transform, exact source precedent, and measured falsifier.
- Apply Rule 12: audit touched/new files for module and folder ownership, public/private placement, filename-to-primary-type alignment, and nearby misplaced counterparts; complete bounded moves/renames with includes, CMake, and documentation updated before the gate.
- Enforce J's canonical concurrency/rendering vocabulary and Rule 10: search canonical terms plus rejected aliases in the touched scope, use one responsibility per name, and delete temporary compatibility spellings before the gate.
- Inspect existing `FrameId`/sequence markers, frame-graph queue assignment, RHI timelines, present path, provider/frame-generation integration, `RenderFrameQueue`, and profiler hooks before adding fields or events.
- Keep the frame graph as queue/dependency/barrier authority and the render coordinator as submit/present owner.
- Apply daily refactoring to converge duplicate frame IDs, waits, pacing settings and provider queue state.
- Do not make the architecture vendor-specific. Vendor latency APIs are optional adapters through existing provider boundaries; engine markers and fallback remain authoritative.

Required implementation:
1. Carry one correlated produced/rendered/presented frame identity through input sample, simulation start/end, packet publish/consume, recording, queue submit, GPU timestamps and present start/end.
2. Measure serial, threaded zero-ahead and one-ahead modes in CPU-bound and GPU-bound workloads; record CPU lead, backpressure, throughput, frame pacing and input-to-present stages.
3. Select real async-compute and copy candidates from declared frame-graph dependencies. Model graphics/compute/copy ownership, cross-queue signal/wait, resource-family/state transitions and lifetime.
4. Compare serial-queue and multi-queue execution using the same logical passes. Measure overlap, queue idle gaps, synchronization/submission CPU cost, bandwidth contention and GPU critical path.
5. Audit D3D12/Vulkan provider/frame-generation queue requirements including shared versus exclusive queues, present/acquire behavior, callbacks, shutdown and fallback. Assert illegal concurrent submission/ownership where possible.
6. Batch submissions only when it reduces CPU cost without delaying required GPU work or inflating latency; record the tradeoff.
7. Establish bounded pacing/backpressure policy for GPU-bound overload, VSync/VRR/present modes available to the engine and provider enabled/disabled paths.
8. Remove async/copy assignments or extra frame lead that lose. Preserve feature correctness and temporal frame semantics.

Validation:
- D3D12/Vulkan serial/parallel recording, graphics-only/multi-queue, zero/one-ahead and supported present/provider modes.
- Queue timelines from PIX/GPUView/RGP/Nsight/RenderDoc or appropriate backend tools correlate with engine frame/pass markers.
- Repeated minimize/resize/recreate, provider toggle/failure, frame-generation, capture and shutdown show no deadlock, stale frame ID or illegal queue use.
- Report CPU/GPU p50/p95/p99, pacing variance, stage latency and queue overlap with exact hardware/driver/configuration.

Acceptance gate:
- L Sections 19-21 (reconciliation, acceptance checklist, and required completion report) and Section 24 (Principal Graphics Engineering review gate) pass for the entire touched changelist, including directly affected neighboring ownership paths; no undocumented exception remains.
- The binding Prompt 13-29 changelist design gate passes for the entire prompt changelist, including touched neighboring counterparts.
- Every overlap claim identifies the two timelines, enabling dependency/fence and measured benefit.
- Pipeline depth/backpressure has an explicit throughput-versus-latency product policy.
- Provider queue ownership is documented and enforced; no shared queue can deadlock through uncontrolled submission.

Positive patterns: correlated frame identity, graph-derived queues, measured overlap, bounded CPU lead, explicit provider ownership, negative-result deletion.
Forbidden: two queues imply overlap, FPS-only latency claim, vendor-only frame identity, provider-owned renderer scheduling, unbounded frames ahead, gratuitous fence/submission traffic.
~~~

## Prompt 29 — Production Forensics, Expert Defense, and Final Portfolio Release

Target CL Title: `SparkleEngine: Complete Production Forensics and Portfolio Release`

~~~text
Implement Prompt 29 only after Prompts 24, 26, and 28 pass.

Objective:
Prove the completed architecture can be diagnosed, defended and reproduced at an AMD/NVIDIA graphics/systems interview bar, then release a concise honest portfolio without adding an interview-only product surface.

Non-negotiable repository rules:
- Apply `Docs/Architecture/00-Review/L_SparkleEngineIntegrationStyleGuide.md` in full. Treat its ownership, implementation-shape, DOD, concurrency, rendering, naming, validation, structure, principal graphics engineering, and completion-report gates as acceptance criteria. Before editing, list the applicable `PGE-01` through `PGE-15` requirements from A, interpret them through H, and classify each as advance, preserve, not applicable, or blocked with expected evidence. Inspect the current repository, reconcile the whole touched ownership path, verify any AI-assisted work independently, delete replaced paths in the same change, reject role-only scaffolding, and report any justified exception explicitly with its owner, scope, evidence, and deletion or review gate.
- Apply Rule 13: audit every data source, transform, stream, packet, cache, table, upload, and hot traversal touched by this prompt; record the concrete access inventory, authoritative/derived ownership, layout decision, stable identity, deterministic transform, exact source precedent, and measured falsifier.
- Apply Rule 12: audit touched/new files for module and folder ownership, public/private placement, filename-to-primary-type alignment, and nearby misplaced counterparts; complete bounded moves/renames with includes, CMake, and documentation updated before the gate.
- Enforce J's canonical concurrency/rendering vocabulary and Rule 10: search canonical terms plus rejected aliases in the touched scope, use one responsibility per name, and delete temporary compatibility spellings before the gate.
- Use existing engine instrumentation and external primary tools; do not add a crash service, telemetry/report framework, task panel or benchmark product.
- Search/consolidate J, K and the existing concise product overview. Do not create another architecture/policy document.
- Apply daily refactoring to remove injected-failure hooks from shipping paths, consolidate markers/settings and close every remaining deletion ledger.
- Do not claim AMD/NVIDIA employment equivalence, proprietary-engine knowledge, unavailable hardware validation or “production ready” without defined scope.

Required implementation:
1. Create three reproducible incidents in existing tests/developer launch surfaces: race/lifetime or stale generation; deadlock/stall/priority inversion; and scaling/pacing/latency regression.
2. Diagnose each timeline-first: exact reproduction, classification, serial comparison, system/native capture, critical path/owner, falsifiable hypotheses, injected delay, root cause, fix, regression and post-fix measurement.
3. Use appropriate evidence among WPA/ETW, Visual Studio, AMD uProf/RGP, Nsight Systems/Graphics, PIX/GPUView, RenderDoc, Vulkan/D3D12 validation, sanitizers and crash dumps. State tool/hardware gaps.
4. Complete J's 30-question bank, whiteboard and coding drills live: bounded-queue atomic protocol, DAG critical path, deadlock repair, deterministic compaction, native recording review, and trace diagnosis.
5. Produce final portfolio artifacts: ownership/frame diagrams, task/ECS graphs, atomic state machine, topology scaling curve, algorithm walkthrough, staged streaming/PSO trace, D3D12/Vulkan recording, GPU queue/latency timeline, incident reports and limitations.
6. Attribute gains separately to removed work, data layout, CPU tasks, game/render overlap, native recording, GPU queue scheduling, and shader/GPU changes. Retain negative results.
7. Re-run the complete serial/thread/backend/feature/cold-cache/stress/performance matrix and independently reproduce representative results.
8. Verify every public API, tool, task primitive, package, setting and retained diagnostic hook has a current product consumer; delete temporary or duplicate surfaces.
9. Rerun the exact repository-wide legacy-concurrency audit from Prompt 00. Explain every retained standard/Qt/native primitive from owner to falsifying test, verify no old LC replacement path reappeared during tuning, and include the zero-unclassified result in the portfolio review.
10. Reconcile MT-01 through MT-44 against the final code. For every ID, link the concrete preventing owner/pattern and test/trace, or mark it non-applicable with a reviewable reason. Run an adversarial review that attempts to reintroduce raw cross-owner state, worker wait, tiny work, false sharing, native allocator sharing, premature GPU reuse, nondeterministic merge, and misleading performance claims.
11. Close every row in `Renderer/RHI Use-Case-to-Prompt Coverage` with code/test/capture evidence or a source-backed non-applicability decision. Teach back light/shadow/mesh preparation, shader/PSO/resource staging, direct versus software-translated RHI recording, D3D12/Vulkan list ownership, the qualified meanings of merge, submit batching, retirement, and the CPU-task versus GPU-queue distinction. Distributed light baking and an RHI translation thread remain defer decisions unless separately approved through their product/ADR gates.
12. Close Rule 13 repository-wide. For each frame-hot GameFramework, editor publication, game-to-render extraction, RenderWorld/GPU-scene, cooker, and renderer-preparation stream, link the authoritative/derived ownership, data/access inventory, concrete layout, stable identity, deterministic transform, replaced object/full-copy path, exact source precedent, and measurement. Demonstrate the Two Data Streams and one representative ECS system live from source columns through packet and render slot to projected GPU update.
13. Update release-facing Current/validated status and limitations only after evidence passes; close J's Definition of Done and every K gate.

Validation:
- All 32 prompt reports exist; exact commands/configurations/hardware are recorded and representative reruns are comparable within stated variability.
- Three incident regressions fail before their fixes or fault injection and pass after; native validation and sanitizer-supported runs remain clean.
- Portfolio claims link to code/tests/captures and separate CPU/GPU/latency causality.
- A mock adversarial review can alter a scenario or trace and still receive a reasoned answer, not a memorized script.
- The renderer/RHI coverage ledger contains no “could be parallel” closure; each row is implemented/proven, explicitly non-applicable, or assigned to a separately approved future renderer program.

Acceptance gate:
- L Sections 19-21 (reconciliation, acceptance checklist, and required completion report) and Section 24 (Principal Graphics Engineering review gate) pass for the entire touched changelist, including directly affected neighboring ownership paths; no undocumented exception remains.
- The binding Prompt 13-29 changelist design gate passes for the entire prompt changelist, including touched neighboring counterparts.
- The owner can design, code, debug, measure and teach the core engine concurrency concepts under questioning.
- Sparkle solves real loading, framework, editor, renderer, RHI and tools problems with one coherent ownership architecture.
- The final product has no interview-only subsystem, hidden duplicate path or unsupported vendor claim.
- The final product has no unclassified thread, lock, atomic, wait, detached lifetime, callback-under-lock, multi-owner queue, or routine device-idle path.
- Every MT atlas hazard has executable evidence or a concrete non-applicability argument; no closure rests only on a vendor example or absence of an observed failure.

Positive patterns: incident-based evidence, tool selection, live reasoning, causal attribution, reproducibility, honest limitations, deletion closure.
Forbidden: memorized definitions, fabricated capture, cherry-picked FPS, generated report product, unbounded résumé feature, unsupported company/parity claim.
~~~

Prompt 29 is the final multithreading/systems-foundation release. It is not the final `PGE-*` persona gate. The additive role sequence below begins only after Prompt 29 passes and must preserve its completed ownership, determinism, backend, reliability, performance and deletion contracts.

## Prompt 30 - Establish the Principal Path-Tracing, Mathematics, and Partner Baseline

Target CL Title: `SparkleRenderer: Establish the Principal Path-Tracing and Partner Baseline`

~~~text
Implement Prompt 30 only after Prompt 29 passes.

Objective:
Turn one representative Sparkle path-traced workload into a principal-level baseline that connects product requirements, rendering mathematics, shader/RHI execution, CPU/GPU system cost, exact hardware/driver configuration, and partner adoption constraints. Do not add a neural feature in this prompt.

Non-negotiable repository rules:
- Apply `Docs/Architecture/00-Review/L_SparkleEngineIntegrationStyleGuide.md` in full. Treat its ownership, implementation-shape, DOD, concurrency, rendering, naming, validation, structure, principal graphics engineering, and completion-report gates as acceptance criteria. Before editing, list the applicable `PGE-01` through `PGE-15` requirements from A, interpret them through H, and classify each as advance, preserve, not applicable, or blocked with expected evidence. Inspect the current repository, reconcile the whole touched ownership path, verify any AI-assisted work independently, delete replaced paths in the same change, reject role-only scaffolding, and report any justified exception explicitly with its owner, scope, evidence, and deletion or review gate.
- Apply K Rules 1-17 and report the complete 19-item completion record.
- Apply Rule 13 to path state, samples, bounces, material/light reads, random sequences, accumulation/history, guide buffers, RT structures, shader records, queues, packets, captures and readback.
- Apply Rule 12/14/16 to every touched renderer, shader, RHI, project and documentation file; orchestration, estimator/math, scene setup, capture and comparison remain separate owners.
- Advance `PGE-01`, `02`, `05`, `06`, `08`, `09`, `10`, `13` and `15`; preserve all other role contracts.
- Use existing reference path, frame graph, RHI, shader package, capture and profiler/debugger hooks. Do not add a second path tracer, report framework, benchmark product or partner facade.
- Keep D3D12/Vulkan behavior and current classic TLAS/PTLAS/provider/capture paths intact.

Required implementation:
1. Define one partner-shaped scenario: target visual result, scene/content constraints, resolution, frame/latency/memory budget, supported/fallback modes, integration inputs/outputs and failure behavior.
2. Select a curated deterministic scene, camera path, light/material state, seed/sample schedule and reference-output procedure. Package heavy optional assets deliberately.
3. Document the estimator and numerical contract used by the current path: coordinate spaces, throughput/radiance units, BRDF/PDF/MIS terms where applicable, termination, accumulation, precision and reference tolerances.
4. Add the smallest executable reference tests for material math, sampling expectations, transforms, accumulation and numerical limits. Keep tests with the owning math/shader contract; do not create a general math-validation framework.
5. Trace the complete frame from GameFramework/render input through RenderWorld, preparation, frame graph, shader parameters, BLAS/TLAS/PTLAS, command recording, submission, GPU execution, history and presentation.
6. Record D3D12 and Vulkan images, native validation, shader/reflection state, CPU/GPU timings, RT build/update cost, upload/descriptor/memory pressure, queue behavior, frame pacing and input-to-present where applicable.
7. Record exact CPU, GPU architecture/adapter, driver, OS, backend, compiler, resolution, feature/capability, provider, power and validation configuration.
8. Compare a simple analytical cost prediction with measured path/sample/bounce/ray/workload behavior. Explain divergence through captures rather than changing the model to fit silently.
9. Produce a concise internal adoption section in the existing appropriate document: prerequisites, integration contract, fallback, reproduction, debugging path, limitations and priority/deletion decisions.
10. Delete temporary capture scripts, duplicate comparison paths and diagnostics that are not required for reproducibility through existing owned surfaces.

Validation:
- Deterministic reference values and image tolerances pass repeatedly.
- D3D12/Vulkan outputs and feature/capability classifications are honest and native validation is clean.
- Tiny and representative scenes, cold/warm state and classic TLAS/PTLAS where supported are recorded.
- A second engineer can reproduce the workload from a clean checkout plus declared optional content.
- The cost model predicts direction/order of magnitude or the discrepancy is explained by measured architecture/driver behavior.

Acceptance gate:
- L Sections 19-21 (reconciliation, acceptance checklist, and required completion report) and Section 24 (Principal Graphics Engineering review gate) pass for the entire touched changelist, including directly affected neighboring ownership paths; no undocumented exception remains.
- One path-traced workload can be defended from math and data through shader/RHI/GPU result and system latency.
- Partner requirements, fallback and adoption cost are explicit without a new broad API.
- No neural/model/tensor framework, fake workload, report product or unsupported hardware claim was added.
- `PGE-01/02/05/06/08/09/10/13/15` have concrete baseline evidence.

Positive patterns: deterministic workload, executable math, exact configuration, paired API evidence, partner-shaped constraints, honest cost model.
Forbidden: screenshot-only proof, FPS-only result, copied equation without tests, vendor sample as proof, unscoped driver claim, new benchmark/report system.
~~~

## Prompt 31 - Select the Neural Feature and Build the Deterministic Model Artifact Contract

Target CL Title: `SparkleRenderer: Establish the Neural Feature and Model Artifact Contract`

~~~text
Implement Prompt 31 only after Prompt 30 passes.

Objective:
Select one current renderer/game path for a real neural replacement or material improvement, establish its classical baseline, model/data/math contract, isolated training or fine-tuning workflow, deterministic export/cook and immutable runtime artifact. Do not integrate GPU runtime inference yet.

Non-negotiable repository rules:
- Apply `Docs/Architecture/00-Review/L_SparkleEngineIntegrationStyleGuide.md` in full. Treat its ownership, implementation-shape, DOD, concurrency, rendering, naming, validation, structure, principal graphics engineering, and completion-report gates as acceptance criteria. Before editing, list the applicable `PGE-01` through `PGE-15` requirements from A, interpret them through H, and classify each as advance, preserve, not applicable, or blocked with expected evidence. Inspect the current repository, reconcile the whole touched ownership path, verify any AI-assisted work independently, delete replaced paths in the same change, reject role-only scaffolding, and report any justified exception explicitly with its owner, scope, evidence, and deletion or review gate.
- Apply K Rules 1-17, Rule 13 DOD, Rule 12 structure, Rule 16 implementation ownership and the 19-item completion report.
- Advance `PGE-03`, `04`, `08`, `11`, `12`, `13` and `15`; preserve the path-tracing/backend/system contracts.
- Search existing denoising, reconstruction, upscaling, sampling, texture/material, animation, shader/provider and artifact/cook paths before selecting a feature.
- Stop if no candidate has current product value, a legal/provenance-clean dataset or training source, a deterministic artifact route, a classical fallback and measurable quality/performance criteria.
- Do not add a general tensor library, model manager, graph runtime, training UI, runtime Python/PyTorch/ONNX dependency, second asset database, opaque downloader or mock network.
- Training/offline dependencies remain tool/research-only and are excluded from runtime/editor packages.

Required implementation:
1. Compare candidate features by current consumer, replaced work/code, dataset/provenance feasibility, mathematical clarity, expected quality gain, inference budget, backend feasibility, fallback and maintenance cost. Select one and record rejected candidates.
2. Define the classical baseline and unchanged input/output semantics. The neural path must be switchable for controlled comparison and removable without breaking the baseline.
3. Define data provenance, license, generation, train/validation/test split, preprocessing, augmentation and leakage controls. Do not commit uncataloged heavyweight data to the core depot.
4. Define model/operator math: shapes, layout, normalization, receptive field/access, loss, quality metrics, gradients/optimization where applicable, numerical range, precision and expected runtime operations/bytes.
5. Implement or adopt the smallest isolated training/fine-tuning/reference workflow required by the feature. Pin inputs/options/seeds and publish deterministic metadata.
6. Measure training/offline preparation separately: step/epoch/export time as applicable, batch/precision behavior, CPU/GPU utilization, peak memory, convergence and retained negative trials. Do not build a telemetry platform.
7. Define one canonical immutable runtime artifact carrying only required weights/constants/metadata with bounded shapes, layout, precision, checksum/provenance and feature capability requirements. Regenerate to the newest format; do not add legacy content-version compatibility.
8. Integrate artifact validation/cooking into the existing narrow shader/asset/tool ownership path. Runtime packages contain the validated artifact, not the training stack or source dataset.
9. Implement a deterministic offline/reference inference oracle for artifact/export validation and compare it with the training framework result within defined tolerance.
10. Add malformed, wrong-shape/layout/precision, truncated, non-finite, provenance/checksum and unsupported-capability rejection tests at the artifact owner.
11. Record AI-tool use, if any, and independently verify generated code, math, model design, data scripts, tests, citations and claims.
12. Delete candidate spikes, duplicate exporters, debug dumps, compatibility artifacts and unused dependencies before the gate.

Validation:
- Repeated training/export or deterministic fixed-model export produces the promised artifact identity and reference output within defined policy.
- Train/validation/test boundaries and data provenance are reviewable.
- Classical baseline and offline neural reference run on the same representative inputs and quality metrics.
- Runtime/editor/tool packages contain no training framework or source dataset dependency.
- Artifact validation rejects malformed and unsupported input without partial publication.
- Peak training/export memory and time are recorded for exact hardware/configuration.

Acceptance gate:
- L Sections 19-21 (reconciliation, acceptance checklist, and required completion report) and Section 24 (Principal Graphics Engineering review gate) pass for the entire touched changelist, including directly affected neighboring ownership paths; no undocumented exception remains.
- A real feature, real data/model/operator path and real classical baseline are selected.
- The artifact is deterministic, immutable, bounded, validated, package-owned and ready for runtime consumption.
- Training/offline and runtime ownership are separate.
- No generic ML framework surface, empty tensor abstraction, mock model or hidden data/license ambiguity remains.

Positive patterns: feature replacement, model/data provenance, bounded artifact, deterministic export, offline oracle, classical baseline.
Forbidden: AI branding, toy model disconnected from product, runtime training dependency, opaque weights, data leakage, quality metric without visual failures.
~~~

## Prompt 32 - Integrate Renderer-Owned Neural Inference with a Classical Fallback

Target CL Title: `SparkleRenderer: Integrate the Neural Graphics Inference Path`

~~~text
Implement Prompt 32 only after Prompt 31 passes.

Objective:
Consume the accepted immutable artifact in one renderer-owned neural graphics path using existing shader, frame-graph, RenderWorld/GPU-scene, task and RHI contracts. Establish correctness and D3D12/Vulkan capability/fallback behavior before aggressive tuning.

Non-negotiable repository rules:
- Apply `Docs/Architecture/00-Review/L_SparkleEngineIntegrationStyleGuide.md` in full. Treat its ownership, implementation-shape, DOD, concurrency, rendering, naming, validation, structure, principal graphics engineering, and completion-report gates as acceptance criteria. Before editing, list the applicable `PGE-01` through `PGE-15` requirements from A, interpret them through H, and classify each as advance, preserve, not applicable, or blocked with expected evidence. Inspect the current repository, reconcile the whole touched ownership path, verify any AI-assisted work independently, delete replaced paths in the same change, reject role-only scaffolding, and report any justified exception explicitly with its owner, scope, evidence, and deletion or review gate.
- Apply K Rules 1-17 and the 19-item completion report.
- Apply Rule 13 to every tensor/image/buffer source, preprocessing transform, weight upload, intermediate, dispatch, history, output, fallback and hot traversal.
- Apply Rule 12/14/15/16: separate feature orchestration, artifact decoding, resource state, preprocessing, inference kernels, postprocessing, quality comparison and backend capability policy by real owner.
- Advance `PGE-03`, `04`, `05`, `07`, `08`, `09`, `10`, `11`, `12` and `15`.
- Use existing HLSL/Slang, shader packages, frame graph, RHI services, renderer tasks, persistent GPU storage, descriptor/upload ownership and capture hooks.
- Do not add a generic inference graph/runtime, CUDA/HIP backend, second frame graph, worker submit, per-frame model parse/upload, mutable model data, completion-order merge or app-level neural state.

Required implementation:
1. Define the feature-owned runtime request/state: artifact handle, bounded input/output descriptors, required history, precision/layout, capability and classical fallback. Keep volatile mechanics private.
2. Resolve and validate the artifact outside frame-hot work. Create persistent renderer-owned weights/constants/resources and retire replacements by existing GPU lifetime rules.
3. Implement explicit preprocessing, inference operator/kernel sequence and postprocessing through declared frame-graph resources/passes. Each pass declares reads/writes/history/queue and performs no lazy resource/PSO/model creation.
4. Keep tensor/image layouts concrete and access-specific. Record selected and rejected AoS/SoA/packed/tiled/channel layouts, alignment, precision and shader access.
5. Integrate the smallest CPU preparation through existing serial/task contracts. Use immutable inputs, exclusive ranges/task-local output and deterministic merge; retain a small-work serial path.
6. Implement the shader/kernel path in HLSL/Slang with explicit bounds, coordinate convention, precision, wave/cooperative capability and fallback. Do not hide operations behind a general operator abstraction unless two current operators share the exact contract.
7. Add D3D12/Vulkan resource, descriptor, synchronization and pipeline behavior through existing neutral RHI contracts. Backend-specific workarounds stay private and evidence-scoped.
8. Compare runtime output with the offline/reference oracle and classical baseline using deterministic inputs, numerical tolerances, feature quality metrics and visual failure cases.
9. Implement unsupported-artifact/capability/device/resource failure as a typed fallback to the classical path without partial history/publication or device idle.
10. Preserve capture, temporal reset, resize/minimize, reload, level change, provider and shutdown behavior.
11. Delete temporary CPU/reference runtime paths, per-frame debug dumps, duplicate feature settings and experiment scaffolding after parity.

Validation:
- Offline/reference and runtime neural outputs match defined numerical/quality tolerance.
- Classical/neural comparison uses identical inputs and deterministic camera/workload.
- D3D12/Vulkan native validation passes where the required capability exists; unsupported cases take the tested classical fallback.
- Reload/resize/history reset/level change/delayed GPU/shutdown have no stale artifact/resource or partial output.
- Packet/model memory poison and delayed consumption prove no transient pointer retention.
- Serial/1/2/N task modes retain deterministic output where CPU preparation is parallel.

Acceptance gate:
- L Sections 19-21 (reconciliation, acceptance checklist, and required completion report) and Section 24 (Principal Graphics Engineering review gate) pass for the entire touched changelist, including directly affected neighboring ownership paths; no undocumented exception remains.
- A real neural model/operator executes inside the actual renderer feature path.
- Resources, passes, artifacts, CPU preparation, GPU lifetime and fallback have one clear owner each.
- The classical product path remains valid and no generic ML/runtime architecture was added.
- Correctness and backend/fallback behavior pass before performance claims.

Positive patterns: concrete tensor layout, immutable artifact, persistent weights, declared passes, offline oracle, classical fallback.
Forbidden: fake model, generic graph runtime, per-frame weight upload, hidden operator dependency, lazy recording mutation, quality-by-screenshot only.
~~~

## Prompt 33 - Tune the Neural Model, Kernels, System Path, and Driver Interaction

Target CL Title: `SparkleRenderer: Tune Neural Graphics across Model, Kernel, and Hardware`

~~~text
Implement Prompt 33 only after Prompt 32 passes.

Objective:
Profile and optimize the accepted neural feature across model/algorithm, training/export, CPU preparation, GPU kernels, memory, scheduling, frame pacing and driver interaction. Keep only changes that improve the measured quality-performance-product frontier.

Non-negotiable repository rules:
- Apply `Docs/Architecture/00-Review/L_SparkleEngineIntegrationStyleGuide.md` in full. Treat its ownership, implementation-shape, DOD, concurrency, rendering, naming, validation, structure, principal graphics engineering, and completion-report gates as acceptance criteria. Before editing, list the applicable `PGE-01` through `PGE-15` requirements from A, interpret them through H, and classify each as advance, preserve, not applicable, or blocked with expected evidence. Inspect the current repository, reconcile the whole touched ownership path, verify any AI-assisted work independently, delete replaced paths in the same change, reject role-only scaffolding, and report any justified exception explicitly with its owner, scope, evidence, and deletion or review gate.
- Apply K Rules 1-17 and the 19-item completion report.
- Advance `PGE-04`, `05`, `06`, `08`, `09`, `10`, `11`, `12` and `15`; preserve partner/final communication for Prompt 34.
- Use existing profiler/debugger/native validation/capture hooks plus appropriate external tools; do not add operator telemetry, CSV/JSON report generation, benchmark UI or permanent tuning switches.
- Every optimization has a serial/classical/reference control, exact hardware/driver/configuration and a falsifiable hypothesis.
- Keep quality, visual stability, temporal behavior, latency, memory and frame pacing beside throughput.
- Driver workarounds require a reduced reproducer, exact applicability predicate and removal/retest rule.

Required implementation:
1. Build an end-to-end timeline separating input/preprocess, CPU preparation, uploads, each inference stage, postprocess, history, frame-graph scheduling, record, submit, GPU execution and presentation.
2. Profile model/operator alternatives and ablations: capacity, receptive field/sequence length as applicable, loss/metric weighting, precision/quantization, pruning/fusion and artifact size. Retain the smallest model meeting quality/product targets.
3. Profile training/offline configuration separately: batch, precision, data pipeline, optimizer/schedule where applicable, memory, convergence and export. Do not optimize training at the expense of runtime contract correctness.
4. Profile kernel/layout alternatives: channel/tile/packing, vectorization/wave/cooperative operations, fusion, dispatch geometry, barriers, intermediates, shared memory, registers, occupancy, cache/bandwidth and divergence.
5. Profile CPU/system alternatives: artifact resolution, preprocessing layout, task grain, allocation reuse, upload/descriptor staging, queue choice, frame overlap and background workload budgets.
6. Measure tiny, representative and stress workloads; classical/neural; serial/task modes; cold/warm; D3D12/Vulkan; supported GPU architectures/drivers available; and concurrent representative gameplay/animation/background work.
7. Record p50/p95/p99 CPU/GPU stage time, end-to-end frame time, frame pacing, input-to-present, peak resident/artifact/intermediate memory, upload bytes and quality metrics.
8. Compare the math/operation/byte cost model with captures and counters/disassembly where available. Explain compute-, bandwidth-, occupancy-, synchronization-, driver- or latency-bound behavior.
9. Investigate at least one hardware/driver-sensitive result. Reduce it outside the full feature where possible, classify application/API/driver ownership, validate the fix/fallback and avoid universal claims.
10. Express future-hardware opportunities only as capability-driven hypotheses with the data/layout seam that would permit adoption. Do not add untested public APIs or dormant runtime paths.
11. Remove tuning controls, model variants, kernels, layouts, workarounds and task policies that lose or have no current product use.

Validation:
- Independent reruns reproduce retained improvements within stated variability.
- Quality does not regress outside the accepted frontier and visual/temporal failure cases remain bounded.
- Tiny workload remains near classical/serial overhead budget; stress workload has a stable memory ceiling.
- D3D12/Vulkan validation and capability/fallback remain correct.
- Reduced hardware/driver case reproduces the issue and distinguishes application behavior from driver hypothesis.
- Capture-on/off and instrumentation-overhead comparisons prevent measurement tooling from becoming the result.

Acceptance gate:
- L Sections 19-21 (reconciliation, acceptance checklist, and required completion report) and Section 24 (Principal Graphics Engineering review gate) pass for the entire touched changelist, including directly affected neighboring ownership paths; no undocumented exception remains.
- The accepted model, precision, layout, kernels, task policy and frame placement are justified by measured product tradeoffs.
- The feature meets its quality, latency, memory and pacing budget on every hardware/backend configuration claimed.
- Negative results and unavailable hardware are retained honestly.
- No losing tuning branch, speculative future path, broad workaround or measurement product remains.

Positive patterns: end-to-end timeline, ablation, quality-performance frontier, architecture counters, reduced driver repro, negative-result deletion.
Forbidden: FPS-only tuning, quality-only model choice, cherry-picked GPU, permanent experiment switches, unscoped workaround, future hardware by speculation.
~~~

## Prompt 34 - Complete Partner Handoff and Principal Graphics Engineering Evidence

Target CL Title: `SparkleEngine: Complete Principal Graphics Engineering Evidence`

~~~text
Implement Prompt 34 only after Prompt 33 passes.

Objective:
Close `PGE-01` through `PGE-15` with reproducible implementation evidence, partner-quality handoff, live demonstration and concise technical communication. Remove all role-only scaffolding and leave the product smaller and maintainable.

Non-negotiable repository rules:
- Apply `Docs/Architecture/00-Review/L_SparkleEngineIntegrationStyleGuide.md` in full. Treat its ownership, implementation-shape, DOD, concurrency, rendering, naming, validation, structure, principal graphics engineering, and completion-report gates as acceptance criteria. Before editing, list the applicable `PGE-01` through `PGE-15` requirements from A, interpret them through H, and classify each as advance, preserve, not applicable, or blocked with expected evidence. Inspect the current repository, reconcile the whole touched ownership path, verify any AI-assisted work independently, delete replaced paths in the same change, reject role-only scaffolding, and report any justified exception explicitly with its owner, scope, evidence, and deletion or review gate.
- Apply K Rules 1-17 and the 19-item completion report.
- Use existing documentation and product/test/capture surfaces. Add one feature technical artifact only if no existing document can own the math/result without becoming incoherent; never add another policy, telemetry or report system.
- Re-audit the entire neural/path-tracing ownership path for Rule 12/13/14/15/16, canonical vocabulary, SOLID/DRY, diagnostics, public surface, packages, licenses and deletion.
- Verify every AI-assisted contribution independently and record the verification categories, not private prompts or generated chatter.
- Do not claim degree, years, employer/partner experience, proprietary knowledge, Linux support, driver-development experience, hardware validation or production readiness that the evidence does not establish.

Required implementation:
1. Re-run Prompt 30 path-tracing and Prompt 31-33 neural feature matrices from a clean build with exact source revision, content/artifact identity, hardware, driver, OS, backend, compiler and configuration.
2. Produce one partner-shaped handoff: requirements, prerequisites, integration boundary, build/cook/package steps, capability/fallback, tuning controls retained as product policy, debugging/capture workflow, known failures and adoption cost.
3. Produce a minimal issue reproducer and incident narrative for the selected hardware/driver-sensitive case: symptom, hypotheses, isolation, ownership, fix/fallback, validation and retest condition.
4. Produce a concise whitepaper-quality feature note in the correct existing documentation owner: problem, prior/classical path, math/model/data, architecture, artifact/training, runtime inference, DOD/layout, backend, optimization, quality, performance, limitations and future hypotheses.
5. Produce a conference-talk outline and live demo script that teach the result without relying on hidden state or inflated claims. The demo covers classical/neural comparison, deterministic reload/fallback, D3D12/Vulkan status, captures and one failure/limitation.
6. Perform an adversarial design/code review as if another engine team must maintain the integration. Address unclear ownership, excessive public surface, model/data/license ambiguity, hidden backend policy, fragile fallback, unreproducible tuning and god units.
7. Close every `PGE-*` row with evidence, truthful non-code boundary, explicit unavailable platform/hardware or blocker. A blocked technical row prevents final PASS.
8. Verify all model/source/data/third-party licenses and package ownership. No training dependency, source dataset, debug dump or optional heavyweight asset leaks into the runtime/core package.
9. Delete temporary experiments, candidate models, duplicate artifacts, tuning flags, capture/report code, compatibility paths, unused abstractions, role-keyword types and stale claims.
10. Update release-facing capability wording only after the exact backend/hardware/quality/performance gate passes.

Validation:
- Clean-checkout reproduction of path-tracing and neural outputs, quality metrics and representative performance.
- A reviewer following only the handoff can build, run, switch/fallback, capture and diagnose the feature.
- Whitepaper equations/claims link to executable reference tests and captures; tables state exact configuration and variability.
- Live demo survives reload, resize, level change, unsupported capability/fallback, capture and shutdown.
- Repository searches find no training runtime, mock model, duplicate inference path, role-only scaffold, unsupported claim or temporary tuning surface.
- Full architecture, native validation, feature preservation, package and style-guide gates pass.

Acceptance gate:
- L Sections 19-21 (reconciliation, acceptance checklist, and required completion report) and Section 24 (Principal Graphics Engineering review gate) pass for the entire touched changelist, including directly affected neighboring ownership paths; no undocumented exception remains.
- All technical `PGE-*` requirements have reproducible evidence; non-code credential/history boundaries are stated honestly.
- The feature is real, mathematically understood, optimized across model/kernel/system, driver-aware, adoptable and maintainable.
- Communication artifacts are concise consequences of completed work, not substitutes for it.
- Sparkle contains no generic ML platform, second scheduler/renderer, public telemetry product, training UI or unsupported platform/vendor claim.
- The final repository is smaller or more cohesive along the changed path and all deletion ledgers are closed.

Positive patterns: partner handoff, clean reproduction, whitepaper-quality causality, live failure demo, honest scope, deletion closure.
Forbidden: resume-keyword subsystem, fabricated partner claim, credentials inferred from code, benchmark theater, hidden dataset/model, presentation without implementation.
~~~

## Final Series Gate

The series is complete only when:

- all 37 prompt reports exist and every gate is PASS;
- J's Definition of Done is satisfied by executable evidence;
- all compatibility/deletion ledgers are closed;
- the current repository-wide concurrency scan has zero unclassified hits and each retained primitive has an owner, invariant, blocking/affinity policy, and falsifying test;
- MT-01 through MT-44 have concrete final-code prevention evidence or reviewable non-applicability, and each implementation report contains its applicable hazard pre-mortem/closure;
- the repository has one coherent ownership/data path per responsibility;
- touched architecture subtrees consistently express module and subsystem ownership through folders, public/private placement, filenames, primary type names, namespaces, exports, and CMake targets; old paths and compatibility spellings are absent;
- every frame-hot or cross-owner data path has a Rule 13 DOD reconciliation: one authority, explicit derived projections, measured access-driven layout, stable identity, bounded lifetime, deterministic transformation, deleted object/full-copy predecessor, and exact applicable source precedent;
- GameFramework-to-render publication uses the single proven `RenderWorldDelta` plus `RenderFrameDynamicData` boundary; Renderer performs no ECS query, gameplay-object dereference, or scene-wide rebuild for an isolated dirty change;
- the implementation remains understandable in serial mode;
- the owner can explain and defend every major design, tradeoff, failure mode, and measurement without relying on this document as a script.
- every `PGE-01` through `PGE-15` requirement has reproducible technical evidence, a truthful non-code boundary or an explicit unavailable-hardware/platform statement; no technical row remains blocked;
- one path-traced workload and one real neural graphics feature connect math/model/data through C++/shader/RHI execution to CPU/GPU/driver evidence, classical fallback and product latency/memory;
- training/offline preparation and runtime inference remain separate, deterministic and intentionally packaged;
- another engineer can adopt, reproduce, debug and tune the principal feature from the handoff;
- the final live demo, whitepaper-quality note and talk outline are backed by code/tests/captures and introduce no runtime reporting product.

If any condition is false, the complete principal graphics engineering program is still in progress. Prompt 29 may still be a valid completed multithreading foundation, but it must not be relabeled as final `PGE-*` completion.

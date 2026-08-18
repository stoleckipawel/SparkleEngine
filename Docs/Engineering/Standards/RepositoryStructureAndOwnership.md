# Repository Structure and Ownership

Status: binding repository structure and ownership standard

Applies to: modules, dependencies, subsystem decomposition, APIs, state authority, and lifetimes

This standard owns implementation guardrails for architecture changes. The [repository map](../../Architecture/WholeRepositoryMap.md) describes a dated current-state snapshot and routes focused documents that own accepted and target system designs. Link those designs here; do not restate them as general rules.

## Repository Shape

Repository structure should read as:

```text
Module
  Public
    stable cross-module contracts
  Private
    Subsystem
      orchestration
      capability- or lifetime-specific implementation
      backend or policy variants where real
```

Hierarchy communicates ownership, lifetime, backend separation, or an independently changing capability. It is not a target for maximum directory or class count.

The repository patterns that currently justify this structure are explicit module `Public`/`Private` ownership, CMake-controlled dependencies, Tasks split by graph/execution/scheduling/lifetime/profiling, GameFramework split by storage/systems/resources/publication/extraction, Renderer split by orchestration and rendering capabilities, and backend-native ownership isolated under D3D12/Vulkan RHI implementation. Existing large units remain audit candidates; existing small files are not automatically cohesive.

## Module Responsibilities

| Module | Owns | Must not own |
| --- | --- | --- |
| `Core` | low-level values, diagnostics bootstrap, process/string/time/math utilities, portable ownership primitives | renderer, editor, game, backend, or feature policy |
| `Platform` | windows, input backends, OS integration | renderer scheduling or game-world policy |
| `Tasks` | product-independent task DAG, execution, scopes, lanes, cancellation, worker runtime | renderer, RHI, ECS, editor, provider, or cooking policy |
| `GameFramework` | runtime world, private ECS, systems, immutable resources, loading commit, world publication and extraction | renderer/RHI mutation or editor UI |
| `RHI` | explicit backend-neutral GPU contracts and backend-private D3D12/Vulkan implementation | frame-graph policy, game types, renderer shader-data policy |
| `Renderer` | render world, frame preparation, frame graph, passes, providers, render coordination, GPU-scene policy | ECS queries, `GameWorld` dereference, editor state, native APIs outside narrow bridges |
| `Editor` | ImGui presentation, panels, immutable editor models, semantic UI requests | live ECS storage, renderer cache ownership, task-scheduler internals |
| `Application` | host orchestration, lifecycle, subsystem composition, boundary publication | mechanisms with an existing subsystem owner |
| `Tools` | import, cook, compiler, launcher, packaging, and validation workflows | runtime scene/render authority or a duplicate task runtime |

For neural graphics, Renderer owns feature policy, inference passes, persistent runtime model resources, fallback, and quality/performance placement. RHI owns resources, pipelines, synchronization, capabilities, and backend implementation without model semantics. Offline tools may own training and export; runtime packages do not link the training stack.

## Dependency Rules

- `Core` remains the bottom dependency.
- `Tasks` depends only on minimal low-level requirements.
- `GameFramework` MUST NOT depend on Renderer or RHI.
- RHI MUST NOT depend on Renderer.
- Renderer consumes stable render contracts and public RHI capability, never private ECS storage or `GameWorld`.
- Native D3D12/Vulkan types remain backend-private or cross through one deliberately narrow provider bridge.
- Editor and Application integrate through narrow public contracts; they are not escape hatches for private storage.
- A public dependency is not justified because one implementation happens to include a type.
- Transitive include convenience is not an ownership argument.

When a dependency changes, update CMake, includes, exports, source groups, PCH assumptions, tests, and boundary checks atomically. Inspect the transitive public surface and remove obsolete compatibility includes.

## Complexity Budget

Complexity is permanent state another engineer must understand, validate, and retire. Every new service, abstraction, type, layer, cache, queue, task, thread, mode, flag, configuration key, public API, and diagnostic needs:

- a current product or engine consumer;
- one owner and one enforced invariant;
- a lifetime and failure contract;
- a reason direct code or an existing owner cannot express the behavior;
- a validation or measurement that can prove its value;
- a deletion, replacement, or reevaluation condition when its need is temporary or evidence-dependent.

Legacy and compatibility paths are not complexity-budget candidates under the [current clean-break policy](IntegrationStyleGuide.md#current-clean-break-policy); they are prohibited for owned Sparkle representations.

A change MAY add many lines while reducing system complexity. Review concepts and authority, not net line count: prefer fewer mutable representations, modes, ownership crossings, public contracts, and paths through which the same result can be produced. Do not prepay for hypothetical backends, variants, reuse, or scale. Add a seam when a real boundary changes independently, not because a design pattern has a familiar name.

## Definition and Usage Placement

Every capability, policy, transform, validation rule, and mutable fact has one authoritative definition in the narrowest owner that knows its invariant, lifetime, and failure behavior. Placement follows responsibility rather than the first caller that needed the behavior:

- high-level modules and orchestrators own intent, ordering, lifecycle, policy selection, and composition;
- capability owners own the cohesive algorithm, state machine, transform, validation, encoding, allocation, or retirement mechanism;
- backend-private owners implement native details without selecting product workflow;
- producers submit through the owner's contract and consumers read the owner's result; neither reimplements the owner's decision;
- genuinely shared behavior belongs to the lowest common semantic owner, not automatically to `Core`, `Common`, a utility file, or a global registry.

Audit both directions: start at each changed definition and find every use, then start at each changed use and find the definition that should own its behavior. Classify every material site as **authority**, **composition**, **producer**, **consumer**, or **duplicate**. Search exact symbols plus semantic equivalents, repeated switches, copied validation/error text, parallel caches/state, and rejected/old names. Build membership and dependency direction must agree with the claimed owner.

Apply the [single-truth and copy budget](DataOrientedDesign.md#single-truth-and-copy-budget) to representation placement. A value holder belongs at the real lifetime or publication boundary it serves; convenience mirrors and copied settings do not become owners merely because they have a type name.

The change fails placement review when callers repeat the same policy, transform, validation, state transition, or fallback; an orchestrator implements stage mechanics; a low-level capability selects unrelated workflows; two modules derive or mutate the same fact independently; a facade exists while callers still reach around it; or feature work must scatter edits across unrelated files. Move the behavior to its real owner, reduce callers to intent-level operations, and delete every duplicate production path in the same change.

Do not merge code merely because syntax looks similar. Different semantic owners, lifetimes, failure contracts, or cost models may justify local repetition; duplicated authority never does.

## Orchestration and Capabilities

An orchestrator owns ordering, lifecycle, policy selection, and composition. Its primary function should expose the workflow:

```text
Acquire input
Apply accepted structural changes
Build independent work
Commit deterministic results
Publish immutable output
```

Extract a nontrivial stage when a precise name makes the workflow clearer, hides a loop or state transition, isolates a reason to change, or enables direct testing of an invariant. One caller is enough; one line of forwarding without policy, ownership, or readability gain is not.

An orchestrator MUST NOT accumulate parsing, data transforms, cache insertion, task partition mechanics, backend-native branches, serialization, diagnostic formatting, UI widget internals, or another owner's allocation/retirement policy.

A capability implementation owns one cohesive operation, state machine, transform, policy, encoding, allocation, or lifetime and exposes only what its orchestrator needs.

### Mandatory Orchestrator/Implementor Boundary

An orchestration owner MUST remain generic over the behaviors it composes. When a workflow contains independently changing payload types, resource kinds, policies, backends, stages, or other behavioral variants, each variant's algorithm and invariant belong to a dedicated capability implementor. The orchestrator may select, order, invoke, and publish those capabilities; it MUST NOT implement their per-variant loops, transforms, validation, encoding, allocation, task-graph construction, or failure details.

Apply this boundary to the complete touched family, not only the newly added case. Adding or materially changing one variant requires auditing its sibling variants and extracting any mechanics still embedded in the orchestrator. A switch or repeated type-specific region in an orchestrator is a placement warning even when every branch is currently small.

The actual lifecycle/composition owner is the orchestrator. Do not add a generic `Manager`, `Builder`, facade, or base interface merely to forward calls to implementors; use the existing lifecycle owner when it already sequences the work. Implementors may share a concrete helper only when they enforce the same invariant and change together. Different lifetimes, inputs, outputs, failure contracts, or cost models require separate implementors even when their syntax looks similar.

At function level, orchestration and mechanism are separate responsibilities:

- an orchestration function reads as named stages, selects policy, sequences capabilities, handles stage-level failure, and publishes the result;
- an implementation function performs one cohesive algorithm, transform, state transition, backend operation, or lifetime action;
- a function does not alternate between high-level sequencing and the internal loops, parsing, allocation, synchronization, or encoding of several stages;
- a small owner may sequence private steps that enforce the same invariant and lifetime; extraction is required by independent responsibility, not by ceremony or line count.

An extracted collaborator must remove knowledge from its caller. Passing an owner facade into a one-method wrapper, moving an unchanged long function to another file, or replacing readable stages with indirection does not improve decomposition.

## Responsibility Audit

Line count locates review candidates but does not decide them. Audit a class, file, or function when it has:

- a responsibility sentence containing unrelated “and” clauses;
- multiple owners, lifetimes, or independently changing policies;
- orchestration mixed with mechanisms;
- deep nesting or long unnamed sequential regions;
- unrelated collections of state;
- cross-module knowledge callers should not possess;
- boolean-controlled alternate architectures;
- frequent edits from unrelated features;
- a public surface exposing private schedules, caches, backends, or mutable storage.

Do not answer a god unit with numbered parts, catch-all `Helpers`/`Common`/`Misc` buckets, one-method wrappers, speculative interfaces/factories/DI, arbitrary line limits, one function per file, or a second facade retaining the same authority.

Every class needs one clear sentence describing what it owns, which invariant it enforces, its lifetime, and why it changes. Prefer one authoritative mutable owner, immutable value types, cohesive operation types, narrow domain services, RAII, and composition. Use inheritance only for a real substitutable contract whose implementations preserve the same behavioral and lifetime invariants.

### File and Folder Cohesion

- A source file owns one primary type, cohesive operation, or tightly coupled private collaboration with one reason to change.
- A folder owns one durable subsystem, capability, lifetime, backend, or visibility boundary. Its name must let a new engineer predict what belongs there and where a responsibility lives.
- Audit a file when unrelated features repeatedly edit it, its private declarations serve different owners, or its name no longer describes most of its contents.
- Audit a folder when it mixes independent owners, acts as a dependency shortcut, requires vague names or filename prefixes to recover missing hierarchy, or has no sentence that excludes unrelated additions.
- New catch-all `Helpers`, `Common`, `Misc`, `Utilities`, `Managers`, or synonym folders are forbidden. Existing catch-all areas are debt: do not add another unrelated responsibility; move touched behavior to its real owner when the extraction is bounded and validated.
- Do not replace a god folder with deep one-file nesting or a directory per class. Split only on a real ownership or change boundary and update build membership, includes, tests, and documentation atomically.

### Change Locality and Engine-Scale Growth

An engine scales when a capability can evolve, validate, and retire inside a predictable ownership path. It does not scale merely because it has more abstraction layers.

- Adding a capability should primarily change its owning subsystem, explicit composition point, tests, and documentation—not unrelated coordinators, generic utilities, or backend internals.
- Backend and policy variants plug into an existing owner contract or a deliberately closed switch; they do not spread feature checks across callers.
- A registry owns discovery for one named product workflow. It is not a service locator or a universal place every feature must edit.
- Repeated edits across unrelated owners for each new feature signal a missing stable contract, misplaced authority, or overly central orchestrator and require responsibility review.
- Removing a capability should have a bounded deletion path with no residual global flags, registrations, compatibility aliases, or empty framework layers.

Review change fan-out and dependency direction, not only local class quality. A locally elegant type that forces repository-wide knowledge is not modular.

## SOLID Without Ceremony

### Single Responsibility

- A class, file, and function owns one coherent reason to change.
- An orchestrator sequences capabilities rather than reimplementing them.
- A data type represents one lifetime and consumption contract.
- Validation belongs to the narrow owner of the invariant.
- Backend-specific behavior remains behind the backend boundary.

### Open/Closed and Substitution

- Prefer stable inputs, outputs, handles, descriptors, and policies only where current implementations share a real contract.
- A closed product enum switch can be clearer than speculative polymorphism.
- An owned class or struct has at most one direct base; compose additional behavior or state instead of using multiple inheritance.
- Derived implementations MUST preserve ownership, error, lifetime, and thread-affinity contracts.
- If callers need type/backend checks to use an abstraction, the abstraction is dishonest.

### Interface Segregation and Dependency Inversion

- Cross-module APIs expose the smallest capability each consumer needs.
- Workers receive immutable input and exclusive output ranges, not an owner facade.
- Panels receive models and semantic commands, not world or renderer pointers.
- High-level policy depends on stable product contracts rather than volatile task/cache/backend mechanisms.
- PImpl is justified only when it hides meaningful lifetime or mechanism volatility.

## DRY and Authority

DRY applies first to truth, policy, and behavior, not similar-looking syntax.

MUST:

- keep one authoritative mutable source;
- keep one canonical identity and stale-handle rule per domain;
- keep one scheduler, frame-graph authority, queue-submit owner, editor-operation runtime, and content-publication path;
- consolidate repeated transforms or validation owned by the same domain;
- delete compatibility spelling and duplicate paths atomically with their replacement;
- derive read models, render projections, caches, and GPU tables in one direction from authority.

Do not unify algorithms with different lifetimes or consumers, hide concrete transforms behind premature generic containers, retain dual mutable representations, or branch one function through full legacy/new or serial/parallel architectures. Local repetition is preferable to a misleading abstraction; duplicated authority is not.

## Public-Surface Budget

A public type needs at least two real owned module consumers or one necessary stable host/module boundary.

Public contracts MAY expose immutable requests/results, stable IDs and typed handles, descriptors, packets/read models, explicit cancellation/lifetime operations, and narrow capability interfaces.

They MUST NOT expose worker records, queues or dependency counters; private ECS storage; mutable renderer caches or device escape hatches; backend-native handles outside an explicit interop contract; compiler sessions; unrelated task execution context; unenforceable mutable spans; or diagnostics snapshots without a current product consumer.

High-level callers use narrow domain verbs such as start, submit, cancel, consume, apply, commit, publish, and release. The owner of independent work owns partitioning, task-local state, lane, cancellation, progress coalescing, deterministic merge, result publication, and shutdown settlement.

## Lifetime Expression

Use:

- values for immutable requests and results;
- `unique_ptr` for sole heap ownership;
- `shared_ptr` only for genuine shared lifetime;
- references for required bounded non-owning access;
- pointers for optional/external access with explicit lifetime;
- handles, tokens, and leases for cross-boundary identity or authority;
- scopes for asynchronous settlement;
- arenas only with a named owner, seal/publication point, and reset condition.

Never hide lifetime in raw-owner callback captures, service locators, global registries, durable spans or indices, or a “valid” boolean that conflates CPU reuse and GPU completion.

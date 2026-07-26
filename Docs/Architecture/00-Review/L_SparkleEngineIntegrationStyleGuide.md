# L. SparkleEngine Integration Style Guide

Status: binding integration and review contract
Applies to: owned code in `Engine`, `Tools`, `Projects`, build files, shaders, tests, and directly related documentation
Last consolidated: 2026-07-26

## Purpose

This guide consolidates SparkleEngine's existing architectural rules, implementation-prompt rules, engineering persona, repository structure, and established cleanup decisions into one attachment-ready contract. It is not a second architecture plan and does not replace the detailed reasoning, source research, prompt dependencies, or hazard atlas in J and K.

It also makes the supplied principal graphics engineering expectations executable through the canonical `PGE-01` through `PGE-15` matrix in A and the persona interpretation in H. The target raises the final bar to include partner technology adoption, path tracing, a real neural graphics feature, model/training/inference optimization, mathematics, CPU/GPU architecture, hardware/driver diagnosis, verified AI-tool use, and principal-quality communication.

The intended result is a compact renderer-first engine whose code is:

- easy to navigate from module to subsystem to capability;
- easy to reason about from owner to lifetime to data flow;
- explicit about CPU, GPU, task, and frame-graph concurrency;
- data-oriented where measured access patterns justify it;
- direct and product-focused instead of framework-heavy;
- maintainable because each authority, class, file, and function has a clear reason to change;
- faster because work, data movement, allocation, synchronization, and GPU cost are designed deliberately;
- honest about feature support, backend parity, measurements, and limitations.

The target engineering persona is defined in [H. Advanced Graphics Engineer Persona](H_AdvancedGraphicsEngineerPersona.md): technically deep, measurable when needed, clean enough for other engineers to extend, and capable of translating research ambiguity into lean product-quality implementation.

The canonical product workload is defined in [I. Bistro and San Miguel Acceptance Workloads](I_BistroAcceptanceWorkload.md): Sponza is the rapid regression tier, Bistro is the primary Tier 1 workload, and San Miguel is the supported Tier 1 secondary workload. Relevant changes must advance or preserve those gates without scene-specific engine branches.

## How to Attach This Guide to Future Prompts

Add this block to each implementation prompt:

> Apply `Docs/Architecture/00-Review/L_SparkleEngineIntegrationStyleGuide.md` in full. Treat its ownership, implementation-shape, DOD, concurrency, rendering, naming, validation, structure, principal graphics engineering, and completion-report gates as acceptance criteria. Before editing, list the applicable `PGE-01` through `PGE-15` requirements from A, interpret them through H, and apply the Sponza/Bistro/San Miguel workload contract from I when the touched behavior affects it; classify each requirement and workload gate as advance, preserve, not applicable, or blocked with expected evidence. Inspect the current repository, reconcile the whole touched ownership path, verify any AI-assisted work independently, delete replaced paths in the same change, reject role-only or scene-specific scaffolding, and report any justified exception explicitly with its owner, scope, evidence, and deletion or review gate.

Referencing the guide is sufficient; do not copy its rules into another policy document. A prompt may impose stricter requirements. It may not silently weaken ownership, correctness, feature-preservation, deterministic behavior, backend parity, `PGE-*` preservation, or evidence requirements.

## Source Authority and Interpretation

This guide is grounded in:

- [A. Principal Rendering Requirements](A_PrincipalRenderingRequirements.md)
- [D. Whole Repository Architecture Map](D_WholeRepositoryArchitectureMap.md)
- [E. External Renderer Repository Comparison](E_ExternalRendererRepositoryComparison.md)
- [G. Advanced Graphics Engine Executive Summary](G_AdvancedGraphicsEngineExecutiveSummary.md)
- [H. Advanced Graphics Engineer Persona](H_AdvancedGraphicsEngineerPersona.md)
- [I. Bistro and San Miguel Acceptance Workloads](I_BistroAcceptanceWorkload.md)
- [J. Multithreaded Engine Architecture and Learning Program](J_MultithreadedEngineArchitectureAndLearningProgram.md)
- [K. Multithreaded Engine Implementation Prompt Series](K_MultithreadedEngineImplementationPromptSeries.md)
- the enforced repository configuration in [.clang-format](../../../.clang-format), [.clang-tidy](../../../.clang-tidy), module `CMakeLists.txt` files, and [ArchitectureBoundaryCheck.cmake](../../../CMake/ArchitectureBoundaryCheck.cmake)
- the current code's strongest ownership and decomposition patterns.

Use `MUST`, `SHOULD`, and `MAY` in their ordinary normative sense:

- **MUST** is an acceptance requirement.
- **SHOULD** is the default; deviation needs a concrete repository or measured reason.
- **MAY** is optional and must still respect ownership, dependency, and complexity budgets.

When sources appear to conflict:

1. preserve correctness, ownership, lifetime, and current product behavior;
2. obey the current task's explicit accepted requirements;
3. obey this guide and K's non-negotiable rules;
4. use J for canonical architecture, concurrency, DOD, renderer, and evidence reasoning;
5. treat CMake and executable boundary checks as the authority for actual module dependencies;
6. treat existing code as precedent only when it already satisfies these rules.

Age and prevalence do not make a weak pattern canonical. A touched legacy path must be classified and narrowed, migrated, or given an exact bounded disposition.

### Repository Characteristics Used to Calibrate This Guide

The current repository was inspected across Core, Platform, Tasks, GameFramework, Renderer, RHI, Editor, Application, shader/compiler tools, cookers, launcher code, CMake, and the architecture documents. The strongest existing patterns are:

- module targets with explicit `Public` and `Private` ownership and CMake-controlled dependencies;
- Tasks split by graph, execution, scheduling, lifetime, and profiling responsibility;
- GameFramework world work split among private ECS storage, system descriptors/execution, resources, publication, and structural/dynamic extraction;
- Renderer frame orchestration separated from lighting, ray tracing, presentation, post-processing, frame-graph, scene-data, and backend-neutral host capabilities;
- backend-native ownership isolated under D3D12/Vulkan RHI implementation;
- substantive private integration capabilities, such as editor UI frame rendering, placed in dedicated files rather than embedded in host orchestration;
- a repository-wide executable rule that owned source contains no anonymous namespaces.

Large files and functions still exist and remain audit candidates. Their existence is not an exception to this guide. Conversely, the presence of small files is not proof of cohesion; each boundary still needs an owner, invariant, and independently changing responsibility.

## 1. Product and Engineering North Star

SparkleEngine is one coherent product, not a collection of reusable framework demonstrations.

Every integration MUST:

- deliver or preserve a real runtime, editor, renderer, RHI, cooking, launcher, capture, or debugging workflow;
- use the existing engine architecture instead of creating a parallel subsystem;
- prefer a complete vertical slice over a broad unfinished framework;
- keep public APIs smaller than their private implementations;
- make the default workflow clean and intentional;
- delete the path, authority, adapter, flag, or representation it replaces;
- distinguish product, developer-preview, research, unsupported, and deleted states honestly;
- identify applicable `PGE-*` requirements and preserve their future integration seams without adding empty role-shaped infrastructure;
- preserve D3D12 and Vulkan behavior where supported;
- preserve raster, classic TLAS, PTLAS, reservoir lighting, temporal/provider behavior, path/reference modes, shader packaging, capture, and owned tools when the change crosses those paths;
- improve the touched ownership path rather than append another layer to it.

The completed repository target additionally MUST include:

- one path-traced workload understood from mathematics through CPU/GPU/driver behavior;
- one real replacement-based neural graphics feature with a real model/operator path, deterministic artifact, classical fallback and quality/performance evidence;
- separate training/offline preparation and runtime inference ownership;
- one partner-shaped adoption case and one reduced hardware/driver investigation;
- one code-backed live demo, whitepaper-quality technical note and conference-talk outline;
- truthful boundaries around credentials, professional history, Linux support and unavailable hardware.

The desired change is usually **additive in capability and reductive in structure**.

Do not add:

- a framework for hypothetical consumers;
- a public diagnostics or reporting product to prove one change;
- a second scheduler, pool, scene schema, asset database, cache family, renderer path, or command vocabulary;
- sample-style architecture that does not fit Sparkle's product;
- branding or abstraction whose name promises more than its evidence.

## 2. Required Workflow Before Editing

Implementation begins with inspection, not type creation.

### 2.1 Search for the Existing Responsibility

Search the whole owned repository with `rg` and `rg --files` before adding or renaming a concept. Search:

- the proposed exact name;
- canonical vocabulary;
- rejected aliases;
- semantic counterparts with different names;
- producer and consumer operations;
- base classes, callbacks, services, registries, queues, caches, packets, handles, and result types;
- filenames, directories, CMake membership, tests, comments, profiler labels, and thread labels.

For each apparent counterpart, decide explicitly:

- **use** it unchanged;
- **extend** its existing ownership;
- **refactor** it into the required responsibility;
- **replace and delete** it;
- **add** a new capability because no owner exists.

Do not escape integration by adding `New*`, `*2`, `Async*`, `ThreadSafe*`, `MultiThreaded*`, `MT*`, a compatibility namespace, or a parallel directory tree.

### 2.2 Map the Touched Ownership Path

Before editing, record for every touched or proposed file:

- CMake target and module;
- subsystem and capability;
- `Public` or `Private`;
- primary type or operation;
- mutable-state owner;
- lifetime owner;
- direct consumers;
- reason to change;
- expected final location;
- nearby files with the same ownership defect.

Inspect the whole direct ownership path, not only files named in the prompt. This is bounded structural reconciliation, not permission for unrelated cosmetic cleanup.

### 2.3 Build Four Small Ledgers

Every material change MUST start with:

1. **Preservation ledger** - current product behavior and backend/feature paths that must remain.
2. **Deletion ledger** - old types, functions, access paths, adapters, flags, scans, allocations, waits, and representations that the new path removes.
3. **Data/access inventory** - required by Section 10.
4. **Concurrency/hazard inventory** - every touched thread, task, queue, wait, lock, atomic, callback, allocator, publication, and GPU lifetime edge.

These may live in the change description or existing implementation document. Do not create a permanent runtime registry or another policy/report format.

### 2.4 Define the Serial and Failure Contracts

Before parallelism or pipelining:

1. define ownership and lifetime;
2. define input, output, identity, and deterministic ordering;
3. implement or preserve one serial reference path using the final data contract;
4. prove state, bytes, image, ordering, and feature parity as applicable;
5. identify the smallest independently writable range;
6. add a serial threshold and then enable worker execution.

Also state the failure policy:

- reject;
- retain previous accepted state;
- cancel and settle;
- request full resynchronization;
- use a bounded fallback;
- fail the operation transactionally.

Partial publication and ambiguous ownership are not failure policies.

## 3. Module and Dependency Ownership

CMake targets and actual link/include direction are authoritative. Folder names clarify ownership but cannot legalize a dependency.

### 3.1 Module Responsibilities

| Module | Responsibility | Must not own |
|---|---|---|
| `Core` | low-level values, diagnostics bootstrap, process/string/time/math utilities, portable ownership primitives | renderer, editor, game, backend, or feature policy |
| `Platform` | window, input backend, OS integration | renderer scheduling or game-world policy |
| `Tasks` | product-independent task DAG, execution, scopes, lanes, cancellation, worker runtime | renderer, RHI, ECS, editor, provider, or cooking policy |
| `GameFramework` | runtime world, private ECS, systems, immutable resources, loading commit, world publication and extraction | renderer/RHI mutation or editor UI |
| `RHI` | explicit backend-neutral GPU contracts and backend-private D3D12/Vulkan implementation | frame-graph policy, game types, renderer shader-data policy |
| `Renderer` | render world, frame preparation, frame graph, passes, providers, render coordination, GPU-scene policy | ECS queries, `GameWorld` dereference, editor state, native backend APIs outside narrow bridges |
| `Editor` | ImGui presentation, panels, immutable editor models, semantic UI requests | live ECS storage, renderer cache ownership, task scheduler internals |
| `Application` | runtime/editor host orchestration, lifecycle, subsystem composition, boundary publication | subsystem mechanisms that have their own owner |
| `Tools` | owned import, cook, compiler, launcher, packaging, and validation workflows | runtime scene/render authority or duplicate task runtime |

For a selected neural graphics feature:

- Renderer owns feature policy, inference passes, persistent runtime model resources, classical fallback and quality/performance placement.
- RHI owns only explicit resources, pipelines, synchronization, capabilities and backend implementation; it does not know model semantics.
- Shader/compiler/cooking tools own deterministic source/model artifact transformation and validation.
- Isolated offline tools may own training or fine-tuning; runtime, editor and shipping packages do not link the training stack.
- Editor exposes only a current product setting/comparison/result through immutable commands/products; it does not become a training or model-graph application.

### 3.2 Dependency Rules

- `Core` remains the bottom dependency.
- `Tasks` depends only on its minimal low-level requirements.
- `GameFramework` MUST NOT depend on Renderer or RHI.
- RHI MUST NOT depend on Renderer.
- Renderer consumes stable render contracts and public RHI capability; it MUST NOT consume private ECS storage or dereference `GameWorld`.
- D3D12/Vulkan native types remain in backend-private RHI or a deliberately narrow external-provider bridge.
- Editor and Application integrate through narrow public contracts; they do not become escape hatches for private storage.
- A public dependency is not justified merely because one implementation currently includes a type.
- Transitive include convenience is not an ownership argument.

When changing a dependency:

- update CMake, includes, exports, source groups, PCH use, tests, and boundary checks atomically;
- inspect the transitive public dependency surface;
- remove obsolete compatibility includes;
- run the architecture boundary gate.

## 4. Hierarchical Subsystem Design

Repository structure SHOULD read as:

```text
Module
  Public
    stable cross-module contracts
  Private
    Subsystem
      Orchestration
      Capability or lifetime-specific implementation
      Backend or policy variants where real
```

The hierarchy communicates ownership, lifetime, backend separation, or an independently changing capability. It is not a mandate to maximize directory count.

### 4.1 Orchestration and Capability Implementation

An **orchestrator** owns ordering, lifecycle, policy selection, and composition. Its main functions SHOULD read like the product workflow:

```text
Acquire input
Apply accepted structural changes
Build independent work
Commit deterministic results
Publish immutable output
```

The top-level workflow MUST remain visible without reading its mechanisms. A reviewer should be able to understand the order and policy by reading the orchestration function alone, then navigate to a named function or capability only when implementation detail is needed.

When an orchestration function contains more than one logical stage, extract each nontrivial stage into a responsibility-bearing member or capability function. This applies even when the extracted function has one caller. Typical stage boundaries include:

- resolve or acquire inputs;
- derive capacity or policy;
- build topology;
- wire prerequisites;
- execute work;
- merge or commit results;
- publish or retire state.

Do not combine topology capacity selection, node construction, dependency wiring, execution, and state publication in one function. Do not require a reader to scan task descriptors, loops, allocation mechanics, or backend branches to discover the high-level workflow.

An orchestrator MUST NOT accumulate:

- parsing/decoding algorithms;
- data transformation details;
- cache insertion mechanics;
- task partition implementation;
- backend-native branches;
- serialization formats;
- diagnostic formatting;
- UI widget internals;
- resource allocation and retirement policy belonging to another owner.

A **capability implementation** owns one cohesive operation, state machine, data transform, policy, encoding, allocation, or lifetime. It exposes the narrow operation the orchestrator needs.

Extract a behavior into a named function even when it has one caller when doing so:

- gives the behavior a precise name;
- makes the caller read as an ordered workflow;
- hides a nontrivial loop, transform, state transition, validation, or backend detail;
- isolates a distinct reason to change;
- enables direct testing of a meaningful invariant.

One use is sufficient. One line of forwarding with no policy, invariant, ownership, or readability gain is not.

### 4.2 God-Class and God-Function Audit

Line count is a locator, not a verdict. Audit a class, file, or function when it has:

- a responsibility description containing "and";
- multiple owners or lifetimes;
- more than one independently changing policy;
- orchestration mixed with implementation mechanics;
- repeated deep nesting or long sequential regions with unnamed intent;
- unrelated collections of state;
- cross-module knowledge that callers should not possess;
- boolean-controlled alternate architectures;
- frequent edits from unrelated features;
- a public surface exposing private scheduling, caches, backend details, or mutable storage.

For each suspect unit, list its reasons to change. Separate lifecycle/orchestration, transform, scheduling, backend policy, persistence, and instrumentation only when they can evolve independently.

Do not "solve" a god file with:

- `Part1`/`Part2`;
- `Helpers`, `Common`, `Misc`, `Util`, `Detail`, or `Internal` catch-all files or namespaces;
- many one-method wrapper classes;
- an interface/factory/DI hierarchy with no real substitution seam;
- arbitrary maximum line counts;
- one function per file;
- a second facade that retains the same broad authority.

### 4.3 Class Responsibility

Every class MUST have one clear sentence describing:

- what it owns;
- what invariant it enforces;
- when it is created and destroyed;
- why it changes.

If that sentence requires unrelated clauses, split the responsibility.

Prefer:

- one authoritative owner for mutable state;
- value types for immutable data;
- explicit operation types for cohesive algorithms;
- narrow services for domain operations;
- RAII for lifetime;
- composition over inheritance.

Use inheritance only when callers require a real substitutable contract and the derived types obey the same lifetime and behavioral invariants. Do not introduce an interface solely to reduce include count or make a test mock possible.

## 5. SOLID Applied Without Ceremony

SOLID is an ownership and reasoning discipline, not a class-count target.

### 5.1 Single Responsibility

- One class, file, and function owns one coherent reason to change.
- An orchestrator sequences capabilities; it does not reimplement them.
- A data type represents one lifetime/consumption contract.
- Validation belongs to the narrow owner of the invariant.
- Backend-specific behavior stays behind the backend boundary.

### 5.2 Open/Closed

- Prefer stable inputs, outputs, handles, descriptors, and policies where a second current implementation exists.
- Do not predict extension points.
- A switch over a closed product enum can be clearer than speculative polymorphism.
- Add a new strategy abstraction only when real implementations share the same owner, lifetime, and contract.

### 5.3 Liskov Substitution

- Derived implementations MUST preserve ownership, error, lifetime, and thread-affinity contracts.
- A caller MUST NOT need backend/type checks to use a purported abstraction.
- If implementations cannot share the same contract honestly, keep distinct types.

### 5.4 Interface Segregation

- Cross-module APIs expose the smallest capability the consumer needs.
- Split broad "device," "world," "renderer," "manager," or "services" access when consumers use unrelated subsets.
- Workers receive immutable input and exclusive output ranges, not an owner facade.
- Panels receive models and semantic commands, not a world or renderer pointer.

### 5.5 Dependency Inversion

- High-level policy depends on stable product contracts, not concrete backend/task/cache mechanisms.
- Volatile implementation remains private.
- PImpl is justified when it hides meaningful lifetime or mechanism volatility; it is not justified as an empty forwarding layer.

## 6. DRY and Authority

DRY applies primarily to truth, policy, and behavior, not to superficial syntax.

MUST:

- keep one authoritative mutable source;
- keep one canonical identity and stale-handle rule per domain;
- keep one scheduler, one frame-graph authority, one queue-submit owner, one editor operation runtime, and one content publication path;
- consolidate repeated transforms or validation owned by the same domain;
- delete compatibility spelling and duplicate paths after migration;
- derive read models, render projections, caches, and GPU tables one way from their authority.

Do not:

- unify coincidentally similar algorithms with different lifetimes or consumers;
- introduce generic containers/templates that hide the concrete data transform;
- retain two representations so old and new callers both mutate state;
- branch one function through large legacy/new or serial/parallel implementations;
- duplicate a check at every call site instead of enforcing it at the owner.

A little local repetition is preferable to a misleading shared abstraction. Duplicated authority is never acceptable.

## 7. Files, Folders, Headers, and Sources

### 7.1 File Placement

- Public files contain stable contracts required across module boundaries.
- Private files contain mechanisms, worker records, caches, graph compilation, backend policy, editor models, and implementation types.
- A filename matches its primary public type or its cohesive operation.
- A `.cpp` matches the type or operation it implements.
- A substantive class receives a dedicated file pair when it has its own responsibility, state, lifecycle, or likely independent evolution.
- Small private records tightly coupled to one owner MAY co-locate with that owner.
- A one-file directory is allowed only when it marks a real growth or ownership boundary.
- Do not create synonym folders for an existing subsystem.

Examples of useful boundaries include:

- `Frame/Core` orchestration versus `Frame/Lighting`, `Frame/PostProcessing`, or `Frame/Presentation` capabilities;
- world `Systems/Descriptors` versus `Systems/Execution`;
- extraction `Identity`, `Structural`, `Dynamic`, and `Resources`;
- task `Graph`, `Execution`, `Scheduling`, `Lifetime`, and `Profiling`;
- RHI `D3D12` and `Vulkan` backend-private ownership.

### 7.2 Headers Are Declaration Surfaces

Owned headers MUST contain declarations and the minimum collaboration contract.

Only these function bodies are allowed in headers:

- templates;
- trivial direct getters;
- trivial direct setters;
- trivial direct accessors.

Move all other definitions to the matching `.cpp`, including:

- constructors and destructors;
- algorithms and transforms;
- parsing and formatting;
- validation;
- orchestration;
- factories;
- non-template operators;
- nontrivial `constexpr` functions;
- convenience functions that perform policy or branching.

Deleted special-member declarations are allowed because they declare the type contract.

Do not use inline implementation to avoid creating the correct source file. Header cleanliness, compile isolation, and visible ownership are more important than saving one `.cpp`.

### 7.3 No Function-Local Types

Do not define a `class`, `struct`, or enum-like implementation record inside a function.

Place the type:

- as a private nested declaration on its owner and define it in the `.cpp`;
- as a private implementation type at source scope;
- in a private header when multiple implementation units genuinely collaborate through it;
- in an established named domain namespace only when it is a real domain concept.

This makes identity, ownership, testability, lifetime, and file placement visible.

### 7.4 No Anonymous or Invented Ownership Namespaces

Anonymous namespaces are forbidden in all owned source. The repository-wide expected count is zero.

Do not evade the rule by introducing arbitrary namespaces named:

- `Detail`;
- `Internal`;
- `Private`;
- `Local`;
- `Implementation`;
- `Helpers`.

Behavior that supports one owner becomes:

- an owner member;
- a private static member;
- a cohesive source-local implementation class or struct with a responsibility-bearing name.

Free functions belong only in an existing, genuine domain namespace whose name is already part of the repository's responsibility vocabulary, such as a focused string, pixel, process, or application-command domain. A namespace is not a substitute for deciding ownership.

### 7.5 Includes and Compile Boundaries

- Include what the file uses.
- Prefer forward declarations in headers when ownership and complete-type requirements allow.
- Do not depend on transitive includes.
- Keep PCH assumptions private to the configured module.
- Public headers MUST NOT include private module headers.
- Backend-native headers stay in backend-private implementation.
- Remove dead includes after a move or refactor.
- Preserve the include grouping and ordering configured by the repository; `.clang-format` does not sort includes automatically.

## 8. C++ Language and Formatting

The repository's `.clang-format` and `.clang-tidy` are binding. Do not create a prose convention that conflicts with the executable configuration.

Current baseline:

- C++20 for owned engine targets;
- Allman braces;
- tabs for indentation, width four;
- 140-column limit;
- no single-line control-flow bodies;
- left-bound pointers and references;
- explicit, diff-friendly wrapping;
- warnings-as-errors for configured clang-tidy checks;
- RAII and modern C++ ownership;
- `nullptr`, scoped enums, explicit constructors, and strong types where meaningful.

Naming baseline:

- types, functions, and methods: `UpperCamelCase`;
- local variables and parameters: `lowerCamelCase`;
- private data members: `m_` plus descriptive lower-camel name;
- boolean names describe a state or predicate and follow the configured boolean naming checks;
- enum values: `UpperCamelCase`;
- macros: reserved for build/export/preprocessor needs and named consistently with existing module macros;
- acronym casing follows canonical Sparkle vocabulary: `Rhi`, `Gpu`, and `Io` in C++ type names unless an external ABI requires exact spelling.

Public aggregate and serialized field spelling MUST follow the owning contract consistently. Do not rename serialized or shader ABI fields cosmetically. New engine packet/descriptor families choose one established domain convention and use it throughout.

Additional rules:

- Use blank lines to expose logical stages inside a function. Keep consecutive initialization or mutation of one record together; place one blank line before the next distinct operation, loop, branch, commit, or publication stage.
- Do not turn a function into an undifferentiated wall of statements. Whitespace is part of the ownership narrative: acquire, transform, commit, and publish should be visually distinct.
- Do not insert arbitrary blank lines inside one cohesive initialization sequence or split a condition from the operation it governs.
- Keep a declaration, call, assignment, return type, and function signature on one line when it fits comfortably within the configured 140-column limit and remains readable.
- Wrap because the expression is genuinely long or its elements benefit from one-per-line review, not merely because a shorter visual column was used elsewhere.
- Once a call or aggregate must wrap, group its arguments and fields by meaning and avoid stair-step fragmentation of simple member access, casts, names, and ternaries.
- Formatting is not a substitute for decomposition. If compact formatting still leaves several independently meaningful stages in one function, extract named behavior.
- During every touched-file review, inspect the complete modified file for dense unnamed sequential regions and mixed orchestration/mechanism. Improve the bounded file while preserving behavior; record any neighboring god unit that cannot safely move in the current change.
- use `final` when a concrete class is not designed for inheritance;
- delete unsupported copy/move operations explicitly where ownership requires it;
- use `noexcept` where the operation's real contract supports it, not as decoration;
- prefer `enum class` and domain value types over boolean mode combinations;
- avoid implicit ownership through raw pointers;
- a non-owning pointer/reference is acceptable only when lifetime and nullability are obvious and bounded by the caller;
- avoid macros, reflection, type erasure, or template metaprogramming when direct typed code is clearer;
- use designated initialization where it improves contract readability and matches the type;
- make units, coordinate spaces, frame domains, and queue domains explicit in names or types.

## 9. Naming and Vocabulary

Use one responsibility per noun and one canonical term per responsibility.

### 9.1 Canonical Concurrency and Rendering Terms

Use the vocabulary defined in J and K:

- `Task`, `TaskDesc`, `TaskNodeHandle`
- `CompiledTaskGraph`
- `TaskExecution`, `TaskExecutionContext`, `TaskExecutor`
- `TaskScope`, `TaskEvent`, `ParallelFor`, `TaskLane`
- `GameThread`, `EditorThread`, `RenderThread`
- `RenderCoordinator`
- `RenderFrameQueue`
- `RenderControlCommandQueue`
- `RenderCommandContext`
- backend-private `D3D12CommandRecordingContext` and `VulkanCommandRecordingContext`
- `RhiCommandRecordingLease`
- `ERhiQueueType`, `RhiSubmissionToken`
- `FrameGraphSubmissionBatch`, `RecordingGroup`

Meaning suffixes:

- `Handle` identifies something.
- `Token` proves ordering, completion, or authority.
- `Lease` is a temporary exclusive borrow.
- `Context` is transient call/run state, not a service locator.
- `Scope` binds work lifetime and settlement.
- `Lane` is scheduling policy, not correctness.
- `Graph` is reusable topology.
- `Execution` is one run.
- `Desc` is immutable/configuration description.
- queue names state the payload they carry.

Use distinct verbs:

- `Build` or `Compile` creates a plan or immutable product;
- `Record` writes commands;
- `Submit` transfers commands/work to an executor or GPU queue;
- `Execute` performs a task/plan;
- `Publish` makes immutable state visible;
- `Commit` mutates authoritative owner state at a boundary;
- `Apply` consumes an accepted command/delta into owned state;
- `Extract` transforms authoritative data into a derived boundary representation;
- `Retire` delays reuse/destruction until its completion condition.

Avoid vague owners such as `Manager`, `System`, or `Services` unless the name states the owned domain and the type truly coordinates that domain. Prefer names that reveal the capability: evaluator, sampler, builder, compiler, planner, allocator, registry, publisher, committer, coordinator, encoder, decoder, or store.

### 9.2 Identity Vocabulary

Do not call every identity a GUID.

- `Guid` is a 128-bit persistent value primitive, not a global manager.
- `AssetGuid`, `AuthoredInstanceGuid`, and `AuthoredObjectGuid` are persistent authoring identities owned by their metadata/document domains.
- `SourceInstanceId`-style derived hashes are migration or lookup keys, not GUIDs.
- `EntityId` is compact runtime ECS identity with stale-handle generation.
- `RenderObjectId` is separate renderer identity created by extraction.
- resource handles identify runtime immutable assets.
- content hashes identify bytes/options for caching and verification, not authored objects.

Persistent GUIDs are resolved at load/editor/reference boundaries into runtime handles or IDs. Do not perform GUID-map joins in frame-critical ECS, extraction, renderer traversal, or GPU upload loops. Do not add a global GUID registry.

### 9.3 Rejected Naming Patterns

Do not introduce:

- `New*`, `*2`, `Legacy*` without a short explicit deletion gate;
- `Async*`, `ThreadSafe*`, `LockFree*`, `MultiThreaded*`, or `MT*` as vague implementation claims;
- `SceneAnimation*` for sampling, pose, morph, skinning, or output storage;
- `Data`, `Info`, `Thing`, `Object`, `Manager`, `Helper`, or `Util` when a more precise responsibility exists;
- different names for CPU task queues, render control commands, RHI command lists, and GPU queues;
- `Snapshot` for arbitrary broad mutable-world copies; use a deliberate packet, read model, immutable state, or diagnostics product only where that is the real contract.

Temporary compatibility names are allowed only inside an active bounded migration and MUST be deleted before its acceptance gate unless the prompt explicitly assigns a later deletion owner.

## 10. Evidence-Grounded Data-Oriented Design

Data-oriented design starts from the work and access pattern, not from choosing ECS, SoA, or templates.

### 10.1 Mandatory Data/Access Inventory

For every material source, transform, stream, packet, cache, table, upload, and hot traversal touched, record:

- producer;
- consumers;
- authoritative or derived ownership;
- data type and semantic fields;
- frequency;
- cardinality and expected high-water mark;
- shape and distribution;
- mutation points;
- publication and reclamation lifetime;
- access order and fields read together;
- stable key;
- CPU/GPU destination;
- bandwidth and latency sensitivity;
- allocation behavior;
- deterministic transform and ordering;
- current measured cost or concrete falsifier.

If these facts are unknown, measure or inspect them before choosing a layout.

### 10.2 Authority and Projection

- There is one authoritative mutable source.
- Read models, render worlds, caches, packets, and GPU tables are derived projections.
- Projection direction is one-way.
- Mutation never flows backward from renderer/editor/cache into the authority.
- Incremental projections use explicit sequence/generation and a full-resync fallback where lag is possible.
- Regenerated content always publishes the newest supported representation.
- Do not add content-version compatibility layers or keep legacy representations alive by default.
- A generation used for stale-handle/lifetime rejection is not a content-compatibility version.

### 10.3 Layout Selection

Choose AoS, SoA, AoSoA, sparse set, archetype chunk, indexed table, flat stream, packed record, or pointer-owning object only from actual access evidence.

- Split hot from cold data when consumers and frequency differ.
- Split structural from per-frame dynamic data.
- Split authoring from runtime data.
- Split CPU policy from GPU-facing packed representation.
- Keep fields together when consumers always read them together.
- Structural operations often benefit from compact typed AoS records.
- Dense frame work may benefit from columns or cohesive AoSoA blocks.
- Sparse-set storage is useful for packed component iteration and stable entity lookup; it is not a universal storage answer.
- Do not introduce archetypes/chunks until current workloads prove that sparse sets or indexed tables lose.
- Record the accepted layout, rejected alternatives, and the measurement that can overturn the choice.

### 10.4 Stable Identity and References

Across owner, frame, task, editor, load, or render boundaries:

- use generational IDs or immutable typed handles;
- never retain vector indices as identity;
- never retain pointers, iterators, spans, component views, dense indices, or arena addresses beyond their documented epoch;
- map persistent authoring GUIDs to compact runtime IDs once;
- keep `EntityId` and `RenderObjectId` distinct;
- key temporal, render, and retirement state by stable identity, not current row order.

### 10.5 Variable-Length and Hot Records

Variable-length packet/GPU data uses bounded flat arrays plus offsets/counts.

Hot records MUST NOT contain:

- owning vectors or strings;
- callbacks;
- mutexes or atomics without a measured per-record protocol;
- allocators;
- service pointers;
- heavyweight immutable assets;
- editor metadata that the hot loop does not consume.

Preallocate stable output slots or bounded arenas when cardinality is known. Reuse per-execution or per-frame storage only with explicit reset and lifetime rules.

### 10.6 Parallel Data Transforms

- Partition a leading dense component/query/input range.
- Each task writes a private output or exclusive preassigned range.
- Do not push into a shared vector, bump one shared frame-hot cursor, mutate an asset, or update a shared cache per item.
- Use task-local pages, arenas, command buffers, reductions, buckets, or fixed slices.
- Merge by stable `(phase, system, entity/object, partition, local sequence)` or another documented key.
- Task completion order is never semantic.
- Structural changes occur at the owner commit after readers join.
- Views and ranges cannot escape the execution epoch.

### 10.7 Memory Efficiency

For any concurrency or buffering design, state the memory equation:

```text
workers * frames in flight * queues * per-worker/per-frame capacity
```

Include relevant:

- packet arenas;
- task scratch;
- command allocators/pools/lists;
- upload/readback pages;
- descriptor blocks;
- decoded/cooked assets;
- GPU persistent/transient buffers;
- retirement backlog.

Define:

- expected capacity;
- high-water measurement;
- reuse point;
- overflow/spill/failure policy;
- cancellation cleanup;
- GPU-token retirement where applicable.

Do not trade a modest CPU gain for uncontrolled peak memory.

### 10.8 DOD Evidence

Measure against the replaced path:

- bytes read and written;
- allocations and reallocations;
- working set and high-water memory;
- cache misses and bandwidth;
- branches and pointer indirection where material;
- query candidates/matches and traversal time;
- extraction/apply/merge/commit time;
- packet bytes;
- dirty and uploaded bytes;
- GPU table updates;
- serial/parallel crossover and critical path.

"Uses ECS," "uses SoA," "cache friendly," and "parallel" are not evidence.

### 10.9 Source-Trace Discipline

Use the exact applicable precedent already researched in J. Attribute only what the source actually demonstrates:

- Richard Fabian's *Data-Oriented Design* supports beginning with concrete data type, quantity, frequency, shape, probability, producer, consumer, and transform; normalizing data by access; and judging layout from the cost of real work. It does not require universal SoA or one ECS.
- Epic MassEntity supports data-only fragments, typed queries, chunk-aware iteration, transient views, and deferred structural change. It does not require Sparkle to reproduce Unreal's archetype scale, reflection, or framework surface.
- Epic's game/render proxy model supports separate authoritative game state, derived render-owned state, explicit dirty propagation, and no game-object dereference during render work. It does not require Unreal's class hierarchy.
- NVIDIA Donut and AMD/GPUOpen renderer sources support persistent/indexed renderer data, access-specific streams, stable batching, backend-explicit resource ownership, and measured GPU/CPU practice. They do not establish a general gameplay ECS for Sparkle.
- D3D12, Vulkan, C++, and vendor SDK specifications are correctness authorities for API, synchronization, lifetime, ABI, and capability behavior. A sample repository is a design precedent, not correctness or performance proof.

For each material choice, record the exact source section/repository location, the Sparkle behavior adopted, the behavior deliberately not copied, and the measurement or test that can falsify the choice.

### 10.10 Neural Graphics Data and Artifact Contract

When a real neural feature is selected, inventory both offline and runtime data:

- source dataset and license/provenance;
- train/validation/test split and leakage controls;
- preprocessing, normalization, augmentation and coordinate/color conventions;
- model/operator topology, shapes, layouts, precision and numerical range;
- loss, optimization and quality metrics;
- weights/constants and deterministic export/cook metadata;
- runtime inputs, intermediates, history and outputs;
- persistent versus per-frame GPU storage;
- CPU preparation, upload and dispatch access;
- classical baseline/fallback;
- artifact/package lifetime and capability requirements.

Rules:

- Name types after the actual feature and operation, such as its denoising, reconstruction, sampling or material function. Do not create broad `AiSystem`, `NeuralManager`, `TensorService`, `ModelGraph`, or `InferenceFramework` ownership.
- A generic tensor value is not a default engine primitive. Prefer feature-private bounded descriptors whose dimensions, layout, precision, alignment, semantics and owner are explicit.
- Runtime artifacts are immutable, validated and feature-owned. They contain only required weights/constants/metadata, not training state, optimizer state, Python objects, callbacks or framework sessions.
- Regenerate artifacts to the newest supported representation; do not add legacy content-version compatibility.
- Parse, validate and resolve an artifact outside frame-hot work. Do not parse a model, allocate weights or build pipelines per frame.
- Persistent GPU weights and resources use renderer/RHI lifetime and retirement rules. Dynamic inputs/intermediates use declared frame-graph resources and bounded storage.
- Training/offline code and runtime inference code have different owners, dependencies, memory budgets and measurements. Sharing an artifact schema does not justify sharing a god service.
- Data provenance and licensing are part of correctness. Opaque or unlicensed weights do not enter the repository or packages.
- The classical path remains a real tested fallback, not a stale compatibility path. Shared input/output semantics are authoritative; implementations remain independently owned.
- Quality metrics are interpreted with visual failure cases and dataset scope. Do not optimize a metric while hiding temporal instability, bias, artifacts or out-of-distribution behavior.

## 11. Concurrency and SparkleTasks

### 11.1 One Runtime

Use `SparkleTasks` for owned CPU task execution.

Do not add:

- subsystem thread pools;
- `std::async` integration paths;
- detached workers;
- animation/editor/cooker/renderer pools;
- worker-side ad hoc wait loops;
- a hidden ECS scheduler;
- a second cancellation or progress runtime.

Blocking platform/file/process work uses the configured blocking-I/O lane or an existing narrow platform operation. Third-party internal worker counts are part of the concurrency budget.

### 11.2 Ownership First

Mutable data has one owner thread/task. Cross-thread data is:

- immutable;
- transferred;
- exclusively leased;
- or explicitly concurrent under one documented protocol.

Atomics do not make an object graph thread-safe. A mutex around a broad scene, renderer, editor, or cache is not an ownership model.

### 11.3 Task Rules

- Dependencies express correctness; priority/lane expresses policy only.
- Worker tasks do not wait for other tasks.
- Fan-in occurs through graph dependencies or a bounded host boundary.
- Tasks read immutable state and write exclusive ranges or task-local results.
- Cleanup/finalization settles exactly once on success, failure, or cancellation.
- Scopes settle before captured owners and storage are destroyed.
- Small workloads use a serial path.
- Grain size is measured from scheduling cost, work variance, cache behavior, and critical path.
- Nested third-party or engine parallelism must not oversubscribe the host.

### 11.4 Publication and Atomics

Treat every atomic protocol as a state machine.

Document:

- states and legal transitions;
- single writer/claimer for each state;
- release publication;
- acquire consumption;
- reuse/reclamation edge;
- ABA/generation behavior;
- shutdown and cancellation transitions;
- why each memory order is sufficient.

Use:

- a mutex for a compound invariant;
- an atomic for a small independent transition;
- a condition variable/semaphore for parking;
- release/acquire for ownership publication;
- relaxed ordering only for independent statistics.

Start correct with owner-only, mutex, or sequentially consistent reference behavior. Weaken only after measurement and a lifetime proof. Cross-module raw-pointer lock-free structures are forbidden.

### 11.5 Lock and Wait Rules

- Every mutex, atomic, wait, condition variable, semaphore, device-idle call, and queue has an owner and invariant.
- Wait predicates handle spurious wakeups and lost-wakeup ordering.
- Lock order is explicit where more than one lock can be held.
- Do not invoke arbitrary callbacks, events, ImGui, renderer/RHI, logging, file I/O, driver calls, or destruction while holding an engine lock.
- Spin only for a measured extremely short handoff whose owner is guaranteed to run.
- Routine worker waits, busy-yield loops, and help-while-waiting correctness are forbidden.
- Final shutdown or explicit backpressure MAY block at the documented host boundary.

The target is zero unclassified synchronization, not zero synchronization.

### 11.6 Cache and Scheduling Behavior

Audit:

- false sharing in worker/queue/frame state;
- contention on shared vectors, loggers, allocators, descriptor cursors, and atomics;
- priority inversion, convoying, starvation, thundering herd, preemption, migration, SMT, NUMA, and oversubscription;
- equal-count partitions with unequal cost;
- unnecessary global barriers;
- unbounded producer queues.

Prefer fewer, coarser, dependency-correct tasks to high worker utilization with a longer critical path.

## 12. GameFramework and ECS

- ECS storage remains private to GameFramework.
- `EntityId` is the runtime identity; dense positions remain private and ephemeral.
- Components are data, not virtual update/render objects.
- Heavy assets remain immutable handle-addressed resources, not embedded mutable component ownership.
- Systems declare phase, typed component read/write access, non-ECS resource access, prerequisites, and execution/grain policy.
- Read/read may overlap; write hazards require an explicit order or graph rejection.
- Structural composition is frozen during queries.
- Workers use typed task-local command buffers; the world owner commits deterministically.
- Systems receive narrow views, not `GameWorld&`, registry access, a controller facade, renderer, UI, or platform services.
- Transform and other derived state are evaluated explicitly; a published `const` read does not lazily mutate a cache.
- World read views are immutable and generation-pinned.
- Extraction is an explicit bulk transform from a frozen world epoch.
- Animation is decomposed by operation: sampling, pose evaluation, morph evaluation, skinning evaluation, output storage, transform, and extraction.
- Pre-resolve asset-generation relationships and stable output slots outside hot loops.

Do not introduce:

- parallel virtual `Entity::Update`;
- a public general-purpose ECS SDK;
- component pointers retained by panels or workers;
- structural mutation during iteration;
- completion-order entity allocation or command merge;
- GUID joins in hot queries;
- an animation god subsystem.

## 13. Renderer, Frame Graph, RHI, and GPU

### 13.1 Ownership

- The render coordinator owns mutable renderer/RHI state, queue submission, present, and render lifecycle.
- Renderer consumes immutable owned values or stable handles; it does not query ECS or dereference `GameWorld`.
- Render-owned proxies/tables are derived state, never a second gameplay authority.
- CPU packet lifetime and GPU resource lifetime are separate.
- Ordinary reload, scene change, capture, resize, and retirement do not call device idle.

### 13.2 Frame Graph

The frame graph is the only renderer scheduling, barrier, aliasing, history, and queue-dependency authority.

- Setup declares every resource read/write, history, queue preference, and imported/persistent resource.
- Compile resolves order, lifetime, aliasing, barriers, queue ownership, and recording eligibility.
- Execute records only the compiled work.
- Execute does not discover dependencies, create hidden resources/pipelines, or retain frame-packet memory.
- Frame-graph setup/compile stays serial until measured evidence justifies a bounded change.

Do not add an ECS scheduler, renderer task graph, or backend queue policy that competes with the frame graph.

### 13.3 RHI and Backend Ownership

- RHI is explicit, backend-neutral at its public surface, and backend-private in native implementation.
- Renderer code does not branch on D3D12/Vulkan identity except in a dedicated narrow external-provider interop boundary.
- Recording workers receive one move-only exclusive `RhiCommandRecordingLease`.
- D3D12 allocator/list and Vulkan command-pool/buffer ownership are never shared concurrently.
- Recording workers do not create/destroy RHI resources, mutate global caches, submit, present, or wait.
- Upload and transient descriptor storage is worker-local or preassigned and token-retired.
- Queue submission and present remain single-owner operations.

### 13.4 Persistent GPU Data

Prefer persistent indexed GPU state plus dirty ranges over rebuilding and uploading full scene data.

- Static assets cross through immutable handles/residency.
- Dynamic transforms, lights, skinning, morph, visibility, and temporal values update only required ranges.
- Resolve stable IDs to renderer slots once before hot traversal.
- Coalesce dirty ranges deliberately.
- Capacity growth publishes a replacement at a frame boundary and retires old storage by GPU completion token.
- Measure upload bytes, descriptor pressure, transient/persistent memory, resource churn, RT build/update time, and queue behavior.

### 13.5 Frame Metadata and Rendering Details

- `FrameId` is the shared correlation identity.
- Temporal discontinuity, camera cut, teleport, history reset, resolution, exposure, provider tags, and motion/depth conventions cross only when their consumer needs them.
- Jitter is a renderer implementation detail derived from `FrameId` and renderer settings; do not expose it as Application-owned mutable state.
- Do not retain content-version compatibility. Regenerate and publish the newest content representation.
- Stable-handle generations and GPU completion tokens remain because they prove lifetime and stale-use safety; they are not legacy content versioning.

### 13.6 CPU/GPU Performance Reasoning

Always distinguish:

- CPU task concurrency;
- render-thread pipelining;
- command recording concurrency;
- GPU graphics/compute/copy queue concurrency;
- frames in flight;
- provider execution;
- input-to-present latency.

Parallel CPU recording does not imply GPU overlap. An additional GPU queue is useful only when correlated queue timelines show real overlap after synchronization and bandwidth costs.

### 13.7 Path-Tracing and Neural Kernel Rules

- State coordinate spaces, units, radiometric meaning, PDFs/weights, precision, accumulation/history and numerical limits at the owning math/shader contract.
- Connect important equations to executable reference tests and known values. A comment citing a paper is not a correctness test.
- Neural preprocessing, operator/kernel stages and postprocessing are explicit passes or cohesive shader operations with declared resources.
- Shader code exposes bounds, tensor/image layout, channel/tile mapping, precision and fallback capability. Do not hide them in opaque macros or a generic operator dispatcher.
- Select fusion, tiling, wave/cooperative operations, shared memory, precision and dispatch size from captures and quality tests.
- Inspect DXIL/SPIR-V, reflection, resource layout and disassembly/counters when they materially answer the performance question.
- Report path-tracing/neural quality, latency, memory and temporal behavior together.
- Preserve a deterministic classical/reference path for comparison and fallback.

### 13.8 Hardware and Driver-Facing Code

- Record exact vendor, adapter/architecture, device ID where available, driver, OS, API, compiler and feature capabilities for a driver-sensitive conclusion.
- First prove API validity and ownership with engine assertions plus D3D12/Vulkan native validation.
- Reduce a suspected driver issue to the smallest reproducible resource/pipeline/command/synchronization sequence before attributing it externally.
- Backend workarounds live in the owning backend-private file, have an exact applicability predicate, explain the evidence, and state the removal/retest condition.
- Renderer-wide policy branches on neutral capability, not vendor name or driver number.
- A vendor-specific fast path has a correct neutral fallback and does not redefine the public feature contract.
- Future-hardware opportunities remain measured hypotheses behind existing capability seams until hardware and driver evidence exists.
- Do not claim Linux support from portable-looking code. Require a native Linux configure/build/run, Vulkan validation, capture/debug, package and shutdown result.

## 14. Editor, UI, and Tools

### 14.1 Editor Ownership

Editor main owns:

- ImGui context and widget state;
- selection;
- immutable editor scene model;
- transactions and undo/redo;
- draft values for active widgets;
- application/document lifetime;
- application of narrow operation results.

Panels:

- read immutable models;
- identify world objects by stable ID;
- submit semantic commands;
- consume typed accepted/stale/rejected results;
- never retain `GameWorld*`, component pointers, mutable spans, registry views, renderer caches, live descriptor pointers, or durable vector indices.

Continuous edits are coalesced into bounded main-thread transactions with deterministic inverse commands or before/after values.

### 14.2 UI and Render Boundary

- ImGui draw data is copied into packet-owned vertices, indices, clip rectangles, texture handles, and commands.
- No `ImDrawData*` or live editor pointer crosses to the render coordinator.
- Viewport requests and products use stable IDs/tokens and explicit release or bounded retirement.
- Settings, preview, and capture use sequenced render commands.
- Capture is bounded and nonblocking: request, render copy/readback, GPU token, background encode/write, narrow result.
- Close/cancel rejects late products before destroying their owner/model.

### 14.3 Background Operations

Use one private `EditorOperationService` over SparkleTasks scopes for owned workflows.

- Inputs are immutable owned request values.
- Progress is bounded and coalesced.
- Results are immutable and applied on the owning thread.
- Search/preview/reload use latest-generation-wins where that is the product policy.
- Document/application close cancels, settles, rejects late results, and then destroys state.
- Workers do not call ImGui or invoke UI callbacks.

### 14.4 Cooking, Import, and Publication

- Separate read, decode, transform, validate, and transactional publication stages by real ownership.
- Keep runtime loading cooked-only.
- Outputs are deterministic and transactionally replaced.
- Cancellation preserves the previous accepted artifact/world.
- File/process work is isolated from frame-critical capacity.
- Concurrency has a weighted memory budget for HDR textures, scenes, compiler sessions, and third-party workers.
- Do not add a second asset database, async loader family, or tool thread pool.

## 15. APIs, Coupling, and Lifetimes

### 15.1 Public-Surface Budget

A public type needs:

- at least two real owned module consumers; or
- a necessary stable host/module boundary.

Public contracts MAY expose:

- immutable requests/results;
- stable IDs and typed handles;
- descriptors;
- packets/read models;
- explicit cancellation/lifetime operations;
- narrow capability interfaces required by real implementations.

Public contracts MUST NOT expose:

- worker records, queues, dependency counters, or scheduler internals;
- private ECS storage;
- mutable renderer caches or device escape hatches;
- backend-native handles except a narrow explicit interop contract;
- compiler sessions;
- task execution context in unrelated domain APIs;
- mutable spans or borrowed storage whose epoch cannot be enforced;
- diagnostic snapshots without a current product UI consumer.

### 15.2 Capability APIs

High-level callers invoke narrow domain verbs:

- start;
- submit;
- cancel;
- consume;
- apply;
- publish;
- commit;
- release.

The owner of independent work owns:

- partitioning;
- task-local state;
- scheduling lane;
- cancellation;
- progress coalescing;
- result publication;
- deterministic merge;
- shutdown settlement.

Do not make the Application, Editor, or Renderer facade understand worker counts, partitions, dependency nodes, merge buffers, compiler sessions, or backend allocation mechanics unless scheduling that mechanism is its actual responsibility.

### 15.3 Lifetime Expression

Use:

- value ownership for immutable requests/results;
- `unique_ptr` for sole heap ownership;
- `shared_ptr` only for genuine shared lifetime, not convenience;
- references for required bounded non-owning access;
- pointers for optional or externally owned access with an explicit lifetime;
- handles/tokens/leases for cross-boundary identity and authority;
- scopes for asynchronous settlement;
- arenas only with a named owner, seal/publication point, and reset condition.

Never hide lifetime in:

- callbacks capturing raw owners;
- service locators;
- global registries;
- durable spans;
- vector indices;
- "valid" booleans that conflate CPU reuse and GPU completion.

## 16. Validation, Diagnostics, Logging, and Comments

### 16.1 Minimal Correctness Checks

Validation belongs at the narrowest owner that can enforce the invariant once.

Use:

- a typed result for expected boundary rejection;
- an assertion for programmer misuse or impossible owner-state violation in the appropriate build;
- a fatal error only when continuing cannot preserve product correctness;
- native D3D12/Vulkan validation for API ownership/state;
- a focused test for state-machine, ordering, or lifetime behavior.

Avoid:

- repeated null/owner/generation checks in every orchestration layer;
- validation mirrors of authoritative state;
- per-item counters or logs;
- broad "diagnostics" types that leak caches/queues;
- a second error path for the same failure;
- permanent production code used only to prove one migration.

### 16.2 Logging

Logging is for actionable product or operation outcomes.

Log:

- one concise failure with enough context to act;
- important lifecycle transitions when an existing workflow needs them;
- a bounded summary of a failed operation.

Do not log:

- every entity, asset, pass, task, descriptor, or packet;
- ownership assertions that should be encoded structurally;
- normal polling/progress state every frame;
- the same failure in each forwarding layer;
- performance counters instead of using existing profiler/debugger tools.

### 16.3 Instrumentation

- Reuse existing profiler, PIX, Nsight, RenderDoc, frame-graph, allocator, and debugger hooks.
- Add only the label/scope necessary to identify the changed critical path.
- Keep capture-time instrumentation private.
- Do not add a public profiler framework, runtime log stream, task panel, cache browser, default CSV/JSON report, or broad snapshot product.
- Remove disposable fault injectors, migration validators, and report builders after their invariant is encoded in the final owner and regression test.

### 16.4 Comments

Comments explain:

- ownership and lifetime;
- why a dependency or ordering edge exists;
- memory-order reasoning;
- units, coordinate spaces, ABI, and backend constraints;
- why a measured layout/policy was chosen;
- non-obvious failure or retirement behavior.

Comments do not:

- narrate obvious code;
- preserve obsolete history;
- promise future refactors;
- call unsupported behavior "thread safe" or "lock free";
- duplicate the prompt;
- conceal a compatibility path without a deletion gate.

### 16.5 AI-Assisted Engineering

AI tools may accelerate search, implementation, test generation, shader/model exploration and design alternatives. They are never an authority.

For AI-assisted work:

- inspect every changed line and surrounding ownership path;
- verify APIs and citations against primary/local sources;
- derive or independently check math, coordinate systems, units and numerical assumptions;
- compile and inspect generated C++/shader code under normal warnings, validation and backend gates;
- check lifetime, synchronization, determinism, bounds, security, license/provenance and package impact;
- test that generated tests fail under the defect they claim to detect;
- measure generated performance claims on the actual workload and hardware;
- remove verbose boilerplate, speculative abstractions, excessive comments, logging and validation commonly produced by generation;
- never commit private prompts, hidden chain-of-thought, generated chatter or credentials as project evidence;
- record the categories independently verified when AI materially influenced a strategic feature.

The human engineer remains accountable for architecture, correctness, source attribution, model/data provenance, performance claims and final communication.

## 17. Performance Engineering

Optimize in this order unless evidence shows otherwise:

1. remove unnecessary work;
2. remove full rebuilds, scans, copies, and uploads;
3. improve algorithm and data access;
4. remove allocation churn and pointer chasing;
5. make lifetime and caching persistent/incremental;
6. reduce synchronization and contention;
7. expose independent ranges;
8. parallelize above the measured crossover;
9. tune backend/GPU queue behavior from captures.

### 17.1 CPU

Inspect:

- algorithmic complexity;
- hot traversal cardinality;
- cache locality and working set;
- branch behavior;
- virtual dispatch/pointer chasing;
- repeated lookup/join/scans;
- allocations and allocator contention;
- false sharing;
- task count, grain, queue delay, critical path, and tail imbalance;
- p50/p95/p99 rather than only average/FPS.

### 17.2 GPU

Inspect:

- resource and descriptor lifetime;
- upload bytes and dirty ranges;
- transient and persistent memory;
- barrier and queue ownership;
- command-list/buffer count and submission overhead;
- PSO/shader cold-cache behavior;
- raster and RT build/update inputs;
- graphics/compute/copy overlap;
- GPU bubbles, bandwidth, occupancy where relevant;
- temporal and provider correctness.

Do not improve CPU utilization by regressing GPU barriers, memory, command-list count, descriptor pressure, residency, RT behavior, or input latency without an explicit measured product tradeoff.

### 17.3 Measurement Discipline

For performance claims, record:

- exact build/configuration;
- machine CPU/topology and worker policy;
- backend and validation state;
- workload and scene;
- cold/warm state;
- serial/1/2/N modes as applicable;
- before/after values;
- critical path;
- memory high-water;
- GPU result;
- regressions and limitations.

Use a tiny workload to expose overhead and a representative heavy workload to expose useful parallelism. A trace, CPU utilization, thread count, or FPS alone is not proof.

### 17.4 Neural Training and Inference Performance

Measure training/offline preparation separately from runtime inference.

Training/offline evidence MAY include:

- dataset/preprocessing throughput;
- batch and precision sweep;
- convergence/quality by step or epoch;
- optimizer/schedule choice where owned;
- CPU/GPU utilization and data stalls;
- peak memory;
- artifact/export time and determinism.

Runtime inference evidence MUST include:

- preprocess, each material operator/kernel stage and postprocess;
- artifact and persistent weight bytes;
- intermediate/history bytes;
- dispatch count and dimensions;
- CPU preparation and upload cost;
- GPU latency and end-to-end frame contribution;
- registers/occupancy/cache/bandwidth/divergence where the tool supports a relevant conclusion;
- frame pacing and input latency under representative concurrent engine work;
- classical-versus-neural quality, visual failures, latency and memory on identical inputs.

Select the accepted model/kernel configuration from a quality-performance-memory frontier. Do not select only the fastest or highest-scoring point.

## 18. Testing and Evidence

Tests follow the changed ownership and failure path.

Required where applicable:

- serial oracle and final-contract serial execution;
- deterministic state/byte/image/order comparison;
- 1, 2, and N workers;
- randomized task completion and delay;
- cancellation at every stage;
- owner destruction/close while work is in flight;
- stale generation/handle/sequence rejection;
- bounded queue/backpressure and memory ceiling;
- structure mutation rejection during frozen views;
- wrong-thread/lease misuse rejection;
- D3D12 GPU validation and Vulkan synchronization/validation;
- delayed GPU and deferred retirement;
- cold and warm caches;
- tiny-work serial threshold;
- feature-preservation matrix.
- mathematical reference values and numerical-error bounds;
- model artifact malformed/provenance/shape/layout/precision rejection;
- offline/reference versus runtime neural output tolerance;
- train/validation/test separation and deterministic export;
- classical/neural identical-input comparison with visual failure cases;
- exact hardware/driver/capability matrix and reduced driver reproducer;
- clean-package proof that training dependencies/data do not enter runtime;
- AI-assisted code/test defect-detection and independent-review evidence.

Prefer explicit controllable barriers, events, generations, and fault hooks over sleeps. Tests must detect an injected defect in the invariant they claim to prove.

Unavailable hardware, toolchains, sanitizers, or provider paths are reported as unavailable. They are not simulated or silently marked passed.

## 19. Change-Set Reconciliation

Before acceptance, reconcile the whole changed ownership path.

### 19.1 Structure Reconciliation

Report:

- files retained, moved, renamed, split, merged, added, and deleted;
- module/subsystem/public-private placement;
- primary type and responsibility;
- orchestration versus capability split;
- deliberate co-location;
- includes, CMake, exports, source groups, tests, and docs updated;
- old paths/names with zero remaining production references.

### 19.2 Implementation-Shape Reconciliation

Report:

- non-accessor/template bodies moved from headers;
- function-local types reassigned to visible owners;
- anonymous behavior converted to owner members or real domain ownership;
- anonymous-namespace repository scan result;
- invented `Detail`/`Internal`/`Helpers` namespace/path additions: expected zero;
- god-class/function audit and resulting splits;
- diagnostics/logging/validation removed or narrowly retained.

### 19.3 DOD Reconciliation

Report:

- replaced object/pointer/full-copy path;
- authoritative and derived stores;
- data/access inventory;
- chosen and rejected layouts;
- stable identity;
- mutation/publication/reclamation phases;
- deterministic partition/merge;
- allocations and memory equation;
- exact source precedent from J;
- measurement and falsifier.

### 19.4 Concurrency Reconciliation

Report:

- touched threads/tasks/queues/locks/atomics/waits/device-idle calls;
- owner and invariant for each retained primitive;
- task dependencies and exclusive outputs;
- cancellation and shutdown settlement;
- serial threshold and grain;
- applicable MT hazard IDs from J;
- concrete falsifying tests/traces;
- zero unclassified synchronization in scope.

## 20. Acceptance Checklist

A change is ready only when the answer to each applicable question is yes.

### Architecture and Ownership

- Is there one authoritative mutable owner?
- Are derived projections one-way and explicitly published?
- Does every class/file/function have one coherent responsibility?
- Does high-level code read as orchestration rather than mechanism?
- Are current product and backend capabilities preserved?
- Is the replaced path deleted?
- Are public APIs narrower than their private implementation?

### Structure and C++

- Do module, folder, filename, and primary type agree?
- Are substantive capabilities in dedicated files where appropriate?
- Are headers declaration-only except templates and trivial accessors?
- Are all nontrivial bodies in `.cpp`?
- Do blank lines expose logical stages without fragmenting cohesive initialization?
- Do signatures and ordinary expressions remain compact when they fit the configured column limit?
- Are there no function-local class/struct definitions?
- Are there no anonymous namespaces?
- Were arbitrary `Detail`/`Internal`/`Helpers` ownership namespaces avoided?
- Do formatting, naming, and includes follow executable repository configuration?

### SOLID and DRY

- Does every type have one owner/invariant/reason to change?
- Are interfaces justified by real substitution?
- Are consumers given only required capabilities?
- Is policy separate from volatile mechanisms?
- Is there one truth, one identity rule, and one production path?
- Were coincidental similarities left separate rather than over-generalized?

### Data and Performance

- Is the data/access inventory concrete?
- Was layout chosen from access evidence?
- Are stable IDs/handles used instead of durable indices/pointers?
- Are hot/cold and structural/dynamic lifetimes appropriate?
- Are variable-length arrays flat and bounded?
- Are allocations, packet bytes, dirty/upload bytes, and memory high-water understood?
- Was unnecessary work removed before threading?
- Does tiny work remain near serial cost?

### Concurrency

- Is SparkleTasks the only owned worker runtime?
- Are dependencies explicit and workers wait-free?
- Does each task write private or exclusive output?
- Is merge order stable and independent of completion?
- Are queues bounded?
- Are scopes settled before owner destruction?
- Are atomics/locks/waits documented and falsified?
- Are CPU tasks, render pipelining, GPU queues, and frames in flight distinguished?

### Renderer/RHI

- Does render mutation remain render-coordinator owned?
- Does Renderer avoid ECS and `GameWorld` access?
- Is the frame graph the scheduling/barrier authority?
- Are native APIs backend-private?
- Are recording contexts exclusive?
- Are GPU lifetimes token-retired without routine device idle?
- Is jitter renderer-derived rather than Application state?
- Are both supported backends validated?

### Editor/Tools

- Does Editor main own ImGui, selection, transactions, and model application?
- Do panels use immutable models and semantic commands?
- Are cross-thread UI/render products owned?
- Is background work scoped, bounded, cancellable, and late-result safe?
- Is publication transactional and deterministic?

### Diagnostics and Evidence

- Is validation enforced once at the narrow owner?
- Are logs minimal and actionable?
- Was diagnostic/reporting infrastructure avoided?
- Are exact validation commands and results recorded?
- Are performance claims causal and reproducible?
- Are unavailable checks stated honestly?

### Principal Graphics Engineering

- Were applicable `PGE-*` requirements classified before implementation?
- Does each advanced role claim have concrete evidence?
- Were path-tracing/neural/math/hardware-driver/partner seams preserved without empty scaffolding?
- If AI tools were used materially, were code, shader, math, model, test, source and performance independently verified?
- Are hardware/driver/platform claims scoped to exact evidence?
- For neural work, are real model/operator, artifact, baseline/fallback, quality, latency, memory, pacing and provenance all present?
- Can another engineer reproduce and adopt the result?
- Does communication follow completed engineering rather than substitute for it?

## 21. Required Completion Report

Every future prompt applying this guide ends with:

1. **Outcome** - what product capability now works.
2. **Repository audit** - existing counterparts searched and the use/extend/refactor/replace/add decision.
3. **Ownership** - mutable owners, lifetime owners, publication and reclamation.
4. **Files changed by responsibility** - grouped by module/subsystem, not one flat list.
5. **Orchestration/capability refinement** - god units audited and responsibilities separated.
6. **SOLID/DRY reconciliation** - authorities consolidated, abstractions justified, duplicate paths removed.
7. **Preservation ledger** - current workflows/features/backends retained.
8. **Deletion ledger** - old APIs, files, adapters, scans, allocations, waits, flags, and aliases removed.
9. **Structure reconciliation** - folder/public-private/file/type/CMake/include alignment.
10. **Implementation-shape reconciliation** - headers, local types, namespaces, diagnostics, and logging.
11. **DOD reconciliation** - data/access/layout/identity/transform/memory/measurements.
12. **Concurrency reconciliation** - tasks, dependencies, exclusive outputs, primitives, hazards, cancellation, shutdown.
13. **Validation** - exact commands, configurations, worker counts, backends, tests, and results.
14. **Performance** - before/after and critical-path/memory/GPU evidence where relevant.
15. **Naming audit** - canonical and rejected-alias searches.
16. **Limitations and unavailable evidence** - precise and honest.
17. **Acceptance status** - `PASS` or `BLOCKED`, with no partial-pass wording.
18. **PGE reconciliation** - applicable IDs, advance/preserve/not-applicable/blocked status, exact evidence, AI-assisted verification where used, honest credential/platform/hardware boundaries, and role-only scaffolding audit.

## 22. Immediate Stop Conditions

Stop and request a product or architecture decision when:

- a prerequisite integration has not passed;
- a required current feature cannot survive the proposed owner/lifetime model;
- the current repository contradicts the assumed architecture;
- a public abstraction has no real consumer;
- exclusive outputs or deterministic order cannot be proven;
- serial state/byte/image behavior does not match and the difference is unexplained;
- D3D12/Vulkan validation reports an ownership or state error;
- the change requires a materially broader subsystem than the prompt authorizes;
- the only justification is future use, familiarity, framework elegance, or an unmeasured performance claim.

Do not hide a stop condition behind compatibility code, a shared mutex, extra logging, or a "temporary" second architecture.

## 23. Compact Forbidden-Pattern Index

The following are rejected unless a prompt explicitly establishes and closes a narrow exception:

- arbitrary mutable-world controller execution;
- live game/editor pointers crossing render or worker boundaries;
- raw asset pointers in packets;
- durable vector indices;
- mutable spans or component views retained across epochs;
- parallel virtual entity/component update;
- shared per-task vector push or asset/cache mutation;
- completion-order identity, merge, publication, draw, or submission;
- structural ECS mutation during iteration;
- hidden ECS/render schedulers;
- subsystem pools, detached threads, or `std::async`;
- worker waits or routine device idle;
- unbounded queues or progress streams;
- broad scene/editor/renderer mutexes;
- lock-free structures without progress, ABA, lifetime, and reclamation proof;
- lazy PSO/resource/cache mutation during recording;
- worker resource creation/destruction, submit, present, UI, or renderer callbacks;
- backend-native types in Renderer or public neutral contracts;
- app-owned jitter;
- content compatibility version layers when content is regenerated to newest;
- duplicate GUID/runtime-ID authorities or hot GUID lookup;
- public cache/queue/task diagnostics;
- validation/logging boilerplate in orchestration;
- orchestration presented as one wall of mechanism statements;
- needless line fragmentation of signatures, calls, casts, member access, or short expressions;
- nontrivial header function definitions;
- function-local class/struct definitions;
- anonymous namespaces;
- arbitrary `Detail`, `Internal`, `Private`, `Local`, `Implementation`, `Helper`, or `Common` ownership buckets;
- god files split by number rather than responsibility;
- one-method wrappers, speculative interfaces, factories, or DI;
- `New*`, `*2`, `Async*`, `ThreadSafe*`, `LockFree*`, `MultiThreaded*`, or permanent compatibility aliases;
- full-scene rebuild/copy/upload where stable dirty data is available;
- performance claims based only on FPS, utilization, traces, or one backend.

## 24. Principal Graphics Engineering Review Gate

This gate applies to every future prompt. It raises the final evidence bar while preserving all earlier architecture and style requirements.

### 24.1 Per-Prompt Traceability

Before implementation:

1. Read A's `PGE-01` through `PGE-15` matrix and H's persona interpretation.
2. List the applicable IDs.
3. Mark each **advance**, **preserve**, **not applicable**, or **blocked**.
4. Name the evidence expected from the current change.
5. Identify role-shaped abstractions the change must not introduce.

After implementation:

1. Link each advanced claim to code, test, capture, math/reference result, artifact or handoff evidence.
2. Show how preserved contracts were kept.
3. State newly exposed gaps honestly.
4. Verify AI-assisted work if used.
5. Confirm no role-only scaffolding, unsupported credential/platform/hardware claim or permanent experiment surface remains.

### 24.2 Role-to-Code Review Matrix

| Role requirement | Code/repository review question |
|---|---|
| `PGE-01` | Can another engine team integrate, configure, fail, fall back, capture, debug and tune this capability without receiving broad owner access? |
| `PGE-02` | Does path tracing have explicit math, scene/resource inputs, RT lifetime, shader/frame-graph/RHI ownership, deterministic reference and paired API evidence? |
| `PGE-03` | Does a real model/operator execute in a real product path and improve or replace a classical path? |
| `PGE-04` | Are model/operator/layout/precision decisions based on ablation and end-to-end profiling rather than framework defaults? |
| `PGE-05` | Are frame time, p95/p99, pacing, input latency, memory and concurrent workload interference inside a bounded product budget? |
| `PGE-06` | Are API correctness, hardware capability and driver behavior separated with exact configuration and a reduced reproducer? |
| `PGE-07` | Does the C++ express ownership/lifetime directly, remain cohesive, and pass debugging/validation/stress gates? |
| `PGE-08` | Can the important math and numerical assumptions be derived, tested and connected to measured cost? |
| `PGE-09` | Are HLSL/Slang, DXIL/SPIR-V, D3D12/Vulkan resources, synchronization and capability/fallback behavior explicit? |
| `PGE-10` | Does the optimization explain CPU/GPU architecture effects using causal experiments, not slogans? |
| `PGE-11` | Are model/data/loss/generalization/deployment understood, and was AI-assisted output independently verified? |
| `PGE-12` | Are training/offline preparation and runtime inference separately owned, measured and packaged? |
| `PGE-13` | Did one bounded GPU/research hypothesis become a measured product decision or get deleted, and is the result prioritized, reproducible and explainable through a concise review, demo, technical note and talk outline? |
| `PGE-14` | Are Windows/Linux/driver claims limited to platforms and workflows actually built, run, validated and debugged? |
| `PGE-15` | Does the change demonstrate independent end-to-end judgment and leave the system simpler for the next engineer? |

### 24.3 Neural Feature Acceptance

A neural graphics change fails review if any of these are missing:

- a current product problem and named classical baseline;
- real data/model/operator provenance;
- train/validation/test or equivalent evaluation boundary;
- model/operator math, shapes, layout, precision and numerical contract;
- deterministic validated artifact and clean package ownership;
- real runtime inference integrated through existing renderer/frame-graph/RHI ownership;
- capability-gated classical fallback;
- offline/reference versus runtime validation;
- quality metrics plus visual/temporal failure cases;
- CPU/GPU latency, memory, pacing and representative concurrent workload evidence;
- model/kernel/system ablation and retained negative results;
- deletion of temporary experiments and unused variants.

The following never count as neural graphics completion:

- an empty tensor abstraction;
- a model/provider enum;
- a mock or random-weight network;
- an isolated shader microbenchmark;
- an offline notebook with no runtime feature;
- a vendor SDK toggle without owned integration evidence;
- a demo image without quality/performance/artifact/fallback proof.

### 24.4 Partner Adoption and Communication

Code intended to demonstrate developer technology competence SHOULD optimize for adoption:

- narrow, stable request/result/handle contracts;
- explicit prerequisites, capability and failure/fallback;
- deterministic content/artifact setup;
- one reproducible workload;
- capture/debug entry points through existing tools;
- scoped tuning policy instead of a wall of CVars;
- no hidden global state or author-only scripts;
- clear deletion and compatibility status.

Only completed strategic work produces:

- an integration case study;
- a reduced issue reproducer;
- a whitepaper-quality technical result;
- a conference-talk outline;
- a live demo script.

These artifacts explain code and evidence. They do not become new runtime reporting systems.

### 24.5 Claims and Experience Boundary

- Repository evidence demonstrates skills; it does not prove a degree, fifteen years of experience, employment, partner engagement, willingness to travel, or proprietary knowledge.
- Say "partner-shaped integration case" unless a real external partner was involved and disclosure is authorized.
- Say "driver-facing investigation" unless actual driver development was performed.
- Say "Linux-ready boundary" only as an architectural statement; say "Linux supported" only after native evidence.
- Say "neural graphics feature" only after Section 24.3 passes.
- Say "optimized for tested configuration" and list it; do not generalize to future architectures without evidence.
- Internal source-trace documents may name the job posting and vendor sources. Release/product wording remains technically descriptive and avoids implied endorsement or equivalence.

## Closing Standard

A strong SparkleEngine integration should let a reviewer answer, without reading every implementation line:

- Who owns the mutable state?
- What is the lifetime and publication boundary?
- What does each file and class do?
- Which data is read together, how often, and in what layout?
- What can run independently and where is the deterministic join?
- Which thread/task/GPU queue owns each operation?
- Which old path was deleted?
- Which existing product features remain intact?
- What evidence would falsify the design?

If those answers are difficult to find, the integration is not finished.

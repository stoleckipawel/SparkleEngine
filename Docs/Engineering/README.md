# Engineering

Status: engineering navigation index

Engineering owns the rules and procedures used to change SparkleEngine safely. It is organized by why a reader comes here, not as one undifferentiated standards folder. System shape belongs in [Architecture](../Architecture/README.md); completion gates belong in [Acceptance](../Acceptance/README.md).

## The Short Route

For most owned code or documentation changes:

1. read [Change Integration](Workflow/ChangeIntegration.md) for the invariants every change preserves;
2. follow [Change Lifecycle](Workflow/ChangeLifecycle.md) for preparation, execution, validation, and handoff;
3. choose only the applicable foundation, module, and verification documents below.

Do not read every engineering document by default. The change surface determines the conditional rules.

## Choose By Task

| I need to... | Read | Why |
| --- | --- | --- |
| plan, implement, or hand off a material change | [Change Lifecycle](Workflow/ChangeLifecycle.md) | owns the end-to-end change procedure and iteration record |
| preserve integration invariants or perform a clean break | [Change Integration](Workflow/ChangeIntegration.md) | owns repository-wide integration and replacement rules |
| review a changelist | [Code Review](Workflow/CodeReview.md) | owns the read-only review route and finding format |
| harden a capability inventory or prepare its evidence map | [Capability Documentation Review](Workflow/CapabilityReview.md) | owns the developer/reviewer questions, dossier fields, and horizontal/vertical audit workflow |
| add, split, move, or retire documentation | [Documentation Organization](Workflow/DocumentationOrganization.md) | owns knowledge placement, granularity, naming, and navigation |
| change a module boundary, dependency, API, owner, or lifetime | [Module Ownership](Foundations/ModuleOwnership.md) | owns structural and dependency rules |
| name a type, file, field, API, or concept | [Naming](Foundations/Naming.md) | owns vocabulary and naming rules |
| add or change data storage, copies, snapshots, or allocation | [Data And Memory](Foundations/DataAndMemory.md) | owns data-oriented design and the single-truth/copy budget |
| apply C++ source formatting and language conventions | [Code Style](Foundations/CodeStyle.md) | owns source-level coding rules; executable configuration remains authoritative |
| change the task system or concurrent work | [Tasks](Modules/Tasks.md) | owns task, synchronization, cancellation, and shutdown rules |
| change worlds, levels, ECS, or gameplay publication | [GameFramework](Modules/GameFramework.md) | owns GameFramework-specific engineering rules |
| change Renderer, renderer-owned shaders, frame graphs, or render products | [Renderer](Modules/Renderer.md) | owns render policy, products, pipelines, GPU-scene, and graphics evidence rules |
| change RHI contracts, D3D12/Vulkan backends, GPU synchronization, or drivers | [RHI](Modules/RHI.md) | owns neutral GPU contracts, backend lowering, native validation, and driver rules |
| change Editor UI, viewport, capture UX, or editor background work | [Editor](Modules/Editor.md) | owns editor state, UI/render boundaries, and interactive workflow rules |
| change import, cooking, shader compilation, Launcher, or tool publication | [Tools](Modules/Tools.md) | owns tool execution, deterministic transformation, and transactional publication rules |
| choose or report validation, tests, evidence, or performance checks | [Validation And Evidence](Verification/ValidationAndEvidence.md) | owns claim-driven validation and evidence semantics |
| capture with an external performance tool | [External Profiling](Verification/ExternalProfiling.md) | owns the operational capture workflow |

## Engineering Areas

| Area | Open it when... | Contents |
| --- | --- | --- |
| [Workflow](Workflow/README.md) | you are deciding how a change, review, or documentation edit proceeds | integration, lifecycle, review, documentation organization |
| [Foundations](Foundations/README.md) | a change affects rules shared across modules | ownership, naming, code style, data and memory |
| [Modules](Modules/README.md) | a change enters a specific implementation domain | Tasks, GameFramework, Renderer, RHI, Editor, Tools |
| [Verification](Verification/README.md) | a claim needs a check or retained evidence | validation selection and external profiling |
| [Decisions](Decisions/README.md) | you need the rationale behind an accepted engineering choice | decision records; not current rule owners |

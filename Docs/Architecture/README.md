# SparkleEngine Architecture

**Status:** subject-first system and feature knowledge index

This tree explains what each system does, how data and responsibility move through the engine, which design choices were made, where current implementation stops, what informed those choices, and how remaining work is sequenced. Related architecture, plans, research, and feature-local proof contracts stay with their owning module or feature.

> [!TIP]
> New to the repository? Read [SparkleEngine At A Glance](EngineAtAGlance.md), then follow either the [Renderer](Modules/Engine/Renderer/README.md) or [RHI](Modules/Engine/RHI/README.md) path.

## Choose Your Depth

| Reading depth | Use it for | Start here |
| --- | --- | --- |
| 2-minute orientation | What exists, what is missing, and the largest tradeoffs | [Engine At A Glance](EngineAtAGlance.md) |
| Current readiness | How far each tracked feature has progressed from implementation through delivery | [Current Feature Readiness](../Acceptance/CurrentReadiness.md) |
| System understanding | Major owners and dependency direction | [Whole Repository Map](WholeRepositoryMap.md) |
| Module understanding | One Engine, Tools, Projects, or build boundary | [Module Architecture](Modules/README.md) |
| Feature understanding | One result, its selection, design, limits, and proof contract | [Renderer Features](Modules/Engine/Renderer/Features/README.md), [RHI Features](Modules/Engine/RHI/Features/README.md), or the owning module |
| Cross-system trace | A workflow or GPU feature spanning several owners | [Cross-Module Architecture](CrossModule/README.md) |
| Documentation coverage | Whether every strategy, plan, acceptance, and research feature has an Architecture owner | [Feature Documentation Coverage](CrossModule/FeatureDocumentation/README.md) |
| First-release implementation | Release order plus module-owned work packages | [First Release Implementation Plan](CrossModule/FirstRelease/README.md) |
| Product journey | Build, content, editor, runtime, and delivery handoffs | [Product Workflow Coverage](CrossModule/ProductWorkflowCoverage.md) and [Product Execution Traces](CrossModule/ProductExecutionTraces.md) |
| Accepted invariant | Why a boundary must remain true | [Architecture Decisions](Decisions/README.md) |

## What Lives Here

| Area | What it answers | Example |
| --- | --- | --- |
| [Modules](Modules/README.md) | Who owns the capability and what does the current source path contain? | Renderer, RHI, Tasks, Launcher, Showcase |
| [CrossModule](CrossModule/README.md) | How does one result cross owners or backends? | graphics execution, product workflows, shader system |
| [Decisions](Decisions/README.md) | Which dependency or ownership invariant was deliberately accepted? | Renderer/RHI boundary |
| [Engine At A Glance](EngineAtAGlance.md) | What is the current overall shape, support state, and main gap? | source-present versus missing/unproved capability summary |

The module hierarchy mirrors [Engine](Modules/Engine/README.md), [Tools](Modules/Tools/README.md), [Projects](Modules/Projects/README.md), and repository-wide [Build And Packaging](Modules/BuildAndPackaging/README.md) ownership. Links may cross those boundaries; document placement does not.

## What A Subject Folder Contains

| Document role | Conventional name | Authority |
| --- | --- | --- |
| feature or module entry | `README.md` | current/target design, ownership, limits, and navigation |
| dated source ledger | `Capability.md` or `CapabilityInventory.md` | source/build presence and explicit gaps at its named snapshot |
| delivery sequence | `Plan.md` or a narrowly named `*Plan.md` | phase order, dependencies, stop rules, and phase exits |
| external or historical study | `Research.md` or a descriptive `*Research.md` | precedent, options, visual exploration, or dated baseline only |
| feature-local proof contract | `Acceptance.md` | criteria, controlled failures, checks, and definition of done |

The directory answers “who owns this subject?” The filename and status header answer “what kind of knowledge is this?” A module-wide or release-wide plan stays at the nearest coherent module or `CrossModule` owner instead of creating a parallel plan hierarchy.

## How To Read A Feature Page

1. Read **At A Glance** and **Current readiness** to distinguish implemented, partial, missing, and unproved behavior.
2. Use the diagram and **How It Works** section to form the execution/ownership model.
3. Check the support matrix and tradeoffs before assuming a backend, mode, or content type works.
4. Read limitations and failure behavior before using the feature in a product claim.
5. Use exact capability IDs, source routes, and acceptance checks only when implementing or reviewing it.

Feature architecture owns the feature-local completion contract—criteria, controlled failures, checks, and definition of done. [Acceptance](../Acceptance/README.md) tracks cross-feature workloads and candidate/release progress without creating a duplicate feature hierarchy.

## Evidence Boundary

A document in this tree may prove that a source/build route exists or define what must be tested. It does not prove that the route built, ran, produced a correct image, met performance targets, or may ship. Plans and research add no readiness credit. Read the projected percentage in [Current Feature Readiness](../Acceptance/CurrentReadiness.md), the current evidence state in [Acceptance](../Acceptance/README.md), and the missing checks in the [Capability Evidence Plan](Modules/CapabilityEvidencePlan.md).

## Writing And Placement

Use [Documentation Organization](../Engineering/Workflow/DocumentationOrganization.md) for ownership and placement, and the [Documentation Page Template](../Engineering/Workflow/DocumentationPageTemplate.md) for reader-first structure and visual guidance.

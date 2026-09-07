# SparkleEngine Architecture

**Status:** architecture navigation index

Architecture explains what each system does, how data and responsibility move through the engine, which design choices were made, and where current implementation stops.

> [!TIP]
> New to the repository? Read [SparkleEngine At A Glance](EngineAtAGlance.md), then follow either the [Renderer](Modules/Engine/Renderer/README.md) or [RHI](Modules/Engine/RHI/README.md) path.

## Choose Your Depth

| Reading depth | Use it for | Start here |
| --- | --- | --- |
| 2-minute orientation | What exists, what is missing, and the largest tradeoffs | [Engine At A Glance](EngineAtAGlance.md) |
| System understanding | Major owners and dependency direction | [Whole Repository Map](WholeRepositoryMap.md) |
| Module understanding | One Engine, Tools, Projects, or build boundary | [Module Architecture](Modules/README.md) |
| Feature understanding | One result, its selection, design, limits, and proof contract | [Renderer Features](Modules/Engine/Renderer/Features/README.md), [RHI Features](Modules/Engine/RHI/Features/README.md), or the owning module |
| Cross-system trace | A workflow or GPU feature spanning several owners | [Cross-Module Architecture](CrossModule/README.md) |
| Documentation coverage | Whether every strategy, plan, acceptance, and research feature has an Architecture owner | [Feature Documentation Coverage](CrossModule/FeatureDocumentation/README.md) |
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

## How To Read A Feature Page

1. Read **At A Glance** to understand the result and current state.
2. Use the diagram and **How It Works** section to form the execution/ownership model.
3. Check the support matrix and tradeoffs before assuming a backend, mode, or content type works.
4. Read limitations and failure behavior before using the feature in a product claim.
5. Use exact capability IDs, source routes, and acceptance checks only when implementing or reviewing it.

Feature architecture owns the feature-local completion contract—criteria, controlled failures, checks, and definition of done. [Acceptance](../Acceptance/README.md) tracks cross-feature workloads and candidate/release progress without creating a duplicate feature hierarchy.

## Evidence Boundary

An Architecture page may prove that a source/build route exists and define what must be tested. It does not prove that the route built, ran, produced a correct image, met performance targets, or may ship. Read the current evidence state in [Acceptance](../Acceptance/README.md) and the missing checks in the [Capability Evidence Plan](../Plans/CapabilityEvidence.md).

## Writing And Placement

Use [Documentation Organization](../Engineering/Workflow/DocumentationOrganization.md) for ownership and placement, and the [Documentation Page Template](../Engineering/Workflow/DocumentationPageTemplate.md) for reader-first structure and visual guidance.

# SparkleEngine Documentation

Status: documentation entry point and authority map

Use this page to choose the owning knowledge area. Do not read `Docs` as one linear manual.

Code and executable build configuration prove implemented behavior. Documentation explains intent, design, rules, plans, evidence requirements, and historical context; each document must state which of those it owns.

## Start By Intent

| I need to... | Start here |
| --- | --- |
| understand the product direction or priorities | [Strategy](Strategy/README.md) |
| understand the current repository, one module, or a system design | [Architecture](Architecture/README.md) |
| find what a capability supports, how it works end to end, or what remains unknown | [Module Capability Inventory](Architecture/Modules/README.md), then [Capability Documentation Review](Engineering/Workflow/CapabilityReview.md) |
| implement a repository change | [Change Integration](Engineering/Workflow/ChangeIntegration.md), then the [Engineering task map](Engineering/README.md#choose-by-task) |
| open or close a release iteration | [Change Lifecycle control record](Engineering/Workflow/ChangeLifecycle.md#create-the-iteration-control-record), then the [Roadmap traceability table](Strategy/Roadmap.md#stage-target-and-evidence-traceability) |
| review a changelist | [Code Review](Engineering/Workflow/CodeReview.md) |
| define or review one feature's completion contract | the owning [Architecture](Architecture/README.md) feature dossier |
| track workload, feature-report, or release completion | [Acceptance](Acceptance/README.md) |
| execute an approved multi-step delivery | [Plans](Plans/README.md) |
| inspect external precedent, option analysis, or historical migration evidence | [Research](Research/README.md) |

## Knowledge Areas

| Area | Owns | Must not own |
| --- | --- | --- |
| [Strategy](Strategy/README.md) | desired capabilities, priorities, roadmap, operating model, dated assessments | implementation rules or system internals |
| [Architecture](Architecture/README.md) | module-oriented current maps, capability inventories, canonical decisions, cross-module and target system shape, and feature-local acceptance contracts | phase sequencing, general coding rules, release evidence, or external precedent |
| [Engineering](Engineering/README.md) | task-oriented workflow, foundations, module rules, verification, and technical decision records | release scope, system design, or research |
| [Acceptance](Acceptance/README.md) | shared completion vocabulary, workload/release gates, feature-report orchestration, and high-level progress | feature design or duplicate feature-local criteria, failure modes, and checks |
| [Plans](Plans/README.md) | ordered delivery phases, dependencies, stop conditions, migration ledgers | enduring architecture or claims of completion |
| [Research](Research/README.md) | source-backed precedent, explorations, visual studies, dated migration baselines | local decisions, rules, or evidence grades |

## Short Reviewer Paths

### External Technical Review

1. [Advanced Graphics Engine Executive Summary](Strategy/ExecutiveSummary.md)
2. [Whole Repository Architecture Map](Architecture/WholeRepositoryMap.md)
3. [Current Capability Inventory](Architecture/Modules/README.md)
4. [Engineering task map](Engineering/README.md#choose-by-task)
5. [Acceptance](Acceptance/README.md)

### Owned Repository Change

1. Read [Change Integration](Engineering/Workflow/ChangeIntegration.md) and [Change Lifecycle](Engineering/Workflow/ChangeLifecycle.md).
2. Select every applicable foundation, module, and verification document from the [Engineering task map](Engineering/README.md#choose-by-task).
3. Read the affected module route or cross-module route from [Architecture](Architecture/README.md).
4. Use [Plans](Plans/README.md) only when an approved plan owns the requested sequence.
5. Use the owning Architecture feature dossier for feature-local criteria and [Acceptance](Acceptance/README.md) for affected workload, report, and release claims.

## Documentation Rules

The binding placement, module hierarchy, naming, status, granularity, navigation, linking, and lifecycle rules are in [Documentation Organization](Engineering/Workflow/DocumentationOrganization.md). Area indexes route readers; they do not duplicate the documents they list.

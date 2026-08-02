# SparkleEngine Engineering Standards

Status: standards index and applicability map

This directory contains repository-wide implementation guardrails. Each subject has one owner; standards link to architecture and strategy rather than restating them.

Start with [L. Integration Style Guide](IntegrationStyleGuide.md), then add the documents matching the touched ownership path.

## Standards Map

| Responsibility | Authoritative document | Apply when |
| --- | --- | --- |
| Integration invariants and standards routing | [Integration Style Guide](IntegrationStyleGuide.md) | Every owned repository change |
| Change preparation, review, and completion | [Change Process](ChangeProcess.md) | Every material change |
| Modules, dependencies, APIs, authority, lifetime | [Repository Structure and Ownership](RepositoryStructureAndOwnership.md) | Changing responsibility or dependency boundaries |
| Language, formatting, files, headers, includes, namespaces | [Coding Style](CodingStyle.md) | Writing or reorganizing owned source |
| Identifiers and cross-boundary terminology | [Naming and Vocabulary](NamingAndVocabulary.md) | Naming or renaming concepts/contracts |
| Data layout, identity, projections, memory | [Data-Oriented Design](DataOrientedDesign.md) | Changing storage, packets, caches, hot paths, or uploads |
| Tasks, threads, queues, atomics, locks, publication | [Concurrency](Concurrency.md) | Changing concurrent work or lifetime edges |
| World, ECS, systems, loading, extraction | [GameFramework and ECS](GameFrameworkAndEcs.md) | Touching GameFramework ownership |
| Persistent GPU data, shaders/kernels, captures, hardware/driver work | [Graphics Engineering](GraphicsEngineering.md) | Changing graphics implementation or evidence |
| Editor, UI, cooking, import, capture, tools | [Editor and Tools](EditorAndTools.md) | Touching editor/tool ownership |
| Correctness, diagnostics, tests, performance evidence | [Validation, Performance, and Evidence](ValidationPerformanceAndEvidence.md) | Designing or reporting proof |

The canonical principal-level capability and evidence target is [`PGE-01` through `PGE-15`](../../Strategy/Requirements.md). The [engineer persona](../../Strategy/EngineerPersona.md) describes the operating model. Neither is duplicated as an engineering standard.

## Reading Rule

Every material change starts with [Integration Style Guide](IntegrationStyleGuide.md) and [Change Process](ChangeProcess.md), then selects the rows matching the touched concerns. Source changes normally require Coding Style; responsibility/dependency changes require Repository Structure and Ownership; behavior or evidence claims require Validation, Performance, and Evidence. Add naming, DOD, concurrency, and domain standards only when their subject is touched.

Documentation-only changes apply the same ownership and routing rules without pretending that unrelated code/domain gates are applicable.

## Authority Model

`MUST`, `SHOULD`, and `MAY` have their ordinary normative meanings: acceptance requirement, evidence-backed default, and optional choice within existing constraints.

Executable configuration owns mechanically enforced behavior:

- [`.clang-format`](../../../.clang-format) owns formatting;
- [`.clang-tidy`](../../../.clang-tidy) and compiler configuration own configured diagnostics;
- module `CMakeLists.txt` files and [`ArchitectureBoundaryCheck.cmake`](../../../CMake/ArchitectureBoundaryCheck.cmake) own actual module membership and dependency legality.

Prose explains intent and semantic rules. If prose and executable behavior drift, report and reconcile both in one explicit standards change. Existing code is precedent only when it follows current authority. External repositories are evidence, never Sparkle authority.

## Maintaining Standards

Follow the documentation ownership and folder rules in the [documentation root](../../README.md). For standards specifically:

- pair a mechanical policy change with executable configuration and migration validation;
- add a standards file only for an independent implementation concern that cannot remain a navigable section of an existing owner;
- update this map and affected links whenever a standard is added, moved, merged, or retired.

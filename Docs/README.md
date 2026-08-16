# SparkleEngine Documentation

Status: documentation root and authority map

SparkleEngine documentation is organized by responsibility, not by chronology or author.

## Areas

| Area | Owns | Does not own |
| --- | --- | --- |
| [Strategy](Strategy/README.md) | capability targets, priorities, dated assessments, roadmaps, persona | implementation rules or system design |
| [Architecture](Architecture/WholeRepositoryMap.md) | current-system maps, target designs, decisions, rationale, external architecture research | coding conventions or evidence grades |
| [Engineering](Engineering/README.md) | binding implementation standards, decision briefs, validation contracts | product strategy or duplicate architecture descriptions |

Code and build configuration remain the authority for implemented behavior. Documentation must identify whether it is a standard, canonical decision, target proposal, plan, snapshot, research reference, or archive.

## Reviewer Paths

### External Technical Review

1. [Advanced Graphics Engine Executive Summary](Strategy/ExecutiveSummary.md)
2. [Whole Repository Architecture Map](Architecture/WholeRepositoryMap.md)
3. [Renderer and RHI Architecture Boundary](Architecture/RendererRhiBoundary.md)
4. [Engineering Standards](Engineering/Standards/README.md)
5. [Bistro and San Miguel Acceptance Workloads](Engineering/BistroAndSanMiguelWorkloads.md)

An external reviewer should follow code, captures, tests, and measurements from these entry points rather than read every planning or research document.

### Implementing a Change

1. Start with [L. Integration Style Guide](Engineering/Standards/IntegrationStyleGuide.md).
2. Select the applicable focused standards from the [standards map](Engineering/Standards/README.md#standards-map).
3. Read the relevant canonical architecture decision.
4. Use the acceptance workload only when the change affects its gates.

### Principal Graphics Planning

Start with the [Strategy index](Strategy/README.md). Requirements, assessment, roadmap, persona, and source archive are planning/audit material; they are not all part of the external reviewer path.

### Multithreading Work

Use [J. Multithreaded Engine Architecture](Architecture/Multithreading/MultithreadedEngineArchitecture.md) for the concurrency target design and the [Principal Graphics Roadmap](Strategy/Roadmap.md) for broader sequencing.

### Performance Diagnostics Work

Start with [Diagnostics Product and UX Research](Architecture/Performance/Diagnostics/DiagnosticsUxResearch.md) for the Epic/NVIDIA/AMD precedent and requirements audit. Use [Performance Diagnostics Visual Design And Tool Wireframes](Architecture/Performance/Diagnostics/PerformanceDiagnosticsAsciiWireframes.md) for the mockups and tool layouts, then [Performance Diagnostics Architecture](Architecture/Performance/Diagnostics/PerformanceDiagnosticsArchitecture.md) for the selected product, metric, ownership, evidence, and stable profiler-handoff contracts. Use the feature-selectable [Performance Diagnostics Delivery Plan](Architecture/Performance/Diagnostics/ImplementationPlan.md) for ordered implementation. Before taking an external capture, use the version-sensitive [External Performance Profiler Runbook](Architecture/Performance/Diagnostics/DiagnosticsProfilerRunbook.md). [Bistro and San Miguel Acceptance Workloads](Engineering/BistroAndSanMiguelWorkloads.md) remains the authority for benchmark gates and case-study deliverables.

## Document Status Vocabulary

- **Canonical** — current authoritative decision for its named subject.
- **Standard** — binding repository implementation/review rule.
- **Target proposal** — intended architecture not automatically proven implemented.
- **Plan** — sequencing and work definition; not proof of completion.
- **Snapshot** — dated observation that must be revalidated before use.
- **Research** — source-backed precedent or comparison; never local authority by itself.
- **Archive** — retained traceability/history; not a current entry point.
- **Summary** — orientation only; owning documents control conflicts.

## Maintenance Rules

- Keep one owner for each decision or rule; link instead of paraphrasing it elsewhere.
- Put current state in maps, intended system shape in architecture, implementation rules in standards, evidence gates in validation, and priorities/sequencing in strategy.
- Give every document a status and one named responsibility; state an authority boundary wherever another document could be mistaken for the owner.
- Add a file only when it has an independent audience and reason to change; otherwise add a navigable section to its owner.
- Create a subfolder only when multiple documents share a durable local context; add a local index only when the parent index cannot route them clearly. Keep a single clearly named document at the area root.
- Keep indices short and update them when files move, merge, or retire.
- Preserve stable A–L document IDs where they remain useful cross-document references.

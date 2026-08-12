# Architecture

Status: architecture index

Architecture documents explain SparkleEngine's implemented shape, accepted system decisions, target designs, and the evidence behind them. They do not own coding style, change workflow, principal-capability grades, or validation workloads.

## Document Map

| Responsibility | Document | Status |
| --- | --- | --- |
| Current repository/module/data-flow map | [D. Whole Repository Architecture Map](WholeRepositoryMap.md) | dated source-backed snapshot |
| External renderer/SDK comparison | [E. External Renderer Repository Comparison](ExternalRendererComparison.md) | research reference |
| Epic/NVIDIA/AMD diagnostics product and UI/UX comparison | [Diagnostics Product And UX Research](DiagnosticsUxResearch.md) | research reference |
| Performance diagnostics visual mockups and system-scope map | [Performance Diagnostics Visualizations](DiagnosticsVisualizations.md) | design visualization; not implementation evidence |
| Individual performance diagnostics tool layouts | [Performance Diagnostics ASCII Tool Wireframes](PerformanceDiagnosticsAsciiWireframes.md) | design visualization; not implementation evidence |
| Renderer/RHI dependency and ownership boundary | [Renderer and RHI Architecture Boundary](RendererRhiBoundary.md) | canonical architecture decision |
| Live stat views, GPU Visualizer, CPU/GPU/memory diagnostics, and profiler correlation | [Performance Diagnostics Architecture](PerformanceDiagnosticsArchitecture.md) | target proposal; implementation must still be proven |
| Multithreaded target architecture and rationale | [J. Multithreaded Engine Architecture](Multithreading/MultithreadedEngineArchitecture.md) | canonical concurrency target; implementation must still be proven |
| Ordered multithreading implementation work | [K. Multithreaded Engine Implementation Plan](Multithreading/ImplementationPlan.md) | internal concurrency execution plan |

## Reading Paths

For external technical review, read the repository map and Renderer/RHI boundary, then inspect the relevant code. Read external comparison only to audit precedent.

For multithreading work, [J](Multithreading/MultithreadedEngineArchitecture.md) owns the target design and rationale while [K](Multithreading/ImplementationPlan.md) owns sequencing. Neither replaces [engineering standards](../Engineering/Standards/README.md) or proves current implementation.

For performance diagnostics work, read [Diagnostics Product And UX Research](DiagnosticsUxResearch.md) to audit the product precedents and option space, use [Performance Diagnostics Visualizations](DiagnosticsVisualizations.md) for the proposed overall experience and [Performance Diagnostics ASCII Tool Wireframes](PerformanceDiagnosticsAsciiWireframes.md) for individual tool layouts, then [Performance Diagnostics Architecture](PerformanceDiagnosticsArchitecture.md) for the selected information architecture, metric semantics, ownership, fixed `Stat` views, focused GPU capture, live presentation, and external-profiler correlation. Use the acceptance workload for benchmark and evidence gates.

## Architecture Document Rules

- State whether described behavior is current, target, historical, or rejected.
- Link general implementation rules to Engineering Standards rather than copying them.
- Keep schedules, completion reports, and portfolio narratives out of canonical architecture decisions.
- Mark external-precedent documents as research and state exactly what was and was not adopted.
- Revalidate snapshot claims against code before using them for a change.
- Update the nearest index whenever an architecture document is added, moved, or retired.

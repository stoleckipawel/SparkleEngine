# Architecture

Status: architecture index

Architecture documents explain SparkleEngine's implemented shape, accepted system decisions, target designs, and the evidence behind them. They do not own coding style, change workflow, principal-capability grades, or validation workloads.

## Document Map

| Responsibility | Document | Status |
| --- | --- | --- |
| Current repository/module/data-flow map | [D. Whole Repository Architecture Map](WholeRepositoryMap.md) | dated source-backed snapshot |
| External renderer/SDK comparison | [E. External Renderer Repository Comparison](ExternalRendererComparison.md) | research reference |
| End-to-end shader authoring, compilation, cooking, runtime use, pipeline preparation, debugging, and requirements traceability | [Shader Authoring and Cooked Program Architecture](ShaderAuthoringAndCookedPrograms.md) | target proposal and current compliance audit; implementation must still be proven |
| Epic/NVIDIA/AMD diagnostics product and UI/UX comparison | [Diagnostics Product And UX Research](DiagnosticsUxResearch.md) | research reference |
| Performance diagnostics mockups, system-scope map, and individual tool layouts | [Performance Diagnostics Visual Design And Tool Wireframes](PerformanceDiagnosticsAsciiWireframes.md) | design visualization; not implementation evidence |
| Renderer/RHI dependency and ownership boundary | [Renderer and RHI Architecture Boundary](RendererRhiBoundary.md) | canonical architecture decision |
| Live stat views, GPU Visualizer, CPU/GPU/memory diagnostics, and profiler correlation | [Performance Diagnostics Architecture](PerformanceDiagnosticsArchitecture.md) | target proposal; implementation must still be proven |
| Version-sensitive external-profiler capability, capture preparation, marker interoperability, and operational checks | [External Performance Profiler Runbook](DiagnosticsProfilerRunbook.md) | operational research/runbook; revalidate before use |
| Multithreaded target architecture and rationale | [J. Multithreaded Engine Architecture](Multithreading/MultithreadedEngineArchitecture.md) | canonical concurrency target; implementation must still be proven |
| Ordered multithreading implementation work | [K. Multithreaded Engine Implementation Plan](Multithreading/ImplementationPlan.md) | internal concurrency execution plan |

## Reading Paths

For external technical review, read the repository map and Renderer/RHI boundary, then inspect the relevant code. Read external comparison only to audit precedent.

For shader authoring, cooking, runtime loading, pipeline, or shader-debugging changes, read [Shader Authoring and Cooked Program Architecture](ShaderAuthoringAndCookedPrograms.md) for the target identity model, current end-to-end audit, requirements traceability, evidence contract, and migration gates. Then verify the current shader registry, compiler, cooked schema, runtime cache, pass definitions, and backend pipeline creation in code. The target document does not prove that manual pass/package strings, first-use pipeline creation, or other recorded gaps have already been removed.

For multithreading work, [J](Multithreading/MultithreadedEngineArchitecture.md) owns the target design and rationale while [K](Multithreading/ImplementationPlan.md) owns sequencing. Neither replaces [engineering standards](../Engineering/Standards/README.md) or proves current implementation.

For performance diagnostics work, read [Diagnostics Product And UX Research](DiagnosticsUxResearch.md) to audit the product precedents and option space, use [Performance Diagnostics Visual Design And Tool Wireframes](PerformanceDiagnosticsAsciiWireframes.md) for the proposed overall experience and individual tool layouts, then [Performance Diagnostics Architecture](PerformanceDiagnosticsArchitecture.md) for the selected information architecture, metric semantics, ownership, fixed `Stat` views, focused GPU capture, live presentation, and stable profiler handoff. Before an external capture, revalidate the current tool path in the [External Performance Profiler Runbook](DiagnosticsProfilerRunbook.md). Use the acceptance workload for benchmark and evidence gates.

## Architecture Document Rules

- State whether described behavior is current, target, historical, or rejected.
- Link general implementation rules to Engineering Standards rather than copying them.
- Keep schedules, completion reports, and portfolio narratives out of canonical architecture decisions.
- Mark external-precedent documents as research and state exactly what was and was not adopted.
- Revalidate snapshot claims against code before using them for a change.
- Update the nearest index whenever an architecture document is added, moved, or retired.

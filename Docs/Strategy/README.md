# Principal Graphics Strategy

Status: strategy index and responsibility map

Strategy owns desired capabilities, priority, dated assessment, sequencing, and the target professional operating model. It does not own code rules, subsystem architecture, or workload-specific proof gates.

## Entry Points

- Orientation: [G. Advanced Graphics Engine Executive Summary](ExecutiveSummary.md)
- Canonical capability/evidence target: [A. Principal Graphics Engineering Requirements](Requirements.md)
- Target operating model: [H. Advanced Graphics Engineer Persona](EngineerPersona.md)
- Current sequence: [F. Release-First Principal Graphics Roadmap](Roadmap.md)
- First-release proof gates: [First Release Acceptance Contract](../Engineering/FirstReleaseAcceptance.md)
- Executable workload gates: [I. Bistro and San Miguel Acceptance Workloads](../Engineering/BistroAndSanMiguelWorkloads.md)

## Planning and Audit Material

| Document | Responsibility | Status/use |
| --- | --- | --- |
| [B. Role Source Archive](RoleSourceArchive.md) | normalized source trace for the canonical requirements | archive; not an external entry point |
| [C. Candidate and Repository Gap Assessment](GapAssessment.md) | evidence/readiness at one repository snapshot | dated assessment; revalidate before acting |
| [F. Release-First Principal Graphics Roadmap](Roadmap.md) | first-release sequence and retained post-release graphics plan | plan; not proof of completion |
| [Repository Quality and Complexity Executive Assessment](RepositoryQualityAndComplexityAssessment.md) | current-feature structural quality and prioritized refactoring direction at one repository snapshot | dated assessment; preserve features and revalidate before acting |

Stable IDs D and E are architecture documents routed from the [Whole Repository Architecture Map](../Architecture/WholeRepositoryMap.md). J is the multithreading architecture/execution contract, and L is the engineering integration contract.

## Ownership Rules

- Requirements owns `PGE-*` definitions and evidence meaning.
- Executive Summary orients; it does not create new requirements.
- Gap Assessment records a dated state; it does not define target architecture.
- Repository Quality and Complexity Assessment records a dated structural/refactoring view; it does not override feature scope, roadmap sequence, or engineering standards.
- Roadmap owns sequence, not implementation rules or workload definitions.
- The [First Release Acceptance Contract](../Engineering/FirstReleaseAcceptance.md) owns release feature, package, clean-machine, and publication proof; it does not set priority.
- Persona owns behavior and judgment, not a duplicate evidence matrix.
- Role Source Archive preserves traceability and should not leak into product-facing claims.
- The [acceptance workload](../Engineering/BistroAndSanMiguelWorkloads.md) owns scene and proof gates; the [Whole Repository Architecture Map](../Architecture/WholeRepositoryMap.md) routes system decisions.

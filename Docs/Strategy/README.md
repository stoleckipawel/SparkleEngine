# Principal Graphics Strategy

Status: strategy index and responsibility map

Strategy owns desired capabilities, priority, dated assessment, sequencing, and the target professional operating model. It does not own code rules, subsystem architecture, or workload-specific proof gates.

## Entry Points

- Orientation: [G. Advanced Graphics Engine Executive Summary](ExecutiveSummary.md)
- Canonical capability/evidence target: [A. Principal Graphics Engineering Requirements](Requirements.md)
- Target operating model: [H. Advanced Graphics Engineer Persona](EngineerPersona.md)
- Executable workload gates: [I. Bistro and San Miguel Acceptance Workloads](../Engineering/BistroAndSanMiguelWorkloads.md)

## Planning and Audit Material

| Document | Responsibility | Status/use |
| --- | --- | --- |
| [B. Role Source Archive](RoleSourceArchive.md) | normalized source trace for the canonical requirements | archive; not an external entry point |
| [C. Candidate and Repository Gap Assessment](GapAssessment.md) | evidence/readiness at one repository snapshot | dated assessment; revalidate before acting |
| [F. Six-to-Twelve-Month Roadmap](Roadmap.md) | priority, sequencing, allocation, and gates | plan; not proof of completion |

Stable IDs D and E are architecture documents in the [Architecture index](../Architecture/README.md). J and K are multithreading architecture/execution documents, and L is the engineering integration contract.

## Ownership Rules

- Requirements owns `PGE-*` definitions and evidence meaning.
- Executive Summary orients; it does not create new requirements.
- Gap Assessment records a dated state; it does not define target architecture.
- Roadmap owns sequence, not implementation rules or workload definitions.
- Persona owns behavior and judgment, not a duplicate evidence matrix.
- Role Source Archive preserves traceability and should not leak into product-facing claims.
- The [acceptance workload](../Engineering/BistroAndSanMiguelWorkloads.md) owns scene and proof gates; [Architecture](../Architecture/README.md) owns system decisions.

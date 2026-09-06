# Principal Graphics Strategy

Status: strategy index and responsibility map

Strategy owns desired capabilities, priority, release-wide sequencing, dated executive assessments, and the target professional operating model. It does not own implementation rules, subsystem design, or completion proof.

## Current Direction

- [G. Advanced Graphics Engine Executive Summary](ExecutiveSummary.md) — compact orientation and product identity.
- [A. Principal Graphics Engineering Requirements](Requirements.md) — canonical `PGE-*` capability and evidence target.
- [F. Release-First Principal Graphics Roadmap](Roadmap.md) — current release sequence, work-in-progress limits, and stop rules.
- [H. Advanced Graphics Engineer Persona](EngineerPersona.md) — target operating model and judgment standard.
- [Capability Coverage Crosswalk](../Architecture/CrossModule/StrategyCoverage.md) — dated source-backed mapping from current module capabilities to the persona, `PGE-*` requirements, roadmap release surfaces, and refreshed gap observations.

## Dated Assessments

- [C. Candidate And Repository Gap Assessment](Assessments/GapAssessment.md) — evidence/readiness at one repository snapshot.
- [Repository Quality And Complexity Assessment](Assessments/RepositoryQualityAndComplexity.md) — feature-preserving structural quality and prioritized refactoring direction at one snapshot.

Assessments must be revalidated before acting; they do not silently become current architecture or implementation status.

## Archive

- [B. Role Sources](RoleSources.md) — normalized source trace for the canonical requirements; retained for audit, not a default reviewer path.

## Neighboring Authorities

- [Architecture](../Architecture/README.md) owns current maps, decisions, and system shape.
- [Architecture](../Architecture/README.md) owns feature-local proof contracts; [Acceptance](../Acceptance/README.md) owns candidate reports, workload/release gates, and high-level progress.
- [Engineering guidance](../Engineering/README.md#choose-by-task) owns implementation rules and routes them by task.
- [Plans](../Plans/README.md) own subsystem delivery sequences; the release-wide roadmap remains here because it sets product priority and ordering.

# SparkleEngine Documentation

This directory is the navigation root for SparkleEngine's design, engineering, and planning documents.

## Documentation Areas

- [Architecture](Architecture/README.md) — repository structure, external research, and subsystem architecture.
- [Engineering](Engineering/README.md) — implementation standards and executable validation contracts.
- [Strategy](Strategy/README.md) — requirements, assessments, roadmaps, and engineering direction.

## Recommended Entry Points

- For a short project-level decision summary, start with [G. Advanced Graphics Engine Executive Summary](Strategy/PrincipalGraphics/ExecutiveSummary.md).
- To understand the current repository, read [D. Whole Repository Architecture Map](Architecture/Repository/WholeRepositoryMap.md).
- For Renderer/RHI ownership and backend parity, read [Renderer and RHI Boundary](Architecture/RHI/RendererRhiBoundary.md).
- Before changing code, apply [L. SparkleEngine Integration Style Guide](Engineering/Standards/IntegrationStyleGuide.md).
- For multithreading work, use the [multithreading reading guide](Architecture/Multithreading/README.md).
- For acceptance evidence, use [I. Bistro and San Miguel Acceptance Workloads](Engineering/Validation/BistroAndSanMiguelWorkloads.md).

## Organization Rules

- Put a document under the area that owns its subject; do not create temporary catch-all folders such as `Review`, `Misc`, or `New`.
- Give files descriptive names. Use ordering prefixes only when order is part of a durable contract.
- Add new documents to the nearest area index so they remain discoverable.
- Keep cross-document links relative and update them when a document moves.
- The A–L labels in the current document set are stable document IDs, not folder-order prefixes.

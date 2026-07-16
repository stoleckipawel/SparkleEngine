# Sparkle Documentation

This folder keeps active architecture guidance. Old review notes, superseded staged plans, and standalone validation notes are intentionally removed unless they directly support the current renderer/productization work.

## Architecture Review Source Set

Use these as source material behind the implementation spine:

- `Architecture/00-Review/D_WholeRepositoryArchitectureMap.md`: full repository map covering modules, code weight, public/private API shape, runtime flow, tools, memory, GPU/CPU surfaces, and cleanup-grade architecture scores.
- `Architecture/00-Review/E_ExternalRendererRepositoryComparison.md`: source-linked comparison against vendor renderer frameworks, RHIs, SDKs, sample frameworks, and branch/productization patterns.
- `Architecture/00-Review/F_StagedDeletionFirstImprovementPlan.md`: staged improvement plan that favors net code and depot-size removal while preserving fatal guardrails and graphics profiler/debugger support.
- `Architecture/00-Review/G_AdvancedGraphicsEngineExecutiveSummary.md`: executive summary of advanced graphics requirements, priorities, skips, and wording to avoid.
- `Architecture/00-Review/H_AdvancedGraphicsEngineerPersona.md`: neutral persona target for the advanced graphics engineer Sparkle should help develop and demonstrate.
- `Architecture/00-Review/I_GameFrameworkRendererRhiResponsibilityExecutiveSummary.md`: source-backed current-state audit and ordered implementation decision for GameFramework, Renderer, and RHI ownership, lifetime, and data flow.

## Review Background

- `Architecture/00-Review/A_PrincipalRenderingRequirements.md`: retained only as background for principal-level rendering expectations.

Keep new docs only when they describe an active implementation prompt, a source-backed contract, or a reference-backed rendering decision that the source cannot make obvious by itself.

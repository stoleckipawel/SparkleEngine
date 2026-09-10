# SparkleEngine Agent Guide

Scope: the entire repository. This file is a routing layer; it does not own or duplicate engineering standards.

## Start Here

- Inspect `git status --short`, the current revision, and every `AGENTS.md` from the repository root to the working directory before material work. Treat pre-existing and concurrently appearing changes as user-owned unless the task clearly says otherwise.
- Read [`Docs/README.md`](Docs/README.md) before a material change. It defines documentation authority, status, and the shortest reviewer paths.
- For an owned repository change, apply [Change Integration](Docs/Engineering/Workflow/ChangeIntegration.md), follow [Change Lifecycle](Docs/Engineering/Workflow/ChangeLifecycle.md), and select every applicable foundation, module, and verification document from the [Engineering task map](Docs/Engineering/README.md#choose-by-task).
- Read the relevant route from the [architecture map](Docs/Architecture/WholeRepositoryMap.md) before changing a responsibility or dependency boundary. Consult [`Strategy`](Docs/Strategy/README.md) and acceptance workloads only when the task affects their targets or evidence gates.
- Code and executable build configuration prove implemented behavior. Interpret every document according to its declared status.

## Choose The Owning Route

| Task | Required route and authority boundary |
| --- | --- |
| Explain, analyze, diagnose, or review | Inspect current code/build configuration and the nearest owning Architecture route, then report evidence and limitations. Treat the task as read-only unless the user explicitly asks for changes. |
| Research, design, or plan a substantial feature | Follow [Documentation Organization](Docs/Engineering/Workflow/DocumentationOrganization.md) and use the [Feature Delivery Documentation Package](Docs/Engineering/Workflow/Templates/FeatureDeliveryPackage.md). Keep research as precedent, discovery as the decision gate, architecture/semantics/UX as target contracts, and the plan as delivery order; none proves implementation. Use the [single-page template](Docs/Engineering/Workflow/Templates/Page.md) when those roles do not need independent lifecycles. |
| Implement, refactor, or fix | Start from the accepted Architecture owner and execute one coherent production slice under Change Integration, Change Lifecycle, and every subject standard selected by the Engineering task map. Update code, build membership, generated surfaces, checks, and directly affected documentation together. |
| Validate, accept, or prepare release evidence | Follow [Validation And Evidence](Docs/Engineering/Verification/ValidationAndEvidence.md) and the applicable [Acceptance](Docs/Acceptance/README.md) route. Bind every result to the exact candidate, configuration, command/workflow, oracle, observation, and artifact; record `PASS`, `BLOCKED`, `EXCLUDED`, or `SUPERSEDED` only in the owning report. |

Strategy owns desired outcomes and priorities. Architecture owns current/target system knowledge and feature-local research, plans, and proof contracts. Engineering owns how work is performed. Acceptance owns candidate and release results. Link to these owners instead of creating a second authority in prompts, comments, or summaries.

## Working Agreements

- Inspect the current owner, producers, consumers, lifetime, and build membership before editing. Search the repository with `rg` and `rg --files` before adding or renaming a concept.
- For feature work, enforce the [feature-enclosure and integration-hook budget](Docs/Engineering/Foundations/ModuleOwnership.md#feature-enclosure-and-integration-hook-budget) at every implementation stage. Keep mechanism and feature state in one predictable owner; treat every feature-named edit outside it as an integration hook that requires ledgered justification and a defect-detecting check.
- Apply the [single-truth and copy budget](Docs/Engineering/Foundations/DataAndMemory.md#single-truth-and-copy-budget) before adding a data holder or snapshot; prefer references, views, handles, and moves unless a real boundary requires a copy.
- Extend the existing owner and production path. Follow the current [clean-break policy](Docs/Engineering/Workflow/ChangeIntegration.md#current-clean-break-policy): update every producer and consumer, delete the replaced path in the same change, and regenerate local artifacts. Do not add internal versioning, legacy paths, migration readers/writers, compatibility adapters, aliases, or dual representations.
- Keep changes scoped, preserve unrelated work, and recheck the revision, status, and scoped diff before handoff. If concurrent work overlaps the owned path, reconcile it rather than restoring an earlier snapshot.
- When ownership or contracts move, update implementation, headers, CMake membership, validation, and documentation together.
- Follow `.clang-format`, `.clang-tidy`, compiler settings, and module `CMakeLists.txt` files as executable policy. Do not restate their settings here.
- Treat generated and AI-assisted output as untrusted until it has been reviewed and validated.

## Verification and Handoff

- Follow the claim-driven escalation ladder in [Validation And Evidence](Docs/Engineering/Verification/ValidationAndEvidence.md#claim-driven-validation-selection). Full engine/game/workspace builds, whole validation sets/cooks, clean rebuilds, paired-backend runs, and acceptance workloads are not default checks; run them only when the affected claim or selected gate requires that breadth.
- For documentation changes, apply the [Documentation Organization review checklist](Docs/Engineering/Workflow/DocumentationOrganization.md#review-checklist), including authority, navigation, local links/anchors, current-state evidence, UTF-8, whitespace, and stale-path checks. Documentation validation does not prove build, runtime, GPU, visual, performance, package, or release behavior.
- Run `architecture_boundary_check` when Renderer/RHI boundaries change.
- Run `git diff --check` before handoff.
- Report the exact commands and results, plus any checks that were unavailable. Never imply that an unrun check passed.

## Maintaining Agent Guidance

Keep this file short and repository-specific. Put durable subject rules in their owning document or executable configuration and link to them here only when every agent needs the route. Add a nested `AGENTS.md` only for durable subtree-specific guidance; it must narrow this file without copying it.

# SparkleEngine Agent Guide

Scope: the entire repository. This file is a routing layer; it does not own or duplicate engineering standards.

## Start Here

- Read [`Docs/README.md`](Docs/README.md) before a material change. It defines documentation authority, status, and the shortest reviewer paths.
- For an owned repository change, apply [`IntegrationStyleGuide.md`](Docs/Engineering/Standards/IntegrationStyleGuide.md), follow [`ChangeProcess.md`](Docs/Engineering/Standards/ChangeProcess.md), and select every applicable subject standard from the [`Standards` map](Docs/Engineering/Standards/README.md#standards-map).
- Read the relevant route from the [architecture map](Docs/Architecture/WholeRepositoryMap.md) before changing a responsibility or dependency boundary. Consult [`Strategy`](Docs/Strategy/README.md) and acceptance workloads only when the task affects their targets or evidence gates.
- Code and executable build configuration prove implemented behavior. Interpret every document according to its declared status.

## Working Agreements

- Inspect the current owner, producers, consumers, lifetime, and build membership before editing. Search the repository with `rg` and `rg --files` before adding or renaming a concept.
- Apply the [single-truth and copy budget](Docs/Engineering/Standards/DataOrientedDesign.md#single-truth-and-copy-budget) before adding a data holder or snapshot; prefer references, views, handles, and moves unless a real boundary requires a copy.
- Extend the existing owner and production path. Follow the current [clean-break policy](Docs/Engineering/Standards/IntegrationStyleGuide.md#current-clean-break-policy): update every producer and consumer, delete the replaced path in the same change, and regenerate local artifacts. Do not add internal versioning, legacy paths, migration readers/writers, compatibility adapters, aliases, or dual representations.
- Keep changes scoped and preserve unrelated or uncommitted work already in the tree.
- When ownership or contracts move, update implementation, headers, CMake membership, validation, and documentation together.
- Follow `.clang-format`, `.clang-tidy`, compiler settings, and module `CMakeLists.txt` files as executable policy. Do not restate their settings here.
- Treat generated and AI-assisted output as untrusted until it has been reviewed and validated.

## Verification and Handoff

- Follow the claim-driven escalation ladder in [Validation, Performance, and Evidence](Docs/Engineering/Standards/ValidationPerformanceAndEvidence.md#claim-driven-validation-selection). Full engine/game/workspace builds, whole validation sets/cooks, clean rebuilds, paired-backend runs, and acceptance workloads are not default checks; run them only when the affected claim or selected gate requires that breadth.
- Run `architecture_boundary_check` when Renderer/RHI boundaries change.
- Run `git diff --check` before handoff.
- Report the exact commands and results, plus any checks that were unavailable. Never imply that an unrun check passed.

## Maintaining Agent Guidance

Keep this file short and repository-specific. Put durable subject rules in their owning document or executable configuration and link to them here only when every agent needs the route. Add a nested `AGENTS.md` only for durable subtree-specific guidance; it must narrow this file without copying it.

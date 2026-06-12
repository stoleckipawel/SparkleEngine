# Planning Documentation

Status: navigation index
Date: 2026-06-12
Last synchronized: 2026-06-13

## Purpose

This folder contains planning and review documents for the whole-repository refactor. It is organized to show both the baseline reasoning and the target implementation path.

The refactor is allowed to preserve, rename, split, merge, rebuild, or delete systems. Current structure is useful evidence, but target ownership, data flow, naming, and validation decide what survives.

## Folder Layout

| Area | Use it for |
| --- | --- |
| [before](before/README.md) | Review inputs, current-state reasoning, baseline risks, and acceptance rubric. |
| [after](after/README.md) | Target execution path, stage map, final gate, and implementation-oriented navigation. |
| Root plan docs | Canonical review and execution documents that remain stable links while the foldered navigation grows around them. |

## Primary Reading Paths

Repository-wide architecture:

1. [sparkle-whole-repository-architecture-review.md](sparkle-whole-repository-architecture-review.md)
2. [../architecture/before/repository-current-state.md](../architecture/before/repository-current-state.md)
3. [../architecture/after/repository-target-architecture.md](../architecture/after/repository-target-architecture.md)
4. [../architecture/after/repository-target-folder-architecture.md](../architecture/after/repository-target-folder-architecture.md)
5. [../architecture/after/repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md)

Implementation:

1. [rhi-renderer-review-ready-implementation-plan.md](rhi-renderer-review-ready-implementation-plan.md)
2. [after/repository-refactor-stage-map.md](after/repository-refactor-stage-map.md): stage groups, status tracker, and implementation navigation.
3. [architecture-review-acceptance-rubric.md](architecture-review-acceptance-rubric.md)

## Single Source Of Truth

| Question | Canonical doc |
| --- | --- |
| What is the global refactor vision? | [sparkle-whole-repository-architecture-review.md](sparkle-whole-repository-architecture-review.md) |
| What are the exact implementation prompts and guardrails? | [rhi-renderer-review-ready-implementation-plan.md](rhi-renderer-review-ready-implementation-plan.md) |
| Which target docs must be opened for each stage? | [Required Target Documents By Stage](rhi-renderer-review-ready-implementation-plan.md#required-target-documents-by-stage) |
| What contract surfaces must each stage prove before acceptance? | [Stage Contract Coverage Matrix](rhi-renderer-review-ready-implementation-plan.md#stage-contract-coverage-matrix) |
| Which broad stages have mandatory split checkpoints? | [Mandatory Split Checkpoints For Large Stages](rhi-renderer-review-ready-implementation-plan.md#mandatory-split-checkpoints-for-large-stages) |
| What is each stage status? | [after/repository-refactor-stage-map.md](after/repository-refactor-stage-map.md) |
| What does acceptance mean? | [architecture-review-acceptance-rubric.md](architecture-review-acceptance-rubric.md) |
| What is the target architecture and folder layout? | [../architecture/after/repository-target-architecture.md](../architecture/after/repository-target-architecture.md) and [../architecture/after/repository-target-folder-architecture.md](../architecture/after/repository-target-folder-architecture.md) |
| What keeps future multithreading easy instead of risky? | [../architecture/after/repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md) |

RHI/Renderer first track:

1. [rhi-renderer-architecture-review.md](rhi-renderer-architecture-review.md)
2. [../architecture/rendering-system-map.md](../architecture/rendering-system-map.md)
3. [../architecture/rendering-coverage-status.md](../architecture/rendering-coverage-status.md)

## Maintenance Rule

New planning docs should be linked from this index and from either [before](before/README.md) or [after](after/README.md). Avoid adding unlinked plan files to the root.

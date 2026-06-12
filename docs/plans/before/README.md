# Before Plans

Status: baseline planning navigation
Date: 2026-06-12
Last synchronized: 2026-06-13

## Purpose

This folder points to the review material that explains why the refactor exists: current architecture, baseline risks, comparison sources, and acceptance criteria.

Baseline documents describe reality, not a preservation mandate. Use them to decide whether a system should be kept and refined, improved and extracted, or replaced/redesigned.

## Baseline Inputs

| Document | Role |
| --- | --- |
| [../sparkle-whole-repository-architecture-review.md](../sparkle-whole-repository-architecture-review.md) | Global current-state review and target tracks for every repository area. |
| [../rhi-renderer-architecture-review.md](../rhi-renderer-architecture-review.md) | Detailed first-track RHI/Renderer architecture review. |
| [../architecture-review-acceptance-rubric.md](../architecture-review-acceptance-rubric.md) | Scoring criteria used to judge proposals and final acceptance. |
| [../../architecture/before/repository-current-state.md](../../architecture/before/repository-current-state.md) | Current broad module interactions and detailed baseline risks. |
| [../../architecture/before/repository-current-graphs.md](../../architecture/before/repository-current-graphs.md) | Current module, graphics, content, host, boundary, and threading-risk graphs. |
| [../../architecture/after/repository-threading-readiness.md](../../architecture/after/repository-threading-readiness.md) | Target comparison for mutable ownership, immutable handoffs, command batches, jobs, and reports. |
| [../../architecture/repository-coverage-status.md](../../architecture/repository-coverage-status.md) | Current source-root status map. |
| [../../architecture/rendering-coverage-status.md](../../architecture/rendering-coverage-status.md) | Current detailed RHI/Renderer status map. |

## Baseline Questions

- What does the repository do today?
- Which dependencies are expected, risky, or transitional?
- Which modules can be damaged by a localized RHI/Renderer refactor?
- Which acceptance criteria must be satisfied before a stage is accepted?
- Which external NVIDIA/AMD/Khronos/CMake/Qt references are we using as design anchors?
- Which current edges would block future worker threads, render-thread work, async queues, cook jobs, shader jobs, or launcher process workflows?

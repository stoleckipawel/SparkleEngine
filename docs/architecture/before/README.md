# Before Architecture

Status: current-state navigation
Date: 2026-06-12
Last synchronized: 2026-06-13

## Purpose

This folder captures the architecture before the whole-repository refactor is complete. "Before" does not mean every system is wrong; it means this is the baseline reality and risk map that future stages must preserve or improve.

## Read First

- [repository-current-state.md](repository-current-state.md): broad current module interactions, source-root shape, and detailed current risks.
- [repository-current-graphs.md](repository-current-graphs.md): current-state module, graphics, content, host, and risk graphs.

## Related Baseline Documents

| Document | Why it belongs to the before view |
| --- | --- |
| [../../plans/sparkle-whole-repository-architecture-review.md](../../plans/sparkle-whole-repository-architecture-review.md) | Repository-wide review of current strengths, risks, and target tracks. |
| [../../plans/rhi-renderer-architecture-review.md](../../plans/rhi-renderer-architecture-review.md) | First-track detailed RHI/Renderer audit and baseline findings. |
| [../repository-coverage-status.md](../repository-coverage-status.md) | Current source-root coverage and owner/risk status. |
| [../rendering-coverage-status.md](../rendering-coverage-status.md) | Current detailed RHI/Renderer coverage and risk status. |
| [../architecture-boundary-guardrails.md](../architecture-boundary-guardrails.md) | Current mechanical guardrails and transitional exceptions. |
| [../after/repository-threading-readiness.md](../after/repository-threading-readiness.md) | Target comparison for current mutable-state, handoff, queue, job, and diagnostics risks. |

## What This Folder Answers

- What systems currently depend on each other?
- What folder/source-root shape exists before the target refactor?
- Which current relationships are expected, risky, or transitional?
- What must not regress while RHI/Renderer refactors continue?
- Which modules require follow-up design or validation before final acceptance?
- Which current edges need snapshot, DTO, command-batch, queue-packet, job-request, or report contracts before future multithreading?

# Implementation Prompt Track

Status: implementation-facing navigation
Date: 2026-06-13

## Purpose

This folder is optimized for Codex implementation sessions. It does not replace the canonical plan; it repackages the same architecture into prompt-ready checklists, stage packets, validation gates, and context rules.

Use this folder when the next action is implementation, validation, or cleanup.

Canonical source of truth:

- [../rhi-renderer-review-ready-implementation-plan.md](../rhi-renderer-review-ready-implementation-plan.md)
- [../architecture-review-acceptance-rubric.md](../architecture-review-acceptance-rubric.md)
- [../../architecture/after/system-design-index.md](../../architecture/after/system-design-index.md)
- [../../architecture/after/repository-threading-readiness.md](../../architecture/after/repository-threading-readiness.md)

## Documents

| Document | Use it for |
| --- | --- |
| [codex-stage-runbook.md](codex-stage-runbook.md) | Session protocol: what Codex must open, prove, validate, and report for every stage. |
| [stage-prompt-packets.md](stage-prompt-packets.md) | Compact stage-by-stage implementation packets with target docs, contract surfaces, split checkpoints, acceptance proof, and validation expectations. |

## Information Preservation Rule

The canonical implementation plan remains the lossless document. These prompt-track files intentionally reference the original stage sections instead of copying every paragraph. When a prompt packet and the canonical stage differ, update both before implementing.

Do not delete source detail from the canonical plan just because this folder exists.

## How To Use In A Codex Session

1. Open [codex-stage-runbook.md](codex-stage-runbook.md).
2. Open the stage row in [stage-prompt-packets.md](stage-prompt-packets.md).
3. Open the original stage section in [../rhi-renderer-review-ready-implementation-plan.md](../rhi-renderer-review-ready-implementation-plan.md).
4. Open every target doc listed by the original stage row in [Required Target Documents By Stage](../rhi-renderer-review-ready-implementation-plan.md#required-target-documents-by-stage).
5. Implement only the current stage or its current split checkpoint.
6. Fill the stage completion packet before marking the stage accepted.


# After Plans

Status: target planning navigation
Date: 2026-06-12
Last synchronized: 2026-06-13

## Purpose

This folder organizes the target execution path for the whole-repository refactor. Use it when you want to know what to implement next, what each stage protects, and what evidence is required before acceptance.

Stages should keep excellent systems, improve systems with good foundations, and replace or redesign systems that cannot support the production target architecture.
They should also remove or justify complexity: retained code must have an owner, consumer, contract, validation value, and smaller alternative considered.

## Read First

- [repository-refactor-stage-map.md](repository-refactor-stage-map.md): grouped Stage 1-29 navigation, status tracker, and before/after evidence.
- [../rhi-renderer-review-ready-implementation-plan.md](../rhi-renderer-review-ready-implementation-plan.md): canonical detailed execution plan.
- [../../architecture/after/repository-target-architecture.md](../../architecture/after/repository-target-architecture.md): broad target architecture.
- [../../architecture/after/repository-target-folder-architecture.md](../../architecture/after/repository-target-folder-architecture.md): target folder structure and per-stage folder guardrails.
- [../../architecture/after/system-design-index.md](../../architecture/after/system-design-index.md): detailed target system contracts.

## Target Planning Questions

- Which stages are local to RHI/Renderer and which are repository-wide?
- What is each stage status: `Not started`, `Started`, `Almost finished`, or `Fully completed`?
- Which adjacent modules must be protected by each stage?
- Which validation is smallest but meaningful?
- Which folders are current migration sources, target owners, or forbidden destinations?
- Which before/after docs should be updated when the architecture changes?
- Which touched systems should be kept, improved, renamed, extracted, rebuilt, or removed?

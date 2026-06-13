# Tutor Track

Status: human learning navigation
Date: 2026-06-13

## Purpose

This folder is for learning while implementing. It explains what each stage is teaching, why the architecture matters, what good engineering judgment looks like, and what to inspect before/after the code changes.

Use this folder when you want to understand the motivation behind a stage, not when you want the shortest possible Codex prompt.

Canonical implementation source:

- [../rhi-renderer-review-ready-implementation-plan.md](../rhi-renderer-review-ready-implementation-plan.md)

Codex-facing implementation source:

- [../implementation/README.md](../implementation/README.md)

## Documents

| Document | Use it for |
| --- | --- |
| [stage-learning-guide.md](stage-learning-guide.md) | Stage-by-stage learning notes, motivations, questions to ask, and what the portfolio reviewer should recognize. |
| [architecture-learning-map.md](architecture-learning-map.md) | Concept map for the big ideas: boundaries, contracts, data handoffs, validation, threading readiness, and complexity control. |

## How To Learn While Implementing

1. Before a stage, read the stage row in [stage-learning-guide.md](stage-learning-guide.md).
2. Open the implementation packet for the same stage in [../implementation/stage-prompt-packets.md](../implementation/stage-prompt-packets.md).
3. Skim the original stage in [../rhi-renderer-review-ready-implementation-plan.md](../rhi-renderer-review-ready-implementation-plan.md).
4. During implementation, keep notes on what surprised you.
5. After validation, reread the tutor row and check whether the lesson became visible in code/docs.

## Learning Goal

By the end of the refactor, you should be able to explain:

- Why RHI, Renderer, GameFramework, tools, launcher, and validation are separate systems.
- How data should cross boundaries without private mutable access.
- Why future multithreading depends on ownership and handoff shape before it depends on a job system.
- How top-tier rendering repos make backend boundaries, command recording, validation, and tooling inspectable.
- How to decide whether code should be kept, improved, renamed, split, rebuilt, or removed.


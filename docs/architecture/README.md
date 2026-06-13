# Architecture Documentation

Status: navigation index
Date: 2026-06-12
Last synchronized: 2026-06-13

## Purpose

This folder is the architecture workspace for the whole SparkleEngine repository. It is organized around the global refactor goal: understand the current system, define the target system, and keep detailed contracts close enough that future work can be reviewed without hunting through unrelated notes.

## Folder Layout

| Area | Use it for |
| --- | --- |
| [before](before/README.md) | Current and baseline architecture: what exists now, known risks, transitional exceptions, and current subsystem relationships. |
| [after](after/README.md) | Target architecture: broad desired module relationships, detailed system ownership, and final acceptance evidence. |
| Root contract docs | Living design contracts that guide implementation stages. These may describe the target state while preserving notes about current debt. |

## Primary Reading Paths

Broad repository view:

1. [before/repository-current-state.md](before/repository-current-state.md)
2. [before/repository-current-graphs.md](before/repository-current-graphs.md)
3. [after/repository-target-architecture.md](after/repository-target-architecture.md)
4. [after/repository-target-graphs.md](after/repository-target-graphs.md)
5. [after/repository-target-folder-architecture.md](after/repository-target-folder-architecture.md)
6. [after/repository-threading-readiness.md](after/repository-threading-readiness.md)
7. [repository-system-map.md](repository-system-map.md)
8. [repository-coverage-status.md](repository-coverage-status.md)

## Single Source Of Truth

| Question | Canonical doc |
| --- | --- |
| What exists today? | [before/repository-current-state.md](before/repository-current-state.md) and [before/repository-current-graphs.md](before/repository-current-graphs.md) |
| What should the finished product look like? | [after/repository-target-architecture.md](after/repository-target-architecture.md) and [after/repository-target-graphs.md](after/repository-target-graphs.md) |
| Where should source folders and durable roots live? | [after/repository-target-folder-architecture.md](after/repository-target-folder-architecture.md) |
| How should the design stay ready for future multithreading? | [after/repository-threading-readiness.md](after/repository-threading-readiness.md) |
| Which system contract applies to a subsystem? | [after/system-design-index.md](after/system-design-index.md) |
| Which roots still need work? | [repository-coverage-status.md](repository-coverage-status.md) and [rendering-coverage-status.md](rendering-coverage-status.md) |
| Which boundaries are mechanically checked? | [architecture-boundary-guardrails.md](architecture-boundary-guardrails.md) |

Detailed system view:

1. [after/system-design-index.md](after/system-design-index.md)
2. [rhi-contract-map.md](rhi-contract-map.md)
3. [rendering-system-map.md](rendering-system-map.md)
4. [frame-graph-contract.md](frame-graph-contract.md)
5. [pass-authoring-contract.md](pass-authoring-contract.md)
6. [pipeline-runtime-contract.md](pipeline-runtime-contract.md)
7. [ray-tracing-contract.md](ray-tracing-contract.md)
8. [upscaler-provider-contract.md](upscaler-provider-contract.md)
9. [game-framework-contract.md](game-framework-contract.md)
10. [tooling-pipeline-contract.md](tooling-pipeline-contract.md)

Guardrails and acceptance:

1. [architecture-boundary-guardrails.md](architecture-boundary-guardrails.md)
2. [rendering-coverage-status.md](rendering-coverage-status.md)
3. [repository-coverage-status.md](repository-coverage-status.md)

## Document Roles

| Document | Role |
| --- | --- |
| [before/repository-current-graphs.md](before/repository-current-graphs.md) | Current/baseline module, graphics, content, host, and risk graphs. |
| [after/repository-target-graphs.md](after/repository-target-graphs.md) | Finished-product global, engine runtime, graphics, tooling, launcher, and evidence graphs. |
| [after/repository-target-folder-architecture.md](after/repository-target-folder-architecture.md) | Target source-root and folder architecture, including current-to-target path moves and forbidden folder edges. |
| [after/repository-threading-readiness.md](after/repository-threading-readiness.md) | Cross-repository threading-readiness contract for mutable ownership, immutable handoffs, command batches, queues, jobs, and reports. |
| [repository-system-map.md](repository-system-map.md) | Living whole-repository module map and dependency intent. |
| [repository-coverage-status.md](repository-coverage-status.md) | Whole-repository coverage baseline and source-root status. |
| [rendering-system-map.md](rendering-system-map.md) | Detailed RHI/Renderer and graphics-system map. |
| [rendering-coverage-status.md](rendering-coverage-status.md) | Detailed RHI/Renderer coverage status. |
| [rendering-glossary.md](rendering-glossary.md) | Shared vocabulary for renderer, RHI, tools, and runtime artifact terms. |
| [architecture-boundary-guardrails.md](architecture-boundary-guardrails.md) | Mechanical checks and staged exceptions. |
| [upscaler-provider-contract.md](upscaler-provider-contract.md) | Upscaler provider, native interop, SDK isolation, and failure-domain ownership. |
| [game-framework-contract.md](game-framework-contract.md) | Runtime scene and cooked asset ownership contract. |
| [tooling-pipeline-contract.md](tooling-pipeline-contract.md) | Launcher, ShaderCompiler, import, cooking, and conversion ownership contract. |

## Maintenance Rule

When a refactor changes a durable source root, update the relevant before/after view or coverage status before calling the stage accepted. New architecture docs should be linked from this file and from either [before](before/README.md) or [after](after/README.md).

# After Architecture

Status: target-state navigation
Date: 2026-06-12
Last synchronized: 2026-06-13

## Purpose

This folder describes the architecture SparkleEngine is moving toward. It is the target shape for the global refactor: RHI/Renderer improvements, GameFramework/runtime contracts, tooling and content pipeline ownership, launcher workflow separation, CMake/CI guardrails, and final reviewer evidence.

The target architecture is not constrained to current bodies or names. It preserves excellent systems, improves systems with strong foundations, and replaces systems whose ownership, edges, or names cannot support the production model.

## Read First

- [repository-target-architecture.md](repository-target-architecture.md): broad target architecture and before-to-after changes.
- [repository-target-graphs.md](repository-target-graphs.md): finished-product global, runtime, graphics, tooling, launcher, and validation graphs.
- [repository-target-folder-architecture.md](repository-target-folder-architecture.md): target folder ownership, current-to-target path moves, and folder guardrails.
- [system-design-index.md](system-design-index.md): detailed target design by subsystem with links to the living contracts.

## Related Target Documents

| Document | Why it belongs to the after view |
| --- | --- |
| [../repository-system-map.md](../repository-system-map.md) | Living whole-repository target map and dependency intent. |
| [../game-framework-contract.md](../game-framework-contract.md) | Target runtime scene and cooked asset ownership. |
| [../tooling-pipeline-contract.md](../tooling-pipeline-contract.md) | Target Launcher, ShaderCompiler, import, cooking, and conversion ownership. |
| [../rhi-contract-map.md](../rhi-contract-map.md) | Target RHI method/service ownership. |
| [../frame-graph-contract.md](../frame-graph-contract.md) | Target renderer frame graph contract. |
| [../pass-authoring-contract.md](../pass-authoring-contract.md) | Target pass/shader authoring model. |
| [../pipeline-runtime-contract.md](../pipeline-runtime-contract.md) | Target PSO/package/runtime ownership. |
| [../ray-tracing-contract.md](../ray-tracing-contract.md) | Target ray tracing ownership from scene data to TLAS/pass binding. |

## What This Folder Answers

- What should the repository look like after the refactor?
- What are the final broad system interactions?
- Which folders should exist, move, split, or disappear?
- Where are the detailed contracts for each subsystem?
- What before-to-after changes are expected?

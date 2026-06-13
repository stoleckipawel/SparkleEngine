# Target System Design Index

Status: after/detail navigation
Date: 2026-06-12
Last synchronized: 2026-06-13

## Purpose

This index groups the detailed target architecture by subsystem. Use it when you know the system you are changing and need the relevant design contract, plan stage, and acceptance evidence.

## Whole Repository

| System | Design docs | Plan stages | Target evidence |
| --- | --- | --- | --- |
| Repository ownership | [repository-system-map.md](../repository-system-map.md), [repository-coverage-status.md](../repository-coverage-status.md), [sparkle-whole-repository-architecture-review.md](../../plans/sparkle-whole-repository-architecture-review.md) | 23, 28, 29, 30 | Every durable source root has owner, dependency rules, validation target, and final status. |
| Before/after navigation | [before/current state](../before/repository-current-state.md), [target architecture](repository-target-architecture.md) | 21, 22, 23, 29, 30 | Reviewer can compare current and target architecture without reading every plan file. |
| Production graph and contract surfaces | [repository-target-architecture.md](repository-target-architecture.md), [repository-target-graphs.md](repository-target-graphs.md) | 22, 23, 24, 25, 26, 27, 28, 29, 30 | Target flow is organized by Foundation, Contracts, Runtime, Graphics, Toolchain, and Evidence rather than current accidental edges. |
| Folder architecture | [repository-target-folder-architecture.md](repository-target-folder-architecture.md), [repository-target-architecture.md](repository-target-architecture.md), [repository-refactor-stage-map.md](../../plans/after/repository-refactor-stage-map.md) | 1-30 | Folder names, source roots, CMake target groups, and staged moves reveal ownership and data flow instead of preserving accidental current layout. |
| Threading readiness and data isolation | [repository-threading-readiness.md](repository-threading-readiness.md), [repository-target-architecture.md](repository-target-architecture.md), [repository-target-graphs.md](repository-target-graphs.md) | 1-30 | Mutable state has a phase owner; handoffs use snapshots, DTOs, manifests, command batches, queue packets, job requests, or reports; future jobs/queues do not need private owner state. |
| Refactor disposition and naming canon | [repository-target-architecture.md](repository-target-architecture.md), [sparkle-whole-repository-architecture-review.md](../../plans/sparkle-whole-repository-architecture-review.md) | 23, 24, 25, 26, 28, 29, 30 | Touched systems are classified as keep/refine, improve/extract, or replace/redesign; names follow shared production vocabulary. |
| Complexity right-to-exist budget | [repository-target-architecture.md](repository-target-architecture.md), [repository-refactor-stage-map.md](../../plans/after/repository-refactor-stage-map.md), [architecture-review-acceptance-rubric.md](../../plans/architecture-review-acceptance-rubric.md) | 1-30 | Retained complexity names owner, consumer, contract, validation value, smaller alternative, and removal stage when temporary. |
| Stage progress | [repository-refactor-stage-map.md](../../plans/after/repository-refactor-stage-map.md), [rhi-renderer-review-ready-implementation-plan.md](../../plans/rhi-renderer-review-ready-implementation-plan.md) | 1-30 | Stage status uses `Not started`, `Started`, `Almost finished`, and `Fully completed`; status advances only with implementation and validation evidence. |
| Stage contract coverage | [Stage Contract Coverage Matrix](../../plans/rhi-renderer-review-ready-implementation-plan.md#stage-contract-coverage-matrix), [Mandatory Split Checkpoints For Large Stages](../../plans/rhi-renderer-review-ready-implementation-plan.md#mandatory-split-checkpoints-for-large-stages) | 1-30 | Each implementation stage proves its contract surfaces and broad stages are split or checkpointed before acceptance. |
| Mechanical guardrails | [architecture-boundary-guardrails.md](../architecture-boundary-guardrails.md) | 3, 28, 29, 30 | Checks report actionable file paths and reasons; exceptions are counted and stage-labeled. |

## Contract Surfaces

| Surface | Design docs | Plan stages | Target evidence |
| --- | --- | --- | --- |
| AssetContracts | [repository-target-architecture.md](repository-target-architecture.md), [tooling-pipeline-contract.md](../tooling-pipeline-contract.md), [game-framework-contract.md](../game-framework-contract.md) | 24, 25, 27, 29, 30 | SourceImporters and cookers produce versioned asset schemas; runtime consumes schemas without depending on tool internals. |
| RenderContracts | [repository-target-architecture.md](repository-target-architecture.md), [rendering-system-map.md](../rendering-system-map.md), [game-framework-contract.md](../game-framework-contract.md) | 13, 20, 24, 29, 30 | GameFramework produces immutable render snapshots; Renderer consumes snapshots without gameplay/private coupling. |
| ShaderContracts | [repository-target-architecture.md](repository-target-architecture.md), [pass-authoring-contract.md](../pass-authoring-contract.md), [pipeline-runtime-contract.md](../pipeline-runtime-contract.md) | 4, 5, 16, 17, 27, 29, 30 | Renderer pass authoring and ShaderCompiler share pass catalogs, package manifests, reflection, and binding layout identity. |
| ToolContracts | [repository-target-architecture.md](repository-target-architecture.md), [tooling-pipeline-contract.md](../tooling-pipeline-contract.md) | 25, 26, 27, 28, 29, 30 | LauncherCore, cookers, compiler, and CI exchange process requests, reports, artifact paths, and failure diagnostics. |
| RhiContracts | [repository-target-architecture.md](repository-target-architecture.md), [rhi-contract-map.md](../rhi-contract-map.md), [architecture-boundary-guardrails.md](../architecture-boundary-guardrails.md) | 3, 6, 7, 8, 9, 19, 20, 29, 30 | Renderer and provider integrations use public RHI descriptors/capabilities; backend-native details stay inside backend folders. |
| ThreadingReadiness | [repository-threading-readiness.md](repository-threading-readiness.md), [frame-graph-contract.md](../frame-graph-contract.md), [tooling-pipeline-contract.md](../tooling-pipeline-contract.md) | 1-30 | Future render/thread/cook/launcher parallelism is enabled by phase ownership, immutable handoffs, command batches, queue packets, and deterministic reports. |

## Engine Runtime

| System | Design docs | Plan stages | Target evidence |
| --- | --- | --- | --- |
| Core foundation | [repository-system-map.md](../repository-system-map.md), [repository-coverage-status.md](../repository-coverage-status.md) | 23, 29, 30 | No platform/render/game/tool policy leaks into Core. |
| Platform | [repository-system-map.md](../repository-system-map.md), [repository-coverage-status.md](../repository-coverage-status.md) | 23, 29, 30 | Platform owns OS/window/input only. |
| RHI public contract | [rhi-contract-map.md](../rhi-contract-map.md), [architecture-boundary-guardrails.md](../architecture-boundary-guardrails.md) | 3, 6, 7, 8, 9, 19, 20, 22, 30 | RHI methods are categorized by service; RHI has no renderer pass ownership. |
| D3D12/Vulkan backends | [rhi-contract-map.md](../rhi-contract-map.md), [rendering-coverage-status.md](../rendering-coverage-status.md) | 7, 8, 9, 19, 20, 30 | Backend-private folders stay separate and parity evidence exists. |
| Renderer system | [rendering-system-map.md](../rendering-system-map.md), [rendering-coverage-status.md](../rendering-coverage-status.md) | 4, 10, 11, 12, 13, 20, 22, 30 | Renderer is a host-facing facade over frame pipeline and feature systems. |
| Frame graph | [frame-graph-contract.md](../frame-graph-contract.md) | 14, 15, 20, 22, 30 | Resource/barrier/aliasing diagnostics are actionable and smoke-visible. |
| Pass authoring | [pass-authoring-contract.md](../pass-authoring-contract.md) | 4, 16, 17, 20, 22, 30 | Ordinary pass additions require no RHI edits or central trait duplication. |
| Pipeline runtime and PSO | [pipeline-runtime-contract.md](../pipeline-runtime-contract.md) | 16, 17, 20, 22, 30 | PSO keys are explicit, printable, and backend-normalized. |
| Ray tracing | [ray-tracing-contract.md](../ray-tracing-contract.md) | 18, 20, 22, 30 | BLAS/TLAS ownership and pass usage are documented and backend-neutral above RHI. |
| Upscaling providers | [upscaler-provider-contract.md](../upscaler-provider-contract.md), [rhi-contract-map.md](../rhi-contract-map.md), [architecture-boundary-guardrails.md](../architecture-boundary-guardrails.md) | 9, 10, 20, 22, 30 | Common renderer code uses provider-neutral contracts; provider targets own vendor SDK calls and narrow native interop. |
| GameFramework | [game-framework-contract.md](../game-framework-contract.md), [repository-coverage-status.md](../repository-coverage-status.md) | 13, 24, 27, 29, 30 | Runtime/cooked ownership is clear; renderer consumes immutable snapshots/DTOs. |
| Editor/Application hosts | [repository-system-map.md](../repository-system-map.md), [tooling-pipeline-contract.md](../tooling-pipeline-contract.md) | 8, 12, 20, 26, 29, 30 | Hosts orchestrate systems and UI without backend-native/cook/import implementation. |

## Tools And Content Pipeline

| System | Design docs | Plan stages | Target evidence |
| --- | --- | --- | --- |
| ShaderCompiler | [tooling-pipeline-contract.md](../tooling-pipeline-contract.md), [pass-authoring-contract.md](../pass-authoring-contract.md), [pipeline-runtime-contract.md](../pipeline-runtime-contract.md) | 4, 5, 16, 17, 20, 27, 29, 30 | Packages can be listed, cooked, inspected, and validated without RHI-specific pass edits. |
| SourceImporters | [tooling-pipeline-contract.md](../tooling-pipeline-contract.md), [game-framework-contract.md](../game-framework-contract.md) | 25, 27, 29, 30 | Current `SourceImportAdapters` are renamed/extracted toward focused importers that emit DTOs and diagnostics; runtime does not parse source formats. |
| TextureCooker | [tooling-pipeline-contract.md](../tooling-pipeline-contract.md), [game-framework-contract.md](../game-framework-contract.md) | 25, 27, 29, 30 | Cooked texture output matches runtime loader/upload expectations. |
| Mesh/Material/Scene cookers | [tooling-pipeline-contract.md](../tooling-pipeline-contract.md), [game-framework-contract.md](../game-framework-contract.md) | 24, 25, 27, 29, 30 | Cooked records and manifests match GameFramework loaders and renderer consumers. |
| AssetCooker | [tooling-pipeline-contract.md](../tooling-pipeline-contract.md) | 25, 27, 29, 30 | Orchestrates focused tools and reports actionable failure evidence. |
| AssetConverter | [tooling-pipeline-contract.md](../tooling-pipeline-contract.md) | 25, 29, 30 | Replaced as a production path: fold into AssetCooker or explicit inspect/debug commands; no parallel cook policy. |
| ToolConsoleSupport / CookDiagnostics | [tooling-pipeline-contract.md](../tooling-pipeline-contract.md) | 25, 29, 30 | Current `CookCommon` is renamed/split so support helpers and diagnostics are not a vague policy sink. |

## Launcher, Build, CI, And Samples

| System | Design docs | Plan stages | Target evidence |
| --- | --- | --- | --- |
| SparkleLauncherCore | [tooling-pipeline-contract.md](../tooling-pipeline-contract.md), [repository-system-map.md](../repository-system-map.md) | 26, 28, 29, 30 | Plans and runs build/cook/launch/maintenance processes and records evidence. |
| SparkleLauncher Qt GUI | [tooling-pipeline-contract.md](../tooling-pipeline-contract.md) | 26, 29, 30 | GUI models/widgets present workflow state without owning tool algorithms. |
| CMake | [repository-system-map.md](../repository-system-map.md), [repository-coverage-status.md](../repository-coverage-status.md) | 23, 28, 29, 30 | Target links/scopes express architecture ownership. |
| CI/local checks | [architecture-boundary-guardrails.md](../architecture-boundary-guardrails.md), [repository-coverage-status.md](../repository-coverage-status.md) | 28, 29, 30 | Local and CI-friendly checks cover boundaries without requiring editor builds. |
| Projects/Showcase | [repository-coverage-status.md](../repository-coverage-status.md), [tooling-pipeline-contract.md](../tooling-pipeline-contract.md) | 20, 27, 29, 30 | Sample content exercises cook/load/render/launcher paths. |
| Docs/reviewer path | [../../plans/architecture-review-acceptance-rubric.md](../../plans/architecture-review-acceptance-rubric.md), [../../plans/rhi-renderer-review-ready-implementation-plan.md](../../plans/rhi-renderer-review-ready-implementation-plan.md) | 21, 22, 23, 29, 30 | README/docs/evidence let an external reviewer navigate broad and detailed design. |

## Stage 2 Completion Packet

| Field | Evidence |
| --- | --- |
| Stage / checkpoint | Stage 2 - Reviewer Architecture Docs And Vocabulary. No split checkpoint was required because this stage is documentation-only. |
| Status | Fully completed. Reopen only if vocabulary, maps, or ownership contracts drift from code or from later accepted target architecture. |
| Target docs opened | `docs/architecture/rendering-glossary.md`, `docs/architecture/rendering-system-map.md`, `docs/architecture/rhi-contract-map.md`, `docs/architecture/frame-graph-contract.md`, `docs/architecture/pass-authoring-contract.md`, `docs/architecture/pipeline-runtime-contract.md`, `docs/architecture/ray-tracing-contract.md`, `docs/architecture/after/repository-threading-readiness.md`, `docs/plans/rhi-renderer-review-ready-implementation-plan.md`, `docs/plans/rhi-renderer-architecture-review.md`, `docs/plans/architecture-review-acceptance-rubric.md`. |
| External reference check | arc42 was used for context/scope, building-block, runtime-view, crosscutting-concept, risks, and glossary structure; Donut was used for clear module role navigation; Falcor was used for reviewer-facing render graph/render pass workflow navigation. |
| Contract surfaces touched | Documentation state only: shared vocabulary, reviewer navigation, rendering system map, RHI map, frame graph contract, pass authoring contract, pipeline runtime contract, ray tracing contract, and threading-readiness links. |
| Refactor disposition | Keep and refine existing Stage 2 docs. They are useful reviewer entry points and should remain focused rather than duplicated across plan prose. |
| Complexity right to exist | The document set earns its complexity by separating index, vocabulary, system map, and subsystem contracts. Each doc has a distinct reader job: navigate, name concepts, understand broad flow, or inspect one ownership contract. |
| Data transfer contract | Vocabulary moves through `rendering-glossary.md`; broad flow moves through Mermaid diagrams and clickable code references; ownership contracts name the owner of renderer snapshots, RHI descriptors, shader packages, cooked artifacts, process requests, validation evidence, BLAS/TLAS data, PSO keys, and native interop metadata. |
| Threading readiness handoff | Stage 2 docs name future-safe shapes: immutable render snapshots, command batches, frame graph plans, shader package manifests, explicit PSO keys, RHI submission packets, tool job requests, launcher process requests, and deterministic validation reports. |
| Acceptance proof | Required docs exist and are linked from this index and the main architecture review; glossary covers RHI, backend, command context, command list, frame graph pass, pass runtime, PSO key, native interop, BLAS, TLAS, and upscaler provider; diagrams cover frame execution, shader package flow, PSO creation, and backend boundaries. |
| Validation | Verified required files, key terms, Mermaid diagrams, and main planning links on 2026-06-13. Docs-only stage; no build or runtime smoke was required. |

## How To Use This Index

1. Find the subsystem you are changing.
2. Open its design docs before editing code.
3. Check the stage numbers in [../../plans/after/repository-refactor-stage-map.md](../../plans/after/repository-refactor-stage-map.md).
4. Run the smallest validation listed in the relevant contract.
5. Update before/after docs only when the current or target architecture changes.

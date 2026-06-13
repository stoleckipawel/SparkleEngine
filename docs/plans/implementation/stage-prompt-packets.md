# Stage Prompt Packets

Status: Codex-facing implementation packets
Date: 2026-06-13

This file is optimized for implementation prompts. It is intentionally compact, but every row points back to the lossless canonical stage in [../rhi-renderer-review-ready-implementation-plan.md](../rhi-renderer-review-ready-implementation-plan.md).

Before implementing any row, open:

- The original stage section.
- The row in [Required Target Documents By Stage](../rhi-renderer-review-ready-implementation-plan.md#required-target-documents-by-stage).
- The row in [Stage Contract Coverage Matrix](../rhi-renderer-review-ready-implementation-plan.md#stage-contract-coverage-matrix).
- Any row in [Mandatory Split Checkpoints For Large Stages](../rhi-renderer-review-ready-implementation-plan.md#mandatory-split-checkpoints-for-large-stages).

Global implementation rules for every packet:

- Do not add explanatory, provenance, planning, stage, or refactor-process comments to source code. Add a source comment only when it documents a non-obvious runtime behavior, API constraint, safety rule, or lifetime rule that cannot be expressed cleanly in code.
- Runtime/user-facing strings must describe behavior, diagnostics, commands, paths, or failure reasons. They must not mention planning documents, implementation stages, authorship, or refactor history.
- Do not move code as a cosmetic relocation. Any moved body must land in the designed owner, be renamed or reshaped to that context, use that owner's contracts, and delete or simplify the old responsibility.
- Reject destination folders, services, or helpers that become new catch-all clutter. If no proper destination exists yet, design the destination first or split the work into a design stage.
- User-facing editor/runtime validation is launcher-first. If direct execution is used for automation, it must mirror [validation-workflow-contract.md](../../architecture/validation-workflow-contract.md): launcher operation shape, project working directory, target/profile, smoke environment, logs, and artifacts.

## Stage Packets

| Stage | Implementation focus | Must prove | Split/checkpoint pressure |
| --- | --- | --- | --- |
| [1](../rhi-renderer-review-ready-implementation-plan.md#stage-1---baseline-status-and-evidence-freeze) | Freeze baseline coverage and evidence. | Coverage rows have owners, risks, validation artifacts, final evidence, and threading-sensitive handoffs. | None. |
| [2](../rhi-renderer-review-ready-implementation-plan.md#stage-2---reviewer-architecture-docs-and-vocabulary) | Align architecture docs and vocabulary. | Maps, glossary, contracts, and target docs use one ownership language for runtime, graphics, tools, artifacts, and threading shapes. | None. |
| [3](../rhi-renderer-review-ready-implementation-plan.md#stage-3---mechanical-boundary-guardrails) | Enforce first mechanical boundaries. | Checks report paths/reasons, count transitional exceptions, and block new RHI/Renderer/backend drift. | None. |
| [4](../rhi-renderer-review-ready-implementation-plan.md#stage-4---move-renderer-shader-registration-out-of-rhi) | Keep renderer pass shader metadata above RHI. | RHI has no renderer-private includes; ShaderCompiler still enumerates packages; package ids remain stable. | None, but stop if ShaderCompiler needs full Renderer runtime. |
| [5](../rhi-renderer-review-ready-implementation-plan.md#stage-5---validation-milestone-a-boundaries-and-shader-registration) | Validate boundary and shader-registration migration. | Boundary check, package enumeration, and smallest affected build/tool validation prove Stage 4. | None. |
| [6](../rhi-renderer-review-ready-implementation-plan.md#stage-6---rhi-method-ownership-and-service-extraction-design) | Classify RHI method ownership. | Every public RHI method has service owner, caller category, backend impact, tool/runtime blast radius, and reason to remain public. | None. |
| [7](../rhi-renderer-review-ready-implementation-plan.md#stage-7---extract-first-rhi-services-interop-capture-diagnostics-presentation) | Extract first RHI services. | Services have caller evidence, public contracts, diagnostics, and frame/queue/resource ownership notes. | Interop, capture/readback, diagnostics, presentation, facade cleanup. |
| [8](../rhi-renderer-review-ready-implementation-plan.md#stage-8---move-smoke-capture-and-backend-native-validation-behind-rhi) | Move backend-native smoke/capture logic behind RHI/backend services. | Application orchestrates only; Stage 8 exception is removed; capture/readback diagnostics remain usable by launcher smoke. | None. |
| [9](../rhi-renderer-review-ready-implementation-plan.md#stage-9---formalize-upscaling-and-native-interop-contracts) | Formalize provider/native interop. | Vendor/native metadata flows through provider and RHI/backend contracts; ordinary passes and GameFramework never see native handles. | None. |
| [10](../rhi-renderer-review-ready-implementation-plan.md#stage-10---validation-milestone-b-rhi-services-capture-interop-upscaling) | Validate RHI services, capture, interop, upscaling. | Comparable D3D12/Vulkan evidence, feature reports, launcher-shaped smoke artifacts, and known-difference notes. | Shader/package validation, launcher smoke, D3D12, Vulkan, feature reports, docs. |
| [11](../rhi-renderer-review-ready-implementation-plan.md#stage-11---decompose-renderer-into-facade-system-root-frame-pipeline) | Decompose Renderer facade and frame pipeline. | `Renderer` is host protocol; composition/system root and frame pipeline own details; future render-thread data has clean handoff. | Host protocol, system root, frame pipeline, diagnostics/lifecycle, old responsibility deletion. |
| [12](../rhi-renderer-review-ready-implementation-plan.md#stage-12---add-viewport-presentation-bridge-and-clean-host-protocol) | Add host-facing presentation bridge. | Application/Editor receive presentation products without frame graph internals, backend resources, or manual transition ownership. | None. |
| [13](../rhi-renderer-review-ready-implementation-plan.md#stage-13---clean-scene-data-mesh-texture-temporal-ownership) | Clean renderer scene/resource ownership. | Renderer consumes immutable render-domain snapshots/DTOs and names cooked/runtime producers for resources. | Snapshot contract, mesh/material, texture, temporal/upscaling, denoising disposition. |
| [14](../rhi-renderer-review-ready-implementation-plan.md#stage-14---harden-frame-graph-contract-and-diagnostics) | Harden frame graph phases and diagnostics. | Setup/compile/execute are distinct; diagnostics are actionable; command batches can derive from frozen graph data. | None. |
| [15](../rhi-renderer-review-ready-implementation-plan.md#stage-15---validation-milestone-c-renderer-facade-frame-pipeline-frame-graph) | Validate renderer facade/frame graph. | D3D12/Vulkan launcher-shaped smoke-visible graph diagnostics prove unresolved resources/barriers are handled. | None. |
| [16](../rhi-renderer-review-ready-implementation-plan.md#stage-16---introduce-explicit-pso-key-and-pipeline-runtime-library) | Introduce explicit PSO key and pipeline runtime ownership. | Keys are printable/deterministic/backend-normalized and tied to package/reflection/layout identity. | Package identity, binding layout, PSO key, runtime cache, backend descriptors. |
| [17](../rhi-renderer-review-ready-implementation-plan.md#stage-17---introduce-declarative-pass-definition-and-migrate-passes) | Introduce declarative pass definition. | Ordinary pass additions avoid RHI edits and central trait duplication; pass definitions are immutable tool/runtime inputs. | Pass schema, catalog, graph setup, pipeline lookup, proof pass, cleanup. |
| [18](../rhi-renderer-review-ready-implementation-plan.md#stage-18---clean-ray-tracing-ownership-and-contracts) | Clean ray tracing ownership. | Renderer owns RT scene/shadow policy; RHI owns AS descriptors/build commands; generations and fallback diagnostics are explicit. | RT scene, BLAS, TLAS, RHI AS contract, shadow data, fallback diagnostics. |
| [19](../rhi-renderer-review-ready-implementation-plan.md#stage-19---slim-backend-facades-and-enforce-d3d12vulkan-service-symmetry) | Slim backend facades and enforce service symmetry. | D3D12/Vulkan services are sibling implementations with named differences and no cross-backend/private leakage. | D3D12 map, Vulkan map, common RHI, CMake, include check, parity differences. |
| [20](../rhi-renderer-review-ready-implementation-plan.md#stage-20---validation-milestone-d-full-rendererrhi-backend-parity) | Full Renderer/RHI backend parity validation. | Build/smoke/capture/log evidence covers shader compiler, launcher-shaped launch paths, RT/upscaling/frame graph/PSO, and known differences. | Build/tool, shader/package, D3D12, Vulkan, fallback reports, performance audit, evidence index. |
| [21](../rhi-renderer-review-ready-implementation-plan.md#stage-21---portfolio-and-repository-review-presentation) | Create reviewer-facing repo path. | README/reviewer path links architecture, validation, screenshots/captures, known issues, threading model, and avoids marketing-only claims. | None. |
| [22](../rhi-renderer-review-ready-implementation-plan.md#stage-22---final-cleanup-rubric-scoring-and-review-ready-gate) | Final first-track RHI/Renderer cleanup and scoring. | RHI/Renderer debt is removed or owned; rubric critical criteria pass; stale exceptions/docs/duplicate paths are gone. | None. |
| [23](../rhi-renderer-review-ready-implementation-plan.md#stage-23---whole-repository-coverage-and-dependency-map) | Audit all durable source roots. | Every root has owner, dependencies, forbidden dependencies, target folder shape, validation command, complexity status, and threading risk. | Inventory, owner rows, folder comparison, generated roots, validation map, threading map. |
| [24](../rhi-renderer-review-ready-implementation-plan.md#stage-24---gameframework-runtime-and-cooked-asset-contract) | Define GameFramework runtime/cooked contract. | GameFramework owns runtime scene/cooked loading and emits render snapshots; tools and renderer pass logic stay outside. | None. |
| [25](../rhi-renderer-review-ready-implementation-plan.md#stage-25---source-import-asset-cooking-and-conversion-architecture) | Split source import, focused cookers, orchestration, diagnostics, conversion. | Tool work is deterministic, report-driven, and avoids production AssetConverter duplication. | SourceImporters, DTO diagnostics, focused cookers, AssetCooker, diagnostics/support, AssetConverter retirement. |
| [26](../rhi-renderer-review-ready-implementation-plan.md#stage-26---launcher-workflow-and-editorapplication-host-boundaries) | Clean LauncherCore, Qt GUI, Application, Editor host boundaries. | LauncherCore owns workflow/process reports; Qt GUI presents; hosts do not duplicate cook/import/backend/tool logic. | LauncherCore contract, Qt boundary, Application, Editor, history/recovery, no widget algorithms. |
| [27](../rhi-renderer-review-ready-implementation-plan.md#stage-27---shader-and-cook-artifact-validation-matrix) | Build artifact validation matrix. | Every shader/cooked artifact names producer, schema owner, consumer, inspector, validation command, report, and smoke/load evidence. | Shader, texture, mesh, material, scene, inspectors, smoke/load. |
| [28](../rhi-renderer-review-ready-implementation-plan.md#stage-28---build-ci-and-boundary-guardrail-expansion) | Expand repo-wide guardrails. | Local/CI-friendly checks catch runtime-to-tools, private include, launcher/tool, generated-root, CMake, and threading-hostile drift. | Runtime-tools, GameFramework/private, launcher/tool, generated root, CMake, threading, CI/local. |
| [29](../rhi-renderer-review-ready-implementation-plan.md#stage-29---whole-repository-evidence-gate) | Run whole-repo evidence gate. | Coverage, target architecture, graphs, CMake/CI, tools, samples, docs, and validation evidence agree. | Coverage, docs/code/CMake, sample/tool evidence, risk table, duplicate cleanup, reviewer path. |
| [30](../rhi-renderer-review-ready-implementation-plan.md#stage-30---threading-readiness-final-audit) | Final threading-readiness audit. | Every durable module names mutable owner, phase, handoff, isolation, ordering, diagnostics identity, deterministic output, and risk owner. | Engine runtime, graphics/RHI, tooling/content, launcher/host, CMake/CI/docs. |

## Codex Final Response Requirements

For every stage, final response must include:

- What changed.
- Which contract surfaces were proven.
- Validation run.
- Validation not run and why.
- Remaining risk, especially for graphics/RHI, shader/compiler/cook, CMake, launcher, and threading-readiness.
- Confirmation that source text discipline and destination-fit rules were respected when source files were touched.

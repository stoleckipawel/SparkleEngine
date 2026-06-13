# Repository Refactor Stage Map

Status: execution navigation and stage status tracker
Date: 2026-06-12
Last synchronized: 2026-06-13

## Purpose

This map makes the implementation plan navigable as a whole-repository refactor. The canonical details remain in [../rhi-renderer-review-ready-implementation-plan.md](../rhi-renderer-review-ready-implementation-plan.md); this file groups stages by architecture area and shows what before/after evidence each group should update.

Folder architecture is tracked in [repository-target-folder-architecture.md](../../architecture/after/repository-target-folder-architecture.md). Threading readiness is tracked in [repository-threading-readiness.md](../../architecture/after/repository-threading-readiness.md). Validation workflow shape is tracked in [validation-workflow-contract.md](../../architecture/validation-workflow-contract.md). Each stage must make source folders, target folders, forbidden folders, cleanup paths, mutable owners, future-safe handoff shapes, and user-facing validation paths visible before implementation.

Required target documents for each individual stage live in [Required Target Documents By Stage](../rhi-renderer-review-ready-implementation-plan.md#required-target-documents-by-stage). Open that row before starting the stage.
Required contract proof and split checkpoints live in [Stage Contract Coverage Matrix](../rhi-renderer-review-ready-implementation-plan.md#stage-contract-coverage-matrix) and [Mandatory Split Checkpoints For Large Stages](../rhi-renderer-review-ready-implementation-plan.md#mandatory-split-checkpoints-for-large-stages). A broad stage cannot be accepted until each listed checkpoint has independent evidence, or the unfinished checkpoint has been promoted into a new numbered stage.

Codex-facing implementation packets live in [../implementation/stage-prompt-packets.md](../implementation/stage-prompt-packets.md). Human-facing learning notes for the same stages live in [../tutor/stage-learning-guide.md](../tutor/stage-learning-guide.md).

Before accepting any stage, classify touched systems as `Keep and refine`, `Improve and extract`, or `Replace or redesign`. The stage may rename, split, merge, rebuild, or delete existing code when that is the cleanest path to the target architecture.

Source text and relocation discipline are part of acceptance. A stage should not add explanatory/provenance/planning comments to source, and moved code must fit the destination owner instead of shifting clutter into a new folder, service, or helper.

Stage status tracks implementation acceptance, not merely whether planning docs exist. Future-stage design docs may be seeded before the stage starts; the stage status should advance only when implementation work, validation, and cleanup evidence justify it.

## Stage Status Vocabulary

| Status | Meaning | When to update |
| --- | --- | --- |
| `Not started` | The stage is planned, but no stage-owned implementation or validation work is underway. Planning docs may already exist. | Default for future stages. |
| `Started` | Stage-owned code/docs/CMake/check work has begun, but acceptance evidence is incomplete. | Set when implementation begins or when a stage-owned migration is opened. |
| `Almost finished` | Main implementation is in place, but validation, cleanup, evidence, or final doc synchronization remains. | Set when the next work is mostly verification, cleanup, or final exception removal. |
| `Fully completed` | Acceptance and validation for the stage are satisfied, docs agree with code, and transitional debt owned by the stage is resolved or explicitly carried by a later stage. | Set only after validation evidence is recorded. |

## Stage Status Tracker

| Stage | Status | Current evidence | Remaining before next status update |
| --- | --- | --- | --- |
| 1 | Fully completed | Rendering coverage baseline has a Stage 1 completion packet, 547 tracked files mapped, 0 unmapped files, and whole-repository coverage delegated to `repository-coverage-status.md`. | Reopen only if a new durable root appears without coverage. |
| 2 | Fully completed | Target system design index has a Stage 2 completion packet; glossary, rendering map, RHI, frame graph, pass authoring, pipeline runtime, ray tracing, threading-readiness, whole-repo, and folder architecture docs are linked. | Reopen only if vocabulary, maps, or ownership contracts drift from code. |
| 3 | Fully completed | `architecture_boundary_check` exists; direct CMake script passes with counted Stage 8/9 transitional exceptions. | Stage 28 expands checks repo-wide. |
| 4 | Fully completed | Pass authoring contract has a Stage 4 completion packet; renderer shader registrations are above RHI; `SparkleRendererShaderRegistrations` is the narrow ShaderCompiler handoff; boundary checks show no `RHI -> Renderer/Private` violation. | Stage 5 validates executable package enumeration and smallest affected build surface. |
| 5 | Fully completed | Boundary check passed; VS2026 configure passed; `ShaderCompiler` and `SparkleLauncher` built; `ShaderCompiler list-shaders --validate` reported 17 valid typed registrations; coverage status has Stage 5 evidence. | Install/discover `clang-format` to enable optional `clang_format_check`; Stage 17 removes remaining duplicate pass registration metadata. |
| 6 | Fully completed | RHI contract map has a Stage 6 completion packet and current root-method ownership table: 74 public declarations after Stage 7 service getters, 70 unique method names, one primary service owner each, backend files, callers, extraction order, and renderer-convenience challenges. | Reopen if a public RHI method is added without an owner/caller/backend/validation row. |
| 7 | Fully completed | Public service contracts exist for interop, capture/readback, diagnostics, and presentation/UI; D3D12/Vulkan expose symmetric service methods through composed adapters rather than backend facade multiple inheritance; Renderer upscaling and FrameGraph, plus Application present/capture/diagnostics paths, use service edges; boundary check and targeted builds passed. | Stage 8 removes Application-owned D3D12 capture/readback; Stage 9 finishes vendor/native interop exceptions; Stage 12 replaced host presentation internals; Stage 19 slims backend root facades into fuller service objects/folders. |
| 8 | Fully completed | Application validation no longer contains D3D12/Vulkan native capture code; capture uses `RhiCaptureService`; smoke evidence reports backend, view mode, frame, capture path, frame graph warning status, upscaler status, and ray tracing status; launcher smoke request/environment fields include view mode and capture path; `APPLICATION_VALIDATION_NO_BACKEND_NATIVE` exception was removed; boundary check, `SparkleApplicationEditor`, and `SparkleLauncher` built in `build/windows-vs2026-stage5`. | Stage 10 runs full D3D12/Vulkan runtime smoke and captures backend parity artifacts. |
| 9 | Fully completed | `upscaler-provider-contract.md` exists; `SparkleRendererNvidiaDlssProvider` owns Streamline/Vulkan linkage; common `SparkleRenderer` no longer links `Vulkan::Vulkan`; provider diagnostics include structured failure domains; direct and configured boundary checks passed; `SparkleApplicationEditor` built in the VS2026 tree. | Stage 10 runs full D3D12/Vulkan DLSS/capture smoke evidence. |
| 10 | Fully completed | `SparkleLauncher`, `ShaderCompiler`, `ShowcaseEditor`, `ShowcaseRuntime`, and `architecture_boundary_check` passed in `build/windows-vs2026-stage5`; D3D12/Vulkan editor smoke produced Lit and GBufferNormal BMP captures with `frameGraphUnresolvedBarrierWarnings=0`, DLSS active, and `failureDomain=None`; Application validation has no backend-native header matches. Stage 15 fixed the Vulkan runtime present-layout validation error and reran the launcher-shaped runtime smoke with zero diagnostic matches. | Reopen only if capture, interop, upscaling, or runtime Vulkan presentation evidence regresses. |
| 11 | Fully completed | `Renderer.cpp` is now a host facade delegating to `RendererSystemRoot` and `FramePipeline`; subsystem construction/lifetime moved to `Engine/Renderer/Private/Host`; begin/setup/record/submit/end frame, resize, frame graph, viewport products, and frame diagnostics moved to `Engine/Renderer/Private/FramePipeline`; `ShowcaseEditor`, `SparkleLauncher`, and boundary checks passed in `build/windows-vs2026-stage5`. | Stage 12 has replaced public manual render-product transition helpers with a presentation bridge; Stage 15 runs full launcher-shaped D3D12/Vulkan smoke after renderer facade/frame graph work. |
| 12 | Fully completed | `Renderer.h` exposes host-facing viewport presentation lifecycle and capture methods; `ViewportContracts.h` contains presentation/capture DTOs; `FramePipeline` owns texture-id resolution, native resource lookup, and render-product state transitions privately; Editor and smoke validation no longer call manual render-product transition or native-resource resolution helpers; `ShowcaseEditor`, `SparkleLauncher`, and boundary checks passed in `build/windows-vs2026-stage5`. | Stage 15 runs launcher-shaped D3D12/Vulkan smoke after frame graph and presentation work; Stage 19 still slims deeper backend presentation service internals. |
| 13 | Fully completed | `RenderSceneSnapshot` is now an explicit renderer-owned snapshot rather than a `GameSceneSnapshot` subclass; `render-scene-data-contract.md` names camera, light, material, texture, mesh, skinning, temporal, and viewport owners plus diagnostics; the empty private denoising placeholder folder was removed; coverage docs now track mesh runtime identity as a Stage 24 contract pressure instead of hiding it. | Stage 15 adds launcher-shaped smoke/report evidence for mesh, texture, material, and temporal diagnostics; Stage 24 extracts shared `RenderContracts`/`AssetContracts` where needed. |
| 14 | Fully completed | Frame graph invalid imports, incompatible imported UAV usage, invalid AS import/bind data, invalid pass usage, unresolved resource barriers, and unresolved aliasing barriers now fail through hard development validation with actionable diagnostic payloads; `ShowcaseEditor` and `architecture_boundary_check` passed. | Stage 15 runs launcher-shaped D3D12/Vulkan smoke and records runtime graph-clean evidence. |
| 15 | Fully completed | `SparkleLauncher`, `ShowcaseEditor`, `ShowcaseRuntime`, and `architecture_boundary_check` passed in `build/windows-vs2026-stage5`; launcher-shaped direct smoke for D3D12/Vulkan editor Lit and GBufferNormal plus D3D12/Vulkan runtime exited `0`; all six runs reported zero diagnostic matches; editor captures were produced under `artifacts/validation/stage15`; Vulkan runtime present-layout validation errors were fixed in `VulkanRenderCommandList`. | Reopen only if renderer facade, viewport presentation, frame graph diagnostics, shader reload, or resize smoke evidence regresses. |
| 16 | Fully completed | `PipelineRuntimeLibrary` and `PipelineRuntimeKey` exist; existing pass runtimes now load packages, validate capabilities, create binding layouts, log explicit backend-normalized PSO keys, and create RHI PSOs through the runtime library. `ShowcaseEditor` and `architecture_boundary_check` passed in the VS2026 build tree. | Stage 17 has removed the central traits and type-index entry path; Stage 20 captures PSO key logs in smoke evidence. |
| 17 | Fully completed | `RenderPassDefinition` and `RenderPassDefinitionRuntime` exist; all ordinary renderer passes expose definitions; `RenderPassPipelineTraits.h` was deleted; package/layout identity is shared through `RendererShaderPackages`; `ShowcaseEditor`, `ShaderCompiler`, `architecture_boundary_check`, and `ShaderCompiler.exe list-shaders --validate` passed in the VS2026 tree. | Stage 20 captures D3D12/Vulkan smoke with PSO key logs; Stage 22 decides whether `RendererShaderPackages` remains lightweight or becomes a fuller ShaderContracts manifest. |
| 17A | Fully completed | Renderer shader classes no longer hand-write `kShaderName`, `kShaderPackageName`, or `kBindingLayoutId`; package/path/entry/stage metadata is authored once through source metadata registration; `ShaderCompiler`, `ShowcaseEditor`, `architecture_boundary_check`, and shader listing/validation passed in the VS2026 tree. | Stage 17B owns the remaining full pass-authoring friction budget, central registration-list cleanup, and scaffolder/generator decision. |
| 17B | Not started | Added after Stage 17A because pass authoring friction must be measured end-to-end, not only by shader registration line count. | Audit current pass-add touch count and implement a one-command or low-touch authoring workflow with a hard budget for ordinary compute/raster passes. |
| 18 | Not started | Ray tracing ownership contract is documented. | Align RT scene policy, AS contracts, and backend build ownership. |
| 19 | Not started | Backend folder/service symmetry target is documented. | Clean backend services and cross-backend include policy. |
| 20 | Not started | Full graphics validation milestone is documented. | Run full D3D12/Vulkan graphics validation evidence. |
| 21 | Not started | Reviewer presentation requirements are documented. | Build reviewer path, README/evidence navigation, and screenshots/captures as required. |
| 22 | Not started | RHI/Renderer final cleanup gate is documented. | Remove stale graphics exceptions/docs and score final first-track state. |
| 23 | Not started | Whole-repo coverage and folder architecture docs are seeded. | Freeze every durable source root with owner/dependencies/forbidden edges, active refactor stage, validation command, and threading handoff risk. |
| 24 | Not started | Core/Platform target boundaries are documented. | Refactor foundation/platform code so Core stays policy-free and Platform owns OS/window/input without renderer/tool/launcher policy. |
| 25 | Not started | GameFramework contract and Asset/RenderContracts target are documented. | Refactor GameFramework runtime scene/cooked ownership and extract/document shared schema and render snapshot contracts. |
| 26 | Not started | Runtime cooked asset loader/schema pairing is documented. | Pair cooked mesh/material/texture/scene/animation/skeleton loader expectations with producer cookers and diagnostics. |
| 27 | Not started | Source importer target naming is documented. | Rename/extract SourceImportAdapters toward SourceImporters with imported DTO/report contracts and runtime dependency cleanup. |
| 28 | Not started | Focused cooker and support split is documented. | Refactor Texture/Mesh/Material/Scene cookers and split CookCommon into precise support/diagnostics owners. |
| 29 | Not started | ShaderCompiler contract target is documented. | Refactor ShaderCompiler around ShaderContracts, deterministic jobs, package/reflection reports, and full renderer runtime decoupling. |
| 30 | Not started | AssetCooker/Launcher/host split is documented. | Refactor AssetCooker orchestration, retire AssetConverter production path, and enforce LauncherCore/Qt GUI/Application/Editor host boundaries. |
| 31 | Not started | Artifact validation matrix requirements are documented. | Build producer/schema/consumer/inspector evidence for shader and cooked artifacts. |
| 32 | Not started | Project and engine asset root policy is documented. | Refactor Projects and Engine/Assets ownership, sample validation content, generated output policy, and Showcase evidence paths. |
| 33 | Not started | Future guardrail rules are documented. | Expand local/CI checks for runtime/tools/folder/generated-root policy and threading-hostile handoffs. |
| 34 | Not started | Whole-repository evidence gate is documented. | Run whole-repo evidence and status reconciliation before the final threading-readiness audit. |
| 35 | Not started | Threading-readiness contract and final audit stage are documented. | Audit all modules/stages for mutable owners, immutable handoffs, command batches, queue packets, tool jobs, launcher reports, and deterministic diagnostics. |
| 36 | Not started | Final whole-repository cleanup and rubric gate is documented. | Delete stale transition paths, score the whole repo, and confirm review-ready evidence across all module families. |

## Stage Groups

| Stages | Theme | Primary before docs | Primary after docs | Required evidence |
| --- | --- | --- | --- | --- |
| 1-3 | Baseline coverage, vocabulary, mechanical boundary guardrails | [current state](../../architecture/before/repository-current-state.md), [rendering coverage](../../architecture/rendering-coverage-status.md) | [target architecture](../../architecture/after/repository-target-architecture.md), [guardrails](../../architecture/architecture-boundary-guardrails.md) | Coverage maps exist; boundary checks are local/CI-friendly and exceptions are counted. |
| 4-5 | Renderer-owned shader registration and first validation | [RHI/Renderer review](../rhi-renderer-architecture-review.md) | [pass authoring](../../architecture/pass-authoring-contract.md), [pipeline runtime](../../architecture/pipeline-runtime-contract.md) | RHI has no renderer-private includes; ShaderCompiler can still enumerate packages. |
| 6-10 | RHI ownership, services, capture/readback, vendor interop, backend parity milestone | [current state](../../architecture/before/repository-current-state.md), [RHI contract map](../../architecture/rhi-contract-map.md) | [target architecture](../../architecture/after/repository-target-architecture.md), [guardrails](../../architecture/architecture-boundary-guardrails.md), [validation workflow](../../architecture/validation-workflow-contract.md) | RHI method ownership exists; Application validation delegates backend-native work; D3D12/Vulkan evidence is launcher-shaped and recorded. |
| 11-15 | Renderer facade, presentation, scene/resource ownership, frame graph contract, graph validation | [rendering coverage](../../architecture/rendering-coverage-status.md) | [rendering system map](../../architecture/rendering-system-map.md), [frame graph contract](../../architecture/frame-graph-contract.md), [validation workflow](../../architecture/validation-workflow-contract.md) | Renderer becomes a clearer facade; frame graph diagnostics are actionable; host presentation uses stable products validated through launcher-shaped smoke. |
| 16-20 | Shader package, pass authoring, PSO runtime, ray tracing, backend services, full graphics validation | [RHI/Renderer review](../rhi-renderer-architecture-review.md) | [pass authoring](../../architecture/pass-authoring-contract.md), [pipeline runtime](../../architecture/pipeline-runtime-contract.md), [ray tracing](../../architecture/ray-tracing-contract.md), [validation workflow](../../architecture/validation-workflow-contract.md) | PSO keys are explicit; pass additions avoid RHI edits; RT ownership is clear; D3D12/Vulkan smoke evidence follows launcher-shaped user paths. |
| 21-22 | Reviewer presentation and RHI/Renderer final gate | [acceptance rubric](../architecture-review-acceptance-rubric.md) | [target system design index](../../architecture/after/system-design-index.md) | README/reviewer path, final cleanup, and RHI/Renderer rubric scoring are complete. |
| 23 | Whole-repository coverage and dependency map | [current state](../../architecture/before/repository-current-state.md) | [repository target](../../architecture/after/repository-target-architecture.md), [repository coverage](../../architecture/repository-coverage-status.md) | Every durable source root has owner, dependencies, validation target, active refactor stage, and acceptance evidence. |
| 24-26 | Core/Platform and GameFramework/runtime contracts | [current state](../../architecture/before/repository-current-state.md), [GameFramework contract](../../architecture/game-framework-contract.md) | [repository target](../../architecture/after/repository-target-architecture.md), [GameFramework contract](../../architecture/game-framework-contract.md) | Foundation/platform policy is clean; GameFramework runtime/cooked schemas are paired with tools and renderer handoffs. |
| 27-30 | Importers, cookers, shader tooling, AssetCooker, launcher, and hosts | [tooling pipeline contract](../../architecture/tooling-pipeline-contract.md) | [tooling pipeline contract](../../architecture/tooling-pipeline-contract.md), [target folder architecture](../../architecture/after/repository-target-folder-architecture.md) | Tool roles are split, weak names are replaced, duplicate production paths retire, and launcher/hosts orchestrate through requests/reports. |
| 31-32 | Artifact matrix, Projects, and Engine assets | [tooling pipeline contract](../../architecture/tooling-pipeline-contract.md), [coverage status](../../architecture/repository-coverage-status.md) | [target system design index](../../architecture/after/system-design-index.md), [validation workflow](../../architecture/validation-workflow-contract.md) | Every artifact/sample asset has producer, schema owner, consumer, inspector, and smoke/load evidence. |
| 33 | Build, CI, and boundary guardrail expansion | [architecture guardrails](../../architecture/architecture-boundary-guardrails.md) | [repository target](../../architecture/after/repository-target-architecture.md) | Checks cover runtime-to-tools, GameFramework/private coupling, launcher/tool ownership, generated folders, CMake scopes, and threading-hostile handoffs. |
| 34 | Whole-repository evidence gate | [current state](../../architecture/before/repository-current-state.md), [coverage status](../../architecture/repository-coverage-status.md) | [target architecture](../../architecture/after/repository-target-architecture.md), [system design index](../../architecture/after/system-design-index.md) | No unowned `Needs refactor` rows remain before the final threading-readiness audit. |
| 35 | Threading-readiness final audit | [current state](../../architecture/before/repository-current-state.md), [repository threading readiness](../../architecture/after/repository-threading-readiness.md) | [target architecture](../../architecture/after/repository-target-architecture.md), [target graphs](../../architecture/after/repository-target-graphs.md), [system design index](../../architecture/after/system-design-index.md) | All modules and stage outputs are safe for future worker threads, render-thread work, async queues, cook jobs, shader jobs, and launcher process workflows without private mutable cross-owner access. |
| 36 | Final whole-repository cleanup and rubric gate | [acceptance rubric](../architecture-review-acceptance-rubric.md), [coverage status](../../architecture/repository-coverage-status.md) | [system design index](../../architecture/after/system-design-index.md), [stage map](repository-refactor-stage-map.md) | Stale paths are deleted, rubric scores are recorded, and the repository is review-ready as one cohesive architecture. |

## Module To Stage Index

| Module or subsystem | Main stages |
| --- | --- |
| Core / Platform | 23, 24, 33, 34, 35, 36 |
| RHI common and backends | 3, 6, 7, 8, 9, 19, 20, 22, 28, 29, 30 |
| Renderer facade/frame/passes/pipeline | 4, 10-17, 20, 22, 29, 30 |
| Ray tracing | 18, 20, 22, 29, 30 |
| GameFramework | 13, 25, 26, 31, 34, 35, 36 |
| Editor/Application | 8, 12, 20, 30, 34, 35, 36 |
| ShaderCompiler | 4, 5, 16, 17, 20, 29, 31, 34, 35, 36 |
| SourceImporters / current SourceImportAdapters | 27, 31, 33, 34, 35, 36 |
| Texture/Mesh/Material/Scene cookers | 26, 28, 31, 34, 35, 36 |
| AssetCooker / retired AssetConverter commands / ToolConsoleSupport / CookDiagnostics | 28, 30, 31, 34, 35, 36 |
| SparkleLauncher | 10, 20, 21, 30, 33, 34, 35, 36 |
| CMake / CI | 3, 5, 20, 23, 33, 34, 35, 36 |
| Projects / Showcase | 10, 20, 31, 32, 34, 35, 36 |
| Folder architecture / source-root ownership | 1-36 |
| Threading readiness / data isolation | 1-36 |
| Docs / reviewer presentation | 1, 2, 21, 22, 23, 29, 30 |

## Disposition Outcomes By Stage

| Stage group | Keep/refine | Improve/extract | Replace/redesign |
| --- | --- | --- | --- |
| 1-3 | Existing boundary check entry points and docs navigation. | Guardrails become repository-wide instead of RHI/Renderer-only. | Broad allowlists and undocumented exceptions. |
| 4-5 | Generic RHI shader package primitives. | Renderer pass metadata moves toward `ShaderContracts`. | RHI-owned renderer pass registration. |
| 6-10 | Backend-private D3D12/Vulkan roots. | Broad RHI facade becomes named services and contracts. | Application-owned D3D12 capture/readback body. |
| 11-15 | Frame graph ownership and diagnostics. | Renderer facade splits into host facade, frame pipeline, scene staging, and presentation products. | Renderer consuming mutable GameFramework internals as the long-term handoff. |
| 16-20 | `PassBinder`, pass definitions, and normalized RHI pipeline descriptors. | `RenderPassShaderRuntime` becomes observable pipeline stages fed by `RenderPassDefinition`. | Type-index PSO identity and central per-pass runtime traits. |
| 23-24 | Core foundation utilities and Platform OS/window/input role. | Repository-wide coverage becomes active refactor routing; Core/Platform shed domain policy. | Core/Platform as renderer/tool/launcher policy sinks or speculative scheduler owners. |
| 25-26 | GameFramework runtime scene/cooked loading role. | Shared schemas and render handoff become `AssetContracts` and `RenderContracts`; loaders pair with producer cookers. | GameFramework as source-import/schema-dump/renderer pipeline owner. |
| 27-29 | Focused import and shader/tool transforms. | `SourceImportAdapters` becomes `SourceImporters`; cookers emit deterministic artifacts; ShaderCompiler consumes `ShaderContracts`. | Runtime source import, shader compiler linking full renderer runtime, or duplicate pass metadata forever. |
| 30 | AssetCooker orchestration and Qt GUI presentation/model split. | AssetCooker and LauncherCore route operation data through `ToolContracts`; AssetConverter production path retires. | Launcher widgets owning build/cook/shader algorithms or AssetCooker owning focused transforms. |
| 31-32 | Showcase/sample content as evidence and artifacts as contracts. | Project/engine asset roots become owned validation inputs with artifact matrices and inspector commands. | Ambiguous `Engine/Assets`, generated outputs in source roots, or sample content as hidden production data. |
| 33-36 | Existing serial paths that are simpler and validated. | Guardrails, evidence, threading readiness, final cleanup, and rubric scoring prove the whole repo. | Broad global locks, speculative job systems, stale duplicate paths, weak non-rendering module scores, and worker access to private mutable owner state. |

## Stage-By-Stage Folder Architecture Budget

Each implementation prompt must name the current folders touched, the target folders strengthened, the folders that must not receive code, and the cleanup path for old folders.

| Stage | Folder shape to make truer | Folder shape to reject |
| --- | --- | --- |
| 1 | Coverage docs accurately inventory source roots and generated/local-only roots. | Coverage that hides roots or treats scratch folders as architecture. |
| 2 | Glossary and maps use target folder names from the naming canon. | Diagrams that preserve noisy folder names without disposition. |
| 3 | Boundary checks live in local/CMake-friendly check files with documented exceptions. | Broad allowlists or CI-only folder policy. |
| 4 | Renderer pass shader metadata moves out of `Engine/RHI/Private/Shaders` toward renderer pass catalog and `ShaderContracts`. | RHI-owned renderer pass folders or duplicate shader registration roots. |
| 5 | Validation commands reference the new shader registration target/folder names. | Build evidence that still relies on old RHI pass folders. |
| 6 | RHI public/private folders map to method ownership categories. | Facade categories with no folder or service consequence. |
| 7 | `Engine/RHI/Public/{Interop,Capture,Diagnostics,Presentation}` names the first service contracts; backend roots compose symmetric service adapters until Stage 19. | Splitting one broad RHI folder into many vague service-locator folders, backend facade multiple inheritance, or pretending compatibility shims are the final backend shape. |
| 8 | Application validation stays host orchestration; backend capture/readback moves to RHI/backend folders. | D3D12/Vulkan capture code under Application. |
| 9 | Vendor providers stay under renderer feature/provider folders and use RHI interop contracts. | Vendor SDK/native API folders in ordinary passes, GameFramework, or Application. |
| 10 | Smoke artifacts and sample paths are organized as validation evidence, not source folders. | One-off validation output under durable source roots. |
| 11 | Renderer host facade separates from frame pipeline, features, scene staging, and diagnostics. | One giant renderer private folder hiding subsystem ownership. |
| 12 | Presentation/viewport bridge folders expose host-facing products. | Hosts reading frame graph or backend folders directly. |
| 13 | Renderer scene/resource folders consume GameFramework snapshots/contracts. | Renderer resource folders including GameFramework private or tool folders. |
| 14 | Frame graph compiler/execution/resources/diagnostics folders remain explicit. | Pass code hiding resource/barrier ownership. |
| 15 | Validation evidence names frame graph folders and sample content roots. | Advancing with unresolved resource warnings hidden in logs only. |
| 16 | `PipelineRuntime`, `PassCatalog`, and `Engine/Contracts/Shader` own package/PSO identity. | Type-index traits or duplicate registries in unrelated folders. |
| 17 | Pass authoring folders allow new passes without RHI edits. | Central folder edits for every ordinary pass. |
| 18 | Ray tracing folders stay renderer-feature owned; RHI owns AS contracts/backends only. | Moving shadow/pass data into RHI or GameFramework folders. |
| 19 | D3D12/Vulkan backend folders are sibling implementations with matching service names where useful. | Cross-backend includes or shared native helper folders that hide API ownership. |
| 20 | Full graphics validation links artifacts to backend, renderer, and project folders. | Passing one backend while the folder evidence assumes parity. |
| 21 | Reviewer docs point to before/after, graph, contract, and folder docs. | More docs without navigation value. |
| 22 | RHI/Renderer cleanup deletes stale transition folders and exceptions. | Keeping old registration/provider/backend shortcuts for comfort. |
| 23 | Every durable root has an owner, allowed dependencies, forbidden dependencies, active refactor stage, and validation command. | Empty, ambiguous, or unowned source roots. |
| 24 | Core and Platform folders remain foundation/OS-window-input focused. | Core/Platform folders receiving renderer, tool, launcher, gameplay, or scheduler policy. |
| 25 | Shared schemas move toward `Engine/Contracts/Asset` and renderer handoff toward `Engine/Contracts/Render`. | GameFramework becoming a schema dump, source importer, or renderer-pass folder. |
| 26 | Runtime loader folders stay GameFramework-owned while shared cooked schemas move to contract roots when needed. | Tools depending on private GameFramework loader folders or runtime loaders parsing source formats. |
| 27 | `Tools/Import/SourceImportAdapters` migrates toward `Tools/Import/SourceImporters`. | Adapter/importer folders kept as parallel production paths. |
| 28 | Focused cookers and `ToolConsoleSupport`/`CookDiagnostics` have separate role folders. | `CookCommon`, `Tools/Common`, `Tools/Utils`, or generic conversion folders as production architecture. |
| 29 | ShaderCompiler consumes `ShaderContracts` and package/report folders. | Shader compiler folders depending on full renderer runtime or duplicating pass metadata indefinitely. |
| 30 | AssetCooker, LauncherCore/workflow folders, Qt GUI folders, and host folders remain separate. | Launcher folders containing cooker/compiler/render algorithms or AssetConverter as a production path. |
| 31 | Artifact inspection/validation folders are read-only and tied to contracts. | Inspectors mutating production cook policy. |
| 32 | Project data/shader roots and Engine built-in asset roots have explicit policies. | Generated outputs in durable roots or `Engine/Assets` as a catch-all. |
| 33 | `CMake/Checks`, CMake target scopes, and CI/local commands enforce folder edges. | Generated/local roots becoming durable architecture or checks hidden in CI only. |
| 34 | Final source-root inventory matches target folder architecture or stricter documented alternatives. | Duplicate old/new folders, ambiguous roots, stale aliases, or compatibility shims. |
| 35 | Optional future `Engine/Contracts/Threading`/`Tools/Contracts` additions only when real callers need job, command-batch, queue-packet, or report primitives. | Premature global scheduler folders, worker code under owner-private folders, or "threading" folders that hide domain policy. |
| 36 | Final cleanup deletes stale folders, targets, docs, exceptions, and duplicate paths. | Review-ready claims with stale compatibility folders or weak module scores. |

## Stage-By-Stage Complexity Budget

Each stage must show that retained code earns its right to exist.

| Stage | Complexity allowed to remain | Complexity to reduce, delete, or justify |
| --- | --- | --- |
| 1 | Coverage/status tables that expose real ownership. | Stale rows, unowned roots, and docs that hide risk. |
| 2 | Glossary and system maps that shorten reviewer navigation. | Duplicate terms, vague labels, and diagrams that preserve bad edges. |
| 3 | Small mechanical checks with counted exceptions. | Broad allowlists and silent suppression. |
| 4 | Generic shader package primitives and renderer-owned pass metadata. | RHI-owned renderer pass registration and renderer-private includes from RHI. |
| 5 | Minimal validation commands proving Stage 4 behavior. | Build-only confidence without package enumeration evidence. |
| 6 | RHI method ownership table if it clarifies service extraction. | Facade categories that do not drive code or guardrail changes. |
| 7 | Focused RHI services with caller evidence. | Splitting one broad facade into many unclear facades. |
| 8 | Validation orchestration and backend capture services. | Application-owned D3D12/Vulkan capture implementation. |
| 9 | Narrow provider/native interop contracts with diagnostics. | Renderer-level Vulkan/D3D12 leakage and provider SDK shortcuts. |
| 10 | Backend parity evidence tied to sample paths. | One-off smoke paths that cannot be reproduced. |
| 11 | Renderer facade as host protocol. | Subsystem ownership hidden behind one large renderer object. |
| 12 | Presentation bridge and viewport products. | Hosts reading frame graph internals or backend resources. |
| 13 | Render-domain scene/resource staging. | Renderer depending on mutable GameFramework internals. |
| 14 | Frame graph diagnostics, resource planning, barrier evidence. | Suppressed warnings and hidden pass/resource coupling. |
| 15 | Milestone validation artifacts. | Advancing while frame graph warnings remain unexplained. |
| 16 | Explicit `PipelineRuntimeLibrary`, `PsoKey`, and package/runtime separation. | Type-index PSO identity and opaque lazy runtime storage. |
| 17 | `RenderPassDefinition` authoring and shared renderer shader package IDs. | Central per-pass runtime traits for ordinary passes. |
| 17B | Pass authoring friction budget and generated/scaffolded mechanical records. | Hand-written central registration edits and repeated pass/package boilerplate for ordinary passes. |
| 18 | Renderer AS scene policy plus RHI AS build contracts. | Moving shadow/pass data into RHI or GameFramework as a shortcut. |
| 19 | Symmetric backend services and named differences. | Cross-backend includes and backend-private policy leaks. |
| 20 | Full graphics evidence across D3D12/Vulkan. | Passing one backend and assuming the other is fine. |
| 21 | Reviewer path docs that reduce orientation cost. | More docs without navigation or ownership value. |
| 22 | Final RHI/Renderer debt cleanup. | Transitional exceptions that survived their owning stage. |
| 23 | Whole-repo map with every durable root owned and routed to an implementation stage. | Empty roots, stale target names, hidden dependency direction, and audit-only follow-up. |
| 24 | Core/Platform abstractions that have multiple consumers and no domain policy. | Foundation modules used as convenience dumping grounds. |
| 25 | `AssetContracts` and `RenderContracts` around GameFramework. | GameFramework as source-import/schema/renderer pipeline owner. |
| 26 | Runtime loaders and cooked schemas with paired producer/consumer evidence. | Schemas understood only by private loader code or producer-only assumptions. |
| 27 | `SourceImporters` and imported DTO diagnostics. | Runtime source import or adapter naming as final design. |
| 28 | Focused cookers and `ToolConsoleSupport`/`CookDiagnostics`. | `CookCommon` as broad policy sink or focused cookers owning orchestration. |
| 29 | ShaderCompiler contracts, deterministic jobs, and package/reflection reports. | Full renderer runtime links or duplicate shader metadata without removal. |
| 30 | AssetCooker orchestration, LauncherCore requests/reports/history, and Qt presentation split. | Widgets owning tool algorithms, AssetCooker owning focused transforms, or AssetConverter production path. |
| 31 | Artifact matrix with producer/schema/consumer/inspector/evidence. | Artifact compatibility proven only by build success. |
| 32 | Project/sample/engine asset roots as deliberate evidence surfaces. | Engine assets or project data as vague file drops. |
| 33 | Boundary checks that enforce target edges locally and in CI. | Checks that only document debt but do not fail new drift. |
| 34 | Final evidence set that removes unearned complexity. | Any duplicate path, vague owner, or exception without an accepted follow-up. |
| 35 | Threading-readiness evidence that names owners, phases, handoffs, ordering, and diagnostics. | Global locks, speculative schedulers, nondeterministic tool output, or hidden mutable cross-module reads. |
| 36 | Final cleanup and rubric scoring for the whole repository. | Review-ready claims based only on RHI/Renderer strength while other modules remain weak. |

## Stage Acceptance Checklist

For any stage:

- Open the before/current document for the affected subsystem.
- Open the Codex packet in [../implementation/stage-prompt-packets.md](../implementation/stage-prompt-packets.md) and the tutor note in [../tutor/stage-learning-guide.md](../tutor/stage-learning-guide.md).
- Open the after/target contract for the subsystem.
- Open the matching row in [Required Target Documents By Stage](../rhi-renderer-review-ready-implementation-plan.md#required-target-documents-by-stage).
- Open the matching row in [Stage Contract Coverage Matrix](../rhi-renderer-review-ready-implementation-plan.md#stage-contract-coverage-matrix).
- Complete any listed row in [Mandatory Split Checkpoints For Large Stages](../rhi-renderer-review-ready-implementation-plan.md#mandatory-split-checkpoints-for-large-stages), or split the unfinished checkpoint into a new numbered stage before implementation continues.
- Classify each touched subsystem using the refactor disposition policy in [../../architecture/after/repository-target-architecture.md](../../architecture/after/repository-target-architecture.md).
- Apply the stage complexity budget above and name the code that earns its right to remain.
- Confirm no explanatory/provenance/planning comments were added to source. New runtime strings must be behavior or diagnostic text only.
- For moved code, prove the destination is the correct owner, the code was reshaped to that owner's vocabulary/contracts, and the old responsibility was deleted or simplified.
- Apply [repository-threading-readiness.md](../../architecture/after/repository-threading-readiness.md) and name mutable owner, phase, handoff shape, isolation, ordering/synchronization expectation, and deterministic diagnostics for changed edges.
- Check new names against the repository naming canon before adding files, targets, types, commands, or schemas.
- Confirm the `Global Refactor Stage Impact Matrix` in the canonical implementation plan names the adjacent modules to protect.
- Run the smallest meaningful validation available locally.
- For editor/runtime validation, use Launcher or launcher-shaped direct execution from [validation-workflow-contract.md](../../architecture/validation-workflow-contract.md); record operation/project/target/profile, working directory, environment, logs, and artifacts.
- Update coverage/status docs when ownership, dependencies, or validation expectations change.
- Update the `Stage Status Tracker` row when the stage starts, reaches validation/cleanup-only state, or is fully accepted.

# Sparkle Repository Review-Ready Implementation Plan

Status: canonical whole-repository execution plan
Date: 2026-06-12
Last synchronized: 2026-06-13
Scope: whole-repository architecture, with the first implementation track focused on `Engine/RHI`, `Engine/Renderer`, D3D12, Vulkan, ray tracing, frame graph, shader/pass runtime, PSO handling, upscaling, and smoke validation. The plan now explicitly tracks GameFramework, Launcher, ShaderCompiler, AssetCooker, TextureCooker, source import, cookers, CMake, CI, projects, and docs so RHI/Renderer refactors do not degrade adjacent modules.

Navigation:

- Stage map: [after/repository-refactor-stage-map.md](after/repository-refactor-stage-map.md)
- Architecture current state: [../architecture/before/repository-current-state.md](../architecture/before/repository-current-state.md)
- Architecture target system index: [../architecture/after/system-design-index.md](../architecture/after/system-design-index.md)
- Target folder architecture: [../architecture/after/repository-target-folder-architecture.md](../architecture/after/repository-target-folder-architecture.md)
- Threading readiness contract: [../architecture/after/repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md)

## Purpose

This document turns the architecture review and acceptance rubric into an execution runbook. The goal is to make SparkleEngine feel designed as a coherent renderer/runtime/tooling repository rather than a set of accumulated fixes.

Primary source documents:

- `docs/plans/sparkle-whole-repository-architecture-review.md`
- `docs/plans/rhi-renderer-architecture-review.md`
- `docs/plans/architecture-review-acceptance-rubric.md`

Stage artifacts:

- Whole-repo architecture status:
  - `docs/plans/sparkle-whole-repository-architecture-review.md`
  - `docs/architecture/repository-system-map.md`
  - `docs/architecture/repository-coverage-status.md`
  - `docs/architecture/after/repository-target-folder-architecture.md`
  - `docs/architecture/after/repository-threading-readiness.md`
  - `docs/architecture/game-framework-contract.md`
  - `docs/architecture/tooling-pipeline-contract.md`
- Stage 1 baseline status: `docs/architecture/rendering-coverage-status.md`
- Stage 2 reviewer docs:
  - `docs/architecture/rendering-glossary.md`
  - `docs/architecture/rendering-system-map.md`
  - `docs/architecture/rhi-contract-map.md`
  - `docs/architecture/frame-graph-contract.md`
  - `docs/architecture/ray-tracing-contract.md`
  - `docs/architecture/pass-authoring-contract.md`
  - `docs/architecture/pipeline-runtime-contract.md`
- Stage 3 mechanical guardrails:
  - `CMake/ArchitectureBoundaryCheck.cmake`
  - `architecture_boundary_check`
  - `docs/architecture/architecture-boundary-guardrails.md`

Current code evidence used while writing this plan:

- `Engine/RHI/Public/Device/RenderHardwareInterface.h`
- `Engine/RHI/Public/Commands/RenderCommandList.h`
- `Engine/RHI/Public/Pipeline/RhiPipelineStateDesc.h`
- `Engine/RHI/Private/Shaders/BuiltinGlobalShaders.cpp`
- `Engine/Renderer/ShaderRegistrations/RendererGlobalShaders.cpp`
- `Engine/Renderer/ShaderRegistrations/DirectLightingShaders.cpp`
- `Engine/Renderer/Public/Renderer.h`
- `Engine/Renderer/Private/Renderer.cpp`
- `Engine/Renderer/Private/Passes/RenderPassDefinition.h`
- `Engine/Renderer/Private/Pipeline/RenderPassDefinitionRuntime.h`
- `Engine/Renderer/ShaderRegistrations/RendererShaderPackages.h`
- `Engine/Renderer/Private/Pipeline/PipelineStateManager.h`
- `Engine/Renderer/Private/Pipeline/RenderPassShaderRuntime.h`
- `Engine/Renderer/Private/Pipeline/PassBinder.cpp`
- `Engine/Application/Private/Validation/RhiSmokeEditorValidation.cpp`
- `Engine/Application/Private/Validation/RhiSmokeValidation.cpp`
- `Engine/Renderer/CMakeLists.txt`
- `Engine/RHI/CMakeLists.txt`
- `Engine/GameFramework/CMakeLists.txt`
- `Tools/Launcher/SparkleLauncher/CMakeLists.txt`
- `Tools/Shaders/ShaderCompiler/CMakeLists.txt`
- `Tools/Cooking/TextureCooker/CMakeLists.txt`
- `Tools/Cooking/AssetCooker/CMakeLists.txt`
- `Tools/Import/SourceImportAdapters/CMakeLists.txt`

## Execution Rules

- Stages are numbered only as `1`, `2`, `3`, and so on. Do not introduce nested stage numbers.
- Each stage must be completed cleanly before moving to the next stage.
- Stage status lives in [after/repository-refactor-stage-map.md](after/repository-refactor-stage-map.md) and uses exactly `Not started`, `Started`, `Almost finished`, and `Fully completed`.
- Update the stage status row when implementation begins, when the stage reaches validation/cleanup-only state, and when acceptance evidence is complete.
- Do not mark a stage `Fully completed` because design docs exist; completion requires the stage acceptance and validation evidence in this plan.
- Each stage must open the matching row in `Required Target Documents By Stage` below before implementation. Stage-local source references are not enough by themselves; the target docs define the shape we are moving toward.
- Each stage must classify touched systems as `Keep and refine`, `Improve and extract`, or `Replace or redesign` using `docs/architecture/after/repository-target-architecture.md`. Existing bodies, names, folders, and CMake targets are not preserved unless they match the target ownership model.
- Each stage must apply the right-to-exist complexity test: every retained abstraction, compatibility path, helper, registry, target, schema, and command must name the problem it uniquely solves and the validation or maintenance value that justifies it.
- Each stage must treat folder architecture as architecture: name current source folders, target folders, forbidden folder edges, CMake target changes, and cleanup/deletion of replaced paths.
- Each stage must treat threading readiness as architecture: name the mutable owner, execution phase, handoff shape, isolation boundary, ordering/synchronization expectation, diagnostics identity, and deterministic-output expectation for every changed edge.
- Temporary compatibility adapters are allowed only inside a stage. They must be removed before that stage is accepted unless the stage explicitly says otherwise.
- Do not keep legacy paths "just in case." If the new path replaces the old path and validation passes, delete the old path in the same stage or the immediately following cleanup stage.
- Rename, split, merge, or rebuild systems when the current shape would require broad exceptions, duplicate owners, private dependency edges, or misleading names.
- Do not add explanatory, provenance, planning, stage, or refactor-process comments to source code. Source comments are allowed only when they clarify non-obvious runtime behavior, API constraints, or safety/lifetime rules that the code cannot express cleanly. Runtime/user-facing strings must describe behavior or diagnostics, not implementation history.
- Do not move clutter from one owner to another. A move is accepted only when the destination module, folder, type, or target is the designed owner, the moved code is reshaped to fit that owner's vocabulary and contracts, and the old owner loses the responsibility rather than delegating confusion elsewhere.
- Do not run full builds after every small edit. Run build/runtime validation at the milestone stages in this document, or earlier only when a local compile failure blocks progress.
- Every strategic code stage must include the rubric fields from `docs/plans/architecture-review-acceptance-rubric.md`: owner, dependency impact, D3D12/Vulkan impact, validation plan, risks, and rollback path.
- Every renderer pass or shader change must obey the hard gate from `docs/plans/rhi-renderer-architecture-review.md`: adding an ordinary renderer shader pass must not require editing `Engine/RHI`.
- Every RHI change must answer whether it introduces a GPU/API concept, a backend implementation detail, or a renderer convenience. Renderer conveniences do not belong in RHI.
- Every RHI/Renderer change must check GameFramework and tool blast radius: ShaderCompiler, TextureCooker, AssetCooker, SourceImporters/current SourceImportAdapters, Launcher workflows, Application validation, and project/sample content where relevant.
- Every stage must satisfy the `Global Refactor Stage Impact Matrix` below. A stage is not accepted until its local work and its whole-repository protection checks are both addressed.
- Every stage must satisfy the threading-readiness contract in `docs/architecture/after/repository-threading-readiness.md`. A stage does not need to add threads, but it must not leave a data shape that would require future workers to read private mutable owner state.
- Every stage must satisfy its row in `Stage Contract Coverage Matrix` below. If the row names a contract surface, the implementation prompt, acceptance evidence, and validation notes must prove that surface was preserved or improved.
- Every stage listed in `Mandatory Split Checkpoints For Large Stages` must complete each checkpoint independently before the stage can be marked `Almost finished`. If a checkpoint cannot be completed, validated, and documented cleanly in the same implementation window, promote that checkpoint into a new numbered stage before continuing.
- Runtime engine modules must not depend on tool internals. Tools may consume public runtime/cooked contracts, but source import and cooking algorithms must stay under `Tools/`.
- Launcher changes must preserve the split between workflow/process orchestration and Qt UI presentation.
- Every backend parity claim must be backed by logs, smoke reports, screenshots/captures, or a clearly marked measurement plan.

## Implementation Prompt Guardrail Contract

Every stage implementation prompt is governed by the stage-local positive guardrails, negative guardrails, and data-transfer contracts in this plan. If a stage edits code, CMake, docs, or validation wiring, the implementation must name:

- The refactor disposition: `Keep and refine`, `Improve and extract`, or `Replace or redesign`.
- The complexity being kept, reduced, or removed, and why the remaining code earns its right to exist.
- The owner module that is allowed to change.
- The intended production name if a file, target, type, command, schema, or module is renamed.
- The modules/files that must not be touched or referenced directly.
- The allowed data transfer mechanism between systems.
- The diagnostics or validation evidence that proves the handoff works.
- The deleted, merged, renamed, or simplified path that reduces future maintenance.
- The folder architecture effect: current path, target path, source root owner, forbidden destination folders, and any CMake target rename or split.
- The threading-readiness effect: mutable owner, phase, handoff shape, isolation scope, queue/job/batch identity when relevant, ordering rule, and deterministic diagnostics.
- The transitional exception, if any, including the exact removal stage.
- The source-text effect: any added source comment or runtime string must be justified as necessary behavior/API/diagnostic text, never as planning, migration, authorship, or stage narration.
- The destination-fit proof for any moved code: why the new owner is correct, what was renamed/reshaped to fit that context, what responsibility was deleted from the old owner, and what prevents the move from becoming displaced clutter.

Stage completion packet:

Every stage implementation record must include these fields before the stage can be accepted:

| Field | Required content |
| --- | --- |
| Target docs opened | The stage row from `Required Target Documents By Stage`, plus any touched subsystem row from `system-design-index.md`. |
| Contract surfaces | Which of `RhiContracts`, `RenderContracts`, `ShaderContracts`, `AssetContracts`, `ToolContracts`, `ThreadingReadiness`, folder architecture, and mechanical guardrails were touched. |
| Rubric critical criteria | Evidence for requirements/constraints, separation of concerns, runtime behavior clarity, observability, reliability/failure handling, threading readiness, maintainability/naming, complexity right-to-exist, testability, and communication/reviewability where relevant. |
| Split decision | Whether the stage was small enough to complete as written, which mandatory checkpoints were completed, or which checkpoint must become a new numbered stage. |
| Ownership/disposition | `Keep and refine`, `Improve and extract`, or `Replace or redesign` for every touched module, folder, target, schema, command, or compatibility path. |
| Folder and target plan | Current folders, target folders, forbidden destination folders, CMake target changes, and cleanup/deletion paths. |
| Data transfer plan | Producer, contract shape, schema/package/job/frame id, consumer, diagnostics, and validation command. |
| Threading-readiness plan | Mutable owner, phase, immutable or versioned handoff, isolation scope, ordering/synchronization expectation, and deterministic output/report behavior. |
| Source text discipline | Confirmation that no explanatory/provenance/planning comments were added to source, and that new runtime strings are behavior or diagnostic text only. |
| Destination-fit proof | For moved code, evidence that the destination is the real owner, names/contracts were adapted to that context, and no new catch-all or displaced-clutter owner was created. |
| Acceptance evidence | Observable file/code/CMake/docs state that proves the stage goal, not only a statement of intent. |
| Validation evidence | Commands run, artifacts/logs inspected, commands not run and why, and remaining risk owner. |

Acceptance completeness rule:

- A stage acceptance section is incomplete if it does not prove the matching contract surfaces in `Stage Contract Coverage Matrix`.
- A broad stage is incomplete if any checkpoint in `Mandatory Split Checkpoints For Large Stages` is skipped, merged silently, or left without validation/evidence.
- A stage is too large when it touches more than one contract surface and any surface lacks an independent validation artifact. Split it before implementation instead of hoping the final gate catches drift.

Positive edge rule:

- Prefer explicit public contracts, DTOs, manifests, registries, process requests, CMake target links, and validation artifacts over private includes or ad hoc global state.
- Prefer one-way data flow from producer to consumer: source import produces DTOs, cookers produce cooked artifacts, GameFramework produces runtime snapshots, Renderer produces render products, RHI produces backend evidence, Launcher records process evidence.
- Prefer future-threading-safe handoffs: immutable snapshots, DTOs, manifests, command batches, queue submission packets, tool job requests, process requests, and reports.
- Prefer narrow targets and APIs such as `SparkleRendererShaderRegistrations`, public cooked schemas, RHI public primitives, LauncherCore process requests, and inspection commands.
- Prefer names from the repository naming canon: `*Contracts`, `*Backend`, `*Pass`, `*FrameGraph`, `*PipelineRuntime`, `*Dto`, `*Snapshot`, `*Manifest`, `Cooked*`, `*Cooker`, `*Compiler`, `*Importer`, `*Provider`, `*Request`, `*Report`, and `*History`.
- Prefer deleting duplicate paths, reducing ceremony, consolidating ownership, and making diagnostics stronger before adding new abstractions.

Negative edge rule:

- Do not make a higher layer solve a lower-layer boundary problem by pulling in private headers.
- Do not move data to the wrong module just to satisfy an include check.
- Do not add broad allowlists, generic service locators, untyped `void*` plumbing, hidden globals, or duplicate registries as band-aid solutions.
- Do not let runtime modules depend on `Tools/*` implementation, `Engine/RHI/Private`, `Engine/Renderer/Private`, backend-native headers, or launcher UI code unless the owning stage explicitly creates a narrow, documented exception.
- Do not preserve names like `Manager`, `Helper`, `Common`, `Bridge`, or `Utils` as permanent architecture labels unless the stage documents the precise owner, contract, and reason that a more specific production name is not appropriate.
- Do not keep an old body and a new body as parallel production paths. Transitional duplication must have one owner, one removal stage, and one validation plan.
- Do not add abstraction because a future feature might need it. Add it only when current callers, contracts, and validation evidence prove it reduces net complexity.
- Do not create or preserve folders whose names hide ownership, such as broad `Common`, `Utils`, `Helpers`, `Bridge`, or `Managers`, unless the stage proves the owner, contract, validation value, and smaller alternative.
- Do not claim threading readiness by adding broad locks around shared mutable state. Fix ownership and handoff shape first.
- Do not add a job system, render thread, async queue, or background worker abstraction for hypothetical use without current callers, diagnostics, and validation value.

Controlled data-transfer rule:

- Cross-system data must move through named contracts: public C++ DTOs, cooked artifact schemas, shader package manifests/reflection, renderer snapshots, RHI public descriptors/handles, CMake target usage requirements, CLI/process request structs, logs/reports, or validation artifacts.
- If a handoff does not have a named contract, create or document the contract before moving code.
- If the handoff crosses runtime/tool boundaries, name the producer, schema owner, consumer, inspection command, and failure diagnostics.
- If a handoff may later become threaded, name the frame/job/package/artifact id, generation, queue/fence/order rule, and report path now.

Folder architecture rule:

- Follow [repository-target-folder-architecture.md](../architecture/after/repository-target-folder-architecture.md) when adding, moving, renaming, or deleting folders.
- A stage prompt must identify which folders are migration sources, which folders are target owners, which old folders are deleted or aliased temporarily, and which folders must not receive code.
- Owner-specific folders are preferred over generic roots: `Engine/Contracts/Asset`, `Engine/Contracts/Render`, `Engine/Contracts/Shader`, `Tools/Contracts`, `Engine/Renderer/Shaders`, `Engine/RHI/Shaders`, `Projects/*/Data`, `Projects/*/Shaders`, `Tools/Import/SourceImporters`, focused `*Cooker` folders, `Tools/Inspection/AssetInspector`, `Tools/Support/ToolConsoleSupport`, and `Tools/Cooking/CookDiagnostics`.

Threading readiness rule:

- Follow [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md) for every changed edge.
- A stage must leave future worker/render/tool jobs able to consume owned data without reading producer-private mutable state.
- Preferred handoff shapes are immutable snapshots, DTOs, manifests, command batches, queue packets, tool job requests, process requests, and reports.
- Existing single-thread behavior may remain, but the data shape must already name owner, phase, generation/job/frame identity, ordering, and diagnostics.

## Global Refactor Stage Impact Matrix

This plan is a global repository refactor. The early stages still focus on RHI/Renderer, but none of them are allowed to damage GameFramework, tools, launcher workflows, CMake/CI, sample content, or docs.

| Stage | Local focus | Adjacent modules to protect | Required whole-repo check before acceptance |
| --- | --- | --- | --- |
| 1 | Rendering coverage baseline | `repository-coverage-status.md`, GameFramework, tools, CMake, CI, Projects. | Confirm the rendering coverage table delegates non-rendering roots to whole-repo coverage instead of leaving them unowned. |
| 2 | Rendering glossary/system docs | Whole-repo vocabulary, GameFramework/tooling contracts. | New rendering terms must not contradict repository-level ownership names. |
| 3 | RHI/Renderer boundary guardrails | Application validation, launcher smoke, backend folders, generated/third-party policy. | Boundary check exceptions must be counted, stage-labeled, and cross-referenced with repository-wide guardrail expansion in Stage 28. |
| 4 | Renderer shader registration ownership | `ShaderCompiler`, renderer shader registrations, RHI shader primitives, editor shader lists. | ShaderCompiler must enumerate the same packages through a narrow renderer registration target, not full renderer runtime. |
| 5 | First validation milestone | Build graph, shader compiler, launcher target names, boundary script. | Validate smallest tool targets affected by shader registration and CMake wiring; no editor build required unless configure/build wiring changed. |
| 6 | RHI method ownership table | Renderer callers, GameFramework RHI-facing contracts, tools that use RHI public types. | Each RHI method classification must name caller categories and note tool/runtime consumers. |
| 7 | RHI service extraction | Renderer frame graph, upscaling, Application validation, texture/runtime loaders. | Service extraction must preserve public contracts used by tools and GameFramework, or update the paired contracts in the same stage. |
| 8 | Capture/readback validation ownership | Application validation, launcher smoke, backend diagnostics, artifact paths. | Application may orchestrate capture, but backend-native capture/readback implementation must be owned by RHI/backend services. |
| 9 | Native interop and vendor provider boundary | Streamline/DLSS, future FSR-style providers, RHI backends, Renderer upscaling. | Provider code may request native metadata; ordinary renderer passes and GameFramework must not see backend-native handles. |
| 10 | Backend parity milestone | Launcher smoke workflows, Projects/Showcase, validation artifacts, docs. | D3D12/Vulkan evidence must use the same scene/camera path and record artifact paths for later Stage 34 evidence and Stage 35 threading-readiness review. |
| 11 | Renderer facade decomposition | Application, Editor viewport, GameFramework snapshots, launcher smoke. | Public renderer host protocol must stay stable or update all host callers and docs together. |
| 12 | Presentation/viewport bridge | Editor panels, Application hosts, RHI present resources, launcher smoke. | Editor/Application must receive presentation products through the bridge, not frame graph internals or backend resources. |
| 13 | Scene and resource ownership | GameFramework scene/assets, SourceImporters/current SourceImportAdapters, cookers, renderer texture/mesh managers. | Renderer scene DTO changes require GameFramework snapshot/schema impact notes and affected cooker checks. |
| 14 | Frame graph contract | Renderer passes, RHI resources/barriers, diagnostics, smoke validation. | Frame graph warnings must stay diagnosable from launcher/Application smoke output. |
| 15 | Frame graph validation milestone | D3D12/Vulkan backends, Projects/Showcase, docs. | Validation artifacts must include unresolved-resource/barrier status and sample scene path. |
| 16 | Shader package, binding, PSO runtime | ShaderCompiler, RHI shader primitives, renderer registrations, CMake. | PSO/shader package changes require shader inspection/list/cook evidence or a documented unavailable-tool note. |
| 17 | Pass authoring model | ShaderCompiler, Editor shader lists, FrameGraph, RHI shader runtime. | Ordinary pass authoring must remain above RHI and must not add central runtime duplication without a migration note. |
| 17A | Shader registration boilerplate removal | ShaderCompiler, renderer shader registrations, pass definitions, generated/manifest records. | Removing boilerplate must preserve typed parameter/reflection diagnostics and must not hide registration failures behind opaque macros. |
| 17B | Pass authoring friction budget | ShaderCompiler, pass authoring tools, renderer frame insertion, pass definitions, generated/manifest records, validation workflow. | Adding an ordinary pass must be measured from missing shader to running frame; mechanical code must be generated or centralized only when diagnostics and navigation improve. |
| 18 | Ray tracing ownership | GameFramework scene snapshots, Renderer passes, RHI RT descs, backend AS builders. | RT changes must not move shadow/pass data into RHI or GameFramework as a shortcut. |
| 19 | Backend service cleanup | D3D12/Vulkan parity, RHI public contracts, tools using RHI headers. | Backend-private folders must not include each other; public RHI changes must update tool/runtime impact notes. |
| 20 | Full graphics validation | ShaderCompiler, Launcher, Projects/Showcase, Application validation, docs. | Final graphics evidence must be reusable by Stage 34 evidence gate and Stage 35 threading-readiness audit. |
| 23 | Whole-repository coverage and dependency map | All durable roots. | Every root has owner, allowed dependencies, forbidden dependencies, validation target, and acceptance evidence. |
| 24 | GameFramework runtime/cooked contract | Renderer snapshots, SourceImporters/current SourceImportAdapters, cookers, AssetCooker. | GameFramework has no Renderer-private, backend-private, or tool-private dependencies; schema changes name paired producers/consumers. |
| 25 | Source import/cooking/conversion architecture | SourceImporters/current SourceImportAdapters, TextureCooker, MeshCooker, MaterialCooker, SceneCooker, AssetCooker, CookCommon, AssetConverter, GameFramework loaders. | SourceImportAdapters rename/extract toward SourceImporters; CookCommon becomes ToolConsoleSupport/CookDiagnostics; AssetConverter is removed as a production path; runtime consumes cooked outputs only. |
| 26 | Launcher and host boundaries | LauncherCore, Qt GUI, Application, Editor, tools, smoke validation. | Launcher invokes tools/processes and records evidence; Qt UI remains presentation/model code. |
| 27 | Shader and cook artifact validation matrix | ShaderCompiler, cookers, GameFramework loaders, Renderer resource managers, Projects. | Every artifact type has producer, schema owner, consumer, inspector, and smoke/load evidence. |
| 28 | Build, CI, and guardrail expansion | CMake, `.github`, runtime-to-tools dependencies, generated/local-only folders. | Boundary checks cover RHI/Renderer plus runtime-to-tools, GameFramework/launcher/tool ownership, and generated folder policy. |
| 34 | Whole-repository evidence gate | All modules, tools, samples, docs, CI/local validation. | No unowned `Needs refactor` rows remain; evidence proves code, docs, checks, and validation agree before the Stage 35 threading-readiness audit. |

## Acceptance Criteria Traceability

This section verifies that the execution plan touches every criterion from `docs/plans/architecture-review-acceptance-rubric.md`, every review-ready goal from `docs/plans/rhi-renderer-architecture-review.md`, and the global constraints in `docs/plans/sparkle-whole-repository-architecture-review.md`.

### Rubric Criteria Coverage

| Rubric criterion | Covered by stages | Required evidence before final acceptance |
| --- | --- | --- |
| Problem framing | 1, 2, 23, 29, 30, 36 | Coverage status map names the subsystem, current risk, intended owner, linked stage, and threading-readiness risk. Final rubric scoring cites the exact issue each completed stage solved. |
| Requirements and constraints | 1, 2, 5, 10, 15, 20, 23-36 | Architecture docs and milestone reports state D3D12/Vulkan, ray tracing, DLSS, frame graph, debug view, shader cooking, content cooking, GameFramework, launcher, platform, validation, and future-threading constraints. |
| Separation of concerns | 3, 4, 7, 8, 9, 11, 12, 19, 24, 25, 26, 28, 29, 30, 36 | Boundary checks pass; runtime modules do not depend on tool internals; RHI has no Renderer-private includes; Renderer has no backend-private includes outside documented provider integration; Application has no backend-native capture implementation; future workers need no private mutable owner access. |
| Source/folder architecture | 1, 3, 4, 7, 11, 16, 19, 23, 24, 25, 26, 28, 29, 30 | Target folder architecture is updated; new/moved code lands in owner folders; shared schemas use contract roots; backend folders stay sibling/private; tool role folders are explicit; generated/local roots stay out of durable architecture; threading folders are not added prematurely. |
| Threading readiness and data isolation | 1-36 | The threading-readiness contract is opened for each stage; changed edges name mutable owner, phase, handoff shape, isolation scope, ordering/synchronization expectation, and deterministic diagnostics. |
| Cohesion and interface size | 6, 7, 11, 12, 16, 19, 24-30, 36 | RHI method ownership table is complete; root facades shrink behind named services; `Renderer` becomes a host facade; LauncherCore, cookers, and GameFramework have focused owners. |
| Tradeoff reasoning | 2, 6, 7, 9, 16, 17, 23-36 | Design docs record alternatives for shader registration ownership, RHI service extraction, interop, pass definition, PSO keying, backend parity, cooked schema ownership, tool orchestration, launcher workflow ownership, and future-threading handoff choices. |
| Quality attributes | 1, 2, 10, 15, 20, 22, 23, 31, 34, 35, 36 | Each strategic stage updates quality impact for maintainability, reliability, portability, performance reasoning, operability, reviewability, and threading-readiness. |
| Risk and technical debt visibility | 1, 3, 5, 10, 15, 20, 23, 33, 34, 35, 36 | Coverage status tracks `Accepted`, `Needs refactor`, and `Needs design decision`; final gates have no unowned `Needs refactor` rows or unresolved private mutable handoff risks across rendering and repository coverage. |
| Runtime behavior clarity | 2, 11, 12, 14, 16, 17, 18, 20, 25, 26, 31, 34, 35 | Docs include frame execution, frame graph, pass definition, shader package, PSO, ray tracing, presentation, GameFramework snapshots, cooked artifact flow, backend flow diagrams, and future execution-lane handoffs. |
| Observability and diagnostics | 7, 8, 10, 14, 15, 16, 18, 20, 26-31, 34, 35 | Smoke reports include frame graph diagnostics, capability reports, debug names/markers, PSO keys, shader package IDs, DLSS status, RT status, cook/tool failures, launcher evidence, capture artifacts, and job/frame/pass/package/artifact identity. |
| Reliability/failure handling | 7, 8, 9, 14, 18, 20, 26-31, 34, 35 | Missing DLSS/RT/extensions/capture/tool/schema support produces deterministic reasons; unresolved frame graph resources fail development smoke; future tool jobs and queue work have deterministic failure reports. |
| Performance reasoning | 2, 16, 19, 20, 28, 29, 31, 35, 36 | PSO/runtime and cook/tool changes include measurement plan or logs; async compute/transfer remains a measured future step; final scoring avoids unsupported performance claims. |
| Portability/backend parity | 3, 5, 7, 8, 9, 10, 18, 19, 20, 27-29, 31, 34, 35 | D3D12/Vulkan service responsibilities are symmetric where appropriate; source/cooked formats stay API-neutral where possible; known differences are documented; queue/fence ownership is explicit. |
| Maintainability and naming | 2, 11, 12, 13, 14, 17, 18, 19, 23-36 | Folder ownership docs, naming conventions, pass/frame split, ray tracing terms, backend services, launcher/cooker names, artifact ownership, and threading handoff terms are updated to match final code. |
| Testability | 3, 5, 8, 10, 15, 20, 25, 26, 27, 28, 29, 30, 36 | Boundary checks, build targets, shader compiler validation, cook validation, launcher workflow checks, smoke validation, captures, final command list, docs link scan, and threading-readiness audit are repeatable. |
| Communication/reviewability | 1, 2, 23, 29, 30, 36 | Docs, diagrams, validation artifacts, repository coverage, threading-readiness evidence, and final rubric score make the whole repo inspectable without adding presentation work to the active implementation track. |

Critical rubric categories are covered by multiple stages:

- Requirements and constraints: 1, 2, 5, 10, 15, 20, 23-36.
- Separation of concerns: 3, 4, 7, 8, 9, 11, 12, 19, 24, 25, 26, 28, 29, 30, 36.
- Source/folder architecture: 1, 3, 4, 7, 11, 16, 19, 23, 24, 25, 26, 28, 29, 30.
- Threading readiness and data isolation: 1-36.
- Tradeoff reasoning: 2, 6, 7, 9, 16, 17, 23-36.
- Runtime behavior clarity: 2, 11, 12, 14, 16, 17, 18, 20, 25, 26, 31, 34, 35.
- Observability and diagnostics: 7, 8, 10, 14, 15, 16, 18, 20, 26-31, 34, 35.
- Portability/backend parity: 3, 5, 7, 8, 9, 10, 18, 19, 20, 27-29, 31, 34, 35.
- Testability: 3, 5, 8, 10, 15, 20, 25, 26, 27, 28, 29, 30, 36.

### Engineering Skill Signal Coverage

| Engineering signal | Covered by stages | Required final evidence |
| --- | --- | --- |
| Role relevance | 20, 23-36 | Implementation evidence shows C++20 renderer/RHI scope, D3D12/Vulkan, frame graph, ray tracing, shader tooling, upscaling, runtime, tools, launcher, and cook pipeline architecture. |
| Modern C++ systems skill | 6, 7, 11, 13, 16, 19, 24-30, 36 | Ownership docs, service extraction, RAII/lifetime contracts, allocator/resource lifetime notes, and clean build commands. |
| Graphics API fluency | 7, 8, 10, 18, 19, 20 | D3D12/Vulkan resource state/layout, descriptors, PSO, swap chain, ray tracing, and capture parity evidence. |
| Shader and pipeline systems | 4, 5, 16, 17, 20 | Renderer-owned shader registration, explicit PSO keys, pass definition model, shader compiler validation. |
| Content pipeline architecture | 23, 25-32, 34, 35, 36 | Source import, focused cooking, cooked schemas, runtime loading, and renderer resource creation are separate, deterministic, and validated. |
| Developer tooling/product workflow | 23, 25, 26, 28, 29, 30 | LauncherCore, Qt launcher UI, AssetCooker, focused tools, process evidence, and recovery paths are cohesive and repeatable. |
| Rendering fundamentals | 10, 14, 18, 20 | Lit, normal/debug, GBuffer, lighting, shadows, temporal/upscaling captures and notes. |
| GPU architecture and performance reasoning | 16, 19, 20, 36 | Timing/diagnostic output, PSO/runtime logs, memory diagnostics, and no unsupported performance claims. |
| Cross-backend architecture | 3, 7, 8, 9, 19, 20 | Mechanical boundary checks and D3D12/Vulkan parity report. |
| Source and folder architecture | 1, 4, 7, 11, 16, 23, 24, 25, 26, 28, 29, 30 | Target folder architecture, source-root inventory, contract roots, backend sibling folders, owner-specific shader/data roots, focused tool folders, generated/local-only policy, and no premature threading folders. |
| Multithreading readiness | 1-36 | Repository threading-readiness contract, immutable render snapshots, frame graph phase split, command-batch/queue-packet model, deterministic cook/shader jobs, LauncherCore requests/reports, and no private mutable cross-module worker edges. |
| Debuggability and validation | 3, 8, 10, 14, 15, 20 | Smoke reports, logs, capture artifacts, validation failure policy. |
| Reliability and fallback behavior | 7, 8, 9, 18, 20 | Deterministic feature fallback reasons for DLSS, RT, capture, and backend capabilities. |
| Testability and CI thinking | 3, 5, 10, 15, 20, 27, 28, 29, 30, 36 | Local/CI-ready commands for boundary checks, formatting, shader compiler, cook tools, launcher, build, smoke, docs link scan, and threading-readiness audit. |
| Documentation and onboarding | 2, 23, 29, 30, 36 | Architecture docs, whole-repo review, contract maps, implementation evidence, and threading-readiness map. |
| Communication and design rationale | 1, 2, 6, 23, 29, 30, 36 | Decision notes, alternatives, risks, non-goals, final rubric score, whole-repository architecture narrative, and future-threading tradeoffs. |
| Git/review hygiene | 33, 36 | No generated junk in source docs, local/CI checks are explicit, and stale docs are deleted. |
| Product/demo clarity | 10, 20, 31, 32, 34, 36 | Showcase launch/cook/load path, captures, validation artifacts, current backend status, and content-pipeline status. |
| Collaboration readiness | 28, 29, 30, 33, 36 | Build/report/validation instructions, CI/local check coverage, and threading-readiness review prompts. |

### Architecture Review Goal Coverage

| Architecture review goal | Covered by stages | Required final evidence |
| --- | --- | --- |
| Module dependency direction is mechanically checked | 3, 5, 20, 22 | Boundary check passes with no stale exceptions. |
| RHI method ownership is documented | 6, 7, 19, 22 | RHI contract map covers every public method and final service owner. |
| D3D12 and Vulkan backend folders remain symmetric and backend-private | 3, 19, 20, 22 | Backend services are separate, symmetric where appropriate, and parity-tested. |
| Renderer pass orchestration has a documented convention | 2, 14, 17, 22 | `Frame/*` composition and `Passes/*` execution split is documented and reflected in code. |
| Ray tracing ownership is explained from scene data to TLAS binding | 2, 18, 20, 22 | Ray tracing contract and smoke evidence cover BLAS/TLAS/pass usage. |
| DLSS/native interop has a documented backend contract | 7, 9, 10, 20, 22 | Upscaler/native interop contract and per-backend DLSS status logs. |
| D3D12/Vulkan smoke validation passes with no unresolved frame graph warnings | 10, 15, 20, 22 | Smoke logs and captures show no unresolved resource/barrier warnings. |
| Visual debug modes are validated for both APIs | 10, 20, 22 | D3D12 and Vulkan debug/normal captures exist and are linked from final evidence. |
| Whole-repository ownership is documented | 23, 29, 30 | Every durable root has owner, allowed dependencies, forbidden dependencies, validation target, acceptance evidence, and threading-ready handoff shape. |
| GameFramework runtime/cooked boundary is protected | 25, 26, 31, 34, 35, 36 | GameFramework has no renderer-private, backend-private, or tool-private dependencies and cooked schemas name producers/consumers. |
| Tooling/content pipeline ownership is protected | 27-31, 33, 34, 35, 36 | Source import, focused cooking, AssetCooker orchestration, shader compilation, and conversion/debug CLI roles are separate and job-shaped. |
| Launcher and host boundaries are protected | 26, 28, 29, 30 | LauncherCore owns process/evidence workflows; Qt GUI owns presentation; Application/Editor do not own cook/import/backend-native internals. |
| Build/CI guardrails cover the global architecture | 28, 29, 30 | Local/CI-friendly checks catch runtime-to-tools, private include, launcher/tool, generated-folder, RHI/Renderer boundary drift, and threading-hostile handoffs. |

Traceability conclusion:

- The plan covers all rubric criteria, all critical categories, all engineering skill signals, the RHI/Renderer definition-of-done goals, and the whole-repository goals from `sparkle-whole-repository-architecture-review.md`.
- The criteria that rely primarily on late-stage work are repo presentation, Git/review hygiene, collaboration readiness, global CI/local guardrails, cooked artifact validation, whole-repository evidence scoring, and threading-readiness auditing. Stages 21 through 30 are therefore mandatory, not optional polish.

## Reference Basis By Stage

This plan should not drift into invented architecture. Each stage has an external reference basis from existing graphics repositories, SDKs, API documentation, or established architecture patterns.

The references are inspiration and calibration points, not copy-paste targets. Sparkle should borrow the ownership model, vocabulary, and validation habits that fit the engine, while preserving its own module boundaries and current working behavior.

Reference index:

- NVIDIA Donut: https://github.com/NVIDIA-RTX/Donut
- NVIDIA Donut Samples: https://github.com/NVIDIA-RTX/Donut-Samples
- NVIDIA NVRHI: https://github.com/NVIDIA-RTX/NVRHI
- NVIDIA NVRHI tutorial: https://github.com/NVIDIA-RTX/NVRHI/blob/main/doc/Tutorial.md
- NVIDIA NVRHI technical blog: https://developer.nvidia.com/blog/writing-portable-rendering-code-with-nvrhi/
- NVIDIA NRI: https://github.com/NVIDIA-RTX/NRI
- NVIDIA Falcor: https://github.com/NVIDIAGameWorks/Falcor
- NVIDIA Falcor getting started docs: https://github.com/NVIDIAGameWorks/Falcor/blob/master/docs/getting-started.md
- NVIDIA Streamline: https://github.com/NVIDIA-RTX/Streamline
- NVIDIA Streamline programming guide: https://github.com/NVIDIA-RTX/Streamline/blob/main/docs/ProgrammingGuide.md
- NVIDIA Streamline manual hooking guide: https://github.com/NVIDIA-RTX/Streamline/blob/main/docs/ProgrammingGuideManualHooking.md
- AMD Cauldron: https://github.com/GPUOpen-LibrariesAndSDKs/Cauldron
- AMD FidelityFX SDK: https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK
- AMD FidelityFX SDK overview: https://gpuopen.com/amd-fidelityfx-sdk-1/
- AMD Compressonator: https://github.com/GPUOpen-Tools/compressonator
- AMD Compressonator GPUOpen page: https://gpuopen.com/compressonator/
- Diligent Engine: https://github.com/DiligentGraphics/DiligentEngine
- Diligent Core PSO model: https://github.com/DiligentGraphics/DiligentCore
- Microsoft D3D12 PSO docs: https://learn.microsoft.com/en-us/windows/win32/direct3d12/pipelines-and-shaders-with-directx-12
- Microsoft D3D12 pipeline state management docs: https://learn.microsoft.com/en-us/windows/win32/direct3d12/managing-graphics-pipeline-state-in-direct3d-12
- Microsoft DXR spec: https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html
- NVIDIA DXR tutorial: https://developer.nvidia.com/rtx/raytracing/dxr/dx12-raytracing-tutorial-part-1
- Khronos Vulkan validation layers: https://github.com/KhronosGroup/Vulkan-ValidationLayers
- Vulkan development tools guide: https://docs.vulkan.org/guide/latest/development_tools.html
- Khronos Vulkan ray tracing sample: https://github.com/KhronosGroup/Vulkan-Samples/blob/main/samples/extensions/ray_tracing_basic/README.adoc
- NVIDIA Vulkan ray tracing tutorial: https://nvpro-samples.github.io/vk_raytracing_tutorial_KHR/
- arc42 overview: https://arc42.org/overview
- CMU SEI ATAM: https://www.sei.cmu.edu/library/architecture-tradeoff-analysis-method-collection/
- ISO/IEC 25010: https://www.iso.org/standard/35733.html
- ADR practice: https://adr.github.io/
- Michael Nygard ADR article: https://www.cognitect.com/blog/2011/11/15/documenting-architecture-decisions
- Composition Root pattern: https://blog.ploeh.dk/2011/07/28/CompositionRoot/
- Facade pattern: https://refactoring.guru/design-patterns/facade
- Command pattern in game code: https://gameprogrammingpatterns.com/command.html
- CMake target usage requirements: https://cmake.org/cmake/help/latest/command/target_link_libraries.html
- Qt model/view programming: https://doc.qt.io/qt-6/model-view-programming.html
- Khronos glTF 2.0 specification: https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html
- Khronos KTX-Software: https://github.com/KhronosGroup/KTX-Software

Stage-to-reference map:

| Stage | Reference basis | Pattern or solution to follow | What Sparkle should borrow | What Sparkle should not copy blindly |
| --- | --- | --- | --- | --- |
| 1 | arc42, CMU SEI ATAM, ISO/IEC 25010, ADR practice | Quality-attribute review and decision tracking | Coverage status, quality risks, decision questions, traceable acceptance evidence | Heavy enterprise ceremony that slows small implementation steps |
| 2 | arc42, Donut, Falcor docs | Reviewer-facing architecture docs and glossary | Clear module map, render pipeline vocabulary, diagrams, and navigation paths | Falcor/Donut terminology where Sparkle already has better local names |
| 3 | NVRHI, NRI, Cauldron, Diligent Engine | Layered RHI plus backend-private implementation trees | Mechanical checks that protect RHI/Renderer/backend direction | A generic RHI shape that ignores Sparkle's existing modules |
| 4 | Donut, Falcor RenderPasses, NVRHI | Renderer passes live above hardware abstraction | Renderer-owned shader registration and pass metadata above RHI infrastructure | Putting every shader concept into Renderer; generic shader package/runtime primitives still belong lower |
| 5 | NVRHI tutorial, Donut Samples, Vulkan validation layers | Early validation milestone after boundary and shader moves | Build the shader compiler and run boundary checks before larger refactors | Treating a successful compile as visual/backend parity |
| 6 | NVRHI, NRI, Diligent Engine | Focused graphics abstraction contracts | RHI method ownership table and service categories | Splitting interfaces before caller evidence proves the boundary |
| 7 | NVRHI technical blog, Diligent Engine, Streamline docs | Explicit services for resources, descriptors, barriers, diagnostics, interop | Service-owned RHI responsibilities and deterministic native metadata | A catch-all interop service that becomes another service locator |
| 8 | Vulkan validation layers, Vulkan development tools guide, Diligent Engine | Validation/capture through graphics-layer services | Application owns test orchestration; RHI/backend owns native capture/readback details | D3D12-only capture path hidden behind a generic name |
| 9 | Streamline, Streamline sample/manual hooking docs, FidelityFX SDK | Vendor feature provider layer with backend capability support | Provider-owned SDK code, backend-owned API setup, deterministic fallback reasons | Vendor SDK details in common renderer pass code or root RHI policy |
| 10 | Donut Samples, NVRHI tutorial, Vulkan validation layers | Milestone validation with artifacts | Backend-specific smoke evidence, logs, captures, and feature status | "It launched" as an acceptance substitute |
| 11 | Donut, Falcor, Composition Root, Facade pattern | Thin public facade plus explicit composition root | `Renderer` as host facade; subsystem wiring in one owned root | Moving the god object to a differently named class |
| 12 | Donut app/render split, NVRHI abstraction, Facade pattern | Host protocol hides presentation/resource details | Viewport/presentation bridge that hides frame graph and resource-state details from Application | Application/editor manually driving frame graph transitions |
| 13 | Donut scene/component graph, Falcor scene/render separation | Render-domain snapshots between scene and renderer | Immutable render DTOs for meshes, materials, lights, camera, skinning, temporal data | Renderer directly depending on gameplay internals for convenience |
| 14 | Falcor RenderGraph, Donut reusable passes, Vulkan validation tools | Graph contract plus diagnostics | Development failures for unresolved resources/barriers and diagnostic dumps | Suppressing warnings to keep smoke green |
| 15 | NVRHI tutorial, Donut Samples, Vulkan validation layers | Milestone validation after renderer/frame graph restructuring | D3D12/Vulkan smoke with lit/debug captures and frame graph diagnostics | Moving to PSO redesign before graph warnings are clean |
| 16 | D3D12 PSO docs, D3D12 pipeline state management, Diligent Core PSO model | Explicit immutable PSO descriptors and cache keys | Printable PSO key, separated package loading, binding layout, validation, and PSO creation | `std::type_index` as final runtime identity |
| 17 | Falcor RenderPasses, Donut reusable passes, Diligent render state notation ideas | Declarative pass definition | One renderer-owned pass definition should drive graph intent and pipeline runtime lookup | Backend-specific pass definitions or duplicated central traits |
| 17A | Falcor render-pass tooling, Donut shader build separation, AMD RPS graph authoring, Diligent render-state packager | Manifest/generated shader registrations | One shader/package declaration feeds ShaderCompiler and pass runtime identity | Boilerplate macros that hide errors or duplicate package metadata |
| 17B | Falcor render-pass scaffolding, Donut reusable pass organization, AMD RPS graph authoring, Diligent render-state notation/packager | Measured one-file or one-command pass authoring workflow | A simple compute/raster pass has an explicit touch-count budget and generated mechanical pieces | Moving boilerplate into another hand-written registry or hiding it in opaque macros |
| 18 | Microsoft DXR spec, NVIDIA DXR tutorial, Khronos Vulkan ray tracing sample, NVIDIA Vulkan ray tracing tutorial, Donut Samples RT | BLAS/TLAS ownership and API-neutral ray tracing contracts | Clear split between renderer AS scene ownership and RHI AS build descriptors/commands | Shadow/pass concepts in RHI ray tracing structs |
| 19 | NVRHI, NRI, Cauldron, Diligent Engine | Symmetric backend services under a common abstraction | D3D12/Vulkan service symmetry for commands, descriptors, memory, pipeline, resources, diagnostics | Merging API-specific details into common code too early |
| 20 | Donut Samples, NVRHI tutorial, Vulkan validation layers, Streamline/FidelityFX docs | Full backend parity validation | Lit/debug captures, DLSS/RT/frame graph/PSO logs, backend feature reports | Exact image match claims where numeric/API differences require tolerance |
| 23 | Whole-repository architecture review, CMake target usage requirements, Donut/Falcor/Cauldron repo layout | Repository-wide ownership map | Every durable root has owner, dependency intent, validation target, and acceptance evidence | Treating non-rendering modules as "out of scope" during a graphics refactor |
| 24 | Donut scene/component graph, Falcor scene/render split, glTF runtime asset delivery | Runtime scene and cooked asset contract | GameFramework owns runtime/cooked loading and emits immutable renderer-facing snapshots | Moving source import, cook algorithms, or renderer pass data into GameFramework |
| 25 | AMD Compressonator, Cauldron content/sample pipeline, glTF, KTX | Focused import/cook tools plus orchestration | SourceImporters and focused cookers own transformations; AssetCooker orchestrates and reports; ToolConsoleSupport/CookDiagnostics replace vague CookCommon; AssetConverter is not a production path | AssetCooker becoming a second implementation of every cooker, or AssetConverter surviving as a parallel cook pipeline |
| 26 | Qt model/view programming, Compressonator GUI/CLI/SDK split, Streamline integration guides | Workflow core separated from UI and host orchestration | LauncherCore owns operations/processes/evidence; Qt GUI owns presentation; Application/Editor host systems | Widgets or host code duplicating cook/render/backend implementation details |
| 27 | NVRHI shader packages/validation, Falcor shader/render tooling, KTX/glTF artifact contracts | Artifact producer/schema/consumer validation matrix | Every shader/cooked asset type has producer, owner, consumer, inspector, and smoke/load evidence | Schema changes accepted with only a build or source-level compile |
| 28 | CMake target usage requirements, NVRHI/NRI/Cauldron backend boundaries, CI workflows in reference repos | Mechanical guardrails beyond RHI/Renderer | Checks cover runtime-to-tools, GameFramework/private coupling, launcher/tool ownership, generated folders | Broad allowlists that hide architecture drift |
| 29 | Whole-repository review, architecture rubric, arc42/ATAM/ADR | Global evidence gate | Code, docs, CMake, CI/local checks, sample content, tools, and validation evidence agree | Declaring review-ready while any source root has unowned risk |
| 30 | Donut threaded rendering, Cauldron thread pool/command-list rings, Diligent multithreading/command queues, NVIDIA async compute guidance | Threading-ready ownership without premature threading implementation | Mutable owners, immutable handoffs, command batches, queue packets, tool jobs, launcher process reports, and deterministic diagnostics are documented across all modules | Adding a broad job system or global locks before data ownership is correct |

Reference use rules:

- Before implementing a stage, inspect at least one listed reference and one Sparkle code path named by that stage.
- Add an ADR or design note when Sparkle intentionally diverges from the reference model.
- Prefer references from NVIDIA, AMD, Khronos, Microsoft, or established open-source graphics engines for GPU/API behavior.
- Use general software architecture references only for process and pattern vocabulary, not for GPU contract details.
- If a stage discovers a better reference implementation, add it here before using it as a design basis.

## Required Target Documents By Stage

Stage-local source references name the immediate review/code context. The target documents below are mandatory implementation context: they define the intended final shape, folder placement, contracts, and acceptance evidence for each stage.

| Stage | Required target documents before implementation |
| --- | --- |
| 1 | [rendering-coverage-status.md](../architecture/rendering-coverage-status.md), [repository-coverage-status.md](../architecture/repository-coverage-status.md), [repository-current-state.md](../architecture/before/repository-current-state.md), [repository-target-architecture.md](../architecture/after/repository-target-architecture.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md) |
| 2 | [system-design-index.md](../architecture/after/system-design-index.md), [rendering-glossary.md](../architecture/rendering-glossary.md), [rendering-system-map.md](../architecture/rendering-system-map.md), [rhi-contract-map.md](../architecture/rhi-contract-map.md), [frame-graph-contract.md](../architecture/frame-graph-contract.md), [pass-authoring-contract.md](../architecture/pass-authoring-contract.md), [pipeline-runtime-contract.md](../architecture/pipeline-runtime-contract.md), [ray-tracing-contract.md](../architecture/ray-tracing-contract.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md) |
| 3 | [architecture-boundary-guardrails.md](../architecture/architecture-boundary-guardrails.md), [repository-system-map.md](../architecture/repository-system-map.md), [repository-target-folder-architecture.md](../architecture/after/repository-target-folder-architecture.md), [rhi-contract-map.md](../architecture/rhi-contract-map.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md) |
| 4 | [pass-authoring-contract.md](../architecture/pass-authoring-contract.md), [pipeline-runtime-contract.md](../architecture/pipeline-runtime-contract.md), [tooling-pipeline-contract.md](../architecture/tooling-pipeline-contract.md), [rhi-contract-map.md](../architecture/rhi-contract-map.md), [repository-target-folder-architecture.md](../architecture/after/repository-target-folder-architecture.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md) |
| 5 | [architecture-boundary-guardrails.md](../architecture/architecture-boundary-guardrails.md), [pass-authoring-contract.md](../architecture/pass-authoring-contract.md), [pipeline-runtime-contract.md](../architecture/pipeline-runtime-contract.md), [tooling-pipeline-contract.md](../architecture/tooling-pipeline-contract.md), [rendering-coverage-status.md](../architecture/rendering-coverage-status.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md) |
| 6 | [rhi-contract-map.md](../architecture/rhi-contract-map.md), [repository-target-architecture.md](../architecture/after/repository-target-architecture.md), [repository-target-folder-architecture.md](../architecture/after/repository-target-folder-architecture.md), [repository-system-map.md](../architecture/repository-system-map.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md) |
| 7 | [rhi-contract-map.md](../architecture/rhi-contract-map.md), [rendering-system-map.md](../architecture/rendering-system-map.md), [architecture-boundary-guardrails.md](../architecture/architecture-boundary-guardrails.md), [repository-target-folder-architecture.md](../architecture/after/repository-target-folder-architecture.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md) |
| 8 | [rhi-contract-map.md](../architecture/rhi-contract-map.md), [architecture-boundary-guardrails.md](../architecture/architecture-boundary-guardrails.md), [repository-system-map.md](../architecture/repository-system-map.md), [tooling-pipeline-contract.md](../architecture/tooling-pipeline-contract.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md) |
| 9 | [rendering-system-map.md](../architecture/rendering-system-map.md), [upscaler-provider-contract.md](../architecture/upscaler-provider-contract.md), [rhi-contract-map.md](../architecture/rhi-contract-map.md), [architecture-boundary-guardrails.md](../architecture/architecture-boundary-guardrails.md), [repository-target-architecture.md](../architecture/after/repository-target-architecture.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md) |
| 10 | [rendering-coverage-status.md](../architecture/rendering-coverage-status.md), [rendering-system-map.md](../architecture/rendering-system-map.md), [rhi-contract-map.md](../architecture/rhi-contract-map.md), [ray-tracing-contract.md](../architecture/ray-tracing-contract.md), [tooling-pipeline-contract.md](../architecture/tooling-pipeline-contract.md), [validation-workflow-contract.md](../architecture/validation-workflow-contract.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md) |
| 11 | [rendering-system-map.md](../architecture/rendering-system-map.md), [repository-target-architecture.md](../architecture/after/repository-target-architecture.md), [repository-target-folder-architecture.md](../architecture/after/repository-target-folder-architecture.md), [game-framework-contract.md](../architecture/game-framework-contract.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md) |
| 12 | [rendering-system-map.md](../architecture/rendering-system-map.md), [rhi-contract-map.md](../architecture/rhi-contract-map.md), [game-framework-contract.md](../architecture/game-framework-contract.md), [repository-target-architecture.md](../architecture/after/repository-target-architecture.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md) |
| 13 | [game-framework-contract.md](../architecture/game-framework-contract.md), [rendering-system-map.md](../architecture/rendering-system-map.md), [tooling-pipeline-contract.md](../architecture/tooling-pipeline-contract.md), [repository-target-folder-architecture.md](../architecture/after/repository-target-folder-architecture.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md) |
| 14 | [frame-graph-contract.md](../architecture/frame-graph-contract.md), [rendering-system-map.md](../architecture/rendering-system-map.md), [pass-authoring-contract.md](../architecture/pass-authoring-contract.md), [pipeline-runtime-contract.md](../architecture/pipeline-runtime-contract.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md) |
| 15 | [frame-graph-contract.md](../architecture/frame-graph-contract.md), [rendering-coverage-status.md](../architecture/rendering-coverage-status.md), [architecture-boundary-guardrails.md](../architecture/architecture-boundary-guardrails.md), [validation-workflow-contract.md](../architecture/validation-workflow-contract.md), [repository-target-graphs.md](../architecture/after/repository-target-graphs.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md) |
| 16 | [pipeline-runtime-contract.md](../architecture/pipeline-runtime-contract.md), [pass-authoring-contract.md](../architecture/pass-authoring-contract.md), [rhi-contract-map.md](../architecture/rhi-contract-map.md), [tooling-pipeline-contract.md](../architecture/tooling-pipeline-contract.md), [repository-target-folder-architecture.md](../architecture/after/repository-target-folder-architecture.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md) |
| 17 | [pass-authoring-contract.md](../architecture/pass-authoring-contract.md), [pipeline-runtime-contract.md](../architecture/pipeline-runtime-contract.md), [tooling-pipeline-contract.md](../architecture/tooling-pipeline-contract.md), [repository-target-folder-architecture.md](../architecture/after/repository-target-folder-architecture.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md) |
| 17A | [pass-authoring-contract.md](../architecture/pass-authoring-contract.md), [pipeline-runtime-contract.md](../architecture/pipeline-runtime-contract.md), [tooling-pipeline-contract.md](../architecture/tooling-pipeline-contract.md), [rendering-coverage-status.md](../architecture/rendering-coverage-status.md), [repository-target-folder-architecture.md](../architecture/after/repository-target-folder-architecture.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md) |
| 17B | [pass-authoring-contract.md](../architecture/pass-authoring-contract.md), [pipeline-runtime-contract.md](../architecture/pipeline-runtime-contract.md), [tooling-pipeline-contract.md](../architecture/tooling-pipeline-contract.md), [validation-workflow-contract.md](../architecture/validation-workflow-contract.md), [rendering-coverage-status.md](../architecture/rendering-coverage-status.md), [repository-target-folder-architecture.md](../architecture/after/repository-target-folder-architecture.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md) |
| 18 | [ray-tracing-contract.md](../architecture/ray-tracing-contract.md), [rhi-contract-map.md](../architecture/rhi-contract-map.md), [frame-graph-contract.md](../architecture/frame-graph-contract.md), [pass-authoring-contract.md](../architecture/pass-authoring-contract.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md) |
| 19 | [rhi-contract-map.md](../architecture/rhi-contract-map.md), [architecture-boundary-guardrails.md](../architecture/architecture-boundary-guardrails.md), [repository-target-folder-architecture.md](../architecture/after/repository-target-folder-architecture.md), [rendering-coverage-status.md](../architecture/rendering-coverage-status.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md) |
| 20 | [rendering-coverage-status.md](../architecture/rendering-coverage-status.md), [rendering-system-map.md](../architecture/rendering-system-map.md), [rhi-contract-map.md](../architecture/rhi-contract-map.md), [ray-tracing-contract.md](../architecture/ray-tracing-contract.md), [pipeline-runtime-contract.md](../architecture/pipeline-runtime-contract.md), [tooling-pipeline-contract.md](../architecture/tooling-pipeline-contract.md), [validation-workflow-contract.md](../architecture/validation-workflow-contract.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md) |
| 23 | [repository-system-map.md](../architecture/repository-system-map.md), [repository-coverage-status.md](../architecture/repository-coverage-status.md), [repository-current-state.md](../architecture/before/repository-current-state.md), [repository-target-architecture.md](../architecture/after/repository-target-architecture.md), [repository-target-folder-architecture.md](../architecture/after/repository-target-folder-architecture.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md) |
| 24 | [repository-system-map.md](../architecture/repository-system-map.md), [repository-target-architecture.md](../architecture/after/repository-target-architecture.md), [repository-target-folder-architecture.md](../architecture/after/repository-target-folder-architecture.md), [architecture-boundary-guardrails.md](../architecture/architecture-boundary-guardrails.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md) |
| 25 | [game-framework-contract.md](../architecture/game-framework-contract.md), [render-scene-data-contract.md](../architecture/render-scene-data-contract.md), [tooling-pipeline-contract.md](../architecture/tooling-pipeline-contract.md), [repository-target-architecture.md](../architecture/after/repository-target-architecture.md), [repository-target-folder-architecture.md](../architecture/after/repository-target-folder-architecture.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md) |
| 26 | [game-framework-contract.md](../architecture/game-framework-contract.md), [tooling-pipeline-contract.md](../architecture/tooling-pipeline-contract.md), [repository-target-folder-architecture.md](../architecture/after/repository-target-folder-architecture.md), [repository-system-map.md](../architecture/repository-system-map.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md) |
| 27 | [tooling-pipeline-contract.md](../architecture/tooling-pipeline-contract.md), [repository-target-folder-architecture.md](../architecture/after/repository-target-folder-architecture.md), [game-framework-contract.md](../architecture/game-framework-contract.md), [repository-target-architecture.md](../architecture/after/repository-target-architecture.md), [repository-system-map.md](../architecture/repository-system-map.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md) |
| 28 | [tooling-pipeline-contract.md](../architecture/tooling-pipeline-contract.md), [game-framework-contract.md](../architecture/game-framework-contract.md), [repository-target-folder-architecture.md](../architecture/after/repository-target-folder-architecture.md), [repository-system-map.md](../architecture/repository-system-map.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md) |
| 29 | [tooling-pipeline-contract.md](../architecture/tooling-pipeline-contract.md), [pass-authoring-contract.md](../architecture/pass-authoring-contract.md), [pipeline-runtime-contract.md](../architecture/pipeline-runtime-contract.md), [repository-target-folder-architecture.md](../architecture/after/repository-target-folder-architecture.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md) |
| 30 | [tooling-pipeline-contract.md](../architecture/tooling-pipeline-contract.md), [repository-system-map.md](../architecture/repository-system-map.md), [repository-target-architecture.md](../architecture/after/repository-target-architecture.md), [architecture-boundary-guardrails.md](../architecture/architecture-boundary-guardrails.md), [validation-workflow-contract.md](../architecture/validation-workflow-contract.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md) |
| 31 | [tooling-pipeline-contract.md](../architecture/tooling-pipeline-contract.md), [game-framework-contract.md](../architecture/game-framework-contract.md), [pass-authoring-contract.md](../architecture/pass-authoring-contract.md), [pipeline-runtime-contract.md](../architecture/pipeline-runtime-contract.md), [system-design-index.md](../architecture/after/system-design-index.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md) |
| 32 | [repository-system-map.md](../architecture/repository-system-map.md), [repository-target-folder-architecture.md](../architecture/after/repository-target-folder-architecture.md), [validation-workflow-contract.md](../architecture/validation-workflow-contract.md), [tooling-pipeline-contract.md](../architecture/tooling-pipeline-contract.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md) |
| 33 | [architecture-boundary-guardrails.md](../architecture/architecture-boundary-guardrails.md), [repository-system-map.md](../architecture/repository-system-map.md), [repository-coverage-status.md](../architecture/repository-coverage-status.md), [repository-target-folder-architecture.md](../architecture/after/repository-target-folder-architecture.md), [repository-target-architecture.md](../architecture/after/repository-target-architecture.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md) |
| 34 | [architecture-review-acceptance-rubric.md](architecture-review-acceptance-rubric.md), [repository-coverage-status.md](../architecture/repository-coverage-status.md), [rendering-coverage-status.md](../architecture/rendering-coverage-status.md), [repository-target-architecture.md](../architecture/after/repository-target-architecture.md), [repository-target-graphs.md](../architecture/after/repository-target-graphs.md), [repository-target-folder-architecture.md](../architecture/after/repository-target-folder-architecture.md), [system-design-index.md](../architecture/after/system-design-index.md), [architecture-boundary-guardrails.md](../architecture/architecture-boundary-guardrails.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md) |
| 35 | [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md), [repository-target-architecture.md](../architecture/after/repository-target-architecture.md), [repository-target-graphs.md](../architecture/after/repository-target-graphs.md), [system-design-index.md](../architecture/after/system-design-index.md), [architecture-review-acceptance-rubric.md](architecture-review-acceptance-rubric.md), [repository-refactor-stage-map.md](after/repository-refactor-stage-map.md) |
| 36 | [architecture-review-acceptance-rubric.md](architecture-review-acceptance-rubric.md), [repository-coverage-status.md](../architecture/repository-coverage-status.md), [rendering-coverage-status.md](../architecture/rendering-coverage-status.md), [system-design-index.md](../architecture/after/system-design-index.md), [repository-refactor-stage-map.md](after/repository-refactor-stage-map.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md) |

Implementation rule:

- If a stage changes a subsystem that has a row in [system-design-index.md](../architecture/after/system-design-index.md), open that subsystem row too, even if it is not listed explicitly above.
- If a target document contradicts the local stage prompt, update the prompt or the target document before editing code. Do not silently choose one.
- If implementation discovers that the target shape is wrong, update the relevant after/target doc, stage status, and acceptance evidence before continuing.

## Stage Contract Coverage Matrix

This matrix makes the contract burden explicit for each stage. Stage-local prompts still provide the exact implementation task; this table defines what the stage must prove before it can be accepted.

| Stage | Contract surfaces to prove | Stage-local acceptance proof required |
| --- | --- | --- |
| 1 | Coverage, folder architecture, threading readiness | Rendering coverage delegates non-rendering roots to whole-repo coverage; current risks name owners, stages, validation evidence, and threading-sensitive handoffs. |
| 2 | Architecture navigation, glossary, contract surfaces | Reviewer docs use one vocabulary for RHI, Renderer, GameFramework, tooling, artifacts, and threading data shapes; graphs and contracts link to the same target model. |
| 3 | Mechanical guardrails, RhiContracts, folder policy, threading-hostile edge prevention | Boundary check reports actionable paths/reasons, counts transitional exceptions, and blocks new private edges that would force future workers through owner-private state. |
| 4 | ShaderContracts, pass authoring, RhiContracts, ToolContracts, threading-ready package catalogs | Renderer pass shader metadata lives above RHI, package ids stay stable, ShaderCompiler sees the same packages, and pass/package data can be consumed as immutable manifests. |
| 5 | Validation evidence, ShaderContracts, guardrails | Boundary and shader-registration validation proves Stage 4 behavior without an editor build; package enumeration/build evidence is recorded with remaining exception counts. |
| 6 | RhiContracts, folder architecture, complexity budget, threading owner map | Each RHI method is classified by service owner, caller category, backend impact, future command/queue ownership, and reason to remain on the public facade if retained. |
| 7 | RHI services, diagnostics, interop, capture, presentation, ThreadingReadiness | First RHI services have caller evidence, public contracts, per-frame/queue/resource ownership notes, diagnostics, and no new service-locator or private backend shortcuts. |
| 8 | Application host boundary, RHI capture/readback services, ToolContracts | Application orchestrates smoke/capture only; backend-native capture/readback implementation moves behind RHI/backend services and the Stage 8 exception is removed. |
| 9 | Provider boundary, native interop, RhiContracts, Renderer feature ownership | Vendor/native metadata flows through provider-owned and RHI/backend-owned contracts; ordinary renderer passes and GameFramework never see D3D12/Vulkan native handles. |
| 10 | Backend parity, validation artifacts, launcher smoke, threading evidence reuse | D3D12/Vulkan smoke evidence uses comparable scene/camera/artifact paths and records logs/captures usable by Stage 34 and Stage 35. |
| 11 | Renderer facade, composition root, RenderContracts, ThreadingReadiness | Public `Renderer` becomes host protocol; frame pipeline/system root owns detailed orchestration; future render-thread work can start from owned snapshots/frame data. |
| 12 | Presentation bridge, host protocol, RhiContracts | Application/Editor receive presentation products through a bridge; hosts do not drive frame graph internals, backend resources, or manual state transitions. |
| 13 | RenderContracts, AssetContracts, renderer scene/resource ownership, ToolContracts | Renderer consumes immutable render-domain DTOs/snapshots; mesh/material/texture/temporal managers name cooked/runtime producers and do not read gameplay/tool internals. |
| 14 | Frame graph contract, pass authoring, pipeline runtime, RHI resource access | Frame graph setup, compile, and execute phases are distinct; resource/barrier diagnostics are actionable; future command batches can be derived from frozen graph data. |
| 15 | Frame graph validation, backend smoke, diagnostics | D3D12/Vulkan smoke-visible graph diagnostics prove unresolved resources/barriers are absent or fail development validation with actionable evidence. |
| 16 | PipelineRuntime, ShaderContracts, RhiContracts, ToolContracts | PSO keys are explicit, printable, deterministic, backend-normalized, and tied to package/reflection/layout identity rather than type-index or pass-private state. |
| 17 | PassCatalog/pass definition, ShaderContracts, frame graph, pipeline runtime | Ordinary pass authoring becomes declarative enough that new passes avoid RHI edits and central trait duplication; pass definitions are immutable runtime/tool inputs. |
| 17A | Shader authoring manifest/generator, ShaderCompiler, pass definition identity | Shader registration boilerplate is removed without weakening typed reflection diagnostics; package/layout/path/entry/stage metadata is authored once. |
| 17B | Pass authoring friction budget, pass scaffolding/generation, frame insertion workflow, validation workflow | The current pass-add touch count is enumerated, a target budget is enforced, and ordinary pass authoring removes mechanical edits without weakening graph/shader/PSO diagnostics. |
| 18 | Ray tracing contract, RhiContracts, RenderContracts, ThreadingReadiness | Renderer owns RT scene/shadow policy; RHI owns API-level AS descriptors/build commands; BLAS/TLAS generations and AS build requests are explicit. |
| 19 | Backend-private symmetry, RHI services, CMake target scopes | D3D12/Vulkan services are sibling implementations with named differences, no cross-backend includes, and no backend-native details leaking into common RHI/Renderer. |
| 20 | Full graphics validation, backend parity, ToolContracts, reviewer evidence | Build/smoke/capture/log evidence covers shader compiler, launcher, RT/upscaling/frame graph/PSO diagnostics, and known backend differences without unsupported performance claims. |
| 23 | Repository ownership, folder architecture, coverage | Every durable root has owner, allowed dependencies, forbidden dependencies, target folder shape, validation command, complexity status, and threading handoff risk before code motion resumes. |
| 24 | Core, Platform, foundation contracts, host boundary | Core remains policy-free; Platform owns OS/window/input only; event/config/logging/file abstractions do not absorb renderer, tool, launcher, or future scheduler policy. |
| 25 | GameFramework, AssetContracts, RenderContracts, ToolContracts | GameFramework owns runtime scene/cooked loading and emits render snapshots; shared schemas move toward contracts; source import/cook/renderer pass logic stays outside GameFramework. |
| 26 | Runtime asset loaders, cooked schemas, sample load evidence | Mesh/material/texture/scene/animation/skeleton loaders validate versioned cooked records with paired cooker/runtime/renderer updates and asset-oriented diagnostics. |
| 27 | SourceImporters, imported DTOs, import diagnostics | Source import architecture is role-named, deterministic, source-format isolated, and produces imported DTOs plus diagnostics without runtime module dependencies. |
| 28 | Focused cookers, AssetContracts, deterministic artifact output | Texture/Mesh/Material/Scene cookers own focused transformations and cooked artifact emission; outputs are deterministic and paired with runtime consumers. |
| 29 | ShaderCompiler, ShaderContracts, package manifests, tool reports | ShaderCompiler consumes contract/catalog data, emits deterministic package/reflection reports, and does not link full renderer runtime as a package enumeration shortcut. |
| 30 | AssetCooker, LauncherCore, Qt GUI, Application/Editor hosts, ToolContracts | AssetCooker orchestrates jobs/reports; LauncherCore owns workflows/process requests; Qt GUI owns presentation; Application/Editor do not duplicate cook/import/backend/tool algorithms. |
| 31 | Artifact validation matrix, ShaderContracts, AssetContracts, ToolContracts | Every shader/cooked artifact names producer, schema owner, consumer, inspector, validation command, failure report, and smoke/load evidence. |
| 32 | Projects, Engine/Assets, sample content, reviewer evidence | Built-in assets and project content have explicit owners, manifests, validation paths, and no ambiguous engine/project shader/data mixing. |
| 33 | Repo-wide guardrails, CMake/CI/local checks, generated-root policy | Local/CI-friendly checks cover runtime-to-tools, private include, launcher/tool ownership, generated/local roots, folder policy, and threading-hostile handoffs. |
| 34 | Whole-repo evidence gate, coverage, docs/code/CMake consistency | Repository coverage, target architecture, graphs, CMake/CI, tools, samples, docs, and validation evidence agree; no unowned refactor rows or duplicate production paths remain. |
| 35 | ThreadingReadiness across all contracts | Every durable module names mutable owner, phase, handoff shape, isolation, ordering, diagnostics identity, deterministic output, and any remaining non-blocking risk owner. |
| 36 | Final cleanup, rubric scoring, review-ready repository gate | All module families have implementation evidence, stale transition paths are removed, rubric scores are recorded, and remaining risks are explicit rather than hidden as future audits. |

## Mandatory Split Checkpoints For Large Stages

These checkpoints are stage-local acceptance gates. They are intentionally smaller than the stage title, so implementation can stop at a clean boundary instead of blending unrelated architectural work. Promote a checkpoint into a new numbered stage when it cannot be implemented, validated, and documented cleanly with the rest of the stage.

| Stage | Required checkpoints before acceptance |
| --- | --- |
| 7 | RHI interop service; RHI capture/readback service; RHI diagnostics service; RHI presentation service; public facade cleanup after each service has callers and validation. |
| 10 | Shader/package validation; launcher smoke workflow validation; D3D12 lit/debug evidence; Vulkan lit/debug evidence; RT/upscaling/capture capability reports; docs/evidence update. |
| 11 | Public Renderer host protocol; composition/system root extraction; frame pipeline extraction; diagnostics and lifecycle handoff; old facade responsibility deletion. |
| 13 | Render snapshot contract; mesh/material scene staging; texture/cooked upload ownership; temporal/upscaling input ownership; denoising placeholder disposition. |
| 16 | Package identity and generation; binding layout/reflection validation; printable PSO key; runtime cache ownership; D3D12/Vulkan normalized descriptor evidence. |
| 17 | Pass definition schema; pass catalog ownership; graph setup integration; pipeline runtime lookup integration; proof pass migration; old trait/duplicate registration cleanup. |
| 17A | Shader registration manifest/generator; duplicate constant cleanup; ShaderCompiler enumeration; duplicate/missing/stage/layout validation; pass definition identity handoff. |
| 17B | Current pass-add touch-count audit; target authoring budget; pass scaffolder or manifest-backed workflow; frame insertion contract; proof pass before/after measurement; ShaderCompiler and boundary validation. |
| 18 | Renderer RT scene generation; BLAS cache/build request ownership; TLAS frame data ownership; RHI AS descriptor/build command contract; shadow pass data ownership; fallback diagnostics. |
| 19 | D3D12 service map; Vulkan service map; shared common RHI surface; CMake target scope cleanup; cross-backend include check; parity evidence for named service differences. |
| 20 | Build/tool validation; shader/package validation; D3D12 smoke artifacts; Vulkan smoke artifacts; feature fallback reports; performance-claim audit; final graphics evidence index. |
| 23 | Durable root inventory; owner/dependency/forbidden dependency rows; target folder comparison; generated/local-only audit; validation command map; threading handoff risk map. |
| 24 | Core policy-free scan; Platform OS/window/input ownership; event/config/logging/file boundaries; dependency cleanup; build evidence. |
| 25 | Shared schema extraction decision; runtime scene snapshot contract; GameFramework loader/asset ownership; renderer/tool forbidden-edge cleanup; snapshot/load validation. |
| 26 | Cooked mesh loader; cooked material loader; cooked texture reference path; cooked scene/level manifest loader; animation/skeleton loader; paired cooker/runtime diagnostics. |
| 27 | Source importer folder/name migration; per-format importer ownership; imported DTO/report contract; runtime/tool-private dependency scan; sample import evidence. |
| 28 | TextureCooker boundary; MeshCooker boundary; MaterialCooker boundary; SceneCooker boundary; CookDiagnostics/ToolConsoleSupport split; deterministic artifact output proof. |
| 29 | ShaderCompiler contract input; package manifest/reflection reports; backend target options; deterministic job identity; renderer runtime decoupling proof. |
| 30 | AssetCooker orchestration-only proof; LauncherCore process request/report contract; Qt model/view presentation boundary; Application host boundary; Editor host boundary; operation history/recovery evidence. |
| 31 | Shader package matrix; texture artifact matrix; mesh artifact matrix; material artifact matrix; scene/animation/skeleton artifact matrix; inspector commands; smoke/load evidence. |
| 32 | Engine asset root disposition; project data/shader root disposition; Showcase validation content; sample cook/load/smoke flow; reviewer artifact naming. |
| 33 | Runtime-to-tools check; GameFramework/private edge check; launcher/tool ownership check; generated/local root check; CMake target-scope check; threading-hostile handoff check; CI/local wiring. |
| 34 | Coverage final state; docs/code/CMake consistency; sample/tool evidence; remaining risk table; duplicate path cleanup. |
| 35 | Engine runtime threading-readiness audit; graphics/RHI threading-readiness audit; tooling/content threading-readiness audit; launcher/host threading-readiness audit; CMake/CI/docs evidence audit. |
| 36 | Final stale-path deletion; final rubric scoring; final source-root inventory; final validation command set; final known-risk signoff. |

## Stage 1 - Baseline Status And Evidence Freeze

Goal:

- Convert the architecture review's coverage audit into a tracked status map before any major refactor.
- Make current debt explicit instead of relying on memory.

Source references:

- `rhi-renderer-architecture-review.md`: `Whole-Codebase Coverage Audit`, `Coverage Acceptance Criteria`, `Initial Proposed Work Items`
- `architecture-review-acceptance-rubric.md`: `The Criteria`, `Sparkle-Specific Review Questions`

Target shape references:

- Stage target-doc checklist: [Required Target Documents By Stage](#required-target-documents-by-stage), Stage 1 row.
- Primary target docs: [rendering-coverage-status.md](../architecture/rendering-coverage-status.md), [repository-coverage-status.md](../architecture/repository-coverage-status.md), [repository-current-state.md](../architecture/before/repository-current-state.md), [repository-target-architecture.md](../architecture/after/repository-target-architecture.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md)

External implementation references:

- arc42 quality and architecture documentation structure: https://arc42.org/overview
- CMU SEI ATAM quality-attribute review method: https://www.sei.cmu.edu/library/architecture-tradeoff-analysis-method-collection/
- ADR practice for traceable decisions: https://adr.github.io/

Code references:

- `Engine/Renderer`
- `Engine/RHI`
- `Engine/Application/Private/Validation`
- `Tools/Launcher/SparkleLauncher/Private/Launch/Smoke`

Tutor note:

- What is wrong today: we know several important risks, but they live as prose and memory rather than a tracked status table.
- What changes: every Renderer/RHI area gets a status, owner, risk, validation artifact, and linked stage.
- Why it improves the engine: this prevents "we forgot that subsystem" refactors and teaches you to drive architecture from evidence instead of anxiety.

Implementation prompt:

```text
Using the arc42, ATAM, and ADR references listed in this stage, create a tracked architecture status document for all Renderer/RHI coverage rows. For each row, assign status Accepted, Needs refactor, or Needs design decision. Add owner layer, primary risk, validation artifact, related stage in this execution plan, and final acceptance evidence. Do not change runtime code in this stage.
```

Positive guardrails:

- Use the exact folder rows from the architecture review as the starting point.
- Mark uncertainty honestly.
- Prefer specific evidence such as file names, command names, and expected artifacts.
- Keep this as a living checklist that future stages update.

Negative guardrails:

- Do not mark a subsystem accepted because it "seems fine."
- Do not combine unrelated subsystems into vague buckets.
- Do not start refactoring before the baseline is written.

Data transfer contracts:

- This stage transfers architecture state through `docs/architecture/rendering-coverage-status.md`, not through source changes.
- Status rows must name producer evidence, owner layer, consumer stage, validation artifact, and final acceptance criteria.
- Do not create implicit ownership by moving files or adding dependencies in this documentation-only stage.

Legacy cleanup:

- None yet. This is an evidence-freeze stage.

Acceptance:

- New status document exists, likely `docs/architecture/rendering-coverage-status.md`.
- Every coverage row has a status and owner.
- Every `Needs refactor` row links to a later stage in this document.
- Every `Needs design decision` row has an explicit question.

Validation:

- Docs-only. No build required.

## Stage 2 - Reviewer Architecture Docs And Vocabulary

Goal:

- Create the architecture docs that make the system reviewable by someone new to the repo.
- Freeze vocabulary before renaming or moving systems.

Source references:

- `rhi-renderer-architecture-review.md`: `Proposed Review Process`, `Target Hierarchy`, `System Edge Review`
- `architecture-review-acceptance-rubric.md`: `Documentation and onboarding`, `Communication and design rationale`

Target shape references:

- Stage target-doc checklist: [Required Target Documents By Stage](#required-target-documents-by-stage), Stage 2 row.
- Primary target docs: [system-design-index.md](../architecture/after/system-design-index.md), [rendering-glossary.md](../architecture/rendering-glossary.md), [rendering-system-map.md](../architecture/rendering-system-map.md), [rhi-contract-map.md](../architecture/rhi-contract-map.md), [frame-graph-contract.md](../architecture/frame-graph-contract.md), [pass-authoring-contract.md](../architecture/pass-authoring-contract.md), [pipeline-runtime-contract.md](../architecture/pipeline-runtime-contract.md), [ray-tracing-contract.md](../architecture/ray-tracing-contract.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md)

External implementation references:

- arc42 architecture documentation structure: https://arc42.org/overview
- NVIDIA Donut repository organization: https://github.com/NVIDIA-RTX/Donut
- NVIDIA Falcor documentation entry point: https://github.com/NVIDIAGameWorks/Falcor/blob/master/docs/getting-started.md

Code references:

- `Engine/Renderer/Public/Renderer.h`
- `Engine/RHI/Public/Device/RenderHardwareInterface.h`
- `Engine/Renderer/Private/FrameGraph`
- `Engine/Renderer/Private/RayTracing`
- `Engine/Renderer/Private/Pipeline`

Tutor note:

- What is wrong today: the code has real systems, but a new reviewer has to infer vocabulary and ownership from source files.
- What changes: we write the map, glossary, contracts, and diagrams that explain the engine before someone dives into implementation details.
- Why it improves the engine: good architecture is not only code shape; it is also shared language that prevents future features from choosing random names and boundaries.

Implementation prompt:

```text
Using arc42's documentation structure and the repository navigation style visible in Donut and Falcor, add architecture docs for rendering vocabulary, system map, RHI contract map, frame graph contract, ray tracing contract, pass authoring contract, and pipeline runtime contract. Link them from the existing architecture review. Keep docs precise, code-referenced, and aligned with current code, even where the current code is imperfect.
```

Positive guardrails:

- Define terms such as RHI, backend, command context, command list, frame graph pass, pass runtime, PSO key, native interop, BLAS, TLAS, and upscaler provider.
- Include simple diagrams for frame execution, shader package flow, PSO creation, and backend boundaries.
- Keep references to code files clickable and current.
- Make docs say what owns what, not only what exists.

Negative guardrails:

- Do not invent architecture that is not implemented or planned in this runbook.
- Do not use vague claims like "clean architecture" without concrete ownership.
- Do not duplicate the same explanation across many docs.

Data transfer contracts:

- Vocabulary moves through `docs/architecture/rendering-glossary.md` and linked contract docs.
- Broad flow moves through Mermaid graphs and code references, not private include shortcuts or new runtime APIs.
- Each doc must name the module that owns the data being passed: RHI descriptors, renderer snapshots, shader packages, cooked artifacts, process requests, or validation evidence.

Legacy cleanup:

- Remove stale references to deleted planning docs.

Acceptance:

- `docs/architecture/rendering-glossary.md` exists.
- `docs/architecture/rendering-system-map.md` exists.
- `docs/architecture/rhi-contract-map.md` exists.
- `docs/architecture/frame-graph-contract.md` exists.
- `docs/architecture/ray-tracing-contract.md` exists.
- `docs/architecture/pass-authoring-contract.md` exists.
- `docs/architecture/pipeline-runtime-contract.md` exists.
- Main planning docs link to the new architecture docs.

Validation:

- Docs-only. No build required.

## Stage 3 - Mechanical Boundary Guardrails

Goal:

- Add automated checks that enforce the intended layer direction before moving code.

Source references:

- `rhi-renderer-architecture-review.md`: `Phase 1: Boundary Audit`, `Strategic Refactor Tracks`, `Shader Registration Ownership`
- `architecture-review-acceptance-rubric.md`: `Separation of concerns`, `Testability`

Target shape references:

- Stage target-doc checklist: [Required Target Documents By Stage](#required-target-documents-by-stage), Stage 3 row.
- Primary target docs: [architecture-boundary-guardrails.md](../architecture/architecture-boundary-guardrails.md), [repository-system-map.md](../architecture/repository-system-map.md), [repository-target-folder-architecture.md](../architecture/after/repository-target-folder-architecture.md), [rhi-contract-map.md](../architecture/rhi-contract-map.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md)

External implementation references:

- NVIDIA NVRHI as a focused RHI layer: https://github.com/NVIDIA-RTX/NVRHI
- NVIDIA NRI as a graphics abstraction boundary: https://github.com/NVIDIA-RTX/NRI
- AMD Cauldron DX12/VK backend separation: https://github.com/GPUOpen-LibrariesAndSDKs/Cauldron
- Diligent Engine backend abstraction layout: https://github.com/DiligentGraphics/DiligentEngine

Current violations to protect against:

- Resolved by Stage 4, but kept protected by the check: RHI must not include `Renderer/Private` headers. The former `Engine/RHI/Private/Shaders/DirectLightingShaders.cpp` dependency on `Renderer/Private/RayTracing/RayTracedShadowUniformData.h` must not return.
- `Engine/Renderer/CMakeLists.txt` links `Vulkan::Vulkan` when Streamline is enabled.
- `Engine/Application/Private/Validation/RhiSmokeEditorValidation.cpp` includes D3D12 headers and owns D3D12-native capture logic.

Tutor note:

- What is wrong today: the intended layer order exists in guidance, but the compiler does not enforce it.
- What changes: forbidden include and dependency checks turn architecture from a promise into a guardrail.
- Why it improves the engine: once boundaries are mechanical, refactors become safer because new code cannot quietly reintroduce the same coupling.

Implementation prompt:

```text
Using the backend separation patterns in NVRHI, NRI, Cauldron, and Diligent Engine as the boundary model, add local and CMake/CI-friendly forbidden dependency checks for Renderer/RHI architecture boundaries. The checks must report actionable file paths and reasons. Initially allow documented transitional exceptions only where this plan has not yet migrated the code. Remove each exception in the stage that fixes it.
```

Positive guardrails:

- Check `Engine/RHI` for `Renderer/Private` includes.
- Check normal `Engine/Renderer` paths for D3D12/Vulkan private headers or native API identifiers, excluding explicitly documented provider integration paths during migration.
- Check D3D12 and Vulkan backend folders do not include each other.
- Check Application validation does not grow new backend-native dependencies.
- Add the check in a form usable by CI and local developers.

Negative guardrails:

- Do not suppress violations silently.
- Do not create a broad allowlist that hides future architectural drift.
- Do not block generated or third-party files unless they are part of engine source policy.

Data transfer contracts:

- Guardrail data moves through `CMake/ArchitectureBoundaryCheck.cmake`, `architecture_boundary_check`, and documented exception records.
- Checks must report file path, violated edge, reason, and owning removal stage.
- Do not encode policy only in prose; each enforceable forbidden edge should have a local/CI-friendly check or an explicit Stage 28 follow-up.

Legacy cleanup:

- No broad cleanup yet. Transitional allowlist entries must include the stage that removes them.

Acceptance:

- New check target or script exists, likely `architecture_boundary_check`.
- Existing violations are listed as temporary exceptions with removal stages.
- The check can be run without building the editor.

Validation:

- Run the boundary check.
- No full renderer build required unless the check is integrated into CMake and configure/build wiring must be verified.

## Stage 4 - Move Renderer Shader Registration Out Of RHI

Goal:

- Enforce the hard gate: ordinary renderer shader passes must not require RHI edits.
- Remove renderer-specific pass shader declarations from `Engine/RHI/Private/Shaders`.

Source references:

- `rhi-renderer-architecture-review.md`: `Shader Registration Ownership`, `Shader Pass And PSO Handling`
- `architecture-review-acceptance-rubric.md`: `Shader and pipeline systems`, `Cross-module architecture`
- `repository-target-folder-architecture.md`: renderer-owned shader folders, `ShaderContracts`, and RHI generic shader fixtures

Target shape references:

- Stage target-doc checklist: [Required Target Documents By Stage](#required-target-documents-by-stage), Stage 4 row.
- Primary target docs: [pass-authoring-contract.md](../architecture/pass-authoring-contract.md), [pipeline-runtime-contract.md](../architecture/pipeline-runtime-contract.md), [tooling-pipeline-contract.md](../architecture/tooling-pipeline-contract.md), [rhi-contract-map.md](../architecture/rhi-contract-map.md), [repository-target-folder-architecture.md](../architecture/after/repository-target-folder-architecture.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md)

External implementation references:

- NVIDIA Donut reusable rendering framework above NVRHI: https://github.com/NVIDIA-RTX/Donut
- NVIDIA NVRHI hardware abstraction layer: https://github.com/NVIDIA-RTX/NVRHI
- NVIDIA Falcor render pass model: https://github.com/NVIDIAGameWorks/Falcor

Code references:

- `Engine/RHI/Private/Shaders/BuiltinGlobalShaders.cpp`
- `Engine/Renderer/ShaderRegistrations/RendererGlobalShaders.cpp`
- `Engine/Renderer/ShaderRegistrations/GBufferShaders.cpp`
- `Engine/Renderer/ShaderRegistrations/DirectLightingShaders.cpp`
- `Engine/Renderer/ShaderRegistrations/IndirectLightingShaders.cpp`
- `Engine/Renderer/ShaderRegistrations/LightingCompositeShaders.cpp`
- `Engine/Renderer/ShaderRegistrations/SkyShaders.cpp`
- `Engine/Renderer/ShaderRegistrations/VisualizeBuffersShaders.cpp`
- `Engine/RHI/Private/Shaders/ComputeClearShader.cpp`
- `Engine/Renderer/Private/Passes`
- `Engine/Renderer/Private/Passes/RenderPassDefinition.h`
- `Engine/Renderer/Private/Pipeline/RenderPassDefinitionRuntime.h`
- `Engine/Renderer/ShaderRegistrations/RendererShaderPackages.h`
- `Tools/Shaders/ShaderCompiler/Private/Cooking/ShaderCookPlanner.cpp`

Tutor note:

- What was wrong before Stage 4: renderer-specific shader pass declarations lived in RHI, and DirectLighting pulled renderer-private shadow data into RHI.
- What changes: shader pass registration moves up to Renderer or a neutral shader-authoring layer, while RHI keeps only generic shader package and layout primitives.
- Why it improves the engine: an RHI should know how to create/bind GPU objects, not know that a renderer has GBuffer, Sky, or DirectLighting passes.

Implementation prompt:

```text
Using Donut and Falcor as examples of renderer-owned passes above a hardware abstraction, and NVRHI as the lower abstraction boundary, move renderer pass shader registration from Engine/RHI/Private/Shaders into Renderer-owned shader authoring folders and/or ShaderContracts. Treat any current Engine/Renderer/ShaderRegistrations folder as a migration source unless it already matches the PassCatalog target. Preserve generic RHI shader infrastructure and genuinely generic builtin test shaders in RHI. Delete the old renderer pass registration files from RHI after the new registrations are wired and the shader compiler can still enumerate the same packages.
```

Positive guardrails:

- Renderer owns pass names, shader paths, entry points, expected stages, binding layout IDs, and pass-specific uniform structs.
- RHI owns only generic shader package/reflection/layout/runtime primitives.
- Target folders are `Engine/Renderer/Private/PassCatalog`, `Engine/Renderer/Shaders`, and `Engine/Contracts/Shader` or their documented stage-local equivalents.
- The shader compiler can still collect registrations without making RHI depend on Renderer.
- Keep package IDs stable unless a deliberate migration note says otherwise.
- Use `DirectLighting` as the proof because it currently includes renderer-private shadow data from RHI.

Negative guardrails:

- Do not move `RayTracedShadowUniformData` into RHI just to satisfy the include rule.
- Do not keep duplicate renderer pass registrations in both RHI and Renderer.
- Do not add backend-specific registration code.
- Do not create a second permanent shader registry folder. If `Engine/Renderer/ShaderRegistrations` remains, name the stage that folds it into `PassCatalog`/`ShaderContracts`.
- Do not solve this by weakening the boundary check.

Data transfer contracts:

- Renderer pass metadata transfers through renderer-owned shader registration APIs and the `SparkleRendererShaderRegistrations` target.
- ShaderCompiler consumes renderer package registration data through the narrow registration target, not `Engine/Renderer/Private` pass runtime internals.
- Long-term ShaderCompiler handoff transfers through `Engine/Contracts/Shader` pass catalogs, package manifests, reflection, and binding layout records.
- RHI receives only generic cooked shader package/reflection/layout/runtime primitives; pass-specific uniform structs remain with Renderer-owned data.

Legacy cleanup:

- Delete renderer pass registration files from `Engine/RHI/Private/Shaders` once moved.
- Remove renderer pass calls from `RegisterBuiltinGlobalShaders`.
- Delete or rename transitional renderer registration folders when `PassCatalog`/`ShaderContracts` own the final structure.
- Remove transitional boundary-check exceptions for RHI-to-Renderer includes.

Acceptance:

- `rg "Renderer/Private" Engine/RHI` returns no violations.
- Adding Bloom, SSAO, SSR, debug visualization, lighting variants, or material shaders would require no `Engine/RHI` edit.
- Shader compiler package enumeration still includes all expected renderer pass packages.
- Docs name the new shader registration ownership.

Validation:

- Defer full build to Stage 5.

## Stage 5 - Validation Milestone A: Boundaries And Shader Registration

Goal:

- Validate the first major section: docs, boundary checks, and shader registration ownership.

Target shape references:

- Stage target-doc checklist: [Required Target Documents By Stage](#required-target-documents-by-stage), Stage 5 row.
- Primary target docs: [architecture-boundary-guardrails.md](../architecture/architecture-boundary-guardrails.md), [pass-authoring-contract.md](../architecture/pass-authoring-contract.md), [pipeline-runtime-contract.md](../architecture/pipeline-runtime-contract.md), [tooling-pipeline-contract.md](../architecture/tooling-pipeline-contract.md), [rendering-coverage-status.md](../architecture/rendering-coverage-status.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md)

External implementation references:

- NVIDIA NVRHI tutorial validation mindset: https://github.com/NVIDIA-RTX/NVRHI/blob/main/doc/Tutorial.md
- NVIDIA Donut Samples as executable graphics evidence: https://github.com/NVIDIA-RTX/Donut-Samples
- Khronos Vulkan validation layers: https://github.com/KhronosGroup/Vulkan-ValidationLayers

Tutor note:

- What is wrong today: it is easy to finish a structural move and only later discover shader cooking or registration broke.
- What changes: this stage validates the first boundary slice before deeper refactors make failures harder to localize.
- Why it improves the engine: milestone validation teaches you to split big architecture work into chunks that can prove they did not break the foundation.

Implementation prompt:

```text
Using the validation habits from NVRHI tutorials, Donut Samples, and Vulkan validation layers, run the smallest meaningful validation for the boundary and shader-registration migration. Verify boundary checks, shader compiler build, renderer build surface, and shader package enumeration. Record commands, results, and remaining exceptions in the coverage status document.
```

Positive guardrails:

- Validate both the mechanical check and the actual build path.
- Capture failure output in the status doc if a build fails.
- Prefer targeted build commands over full solution build unless dependencies require it.

Negative guardrails:

- Do not proceed with known RHI-to-Renderer include violations.
- Do not accept duplicated shader package registration.
- Do not leave deleted files referenced by CMake or build tooling.

Data transfer contracts:

- Validation evidence transfers through command output, coverage-status notes, and shader package enumeration results.
- CMake target relationships must remain explicit: `ShaderCompiler` may link the renderer registration target, not full renderer runtime.
- Failure context must include target name, package/registration path, boundary check rule, and command used.

Suggested validation:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config DevelopmentEditor --target ShaderCompiler -- /nologo /v:minimal /m:1
cmake --build build --config DevelopmentEditor --target SparkleLauncher -- /nologo /v:minimal /m:1
cmake --build build --config DevelopmentEditor --target clang_format_check -- /nologo /v:minimal
```

Acceptance:

- Boundary check passes with no permanent exceptions for RHI-to-Renderer includes.
- `ShaderCompiler` builds.
- `SparkleLauncher` builds or a more relevant runtime/editor target builds if target names change.
- Coverage status is updated.

## Stage 6 - RHI Method Ownership And Service Extraction Design

Goal:

- Classify the large `RenderHardwareInterface` before splitting it.
- Prevent future root-facade bloat.

Source references:

- `rhi-renderer-architecture-review.md`: `RHI Interface Is Too Broad`, `Phase 2: RHI Contract Classification`
- `architecture-review-acceptance-rubric.md`: `Cohesion and interface size`, `Tradeoff reasoning`

Target shape references:

- Stage target-doc checklist: [Required Target Documents By Stage](#required-target-documents-by-stage), Stage 6 row.
- Primary target docs: [rhi-contract-map.md](../architecture/rhi-contract-map.md), [repository-target-architecture.md](../architecture/after/repository-target-architecture.md), [repository-target-folder-architecture.md](../architecture/after/repository-target-folder-architecture.md), [repository-system-map.md](../architecture/repository-system-map.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md)

External implementation references:

- NVIDIA NVRHI focused graphics abstraction: https://github.com/NVIDIA-RTX/NVRHI
- NVIDIA NRI low-level rendering interface: https://github.com/NVIDIA-RTX/NRI
- Diligent Engine device/context/pipeline abstraction: https://github.com/DiligentGraphics/DiligentEngine

Code references:

- `Engine/RHI/Public/Device/RenderHardwareInterface.h`
- `Engine/RHI/Private/D3D12/D3D12RenderHardwareInterface.cpp`
- `Engine/RHI/Private/Vulkan/VulkanRenderHardwareInterface.cpp`
- `Engine/RHI/Public/Commands/RenderCommandList.h`

Tutor note:

- What is wrong today: `RenderHardwareInterface` is convenient, but it mixes device, resources, descriptors, pipelines, constants, RT, presentation, diagnostics, capture, interop, and UI.
- What changes: we classify every method before extracting anything, so the split is based on real caller pressure.
- Why it improves the engine: reviewers trust an interface more when every method has one reason to exist and one owner.

Implementation prompt:

```text
Using NVRHI, NRI, and Diligent Engine as examples of focused graphics abstraction boundaries, create a complete RHI method ownership table. Categorize every RenderHardwareInterface method by service: device/capability, command queue/list, resources, descriptors/views, pipelines/binding layouts, constants/uploads, ray tracing, presentation, diagnostics, interop, capture/readback, UI. Map current callers and propose extraction order. Do not split the interface before this table is complete.
```

Positive guardrails:

- Every method gets one primary owner category.
- Methods may list secondary users, but not multiple primary owners.
- Each category names D3D12 and Vulkan implementation files.
- Identify which methods are renderer conveniences and should move upward or behind a narrower bridge.

Negative guardrails:

- Do not start by creating many abstract interfaces without caller evidence.
- Do not move methods just to reduce line count.
- Do not hide API-specific requirements behind vague "misc" categories.

Data transfer contracts:

- RHI ownership data transfers through `docs/architecture/rhi-contract-map.md`.
- Every public method row must name caller modules, primary service owner, backend implementation files, and any tool/runtime consumers.
- Proposed service boundaries must move typed RHI descriptors, capabilities, diagnostics, or handles through public RHI contracts, not backend-private headers.

Legacy cleanup:

- Mark methods that should disappear from the root facade after service extraction.

Acceptance:

- `docs/architecture/rhi-contract-map.md` contains every public RHI method.
- Categories with more than 10 methods have an extraction proposal.
- New RHI methods are forbidden unless added to the map with owner and caller.

Validation:

- Docs-only unless helper scripts are added.

## Stage 7 - Extract First RHI Services: Interop, Capture, Diagnostics, Presentation

Goal:

- Move the most cross-cutting pressure points out of the root RHI facade first.
- Give Application validation and DLSS a stable backend-owned route instead of native API leakage.

Source references:

- `rhi-renderer-architecture-review.md`: `Native Interop Is Necessary But Needs A Formal Contract`, `System Edge Review`, `Track 4`
- `architecture-review-acceptance-rubric.md`: `Reliability/failure handling`, `Observability and diagnostics`

Target shape references:

- Stage target-doc checklist: [Required Target Documents By Stage](#required-target-documents-by-stage), Stage 7 row.
- Primary target docs: [rhi-contract-map.md](../architecture/rhi-contract-map.md), [rendering-system-map.md](../architecture/rendering-system-map.md), [architecture-boundary-guardrails.md](../architecture/architecture-boundary-guardrails.md), [repository-target-folder-architecture.md](../architecture/after/repository-target-folder-architecture.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md)

External implementation references:

- NVIDIA NVRHI portable rendering code article: https://developer.nvidia.com/blog/writing-portable-rendering-code-with-nvrhi/
- Diligent Engine service-like graphics abstractions: https://github.com/DiligentGraphics/DiligentEngine
- NVIDIA Streamline programming guide: https://github.com/NVIDIA-RTX/Streamline/blob/main/docs/ProgrammingGuide.md

Code references:

- `RenderHardwareInterface::GetDeviceHandle`
- `RenderHardwareInterface::GetGraphicsQueueHandle`
- `RenderHardwareInterface::UpgradePresentationInterface`
- `RenderHardwareInterface::CaptureTextureToBmp`
- `RenderHardwareInterface::GetNativeTextureViewInfo`
- `RenderHardwareInterface::ResolveImGuiTextureId`
- `RenderHardwareInterface::BeginPresentRenderPass`
- `RenderHardwareInterface::BeginPresentOverlayPass`
- `RenderHardwareInterface::EndPresentRenderPass`
- `Engine/RHI/Public/Interop/RhiNativeHandles.h`
- `Engine/RHI/Public/Diagnostics/RhiDiagnostics.h`
- `Engine/RHI/Public/UI/RhiImGuiRenderer.h`

Tutor note:

- What is wrong today: external SDKs, capture, diagnostics, UI, and presentation all pressure the root RHI facade.
- What changes: these become narrow services so renderer/application code asks for the capability it needs instead of grabbing the whole backend.
- Why it improves the engine: service boundaries make native interop explicit and stop one integration from turning RHI into a bag of unrelated escape hatches.

Implementation prompt:

```text
Using NVRHI/Diligent-style service responsibilities and Streamline's explicit interop requirements as references, extract the first narrow RHI services for external interop, capture/readback, diagnostics, and presentation/UI. Keep RenderHardwareInterface as a facade during migration, but delegate to service objects internally. Update D3D12 and Vulkan implementations symmetrically. Keep temporary facade forwarding only until callers are migrated.
```

Positive guardrails:

- Keep D3D12 and Vulkan service names symmetric.
- Let RHI fill native metadata deterministically.
- Make provider needs explicit: Streamline D3D12, Streamline Vulkan, future FSR/NRD.
- Keep diagnostics and capture errors actionable.

Negative guardrails:

- Do not add more `void*` casting in Renderer or Application.
- Do not make Renderer include backend-private headers.
- Do not let interop structs become undifferentiated bags of fields without consumer notes.

Data transfer contracts:

- Native interop transfers through typed RHI public structs and provider-scoped capability records.
- Capture/readback transfers through an RHI capture service returning explicit success/failure diagnostics and artifact paths.
- Presentation/UI handoff transfers through RHI presentation/UI service contracts; Application and Renderer must not exchange backend-native objects directly.

Legacy cleanup:

- Remove facade methods from direct callers after service migration where practical.
- Remove transitional casts outside backend/provider integration.

Acceptance:

- DLSS/upscaling provider talks to explicit interop/presentation services.
- Application validation no longer needs native device/queue handles directly after Stage 8.
- D3D12/Vulkan service implementations have matching responsibilities.

Validation:

- Defer full build to Stage 10 unless interface extraction creates immediate compile blockers.

## Stage 8 - Move Smoke Capture And Backend-Native Validation Behind RHI

Goal:

- Remove backend-native D3D12 capture code from Application validation.
- Make smoke validation usable for both D3D12 and Vulkan.

Source references:

- `rhi-renderer-architecture-review.md`: `Application Validation -> Backend APIs`, `Backend Parity Matrix`
- `architecture-review-acceptance-rubric.md`: `Debuggability and validation`, `Testability`

Target shape references:

- Stage target-doc checklist: [Required Target Documents By Stage](#required-target-documents-by-stage), Stage 8 row.
- Primary target docs: [rhi-contract-map.md](../architecture/rhi-contract-map.md), [architecture-boundary-guardrails.md](../architecture/architecture-boundary-guardrails.md), [repository-system-map.md](../architecture/repository-system-map.md), [tooling-pipeline-contract.md](../architecture/tooling-pipeline-contract.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md)

External implementation references:

- Khronos Vulkan validation layers: https://github.com/KhronosGroup/Vulkan-ValidationLayers
- Vulkan development tools guide: https://docs.vulkan.org/guide/latest/development_tools.html
- Diligent Engine backend abstraction and testing style: https://github.com/DiligentGraphics/DiligentEngine

Code references:

- `Engine/Application/Private/Validation/RhiSmokeEditorValidation.cpp`
- `Engine/Application/Private/Validation/RhiSmokeValidation.cpp`
- `Tools/Launcher/SparkleLauncher/Private/Launch/Smoke/RhiSmokeLaunchOperations.cpp`
- RHI capture/readback service from Stage 7

Tutor note:

- What is wrong today: Application validation knows how to do D3D12-native capture, which means the test layer owns backend details.
- What changes: Application asks RHI to capture/read back; D3D12 and Vulkan own the native work.
- Why it improves the engine: tests should verify behavior across backends, not become another backend implementation.

Implementation prompt:

```text
Using Vulkan validation tooling and Diligent-style backend abstraction as references, refactor RHI smoke capture/readback so Application validation requests capture through an RHI-owned service. Remove D3D12 headers and D3D12-specific capture implementation from Application. Extend smoke evidence to support backend, view mode, capture path, frame graph warning status, DLSS status, and ray tracing status.
```

Positive guardrails:

- Application owns test orchestration, not backend implementation.
- Backend capture code lives in RHI backend or a clearly marked backend-owned validation helper.
- Smoke validation can capture lit and debug/normal view modes for D3D12 and Vulkan.
- Smoke failures should be deterministic and logged with backend, frame, view mode, and output path.

Negative guardrails: 

- Do not keep a D3D12-only capture fallback in Application.
- Do not make Vulkan capture a TODO while claiming backend parity.
- Do not allow smoke to pass while frame graph unresolved-resource warnings are present.

Data transfer contracts:

- Application sends capture requests through the RHI capture/readback service, not D3D12/Vulkan headers.
- Capture evidence transfers as backend, view mode, frame number, output path, support status, and failure reason.
- Launcher smoke workflows transfer validation parameters through documented command-line/environment contracts, not hardcoded Application internals.

Legacy cleanup:

- Delete Application-local BMP/D3D12 readback helpers after RHI capture service works.
- Remove temporary boundary-check exception for Application D3D12 native headers.

Acceptance:

- `RhiSmokeEditorValidation.cpp` includes no D3D12/Vulkan native headers.
- Smoke capture uses RHI service on both D3D12 and Vulkan or reports unsupported with a precise reason.
- Launcher smoke options expose or document backend, view mode, and capture evidence path.

Validation:

- Defer full runtime smoke to Stage 10.

Completion evidence:

| Field | Evidence |
| --- | --- |
| Status | Fully completed for Stage 8 code ownership, guardrail cleanup, launcher smoke controls, and targeted builds. |
| Source ownership | `RhiSmokeEditorValidation.cpp` no longer includes D3D12/Vulkan native headers and no longer owns Application-local BMP/D3D12 readback helpers. |
| RHI capture contract | `RhiCaptureService` now returns backend, status, frame, view mode, artifact path, and failure reason through `RhiCaptureResult`. D3D12 and Vulkan adapters fill the same evidence fields. |
| Renderer evidence | `RendererSmokeDiagnosticsSnapshot` reports backend, frame graph unresolved-barrier warnings, upscaler provider/status/reason, and ray tracing support without Application inspecting renderer internals. Smoke validation fails unresolved frame graph barrier warnings. |
| Launcher evidence | `SparkleLauncher` exposes smoke view mode and capture path through GUI and CLI, and transfers them through `SPARKLE_SMOKE_VIEW_MODE` and `SPARKLE_SMOKE_SCENE_COLOR_CAPTURE`. |
| Guardrail cleanup | The `APPLICATION_VALIDATION_NO_BACKEND_NATIVE` counted exception was removed from `CMake/ArchitectureBoundaryCheck.cmake`; future native API usage under `Engine/Application/Private/Validation` fails the check. |
| Validation | `architecture_boundary_check`, `SparkleApplicationEditor`, and `SparkleLauncher` passed in `build/windows-vs2026-stage5` with `DevelopmentEditor`. `clang_format_check` was not generated in this build tree because `clang-format` was unavailable during configure. Full D3D12/Vulkan runtime smoke remains Stage 10. |

## Stage 9 - Formalize Upscaling And Native Interop Contracts

Goal:

- Make DLSS and future external providers cleanly separated from RHI and Renderer policy.
- Fix current ambiguity around Vulkan linkage in `Engine/Renderer/CMakeLists.txt`.

Source references:

- `rhi-renderer-architecture-review.md`: `Native Interop Is Necessary But Needs A Formal Contract`, `Vendor SDKs Should Stay Out Of Core RHI Policy`
- `architecture-review-acceptance-rubric.md`: `Reliability and fallback behavior`, `Graphics API fluency`

Target shape references:

- Stage target-doc checklist: [Required Target Documents By Stage](#required-target-documents-by-stage), Stage 9 row.
- Primary target docs: [rendering-system-map.md](../architecture/rendering-system-map.md), [upscaler-provider-contract.md](../architecture/upscaler-provider-contract.md), [rhi-contract-map.md](../architecture/rhi-contract-map.md), [architecture-boundary-guardrails.md](../architecture/architecture-boundary-guardrails.md), [repository-target-architecture.md](../architecture/after/repository-target-architecture.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md)

External implementation references:

- NVIDIA Streamline repository: https://github.com/NVIDIA-RTX/Streamline
- NVIDIA Streamline programming guide: https://github.com/NVIDIA-RTX/Streamline/blob/main/docs/ProgrammingGuide.md
- NVIDIA Streamline manual hooking guide: https://github.com/NVIDIA-RTX/Streamline/blob/main/docs/ProgrammingGuideManualHooking.md
- AMD FidelityFX SDK provider/backend pattern: https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK

Code references:

- `Engine/Renderer/Private/Upscaling`
- `Engine/Renderer/Private/Upscaling/NvidiaDlss`
- `Engine/Renderer/CMakeLists.txt`
- `Engine/RHI/Public/Interop/RhiNativeHandles.h`
- `Engine/RHI/Private/D3D12/Device/D3D12ExternalFeatureInteropCapabilities.*`
- `Engine/RHI/Private/Vulkan/Device/VulkanExternalFeatureInteropCapabilities.*`

Tutor note:

- What is wrong today: DLSS needs native resource details, but without a formal contract those details can leak into Renderer or root RHI policy.
- What changes: provider code owns vendor SDK calls, RHI backends own API setup/native metadata, and common renderer code stays provider-neutral.
- Why it improves the engine: vendor integrations are powerful but contagious; isolating them keeps future FSR/NRD integrations from warping the renderer architecture.

Implementation prompt:

```text
Using Streamline's programming/manual-hooking guides and FidelityFX SDK's provider/backend separation as references, document and implement a provider-facing upscaler input and native interop contract. Keep NVIDIA Streamline implementation inside the NvidiaDlss provider, keep backend extension/device setup inside RHI backends, and remove unexplained renderer-level Vulkan linkage or document it as a provider SDK requirement with a narrow wrapper. Make failure reasons visible.
```

Positive guardrails:

- Provider code may use vendor SDK headers.
- Backend code may enable provider-required API features and log why.
- Renderer pass/frame code should see provider-neutral upscaler contracts.
- DLSS unavailable states must say whether the cause is SDK, driver, backend, feature, resource state, or input contract.

Negative guardrails:

- Do not let `NvidiaDlss` policy leak into common RHI.
- Do not let Streamline-specific details appear in general renderer pass code.
- Do not use passthrough fallback without logging the reason.

Data transfer contracts:

- Upscaler inputs transfer through provider-neutral renderer upscaler input structs.
- Provider/native metadata transfers through RHI interop capability and texture/view metadata contracts.
- Provider failure state transfers through structured diagnostics naming SDK, driver, backend, feature, resource state, or input-contract cause.

Legacy cleanup:

- Remove or narrow `Vulkan::Vulkan` linkage from `SparkleRenderer` if the provider can be isolated behind RHI/provider wrapper.
- Delete obsolete native handle fields after the new contract replaces them.

Acceptance:

- Upscaler contract doc exists.
- D3D12 and Vulkan DLSS setup paths log capability/failure reasons.
- Renderer common code remains provider-neutral.
- Boundary check exceptions for Renderer Vulkan native linkage are removed or documented as a narrow provider exception.

Validation:

- Defer full backend DLSS smoke to Stage 10.

Completion evidence:

| Field | Evidence |
| --- | --- |
| Status | Fully completed for provider/native interop contract, CMake target ownership, structured DLSS diagnostics, and targeted build validation. Full D3D12/Vulkan DLSS runtime smoke remains Stage 10. |
| Contract doc | [upscaler-provider-contract.md](../architecture/upscaler-provider-contract.md) defines provider-neutral inputs, provider/native metadata flow, failure domains, linkage/header rules, and threading-ready handoffs. |
| CMake ownership | `SparkleRendererNvidiaDlssProvider` owns `NVIDIA::Streamline` and the narrow `Vulkan::Vulkan` link; common `SparkleRenderer` no longer links `Vulkan::Vulkan` directly. |
| Runtime diagnostics | `UpscalerProviderCapabilities`, `UpscalerEvaluationResult`, `DlssCapabilityReport`, and `StreamlineDlssRuntimeDiagnostics` carry `EUpscalerProviderFailureDomain` so unavailable/fallback states identify SDK, driver, backend, feature, resource-state, or input-contract cause. |
| Provider isolation | Streamline/Vulkan-native identifiers remain counted and pattern-limited to `Engine/Renderer/Private/Upscaling/NvidiaDlss/StreamlineDlssRuntime.cpp`; common frame/pass code uses provider-neutral upscaler contracts. |
| Guardrail evidence | Direct script and configured `architecture_boundary_check` passed with counted provider exceptions only. |
| Build evidence | VS2026 configure passed; `SparkleApplicationEditor` built in `build/windows-vs2026-stage5` with `DevelopmentEditor`, including `SparkleRendererNvidiaDlssProvider.lib` before `SparkleRenderer.lib`. |
| Source text hygiene | Source/provenance scan across `Engine/Renderer`, `Engine/RHI`, `Engine/Application`, and `CMake` found no stage/vibe/generated-process text. |

## Stage 10 - Validation Milestone B: RHI Services, Capture, Interop, Upscaling

Goal:

- Validate the second major section: RHI service extraction, smoke capture isolation, and upscaling interop.

Target shape references:

- Stage target-doc checklist: [Required Target Documents By Stage](#required-target-documents-by-stage), Stage 10 row.
- Primary target docs: [rendering-coverage-status.md](../architecture/rendering-coverage-status.md), [rendering-system-map.md](../architecture/rendering-system-map.md), [rhi-contract-map.md](../architecture/rhi-contract-map.md), [ray-tracing-contract.md](../architecture/ray-tracing-contract.md), [tooling-pipeline-contract.md](../architecture/tooling-pipeline-contract.md), [validation-workflow-contract.md](../architecture/validation-workflow-contract.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md)

External implementation references:

- NVIDIA Donut Samples validation-by-running approach: https://github.com/NVIDIA-RTX/Donut-Samples
- NVIDIA NVRHI tutorial: https://github.com/NVIDIA-RTX/NVRHI/blob/main/doc/Tutorial.md
- Khronos Vulkan validation layers: https://github.com/KhronosGroup/Vulkan-ValidationLayers
- NVIDIA Streamline programming guide: https://github.com/NVIDIA-RTX/Streamline/blob/main/docs/ProgrammingGuide.md

Tutor note:

- What is wrong today: upscaler, capture, and interop bugs can look like rendering noise or backend instability unless evidence names the active path.
- What changes: milestone validation records backend, view mode, capture artifact, DLSS status, and frame graph health.
- Why it improves the engine: good graphics engineering means failures explain themselves enough that you know which system to inspect first.

Implementation prompt:

```text
Using Donut Samples, NVRHI tutorial practices, Vulkan validation layers, Streamline diagnostics, and the local validation workflow contract as references, build the affected editor/runtime targets and run smoke validation for D3D12 and Vulkan with lit and debug/normal captures. Prefer the Launcher smoke workflow. If executing the project executable directly for automation, mirror `LaunchOperationPlan`: `RunProject`, `Showcase`, target/profile, `Projects/Showcase` working directory, smoke environment fields, logs, and artifact paths. Verify Application has no backend-native capture code and DLSS reports active provider or deterministic fallback. Record exact commands, launcher-plan fields, and artifacts.
```

Positive guardrails:

- Validate both backend selection paths.
- Capture logs and screenshots/BMPs with backend and view mode in file names.
- Treat frame graph unresolved handles as a failure.
- Treat silent DLSS fallback as a failure.
- Use launcher-shaped validation: same project working directory, target/profile, smoke environment, logs, and artifact naming that the launcher operation plan would use.

Negative guardrails:

- Do not accept "it launched" without capture/log evidence.
- Do not compare D3D12 and Vulkan only in lit mode; include normal/debug view mode.
- Do not close the milestone with a direct executable run that does not mirror the launcher launch contract.

Data transfer contracts:

- Milestone evidence transfers through smoke logs, capture artifacts, backend capability reports, and coverage-status updates.
- Each validation artifact must name backend, scene/project, view mode, frame count, feature flags, output paths, and command.
- DLSS/upscaler and capture status must be reported as structured state, not inferred from screenshots alone.
- Each editor/runtime launch must name whether it was run through Launcher, LauncherCore plan, or launcher-shaped direct execution.

Suggested validation:

```powershell
cmake --build build --config DevelopmentEditor --target SparkleLauncher -- /nologo /v:minimal /m:1
cmake --build build --config DevelopmentEditor --target ShaderCompiler -- /nologo /v:minimal /m:1
cmake --build build --config DevelopmentEditor --target clang_format_check -- /nologo /v:minimal

$env:SPARKLE_SMOKE_VALIDATE_RHI='1'
$env:SPARKLE_SMOKE_FRAME_LIMIT='120'
$env:SPARKLE_SMOKE_SCENE_COLOR_CAPTURE_FRAME='30'
$env:SPARKLE_RHI_BACKEND='D3D12'
$env:SPARKLE_SMOKE_VIEW_MODE='0'
$env:SPARKLE_SMOKE_SCENE_COLOR_CAPTURE='artifacts/validation/d3d12-lit.bmp'
# Launch Showcase editor/runtime target here through the existing launcher or executable path.

$env:SPARKLE_RHI_BACKEND='Vulkan'
$env:SPARKLE_SMOKE_VIEW_MODE='0'
$env:SPARKLE_SMOKE_SCENE_COLOR_CAPTURE='artifacts/validation/vulkan-lit.bmp'
# Launch Showcase editor/runtime target here.
```

Acceptance:

- D3D12 and Vulkan smoke runs complete.
- Captures are produced or unsupported capture is reported with a backend-specific reason.
- DLSS status is logged for both backends.
- No Application source includes D3D12/Vulkan native headers for smoke capture.
- Coverage status is updated.

## Stage 10 Evidence Packet

| Field | Evidence |
| --- | --- |
| Status | Fully completed. Editor capture, boundary, build, DLSS, Application ownership, and runtime smoke evidence pass. Stage 15 fixed and retested the runtime Vulkan present-layout blocker. |
| Build commands | `cmake --build build/windows-vs2026-stage5 --config DevelopmentEditor --target SparkleLauncher -- /nologo /v:minimal /m:1`, `ShaderCompiler`, `ShowcaseEditor`, `ShowcaseRuntime`, and `architecture_boundary_check` passed. `ShaderCompiler` initially hit a parallel Visual Studio `ZERO_CHECK.lastbuildstate` lock and passed when rerun alone. |
| Format command | `cmake --build build/windows-vs2026-stage5 --config DevelopmentEditor --target clang_format_check -- /nologo /v:minimal` could not run because this build tree does not generate `clang_format_check.vcxproj`. |
| Boundary command | `cmake -DSPARKLE_REPO_ROOT="$PWD" -P CMake/ArchitectureBoundaryCheck.cmake` passed with provider-owned counted exceptions only: `SparkleRendererNvidiaDlssProvider` Vulkan linkage and `StreamlineDlssRuntime.cpp` Vulkan identifiers. |
| Application native-header scan | `rg -n "#include <d3d12|#include <vulkan|ID3D12|D3D12_|Vk[A-Z]|vk[A-Z]|Vulkan/|D3D12/" Engine/Application/Private/Validation` returned no matches. |
| Editor smoke command shape | Launcher-shaped direct execution of the `RunProject` smoke path for `Showcase` editor. Working directory `Projects/Showcase`; executable `artifacts/dev/projects/Showcase/editor/DevelopmentEditor/ShowcaseEditor.exe`; env: `SPARKLE_SMOKE_VALIDATE_RHI=1`, `SPARKLE_SMOKE_FRAME_LIMIT=120`, `SPARKLE_SMOKE_SCENE_COLOR_CAPTURE_FRAME=30`, `SPARKLE_SMOKE_SKIP_LEVEL_SWITCHING=1`, backend `D3D12`/`Vulkan`, view mode `0`/`3`, per-run `SPARKLE_SMOKE_SCENE_COLOR_CAPTURE`, `SPARKLE_LOG_FILE`, and console capture. |
| Editor capture artifacts | `artifacts/validation/stage10/d3d12-lit.bmp`, `d3d12-normal.bmp`, `vulkan-lit.bmp`, and `vulkan-normal.bmp` exist. Each is a 32-bit BMP with `BM` signature, 21,608,214 bytes, and dimensions recorded as width 4578 and top-down height 1180. |
| Editor D3D12 smoke | Lit and GBufferNormal runs exited `0`; logs report `backend=D3D12`, `frameGraphUnresolvedBarrierWarnings=0`, `upscalerProvider='NVIDIA DLSS'`, `upscalerStatus=Active`, `failureDomain=None`, `rayTracing=true`, `inlineRayQuery=true`, and capture paths at frame 30. |
| Editor Vulkan smoke | Lit and GBufferNormal runs exited `0`; logs report `backend=Vulkan`, `frameGraphUnresolvedBarrierWarnings=0`, `upscalerProvider='NVIDIA DLSS'`, `upscalerStatus=Active`, `failureDomain=None`, `rayTracing=true`, `inlineRayQuery=true`, and capture paths at frame 30. |
| Runtime smoke command shape | Launcher-shaped direct execution of the `RunProject` smoke path for `Showcase` runtime. Working directory `Projects/Showcase`; executable `artifacts/dev/projects/Showcase/runtime/DevelopmentEditor/ShowcaseRuntime.exe`; env: `SPARKLE_SMOKE_VALIDATE_RHI=1`, `SPARKLE_SMOKE_FRAME_LIMIT=120`, `SPARKLE_SMOKE_SKIP_LEVEL_SWITCHING=1`, backend `D3D12`/`Vulkan`, per-run `SPARKLE_LOG_FILE`, and console capture. |
| Runtime D3D12 smoke | Run exited `0`; `artifacts/validation/stage10/d3d12-runtime.log` reports `frameGraphUnresolvedBarrierWarnings=0`, DLSS active, `failureDomain=None`, ray tracing enabled, and no error/critical lines. |
| Runtime Vulkan smoke | Initial Stage 10 run exited `0` and reported DLSS active with `failureDomain=None`, ray tracing enabled, and `frameGraphUnresolvedBarrierWarnings=0`, but exposed validation-layer present-layout errors. Stage 15 fixed the Vulkan transition path and `artifacts/validation/stage15/runtime-vulkan.log` reran with zero diagnostic matches. |
| Artifact index | Editor matrix: `artifacts/validation/stage10/stage10-smoke-results.json`. Runtime matrix: `artifacts/validation/stage10/stage10-runtime-results.json`. Logs and console output sit beside each BMP using backend/view-mode/runtime names. |
| Data transfer contract | Milestone evidence transfers through structured logs, BMP capture artifacts, JSON result summaries, backend capability reports, and this coverage update. DLSS state is explicit and not inferred from screenshots. |
| Threading readiness handoff | Smoke validation uses environment/request packets and immutable log/artifact outputs, which keeps future launcher/process orchestration and render-thread smoke collection separate from backend-private mutable state. |
| Remaining risk | Stage 19 still owns deeper backend presentation-service slimming and documentation; the Stage 10 runtime Vulkan validation blocker is closed by Stage 15 evidence. |

## Stage 11 - Decompose Renderer Into Facade, System Root, Frame Pipeline

Goal:

- Reduce `Renderer.cpp` from central hub to clear public facade.
- Make frame lifecycle and subsystem ownership reviewable.

Source references:

- `rhi-renderer-architecture-review.md`: `Renderer Is Too Central`, `Target Hierarchy`, `Track 3`
- `architecture-review-acceptance-rubric.md`: `Runtime behavior clarity`, `Maintainability and naming`

Target shape references:

- Stage target-doc checklist: [Required Target Documents By Stage](#required-target-documents-by-stage), Stage 11 row.
- Primary target docs: [rendering-system-map.md](../architecture/rendering-system-map.md), [repository-target-architecture.md](../architecture/after/repository-target-architecture.md), [repository-target-folder-architecture.md](../architecture/after/repository-target-folder-architecture.md), [game-framework-contract.md](../architecture/game-framework-contract.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md)

External implementation references:

- NVIDIA Donut app/render layering: https://github.com/NVIDIA-RTX/Donut
- NVIDIA Falcor renderer/application organization: https://github.com/NVIDIAGameWorks/Falcor
- Composition Root pattern: https://blog.ploeh.dk/2011/07/28/CompositionRoot/
- Facade pattern: https://refactoring.guru/design-patterns/facade

Code references:

- `Engine/Renderer/Public/Renderer.h`
- `Engine/Renderer/Private/Renderer.cpp`
- `Engine/Renderer/Private/Frame`
- `Engine/Renderer/Private/Diagnostics`
- `Engine/Renderer/Private/Upscaling`
- `Engine/Renderer/Private/RayTracing`

Tutor note:

- What is wrong today: `Renderer` is doing too many jobs, so any feature change risks touching the central hub.
- What changes: construction, frame scheduling, feature systems, and public host API become separate responsibilities.
- Why it improves the engine: a facade is good when it hides complexity, but dangerous when it owns all complexity; this stage restores that distinction.

Implementation prompt:

```text
Using Donut/Falcor application-renderer separation plus the Composition Root and Facade patterns as references, extract Renderer internals into explicit ownership objects: RendererSystemRoot for subsystem construction/lifetime, FramePipeline for begin/setup/record/submit/end frame, and feature systems for ray tracing/upscaling/meshes/textures/materials. Keep Renderer as the public host facade. Preserve behavior while moving ownership out of Renderer.cpp.
```

Positive guardrails:

- Keep public API stable until host protocol is intentionally revised in Stage 12.
- Move coherent groups, not random functions.
- Make ownership visible in constructor dependencies.
- Keep diagnostics and logging names stable or improve them deliberately.

Negative guardrails:

- Do not create a new god object with a different name.
- Do not spread frame state through global/singleton access.
- Do not expose private feature systems through `Renderer.h`.

Data transfer contracts:

- Host-facing data transfers through `Renderer` public API and documented viewport/presentation contracts only.
- Frame state transfers from `Renderer` to `FramePipeline` through explicit construction/frame context objects, not globals.
- Feature systems receive dependencies from `RendererSystemRoot` or equivalent composition root; they must not fetch cross-system state through private singleton access.

Legacy cleanup:

- Delete moved private methods from `Renderer.cpp`.
- Remove unused includes and forward declarations from `Renderer.h`.
- Update coverage status for `Renderer.cpp` root orchestration.

Acceptance:

- `Renderer.cpp` is a facade and host boundary, not the full frame scheduler.
- Frame execution can be documented as `Renderer -> FramePipeline -> FrameGraph/PassSystem`.
- Subsystem construction is centralized in a system root or equivalent composition object.

Validation:

- Defer full build to Stage 15 unless refactor creates obvious compile blockers.

Completion evidence:

| Field | Evidence |
| --- | --- |
| Status | Fully completed for facade decomposition, system-root ownership, frame-pipeline extraction, CMake source discovery, and targeted build validation. Full D3D12/Vulkan launcher-shaped smoke remains Stage 15. |
| Renderer facade | `Engine/Renderer/Private/Renderer.cpp` now delegates host API calls to `RendererSystemRoot` and `FramePipeline`. It no longer owns backend construction, scene systems, frame graph construction, resize handling, frame context build, frame graph setup/compile/execute, or frame submission. |
| System root | `Engine/Renderer/Private/Host/RendererSystemRoot.*` owns subsystem construction/lifetime for backend services, pipeline manager, mesh/texture/material/scene builders, ray tracing scene/settings, upscaler subsystem, memory monitor, and scene coordinator. Dependencies are passed at construction rather than fetched through globals. |
| Frame pipeline | `Engine/Renderer/Private/FramePipeline/FramePipeline.*` owns resize tracking, frame graph lifetime, viewport products, frame diagnostics, begin/setup/record/submit/end frame, frame context build, frame graph setup/compile/execute, RT/TLAS binding, and upscaler frame setup. |
| Public API stability | Stage 11 kept the existing host-facing API for the next migration step; Stage 12 later replaced the manual render-product helper surface with a viewport presentation protocol. Private feature systems are not exposed through `Renderer.h`; the class owns only `RendererSystemRoot` and `FramePipeline`. |
| CMake ownership | `Engine/Renderer/CMakeLists.txt` now uses `CONFIGURE_DEPENDS` for renderer public/private source globs so new source files under renderer ownership enter the generated build graph predictably. |
| Data transfer contract | Host data still enters through `Renderer` public API. Frame state moves into `FramePipeline` through explicit references to `RendererSystemRoot`, `FrameContext`, frame graph products, pass runtime services, and diagnostics. Feature systems receive dependencies from `RendererSystemRoot`; no singleton/global access path was introduced. |
| Threading readiness handoff | The extraction makes future render-thread work easier by isolating mutable lifetime ownership in `RendererSystemRoot` and per-frame graph/diagnostic state in `FramePipeline`. Frame setup, compile, record, submit, and diagnostics are now named phases. |
| Validation | `cmake -S . -B build/windows-vs2026-stage5 -G "Visual Studio 18 2026" -A x64` passed; `cmake --build build/windows-vs2026-stage5 --config DevelopmentEditor --target ShowcaseEditor -- /nologo /v:minimal /m:1` passed; `SparkleLauncher` passed; `cmake -DSPARKLE_REPO_ROOT="$PWD" -P CMake/ArchitectureBoundaryCheck.cmake` passed with only provider-owned counted exceptions. Source text hygiene scan over changed renderer source found no planning/stage/provenance text. |
| Validation notes | `clang-format` is not discoverable in this shell, so formatting target validation remains unavailable. Configure still emits existing third-party developer warnings from Assimp custom commands and FetchContent deprecation in RHI dependencies. `SparkleLauncher` Qt deploy still warns that `VCINSTALLDIR` is not set after producing the executable. |
| Remaining risk | Stage 15 owns launcher-shaped D3D12/Vulkan smoke after frame facade/presentation work. |

## Stage 12 - Add Viewport Presentation Bridge And Clean Host Protocol

Goal:

- Remove ad hoc editor/application manipulation of renderer products and transitions.
- Make Application -> Renderer edge a stable host protocol.

Source references:

- `rhi-renderer-architecture-review.md`: `Application -> Renderer`, `Renderer Public Coverage`, `Target Hierarchy`
- `architecture-review-acceptance-rubric.md`: `Communication/reviewability`, `Separation of concerns`

Target shape references:

- Stage target-doc checklist: [Required Target Documents By Stage](#required-target-documents-by-stage), Stage 12 row.
- Primary target docs: [rendering-system-map.md](../architecture/rendering-system-map.md), [rhi-contract-map.md](../architecture/rhi-contract-map.md), [game-framework-contract.md](../architecture/game-framework-contract.md), [repository-target-architecture.md](../architecture/after/repository-target-architecture.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md)

External implementation references:

- NVIDIA Donut app/render split: https://github.com/NVIDIA-RTX/Donut
- NVIDIA NVRHI abstraction model: https://github.com/NVIDIA-RTX/NVRHI
- Facade pattern: https://refactoring.guru/design-patterns/facade

Code references:

- `Engine/Renderer/Public/Viewport/ViewportContracts.h`
- `Engine/Renderer/Public/Renderer.h`
- `Renderer::BeginViewportPresentation`
- `Renderer::EndViewportPresentation`
- `Renderer::CaptureViewportProductToBmp`
- `Engine/Application/Private`
- Editor UI code that consumes viewport products

Tutor note:

- What is wrong today: Application/editor paths can manually resolve render products and drive transitions, which exposes renderer internals.
- What changes: a presentation bridge owns viewport products, ImGui texture IDs, and state transitions.
- Why it improves the engine: host code should request presentation, not know the frame graph resource state machine.

Implementation prompt:

```text
Using Donut's app/render split, NVRHI's abstraction boundary, and the Facade pattern as references, introduce a viewport/presentation bridge that owns render product publication, texture ID resolution, and required state transitions for editor/runtime presentation. Replace public Renderer methods that expose manual render product transitions with a host-facing protocol. Remove old public transition helpers once all callers migrate.
```

Positive guardrails:

- Application asks for a viewport product; Renderer/presentation bridge handles resource state details.
- ImGui texture ID resolution stays behind an RHI UI/presentation bridge.
- The host protocol names lifecycle operations clearly.

Negative guardrails:

- Do not make Application call frame graph resource APIs.
- Do not expose `NativeResourceHandle` to editor UI unless a capture/test service explicitly needs it.
- Do not duplicate transition logic between Application and Renderer.

Data transfer contracts:

- Application/Editor receive viewport products through public presentation DTOs or handles, not frame graph internals.
- ImGui texture IDs transfer through the RHI UI/presentation bridge.
- Render product state transitions are owned by Renderer/presentation bridge and reported through diagnostics when presentation fails.

Legacy cleanup:

- Remove public `TransitionRenderProduct` after migration.
- Remove public native resource resolution if only smoke/capture needed it; route capture through RHI service.
- Update docs and call sites.

Acceptance:

- Application/editor no longer performs manual frame graph product transitions.
- `Renderer.h` reads as host protocol plus diagnostics, not internal access.
- Viewport presentation contract is documented.

Validation:

- Defer full build to Stage 15.

Completion evidence:

| Field | Evidence |
| --- | --- |
| Status | Fully completed for host protocol cleanup and targeted build validation. |
| Target docs opened | [rendering-system-map.md](../architecture/rendering-system-map.md), [rhi-contract-map.md](../architecture/rhi-contract-map.md), [game-framework-contract.md](../architecture/game-framework-contract.md), [repository-target-architecture.md](../architecture/after/repository-target-architecture.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md), [validation-workflow-contract.md](../architecture/validation-workflow-contract.md). |
| Contract surfaces touched | RenderContracts-style viewport DTOs in [ViewportContracts.h](../../Engine/Renderer/Public/Viewport/ViewportContracts.h), Renderer host protocol in [Renderer.h](../../Engine/Renderer/Public/Renderer.h), RHI presentation/capture service usage behind `FramePipeline`, mechanical guardrails through `architecture_boundary_check`, and threading-readiness handoff through explicit presentation lifecycle/capture request packets. |
| Ownership/disposition | Improve and extract `Renderer` host protocol: retain lifecycle/diagnostics/RHI access, remove public manual render-product texture/resource/transition helpers, and make `FramePipeline` the private owner of frame graph product resolution and transitions. |
| Folder and target plan | Current owner folders remain `Engine/Renderer/Public/Viewport`, `Engine/Renderer/Public`, and `Engine/Renderer/Private/FramePipeline`; no code was moved into Application, Editor, RHI private, backend private, or tool folders. No CMake target split was required. |
| Data transfer plan | Application/Editor submit `ViewportRenderRequest`, receive `ViewportPresentationProduct`, and send `ViewportCaptureRequest` for smoke capture. ImGui texture IDs are resolved inside Renderer through `RhiPresentationService`; native resource handles remain private to FramePipeline/RHI capture service calls. |
| Threading-readiness plan | Presentation now has explicit begin/end lifecycle and capture request packets named by output, frame, view mode, and artifact path. Future render-thread or queued capture work can consume these packets without reading Application or frame graph private mutable state. |
| Source text discipline | Touched source was scanned for planning/stage/provenance text; no matches were found. New runtime strings are failure diagnostics for presentation/capture behavior. |
| Destination-fit proof | Texture ID resolution, native resource lookup, and render-product transitions moved behind `FramePipeline` because it owns frame graph lifetime and tracked resource state. Application remains host orchestration and no new catch-all helper or displaced-clutter owner was added. |
| Acceptance evidence | [Renderer.h](../../Engine/Renderer/Public/Renderer.h) exposes `BeginViewportPresentation`, `EndViewportPresentation`, and `CaptureViewportProductToBmp`; old public `ResolveRenderProductTextureId`, `ResolveRenderProductResource`, and `TransitionRenderProduct` helpers are gone; [EditorApplication.cpp](../../Engine/Application/Private/EditorApplication.cpp) and [RhiSmokeEditorValidation.cpp](../../Engine/Application/Private/Validation/RhiSmokeEditorValidation.cpp) use the viewport presentation protocol. |
| Validation evidence | `ShowcaseEditor` and `SparkleLauncher` built with `DevelopmentEditor` in `build/windows-vs2026-stage5`; `architecture_boundary_check` passed. `clang_format_check` could not run because the VS2026 build tree has no generated target. Stage 15 later closed launcher-shaped D3D12/Vulkan smoke for this host protocol path. |

## Stage 13 - Clean Scene Data, Mesh, Texture, Temporal Ownership

Goal:

- Make data flow from GameFramework to Renderer explicit and immutable.
- Stabilize supporting systems before deeper frame graph/pass work.

Source references:

- `rhi-renderer-architecture-review.md`: `Renderer -> GameFramework`, `Renderer Private Coverage`
- `architecture-review-acceptance-rubric.md`: `Modern C++ systems skill`, `Rendering fundamentals`

Target shape references:

- Stage target-doc checklist: [Required Target Documents By Stage](#required-target-documents-by-stage), Stage 13 row.
- Primary target docs: [game-framework-contract.md](../architecture/game-framework-contract.md), [rendering-system-map.md](../architecture/rendering-system-map.md), [tooling-pipeline-contract.md](../architecture/tooling-pipeline-contract.md), [repository-target-folder-architecture.md](../architecture/after/repository-target-folder-architecture.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md)

External implementation references:

- NVIDIA Donut scene/application organization: https://github.com/NVIDIA-RTX/Donut
- NVIDIA Falcor scene/rendering separation: https://github.com/NVIDIAGameWorks/Falcor

Code references:

- `Engine/Renderer/Private/SceneData`
- `Engine/Renderer/Private/Meshes`
- `Engine/Renderer/Private/Textures`
- `Engine/Renderer/Private/Temporal`
- `Engine/GameFramework`

Tutor note:

- What is wrong today: renderer scene input can become coupled to gameplay structures if we keep adding direct access.
- What changes: Renderer consumes render-domain snapshots and DTOs for things it needs to draw.
- Why it improves the engine: render snapshots make the renderer easier to test, cache, parallelize, and reason about independently from gameplay.

Implementation prompt:

```text
Using Donut's scene/application organization and Falcor's scene/rendering separation as references, review and tighten the render snapshot boundary. Ensure Renderer consumes render-domain DTOs for meshes, materials, cameras, lights, skinning, and temporal state. Document lifetime and ownership for mesh cache, material cache, texture manager, and temporal jitter. Remove direct gameplay internals from renderer paths where a render snapshot can carry the data.
```

Positive guardrails:

- Preserve current rendering behavior.
- Keep DTOs simple and immutable per frame.
- Tie mesh/texture/material diagnostics to smoke evidence.
- Document temporal jitter conventions because DLSS and debug modes depend on them.

Negative guardrails:

- Do not introduce broad data-copy churn without reason.
- Do not put RHI objects into GameFramework.
- Do not let material/texture loading become a source-import/cooking concern inside Renderer.

Data transfer contracts:

- GameFramework transfers scene state to Renderer through immutable render-domain snapshots/DTOs.
- Renderer transfers GPU-adjacent requests to RHI through public RHI descriptors and resource/upload contracts.
- Texture/material/mesh data enters Renderer as cooked/runtime records and render DTOs, not source import structures or tool-private types.

Legacy cleanup:

- Remove unused snapshot adapters or duplicated DTO paths after migration.
- Remove empty/private placeholder folders if they are not planned, especially `Renderer/Private/Denoising`, or document their exact purpose.

Acceptance:

- Scene data contract doc names every render-domain DTO and owner.
- Renderer no longer depends on gameplay internals where a snapshot should be used.
- Mesh, material, texture, and temporal diagnostics are represented in smoke/reporting plan.

Validation:

- Defer full build to Stage 15.

Completion evidence:

| Field | Evidence |
| --- | --- |
| Status | Fully completed for snapshot-boundary tightening, ownership documentation, and targeted build validation. |
| Target docs opened | [game-framework-contract.md](../architecture/game-framework-contract.md), [rendering-system-map.md](../architecture/rendering-system-map.md), [tooling-pipeline-contract.md](../architecture/tooling-pipeline-contract.md), [repository-target-folder-architecture.md](../architecture/after/repository-target-folder-architecture.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md), [render-scene-data-contract.md](../architecture/render-scene-data-contract.md). |
| Contract surfaces touched | RenderContracts-style scene handoff, GameFramework runtime snapshot contract, renderer scene/resource staging, mesh/texture/material/temporal diagnostics plan, folder architecture cleanup, and threading-readiness handoff shape. |
| Ownership/disposition | Improve and extract renderer scene handoff. `RenderSceneSnapshot` is now a renderer-owned frame snapshot with explicit fields instead of inheriting the full GameFramework snapshot. Empty private denoising placeholder folder was removed. |
| Folder and target plan | Current folders touched: `Engine/Renderer/Private/SceneData/Lifecycle`, `Engine/Renderer/Private/Denoising`, and architecture docs. Target owner remains Renderer scene data; no code moved into GameFramework, RHI private, tools, or Application. No CMake target split was required. |
| Data transfer plan | GameFramework produces `GameSceneSnapshot`; Renderer captures it into `RenderSceneSnapshot`; scene builders consume camera, lighting, material, texture, mesh, animation/skinning, and temporal frame data from renderer-owned frame state. GPU work still transfers to RHI through public RHI descriptors/resources. |
| Threading-readiness plan | Scene mutation stays in GameFramework. Renderer frame setup consumes an explicit snapshot copy. Temporal state remains renderer-owned. Future worker/cook/render-thread work can use the documented producer, frame snapshot, cache owner, diagnostics, and Stage 24 schema extraction plan. |
| Source text discipline | No explanatory/provenance/planning comments were added to source. Existing source comments outside this stage were not expanded. |
| Destination-fit proof | Snapshot ownership was reshaped instead of moved: `RenderSceneSnapshot` now names the exact renderer frame fields it owns. The empty private denoising folder was deleted because the actual integration point is the public shadow denoise contract plus frame graph resource registration. |
| Acceptance evidence | [render-scene-data-contract.md](../architecture/render-scene-data-contract.md) names every current render-domain DTO owner and consumer; [RenderSceneSnapshot.h](../../Engine/Renderer/Private/SceneData/Lifecycle/RenderSceneSnapshot.h) no longer derives from `GameSceneSnapshot`; [rendering-coverage-status.md](../architecture/rendering-coverage-status.md) marks SceneData, Temporal, Textures, public mesh/resource diagnostics, and denoising decisions with Stage 13 evidence. |
| Remaining pressure | Mesh snapshots still carry runtime `Mesh*` references because morph/dirty geometry and GPU cache keys depend on runtime mesh identity. This is documented as a Stage 24 `RenderContracts`/`AssetContracts` extraction pressure rather than hidden inside Stage 13. |
| Validation evidence | `ShowcaseEditor` build and boundary/source-hygiene checks were run after implementation. Full D3D12/Vulkan smoke remains Stage 15. |

## Stage 14 - Harden Frame Graph Contract And Diagnostics

Goal:

- Make frame graph resource/barrier failures impossible to ignore.
- Preserve the strong frame graph architecture while making contracts explicit.

Source references:

- `rhi-renderer-architecture-review.md`: `Frame Orchestration And Pass Implementation Are Still Blurry`, `Phase 3: Frame Graph Contract Review`
- `architecture-review-acceptance-rubric.md`: `Runtime behavior clarity`, `Observability and diagnostics`, `Reliability/failure handling`

Target shape references:

- Stage target-doc checklist: [Required Target Documents By Stage](#required-target-documents-by-stage), Stage 14 row.
- Primary target docs: [frame-graph-contract.md](../architecture/frame-graph-contract.md), [rendering-system-map.md](../architecture/rendering-system-map.md), [pass-authoring-contract.md](../architecture/pass-authoring-contract.md), [pipeline-runtime-contract.md](../architecture/pipeline-runtime-contract.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md)

External implementation references:

- NVIDIA Falcor RenderGraph system: https://github.com/NVIDIAGameWorks/Falcor
- NVIDIA Donut reusable passes and graph-style orchestration: https://github.com/NVIDIA-RTX/Donut
- Vulkan validation tooling philosophy: https://docs.vulkan.org/guide/latest/development_tools.html

Code references:

- `Engine/Renderer/Private/FrameGraph`
- `Engine/Renderer/Private/FrameGraph/Execution/FrameGraphBarrierPlanPlayback.cpp`
- `Engine/Renderer/Private/FrameGraph/Diagnostics`
- `Engine/Renderer/Private/Frame`
- `Engine/Renderer/Private/Passes`

Tutor note:

- What is wrong today: frame graph unresolved resources can show up as warnings instead of hard contract failures.
- What changes: graph declaration, compile, transient planning, barrier playback, and execution get explicit contracts and diagnostics.
- Why it improves the engine: a frame graph is valuable only if it makes dependencies more reliable, not if it hides broken resources behind warnings.

Implementation prompt:

```text
Using Falcor RenderGraph, Donut pass orchestration, and Vulkan validation tooling as references, turn frame graph warnings for unresolved resources, unresolved aliasing barriers, invalid external resources, and incompatible resource usages into explicit development validation failures with diagnostic dumps. Document declare/compile/plan/allocate/execute contracts and ensure pass composition files only wire graph resources while pass implementation files own execution details.
```

Positive guardrails:

- Every warning class gets a contract explanation and remediation hint.
- Diagnostics must include pass name, resource handle, declared usage, resolved state, and physical allocation when relevant.
- Keep frame graph internals private.
- Make `Frame/*` orchestration naming consistent.

Negative guardrails:

- Do not suppress warnings to make smoke pass.
- Do not move pass-specific shader behavior into frame graph.
- Do not let transient aliasing become opaque again.

Data transfer contracts:

- Pass/resource intent transfers into FrameGraph through typed pass declarations and resource usage descriptors.
- FrameGraph transfers execution plans to RHI through barrier plans, resolved resources/views, and command recording contexts.
- Diagnostics transfer pass name, resource handle, declared usage, resolved state, physical allocation, and remediation hint.

Legacy cleanup:

- Remove fallback paths that silently skip unresolved barriers/resources.
- Remove stale diagnostic code that cannot be triggered or tested.

Acceptance:

- Unresolved frame graph resources fail development smoke.
- Transient aliasing diagnostics explain physical block reuse.
- Frame composition versus pass execution ownership is documented and visible in names.

Validation:

- Defer full build/runtime smoke to Stage 15.

Completion evidence:

| Field | Evidence |
| --- | --- |
| Status | Fully completed for frame graph contract hardening and targeted validation. Full D3D12/Vulkan smoke remains Stage 15. |
| Target docs opened | [frame-graph-contract.md](../architecture/frame-graph-contract.md), [rendering-system-map.md](../architecture/rendering-system-map.md), [pass-authoring-contract.md](../architecture/pass-authoring-contract.md), [pipeline-runtime-contract.md](../architecture/pipeline-runtime-contract.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md), [rendering-coverage-status.md](../architecture/rendering-coverage-status.md). |
| Contract surfaces touched | Renderer frame graph resource contracts, barrier playback, transient aliasing diagnostics, external resource import validation, acceleration-structure graph binding, pass resource usage validation, smoke-visible unresolved-barrier evidence, and threading-readiness handoff shape. |
| Ownership/disposition | Keep and refine the existing frame graph folder split. Improve and harden `FrameGraph/Resources`, `FrameGraph/Diagnostics`, and `FrameGraph/Execution` instead of moving pass policy into graph code or adding public graph API. |
| Complexity right to exist | The remaining validation code earns its complexity by stopping invalid graph state at the boundary where enough context exists to name pass/resource/state/allocation/remediation. Fallback paths that skipped unresolved barriers were removed. |
| Folder and target plan | Current folders touched: `Engine/Renderer/Private/FrameGraph/Resources`, `Diagnostics`, and `Execution`. Target owner remains Renderer frame graph. No code was moved into RHI, passes, Application, GameFramework, or tools. No CMake target split was required. |
| Data transfer plan | Pass/resource intent transfers into FrameGraph through typed declarations and usage descriptors; compiled plans transfer barrier/resource/view data to RHI command recording; diagnostics transfer pass name, resource handle/name, label/usage-derived state, physical block where relevant, and remediation hint. |
| Threading-readiness plan | Frame graph execution remains serial, but the frozen plan cannot silently skip unresolved physical resources. Future worker command batches can treat graph validation failures as deterministic pre-recording failures instead of hidden runtime drift. |
| Source text discipline | No explanatory/provenance/planning comments were added to source. Runtime strings added in this stage are diagnostic failure text only. |
| Destination-fit proof | No relocation was needed. Existing files were hardened in place because `Resources` owns imports/bindings, `Diagnostics` owns declaration validation, and `Execution` owns barrier playback. |
| Acceptance evidence | Invalid external texture/buffer imports, incompatible imported UAV usage, invalid AS imports/bindings, invalid pass resource usages, unresolved compiled barriers, and unresolved aliasing barriers now call `Diagnostics::Fail` with actionable diagnostic payloads. [frame-graph-contract.md](../architecture/frame-graph-contract.md) documents every validation class and remediation path. |
| Validation evidence | `ShowcaseEditor` built with `DevelopmentEditor` in `build/windows-vs2026-stage5`; `architecture_boundary_check` passed; source text hygiene scan over touched source found no planning/stage/provenance text. |
| Remaining risk | Closed by Stage 15 launcher-shaped D3D12/Vulkan editor/runtime smoke; later stages must preserve zero unresolved frame graph counters and zero validation-layer errors. |

## Stage 15 - Validation Milestone C: Renderer Facade, Frame Pipeline, Frame Graph

Goal:

- Validate the renderer decomposition and frame graph contract before changing pass/PSO architecture.

Target shape references:

- Stage target-doc checklist: [Required Target Documents By Stage](#required-target-documents-by-stage), Stage 15 row.
- Primary target docs: [frame-graph-contract.md](../architecture/frame-graph-contract.md), [rendering-coverage-status.md](../architecture/rendering-coverage-status.md), [architecture-boundary-guardrails.md](../architecture/architecture-boundary-guardrails.md), [validation-workflow-contract.md](../architecture/validation-workflow-contract.md), [repository-target-graphs.md](../architecture/after/repository-target-graphs.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md)

External implementation references:

- NVIDIA NVRHI tutorial: https://github.com/NVIDIA-RTX/NVRHI/blob/main/doc/Tutorial.md
- NVIDIA Donut Samples: https://github.com/NVIDIA-RTX/Donut-Samples
- Khronos Vulkan validation layers: https://github.com/KhronosGroup/Vulkan-ValidationLayers

Tutor note:

- What is wrong today: if renderer decomposition or graph validation breaks presentation, later pass/PSO work will be blamed incorrectly.
- What changes: this milestone proves the host facade, presentation bridge, frame pipeline, and frame graph are stable before deeper pipeline work.
- Why it improves the engine: validation milestones isolate risk so each large refactor has a trustworthy baseline.

Implementation prompt:

```text
Using NVRHI tutorial validation, Donut Samples, Vulkan validation layers, and the local validation workflow contract as references, build the editor/runtime targets and run D3D12/Vulkan smoke validation with lit and debug/normal view modes. Prefer the Launcher smoke workflow; if using direct execution, mirror `LaunchOperationPlan` exactly for target/profile, `Projects/Showcase` working directory, smoke environment, logs, and artifacts. Confirm frame graph diagnostics are clean, render products present correctly, and Application no longer owns renderer resource transitions.
```

Positive guardrails:

- Validate window resize/restore/maximize if smoke supports it.
- Validate shader reload if the current milestone touched pipeline/runtime ownership.
- Capture frame graph diagnostic output.
- Record launcher-plan fields or launcher-shaped direct execution fields for every editor/runtime smoke run.

Negative guardrails:

- Do not move into PSO/pass runtime redesign with unresolved frame graph warnings.
- Do not accept editor viewport presentation if it only works through legacy transition helpers.
- Do not accept a manually launched executable if it used a different working directory, project, target profile, smoke environment, or artifact path than the launcher workflow.

Data transfer contracts:

- Validation transfers renderer facade, presentation bridge, frame graph, and shader reload evidence through logs and coverage-status updates.
- Smoke evidence must name backend, view mode, render product path, frame graph warning count, and whether legacy transition helpers were used.
- If validation fails, record the failing contract owner: host protocol, presentation bridge, frame graph, RHI, or shader runtime.
- Launch evidence must name whether the run used Launcher, LauncherCore plan, or launcher-shaped direct execution.

Suggested validation:

```powershell
cmake --build build --config DevelopmentEditor --target SparkleLauncher -- /nologo /v:minimal /m:1
cmake --build build --config DevelopmentEditor --target clang_format_check -- /nologo /v:minimal

$env:SPARKLE_SMOKE_VALIDATE_RHI='1'
$env:SPARKLE_SMOKE_FRAME_LIMIT='120'
$env:SPARKLE_SMOKE_SHADER_RELOAD_FRAME='60'
$env:SPARKLE_SMOKE_SCENE_COLOR_CAPTURE_FRAME='30'
$env:SPARKLE_RHI_BACKEND='D3D12'
$env:SPARKLE_SMOKE_VIEW_MODE='0'
$env:SPARKLE_SMOKE_SCENE_COLOR_CAPTURE='artifacts/validation/d3d12-stage15-lit.bmp'
# Launch editor/runtime smoke.

$env:SPARKLE_RHI_BACKEND='Vulkan'
$env:SPARKLE_SMOKE_VIEW_MODE='0'
$env:SPARKLE_SMOKE_SCENE_COLOR_CAPTURE='artifacts/validation/vulkan-stage15-lit.bmp'
# Launch editor/runtime smoke.
```

Acceptance:

- D3D12 and Vulkan smoke pass with no frame graph unresolved-resource warnings.
- Renderer public host API no longer exposes manual product transitions.
- Coverage status is updated.

Completion evidence:

| Field | Evidence |
| --- | --- |
| Status | Fully completed. |
| Code fix | `VulkanRenderCommandList::TransitionResource` now treats allocator-untracked external present images as discardable when transitioning from present into a writable Vulkan layout, matching acquired swapchain-image reality after creation/resize without changing Renderer or Application ownership. |
| Build evidence | `SparkleLauncher`, `ShowcaseEditor`, and `ShowcaseRuntime` built with `DevelopmentEditor` in `build/windows-vs2026-stage5`. `ShowcaseRuntime` initially hit a parallel Visual Studio `.tlog` lock while `ShowcaseEditor` was building and passed when rerun alone. |
| Boundary evidence | `cmake -DSPARKLE_REPO_ROOT="$PWD" -P CMake/ArchitectureBoundaryCheck.cmake` passed with only provider-owned counted exceptions. |
| Launch basis | Launcher-shaped direct execution of `RunProject` / `project.run.smoke` for `Showcase`, `DevelopmentEditor`, working directory `Projects/Showcase`, with `SPARKLE_SMOKE_VALIDATE_RHI=1`, `SPARKLE_SMOKE_FRAME_LIMIT=120`, `SPARKLE_SMOKE_SHADER_RELOAD_FRAME=60`, `SPARKLE_SMOKE_SKIP_LEVEL_SWITCHING=1`, `SPARKLE_STARTUP_LEVEL=Sponza`, backend-specific `SPARKLE_RHI_BACKEND`, per-run `SPARKLE_LOG_FILE`, and editor capture fields. |
| Editor smoke matrix | D3D12 Lit, D3D12 GBufferNormal, Vulkan Lit, and Vulkan GBufferNormal exited `0`, produced captures in `artifacts/validation/stage15`, and reported zero diagnostic matches. Each BMP was 21,608,214 bytes. |
| Runtime smoke matrix | D3D12 and Vulkan runtime smoke exited `0` and reported zero diagnostic matches. The previous Vulkan `VK_IMAGE_LAYOUT_PRESENT_SRC_KHR` versus `VK_IMAGE_LAYOUT_UNDEFINED` validation-layer error did not recur. |
| Artifact index | `artifacts/validation/stage15/stage15-smoke-results.json` records executable path, working directory, backend, view mode, frame limit, shader reload frame, capture path, logs, exit code, timeout state, and diagnostic matches for all six runs. |
| Format evidence | `clang_format_check` is still not generated in this VS2026 build tree; the attempted target build failed with missing `clang_format_check.vcxproj`. |
| Source text discipline | No explanatory/provenance/planning comments were added to source. A source-text scan over the touched Vulkan file found no stage/provenance planning text. |
| Acceptance evidence | D3D12/Vulkan editor and runtime smoke passed with no frame graph unresolved-resource warnings, no validation-layer errors, no hard graph contract failures, and no legacy public render-product transition helpers exposed by `Renderer.h`. |

## Stage 16 - Introduce Explicit PSO Key And Pipeline Runtime Library

Status: fully completed for explicit key/library ownership; Stage 17 removes the remaining pass-trait/type-index compatibility path.

Goal:

- Replace implicit pass-type runtime identity with an explicit PSO/runtime key model.
- Separate shader package loading, binding layout creation, validation, and PSO creation.

Source references:

- `rhi-renderer-architecture-review.md`: `Shader Pass And PSO Handling`, `Track 5`
- `architecture-review-acceptance-rubric.md`: `Shader and pipeline systems`, `Performance reasoning`, `Portability/backend parity`

Target shape references:

- Stage target-doc checklist: [Required Target Documents By Stage](#required-target-documents-by-stage), Stage 16 row.
- Primary target docs: [pipeline-runtime-contract.md](../architecture/pipeline-runtime-contract.md), [pass-authoring-contract.md](../architecture/pass-authoring-contract.md), [rhi-contract-map.md](../architecture/rhi-contract-map.md), [tooling-pipeline-contract.md](../architecture/tooling-pipeline-contract.md), [repository-target-folder-architecture.md](../architecture/after/repository-target-folder-architecture.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md)

External implementation references:

- Microsoft D3D12 pipelines and shaders docs: https://learn.microsoft.com/en-us/windows/win32/direct3d12/pipelines-and-shaders-with-directx-12
- Microsoft D3D12 pipeline state management docs: https://learn.microsoft.com/en-us/windows/win32/direct3d12/managing-graphics-pipeline-state-in-direct3d-12
- Diligent Core pipeline state object model: https://github.com/DiligentGraphics/DiligentCore

Code references:

- `Engine/Renderer/Private/Pipeline/PipelineStateManager.h`
- `Engine/Renderer/Private/Passes/RenderPassDefinition.h`
- `Engine/Renderer/Private/Pipeline/RenderPassDefinitionRuntime.h`
- `Engine/Renderer/Private/Pipeline/RenderPassShaderRuntime.h`
- `Engine/RHI/Public/Pipeline/RhiPipelineStateDesc.h`
- `Engine/RHI/Private/D3D12/Pipeline`
- `Engine/RHI/Private/Vulkan/Pipeline`

Tutor note:

- What is wrong today: runtime identity is centered on pass C++ type and central traits, not a complete backend-visible PSO key.
- What changes: PSO identity becomes explicit, printable, and based on shader package, render state, formats, features, and backend.
- Why it improves the engine: PSO systems are easier to debug and cache when the key describes the actual GPU pipeline, not the C++ class that asked for it.

Implementation prompt:

```text
Using Microsoft D3D12 PSO documentation and Diligent Core's pipeline-state model as references, introduce a PipelineRuntimeLibrary with explicit PipelineKey/PsoKey structures. The key must include backend, shader package ID/generation/hash, pipeline kind, shader stages, render target formats, depth format, raster/depth/blend state, vertex layout, required feature/permutation bits, and binding layout identity. Separate shader package loading from PSO creation. Keep the old pass runtime path only until all passes are migrated, then delete it.
```

Positive guardrails:

- PSO keys are printable in logs.
- D3D12 and Vulkan receive equivalent normalized descriptors.
- Shader reload invalidates by package generation/hash where possible.
- Runtime errors name pass, package, backend, key fields, and suggested fix.

Negative guardrails:

- Do not use `std::type_index` as the final runtime key.
- Do not keep central trait specialization as the long-term ordinary-pass registration mechanism.
- Do not hide PSO variants in ad hoc lambdas.

Data transfer contracts:

- Pass/runtime identity transfers through explicit `PipelineKey`/`PsoKey` data, not C++ type identity.
- Shader package data transfers from cooked package/reflection into binding layout and PSO descriptors through typed runtime library inputs.
- Backend PSO creation receives normalized RHI pipeline descriptors; D3D12/Vulkan code must not infer renderer pass policy from private pass types.

Legacy cleanup:

- Mark `RenderPassPipelineTraits` and old lazy runtime storage as legacy at stage start.
- Delete them after Stage 17 migration.

Acceptance:

- New `PipelineRuntimeLibrary` exists.
- PSO key is explicit and logged.
- Runtime package loading and PSO creation are separate responsibilities.
- D3D12/Vulkan pipeline creation paths consume normalized descriptors.

Validation:

- `ShowcaseEditor` built with `DevelopmentEditor` in `build/windows-vs2026-stage5`.
- `architecture_boundary_check` passed with only documented provider-owned DLSS exceptions.
- `clang_format_check` could not run because the VS2026 build tree does not generate `clang_format_check.vcxproj`.
- Full D3D12/Vulkan smoke and runtime log capture remain assigned to Stage 20.

## Stage 17 - Introduce Declarative Pass Definition And Migrate Passes

Status: fully completed for ordinary renderer passes; Stage 20 captures runtime smoke/PSO log evidence.

Goal:

- Make adding a shader pass high-level and Renderer-owned.
- Remove central pass traits and duplicate shader package declarations.

Source references:

- `rhi-renderer-architecture-review.md`: `Target Shader Pass Model`, `Hard Gate: Renderer Shader Passes Must Not Require RHI Edits`
- `architecture-review-acceptance-rubric.md`: `Role relevance`, `Shader and pipeline systems`, `Maintainability and naming`

Target shape references:

- Stage target-doc checklist: [Required Target Documents By Stage](#required-target-documents-by-stage), Stage 17 row.
- Primary target docs: [pass-authoring-contract.md](../architecture/pass-authoring-contract.md), [pipeline-runtime-contract.md](../architecture/pipeline-runtime-contract.md), [tooling-pipeline-contract.md](../architecture/tooling-pipeline-contract.md), [repository-target-folder-architecture.md](../architecture/after/repository-target-folder-architecture.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md)

External implementation references:

- NVIDIA Falcor RenderPasses model: https://github.com/NVIDIAGameWorks/Falcor
- NVIDIA Donut reusable passes above NVRHI: https://github.com/NVIDIA-RTX/Donut
- Diligent Engine render state and pipeline abstractions: https://github.com/DiligentGraphics/DiligentEngine

Code references:

- `Engine/Renderer/Private/Passes`
- `Engine/Renderer/Private/Frame`
- `Engine/Renderer/Private/Pipeline`
- `Engine/Renderer/Public/ShaderParameters`
- `Engine/RHI/Public/Shaders`
- `Tools/Shaders/ShaderCompiler`

Tutor note:

- What is wrong today: adding a pass means touching too many central systems and understanding too much low-level ceremony.
- What changes: a pass definition becomes the high-level source of pass intent, resources, shader package, render state, and dispatch/draw behavior.
- Why it improves the engine: a regular shader pass is a renderer feature; the engine should make the common path simple while keeping the deep systems powerful.

Implementation prompt:

```text
Using Falcor RenderPasses, Donut reusable passes, and Diligent render-state abstractions as references, add a Renderer-owned RenderPassDefinition model that describes pass name, shader package, pipeline kind, render state, resources, dispatch/draw behavior, feature requirements, and binding behavior. Migrate one simple proof pass first, preferably VisualizeBuffers or ComputeClear. Then migrate all ordinary passes. Delete the old central RenderPassPipelineTraits path after migration.
```

Positive guardrails:

- Pass authoring should require one pass definition file and shader files for ordinary passes.
- Pass definitions should feed frame graph declaration and pipeline runtime lookup.
- Binding should be reflection-driven or generated where possible.
- Resource declarations should be close to pass intent.

Negative guardrails:

- Do not require RHI edits for ordinary renderer passes.
- Do not keep both old and new pass systems indefinitely.
- Do not make pass definitions backend-specific.
- Do not sacrifice diagnostics to reduce code.

Data transfer contracts:

- Pass intent transfers through `RenderPassDefinition`: pass name, resources, shader package, render state, feature requirements, dispatch/draw behavior, and binding behavior.
- Shader metadata transfers from renderer registration/cooked reflection into FrameGraph declaration and PipelineRuntime lookup.
- Diagnostics must carry pass name, shader package, binding/resource name, expected type, actual type, backend, and suggested fix.

Legacy cleanup:

- Delete `RenderPassPipelineTraits.h` once all passes migrate.
- Delete old duplicate `DescribeShaderPackage` paths if the pass definition becomes the source of truth.
- Remove dead binding override paths that no migrated pass uses.

Acceptance:

- All existing ordinary passes are migrated or explicitly classified as special cases.
- Adding a new simple compute pass requires no RHI edit and no central trait edit.
- Shader package ID and binding layout ID have one source of truth.
- Pass validation errors are actionable.

Validation:

- `ShowcaseEditor` built with `DevelopmentEditor` in `build/windows-vs2026-stage5`.
- `ShaderCompiler` built with `DevelopmentEditor` in `build/windows-vs2026-stage5`.
- `architecture_boundary_check` passed with only documented provider-owned DLSS exceptions.
- `ShaderCompiler.exe list-shaders --validate` reported 17 valid typed registrations.
- Full D3D12/Vulkan runtime smoke remains assigned to Stage 20.

## Stage 17A - Remove Shader Registration Boilerplate With Manifest-Driven Authoring

Status: fully completed for renderer shader registration constant removal. Renderer shader classes now keep parameter metadata and real feature flags while package/path/entry/stage metadata is authored once through source metadata registration. Stage 17B owns the remaining whole-pass authoring workflow and central registration friction.

Goal:

- Remove repetitive per-shader registration boilerplate such as duplicated `kShaderName`, `kShaderPackageName`, `kBindingLayoutId`, and `IMPLEMENT_GLOBAL_SHADER` inputs.
- Make shader package/stage/path/entry/binding metadata authored once and consumed by both runtime pass definitions and ShaderCompiler.
- Keep pass authoring high-signal: ordinary pass code should describe render intent, not repeat registration plumbing.

Source references:

- `rhi-renderer-architecture-review.md`: `Target Shader Pass Model`, `Shader Pass And PSO Handling`
- `architecture-review-acceptance-rubric.md`: `Shader and pipeline systems`, `Maintainability and naming`, `Documentation and onboarding`, `Testability`

Target shape references:

- Stage target-doc checklist: [Required Target Documents By Stage](#required-target-documents-by-stage), Stage 17A row.
- Primary target docs: [pass-authoring-contract.md](../architecture/pass-authoring-contract.md), [pipeline-runtime-contract.md](../architecture/pipeline-runtime-contract.md), [tooling-pipeline-contract.md](../architecture/tooling-pipeline-contract.md), [repository-target-folder-architecture.md](../architecture/after/repository-target-folder-architecture.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md)

External implementation references:

- NVIDIA Falcor render pass workflow and render-pass tooling: https://github.com/NVIDIAGameWorks/Falcor/blob/master/docs/development/cmake.md
- NVIDIA Donut reusable passes and shader build separation: https://github.com/NVIDIA-RTX/Donut
- AMD Render Pipeline Shaders render-graph authoring model: https://github.com/GPUOpen-LibrariesAndSDKs/RenderPipelineShaders
- Diligent render state packager and pipeline-state archive samples: https://github.com/DiligentGraphics/DiligentSamples

Code references:

- `Engine/Renderer/ShaderRegistrations`
- `Engine/Renderer/ShaderRegistrations/RendererShaderPackages.h`
- `Engine/Renderer/Private/Passes/RenderPassDefinition.h`
- `Engine/Renderer/Private/Pipeline/RenderPassDefinitionRuntime.h`
- `Engine/RHI/Public/Shaders/Authoring/GlobalShader.h`
- `Engine/RHI/Public/Shaders/CookedShaderPackageUtils.h`
- `Tools/Shaders/ShaderCompiler`

Tutor note:

- What is wrong today: Stage 17 removed central runtime traits, but each shader registration class still repeats its shader name, package id, binding layout id, source path, entry point, and stage in multiple places.
- What changes: shader/package metadata moves toward a manifest or generated registration source that derives names and registration records from one authoring declaration.
- Why it improves the engine: the best pass-authoring systems let the author state intent once, then tools generate or validate low-level registration records; this reduces typo risk and keeps future pass additions boring.

Implementation prompt:

```text
Using Falcor's render-pass tooling workflow, Donut's separation between reusable passes and shader build metadata, AMD RPS's graph-authoring approach, and Diligent's render-state packager/pipeline archive examples, redesign renderer shader registration so ordinary pass authors do not manually repeat shader class name, package id, binding layout id, shader path, entry point, and stage constants. Prefer a manifest-driven or generated registration model that produces typed registrations for ShaderCompiler and feeds RenderPassDefinition/package identity. Keep diagnostics and reflection validation at least as strong as today. Migrate all existing renderer shader registrations and delete redundant constants or macro paths after validation.
```

Positive guardrails:

- A new ordinary compute shader should require one manifest/pass declaration plus shader source, not hand-written class constants.
- Shader package id and binding layout id should be derived from the pass/shader package declaration unless explicitly overridden.
- Generated or manifest-derived records must still preserve typed parameter metadata, feature flags, shader path, entry point, stage, and diagnostics labels.
- ShaderCompiler should enumerate the same packages with stronger validation for missing paths, duplicate package ids, duplicate shader names, stage mismatches, and binding layout mismatches.
- Keep renderer-owned shader package metadata above RHI; RHI receives only generic package/reflection/runtime primitives.

Negative guardrails:

- Do not replace obvious boilerplate with a macro that hides errors and makes navigation worse.
- Do not move renderer pass metadata into RHI to make generation easier.
- Do not require backend-specific registration code for ordinary passes.
- Do not weaken reflection validation or typed parameter checks to reduce source lines.
- Do not keep both handwritten class registrations and generated manifest registrations indefinitely.

Data transfer contracts:

- Shader authoring metadata transfers through a renderer-owned manifest/generated record: package id, binding layout id, shader name, shader path, entry point, stage, feature flags, parameter struct metadata, and diagnostics label.
- Runtime pass definitions consume package/layout identity from the same source as ShaderCompiler registration.
- ShaderCompiler consumes generated/manifest records without linking full renderer runtime behavior.
- Diagnostics must name pass, package, shader name, path, entry point, stage, binding layout, backend format, and suggested fix.

Legacy cleanup:

- Delete redundant `kShaderName`, `kShaderPackageName`, and `kBindingLayoutId` constants where the new model derives them.
- Delete old handwritten registration classes if generated records replace them.
- Remove `RendererShaderPackages.h` if a stronger manifest source supersedes it, or keep it only as generated output.

Acceptance:

- Existing renderer shader registrations are manifest-driven or generated from a single source of truth.
- The `SkyCS`-style registration no longer repeats class name, package name, binding layout id, shader path, entry point, and stage by hand.
- `ShaderCompiler.exe list-shaders --validate` still reports all expected renderer shader registrations.
- Adding a simple compute pass requires no RHI edit, no central trait edit, and no repeated package/layout/shader constants.
- Docs explain the new shader authoring workflow and why the remaining complexity earns its right to exist.

Validation:

- `ShaderCompiler` built with `DevelopmentEditor` in `build/windows-vs2026-stage5`.
- `ShaderCompiler.exe list-shaders --validate` reported 17 valid typed registrations.
- `ShaderCompiler.exe list-shaders` confirmed renderer packages still enumerate as `GBuffer`, `DirectLighting`, `IndirectLighting`, `LightingComposite`, `Sky`, `VisualizeBuffers`, and `ComputeClear` with matching binding layouts.
- `ShowcaseEditor` built with `DevelopmentEditor` in `build/windows-vs2026-stage5`.
- `architecture_boundary_check` passed with only documented provider-owned DLSS exceptions.
- Source scans over `Engine/Renderer/ShaderRegistrations` found no `kShaderName`, `kShaderPackageName`, `kBindingLayoutId`, or old `IMPLEMENT_GLOBAL_SHADER(` usage.
- Full D3D12/Vulkan smoke remains assigned to Stage 20 unless registration generation changes runtime package loading semantics.

## Stage 17B - Pass Authoring Friction Budget And One-Command Workflow

Status: fully completed for the Stage 17B implementation slice. Frame insertion boilerplate reduction, shader registration validation, and central renderer registration-list cleanup are implemented. Stage 29 owns any larger ShaderContracts catalog/scaffolder decision.

Goal:

- Enumerate every current file, system, and validation step required to add a new shader pass from no shader source to a pass executing in a frame.
- Define and enforce a pass-authoring friction budget so ordinary compute/raster passes require the minimum intentional code and no central runtime/RHI/backend edits.
- Establish the next long-term pass authoring workflow decision point so a future manifest or scaffolder generates mechanical pieces only when it preserves navigation, diagnostics, and explicit contracts.

Source references:

- `rhi-renderer-architecture-review.md`: `Target Shader Pass Model`, `Shader Pass And PSO Handling`, `Hard Gate: Renderer Shader Passes Must Not Require RHI Edits`
- `architecture-review-acceptance-rubric.md`: `Shader and pipeline systems`, `Maintainability and naming`, `Documentation and onboarding`, `Testability`, `Role relevance`

Target shape references:

- Stage target-doc checklist: [Required Target Documents By Stage](#required-target-documents-by-stage), Stage 17B row.
- Primary target docs: [pass-authoring-contract.md](../architecture/pass-authoring-contract.md), [pipeline-runtime-contract.md](../architecture/pipeline-runtime-contract.md), [tooling-pipeline-contract.md](../architecture/tooling-pipeline-contract.md), [validation-workflow-contract.md](../architecture/validation-workflow-contract.md), [repository-target-folder-architecture.md](../architecture/after/repository-target-folder-architecture.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md)

External implementation references:

- NVIDIA Falcor render-pass scaffolding workflow: https://github.com/NVIDIAGameWorks/Falcor/blob/master/docs/development/cmake.md
- NVIDIA Falcor getting-started render pass library workflow: https://github.com/NVIDIAGameWorks/Falcor/blob/master/docs/getting-started.md
- NVIDIA Donut reusable renderer passes and shader build separation: https://github.com/NVIDIA-RTX/Donut
- AMD Render Pipeline Shaders render graph authoring model: https://github.com/GPUOpen-LibrariesAndSDKs/RenderPipelineShaders
- Diligent Render State Notation and render state packager model: https://github.com/DiligentGraphics/DiligentEngine

Code references:

- `Engine/Renderer/Private/Passes`
- `Engine/Renderer/Private/Frame`
- `Engine/Renderer/ShaderRegistrations`
- `Engine/Renderer/Private/Passes/RenderPassDefinition.h`
- `Engine/Renderer/Private/Pipeline/RenderPassDefinitionRuntime.h`
- `Engine/RHI/Public/Shaders/Authoring/GlobalShader.h`
- `Tools/Shaders/ShaderCompiler`
- Future `Tools/Shaders/PassAuthoring` or equivalent pass scaffolding/generation location remains a Stage 29 decision if the audit proves the tool earns its right to exist.

Tutor note:

- What is wrong today: Stage 17 made ownership better, but a pass author still needs to know too many files and repeat too much mechanical code before seeing pixels.
- What changes: we count the whole workflow, then build a supported authoring path where the author states pass intent and shader behavior while tools or shared libraries handle mechanical registration, package identity, diagnostics labels, and validation wiring.
- Why it improves the engine: shader passes are a high-frequency renderer operation. A reviewer should see a pass authoring path that optimizes for clear intent, low boilerplate, and precise failures rather than ceremony.

Implementation prompt:

```text
Using Falcor's render-pass scaffolding workflow, Donut's reusable pass organization, AMD RPS's graph-authoring model, and Diligent's render-state notation/packager model, audit Sparkle's current pass-add workflow from no shader source to a pass executing in a frame. Enumerate every required file, central edit, generated/cooked artifact, validation command, and reason for touching it. Implement the smallest immediate workflow reductions that remove repeated frame insertion plumbing and hand-written central shader registration lists. Defer a manifest-backed scaffolder or generator to Stage 29 unless it removes real mechanical edits and improves validation. Do not solve this with opaque macros, duplicate registries, or by moving boilerplate into a different hand-written file.
```

Positive guardrails:

- Produce a before/after touch-count table for at least `ComputeClear`, `VisualizeBuffers`, and one raster-style pass such as `GBuffer`.
- Define a hard authoring budget for ordinary passes: shader source, one pass intent record or pass definition, and frame insertion should be the only expected intentional edits for a simple pass.
- A scaffolder/generator must emit deterministic files or deterministic generated records and must validate duplicate pass names, duplicate package ids, missing shader paths, entry-point mismatch, stage mismatch, and binding-layout mismatch.
- Keep generated and hand-authored code visually separated by folder and CMake target policy so reviewers can tell what is owned by humans and what is produced by tools.
- Preserve strong diagnostics: pass name, package id, shader name, shader path, entry point, stage, binding/resource name, expected type, actual reflection type, backend, PSO key, and suggested fix.
- Treat dispatch/draw helpers, full-screen compute defaults, debug names, package/layout naming, and registration calls as boilerplate unless they express pass-specific behavior.

Negative guardrails:

- Do not hide the workflow behind broad macros that make compiler or reflection errors harder to understand.
- Do not keep two editable sources of truth for pass identity, shader package identity, binding layout identity, or frame graph resource intent.
- Do not require ordinary pass authors to edit RHI, D3D12, Vulkan, `PipelineStateManager`, ShaderCompiler internals, or a central renderer registration list.
- Do not create a broad "pass common" bucket that simply becomes the new place where unrelated boilerplate accumulates.
- Do not reduce code count by weakening validation, removing diagnostics, or accepting runtime guessing.

Data transfer contracts:

- Pass intent transfers from the authored pass record into frame graph declarations, shader registration records, pipeline runtime lookup, and validation reports.
- Shader package data transfers from the pass/shader manifest into ShaderCompiler package enumeration, cooked package reflection, and runtime `RenderPassDefinition` identity.
- Frame insertion transfers only graph resource handles, pass parameters, and execution callbacks; it must not manually rebuild package/PSO/registration metadata.
- Validation evidence transfers through `ShaderCompiler` list/validate output, boundary checks, target build output, and smoke evidence recorded in coverage status.

Legacy cleanup:

- Remove manual central registration calls when a generated/manifest pass catalog can enumerate registrations deterministically.
- Remove duplicate package/debug-name/string constants that can be derived from the pass authoring record.
- Delete unused pass-specific helper skeletons if the shared dispatch/draw helpers cover the ordinary case with better diagnostics.

Acceptance:

- `pass-authoring-contract.md` contains a current pass-add checklist with file paths and reasons for every touch.
- A target pass authoring budget exists and names the maximum intentional edits for simple compute and raster passes.
- Proof pass families are re-migrated through the reduced workflow with before/after touch counts.
- Adding a simple compute pass requires no RHI edit, no backend edit, no `PipelineStateManager` edit, no ShaderCompiler internal edit, and no repeated shader/package/layout constants.
- The workflow validates duplicate names, missing shaders, stage mismatches, binding mismatches, and package/reflection errors before runtime smoke, and runtime executables retain registrations without a hand-written central C++ list.

Validation:

- `ShowcaseEditor` built with `DevelopmentEditor` in `build/windows-vs2026-stage5`.
- `ShaderCompiler` built with `DevelopmentEditor` in `build/windows-vs2026-stage5`.
- `ShaderCompiler.exe list-shaders --validate` reported 17 valid typed registrations after adding authoring-record validation.
- `architecture_boundary_check` passed with only documented provider-owned DLSS exceptions.
- `pass-authoring-contract.md` records before/after touch-count evidence for `ComputeClear`, `VisualizeBuffers`, and `GBuffer`.
- Run targeted launcher-shaped smoke in Stage 20 unless the proof pass is user-visible enough to justify smoke in this stage.
- Scan for `RegisterRendererGlobalShaders`, `RendererShaderRegistration`, and renderer `Register*Shaders() noexcept {}` stubs after central registration-list cleanup.

## Stage 18 - Clean Ray Tracing Ownership And Contracts

Status: fully completed. Renderer/RHI ownership is aligned in docs and diagnostics; D3D12/Vulkan visual, RT scene, and deterministic camera-motion validation passed in Stage 20.

Goal:

- Preserve the useful renderer/RHI ray tracing split while making lifetime and naming contractual.

Source references:

- `rhi-renderer-architecture-review.md`: `Ray Tracing Is Mostly Well-Bounded`, `RayTracing coverage rows`
- `architecture-review-acceptance-rubric.md`: `Graphics API fluency`, `Rendering fundamentals`, `Reliability/failure handling`

Target shape references:

- Stage target-doc checklist: [Required Target Documents By Stage](#required-target-documents-by-stage), Stage 18 row.
- Primary target docs: [ray-tracing-contract.md](../architecture/ray-tracing-contract.md), [rhi-contract-map.md](../architecture/rhi-contract-map.md), [frame-graph-contract.md](../architecture/frame-graph-contract.md), [pass-authoring-contract.md](../architecture/pass-authoring-contract.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md)

External implementation references:

- Microsoft DXR specification: https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html
- NVIDIA DXR tutorial: https://developer.nvidia.com/rtx/raytracing/dxr/dx12-raytracing-tutorial-part-1
- Khronos Vulkan ray tracing sample: https://github.com/KhronosGroup/Vulkan-Samples/blob/main/samples/extensions/ray_tracing_basic/README.adoc
- NVIDIA Vulkan ray tracing tutorial: https://nvpro-samples.github.io/vk_raytracing_tutorial_KHR/

Code references:

- `Engine/RHI/Public/RayTracing/RhiRayTracingDesc.h`
- `Engine/Renderer/Private/RayTracing`
- `Engine/Renderer/Private/Frame/RayTracingScene*`
- `Engine/Renderer/Private/Passes/DirectLightingPass.*`
- D3D12/Vulkan AS build implementations in command lists

Tutor note:

- What is wrong today: the renderer/RHI split is mostly good, but names and ownership around RT scene, TLAS frame data, pass services, and shadow data can blur.
- What changes: the lifetime from scene snapshot to BLAS cache, TLAS build, frame graph AS import, pass binding, and shader-visible data is made contractual.
- Why it improves the engine: ray tracing bugs are often ownership bugs; clear AS lifetime and API-neutral descriptors make both D3D12 and Vulkan easier to trust.

Implementation prompt:

```text
Using the DXR spec, NVIDIA DXR tutorial, Khronos Vulkan ray tracing sample, and NVIDIA Vulkan ray tracing tutorial as references, document and clean ray tracing ownership from scene snapshot to BLAS cache, TLAS build, frame graph AS registration, pass service binding, and shader-visible shadow data. Keep RHI ray tracing descriptors generic. Rename or split ambiguous types where ownership is unclear. Validate D3D12/Vulkan AS builds and ray-traced shadows after camera movement.
```

Positive guardrails:

- Renderer owns scene acceleration structure lifetime.
- RHI owns AS descriptors, prebuild info, scratch/result allocation primitives, and command-list build operations.
- Direct lighting/shadow data stays with renderer pass or shared render-data module, not RHI.
- Diagnostics report TLAS instance count, rejected meshes, missing GPU mesh data, and unsupported feature reasons.

Negative guardrails:

- Do not put shadow pass concepts in `RhiRayTracingDesc.h`.
- Do not include D3D12/Vulkan headers in renderer ray tracing code.
- Do not hide unsupported RT fallback behind black/noisy output.

Data transfer contracts:

- GameFramework/Renderer scene data transfers into RT through render snapshots, mesh GPU data references, and renderer-owned AS scene records.
- Renderer transfers AS build requests to RHI through generic RHI ray tracing descriptors and command-list build operations.
- Shadow/pass data transfers through renderer pass uniform data and shader parameters, not RHI public RT structs.

Legacy cleanup:

- Remove old ambiguous helper names after replacements exist.
- Remove duplicated RT capability checks if a single capability report owns them.

Acceptance:

- Ray tracing contract doc matches code.
- D3D12/Vulkan shadow behavior is stable during camera rotation.
- RHI ray tracing public structs contain GPU/API concepts only.

Validation:

- `ShowcaseEditor` build is required after Stage 18 code changes.
- `architecture_boundary_check` must pass with no renderer/backend-native RT leaks.
- Source scan should confirm `RhiRayTracingDesc.h` contains no shadow/pass concepts.
- Include D3D12/Vulkan camera-motion shadow parity in Stage 20 full visual parity.

## Stage 19 - Slim Backend Facades And Enforce D3D12/Vulkan Service Symmetry

Goal:

- Make backend implementation reviewable by service area instead of giant facade classes.
- Ensure D3D12 and Vulkan are separated, symmetric, and parity-driven.

Source references:

- `rhi-renderer-architecture-review.md`: `D3D12 Backend Coverage`, `Vulkan Backend Coverage`, `Backend Parity Matrix`
- `architecture-review-acceptance-rubric.md`: `Graphics API fluency`, `Portability/backend parity`, `Modern C++ systems skill`

Target shape references:

- Stage target-doc checklist: [Required Target Documents By Stage](#required-target-documents-by-stage), Stage 19 row.
- Primary target docs: [rhi-contract-map.md](../architecture/rhi-contract-map.md), [architecture-boundary-guardrails.md](../architecture/architecture-boundary-guardrails.md), [repository-target-folder-architecture.md](../architecture/after/repository-target-folder-architecture.md), [rendering-coverage-status.md](../architecture/rendering-coverage-status.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md)

External implementation references:

- NVIDIA NVRHI backend abstraction: https://github.com/NVIDIA-RTX/NVRHI
- NVIDIA NRI backend-oriented rendering interface: https://github.com/NVIDIA-RTX/NRI
- AMD Cauldron backend folders: https://github.com/GPUOpen-LibrariesAndSDKs/Cauldron
- Diligent Engine backend implementations: https://github.com/DiligentGraphics/DiligentEngine

Code references:

- `Engine/RHI/Private/D3D12/D3D12RenderHardwareInterface.cpp`
- `Engine/RHI/Private/Vulkan/VulkanRenderHardwareInterface.cpp`
- `Engine/RHI/Private/D3D12/Commands`
- `Engine/RHI/Private/Vulkan/Commands`
- `Engine/RHI/Private/D3D12/Descriptors`
- `Engine/RHI/Private/Vulkan/Descriptors`
- `Engine/RHI/Private/D3D12/Memory`
- `Engine/RHI/Private/Vulkan/Memory`
- `Engine/RHI/Private/D3D12/Pipeline`
- `Engine/RHI/Private/Vulkan/Pipeline`
- `Engine/RHI/Private/D3D12/RayTracing`
- `Engine/RHI/Private/Vulkan/RayTracing`
- `Engine/RHI/Private/D3D12/Resources`
- `Engine/RHI/Private/Vulkan/Resources`

Tutor note:

- What is wrong today: D3D12 and Vulkan root facade files implement too many responsibilities, which makes backend parity hard to audit.
- What changes: backend responsibilities move into symmetric service areas for commands, descriptors, memory, pipeline, resources, diagnostics, swap chain, UI, and interop.
- Why it improves the engine: reviewers can compare D3D12 and Vulkan subsystem by subsystem instead of reading two giant backend facades.

Implementation prompt:

```text
Using NVRHI, NRI, Cauldron, and Diligent Engine backend organization as references, decompose D3D12RenderHardwareInterface and VulkanRenderHardwareInterface behind service-owned implementation objects. Preserve backend folder symmetry for commands, descriptors, memory, pipeline, resources, diagnostics, swap chain, UI, and interop. Make API-specific differences explicit in type conversion and service implementation files. Remove root-facade forwarding code when callers use services.
```

Positive guardrails:

- Keep backend-private code inside backend folders.
- Type conversion files should be total over public enum/desc values.
- Depth, winding, viewport, culling, and resource state/layout conventions must be documented and parity-tested.
- Service names should match across D3D12 and Vulkan where responsibilities match.

Negative guardrails:

- Do not move backend code into Renderer.
- Do not merge D3D12 and Vulkan implementation details into common code unless the abstraction is truly backend-neutral.
- Do not leave old facade methods forwarding forever.

Data transfer contracts:

- Public RHI data transfers into backend services through normalized descriptors, command parameters, capabilities, and diagnostics records.
- D3D12/Vulkan service implementations may exchange only shared backend-neutral helpers from RHI common code, not each other's private headers.
- Backend differences must be represented as explicit capability/type-conversion/service behavior notes, not hidden caller-side conditionals.

Legacy cleanup:

- Delete obsolete root-facade methods after service migration.
- Delete unused backend helper methods and duplicate conversion code.
- Remove duplicated CMake source grouping if obsolete.

Acceptance:

- Backend root facade files are materially smaller and mostly wire services.
- D3D12/Vulkan service folders are symmetric where appropriate.
- Public RHI method map is updated to reflect remaining facade methods and services.

Validation:

- Include in Stage 20 full backend validation.

Stage 19 progress evidence:

| Item | Evidence |
| --- | --- |
| Status | Started. |
| Extracted service slice | Ray tracing prebuild queries, scratch/result AS buffer creation, and instance-buffer creation now live in `D3D12RayTracingServices` and `VulkanRayTracingServices`. |
| Facade shape | D3D12/Vulkan root facades compose the ray tracing services and keep thin forwarding methods only for the existing public `RenderHardwareInterface` API. |
| Symmetry | Both backends now have sibling `RayTracing` folders and matching service class names/responsibilities. |
| Validation | `ShowcaseEditor` built in `build/windows-vs2026-stage5`; `architecture_boundary_check` passed with only documented provider exceptions. |
| Remaining risk | Command, descriptor, memory, pipeline, resource, presentation/UI, and diagnostics implementations still need deeper service slimming; Stage 20 owns runtime D3D12/Vulkan RT parity smoke. |

## Stage 20 - Validation Milestone D: Full Renderer/RHI Backend Parity

Goal:

- Validate the full architectural refactor before the whole-repository extension stages continue.

Target shape references:

- Stage target-doc checklist: [Required Target Documents By Stage](#required-target-documents-by-stage), Stage 20 row.
- Primary target docs: [rendering-coverage-status.md](../architecture/rendering-coverage-status.md), [rendering-system-map.md](../architecture/rendering-system-map.md), [rhi-contract-map.md](../architecture/rhi-contract-map.md), [ray-tracing-contract.md](../architecture/ray-tracing-contract.md), [pipeline-runtime-contract.md](../architecture/pipeline-runtime-contract.md), [tooling-pipeline-contract.md](../architecture/tooling-pipeline-contract.md), [validation-workflow-contract.md](../architecture/validation-workflow-contract.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md)

External implementation references:

- NVIDIA Donut Samples executable graphics evidence: https://github.com/NVIDIA-RTX/Donut-Samples
- NVIDIA NVRHI tutorial: https://github.com/NVIDIA-RTX/NVRHI/blob/main/doc/Tutorial.md
- Khronos Vulkan validation layers: https://github.com/KhronosGroup/Vulkan-ValidationLayers
- NVIDIA Streamline programming guide: https://github.com/NVIDIA-RTX/Streamline/blob/main/docs/ProgrammingGuide.md
- AMD FidelityFX SDK: https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK

Tutor note:

- What is wrong today: backend parity has been judged manually through editor launches and screenshots, which is useful but not enough for a review-ready repo.
- What changes: final renderer/RHI validation produces named evidence for build, smoke, captures, DLSS, RT, frame graph, and PSO runtime.
- Why it improves the engine: external reviewers trust repeatable evidence more than claims, especially for graphics bugs that can be machine-dependent.

Implementation prompt:

```text
Using Donut Samples, NVRHI tutorial validation, Vulkan validation layers, Streamline diagnostics, FidelityFX-style feature reporting, and the local validation workflow contract as references, run the full D3D12/Vulkan validation suite for build, shader compiler, launcher, smoke validation, lit captures, normal/debug captures, ray tracing, DLSS/upscaling, frame graph diagnostics, PSO runtime logs, and boundary checks. Use Launcher or LauncherCore plan execution for user-facing launch validation whenever practical; direct execution must mirror the launcher plan and be recorded as launcher-shaped. Record artifacts and compare against the architecture rubric.
```

Positive guardrails:

- Validate D3D12 and Vulkan in the same scene and camera path.
- Capture lit and normal/debug outputs.
- Check logs for PSO keys, shader package IDs, backend capability reports, DLSS provider status, RT capability status, and frame graph diagnostics.
- Keep exact commands and artifact paths.
- Keep launch validation aligned with launcher operation planning: project, target, profile, working directory, smoke environment, log path, and artifacts.

Negative guardrails:

- Do not accept visual parity based only on memory.
- Do not ignore validation warnings that indicate contract drift.
- Do not hide known differences; document them with reason and owner.
- Do not use private direct-launch habits as final evidence when the launcher path would use different readiness checks, environment, working directory, or artifact locations.

Data transfer contracts:

- Final graphics evidence transfers through build logs, smoke logs, captures, PSO logs, shader package reports, capability reports, and coverage-status updates.
- Each artifact must name producer command, backend, project/scene, output path, and consumer reviewer doc.
- Known differences transfer into docs as owned issues with reason and follow-up, not as unstated acceptance gaps.
- Launcher evidence transfers through operation records, dry-run/plan fields, process logs, smoke artifacts, and coverage-status updates.

Suggested validation:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config DevelopmentEditor --target ShaderCompiler -- /nologo /v:minimal /m:1
cmake --build build --config DevelopmentEditor --target SparkleLauncher -- /nologo /v:minimal /m:1
cmake --build build --config DevelopmentEditor --target clang_format_check -- /nologo /v:minimal

# Run D3D12 editor smoke lit.
$env:SPARKLE_SMOKE_VALIDATE_RHI='1'
$env:SPARKLE_SMOKE_FRAME_LIMIT='120'
$env:SPARKLE_SMOKE_SCENE_COLOR_CAPTURE_FRAME='30'
$env:SPARKLE_RHI_BACKEND='D3D12'
$env:SPARKLE_SMOKE_VIEW_MODE='0'
$env:SPARKLE_SMOKE_SCENE_COLOR_CAPTURE='artifacts/validation/d3d12-final-lit.bmp'
# Launch Showcase editor/runtime.

# Run Vulkan editor smoke lit.
$env:SPARKLE_RHI_BACKEND='Vulkan'
$env:SPARKLE_SMOKE_VIEW_MODE='0'
$env:SPARKLE_SMOKE_SCENE_COLOR_CAPTURE='artifacts/validation/vulkan-final-lit.bmp'
# Launch Showcase editor/runtime.

# Run D3D12 and Vulkan normal/debug captures with the appropriate RenderViewMode value.
```

Acceptance:

- Build targets pass.
- Boundary checks pass with no architecture exceptions except documented provider-only SDK integration if still required.
- D3D12/Vulkan smoke runs pass.
- Frame graph unresolved-resource warnings are zero.
- Normal/debug view modes work for both backends.
- Lit output is within agreed tolerance or differences are documented.
- DLSS is active when requested and supported, or fallback reason is deterministic.
- RT/shadow behavior is stable under camera rotation.
- Coverage status is updated.

Execution status:

| Field | Evidence |
| --- | --- |
| Status | Fully completed. Build, boundary, shader package, D3D12/Vulkan editor smoke, D3D12/Vulkan runtime smoke, capture, frame graph, DLSS, RT capability, RT scene, PSO runtime, visual comparison, and deterministic camera-motion RT validation passed. |
| Build commands | `cmake --build build/windows-vs2026-stage5 --config DevelopmentEditor --target ShaderCompiler -- /nologo /v:minimal /m:1`; `cmake --build build/windows-vs2026-stage5 --config DevelopmentEditor --target SparkleLauncher -- /nologo /v:minimal /m:1`; `cmake --build build/windows-vs2026-stage5 --config DevelopmentEditor --target ShowcaseEditor -- /nologo /v:minimal /m:1`; `cmake --build build/windows-vs2026-stage5 --config DevelopmentEditor --target ShowcaseRuntime -- /nologo /v:minimal /m:1`; `cmake --build build/windows-vs2026-stage5 --config DevelopmentEditor --target architecture_boundary_check -- /nologo /v:minimal`. |
| Shader validation | `artifacts/dev/tools/ShaderCompiler/DevelopmentEditor/ShaderCompiler.exe list-shaders --validate` exited `0`; `artifacts/validation/stage20/shader-list-validate.log` reports 17 valid typed registrations. |
| Smoke matrix | Launcher-shaped direct execution from `Projects/Showcase` ran `editor-d3d12-lit`, `editor-d3d12-normal`, `editor-vulkan-lit`, `editor-vulkan-normal`, `runtime-d3d12`, and `runtime-vulkan`; every run exited `0` with zero diagnostic matches. |
| Capture artifacts | `artifacts/validation/stage20/d3d12-stage20-lit.bmp`, `artifacts/validation/stage20/d3d12-stage20-normal.bmp`, `artifacts/validation/stage20/vulkan-stage20-lit.bmp`, and `artifacts/validation/stage20/vulkan-stage20-normal.bmp`. |
| Runtime evidence | Smoke logs report `frameGraphUnresolvedBarrierWarnings=0`, active NVIDIA DLSS, RT and inline ray query availability, stable BLAS/TLAS construction, and explicit PSO runtime keys on both backends. |
| Camera-motion RT evidence | `artifacts/validation/stage20-camera-motion/stage20-camera-motion-results.json` records `editor-d3d12-camera`, `editor-vulkan-camera`, `runtime-d3d12-camera`, and `runtime-vulkan-camera`. Every run exited `0`, emitted camera-motion smoke evidence, reported `tlasValid=true`, and reported `frameGraphUnresolvedBarrierWarnings=0`; editor runs produced `d3d12-camera-lit.bmp` and `vulkan-camera-lit.bmp`. |
| Visual comparison | `artifacts/validation/stage20/stage20-visual-comparison.json` records D3D12/Vulkan lit mean absolute byte difference `0.45850590702771543` and normal mean absolute byte difference `0.5269850834129329` for 4578x1180 captures. |
| Remaining strict gap | None for Stage 20. Stage 19 still owns deeper backend service slimming, but the renderer/RHI parity validation milestone is complete. |

## Whole-Repository Extension Stages

The first 20 stages bring the graphics track to a strong implementation and validation checkpoint. Stages 23-36 are not a passive audit. They apply the same treatment to the rest of the repository: classify ownership, redesign weak boundaries, rename misleading folders, remove duplicate production paths, update CMake/CI guardrails, and validate each module family through its real user path. Portfolio/repository presentation work is intentionally out of scope until the code architecture work is green.

Reference basis for the whole-repository track:

- NVIDIA Donut separates `donut_core`, `donut_engine`, `donut_render`, `donut_app`, shaders, and samples, which is the model for keeping framework, renderer, app, shader, and sample responsibilities distinct: https://github.com/NVIDIA-RTX/Donut
- AMD Cauldron is a DX12/Vulkan sample framework used for SDK samples and prototypes, which is the model for sample/runtime/backend separation and validation through real sample projects: https://github.com/GPUOpen-LibrariesAndSDKs/Cauldron
- AMD Compressonator separates GUI, CLI, and SDK surfaces for asset tooling, which is the model for launcher/tool UI separation and focused command-line/SDK tool boundaries: https://github.com/GPUOpen-Tools/compressonator
- Khronos glTF and KTX keep runtime asset delivery formats separate from source-authoring workflows, which is the model for `AssetContracts`, cooked artifacts, and source import boundaries: https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html and https://github.com/KhronosGroup/KTX-Software

Whole-repository extension rule:

- Do not let Stage 23, 34, or 35 become a substitute for implementation. They only establish coverage, final evidence, and threading-readiness checks. Stages 24-33 must do the active design/refactor work for Core, Platform, GameFramework, assets, importers, cookers, shader tooling, launcher, hosts, projects, build, and CI.

## Stage 23 - Whole-Repository Coverage And Dependency Map

Goal:

- Make every durable source root visible in the architecture review.
- Freeze the dependency intent before refactoring Core, Platform, GameFramework, tools, launcher, projects, build, CI, or docs.
- Prevent later whole-repository work from hiding behind the graphics-focused coverage map.

Source references:

- `repository-system-map.md`
- `repository-coverage-status.md`
- `architecture-review-acceptance-rubric.md`

Target shape references:

- Stage target-doc checklist: [Required Target Documents By Stage](#required-target-documents-by-stage), Stage 23 row.
- Primary target docs: [repository-system-map.md](../architecture/repository-system-map.md), [repository-coverage-status.md](../architecture/repository-coverage-status.md), [repository-current-state.md](../architecture/before/repository-current-state.md), [repository-target-architecture.md](../architecture/after/repository-target-architecture.md), [repository-target-folder-architecture.md](../architecture/after/repository-target-folder-architecture.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md)

Implementation prompt:

```text
Using the repository system map, CMake target dependency graph, Donut/Cauldron repository organization, and Compressonator GUI/CLI/SDK split as references, verify every Engine, Tools, Projects, CMake, docs, and CI root has a named owner, allowed dependencies, forbidden dependencies, validation target, active refactor stage, and acceptance evidence. This stage freezes the map only; stages 24-33 perform the implementation refactors.
```

Positive guardrails:

- Name allowed edges explicitly, such as `Renderer -> RHI`, `Renderer -> GameFramework snapshots`, `Tools -> public cooked schemas`, and `LauncherCore -> tool process requests`.
- Name forbidden edges explicitly, including runtime modules to `Tools/*`, GameFramework to `Engine/Renderer/Private`, GameFramework to `Engine/RHI/Private`, and Renderer to backend-native headers outside documented providers.
- Keep source-root ownership in `repository-coverage-status.md` and broad edges in `repository-system-map.md`.

Negative guardrails:

- Do not create a vague "misc tools" or "engine utilities" bucket.
- Do not mark a root accepted without naming validation evidence.
- Do not solve dependency ambiguity by adding public include directories broadly.

Data transfer contracts:

- Repository ownership transfers through coverage rows: root, owner, allowed dependencies, forbidden dependencies, producer/consumer role, validation target, and acceptance evidence.
- CMake dependency data transfers through target links and `PUBLIC`/`PRIVATE`/`INTERFACE` scope notes.
- New source roots must update the coverage map before later stages use them.

Acceptance:

- `repository-coverage-status.md` covers every durable source root.
- Rendering detail remains delegated to `rendering-coverage-status.md`.
- Every non-rendering `Needs refactor` row links to a concrete active refactor stage, not only Stage 34/35/36.
- Any new source folder added during later stages must update one of the coverage maps before code lands there.

Validation:

- Docs/link scan with `rg`.
- No build required.

## Stage 24 - Core And Platform Foundation Boundary Refactor

Goal:

- Keep Core and Platform small, policy-free, and useful.
- Prevent foundation code from becoming a hidden scheduler, renderer, tool, launcher, or platform policy sink.

Source references:

- `repository-coverage-status.md`: `Engine/Core`, `Engine/Platform`
- `repository-system-map.md`: foundation and platform edges
- `architecture-review-acceptance-rubric.md`: cohesion, maintainability, reliability

Target shape references:

- Stage target-doc checklist: [Required Target Documents By Stage](#required-target-documents-by-stage), Stage 24 row.
- Primary target docs: [repository-system-map.md](../architecture/repository-system-map.md), [repository-target-architecture.md](../architecture/after/repository-target-architecture.md), [repository-target-folder-architecture.md](../architecture/after/repository-target-folder-architecture.md), [architecture-boundary-guardrails.md](../architecture/architecture-boundary-guardrails.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md)

External implementation references:

- Donut core/app separation: https://github.com/NVIDIA-RTX/Donut
- Cauldron framework/sample separation: https://github.com/GPUOpen-LibrariesAndSDKs/Cauldron

Implementation prompt:

```text
Review and refactor Engine/Core and Engine/Platform so foundation modules own only reusable diagnostics, files, strings, math, timing, events, and OS/window/input abstractions. Move or redesign any renderer, RHI, tool, launcher, gameplay, editor, or scheduling policy that has drifted into Core/Platform. Keep Core policy-free and Platform host-facing but not renderer/application policy-owned.
```

Positive guardrails:

- Core helpers must have multiple consumers and no domain policy.
- Platform owns OS/window/input boundaries and hands events upward as typed events/requests.
- Diagnostics and file utilities should remain deterministic and useful for tools and runtime.
- Threading-readiness means Core/Platform expose value types, events, and reports; they do not own a speculative global scheduler.

Negative guardrails:

- Do not put renderer/RHI/tool/launcher policy into Core to avoid dependency direction.
- Do not add a global job system or broad lock layer here.
- Do not let Platform know about renderer passes, cooked schemas, launcher workflows, or editor panels.

Data transfer contracts:

- Platform transfers host input/window state through typed events and window contracts.
- Core transfers diagnostics and file/path results through value types and reports.
- Cross-module scheduling decisions remain in owning systems until a real shared contract earns its right to exist.

Acceptance:

- Core has no renderer/RHI/tool/launcher policy includes.
- Platform has no renderer/pass/tool/cook policy ownership.
- Any moved code lands in the module that owns the domain vocabulary.

Validation:

- Targeted include/dependency scan.
- Build `SparkleCore`, `SparklePlatform`, and the smallest affected host target.

## Stage 25 - GameFramework Runtime, Render Contracts, And Shared Schema Refactor

Goal:

- Protect runtime scene/gameplay ownership while renderer and tools evolve.
- Prevent source import/cooking or renderer pass policy from leaking into GameFramework.
- Extract or document shared `AssetContracts` and `RenderContracts` where GameFramework, tools, Renderer, and projects need the same schema.

Source references:

- `game-framework-contract.md`
- `Engine/GameFramework/CMakeLists.txt`
- `Tools/Cooking/*`
- `Tools/Import/SourceImportAdapters`

Target shape references:

- Stage target-doc checklist: [Required Target Documents By Stage](#required-target-documents-by-stage), Stage 25 row.
- Primary target docs: [game-framework-contract.md](../architecture/game-framework-contract.md), [repository-target-architecture.md](../architecture/after/repository-target-architecture.md), [repository-target-folder-architecture.md](../architecture/after/repository-target-folder-architecture.md), [tooling-pipeline-contract.md](../architecture/tooling-pipeline-contract.md), [system-design-index.md](../architecture/after/system-design-index.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md)

Implementation prompt:

```text
Review and refactor GameFramework as the runtime owner for scenes, levels, components, cameras, lighting, runtime asset IDs, cooked payload loading, and gameplay-facing scene APIs. Extract shared producer/consumer schemas toward AssetContracts and renderer handoff DTOs toward RenderContracts when current callers prove the need. Enforce the split between public cooked/runtime schemas, tool-side source import/cooking, and renderer-side render snapshots/resources.
```

Positive guardrails:

- Prefer `GameFramework -> Renderer` handoff through immutable render snapshots/DTOs.
- Prefer `Tools -> GameFramework` interaction through public cooked schema headers and validators.
- Keep runtime failure diagnostics asset-oriented: asset id, file path, record kind, schema version, expected feature, and reason.

Negative guardrails:

- Do not include `Engine/Renderer/Private`, `Engine/RHI/Private`, D3D12/Vulkan headers, or `Tools/*` implementation headers from GameFramework.
- Do not move renderer pass/shader data into GameFramework to avoid a Renderer/RHI boundary issue.
- Do not make GameFramework parse source glTF/FBX/images directly.

Data transfer contracts:

- Cookers transfer cooked mesh/material/scene/animation/skeleton records to GameFramework through versioned cooked schemas.
- GameFramework transfers renderable state to Renderer through immutable snapshots/DTOs, not mutable gameplay components.
- Schema changes must name producer cooker, schema owner, GameFramework loader, renderer consumer, inspection command, and smoke/load evidence.

Acceptance:

- GameFramework has no Renderer-private, RHI-backend-private, or Tools-private dependencies.
- Cooked schema changes identify paired cooker, loader, and renderer-scene-data updates.
- Renderer consumes immutable runtime snapshots or DTOs, not gameplay mutation paths.
- Any shared schema kept in GameFramework has a documented reason; otherwise it moves toward a contract root.

Validation:

- Targeted include scan.
- Build `SparkleGameFramework` when a compiler is available.
- Run affected cook/load smoke when schema changes occur.

## Stage 26 - Runtime Cooked Asset Loader And Schema Pairing Refactor

Goal:

- Make cooked asset schemas and runtime loaders first-class contracts.
- Pair every runtime loader expectation with a producer cooker and validation/inspection path.

Source references:

- `game-framework-contract.md`
- `tooling-pipeline-contract.md`
- `Engine/GameFramework`
- `Tools/Cooking`

Target shape references:

- Stage target-doc checklist: [Required Target Documents By Stage](#required-target-documents-by-stage), Stage 26 row.
- Primary target docs: [game-framework-contract.md](../architecture/game-framework-contract.md), [tooling-pipeline-contract.md](../architecture/tooling-pipeline-contract.md), [repository-target-folder-architecture.md](../architecture/after/repository-target-folder-architecture.md), [repository-system-map.md](../architecture/repository-system-map.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md)

External implementation references:

- glTF runtime asset delivery format: https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html
- KTX texture tooling/container model: https://github.com/KhronosGroup/KTX-Software
- Cauldron glTF/sample framework: https://github.com/GPUOpen-LibrariesAndSDKs/Cauldron

Implementation prompt:

```text
Refactor runtime cooked asset loading so mesh, material, texture-reference, scene, animation, and skeleton records have explicit schema owners, loader diagnostics, producer cooker links, and renderer consumer links. Do not change a schema unless the focused cooker, GameFramework loader, renderer consumer, inspector/validation command, and sample load evidence are updated together.
```

Positive guardrails:

- Loader failures name asset id, path, schema version, record kind, expected feature, and reason.
- Runtime loaders consume cooked records only.
- Schema ownership should be neutral when tools and runtime both own the data.

Negative guardrails:

- Do not let GameFramework parse source glTF/FBX/images.
- Do not let cookers depend on private GameFramework loader implementation as schema documentation.
- Do not update runtime loaders without producer cooker evidence.

Data transfer contracts:

- Cookers transfer cooked records through versioned schemas.
- GameFramework transfers validated runtime payloads to scene/runtime owners.
- Renderer receives render-domain DTOs or cooked/runtime records, not source-import structures.

Acceptance:

- Mesh/material/texture/scene/animation/skeleton loader expectations are paired with producer cooker expectations.
- Loader diagnostics are asset-oriented and deterministic.
- Runtime/cooker schema ownership is documented or extracted.

Validation:

- Build `SparkleGameFramework` and affected cookers.
- Run targeted sample cook/load or loader inspection when available.

## Stage 27 - Source Importer Architecture Refactor

Goal:

- Replace pattern-centered `SourceImportAdapters` architecture with role-centered source importers.
- Keep source-format assumptions out of runtime and focused cookers.

Source references:

- `tooling-pipeline-contract.md`
- `Tools/Import/SourceImportAdapters`
- `Tools/Cooking/*Cooker`

Target shape references:

- Stage target-doc checklist: [Required Target Documents By Stage](#required-target-documents-by-stage), Stage 27 row.
- Primary target docs: [tooling-pipeline-contract.md](../architecture/tooling-pipeline-contract.md), [repository-target-folder-architecture.md](../architecture/after/repository-target-folder-architecture.md), [game-framework-contract.md](../architecture/game-framework-contract.md), [repository-target-architecture.md](../architecture/after/repository-target-architecture.md), [repository-system-map.md](../architecture/repository-system-map.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md)

External implementation references:

- Donut scene loading and framework split: https://github.com/NVIDIA-RTX/Donut
- Cauldron glTF/sample framework: https://github.com/GPUOpen-LibrariesAndSDKs/Cauldron
- glTF as source/runtime delivery reference: https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html

Implementation prompt:

```text
Refactor Tools/Import/SourceImportAdapters toward Tools/Import/SourceImporters. Each importer should own one source-format family and emit imported DTOs plus diagnostics. Focused cookers consume imported DTOs, not source-loader internals. Runtime modules must not include importer headers.
```

Positive guardrails:

- Importers produce imported DTOs and import reports.
- Import jobs are deterministic and named by source path, importer id, options, and output DTO/report.
- Source import failure reports include source file, node/material/mesh reference when available, and reason.

Negative guardrails:

- Do not keep adapter and importer folders as parallel production paths.
- Do not make runtime loaders understand source file formats.
- Do not let focused cookers bypass importer contracts by calling format-specific internals.

Data transfer contracts:

- Source files transfer into imported DTOs and diagnostics.
- Focused cookers consume DTOs and produce cooked artifacts.
- AssetCooker sees job/report data, not importer-private state.

Acceptance:

- Import folder names and CMake targets express importer ownership.
- Runtime modules have no `Tools/Import` implementation dependency.
- At least one representative import path validates the DTO/report handoff.

Validation:

- Build affected import targets.
- Run targeted sample import/cook when available.

## Stage 28 - Focused Cooker And Tool Support Refactor

Goal:

- Apply the same owner/contract/refactor treatment to TextureCooker, MeshCooker, MaterialCooker, SceneCooker, and shared cook support.
- Split weak support names such as `CookCommon` into precise support/diagnostics owners.

Source references:

- `tooling-pipeline-contract.md`
- `repository-target-folder-architecture.md`
- `Tools/Import/SourceImportAdapters`
- `Tools/Cooking`
- `Tools/Conversion/AssetConverter`

Target shape references:

- Stage target-doc checklist: [Required Target Documents By Stage](#required-target-documents-by-stage), Stage 28 row.
- Primary target docs: [tooling-pipeline-contract.md](../architecture/tooling-pipeline-contract.md), [repository-target-folder-architecture.md](../architecture/after/repository-target-folder-architecture.md), [game-framework-contract.md](../architecture/game-framework-contract.md), [repository-target-architecture.md](../architecture/after/repository-target-architecture.md), [repository-system-map.md](../architecture/repository-system-map.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md)

Implementation prompt:

```text
Refactor focused cookers so TextureCooker, MeshCooker, MaterialCooker, and SceneCooker each own one artifact transformation family, deterministic cooked artifact emission, and actionable diagnostics. Split Tools/Cooking/CookCommon into precise support owners such as Tools/Support/ToolConsoleSupport and Tools/Cooking/CookDiagnostics when current callers justify the split. Do not move orchestration into focused cookers and do not move focused algorithms into AssetCooker or Launcher.
```

Positive guardrails:

- TextureCooker, MeshCooker, MaterialCooker, and SceneCooker own focused transformations and cooked artifact emission.
- CookCommon is renamed/split into precise support surfaces such as ToolConsoleSupport and CookDiagnostics.
- Cooker jobs publish final artifacts only after validation succeeds.
- Cookers produce deterministic reports with source path, artifact id, schema version, output path, and reason.

Negative guardrails:

- Do not let runtime modules include `Tools/Import`, `Tools/Cooking`, or `Tools/Conversion` implementation headers.
- Do not let focused cookers own project workflow/UI policy.
- Do not preserve CookCommon as a broad permanent owner name.
- Do not create a generic `Tools/Common`, `Tools/Utils`, or `Tools/Conversion` replacement that hides ownership.
- Do not put source importers, cookers, or inspectors under `Engine/` to make runtime links easier.
- Do not hide failures behind generic "cook failed" messages.

Data transfer contracts:

- Focused cookers transfer data as cooked artifacts with schema/version/feature records.
- Tool support transfers console/report data through ToolContracts-compatible reports, not ad hoc global helpers.
- Folder ownership transfers through CMake target renames/splits that preserve one focused production path per artifact family.

Acceptance:

- Focused cookers own focused transformations.
- CookCommon is replaced by precise support/diagnostics surfaces.
- Cooker outputs are deterministic and paired with runtime loader expectations.

Validation:

- Build affected cook/import target.
- Run targeted sample cook when available.
- Verify failures report source path, asset id, output artifact, and reason.

## Stage 29 - Shader Toolchain Contract Refactor

Goal:

- Give ShaderCompiler the same contract discipline as Renderer/RHI shader runtime.
- Remove any remaining need to link broad renderer runtime for shader package enumeration.

Source references:

- `tooling-pipeline-contract.md`
- `pass-authoring-contract.md`
- `pipeline-runtime-contract.md`
- `Tools/Shaders/ShaderCompiler`

Target shape references:

- Stage target-doc checklist: [Required Target Documents By Stage](#required-target-documents-by-stage), Stage 29 row.
- Primary target docs: [tooling-pipeline-contract.md](../architecture/tooling-pipeline-contract.md), [pass-authoring-contract.md](../architecture/pass-authoring-contract.md), [pipeline-runtime-contract.md](../architecture/pipeline-runtime-contract.md), [repository-target-folder-architecture.md](../architecture/after/repository-target-folder-architecture.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md)

External implementation references:

- Donut shader/tool organization: https://github.com/NVIDIA-RTX/Donut
- NVRHI shader package/runtime separation reference: https://github.com/NVIDIA-RTX/NVRHI

Implementation prompt:

```text
Refactor ShaderCompiler around explicit ShaderContracts: pass catalogs, package manifests, reflection schemas, backend compile options, deterministic job ids, inspection reports, and verification failures. ShaderCompiler may consume narrow shader registration/catalog targets but must not link full renderer runtime to discover packages.
```

Positive guardrails:

- Shader jobs are keyed by package id, backend format, compile options, profile, and generation.
- Package/reflection output is immutable after publish.
- Verification failures name package, entry point, stage, parameter, binding, expected layout, and reason.

Negative guardrails:

- Do not make ShaderCompiler depend on Renderer private runtime.
- Do not duplicate pass metadata between runtime and tooling without a removal stage.
- Do not let backend compiler details leak into runtime shader contracts.

Data transfer contracts:

- Shader contracts transfer pass catalogs and package manifests.
- ShaderCompiler transfers package/reflection/inspection reports to runtime consumers.
- Renderer pipeline runtime consumes packages and reflection, not compiler internals.

Acceptance:

- ShaderCompiler package enumeration is contract-based and deterministic.
- ShaderCompiler validation can run without building the editor.
- Renderer runtime and ShaderCompiler agree on package/reflection identity.

Validation:

- Build `ShaderCompiler`.
- Run `list-shaders`, package validation, and any available inspect command.

## Stage 30 - AssetCooker, Launcher Workflow, And Host Boundary Refactor

Goal:

- Make AssetCooker, launcher, editor, and application hosts reliable orchestration layers rather than hidden owners of cook/render/backend behavior.
- Retire `AssetConverter` as a production path by folding useful behavior into AssetCooker or explicit read-only inspection/debug commands.

Source references:

- `tooling-pipeline-contract.md`
- `Tools/Cooking/AssetCooker`
- `Tools/Conversion/AssetConverter`
- `Tools/Launcher/SparkleLauncher`
- `Engine/Application`
- `Engine/Editor`

Target shape references:

- Stage target-doc checklist: [Required Target Documents By Stage](#required-target-documents-by-stage), Stage 30 row.
- Primary target docs: [tooling-pipeline-contract.md](../architecture/tooling-pipeline-contract.md), [repository-system-map.md](../architecture/repository-system-map.md), [repository-target-architecture.md](../architecture/after/repository-target-architecture.md), [architecture-boundary-guardrails.md](../architecture/architecture-boundary-guardrails.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md)

Implementation prompt:

```text
Refactor AssetCooker, AssetConverter, SparkleLauncher, Application, and Editor host boundaries. AssetCooker owns project discovery, cook planning, dispatch, process/library isolation, aggregation, and reports. Focused cookers own algorithms. AssetConverter is not a production cook path; useful behavior becomes AssetCooker subcommands or explicit read-only inspection/debug commands. LauncherCore owns build/cook/launch/maintenance process orchestration and evidence. Qt GUI owns presentation, models, prompts, and action history. Application/Editor orchestrate systems without owning backend-native or cook/import implementation details.
```

Positive guardrails:

- `AssetCooker` owns discovery, planning, dispatch, process isolation, aggregation, and actionable diagnostics.
- `SparkleLauncherCore` owns workflow catalogs, tool resolution, process requests, process runner integration, and operation history.
- `SparkleLauncher` Qt GUI owns models, widgets, prompts, shell, style, and presentation.
- Application/Editor own lifecycle and UI orchestration through public Renderer/GameFramework/RHI contracts.
- AssetConverter is retired as a production path; useful commands become AssetCooker subcommands or explicit inspect/debug commands.

Negative guardrails:

- Do not let Qt widgets invoke cooker/compiler internals directly.
- Do not let LauncherCore duplicate focused tool algorithms.
- Do not let AssetCooker reimplement focused cooker algorithms.
- Do not keep AssetConverter as a second production cook path.
- Do not let Application or Editor include tool-private cook/import headers.
- Do not let Application validation grow new D3D12/Vulkan-native implementation code.

Data transfer contracts:

- AssetCooker transfers work as process/library dispatch requests and reports source path, asset id, target profile, output path, step, and reason.
- Launcher GUI transfers user intent to LauncherCore as operation requests.
- LauncherCore transfers work to CMake/tools/runtime hosts as process requests with arguments, environment, working directory, and expected artifacts.
- Application/Editor transfer validation evidence as logs, captures, and smoke status, not backend-native objects.

Acceptance:

- AssetCooker owns discovery, planning, dispatch, process isolation, and diagnostics.
- AssetConverter is retired as a production path.
- Launcher UI models and widgets do not duplicate operation logic.
- LauncherCore invokes tools/processes and records actionable evidence.
- Application validation does not grow new backend-native dependencies.
- Editor does not include tool-private cook/import internals.

Validation:

- Build `SparkleLauncher` and `SparkleLauncherProbe` when a compiler/Qt are available.
- Build `AssetCooker` and any affected inspect/debug command.
- Run launcher workflow inspection or smoke command where available.

## Stage 31 - Shader And Cook Artifact Validation Matrix

Goal:

- Ensure shader packages, cooked textures, cooked scenes, and runtime loaders stay compatible as renderer/RHI contracts change.

Source references:

- `ShaderCompiler`
- `TextureCooker`
- `AssetCooker`
- `GameFramework` cooked loaders
- `Renderer` texture/material/mesh/scene data paths

Target shape references:

- Stage target-doc checklist: [Required Target Documents By Stage](#required-target-documents-by-stage), Stage 31 row.
- Primary target docs: [tooling-pipeline-contract.md](../architecture/tooling-pipeline-contract.md), [game-framework-contract.md](../architecture/game-framework-contract.md), [pass-authoring-contract.md](../architecture/pass-authoring-contract.md), [pipeline-runtime-contract.md](../architecture/pipeline-runtime-contract.md), [system-design-index.md](../architecture/after/system-design-index.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md)

Implementation prompt:

```text
Create a validation matrix that maps every produced artifact type to its producer, schema owner, runtime consumer, inspection command, and smoke/load evidence. Include shader packages, cooked textures, cooked materials, cooked meshes, cooked scene manifests, animations, skeletons, and project cook plans.
```

Positive guardrails:

- Treat every artifact as a contract with producer, schema owner, consumer, inspector, and validation evidence.
- Prefer inspection/list commands over manual binary inspection.
- Tie artifact validation to Showcase or another named sample path when possible.

Negative guardrails:

- Do not change cooked schemas without updating producer, loader, renderer consumer, and inspector together.
- Do not accept "builds successfully" as artifact compatibility evidence.
- Do not let shader packages, cooked assets, or project cook plans have undocumented ownership.

Data transfer contracts:

- ShaderCompiler transfers shader packages, reflection, `ShaderContracts` pass catalog/manifest data, and inspection reports to RHI/Renderer runtime consumers.
- Cookers transfer cooked textures/materials/meshes/scenes/animations/skeletons to GameFramework/Renderer/RHI runtime consumers.
- The validation matrix must name artifact path pattern, schema/version owner, inspection command, failure diagnostics, and sample smoke/load evidence.

Acceptance:

- Every cooked artifact type has producer, schema owner, consumer, and validation evidence.
- ShaderCompiler package enumeration includes renderer packages.
- Texture/material/mesh/scene cooker outputs are tied to runtime loader expectations.

Validation:

- `ShaderCompiler` list/inspect/cook commands.
- `TextureCooker` inspect/cook request commands.
- `AssetCooker` plan/dispatch output.
- Runtime/editor smoke that loads cooked sample content.

## Stage 32 - Projects, Engine Assets, And Sample Evidence Refactor

Goal:

- Make project content and built-in engine assets part of the architecture instead of unowned files that tests happen to use.
- Separate engine built-in assets, project sample data, project shaders, validation artifacts, and generated outputs.

Source references:

- `repository-coverage-status.md`: `Engine/Assets`, `Projects`
- `repository-target-folder-architecture.md`
- `tooling-pipeline-contract.md`
- `validation-workflow-contract.md`

Target shape references:

- Stage target-doc checklist: [Required Target Documents By Stage](#required-target-documents-by-stage), Stage 32 row.
- Primary target docs: [repository-system-map.md](../architecture/repository-system-map.md), [repository-target-folder-architecture.md](../architecture/after/repository-target-folder-architecture.md), [validation-workflow-contract.md](../architecture/validation-workflow-contract.md), [tooling-pipeline-contract.md](../architecture/tooling-pipeline-contract.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md)

External implementation references:

- Donut Samples as executable sample evidence: https://github.com/NVIDIA-RTX/Donut-Samples
- Cauldron sample/project framework: https://github.com/GPUOpen-LibrariesAndSDKs/Cauldron

Implementation prompt:

```text
Refactor Projects and Engine/Assets ownership so built-in engine assets, project data, project shaders, validation scenes, generated artifacts, and sample outputs have explicit owners and folder policies. Showcase and future sample projects should act as validation evidence for cook/load/render/launcher workflows, not as hidden production data stores. Move or document ambiguous Engine/Assets content into owner-specific roots when needed.
```

Positive guardrails:

- Project content lives under `Projects/<Project>/Data` or `Projects/<Project>/Shaders` when project-owned.
- Engine built-in assets have a manifest, owner, and validation path.
- Validation artifacts stay under `artifacts/validation`, not durable source roots.
- Showcase remains a reusable evidence project for cook/load/render/launcher workflows.

Negative guardrails:

- Do not mix generated outputs with durable project/source assets.
- Do not use `Engine/Assets` as a catch-all for renderer shaders, sample data, or temporary artifacts.
- Do not let Projects depend on tool-private or backend-private implementation details.

Data transfer contracts:

- Projects transfer source content to tools through project cook plans and tool job requests.
- Cooked outputs transfer to runtime through versioned artifacts.
- Validation artifacts transfer through logs, reports, captures, and index files.

Acceptance:

- `Engine/Assets` has a narrow documented role or its ambiguous content has moved to owner roots.
- Projects have data/shader/generated-output policies.
- Showcase validation content is tied to Stage 31/34 evidence.

Validation:

- Inventory project and engine asset roots.
- Run targeted cook/load/smoke if content moved.

## Stage 33 - Build, CI, And Boundary Guardrail Expansion

Goal:

- Extend mechanical checks beyond RHI/Renderer so future changes cannot quietly reintroduce tool/runtime or launcher/cooker coupling.

Source references:

- `CMake/ArchitectureBoundaryCheck.cmake`
- `repository-system-map.md`
- `repository-coverage-status.md`
- `repository-target-folder-architecture.md`
- `.github`
- `CMake`

Target shape references:

- Stage target-doc checklist: [Required Target Documents By Stage](#required-target-documents-by-stage), Stage 33 row.
- Primary target docs: [architecture-boundary-guardrails.md](../architecture/architecture-boundary-guardrails.md), [repository-system-map.md](../architecture/repository-system-map.md), [repository-coverage-status.md](../architecture/repository-coverage-status.md), [repository-target-folder-architecture.md](../architecture/after/repository-target-folder-architecture.md), [repository-target-architecture.md](../architecture/after/repository-target-architecture.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md)

Implementation prompt:

```text
Add or extend local/CI-friendly checks for runtime-to-tools dependencies, tool-private include leaks, GameFramework-to-Renderer private coupling, launcher-to-cooker implementation coupling, generated/local-only folder policy, and target folder ownership. Keep exceptions narrow, counted, documented, and stage-labeled. If moving build-support files improves navigation, migrate CMake checks toward CMake/Checks while preserving a documented local command.
```

Positive guardrails:

- Extend `CMake/ArchitectureBoundaryCheck.cmake` or equivalent local scripts with actionable checks.
- Encourage allowed edges: runtime to public runtime contracts, tools to public schemas, LauncherCore to process requests, Qt GUI to LauncherCore APIs, CMake to explicit target usage requirements.
- Encourage target folder edges from `repository-target-folder-architecture.md`: contracts roots for shared schemas, backend sibling folders, owner-specific shader/data folders, and explicit tool role folders.
- Keep checks runnable without building the editor.

Negative guardrails:

- Do not add broad allowlists such as all of `Tools` or all of `Engine`.
- Do not suppress generated/third-party violations by path unless that path is documented as generated or third-party source policy.
- Do not make CI the only way to run the checks locally.
- Do not create checks that only report "failed" without file path and reason.
- Do not add source folders in generated/local roots or let `tmp_*`, `build*`, `artifacts`, `dist`, or `logs` become durable architecture.
- Do not move checks into a folder that hides local usage behind CI-only behavior.

Data transfer contracts:

- Boundary-check data transfers as rule id, file path, forbidden pattern, reason, transitional exception count, and removal stage.
- CMake target ownership transfers through target link scopes and named target dependencies.
- CI/local workflow data transfers through documented commands and generated validation reports.
- Folder policy transfers through source-root allow/deny rules, target path records, owner names, and generated/local-only exclusions.

Acceptance:

- Boundary checks cover RHI/Renderer plus at least runtime-to-tools and GameFramework/launcher/tool ownership.
- Checks or docs enforce target folder ownership for new roots and generated/local-only roots.
- Checks report actionable file paths and reasons.
- CI or documented local commands run the checks without building the editor.

Validation:

- Run boundary checks.
- Run CMake configure when compiler/toolchain is available.

## Stage 34 - Whole-Repository Evidence Gate

Goal:

- Score SparkleEngine as a cohesive repository, not only a renderer/RHI track.

Source references:

- `architecture-review-acceptance-rubric.md`
- `repository-coverage-status.md`
- `rendering-coverage-status.md`
- `repository-target-folder-architecture.md`
- All architecture docs

Target shape references:

- Stage target-doc checklist: [Required Target Documents By Stage](#required-target-documents-by-stage), Stage 34 row.
- Primary target docs: [architecture-review-acceptance-rubric.md](architecture-review-acceptance-rubric.md), [repository-coverage-status.md](../architecture/repository-coverage-status.md), [rendering-coverage-status.md](../architecture/rendering-coverage-status.md), [repository-target-architecture.md](../architecture/after/repository-target-architecture.md), [repository-target-graphs.md](../architecture/after/repository-target-graphs.md), [repository-target-folder-architecture.md](../architecture/after/repository-target-folder-architecture.md), [system-design-index.md](../architecture/after/system-design-index.md), [architecture-boundary-guardrails.md](../architecture/architecture-boundary-guardrails.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md)

Implementation prompt:

```text
Run the repository-wide architecture evidence gate before the final threading-readiness audit. Verify coverage maps, target folder architecture, boundary checks, build/tool validation, launcher workflows, cooked artifact compatibility, renderer/RHI backend evidence, docs, and known risks. Do not call the repository evidence-ready while any source root has unowned risk, ambiguous folder ownership, stale migration folders, contradictory docs, or unresolved handoff shape required by Stage 35.
```

Positive guardrails:

- Score the whole repository, not only RHI/Renderer.
- Require final evidence for GameFramework, Launcher, ShaderCompiler, AssetCooker, TextureCooker, SourceImporters/current SourceImportAdapters, CMake, CI/local checks, Projects, and docs.
- Require final folder evidence for contract roots, backend roots, renderer pass/shader roots, tool role roots, project data/shader roots, CMake checks, and generated/local-only exclusions.
- Keep known non-blocking risks visible with owner, reason, and follow-up stage or issue.

Negative guardrails:

- Do not call the repo review-ready while any source root has unowned `Needs refactor` risk.
- Do not hide contradictory docs to make the final gate pass.
- Do not accept private includes, broad allowlists, duplicated pipelines, or temporary adapters as final architecture.
- Do not claim performance, parity, or reliability improvements without evidence.
- Do not accept old and new folders as parallel production paths.
- Do not accept ambiguous roots such as unqualified `Engine/Assets`, `Tools/Common`, `Tools/Conversion`, or generic helper folders unless their owner, contract, validation value, and smaller alternative are documented.

Data transfer contracts:

- Final architecture state transfers through coverage maps, graph pages, contracts, stage evidence, validation artifacts, and rubric scoring.
- Final validation transfers through command logs, tool inspection output, launcher workflow evidence, cooked artifact load evidence, smoke captures, and boundary reports.
- Final folder state transfers through source-root inventory, target-folder comparison, CMake target list, and generated/local-only root audit.
- Every final claim must name the producing command/doc, consuming architecture evidence path, and remaining risk owner if incomplete.

Acceptance:

- No unowned `Needs refactor` rows remain in repository or rendering coverage maps.
- RHI/Renderer final definition is satisfied.
- GameFramework, Launcher, ShaderCompiler, AssetCooker, TextureCooker, source import, CMake, CI, Projects, and docs all have final evidence or documented non-blocking risks.
- Folder architecture matches the target design or has a stricter documented alternative with no duplicate production paths.
- Architecture evidence points to whole-repo architecture, not only rendering internals.

Validation:

- Boundary checks.
- Smallest meaningful target builds: `ShaderCompiler`, `AssetCooker`, `TextureCooker`, `SparkleLauncher`, affected runtime/editor target.
- Targeted sample cook/load/smoke where available.

## Stage 35 - Threading Readiness Final Audit

Goal:

- Triple-check that every engine module, tool, subsystem, contract, graph, folder, and implementation prompt is shaped for future multithreading without implementing multithreading prematurely.
- Ensure future render-thread, command-recording, async queue, cook-job, shader-job, and launcher workflow work can consume explicit handoff data rather than private mutable state.

Source references:

- `repository-threading-readiness.md`
- `repository-target-architecture.md`
- `repository-target-graphs.md`
- `system-design-index.md`
- `architecture-review-acceptance-rubric.md`
- All architecture contracts and stage evidence

Target shape references:

- Stage target-doc checklist: [Required Target Documents By Stage](#required-target-documents-by-stage), Stage 35 row.
- Primary target docs: [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md), [repository-target-architecture.md](../architecture/after/repository-target-architecture.md), [repository-target-graphs.md](../architecture/after/repository-target-graphs.md), [system-design-index.md](../architecture/after/system-design-index.md), [architecture-review-acceptance-rubric.md](architecture-review-acceptance-rubric.md), [repository-refactor-stage-map.md](after/repository-refactor-stage-map.md)

External implementation references:

- NVIDIA Donut threaded rendering sample: https://github.com/NVIDIA-RTX/Donut-Samples/tree/main/examples/threaded_rendering
- NVIDIA Donut thread pool: https://github.com/NVIDIA-RTX/Donut/blob/main/include/donut/engine/ThreadPool.h
- AMD Cauldron thread pool and command-list rings: https://github.com/GPUOpen-LibrariesAndSDKs/Cauldron
- Diligent Engine Tutorial 06 multithreading: https://github.com/DiligentGraphics/DiligentSamples/tree/master/Tutorials/Tutorial06_Multithreading
- Diligent Engine Tutorial 23 command queues: https://github.com/DiligentGraphics/DiligentSamples/tree/master/Tutorials/Tutorial23_CommandQueues
- NVIDIA async compute and overlap guidance: https://developer.nvidia.com/blog/advanced-api-performance-async-compute-and-overlap/

Implementation prompt:

```text
Run a repository-wide threading-readiness audit without adding a job system or new worker-thread implementation. For every durable module and every stage-owned target, verify mutable-state owner, execution phase, handoff shape, isolation scope, ordering/synchronization expectation, diagnostics identity, and deterministic-output expectation. Update docs, stage evidence, and guardrails when an edge would force future workers, render threads, async queues, cook jobs, shader jobs, or launcher workflows to read private mutable state. Prefer redesigning the data shape over adding locks or generic schedulers.
```

Positive guardrails:

- Keep current serial execution where it is simpler and sufficient.
- Prefer immutable snapshots, DTOs, manifests, command batches, queue packets, tool job requests, process requests, and reports.
- Require frame/job/package/artifact ids, generations, queue/fence/order rules, and report paths where future parallelism is expected.
- Preserve central RHI submission ownership while allowing future worker-side command recording through explicit command batches.
- Preserve LauncherCore as workflow/process owner and Qt GUI as UI presentation owner.

Negative guardrails:

- Do not add a global job system, render thread, async compute scheduler, or worker pool just to satisfy this stage.
- Do not declare a design threading-ready because shared objects are locked.
- Do not let future workers require `Engine/Renderer/Private`, `Engine/RHI/Private`, `Tools/*` implementation, backend-native headers, or Qt widget internals across owner boundaries.
- Do not accept nondeterministic cook/shader/package output ordering.
- Do not claim async compute or transfer performance benefit without measurement plan and queue/fence/resource hazard evidence.

Data transfer contracts:

- Runtime-to-renderer data transfers through immutable `RenderContracts` snapshots.
- Renderer-to-RHI recording transfers through frame graph plans, command batches, RHI descriptors, and queue submission packets.
- Shader/pass data transfers through `ShaderContracts` package manifests, pass catalogs, reflection, and explicit PSO keys.
- Tool work transfers through imported DTOs, cooked artifacts, tool job requests, temp/final artifact policy, reports, and inspection commands.
- Launcher work transfers through process requests, operation reports, and history records.

Acceptance:

- `repository-threading-readiness.md` covers all durable modules and tool areas.
- Every stage in this plan references the threading-readiness target doc.
- Target graphs show future execution lanes and controlled handoff shapes.
- Detailed contracts for RHI, Renderer/frame graph/pass/pipeline/ray tracing, GameFramework, and tooling name threading-ready ownership rules.
- No final architecture edge requires future workers to read producer-private mutable state.
- Stage status map includes this audit and names remaining non-blocking threading-readiness risks, if any.

Validation:

- Docs/link scan.
- Boundary check.
- No full build required unless this audit discovers code/CMake changes.

## Stage 36 - Final Whole-Repository Cleanup, Rubric Scoring, And Review-Ready Gate

Goal:

- Finish the whole-repository refactor cleanly.
- Delete stale transition paths across engine, tools, launcher, projects, build, CI, and docs.
- Score SparkleEngine as one cohesive repository, not a graphics track plus unreviewed surroundings.

Source references:

- `architecture-review-acceptance-rubric.md`
- `repository-coverage-status.md`
- `rendering-coverage-status.md`
- `repository-refactor-stage-map.md`
- All Stage 1-35 completion evidence

Target shape references:

- Stage target-doc checklist: [Required Target Documents By Stage](#required-target-documents-by-stage), Stage 36 row.
- Primary target docs: [architecture-review-acceptance-rubric.md](architecture-review-acceptance-rubric.md), [repository-coverage-status.md](../architecture/repository-coverage-status.md), [rendering-coverage-status.md](../architecture/rendering-coverage-status.md), [system-design-index.md](../architecture/after/system-design-index.md), [repository-refactor-stage-map.md](after/repository-refactor-stage-map.md), [repository-threading-readiness.md](../architecture/after/repository-threading-readiness.md)

External implementation references:

- arc42 quality and risk documentation: https://arc42.org/overview
- ADR practice: https://adr.github.io/
- CMU SEI ATAM: https://www.sei.cmu.edu/library/architecture-tradeoff-analysis-method-collection/

Implementation prompt:

```text
Perform the final whole-repository cleanup and review-ready scoring pass. Delete stale adapters, duplicate production paths, broad compatibility shims, obsolete docs, dead CMake targets, stale generated-root references, and transitional boundary exceptions across Engine, Tools, Projects, CMake, CI, and docs. Score every acceptance rubric category for the repository as a whole. Do not call the repository review-ready unless Core, Platform, RHI, Renderer, GameFramework, Editor/Application, Launcher, ShaderCompiler, importers, cookers, AssetCooker, Projects, build, CI, docs, and threading-readiness evidence all agree.
```

Positive guardrails:

- Treat cleanup as mandatory architecture work.
- Prefer deleting duplicate paths over documenting them as harmless.
- Keep known remaining risks visible with owner, severity, reason, and follow-up.
- Score the whole repository using evidence, not memory.

Negative guardrails:

- Do not leave old and new production paths side by side.
- Do not hide weak module scores behind strong RHI/Renderer scores.
- Do not keep an ambiguous root, broad helper folder, or compatibility shim without accepted evidence that it earns its right to exist.

Data transfer contracts:

- Final acceptance transfers through an evidence index, coverage maps, rubric score table, validation commands, logs, captures, tool reports, and known-risk table.
- Cleanup decisions transfer through docs/ADR-style notes when a path is intentionally removed or a risk is accepted.
- No final score may rely on private reviewer memory.

Acceptance:

- No unowned `Needs refactor` rows remain in repository or rendering coverage maps.
- No duplicate production path remains for shader packages, source import, focused cooking, asset cooking, launcher workflows, renderer presentation, RHI services, or project artifacts.
- Boundary checks pass and all transitional exceptions are either removed or accepted with owner and reason.
- Rubric critical categories score 3 or have explicit accepted exceptions; no category scores below 2.
- Architecture evidence points to whole-repo architecture, validation evidence, known risks, and final target graphs.

Validation:

- Boundary checks.
- Smallest meaningful target builds across graphics, tools, launcher, and sample project.
- Artifact validation matrix commands from Stage 31.
- Final docs/link/stale-text scan.

## Final Review-Ready Definition

SparkleEngine is review-ready when all of these are true:

- RHI does not depend on Renderer.
- Renderer does not own backend-native API details outside documented provider integration.
- Application does not own backend-native validation/capture implementation.
- Adding an ordinary renderer shader pass does not require RHI edits.
- `Renderer` is a facade/host boundary, not a giant orchestration hub.
- `RenderHardwareInterface` is categorized and slimmed behind service responsibilities.
- Frame graph contract failures are actionable and fail development smoke.
- PSO creation is keyed, logged, and backend-normalized.
- D3D12 and Vulkan backend services are separated, symmetric where appropriate, and parity-tested.
- Ray tracing ownership is clear from scene data to TLAS binding and pass usage.
- DLSS/upscaling interop is provider-owned and backend-supported through explicit contracts.
- Validation artifacts prove lit and debug/normal parity across D3D12 and Vulkan.
- GameFramework remains cooked-data/runtime-scene oriented and does not absorb source import, cook algorithms, renderer pass policy, or backend-native RHI objects.
- Source import, focused cooking, project-level cook orchestration, shader compilation, and debug conversion have explicit owners and validation evidence.
- Launcher workflows are process/evidence orchestration over focused tools, with Qt UI separated from operation logic.
- Runtime engine modules do not depend on tool internals.
- Cooked artifact schemas have producer, owner, consumer, and validation evidence.
- Future multithreading is already designed into the architecture: mutable state has phase owners, and parallel-ready handoffs use snapshots, DTOs, manifests, command batches, queue packets, job/process requests, and reports.
- CMake and CI/local checks make module dependencies and validation commands repeatable.
- Repository docs make the whole system navigable for an external graphics, engine, or tools reviewer.

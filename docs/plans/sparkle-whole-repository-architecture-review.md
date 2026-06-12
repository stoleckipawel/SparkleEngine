# Sparkle Whole-Repository Architecture Review

Status: strategic system-design review draft
Date: 2026-06-12
Scope: all durable SparkleEngine source roots under `Engine/`, `Tools/`, `Projects/`, `CMake/`, `.github/`, and `docs/`.

## Purpose

This document treats the RHI/Renderer work as one track inside a global repository refactor. The goal is to make SparkleEngine cohesive as an engine, renderer, runtime, content pipeline, and developer tooling repository without damaging adjacent modules while one subsystem is being improved.

This review complements:

- [rhi-renderer-architecture-review.md](rhi-renderer-architecture-review.md)
- [rhi-renderer-review-ready-implementation-plan.md](rhi-renderer-review-ready-implementation-plan.md)
- [architecture-review-acceptance-rubric.md](architecture-review-acceptance-rubric.md)
- [repository-system-map.md](../architecture/repository-system-map.md)
- [repository-coverage-status.md](../architecture/repository-coverage-status.md)
- [tooling-pipeline-contract.md](../architecture/tooling-pipeline-contract.md)
- [game-framework-contract.md](../architecture/game-framework-contract.md)

Generated or local-only folders are out of scope unless a stage explicitly audits generated output policy: `build/`, `cmake-build-debug/`, `artifacts/`, `dist/`, and `logs/`.

## Reference Model

The design target is calibrated against public NVIDIA, AMD, and adjacent primary sources. These are reference anchors, not templates to copy verbatim.

| Reference | Source | What the source demonstrates | Sparkle rule derived from it |
| --- | --- | --- | --- |
| NVIDIA NVRHI | https://github.com/NVIDIA-RTX/NVRHI | A focused rendering hardware interface over D3D11, D3D12, and Vulkan with resource states, barriers, bindings, pipelines, validation, and optional native API interaction. | `Engine/RHI` owns GPU/API concepts and backend services. It must not know renderer passes, GameFramework scenes, launcher workflows, or cook algorithms. |
| NVIDIA NRI | https://github.com/NVIDIA-RTX/NRI | A modular low-level rendering interface designed around D3D12/Vulkan features. | RHI service extraction should be based on method ownership and capability groups, not on convenience methods requested by renderer features. |
| NVIDIA Donut | https://github.com/NVIDIA-RTX/Donut | A rendering framework with reusable passes, scene/component support, app framework, and NVRHI as the lower graphics abstraction. | Renderer-owned passes and scene-to-render data should live above RHI. Host/application helpers should not become GPU backend policy. |
| NVIDIA Falcor | https://github.com/NVIDIAGameWorks/Falcor | A D3D12/Vulkan rendering framework with render graph, scene rendering, shader compilation, ray tracing, and RTX SDK integrations. | Sparkle should keep render graph, pass systems, shader tooling, scene data, and vendor providers reviewable as separate systems. |
| NVIDIA Streamline | https://github.com/NVIDIA-RTX/Streamline | A vendor integration framework with general, manual-hooking, and per-feature programming guides plus plugin source. | DLSS and future vendor features belong in named provider layers. Backend-native data flows through explicit RHI interop contracts. |
| AMD Cauldron | https://github.com/GPUOpen-LibrariesAndSDKs/Cauldron | A rapid prototyping framework for Vulkan and DirectX 12 with visible API-specific build/output separation and glTF-oriented sample features. | D3D12 and Vulkan code should stay backend-private and symmetric where useful. Sample/project validation should exercise real content paths. |
| AMD FidelityFX SDK | https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK | AMD effect SDK and samples for DirectX 12/Vulkan applications, with explicit feature/API support status. | Upscaling/denoising/provider integrations must have capability reports, backend support notes, and deterministic fallback reasons. |
| AMD Compressonator | https://github.com/GPUOpen-Tools/compressonator | Texture and mesh optimization tool suite with GUI, CLI, and SDK integration surfaces. | `TextureCooker` and related cook tools should stay focused and callable; Launcher should orchestrate them, not duplicate compression/import logic. |
| CMake target usage requirements | https://cmake.org/cmake/help/latest/command/target_link_libraries.html | `PUBLIC`, `PRIVATE`, and `INTERFACE` link scopes encode dependency propagation. | CMake links are architecture. Incorrect `PUBLIC`/`PRIVATE` scope is a design bug, not only a build detail. |
| Qt model/view programming | https://doc.qt.io/qt-6/model-view-programming.html | Qt separates models, views, and delegates to decouple data from presentation. | `SparkleLauncherCore` owns operation state and process requests; Qt widgets own presentation and prompts. |
| Khronos glTF 2.0 | https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html | API-neutral runtime asset delivery from authoring tools to graphics applications. | Source import should produce DTOs and diagnostics; runtime should load cooked/runtime artifacts, not source authoring formats. |
| Khronos KTX-Software | https://github.com/KhronosGroup/KTX-Software | Texture container/library/tools for GPU API texture loading. | Cooked texture artifacts need explicit schema, format, mip, and upload contracts between tools, GameFramework, Renderer, and RHI. |

## Repository-Wide Design Target

Sparkle should have one visible ownership story:

```text
Core -> Platform -> RHI -> Renderer -> GameFramework -> Editor/Application

Tools produce and validate artifacts for runtime modules.
Runtime modules consume public cooked/runtime contracts.
Launcher orchestrates workflows and records evidence.
CMake and CI make dependency direction repeatable.
Docs explain the exact current architecture and accepted debt.
```

The global target is not "no dependencies." The target is known dependencies with a reason, owner, and validation artifact.

## Whole-Repository Findings

### Strengths To Preserve

- The repository already has meaningful top-level module boundaries: `Core`, `Platform`, `RHI`, `Renderer`, `GameFramework`, `Editor`, `Application`, focused cooking tools, shader compiler, and launcher.
- Stage 4 removed the clearest hard violation: RHI-owned renderer shader registration depending on renderer-private data.
- `Tools/Launcher/SparkleLauncher` already shows a useful split between core workflow code, GUI app/models/shell/widgets, and probe executable.
- Cooking is already split into focused tools: `TextureCooker`, `MeshCooker`, `MaterialCooker`, `SceneCooker`, `AssetCooker`, and `CookCommon`.
- `ShaderCompiler` already has backend, CLI, cooking, cache, inspection, verification, and reflection areas that can become strong reviewer evidence.
- CMake target names mostly reflect architectural modules, which gives us a practical place to enforce dependency intent.

### Critical Risks

| Risk | Current pressure point | Why it matters | Target response |
| --- | --- | --- | --- |
| Local refactor damage | RHI/Renderer changes can affect `ShaderCompiler`, `TextureCooker`, GameFramework loaders, launcher smoke, and editor viewport behavior. | A renderer fix can silently break cook/runtime or validation paths. | Every stage needs a blast-radius row that names affected tools and runtime consumers. |
| Tool/runtime leakage | Runtime modules must not include `Tools/*`, while tools may need public runtime schemas. | Source import and cooked loading become unreviewable if both sides know private internals. | Public schemas and DTOs become the contract; private loaders and cook algorithms remain private. |
| GameFramework/Renderer coupling | Renderer currently consumes GameFramework scene/camera/level data. | Renderer refactors should not mutate gameplay/runtime scene ownership. | Move toward immutable render snapshots and explicit schema versioning for cooked assets. |
| RHI catch-all growth | `RenderHardwareInterface` can absorb renderer convenience, capture, interop, and diagnostics pressure. | A broad facade hides ownership and backend parity obligations. | Classify methods by service and extract only where caller evidence supports it. |
| Vendor integration sprawl | Streamline/DLSS and future FSR/NRD-style providers need native handles and backend capability data. | Vendor SDK code can leak into RHI policy or ordinary pass code. | Provider-specific folders request explicit RHI interop data; backends report capability/fallback reasons. |
| Cooked artifact drift | Shader, texture, material, mesh, scene, animation, and skeleton artifacts are produced by tools and loaded by runtime. | Schema drift creates late runtime failures. | Stage 27 artifact matrix maps producer, schema owner, consumer, inspector, and smoke evidence. |
| Launcher responsibility creep | Launcher can become a second implementation of build/cook/launch logic. | A developer tool loses reliability if UI and operation logic duplicate each other. | LauncherCore plans and runs processes; GUI observes and presents state. |
| Build graph drift | CMake `PUBLIC`/`PRIVATE` links can expose private implementation dependencies. | Transitive links can hide architecture violations. | Stage 28 expands checks to runtime-to-tools and target-scope policy. |
| CI/documentation lag | Docs and CI can continue describing a previous architecture. | Reviewers lose trust if plans, checks, and code disagree. | Stage 29 requires docs, coverage maps, checks, and validation artifacts to agree. |

## Module Review Matrix

| Area | Current role | Global refactor risk | Reference basis | Target acceptance evidence |
| --- | --- | --- | --- | --- |
| `Engine/Core` | Foundation diagnostics, files, strings, math, events, process helpers, JSON, console, time, input value types. | Foundation can absorb renderer, platform, or tool policy because every module can see it. | CMake target usage requirements for minimal propagation. | Include/dependency scan shows no RHI/Renderer/GameFramework/Tools policy leaks. |
| `Engine/Platform` | OS/window/input platform layer. | Platform can become host/editor or renderer presentation policy. | Donut app/device-manager split as a cautionary host boundary. | Platform owns OS integration only; renderer/editor behavior stays above it. |
| `Engine/RHI` | GPU/API contract, D3D12/Vulkan backends, shader package primitives, resource/descriptor/command/pipeline services. | Root facade and backend implementations can hide renderer convenience methods. | NVRHI, NRI, Cauldron, Diligent-style backend separation. | RHI method ownership table, backend-private include checks, no `Renderer/Private` includes. |
| `Engine/Renderer` | Frame graph, passes, shader registrations, PSO runtime, textures, meshes, scene data, ray tracing, upscaling. | Renderer can include backend-native details or mutate GameFramework ownership. | Donut renderer/pass layering, Falcor render graph/pass/scene separation. | Renderer has no backend-private headers outside documented provider paths; pass additions require no RHI edit. |
| `Engine/GameFramework` | Runtime levels, scenes, components, cameras, lighting, cooked asset loading, gameplay-facing data. | Can absorb source import/cook algorithms or renderer pass data. | Donut scene component boundary, glTF runtime delivery model. | GameFramework loads cooked data and emits immutable render snapshots; no `Tools/*` or renderer-private dependencies. |
| `Engine/Editor` | Editor panels, viewport UI, profiler/output/material/mesh/scene inspectors. | Editor can duplicate cook/import or backend validation logic. | Qt model/view and host/presentation separation. | Editor consumes public Application/Renderer/GameFramework protocols only. |
| `Engine/Application` | Runtime/editor host lifecycle, validation orchestration, runtime console, shader recook process bridge. | Existing validation debt uses backend-native D3D12 capture. | NVRHI validation/capture ownership, Vulkan validation tooling. | Application orchestrates validation; backend-native capture/readback lives behind RHI/backend services. |
| `Engine/Assets` | Source/default content and shader assets. | Content root can become an implicit schema owner without docs. | glTF and KTX explicit asset/container specs. | Asset root has documented source/cooked/generated policy and sample validation coverage. |
| `Tools/Shaders/ShaderCompiler` | Shader backend selection, preprocessing, reflection, package cooking, cache, inspection, verification. | Can become tightly linked to full renderer runtime instead of a narrow registry/schema contract. | Falcor shader/tooling productivity model, Donut ShaderMake usage, NVRHI shader package/runtime concepts. | `ShaderCompiler` lists/cooks/inspects renderer packages via renderer-owned registration target and generic RHI shader primitives. |
| `Tools/Import/SourceImportAdapters` | glTF/FBX/source scene import into imported DTOs and diagnostics. | Source format assumptions can leak into runtime loaders. | glTF runtime asset delivery, Cauldron glTF sample pipeline. | Import adapters emit DTOs and diagnostics only; runtime modules never parse source formats directly. |
| `Tools/Cooking/TextureCooker` | Source image loading, texture pipeline stages, compression policy, cooked texture emission. | RHI texture upload contracts and cooked texture schema can drift. | AMD Compressonator CLI/SDK/tool model, KTX texture container model. | Texture cook output includes format/mip/schema diagnostics and is validated against runtime loader/upload expectations. |
| `Tools/Cooking/MeshCooker` | Imported mesh to cooked mesh conversion. | Mesh schema can drift from GameFramework loaders and Renderer mesh cache. | Cauldron/glTF mesh/material sample handling. | Cooked mesh records have producer, schema owner, runtime loader, renderer consumer, and sample validation. |
| `Tools/Cooking/MaterialCooker` | Imported material conversion and texture cook request generation. | Material schema can drift from renderer material cache and texture cooker. | glTF PBR material model and Cauldron material support. | Material cook output and texture requests are deterministic and inspectable. |
| `Tools/Cooking/SceneCooker` | Cooked scene manifest assembly for cameras, lights, instances, skeletons, animations, metadata. | Scene manifest changes can break GameFramework load and Renderer scene snapshots. | Donut scene/component graph and glTF scene delivery. | Scene manifests validate against GameFramework loaders and renderer scene-data builders. |
| `Tools/Cooking/AssetCooker` | Project discovery, cook planning, dispatch, diagnostics. | Can hide focused cooker failures or duplicate their algorithms. | Compressonator CLI/SDK split and CMake target separation. | AssetCooker orchestrates focused tools and reports source path, artifact path, step, target, and reason on failure. |
| `Tools/Cooking/CookCommon` | Shared tool console/helpers. | Common helper can become policy sink. | CMake `PRIVATE`/`PUBLIC` dependency discipline. | Remains small support library with no asset/shader policy. |
| `Tools/Conversion/AssetConverter` | Debug/direct conversion CLI. | Can become a parallel cook pipeline. | Tool CLI shells over focused libraries. | Thin debug shell or folded into AssetCooker; no unique production cook policy. |
| `Tools/Launcher/SparkleLauncherCore` | Build/cook/launch/maintenance workflow planning, tool resolution, process requests, history. | Can duplicate focused build/cook/render logic. | Qt model/view, Compressonator GUI/CLI/SDK separation. | LauncherCore plans and runs processes, captures evidence, and delegates implementation to owning tools. |
| `Tools/Launcher/SparkleLauncher` GUI | Qt presentation, models, shell, widgets, style, prompts. | UI can absorb operation logic and hide recovery paths. | Qt model/view separation. | GUI models present LauncherCore state and requests; widgets remain presentation. |
| `CMake` | Build profiles, dependencies, artifact contract, Qt discovery, release assembly, boundary checks. | Build links can silently encode wrong architecture. | CMake target usage requirements. | Target graph matches layer rules and validation targets exist for local/CI use. |
| `.github` | CI workflows. | CI can lag local checks. | Public SDK repository CI/release discipline. | CI or documented local equivalents run boundary, shader/tool, format, and selected build checks. |
| `Projects` | Showcase project and sample content. | Sample can drift from cook/runtime/render contracts. | Donut-Samples and Cauldron sample usage. | Showcase exercises runtime loading, cooking, rendering, and launcher workflows. |
| `docs` | Architecture plans, contracts, reviews, evidence map. | Docs can contradict final code after staged refactors. | arc42/ATAM/ADR traceability style. | Docs, coverage maps, implementation plan, and final validation evidence agree. |

## Global Refactor Invariants

These invariants apply to every stage, including RHI/Renderer-specific stages:

- Runtime engine modules do not include or link tool-private implementation.
- Tools may use public runtime/cooked contracts, but runtime modules must not depend on source import or cook algorithms.
- RHI owns GPU/API primitives and backend implementation. Renderer owns render intent, pass metadata, shader registrations, frame graph, and features.
- GameFramework owns runtime scene and cooked-data loading. It hands immutable snapshots/DTOs to Renderer.
- Application and Editor orchestrate systems and UI. They do not own backend-native capture/readback, cook/import implementation, or renderer internals.
- Launcher owns workflow orchestration and evidence. Focused tools own actual build/cook/shader/import algorithms.
- CMake dependency scopes must express ownership. A convenient transitive link is not acceptable if it hides a layer violation.
- Generated/local-only folders are not durable architecture sources.
- Every exception must be narrow, counted, stage-labeled, and removed by its owning stage.

## Cross-Module Impact Matrix

| When changing | Also inspect | Required evidence |
| --- | --- | --- |
| RHI resource, texture, or upload contracts | `TextureCooker`, `MaterialCooker`, GameFramework cooked texture loaders, Renderer texture manager, CMake links. | Schema/upload note, targeted cooker/runtime validation, boundary check. |
| RHI shader package, reflection, or binding layout | `ShaderCompiler`, renderer shader registrations, pass runtime, PSO key/runtime, editor shader views. | `ShaderCompiler` list/inspect/cook evidence and pass runtime validation. |
| Renderer pass, frame graph, or PSO runtime | `ShaderCompiler`, GameFramework snapshots, Application validation, launcher smoke, Projects/Showcase. | Boundary check, shader package enumeration, smoke or documented validation plan. |
| Renderer scene DTOs | GameFramework scene/camera/light/material/mesh snapshots, `SceneCooker`, editor scene panels. | Snapshot/schema compatibility note and targeted scene load/render validation. |
| GameFramework cooked schema | Source import adapters, focused cookers, AssetCooker, Renderer scene/material/mesh/texture consumers. | Producer/owner/consumer matrix update and sample cook/load evidence. |
| Source import DTOs | Mesh/Material/Scene cookers, AssetConverter, GameFramework loaders, docs. | Import diagnostics and targeted cook validation. |
| Texture/mesh/material/scene cookers | AssetCooker, Launcher workflows, GameFramework loaders, Renderer resource managers. | Focused tool build plus dispatch/log evidence. |
| Launcher workflow or tool names | CMake artifact contract, AssetCooker, ShaderCompiler, TextureCooker, Projects, docs. | LauncherCore/Probe or workflow inspection output. |
| CMake target links or dependency fetch | Every target that consumes the linked module, `.github`, launcher tool resolution. | Configure/build plan and target-scope review. |
| CI or docs | All validation commands and known generated/local-only paths. | Link/stale-text scan and final evidence index update. |

## Global Refactor Tracks

### Track A - Repository Vocabulary And Mechanical Guardrails

Goal: make layer direction, source-root ownership, generated-folder policy, and exceptions visible before moving more code.

Acceptance:

- `repository-system-map.md` and `repository-coverage-status.md` cover all durable roots.
- Boundary checks cover RHI/Renderer now and expand to runtime-to-tools, GameFramework, launcher/tool ownership, and generated/local-only policy.

### Track B - RHI/Renderer First Track

Goal: finish the current graphics architecture refactor without breaking shader tools, cooked assets, launcher smoke, GameFramework snapshots, or editor/application hosts.

Acceptance:

- RHI has no renderer-private dependencies.
- Renderer shader/pass additions do not require RHI edits.
- D3D12/Vulkan parity evidence remains tied to real tools and projects.

### Track C - GameFramework And Cooked Runtime Contracts

Goal: separate runtime scene/cooked loading from source import/cook algorithms while preserving renderer-facing data.

Acceptance:

- GameFramework has no renderer-private, RHI-backend-private, or tool-private dependencies.
- Cooked schema changes update producer, owner, consumer, inspector, and smoke evidence.

### Track D - Tooling And Content Pipeline

Goal: make SourceImportAdapters, focused cookers, AssetCooker, ShaderCompiler, AssetConverter, and CookCommon reviewable as a content toolchain.

Acceptance:

- Focused tools own transformations.
- AssetCooker orchestrates and reports.
- ShaderCompiler and cookers can validate artifacts without building the editor.

### Track E - Launcher And Host Applications

Goal: keep Launcher, Application, and Editor as orchestration/presentation layers.

Acceptance:

- LauncherCore invokes tools/processes and records evidence.
- Qt GUI remains presentation/model code.
- Application validation delegates backend-native work to RHI/backend services.

### Track F - Build, CI, Projects, And Docs

Goal: make build graph, CI, sample project, and docs enforce the architecture.

Acceptance:

- CMake scopes express ownership.
- CI or local equivalents run the boundary and tool validation checks.
- Showcase content exercises representative runtime/cook/render paths.
- Docs match the final code and known debt.

## Review-Ready Definition

SparkleEngine is globally architecture-review-ready when:

- Every durable source root has a named owner, allowed dependencies, forbidden dependencies, validation target, and acceptance evidence.
- No RHI/Renderer stage is accepted without checking GameFramework, tools, launcher, Application/Editor, CMake/CI, Projects, and docs for blast radius.
- Runtime modules do not depend on tool internals.
- Source import, focused cooking, shader compilation, project cook orchestration, runtime loading, and renderer resource creation are separate responsibilities.
- Vendor integrations are provider-owned and backend-supported through explicit capability/interop contracts.
- Build targets and CI/local checks make the architecture mechanically repeatable.
- The implementation plan links every stage to global safeguards, not only local rendering goals.

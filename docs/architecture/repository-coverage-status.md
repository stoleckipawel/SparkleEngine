# Repository Coverage Status

Status: whole-repository coverage baseline
Date: 2026-06-12
Last synchronized: 2026-06-13
Source plan: `docs/plans/rhi-renderer-review-ready-implementation-plan.md`
Source review: `docs/plans/sparkle-whole-repository-architecture-review.md`
Related detail: [Rendering coverage status](rendering-coverage-status.md)

## Purpose

This document extends the Renderer/RHI coverage map to every durable source root in SparkleEngine. It is a guardrail for large refactors: changing RHI or Renderer is not accepted if it silently degrades Launcher, ShaderCompiler, AssetCooker, TextureCooker, SourceImporters/current SourceImportAdapters, GameFramework, Editor/Application, build profiles, or project content.

Generated/local-only folders remain out of scope: `build/`, `build-*`, `cmake-build-debug/`, `artifacts/`, `dist/`, `logs/`, and local reference/scratch roots such as `tmp_*`.

## Inventory Snapshot

Inventory command:

```powershell
Get-ChildItem -Directory Engine,Tools | ForEach-Object { $count = (Get-ChildItem -Recurse -File $_.FullName -Include *.cpp,*.h,*.hpp,*.cxx,*.cc | Measure-Object).Count; "{0}: {1}" -f ($_.FullName.Replace((Resolve-Path .).Path + '\','')), $count }
```

Snapshot:

| Root | C/C++ files | Coverage owner |
| --- | ---: | --- |
| `Engine/Core` | 83 | Foundation runtime status row below. |
| `Engine/Platform` | 11 | Platform status row below. |
| `Engine/RHI` | 189 | [Rendering coverage status](rendering-coverage-status.md). |
| `Engine/Renderer` | 240 | [Rendering coverage status](rendering-coverage-status.md). |
| `Engine/GameFramework` | 185 | GameFramework rows below and [GameFramework contract](game-framework-contract.md). |
| `Engine/Editor` | 58 | Editor/Application host rows below. |
| `Engine/Application` | 32 | Editor/Application host rows below and rendering validation rows. |
| `Tools/Launcher` | 103 | Launcher rows below and [tooling contract](tooling-pipeline-contract.md). |
| `Tools/Shaders` | 110 | ShaderCompiler rows below and rendering shader coverage rows. |
| `Tools/Cooking` | 106 | Cook pipeline rows below. |
| `Tools/Import` | 85 | Source import rows below. |
| `Tools/Conversion` | 4 | AssetConverter row below. |
| `Engine/Assets` | 0 | Non-code asset root policy row below. Current subfolders include `Meshes`, `Shaders`, and `Textures`. |

## Status Legend

| Status | Meaning |
| --- | --- |
| `Accepted` | Current ownership appears aligned; later stages preserve the role, but may still rename or extract if the naming canon requires it. |
| `Needs refactor` | Known implementation, ownership, validation, or documentation gap. |
| `Needs design decision` | Ownership or policy question must be resolved before moving code. |

## Disposition Pass

This pass applies the target architecture's keep/improve/replace policy to the coverage map. It is the design answer to "what should survive?"

| Area | Disposition | Target decision |
| --- | --- | --- |
| `Engine/Core` | Keep and refine | Keep foundation code if it remains policy-free. |
| `Engine/Platform` | Improve and extract | Keep OS/window/input; push presentation and host behavior upward. |
| `Engine/RHI` | Improve and extract | Split broad facade into `RhiContracts` and focused services. |
| D3D12/Vulkan backend folders | Keep and refine | Preserve backend-private roots and improve service parity. |
| `Engine/Renderer` | Improve and extract | Keep renderer ownership, but split facade, frame pipeline, pass authoring, providers, and pipeline runtime. |
| Renderer central pass traits | Replace or redesign | Replace with `PassCatalog` and `PipelineRuntimeLibrary`. |
| `Engine/GameFramework` | Improve and extract | Keep runtime scene/cooked loading; move shared schemas to `AssetContracts` and renderer handoff to `RenderContracts`. |
| `Engine/Editor` | Improve and extract | Keep UI/panels; prevent backend/tool logic from entering editor code. |
| `Engine/Application` | Improve and extract | Keep orchestration; replace backend-native validation body with RHI/backend services. |
| `Engine/Assets` | Replace or redesign | Narrow to built-in engine assets with manifest/validation, or move shaders/data to owner-specific roots. |
| `Tools/Launcher/SparkleLauncherCore` | Improve and extract | Keep workflow planning/process execution; route handoffs through `ToolContracts`. |
| SparkleLauncher Qt GUI | Keep and refine | Preserve presentation/model split. |
| `Tools/Shaders/ShaderCompiler` | Improve and extract | Consume `ShaderContracts`, not renderer runtime. |
| `Tools/Import/SourceImportAdapters` | Improve and extract | Rename/extract target to `SourceImporters`. |
| Focused cookers | Keep and refine | Preserve focused cookers and tighten artifact contracts. |
| `Tools/Cooking/AssetCooker` | Improve and extract | Keep orchestration only. |
| `Tools/Conversion/AssetConverter` | Replace or redesign | Fold into AssetCooker or explicit inspect/debug commands. |
| `Tools/Cooking/CookCommon` | Improve and extract | Rename/split to `ToolConsoleSupport` and/or `CookDiagnostics`. |
| `CMake` | Improve and extract | Make target scopes enforce architecture. |
| `.github` | Improve and extract | Mirror local checks and evidence capture. |
| `Projects` | Keep and refine | Preserve as validation/sample evidence. |
| `docs` | Keep and refine | Keep as architecture evidence, updated with code changes. |

## Right-To-Exist Review Pass

| Area | Must prove | Default action when it cannot prove it |
| --- | --- | --- |
| Foundation helpers | Multiple consumers and no domain policy. | Move to owning subsystem or delete. |
| Platform abstractions | Real OS/window/input boundary value. | Move host/presentation behavior upward. |
| RHI services | GPU/API concept, backend parity, diagnostics, or explicit interop value. | Keep out of RHI and implement in Renderer/provider/host. |
| Renderer subsystems | Clear render-domain owner and validation path. | Split, fold into pass/feature owner, or delete duplicate registry. |
| GameFramework schemas | Runtime ownership without tool/renderer/private coupling. | Extract to `AssetContracts` or `RenderContracts`. |
| Editor/Application code | Orchestration/presentation value. | Move implementation to RHI/backend/tool/renderer owner. |
| Tools | Focused transformation or orchestration value with actionable reports. | Fold into focused tool, AssetCooker, or inspect/debug command. |
| CMake/CI | Enforces ownership or captures evidence. | Remove broad links or redundant checks. |
| Projects/docs | Validates or explains a real contract. | Remove stale/unlinked content or rewrite it as evidence. |

## Active Refactor Routing

The whole-repository extension is implementation work, not only audit work. Non-rendering roots route to active stages before final evidence gates:

| Root family | Active refactor stages | Final evidence stages |
| --- | --- | --- |
| Core / Platform | Stage 24 | Stage 33, Stage 34, Stage 35, Stage 36 |
| GameFramework runtime and cooked loaders | Stage 25, Stage 26 | Stage 31, Stage 34, Stage 35, Stage 36 |
| Source import | Stage 27 | Stage 31, Stage 33, Stage 34, Stage 35, Stage 36 |
| Focused cookers and cook support | Stage 28 | Stage 31, Stage 34, Stage 35, Stage 36 |
| ShaderCompiler | Stage 29 | Stage 31, Stage 34, Stage 35, Stage 36 |
| AssetCooker, AssetConverter, Launcher, Application, Editor | Stage 30 | Stage 33, Stage 34, Stage 35, Stage 36 |
| Projects and Engine assets | Stage 32 | Stage 34, Stage 35, Stage 36 |
| CMake / CI / docs | Stage 33, Stage 34 | Stage 35, Stage 36 |

## Engine Module Coverage

| Area | Status | Owner layer | Primary risk | Stage / decision question | First validation artifact | Final acceptance evidence |
| --- | --- | --- | --- | --- | --- | --- |
| `Engine/Core` | Needs refactor | Foundation | Foundation helpers can accumulate platform, renderer, tool policy, or hidden scheduler policy. | Stage 23, Stage 24, Stage 34, Stage 35, Stage 36 | Include/dependency scan. | Core owns only reusable foundation types, diagnostics, math, files, strings, events, time, input value types, and no hidden scheduler policy. |
| `Engine/Platform` | Needs refactor | Platform abstraction | Window/input code can leak renderer/editor policy. | Stage 23, Stage 24, Stage 34, Stage 35, Stage 36 | Platform dependency scan and launcher/runtime build. | Platform owns OS/window/input behavior without RHI/Renderer/GameFramework policy; event handoff can become a snapshot/request. |
| `Engine/RHI` | Needs refactor | Graphics API contract/backends | RHI refactors can break shader tools, texture cook, GameFramework cooked contracts, renderer runtime, or future command recording. | Stages 3-20 plus Stage 34, Stage 35, and Stage 36 | Boundary check and RHI detail coverage. | RHI detail rows are accepted, dependent tools still validate, and command/queue handoffs name frame/queue/batch ownership. |
| `Engine/Renderer` | Needs refactor | Render system | Renderer refactors can break GameFramework snapshots, shader compiler package enumeration, launcher smoke, editor viewport, or future render-thread work. | Stages 4-20 plus Stage 34, Stage 35, and Stage 36 | Boundary check, shader enumeration, launcher/editor smoke. | Renderer detail rows are accepted, dependent modules still validate, and frame data/pass execution use immutable or frame-scoped handoffs. |
| `Engine/GameFramework` | Needs refactor | Runtime scene and cooked assets | Runtime scene/cooked schema changes can desync cookers, Renderer scene data, asset loaders, or future simulation/render parallelism. | Stage 25, Stage 26, Stage 31, Stage 34, Stage 35, Stage 36 | GameFramework contract review and affected cooker build. | GameFramework stays cooked-data/runtime-scene oriented and uses immutable renderer-facing snapshots. |
| `Engine/Editor` | Needs refactor | Editor UI surface | Editor can absorb tool internals, backend-native validation shortcuts, or background operation ownership. | Stage 30, Stage 34, Stage 35, Stage 36 | `SparkleLauncher` or editor target build. | Editor owns UI/panels/viewport controls only; cook/import remains behind tools and workflow requests/reports. |
| `Engine/Application` | Needs refactor | Runtime/editor host | Application validation already had backend-native debt; host code can become a service locator or worker owner. | Stage 8, Stage 30, Stage 34, Stage 35, Stage 36 | Application validation include scan and smoke command. | Application orchestrates runtime/editor/validation without backend-native, cook/import, or cross-owner worker implementation. |
| `Engine/Assets` | Needs refactor | Non-code asset root | Current `Meshes`, `Shaders`, and `Textures` ownership is ambiguous: built-in engine assets, renderer shaders, and project content need separate policies. | Stage 23, Stage 32, Stage 34, Stage 35, Stage 36 | Repository root audit and asset/shader source inventory. | Root is narrowed to documented built-in assets with manifest/validation, or content moves to owner-specific shader/data roots. |

## Tool Module Coverage

| Area | Status | Owner layer | Primary risk | Stage / decision question | First validation artifact | Final acceptance evidence |
| --- | --- | --- | --- | --- | --- | --- |
| `Tools/Launcher/SparkleLauncherCore` | Needs refactor | Developer workflow orchestration | Launcher operation logic can duplicate build/cook/import/render behavior instead of invoking owners. | Stage 30, Stage 34, Stage 35, Stage 36 | `SparkleLauncherProbe` and workflow command inspection. | LauncherCore plans and runs process requests, records reports/evidence, and does not own focused tool algorithms. |
| `Tools/Launcher/SparkleLauncher` GUI | Needs refactor | Qt developer UI | UI models/widgets can absorb workflow state, own background work, or hide recovery paths. | Stage 30, Stage 34, Stage 35, Stage 36 | `SparkleLauncher` build and UI model review. | Qt UI follows model/view-style separation: core workflows in LauncherCore, GUI owns presentation and prompts. |
| `Tools/Shaders/ShaderCompiler` | Needs refactor | Shader toolchain | Shader package/reflection changes can break renderer pass runtime, RHI contracts, or future parallel shader jobs. | Stage 4, Stage 17, Stage 29, Stage 31, Stage 34, Stage 35, Stage 36 | `ShaderCompiler list-shaders` and package inspection. | ShaderCompiler compiles, verifies, cooks, lists, and inspects renderer packages with deterministic package/job reports. |
| `Tools/Import/SourceImportAdapters` | Needs refactor | Source scene import | Source format assumptions can leak into GameFramework runtime or cookers; current name is pattern-centered. | Stage 27, Stage 31, Stage 34, Stage 35, Stage 36 | Import diagnostics and sample source import/cook. | Target `SourceImporters` produce imported DTOs plus diagnostics; runtime modules do not read source formats. |
| `Tools/Cooking/TextureCooker` | Needs refactor | Texture cook pipeline | Texture format/schema changes can desync RHI cooked texture contract, renderer texture manager, or parallel cook determinism. | Stage 28, Stage 31, Stage 34, Stage 35, Stage 36 | Texture request inspect/cook command. | TextureCooker emits cooked texture assets compatible with runtime loaders and reports source/format errors clearly. |
| `Tools/Cooking/MeshCooker` | Needs refactor | Mesh cook pipeline | Mesh schema can drift from GameFramework loaders and renderer mesh cache. | Stage 26, Stage 28, Stage 31, Stage 34, Stage 35, Stage 36 | Targeted mesh cook. | MeshCooker converts imported mesh DTOs to cooked runtime mesh records with validation. |
| `Tools/Cooking/MaterialCooker` | Needs refactor | Material cook pipeline | Material/texture reference generation can drift from renderer material cache. | Stage 26, Stage 28, Stage 31, Stage 34, Stage 35, Stage 36 | Targeted material cook plus texture request output. | MaterialCooker emits cooked material records and deterministic texture cook requests. |
| `Tools/Cooking/SceneCooker` | Needs refactor | Scene manifest cook pipeline | Scene manifest changes can break level/runtime loading and renderer scene snapshots. | Stage 26, Stage 28, Stage 31, Stage 34, Stage 35, Stage 36 | Targeted scene cook/load validation. | SceneCooker emits cooked scene manifests compatible with GameFramework loaders and renderer scene data. |
| `Tools/Cooking/AssetCooker` | Needs refactor | Cook orchestration | Orchestration can hide focused tool failures, duplicate cook algorithms, or make job ordering nondeterministic. | Stage 30, Stage 31, Stage 34, Stage 35, Stage 36 | Cook plan/dispatch diagnostic output. | AssetCooker discovers projects, builds cook plans, dispatches focused tools, and reports actionable deterministic failures. |
| `Tools/Conversion/AssetConverter` | Needs refactor | Debug conversion CLI | Legacy direct conversion can become a second cook pipeline. | Stage 30, Stage 34, Stage 35, Stage 36 | CLI command review. | AssetConverter is removed as a production path; useful commands fold into AssetCooker or explicit inspect/debug commands. |
| `Tools/Cooking/CookCommon` | Needs refactor | Tool console support | Vague `Common` naming can become a policy sink. | Rename/split to `ToolConsoleSupport` and/or `CookDiagnostics` in Stage 28, then validate in Stage 31, Stage 34, Stage 35, and Stage 36. | Include/build check. | Shared support has a precise name, owner, report contract, and no asset/shader policy. |

## Build, Project, And Documentation Coverage

| Area | Status | Owner layer | Primary risk | Stage / decision question | First validation artifact | Final acceptance evidence |
| --- | --- | --- | --- | --- | --- | --- |
| `CMake` | Needs refactor | Build architecture | Target dependencies and artifact layout can hide architectural coupling. | Stage 23, Stage 33, Stage 34, Stage 35, Stage 36 | Fresh configure and target graph review. | CMake target links express ownership; artifacts and dependencies are documented. |
| `.github` | Needs refactor | CI workflow | CI can lag local architecture checks and miss tool regressions. | Stage 33, Stage 34, Stage 35, Stage 36 | Workflow command review. | CI or documented local equivalent covers boundary, formatting, shader/tool/build checks. |
| `Projects` | Needs refactor | Runnable sample/content | Showcase content can drift from cook/runtime contracts. | Stage 31, Stage 32, Stage 34, Stage 35, Stage 36 | Showcase cook/load smoke. | Sample project exercises runtime loading, rendering, launcher, and cook paths. |
| `docs` | Needs refactor | Architecture and action records | Docs can contradict code after staged refactors. | Stage 34, Stage 35, Stage 36 | Link/stale-text scan. | Docs, coverage maps, plans, final evidence, and threading-readiness audit agree with code. |

## Whole-Repo Acceptance Check

- Every durable source root has a status row.
- RHI/Renderer detail remains delegated to [Rendering coverage status](rendering-coverage-status.md).
- Tools and GameFramework have explicit contracts.
- New or moved folders must update this file or a linked status file before final acceptance.
- Final acceptance requires no unowned `Needs refactor` rows and no unresolved Stage 35 threading-readiness handoff risks.

# Repository Coverage Status

Status: whole-repository coverage baseline
Date: 2026-06-12
Source plan: `docs/plans/rhi-renderer-review-ready-implementation-plan.md`
Source review: `docs/plans/sparkle-whole-repository-architecture-review.md`
Related detail: [Rendering coverage status](rendering-coverage-status.md)

## Purpose

This document extends the Renderer/RHI coverage map to every durable source root in SparkleEngine. It is a guardrail for large refactors: changing RHI or Renderer is not accepted if it silently degrades Launcher, ShaderCompiler, AssetCooker, TextureCooker, SourceImportAdapters, GameFramework, Editor/Application, build profiles, or project content.

Generated/local-only folders remain out of scope: `build/`, `cmake-build-debug/`, `artifacts/`, `dist/`, and `logs/`.

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
| `Engine/Assets` | 0 | Placeholder/root policy row below. |

## Status Legend

| Status | Meaning |
| --- | --- |
| `Accepted` | Current ownership appears aligned; later stages must preserve it. |
| `Needs refactor` | Known implementation, ownership, validation, or documentation gap. |
| `Needs design decision` | Ownership or policy question must be resolved before moving code. |

## Engine Module Coverage

| Area | Status | Owner layer | Primary risk | Stage / decision question | First validation artifact | Final acceptance evidence |
| --- | --- | --- | --- | --- | --- | --- |
| `Engine/Core` | Needs refactor | Foundation | Foundation helpers can accumulate platform, renderer, or tool policy. | Stage 23, Stage 29 | Include/dependency scan. | Core owns only reusable foundation types, diagnostics, math, files, strings, events, time, and input value types. |
| `Engine/Platform` | Needs refactor | Platform abstraction | Window/input code can leak renderer/editor policy. | Stage 23, Stage 29 | Platform dependency scan and launcher/runtime build. | Platform owns OS/window/input behavior without RHI/Renderer/GameFramework policy. |
| `Engine/RHI` | Needs refactor | Graphics API contract/backends | RHI refactors can break shader tools, texture cook, GameFramework cooked contracts, or renderer runtime. | Stages 3-20 plus Stage 29 | Boundary check and RHI detail coverage. | RHI detail rows are accepted and dependent tools still validate. |
| `Engine/Renderer` | Needs refactor | Render system | Renderer refactors can break GameFramework snapshots, shader compiler package enumeration, launcher smoke, and editor viewport. | Stages 4-20 plus Stage 29 | Boundary check, shader enumeration, launcher/editor smoke. | Renderer detail rows are accepted and dependent modules still validate. |
| `Engine/GameFramework` | Needs refactor | Runtime scene and cooked assets | Runtime scene/cooked schema changes can desync cookers, Renderer scene data, and asset loaders. | Stage 24, Stage 29 | GameFramework contract review and affected cooker build. | GameFramework stays cooked-data/runtime-scene oriented and uses immutable renderer-facing snapshots. |
| `Engine/Editor` | Needs refactor | Editor UI surface | Editor can absorb tool internals or backend-native validation shortcuts. | Stage 26, Stage 29 | `SparkleLauncher` or editor target build. | Editor owns UI/panels/viewport controls only; cook/import remains behind tools. |
| `Engine/Application` | Needs refactor | Runtime/editor host | Application validation already has backend-native debt; host code can become a service locator. | Stage 8, Stage 26, Stage 29 | Application validation include scan and smoke command. | Application orchestrates runtime/editor/validation without backend-native or cook/import implementation. |
| `Engine/Assets` | Needs design decision | Asset root placeholder | Empty roots confuse ownership if future assets land there. | Question: keep as placeholder, remove, or document intended runtime asset role? Stage 29 | Repository root audit. | Root is documented, populated with a contract, or removed. |

## Tool Module Coverage

| Area | Status | Owner layer | Primary risk | Stage / decision question | First validation artifact | Final acceptance evidence |
| --- | --- | --- | --- | --- | --- | --- |
| `Tools/Launcher/SparkleLauncherCore` | Needs refactor | Developer workflow orchestration | Launcher operation logic can duplicate build/cook/import/render behavior instead of invoking owners. | Stage 26, Stage 29 | `SparkleLauncherProbe` and workflow command inspection. | LauncherCore plans and runs processes, records evidence, and does not own focused tool algorithms. |
| `Tools/Launcher/SparkleLauncher` GUI | Needs refactor | Qt developer UI | UI models/widgets can absorb workflow state or hide recovery paths. | Stage 26, Stage 29 | `SparkleLauncher` build and UI model review. | Qt UI follows model/view-style separation: core workflows in LauncherCore, GUI owns presentation and prompts. |
| `Tools/Shaders/ShaderCompiler` | Needs refactor | Shader toolchain | Shader package/reflection changes can break renderer pass runtime and RHI contracts. | Stage 4, Stage 17, Stage 27, Stage 29 | `ShaderCompiler list-shaders` and package inspection. | ShaderCompiler compiles, verifies, cooks, lists, and inspects renderer packages without RHI-specific pass edits. |
| `Tools/Import/SourceImportAdapters` | Needs refactor | Source scene import | Source format assumptions can leak into GameFramework runtime or cookers. | Stage 25, Stage 29 | Import diagnostics and sample source import/cook. | Import adapters produce imported DTOs plus diagnostics; runtime modules do not read source formats. |
| `Tools/Cooking/TextureCooker` | Needs refactor | Texture cook pipeline | Texture format/schema changes can desync RHI cooked texture contract and renderer texture manager. | Stage 25, Stage 27, Stage 29 | Texture request inspect/cook command. | TextureCooker emits cooked texture assets compatible with runtime loaders and reports source/format errors clearly. |
| `Tools/Cooking/MeshCooker` | Needs refactor | Mesh cook pipeline | Mesh schema can drift from GameFramework loaders and renderer mesh cache. | Stage 25, Stage 29 | Targeted mesh cook. | MeshCooker converts imported mesh DTOs to cooked runtime mesh records with validation. |
| `Tools/Cooking/MaterialCooker` | Needs refactor | Material cook pipeline | Material/texture reference generation can drift from renderer material cache. | Stage 25, Stage 29 | Targeted material cook plus texture request output. | MaterialCooker emits cooked material records and deterministic texture cook requests. |
| `Tools/Cooking/SceneCooker` | Needs refactor | Scene manifest cook pipeline | Scene manifest changes can break level/runtime loading and renderer scene snapshots. | Stage 25, Stage 29 | Targeted scene cook/load validation. | SceneCooker emits cooked scene manifests compatible with GameFramework loaders and renderer scene data. |
| `Tools/Cooking/AssetCooker` | Needs refactor | Cook orchestration | Orchestration can hide focused tool failures or duplicate cook algorithms. | Stage 25, Stage 29 | Cook plan/dispatch diagnostic output. | AssetCooker discovers projects, builds cook plans, dispatches focused tools, and reports actionable failures. |
| `Tools/Conversion/AssetConverter` | Needs refactor | Debug conversion CLI | Legacy direct conversion can become a second cook pipeline. | Stage 25, Stage 29 | CLI command review. | AssetConverter remains a debug/developer shell over focused modules or is folded into AssetCooker. |
| `Tools/Cooking/CookCommon` | Accepted | Tool console support | Shared helpers can grow tool policy. | Preserve through Stage 29 | Include/build check. | CookCommon remains small console/tool support only. |

## Build, Project, And Documentation Coverage

| Area | Status | Owner layer | Primary risk | Stage / decision question | First validation artifact | Final acceptance evidence |
| --- | --- | --- | --- | --- | --- | --- |
| `CMake` | Needs refactor | Build architecture | Target dependencies and artifact layout can hide architectural coupling. | Stage 23, Stage 28, Stage 29 | Fresh configure and target graph review. | CMake target links express ownership; artifacts and dependencies are documented. |
| `.github` | Needs refactor | CI workflow | CI can lag local architecture checks and miss tool regressions. | Stage 28, Stage 29 | Workflow command review. | CI or documented local equivalent covers boundary, formatting, shader/tool/build checks. |
| `Projects` | Needs refactor | Runnable sample/content | Showcase content can drift from cook/runtime contracts. | Stage 27, Stage 29 | Showcase cook/load smoke. | Sample project exercises runtime loading, rendering, launcher, and cook paths. |
| `docs` | Needs refactor | Architecture and action records | Docs can contradict code after staged refactors. | Stage 22, Stage 29 | Link/stale-text scan. | Docs, coverage maps, plans, and final evidence agree with code. |

## Whole-Repo Acceptance Check

- Every durable source root has a status row.
- RHI/Renderer detail remains delegated to [Rendering coverage status](rendering-coverage-status.md).
- Tools and GameFramework have explicit contracts.
- New or moved folders must update this file or a linked status file before final acceptance.
- Final acceptance requires no unowned `Needs refactor` rows.

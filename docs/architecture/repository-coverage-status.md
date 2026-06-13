# Repository Coverage Status

Status: whole-repository coverage baseline
Date: 2026-06-12
Last synchronized: 2026-06-13
Source plan: `docs/plans/rhi-renderer-review-ready-implementation-plan.md`
Source review: `docs/plans/sparkle-whole-repository-architecture-review.md`
Related detail: [Rendering coverage status](rendering-coverage-status.md)

## Purpose

This document extends the Renderer/RHI coverage map to every durable source root in SparkleEngine. It is a guardrail for large refactors: changing RHI or Renderer is not accepted if it silently degrades Launcher, ShaderCompiler, AssetCooker, TextureCooker, SourceImporters, GameFramework, Editor/Application, build profiles, or project content.

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
| `Engine/RHI` | 197 | [Rendering coverage status](rendering-coverage-status.md). |
| `Engine/Renderer` | 249 | [Rendering coverage status](rendering-coverage-status.md). |
| `Engine/GameFramework` | 185 | GameFramework rows below and [GameFramework contract](game-framework-contract.md). |
| `Engine/Editor` | 58 | Editor/Application host rows below. |
| `Engine/Application` | 34 | Editor/Application host rows below and rendering validation rows. |
| `Tools/Launcher` | 103 | Launcher rows below and [tooling contract](tooling-pipeline-contract.md). |
| `Tools/Shaders` | 110 | ShaderCompiler rows below and rendering shader coverage rows. |
| `Tools/Cooking` | 106 | Cook pipeline rows below. |
| `Tools/Import` | 85 | Source import rows below. |
| `Tools/Conversion` | 4 | Former AssetConverter root removed in Stage 30; keep this root empty unless a future explicit inspection tool earns a precise owner. |
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
| `Tools/Shaders/ShaderCompiler` | Accepted | Consumes `ShaderContracts` and the narrow renderer shader registration target, not renderer runtime. |
| `Tools/Import/SourceImporters` | Keep and refine | Role-centered source import target emits imported DTOs, reports, and diagnostics. |
| Focused cookers | Keep and refine | Preserve focused cookers and tighten artifact contracts. |
| `Tools/Cooking/AssetCooker` | Improve and extract | Keep orchestration only. |
| `Tools/Conversion/AssetConverter` | Removed | Production path retired; surviving source inspection/debug behavior lives in AssetCooker commands. |
| `Tools/Support/ToolConsoleSupport` | Keep and refine | Generic tool console/report support with no cook-domain policy. |
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
| Core / Platform | Stage 24 | Stage 34, Stage 35, Stage 36 |
| GameFramework runtime and cooked loaders | Stage 25, Stage 26 | Stage 31, Stage 34, Stage 35, Stage 36 |
| Source import | Stage 27 | Stage 31, Stage 34, Stage 35, Stage 36 |
| Focused cookers and cook support | Stage 28 | Stage 31, Stage 34, Stage 35, Stage 36 |
| ShaderCompiler | Stage 29 | Stage 31, Stage 34, Stage 35, Stage 36 |
| AssetCooker, retired AssetConverter behavior, Launcher, Application, Editor | Stage 30 | Stage 34, Stage 35, Stage 36 |
| Projects and Engine assets | Stage 32 | Stage 34, Stage 35, Stage 36 |
| CMake / CI / docs | Stage 34 | Stage 35, Stage 36 |

## Stage 23 Coverage Freeze Matrix

This is the dependency-intent freeze for the whole-repository track. Rendering/RHI internals remain delegated to [Rendering coverage status](rendering-coverage-status.md); this table owns durable source-root visibility, allowed edges, forbidden edges, validation targets, and the stage that must update the row before later code lands in a new root.

| Root | Owner | Producer / consumer role | Allowed dependencies | Forbidden dependencies | Validation target | Active refactor stage | Final acceptance evidence |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `Engine/Core` | Foundation runtime | Produces logging, diagnostics, math, file/string/event/time/input value primitives consumed by all modules. | Standard library, platform-neutral third-party header libraries, OS crypto only through narrow implementation details. | `Engine/Platform`, `Engine/RHI`, `Engine/Renderer`, `Engine/GameFramework`, `Engine/Editor`, `Engine/Application`, `Tools/*`, backend SDKs, Qt. | `SparkleCore` build and dependency/include scan. | Stage 24 | Core remains policy-free and has no renderer, RHI, platform window, tool, launcher, or scheduler ownership. |
| `Engine/Platform` | Platform abstraction | Produces OS, window, and input integration consumed by RHI, Application, GameFramework, and tools when needed. | `Engine/Core`, OS/window APIs, narrow platform SDKs. | Renderer frame graph, RHI backend-private headers, GameFramework gameplay policy, `Tools/*`, Qt launcher UI. | `SparklePlatform` build and platform dependency scan. | Stage 24 | Platform owns OS/window/input only and hands state upward as events, snapshots, or requests. |
| `Engine/RHI` | Graphics API contract/backends | Produces GPU contracts, backend services, shader package primitives, resource/command/pipeline/capture/interop services consumed by Renderer and validation. | `Engine/Core`, `Engine/Platform`, backend SDKs inside backend folders, generic third-party GPU allocators in backend/private support. | `Engine/Renderer/Private`, GameFramework gameplay types, `Tools/*`, Editor/Application host policy, cross-backend private includes. | `architecture_boundary_check`, `ShowcaseEditor`, `ShowcaseRuntime`, rendering coverage rows. | Stages 3-20, Stage 19 | Detail rows in rendering coverage pass and backend service symmetry is complete or explicitly staged. |
| `Engine/Renderer` | Render system | Produces frame graph, passes, renderer snapshots, pipeline runtime, render diagnostics, ray tracing scene, upscaling provider contracts. | `Engine/Core`, `Engine/Platform` only where already established for support, `Engine/RHI`, renderer-facing GameFramework snapshots/DTOs, provider SDKs inside provider targets. | RHI backend-private headers outside provider integration, `Tools/*`, source import/cooking internals, GameFramework private mutation paths, Application/Editor internals. | `architecture_boundary_check`, `ShaderCompiler list-shaders --validate`, D3D12/Vulkan smoke. | Stages 4-20, Stage 19, Stage 29 | Rendering detail rows pass, ordinary passes require no RHI/backend edits, and renderer consumes immutable render-domain inputs. |
| `Engine/GameFramework` | Runtime scene and cooked asset loading | Produces runtime levels, scene state, cameras, lights, mesh/material/cooked asset loaders consumed by Application, Editor, Renderer snapshots, and projects. | `Engine/Core`, `Engine/Platform`, public cooked schemas. | `Engine/Renderer/Private`, `Engine/RHI/Private`, full `SparkleRHI` implementation target, `Tools/*`, source importer internals, renderer pass data, backend-native handles. | `SparkleGameFramework` build, Showcase load smoke, cooked asset loader checks. | Stages 25-26 | Runtime scene/cooked loaders are separated from source import/cooking and export immutable renderer-facing DTOs. |
| `Engine/Editor` | Editor UI surface | Produces editor panels, viewport UI, and editor-facing controls consumed by ApplicationEditor. | `Engine/Core`, `Engine/Platform`, `Engine/GameFramework`, public `Engine/Renderer`, public `Engine/RHI` presentation/diagnostic contracts. | `Tools/*` algorithms, RHI backend-private headers, renderer private frame graph/pass internals, native capture bodies. | `SparkleEditor`, `SparkleApplicationEditor`, `ShowcaseEditor`. | Stage 30 | Editor owns UI and presentation only; tool work and backend capture stay behind workflow/RHI services. |
| `Engine/Application` | Runtime/editor host | Produces app lifecycle, host composition, validation orchestration, runtime console, smoke evidence consumed by projects and launcher-shaped validation. | `Engine/Core`, `Engine/Platform`, `Engine/GameFramework`, public `Engine/Renderer`, public RHI validation/presentation/capture services. | `Tools/*`, RHI backend-private headers, renderer private resources/transitions, cook/import implementation, vendor SDK policy. | `SparkleApplication`, `SparkleApplicationEditor`, D3D12/Vulkan smoke. | Stage 30 | Application orchestrates hosts and validation without owning backend-native, renderer-private, or tool implementation logic. |
| `Engine/Assets` | Built-in engine asset policy | Produces built-in meshes, shaders, and textures consumed by runtime/tool validation until ownership is narrowed. | Engine asset manifest policy, renderer shader/data owners, cook input/output contracts. | Project-specific content policy, generated cooked artifacts, tool-private temporary output. | Asset inventory and Stage 32 cook/load validation. | Stage 32 | Root is narrowed to documented built-in assets or split into owner-specific shader/data roots with manifest validation. |
| `Tools/Launcher/SparkleLauncherCore` | Developer workflow orchestration | Produces build/cook/launch/process requests, operation reports, and history consumed by Qt GUI and validation. | `Engine/Core`, public tool executable contracts, CMake/artifact contracts, process/environment APIs. | Cook/import/shader/render algorithms, `Engine/Renderer/Private`, RHI backend-private headers, direct project runtime mutation. | `SparkleLauncherCore`, `SparkleLauncherProbe`, launcher smoke workflow. | Stage 30 | LauncherCore plans and runs tool/process requests and records evidence without duplicating focused tool algorithms. |
| `Tools/Launcher/SparkleLauncher` GUI | Qt developer UI | Produces Qt presentation, widgets, models, icons, and user workflows over LauncherCore state. | `SparkleLauncherCore`, Qt, UI assets, `Engine/Core` through the core target edge. | Direct cook/import/shader algorithms, direct renderer/RHI backend logic, hidden worker ownership in widgets. | `SparkleLauncher` build and probe. | Stage 30 | GUI owns presentation while LauncherCore owns operation state and process execution. |
| `Tools/Shaders/ShaderCompiler` | Shader toolchain | Produces shader packages, reflection, verification, cooked registries, and inspection reports consumed by Renderer/RHI runtime validation. | `Engine/Core`, `ShaderContracts`, public `Engine/RHI` shader contracts, narrow renderer shader registration target, shader backend SDKs. | Full renderer runtime behavior, renderer private pass execution, GameFramework runtime state, launcher UI. | `ShaderCompiler`, `ShaderCompiler.exe list-shaders --validate`, package inspection. | Stage 29 | ShaderCompiler consumes deterministic shader/pass contracts and emits verifiable cooked packages. |
| `Tools/Import/SourceImporters` | Source asset import | Produces imported scene/mesh/material DTOs, import reports, and diagnostics consumed by focused cookers. | `Engine/Core`, public GameFramework/source DTO contracts, source-format libraries such as Assimp/cgltf through tool targets. | Runtime cooked loaders, Renderer GPU resources, RHI backend/private headers, Application/Editor policy. | `SourceImporters` build and targeted import/cook diagnostic. | Stage 27 | `SourceImporters` produce source DTOs without leaking source format policy into runtime modules. |
| `Tools/Support/ToolConsoleSupport` | Tool console/report support | Produces small shared console/report helpers consumed by cook and shader tools. | `Engine/Core`, pure reporting/CLI helpers. | Asset, renderer, RHI, source import, cook-domain, shader, launcher, or project policy. | `ToolConsoleSupport` build and include scan. | Stage 28 | Support has a precise owner, report contract, and no vague common-policy sink. |
| `Tools/Cooking/TextureCooker` | Texture cook pipeline | Produces cooked texture artifacts and texture diagnostics consumed by GameFramework/Renderer texture loading. | `Engine/Core`, public RHI texture/cooked schema contracts, KTX/Compressonator/stb/tinyexr through tool targets, `ToolConsoleSupport`. | Renderer private texture manager, RHI backend-private headers, GameFramework mutable runtime state. | `TextureCooker` build and targeted texture cook/inspect. | Stage 28 | TextureCooker emits deterministic cooked texture records and clear source/format errors. |
| `Tools/Cooking/MeshCooker` | Mesh cook pipeline | Produces cooked mesh records consumed by GameFramework loaders and Renderer mesh cache. | `Engine/Core`, `SourceImporters`, public GameFramework/asset schemas. | Renderer private mesh cache, RHI backend-private headers, source importer private policy outside import DTOs. | `MeshCooker` build and targeted mesh cook. | Stages 26, 28 | MeshCooker converts imported mesh DTOs into validated cooked runtime mesh records. |
| `Tools/Cooking/MaterialCooker` | Material cook pipeline | Produces cooked material records and texture cook requests consumed by SceneCooker/GameFramework/Renderer material paths. | `Engine/Core`, `SourceImporters`, `TextureCooker`, public material schemas. | Renderer private material cache, RHI backend-private headers, Application/Editor policy. | `MaterialCooker` build and targeted material cook plus texture request output. | Stages 26, 28 | MaterialCooker emits deterministic material records and texture dependencies. |
| `Tools/Cooking/SceneCooker` | Scene manifest cook pipeline | Produces cooked scene manifests consumed by GameFramework level/scene loading and Renderer scene snapshots. | `Engine/Core`, `SourceImporters`, `MeshCooker`, `MaterialCooker`, public GameFramework schemas. | Renderer private scene data, RHI backend-private headers, launcher UI, Application runtime mutation. | `SceneCooker` build and targeted scene cook/load validation. | Stages 26, 28 | SceneCooker emits cooked scene manifests compatible with runtime loaders and render snapshots. |
| `Tools/Cooking/AssetCooker` | Cook orchestration and source inspection | Produces cook plans, dispatches focused cookers, aggregates reports, and exposes explicit source inspect/debug commands consumed by LauncherCore/CI/users. | `Engine/Core`, focused cooker targets, `SourceImporters`, `ToolConsoleSupport`, project/artifact contracts. | Reimplemented focused cook algorithms, renderer/RHI private runtime behavior, Qt GUI, generic conversion ownership. | `AssetCooker`, cook plan/report validation, `inspect-source`, `collect-texture-requests`. | Stage 30 | AssetCooker orchestrates deterministic jobs, preserves focused tool failure details, and owns surviving read-only source debug behavior. |
| `Tools/Conversion/AssetConverter` | Removed | No durable source root remains after Stage 30. | None. | Any rebuilt production cook path, runtime loader ownership, renderer/RHI private behavior. | Live-code reference scan. | Stage 30 | Source/CMake target removed; useful behavior moved to AssetCooker inspect/debug commands. |
| `Projects/Showcase` | Runnable sample/project evidence | Produces project manifests, source content, level code, runtime/editor executables, and smoke artifacts. | Public Engine modules, project-local assets/source, cooked artifact contracts. | Engine/tool implementation policy, generated logs as source, RHI/Renderer private internals. | `ShowcaseEditor`, `ShowcaseRuntime`, Showcase cook/load/smoke. | Stages 31-32 | Showcase exercises cook, load, renderer, launcher, and validation paths with owned artifacts. |
| `CMake` | Build architecture | Produces build profiles, dependency fetch, target scopes, artifact layout, and validation targets consumed by local/CI builds. | CMake modules, external dependency declarations, target usage requirements. | Runtime/tool source policy hidden in broad include/link scopes, durable generated artifacts. | Fresh configure, target graph review, focused target builds. | Stage 34 | Target links express ownership with correct `PUBLIC`/`PRIVATE`/`INTERFACE` scope and no broad dependency shortcuts. |
| `.github/workflows` | CI workflow | Produces repeatable CI command wiring and evidence gates. | CMake/local validation commands, repository scripts/checks. | CI-only behavior that cannot be reproduced locally, local machine state. | Workflow command review and CI/local parity checklist. | Stage 34 | CI or documented local equivalents cover boundary, formatting, shader/tool/build checks. |
| `docs/architecture` | Architecture contracts | Produces current/target maps, contracts, guardrails, and evidence routes consumed by implementation stages. | Repository docs, code/file links, evidence artifacts by reference. | Generated/local logs as durable docs, stale paths, duplicated contradictory plans. | `rg` link/stale-text scan. | Stage 23, Stage 34 | Architecture docs agree with code, stage map, coverage status, and validation evidence. |
| `docs/plans` | Refactor execution/tutor plans | Produces implementation prompts, status map, tutor notes, and rubric gates consumed by future work. | Architecture docs, acceptance rubric, stage evidence. | Code ownership decisions not reflected in architecture contracts, stale presentation-only README clutter. | `rg` stage/status/link scan. | Stage 23, Stage 34 | Plans route every active refactor to concrete target docs and no stale stage status remains. |
| `External/NVIDIA` | Vendor SDK holding root | Produces checked-in or vendored SDK inputs if present. | Vendor SDK contents and narrow provider/build integration. | Engine policy edits inside vendor code, runtime modules linking SDKs outside documented providers. | Vendor inventory and boundary/CMake review. | Stage 34 | Vendor code is isolated, unmodified for engine policy, and consumed only through narrow provider/build targets. |

## Engine Module Coverage

| Area | Status | Owner layer | Primary risk | Stage / decision question | First validation artifact | Final acceptance evidence |
| --- | --- | --- | --- | --- | --- | --- |
| `Engine/Core` | Accepted | Foundation | Foundation helpers can accumulate platform, renderer, tool policy, or hidden scheduler policy. | Stage 23, Stage 24, Stage 34, Stage 35, Stage 36 | Include/dependency scan passed for renderer/RHI/tool/launcher policy terms; `SparkleCore` built. | Core owns only reusable foundation types, diagnostics, math, files, strings, events, time, input value types, and no hidden scheduler policy. |
| `Engine/Platform` | Accepted | Platform abstraction | Window/input code can leak renderer/editor policy. | Stage 23, Stage 24, Stage 34, Stage 35, Stage 36 | Platform policy scan passed with no ImGui/tool/render ownership; `SparklePlatform`, `ShowcaseEditor`, and `ShowcaseRuntime` built. | Platform owns OS/window/input behavior without RHI/Renderer/GameFramework policy; event handoff can become a snapshot/request. |
| `Engine/RHI` | Accepted | Graphics API contract/backends | RHI refactors can break shader tools, texture cook, GameFramework cooked contracts, renderer runtime, or future command recording. Stage 19 still owns deeper backend service slimming as a non-blocking follow-up. | Stages 3-20 plus Stage 34, Stage 35, and Stage 36 | Boundary check, targeted builds, D3D12 runtime smoke, and RHI detail coverage passed. | RHI detail rows are accepted, dependent tools still validate, and command/queue handoffs name frame/queue/batch ownership. |
| `Engine/Renderer` | Accepted | Render system | Renderer refactors can break GameFramework snapshots, shader compiler package enumeration, launcher smoke, editor viewport, or future render-thread work. Stage 19's backend-service slimming remains an owned non-blocking follow-up. | Stages 4-20 plus Stage 34, Stage 35, and Stage 36 | Boundary check, shader enumeration, launcher-shaped runtime smoke, and targeted builds passed. | Renderer detail rows are accepted, dependent modules still validate, and frame data/pass execution use immutable or frame-scoped handoffs. |
| `Engine/GameFramework` | Accepted | Runtime scene and cooked assets | Runtime scene/cooked schema changes can desync cookers, Renderer scene data, asset loaders, or future simulation/render parallelism. | Stage 25, Stage 26, Stage 31, Stage 34, Stage 35, Stage 36 | GameFramework dependency scan passed; `SparkleGameFramework`, `ShowcaseEditor`, `ShowcaseRuntime`, `SourceImporters`, `MeshCooker`, `MaterialCooker`, `SceneCooker`, and `AssetCooker` built. | GameFramework stays cooked-data/runtime-scene oriented and uses immutable renderer-facing snapshots. |
| `Engine/Editor` | Accepted | Editor UI surface | Editor can absorb tool internals, backend-native validation shortcuts, or background operation ownership. Stage 35 hardens future worker ownership where the current shape blocks safe handoff. | Stage 30, Stage 34, Stage 35, Stage 36 | Host/tool boundary scans and `SparkleLauncher`/runtime builds passed. | Editor owns UI/panels/viewport controls only; cook/import remains behind tools and workflow requests/reports. |
| `Engine/Application` | Accepted | Runtime/editor host | Application validation already had backend-native debt; host code can become a service locator or worker owner. Stage 35 hardens future worker ownership where the current shape blocks safe handoff. | Stage 8, Stage 30, Stage 34, Stage 35, Stage 36 | Application validation include scan and D3D12 launcher-shaped smoke passed. | Application orchestrates runtime/editor/validation without backend-native, cook/import, or cross-owner worker implementation. |
| `Engine/Assets` | Accepted | Non-code asset root | Built-in engine assets still need long-term manifest discipline, but Stage 32 narrowed the role and removed broken runtime level evidence. | Stage 23, Stage 32, Stage 34, Stage 35, Stage 36 | Repository root audit, project asset ownership contract, Showcase cook/load smoke passed. | Root is narrowed to documented built-in assets with validation; project content and generated outputs stay outside this root. |

## Tool Module Coverage

| Area | Status | Owner layer | Primary risk | Stage / decision question | First validation artifact | Final acceptance evidence |
| --- | --- | --- | --- | --- | --- | --- |
| `Tools/Launcher/SparkleLauncherCore` | Accepted | Developer workflow orchestration | Launcher operation logic can duplicate build/cook/import/render behavior instead of invoking owners. Stage 35 hardens async/process handoff readiness where needed. | Stage 30, Stage 34, Stage 35, Stage 36 | `SparkleLauncher`, launcher-shaped runtime smoke, and workflow command inspection passed. | LauncherCore plans and runs process requests, records reports/evidence, and does not own focused tool algorithms. |
| `Tools/Launcher/SparkleLauncher` GUI | Accepted | Qt developer UI | UI models/widgets can absorb workflow state, own background work, or hide recovery paths. Stage 35 hardens UI/background operation handoffs where needed. | Stage 30, Stage 34, Stage 35, Stage 36 | `SparkleLauncher` build and UI model review passed. | Qt UI follows model/view-style separation: core workflows in LauncherCore, GUI owns presentation and prompts. |
| `Tools/Shaders/ShaderCompiler` | Accepted | Shader toolchain | Shader package/reflection changes can break renderer pass runtime, RHI contracts, or future parallel shader jobs. | Stage 31, Stage 34, Stage 35, Stage 36 | `ShaderCompiler list-shaders`, package cook, and package inspection. | ShaderCompiler compiles, verifies, cooks, lists, and inspects renderer packages with deterministic package/job reports. |
| `Tools/Import/SourceImporters` | Accepted | Source scene import | Source format assumptions can leak into GameFramework runtime or cookers if importer-private state crosses the DTO/report boundary. | Stage 27, Stage 31, Stage 34, Stage 35, Stage 36 | `SourceImporters` build and sample source import/cook diagnostics. | `SourceImporters` produce imported DTOs plus diagnostics; runtime modules do not read source formats. |
| `Tools/Cooking/TextureCooker` | Accepted | Texture cook pipeline | Texture format/schema changes can desync RHI cooked texture contract, renderer texture manager, or parallel cook determinism. | Stage 31, Stage 34, Stage 35, Stage 36 | Texture request inspect/cook command. | TextureCooker emits cooked texture assets compatible with runtime loaders and reports source/format errors clearly. |
| `Tools/Cooking/MeshCooker` | Accepted | Mesh cook pipeline | Mesh schema can drift from GameFramework loaders and renderer mesh cache. | Stage 31, Stage 34, Stage 35, Stage 36 | Targeted mesh cook. | MeshCooker converts imported mesh DTOs to cooked runtime mesh records with validation. |
| `Tools/Cooking/MaterialCooker` | Accepted | Material cook pipeline | Material/texture reference generation can drift from renderer material cache. | Stage 31, Stage 34, Stage 35, Stage 36 | Targeted material cook plus texture request output. | MaterialCooker emits cooked material records and deterministic texture cook requests. |
| `Tools/Cooking/SceneCooker` | Accepted | Scene manifest cook pipeline | Scene manifest changes can break level/runtime loading and renderer scene snapshots. | Stage 31, Stage 34, Stage 35, Stage 36 | Targeted scene cook/load validation. | SceneCooker emits cooked scene manifests compatible with GameFramework loaders and renderer scene data. |
| `Tools/Cooking/AssetCooker` | Accepted | Cook orchestration and source inspection | Orchestration can hide focused tool failures, duplicate cook algorithms, or make job ordering nondeterministic. | Stage 31, Stage 34, Stage 35, Stage 36 | Cook plan/dispatch diagnostic output plus source inspect/debug commands. | AssetCooker discovers projects, builds cook plans, dispatches focused tools, reports actionable deterministic failures, and owns surviving read-only source debug behavior. |
| `Tools/Conversion/AssetConverter` | Accepted removal | Retired conversion path | Legacy direct conversion can return as a second cook pipeline. | Stage 34, Stage 35, Stage 36 | Live-code reference scan. | No production target or source remains; useful commands live in AssetCooker inspect/debug commands. |
| `Tools/Support/ToolConsoleSupport` | Accepted | Tool console support | Console/report helpers can become a hidden common policy sink if they grow asset, shader, launcher, project, or cook-domain behavior. | Stage 28, Stage 31, Stage 34, Stage 35, Stage 36 | Include/build check. | Shared support has a precise name, owner, report contract, and no asset/shader/cook policy. |

## Build, Project, And Documentation Coverage

| Area | Status | Owner layer | Primary risk | Stage / decision question | First validation artifact | Final acceptance evidence |
| --- | --- | --- | --- | --- | --- | --- |
| `CMake` | Accepted | Build architecture | Target dependencies and artifact layout can hide architectural coupling. Stage 36 owns final stale-target deletion if new evidence appears. | Stage 23, Stage 34, Stage 35, Stage 36 | Existing configure tree, target graph review, `architecture_boundary_check`, and focused target builds passed. | CMake target links express ownership; artifacts and dependencies are documented. |
| `.github` | Accepted | CI workflow | CI can lag local architecture checks and miss tool regressions; current CI is shader-cook focused and local commands cover broader validation. | Stage 34, Stage 35, Stage 36 | Workflow command review and local validation command set. | CI or documented local equivalent covers boundary, formatting, shader/tool/build checks. |
| `Projects` | Accepted | Runnable sample/content | Showcase content can drift from cook/runtime contracts or advertise levels whose scene assets are not cookable. | Stage 31, Stage 32, Stage 34, Stage 35, Stage 36 | Showcase cook/load smoke and [project-asset-ownership-contract.md](project-asset-ownership-contract.md). | Sample project exercises runtime loading, rendering, launcher, and cook paths; generated/local outputs remain outside source policy. |
| `docs` | Accepted | Architecture and action records | Docs can contradict code after staged refactors. Stage 35 and Stage 36 still own final threading and cleanup synchronization. | Stage 23, Stage 34, Stage 35, Stage 36 | Link/stale-text scan and Stage 34 evidence reconciliation passed. | Docs, coverage maps, plans, final evidence, and threading-readiness hardening agree with code. |

## Whole-Repo Acceptance Check

- Every durable source root has a status row.
- RHI/Renderer detail remains delegated to [Rendering coverage status](rendering-coverage-status.md).
- Tools and GameFramework have explicit contracts.
- New or moved folders must update this file or a linked status file before final acceptance.
- Final acceptance requires no unowned `Needs refactor` rows and no unresolved Stage 35 threading-readiness hardening risks.

## Stage 34 Evidence Gate

Stage 34 accepts the whole-repository evidence gate with owned follow-up risk rather than pretending the repository is final-review ready before Stage 35 and Stage 36.

| Area | Evidence |
| --- | --- |
| Coverage status | High-level repository rows are accepted or accepted-removal. Remaining detailed rendering/backend risks are owned by Stage 19, Stage 35, or Stage 36. |
| Boundary status | `architecture_boundary_check` passed; counted exceptions remain limited to provider-owned NVIDIA DLSS Vulkan/Streamline linkage. |
| Build status | `ShaderCompiler`, `AssetCooker`, `TextureCooker`, `SparkleLauncher`, and `ShowcaseRuntime` built in `DevelopmentEditor` from `build/windows-vs2026-stage5`. |
| Tool/artifact status | Shader registration validation reported `17` valid typed registrations and `10` packages. `AssetCooker cook-assets Showcase DevelopmentEditor` cooked `9` scenes. `TextureCooker` inspected `6` DamagedHelmet requests and cooked `6/6` textures with summary `artifacts/diagnostics/cook/Summaries/stage34-damagedhelmet-texturecook-summary.json`. |
| Runtime status | Launcher-shaped D3D12 `ShowcaseRuntime` smoke from `Projects/Showcase` exited `0`, completed `5/5` level switches, and reported `frameGraphUnresolvedBarrierWarnings=0`, DLSS active, RT available, and valid TLAS evidence. |
| Folder status | Empty retired `Tools/Conversion/AssetConverter` directories were removed. `Tools/Cooking/CookCommon`, `Tools/Import/SourceImportAdapters`, `Tools/Conversion/AssetConverter`, and `Projects/Showcase/Levels/Bistro.level` are absent from durable source roots. |
| Owned follow-up | Stage 19 owns deeper backend service slimming. Stage 35 owns threading-readiness hardening and final handoff audit. Stage 36 owns final stale-path cleanup, rubric scoring, and review-ready gate. |

## Stage 35 Threading-Readiness Hardening

Stage 35 accepts the current whole-repository threading-readiness hardening pass with code changes, not an audit-only result.

| Area | Evidence |
| --- | --- |
| Runtime handoff | `RenderSceneSnapshot` is owned by `FramePipeline`; `RendererSystemRoot` no longer owns or exposes the frame snapshot; `SceneRenderStateCoordinator` no longer mutates frame snapshot data from level lifecycle events. |
| Catalog handoff | `GlobalShaderRegistry` and builtin ShaderCompiler backend registrations publish deterministic sorted snapshots to consumers. |
| Launcher/tool handoff | Tool resolver, Qt kit detection, maintenance clean target planning, and maintenance clean process request generation sort discovered candidates/projects before producing process-facing data. |
| Validation status | VS2026 configure passed in `build-vs2026`; `ShaderCompiler`, `SparkleLauncher`, and `ShowcaseRuntime` built; `architecture_boundary_check` passed; `ShaderCompiler.exe list-shaders --validate` reported `17` valid typed registrations and `10` packages; `git diff --check` reported only line-ending normalization warnings. |
| Remaining owned follow-up | Stage 19 remains the owner for deeper backend command/descriptor/memory/pipeline service slimming before real multi-worker command recording. Stage 36 owns final cleanup and rubric scoring. |

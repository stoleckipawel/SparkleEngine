# D. Whole Repository Architecture Map

Status: dated source-backed repository snapshot; not a normative architecture or strategy contract
Date: 2026-07-24
Scope: implemented depot structure, module boundaries, runtime flow, tools, APIs, memory, and CPU/GPU surfaces observed at the stated date

## Intent

This document records the repository shape observed on 2026-07-24 so reviewers can locate modules, runtime flows, APIs, tools, memory/performance surfaces, and source evidence. It is descriptive: revalidate facts against current code before using them in a change.

Target capability belongs to [Principal Graphics Requirements](../Strategy/Requirements.md), binding implementation rules belong to [Engineering Standards](../Engineering/Standards/README.md), and accepted subsystem decisions belong to focused architecture documents. This map does not override any of them.
## Executive Map

SparkleEngine is already shaped like a modern rendering engine:

- `Engine/RHI` provides D3D12 and Vulkan backends behind a common RHI service surface.
- `Engine/GameFramework` owns world-to-render extraction into immutable structural/dynamic input, while `Engine/Renderer` owns render-world consumption, frame graph, passes, ray tracing scene ownership, upscaling/ray reconstruction provider contracts, shader registrations, and render-owned diagnostics.
- `Tools/Shaders/ShaderCompiler` is a serious offline shader pipeline with DXC/Slang backends, reflection, contracts, cache, package cooking, and inspection commands.
- `Tools/Cooking` and `Tools/Import` form a source-to-cooked content pipeline.
- `Tools/Launcher/SparkleLauncher` is a workflow shell for build, cook, launch, clean, dependency, quality, and GUI flows.
- `Projects/Showcase` is the main sample project and dominates depot size.

The main issue is not lack of feature ambition. The issue is that Sparkle currently tries to be all of these at once:

- engine runtime
- renderer research platform
- shader SDK
- content pipeline
- launcher/workflow product
- validation/reporting environment
- heavy showcase depot

Top-tier repositories usually declare a much sharper scope. Sparkle should do the same, then cut anything outside the chosen product line.

## Authority Boundary

The snapshot may identify risks or opportunities, but it does not own principal-capability requirements, repository grades, implementation rules, or roadmap priority. Follow the owning documents linked from the [documentation root](../README.md).
## Repository Measurements

Scan excludes `.git`, `build`, `artifacts`, and `logs`.

| Root | Files | Approx MB | Interpretation |
| --- | ---: | ---: | --- |
| `Projects` | 775 | 1527.06 | Still the dominant depot weight; content organization remains the biggest byte-reduction lever. |
| `Engine` | 1164 | 340.29 | Main runtime, shaders, default assets, RHI, renderer, editor, application. |
| `Tools` | 422 | 15.01 | Launcher, shader compiler, import/cooking tools. |
| `Docs` | 12 | 0.21 | Active review and implementation guidance. |
| `CMake` | 6 | 0.09 | Build profiles, dependencies, packaging, boundary checks. |
| Config/root metadata | 8 | 0.02 | Formatting, engine config, license, root CMake, and root metadata. |

Text/source line distribution:

| Extension | Files | Lines |
| --- | ---: | ---: |
| `.cpp` | 596 | 86199 |
| `.h` | 832 | 37025 |
| `.hlsli` | 70 | 4253 |
| `.md` | 14 | 3740 |
| `.txt` | 23 | 1979 |
| `.cmake` | 7 | 1790 |
| `.hlsl` | 20 | 1222 |
| `.ini` | 2 | 37 |

Largest source areas:

| Area | Files | Lines | Read |
| --- | ---: | ---: | --- |
| `Engine/RHI` | 247 | 32559 | Largest source owner; backend code and the public RHI contract remain the highest-leverage architecture surface. |
| `Engine/Renderer` | 452 | 32095 | Almost as large as RHI; most feature complexity should stay private and pass/frame-graph owned. |
| `Tools/Launcher` | 107 | 16802 | Large for a workflow shell; still a prime slimming target after content/capture guardrails. |
| `Tools/Shaders` | 115 | 8241 | Strong product-like subsystem; preserve ABI while removing debug/default artifact bloat. |
| `Engine/GameFramework` | 174 | 7994 | Runtime scene/assets/components and cooked loaders; public surface is the cleanup risk. |
| `Engine/Editor` | 57 | 7639 | Editor panels and utilities; useful only when product-owned, not as diagnostic sprawl. |
| `Tools/Cooking` | 105 | 7300 | Multiple cookers plus AssetCooker orchestration; default report behavior should shrink. |
| `Engine/Core` | 77 | 5482 | Foundation utilities; public-heavy and worth auditing for convenience APIs. |
| `Engine/Assets` | 90 | 5475 | HLSL and default assets. |
| `Tools/Import` | 62 | 3222 | Source import pipeline. |
| `Engine/Application` | 29 | 2196 | Runtime/editor hosts and shader recook/runtime console. |
| `Engine/Platform` | 11 | 1619 | Window/input/platform services. |

Largest individual files:

| File | Lines | Action signal |
| --- | ---: | --- |
| `Engine/RHI/Private/D3D12/ThirdParty/d3dx12.h` | 3850 | Third-party header, not a Sparkle cleanup target. |
| `Engine/RHI/Private/Vulkan/Commands/VulkanRenderCommandList.cpp` | 1235 | Backend command code, performance-sensitive. |
| `Engine/RHI/Private/Shaders/CookedShaderPackageCache.cpp` | 1061 | Shader runtime package load/cache. |
| `Engine/RHI/Private/Vulkan/Device/VulkanRhi.cpp` | 948 | Vulkan device/bootstrap. |
| `Engine/RHI/Private/Vulkan/Memory/VulkanGpuMemoryAllocator.cpp` | 941 | Memory allocator integration. |
| `Engine/Editor/Private/Util/UiUtil.cpp` | 876 | Editor utility concentration. |
| `CMake/Dependencies/FetchDependencies.cmake` | 831 | Dependency policy concentration. |
| `Tools/Launcher/SparkleLauncher/Private/Shell/LauncherShell.cpp` | 809 | CLI shell workflow concentration. |
| `Engine/RHI/Private/Vulkan/VulkanRenderHardwareInterface.cpp` | 783 | Vulkan RHI service wiring concentration. |
| `Tools/Cooking/AssetCooker/Private/Dispatch/AssetCookerDispatcher.cpp` | 782 | Cook orchestration concentration. |
| `Engine/RHI/Private/Vulkan/Descriptors/VulkanDescriptorAllocator.cpp` | 768 | Descriptor allocator implementation concentration. |
| `Engine/Core/Private/FileSystemUtils.cpp` | 729 | Foundation utility weight. |

## Showcase Content Inventory

Stage 04 discovery found seven in-repo Showcase levels. Multi-level support is a preserved capability; cleanup must catalog or externalize content without reducing the engine to one sample level.

Current repository-resident source-content map set:

- `Empty`
- `DamagedHelmet`
- `CesiumMan`
- `DiffuseTransmissionPlant`
- `ABeautifulGame`
- `Sponza`

There is no required downloadable map or catalog startup default. `Empty` is the permanent repository core level and is not exposed as a sync choice; all showcase maps remain independent selections. If the core level cannot be read, runtime still creates an in-memory `Empty` scene so a code-only checkout remains launchable.
The former `SponzaPtlas` level was removed because it duplicated Sponza/Cesium scene composition as a scene-specific validation path rather than owning an independent map or workload contract.

Level inventory:

| Level File | Level Name | Scene Asset Refs | Unique Scene Assets | Repository Resident |
| --- | --- | ---: | --- | --- |
| `Projects/Showcase/Levels/Empty.level` | `Empty` | 0 | none | Yes; fallback/minimal scene. |
| `Projects/Showcase/Levels/DamagedHelmet.level` | `DamagedHelmet` | 1 | `DamagedHelmet/DamagedHelmet` | Yes; small material/mesh check. |
| `Projects/Showcase/Levels/CesiumMan.level` | `CesiumMan` | 1 | `CesiumMan/CesiumMan` | Yes; animation/skinned asset check. |
| `Projects/Showcase/Levels/DiffuseTransmissionPlant.level` | `DiffuseTransmissionPlant` | 1 | `DiffuseTransmissionPlant/DiffuseTransmissionPlant` | Yes; material/transmission asset check. |
| `Projects/Showcase/Levels/ABeautifulGame.level` | `ABeautifulGame` | 1 | `ABeautifulGame/ABeautifulGame` | Yes; medium mesh/material scene. |
| `Projects/Showcase/Levels/Sponza.level` | `Sponza` | 1 | `Sponza/Sponza` | Yes; current renderer review scene and future Tier 0 regression workload. |

Heavy optional/generated candidates:

| Path | Approx MB | Candidate Type | Stage 04 Decision |
| --- | ---: | --- | --- |
| `Projects/Showcase/Assets/Meshes/Bistro` | 1438.80 | Optional heavy source content pack. | Removed from the core repo in Stage 07; catalog entry remains external/unavailable. |
| `Projects/Showcase/logs/trace.json` | 1261.82 | Generated log artifact. | Do not treat as required content; remove or ignore only in a later cleanup stage. |
| `Projects/Showcase/Cooked` | 508.96 | Generated cooked output. | Do not treat as required source content; later stages should keep default cook outputs out of the source footprint. |
| `Projects/Showcase/Assets/Textures/Sponza` | 41.00 | Default-set texture content. | Keep with default set unless a later catalog supports external packs. |
| `Projects/Showcase/Assets/Textures/ABeautifulGame` | 18.05 | Default-set texture content. | Keep with default set. |
| `Projects/Showcase/Assets/Meshes/ABeautifulGame` | 10.37 | Default-set mesh content. | Keep with default set. |
| `Projects/Showcase/Assets/Meshes/Sponza` | 9.26 | Default-set mesh content. | Keep with default set. |

Stage 04 rule:

- Do not remove content yet.
- Preserve the repository-resident catalog while allowing every map to be independently selected or removed from the active set.
- Treat logs and cooked outputs as later cleanup/package-boundary candidates. Bistro was externalized from the core repo in Stage 07.

Stage 05 catalog result:

- `Projects/Showcase/Levels.catalog` is the launcher’s single content-owned level catalog.
- Each map has `Id`, `DisplayName`, `Description`, `Source`, and `Selected` metadata, plus optional presentation and asset-pack references.
- Runtime level discovery loads ready catalog maps in the active set; the launcher maps each card's Sync/Clean action to the catalog's internal `Selected` state, and a missing active map is non-fatal.
- Asset cook discovery reads the same selected map set and filters source scenes to those map-referenced assets. An empty set is valid.
- Acquisition metadata is represented by `AssetPack` on maps and `[AssetPack]` records. Pack acquisition is derived from selected maps rather than a second selection authority.

Stage 06 asset-pack boundary:

- Asset-pack ownership is project-owned. `Projects/Showcase/Levels.catalog` is the single boundary for Showcase acquisition metadata.
- An asset-pack root is the catalog `Root` value relative to `Projects/Showcase`; the Bistro root is `Assets/Meshes/Bistro`.

### Canonical Acceptance-Workload Overlay

The inventory above describes current repository truth. Future tier assignment, required architecture consequences, and completion evidence belong only to [I. Bistro and San Miguel Acceptance Workloads](../Engineering/BistroAndSanMiguelWorkloads.md) and are not repeated in this snapshot.
- No downloadable map is required for build, cook, or launch. The core Empty level needs no asset pack, so a base sync does not acquire Bistro or another map pack.
- Missing asset packs are non-fatal: affected selected maps are skipped while other selected maps, or the built-in empty fallback, remain usable.
- Core repo byte reduction target for Stage 07 is at least 1438.80 MB by removing or externalizing `Projects/Showcase/Assets/Meshes/Bistro`, reducing `Projects` source content from about 1527.06 MB to about 88.26 MB before generated-output cleanup.

Stage 07 externalization result:

- `Projects/Showcase/Assets/Meshes/Bistro` was removed from the core repo.
- `Projects/Showcase/Levels.catalog` keeps pack `Bistro` discoverable as `Root = Assets/Meshes/Bistro`, `External = true`.
- The repository-resident map set remains intact, while its active sync state is independently configurable.
- `Projects` source content, excluding generated logs/cooked output, is now about 88.26 MB.

## Module And Layer Shape

Declared engine modules:

| Module | Role | Main dependencies |
| --- | --- | --- |
| `Core` | Math, events, file utilities, logging, fatal checks, config helpers. | Bottom layer. |
| `Platform` | Window, input, platform services. | Publicly depends on Core. |
| `RHI` | D3D12/Vulkan abstraction, resources, descriptors, pipelines, commands, ray tracing, memory, diagnostics, presentation. | Public Core; private Platform. |
| `Renderer` | Frame graph, passes, scene extraction, ray tracing scene, providers, textures, shader registrations. | Public Core/RHI; private Platform/GameFramework/provider target. |
| `GameFramework` | Scene, level, components, cooked asset loading. | Public Core/Platform; no renderer dependency. |
| `Editor` | Editor UI and panels. | Private Renderer/RHI/Core/Platform/GameFramework. |
| `Application` | Runtime/editor host orchestration. | Public Renderer; private Core/Platform/GameFramework/Editor. |

Observed include links between engine modules:

| From | To | Include count |
| --- | --- | ---: |
| `Renderer` | `RHI` | 221 |
| `Renderer` | `Core` | 179 |
| `RHI` | `Core` | 56 |
| `GameFramework` | `Core` | 38 |
| `Editor` | `Core` | 26 |
| `Application` | `Core` | 20 |
| `Editor` | `Renderer` | 12 |
| `Renderer` | `GameFramework` | 7 |
| `Application` | `RHI` | 3 |
| `Application` | `Editor` | 3 |
| `Editor` | `RHI` | 3 |
| `Application` | `Renderer` | 2 |

The module direction is mostly good. The renderer consumes runtime scene data privately instead of making GameFramework depend on rendering. RHI does not include Renderer private headers. The architecture boundary check is narrowly focused on RHI/Renderer native API leakage, D3D12/Vulkan backend separation, and renderer PTLAS native identifier leakage.

## Public Versus Private API Shape

| Area | Public lines | Private lines | Public share | Read |
| --- | ---: | ---: | ---: | --- |
| `Engine/Core` | 2658 | 2824 | 48.5% | Public-heavy foundation. Worth auditing for convenience APIs that are not true engine contracts. |
| `Engine/Platform` | 363 | 1256 | 22.4% | Healthy for low-level platform abstractions. |
| `Engine/RHI` | 4424 | 28135 | 13.6% | Broad but plausible because it is the contract layer. Diagnostic/capture APIs raise review cost. |
| `Engine/Renderer` | 2155 | 29298 | 6.9% | Good instinct: most renderer complexity is private. Public diagnostics/capture still widen the surface. |
| `Engine/GameFramework` | 2151 | 5843 | 26.9% | Still public-heavy; level/scene concepts stay, asset/manifest implementation details should be audited. |
| `Engine/Editor` | 616 | 7023 | 8.1% | Good public ratio; private panel volume still matters. |
| `Engine/Application` | 133 | 2063 | 6.1% | Good. |
| `Tools/Launcher/SparkleLauncher` | 763 | 15968 | 4.6% | Good surface ratio, but private workflow volume is high. |
| `Tools/Shaders/ShaderCompiler` | 0 | 6053 | 0.0% | CLI-private, appropriate. |
| `Tools/Cooking/AssetCooker` | 58 | 2064 | 2.7% | Mostly private, but public bridge may be unnecessary. |
| `Tools/Cooking/TextureCooker` | 79 | 3247 | 2.4% | Mostly private. |
| `Tools/Import/SourceImporters` | 416 | 2806 | 12.9% | Public import API exists; keep only if used by multiple cookers. |

API cleanup direction:

- RHI public surface should remain explicit, but separate "runtime render contract" from "developer/profiler/debug tooling contract."
- Renderer public surface should expose render requests/products, host frame orchestration, shader reload if product-owned, and only minimal editor-facing status.
- Public renderer capture, memory, mesh, and texture diagnostic snapshots should either be product editor APIs or moved behind an editor-private adapter.
- GameFramework public surface should be re-audited for asset loader and scene-manifest implementation details.
- Tool public headers should exist only when another tool or executable consumes them as a stable API.

## Engine Runtime Flow

High-level runtime flow:

| Stage | Owner | Notes |
| --- | --- | --- |
| Application launch | `Engine/Application` | Hosts runtime/editor application classes and shader recook/runtime console plumbing. |
| Window/input/time | `Engine/Platform`, `Engine/Core` | Platform service layer and basic timing. |
| Runtime scene | `Engine/GameFramework` | Level, scene, assets, cooked loaders, material/mesh/light/camera data. |
| Renderer facade | `Engine/Renderer/Public/Renderer.h` | Public facade for render requests/products, host frame steps, RHI access, shader reload, and diagnostics/capture APIs. |
| System root | `RendererSystemRoot` | Private dependency hub for renderer services. |
| Frame pipeline | `FramePipeline` | Prepares, records, submits frames; owns histories and frame graph lifetime. |
| Frame graph | `FrameGraph` | Declares passes/resources, compiles dependencies/barriers/transients, executes passes. |
| RHI | `RenderHardwareInterface` | Backend services for resources, descriptors, pipelines, upload, ray tracing, interop, capture, diagnostics, presentation. |

Per-frame renderer flow:

1. `FramePipeline::BeginFrame()` handles resize, scene extent changes, render path switches, image provider graph key changes, backend begin-frame, and timing resolution.
2. `FramePipeline::SetupFrame()` ticks time, refreshes viewport products, captures a scene snapshot, loads scene textures, updates camera, and builds per-frame constants.
3. `FramePipeline::RecordFrame()` builds `FrameContext`, sets provider input contracts, prepares ray tracing scene/TLAS resources, binds exposure and direct-light reservoir histories, runs `FrameGraph::Setup(frame)`, compiles the frame graph, builds pass runtime services, opens GPU frame scope when enabled, and executes the graph.
4. `FramePipeline::SubmitFrame()` submits the current backend frame.
5. `FramePipeline::EndFrame()` marks exposure/reservoir history validity and advances frame-in-flight.

Important CPU observation:

- The frame graph object is rebuilt on resize, render path changes, or provider graph changes.
- The graph setup and compile happen during `RecordFrame()` each frame.
- This is architecturally clear but should be profiled: per-frame setup/compile gives flexibility but can become CPU overhead once pass topology is stable. A future cleanup should cache compiled topology when it can delete more dynamic per-frame planning code than it adds.

## RHI Map

`RenderHardwareInterface` exposes these services:

| Service | Role | Cleanup read |
| --- | --- | --- |
| Capabilities/backend info | Adapter, feature, format, diagnostics, memory support, shader binary format, backend API. | Keep. Essential review signal. |
| `RhiResourceService` | Texture/buffer/resource allocation and views. | Keep. |
| `RhiDescriptorService` | Binding sets, descriptors, descriptor tables, shared samplers, resource views, descriptor usage snapshots. | Keep core binding path; audit public usage snapshot API. |
| `RhiPipelineService` | Pipeline and layout creation. | Keep. |
| `RhiUploadService` | Upload/readback. | Keep. |
| `RhiRayTracingService` | Ray tracing pipelines, BLAS/TLAS services, PTLAS services. | Keep classic TLAS and PTLAS functional; trim PTLAS to minimal D3D12/Vulkan product capability. |
| `RhiInteropService` | Native device/queue/resource handles for provider bridges. | Keep but require explicit consumer enum and narrow allowed use. |
| `RhiCaptureService` | Texture-to-BMP capture. | Preserve as a hardened editor/tool capability; narrow ownership and remove smoke/ad hoc coupling. |
| `RenderDiagnostics` | Object names, GPU events, timing, messages, failure, memory. | Preserve PIX/RenderDoc/Nsight/debug-layer capability; collapse public report shape if noisy. |
| `RhiPresentationService` | Backbuffer/presentation. | Keep. |

Backend split:

- D3D12 backend: command list, device, descriptor heaps, pipeline state/root signatures, resources, memory via D3D12MA, ray tracing, NVAPI PTLAS provider, diagnostics, presentation, swap chain, ImGui backend.
- Vulkan backend: command list/context, descriptor allocator/manager, pipeline/layout/shader modules, resources, memory via VMA, ray tracing/classic/PTLAS services, diagnostics/debug labels/names, presentation, swap chain, ImGui backend.
- Common RHI: shader packages/cache, bindings, capture writer, config/depth conventions, resource validation, type conversions.

RHI strength:

- Explicit service ownership.
- Backend-private native API usage.
- D3D12MA and VMA integrations.
- Optional NVAPI and Vulkan backend gates.
- Capabilities model broad enough for ray tracing/provider decisions.

RHI cleanup risk:

- Public diagnostics/memory/descriptor snapshots look like product API even when they may be developer tooling; screenshot/BMP capture should remain but be narrowly owned.
- `RenderHardwareInterface` is a "god interface" for every service. That can be acceptable for a low-level engine boundary, but it should be documented as the RHI service locator and kept stable.

## Renderer Map

Renderer source weight by area:

| Renderer area | Files | Lines | Read |
| --- | ---: | ---: | --- |
| `Private/FrameGraph` | 43 | 6134 | Core graph ownership: resources, compiler, barriers, transient allocator, execution diagnostics. |
| `Private/Passes` | 43 | 4975 | Typed raster/compute/reference/deferred pass implementations. |
| `Private/RayTracing` | 69 | 4724 | BLAS/TLAS, classic TLAS, PTLAS, scene, effects, metrics. |
| `Private/Frame` | 95 | 4141 | Frame assembly, builders, deferred/reference/lighting/presentation products. |
| `Private/Upscaling` | 27 | 2067 | Upscaler provider contracts and DLSS/Streamline bridge. |
| `Private/RayReconstruction` | 21 | 1766 | Ray reconstruction provider contracts and DLSS-RR bridge. |
| `Private/SceneData` | 27 | 1663 | Scene extraction/builders. |
| `Public/ShaderParameters` | 4 | 1456 | Strong but broad public parameter/binding machinery. |
| `Private/Settings` | 13 | 1287 | Runtime renderer settings/CVars. |
| `Private/Pipeline` | 10 | 1195 | Pipeline state/runtime management. |
| `Private/Diagnostics` | 9 | 1099 | Renderer memory/mesh/texture/status snapshots. |
| `Private/FramePipeline` | 7 | 1015 | Host frame orchestration. |
| `ShaderRegistrations` | 19 | 723 | C++ shader registration and package metadata. |

Feature map:

| Feature | Owner | State |
| --- | --- | --- |
| Frame graph | `FrameGraph` and `Frame/Core/Frame.cpp` factory | Strong foundation with product roots, transient resources, barriers, typed pass parameters. |
| Deferred path | `Frame/Deferred`, `Frame/Lighting`, `Passes/Deferred`, HLSL | GBuffer, direct lighting, reservoir path, shadows, indirect diffuse/specular, composite, sky. |
| Reference path tracing | `Frame/Reference`, `Passes/Reference`, HLSL | Owns reference outputs and guide buffers; motion-vector/accumulation/product story needs sharper scope. |
| Direct-light reservoir | `DirectLightReservoir*` C++ and HLSL | ReSTIR DI-shaped native path; should be tuned/qualified before being marketed as RTXDI-equivalent. |
| Ray tracing scene | `RayTracing/Scene`, `RayTracing/Acceleration`, RHI RT services | Classic TLAS and PTLAS should both be product-owned; remaining ambiguity is scaffolding, not capability. |
| Upscaling | `Upscaling/*`, `Streamline/*` | Provider contract and DLSS bridge. |
| Ray reconstruction | `RayReconstruction/*` | Separate provider category, good architecture. |
| Shader ABI | `ShaderRegistrations`, `RHI/Public/Shaders`, `Tools/Shaders` | Strong source-to-cooked linkage. |
| Memory/diagnostics | `RendererMemoryMonitor`, public snapshots, RHI memory | Useful, but public/API shape should shrink if only editor/debug consumes it. |

## Shader And Cook Pipeline

Shader source shape:

- Shared HLSL include libraries under `Engine/Assets/Shaders/BRDF`, `Common`, `Geometry`, `Lighting`, `Material`, `RayTracing`, `Resources`, `Display`, and `Debug`.
- Pass shaders under `Engine/Assets/Shaders/Passes`.
- C++ shader registrations under `Engine/Renderer/ShaderRegistrations`.
- RHI public shader package, reflection, authoring macros, and pass parameter layout types.
- Shader compiler tool under `Tools/Shaders/ShaderCompiler` with DXC/Slang backends, reflection extractors, contracts, cooking, cache, inspection, CLI commands, and verification.

Strengths:

- Offline compiler and runtime shader package cache are product-level systems.
- Cooked shader packages and reflection give a real ABI story.
- Typed pass parameters make render pass bindings reviewable.
- DXC and Slang support positions the engine for D3D12/Vulkan and future neural rendering paths.

Slimming targets:

- Debug artifact bundles and cooked stats CSV should be opt-in developer tooling or deleted from default workflows.
- Shader registration and HLSL resource declarations remain duplicated; avoid adding a generator unless it deletes materially more code than it adds.
- Launcher shader options expose many compiler/debug toggles. Keep them only if the launcher is a developer workstation product.

## Tools And Workflow Map

Tool areas:

| Tool area | Lines | Read |
| --- | ---: | --- |
| `Launcher/SparkleLauncher` | 18986 | Full workflow product: GUI, shell, quick start, build/cook/launch/clean/dependency/status/quality. |
| `Shaders/ShaderCompiler` | 9452 | Strong CLI/private shader pipeline. |
| `Cooking/TextureCooker` | 4039 | Texture source loading and processing. |
| `Import/SourceImporters` | 3885 | Source asset import, likely Assimp/scene-related. |
| `Cooking/AssetCooker` | 2501 | Multi-stage cook orchestration and diagnostics. |
| `Cooking/SceneCooker` | 1360 | Scene cook output. |
| `Cooking/MaterialCooker` | 605 | Material cook output. |
| `Cooking/MeshCooker` | 316 | Mesh cook output. |
| `Support/ToolConsoleSupport` | 233 | Shared console helper. |
| `Shaders/ShaderContracts` | 102 | Shared shader contract catalog. |

Launcher read:

- The launcher is large enough to be judged as an application.
- The launcher GUI owns one implicit repository content root. It does not expose project discovery or selection; the content model rejects ambiguous repositories instead of choosing among multiple roots.
- The GUI repository root comes exclusively from the generated `RepositoryRoot.txt` deployed beside the launcher artifact and copied unchanged into live shadow generations. Startup validates that exact root and fails explicitly when the deployment context is missing or invalid; command-line arguments, process working directory, and executable ancestry are not alternate GUI authorities.
- `Quick Start` is the primary product path. Host-tool, source-dependency, workspace, level, cook, and launch providers independently register capabilities and hierarchical dependencies with a generic resolver; it re-evaluates the graph after every completed operation and automatically drives only the missing work. Editor, runtime, and IDE opening are graph goals, while manual pages retain direct access to sync, build, cook, and clean operations for diagnosis.
- The generic capability graph is independent of Qt and launcher operation types. It supports any number of registered dependencies, validates its complete topology before every resolution, rejects missing or duplicate registrations and edges, detects cycles, short-circuits already-ready branches, propagates blockers, and derives direct-consumer invalidations from successful operations. Operation requests are explicit optional products of evaluation rather than default-constructed placeholders.
- Domain provider files own host-tool, source-dependency, workspace, level, cook, and launch readiness/action policy. The planner is the composition root for those providers, `LauncherQuickStartExecution` owns one run's active-step/invalidation/stall state, and the main window is limited to presenting progress and dispatching the selected operation.
- `Sync Code` owns a flat dependency list: a row action populates only that dependency through an isolated CMake configure sharing `build/_deps`, while the page footer syncs the enabled code set. Neither path enables workspace features or acquires level content. Level acquisition is a separate UI-independent `LevelOperations` domain with the canonical `levels.sync` operation; it does not enter `BuildWorkspaceOperationKind`. The GUI workflow catalog is a typed presentation projection whose ordering places `Levels` below `Cook`, while the shell projects the same backend definition as its own `Levels` group. A focused catalog test rejects backend/frontend ownership drift and the removed `workspace.sync-levels` identity.
- Operational `Build` and `Cook` pages remain single-operation diagnostic surfaces: inline status communicates current state and running the page executes only that selected operation. Automatic dependency traversal belongs exclusively to the capability graph instead of hardcoded prerequisite prompts. The former Launch page and launcher-owned VSync, GPU-preference, argument, and CVar overrides were removed; applications own those defaults. The top context bar owns only startup level and graphics API, while build configuration, compiler, and IDE live in the persistent footer below activity output.
- Context selectors are read-only projections of their existing authorities: the level catalog supplies launchable maps, the build-profile catalog supplies configurations, and toolchain detection supplies graphics SDK, compiler, and IDE evidence. Each popup groups usable choices under `Available` and keeps recognized choices that still need setup visible but disabled under `Supported`; refresh rebuilds the projection after launcher operations or application activation. Visual Studio IDE availability requires an actual `devenv.exe` and remains separate from Visual Studio C++ Build Tools availability.
- It currently models dry-run plans, logs, dependency state, GUI status pages, operation catalogs, and build/cook/launch/maintenance requests. Distribution packaging is intentionally manual and is not a launcher responsibility.
- This is useful for productization, but it should not keep validation/report/debug scaffolding alive.
- Preferred target: launcher as a small workflow shell with one automatic quick-start path plus direct sync, build, cook, clean, and source dependency controls for diagnosis.

Quick Start capability ownership:

```text
Quick Start goals
|-- launch.<product>
|   |-- content.project.<product>
|   |-- product.<product>
|   |   `-- workspace.build-files
|   |-- content.selected-levels
|   `-- content.cooked
|       |-- content.selected-levels
|       `-- content.cooking-tools
|           `-- workspace.build-files
`-- workspace.open-ide
    `-- workspace.build-files
        `-- workspace.source-dependencies
            `-- source-dependency.<enabled-id> (one node per enabled dependency)
                `-- workspace.host-tools
                    `-- host-tool.<required-id> (one node per selected-toolchain requirement)
```

Each provider owns only its registrations and readiness evaluators. Host-tool nodes come from the selected toolchain, and source-dependency nodes come from the authoritative dependency inventory, so new dependencies join Quick Start without adding orchestration branches or expanding workspace ownership. Host-tool installer definitions are registered separately from detection and orchestration; a selected missing compiler can therefore contribute its own launcher operation while unavailable tools without a safe installer remain explicit blockers. An installer exit is not sufficient evidence: the executor refreshes the toolchain inventory and reports success only when the requested tool is detected. Compiler selection is a typed launcher request, not an environment-variable side channel, and build-file freshness includes the resulting CMake toolset. A generator, platform, toolset, SDK, or output-contract mismatch schedules one native build-output reset before configure; downloaded dependency sources and cooked content remain intact while compiler-produced build state, binaries, libraries, and symbols are removed. Because `content.cooked` directly depends on `content.selected-levels`, a successful level sync invalidates cooked content through graph topology rather than cross-provider knowledge; the next resolution therefore schedules an incremental cook even when older cooked directories still contain files. Every automatic activity log records the resolved capability path and selected operation.

Cooker read:

- AssetCooker has plan/timing/summary artifact concepts that may be too heavy for default cooking.
- A lean product cooker should emit cooked outputs and clear fatal failures, not durable diagnostic report schemas by default.

## Memory Map

Current memory-related ownership:

| Layer | Existing surface | Read |
| --- | --- | --- |
| RHI backends | D3D12MA and VMA allocation backends, allocator snapshots, budget support, delayed destruction tracking. | Strong. This is a production-recognized foundation. |
| RHI public | `RhiMemoryUsageSnapshot`, category stats, allocation details, JSON dump support. | Useful but broad. |
| Renderer public | `RendererMemoryDiagnosticsSnapshot`, pressure thresholds, texture streaming policy snapshot, scene memory report. | Product value if it drives streaming; bloat if only displayed. |
| Renderer private | Memory monitor and texture/mesh/ray tracing categories. | Good place for policy. |
| Content depot | The initial audit measured `Projects/Showcase` at about 2.79 GB. | Biggest memory/depot pressure signal was content packaging, not code. |

Best next shape:

- Keep allocator-backed memory budget and usage.
- Keep texture streaming decisions if they affect runtime behavior.
- Keep one compact memory pressure/status object if the editor needs it.
- Delete JSON dumps, largest-allocation detail lists, and public report structs unless they are part of a real memory workflow.
- Externalize heavyweight showcase content so repository size does not dominate every clone/review.

## GPU Performance Map

Current GPU performance strengths:

- Explicit D3D12/Vulkan backends.
- D3D12MA/VMA allocator integration.
- Frame graph resource declarations, barriers, transient allocator, and product roots.
- GPU marker/timing support through RHI diagnostics and frame execution diagnostics.
- Ray tracing scene preparation with BLAS/TLAS ownership.
- Provider contracts for DLSS upscaling and ray reconstruction guide resources.
- HLSL layout and shader package ABI validation.

GPU performance risks:

- PTLAS code mixes shipping path, capability selection, classic fallback, metrics, and future GPU-pack scaffolding. It should be refactored to the bare functional reference-style path while preserving D3D12 and Vulkan capability.
- ReSTIR DI-shaped direct lighting needs measured quality/perf comparison against representative many-light scenes.
- Reference path tracing should be either an offline/progressive quality reference or a scoped debug reference; ambiguous modes attract extra buffers and settings.
- Provider resource contracts should stay decisive; do not add fallback provider objects or diagnostic panels as a substitute for correct signals.

## CPU Performance Map

CPU-sensitive surfaces:

| Surface | Current behavior | Risk |
| --- | --- | --- |
| Frame graph setup/compile | Runs every recorded frame. | Flexible, but could become CPU overhead. Cache topology if it deletes more dynamic planning code than it adds. |
| Scene snapshot/build frame context | Captures scene snapshot, builds render scene data, ray tracing plan, per-view/temporal data. | Needs profiling on large scenes. |
| Texture loading | `TextureManager::LoadSceneTextures` during setup. | Ensure it is incremental/cache-backed; avoid per-frame redundant scans. |
| Shader package cache | Large runtime loader file. | Strong, but watch startup/cook/runtime split. |
| Launcher | Qt GUI and CLI planning for many workflows. | Large workstation app overhead, not runtime overhead. |
| Cookers/importers | Source pipeline tools. | Keep out of runtime package. |

## Coding Pattern And Style Map

Observed patterns:

- C++20 across modules.
- `Public`/`Private` module directories.
- CMake `GLOB_RECURSE` with `CONFIGURE_DEPENDS` for module source ownership.
- Service interfaces for RHI subsystems.
- RAII and move/delete semantics for owning runtime classes.
- `final` on many structs/classes where appropriate.
- `noexcept` used broadly in runtime code.
- PCH per module.
- Shader authoring macros modeled after Unreal-like global shader and parameter structs.
- Explicit static/dynamic library switches via `SPARKLE_BUILD_SHARED`.
- Build profiles for Debug/Development/Shipping and editor/game variants.
- Boundary check as a CMake target.

Style risks:

- CMake source ownership is convenient but less reviewable than explicit file lists for release-critical modules.
- Formatter workflow is launcher-owned, not root-owned.
- Debug/report concepts often become structs and public APIs rather than internal, transient views.
- A few UI files and tool planners concentrate many unrelated branches in one file.

Normative coding, graphics, neural, driver, and evidence implications belong in [Engineering Standards](../Engineering/Standards/README.md); this section records only observed repository patterns and risks.

## System Links

| Producer | Consumer | Contract |
| --- | --- | --- |
| GameFramework world extraction | Renderer input consumer/RenderWorld | Sequenced `RenderWorldDelta` plus immutable `RenderFrameDynamicData` and stable resource handles. |
| Renderer frame graph | RHI services | Resource, descriptor, pipeline, command, ray tracing, interop, presentation services. |
| Shader registrations | ShaderCompiler | Package names, source paths, entries, stages, parameters. |
| ShaderCompiler | RHI runtime shader cache | Cooked shader package binary, reflection, layout metadata. |
| Asset cookers | GameFramework loaders | Cooked scene/mesh/material/texture package contracts. |
| Launcher | CMake/tools/projects | Build/cook/launch/clean process requests. Distribution assembly remains manual and outside launcher ownership. |
| Renderer providers | Streamline/DLSS | Tagged resources and native interop. |
| Editor | Renderer public diagnostics | Mesh/texture/memory panels. |
| Application | Renderer/Editor | Host frame orchestration and editor application integration. |

## Snapshot Limitations

Counts, file paths, implemented capabilities, and identified risks describe the 2026-07-24 observation. They are not proof of current behavior and should not be copied into plans or standards. Use the source locations below to repeat the audit against the current worktree.
## Local Source Evidence

Primary local files reviewed:

- `CMakeLists.txt`
- `Engine/CMakeLists.txt`
- `Engine/RHI/CMakeLists.txt`
- `Engine/Renderer/CMakeLists.txt`
- `Engine/Application/CMakeLists.txt`
- `Tools/CMakeLists.txt`
- `CMake/ArchitectureBoundaryCheck.cmake`
- `Engine/RHI/Public/Device/RenderHardwareInterface.h`
- `Engine/RHI/Public/Descriptors/RhiDescriptorService.h`
- `Engine/RHI/Public/Memory/RhiMemoryDiagnostics.h`
- `Engine/RHI/Public/Diagnostics/RhiDiagnostics.h`
- `Engine/Renderer/Public/Renderer.h`
- `Engine/Renderer/Private/FramePipeline/FramePipeline.h`
- `Engine/Renderer/Private/FramePipeline/FramePipeline.cpp`
- `Engine/Renderer/Private/Frame/Core/FrameAssembly.h`
- `Engine/Renderer/Private/FrameGraph/FrameGraph.h`
- `Engine/Renderer/Public/Diagnostics/RendererMemoryDiagnostics.h`
- `Tools/Launcher/SparkleLauncher/Private/Maintenance/MaintenanceOperations.cpp`
- `Tools/Launcher/SparkleLauncher/Private/Gui/Shell/LauncherMainWindowOptionPages.cpp`

# D. Whole Repository Architecture Map

Status: source-backed repository map
Date: 2026-07-24
Scope: full depot structure, module boundaries, runtime flow, tools, public/private API shape, memory, GPU/CPU performance surfaces, neural-graphics evidence, developer-technology transfer, extensibility, and productization risk

## Intent

This document maps SparkleEngine as a whole repository, not only as a renderer. It is written for high-level planning and deep review. The goal is to make architecture decisions easier by showing what exists, how systems link together, where code weight lives, what is public versus private, and which surfaces are worth slimming before adding features.

The guiding preference is a thinner depot with stronger ownership:

- Keep product behavior, fatal correctness checks, graphics debugger support, screenshot/BMP capture, explicit API control, shader ABI safety, and a small set of real workflows.
- Delete or collapse report-only, smoke-only, validation-only, debug-artifact, wrapper-only, and future-scaffold code unless it is part of the product.
- Do not add new documentation, diagnostics, logging, validation, wrappers, abstractions, or scaffolding before cleanup.
- Prefer net code and depot size removal for every cleanup change.

## Executive Map

SparkleEngine is already shaped like a modern rendering engine:

- `Engine/RHI` provides D3D12 and Vulkan backends behind a common RHI service surface.
- `Engine/GameFramework` owns world-to-render extraction into immutable structural/dynamic input, while `Engine/Renderer` owns render-world consumption, frame graph, passes, ray tracing scene ownership, upscaling/ray reconstruction provider contracts, shader registrations, and render-owned diagnostics.
- `Tools/Shaders/ShaderCompiler` is a serious offline shader pipeline with DXC/Slang backends, reflection, contracts, cache, package cooking, and inspection commands.
- `Tools/Cooking` and `Tools/Import` form a source-to-cooked content pipeline.
- `Tools/Launcher/SparkleLauncher` is a workflow shell for build, cook, launch, package, clean, dependency, quality, and GUI flows.
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

## Industry Requirements Overlay

Advanced graphics and neural rendering expectations add a sharper target for this repo. Sparkle should become evidence for:

| Requirement cluster | What the repo should prove | Current evidence | Direction |
| --- | --- | --- | --- |
| D3D12/Vulkan workload analysis | Late ability to analyze how modern engines use explicit graphics APIs. | D3D12/Vulkan RHI, frame graph, boundary checks, descriptor/memory snapshots. | Defer until feature cleanup; do not add new diagnostics now. |
| High-level shader engineering | Efficient HLSL SM6/Slang shader code, reflection, package ABI, and shader optimization. | Strong shader compiler, typed shader registrations, HLSL libraries. | Keep this as a centerpiece; trim debug artifacts and non-product shader demos. |
| Real-time rendering fundamentals | Rasterization, physically based shading, sampling, light transport, GI, path tracing. | Deferred path, BRDF libraries, reservoir direct lighting, reference path tracing. | Make reference mode honest and tune direct lighting against representative scenes. |
| Ray tracing architecture | BLAS/TLAS lifecycle, ray queries/tracing, GI/path tracing integration. | Classic TLAS plus PTLAS path. | Preserve both classic TLAS and PTLAS; minimize PTLAS toward the reference implementation and cut research scaffolding. |
| GPU debugging/capture | PIX/RenderDoc/Nsight marker, timing, object naming, API debug layer fluency, screenshot/BMP capture. | RHI diagnostics, frame execution diagnostics, capture writer. | Preserve debugger/capture support while deleting bespoke reports/logs. |
| GPU architecture and memory | Cache, bandwidth, memory budget, descriptors, pipeline pressure. | Allocator-backed memory and descriptor usage snapshots. | Keep compact pressure facts; remove broad public observation APIs. |
| Neural rendering implementation | Translate model/operator ideas into efficient GPU shader/kernel paths and tune the model/runtime quality-performance frontier. | Slang path and shader ABI foundation; no completed product-owned neural feature yet. | Preserve readiness, then implement one real replacement feature with deterministic model artifacts, classical fallback, and quality/performance evidence. |
| AI/ML workload depth | Understand datasets, loss/metrics, training/export, precision/layout, inference, batching, and deployment. | Shader/cook infrastructure can host immutable runtime artifacts; no owned training/inference evidence yet. | Keep training/offline work isolated, add only a feature-owned deterministic export/cook path, and profile training and inference separately. |
| Math and performance modeling | Apply linear algebra, calculus, numerical analysis, sampling, reconstruction, and cost models to real features. | Math/shader foundations and profiler hooks exist, but derivations are not a repository-wide gate. | Require feature math notes plus executable reference tests and predicted-versus-measured cost for material algorithm changes. |
| Architecture/driver collaboration | Diagnose current hardware/driver behavior and prepare capability-driven paths for future hardware. | Explicit D3D12/Vulkan backends, validation, capabilities, and provider bridges. | Record exact adapter/driver/configuration, keep reduced reproducers, distinguish application defects from driver behavior, and avoid untested universal claims. |
| Partner technology adoption | Integrate advanced rendering/AI into another team's product constraints with a narrow handoff surface. | Module boundaries and provider contracts are a foundation; current evidence is internal. | Require one partner-shaped integration case study, adoption path, failure/fallback contract, and reproducible demo. |
| Tooling and productization | Developer tools that streamline graphics workflows. | Launcher, shader compiler, cookers, package assembly. | Slim launcher/tools to current product workflows and workload inspection. |
| Communication and standards | Clear docs, coding standards, reviewability, prioritization, whitepaper-quality analysis, and conference-ready explanation. | Architecture docs, style guide, and boundary check. | Keep docs precise and code-backed; produce polished communication artifacts only for completed strategic work. |

Advanced-graphics implication:

- The repo should show fewer systems, deeper ownership, and stronger evidence. A reviewer should see graphics API control, shader/compiler expertise, performance reasoning, a real neural-graphics result, driver/hardware diagnosis, and adoption-quality communication without needing to read thousands of lines of diagnostic scaffolding.

## Principal Graphics Engineering Repository Contract

The canonical `PGE-01` through `PGE-15` requirements live in [A. Principal Graphics Engineering Requirements](A_PrincipalRenderingRequirements.md). This repository map applies them to physical ownership:

| Repository surface | Required principal-level evidence | Structural constraint |
|---|---|---|
| `Engine/Core` | Strong math/value/identity primitives required by real consumers; portable diagnostics bootstrap | No AI, renderer, model, or driver policy |
| `Engine/Tasks` | Bounded CPU execution, cancellation, determinism, topology/latency evidence | No neural scheduler, tensor runtime, renderer policy, or third-party model framework |
| `Engine/GameFramework` | Stable simulation/animation inputs and immutable extraction for rendering or AI-driven behaviors where a real feature needs them | No GPU inference ownership, renderer access, or heavyweight model assets in hot components |
| `Engine/Renderer` | Path-traced and neural feature ownership, persistent GPU data, quality/performance policy, classical fallback, frame integration | No ECS query, generic ML framework, training loop, or backend-native leakage |
| `Engine/RHI` | Explicit D3D12/Vulkan resources, synchronization, pipeline/shader capabilities, timestamps, native validation, future-feature capability discovery | No model semantics, neural feature policy, or renderer-specific tensor graph |
| `Engine/Editor` | Narrow feature selection, immutable result/quality comparison, capture, and product-owned model metadata only where a current workflow needs it | No training UI, task debugger, model-graph browser, cache browser, or live renderer pointer |
| `Engine/Application` | Host lifecycle, reproducible workload selection, immutable publication, and bounded operation composition | No algorithm/model implementation or driver workaround policy |
| `Tools/Shaders` | HLSL/Slang, DXIL/SPIR-V, reflection, precision/layout contracts, deterministic shader/model-derived package inputs | No second runtime shader/model schema |
| `Tools/Cooking` and `Tools/Import` | Deterministic feature-owned model/artifact validation and cooking when a neural feature exists | No general ML platform, opaque downloaded artifact, or nondeterministic publication |
| `Projects/Showcase` | Curated path tracing/neural comparison workload, classical baseline, stress case, capture script, and live demo | No uncataloged heavyweight dataset or feature logic hidden in project callbacks |
| `Docs` | Source-backed design decision, math/algorithm explanation, integration case study, incident report, reproducible results, whitepaper/talk artifact | No documentation used to imply implementation or create another competing policy |
| `CMake` and packaging | Optional dependency/capability gates, deterministic artifacts, platform/backend matrix, runtime/editor/tools/content separation | No unconditional training runtime or vendor SDK dependency in the core product |

Repository-wide requirements:

- One actual neural graphics feature is a final target; empty tensor types, model registries, capability flags, or provider placeholders are forbidden.
- Model/training artifacts have provenance, license, deterministic export/cook identity, validation, and package ownership. Runtime code consumes only validated immutable artifacts.
- Quality is measured against a named classical baseline with feature-appropriate metrics and visual failure cases. Performance includes CPU, GPU, memory, latency, and frame-pacing cost.
- Training/offline tuning and runtime inference remain separate workloads with separate owners, dependencies, measurements, and packaging.
- Math-heavy changes carry a derivation/reference note and executable validation close to the owning feature. A copied equation or generated shader is not self-validating.
- Hardware/driver investigations record adapter, architecture, driver, OS, backend, compiler, feature capabilities, and exact reproduction. Suspected driver defects require a minimal reproducer before escalation.
- Windows is not evidence of Linux support. Linux/Vulkan support becomes a claim only after a native build, run, validation, capture, packaging, and ownership audit.
- AI-assisted code or design follows the same review, source, security, determinism, ABI, correctness, and performance gates as human-authored work.
- Partner readiness is judged by adoption cost: narrow contracts, explicit prerequisites/fallbacks, reproducible setup, issue diagnosis, and a handoff another engineer can follow.
- Strategic completed work produces a concise demo, whitepaper-quality note, and talk outline; routine prompts do not manufacture presentation artifacts.

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

Current curated default level set:

- `Empty`
- `DamagedHelmet`
- `CesiumMan`
- `DiffuseTransmissionPlant`
- `ABeautifulGame`
- `Sponza`
- `SponzaPtlas`

Startup default:

- `Sponza`, matching current launcher/runtime defaults.

Level inventory:

| Level File | Level Name | Scene Asset Refs | Unique Scene Assets | Default Set |
| --- | --- | ---: | --- | --- |
| `Projects/Showcase/Levels/Empty.level` | `Empty` | 0 | none | Yes; fallback/minimal scene. |
| `Projects/Showcase/Levels/DamagedHelmet.level` | `DamagedHelmet` | 1 | `DamagedHelmet/DamagedHelmet` | Yes; small material/mesh check. |
| `Projects/Showcase/Levels/CesiumMan.level` | `CesiumMan` | 1 | `CesiumMan/CesiumMan` | Yes; animation/skinned asset check. |
| `Projects/Showcase/Levels/DiffuseTransmissionPlant.level` | `DiffuseTransmissionPlant` | 1 | `DiffuseTransmissionPlant/DiffuseTransmissionPlant` | Yes; material/transmission asset check. |
| `Projects/Showcase/Levels/ABeautifulGame.level` | `ABeautifulGame` | 1 | `ABeautifulGame/ABeautifulGame` | Yes; medium mesh/material scene. |
| `Projects/Showcase/Levels/Sponza.level` | `Sponza` | 1 | `Sponza/Sponza` | Yes; current renderer review scene and startup default; future Tier 0 regression workload. |
| `Projects/Showcase/Levels/SponzaPtlas.level` | `SponzaPtlas` | 11 | `Sponza/Sponza`, `CesiumMan/CesiumMan` | Yes; PTLAS/multiple-instance coverage. |

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
- Preserve all seven levels as the current default level set.
- Treat logs and cooked outputs as later cleanup/package-boundary candidates. Bistro was externalized from the core repo in Stage 07.

Stage 05 catalog result:

- `Projects/Showcase/Levels.catalog` is now the tiny project-owned level catalog.
- Each default level has `Id`, `DisplayName`, `Source`, and `Default` metadata.
- Runtime level discovery loads catalog levels whose source and optional pack are available.
- Asset cook discovery reads catalog `Default` levels and filters project source scenes to the level-referenced scene assets.
- Optional content pack state is represented by `OptionalPack` on levels and `Id`/`Root`/`Available` pack metadata; `Bistro` remains cataloged after removal from the core tree.

Stage 06 optional pack boundary:

- Optional pack ownership is project-owned. `Projects/Showcase/Levels.catalog` is the single boundary for Showcase optional pack metadata.
- Optional pack root is the catalog `Root` value relative to `Projects/Showcase`; current root is `Assets/Meshes/Bistro` for pack `Bistro`.

### Canonical Acceptance-Workload Overlay

The inventory above describes current repository truth. It does not define the future evidence bar.

[I. Bistro and San Miguel Acceptance Workloads](I_BistroAcceptanceWorkload.md) adds this product direction:

| Tier | Scene | Current repository state | Required architecture consequence |
| --- | --- | --- | --- |
| Tier 0 | Sponza | Available, required, startup default. | Preserve as the low-cost smoke/regression loop. |
| Tier 1 primary | Bistro exterior/interior | External/unavailable catalog pack; no runnable level. | Re-establish through deterministic provenance/import/cook manifests, material support records, frozen routes, and benchmark/reference artifacts; keep heavyweight media external. |
| Tier 1 secondary | San Miguel 2.0 | Not cataloged or present. | Add as a second external optional pack through the same generic discovery/import/cook/level path; no scene-specific importer, shader, renderer, or scheduler branch. |

Both Tier 1 scenes must exercise the same scene asset, material, texture, renderer, RHI, ray-tracing, benchmark, and capture ownership. Differences belong in authored data, conversion manifests, camera routes, and declared capability/fallback records.

This makes the external-pack boundary a product requirement rather than a depot-cleanup convenience. Completion evidence is defined in I; this map continues to report only what currently exists.
- Default levels do not reference `Bistro`, so default build/cook/run does not require the heavy optional pack.
- Missing optional pack state is non-fatal: catalog levels that name a missing or disabled optional pack are unavailable, while default catalog levels continue to load/cook.
- Core repo byte reduction target for Stage 07 is at least 1438.80 MB by removing or externalizing `Projects/Showcase/Assets/Meshes/Bistro`, reducing `Projects` source content from about 1527.06 MB to about 88.26 MB before generated-output cleanup.

Stage 07 externalization result:

- `Projects/Showcase/Assets/Meshes/Bistro` was removed from the core repo.
- `Projects/Showcase/Levels.catalog` keeps pack `Bistro` discoverable as `Root = Assets/Meshes/Bistro`, `Available = false`, `External = true`.
- Curated default level set remains intact: seven level files resolve and the five default source scene IDs still resolve.
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
| `Launcher/SparkleLauncher` | 18986 | Full workflow product: GUI, shell, build/cook/launch/package/clean/dependency/status/quality. |
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
- It currently models dry-run plans, logs, dependency state, GUI status pages, operation catalogs, build/cook/launch/maintenance requests, and package assembly.
- This is useful for productization, but it should not keep validation/report/debug scaffolding alive.
- Preferred target: launcher as a small workflow shell for build, cook, run, clean, package if shipping, and source dependency sync if truly needed.

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

Principal-role coding implications:

- Algorithm names must describe the mathematical or rendering operation, not the research brand.
- Tensor/model metadata must expose bounded shape, layout, precision, ownership, and lifetime; it must not become a generic `void*` graph or service locator.
- CPU and GPU kernels require explicit input/output domains, coordinate/precision conventions, numerical limits, and a measurable fallback.
- Capability and backend decisions occur at a narrow policy boundary; frame-hot code consumes pre-resolved policy and packed data.
- Model inference, shader dispatch, readback, capture, and training/export must not share one god service.
- Driver workarounds are backend-private, scoped to exact vendor/device/driver evidence, documented with a removal/retest condition, and never leak as renderer-wide policy.
- Performance experiments use existing profiler/debugger hooks and disposable analysis, not permanent per-operator logging or a new telemetry product.

## System Links

| Producer | Consumer | Contract |
| --- | --- | --- |
| GameFramework world extraction | Renderer input consumer/RenderWorld | Sequenced `RenderWorldDelta` plus immutable `RenderFrameDynamicData` and stable resource handles. |
| Renderer frame graph | RHI services | Resource, descriptor, pipeline, command, ray tracing, interop, presentation services. |
| Shader registrations | ShaderCompiler | Package names, source paths, entries, stages, parameters. |
| ShaderCompiler | RHI runtime shader cache | Cooked shader package binary, reflection, layout metadata. |
| Asset cookers | GameFramework loaders | Cooked scene/mesh/material/texture package contracts. |
| Launcher | CMake/tools/projects | Build/cook/launch/package/clean process requests. |
| Renderer providers | Streamline/DLSS | Tagged resources and native interop. |
| Editor | Renderer public diagnostics | Mesh/texture/memory panels. |
| Application | Renderer/Editor | Host frame orchestration and editor application integration. |

## Architecture Grades

Scale: 1 weak, 3 credible, 5 production-sharp.

| Quality | Grade | Why |
| --- | ---: | --- |
| Module layering | 4.0 | Good bottom-up shape; Renderer consumes GameFramework privately; boundary check exists. |
| RHI explicitness | 4.0 | Strong services, D3D12/Vulkan backends, memory allocators, ray tracing; public diagnostics/capture widen surface. |
| Renderer frame architecture | 3.8 | Real frame graph/pass system/history/provider ownership; per-frame compile and broad assembly structs need review. |
| Shader pipeline | 4.4 | One of the strongest product systems: compiler, reflection, contracts, cache, cook, inspection. |
| Ray tracing architecture | 3.4 | Classic TLAS and PTLAS are both valuable; PTLAS needs minimization and clearer ownership. |
| Provider integration | 3.6 | Upscaling and ray reconstruction are separated well; Streamline bridge should remain narrow. |
| Memory model | 3.5 | Allocator-backed and visible; too much report shape may be public. |
| CPU performance posture | 3.0 | Clear frame pipeline, but per-frame graph setup/compile and scene extraction need evidence. |
| GPU performance posture | 3.5 | Strong explicit API foundation; needs representative benchmark discipline. |
| Public API minimalism | 3.2 | Renderer is mostly private; RHI and Core public surfaces should be pruned. |
| Tooling productization | 3.0 | Launcher is polished but large; cookers/report artifacts should be narrowed. |
| Depot hygiene | 2.0 | Showcase content dominates depot size; levels need cataloging so capability stays broad while default footprint shrinks. |
| Deletion readiness | 4.0 | Many cleanup targets are identifiable and isolated enough for staged removal. |

## Principal Conclusions

1. The core renderer/RHI architecture is worth preserving. Do not replace it with a new abstraction.
2. The first big win is depot size: catalog Showcase levels, keep multi-level support, and move heavy media out of git or into optional content packs.
3. The second big win is deleting validation/report/debug scaffolding, especially around launcher, cookers, shader debug artifacts, and public diagnostics, while preserving hardened screenshot/BMP capture.
4. PTLAS should remain a named product feature alongside classic TLAS, be minimized toward the original reference implementation, and be stripped of future GPU-pack placeholders.
5. The shader compiler is a strength. Slim defaults and debug artifacts, but keep the source-to-package ABI.
6. The launcher should be treated as a product with a smaller mission, not as a home for every local workflow.
7. Public API should shrink around behavior, not observation. Keep runtime contracts; move or delete report APIs.
8. Neural readiness is now a prerequisite, not completion. The repository ultimately needs one real, replacement-based, quality-and-performance-validated neural graphics feature.
9. Principal-level evidence must connect math, C++/shader implementation, CPU/GPU architecture, API/driver behavior, and product adoption rather than presenting them as separate demonstrations.
10. A partner-shaped integration case, reduced driver/hardware investigation, and whitepaper/demo-quality explanation are required portfolio outputs of completed work, not new runtime systems.

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

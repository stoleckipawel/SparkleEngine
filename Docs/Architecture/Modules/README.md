# Module Architecture And Capability Inventory

Status: module architecture index and dated capability snapshot; not release approval or executable evidence

Snapshot: 2026-09-06 at committed `master` revision `8414b5dc`; the detailed inventories listed below inspected their named source/build surfaces in the live working tree; no build, shader cook, launch, GPU capture, performance run, package run, or clean-machine run was performed for this inventory

Scope: navigation by repository module plus current engine, tool, project, and build capabilities, their exact coverage and limits, and the evidence still required before a release claim

Release classification authority: [First Release Acceptance Contract](../../Acceptance/FirstRelease.md)

Per-feature candidate report authority: [First Release Feature Completion Reports](../../Acceptance/FeatureCompletionReports.md)

Sequencing authority: [Release-First Principal Graphics Roadmap](../../Strategy/Roadmap.md)

## Browse By Repository Boundary

| Boundary | Modules |
| --- | --- |
| [Engine](Engine/README.md) | Application, Assets, Core, Editor, GameFramework, Platform, Renderer, RHI, Tasks |
| [Tools](Tools/README.md) | Cooking, Launcher, ShaderCompiler, SourceImporters, ToolSupport |
| [Projects](Projects/README.md) | Showcase product composition and workloads |
| [Build And Packaging](BuildAndPackaging.md) | repository-wide build, dependency, staging, installation, and packaging surfaces |
| [CrossModule](../CrossModule/README.md) | systems that genuinely span multiple durable owners without one primary module |

Physical placement follows ownership: Renderer details stay under `Engine/Renderer`, RHI details under `Engine/RHI`, and relationships are expressed through links. The inventory below provides the repository-wide evidence ledger without replacing those module routes.

## Purpose

This inventory answers four different questions without collapsing them into one:

1. What contract or implementation path exists in the current tree?
2. Which backends, modes, resources, materials, passes, or tools does that path actually cover?
3. What is incomplete, capability-gated, vocabulary-only, or absent from that path?
4. What evidence is still needed before the capability can be included in a release?

Code and executable build configuration remain the authority for implementation. These files are a dated reviewer map. Reconcile a row with current producers, consumers, build membership, and public selection surfaces whenever any of those change.

## How To Read A Row

### Implementation state

| State | Meaning |
| --- | --- |
| **Implemented path** | Current build membership and source contain the named producer-to-consumer path. This is not proof that it builds or works on a machine. |
| **Capability-gated** | The path exists, but build options, SDKs, device features, vendor support, or optional binaries decide whether it can run. |
| **Partial** | A useful path exists, but the row names material coverage gaps, unsupported variants, or an unfinished product boundary. |
| **Vocabulary only** | An enum, schema, flag, or low-level primitive exists without a current product producer/consumer path. It must not be advertised as supported. |
| **Not yet audited** | This inventory has not reconciled the module deeply enough to make a feature claim. It does not mean absent. |
| **Not found** | A targeted source/build search did not find the named capability in this snapshot. This remains a dated observation, not a permanent prohibition. |

### Evidence state

| Mark | Evidence |
| --- | --- |
| `S` | Source and executable build membership reconciled for the stated scope. |
| `B` | Relevant targets built from the recorded revision/configuration. |
| `R` | The capability was exercised through its real runtime or tool consumer. |
| `N` | Native validation or an external GPU capture checked the backend claim. |
| `P` | Performance and resource behavior were measured on a named machine/workload. |
| `A` | The release owner approved the row as Included, Experimental, Excluded, or Removed. |

The initial detailed inventories carry `S` only. No `B`, `R`, `N`, `P`, or `A` evidence was produced by this documentation pass. A source-present row therefore remains **Pending** for release disposition. Candidate-bound results belong in the feature completion reports and their retained artifacts, not in this dated source snapshot.

## Detailed Inventories

| Module | Depth in this snapshot | Document | Current evidence boundary |
| --- | --- | --- | --- |
| Application | Detailed | [Application Capability Inventory](Engine/Application/README.md) | Runtime/editor hosts, configuration, loop composition, console, recook, capture coordination, startup/shutdown, and build split reconciled; source-only. |
| Build and packaging | Detailed | [Build And Packaging Capability Inventory](BuildAndPackaging.md) | CMake profiles, toolchains, dependencies, targets, artifacts, staging, discovery, checks, automation, tests, installation, and packaging reconciled; source-only. |
| Core | Detailed | [Core Capability Inventory](Engine/Core/README.md) | Public/private source, CMake membership, diagnostics, configuration, files, processes, serialization, input, math, time, and thread vocabulary reconciled; source-only. |
| Editor | Detailed | [Editor Capability Inventory](Engine/Editor/README.md) | Workspace, viewport, level/editing actions, settings, tools, console, capture, restart, runtime separation, and build surfaces reconciled; source-only. |
| Engine asset corpus | Detailed | [Engine Assets Capability Inventory](Engine/Assets/README.md) | Tracked shaders, default textures, sky environments, fixtures, and known transformation/consumption paths reconciled; source-only. |
| GameFramework | Detailed | [GameFramework Capability Inventory](Engine/GameFramework/README.md) | Levels, cooked loading, world/ECS, editing, publication, and render extraction reconciled; source-only. |
| Launcher | Detailed | [Launcher Capability Inventory](Tools/Launcher/README.md) | Discovery, readiness, configure/build/cook/acquire/run/clean workflows, history, cancellation, GUI/shell access, and build membership reconciled; source-only. |
| Asset cooking | Detailed | [Asset Cooking Capability Inventory](Tools/Cooking/README.md) | Asset/texture/mesh/material/scene cooking, validation, concurrency, publication, runtime handoff, and build membership reconciled; source-only. |
| Platform | Detailed | [Platform Capability Inventory](Engine/Platform/README.md) | Window, DPI, message, input, routing, capture, cursor, and build surfaces reconciled; source-only. |
| RHI | Detailed | [RHI Capability Inventory](Engine/RHI/README.md) | Public contracts, both backend implementations, build switches, and Renderer consumers reconciled; source-only. |
| Renderer | Detailed | [Renderer Capability Inventory](Engine/Renderer/README.md) | Public settings/facade, Scene/View/Frame ownership, frame graph, passes, providers, and shader registrations reconciled; source-only. |
| Shader compilation and delivery | Detailed | [Shader Compilation Capability Inventory](Tools/ShaderCompiler/README.md) | Tool build, CLI, typed registrations, compiler backends, validation, publication, editor recook, and runtime loading reconciled; source-only. |
| Showcase products | Detailed | [Showcase Product Capability Inventory](Projects/Showcase/README.md) | Editor/runtime targets, startup/selection, all 16 catalog levels, asset-pack readiness, and workload coverage reconciled; source-only. |
| Source importers | Detailed | [Source Importers Capability Inventory](Tools/SourceImporters/README.md) | Formats, geometry, transforms, materials, textures, cameras, lights, deformation, animation, validation, dependencies, and known losses reconciled; source-only. |
| Tasks | Detailed | [Tasks Capability Inventory](Engine/Tasks/README.md) | Task graphs, lanes, parallel ranges, cancellation, handles, events, shutdown, failure propagation, and tracing reconciled; source-only. |
| Shared tool support | Detailed | [Shared Tool Support Capability Inventory](Tools/ToolSupport/README.md) | Common cooker/compiler console messages, fields, progress, summaries, consumers, and game-profile isolation reconciled; source-only. |
| Persona/roadmap/gap coverage | Strategy crosswalk | [Capability Coverage Against Persona, Roadmap, And Gap Assessment](../CrossModule/StrategyCoverage.md) | All current module inventories mapped to `NS-*`, `PGE-*`, release subsystems, and refreshed volatile gap observations; source-only. |
| Performance diagnostics | Focused dated snapshot | [Performance Diagnostics Capability Inventory](../CrossModule/PerformanceDiagnosticsCapability.md) | Existing timing, marker, memory, editor, and external-capture surfaces reconciled at the document's stated snapshot; source-only and due for refresh before implementation. |
| Geometry-cache animation | Focused dated snapshot | [Geometry Cache Animation Capability Snapshot](../CrossModule/GeometryCacheAnimationCapability.md) | Existing import, cook, animation, scene-publication, deformation, residency, and workload-source seams reconciled; source-only. |
| Deferred GBuffer decals | Focused dated snapshot | [Deferred GBuffer Decals Capability Snapshot](Engine/Renderer/DeferredGBufferDecalsCapability.md) | Existing GBuffer, material, ray-hit, frame-graph, blend, and absent decal surfaces reconciled; source-only. |
| Debug-view presentation | Focused dated snapshot | [Debug View Presentation Capability Snapshot](Engine/Renderer/DebugViewPresentationCapability.md) | Existing visualization, exposure, tone-mapping, encoding, viewport, and view-mode seams reconciled; source-only. |
| Cross-system graphics coverage | Deep horizontal | [Graphics Feature Coverage Matrix](../CrossModule/GraphicsCoverageMatrix.md) | Feature-by-feature selectors, passes, shaders, RHI gates, backend asymmetries, fallbacks, resources, and negative coverage reconciled; source-only. |
| Feature execution | Deep vertical | [Graphics Feature Execution Traces](../CrossModule/FeatureExecutionTraces.md) | Frame, raster, ray GBuffer, ReSTIR, reference, provider, shader-delivery, and capture paths traced producer-to-consumer; source-only. |
| Registered Renderer programs | Exact catalog | [Renderer Shader Program Catalog](Engine/Renderer/ShaderProgramCatalog.md) | All 35 registrations mapped to source, entry, stage, consumer, and binding/traversal boundary; source-only. |
| Evidence closure | Plan | [Capability Evidence Plan](../../Plans/CapabilityEvidence.md) | Missing source audits and the smallest proof needed to promote individual claims. It does not replace release gates or roadmap order. |

## Repository-Wide Module Ledger

Detailed inventories are linked above. Every roadmap product area now has a source-depth owner, horizontal coverage statement, vertical trace, negative boundary, and evidence backlog route. This is documentation completeness, not release completeness.

| Area | Current owner/build surface | Inventory state | Next evidence or release action |
| --- | --- | --- | --- |
| Core | `Engine/Core`; common containers, files, logging, math, module/config facilities | Detailed (`S`) | See [Core](Engine/Core/README.md); build representative consumers and execute failure, path, process, configuration, and Shipping checks. |
| Tasks | `Engine/Tasks`; task graph and parallel work used by Renderer and tools | Detailed (`S`) | See [Tasks](Engine/Tasks/README.md); execute serial/parallel, cancellation, wait, failure, shutdown, tracing, and stress evidence. |
| Platform | `Engine/Platform`; Windows application/window/filesystem/process integration | Detailed (`S`) | See [Platform](Engine/Platform/README.md); execute window/input/DPI/capture behavior and package portability checks. |
| RHI | `Engine/RHI`; common contracts, diagnostics, D3D12, Vulkan, facade | Detailed (`S`) | See [RHI](Engine/RHI/README.md); execute the backend/device matrix. |
| Renderer | `Engine/Renderer`; renderer facade, persistent scene, prepared views, frame execution and passes | Detailed (`S`) | See [Renderer](Engine/Renderer/README.md); freeze selectable surface and execute feature/backend evidence. |
| Game framework and world | `Engine/GameFramework`; scenes, levels, components, systems, render submission | Detailed (`S`) | See [GameFramework](Engine/GameFramework/README.md); execute activation/reload, edit, animation, extraction, and failure evidence. |
| Application runtime | `Engine/Application`; runtime/editor host and content startup | Detailed (`S`) | See [Application](Engine/Application/README.md); execute startup/shutdown, configuration, console, threaded-render, recook, capture, and Shipping checks. |
| Editor | `Engine/Editor`; editor UI, viewport sessions, settings and asset/tool entry points | Detailed (`S`) | See [Editor](Engine/Editor/README.md); execute user workflows, persistence, undo/error paths, capture, restart, and runtime-separation checks. |
| Engine asset corpus | `Engine/Assets`; shader source, default textures, sky environments, and fixtures | Detailed (`S`) | See [Engine Assets](Engine/Assets/README.md); prove cook, package, integrity, license, and runtime consumption for included assets. |
| Importers | `Tools/Import/SourceImporters`; source-format ingestion | Detailed (`S`) | See [Source Importers](Tools/SourceImporters/README.md); execute format/fidelity fixtures, malformed-input failures, provenance, and end-to-end cook evidence. |
| Cooking | `Tools/Cooking`; project, scene, mesh, material, texture, and animation products | Detailed (`S`) | See [Asset Cooking](Tools/Cooking/README.md); execute deterministic, partial/failure, concurrency, publication, and runtime-handoff evidence. |
| Shader compiler | `Tools/Shaders` plus Renderer registrations | Detailed (`S`) | See [Shader Compilation](Tools/ShaderCompiler/README.md); run deterministic cook and runtime-consumer proof. |
| Shared tool support | `Tools/Support/ToolConsoleSupport`; common CLI presentation for cookers and ShaderCompiler | Detailed (`S`) | See [Shared Tool Support](Tools/ToolSupport/README.md); execute stream/layout/escaping/Unicode and game-product isolation evidence. |
| Launcher | `Tools/Launcher`; configure/build/cook/run front end | Detailed (`S`) | See [Launcher](Tools/Launcher/README.md); execute readiness, configure/build/cook/acquire/run/clean, cancellation, handoff, history, and packaged-workflow checks. |
| Showcase products | `Projects/Showcase`; editor/game executables and level catalog | Detailed (`S`) | See [Showcase](Projects/Showcase/README.md); execute both products, every included level, fallback honesty, required content, repeat load/switch/exit, and workload gates. |
| Build and packaging | root/module CMake, presets, dependency/staging logic | Detailed (`S`) | See [Build And Packaging](BuildAndPackaging.md); execute supported profiles/toolchains, reproducibility, install/stage/package, clean-source, and CI/test evidence. |

## Cross-Cutting Findings Already Visible

- “D3D12 and Vulkan” currently means two backend implementations behind one RHI contract. It does not yet mean paired behavioral parity, native validation, or equal device coverage.
- “Ray tracing” is several distinct paths: acceleration-structure construction, inline ray queries, native ray-tracing pipelines, shader tables, classic TLAS, and vendor/API-gated partitioned TLAS. Renderer coverage differs per effect.
- “Bindless” is not engine-wide. The detailed inventory found a fixed-capacity material texture descriptor array used by specific ray/path lighting paths, while raster GBuffer material sampling remains an eight-slot bindful layout.
- “PBR” currently names a concrete deferred material and lighting model, not every modern material lobe. The implemented components and missing lobes are listed in the Renderer inventory.
- “Shader support” separates authored language, compiler backend, target binary, registered stage, reflection/contract validation, cooked publication, runtime loading, and editor recook. Having a stage enum or compiler target does not prove a registered runtime program.
- Selectability is a release obligation. Any backend, render mode, provider, debug view, map, command, or setting reachable by a release user must receive a disposition and evidence or be made unreachable in that product.
- The [strategy crosswalk](../CrossModule/StrategyCoverage.md) is the completeness check: it maps every roadmap subsystem and all `PGE-*` requirements to current capability evidence or an explicit gap without counting scaffolding as support.

## Maintenance Contract

When a capability changes:

1. update its implementation and all producers/consumers first;
2. update the corresponding detailed row and its limitations;
3. add or retire the matching capability-evidence plan item;
4. update release disposition only through the acceptance contract and recorded evidence;
5. update this module ledger when a new area receives a detailed audit.

Do not mark a row `B`, `R`, `N`, `P`, or `A` from documentation review, a responsive process, an uninspected screenshot, or an unrecorded local result.

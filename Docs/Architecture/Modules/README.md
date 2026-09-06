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

## Use The Inventory By Question

| Reader question | Shortest route |
| --- | --- |
| What does one module support and explicitly not support? | Open its detailed inventory below; capability rows and non-capabilities are the source snapshot. |
| Where is the implementation owner or build boundary? | Start with [Engine](Engine/README.md), [Tools](Tools/README.md), [Projects](Projects/README.md), or [Build And Packaging](BuildAndPackaging.md), then verify source/CMake. |
| How does a graphics feature vary across backend or execution mode? | Use the [Graphics Feature Coverage Matrix](../CrossModule/GraphicsCoverageMatrix.md). |
| How does a graphics feature execute end to end? | Use the [Graphics Feature Execution Traces](../CrossModule/FeatureExecutionTraces.md). |
| Can a developer or user complete a build/content/editor/runtime/delivery journey? | Use [Product Workflow Coverage](../CrossModule/ProductWorkflowCoverage.md). |
| Where does that product journey cross owners and fail or settle? | Use [Product Execution Traces](../CrossModule/ProductExecutionTraces.md). |
| What question is still unanswered and what is the smallest next check? | Use the [Capability Evidence Plan](../../Plans/CapabilityEvidence.md). |
| What must this capability prove, and can it ship? | Use its Architecture feature dossier for the local proof contract, then its `FCR-*` report and the [First Release Acceptance Contract](../../Acceptance/FirstRelease.md) for actual approval. |
| How do I harden or review this dossier? | Follow [Capability Documentation Review](../../Engineering/Workflow/CapabilityReview.md). |

## How To Read A Row

### Capability identity

Every independently referenceable inventory row has a durable capability ID. Module inventories use `<module>-<family>-NN` so the identifier states both the owner and the local subject, for example `RHI-BIND-05`, `REN-LGT-03`, or `SHD-PUB-05`. Evidence work uses the separate `<module>-ENN` namespace, such as `RHI-E06`; an evidence item may cover several capability rows and a capability may require several evidence items.

Keep an ID with its semantic capability when rows move or tables are reordered. Append new IDs within the owning family. Do not reuse an identifier after a capability is removed or split; record its replacement or terminal disposition in the applicable candidate report before removing the current-snapshot row. Cross-module workflows (`WF-*`) and candidate reports (`FCR-*`) remain separate identities and must map back to capability IDs rather than replace them.

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
| Renderer | Detailed | [Renderer Capability Inventory](Engine/Renderer/CapabilityInventory.md), reached through the [Renderer module route](Engine/Renderer/README.md) | Whole-frame ownership plus feature families for surface production, Direct/Indirect/Volumetric Lighting, Ray Tracing, and Post Processing with distinct exposure, reconstruction/upscaling, tone mapping, color grading, chromatic aberration, frame generation, and output boundaries; source-only. |
| Shader compilation and delivery | Detailed | [Shader Compilation Capability Inventory](Tools/ShaderCompiler/README.md) | Tool build, CLI, typed registrations, compiler backends, validation, publication, editor recook, and runtime loading reconciled; source-only. |
| Showcase products | Detailed | [Showcase Product Capability Inventory](Projects/Showcase/README.md) | Editor/runtime targets, startup/selection, all 16 catalog levels, asset-pack readiness, and workload coverage reconciled; source-only. |
| Source importers | Detailed | [Source Importers Capability Inventory](Tools/SourceImporters/README.md) | Formats, geometry, transforms, materials, textures, cameras, lights, deformation, animation, validation, dependencies, and known losses reconciled; source-only. |
| Tasks | Detailed | [Tasks Capability Inventory](Engine/Tasks/README.md) | Task graphs, lanes, parallel ranges, cancellation, handles, events, shutdown, failure propagation, and tracing reconciled; source-only. |
| Shared tool support | Detailed | [Shared Tool Support Capability Inventory](Tools/ToolSupport/README.md) | Common cooker/compiler console messages, fields, progress, summaries, consumers, and game-profile isolation reconciled; source-only. |
| Persona/roadmap/gap coverage | Strategy crosswalk | [Capability Coverage Against Persona, Roadmap, And Gap Assessment](../CrossModule/StrategyCoverage.md) | All current module inventories mapped to `NS-*`, `PGE-*`, release subsystems, and refreshed volatile gap observations; source-only. |
| Performance diagnostics | Focused dated snapshot | [Performance Diagnostics Capability Inventory](../CrossModule/PerformanceDiagnostics/Capability.md) | Existing timing, marker, memory, editor, and external-capture surfaces reconciled at the document's stated snapshot; source-only and due for refresh before implementation. |
| Geometry-cache animation | Focused dated snapshot | [Geometry Cache Animation Capability Snapshot](../CrossModule/GeometryCacheAnimation/Capability.md) | Existing import, cook, animation, scene-publication, deformation, residency, and workload-source seams reconciled; source-only. |
| Deferred GBuffer decals | Current feature-gap dossier | [Deferred Decals](Engine/Renderer/Features/DeferredDecals/README.md) | No current decal feature; existing extension seams and separately labeled target architecture are reconciled without implying implementation. |
| Debug-view presentation | Current feature dossier | [Debug Views](Engine/Renderer/Features/DebugViews/README.md) | Existing modes, visualization products, exposure/tone/encoding limitation, viewport ownership, and target-presentation boundary reconciled; source-only. |
| Cross-system graphics coverage | Deep horizontal | [Graphics Feature Coverage Matrix](../CrossModule/GraphicsCoverageMatrix.md) | Feature-by-feature selectors, passes, shaders, RHI gates, backend asymmetries, fallbacks, resources, and negative coverage reconciled; source-only. |
| Feature execution | Deep vertical | [Graphics Feature Execution Traces](../CrossModule/FeatureExecutionTraces.md) | Frame, raster, ray GBuffer, ReSTIR, reference, provider, shader-delivery, and capture paths traced producer-to-consumer; source-only. |
| Product/developer workflow coverage | Deep horizontal | [Product And Developer Workflow Coverage](../CrossModule/ProductWorkflowCoverage.md) | Discovery, build, content, cook, launch, editor, runtime, diagnostics, cancellation, package, release, and support journeys compared actor-by-actor; source-only. |
| Product/developer execution | Deep vertical | [Product And Developer Execution Traces](../CrossModule/ProductExecutionTraces.md) | Quick Start, asset-to-frame, editor transaction, settings, shader reload, capture, and settlement paths traced across owners; source-only. |
| Registered Renderer programs | Exact catalog | [Renderer Shader Program Catalog](Engine/Renderer/Features/ShaderPrograms.md) | All 35 registrations mapped to source, entry, stage, consumer, and binding/traversal boundary; source-only. |
| Evidence closure | Plan | [Capability Evidence Plan](../../Plans/CapabilityEvidence.md) | Missing source audits and the smallest proof needed to promote individual claims. It does not replace release gates or roadmap order. |

## Current Coverage Boundary

Every top-level implementation owner is routed above and has a source-depth capability inventory. That closes the first navigation skeleton, not the release or the dossier audit. Horizontal graphics and product-workflow matrices, vertical execution traces, and the evidence plan own the remaining cross-module questions. [Inventory hardening items `INV-009` through `INV-012`](../../Plans/CapabilityEvidence.md#inventory-expansion) deliberately remain Open until every capability has all dossier dimensions, journeys, proof destinations, and reverse mappings reconciled. Any new public selector, executable, catalog entry, backend/mode, generated product, or module must enter those routes before it can be treated as inventoried.

## Claims That Require The Deeper Route

| Claim or reviewer concern | Owning detail |
| --- | --- |
| D3D12/Vulkan coverage or parity | [RHI](Engine/RHI/README.md), [graphics matrix](../CrossModule/GraphicsCoverageMatrix.md), and candidate evidence |
| Ray tracing, inline/native pipelines, shader tables, classic/PTLAS | [Renderer ray-tracing dossier](Engine/Renderer/Features/RayTracing/README.md), [RHI](Engine/RHI/README.md), and [graphics traces](../CrossModule/FeatureExecutionTraces.md) |
| Bindless/material binding coverage | [graphics matrix](../CrossModule/GraphicsCoverageMatrix.md#binding-and-material-coverage) |
| PBR/material/direct-light coverage | [Geometry, Materials, and GBuffer](Engine/Renderer/Features/GeometryMaterialsAndGBuffer.md), [Direct Lighting](Engine/Renderer/Features/Lighting/DirectLighting.md), and the [exact inventory](Engine/Renderer/CapabilityInventory.md) |
| Indirect-light/history/environment coverage | [Indirect Lighting](Engine/Renderer/Features/Lighting/IndirectLighting.md) and the [whole-frame route](Engine/Renderer/RenderingASparkleFrame.md) |
| Volumetric/fog/atmosphere coverage | [Volumetric Lighting](Engine/Renderer/Features/Lighting/VolumetricLighting.md), which currently records an explicit absence |
| Exposure, tone mapping, color grading, chromatic aberration, reconstruction/upscaling, frame generation, and output | [Post Processing](Engine/Renderer/Features/PostProcessing/README.md), including explicit negative dossiers for unsupported stages |
| Shader language/stage/compiler/target/runtime coverage | [ShaderCompiler](Tools/ShaderCompiler/README.md) and [program catalog](Engine/Renderer/Features/ShaderPrograms.md) |
| Build/cook/editor/runtime/package user journey | [product workflow matrix](../CrossModule/ProductWorkflowCoverage.md) and [product traces](../CrossModule/ProductExecutionTraces.md) |
| Persona, roadmap, and gap completeness | [strategy coverage](../CrossModule/StrategyCoverage.md) |
| Release inclusion or exclusion | [feature reports](../../Acceptance/FeatureCompletionReports.md) and [release acceptance](../../Acceptance/FirstRelease.md) |

## Maintenance Route

Use [Capability Documentation Review](../../Engineering/Workflow/CapabilityReview.md) to audit a changed dossier, [Documentation Organization](../../Engineering/Workflow/DocumentationOrganization.md) for placement and ownership, and the [Capability Evidence Plan](../../Plans/CapabilityEvidence.md) for unanswered checks. Do not mark a row `B`, `R`, `N`, `P`, or `A` from documentation review, a responsive process, an uninspected screenshot, or an unrecorded local result.

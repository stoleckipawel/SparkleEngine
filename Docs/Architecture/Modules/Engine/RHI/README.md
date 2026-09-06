# RHI

Status: module index and current-system reading route; not executable backend or release evidence

Scope: route backend-neutral GPU contracts, D3D12/Vulkan lowering, and each independently reviewable RHI capability to one documentation owner

Current-state basis: source and build configuration inspected 2026-09-06 at committed `master` revision `8414b5dc`; no build, device run, native validation, capture, performance, or package result is claimed

## Start Here

Use the [RHI Capability Inventory](CapabilityInventory.md) for exact `RHI-*` implementation-state, backend-coverage, limit, and evidence rows. Use the [RHI Feature Dossiers](Features/README.md) to understand one contract, its ownership and lifetime, or its feature-local completion requirements.

The inventory is an exact source ledger. The feature dossiers explain cohesive mechanisms and proof contracts without copying the ledger or claiming that its checks ran.

## RHI Feature Dossiers

| Contract family | What the RHI owns | Owning document |
| --- | --- | --- |
| Backend selection and device capabilities | compiled-backend availability, requested backend, adapter/device creation, queue topology, and neutral capability reporting | [Backend Selection and Device Capabilities](Features/BackendSelectionAndDeviceCapabilities.md) |
| Resource lifetime and memory | formats, textures, buffers, samplers, upload/readback, allocation, transient aliasing, residency, and completion-safe reclamation | [Resource Lifetime and Memory](Features/ResourceLifetimeAndMemory.md) |
| Descriptor binding | layouts, descriptor handles, writes, bindful arrays, bounded bindless capability, and recording lifetime | [Descriptor Binding](Features/DescriptorBinding.md) |
| Pipeline and shader contracts | graphics/compute descriptors, shader bytecode/reflection, parameter layout, validation, materialization, and cache identity | [Pipeline and Shader Contracts](Features/PipelineAndShaderContracts.md) |
| Command submission and synchronization | recording leases, command operations, queue batches, waits, completion tokens, barriers, and retirement authority | [Command Submission and Synchronization](Features/CommandSubmissionAndSynchronization.md) |
| Ray tracing | acceleration structures, inline queries, native pipelines, shader tables, classic TLAS, and partitioned TLAS | [Ray Tracing](Features/RayTracing.md) |
| Presentation | swapchain ownership, back-buffer acquisition/state, resize, pacing, VSync, present, and the explicit HDR gap | [Presentation](Features/Presentation.md) |
| Diagnostics | object names, GPU events/timestamps, native validation/crash data, live objects, and bounded diagnostic delivery | [Diagnostics](Features/Diagnostics.md) |
| Texture capture | asynchronous texture readback, format/layout conversion, polling, result delivery, and cleanup | [Texture Capture](Features/TextureCapture.md) |
| External interop | narrow native handles, resource states, capability reports, and hooks for optional providers | [External Interop](Features/ExternalInterop.md) |
| ImGui rendering | backend device objects, texture descriptors, draw-data lowering, clipping, resize, and retirement | [ImGui Rendering](Features/ImGuiRendering.md) |

## Choose By Question

| Question | Read |
| --- | --- |
| What is implemented, gated, partial, vocabulary-only, or absent? | [Capability Inventory](CapabilityInventory.md) |
| Which contract owns a public RHI directory or backend implementation seam? | [Feature dossier source-owner audit](Features/README.md#source-owner-coverage-audit) |
| How does a Renderer feature differ across D3D12, Vulkan, raster, inline, or native ray traversal? | [Graphics Feature Coverage Matrix](../../../CrossModule/GraphicsCoverageMatrix.md) |
| How does a request travel from Renderer policy into native work and retirement? | [Graphics Feature Execution Traces](../../../CrossModule/FeatureExecutionTraces.md) |
| What proof is still missing? | [RHI capability-to-evidence map](../../../../Plans/CapabilityEvidence.md#rhi-capability-to-evidence-map) |
| Can the backend or capability ship? | Its candidate record in [Feature Completion Reports](../../../../Acceptance/FeatureCompletionReports.md) and the [First Release Acceptance Contract](../../../../Acceptance/FirstRelease.md) |

## Ownership Boundary

RHI owns backend-neutral GPU resources, formats, descriptors, pipelines, commands, queues, synchronization, presentation, native lowering, device capabilities, GPU diagnostics, capture mechanics, and narrow external interop. Renderer owns scene/view/frame policy, graph scheduling intent, feature selection, render products, shader-program use, and semantic fallbacks. The binding decision is [Renderer and RHI](../../../Decisions/RendererRhiBoundary.md).

Public contracts remain backend-neutral under `Engine/RHI/Public`. Common policy and adapters live in non-backend `Engine/RHI/Private` directories. `Private/D3D12` and `Private/Vulkan` lower the same neutral contracts and own only real API/capability differences. `Engine/RHI/CMakeLists.txt` is executable authority for target membership and backend gates.

## Placement Model

The RHI root contains this module route and the exact [Capability Inventory](CapabilityInventory.md). Cohesive contract explanations and feature-local acceptance live under [Features](Features/README.md). Renderer-specific effect policy stays under [Renderer](../Renderer/README.md); cross-backend comparisons and cross-module traces stay under [CrossModule](../../../CrossModule/README.md); implementation rules stay in [RHI Engineering](../../../../Engineering/Modules/RHI.md).

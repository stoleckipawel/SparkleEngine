# Rendering Glossary

Status: Stage 2 reviewer vocabulary
Date: 2026-06-12
Last synchronized: 2026-06-13

## Purpose

This glossary freezes the vocabulary used by the Renderer/RHI review-ready refactor. It exists so future renames, file moves, and contract changes use one shared language.

Source basis:

- arc42 glossary and crosscutting concept guidance: https://arc42.org/overview
- NVIDIA Donut repository structure and rendering module descriptions: https://github.com/NVIDIA-RTX/Donut
- NVIDIA Falcor getting-started docs and render-pass/render-graph workflow: https://github.com/NVIDIAGameWorks/Falcor/blob/master/docs/getting-started.md
- Sparkle execution plan: [rhi-renderer-review-ready-implementation-plan.md](../plans/rhi-renderer-review-ready-implementation-plan.md)
- Sparkle coverage status: [rendering-coverage-status.md](rendering-coverage-status.md)

Use this document as the naming source for Stage 2 and later stages. If code uses a different term today, the glossary marks whether the term is current, target, or debt.

## Layer Terms

| Term | Meaning | Current Sparkle references | Ownership rule |
| --- | --- | --- | --- |
| Renderer | The high-level render system that consumes game/editor scene state, builds frame data, declares frame graph passes, owns renderer features, and asks RHI for GPU work. | [Renderer.h](../../Engine/Renderer/Public/Renderer.h), [Renderer.cpp](../../Engine/Renderer/Private/Renderer.cpp) | Renderer owns render intent, pass composition, scene snapshots, frame products, debug views, ray tracing scene data, and upscaler providers. |
| RHI | Render Hardware Interface. The API-neutral graphics contract used by Renderer and tools. | [RenderHardwareInterface.h](../../Engine/RHI/Public/Device/RenderHardwareInterface.h), [RenderCommandList.h](../../Engine/RHI/Public/Commands/RenderCommandList.h) | RHI owns GPU/API concepts: devices, resources, descriptors, command lists, pipeline descriptors, resource states, memory, diagnostics, and native interop contracts. |
| Backend | A concrete implementation of the RHI for one graphics API. | [D3D12](../../Engine/RHI/Private/D3D12), [Vulkan](../../Engine/RHI/Private/Vulkan) | Backend owns API objects, API-specific type conversion, command encoding, descriptors, memory, swap chain, diagnostics, and optional API extensions. |
| D3D12 backend | The Direct3D 12 implementation of the RHI. | [D3D12RenderHardwareInterface.h](../../Engine/RHI/Private/D3D12/D3D12RenderHardwareInterface.h) | Must remain backend-private and must not leak D3D12 types into Renderer except through explicit native interop handles. |
| Vulkan backend | The Vulkan implementation of the RHI. | [VulkanRenderHardwareInterface.h](../../Engine/RHI/Private/Vulkan/VulkanRenderHardwareInterface.h), [VulkanRhi.cpp](../../Engine/RHI/Private/Vulkan/Device/VulkanRhi.cpp) | Must remain backend-private and must not leak Vulkan types into Renderer except through explicit native interop handles. |
| Application | The host/editor/game layer that owns windows, project flow, launch flow, and smoke orchestration. | [RhiSmokeValidation.cpp](../../Engine/Application/Private/Validation/RhiSmokeValidation.cpp) | Application can orchestrate renderer/RHI use, but should not implement backend-native capture or rendering behavior. |
| GameFramework | Runtime scene, level, component, and cooked asset layer. | [Engine/GameFramework](../../Engine/GameFramework), [game-framework-contract.md](game-framework-contract.md) | GameFramework owns runtime scenes and cooked-data loading. It must not own renderer passes, backend-native RHI details, or source import/cooking algorithms. |
| SparkleLauncher | Developer workflow application for build, cook, launch, maintenance, and smoke workflows. | [Tools/Launcher/SparkleLauncher](../../Tools/Launcher/SparkleLauncher), [tooling-pipeline-contract.md](tooling-pipeline-contract.md) | Launcher owns workflow/process orchestration and UI presentation. It must not duplicate focused cook/import/render algorithms. |
| ShaderCompiler | The tool that compiles/cooks shader packages and reflection consumed at runtime. | [ShaderCompiler](../../Tools/Shaders/ShaderCompiler) | Tools own source compilation, reflection extraction, package writing, and inspection commands through `ShaderContracts`, not full renderer runtime. |
| AssetCooker | Project-level cook orchestrator. | [Tools/Cooking/AssetCooker](../../Tools/Cooking/AssetCooker) | AssetCooker owns discovery, planning, dispatch, process isolation, and diagnostics. Focused cookers own actual transformations. |
| TextureCooker | Focused texture cooking tool. | [Tools/Cooking/TextureCooker](../../Tools/Cooking/TextureCooker) | TextureCooker owns source image loading, texture pipeline stages, compression policy, and cooked texture asset emission. |
| SourceImporters | Source asset importers for formats such as glTF/FBX. Current path: `Tools/Import/SourceImportAdapters`. | [Tools/Import/SourceImportAdapters](../../Tools/Import/SourceImportAdapters) | Source importers produce imported DTOs and diagnostics. Runtime modules must not read source formats directly. |
| Vendor provider | Renderer feature adapter for an external SDK such as NVIDIA DLSS. | [Upscaling](../../Engine/Renderer/Private/Upscaling) | Provider owns SDK-level policy; RHI/backend owns native handles and API metadata. |

## Runtime Terms

| Term | Meaning | Current Sparkle references | Ownership rule |
| --- | --- | --- | --- |
| Frame | One renderer update/record/submit cycle. | [Frame](../../Engine/Renderer/Private/Frame), [Renderer.h](../../Engine/Renderer/Public/Renderer.h) | Renderer owns frame orchestration. RHI owns command recording/submission details behind its interfaces. |
| FrameContext | Per-frame render data built from the scene snapshot, camera, lighting, temporal state, and GPU addresses. | [FrameContext.cpp](../../Engine/Renderer/Private/Frame/FrameContext.cpp), [FrameContext.h](../../Engine/Renderer/Private/Frame/FrameContext.h) | Renderer frame builders own it. Passes consume it through `PassExecutionContext`. |
| RenderSceneSnapshot | Immutable scene input captured before render-domain data is built. | [RenderSceneSnapshot.h](../../Engine/Renderer/Private/SceneData/Lifecycle/RenderSceneSnapshot.h) | Scene bridge owns capture/lifecycle. Renderer features should consume render-domain DTOs rather than gameplay internals. |
| RenderSceneData | Render-domain mesh, material, light, and instance data used by passes and ray tracing. | [RenderSceneData.h](../../Engine/Renderer/Private/SceneData/RenderSceneData.h) | Renderer scene data owns it. RHI must not know about it. |
| Render product | A texture/resource product exposed from renderer to editor/application, such as scene color or depth. | [ViewportContracts.h](../../Engine/Renderer/Public/Viewport/ViewportContracts.h) | Renderer presentation bridge owns handles and state transitions. Application should not infer frame graph internals. |
| Command context | Renderer-level command wrapper that expresses render intent while carrying an RHI command list. | [RenderCommandContext.h](../../Engine/Renderer/Private/Commands/RenderCommandContext.h) | Renderer owns pass-friendly helpers and diagnostic scopes. It must not become a backend API facade. |
| Command list | RHI-level GPU command recording interface. | [RenderCommandList.h](../../Engine/RHI/Public/Commands/RenderCommandList.h) | RHI owns GPU/API operations: bind pipeline, bind descriptors, draw, dispatch, copy, transition, alias, AS build, and markers. |
| Resource state | Backend-neutral state/layout intent for resources. | [ResourceState.h](../../Engine/RHI/Public/Interop/ResourceState.h) | RHI defines the vocabulary; backends map to D3D12 states or Vulkan access/layout/stage masks. |
| Native interop | Typed escape hatch for external SDKs that need API-native handles or view metadata. | [RhiNativeHandles.h](../../Engine/RHI/Public/Interop/RhiNativeHandles.h) | RHI defines consumer-scoped metadata; backends fill it. Renderer/provider code must not guess API state. |

## Frame Graph Terms

| Term | Meaning | Current Sparkle references | Ownership rule |
| --- | --- | --- | --- |
| Frame graph | Renderer system that records passes and resource usage, compiles dependencies/barriers/transients, and executes passes. | [FrameGraph.h](../../Engine/Renderer/Private/FrameGraph/FrameGraph.h) | Renderer owns the graph. RHI only executes explicit commands emitted by the graph and passes. |
| Frame graph pass | A named unit of render work with setup and execute callbacks. | [FrameGraph.h](../../Engine/Renderer/Private/FrameGraph/FrameGraph.h) | Pass setup declares resources; pass execute records commands. |
| Setup callback | The pass-time declaration step that registers reads/writes/uses. | [PassResourceBuilder.h](../../Engine/Renderer/Private/FrameGraph/Builder/PassResourceBuilder.h) | Must be side-effect-light and describe resource intent. |
| Execute callback | The command recording step for a compiled pass. | [PassExecutionContext.h](../../Engine/Renderer/Private/FrameGraph/Execution/PassExecutionContext.h) | May bind runtime state, resolve resources, and record RHI commands through `RenderCommandContext`. |
| PassResourceBuilder | Helper used by setup callbacks to declare resource access. | [PassResourceBuilder.h](../../Engine/Renderer/Private/FrameGraph/Builder/PassResourceBuilder.h) | Owns pass declarations, not resource allocation. |
| Resource usage | Renderer vocabulary for how a pass uses a graph resource. | [ResourceUsage.h](../../Engine/Renderer/Private/FrameGraph/ResourceUsage.h) | Frame graph compiler maps usage to RHI `ResourceState`. |
| Imported resource | Resource owned outside the graph but visible to graph scheduling. | [FrameGraphTextureRegistration.cpp](../../Engine/Renderer/Private/FrameGraph/Resources/FrameGraphTextureRegistration.cpp) | Owner keeps lifetime; graph tracks states/views during the frame. |
| Persistent resource | Resource intended to survive across graph rebuilds or frames. | [FrameGraphResourceTypes.h](../../Engine/Renderer/Private/FrameGraph/FrameGraphResourceTypes.h) | Renderer feature owns lifetime; graph tracks access. |
| Transient resource | Resource created by the graph for temporary frame use. | [FrameGraphTransientAllocator.h](../../Engine/Renderer/Private/FrameGraph/Resources/FrameGraphTransientAllocator.h) | Frame graph owns planning/materialization; RHI owns actual resource/memory creation. |
| FrameGraphPlan | Compiled execution plan containing pass nodes, resource nodes, dependencies, barriers, and transient aliasing plans. | [FrameGraphPlan.h](../../Engine/Renderer/Private/FrameGraph/Compiler/FrameGraphPlan.h) | Compiler owns the plan. Execution consumes it. |

## Shader And Pipeline Terms

| Term | Meaning | Current Sparkle references | Ownership rule |
| --- | --- | --- | --- |
| Shader package | Named set of shader stages and reflection used to build one runtime pipeline path. | [CookedShaderPackage.h](../../Engine/RHI/Public/Shaders/CookedShaderPackage.h) | Runtime package primitives are generic RHI/tool data. Renderer-specific package declarations should live above RHI after Stage 4. |
| Cooked shader package | Binary package emitted by ShaderCompiler and loaded by runtime. | [CookedShaderPackageCache.h](../../Engine/RHI/Public/Shaders/CookedShaderPackageCache.h) | Tools write it; runtime reads it. Backends consume bytecode format requested by RHI capabilities. |
| Cooked asset | Runtime-ready asset emitted by cook tools and loaded by GameFramework/Renderer/RHI-facing runtime paths. | [GameFramework cooked assets](../../Engine/GameFramework/Public/Assets/Cooked), [Tools/Cooking](../../Tools/Cooking) | Tools write cooked assets; runtime modules load them. Source import and cook algorithms stay out of runtime modules. |
| Shader reflection | Cooked metadata for bindings, constant buffers, inputs, push constants, and specialization constants. | [ShaderReflection.h](../../Engine/RHI/Public/Shaders/ShaderReflection.h) | ShaderCompiler emits it; RHI/Renderer validate against it. |
| Binding layout | Runtime layout compiled from pass parameter declarations and package reflection. | [PassParameterLayout.h](../../Engine/RHI/Public/ShaderParameters/PassParameterLayout.h), [RenderPassShaderRuntime.h](../../Engine/Renderer/Private/Pipeline/RenderPassShaderRuntime.h) | Renderer defines pass parameters; RHI/backends create API-native layout objects. |
| Binding set | RHI allocation for descriptor tables. | [RenderBindingSet.h](../../Engine/RHI/Public/Bindings/RenderBindingSet.h) | RHI owns allocation and backend descriptor handles. Renderer can own feature-level sets, such as material texture tables. |
| Pass runtime | Renderer-side runtime bundle needed to execute a pass, usually binding layout plus pipeline state. | [PassPipelineRuntime.h](../../Engine/Renderer/Private/Pipeline/PassPipelineRuntime.h) | Renderer pipeline system owns lookup and cache lifetime. |
| PSO | Pipeline State Object. Backend/API object created from shader stages, binding layout, formats, raster/depth state, and related fixed-function settings. | [RhiPipelineStateDesc.h](../../Engine/RHI/Public/Pipeline/RhiPipelineStateDesc.h) | RHI defines normalized descriptors; backends create API objects. |
| PSO key | Planned explicit identity for a pass runtime/pipeline state. | [Pipeline runtime contract](pipeline-runtime-contract.md) | Target owner is renderer pipeline runtime. Current code uses `std::type_index` in `PipelineStateManager`, which is debt for Stage 16. |
| RenderPassPipelineTraits | Current central traits table that maps pass C++ type to runtime creation logic. | [RenderPassPipelineTraits.h](../../Engine/Renderer/Private/Pipeline/RenderPassPipelineTraits.h) | Current mechanism. Target is to reduce central edits for ordinary pass additions. |
| PipelineRuntimeLibrary | Planned owner for explicit PSO keys, package/runtime lookup, reload invalidation, and diagnostics. | [pipeline-runtime-contract.md](pipeline-runtime-contract.md) | Planned Stage 16 concept, not implemented today. |

## Ray Tracing Terms

| Term | Meaning | Current Sparkle references | Ownership rule |
| --- | --- | --- | --- |
| Acceleration structure | GPU structure used by ray tracing to query geometry intersections. | [RhiRayTracingDesc.h](../../Engine/RHI/Public/RayTracing/RhiRayTracingDesc.h) | RHI owns generic descriptors and build commands; renderer owns scene membership and pass usage. |
| BLAS | Bottom-level acceleration structure for mesh geometry. | [RayTracingBlasCache.h](../../Engine/Renderer/Private/RayTracing/RayTracingBlasCache.h) | Renderer ray tracing scene owns cache policy; RHI owns buffers, prebuild info, and build command execution. |
| TLAS | Top-level acceleration structure containing instances of BLAS entries. | [RayTracingTlasBuilder.h](../../Engine/Renderer/Private/RayTracing/RayTracingTlasBuilder.h) | Renderer builds instance list from render scene data; RHI builds the API acceleration structure. |
| Inline ray query | Shader feature where compute/raster shaders issue ray queries without a full ray tracing pipeline. | [RayTracingCapabilityReport.h](../../Engine/Renderer/Private/RayTracing/RayTracingCapabilityReport.h) | Renderer decides feature use from RHI capabilities. Backends report support. |
| Ray traced shadow settings | Renderer feature settings for ray traced shadows. | [RayTracedShadowSettings.h](../../Engine/Renderer/Private/RayTracing/RayTracedShadowSettings.h) | Renderer feature owns it. RHI must not know shadow quality or denoiser modes. |
| Ray tracing pass services | Lightweight service bundle exposed to passes that need ray tracing scene/shadow settings. | [RenderRayTracingPassServices.h](../../Engine/Renderer/Private/RayTracing/RenderRayTracingPassServices.h) | Renderer owns it; passes consume it; RHI stays GPU/API-only. |

## Upscaling Terms

| Term | Meaning | Current Sparkle references | Ownership rule |
| --- | --- | --- | --- |
| Upscaler provider | Renderer interface for a frame upscaling implementation. | [UpscalerProvider.h](../../Engine/Renderer/Private/Upscaling/UpscalerProvider.h) | Renderer feature owns provider selection and input contract. |
| Passthrough upscaler | Fallback provider that returns native frame output without vendor upscaling. | [PassthroughUpscalerProvider.h](../../Engine/Renderer/Private/Upscaling/PassthroughUpscalerProvider.h) | Renderer owns fallback behavior and diagnostics. |
| DLSS provider | NVIDIA DLSS implementation through Streamline. | [NvidiaDlssUpscalerProvider.h](../../Engine/Renderer/Private/Upscaling/NvidiaDlss/NvidiaDlssUpscalerProvider.h) | Provider owns Streamline/DLSS policy; RHI/backend owns native interop. |
| Native AA | DLSS/DLAA-style use where input and output resolution are equal while DLSS acts as an anti-aliasing/reconstruction pass. | [UpscalerSettings.h](../../Engine/Renderer/Private/Upscaling/UpscalerSettings.h) | Renderer settings decide mode. Provider must report whether it is supported. |

## Review Status Terms

| Term | Meaning |
| --- | --- |
| Accepted | Current ownership appears aligned with the target architecture and must be preserved. |
| Needs refactor | Known implementation or ownership gap linked to a later implementation stage. |
| Needs design decision | Unresolved ownership or policy question that must be decided before code motion. |
| Acceptance evidence | Build, smoke log, capture, screenshot, diagnostic report, or doc update that proves a row is complete. |
| Boundary violation | A dependency edge that crosses the intended layer direction, such as RHI including Renderer-private files. |

## Naming Rules

- Use `Renderer` for render-domain orchestration and feature ownership.
- Use `RHI` for API-neutral graphics contracts and GPU concepts.
- Use `D3D12` and `Vulkan` only inside backend implementation names or explicit native interop/capability documentation.
- Use `FrameGraph` for pass/resource scheduling and graph execution.
- Use `Pass` for a renderer feature unit that declares graph resources and records commands.
- Use `Pipeline` or `PassRuntime` for runtime shader/package/binding/PSO state.
- Use `ShaderCompiler` for tool-side compilation/cooking and `CookedShaderPackage` for runtime package artifacts.
- Use `BLAS` and `TLAS` only for acceleration structure concepts; do not use them for renderer pass resources that are merely handles.

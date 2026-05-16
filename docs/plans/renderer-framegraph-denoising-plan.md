# Renderer FrameGraph Denoising Plan

## Intent

Plan a deep cleanup of Sparkle's Renderer framegraph systems and their dependencies. The goal is the same kind of denoising as the recent RHI, Renderer, and GameFramework work: public contracts should be narrow and intentional, private implementation should be free to move, names should say whether a type belongs to setup, compilation, execution, resource realization, or diagnostics, and backend details should stay behind the RHI or renderer-private seams.

This is a roadmap, not an implementation patch. Each phase should remove the replaced path in the same phase rather than adding compatibility headers or long-lived forwarding shims.

## Reference Compass

Use these industry patterns as a compass, not as a reason to copy another engine literally.

- Epic UE RDG is the closest architectural reference for Sparkle's current direction. Prefer its split between setup and execute timelines, builder-owned graph resources, pass parameter driven dependency declaration, transient resource planning, external resource registration/extraction, validation, event scopes, and graph/debug visualization concepts.
- NVIDIA Donut and NVRHI are the reference for device-facing vocabulary and boundaries. Donut uses clear `*Pass` classes and explicit `Render(...)` methods rather than a named graph layer; NVRHI uses device, command list, binding layout, binding set, pipeline state, resource state tracking, automatic barriers, explicit `commitBarriers`, and validation vocabulary. NVRHI is not a render graph model, but it is a useful check that Sparkle's framegraph does not become a hidden second RHI.
- Vulkan is the reference for synchronization honesty. Resource state, access, stage, barrier, aliasing, and queue ownership concepts should be named clearly enough that a future Vulkan backend would not have to reinterpret D3D12-shaped assumptions.
- AMD Cauldron and FidelityFX sample structure are useful as a simplicity check. Older Cauldron uses explicit `*RenderPass`, `ResourceViewHeaps`, `CommandListRing`, and manual barriers. Newer Cauldron2/FidelityFX uses `RenderModule`, `CommandList`, `GPUResource`, `ResourceState`, `Barrier`, and `ResourceBarrier`, with render modules returning resources to shader-readable states at module boundaries.
- Sparkle should keep its UE/RDG leaning when tradeoffs are ambiguous, but retain `FrameGraph` as the graph/system term because the user wants the terminology aligned with framegraph literature. Use NVIDIA/AMD/Vulkan terms for lower-level execution and synchronization names where they make the boundary clearer.

## NVIDIA Donut/NVRHI Shape To Imitate

Use Donut and NVRHI as the primary practical reference for layer shape:

- RHI shape: keep device-facing concepts explicit and small. Prefer `Device`, `CommandList`, `Texture`, `Buffer`, `Framebuffer`, `BindingLayout`, `BindingSet`, `GraphicsPipeline`, `ComputePipeline`, `ResourceState`, and `Barrier` style names over renderer-owned abstractions that hide backend work.
- State tracking shape: framegraph barrier planning may exist above the RHI, but the RHI should still have a visible command-list state/barrier boundary. Avoid making the framegraph the only place where resource state transitions make sense.
- Render shape: concrete renderer features should look like Donut's `*Pass` classes: own stable shaders/pipelines/binding caches, have a clear `Create`/`Init` lifetime, and expose an explicit `Render`/`Execute` method that consumes a command list and declared inputs.
- FrameGraph shape: Sparkle may keep a real `FrameGraph`, unlike Donut, but it should remain the scheduler/resource lifetime layer. It should declare resources and dependencies, compile pass order/lifetimes/barriers, and hand execution to pass code rather than becoming a second RHI or a kitchen-sink renderer facade.
- Boundary shape: `Private/Passes` owns pass implementation, `Private/Pipeline` owns pipeline creation/cache/binding, `Private/Frame` owns frame orchestration, `Private/FrameGraph` owns graph declaration/compile/execution glue, and RHI owns command/device/resource primitives.

## Sparkle Vs Donut/NVRHI: Biggest Differences

1. RHI device scope is broader in Sparkle. `RenderHardwareInterface` acts like a device, descriptor allocator, per-frame constant allocator, transient heap/resource factory, backbuffer accessor, present-pass helper, texture loader, ImGui bridge, and diagnostics provider. NVRHI keeps the device/command list abstraction central and lets higher renderer helpers sit above it.
2. Sparkle has a good command-list core, but no explicit NVRHI-style barrier accumulator contract. `RenderCommandList` exposes `TransitionResource`, `AliasResource`, and `UnorderedAccessBarrier`, while NVRHI also has visible state tracking functions such as begin tracking, set state, set permanent state, automatic barrier control, and `commitBarriers`.
3. Sparkle has `RenderBindingLayout`, but binding sets are not a first-class RHI concept. Binding work is spread across descriptor handles, descriptor tables, `PassBinder`, shader parameter metadata, and pass utilities. Donut/NVRHI makes `BindingLayout` and `BindingSet` a stable pair.
4. Sparkle does not have an obvious RHI `Framebuffer`/render-target attachment object. Passes and FrameGraph execution bind CPU descriptor handles directly. Donut/NVRHI commonly route graphics state through framebuffer/framebuffer-factory concepts.
5. Sparkle concrete passes are mostly static type bundles: metadata, declaration, parameter setting, and execution are static functions keyed by `PipelineStateManager`/traits. Donut passes are easier to read as objects that own or cache their shaders, pipelines, binding layouts, binding sets, framebuffers, and render entry points.
6. Sparkle pass execution still sees too much graph and service state. `RenderGraphPassContext` carries `Commands`, `Frame`, `Runtime`, `Diagnostics`, and the full `FrameGraph`; authored passes also use `RenderPassContext` as a service locator. Donut-style pass code usually consumes an explicit command list plus direct inputs/caches.
7. Sparkle's FrameGraph is more powerful than Donut's explicit render flow, which is intentional, but it currently absorbs too many roles: resource declaration, compile plan, execution helpers, resource resolver, copy/clear helpers, and transient allocation orchestration.

## Current Sparkle FrameGraph Shape

Public Renderer framegraph surface is small but generic:

- `Engine/Renderer/Public/FrameGraph/ResourceHandle.h`
- `Engine/Renderer/Public/FrameGraph/TextureHandle.h`
- `Engine/Renderer/Public/FrameGraph/BufferHandle.h`
- `Engine/Renderer/Public/FrameGraph/FrameGraphTextureDesc.h`
- `Engine/Renderer/Public/FrameGraph/FrameGraphBufferDesc.h`

Private implementation is broader:

- `Engine/Renderer/Private/FrameGraph/FrameGraph.h` is the central owner for pass registration, resource creation/import, parameter allocation, compile product types, execution helpers, resource resolution, copy/clear helpers, and transient materialization.
- `Engine/Renderer/Private/FrameGraph/Builder/PassBuilder.h` records resource usage by mutating `FrameGraph` through friendship.
- `Engine/Renderer/Private/FrameGraph/Compiler/FrameGraphCompiler.*` builds pass dependencies, resource versions, execution order, transient lifetimes, physical aliasing blocks, and barriers.
- `Engine/Renderer/Private/FrameGraph/ResourceRegistry.h` owns metadata, runtime resource states, resolved native handles, and descriptors in one registry.
- `Engine/Renderer/Private/FrameGraph/Execution/RenderGraphPassContext.h` is the per-pass execution context, while `Engine/Renderer/Private/FrameGraph/RenderPassContext.h` is a renderer service locator for pass execution.
- `Engine/Renderer/Private/FrameGraph/RenderPassRuntime.h` contains per-pass runtime structs and traits for pipeline/binding lookup.
- `Engine/Renderer/Private/FrameGraph/Features/FrameGraphProducts.h` stores target bundles such as scene, GBuffer, and lighting outputs.
- `Engine/Renderer/Private/Frame/*` builds the frame by calling graph resource creation and pass registration functions directly.
- `Engine/Renderer/Private/Passes/*` authored passes depend on `FrameGraph`, `PassBuilder`, `RenderGraphPassContext`, `RenderPassContext`, and shader parameter metadata.
- `Engine/Renderer/Private/Pipeline/*` resolves pass pipeline runtime and binds framegraph resources into RHI descriptor tables.

## Biggest Issues To Fix

1. Private framegraph internals are exported with `SPARKLE_RENDERER_API`. `FrameGraph`, `PassBuilder`, `ResourceRegistry`, `RenderPassContext`, `RenderGraphPassContext`, and pass runtime structs live under `Private` but still look like DLL surface.
2. `FrameGraph` is a monolith. It owns setup, graph resources, compile data structures, execution entry points, resource resolution, copy/clear helpers, and transient allocator orchestration.
3. Naming mixes timelines. `PassBuilder`, `FrameGraphBuilder`, `FrameGraph`, `FrameGraphCompiler`, `RenderGraphPassContext`, and `RenderPassContext` do not make it obvious which names belong to setup, compilation, execution, or runtime services.
4. `FrameGraph` and `RenderGraph` vocabulary is mixed. Public folders say `FrameGraph`, one execution context says `RenderGraphPassContext`, and the cleanup should converge on `FrameGraph` as Sparkle's chosen graph term.
5. Public handle names are too generic in a codebase without a strong namespace wall. `TextureHandle`, `BufferHandle`, and `ResourceHandle` are render-graph handles, not general renderer or RHI handles.
6. Resource registry responsibilities are fused. Metadata, runtime state, imported/native access, descriptor access, and resolved transient views all sit in one type.
7. Compiler plan types are nested in `FrameGraph`, which forces compiler, execution, diagnostics, and resource planning code to depend on the monolithic graph type.
8. Execution context names are ambiguous. `RenderPassContext` is not a pass context; it is shared pass runtime services. `RenderGraphPassContext` is the actual per-pass execution context.
9. Pipeline runtime is hardcoded by pass type through exported traits and per-pass runtime structs. This makes adding passes noisier than necessary and couples pass classes to runtime storage details.
10. `FrameGraphProducts` is vague. The types are render target bundles or frame targets, not products.
11. Diagnostics are present, but the graph lacks a first-class plan/lifetime/debug contract similar to RDG transition logs, resource filters, pass filters, transient allocator toggles, and graph inspection.
12. RHI device/service responsibilities are too broad compared with NVRHI's cleaner device plus command-list shape.
13. Binding-set and framebuffer concepts are implicit, which makes pass binding and render-target setup harder to compare with Donut/NVRHI.
14. Pass classes are static metadata/procedure bundles rather than Donut-style feature pass objects with clear owned caches and render entry points.
15. Command-list state tracking exists at the FrameGraph planning level, but the RHI seam does not yet expose an NVRHI-like state/barrier policy clearly enough.

## Target Vocabulary

Preferred long-term names:

- RHI-facing vocabulary should stay close to NVRHI: `CommandList`, `BindingLayout`, `BindingSet`, `PipelineState` or `GraphicsPipeline`/`ComputePipeline`, `Framebuffer`, `ResourceState`, `Barrier`, `Texture`, and `Buffer`.
- `FrameGraph` for the graph system, folder, and central concept. Keep `Engine/Renderer/Public/FrameGraph` and do not introduce new `RenderGraph` names for graph-owned types.
- `FrameGraphBuilder` for setup-time graph authoring.
- `PassResourceBuilder` or `PassResourceDeclarationBuilder` for the object that declares a pass's resource usage. Avoid plain `PassBuilder` unless it really constructs a pass.
- `FrameGraphPlan` for immutable compiler output.
- `FrameGraphCompiler` for dependency, lifetime, culling, and barrier planning.
- `FrameGraphExecutor` for replaying a compiled plan.
- `PassExecutionContext` for the context passed into pass execute lambdas.
- `PassRuntimeServices` for stable shared services such as RHI, diagnostics, pipeline runtime, and textures.
- `FrameGraphResourceRegistry` for resource metadata only.
- `FrameGraphResourceStateTracker` for current and boundary states.
- `FrameGraphResourceResolver` for native handles and descriptor views.
- `FrameGraphTransientAllocator` for physical transient allocation and aliasing.
- `FrameRenderTargets`, `SceneRenderTargets`, `GBufferRenderTargets`, and `LightingRenderTargets` for frame target bundles.
- `FrameGraphResourceHandle`, `FrameGraphTextureHandle`, and `FrameGraphBufferHandle` if graph handles stay in the global namespace.
- `*Pass` for concrete renderer features that own shader/pipeline/binding behavior, following Donut's pass shape.
- `Add*Passes` or `Schedule*Passes` for frame assembly helpers that register work into the FrameGraph.
- `Create*Targets` for frame assembly helpers that create graph textures/buffers.

Names to avoid for new code:

- `Product` for texture target bundles.
- `Context` for long-lived service containers.
- `Runtime` for both pipeline state storage and per-frame execution state without a qualifier.
- RHI-like names in `FrameGraph` types when the type is not actually device-facing.
- `RenderGraph` for Sparkle graph-owned types unless referring to external literature or UE RDG by name.
- `FrameGraph` and `RenderGraph` mixed in the same layer.

## Implementation Ground Rules

- Do not run CMake, MSBuild, CTest, or full project build commands between individual phases. Keep per-phase validation to source-level checks such as `git diff --check`, targeted stale-name greps, file existence checks, and focused include-boundary scans. Run the real build only after all selected phases are complete.
- Do not add compatibility headers for old paths. Each phase should remove the replaced path in the same phase.
- Prefer precise symbol renames and file moves over forwarding wrappers. When possible, use language-aware rename for C++ symbols and then patch includes/paths.
- Keep graph implementation private unless a public Renderer contract truly needs it. If a public header pulls a private framegraph type into view, fix the boundary instead of exporting the private type.
- Keep D3D12/DXGI/native resource details out of public graph contracts. Opaque RHI handles may remain in private renderer/RHI seams until a later backend-ownership refactor.
- Preserve current behavior while denoising names and ownership. Do not introduce async compute, bindless resources, plugin pass authoring, or new transient allocation behavior during this cleanup.
- After moving files, update CMake only if the existing glob/freshness workflow does not discover them naturally. The current Renderer CMake glob pattern should usually pick up source moves.
- For every phase, update includes to use the selected module-root style and run stale-name greps before moving to the next phase.

## Phase 0: Lock The Study And Boundary Inventory

Goal: make the planned work measurable before moving files.

Planned work:

- Add a focused inventory of framegraph private headers that still carry `SPARKLE_RENDERER_API`.
- Identify every non-Renderer module include of `Renderer/Public/FrameGraph/*`.
- Identify every include of `FrameGraph/FrameGraph.h` outside `Private/FrameGraph`, `Private/Frame`, `Private/Passes`, and `Private/Pipeline`.
- Record whether the public framegraph surface is needed by shader parameter code, editor diagnostics, or only private renderer code.
- Confirm that this sequence keeps `FrameGraph` as the graph/system term and only removes stray `RenderGraph` names from Sparkle-owned graph types.

Done criteria:

- The planned public surface is explicitly listed.
- All exported-private framegraph symbols are known.
- No implementation phase starts with unknown external consumers.

Implementation prompt:

```text
Implement Phase 0 of docs/plans/renderer-framegraph-denoising-plan.md. Do not build. Produce only source-level inventory changes and, if useful, update this plan with confirmed findings.

Start by scanning:
- SPARKLE_RENDERER_API in Engine/Renderer/Private/FrameGraph/**
- includes of Renderer/Public/FrameGraph/** outside Engine/Renderer
- includes of FrameGraph/FrameGraph.h outside Engine/Renderer/Private/FrameGraph, Private/Frame, Private/Passes, and Private/Pipeline
- public Renderer headers that include private FrameGraph headers

Record the planned public graph surface and all accidental exported-private symbols. Confirm that the implementation sequence keeps FrameGraph as the chosen graph term and removes stray RenderGraph names from Sparkle-owned types. Do not change runtime code unless a tiny include audit fix is clearly necessary. Validate with targeted greps and git diff --check only.
```

## Phase 0A: Map Sparkle To Donut/NVRHI Layers

Goal: make the Donut/NVRHI inspiration concrete before changing names or moving files.

Planned work:

- Create a short mapping table in this plan or a nearby inventory note that compares:
  - Sparkle RHI device/resource/command types against NVRHI device, command list, resource, state, barrier, binding layout, binding set, pipeline, and framebuffer concepts.
  - Sparkle renderer passes against Donut `*Pass` classes such as GBuffer, depth, bloom, tone mapping, SSAO/TAA-style passes.
  - Sparkle frame orchestration and FrameGraph concepts against Donut's simpler explicit render-call flow.
- Identify names where Sparkle is already close to NVRHI/Donut and should not be churned.
- Identify names where Sparkle currently hides RHI work behind a framegraph or renderer type that should become more explicit.
- Keep this phase read-only except for the inventory note or plan update.

Done criteria:

- The plan has a concrete Sparkle-to-Donut/NVRHI layer map.
- Future phases can cite exact files and symbols rather than a vague external reference.
- No runtime code has changed.

Implementation prompt:

```text
Implement Phase 0A of docs/plans/renderer-framegraph-denoising-plan.md. Do not build. This is an inventory and plan-update phase only.

Study Sparkle's RHI, Renderer pass, Pipeline, Frame, and FrameGraph layers, then map them to NVIDIA Donut/NVRHI concepts. Start from:
- Engine/RHI/Public/** and Engine/RHI/Private/**
- Engine/Renderer/Private/Passes/**
- Engine/Renderer/Private/Pipeline/**
- Engine/Renderer/Private/Frame/**
- Engine/Renderer/Private/FrameGraph/**

Use the Donut/NVRHI shape as the reference:
- RHI: Device, CommandList, Texture, Buffer, Framebuffer, BindingLayout, BindingSet, GraphicsPipeline, ComputePipeline, ResourceState, Barrier.
- Render: concrete *Pass classes that own pipeline/binding caches and execute with an explicit command list.
- FrameGraph: Sparkle-specific scheduling/lifetime layer that should not absorb RHI responsibilities.

Update this plan with a compact mapping table and a list of names to keep versus names to revisit. Do not rename symbols in this phase. Validate with targeted file searches and git diff --check only; skip compile/build.
```

Phase 0A inventory findings:

| Donut/NVRHI concept | Current Sparkle symbols and files | Phase 0A reading | Follow-up |
| --- | --- | --- | --- |
| Device | `RenderHardwareInterface`, `D3D12RenderHardwareInterface`, `RenderDeviceServices` in `Engine/RHI/Public/Device` and `Engine/RHI/Private/D3D12` | Sparkle has a clear device seam, but `RenderHardwareInterface` also owns renderer services: constants, transient resources, present helpers, ImGui, texture loading, descriptor allocation, and diagnostics. | Keep the device seam; split service breadth in Phase 0C. |
| Command list | `RenderCommandList`, `D3D12RenderCommandList`, renderer `RenderCommandContext` | This is close to NVRHI. `RenderCommandList` owns draw, dispatch, binding, copy, render-target, diagnostic, and barrier commands; `RenderCommandContext` is a renderer wrapper over it. | Keep `RenderCommandList`; make command-list state policy explicit in Phase 0E. |
| Texture | `Texture`, `D3D12Texture`, `RhiTextureResourceDesc`, `FrameGraphTextureDesc`, `TextureHandle` | RHI texture vocabulary is clear. FrameGraph texture descriptors are graph-owned virtual resources, not RHI textures. | Keep names; Phase 2 can clarify public graph handles if needed. |
| Buffer | `RhiBufferResourceDesc`, `RhiVertexBufferView`, `RhiIndexBufferView`, `NativeResourceHandle`, `RhiOwnedResourceHandle`, `FrameGraphBufferDesc`, `BufferHandle` | Sparkle has buffer descriptors and handles but not a first-class public RHI `Buffer` object analogous to NVRHI. Current handle shape is acceptable for this cleanup. | Defer first-class `Buffer` unless later RHI work needs it. |
| Native resources | `NativeResourceHandle`, `RhiOwnedResourceHandle`, `RhiOwnedHeapHandle` | Opaque native handles keep backend details out of Renderer public FrameGraph contracts. | Keep; do not replace with D3D12-specific types. |
| Resource state and barriers | `ResourceState`, `FrameGraph::CompiledBarrier`, `FrameGraph::CompiledAliasingBarrier`, `TransitionResource`, `AliasResource`, `UnorderedAccessBarrier` | State naming is already NVRHI/Vulkan-readable. The unclear part is policy: FrameGraph plans barriers, command lists emit them, but there is no visible NVRHI-style tracking/commit contract. | Keep `ResourceState`; clarify manual graph-planned versus RHI-owned barrier policy in Phase 0E. |
| Binding layout | `RenderBindingLayout`, `RenderBindingLayoutCompileDesc`, `D3D12BindingLayout`, `PassParameterLayout` | This maps well to NVRHI `BindingLayout`. Sparkle's compile path from shader parameter layout to RHI binding layout is coherent. | Keep; do not churn the layout name. |
| Binding set | `PassBinder`, `PassBindingOverrides`, `RhiDescriptorTableBinding`, `RhiDescriptorTableHandle`, shader parameter bindings | Binding-set behavior exists as descriptor tables plus pass binder logic, but it is not named as a first-class concept. | Revisit in Phase 0D with private `PassBindingSet` or `RenderBindingSet` vocabulary if the shape is clear. |
| Pipeline | `RenderPipelineState`, `GraphicsPipelineStateDesc`, `ComputePipelineStateDesc`, `D3D12PipelineState`, `PipelineStateManager`, `RenderPassPipelineTraits` | RHI pipeline naming is close enough. The bigger issue is ownership: pass pipeline runtime is trait/manager-owned instead of Donut-style pass-owned or pipeline-owned cache shape. | Keep RHI pipeline names; denoise runtime ownership in Phases 8 and 8A. |
| Framebuffer | `SetRenderTarget`, `SetRenderTargets`, `RhiCpuDescriptorHandle`, `ResolveRenderTargetView`, `ResolveDepthStencilView`, `GBufferTargets`, `LightingTargets` | Sparkle binds raw RTV/DSV handles and target bundles directly. There is no RHI `Framebuffer` or private renderer `FrameGraphFramebuffer` equivalent. | Revisit in Phase 0D; prefer a private proof-of-shape before public RHI exposure. |
| Descriptor heap/table | `RhiDescriptorAllocation`, `RhiDescriptorTableHandle`, `RhiDescriptorTableBinding`, `D3D12DescriptorHeapManager` | The explicit `Rhi` prefix is useful because these are device-facing descriptor primitives, not graph resources. | Keep names; avoid hiding them behind FrameGraph names. |
| Shader package and parameter layout | `LoadedShaderPackage`, `CookedShaderPackageCache`, `ShaderPackageLayoutBuilder`, `PassParameterLayout`, `ShaderParameterStructBuilder` | This is a Sparkle-specific cooked shader contract that feeds binding layout creation. It does not need Donut naming. | Keep; make pass ownership clearer around it in Phases 0F, 8, and 8A. |

Renderer pass layer map:

| Donut render shape | Current Sparkle symbols and files | Phase 0A reading | Follow-up |
| --- | --- | --- | --- |
| Concrete `*Pass` feature classes | `GBufferPass`, `DirectLightingPass`, `IndirectLightingPass`, `LightingCompositePass`, `SkyPass`, `VisualizeBuffersPass`, `ComputeClearPass` in `Engine/Renderer/Private/Passes` | Sparkle already uses Donut-like `*Pass` names. The classes are mostly static metadata/procedure bundles rather than objects with owned caches and a direct render method. | Keep class names; address shape in Phases 0F and 8A. |
| Pass-owned pipeline and binding caches | `PipelineStateManager`, `RenderPassPipelineTraits`, `RenderPassShaderRuntimeStorage`, `RenderPassRuntimeTraits`, `PassBinder` | Pipeline and binding caches are centralized by trait and type index. This works, but it is less local than Donut's pass-owned cache style. | Move runtime ownership toward `Private/Pipeline` in Phase 8 and pass-local ownership in Phase 8A. |
| Explicit `Render`/`Execute` entry point | Static `Execute(RenderGraphPassContext&, ParameterInstance&)` on each pass | Sparkle has an execute entry point, but it receives graph-heavy context instead of an explicit command list plus narrow resource access. | Rename/narrow execution context in Phase 7, then revisit pass object shape in Phase 8A. |
| Pass setup/init lifetime | Static `DeclareResources`, `SetParameters`, `Describe*ShaderPackage`, `GetParameterMetadata` | Setup is split across graph declaration, parameter setting, shader metadata, and pipeline traits. The behavior is coherent but not Donut-local. | Keep metadata static where useful; move lifecycle/cache behavior out of static bundles later. |
| Render module style | No broad `RenderModule` abstraction for individual passes | Good fit for Sparkle today. Passes are framegraph-scheduled features, not plugin-style render modules. | Do not introduce `RenderModule` unless a feature owns multi-pass lifecycle beyond one graph pass. |

Frame orchestration and FrameGraph map:

| Donut/explicit flow concept | Current Sparkle symbols and files | Phase 0A reading | Follow-up |
| --- | --- | --- | --- |
| Explicit frame render sequence | `BuildFrame`, `CreateGBufferTargets`, `AddGBufferPass`, `CreateLightingTargets`, `AddLightingPasses`, `AddDirectLightingPass`, `AddIndirectLightingPass`, `AddLightingCompositePass`, `AddSkyPass`, `AddPresentationPass`, `AddVisualizeBuffersPass` in `Engine/Renderer/Private/Frame` | Sparkle's frame helpers are the closest equivalent to Donut's explicit render-call sequence. Phase 0F split the most visible creation/scheduling blur. | Keep the Phase 0F helper shape; Phase 9 should focus on target bundle names and any remaining frame-level product wording. |
| Frame target bundles | `SceneTargets`, `GBufferTargets`, `LightingTargets` in `FrameGraphProducts.h` plus `LightingTargets.h` helpers | Target bundle names are mostly good; `FrameGraphProducts` is vague and makes frame targets look like generic graph products. | Keep target struct names; revisit file/folder ownership in Phase 9. |
| Graph scheduler/lifetime layer | `FrameGraph`, `FrameGraphBuilder`, `PassBuilder`, `PassResourceDeclaration`, `FrameGraphCompiler`, `FrameGraphTransientAllocator`, `ResourceRegistry` | This is Sparkle's intentional difference from Donut. The graph owns scheduling, virtual resources, compile order, transient lifetimes, aliasing, and barrier planning. | Keep `FrameGraph`; split monolithic ownership across Phases 3-6. |
| Compiled graph plan | Nested `FrameGraph::CompiledPlan`, `CompilePassRecord`, `CompileResourceEntry`, `CompiledTransientResourcePlan`, `CompiledPhysicalBlockPlan` | Sparkle already has a graph plan concept, but it is nested in the graph owner and leaks into compiler/allocator dependencies. | Extract to `FrameGraphPlan` in Phase 4. |
| Resource realization | `ResourceRegistry`, `FrameGraphResourceAccess`, `Resolve*View`, `ResolveResource`, `FrameGraphResourceResolution.cpp` | Metadata, state, and realized RHI handles are fused. This is where FrameGraph starts to feel like a second RHI. | Split registry, state tracker, and resolver in Phase 5. |

Names to keep for now:

- `FrameGraph` as the graph/system term and folder name.
- `RenderCommandList`, `ResourceState`, `TransitionResource`, `AliasResource`, and `UnorderedAccessBarrier`.
- `RenderBindingLayout`, `RenderPipelineState`, `GraphicsPipelineStateDesc`, and `ComputePipelineStateDesc`.
- `RhiDescriptor*`, `NativeResourceHandle`, `RhiOwnedResourceHandle`, and `RhiOwnedHeapHandle`.
- Concrete `*Pass` feature names such as `GBufferPass`, `DirectLightingPass`, `SkyPass`, and `VisualizeBuffersPass`.
- Frame target bundle names like `SceneTargets`, `GBufferTargets`, and `LightingTargets`.

Names and shapes to revisit in later phases:

- `RenderHardwareInterface`, because it mixes NVRHI-like device work with renderer services and present/UI helpers.
- `RenderGraphPassContext`, because Sparkle should keep `FrameGraph` terminology and pass execution should see a narrower context.
- `RenderPassContext`, because it is runtime services rather than a per-pass context.
- `PassBuilder`, because it declares resource usage rather than constructing a pass.
- `ResourceRegistry`, because it owns metadata, resolved RHI access, and runtime state together.
- `FrameGraph::Compiled*` nested plan records, because compiler output should be a first-class `FrameGraphPlan` contract.
- `FrameGraphProducts.h`, because it stores frame render target bundles, not generic products.
- Binding-set and framebuffer vocabulary, because current behavior is spread across descriptors, `PassBinder`, target bundles, and graph resolution helpers.
- Static pass procedure shape, because Donut-style pass ownership should be easier to read once execution and pipeline seams are narrower.

## Phase 0B: Align RHI Boundary With NVRHI Shape

Goal: make Sparkle's RHI boundary readable to someone familiar with NVRHI before the FrameGraph depends on it more cleanly.

Planned work:

- Audit RHI-facing names used by Renderer and FrameGraph for NVRHI-style clarity:
  - command list or command context ownership
  - resource state and barrier types
  - binding layout and binding set/descriptor concepts
  - graphics/compute pipeline objects
  - framebuffer or render-target attachment objects
- Prefer explicit RHI vocabulary at the RHI seam and avoid graph-specific names for device-facing work.
- Do not perform a broad RHI rewrite in this framegraph denoising sequence. Keep changes limited to names, include boundaries, and small ownership clarifications that unblock the FrameGraph cleanup.
- Record any larger RHI modernization as deferred work if it would exceed this plan's scope.

Done criteria:

- Renderer/FrameGraph code has a clear distinction between RHI command/resource concepts and graph resource concepts.
- Any RHI naming drift from NVRHI-style vocabulary is either fixed or explicitly deferred.
- Public FrameGraph headers still do not expose backend-native details.

Implementation prompt:

```text
Implement Phase 0B of docs/plans/renderer-framegraph-denoising-plan.md. Do not build between phases.

Audit and lightly align the RHI boundary with NVIDIA NVRHI-style vocabulary. Start from:
- Engine/RHI/Public/**
- Engine/RHI/Private/**
- Engine/Renderer/Private/FrameGraph/Execution/**
- Engine/Renderer/Private/FrameGraph/Resources/**
- Engine/Renderer/Private/Pipeline/**
- Engine/Renderer/Private/Passes/**

Look for command-list, resource-state, barrier, binding-layout, binding-set/descriptor, framebuffer, graphics-pipeline, and compute-pipeline names. Prefer explicit RHI names at the RHI seam and FrameGraph-prefixed names only for graph-owned handles/plans/resources.

Keep this phase narrow. Rename or move only if the existing name actively blurs RHI versus FrameGraph ownership. Do not introduce new backend-native public types, do not expose D3D12/DXGI from public FrameGraph contracts, and do not change behavior. If a larger RHI shape mismatch is found, record it as deferred work instead of fixing it here.

Validation greps:
- D3D12|DXGI|ID3D12|ComPtr in Engine/Renderer/Public/FrameGraph
- ResourceState|Barrier|CommandList usage across Renderer/FrameGraph/RHI
- graph-owned types with RHI-like names and RHI-owned types with graph-like names
- git diff --check

Skip compile/build.
```

Phase 0B audit findings:

Source checks performed:

- `Engine/Renderer/Public/FrameGraph/**/*.h` has no `D3D12`, `DXGI`, `ID3D12`, or `ComPtr` references.
- Public FrameGraph descriptors use graph vocabulary plus backend-neutral RHI-facing concepts such as `PixelFormat`, `RenderConfig`, and `DepthConvention`; they do not expose backend-native resource or descriptor types.
- RHI public command/pipeline names are already close to NVRHI: `RenderCommandList`, `ResourceState`, `RenderBindingLayout`, `RenderPipelineState`, `GraphicsPipelineStateDesc`, `ComputePipelineStateDesc`, `RhiDescriptor*`, and `Native*Handle`.
- Renderer private FrameGraph execution still resolves and binds RHI handles directly through `Resolve*View`, `ResolveResource`, `BindRenderTarget`, `ClearRenderTarget`, `ClearDepthStencil`, `CopyResource`, and barrier emission helpers.

Boundary decisions:

| Area | Current Sparkle shape | Phase 0B decision |
| --- | --- | --- |
| Command list ownership | RHI owns `RenderCommandList`; Renderer owns private `RenderCommandContext` as a diagnostics/wrapper layer over a command list. | Keep both names. `RenderCommandContext` is renderer-private and does not blur public RHI ownership enough to rename in this phase. Phase 7 should narrow what pass execution receives. |
| Resource states | RHI owns `ResourceState`; FrameGraph records initial/final/current state and compiler barrier state. | Keep `ResourceState` unchanged. This is useful NVRHI/Vulkan-style vocabulary at the RHI seam. |
| Barrier records | `FrameGraph::CompiledBarrier` and `FrameGraph::CompiledAliasingBarrier` are graph compiler output, while `RenderCommandList` emits `TransitionResource`, `AliasResource`, and `UnorderedAccessBarrier`. | Do not rename now. Phase 4 should move graph plan records to `FrameGraphBarrier`/`FrameGraphAliasingBarrier`; Phase 0E should document command-list barrier policy. |
| Binding layout | RHI owns `RenderBindingLayout` and `RenderBindingLayoutCompileDesc`; Renderer pipeline code builds layouts from `PassParameterLayout`. | Keep. This maps clearly to NVRHI `BindingLayout`, and the `Render` prefix is consistent with existing RHI object names. |
| Binding set | No first-class binding-set type. `PassBinder`, `PassBindingOverrides`, `RhiDescriptorTableBinding`, and shader parameter bindings collectively act like one. | Defer to Phase 0D. The missing concept is real, but introducing it belongs with the binding/framebuffer vocabulary phase. |
| Graphics/compute pipelines | RHI owns `RenderPipelineState` with separate `GraphicsPipelineStateDesc` and `ComputePipelineStateDesc`; Renderer owns `PipelineStateManager` and `RenderPassPipelineTraits`. | Keep RHI names. A split into `GraphicsPipeline`/`ComputePipeline` is larger than Phase 0B and not required for FrameGraph denoising. Phase 8 should address Renderer pipeline runtime ownership. |
| Framebuffer/render targets | RHI command lists bind raw `RhiCpuDescriptorHandle` RTV/DSV values; FrameGraph resolves views and target bundles such as `GBufferTargets`/`LightingTargets`. | Defer to Phase 0D. A named private `RenderFramebuffer` or `FrameGraphFramebuffer` may help, but changing it here would exceed the light boundary alignment scope. |
| Descriptor handles/tables | `RhiCpuDescriptorHandle`, `RhiGpuDescriptorHandle`, `RhiDescriptorAllocation`, `RhiDescriptorTableHandle`, and `RhiDescriptorTableBinding` are RHI-facing. | Keep. The `Rhi` prefix makes device ownership explicit and prevents confusion with graph resources. |
| Graph resource realization | `FrameGraph` private methods expose `Rhi*DescriptorHandle` and `NativeResourceHandle` during execution. | Accept as a private transitional seam. Phase 5 should split resource metadata/state/resolution, and Phase 7 should hide execution-safe operations behind a narrower resource access facade. |
| Backend-native implementation | D3D12 names and COM types remain under `Engine/RHI/Private/D3D12` and do not leak into public FrameGraph headers. | Keep as-is. No backend-native public contract was found. |

Names to keep unchanged after Phase 0B:

- `RenderCommandList` and renderer-private `RenderCommandContext`.
- `ResourceState`, `TransitionResource`, `AliasResource`, and `UnorderedAccessBarrier`.
- `RenderBindingLayout` and `RenderBindingLayoutCompileDesc`.
- `RenderPipelineState`, `GraphicsPipelineStateDesc`, and `ComputePipelineStateDesc`.
- `RhiDescriptor*`, `RhiCpuDescriptorHandle`, `RhiGpuDescriptorHandle`, `RhiDescriptorTableHandle`, and `RhiDescriptorTableBinding`.
- `NativeGraphics*Handle`, `NativeDescriptorHeapHandle`, and `NativeResourceHandle`.

Deferred RHI shape work recorded by this phase:

- Add explicit binding-set vocabulary only when Phase 0D can map it to concrete `PassBinder` and descriptor-table behavior.
- Add explicit framebuffer or attachment-group vocabulary only when Phase 0D can keep it private and behavior-preserving.
- Decide command-list state tracking and barrier policy in Phase 0E rather than mixing it into the naming audit.
- Keep RHI public pipeline objects unified for now; revisit separate `GraphicsPipeline` and `ComputePipeline` object names only in a broader RHI modernization.
- Move FrameGraph RHI-handle resolution out of the monolithic graph owner in Phases 5 and 7 rather than renaming private methods piecemeal here.

Phase 0B result: no runtime code changes and no file moves are needed. The RHI boundary is readable enough to proceed; the active alignment work belongs in the planned binding/framebuffer, command-list policy, resource resolver, and execution-context phases.

## Phase 0C: Split RHI Device Shape From Renderer Services

Goal: make Sparkle's RHI feel closer to NVRHI's device/command-list model by separating true device work from renderer convenience services.

Planned work:

- Audit `RenderHardwareInterface` and group methods into categories:
  - core device and queue access
  - command-list access and submission-facing operations
  - resource and descriptor creation
  - shader/pipeline creation
  - per-frame constant/upload services
  - transient allocation services
  - present/backbuffer helpers
  - ImGui/backend UI helpers
  - diagnostics
- Keep core RHI vocabulary explicit: device, command list, resource, descriptor, pipeline, binding layout, state, barrier.
- Move or plan movement for renderer convenience services into `RenderDeviceServices`, renderer private services, or narrower helper interfaces.
- Do not break existing backbuffer/present behavior; this phase should carve interfaces and ownership seams, not rewrite the backend.

Done criteria:

- `RenderHardwareInterface` responsibilities are documented and any immediate low-risk split is implemented.
- Renderer-only services are not mistaken for NVRHI-style device responsibilities.
- Deferred work is listed for any service split that is too large for this sequence.

Implementation prompt:

```text
Implement Phase 0C of docs/plans/renderer-framegraph-denoising-plan.md. Do not build between phases.

Compare Sparkle's RenderHardwareInterface with NVIDIA NVRHI's device/command-list split and reduce obvious responsibility blur. Start from:
- Engine/RHI/Public/Device/RenderHardwareInterface.h
- Engine/RHI/Public/Device/RenderDeviceServices.h
- Engine/RHI/Private/Device/RenderDeviceServices.cpp
- Engine/RHI/Private/D3D12/**
- Engine/Renderer/Public/Renderer.h
- Engine/Renderer/Private/** call sites of RenderHardwareInterface

Classify RenderHardwareInterface methods into device, command list, resource/descriptor creation, shader/pipeline creation, uploads/constants, transient allocation, present/backbuffer, ImGui/UI, and diagnostics. Prefer moving only low-risk renderer convenience access behind RenderDeviceServices or renderer-private helpers. Do not perform a broad backend rewrite and do not change behavior.

If a method cannot move safely in this phase, document it as deferred RHI shape work in this plan. Keep NVRHI-like core names visible: device, command list, resource, descriptor, pipeline, binding layout, resource state, barrier.

Validation greps:
- RenderHardwareInterface call sites by category
- BeginPresentRenderPass|BeginPresentOverlayPass|RenderImGuiDrawData|BeginImGuiFrame|CreateTextureFromPath
- RenderDeviceServices usage and new helper boundaries
- git diff --check

Skip compile/build.
```

Phase 0C audit findings:

Source checks performed:

- `RenderHardwareInterface` is the only public RHI device facade used by Renderer systems and by Application/Editor host presentation code through `Renderer::GetRenderHardwareInterface`.
- `RenderDeviceServices` already owns backend lifecycle, swapchain resize, begin/submit frame sequencing, frame-in-flight advancement, per-frame constant updates, and flush behavior. It is a useful lifecycle facade, but it still exposes the full `RenderHardwareInterface` for downstream convenience.
- `Renderer` public API currently forwards the full RHI through `GetRenderHardwareInterface`, which is why Application, Editor, runtime console, validation, shader recook, Renderer internals, FrameGraph, Pipeline, texture, material, and mesh systems can all depend on one broad interface.
- No low-risk implementation split was made in this phase. Moving methods now would require coordinated public Renderer API changes plus D3D12 backend, Application, Editor, Renderer, FrameGraph, Pipeline, and asset-cache call-site edits.

`RenderHardwareInterface` responsibility map:

| Category | Current methods | Current consumers | Phase 0C decision |
| --- | --- | --- | --- |
| Core device and backend identity | `GetBackendApi`, `GetRequiredShaderBinaryFormat`, `WaitForIdle`, `GetDeviceHandle`, `GetGraphicsQueueHandle`, `GetRayTracingCapabilities` | Pipeline shader package loading uses shader binary format; validation/shader recook use idle waits; native handles are backend-facing escape hatches. | Keep on the RHI device facade. These are closest to NVRHI `Device` responsibilities. |
| Frame and command-list access | `GetCurrentFrameIndex`, `GetGraphicsCommandList` | Renderer host frame setup, timing diagnostics, frame diagnostics indexing, and current command recording. | Keep for now, but prefer future host-frame access through `RenderDeviceServices` or a narrow frame context so Renderer code does not query the full device for frame bookkeeping. |
| Diagnostics | `GetDiagnostics` | Renderer initialization and per-frame execution context diagnostics. `RenderDeviceServices` already forwards this. | Keep reachable, but the preferred owner is a narrow diagnostics facade exposed by `RenderDeviceServices`, not arbitrary pass/system access to the full device. |
| ImGui/UI backend bridge | `InitializeImGuiBackend`, `BeginImGuiFrame`, `RenderImGuiDrawData`, `ShutdownImGuiBackend` | Editor UI and runtime console overlay. | Defer split. This should become a host UI/backend service or presentation overlay service, not a core NVRHI-like device method. |
| Shader and pipeline creation | `CreateBindingLayout`, `CreateGraphicsPipelineState`, `CreateComputePipelineState` | `RenderPassShaderRuntime` and `PipelineStateManager`. | Keep on RHI. This maps cleanly to NVRHI device creation of binding layouts and pipeline states. |
| Descriptor heaps, descriptors, and sampler tables | `SetShaderVisibleDescriptorHeaps`, `GetShaderResourceHeapHandle`, `AllocateDescriptor`, `ReleaseDescriptor`, `AllocateDescriptorTable`, `GetDescriptorTableCpuHandle`, `ReleaseDescriptorTable`, `AllocateShaderResourceDescriptor`, `ReleaseShaderResourceDescriptor`, `GetSharedSamplerBinding` | `PassBinder`, `MaterialCacheManager`, `SkyPass`, shader resource view writing, pass binding setup. | Keep as an RHI descriptor service for now. Phase 0D should decide whether binding-set vocabulary wraps part of this for passes. |
| Per-frame constants and upload allocation | `GetPerFrameConstantData`, `GetPerFrameConstantGpuAddress`, `AllocateUniformConstantBuffer`, `AllocatePerViewConstantBuffer`, `AllocatePerObjectVertexConstants`, `AllocatePerObjectPixelConstants` | Frame context, passes, pass binder, and per-frame data builders. | Defer split. These are renderer frame services layered on backend allocation; a future `RenderFrameConstants` or upload allocator facade should be introduced only after pass execution context is narrower. |
| Backbuffer and present helpers | `GetBackBufferViewport`, `GetBackBufferScissorRect`, `GetBackBufferRenderTargetView`, `GetBackBufferResource`, `BeginPresentRenderPass`, `BeginPresentOverlayPass`, `EndPresentRenderPass`, `GetPresentColorFormat` | Per-view data builder, Editor app, runtime console, smoke validation, host presentation. | Defer split. These belong behind a presentation/backbuffer service owned by `RenderDeviceServices` or Renderer host integration. They are not core device vocabulary. |
| Texture and mesh convenience creation | `CreateTextureFromPath`, `CreateVertexBuffer`, `CreateIndexBuffer`, `ReleaseOwnedResource`, `GetNativeResource`, `GetResourceGpuVirtualAddress` | `TextureManager`, `GPUMesh`, FrameGraph transient allocator native-handle resolution/release. | Partially keep. Buffer/resource creation is RHI-facing; `CreateTextureFromPath` is a renderer asset-loading convenience and should move toward `TextureManager` plus a lower-level RHI texture factory in a later texture-resource cleanup. |
| Ray tracing resources | BLAS/TLAS prebuild info, scratch/acceleration/instance buffers, acceleration-structure SRV creation | No Renderer private call sites found in this audit outside the public RHI surface. | Keep as RHI device functionality. It is backend/device-specific and not a Renderer service. |
| Transient allocation and placed resources | `GetTextureAllocationInfo`, `GetBufferAllocationInfo`, `CreateOwnedHeap`, `ReleaseOwnedHeap`, `CreatePlacedTextureResource`, `CreatePlacedBufferResource` | FrameGraph transient planning and transient allocator. | Keep as RHI resource allocation primitives until Phase 5/6 splits graph resource resolving and transient allocation. Do not move independently. |
| View creation and resource capability queries | RTV/DSV/SRV/UAV creation methods and `SupportsUnorderedAccess` | FrameGraph transient allocator and external resource realization. | Keep as RHI device/resource-view creation. Phase 5 should hide these behind a FrameGraph resource resolver so authored passes do not see them. |

Call-site classification:

| Area | Uses | Reading |
| --- | --- | --- |
| `Renderer` public API | `GetRenderHardwareInterface` exposes full RHI to Application and Editor. | This is the widest seam. A true split starts by replacing broad public access with narrower host presentation, diagnostics, idle/flush, and UI backend accessors. |
| Renderer initialization | Constructs `PipelineStateManager`, `GPUMeshCache`, `TextureManager`, `MaterialCacheManager`, and `FrameGraph` with the full RHI. | These systems each need different slices. Future constructors should accept narrower services only after those slices exist. |
| Pipeline | `RenderPassShaderRuntime` creates binding layouts and graphics/compute pipeline states and queries required shader binary format. | This is legitimate RHI device use and should remain device-facing. |
| Pass binding | `PassBinder` sets shader-visible heaps, allocates uniform constants, and resolves shared sampler bindings. | Binding-set work is implicit here. Phase 0D should decide whether a `PassBindingSet`/`RenderBindingSet` name reduces this dependency. |
| Passes | Passes read per-frame constants; `SkyPass` allocates a descriptor table for the sky texture. | Pass execution sees too much RHI today. Phase 7 should narrow execution context and resource/binding access. |
| Texture and material caches | `TextureManager` calls `CreateTextureFromPath`; `MaterialCacheManager` allocates/releases descriptor tables and writes texture SRVs. | Texture file loading is the clearest non-device method on RHI. Descriptor-table ownership is RHI-facing but should be hidden behind material/binding services where possible. |
| Mesh cache | `GPUMesh` creates and releases vertex/index resources. | This is RHI resource creation and can stay until a lower-level `Buffer` object/factory exists. |
| FrameGraph resources | Transient planning asks allocation info; transient allocator creates heaps/placed resources and views; external resources create views and query UAV support. | This should remain private, then move behind `FrameGraphResourceResolver` and `FrameGraphTransientAllocator` splits in Phases 5 and 6. |
| Application/Editor host code | Editor app and validation call present helpers; runtime console and editor UI call ImGui backend methods; validation/shader recook call `WaitForIdle`. | These are the strongest evidence that present/UI/idle host services need a public Renderer or `RenderDeviceServices` facade before RHI can shrink. |

Names and methods to keep on the RHI facade for now:

- Device/backend identity, native device/queue handles, shader binary format, ray tracing capabilities, and idle wait.
- `RenderCommandList` access until Renderer host-frame code has a narrower frame context.
- Binding layout and graphics/compute pipeline creation.
- RHI descriptor handles, descriptor tables, shader-visible heap setup, and shared sampler binding until Phase 0D introduces explicit binding-set vocabulary.
- Resource allocation, placed-resource creation, view creation, and capability queries used by FrameGraph resource realization.

Deferred service splits recorded by this phase:

- Move ImGui/backend UI methods behind a host UI or overlay backend service consumed by Editor and runtime console.
- Move present/backbuffer render-pass helpers behind a presentation/backbuffer service owned by `RenderDeviceServices` or a public Renderer host-presentation facade.
- Replace public `Renderer::GetRenderHardwareInterface` consumers with narrower accessors before shrinking `RenderHardwareInterface` itself.
- Move `CreateTextureFromPath` out of core RHI toward `TextureManager` plus a lower-level RHI texture resource factory when texture loading is revisited.
- Introduce a constants/upload allocation facade only after pass execution no longer receives broad RHI access.
- Keep transient allocation and view creation in RHI until the FrameGraph resource resolver/transient allocator phases create the right private seam.

Phase 0C result: documentation-only. The correct split direction is clear, but the immediate implementation would be a broad public API/backend refactor. The plan now treats `RenderHardwareInterface` as a transitional device-plus-service facade and directs future phases to introduce narrower services before removing methods.

## Phase 0D: Make Binding Set And Framebuffer Vocabulary Explicit

Goal: address the biggest Donut/NVRHI readability gap in Sparkle's render path: binding sets and framebuffers are implicit today.

Planned work:

- Audit how Sparkle currently represents NVRHI/Donut concepts:
  - `RenderBindingLayout` maps well to NVRHI `BindingLayout`.
  - descriptor handles/tables plus `PassBinder` currently act like partial binding sets.
  - render target/depth descriptor handles currently act like ad hoc framebuffer attachments.
- Decide whether Sparkle should introduce private renderer/RHI vocabulary such as:
  - `RenderBindingSet` or `PassBindingSet` for compiled per-pass bindings.
  - `RenderFramebuffer` or `FrameGraphFramebuffer` for grouped render target/depth attachments.
- Keep public FrameGraph descriptors backend-neutral; do not expose D3D12 descriptors or NVRHI-style objects publicly unless the RHI owns them.
- Prefer private proof-of-shape first if the abstraction is not yet stable.

Done criteria:

- Binding layout, binding set, descriptor table, and pass binder responsibilities are distinct in the plan and code.
- Render target attachment grouping has a named owner or a documented reason to defer it.
- Pass code becomes easier to compare with Donut pass setup.

Implementation prompt:

```text
Implement Phase 0D of docs/plans/renderer-framegraph-denoising-plan.md. Do not build between phases.

Audit and, where low-risk, introduce explicit binding-set and framebuffer vocabulary inspired by Donut/NVRHI. Start from:
- Engine/RHI/Public/Pipeline/RhiPipelineStateDesc.h
- Engine/RHI/Public/Descriptors/RhiDescriptorHandles.h
- Engine/RHI/Public/Commands/RenderCommandList.h
- Engine/Renderer/Private/Pipeline/PassBinder.h/.cpp
- Engine/Renderer/Private/Passes/PassUtilities.h
- Engine/Renderer/Private/FrameGraph/Execution/**
- Engine/Renderer/Private/FrameGraph/Resources/**

Map current behavior first: RenderBindingLayout, descriptor tables, PassBinder binding calls, render target/depth descriptor handles, and FrameGraph resource resolution. If an abstraction is clearly missing and can be added privately without changing behavior, introduce a private name such as PassBindingSet, RenderBindingSet, RenderFramebuffer, or FrameGraphFramebuffer. If not, update this plan with the deferred design and leave runtime code unchanged.

Do not expose backend-native descriptor types in public FrameGraph headers. Do not add compatibility wrappers. Keep the phase focused on naming/ownership clarity rather than a full binding rewrite.

Validation greps:
- RenderBindingLayout|PassBinder|DescriptorTable|RhiDescriptor
- SetRenderTarget|SetRenderTargets|ClearRenderTarget|ClearDepthStencil call sites
- D3D12|DXGI|ID3D12|ComPtr in Engine/Renderer/Public/FrameGraph
- git diff --check

Skip compile/build.
```

Phase 0D audit findings:

Source checks performed:

- `RenderBindingLayout` already maps cleanly to NVRHI `BindingLayout`. It owns compiled root binding metadata through `CompiledBinding` and stays tied to `PassParameterLayout` and shader package compilation.
- No existing `BindingSet`, `RenderBindingSet`, `PassBindingSet`, `Framebuffer`, `RenderFramebuffer`, or `FrameGraphFramebuffer` type existed before this phase.
- Public FrameGraph headers still have no `D3D12`, `DXGI`, `ID3D12`, or `ComPtr` references.
- Binding calls are concentrated in `PassBinder`, `ShaderPass`, `PassUtilities`, typed pass parameters, material texture tables, and pass-specific overrides.
- Render-target attachment binding is concentrated in `FrameGraph::BindRenderTarget`/`BindRenderTargets`; only `GBufferPass` currently binds a multi-RTV graph framebuffer-like set directly.

Binding concept map:

| Donut/NVRHI concept | Current Sparkle owner | Reading | Phase 0D decision |
| --- | --- | --- | --- |
| Binding layout | `RenderBindingLayout`, `RenderBindingLayoutCompileDesc`, `CompiledBinding` | Good match. The layout records root parameter indices, binding kinds, shader registers, descriptor counts, and the source parameter layout. | Keep unchanged. This is the stable RHI-facing layout contract. |
| Pass parameter values | `PassParameterSet`, typed shader parameter fields, `PassParameterBinding` variants | Parameters hold graph texture/buffer handles, direct RHI descriptor table bindings, acceleration structure GPU addresses, uniform data references, and sampler descriptions. | Keep as the authored pass data shape. Do not rename to binding set; it is layout-indexed parameter data, not a compiled device binding object. |
| Descriptor tables | `RhiDescriptorTableHandle`, `RhiDescriptorTableBinding`, `RhiGpuDescriptorHandle` | Device-facing descriptor-table primitives. Material textures and sky texture use logical descriptor table bindings; graph resources resolve to GPU descriptor handles. | Keep explicit `Rhi` names. These remain device primitives rather than pass-owned binding-set objects. |
| Pass binding operation | `PassBinder`, `BindRasterShaderPass`, `BindComputeShaderPass`, `PassUtilities` | This is the closest current equivalent to NVRHI binding-set application. It sets binding layout, resolves graph SRV/UAV handles, uploads uniform data, resolves shared samplers, applies overrides, and emits root/descriptor table bindings. | Do not introduce `PassBindingSet` yet. The binder performs dynamic per-draw/per-dispatch work, so a stored binding-set object would be misleading until resource access and pass runtime services are narrower. |
| Binding overrides | `PassBindingOverrides` | Used for per-pass direct descriptor/root overrides such as sky texture, root SRV/UAV, and root constants. | Keep name. It is an override list, not a binding set. |
| Material texture tables | `MaterialCacheManager`, `MaterialData::textureTableHandle`, `MaterialTextureSlots` | Material cache owns persistent descriptor tables; GBuffer creates per-draw table bindings from slot offsets. | Keep RHI descriptor table naming. A later material binding wrapper may help, but it is material-cache work, not FrameGraph vocabulary. |
| Graph resource views | `FrameGraph::ResolveShaderResourceView`, `ResolveUnorderedAccessView`, transient/external access records | Graph handles become GPU descriptor handles at execution time. | Defer to Phase 5/7. These should move behind `FrameGraphResourceResolver`/execution resource access before any binding-set object is introduced. |

Framebuffer concept map:

| Donut/NVRHI concept | Current Sparkle owner | Reading | Phase 0D decision |
| --- | --- | --- | --- |
| Render-target attachment group | `FrameGraph::BindRenderTarget` and `FrameGraph::BindRenderTargets` | The graph receives graph texture handles, resolves RTV/DSV descriptor handles, and passes raw handles to `RenderCommandContext`. | Introduced private `FrameGraphFramebuffer` in execution code as a proof-of-shape value for grouped RTV/DSV handles. |
| Command-list framebuffer bind | `RenderCommandContext::SetRenderTarget(s)` and `RenderCommandList::SetRenderTarget(s)` | RHI command list still binds descriptor handles directly. | Keep unchanged. Do not add a public RHI framebuffer object in this phase. |
| Clear operations | `FrameGraph::ClearRenderTarget`, `ClearDepthStencil` | Clears still resolve one descriptor handle at a time from graph handles. | Keep unchanged. A future framebuffer/resource-access facade can group clears if pass execution needs it. |
| Transient/external view storage | `FrameGraphResourceAccess`, `FrameGraphTransientAllocator::AllocationRecord` | View descriptors are stored per resource, not per framebuffer. | Keep unchanged until Phase 5 splits resolver/access ownership. |

Code change made in Phase 0D:

- Added a private `FrameGraphFramebuffer` value in `Engine/Renderer/Private/FrameGraph/Execution/FrameGraphExecution.cpp`.
- `FrameGraph::BindRenderTarget` and `FrameGraph::BindRenderTargets` now resolve graph texture handles into that grouped attachment value before calling `RenderCommandContext`.
- Behavior is unchanged: the command context still receives the same RTV/DSV descriptor handles and count; the new type only names the grouped framebuffer concept privately.

Deferred binding-set design:

- Do not add `RenderBindingSet` or `PassBindingSet` until Phase 7 narrows pass execution resource access and Phase 8 moves pipeline runtime ownership out of FrameGraph.
- The future binding-set shape should likely be private to `Pipeline` or pass runtime ownership, and it should consume a `RenderBindingLayout`, a resolved graph resource access facade, uniform upload allocation, sampler bindings, and explicit overrides.
- Avoid making `PassParameterSet` itself the binding set. It stores authored parameter values before graph handles are resolved and before per-frame uniform upload happens.
- Avoid hiding `RhiDescriptorTableHandle`/`RhiDescriptorTableBinding` behind graph names. Descriptor tables are device primitives and should remain RHI vocabulary.

Phase 0D result: Sparkle now has a private framebuffer proof-of-shape where it was lowest risk, while binding-set vocabulary is intentionally deferred until the execution/resource-access and pipeline-runtime phases can introduce it without lying about the current dynamic binding behavior.

## Phase 0E: Define Command-List State And Barrier Policy

Goal: make Sparkle's command-list state policy as readable as NVRHI's automatic/manual barrier model without changing default behavior.

Planned work:

- Compare Sparkle's current policy to NVRHI:
  - Sparkle FrameGraph plans transitions and emits `TransitionResource`, `AliasResource`, and `UnorderedAccessBarrier` on `RenderCommandList`.
  - NVRHI command lists expose automatic barriers, state tracking, explicit state setting, permanent states, and `commitBarriers`.
- Decide which responsibilities belong to FrameGraph planning and which belong to RHI command-list state tracking.
- Add naming or documentation that makes manual FrameGraph-planned barriers versus RHI-owned state tracking explicit.
- Defer automatic barrier implementation unless it is already present behind the D3D12 backend.

Done criteria:

- FrameGraph barrier planning and RHI command-list barrier emission have clear names and boundaries.
- Future Vulkan/D3D12 backend work can see where state tracking belongs.
- No hidden behavior change to resource transitions.

Implementation prompt:

```text
Implement Phase 0E of docs/plans/renderer-framegraph-denoising-plan.md. Do not build between phases.

Define Sparkle's command-list state and barrier policy against NVRHI's model. Start from:
- Engine/RHI/Public/Commands/RenderCommandList.h
- Engine/RHI/Public/Interop/ResourceState.h
- Engine/RHI/Private/D3D12/D3D12RenderCommandList.h/.cpp
- Engine/Renderer/Private/FrameGraph/Compiler/FrameGraphCompiler*.cpp
- Engine/Renderer/Private/FrameGraph/Execution/FrameGraphBarrierPlayback.cpp
- Engine/Renderer/Private/FrameGraph/FrameGraph.h

Document and/or rename the boundary between FrameGraph-planned barriers and RHI command-list barrier emission. Keep default behavior unchanged. Do not implement broad automatic barriers unless existing backend code already supports it cleanly. Prefer names that distinguish planned graph barriers from command-list barrier commands.

Validation greps:
- TransitionResource|AliasResource|UnorderedAccessBarrier
- CompiledBarrier|CompiledAliasingBarrier|ResourceState
- automatic barrier/state tracking wording added to docs or comments, if any
- git diff --check

Skip compile/build.
```

Phase 0E audit findings:

Source checks performed:

- `RenderCommandList` exposes manual command-list emission for `CopyResource`, `AliasResource`, `TransitionResource`, and `UnorderedAccessBarrier`. There is no RHI-side automatic barrier API, no `commitBarriers` equivalent, and no persistent command-list state tracker in the current abstraction.
- `ResourceState` is a small backend-neutral state vocabulary and should remain the shared state name. It maps cleanly to Donut/NVRHI-style `ResourceState` vocabulary and to D3D12 resource states through backend conversion.
- `D3D12RenderCommandList` directly translates the three barrier commands into one D3D12 `ResourceBarrier` call each. It does not batch, infer, split, or merge barriers.
- `FrameGraphCompiler` is the current state-planning owner. It seeds compile resources from `ResourceRegistry` runtime state, infers each pass declaration's required `ResourceState`, appends transition/UAV `CompiledBarrier` records, updates runtime state feedback, and appends final restore barriers where required.
- `FrameGraphCompilerTransients` owns aliasing placement for transient physical-block reuse and appends `CompiledAliasingBarrier` records to the pass that starts the next alias owner.
- `FrameGraphBarrierPlayback` owns translation from graph plan records to renderer command-context calls. It resolves graph handles to native resources, then emits `AliasResource`, `TransitionResource`, or `UnorderedAccessBarrier`.
- `RenderCommandContext` is only a renderer-private forwarding and diagnostics wrapper over `RenderCommandList` for these commands.
- The one non-FrameGraph manual renderer transition is `Renderer::TransitionRenderProduct`, which asks FrameGraph for tracked state, emits a command-list transition, then feeds the new state back to FrameGraph. That path should remain explicit and rare because it bypasses compiled pass planning.

Policy decision:

| Layer | Barrier/state responsibility | Naming policy |
| --- | --- | --- |
| FrameGraph compiler | Owns planned pass-order transitions, UAV barriers, final-state restores, transient aliasing barriers, and runtime state feedback for graph resources. | Keep plan-record wording for now; extract/rename in Phase 4 to graph-owned names such as `FrameGraphBarrier` and `FrameGraphAliasingBarrier` once compile products move out of the monolithic `FrameGraph` class. |
| Resource registry/runtime state | Owns carried `currentState` for graph resources between compile steps and explicit renderer feedback. | Keep `FrameGraphResourceRuntimeState::currentState`; it describes graph-tracked state, not command-list internal state. |
| Barrier playback | Owns converting planned graph records into command-context emission calls. | Keep `EmitCompiledBarriers`/`EmitCompiledAliasingBarriers` until Phase 4 extraction; future names should emphasize playback of a graph plan. |
| Render command context | Owns renderer-local forwarding to `RenderCommandList`; it should not infer missing transitions. | Keep `TransitionResource`, `AliasResource`, and `UnorderedAccessBarrier` as imperative command emission names. |
| Render command list / D3D12 command list | Owns backend-neutral command-list command emission and backend translation only. | Do not rename to automatic/stateful names unless a real RHI command-list state tracker is introduced. |

NVRHI comparison:

- NVRHI's command-list model can track resource states and commit barriers as command-list state. Sparkle's current model is graph-planned and manually emitted: the FrameGraph compiler decides barriers, and the command list executes exactly what it is told.
- That difference is acceptable for Sparkle's current single-graph renderer because it makes pass declarations the source of truth. It should be documented as intentional rather than papered over with NVRHI-like automatic names.
- If automatic barriers are introduced later for non-FrameGraph code, they should be a separate RHI command-list state-tracker feature with explicit opt-in naming, not an implicit change to FrameGraph playback.
- Future Vulkan work should preserve the same boundary: FrameGraph produces planned synchronization intent; backend command lists translate command emission to backend-specific barriers.

Phase 0E result: keep the current behavior and command names. Sparkle's near-term standard is manual FrameGraph-planned barriers plus direct RHI command-list emission. Defer graph plan type renames to Phase 4, and defer any NVRHI-style automatic barrier tracker until there is a concrete non-FrameGraph use case and a backend-neutral contract for it.

## Phase 0F: Align Render Pass Shape With Donut

Goal: make Sparkle's concrete renderer passes understandable in the same way Donut render passes are understandable.

Planned work:

- Audit every concrete pass under `Engine/Renderer/Private/Passes` for Donut-style shape:
  - pass owns or references stable shader/pipeline/binding state through `Private/Pipeline` ownership.
  - pass exposes a clear setup/init path and an execution path.
  - execution consumes declared inputs and a command/resource access context, not the full graph when avoidable.
- Keep pass implementation in `Private/Passes`; keep frame orchestration in `Private/Frame`; keep pipeline creation/cache/binding in `Private/Pipeline`.
- Rename helper functions so frame assembly says whether it creates targets or schedules passes.
- Prefer Donut-like `*Pass` class names for concrete features, but do not force `RenderModule` unless a class truly owns feature-level lifecycle beyond one pass.

Done criteria:

- Concrete passes read as pass implementations rather than graph internals.
- Frame assembly reads as `Create*Targets` plus `Add*Passes`/`Schedule*Passes`.
- Pipeline/binding ownership is ready for the later pipeline runtime denoising phase.

Implementation prompt:

```text
Implement Phase 0F of docs/plans/renderer-framegraph-denoising-plan.md. Do not build between phases.

Align renderer pass shape with NVIDIA Donut while preserving Sparkle's FrameGraph scheduler. Start from:
- Engine/Renderer/Private/Passes/**
- Engine/Renderer/Private/Pipeline/**
- Engine/Renderer/Private/Frame/**
- Engine/Renderer/Private/FrameGraph/Execution/**

For each concrete pass, check whether the class owns or references pipeline/binding state through Pipeline, whether setup/init is separated from execution, and whether execution can depend on a narrow command/resource access context rather than the full FrameGraph. Keep pass classes under Private/Passes and frame orchestration under Private/Frame.

Do not invent plugin-level RenderModule abstractions in this phase. Use Donut's explicit *Pass shape as the guide. Rename frame helper functions only when the name hides whether it creates targets or schedules passes. Preserve behavior and defer compile/build.

Validation greps:
- FrameGraph includes inside Private/Passes that can wait for the Phase 7 execution facade
- Build* helper names in Private/Frame that actually schedule passes
- pipeline/binding ownership includes from Passes to Pipeline
- git diff --check

Skip compile/build.
```

Phase 0F audit findings:

Source checks performed:

- Concrete feature passes under `Engine/Renderer/Private/Passes` are already named in a Donut-readable `*Pass` style: `GBufferPass`, `DirectLightingPass`, `IndirectLightingPass`, `LightingCompositePass`, `SkyPass`, `VisualizeBuffersPass`, and `ComputeClearPass`.
- Pass implementation files already stay under `Private/Passes`; frame orchestration stays under `Private/Frame`; pipeline creation, shader runtime storage, binding overrides, and binding application mostly stay under `Private/Pipeline`.
- Every concrete pass still uses static metadata/setup/execution methods rather than an object that owns lifecycle and caches. This is coherent for the current FrameGraph scheduler, but it is not yet Donut's pass-object shape.
- `RenderPassPipelineTraits`, `RenderPassShaderRuntime`, `PipelineStateManager`, `PassBinder`, and `PassBindingOverrides` are the stable pipeline/binding seam today. The main mismatch is that runtime type aliases still live in `FrameGraph/RenderPassRuntime.h`, so pipeline ownership is not fully visually separated from FrameGraph ownership yet.
- Pass execution still depends on `RenderGraphPassContext`. Most passes reach `context.Graph`, `context.Commands`, `context.Runtime.HardwareInterface`, `context.Frame`, and `context.Diagnostics`, so Phase 7 should introduce the narrower pass execution/resource access facade before pass-object work goes deeper.
- `PassUtilities` is the shared bridge between authored passes and pipeline/binding operations. It still takes `FrameGraph&`/`const FrameGraph&` because binding resolves graph resources directly.
- `SkyPass` has the clearest pass-local cache smell: the sky texture descriptor table is cached in file-local helpers rather than in a pass-owned or pipeline-owned runtime object. Keep behavior for now; revisit after Phase 8/8A.

Concrete pass shape map:

| Pass | Current setup/init shape | Current execution shape | Phase 0F decision |
| --- | --- | --- | --- |
| `GBufferPass` | Static shader metadata, static resource declaration, static per-view parameter sync, pipeline runtime through `PipelineStateManager`/`RenderPassPipelineTraits`. | `Execute` prepares render targets through `context.Graph`, configures the command context, binds through `PassUtilities`, and draws mesh data directly. | Keep class name and behavior. It is the representative raster pass for Phase 7 resource-access narrowing and Phase 8A pass-object ownership. |
| `DirectLightingPass` | Static metadata, resource declaration, and per-frame/per-view parameter sync. | Dispatches through `PassUtilities` using runtime from `context.Runtime.GetPassRuntime`. | Keep. It already reads like a single compute pass, but execution should stop receiving the full graph in Phase 7. |
| `IndirectLightingPass` | Static metadata and resource declaration. | Dispatches through `PassUtilities`; no per-frame parameter sync currently needed. | Keep. It is a simple compute-pass baseline. |
| `LightingCompositePass` | Static metadata, resource declaration, and per-frame/per-view parameter sync. | Dispatches through `PassUtilities` and writes scene color. | Keep. It should follow the same Phase 7/8/8A path as direct lighting. |
| `SkyPass` | Static metadata, resource declaration, per-frame/per-view/sampler parameter sync, and a file-local sky texture descriptor cache. | Dispatches through `PassUtilities` with a descriptor-table override. | Keep behavior. Move persistent sky binding/cache ownership later, after binding-set and pipeline runtime seams are clearer. |
| `VisualizeBuffersPass` | Static metadata, resource declaration, and per-frame parameter sync. | Dispatches through `PassUtilities` and writes scene color for debug view modes. | Keep. It should stay a concrete `*Pass`, not become a module. |
| `ComputeClearPass` | Static metadata and resource declaration. | Dispatch helper used by utility/clear paths with explicit width/height. | Keep as a utility pass. Do not force Donut object ownership until there is a broader compute utility pattern. |

Code change made in Phase 0F:

- Split `BuildGBuffer` into `CreateGBufferTargets` and `AddGBufferPass` so target creation and pass scheduling are distinct.
- Renamed pure scheduling helpers from `Build*` to `Add*Pass`/`Add*Passes`:
  - `BuildDirectLighting` -> `AddDirectLightingPass`
  - `BuildIndirectLighting` -> `AddIndirectLightingPass`
  - `BuildLightingComposite` -> `AddLightingCompositePass`
  - `BuildVisualizeBuffers` -> `AddVisualizeBuffersPass`
  - `BuildSky` -> `AddSkyPass`
  - `BuildPresentation` -> `AddPresentationPass`
  - `BuildLighting` -> `AddLightingPasses`
- `BuildFrame` now reads as target creation followed by pass scheduling for GBuffer, lighting, and presentation. `BuildFrame` and `BuildFrameContext` remain acceptable because they build frame-level data rather than hiding one specific scheduled pass.

Deferred Donut-style ownership work:

- Do not introduce `RenderModule`. None of the audited classes owns feature-level lifecycle beyond a scheduled pass strongly enough to justify it.
- Do not convert static pass classes into stateful objects in Phase 0F. First narrow `RenderGraphPassContext` in Phase 7, move pass runtime storage out of FrameGraph ownership in Phase 8, then establish the pass-object pattern in Phase 8A.
- Keep pass pipeline creation in `RenderPassPipelineTraits` for now. It is centralized and behavior-preserving, but Phase 8 should move runtime type ownership fully into `Private/Pipeline`.
- Keep pass binding through `PassUtilities`/`PassBinder` until Phase 7 provides a resource access facade and Phase 0D's deferred binding-set design has a concrete owner.

Phase 0F result: frame assembly now uses creation/scheduling names where the distinction was cheap and behavior-preserving. Concrete pass classes remain Donut-readable by name, while deeper Donut-style ownership is explicitly deferred until execution context and pipeline runtime seams are narrower.

## Phase 1: Collapse Accidental DLL Surface

Goal: make private implementation private before doing deeper structural changes.

Planned moves and renames:

- Remove `SPARKLE_RENDERER_API` from private-only framegraph types where no public DLL boundary requires it:
  - `FrameGraph`
  - `PassBuilder`
  - `ResourceRegistry`
  - `RenderPassContext`
  - `RenderGraphPassContext`
  - per-pass runtime structs in `RenderPassRuntime.h`
- Keep public API export only on Renderer public contracts and intentional cross-module types.
- If any exported private type is required only because a public header includes it, fix the include boundary rather than keeping the export.

Done criteria:

- `SPARKLE_RENDERER_API` does not appear in `Engine/Renderer/Private/FrameGraph/**` unless explicitly justified.
- Renderer public headers do not include private framegraph headers.
- Source-level validation is clean; compile/build validation is deferred until the selected phase batch is complete.

Implementation prompt:

```text
Implement Phase 1 of docs/plans/renderer-framegraph-denoising-plan.md. Do not build between phases.

Remove accidental DLL export markers from private framegraph implementation types. Start with these files:
- Engine/Renderer/Private/FrameGraph/FrameGraph.h
- Engine/Renderer/Private/FrameGraph/Builder/PassBuilder.h
- Engine/Renderer/Private/FrameGraph/ResourceRegistry.h
- Engine/Renderer/Private/FrameGraph/RenderPassContext.h
- Engine/Renderer/Private/FrameGraph/Execution/RenderGraphPassContext.h
- Engine/Renderer/Private/FrameGraph/RenderPassRuntime.h

Before editing, confirm no public Renderer header includes these private headers. If a public header does, repair the include boundary instead of leaving the private type exported. Remove SPARKLE_RENDERER_API only from private implementation types; do not touch Renderer public API exports. Preserve all names and behavior in this phase. Validate with grep for SPARKLE_RENDERER_API under Engine/Renderer/Private/FrameGraph/**, grep public headers for private FrameGraph includes, and git diff --check. Skip compile/build.
```

Phase 1 findings:

Source checks performed:

- `Engine/Renderer/Public/**/*.h` does not include the private FrameGraph implementation headers listed in this phase.
- Public Renderer headers only include public FrameGraph handle contracts where expected, such as `PassParameterSet.h` including public `BufferHandle.h` and `TextureHandle.h`.
- The accidental private export markers were limited to `FrameGraph`, `PassBuilder`, `ResourceRegistry`, `RenderPassContext`, `RenderGraphPassContext`, and the pass runtime structs in `RenderPassRuntime.h`.
- Public Renderer API exports remain in public contracts such as `Renderer`, viewport products, diagnostics snapshots, public scene data, texture diagnostics, and `RendererAPI.h` itself.

Code change made in Phase 1:

- Removed `SPARKLE_RENDERER_API` from private-only FrameGraph implementation types:
  - `FrameGraph`
  - `PassBuilder`
  - `ResourceRegistry`
  - `RenderPassContext`
  - `RenderGraphPassContext`
  - `RasterPassRuntime`
  - `DirectLightingPassRuntime`
  - `IndirectLightingPassRuntime`
  - `LightingCompositePassRuntime`
  - `SkyPassRuntime`
  - `VisualizeBuffersPassRuntime`
  - `ComputeClearPassRuntime`
- Removed now-unused `RendererAPI.h` includes from those private headers where the macro was the only reason for the include.

Phase 1 result: private FrameGraph implementation types no longer look like Renderer DLL surface. No public include boundary had to be repaired, and no public Renderer exports were changed.

## Phase 2: Choose And Apply The Public FrameGraph Vocabulary

Goal: make the public graph contract vendor-readable and unambiguous.

Planned moves and renames:

- Rename public graph handle and desc types in one pass if the global names remain un-namespaced:
  - `ResourceHandle` -> `FrameGraphResourceHandle`
  - `TextureHandle` -> `FrameGraphTextureHandle`
  - `BufferHandle` -> `FrameGraphBufferHandle`
  - `FrameGraphTextureDesc` stays `FrameGraphTextureDesc`
  - `FrameGraphBufferDesc` stays `FrameGraphBufferDesc`
- Update shader parameter fields and pass parameter bindings to use the new handle names.
- Avoid compatibility headers for the old public paths.
- Keep descriptor vocabulary backend-neutral: size, format, usage intent, clear value, mip count, sample count, and resource class. No D3D12/DXGI naming.
- Rename private Sparkle-owned `RenderGraph*` execution names in a later execution-context phase, not as a public folder rename.

Done criteria:

- Public graph handles cannot be confused with RHI or GameFramework handles.
- Public folder and type names use `FrameGraph` consistently.
- No old public include paths remain in code.

Implementation prompt:

```text
Implement Phase 2 of docs/plans/renderer-framegraph-denoising-plan.md. Do not build between phases.

Make the public graph vocabulary coherent while keeping FrameGraph as Sparkle's chosen graph term. Keep the public folder Engine/Renderer/Public/FrameGraph and rename only the generic public handle contracts:
- ResourceHandle -> FrameGraphResourceHandle
- TextureHandle -> FrameGraphTextureHandle
- BufferHandle -> FrameGraphBufferHandle
- Keep FrameGraphTextureDesc and FrameGraphBufferDesc names unless the inventory finds a more specific reason to change them.

Use symbol-aware rename where available for C++ types, then move renamed handle headers and patch include paths. Update shader parameter wrappers, pass parameter bindings, frame assembly, passes, pipeline binder, resource registry, compiler, and diagnostics call sites. Do not add old-path forwarding headers. Keep descriptor fields backend-neutral and do not introduce D3D12/DXGI vocabulary.

Key searches before and after:
- Renderer/Public/FrameGraph
- #include "Renderer/Public/FrameGraph/
- \bResourceHandle\b, \bTextureHandle\b, \bBufferHandle\b in Renderer code
- FrameGraphTextureDesc|FrameGraphBufferDesc
- RenderGraph in Sparkle-owned graph type names, to defer or assign to Phase 7 when it is execution-context specific

Validation is grep/file-search/git diff --check only. The phase is complete when public graph handle names are unmistakably FrameGraph-owned and no new RenderGraph public vocabulary has been introduced.
```

Phase 2 findings:

Source checks performed:

- The public graph handle contracts were the only generic public FrameGraph handles that needed renaming.
- `FrameGraphTextureDesc` and `FrameGraphBufferDesc` were already FrameGraph-prefixed and backend-neutral, so this phase kept those descriptor names unchanged.
- Public FrameGraph descriptors still do not expose `D3D12`, `DXGI`, `ID3D12`, or `ComPtr` vocabulary.
- No public `RenderGraph*` vocabulary was introduced. Existing private execution-context names remain deferred to Phase 7.

Code change made in Phase 2:

- Renamed public handle types:
  - `ResourceHandle` -> `FrameGraphResourceHandle`
  - `TextureHandle` -> `FrameGraphTextureHandle`
  - `BufferHandle` -> `FrameGraphBufferHandle`
- Renamed public handle headers without adding old-path forwarding headers:
  - `ResourceHandle.h` -> `FrameGraphResourceHandle.h`
  - `TextureHandle.h` -> `FrameGraphTextureHandle.h`
  - `BufferHandle.h` -> `FrameGraphBufferHandle.h`
- Updated Renderer public shader parameter wrappers and private FrameGraph, frame assembly, pass, pipeline/binding, resource registry, compiler, execution, and diagnostics call sites to use the FrameGraph-prefixed handle names.

Phase 2 result: the public graph handle vocabulary is now unambiguously FrameGraph-owned while the public folder remains `Engine/Renderer/Public/FrameGraph`. Descriptor vocabulary stayed backend-neutral, no compatibility headers were kept, and `RenderGraph` cleanup remains assigned to the later execution-context phase.

## Phase 3: Separate Setup-Time Authoring From Graph Storage

Goal: align Sparkle with RDG's setup timeline and remove the `PassBuilder` friendship pressure.

Planned moves and renames:

- Introduce or clarify the setup-time `FrameGraphBuilder` owner for AddPass, resource creation/import, and parameter allocation.
- Rename `PassBuilder` to `PassResourceBuilder` or `PassResourceDeclarationBuilder` because it records resource declarations rather than constructing passes.
- Move pass resource declaration collection out of direct `FrameGraph` mutation where practical. The declaration builder should produce declarations that graph storage consumes.
- Keep authored pass setup lambdas ergonomic:
  - create parameters
  - bind graph resources into parameters
  - register pass flags and execute lambda
- Preserve Sparkle's typed shader-pass helpers, but make them setup helpers on the builder rather than methods that force all callers to include the full graph implementation.

Done criteria:

- Setup code reads as builder work, not graph execution work.
- `PassResourceBuilder` no longer needs broad friendship with the full graph type, or the friendship is narrowed to a small declaration sink.
- Frame assembly code still reads naturally when adding GBuffer, lighting, sky, visualize, and presentation passes.

Implementation prompt:

```text
Implement Phase 3 of docs/plans/renderer-framegraph-denoising-plan.md. Do not build between phases.

Separate setup-time graph authoring from graph storage. Start by reading:
- Engine/Renderer/Private/FrameGraph/FrameGraph.h/.cpp
- Engine/Renderer/Private/FrameGraph/Builder/FrameGraphBuilder.h/.cpp
- Engine/Renderer/Private/FrameGraph/Builder/PassBuilder.h/.cpp
- Engine/Renderer/Private/FrameGraph/FrameGraphDeclaration.cpp
- Engine/Renderer/Private/Frame/*.h/.cpp
- Engine/Renderer/Private/Passes/*Pass*.h/.cpp

Preferred shape:
- Introduce or expand FrameGraphBuilder as the setup-time API for AddPass, AddRasterPass, AddComputePass, CreateTexture, CreateBuffer, ImportTexture, ImportBuffer, AllocParameters, and AllocPassParameters.
- Rename PassBuilder to PassResourceBuilder or PassResourceDeclarationBuilder. It should declare resource usage, not own pass construction.
- Move declaration recording behind a narrow sink or builder-owned storage rather than broad friendship with the full graph owner when practical.
- Keep authored pass ergonomics: allocate parameters, bind graph resources into parameters, register a pass and execute lambda.

Keep behavior equivalent. Do not split compiler or plan types in this phase. Validate with stale-name greps for PassBuilder if renamed, include-boundary scans for full graph includes that can be narrowed, and git diff --check. Skip compile/build.
```

Phase 3 findings:

- `FrameGraphBuilder` is now the setup-time authoring wrapper around a `FrameGraph`. Frame assembly and pass declaration code use it for `AddPass`, typed pass registration, texture/buffer creation and import, parameter allocation, and shader-parameter helper construction.
- The old dependency-driven graph creator is now named `FrameGraphFactory`; it constructs the underlying `FrameGraph`, creates a setup builder, and calls `BuildFrame` through that setup surface.
- `PassBuilder` was renamed to `PassResourceBuilder` to make the pass setup lambda parameter read as resource declaration work rather than pass construction.
- `PassResourceDeclarationSink` now owns declaration recording during `FrameGraph::Setup`. `FrameGraph` no longer exposes pass-setup declaration mutation APIs, no longer grants `PassResourceBuilder` friendship, and no longer stores active setup declaration state.
- Frame helper functions and concrete pass `DeclareResources` functions now accept `FrameGraphBuilder&`. Execution-time helpers and binding utilities still accept `const FrameGraph&` because they resolve descriptors, bind resources, clear targets, and copy graph resources during execution.
- Behavior is intended to remain equivalent: this phase only separates setup authoring from graph storage and keeps compile plan ownership inside `FrameGraph` for Phase 4.
- No build was run for this phase per the phase workflow; validation is source-only.

## Phase 4: Extract Compile Plan Types From The Graph Class

Goal: make compilation output a first-class artifact rather than nested state inside the graph owner.

Planned moves and renames:

- Move these nested types out of `FrameGraph` into a compile/plan header:
  - `CompiledPlan`
  - `CompilePassRecord`
  - `CompileResourceEntry`
  - `CompiledBarrier`
  - `CompiledAliasingBarrier`
  - `CompiledTransientResourcePlan`
  - `CompiledPhysicalBlockPlan`
  - `ResourceVersion`
- Preferred names:
  - `FrameGraphPlan`
  - `FrameGraphPassNode`
  - `FrameGraphResourceNode`
  - `FrameGraphResourceVersion`
  - `FrameGraphBarrier`
  - `FrameGraphAliasingBarrier`
  - `FrameGraphTransientResourcePlan`
  - `FrameGraphPhysicalAllocationPlan`
- Keep compiler output immutable from the executor's point of view.
- Move plan-oriented diagnostics names and labels into the pass node rather than recomputing them across execution.

Done criteria:

- `FrameGraphCompiler` can be understood without opening the main graph owner.
- Execution consumes `const FrameGraphPlan&`.
- Compiler, executor, and diagnostics no longer need nested type aliases from `FrameGraph`.

Implementation prompt:

```text
Implement Phase 4 of docs/plans/renderer-framegraph-denoising-plan.md. Do not build between phases.

Extract compile plan data structures out of the graph owner without changing compiler behavior. Start from:
- Engine/Renderer/Private/FrameGraph/FrameGraph.h
- Engine/Renderer/Private/FrameGraph/Compiler/FrameGraphCompiler.h/.cpp
- Engine/Renderer/Private/FrameGraph/Compiler/FrameGraphCompilerDependencies.cpp
- Engine/Renderer/Private/FrameGraph/Compiler/FrameGraphCompilerTransients.cpp
- Engine/Renderer/Private/FrameGraph/Execution/FrameGraphExecution.cpp
- Engine/Renderer/Private/FrameGraph/Execution/FrameGraphBarrierPlayback.cpp

Create a focused private plan header, for example Engine/Renderer/Private/FrameGraph/Compiler/FrameGraphPlan.h or Engine/Renderer/Private/FrameGraph/Plan/FrameGraphPlan.h. Move the nested compile structs out of FrameGraph and rename them to the selected FrameGraphPlan vocabulary. Keep the compiler's algorithms intact and update aliases/call sites mechanically. Execution should consume const FrameGraphPlan&. Do not split resource registry or transient allocation in this phase.

Validation greps:
- FrameGraph::CompiledPlan|FrameGraph::CompilePassRecord|FrameGraph::CompiledBarrier|FrameGraph::ResourceVersion
- CompiledPlan\b if renamed
- FrameGraphPlan include usage
- git diff --check

Skip compile/build.
```

Phase 4 findings:

- Added the private plan contract `Engine/Renderer/Private/FrameGraph/Compiler/FrameGraphPlan.h` so compiler output can be read without opening the main graph owner.
- Moved the former nested compile records out of `FrameGraph` and renamed them to the selected FrameGraph vocabulary: `FrameGraphPlan`, `FrameGraphPassNode`, `FrameGraphResourceNode`, `FrameGraphResourceVersion`, `FrameGraphBarrier`, `FrameGraphAliasingBarrier`, `FrameGraphTransientResourcePlan`, and `FrameGraphPhysicalAllocationPlan`.
- Moved pass/resource index aliases and invalid index constants into the plan contract as `FrameGraphPassIndex`, `FrameGraphResourceIndex`, `INVALID_FRAME_GRAPH_PASS_INDEX`, and `INVALID_FRAME_GRAPH_RESOURCE_INDEX`.
- `FrameGraph::Compile` now returns `FrameGraphPlan`, `FrameGraph::Execute` consumes `const FrameGraphPlan&`, and the Renderer frame record path stores the compile output as `FrameGraphPlan`.
- `FrameGraphCompiler`, barrier playback, transient materialization planning, external resource view sync, and the transient allocator now consume the extracted plan types instead of `FrameGraph::...` nested records.
- Pass diagnostics names, display labels, and event scope labels remain stored on `FrameGraphPassNode`, preserving the Phase 3 setup-time formatting path and keeping execution from recomputing those labels.
- No compiler behavior, dependency planning, barrier planning, transient allocation, or resource registry ownership was intentionally changed. No build was run for this phase per the phase workflow.

## Phase 5: Split Resource Metadata, State Tracking, And Resolution

Goal: make resource lifetime and backend realization explicit without turning the graph into a second RHI.

Planned moves and renames:

- Split `ResourceRegistry` into focused owners:
  - `FrameGraphResourceRegistry`: graph handles, descriptors, ownership, kind, debug name, initial/final boundary states.
  - `FrameGraphResourceStateTracker`: carried runtime state and host-side state feedback.
  - `FrameGraphResourceResolver`: resolved native resource handles and RHI descriptor handles used during execution.
- Keep D3D12-native conversion and descriptor creation behind existing private RHI/renderer seams.
- Preserve the repo memory rule: imported registry state should store opaque handles only, not concrete D3D12 types.
- Make transient resource materialization read from the compiled plan and write resolved accesses through the resolver.

Done criteria:

- Metadata changes do not accidentally mutate runtime states.
- Runtime state feedback from host-side render-product transitions has a single owner.
- Resource resolution is visibly execution-time/backend-facing work.

Implementation prompt:

```text
Implement Phase 5 of docs/plans/renderer-framegraph-denoising-plan.md. Do not build between phases.

Split ResourceRegistry responsibilities only after Phase 4 plan extraction is complete. Start from:
- Engine/Renderer/Private/FrameGraph/ResourceRegistry.h
- Engine/Renderer/Private/FrameGraph/Resources/ResourceRegistry.cpp
- Engine/Renderer/Private/FrameGraph/Resources/FrameGraphExternalResources.cpp
- Engine/Renderer/Private/FrameGraph/Resources/FrameGraphResourceResolution.cpp
- Engine/Renderer/Private/FrameGraph/Resources/FrameGraphTextureRegistration.cpp
- Engine/Renderer/Private/FrameGraph/Resources/FrameGraphTransientPlanning.cpp
- Engine/Renderer/Private/FrameGraph/Execution/FrameGraphExecution.cpp
- Engine/Renderer/Private/FrameGraph/Compiler/*

Preferred split:
- FrameGraphResourceRegistry owns metadata, descriptors, ownership, resource kind, debug names, and registered handle lists.
- FrameGraphResourceStateTracker owns current state and boundary state feedback from host-side transitions.
- FrameGraphResourceResolver owns resolved native resource handles and RHI descriptor handles used during execution.

Keep imported resource state opaque: NativeResourceHandle plus RhiCpuDescriptorHandle/RhiGpuDescriptorHandle only. Do not introduce concrete D3D12 types or backend-specific public contracts. Preserve UpdateTrackedResourceState/GetTrackedResourceState semantics. Keep behavior equivalent and avoid transient allocator behavior changes.

Validation greps:
- ResourceRegistry old responsibilities after split
- D3D12|DXGI in Renderer/Public/FrameGraph
- GetTrackedResourceState|UpdateTrackedResourceState call sites
- git diff --check

Skip compile/build.
```

## Phase 6: Make Transient Allocation And Aliasing A Named Subsystem

Goal: make Sparkle's transient resource planning easier to debug and eventually portable beyond D3D12.

Planned moves and renames:

- Keep `FrameGraphTransientAllocator` private, but split planning vocabulary from physical allocator implementation.
- Use plan names that map to Vulkan/D3D12 concepts without exposing backend objects:
  - transient resource lifetime
  - physical allocation block
  - aliasing barrier
  - first/last user pass
  - first/last execution index
- Add debug toggles or compile-time switches after the structure settles:
  - disable transient aliasing
  - extend transient lifetimes
  - log aliasing barriers for a filtered resource name
- Do not move backend native allocation internals into public graph contracts.

Done criteria:

- Aliasing barriers are planned in one place and emitted in one place.
- It is easy to compare transient lifetimes against the execution order.
- Disabling transient aliasing for debugging does not require editing pass code.

Implementation prompt:

```text
Implement Phase 6 of docs/plans/renderer-framegraph-denoising-plan.md. Do not build between phases.

Make transient allocation and aliasing a named subsystem without changing the default runtime result. Start from:
- Engine/Renderer/Private/FrameGraph/Resources/FrameGraphTransientAllocator.h/.cpp
- Engine/Renderer/Private/FrameGraph/Resources/FrameGraphTransientPlanning.cpp
- Engine/Renderer/Private/FrameGraph/Compiler/FrameGraphCompilerTransients.cpp
- Engine/Renderer/Private/FrameGraph/Execution/FrameGraphBarrierPlayback.cpp
- the FrameGraphPlan types extracted in Phase 4

Separate transient planning vocabulary from physical allocation implementation. Keep plan records backend-neutral: lifetime, physical block, pool, aliasing barrier, first/last user pass, first/last execution index, required states. Keep native heap/resource creation inside the private allocator. If adding debug toggles, make them narrow and default-preserving; do not add console/UI dependencies unless the repo already has a matching pattern.

Validation greps:
- aliasing barrier planning and emission call sites
- FrameGraphTransient names if renamed
- native D3D12 concrete types leaking out of allocator/private RHI seams
- git diff --check

Skip compile/build.
```

## Phase 7: Rename Execution Contexts And Runtime Services

Goal: make pass execution code read like the actual timeline.

Planned moves and renames:

- Rename `RenderGraphPassContext` to `PassExecutionContext` or `FrameGraphPassExecutionContext`.
- Rename `RenderPassContext` to `PassRuntimeServices` or `RenderPassRuntimeServices`.
- Keep `PassExecutionContext` narrow:
  - command context
  - frame data
  - pass runtime services
  - pass diagnostics
  - graph resource access facade
- Avoid handing authored passes the full graph if they only need to bind, clear, copy, or resolve declared graph resources.
- Consider a small `FrameGraphResourceCommands` or `FrameGraphResourceAccess` facade for execution-time operations currently reached through `context.Graph`.

Done criteria:

- A reader can distinguish setup builder, execution context, runtime services, and resource access without opening headers.
- Passes no longer call into a kitchen-sink graph object when a narrower execution facade would do.
- Context names are not reused for service locators.

Implementation prompt:

```text
Implement Phase 7 of docs/plans/renderer-framegraph-denoising-plan.md. Do not build between phases.

Rename execution-time contexts and introduce a narrower resource access facade if practical. Start from:
- Engine/Renderer/Private/FrameGraph/Execution/RenderGraphPassContext.h
- Engine/Renderer/Private/FrameGraph/RenderPassContext.h
- Engine/Renderer/Private/FrameGraph/Execution/FrameGraphExecution.cpp
- Engine/Renderer/Private/Passes/*.h/.cpp
- Engine/Renderer/Private/Passes/PassUtilities.h
- Engine/Renderer/Private/Pipeline/PassBinder.h/.cpp

Preferred names:
- RenderGraphPassContext -> PassExecutionContext or FrameGraphPassExecutionContext
- RenderPassContext -> PassRuntimeServices or RenderPassRuntimeServices

Keep PassExecutionContext narrow. It may contain Commands, Frame, RuntimeServices, Diagnostics, and a graph resource access facade. Avoid giving passes the full graph if only bind/clear/copy/resolve helpers are needed. If introducing FrameGraphResourceAccess or FrameGraphResourceCommands, move only execution-safe operations first; leave setup and compiler APIs out of it.

Validation greps:
- RenderGraphPassContext|RenderPassContext old names
- context.Graph usages that should become resource-access calls
- Context names reused for service locators
- git diff --check

Skip compile/build.
```

## Phase 8: Denoise Pass Runtime And Pipeline Binding

Goal: reduce hardcoded pass runtime traits and make pipeline/binding ownership clearer.

Planned moves and renames:

- Move per-pass pipeline runtime details out of `FrameGraph/RenderPassRuntime.h` and into `Pipeline` ownership.
- Replace repeated runtime structs where possible with a common `RasterPassPipelineRuntime` and `ComputePassPipelineRuntime`, or a `PassPipelineRuntime` variant if the shape stays small.
- Keep pass-specific pipeline creation in `RenderPassPipelineTraits` only if it remains the cleanest local pattern. Otherwise move toward a private `PassPipelineRegistry` keyed by pass identity.
- Keep `PassBinder` private to pipeline/resource binding. It should depend on graph resource access, not the full graph class, once Phase 7 creates a narrower facade.
- Preserve shader parameter layout ownership rules: binding layouts must not outlive generated pass parameter layouts.

Done criteria:

- Adding a new pass does not require touching a broad framegraph runtime header unless the pipeline shape itself changes.
- Pipeline state ownership is visibly in `Private/Pipeline`, not `Private/FrameGraph`.
- Pass binding depends on declared parameters and graph resource access, not hidden global state.

Implementation prompt:

```text
Implement Phase 8 of docs/plans/renderer-framegraph-denoising-plan.md. Do not build between phases.

Denoise pass pipeline runtime ownership after execution context naming is settled. Start from:
- Engine/Renderer/Private/FrameGraph/RenderPassRuntime.h
- Engine/Renderer/Private/Pipeline/PipelineStateManager.h/.cpp
- Engine/Renderer/Private/Pipeline/RenderPassPipelineTraits.h
- Engine/Renderer/Private/Pipeline/PassBinder.h/.cpp
- Engine/Renderer/Private/Passes/*Pass*.h/.cpp

Move pass runtime types out of FrameGraph ownership and into Pipeline ownership. Prefer shared shapes such as RasterPassPipelineRuntime and ComputePassPipelineRuntime when the layout is identical. Keep pass-specific creation in RenderPassPipelineTraits if it remains the smallest clear abstraction; otherwise introduce a private PassPipelineRegistry. Preserve shader package reload behavior and lazy pipeline creation semantics. Do not change shader binding layout lifetimes.

Validation greps:
- FrameGraph/RenderPassRuntime includes
- RenderPassRuntimeTraits call sites
- per-pass runtime struct names moved/renamed
- PipelineStateManager lazy creation path
- git diff --check

Skip compile/build.
```

## Phase 8A: Move Passes Toward Donut-Style Ownership

Goal: address the largest render-shape difference after pipeline runtime ownership is cleaner: Sparkle passes should read more like Donut pass objects and less like static graph procedures.

Planned moves and renames:

- Revisit concrete pass classes after Phases 7 and 8 have narrowed execution context and pipeline runtime ownership.
- Prefer pass classes that have clear owned or injected dependencies:
  - shader package definition and metadata
  - pipeline/binding runtime or cache references
  - pass-specific persistent binding/cache data when needed
  - explicit execute entry point consuming `PassExecutionContext` and typed parameters
- Reduce reliance on static functions for behavior that is really pass-owned lifecycle or cache work.
- Keep static `Describe`/metadata helpers where they remain compile-time shader parameter contracts.
- Do not introduce a broad plugin `RenderModule` abstraction unless a pass owns feature-level lifecycle beyond a single graph pass.

Done criteria:

- Concrete pass headers separate shader parameter metadata from pass execution ownership.
- Pipeline/runtime access looks like pass-owned or pipeline-owned state, not a global trait maze.
- New passes can follow a Donut-like pattern without touching a broad FrameGraph runtime header.

Implementation prompt:

```text
Implement Phase 8A of docs/plans/renderer-framegraph-denoising-plan.md. Do not build between phases.

After Phase 7 narrows execution context and Phase 8 moves pipeline runtime into Pipeline ownership, move Sparkle concrete passes toward Donut-style ownership. Start from:
- Engine/Renderer/Private/Passes/*.h/.cpp
- Engine/Renderer/Private/Pipeline/**
- Engine/Renderer/Private/FrameGraph/Execution/**
- Engine/Renderer/Private/Frame/**

Keep shader parameter Describe/metadata functions static where useful, but move behavior that is pass-owned lifecycle, pipeline/binding cache access, or execution setup into clearer pass-owned methods or small pass runtime objects. Prefer concrete *Pass classes with explicit execute entry points consuming PassExecutionContext and typed parameters. Do not create a plugin RenderModule abstraction unless the code already has feature-level lifecycle that needs it.

Preserve behavior. Avoid broad rewrites of every pass if one representative pass can establish the pattern and the rest can follow mechanically in later phases. Do not add compatibility wrappers.

Validation greps:
- static void Execute|static void SetParameters|static void DeclareResources in Private/Passes
- RenderPassRuntimeTraits and PipelineStateManager usage from pass code
- FrameGraph includes in pass headers after the resource access facade exists
- git diff --check

Skip compile/build.
```

## Phase 9: Rename Frame Target Bundles And Frame Assembly Helpers

Goal: make frame assembly files communicate what they produce and what they schedule.

Planned moves and renames:

- Rename `FrameGraphProducts.h` to a target-bundle name, likely `FrameRenderTargets.h` or `RenderTargetBundles.h`.
- Consider names such as:
  - `SceneRenderTargets`
  - `GBufferRenderTargets`
  - `LightingRenderTargets`
- Move target bundle records under `Private/Frame/Targets` if they are frame assembly data, or `Private/FrameGraph/Resources` only if they are generic graph concepts.
- Rename frame helper functions to say whether they create resources or schedule passes:
  - `CreateLightingTargets` for target creation is good.
  - `CreateGBufferTargets` plus `AddGBufferPass` already split GBuffer target creation from pass scheduling in Phase 0F.
- Keep pass classes under `Private/Passes`; keep frame orchestration under `Private/Frame`.

Done criteria:

- Target bundles do not use vague product language.
- Frame assembly reads as resource creation plus pass scheduling.
- Pass implementation files do not become responsible for frame-level orchestration.

Implementation prompt:

```text
Implement Phase 9 of docs/plans/renderer-framegraph-denoising-plan.md. Do not build between phases.

Rename frame target bundles and frame assembly helpers after the graph core and execution context names have settled. Start from:
- Engine/Renderer/Private/FrameGraph/Features/FrameGraphProducts.h
- Engine/Renderer/Private/Frame/*.h/.cpp
- Engine/Renderer/Private/Passes/*
- any call sites that consume scene, GBuffer, lighting, or present target bundles

Preferred direction:
- FrameGraphProducts.h -> FrameRenderTargets.h or RenderTargetBundles.h
- product structs -> SceneRenderTargets, GBufferRenderTargets, LightingRenderTargets, and similar target-specific names
- helper functions should continue saying whether they create resources or add passes, following the Phase 0F `CreateGBufferTargets`/`AddGBufferPass` and `Add*Passes` naming.

Keep pass classes under Private/Passes and frame orchestration under Private/Frame. Do not move target bundles into generic FrameGraph resources unless they are truly reusable graph concepts. Avoid compatibility headers and remove old includes in the same phase.

Validation greps:
- FrameGraphProducts|Product in frame target bundle names
- BuildGBuffer|BuildLighting names that actually schedule passes
- includes of old target bundle header
- git diff --check

Skip compile/build.
```

## Phase 10: Add FrameGraph Validation And Debugging Gates

Goal: enforce the new boundaries after the structure settles.

Planned cleanup:

- Add targeted validation checks for:
  - no `SPARKLE_RENDERER_API` in private framegraph implementation headers unless allowlisted.
  - no mixed `FrameGraph`/`RenderGraph` names in Sparkle-owned graph types after the execution-context rename phase.
  - no public Renderer graph headers including private headers.
  - no D3D12/DXGI/native handle vocabulary in public graph contracts except deliberate interop contracts.
  - no pass execution code including compiler-only headers.
- Add graph diagnostic output after the plan types are stable:
  - pass order
  - culled passes
  - resource lifetime ranges
  - planned barriers
  - aliasing blocks
- Add resource and pass filters before adding more advanced async compute or multi-queue work.

Done criteria:

- Validation scripts encode the selected structure rather than transitional names.
- `git diff --check` passes.
- Targeted compile of Renderer and dependent app targets passes when the final post-batch build is run.
- Diagnostics can answer: why did this pass run, why was this resource alive, and why did this barrier exist?

Implementation prompt:

```text
Implement Phase 10 of docs/plans/renderer-framegraph-denoising-plan.md. This is the validation/diagnostics phase; do not run full builds until all selected code changes are complete or the user explicitly asks.

Add source-level validation gates and diagnostics after Phases 0A-9 settle the names and ownership. Start from:
- CMake/SparkleValidationTargets.cmake and CMake/Validation/** if an existing validation target fits
- Scripts/CI/** and Scripts/Internal/** for existing source-check patterns
- Engine/Renderer/Private/FrameGraph/**
- Engine/Renderer/Public/FrameGraph/**
- Engine/Renderer/Private/Frame/**
- Engine/Renderer/Private/Passes/**

Validation gates should check the final chosen structure:
- no SPARKLE_RENDERER_API in private framegraph headers unless allowlisted
- no new RenderGraph-prefixed Sparkle graph-owned types; FrameGraph remains the system term
- no public Renderer FrameGraph headers including Renderer private headers
- no D3D12, DXGI, ID3D12, ComPtr, or native backend vocabulary in public FrameGraph contracts unless the contract is explicitly an interop surface
- no pass execution code including compiler-only plan construction headers

Add graph diagnostics only where the plan/resource data already exists: pass order, culled passes, resource lifetime ranges, planned barriers, aliasing blocks, and optional pass/resource name filters. Prefer existing logging/diagnostics patterns and keep output quiet by default.

Validation for this phase:
- run the new validation script/target if it is source-only
- run targeted greps for each rule above
- run git diff --check
- defer compile/build until the final requested build batch
```

## Suggested Execution Order

1. Boundary inventory and exported-private cleanup.
2. Sparkle-to-Donut/NVRHI layer mapping.
3. RHI boundary alignment, device/service split, binding-set/framebuffer vocabulary, and command-list state policy.
4. Render pass shape audit against Donut's `*Pass` pattern.
5. Public FrameGraph vocabulary and public handle/desc rename.
6. Setup builder and pass declaration separation.
7. Compile plan extraction.
8. Resource registry/state/resolver split.
9. Transient allocator and aliasing cleanup.
10. Execution context and runtime services rename.
11. Pipeline runtime and binder denoising.
12. Donut-style pass ownership cleanup.
13. Frame target bundle and frame assembly naming cleanup.
14. Validation and diagnostic gates.

This order keeps the blast radius controlled: first compare Sparkle to Donut/NVRHI, then clarify the RHI and render-pass seams, then shrink and clarify the FrameGraph surface, then split authoring from compilation, then split resource ownership, then rename execution and pipeline dependencies once the graph core has smaller seams.

## Validation Strategy

For each phase:

- Run `git diff --check`.
- Run targeted stale-name greps for the phase's old names.
- Run boundary greps for public/private include leaks.
- Do not run CMake, MSBuild, CTest, or full project builds between phases. Defer compile/build validation until all selected phases are complete or the user explicitly asks for it.
- If CMake Tools cannot configure during the final build, report that explicitly and use source-level validation only unless the user requests a manual generated-project fallback.

At the end of the sequence:

- Run the Renderer target and the Showcase editor/runtime targets.
- Run a simple frame with transient aliasing enabled and disabled.
- Capture texture diagnostics and frame execution diagnostics to confirm pass names and resource names remain readable.
- Re-run renderer/RHI boundary checks after the chosen validation scripts are updated.

## Non-Goals

- Do not introduce bindless rendering as part of framegraph denoising.
- Do not add compatibility headers for old graph paths.
- Do not move D3D12 implementation details out of RHI private folders.
- Do not expose pass authoring outside Renderer until there is a real plugin/module use case.
- Do not add async compute or multi-queue scheduling until the graph plan, resource state tracker, and diagnostics are clearer.
- Do not clone NVRHI or Donut wholesale. Use their naming and ownership shape as a compass while keeping Sparkle's FrameGraph scheduler and existing module boundaries.
- Do not rename `FrameGraph` to `RenderGraph`; keep `FrameGraph` as Sparkle's graph/system term and remove stray `RenderGraph` names from Sparkle-owned graph types as part of the planned cleanup.

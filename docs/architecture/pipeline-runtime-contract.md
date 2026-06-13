# Pipeline Runtime Contract

Status: Stage 17 implementation contract
Date: 2026-06-12
Last synchronized: 2026-06-13

## Purpose

This document explains Sparkle's current renderer pipeline runtime and the target direction for explicit PSO handling. This is one of the main cleanup areas because pass authoring, cooked shader packages, binding layouts, and backend PSO creation currently require too much scattered knowledge.

Primary code references:

- [PipelineStateManager.h](../../Engine/Renderer/Private/Pipeline/PipelineStateManager.h)
- [RenderPassDefinitionRuntime.h](../../Engine/Renderer/Private/Pipeline/RenderPassDefinitionRuntime.h)
- [RenderPassDefinition.h](../../Engine/Renderer/Private/Passes/RenderPassDefinition.h)
- [RenderPassShaderRuntime.h](../../Engine/Renderer/Private/Pipeline/RenderPassShaderRuntime.h)
- [PipelineRuntimeLibrary.h](../../Engine/Renderer/Private/PipelineRuntime/PipelineRuntimeLibrary.h)
- [PipelineRuntimeKey.h](../../Engine/Renderer/Private/PipelineRuntime/PipelineRuntimeKey.h)
- [PassBinder.cpp](../../Engine/Renderer/Private/Pipeline/PassBinder.cpp)
- [RhiPipelineStateDesc.h](../../Engine/RHI/Public/Pipeline/RhiPipelineStateDesc.h)
- [D3D12/Pipeline](../../Engine/RHI/Private/D3D12/Pipeline)
- [Vulkan/Pipeline](../../Engine/RHI/Private/Vulkan/Pipeline)
- [Target folder architecture](after/repository-target-folder-architecture.md)

Reference basis:

- Donut separates render passes, shaders, and NVRHI-backed rendering infrastructure visibly in the repo: https://github.com/NVIDIA-RTX/Donut
- Falcor makes render passes/render graphs first-class workflow concepts: https://github.com/NVIDIAGameWorks/Falcor/blob/master/docs/getting-started.md
- Diligent Engine render state cache/packager samples show PSO/package work as explicit artifacts: https://github.com/DiligentGraphics/DiligentSamples
- arc42 crosscutting concept and runtime-view guidance: https://arc42.org/overview
- Repository threading readiness: [after/repository-threading-readiness.md](after/repository-threading-readiness.md)

## Contract Summary

Renderer pipeline runtime owns:

- Mapping pass definitions to shader packages.
- Loading cooked shader packages.
- Building pass binding layout definitions.
- Validating package features against RHI capabilities.
- Creating RHI binding layouts.
- Creating normalized RHI graphics/compute pipeline descriptors.
- Requesting RHI pipeline state objects.
- Owning runtime cache invalidation when cooked shaders reload.

RHI owns:

- The normalized pipeline descriptor types.
- Backend-neutral binding layout and pipeline objects.
- D3D12/Vulkan native root signatures, pipeline layouts, shader modules, and PSO objects.

## Current Runtime Flow

```mermaid
flowchart TD
    Request[Pass requests runtime]
    Manager[PipelineStateManager]
    Definition[RenderPassDefinition]
    DefinitionRuntime[RenderPassDefinitionRuntime]
    Runtime[RenderPassShaderRuntime]
    Library[PipelineRuntimeLibrary]
    Key[PipelineRuntimeKey]
    Layout[Build PassParameterLayout]
    Package[Load CookedShaderPackage]
    Caps[Validate capabilities]
    Binding[Create RHI BindingLayout]
    Desc[Build RHI PipelineStateDesc]
    PSO[Create RHI PipelineState]
    Backend[D3D12/Vulkan native PSO]

    Request --> Manager
    Manager --> Definition
    Definition --> DefinitionRuntime
    DefinitionRuntime --> Runtime
    Runtime --> Layout
    Runtime --> Library
    Library --> Package
    Library --> Caps
    Library --> Binding
    Library --> Key
    Runtime --> Desc
    Desc --> Library
    Library --> PSO
    PSO --> Backend
```

This path is functional and emits explicit pipeline runtime keys during PSO creation. Stage 17 removed the ordinary-pass dependency on `std::type_index` and `RenderPassPipelineTraits`.

## Current Objects

| Object | Current responsibility | Current issue |
| --- | --- | --- |
| `PipelineStateManager` | Lazy runtime cache keyed by pass definition name; owns `CookedShaderPackageCache`; handles reload by replacing the package cache and clearing pass-name runtimes. | Final smoke evidence still needs to capture PSO key logs in Stage 20. |
| `RenderPassDefinitionRuntime` | Converts immutable pass definitions into runtime storage, graphics/compute descriptors, and shader runtime calls. | It must remain generic and avoid pass-specific policy. |
| `RenderPassShaderRuntime` | Validates pipeline kind/stages, builds binding layout metadata, and delegates package/layout/PSO work to `PipelineRuntimeLibrary`. | It should keep shrinking toward reusable runtime stages. |
| `PipelineRuntimeLibrary` | Loads cooked packages, validates package/backend capabilities, creates binding layouts, builds printable pipeline keys, and creates RHI PSOs from normalized descriptors. | It must stay a runtime contract owner, not a place for pass-specific policy. |
| `PipelineRuntimeKey` | Captures pass, package, binding layout, backend, shader format, package generation/hash, pipeline kind, stages, features, and graphics render state. | It does not yet own final cache lookup because Stage 17 still feeds through pass traits. |
| `PassBinder` | Binds compiled reflection/layout entries to pass parameter data and overrides. | Critical path for runtime errors; diagnostics should remain precise. |
| `RhiPipelineStateDesc` | Backend-neutral graphics/compute pipeline descriptors. | Needs to become part of an explicit key/diagnostic model. |
| D3D12/Vulkan pipeline services | Native root signature/layout/shader module/PSO creation. | Should consume normalized descs without renderer pass policy. |

## Disposition Decisions

| Current object/body | Disposition | Target decision |
| --- | --- | --- |
| `PipelineStateManager` | Improve and extract | Stage 17 replaced type-index entry identity with pass-definition names; Stage 20 captures validation evidence. |
| `RenderPassDefinitionRuntime` | Keep and refine | Preserve generic definition-to-runtime conversion; do not add per-pass branches. |
| `RenderPassShaderRuntime` | Improve and extract | Preserve pass compatibility while package loading, capability validation, binding layout creation, explicit key formatting, and PSO creation live in `PipelineRuntimeLibrary`. |
| `PassBinder` | Keep and refine | Preserve as the binding error hotspot; improve diagnostics and reflection mismatch reporting. |
| `RhiPipelineStateDesc` | Keep and refine | Preserve normalized backend-neutral descriptors and make them part of printable PSO identity. |
| Shader package declaration duplication | Replace or redesign | Stage 17 shares package/layout IDs through `RendererShaderPackages`; Stage 17A removes remaining handwritten shader registration boilerplate with manifest/generated records. |

## Folder Target

| Folder | Target role | Rejected use |
| --- | --- | --- |
| `Engine/Renderer/Private/PipelineRuntime` | Explicit package/layout/backend/PSO identity, runtime cache, reload invalidation, printable diagnostics. | Opaque catch-all manager for pass-specific policy. |
| `Engine/Renderer/Private/PassCatalog` | Source of pass package metadata consumed by runtime and ShaderCompiler. | Duplicate declarations split from pass definitions. |
| `Engine/Contracts/Shader` | Shared manifest/reflection/binding schema. | Renderer execution or backend compiler internals. |
| `Engine/RHI/Public/Pipeline` | Normalized backend-neutral pipeline descriptor contracts. | Pass-specific render intent or material policy. |
| `Engine/RHI/Private/D3D12/Pipeline` and `Engine/RHI/Private/Vulkan/Pipeline` | Native PSO/root-signature/pipeline-layout implementation. | Renderer pass traits, shader package selection, or GameFramework data. |

## Pipeline Runtime Complexity Budget

| Runtime complexity | Earns its right when | Remove or redesign when |
| --- | --- | --- |
| `PipelineRuntimeLibrary` | It centralizes explicit package/layout/backend/PSO identity and reload invalidation. | It becomes another opaque manager hiding pass-specific policy. |
| `PsoKey` / pipeline key | It is printable, deterministic, and explains backend object creation. | It omits enough state that failures still require code spelunking. |
| Package cache | It avoids duplicate loads and reports package generation/hash. | It hides stale packages or reload behavior. |
| Binding validation | It names pass, binding, package, layout, backend, and source of mismatch. | It collapses failures into generic missing-resource messages. |

## Target Runtime Flow

```mermaid
flowchart TD
    Definition[Renderer pass definition]
    Key[Explicit PSO/pass runtime key]
    Library[PipelineRuntimeLibrary]
    Package[Cooked shader package cache]
    Reflection[Reflection and binding validation]
    Desc[Normalized RHI pipeline desc]
    RHI[RHI pipeline service]
    Backend[D3D12/Vulkan PSO]
    Diagnostics[Printable runtime diagnostics]

    Definition --> Key
    Key --> Library
    Library --> Package
    Package --> Reflection
    Reflection --> Desc
    Desc --> RHI
    RHI --> Backend
    Library --> Diagnostics
```

`PipelineRuntimeLibrary` is fed by declarative pass definitions after Stage 17. Ordinary passes no longer need central runtime traits.

## PSO Key Definition

A target PSO key should include enough identity to explain exactly what backend object is being requested.

Minimum fields:

- Pass name.
- Package id.
- Binding layout id.
- Pipeline kind: graphics or compute.
- Backend shader binary format: DXIL or SPIR-V.
- Shader package generation.
- Render target formats and count for graphics.
- Depth/stencil format and depth state for graphics.
- Vertex layout kind for graphics.
- Cull/fill/topology/raster state for graphics.
- Feature requirements such as inline ray query or acceleration structure usage.

Not required in the key:

- Native D3D12/Vulkan handles.
- Renderer scene data.
- Frame graph resource handles unless the resource changes pipeline shape.

## Threading Readiness Contract

Pipeline runtime must be safe to warm, inspect, invalidate, or rebuild in future background jobs without hidden renderer state.

| Runtime part | Threading-ready rule |
| --- | --- |
| PSO key | Immutable, printable, deterministic, and independent of C++ pointer/type identity. |
| Package cache | Keyed by package id, backend format, options, and generation; reload publishes a new generation instead of mutating readers in place. |
| Binding layout | Built from package reflection and pass catalog data; errors name package/layout/pass/backend. |
| Runtime cache | Mutated by one owner phase or protected by explicit generation swap; readers consume stable handles for the frame. |
| Warmup/validation | Future warmup jobs use package manifests and RHI capability reports, not live frame/pass objects. |

Forbidden shortcuts:

- Do not reintroduce C++ type identity as the runtime key if it hides package/layout/backend state.
- Do not let background package validation reach into Renderer private pass instances.
- Do not mutate a cache entry while a frame may still consume it; publish by generation or frame-safe swap.

## Shader Package Flow

```mermaid
sequenceDiagram
    participant Tool as ShaderCompiler
    participant Registry as Shader registrations
    participant Package as Cooked package
    participant Cache as CookedShaderPackageCache
    participant Runtime as RenderPassShaderRuntime
    participant RHI as RHI pipeline service

    Registry->>Tool: Package/stage/reflection definitions
    Tool->>Package: Emit bytecode and reflection
    Runtime->>Cache: LoadPackage(package id, layout id, format)
    Cache-->>Runtime: LoadedShaderPackage
    Runtime->>Runtime: Validate reflection and capabilities
    Runtime->>RHI: CreateBindingLayout and CreatePipelineState
```

Current debt: package registration for renderer passes lives in RHI private files. This makes pass authoring feel lower-level than it should.

## Binding Contract

Binding has three layers:

| Layer | Owner | Current files | Rule |
| --- | --- | --- | --- |
| Pass parameter metadata | Renderer pass/shader-parameter authoring | [Renderer/Public/ShaderParameters](../../Engine/Renderer/Public/ShaderParameters) | Describes what the pass expects by name/type/visibility/usage. |
| Compiled binding layout | Renderer pipeline runtime plus RHI binding layout | [RenderPassShaderRuntime.h](../../Engine/Renderer/Private/Pipeline/RenderPassShaderRuntime.h), [PassParameterLayout.h](../../Engine/RHI/Public/ShaderParameters/PassParameterLayout.h) | Must be validated against cooked reflection. |
| Native binding layout | Backend pipeline/descriptor implementation | [D3D12/Pipeline](../../Engine/RHI/Private/D3D12/Pipeline), [Vulkan/Pipeline](../../Engine/RHI/Private/Vulkan/Pipeline) | Converts normalized layout to root signature/pipeline layout/descriptors. |

Runtime binding errors should include:

- Pass name.
- Binding name.
- Package id.
- Binding layout id.
- Expected binding type.
- Backend.
- Whether the source was parameter data or override.

## Reload And Invalidation

Current behavior:

- `PipelineStateManager::ReloadCookedShaders` reloads packages and clears lazy pipeline runtimes.
- Runtime is recreated on the next `GetPassRuntime<TPass>()`.

Target behavior:

- Reload result reports changed packages and generation.
- Pipeline runtime library invalidates by explicit key/package dependency.
- Logs show which pass runtime/PSO keys were invalidated.

## Backend PSO Contract

Backends must consume `GraphicsPipelineStateDesc` and `ComputePipelineStateDesc` without renderer feature policy.

D3D12 owns:

- Root signatures.
- D3D12 pipeline state objects.
- DXIL shader bytecode usage.
- D3D12 format/state conversion.

Vulkan owns:

- Pipeline layouts.
- Shader modules.
- Vulkan graphics/compute pipeline objects.
- SPIR-V usage.
- Vulkan format/state/cull/depth/viewport mapping.

Backend pipeline code may log debug names and native errors. It should not know that a PSO is for GBuffer, sky, DLSS, or shadows except through debug names.

## Current Gaps

| Gap | Evidence | Owning stage |
| --- | --- | --- |
| Final smoke artifacts do not yet archive PSO key logs. | Stage 16/17 builds passed and runtime logs emit keys; Stage 20 must capture them in validation evidence. | Stage 20 |
| Shader registration metadata is not yet manifest/generated. | [RendererShaderPackages.h](../../Engine/Renderer/ShaderRegistrations/RendererShaderPackages.h) shares identity, but shader names, paths, entry points, and stages are still repeated in registration classes. | Stage 17A |
| Pipeline diagnostics are not yet final acceptance artifacts. | Logs exist, but final smoke reports need package/key/backend evidence. | Stage 16, Stage 20 |

## Change Rules

Before changing pipeline runtime:

1. State whether the change affects package identity, binding layout, reflection validation, PSO descriptor, backend pipeline creation, cache invalidation, or diagnostics.
2. Print or record enough information to reconstruct the runtime/PSO key.
3. Keep renderer pass policy above RHI.
4. Keep native PSO creation inside D3D12/Vulkan backend pipeline services.
5. Validate both backends when descriptor semantics change.

## Acceptance Evidence

This contract is accepted when:

- Runtime logs print explicit pass/package/binding/backend/PSO key information.
- A proof pass does not require central RHI or backend edits.
- D3D12/Vulkan PSO creation consumes normalized descriptors and reports failures with enough context.
- Shader reload invalidates by explicit package/runtime identity.
- Final smoke evidence includes pipeline runtime diagnostics for lit and debug/normal view modes.

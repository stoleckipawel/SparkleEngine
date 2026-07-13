# Frame-Graph Resource Reference Design

Status: implemented for ordinary external textures and uploaded scene buffers. Bindless material tables and per-draw material/geometry descriptors remain explicit exceptions.

## Decision summary

Sparkle high-level rendering code uses graph-owned resource and declared view references for ordinary frame resources.

The target model is:

```text
Owning system resource handle
    -> FrameGraphTextureRef / FrameGraphBufferRef
    -> FrameGraphTextureSRVRef / FrameGraphTextureUAVRef
    -> shader pass parameter
    -> descriptor materialized internally by the frame graph
```

An RHI descriptor is an execution detail. It must not appear in `RenderSceneData`, `FrameContext` public data, authored pass setup, or feature-specific binding helpers.

## Why this is needed

Before this migration Sparkle had two parallel resource paths:

1. `FrameGraphTextureHandle` and `FrameGraphBufferHandle` are registered, declared by passes, transitioned, and resolved by the frame graph.
2. `ShaderTexture2DSRV`, `ShaderBufferSRV`, and `ShaderTexture2DUAV` carried `RhiDescriptorTableBinding` or `RhiGpuDescriptorHandle` directly and were marked `FrameGraphTracked = false`.

The second path was used by the sky, uploaded lighting buffers, ray-tracing hit data, mesh instances, and skinning data. Those ordinary resources are now graph tracked. Material descriptor tables and per-draw material/skin-influence bindings remain explicit dynamic exceptions.

The frame graph already contains most of the mechanism needed for the first path:

- resource handles and metadata;
- transient, imported, and externally persistent ownership;
- resource-state tracking and barrier planning;
- SRV/UAV/RTV/DSV creation for tracked resources;
- parameter fields backed by graph handles;
- persistent external texture, buffer, and acceleration-structure slots.

The migration was implemented as generalized texture/buffer external binding; no sky-specific binding wrapper was added.

## Unreal RDG reference model

Unreal's RDG separates resources from views and limits both to graph lifetime. External resources are registered into the graph, then SRV/UAV references are created through the graph builder. The underlying RHI resource is accessible only during declared pass execution. Epic's [RDG programming guide](https://dev.epicgames.com/documentation/en-us/unreal-engine/render-dependency-graph-in-unreal-engine) documents these lifetimes, external registration, and view creation. The official APIs describe [`RegisterExternalTexture`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RenderCore/RegisterExternalTexture) and graph-tracked [`CreateUAV`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RenderCore/FRDGBuilder/CreateUAV).

Sparkle differs in one important respect: its graph structure is built once and set up/compiled repeatedly, while Unreal commonly builds an immediate graph for each frame. Sparkle references can therefore remain stable for the lifetime of one `FrameGraph`, but they must never be stored as general renderer-scene or asset identities.

## Scope boundary

Adopting RDG-style references does not require converting Sparkle to Unreal's immediate per-frame graph in the same change. The recommended first step preserves Sparkle's persistent graph structure and improves its resource/view contract:

- graph construction reserves transient and external resource identities;
- per-frame assembly binds changing external backing resources;
- setup declares resource/view use;
- compilation plans dependencies and transitions;
- execution materializes views and resolves descriptors internally.

Changing graph construction lifetime, pass culling strategy, or command scheduling should be evaluated independently after ordinary resources no longer bypass tracking. Combining those changes would make backend and lifetime regressions much harder to isolate.

## Resource and view types

### Resource references

Rename or evolve the existing value handles into graph-owned references:

```cpp
struct FrameGraphTextureRef;
struct FrameGraphBufferRef;
struct FrameGraphAccelerationStructureRef;
```

A reference identifies a resource registered with one `FrameGraph`. It contains no native resource pointer and no descriptor. In debug builds it should also carry a graph identity or generation so use with another graph fails immediately.

Keeping these as small value tokens is appropriate for Sparkle; they do not need to copy Unreal's pointer implementation. `Ref` communicates graph ownership and lifetime, while the internal representation can remain an indexed handle.

### View references

Add distinct graph-owned view references:

```cpp
struct FrameGraphTextureSRVRef;
struct FrameGraphTextureUAVRef;
struct FrameGraphBufferSRVRef;
struct FrameGraphBufferUAVRef;
```

Each view reference identifies:

- its source resource reference;
- view kind;
- optional format override;
- mip range;
- array/depth slice range.

The initial implementation may support only default whole-resource views, but the types and cache key should not prevent subresource views.

`FrameGraphBuilder::CreateSRV()` and `CreateUAV()` return view references. They do not return an RHI descriptor wrapper:

```cpp
FrameGraphTextureSRVRef CreateSRV(
    FrameGraphTextureRef texture,
    const FrameGraphTextureSRVDesc& desc = {});

FrameGraphTextureUAVRef CreateUAV(
    FrameGraphTextureRef texture,
    const FrameGraphTextureUAVDesc& desc = {});
```

Pass parameter fields store these view references. During graph compilation, dependencies are attributed to the underlying resource. During execution, the graph resolves or creates the backend descriptor for the requested view.

## Persistent resources versus graph resources

Three identities must remain separate:

| Identity | Lifetime | Example owner |
|---|---|---|
| Asset identity | project/level | `Assets::CookedTextureReference` |
| Renderer resource identity | loaded GPU resource lifetime | `TextureManager` |
| Frame-graph reference | one `FrameGraph` lifetime | `FrameAssemblyResourceLayout` |

`RenderSkyData` stores the renderer resource identity. It must not store a frame-graph reference because scene data is not owned by one graph. It also must not store a descriptor binding because descriptors are graph/RHI execution state.

## External resource API

Sparkle's persistent graph needs generalized external slots that can be rebound without exposing native resources to passes:

```cpp
FrameGraphTextureRef ReserveExternalTexture(std::string_view debugName);
FrameGraphBufferRef ReserveExternalBuffer(std::string_view debugName);

void BindExternalTexture(
    FrameGraphTextureRef texture,
    RendererTextureHandle resource,
    ResourceState currentState);

void BindExternalBuffer(
    FrameGraphBufferRef buffer,
    RendererBufferHandle resource,
    ResourceState currentState);
```

The renderer resource handles above are conceptual names. Their concrete owner may be `TextureManager`, a generalized GPU resource owner, or a frame upload owner. The high-level API must not require callers to provide descriptor tables or GPU descriptor addresses.

Binding must update full resource metadata, including dimensions, format, mip count, array size, stride, and usage capabilities. A reserved slot cannot assume that two level-selected textures have the same descriptor.

When the backing resource or its description changes, the frame graph must:

1. release or retire views created for the previous backing resource;
2. replace resource metadata atomically;
3. update the tracked incoming state;
4. create only the views required by compiled pass declarations;
5. preserve the external owner's resource lifetime through GPU completion.

The current `BindPersistentTexture()`/`BindPersistentBuffer()` path replaces resolved access wholesale. Because resolved access also contains graph-created view handles, rebinding and view release must be audited together; a binding operation must never orphan descriptors from the previous resource.

## Texture abstraction change

The existing RHI `Texture` abstraction primarily exposes `WriteShaderResourceView()` and a string-based `TextureRuntimeInfo::FormatName`. That shape encourages prebuilt descriptors and is insufficient for graph registration.

The generalized texture resource contract should expose backend-neutral resource identity and structured metadata to the frame graph. At minimum:

- width, height, depth/array size;
- resource dimension;
- `PixelFormat`, not only a format name string;
- mip count;
- supported usage flags;
- an opaque owning resource handle resolvable only inside the RHI/frame-graph boundary.

Texture loading remains responsible for creating and owning the GPU resource. The frame graph becomes responsible for creating views required by passes. Existing texture-manager descriptor caches should be retained only for systems that intentionally remain outside graph tracking, such as a genuinely bindless global table.

## Pass authoring target

Tracked texture parameters should look like this:

```cpp
struct SkyPassParameters
{
    FrameGraphTextureUAVRef SceneColor;
    FrameGraphTextureSRVRef SceneDepth;
    FrameGraphTextureSRVRef SkyTexture;
    ShaderUniform<SkyUniformData> Sky;
};

parameters->SceneColor = builder.CreateUAV(sceneColor);
parameters->SceneDepth = builder.CreateSRV(sceneDepth);
parameters->SkyTexture = builder.CreateSRV(skyTexture);
```

The existing typed shader-parameter layer may wrap these reference types if it still adds compile-time texture dimension/value checks. It must not wrap an RHI descriptor or hide several unrelated resources behind a feature binding object.

## Bindless exception

Not every descriptor table should be forced into individual graph references. A global bindless material texture table has dynamic shader indexing and potentially thousands of entries; tracking each entry as a normal pass parameter may be inappropriate.

That exception must be explicit:

- the table owner owns descriptor lifetime;
- resources have a documented external-state contract;
- the graph sees one declared bindless-table dependency or conservative boundary state;
- ordinary single textures and buffers do not use the exception for convenience.

Sky, light buffers, mesh-instance buffers, and ray-tracing hit buffers are ordinary known resources and should be graph tracked.

## Implementation record

### Stage 1: References and validation — complete

- introduce `FrameGraph*Ref` names, initially backed by the current handles;
- add graph identity/generation validation;
- keep compatibility aliases only while migrating callers.

### Stage 2: Declared graph views — complete for whole-resource views

- add SRV/UAV reference descriptors and caches;
- make `CreateSRV()`/`CreateUAV()` return graph view references;
- resolve descriptors only inside graph execution;
- add subresource-ready metadata without requiring every view feature immediately.

### Stage 3: External binding — complete

- add structured renderer resource handles and metadata;
- make external rebinding release/retire old views correctly;
- add descriptor-lifetime and repeated-rebind tests on D3D12 and Vulkan.

### Stage 4: Sky proving path — complete

- reserve one external sky texture reference in frame assembly;
- bind the selected sky texture before graph setup;
- migrate every visible-sky and indirect-lighting pass to a graph-created SRV;
- remove the sky's `ShaderTexture2DSRV` and texture-manager descriptor lookup.

### Stage 5: Known buffers — complete

- migrate light, mesh-instance, skinning, and ray-tracing hit buffers;
- move view creation out of their frame-data owners;
- replaced `ShaderBufferSRV` parameters with graph buffer/view references.

### Stage 6: Remove the bypass — complete

- classify remaining descriptor-table uses as explicit bindless/external exceptions;
- delete untracked single-resource parameter fields when they have no legitimate callers;
- keep raw RHI handles below the frame-graph execution boundary.

## Acceptance criteria

- High-level pass code does not mention `RhiDescriptorTableBinding`, `RhiGpuDescriptorHandle`, or `RhiResourceViewHandle`.
- `RenderSceneData` and `FrameContext` do not store descriptor bindings.
- Every ordinary texture/buffer pass parameter resolves through a graph resource/view reference.
- External resource changes invalidate old graph views without descriptor leaks.
- Graph compilation sees the sky and uploaded scene buffers as declared dependencies and plans their transitions.
- D3D12 and Vulkan produce identical resource-state behavior for the migrated passes.
- Debug validation catches cross-graph references, invalid views, missing external bindings, and unsupported UAV usage before dispatch.
- Bindless exceptions are few, named, and documented.

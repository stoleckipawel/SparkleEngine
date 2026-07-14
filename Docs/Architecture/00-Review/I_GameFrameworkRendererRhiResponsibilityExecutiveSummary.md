# I. GameFramework / Renderer / RHI Responsibility Executive Summary

Status: current-state architecture audit and implementation decision
Date: 2026-07-14
Scope: ownership, lifetime, data flow, and future development boundaries across GameFramework, Renderer, and RHI

## Executive Verdict

Sparkle's top-level dependency direction is already correct:

```text
GameFramework  --->  Renderer  --->  RHI  --->  D3D12 / Vulkan
       CPU scene        GPU policy       backend execution
```

GameFramework does not depend on Renderer or RHI. Renderer consumes GameFramework privately and RHI publicly. RHI does not depend on Renderer. The architecture boundary check passes. The Renderer frame graph decides pass dependencies, transient lifetimes, aliasing, resource states, and barriers; RHI performs the backend commands. This is the right split and should not be replaced.

The foundations are nevertheless not yet solid enough for unbounded feature growth. The three highest-risk gaps identified by the audit were:

1. RHI GPU lifetime was only partially unified. Priority 0A now makes descriptor allocations, tables, and views retire through the backend's waited frame-slot boundary and protects recycled table/view records with generations. Priority 0B now routes sampled textures through that same resource/view lifetime model and fence-retired upload storage. Runtime material-rebuild and texture-upload validation remains before final acceptance.
2. Material textures use a second resource model outside `RhiOwnedResourceHandle` and `RhiResourceViewHandle`. That path records hidden uploads, keeps upload allocations with textures, and performs a queue-wide idle wait for every Vulkan texture upload.
3. The GameFramework-to-Renderer snapshot has no stable per-instance identity or revision contract. It copies static data each frame, exports raw `Mesh*`, keys GPU meshes by pointer, and keys temporal data by vector position or shared skeleton asset.

These are correctness and scalability issues, not presentation issues. They should be fixed before adding another large renderer feature, another descriptor model, streaming, or asynchronous compute.

The recommended direction is deliberately narrow:

- GameFramework owns durable CPU scene and asset identity, then publishes stable IDs, revisions, and immutable change records.
- Renderer owns the imported render scene, GPU feature policy, frame graph, pass scheduling, persistent feature histories, and renderer-facing material/mesh tables.
- RHI owns every native GPU object, upload implementation, descriptor allocation, queue, fence, synchronization primitive, and deferred destruction rule.

Do not add a fourth abstraction layer. Strengthen and simplify the three layers that already exist.

## Target Responsibility Contract

| Layer | Must own | May publish upward/downward | Must not own |
| --- | --- | --- | --- |
| GameFramework | Levels, scene components, transforms, animation state, lights, cameras, material descriptions, cooked asset identity, immutable CPU asset payloads, stable scene-instance IDs, scene and asset revisions. | A renderer-neutral, lifetime-safe scene snapshot/change set containing IDs, revisions, authored values, and immutable payload references. | RHI handles, descriptor indices, GPU addresses, resource states, barriers, frame-graph handles, render passes, temporal render history. |
| Renderer | Imported render-scene state, GPU mesh/material/texture indexing, culling/batching, shader payloads, frame graph, render modes, feature policy, pass order, persistent feature history, acceleration-structure policy, provider placement, viewport products. | Backend-neutral resource/view/binding descriptions and command intent through public RHI contracts. | Native D3D12/Vulkan objects, physical descriptor allocation, fence completion rules, staging allocation, queue implementation, authored game-scene ownership. |
| RHI | Devices, native resources and views, memory allocation, upload staging, descriptors, pipelines, command lists, queues, fences, swapchain execution, deferred destruction, backend capabilities, bounded native interop. | Opaque handles, capabilities, allocation facts, submission/retirement tokens, and explicit command operations. | Scene semantics, material semantics, render-feature selection, pass ordering, temporal-history policy, frame-graph dependency analysis. |

Application/Editor remains the composition root: it constructs these modules, owns the window/runtime loop, and wires presentation. It should not become a fourth home for scene synchronization or GPU lifetime policy.

## Current-State Scorecard

| Area | State | Finding |
| --- | --- | --- |
| Module dependency direction | Strong | GameFramework links only Core/Platform; Renderer privately consumes GameFramework and publicly consumes RHI; RHI does not consume Renderer. |
| Native API containment | Strong | Ordinary Renderer code contains no D3D12/Vulkan identifiers; native escape is confined to RHI and dedicated provider bridges. |
| Frame graph versus RHI | Strong | Renderer compiles dependencies, transients, aliasing, final states, and barriers; RHI command lists execute transitions, UAV barriers, aliases, draws, and dispatches. |
| Vendor provider boundary | Good | External image features are isolated behind Renderer provider code and bounded RHI interop. The provider remains a client of the engine rather than defining it. |
| Scene handoff | High risk | The snapshot copies static collections, includes raw mesh pointers, and lacks scene-instance/revision semantics. The Renderer copy is field-for-field rather than a real import boundary. |
| GPU object model | High risk | Generic owned resources/views coexist with a polymorphic `Texture` hierarchy that has separate creation, view, upload, and destruction rules. |
| In-flight lifetime | Priorities 0A and 0B implemented; runtime stress validation pending | Resources, descriptor allocations, descriptor tables, views, and texture staging now have backend-owned deferred reuse. The legacy texture hierarchy and its bespoke destruction paths have been deleted. |
| Feature ownership | Needs work | `FramePipeline` owns exposure, reference-lighting, direct-reservoir, and indirect-reservoir resources, validity flags, resets, and feature keys. |
| Material bindings | Needs work | Per-material binding sets and a global material table are both valid policies, but ownership is exposed as raw pointers and repeated availability checks across passes. |
| Queue model | Intentionally limited | The public submission contract exposes only graphics command lists. This is honest today but must be extended before real asynchronous copy/compute scheduling. |

The passing boundary script is evidence that compile-time direction is healthy, not evidence that runtime ownership is complete. The current check protects Renderer/RHI source dependencies; it does not prove scene identity, descriptor lifetime, upload behavior, or feature ownership.

## What The NVIDIA And AMD References Actually Support

There is no single "NVIDIA architecture" or "AMD architecture" to copy. The reviewed repositories serve different purposes. The useful result is the set of principles on which they agree, plus the places where Sparkle should intentionally choose one model.

### NVIDIA: low-level explicitness and lifetime must be intentional

[NRI](https://github.com/NVIDIA-RTX/NRI) describes a low-level interface whose goals include explicitness and low overhead, and whose non-goals include hidden management and automatic barriers; it says barriers are better handled by a higher-level abstraction. That supports Sparkle's current choice: Renderer frame graph plans barriers, while RHI exposes explicit execution commands. Moving frame-graph policy into RHI would be a regression.

[NVRHI's programming guide](https://github.com/NVIDIA-RTX/NVRHI/blob/main/doc/ProgrammingGuide.md) chooses more management than NRI, but is very clear about the contract it provides:

- command lists/device references keep resources alive until GPU use has completed;
- destruction is deferred;
- binding sets can retain the resources they reference;
- bindless descriptor tables require explicit application synchronization;
- upload buffers are tracked as part of texture/buffer writes;
- graphics, compute, and copy queues plus inter-queue waits are explicit;
- persistent resource handles normally live in the render pass or feature that owns them.

Sparkle does not need NVRHI's reference-counted API or automatic barriers. It does need an equally complete answer for every GPU-visible allocation: who owns it, which submission last used it, and when its storage or descriptor may be reused.

### NVIDIA Donut: scene maintenance and feature resources are not one central pipeline

[Donut's repository structure](https://github.com/NVIDIA-RTX/Donut#structure) separates core utilities, scene/engine maintenance, rendering passes, and application hosting. Its [scene refresh path](https://github.com/NVIDIA-RTX/Donut/blob/main/src/engine/Scene.cpp) tracks structure and transform changes, updates dirty materials, and resizes or updates GPU arrays only when required. Its render pass classes own their device handles, shaders, layouts, buffers, pipelines, and binding caches; for example, [ForwardShadingPass](https://github.com/NVIDIA-RTX/Donut/blob/main/include/donut/render/ForwardShadingPass.h).

Sparkle should borrow two principles, not Donut's exact class hierarchy:

- scene-to-renderer synchronization should be revision/change driven;
- persistent resources should normally be owned by the renderer feature that gives them meaning.

### AMD FidelityFX: the engine remains in control of scheduling and backend translation

The current [FidelityFX SDK backend interface](https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK/blob/main/Kits/FidelityFX/api/internal/ffx_interface.h) explicitly separates effect logic from backend callbacks for resource creation/registration, pipelines, scheduled jobs, and execution. The [FFX API guide](https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK/blob/main/Kits/FidelityFX/docs/getting-started/ffx-api.md#dispatch) states that GPU dispatches encode into a command list/buffer supplied through the dispatch description.

That supports Sparkle's provider direction:

- Renderer decides where a feature belongs in the frame and supplies renderer-owned inputs;
- a narrow provider bridge translates resources and invokes the SDK;
- RHI owns the native device/resource/command representation;
- the SDK must not become a second scene model, scheduler, resource lifetime system, or public renderer API.

### AMD Cauldron: useful historical mechanics, not a modern blueprint

[Cauldron](https://github.com/GPUOpen-LibrariesAndSDKs/Cauldron) demonstrates explicit upload heaps, command-list rings, resource-view heaps, static/dynamic buffer suballocation, and parallel D3D12/Vulkan implementations. Those mechanics reinforce the need for bounded uploads and explicit resource ownership. Its latest release is from 2022, and its feature organization is intentionally sample-framework oriented, so it should be treated as historical implementation evidence rather than the target module design for Sparkle.

### Reference conclusion

The practical cross-reference principles are:

1. Keep low-level GPU ownership explicit.
2. Put barrier and scheduling policy above the low-level interface.
3. Never recycle GPU-visible storage until the relevant submission has completed.
4. Make uploads part of an explicit command/submission lifetime.
5. Let features own their persistent GPU state.
6. Synchronize scene changes through stable identity and dirty/revision information.
7. Keep native SDK/API translation in narrow backend/provider bridges.

Sparkle already satisfies principles 2 and 7. The immediate work is principles 3, 4, 5, and 6.

## Legacy-Removal Completion Gate

Every priority is a replacement, not an addition. A priority is not complete while its superseded path, compatibility wrapper, duplicate state, forwarding facade, or obsolete ownership branch remains in production code. Temporary migration code is allowed only inside the active implementation batch and must have an explicit deletion step in that same batch.

| Priority | Legacy surface that must be gone before completion |
| --- | --- |
| 0A | Immediate descriptor/table reuse paths, recycled unversioned table/view records, and the shader-resource-only allocate/release convenience API. |
| 0B | `Texture`, `D3D12Texture`, `VulkanTexture`, texture factories, `RhiResourceService::CreateTexture`, object-owned descriptor writes, private upload submissions, and special frame-graph texture overloads. |
| 0C | Pointer-keyed mesh identity, vector-position temporal identity, skeleton-asset-only pose identity, raw mutable asset pointers in snapshots, the field-for-field Renderer snapshot, per-frame deep material comparison, and repeated texture-path traversal. |
| 1A | Feature-specific history resources, validity fields, state keys, reset methods, and reset fan-out in `FramePipeline`; no forwarding methods remain after feature ownership moves. |
| 1B | Raw binding-set pointers in frame data, repeated pass-local readiness branches, pointer/native-address semantic hashes, and duplicate ownership facts outside `MaterialCacheManager`. |
| 2A | Ad hoc/private upload submission and wait paths replaced by the first real multi-queue workload; no speculative queue facade or unused parallel submission API is retained. |
| 2B | Misplaced exports/includes and obsolete public helpers are deleted or relocated; no compatibility facade is added in front of them. |

Code search for each named legacy symbol is part of acceptance. If an old path must remain for an external consumer, that is a separately approved compatibility requirement, not an implicit exception.

## Priority 0A: Complete The RHI Lifetime Contract

### Implementation status: code complete, runtime stress validation pending

- [`D3D12ResourceService::ReleaseOwnedResource`](../../../Engine/RHI/Private/D3D12/Resources/D3D12ResourceService.cpp) stamps resource releases with a fence value and drains completed releases.
- [`VulkanResourceService::ReleaseOwnedResource`](../../../Engine/RHI/Private/Vulkan/Resources/VulkanResourceService.cpp) queues resource destruction against the backend retire fence.
- [`RenderBindingSet::Reset`](../../../Engine/RHI/Private/Bindings/RenderBindingSet.cpp) calls `ReleaseDescriptorTable` immediately.
- [`D3D12DescriptorService`](../../../Engine/RHI/Private/D3D12/Descriptors/D3D12DescriptorService.cpp) now logically invalidates descriptor tables and views at release, but delays `FreeContiguous`, individual descriptor frees, view descriptor frees, and record reuse until `BeginFrame` for the waited frame slot.
- [`VulkanDescriptorAllocator`](../../../Engine/RHI/Private/Vulkan/Descriptors/VulkanDescriptorAllocator.cpp) now delays descriptor-table record reuse to the same waited frame-slot boundary. Its encoded CPU descriptor handle carries the complete generational table handle, so stale writes cannot target a replacement table.
- [`VulkanDescriptorManager`](../../../Engine/RHI/Private/Vulkan/Descriptors/VulkanDescriptorManager.cpp) defers image-view/registered-descriptor destruction and logical view-record reuse through that boundary.
- `RhiDescriptorTableHandle` and `RhiResourceViewHandle` are packed index/generation values shared by both backends. Exhausted generations are not wrapped back into reuse.
- The shader-resource-only allocate/release convenience API was deleted from `RhiDescriptorService` and both backends. ImGui now uses the general `AllocateDescriptor` / `ReleaseDescriptor` contract.

Both backend RHI targets compile in `DevelopmentEditor`. The remaining acceptance action is a runtime scenario that rebuilds material bindings repeatedly with multiple frames in flight and confirms rendering remains correct without a flush.

### Before and after

| Before | After |
| --- | --- |
| Resource release is fence-deferred; descriptor-table release is immediate. | Every GPU-visible allocation has one backend-owned retirement rule tied to completed work. |
| Callers must implicitly know which release calls are safe while frames are in flight. | All ordinary release calls are safe; RHI delays physical reuse. |
| A recycled integer table handle can alias a new table. | Table records use a generation or equivalent stale-handle protection. |
| Material updates either risk reuse or require a broad flush. | Material tables can be replaced incrementally; old tables retire without stalling the device. |

### Implemented batch

1. Extend the existing descriptor retirement mechanism to descriptor tables and individually allocated shader-resource descriptors. Do not add a new lifetime manager; use the existing backend descriptor services.
2. Stamp retirement with the backend's next submission fence, or with a frame-slot reuse event that is already proven to occur after that slot's fence completes.
3. On D3D12, do not call `FreeContiguous` until retirement completes.
4. On Vulkan, do not recycle the logical table record or any backing descriptor set/pool storage until retirement completes.
5. Add stale-handle protection to recycled table/view records. A packed index/generation value is sufficient; no public object hierarchy is required.
6. Route legacy texture descriptors through this same rule as part of Priority 0B, then delete their bespoke destruction paths.

Items 1-5 are implemented. Item 6 is deliberately the Priority 0B deletion gate; Priority 0A must not acquire a compatibility branch for the old texture hierarchy.

### Acceptance bar

- Rebuilding materials every frame for several frames in flight never calls `Flush` and never reuses a descriptor range still referenced by submitted work.
- `ReleaseDescriptorTable`, `ReleaseResourceView`, and `ReleaseOwnedResource` all mean logical release now and physical reuse only when safe.
- D3D12 and Vulkan implement the same observable lifetime contract.
- The public API does not expose native fences merely to make Renderer manage RHI destruction.
- `rg "AllocateShaderResourceDescriptor|ReleaseShaderResourceDescriptor" Engine` returns no hits.

## Priority 0B: Replace The Second Texture Path With Generic RHI Resources, Views, And Uploads

### Implementation status: code and static deletion checks complete; build and runtime validation intentionally pending

The implementation is present in the working tree. In accordance with the priority execution rule, no engine build, backend compile, or runtime launch was performed for Priority 0B; those checks remain pending until explicitly requested.

- [`TextureManager`](../../../Engine/Renderer/Private/Textures/TextureManager.cpp) now owns non-polymorphic `RendererTexture` records containing generic owned-resource and resource-view handles plus CPU metadata.
- [`RhiUploadService`](../../../Engine/RHI/Public/Resources/RhiUploadService.h) now accepts explicit texture uploads recorded into the Renderer-selected graphics command list.
- D3D12 staging resources are retained by `D3D12UploadService` until the stamped submission fence completes; Vulkan staging resources enter the existing allocator retirement queue with the command context's next retire fence.
- Vulkan texture uploads use the active frame command buffer and contain no private command pool, private queue submission, or per-texture queue-idle wait. A canceled/non-recording frame command buffer is rejected and default loading retries on a later valid frame.
- Material binding sets and the global material texture table copy logical `RhiResourceViewHandle` records through `RhiDescriptorService`; they no longer ask a texture object to write native descriptors.
- The sky persistent import passes its existing generic resource and view to the frame graph. The frame graph tracks the view as borrowed, avoiding a duplicate SRV and avoiding ownership ambiguity.
- The legacy `Texture` base class, D3D12/Vulkan subclasses, factories, old constant-buffer-manager names, backend-specific texture upload/destruction paths, `RhiResourceService::CreateTexture`, and the legacy frame-graph overload have been deleted.

### Baseline evidence addressed by this priority

Before this implementation, [`RhiResourceService`](../../../Engine/RHI/Public/Resources/RhiResourceService.h) exposed both:

- `CreateTextureResource(...) -> RhiOwnedResourceHandle`; and
- `CreateTexture(...) -> std::unique_ptr<Texture>`.

The now-deleted polymorphic `Texture` exposed a native resource and wrote its own SRV into arbitrary CPU descriptor storage. Renderer [`TextureManager`](../../../Engine/Renderer/Private/Textures/TextureManager.cpp) owned these objects, material code asked them to write descriptors, and the frame graph borrowed their native handles.

The backend behavior is more serious than the duplicated API:

- The now-deleted `D3D12Texture` recorded copies and a transition directly into the current frame command list, and kept its upload allocation until the texture was destroyed.
- The now-deleted `VulkanTexture` created a transient command pool and buffer, submitted directly to the graphics queue, and called `vkQueueWaitIdle` for each texture upload. It also destroyed its image view, descriptor, image, and upload allocation through a path separate from generic RHI retirement.
- [`RhiUploadService`](../../../Engine/RHI/Public/Resources/RhiUploadService.h) handled only uniform constant-buffer allocation, so textures could not use a shared explicit upload contract.

This path hides command recording inside resource construction, serializes Vulkan texture loading, holds staging memory too long on D3D12, and creates two answers for resource/view lifetime.

### Before and after

| Before | After |
| --- | --- |
| `Texture` subclasses own native resources, private views/descriptors, staging, and upload commands. | TextureManager owns a Renderer record containing generic RHI resource/view handles plus CPU metadata. |
| Vulkan waits for the entire graphics queue once per uploaded texture. | Uploads are recorded into an explicit batch/command list and retire staging by submission fence. |
| D3D12 upload buffers live for the lifetime of the texture. | Upload storage lives only until the copy submission completes. |
| Materials ask a texture object to write backend descriptors. | RHI creates a resource view; binding tables consume logical RHI view/binding records. |
| Frame graph has special overloads for legacy `Texture`. | All persistent textures register through the generic owned-resource/view path. |

### Implemented batch

1. Extend `RhiTextureResourceDesc` only as needed to represent the existing cooked texture cases: mip count, array size, and 2D/cube dimension. Keep color-space/format intent in Renderer metadata unless it changes native creation.
2. Add an explicit texture write/upload operation to the existing `RhiUploadService`. Renderer supplies the destination handle and chooses when the upload is recorded; RHI owns staging layout, native copies, transitions, and fence retirement.
3. Initially use the graphics queue to keep the change small. Remove Vulkan's private command pool and `vkQueueWaitIdle` before adding a copy queue.
4. Let existing `TextureManager` hold a small non-polymorphic record: `RhiOwnedResourceHandle`, `RhiResourceViewHandle`, dimensions/format/mips, and upload readiness. Do not add another manager.
5. Create material SRVs through `RhiDescriptorService::CreateResourceView`; populate binding tables from logical views/bindings rather than `Texture::WriteShaderResourceView`.
6. Delete `Texture`, `D3D12Texture`, `VulkanTexture`, their factories, `RhiResourceService::CreateTexture`, and the special frame-graph texture registration overload after all callers migrate.

All six implementation items are present. The migration did not retain a compatibility wrapper or second texture factory. The existing upload service, descriptor services, backend memory allocators, and `TextureManager` were extended instead of introducing another manager or lifetime layer.

### Acceptance bar

| Criterion | Status | Evidence / remaining work |
| --- | --- | --- |
| No legacy `Texture`, `D3D12Texture`, `VulkanTexture`, or `WriteShaderResourceView` resource path remains. | **Fulfilled by static inspection** | Exact legacy class/symbol and deleted-file searches return no hits in RHI or Renderer. The broader literal pattern `class Texture` also matches valid names such as `TextureManager` and `enum class Texture...`; those are not legacy resource-path hits. |
| Vulkan texture loading contains no per-texture `vkQueueWaitIdle`. | **Fulfilled by static inspection** | `rg "vkQueueWaitIdle" Engine/RHI Engine/Renderer` returns no hits. Uploads record into the active graphics command buffer. |
| Upload allocations are released after their copy fence, not when the sampled texture is destroyed. | **Implemented; runtime verification pending** | D3D12 holds staging in `D3D12UploadService` until `GetCompletedValue()` reaches the stamped next fence. Vulkan queues staging in `VulkanGpuMemoryAllocator` using `GetNextRetireFenceValue()`. A multi-frame upload run has not yet been executed. |
| Material, sky, frame-graph, and provider paths use the same resource/view lifetime model on both backends. | **Implemented by code inspection; runtime backend parity pending** | Materials consume logical views, sky imports the generic resource/view pair without creating a duplicate SRV, and no Renderer or provider caller can construct the deleted legacy texture type. D3D12/Vulkan runtime comparison remains pending. |
| Shader ABI, material slot count, cooked texture payload, classic TLAS, and PTLAS contracts remain unchanged. | **Fulfilled by diff inspection** | Shader files, material slot constants/layouts, cooked texture asset structures/payload parsing, and acceleration-structure contracts were not changed by this priority. |

Priority 0B is therefore **implementation-complete but not runtime-accepted**. Final acceptance requires the explicitly deferred build plus D3D12/Vulkan runtime upload, material, sky, and several-frames-in-flight retirement scenarios.

## Priority 0C: Give The GameFramework / Renderer Boundary Stable Identity And Revisions

### Current evidence

- [`GameScene::CaptureSnapshot`](../../../Engine/GameFramework/Private/Scene/GameScene.cpp) copies camera, animation, lighting, sky, texture paths, material descriptions, and mesh records every frame.
- [`MeshInstanceSnapshot`](../../../Engine/GameFramework/Public/Scene/Meshes/MeshSnapshot.h) includes both cooked asset identity and a raw `const Mesh*`.
- [`SceneMeshes::CaptureSnapshot`](../../../Engine/GameFramework/Private/Scene/Meshes/SceneMeshes.cpp) filters by visibility and appends visible instances in the current vector order.
- [`RenderSceneSnapshot`](../../../Engine/Renderer/Private/SceneData/Lifecycle/RenderSceneSnapshot.cpp) is a field-for-field move of `GameSceneSnapshot`; it does not convert identity, lifetime, or semantics.
- [`GPUMeshCache`](../../../Engine/Renderer/Private/Meshes/GPUMeshCache.h) is keyed by raw `const Mesh*` and is cleared as a whole on level unload.
- [`RenderSceneDataBuilder`](../../../Engine/Renderer/Private/SceneData/Builders/RenderSceneDataBuilder.cpp) stores previous transforms by snapshot vector index and previous skin matrices by skeleton asset ID.
- Material descriptions are copied and then deep-compared in [`MaterialCacheUtils`](../../../Engine/Renderer/Private/SceneData/Caching/MaterialCacheUtils.cpp). Texture paths are traversed and normalized/cache-checked again each frame.

This is not a lifetime-safe immutable snapshot and it is not an incremental synchronization contract. Visibility changes or insertion/removal can move an instance to a different vector position, causing the wrong previous transform to be used. Multiple animated instances sharing one skeleton asset can also share one previous-pose key. Pointer-keyed mesh caches make allocator addresses part of renderer identity.

### Target handoff

GameFramework should publish two kinds of facts through the existing scene snapshot family:

1. Dynamic frame facts: stable instance ID, transform, pose, visibility, camera, and lights.
2. Revisioned static facts: mesh asset payload/reference, material description, texture reference, instance topology, and skeleton binding only when their revision changes.

Minimum identity fields:

- `SceneEpoch`: changes when a level/scene identity is replaced;
- `SceneInstanceId`: stable for the lifetime of one component/instance;
- `StructureRevision`: changes on add/remove/reparent/mesh assignment;
- `MaterialRevision` and `TextureRevision`: change when their authored tables change;
- an animation-instance or pose ID, distinct from the shared skeleton asset ID.

Cooked asset ID plus mesh asset index should identify immutable mesh assets. `SceneInstanceId` should identify temporal and per-instance state. These are different jobs and should remain different types.

### Before and after

| Before | After |
| --- | --- |
| Raw `Mesh*` and vector position participate in identity. | Cooked asset keys identify assets; stable scene IDs identify instances. |
| Full material/texture/static mesh facts are copied and compared every frame. | Revisions allow Renderer to skip unchanged static import work. |
| Renderer cache lifetime is "until level unload, then Flush and clear all." | Renderer adds/updates/removes records by scene epoch, revision, and stable ID; RHI retires old GPU objects. |
| Skeleton asset ID identifies previous pose. | Animation/scene instance ID identifies previous pose; skeleton asset ID identifies shared skeleton data. |
| `RenderSceneSnapshot` duplicates the source snapshot without translating it. | Either use `GameSceneSnapshot` directly as import input or make the Renderer type a real persistent imported state; remove the field-for-field copy. |

### Implementation batches

1. Add scene epoch, revisions, and stable per-instance IDs in GameFramework. Assign IDs when components/instances are created, not during snapshot capture.
2. Change temporal transform/pose maps in Renderer to stable instance IDs immediately. This yields a correctness improvement before the full data-flow refactor.
3. Change `GPUMeshCache` to a cooked mesh asset key. During migration, the raw mesh pointer may supply upload bytes, but it must no longer be the cache key or temporal identity.
4. Move immutable mesh payload ownership to a lifetime-safe asset record. Emit an owned/shared immutable payload only for asset-add/change records, then remove raw `Mesh*` from the public snapshot.
5. Add revision checks to existing `MaterialCacheManager` and `TextureManager`; remove per-frame deep material comparison and repeated full texture-path traversal.
6. Delete the field-for-field `RenderSceneSnapshot` once Renderer has a real import/cache boundary.

This does not require a new ECS, a new scene graph, or a general event bus. It extends the IDs and snapshots already present.

### Acceptance bar

- Toggling visibility, inserting/removing an instance, or reordering scene storage does not transfer motion history to another object.
- Two animated characters using the same skeleton asset maintain independent previous poses.
- `GPUMeshCache` contains no raw-pointer key.
- `MeshInstanceSnapshot` contains no raw `Mesh*` once immutable asset records are in place.
- An unchanged scene frame does not deep-copy/compare all materials or revisit every texture path.
- Level changes no longer require a GPU-wide flush merely to make Renderer cache destruction safe.

## Priority 1A: Move Persistent History To The Feature That Uses It

### Current evidence

[`FramePipeline`](../../../Engine/Renderer/Private/FramePipeline/FramePipeline.h) owns:

- exposure history resources and validity;
- reference-lighting history resources, extent, validity, and state/settings keys;
- direct-light reservoir history resources and validity;
- indirect ReSTIR reservoir history resources and validity;
- all feature-specific create/release/bind/reset methods;
- all reset decisions for window, resolution, GBuffer mode, lighting mode, scene state, and provider graph changes.

The implementation is split into several `.cpp` files, but all methods and members still belong to one class. Every temporal feature therefore expands the central pipeline's knowledge and reset matrix.

### Target ownership

- Exposure/post-processing owns exposure history.
- Reference-lighting owns reference accumulation history and its scene/settings key.
- Direct-light reservoir owns direct reservoir history.
- ReSTIR indirect owns indirect reservoir history.
- Image providers continue to own provider-specific histories.
- `FramePipeline` owns only frame coordination, the frame graph, a scene/frame reset epoch, and submission.

Each existing feature unit should expose small operations such as reserve graph handles, ensure physical resources, bind current/previous resources, consume a reset epoch, and report readiness. Do not create a generic `HistoryManager`; temporal resources have different formats, invalidation rules, and feature meanings.

### Before and after

| Before | After |
| --- | --- |
| Adding a temporal feature edits `FramePipeline.h`, multiple reset branches, pass runtime services, and end-of-frame validity updates. | Adding a temporal feature edits its feature owner and graph assembly; FramePipeline sends one reset epoch/reason. |
| Feature history is a collection of arrays and booleans in a coordinator. | A vertical feature slice owns its resources, keys, validity, and binding. |
| Reset logic is repeated across mode/resize/state branches. | Pipeline increments one history epoch; features decide what that invalidates. |

### Acceptance bar

- `FramePipeline.h` has no exposure/reference/direct/indirect history resource arrays or feature-specific validity booleans.
- Resize/mode/scene changes advance one explicit temporal reset epoch rather than calling every feature reset function.
- Each feature can be destroyed and have its resources safely retired without changing pipeline-wide destruction code.
- No generic history abstraction is introduced.

## Priority 1B: Make Material GPU State A Stable Renderer-Owned Table

The current material design has two legitimate binding policies:

- an eight-texture per-material table for raster batches;
- a scene-wide indexed texture table for ray tracing/bindless access.

The issue is not that both exist. The issue is how their ownership leaks:

- [`MaterialData`](../../../Engine/Renderer/Private/SceneData/MaterialData.h) stores `const RenderBindingSet*`;
- [`RenderSceneData`](../../../Engine/Renderer/Private/SceneData/RenderSceneData.h) stores another raw binding-set pointer plus validity/count/status fields;
- many passes repeat pointer, count, capacity, and validity checks;
- [`LightingSceneState`](../../../Engine/Renderer/Private/Frame/Lighting/LightingSceneState.cpp) hashes GPU mesh, texture, binding-set, material-table, and native-resource addresses into temporal scene identity.

Allocator address reuse is not a durable content revision. It can cause a false reset or, worse, fail to distinguish semantically changed state that happens to reuse an address.

### Implementation batch

1. Make the existing `MaterialCacheManager` the sole owner of material GPU records and both binding policies. Do not add another material manager.
2. Give material GPU records and the scene texture table stable Renderer IDs/revisions.
3. Store material IDs/indices and value-like logical table bindings in frame/pass data, not pointers to owner internals.
4. Centralize table readiness/capability resolution when building frame scene data; passes consume the resolved binding or an invalid value without repeating ownership checks.
5. Build lighting/temporal scene keys from scene epoch, asset/material/texture revisions, stable instance IDs, and authored values. Remove CPU/native pointer values from semantic hashes.
6. After Priority 0B, populate both material policies from generic RHI resource views and delete texture-driven descriptor writes.

### Acceptance bar

- `MaterialData` and `RenderSceneData` contain no raw `RenderBindingSet*`.
- Lighting scene keys contain no `reinterpret_cast<std::uintptr_t>` of GPU/cache objects and no native resource address.
- Replacing a material table while frames are in flight is safe through RHI retirement and does not require a flush.
- Raster and ray-tracing material paths retain the current shader-visible layouts and texture-slot semantics.

## Priority 2A: Add Queue/Submissions Only When A Real Workload Needs Them

[`RhiCommandSubmissionService`](../../../Engine/RHI/Public/Commands/RhiCommandSubmissionService.h) currently exposes only graphics command lists and frame submission. That is acceptable while Sparkle intentionally records one graphics-queue frame. It is not enough for asynchronous uploads or compute.

Do not build a generic queue scheduler now. First complete lifetime and upload unification. Before the first real copy-queue streaming or asynchronous-compute feature lands:

1. RHI should expose backend-neutral graphics/compute/copy queue capability, submission tokens, and queue waits.
2. Renderer frame graph should assign passes/uploads to queues and compile inter-queue dependencies.
3. RHI should translate that plan to native queues, fences/timeline semaphores, and waits.
4. Resource/descriptor retirement should use the submission token of the last queue that referenced the object.

This preserves the same split used for barriers: Renderer decides scheduling policy; RHI owns physical execution and synchronization.

## Priority 2B: Trim Misplaced Public Surface After The Ownership Work

This is a cleanup, not a prerequisite:

- remove export macros from Renderer-private helpers that are not true cross-module contracts;
- remove unused full-RHI includes from Renderer frame/context types;
- keep shader bytecode, reflection, binding layouts, and backend pipeline creation in RHI, while keeping renderer shader/package selection and pass policy in Renderer;
- keep native presentation and ImGui backend implementation in RHI private code, but let Application/Editor own UI and window/presentation orchestration;
- keep native handles available only through explicit interop requests for provider/debugger integration.

Do this by relocating or deleting APIs, not by placing another facade in front of the existing facade.

## Alignment With The Existing Architecture Requirements

This program refines the active architecture direction rather than replacing it.

| Existing requirement | How this decision satisfies it |
| --- | --- |
| Renderer-first product identity | Work is concentrated on scene-to-renderer import, renderer feature ownership, and explicit D3D12/Vulkan execution. It does not broaden GameFramework into a full game engine or add unrelated systems. |
| Explicit ownership of resources, descriptors, queues, fences, and native handles | Priorities 0A, 0B, and 2A make ownership and completion rules uniform while keeping native implementation in RHI. |
| Frame graph owns high-level state/barrier policy | The current Renderer frame graph is retained. RHI continues to execute backend-neutral transition/alias/UAV commands. |
| D3D12/Vulkan parity | Every foundation change has one public observable contract and requires both backend implementations before completion. |
| Preserve shader/compiler/runtime ABI strength | The program changes resource ownership and scene identity without changing shader payload layouts, cooked packages, or reflection contracts. |
| Preserve classic TLAS and PTLAS | Acceleration-structure policy remains Renderer-owned and native execution remains RHI-owned; neither path is demoted or replaced. |
| Provider-neutral SDK integration | Renderer chooses placement and inputs, provider bridges translate, and RHI supplies bounded native interop. SDKs do not acquire engine ownership. |
| Deletion-first implementation | The texture change deletes a complete parallel hierarchy; the scene change deletes pointer-keyed caches, duplicate snapshots, and deep comparisons; feature ownership deletes central pipeline fan-out. |
| No diagnostics/logging as substitute for behavior | Every acceptance criterion is a runtime ownership, correctness, synchronization, code-shape, or deletion result. No new observer system is proposed. |
| Avoid new wrappers and scaffolding | Work extends existing descriptor services, upload service, snapshots, caches, feature units, and frame graph. New types are limited to stable IDs/revisions and value records that replace unsafe pointers. |

This is also why a wholesale adoption of either NVRHI or Cauldron is not recommended. Sparkle's existing frame graph, shader system, RHI services, and provider boundaries already satisfy important repository requirements; the right move is to complete their contracts and remove the parallel paths.

## Ordered Change Program

| Order | Change list | Main outcome | Expected deletion/simplification |
| --- | --- | --- | --- |
| 1 | Fence-safe descriptor-table retirement (implemented; runtime stress pending) | Removes the in-flight descriptor reuse correctness hole without stalls. | Deleted immediate table/view record reuse and the shader-resource-only descriptor API; no caller-side fence or flush branch was added. |
| 2 | Generic texture resource/view/upload migration (implemented; build/runtime validation pending) | Removes per-texture Vulkan idle waits and the second GPU object model. | Deleted `Texture`, both backend subclasses, factories, the special frame-graph overload, duplicate sky SRV creation, and texture-driven descriptor writes. |
| 3 | Stable GameFramework scene/animation IDs | Fixes temporal identity under visibility/order changes and shared skeletons. | Removes vector-position and skeleton-only temporal maps. |
| 4 | Scene revisions and asset-keyed Renderer import | Makes static scene synchronization incremental and lifetime safe. | Removes raw mesh pointer keys, field-for-field Renderer snapshot, deep material comparisons, repeated texture traversal, and broad level cache invalidation. |
| 5 | Stable material GPU table and semantic scene keys | Makes material ownership explicit and history invalidation deterministic. | Removes raw binding pointers and pointer/native-address hashing from frame data. |
| 6 | Feature-owned persistent histories | Stops `FramePipeline` from growing with every temporal feature. | Removes feature arrays, booleans, keys, and repeated reset fan-out from the central pipeline. |
| 7 | Queue/submission extension for the first real async workload | Enables copy/compute overlap without moving scheduler policy into RHI. | Replaces ad hoc upload submission; should not be built speculatively. |

Change lists 1-4 are the foundation. Change lists 5-6 make continued renderer feature development sustainable. Change list 7 is a capability expansion and should wait for a measured workload.

## Non-Goals And Guardrails

This program must not turn into architecture theatre.

- Do not add diagnostics, logging, reports, dashboards, or validation layers as the deliverable.
- Do not replace the frame graph or move its scheduling/barrier policy into RHI.
- Do not add a generic resource manager, history manager, scene synchronization framework, or wrapper RHI.
- Do not copy NVRHI reference counting wholesale; copy the completeness of its lifetime contract.
- Do not copy Cauldron's sample framework organization wholesale.
- Do not expose native fences, descriptor heaps, command pools, or resources to ordinary Renderer code.
- Do not use `Flush`, `WaitForIdle`, or `vkQueueWaitIdle` as a normal mutation/lifetime mechanism.
- Do not build async compute/copy abstractions without a real feature ready to consume them.
- Do not redesign GameFramework into an ECS as part of stable identity/revision work.
- Do not change shader ABI, cooked packages, classic TLAS/PTLAS, material slots, or provider contracts unless a migration step proves it is necessary.

## Definition Of A Solid Foundation

The repository is ready for aggressive renderer development when all of the following are true:

- GameFramework snapshots contain stable scene/animation identity and no borrowed mutable asset pointers.
- Renderer imports static scene state only when revisions change and processes dynamic transforms/poses by stable instance ID.
- RHI has one resource/view/upload/destruction contract for material, history, frame-graph, ray-tracing, and provider resources.
- No GPU-visible resource, view, descriptor, binding table, or pipeline storage is recycled before the submission using it completes.
- Vulkan texture upload does not idle the graphics queue per texture.
- Persistent history is owned by its renderer feature, not by the central frame coordinator.
- Renderer frame/pass data carries IDs, indices, handles, and immutable values rather than raw pointers to cache-owned binding objects.
- Semantic history keys contain stable IDs/revisions and authored values, not allocator or native addresses.
- Renderer still owns frame-graph dependency/barrier/queue policy; RHI still owns backend execution.
- D3D12 and Vulkan pass the same lifetime, material, scene mutation, level change, and temporal-history scenarios.
- The existing architecture boundary check still passes.

## Immediate Executive Decision

If only three investments are approved now, approve these in order:

1. Fence-safe descriptor and binding-table retirement.
2. Deletion of the legacy texture path in favor of generic RHI resources/views and explicit uploads.
3. Stable GameFramework scene identity and revisioned Renderer synchronization.

Those changes produce a visible before/after in correctness, Vulkan loading behavior, memory lifetime, scene scalability, and the cost of adding future features. Feature-history ownership should follow immediately after them.

## Reviewed Source Revisions

Primary repositories were reviewed at these revisions on 2026-07-14:

| Repository | Revision | Use in this audit |
| --- | --- | --- |
| [NVIDIA NVRHI](https://github.com/NVIDIA-RTX/NVRHI/tree/8e8c36e37558acec333204619b95d9d2fcdc4a79) | `8e8c36e37558acec333204619b95d9d2fcdc4a79` | GPU lifetime, bindings, uploads, states, queues, feature-owned handles. |
| [NVIDIA NRI](https://github.com/NVIDIA-RTX/NRI/tree/4b485316463969f182db15e67aad2aec2f40a3d7) | `4b485316463969f182db15e67aad2aec2f40a3d7` | Low-level explicitness and higher-level barrier ownership. |
| [NVIDIA Donut](https://github.com/NVIDIA-RTX/Donut/tree/bc1ea24b0486f1c00d89327fe16c0b4dd11c5937) | `bc1ea24b0486f1c00d89327fe16c0b4dd11c5937` | Module split, dirty scene maintenance, pass-owned GPU state. |
| [AMD FidelityFX SDK](https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK/tree/60f4ea81909200d8542eca14dccb2628b763a9a3) | `60f4ea81909200d8542eca14dccb2628b763a9a3` | Effect/backend interface, resource registration, caller-owned command recording. |
| [AMD Cauldron](https://github.com/GPUOpen-LibrariesAndSDKs/Cauldron/tree/b92d559bd083f44df9f8f42a6ad149c1584ae94c) | `b92d559bd083f44df9f8f42a6ad149c1584ae94c` | Historical D3D12/Vulkan resource, upload, descriptor, and command-ring mechanics. |

Local source conclusions are based on the repository state on `master` at the same date and the requirements in this architecture review set.

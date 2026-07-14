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

The foundations are nevertheless not yet solid enough for unbounded feature growth. The two highest-risk gaps retained by the implementation plan are:

1. RHI GPU lifetime was only partially unified. Priority 0A now makes descriptor allocations, tables, and views retire through the backend's waited frame-slot boundary and protects recycled table/view records with generations. Runtime material-rebuild validation remains before final acceptance.
2. Material textures used a second resource model outside `RhiOwnedResourceHandle` and `RhiResourceViewHandle`. Priority 0B removed that hierarchy, its hidden uploads, lifetime-long staging, and per-texture Vulkan queue-idle wait. Build and runtime validation remain pending by explicit instruction.

Priority 1A has also moved persistent temporal mechanics and validity out of `FramePipeline`. Feature code retains temporal meaning and pass use, while one typed Renderer resource set owns concrete declarations and lifecycle fan-out. Its code and static deletion gates are complete; build and runtime validation remain deferred by explicit instruction.

These were correctness and scalability issues, not presentation issues. Their implementations must pass the deferred runtime acceptance before adding another descriptor model, streaming, or asynchronous compute.

The recommended direction is deliberately narrow:

- GameFramework owns authored CPU scene and asset data and publishes the current renderer-neutral scene snapshot.
- Renderer owns the imported render scene, GPU feature policy, frame graph, pass scheduling, persistent feature histories, and renderer-facing material/mesh tables.
- RHI owns every native GPU object, upload implementation, descriptor allocation, queue, fence, synchronization primitive, and deferred destruction rule.

Do not add a fourth abstraction layer. Strengthen and simplify the three layers that already exist.

## Target Responsibility Contract

| Layer | Must own | May publish upward/downward | Must not own |
| --- | --- | --- | --- |
| GameFramework | Levels, scene components, transforms, animation state, lights, cameras, material descriptions, cooked asset references, and authored CPU asset data. | The current renderer-neutral scene snapshot containing authored values and asset references. | RHI handles, descriptor indices, GPU addresses, resource states, barriers, frame-graph handles, render passes, temporal render history. |
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
| Scene handoff | Accepted current design | GameFramework publishes the current scene snapshot and Renderer consumes it privately. No additional scene-synchronization change program is planned. |
| GPU object model | Priority 0B implemented; build/runtime validation pending | The polymorphic texture hierarchy and its separate creation, view, upload, and destruction rules have been deleted. Sampled textures now use generic RHI resources, views, and explicit uploads. |
| In-flight lifetime | Priorities 0A and 0B implemented; runtime stress validation pending | Resources, descriptor allocations, descriptor tables, views, and texture staging now have backend-owned deferred reuse. The legacy texture hierarchy and its bespoke destruction paths have been deleted. |
| Feature ownership | Priority 1A implemented; build/runtime validation pending | Feature files declare semantic continuity and pass/resource use. A typed Renderer history resource set owns names, formats, sizing, frame-slot allocation, validity, graph binding, and retirement; `FramePipeline` coordinates it through one temporal reset generation. |
| Material bindings | Needs work | Per-material binding sets and a global material table are both valid policies, but ownership is exposed as raw pointers and repeated availability checks across passes. |
| Queue model | Intentionally limited | The public submission contract exposes only graphics command lists. This is honest today but must be extended before real asynchronous copy/compute scheduling. |

The passing boundary script is evidence that compile-time direction is healthy, not evidence that runtime ownership is complete. The current check protects Renderer/RHI source dependencies; it does not prove descriptor lifetime, upload behavior, or feature ownership.

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

Sparkle should borrow Donut's feature-ownership principle, not its exact class hierarchy: persistent resources should normally be owned by the renderer feature that gives them meaning. Donut's scene-maintenance model is not part of this implementation plan.

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
5. Let features own temporal meaning and resource use; keep repeated persistent-resource mechanics in the Renderer resource layer.
6. Keep native SDK/API translation in narrow backend/provider bridges.

Sparkle already satisfied principles 2 and 6. Priorities 0A, 0B, and 1A now implement principles 3, 4, and 5 in code; deferred build/runtime validation and the Priority 1B material-state cleanup remain.

## Legacy-Removal Completion Gate

Every priority is a replacement, not an addition. A priority is not complete while its superseded path, compatibility wrapper, duplicate state, forwarding facade, or obsolete ownership branch remains in production code. Temporary migration code is allowed only inside the active implementation batch and must have an explicit deletion step in that same batch.

| Priority | Legacy surface that must be gone before completion |
| --- | --- |
| 0A | Immediate descriptor/table reuse paths, recycled unversioned table/view records, and the shader-resource-only allocate/release convenience API. |
| 0B | `Texture`, `D3D12Texture`, `VulkanTexture`, texture factories, `RhiResourceService::CreateTexture`, object-owned descriptor writes, private upload submissions, and special frame-graph texture overloads. |
| 1A | Feature-specific history resources, validity fields, state keys, reset methods, and lifecycle fan-out in `FramePipeline`; no forwarding methods or intermediate feature history wrappers remain. |
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
- `RenderSkyData` carries one non-owning reference to the canonical Renderer texture record rather than flattening its resource, view, and metadata fields. The sky persistent import passes that record's existing generic resource and view to the frame graph, which tracks the view as borrowed and does not create a duplicate SRV.
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

## Priority 1A: Move Persistent History To The Feature That Uses It

### Implementation status: code, deduplication, and static deletion checks complete; build and runtime validation intentionally pending

The implementation is present in the working tree. In accordance with the priority execution rule, no engine build, backend compile, or runtime launch was performed for Priority 1A.

The ownership and frontend shape after the change is:

- [`FrameHistory`](../../../Engine/Renderer/Private/Resources/History/FrameHistory.h) exposes the complete intent-level graph layout as named exposure, reference-lighting, direct-reservoir, and indirect-reservoir records.
- [`PersistentFrameHistory`](../../../Engine/Renderer/Private/Resources/History/PersistentFrameHistory.h) is one concrete resource aggregate. It hides the four physical history declarations and fans out graph-layout assignment, activation, binding, validity, and commit operations.
- [`PersistentTextureHistory`](../../../Engine/Renderer/Private/Resources/History/PersistentTextureHistory.h) owns the common frame-slot allocation, graph binding, reset consumption, validity, and retirement mechanics outside the pass/resource frontend.
- [`PersistentReservoirHistory`](../../../Engine/Renderer/Private/Resources/History/PersistentReservoirHistory.h) composes three persistent textures behind named sample/weight/surface handles.
- [`ReferenceLightingState`](../../../Engine/Renderer/Private/Frame/Lighting/ReferenceLightingState.cpp) and [`RestirLightingState`](../../../Engine/Renderer/Private/Frame/Lighting/RestirLightingState.cpp) retain only the feature-authored values that define semantic continuity.
- [`FramePipeline`](../../../Engine/Renderer/Private/FramePipeline/FramePipeline.h) retains one `PersistentFrameHistory` and one `m_temporalResetGeneration`; it no longer contains per-feature resource objects, validity booleans, formats, extents, or lifecycle fan-out.

Resize, presentation, resolution, GBuffer, lighting-mode, provider-graph, and scene-coordinator invalidations use one `ResetTemporalState` path. Feature settings and scene inputs are combined into one semantic history key by the feature state builders, while the shared resource primitive performs the mechanical invalidation and binding work.

The required cleanup pass also completed:

- the five `FramePipeline*History.cpp` implementations and the pipeline-local reservoir helper were deleted rather than retained as forwarding compatibility layers;
- the flat 16-field history handle bag was replaced with four feature-shaped graph records;
- the intermediate feature history headers and `.cpp` files were deleted after names, formats, sizing, graph declaration, and repeated frame-slot/resource mechanics moved to `Resources/History`;
- `FrameSceneResources` declares the complete history layout with one intent-level call; it contains no history formats, persistent allocation calls, or per-feature reserve sequence;
- `FramePipeline` performs one graph-layout assignment, one activation declaration, one bind, one validity export, and one commit rather than repeating each operation for four histories;
- reservoir graph handles are grouped as named sample/weight/surface previous/current pairs, and the indirect clear helper accepts only that reservoir record;
- exposure and direct-shadow assembly preserve those history records through frontend plumbing instead of flattening them back into unrelated handle arguments or fields;
- reference and ReSTIR settings/scene hashes were each collapsed into one feature-level history key instead of being passed through separate settings/state update calls.

A repository-wide Renderer audit found no additional ordinary persistent GPU histories outside these four declarations. The remaining history-related state is intentionally different:

- `TemporalDataBuilder` owns CPU camera/jitter continuity, not a GPU resource;
- `SkinningFrameData::PreviousBuffer` uploads the previous pose supplied in the current frame snapshot and does not persist a resource across frames;
- image reconstruction/upscaling providers own opaque SDK histories and receive only reset intent through `RendererImageProviderStack`;
- shader uniform `HistoryValid` fields describe pass behavior and do not own lifetime.

These paths should not be wrapped in `PersistentTextureHistory`. Future Renderer-owned previous/current GPU textures should extend the concrete frame-history layout and implementation under `Resources/History`; pass/resource frontend files should consume the named record without adding allocation, frame-slot, format, binding, or commit code.

### Baseline evidence addressed by this priority

Before this implementation, [`FramePipeline`](../../../Engine/Renderer/Private/FramePipeline/FramePipeline.h) owned:

- exposure history resources and validity;
- reference-lighting history resources, extent, validity, and state/settings keys;
- direct-light reservoir history resources and validity;
- indirect ReSTIR reservoir history resources and validity;
- all feature-specific create/release/bind/reset methods;
- all reset decisions for window, resolution, GBuffer mode, lighting mode, scene state, and provider graph changes.

The implementation was split into several `.cpp` files, but all methods and members still belonged to one class. Every temporal feature therefore expanded the central pipeline's knowledge and reset matrix.

### Implemented ownership

- Exposure/post-processing owns exposure history meaning and pass usage.
- Reference-lighting owns the scene/settings key and accumulation pass usage.
- Direct-light reservoir owns its temporal/spatial pass usage.
- ReSTIR indirect owns its semantic key and temporal/spatial/resolve pass usage.
- `Resources/History` owns the concrete physical resource catalog and repeated lifecycle mechanics.
- Image providers continue to own provider-specific histories.
- `FramePipeline` owns only frame coordination, the frame graph, a temporal reset generation, and submission.

Feature code declares resource use and the values that define semantic continuity. Formats, sizing, frame slots, and lifecycle operations are not frontend concerns. The concrete aggregate is not a generic `HistoryManager`: it contains no feature settings, scene hashing, reset decisions, pass scheduling, or provider policy.

### Before and after

| Before | After |
| --- | --- |
| Adding a temporal feature edits `FramePipeline.h`, multiple reset branches, pass runtime services, and end-of-frame validity updates. | A feature declares pass use and a semantic key; the concrete history resource set supplies its layout, frame-slot lifetime, activation, binding, validity, and commit. |
| Feature history is a collection of arrays and booleans in a coordinator. | `Frame` sees named previous/current resource records; reusable Renderer resource objects own every concrete lifecycle detail. |
| Reset logic is repeated across mode/resize/state branches. | Pipeline increments one temporal reset generation; features decide what that invalidates. |

### Acceptance bar

| Criterion | Status | Evidence / remaining work |
| --- | --- | --- |
| `FramePipeline.h` has no exposure/reference/direct/indirect history resource arrays or feature-specific validity booleans. | Fulfilled statically | The pipeline contains one concrete history aggregate. Named resource arrays, extents, validity flags, formats, and lifecycle methods are absent. |
| Resize/mode/scene changes advance one explicit temporal reset generation rather than calling every feature reset function. | Fulfilled statically | Global invalidation paths call `ResetTemporalState`; camera/temporal invalidity advances the same generation. No feature reset fan-out functions remain. |
| Each feature can be disabled and have its resources safely retired without changing pipeline-wide destruction code. | Fulfilled in code; runtime pending | The concrete aggregate translates feature activation intent into primitive release. Persistent resource primitives retire through `RhiResourceService`; `FramePipeline` has a default destructor and no history destruction sequence. Runtime retirement still requires deferred multi-frame validation. |
| No generic history policy manager is introduced; repeated physical mechanics are not left in feature files. | Fulfilled statically | `Resources/History` implements a concrete typed resource set and common mechanics. Feature state builders still own semantic keys, while the pipeline still owns reset decisions. |
| Every Renderer-owned persistent GPU texture history follows the same record and lifetime pattern. | Fulfilled statically | Exposure, reference lighting, direct-light reservoirs, and indirect ReSTIR reservoirs are the complete ordinary GPU-history inventory. All names, formats, sizing, frame-slot allocation, binding, validity aggregation, and retirement code exists only under `Resources/History`. |

Priority 1A is therefore **implementation-complete but not runtime-accepted**. Final acceptance requires the explicitly deferred build plus resize, lighting-mode, settings, scene-state, and several-frames-in-flight history scenarios.

## Priority 1B: Make Material GPU State A Stable Renderer-Owned Table

The current material design has two legitimate binding policies:

- an eight-texture per-material table for raster batches;
- a scene-wide indexed texture table for ray tracing/bindless access.

The issue is not that both exist. The issue is how their ownership leaks:

- [`MaterialData`](../../../Engine/Renderer/Private/SceneData/MaterialData.h) stores `const RenderBindingSet*`;
- [`RenderSceneData`](../../../Engine/Renderer/Private/SceneData/RenderSceneData.h) stores another raw binding-set pointer plus validity/count/status fields;
- many passes repeat pointer, count, capacity, and validity checks;
- [`LightingSceneState`](../../../Engine/Renderer/Private/Frame/Lighting/LightingSceneState.cpp) hashes GPU mesh, texture, binding-set, material-table, and native-resource addresses into temporal scene identity.

Allocator address reuse is not durable semantic state. It can cause a false reset or, worse, fail to distinguish changed material state that happens to reuse an address.

### Implementation batch

1. Make the existing `MaterialCacheManager` the sole owner of material GPU records and both binding policies. Do not add another material manager.
2. Give material GPU records value-like Renderer handles and give each rebuilt scene texture table an explicit Renderer-local generation.
3. Store material IDs/indices and value-like logical table bindings in frame/pass data, not pointers to owner internals.
4. Centralize table readiness/capability resolution when building frame scene data; passes consume the resolved binding or an invalid value without repeating ownership checks.
5. Build lighting/temporal keys from existing authored scene/material values, cooked asset identifiers, logical GPU handles, and Renderer-local table generations. Remove CPU/native pointer values from semantic hashes. This does not require a new GameFramework synchronization contract.
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
| Renderer-first product identity | Work is concentrated on renderer feature ownership and explicit D3D12/Vulkan execution. The current GameFramework snapshot boundary remains unchanged. |
| Explicit ownership of resources, descriptors, queues, fences, and native handles | Priorities 0A, 0B, and 2A make ownership and completion rules uniform while keeping native implementation in RHI. |
| Frame graph owns high-level state/barrier policy | The current Renderer frame graph is retained. RHI continues to execute backend-neutral transition/alias/UAV commands. |
| D3D12/Vulkan parity | Every foundation change has one public observable contract and requires both backend implementations before completion. |
| Preserve shader/compiler/runtime ABI strength | The program changes GPU resource and feature ownership without changing shader payload layouts, cooked packages, or reflection contracts. |
| Preserve classic TLAS and PTLAS | Acceleration-structure policy remains Renderer-owned and native execution remains RHI-owned; neither path is demoted or replaced. |
| Provider-neutral SDK integration | Renderer chooses placement and inputs, provider bridges translate, and RHI supplies bounded native interop. SDKs do not acquire engine ownership. |
| Deletion-first implementation | The texture change deletes a complete parallel hierarchy; material ownership deletes raw binding pointers and duplicate readiness state; feature ownership deletes central pipeline fan-out. |
| No diagnostics/logging as substitute for behavior | Every acceptance criterion is a runtime ownership, correctness, synchronization, code-shape, or deletion result. No new observer system is proposed. |
| Avoid new wrappers and scaffolding | Work extends existing descriptor services, upload service, caches, feature units, and frame graph. New types are limited to value records, logical handles, and local generations that replace unsafe pointers or duplicated state. |

This is also why a wholesale adoption of either NVRHI or Cauldron is not recommended. Sparkle's existing frame graph, shader system, RHI services, and provider boundaries already satisfy important repository requirements; the right move is to complete their contracts and remove the parallel paths.

## Ordered Change Program

| Order | Change list | Main outcome | Expected deletion/simplification |
| --- | --- | --- | --- |
| 1 | Fence-safe descriptor-table retirement (implemented; runtime stress pending) | Removes the in-flight descriptor reuse correctness hole without stalls. | Deleted immediate table/view record reuse and the shader-resource-only descriptor API; no caller-side fence or flush branch was added. |
| 2 | Generic texture resource/view/upload migration (implemented; build/runtime validation pending) | Removes per-texture Vulkan idle waits and the second GPU object model. | Deleted `Texture`, both backend subclasses, factories, the special frame-graph overload, duplicate sky SRV creation, and texture-driven descriptor writes. |
| 3 | Feature-owned persistent histories | Stops `FramePipeline` from growing with every temporal feature. | Removes feature arrays, booleans, keys, and repeated reset fan-out from the central pipeline. |
| 4 | Stable material GPU table and semantic scene keys | Makes material ownership explicit and history invalidation deterministic using existing authored/cooked values and logical handles. | Removes raw binding pointers and pointer/native-address hashing from frame data. |
| 5 | Queue/submission extension for the first real async workload | Enables copy/compute overlap without moving scheduler policy into RHI. | Replaces graphics-queue-only upload scheduling when a measured workload justifies it; it should not be built speculatively. |

Change lists 1-2 are the GPU ownership foundation. Change lists 3-4 make continued renderer feature development sustainable. Change list 5 is a capability expansion and should wait for a measured workload.

## Non-Goals And Guardrails

This program must not turn into architecture theatre.

- Do not add diagnostics, logging, reports, dashboards, or validation layers as the deliverable.
- Do not replace the frame graph or move its scheduling/barrier policy into RHI.
- Do not add a generic resource manager, central history policy manager, scene synchronization framework, or wrapper RHI. Reusable physical history-resource mechanics are allowed and should stay outside feature/pass frontend files.
- Do not copy NVRHI reference counting wholesale; copy the completeness of its lifetime contract.
- Do not copy Cauldron's sample framework organization wholesale.
- Do not expose native fences, descriptor heaps, command pools, or resources to ordinary Renderer code.
- Do not use `Flush`, `WaitForIdle`, or `vkQueueWaitIdle` as a normal mutation/lifetime mechanism.
- Do not build async compute/copy abstractions without a real feature ready to consume them.
- Do not expand the current GameFramework/Renderer scene handoff as part of the remaining Renderer/RHI priorities.
- Do not change shader ABI, cooked packages, classic TLAS/PTLAS, material slots, or provider contracts unless a migration step proves it is necessary.

## Definition Of A Solid Foundation

The repository is ready for aggressive renderer development when all of the following are true:

- RHI has one resource/view/upload/destruction contract for material, history, frame-graph, ray-tracing, and provider resources.
- No GPU-visible resource, view, descriptor, binding table, or pipeline storage is recycled before the submission using it completes.
- Vulkan texture upload does not idle the graphics queue per texture.
- Persistent history is owned by its renderer feature, not by the central frame coordinator.
- Renderer frame/pass data carries IDs, indices, handles, and immutable values rather than raw pointers to cache-owned binding objects.
- Semantic history keys contain existing authored/cooked values, logical handles, and Renderer-local table generations, not allocator or native addresses.
- Renderer still owns frame-graph dependency/barrier/queue policy; RHI still owns backend execution.
- D3D12 and Vulkan pass the same lifetime, material, scene mutation, level change, and temporal-history scenarios.
- The existing architecture boundary check still passes.

## Immediate Executive Decision

If only three investments are approved now, approve these in order:

1. Fence-safe descriptor and binding-table retirement.
2. Deletion of the legacy texture path in favor of generic RHI resources/views and explicit uploads.
3. Feature-owned persistent histories that remove feature-specific resource and reset fan-out from `FramePipeline`.

Those changes produce a visible before/after in correctness, Vulkan loading behavior, memory lifetime, central-pipeline size, and the cost of adding future renderer features. Stable material GPU ownership should follow them without introducing a new GameFramework synchronization program.

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

# Frame-Graph Resource Reference Design

Status: implemented and cleanup-audited

Last verified: 2026-07-14

## Outcome

Sparkle now has one ordinary-resource path for frame textures, buffers, and acceleration structures:

```text
resource owner
    -> frame-graph resource handle
    -> typed pass field declaring SRV, UAV, RTV, DSV, or AS access
    -> frame-graph dependency and transition plan
    -> RHI-created view during execution
```

High-level renderer data does not carry `RhiResourceViewHandle`, GPU descriptor handles, or descriptor-table bindings for ordinary resources. The frame graph owns resource-use declarations and view materialization. The RHI descriptor service owns native view destruction and defers it until the relevant frame-in-flight slot is GPU-safe.

The implementation deliberately keeps the existing `FrameGraphTextureHandle`, `FrameGraphBufferHandle`, and `FrameGraphAccelerationStructureHandle` names. These small value handles are Sparkle's equivalent of an RDG resource reference. Adding parallel `*Ref` aliases would create two names for one identity without improving ownership or validation.

## Scope

Implemented:

- whole-resource SRV and UAV declarations for textures and structured buffers;
- RTV and DSV declarations;
- graph-tracked acceleration-structure access;
- transient graph resources;
- compiler-derived dependencies, culling, execution order, resource lifetimes, and barriers;
- conservative transient-memory aliasing on both D3D12 and Vulkan;
- the swap-chain back buffer;
- persistent external slots whose backing resource can change between frames;
- automatic state transitions derived from pass declarations;
- descriptor/view creation inside the graph/RHI boundary;
- GPU-safe view retirement in the descriptor owner;
- explicit bindless material-table and per-draw descriptor exceptions.

Not part of this change:

- per-mip or per-array-slice view references;
- graph identity or generation bits in public handles;
- conversion to an immediate graph rebuilt every frame;
- bindless per-entry dependency tracking;
- multi-queue scheduling or parallel command recording.

Those are independent capabilities. They should be added only when a consumer needs them, without introducing compatibility aliases in advance.

## Ownership model

| Identity | Lifetime | Owner | Contains |
|---|---|---|---|
| Asset reference | project or level | game framework / asset system | path and import intent |
| Renderer resource | loaded GPU-resource lifetime | texture or frame-buffer owner | owning RHI resource identity |
| Frame-graph handle | one `FrameGraph` lifetime | frame assembly / frame graph | indexed graph resource identity |
| Native resource view | GPU-use lifetime | RHI descriptor service | backend SRV, UAV, RTV, or DSV state |
| Pass binding | one graph setup/execution | typed pass parameters | graph handle plus access semantic |

These identities are intentionally separate. For example, `SceneSkyDesc` stores an asset reference, `RenderSkyData` stores a non-owning renderer `Texture*`, frame assembly stores a `FrameGraphTextureHandle`, and a Sky pass stores a typed `ShaderTexture2D` field. None of those high-level values owns a native descriptor.

Snapshots are also intentional boundaries. `SceneSkySnapshot` may currently contain the same value payload as `SceneSkyDesc`, but it gives the renderer a stable copy whose lifetime and future synchronization can differ from mutable game-framework state. It is not considered duplicated ownership.

## Actual Sparkle API model

### Graph construction

The construction-only `FrameGraphBuilder` creates transient resources, imports the back buffer, and reserves external slots:

```cpp
FrameGraphTextureHandle sceneColor = builder.CreateTexture(desc);
FrameGraphTextureHandle backBuffer = builder.ImportBackBuffer(backBufferDesc, ResourceState::Present);
FrameGraphTextureHandle sky = builder.ReservePersistentTexture(skyDesc, ResourceState::ShaderResource);
FrameGraphBufferHandle lights = builder.ReservePersistentBuffer(lightDesc, ResourceState::ShaderResource);
```

The builder does not expose runtime bind or clear operations. Runtime rebinding belongs directly to `FrameGraph`, which owns the persistent slot. Unused general `ImportTexture`, `ImportBuffer`, `ImportAccelerationStructure`, and combined persistent-import variants were removed; Sparkle had no callers and already had the clearer reserve-then-bind model.

`ImportBackBuffer` is named specifically because it is not a general external-resource import. The presentation service supplies its native resource during execution.

### Runtime external binding

Before setup and compilation, frame orchestration binds current external resources:

```cpp
frameGraph.BindPersistentTexture(resources.External.Sky, skyTexture, ResourceState::ShaderResource);
frameGraph.BindPersistentBuffer(
    resources.External.MeshInstances,
    frameBuffer.GetResource(),
    FrameGraphBufferDesc::Create(
        "MeshInstances",
        frameBuffer.GetSizeInBytes(),
        frameBuffer.GetStrideInBytes()),
    ResourceState::ShaderResource);
```

Binding performs only graph-resource work:

1. validate that the handle identifies the expected persistent resource class;
2. update structured metadata such as format, dimensions, size, and stride;
3. release views if the native resource or view-relevant metadata changed;
4. bind the new native resource identity;
5. update the incoming tracked state.

The frame graph does not own the external resource. The texture manager or `FrameBufferResource` keeps it alive. The graph owns only its reference, state, and generated views.

### Pass declaration

Passes bind resources one by one through typed fields:

```cpp
parameters->SceneColor = builder.CreateUAV(sceneColor);
parameters->SceneDepth = builder.CreateSRV(sceneDepth);
parameters->SkyTexture = builder.CreateSRV(sky);
```

The typed field records a graph handle and semantic. It is not a descriptor wrapper. Compilation attributes the declaration to the underlying resource; execution resolves the RHI view and binding.

Sparkle currently materializes one default whole-resource view per required access kind. Separate `FrameGraphTextureSRVRef` or `FrameGraphBufferUAVRef` objects would add no information until subresource or format-overridden views are supported, so they are intentionally deferred.

## Resource-view lifetime

The original artifact investigation exposed an ownership error: a pass-local helper could release a descriptor before the submitted GPU work consumed it. The final lifetime rule is now general:

- `RhiDescriptorService::CreateResourceView` creates an RHI-owned view record;
- callers may release the opaque view handle when CPU ownership ends;
- the D3D12 and Vulkan descriptor services retain the native record in a per-frame retirement queue;
- `RenderDeviceServices::BeginFrame` reclaims the queue only after the backend has waited for that frame-in-flight slot;
- Vulkan image views and registered descriptors, and D3D12 descriptor allocations, are destroyed or recycled by their owning service;
- framegraph code does not maintain its own duplicate retirement queue.

This follows the same principle as resource retirement: logical release and native destruction are different events. Centralizing this in the RHI makes the rule apply to every graph user and keeps backend details out of renderer orchestration.

Vulkan swap-chain rebuilds release only swap-chain view records. They no longer clear unrelated graph views from the shared descriptor manager.

## Transient resources and aliasing

The compiler derives each transient lifetime from declarations in the surviving, topologically ordered passes. A render product exported to frame orchestration remains live through graph completion because presentation, capture, or editor UI may consume it after `FrameGraph::Execute` returns.

Physical blocks are reused only when lifetimes are disjoint and the allocation pool, resource descriptor, alignment, size, offset, and optimized-clear contract are compatible. This is deliberately conservative: exact compatibility saves memory without introducing reinterpretation rules that the current whole-resource graph cannot express.

The allocator consumes that compiled plan through backend-neutral RHI operations:

- ordinary, render-target, depth-stencil, and buffer memory classes remain distinct;
- D3D12 maps those classes to legal heap flags and creates placed alias resources through D3D12MA;
- Vulkan creates VMA alias images or buffers from the matching memory block;
- the graph emits alias barriers between successive owners and a frame-begin wrap barrier before a persistent graph reuses its first owner;
- a changed compiled assignment or view requirement rematerializes the transient plan instead of retaining stale physical resources.

Resource barriers, allocation, and alias hand-offs are graph-managed. Draw, dispatch, and ray-dispatch commands remain explicit in each pass execution callback. The current executor is one serial graphics command stream; the code does not imply async-compute or parallel recording that the scheduler and RHI do not yet provide.

## Uploaded frame buffers

Uploaded light, mesh-instance, skinning, and ray-hit buffers share one `FrameBufferResource` owner. It stores exactly one owning RHI resource plus the size and stride required to register that resource with the graph. It is move-only and releases through `RhiResourceService` in its destructor.

This replaced four copies of move construction, move assignment, destruction, partial-upload cleanup, and `ReleaseOwnedResource` loops. Feature frame-data classes now contain domain data and default their move/destruction behavior. The abstraction is retained because it owns a real lifetime and prevents partial-failure leaks; it is not a pass-binding facade.

## Bindless exception

`ShaderTexture2DTableSRV` remains for the global material table. Dynamic indexing means the graph cannot infer an individual texture dependency from one shader parameter. The exception has a distinct contract:

- the material table owner owns descriptor and texture lifetime;
- table resources are stable for GPU use;
- pass code binds the table explicitly;
- ordinary known textures and buffers may not use the bindless path for convenience.

Raster per-draw material textures also remain explicit because the draw loop selects a binding set for each material. Sky and indirect-lighting resources are not exceptions.

## Orchestrator and implementer boundaries

| Layer | Responsibility | Forbidden detail |
|---|---|---|
| `FramePipeline` | choose and bind this frame's external resources; sequence setup, compile, execute | native API objects, descriptor allocation |
| frame subsystem functions | create graph resources and add passes | backend state transitions |
| pass classes | declare each shader resource and assign constants | resource ownership or hidden multi-resource binding helpers |
| `FrameGraph` | validate graph resources, derive dependencies/states, resolve required views | asset paths or game objects |
| `RhiDescriptorService` | create, resolve, retire, and destroy native views | scene or pass policy |
| backend descriptor implementation | D3D12/Vulkan allocation and destruction | renderer feature names |

Feature-specific binding wrappers and macro-expanded resource groups were removed. Foundational shader-declaration macros remain because each invocation explicitly names one shader field and supplies reflection metadata; they do not hide a collection of unrelated bindings.

## Renderer and RHI module boundary

The dependency is intentionally one-way:

```text
host / game framework
        -> Renderer public facade
        -> Renderer graph, passes, and resource owners
        -> RHI public contracts
        -> RHI common implementation
        -> D3D12 or Vulkan private backend
```

“Renderer and RHI separation” does not mean that Renderer can issue GPU work without an RHI. It means Renderer consumes only the backend-neutral public RHI contract, while RHI never depends on Renderer and native API mechanics never become renderer policy.

The cleanup enforces this in concrete APIs:

- `PassParameterLayout` remains an RHI shader-binding contract and no longer contains `FrameGraphTracked` or any other graph policy. Whether a typed field uses a graph resource is Renderer metadata stored with `PassParameterSet`.
- Renderer feature selection consumes neutral capabilities such as sampled-image non-uniform indexing and TLAS descriptor/device-address access. It does not infer support from `ERhiBackendApi` or combine Vulkan/D3D12 implementation flags.
- D3D12 and Vulkan both translate `ResourceState` into the native state carried by `NativeTextureViewInfo`. The Streamline provider consumes that interop record and contains no numeric D3D12 state constants.
- the public `Renderer` facade exposes focused wait and host-presentation operations instead of returning the complete RHI device or command-submission service to Application.
- backend identity branches are confined to the dedicated Streamline external-provider adapter, where the third-party API explicitly requires different D3D12 and Vulkan payload shapes.

The architecture check rejects reverse RHI-to-Renderer dependencies, frame-graph policy in RHI public headers, RHI-private includes from Renderer, native D3D12/Vulkan symbols in Renderer, backend policy branches outside explicit vendor interop adapters, and public Renderer escape hatches for the whole RHI device.

## Reference alignment

The design follows the parts of established systems that match Sparkle's persistent graph:

- NVIDIA NVRHI tracks resource states and barriers and keeps command-list references so destruction is deferred until GPU use completes. Its binding sets contain resources while the implementation creates the native views. See the [NVRHI programming guide](https://github.com/NVIDIA-RTX/NVRHI/blob/main/doc/ProgrammingGuide.md) and [NVRHI overview](https://github.com/NVIDIA-RTX/NVRHI).
- NVIDIA Donut is used as a reference for explicit pass implementation on NVRHI, not as a frame-graph compiler: renderer code records concrete draw and dispatch work while NVRHI supplies the backend abstraction. See the [Donut repository](https://github.com/NVIDIA-RTX/Donut).
- AMD Render Pipeline Shaders declares resource access in the graph, derives transitions, and distinguishes external, persistent, transient, and temporal resources. See the [RPS introduction](https://github.com/GPUOpen-LibrariesAndSDKs/RenderPipelineShaders/blob/main/docs/tutorial/rps_tutorial_intro.md) and [resource tutorial](https://github.com/GPUOpen-LibrariesAndSDKs/RenderPipelineShaders/blob/main/docs/tutorial/rps_tutorial_p2.md).
- Epic's RDG uses graph resource references, derives barriers and lifetimes from pass parameters, registers external resources, and creates SRV/UAV views through the graph. See Epic's [Render Dependency Graph guide](https://dev.epicgames.com/documentation/en-us/unreal-engine/render-dependency-graph-in-unreal-engine) and [`FRDGBuilder` API](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RenderCore/FRDGBuilder).

Sparkle does not copy these APIs mechanically. Unlike Unreal's usual immediate per-frame graph, Sparkle builds a graph structure once and rebinds persistent external slots each frame. The ownership and declaration principles transfer; the exact reference representation does not need to.

## D3D12 and Vulkan parity contract

The current and preceding rendering changes were audited as one resource-lifetime change, not as independent backend patches. The retained cross-RHI contract is:

| Concern | Shared contract | D3D12 implementation | Vulkan implementation |
|---|---|---|---|
| View creation | one validated `RhiResourceViewDesc` | descriptor allocation and D3D view creation | image/buffer view registration and descriptor write |
| View release | logical release now, native reclaim after the frame slot is safe | recycle CPU/GPU descriptor allocation | destroy owned `VkImageView` and recycle registered-descriptor index |
| Shader-readable texture state | pixel, compute, and ray-query consumers are valid | `ALL_SHADER_RESOURCE` | graphics plus compute stage scope and shader-read layout |
| Persistent history first use | physical creation starts undefined; graph declarations establish the first legal state | graph-generated transition | graph-generated image-layout transition |
| TLAS build/update | sizing flags and submitted build flags must match | `ALLOW_UPDATE` retained for build-with-update and update | `ALLOW_UPDATE` retained for build-with-update and update |
| Shader binding identity | one package parameter index is one logical binding across stages | DXIL register binding | normalized SPIR-V descriptor set/binding |
| Non-uniform texture indexing | Renderer consumes one neutral capability | fixed descriptor arrays supported | queried feature must be enabled before material-table selection |
| External native texture state | provider receives backend-native state from RHI | `ResourceState` converted to `D3D12_RESOURCE_STATES` | `ResourceState` converted to image layout |
| Storage image format | graph format remains authoritative | typed DXIL UAV view | DXC formatless storage image with required Vulkan read/write-without-format features |
| Transient descriptor capacity | allocation must fit the compiled layout, including bindless arrays | descriptor heap/table allocation | pool pages sized and consumed from compiled descriptor counts |

The shared RHI validator rejects unusable view descriptions before either backend allocates native state. Backend code retains only API-specific mechanics; no Sky, indirect-lighting, or pass names appear in descriptor allocation, state conversion, or retirement.

SPIR-V binding normalization is an offline compiler operation driven by the package's existing parameter layout. It does not introduce a second runtime binding map. This mirrors NVRHI's requirement that Vulkan binding offsets be shared across all stages in a layout and prevents one logical constant buffer from acquiring a different Vulkan binding in the vertex and pixel modules.

Sparkle uses DXC's documented [`-fspv-use-unknown-image-format`](https://github.com/microsoft/DirectXShaderCompiler/blob/main/docs/SPIR-V.rst) option because HLSL `RWTexture` element types do not carry the actual frame-graph format. Vulkan device selection explicitly requires and enables `shaderStorageImageReadWithoutFormat` and `shaderStorageImageWriteWithoutFormat`; there is no silent fallback to a mismatched typed image.

## Cleanup record

The final implementation intentionally removed:

- pass-local sky descriptor ownership;
- the temporary texture-manager sky-binding cache;
- feature binding wrappers and parameter-list macros;
- raw ordinary buffer SRV/UAV pass fields;
- descriptor bindings from renderer scene/frame data;
- framegraph-specific descriptor retirement queues;
- runtime bind/clear pass-through methods on `FrameGraphBuilder`;
- unused one-shot and persistent import APIs;
- duplicated uploaded-buffer ownership code;
- duplicate texture format string state (`PixelFormat` is authoritative);
- the unused pass-declaration sink and unlabeled declaration overloads;
- inactive transient-planning switches, duplicate physical-block metadata, and three resource-kind-specific allocation lists;
- the unused duplicate D3D12 transient-heap conversion helper;
- the parameterized constant-buffer validation macro; requirements are explicit static assertions beside each layout;
- frame-graph policy from the public RHI shader layout;
- numeric D3D12 resource-state constants from the Streamline Renderer adapter;
- broad public Renderer accessors for the RHI device and command-submission service.

## Validation and invariants

Required invariants:

- ordinary pass resources are declared through graph handles;
- no `RenderSceneData` or `FrameContext` field stores a descriptor binding;
- changed external backing resources invalidate their old generated views;
- a released view is not physically reclaimed until its frame slot is safe;
- buffer metadata matches the uploaded allocation;
- bindless exceptions are named and owner-managed;
- renderer code contains no D3D12 or Vulkan native types;
- Renderer policy branches on neutral capabilities rather than backend identity; only dedicated external-provider interop may select a third-party API payload by backend.
- RHI contains no Renderer dependency and no frame-graph lifetime policy;
- exported products cannot alias with resources whose lifetime begins before graph completion;
- every physical alias hand-off has disjoint compiled lifetimes and matching allocation contracts;
- optimized clear values are supplied only to render-target or depth-stencil textures.

Verification performed for this implementation:

- `ShowcaseEditor` DevelopmentEditor build;
- `ShaderCompiler` DevelopmentEditor build;
- D3D12 and Vulkan backend compilation;
- architecture-boundary check;
- cache-disabled cook of 28 packages / 58 jobs for DXIL SM 6.6 and SPIR-V 1.6;
- D3D12 editor runtime smoke from the Showcase project directory;
- Vulkan editor runtime smoke from the Showcase project directory with validation enabled;
- active transient alias allocation and barrier playback on both RHIs;
- zero Vulkan validation warnings or errors during the final smoke interval;
- editor startup and moving-camera visual verification in Lit, Indirect Diffuse, and Indirect Specular.

Future subresource views should extend the existing typed declaration and view-key system. They should not reintroduce descriptor-bearing scene data or a second resource-reference vocabulary.

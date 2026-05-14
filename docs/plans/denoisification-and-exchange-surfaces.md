# Denoisification And Exchange Surfaces Report

## What This Is

A readable map of the biggest noisy exchange surfaces across Sparkle modules.

The goal is to make boundaries smaller and more controlled. We want modules to trade stable, general contracts instead of passing implementation details, parallel vectors, raw backend types, or authoring metadata through the system.

## Current State At A Glance

| Area | Status | Main Issue | First Action |
| --- | --- | --- | --- |
| Tools texture requests | Noisy | Policy fields and dedup logic duplicated | Group policy and centralize request set |
| Tools cooker outputs | Noisy | Paired vectors and mixed scene build state | Add grouped output records |
| Source import boundary | Too broad | Import result exposes runtime mesh/material types | Introduce tool-side import contract |
| GameFramework scene data | Mixed | Runtime structs still carry source/upload details | Split source/import data from runtime data |
| Renderer shader params | Noisy | Binding stores every possible payload at once | Introduce binding value wrapper |
| RHI interface | Broad | One large interface exposes many subsystems | Split service facets over time |
| Core paths/math | Broad | Public utility layer mixes app/tool/runtime concerns | Separate path roles and math ownership |
| Editor UI | Leaky | Public header exposes Win32/RHI details | Hide behind editor host/viewport facade |

## Boundary Rules

Use these rules when touching any module boundary:

1. Public contracts describe intent, not implementation.
2. Runtime contracts should use cooked references, handles, and generic diagnostics.
3. Tool contracts may know source metadata, but should not leak runtime implementation structs by default.
4. RHI public contracts stay backend-neutral unless the file is explicitly backend-specific.
5. Renderer public contracts expose render products and diagnostics, not backend lifetime details.
6. Prefer grouped records over same-index vectors and multi-out-parameter APIs.
7. Keep display/source labels in tool/editor metadata, not runtime diagnostics.
8. Put shared helpers in the narrowest correct shared library.
9. Related flows should use a recognizable shape when that shape fits the domain.

## Shared Process Shape

When two systems perform the same kind of work, they should look related from the outside. This makes the architecture easier to read and easier to extend.

Use this only where it helps. Do not force identical structure onto systems that have different ownership or runtime constraints.

Good candidates for shared shape:

- cook request records
- cook output records
- import result records
- runtime load result records
- diagnostics/status records
- resource binding records

Prefer this pattern for related flows:

1. identity: stable id, display/source name only where the owner allows it
2. input: source/cooked paths, handles, or references
3. policy: options that change processing behavior
4. output: produced records, grouped by meaning
5. diagnostics: warnings/errors/status separate from payload data

## Similar Systems Found

This scan found several related process families. These are useful because once one family member is cleaned up, the same shape can guide the others.

| Family | Representative Systems | Reuse After Refactor |
| --- | --- | --- |
| Cook requests and outputs | `TextureCookRequest`, `AssetCookRequest`, `AssetCookResult`, `CookedSceneBuild`, `CookedMeshAssetBuild`, `CookedMaterialAssetBuild`, shader package cook outputs | Use one visible shape for `identity + input + policy + output + diagnostics`. |
| Import and load handoffs | `SourceImportResult`, `GameSceneLoadResult`, `SceneAssetLoadResult`, `TextureLoadResult`, shader reload results | Split `status/diagnostics` from payload, and keep source/import types separate from runtime load types. |
| Diagnostics snapshots | `TextureDiagnosticsRow/Snapshot`, `MeshDiagnosticsRow/Snapshot`, `AssetCookerDiagnosticRecord`, `ConsoleCommandResult`, RHI diagnostic messages | Use `row + snapshot` for inspectable runtime state, and `severity + message + owner id` for events/status. |
| Registries, managers, and caches | `LevelRegistry`, `SceneAssetRegistry`, `SceneAssetManager`, `CookedShaderPackageCache`, `TextureManager`, `GPUMeshCache`, `MaterialCacheManager`, `ResourceRegistry` | Keep the roles distinct: registry maps identity to location, manager owns orchestration/lifetime, cache owns residency. |
| Binding and layout records | shader reflection records, cooked shader binding records, `ShaderParameterStructBinding`, `PassParameterBinding`, RHI descriptor table bindings | Separate serialized layout records from runtime binding values, then make binding values typed and small. |
| Viewport request/product flow | `ViewportRenderRequest`, `ViewportRenderProducts`, `ViewportPanel`, `ProjectApp`, `Renderer` viewport APIs | Preserve the `request -> render -> products -> UI` flow, but keep editor intent separate from renderer implementation details. |
| Path and layout policies | `PathUtils`, `DirectoryPaths`, cook output paths, shader symbol paths, cooked asset paths | Keep generic normalization in Core and move cook/runtime layout policy into the owning module. |
| Descriptor-dispatch-result flows | `ConsoleCommandDescriptor`, `ConsoleCommandResult`, input backend results, routed input events, shader recook requests/publications | Use a consistent `descriptor/request -> dispatcher -> result/status` shape where systems execute named or routed work. |
| Build-frame products | `FrameGraphBuildResult`, `FrameBuildResult`, render scene snapshots, cooked scene build records | Keep build records as temporary products, not long-lived service state. Group produced resources by meaning. |

Use these similarities as leverage, not a mandate. If two systems share the same lifecycle, they should look familiar. If their ownership, threading, or runtime cost is different, keep the shape local and only reuse naming principles.

## Naming Standard Scan

Naming should use common engine terms for common engine processes. The main rule is not to make everything shorter; it is to make names reveal ownership and lifecycle.

### File Naming Rules

1. A file with one primary type should match that type name.
2. A file with several small related records should use the shared contract name, such as `ViewportContracts.h` or `CookedAssetCommon.h`.
3. Folder names should describe the domain or lifecycle stage, not just the implementation pattern. Good examples are `FrameGraph/Compiler`, `FrameGraph/Resources`, `SceneData/Caching`, `SceneData/Lifecycle`, `D3D12/Descriptors`, and `Assets/Loaders`.
4. Backend-specific files should carry the backend prefix when the type or file is backend-specific.
5. Runtime-facing files should avoid tool/cook wording unless they expose an explicit cooked artifact contract.

### Common Suffix Meanings

| Name Part | Use It For | Avoid Using It For |
| --- | --- | --- |
| `Desc` | declarative configuration used to create/apply something | mutable runtime state |
| `Request` | caller intent submitted to another system | completed output or persistent state |
| `Result` | status plus optional payload from one operation | long-lived records or snapshots |
| `Snapshot` | immutable state capture at a moment in time | live mutable subsystem state |
| `Registry` | identity-to-location or identity-to-descriptor catalog | resource ownership or loading orchestration |
| `Loader` | reading/translating an artifact into memory | long-lived residency or cache policy |
| `Cache` | reused/resident objects keyed by identity | broad orchestration across unrelated systems |
| `Manager` | lifecycle owner that coordinates multiple collaborators | a simple map/cache with no orchestration |
| `Builder` | constructs a record/graph/layout in memory | file IO or long-lived runtime services |
| `Factory` | creates backend/resource objects | policy decisions or high-level orchestration |
| `Context` | scoped services passed into an operation | plain POD data payloads |
| `Data` | POD payload or packed data sent across a boundary | services, ownership, or behavior |
| `Runtime` | prepared execution state derived from authored/cooked inputs | source/import metadata |

### Function Verb Meanings

| Verb | Use It For |
| --- | --- |
| `Get` | cheap accessor with no ownership transfer |
| `Find` | optional lookup that may return null/not found |
| `Resolve` | translate an id/path/request into a concrete value |
| `Ensure` | lazy initialization or validation that may mutate state |
| `Build` | construct an in-memory record, graph, layout, or payload |
| `Create` | allocate or return a new owned runtime/backend object |
| `Load` | read/translate an external artifact into memory |
| `Reload` | replace existing loaded state with a fresh version |
| `Capture` | produce a snapshot from live state |
| `Apply` | mutate the receiver from a descriptor, payload, or snapshot |
| `Submit` | hand intent to another system for processing |
| `Request` | record deferred intent without executing immediately |
| `Process` | drain or execute pending work |
| `Append` | add to an existing collection or payload |
| `Reset` | restore reusable object state but keep owned capacity/resources where useful |
| `Clear` | remove contained state/data |
| `Unload` | remove loaded runtime state by ownership policy |
| `Release` | free backend/native resources explicitly |

### Prefix And Layer Rules

1. `D3D12*` belongs in backend-specific RHI internals, ideally private RHI headers.
2. `Rhi*` belongs on backend-neutral RHI value types and opaque handles.
3. `Native*` is acceptable for opaque native handles passed across module seams.
4. `Cooked*` is a good artifact-layer word for serialized cooked file formats and cooked asset references.
5. Avoid `Cooked*` on live runtime managers/caches unless the object is specifically about the cooked artifact, not the loaded runtime view.
6. Prefer industry terms like `FrameGraph`, `RenderPass`, `PipelineState`, `DescriptorHeap`, `ShaderReflection`, `ShaderPackage`, `Scene`, `Level`, `Snapshot`, `Registry`, `Loader`, and `Cache` when they fit.

### Renderer Naming Findings

| Current Name | Current Read | Naming Direction | Priority |
| --- | --- | --- | --- |
| `FrameGraph`, `PassBuilder`, `RenderGraphPassContext`, `ResourceHandle`, `GBufferPass`, `DirectLightingPass` | Uses recognizable render-graph/pass terms | Keep | Keep |
| `RenderPassContext` | scoped services passed to authored pass execution | Keep as `Context`, or rename to `RenderPassExecutionContext` only if context names keep colliding | Later |
| `RenderViewContext` | POD per-view constants, GPU address, viewport, scissor | Prefer `RenderViewData` or `PerViewRenderData`; this is data, not services | Rename when touching frame builders |
| `MaterialCacheManager` | builds/caches material runtime data and descriptor tables | Prefer `MaterialResourceCache` if it is mainly cache/residency; use `MaterialResourceManager` only if orchestration grows | Rename with material cache cleanup |
| `SceneRenderStateCoordinator` | subscribes to level events and refreshes renderer scene state | Prefer `RenderSceneStateSynchronizer` or `SceneRenderStateLifecycle`; `Coordinator` is too vague | Rename with lifecycle cleanup |
| `PipelineStateManager` | owns pass runtimes, shader package cache, and pipeline-state creation | Keep for now; `PipelineStateCache` would only fit if it becomes a pure PSO cache | Keep |
| `RuntimeManager` member inside `RenderPassContext` | actually a `PipelineStateManager` | Rename field to `PipelineStates` or `PipelineStateManager` when touching this file | Small cleanup |

Renderer naming rule: `Context` should mean services/scope, `Data` should mean payload, `Cache` should mean keyed residency/reuse, and `Manager` should mean lifecycle orchestration.

### RHI Naming Findings

| Current Name | Current Read | Naming Direction | Priority |
| --- | --- | --- | --- |
| `RenderHardwareInterface`, `RendererBackendServices`, `RenderBindingLayout`, `RenderPipelineState` | clear backend-neutral RHI/API names | Keep | Keep |
| `D3D12DescriptorHeapManager`, `D3D12DescriptorAllocator`, `D3D12RootSignatureBuilder`, `D3D12PipelineState` | clear backend-specific implementation names | Keep private/backend-scoped | Keep |
| `CookedShaderPackageHeader`, `CookedShaderBinaryRecord`, `CookedShaderReflectionRecord` | serialized cooked shader package records | Keep `Cooked*` while these are file-format/POD artifact records | Keep for artifact layer |
| `CookedShaderPackageCache` | runtime cache of loaded shader packages | Prefer `ShaderPackageCache` if the cache becomes a runtime view rather than a cooked-file reader | Later |
| `LoadedShaderPackage` | loaded runtime view of a package | Good contrast with cooked records | Keep |
| `CookedShaderResourceKind`, `CookedShaderScalarType`, `CookedShaderResourceBindingRecord` | reflection metadata from cooked artifact | Consider `ShaderResourceKind`, `ShaderScalarType`, `ShaderResourceBindingRecord` if reflection is promoted as runtime metadata | Later |
| `TextureLoadResult` under public `D3D12/Textures` | backend-shaped but generic-looking name | Move private, or rename to `D3D12TextureLoadResult` if it must stay public/backend-specific | Later |
| `CookedTextureAsset` under public `D3D12/Textures` | cooked artifact plus backend-specific texture interpretation | Keep only if this remains the explicit D3D12 cooked texture contract | Review with texture format abstraction |

RHI naming rule: keep `Cooked*` for serialized artifact records, use `Loaded*` or plain domain names for runtime views, and keep backend names explicit when a type is backend-specific.

### GameFramework Naming Findings

| Current Name | Current Read | Naming Direction | Priority |
| --- | --- | --- | --- |
| `GameScene`, `SceneLighting`, `SceneMeshes`, `SceneMaterials`, `SceneTextures`, `SceneCamera` | clear scene subsystem ownership | Keep | Keep |
| `LightingSnapshot`, `MeshSnapshot`, `MaterialSnapshot`, `TextureSnapshot`, `CameraSnapshot` | clear immutable capture terms | Keep | Keep |
| `LevelAsset`, `LevelRegistry`, `LevelChangeEvents` | clear level identity/catalog/lifecycle event terms | Keep | Keep |
| `LevelManager` | owns startup level, pending level change, scene apply/capture, save/load | Acceptable now, but broad; split or rename only when responsibilities split | Later |
| `SceneAssetManager` | loads scene assets using registry and builds runtime payload | Prefer `SceneAssetLoader` if it remains loading-only; keep `Manager` if it owns loaded asset lifetime | Later |
| `RuntimeScenePayload` | runtime scene handoff payload | Good if it stays runtime-only after source/import data is removed | Keep after cleanup |
| `MaterialDesc`, `MeshData` | standard names but currently cross source/runtime boundaries | Naming is acceptable; ownership/layering is the real issue | Fix through boundary refactor |
| `ApplyFromDesc` vs `ApplyDesc` | mixed function naming for applying descriptors | Standardize on one verb, preferably `ApplyFromDesc` because it already appears on scene-level APIs | Small cleanup |

GameFramework naming rule: `Scene*` owns live scene state, `Level*` owns level identity/lifecycle, `*Snapshot` is read-only capture, `*Desc` is declarative input, and `Runtime*Payload` should contain runtime-only data.

### Naming Actions To Carry Into Refactors

1. When denoising a type, check whether its suffix matches the suffix table before changing behavior.
2. Rename files with their primary class/struct when the primary type is renamed.
3. Do not rename broad public shader/cooked types as a separate churn pass; rename them only when the shader package or cooked artifact boundary is already being touched.
4. Prefer small local renames during related refactors: `RenderViewContext`, `MaterialCacheManager`, `SceneRenderStateCoordinator`, `SceneAssetManager`, and `ApplyDesc`/`ApplyFromDesc` consistency.
5. Preserve good established names: render passes, frame graph, D3D12 backend internals, GameFramework scene subsystem names, snapshots, registries, and level events.

### NVIDIA And AMD Pattern Benchmark

This pass compared Sparkle terminology against representative NVIDIA Falcor and AMD Cauldron/FidelityFX SDK patterns. The goal is not to copy their style wholesale; it is to avoid surprising names for well-known rendering concepts.

| Pattern | NVIDIA / AMD Shape | Sparkle Status | Biggest Sparkle Issue | Direction |
| --- | --- | --- | --- | --- |
| Render graph | Falcor uses `RenderGraph`, `RenderPass`, `RenderPassReflection`, `RenderGraphCompiler`, `ResourceCache` | Sparkle uses `FrameGraph`, `PassBuilder`, `RenderGraphPassContext`, `FrameGraphCompiler`, `ResourceRegistry` | Mostly aligned; `ResourceRegistry` may imply catalog rather than render-graph resource cache/resolution | Keep `FrameGraph`; review whether `ResourceRegistry` should become `FrameGraphResourceRegistry` or `FrameGraphResourceCache` based on ownership. |
| Pass execution payload | Falcor passes `RenderContext` plus `RenderData`; AMD render modules use `Init`/`Execute(CommandList*)` | Sparkle has `RenderPassContext`, `RenderGraphPassContext`, `FrameContext`, `RenderViewContext` | Too many `Context` names; `RenderViewContext` is really POD data | Rename data-only contexts toward `Data`; reserve `Context` for scoped services. |
| Resource cache/pool/heap | Falcor `ResourceCache`; AMD `DynamicBufferPool`, `StaticBufferPool`, `UploadHeap`, `CommandListRing` | Sparkle has `TextureManager`, `GPUMeshCache`, `MaterialCacheManager`, `D3D12ConstantBufferManager`, transient allocator types | `Manager` hides whether a type is a cache, pool, allocator, or lifecycle owner | Prefer `Cache`, `Pool`, `Heap`, `Ring`, or `Allocator` when that is the real role. |
| Device and command abstractions | AMD/FidelityFX expose backend-neutral `Device`, `CommandList`, `SwapChain`, `ResourceView`; NVIDIA uses `Device` and `RenderContext` | Sparkle exposes broad `RenderHardwareInterface`, opaque native handles, and backend services | Public RHI is one large service surface instead of recognizable device/command/resource-view facets | Split future RHI facets around `Device`, `CommandList`, `SwapChain`, `ResourceView`/descriptor allocation, and diagnostics. |
| Resource views/descriptors | AMD uses `ResourceView`, `ResourceViewInfo`, `RasterView`, descriptor allocators; D3D12 internals name descriptor heaps explicitly | Sparkle uses `RhiDescriptorTableHandle`, `RhiDescriptorAllocation`, `NativeDescriptorHeapHandle`, D3D12 descriptor heap classes | Low-level descriptor names leak broadly; higher-level resource-view intent is less visible | Keep D3D12 descriptor names backend-private; expose resource-view intent at renderer/RHI seams when possible. |
| Swap chain and presentation | AMD has `SwapChain`, `SwapChainRenderTarget`, `GetBackBufferRTV`, `PresentSwapChain`; Falcor graph outputs are explicit | Sparkle has present helpers and `ViewportRenderProducts` | Presentation/viewport product terminology is custom and may hide standard swap-chain/back-buffer concepts | Keep viewport products for editor outputs, but use `SwapChain`, `BackBuffer`, `RenderTarget`, and `Present` terminology at presentation seams. |
| Render modules/passes | AMD Cauldron uses `RenderModule::Init` and `Execute`; Falcor uses pass `reflect`, `compile`, `execute` | Sparkle authored passes and frame graph are close to standard render-pass wording | No major issue | Preserve pass names and lifecycle verbs; consider `Execute` consistently for pass runtime work. |
| Scene and level | Falcor uses `Scene`; AMD samples mostly focus on render framework/resource abstractions | Sparkle has `GameScene`, `Scene*` subsystems, `Level*` lifecycle | Scene names are good; `LevelManager` remains broad | Keep scene subsystem naming; split or rename `LevelManager` only when responsibilities split. |

Biggest issues to fix first:

1. `RenderViewContext`: rename to `RenderViewData` or `PerViewRenderData` when frame builders are touched.
2. `MaterialCacheManager`: rename toward `MaterialResourceCache` if it remains a keyed material/runtime resource cache.
3. `SceneRenderStateCoordinator`: rename toward `RenderSceneStateSynchronizer` or `SceneRenderStateLifecycle` when level-change renderer state is refactored.
4. `RenderHardwareInterface`: do not expand this broad interface; new work should move toward recognizable `Device`/`CommandList`/`SwapChain`/`ResourceView`/diagnostics facets.
5. `ResourceRegistry`: review against Falcor-style `ResourceCache`; use `Registry` only if it catalogs declarations/identity, and `Cache` if it owns reusable resources.
6. Public D3D12 texture load contracts: either move private/backend-specific, or make the backend name explicit in the type.
7. `Cooked*` runtime naming: keep it for artifact records, but prefer `ShaderPackage`, `LoadedShaderPackage`, or `ShaderPackageCache` for runtime views.

## Concrete Denoise Blocks

### Block 1: Texture Cook Request Contract

**Area:** Tools

**Files:**

- `Tools/TextureCooker/Public/TextureCookRequestList.h`
- `Tools/TextureCooker/Private/Requests/TextureCookRequestList.cpp`
- `Tools/MaterialCooker/Private/MaterialCooker.cpp`
- `Tools/AssetCooker/Private/Dispatch/AssetCookerDispatcher.cpp`

**Where we are:**

`TextureCookRequest` carries identity, source path, output path, color space, mip policy, mip filter, processing policy, texture group, dimension, and channel mask all at the same level. AssetCooker has its own full-field comparison and dedup logic. MaterialCooker dedups differently by raw `TextureAssetId` only.

**Why it is noisy:**

The same texture request meaning is reconstructed in multiple places. Policy fields are not named as a policy, and conflict handling is inconsistent.

**Denoise target:**

```cpp
struct TextureCookPolicy
{
    TextureColorSpace colorSpace;
    TextureMipPolicy mipPolicy;
    TextureMipFilter mipFilter;
    TextureColorProcessingPolicy colorProcessingPolicy;
    TextureGroup textureGroup;
    TextureDimension dimension;
    TextureChannelMask channelMask;
};

struct TextureCookRequest
{
    TextureAssetId assetId;
    std::filesystem::path sourcePath;
    std::filesystem::path outputPath;
    TextureCookPolicy policy;
};
```

Add a `TextureCookRequestSet` in `TextureCookShared` to own dedup and conflict diagnostics.

**First move:**

Group policy fields and replace `AssetCookerTextureCookRequestsMatch` with shared equality/conflict helpers.

**Validation:**

- No local request compare helper in AssetCooker.
- No by-id-only dedup in MaterialCooker.
- Texture request summaries produce the same intended outputs.

### Block 2: Mesh And Material Cooker Outputs

**Area:** Tools

**Files:**

- `Tools/MeshCooker/Public/MeshCooker.h`
- `Tools/MaterialCooker/Public/MaterialCooker.h`
- `Tools/MeshCooker/Private/MeshCooker.cpp`
- `Tools/MaterialCooker/Private/MaterialCooker.cpp`
- `Tools/AssetCooker/Private/Dispatch/AssetCookerDispatcher.cpp`
- `Tools/AssetConverter/Private/Cli/AssetConverterCommands.cpp`

**Where we are:**

MeshCooker and MaterialCooker return paired vectors through out parameters: assets and scene references. Callers must keep the same-index relationship in their heads.

**Why it is noisy:**

The API tells callers implementation mechanics instead of returning one meaningful result.

**Denoise target:**

```cpp
struct MeshCookOutput
{
    std::vector<CookedMeshAssetBuild> assets;
    std::vector<Assets::CookedSceneMeshAssetRef> references;
};

struct MaterialCookOutput
{
    std::vector<CookedMaterialAssetBuild> assets;
    std::vector<Assets::CookedSceneMaterialAssetRef> references;
};
```

**First move:**

Change build APIs to fill one output record each.

**Validation:**

- No `outMeshAssets/outMeshAssetReferences` API pairs remain.
- No `outMaterialAssets/outMaterialAssetReferences` API pairs remain.

### Block 3: Cooked Scene Build Record

**Area:** Tools

**Files:**

- `Tools/SceneCooker/Public/CookedSceneBuild.h`
- `Tools/SceneCooker/Private/SceneCooker.cpp`
- `Tools/AssetCooker/Private/Dispatch/AssetCookerDispatcher.cpp`

**Where we are:**

`CookedSceneBuild` contains scene identity, manifest path, manifest header, mesh references, material references, instances, mesh assets, material assets, and error state in one flat struct.

**Why it is noisy:**

It mixes identity, payload, manifest data, generated assets, and failure state. The struct is both a result and a workspace.

**Denoise target:**

```cpp
struct CookedSceneIdentity { /* scene id and manifest path */ };
struct CookedSceneManifestBuild { /* header, references, instances */ };
struct CookedSceneAssetOutputs { MeshCookOutput meshes; MaterialCookOutput materials; };
struct CookedSceneBuild { CookedSceneIdentity identity; CookedSceneManifestBuild manifest; CookedSceneAssetOutputs outputs; };
```

Move status/error into a result wrapper or at least a separate status field.

**First move:**

Do this after Block 2 so scene build can consume grouped mesh/material outputs.

**Validation:**

- Manifest write behavior unchanged.
- Scene registry output path unchanged.
- Instance counts and reference counts still match.

### Block 4: Source Import Contract

**Area:** Tools / GameFramework boundary

**Files:**

- `Tools/SourceImportAdapters/Public/SourceImportResult.h`
- `Tools/SourceImportAdapters/Private/Fbx/*`
- `Tools/SourceImportAdapters/Private/Gltf/*`
- `Engine/GameFramework/Public/Scene/Materials/MaterialDesc.h`
- `Engine/GameFramework/Public/Scene/Meshes/MeshData.h`

**Where we are:**

`SourceImportResult` is structurally better after the recent refactor, but it still exposes runtime-flavored types: `MeshData`, `MaterialDesc`, `MaterialHandle`, `Transform`, `TextureGroup`, and `TextureChannelMask`.

**Why it is noisy:**

Source import is an authoring/tool concern. Runtime scene and GPU upload structures should not be the permanent import exchange model.

**Denoise target:**

```cpp
struct ImportedMesh { /* geometry, displayName, nodeTransform, materialIndex */ };
struct ImportedMaterial { /* properties and sourceTextureBindings */ };
struct ImportedScene { /* meshes, materials, sourceScenePath, importerType */ };
```

The cookers translate this into runtime cooked records.

**First move:**

Do not churn this before Blocks 1-3. Start by adding the new contract beside the old one, then migrate importers and cookers.

**Validation:**

- SourceImportAdapters public headers no longer need broad GameFramework scene/material/mesh headers.
- Import output is source/cook domain, not runtime scene domain.

### Block 5: GameFramework Runtime Scene Data

**Area:** GameFramework

**Files:**

- `Engine/GameFramework/Public/Scene/RuntimeScenePayload.h`
- `Engine/GameFramework/Public/Scene/Materials/MaterialDesc.h`
- `Engine/GameFramework/Public/Scene/Meshes/MeshData.h`
- `Engine/GameFramework/Private/Assets/SceneAssetManager.cpp`
- `Engine/GameFramework/Private/Scene/GameScene.cpp`

**Where we are:**

`RuntimeScenePayload` has been improved with grouped mesh instances. Remaining broad areas are `MaterialDesc`, which still carries source texture paths, and `MeshData`, which exposes exact vertex upload layout.

**Why it is noisy:**

GameFramework currently straddles authoring data, cooked data, and runtime scene data. That makes it easy for source paths or GPU layout details to bleed into places that only need handles or cooked references.

**Denoise target:**

- `MaterialDesc`: source/import material description.
- `RuntimeMaterialSnapshot`: runtime material data with cooked texture refs or handles.
- `MeshData`: eventually private cooked/load-time geometry, not a general gameplay contract.

**First move:**

Wait until Block 4 gives cookers a better import contract. Then split material source paths away from runtime material state.

**Validation:**

- Runtime material loading does not depend on source texture paths.
- Renderer receives material facts and texture handles/refs, not import paths.

### Block 6: Renderer Shader Parameter Binding

**Area:** Renderer / RHI boundary

**Files:**

- `Engine/Renderer/Public/ShaderParameters/PassParameterSet.h`
- `Engine/Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h`
- `Engine/Renderer/Private/FrameGraph/Builder/PassBuilder.cpp`
- `Engine/Renderer/Private/Pipeline/PassBinder.cpp`
- `Engine/Renderer/Private/Passes/ShaderPass.h`

**Where we are:**

`PassParameterBinding` stores every possible binding payload at once: textures, buffers, descriptor table, acceleration structure address, uniform data, sampler, plus a `Kind` discriminator.

**Why it is noisy:**

Every binding object carries fields that are invalid for most binding kinds. Setters repeat validation and reset logic.

**Denoise target:**

```cpp
struct TextureBindingValue { std::vector<TextureHandle> handles; };
struct BufferBindingValue { std::vector<BufferHandle> handles; };
struct DescriptorTableBindingValue { RhiDescriptorTableBinding table; };
```

Then `PassParameterBinding` owns one active value shape.

**First move:**

Refactor binding storage after the tool request cleanup. Keep public behavior unchanged.

**Validation:**

- Missing-binding diagnostics unchanged.
- FrameGraph resource declarations unchanged.
- PassBinder binding behavior unchanged.

### Block 7: Renderer Viewport Products

**Area:** Renderer / Application / Editor boundary

**Files:**

- `Engine/Renderer/Public/Viewport/ViewportContracts.h`
- `Engine/Application/Public/ProjectApp.h`
- `Engine/Editor/Public/UI.h`

**Where we are:**

`ViewportRenderProducts` has `AvailableOutputs` plus fixed product fields like `SceneColor`, `SceneDepth`, `ObjectId`, `Normals`, and `OverlayMask`.

**Why it is somewhat noisy:**

Flags and fields must stay in sync. The current shape is acceptable while the output set is small, but it can drift as more outputs are added.

**Denoise target:**

Either add helper accessors that enforce the flag/field relationship, or move to a compact product-list model later.

**First move:**

Low priority. Do not block tool or shader-parameter cleanup on this.

**Validation:**

- Editor viewport still receives scene color and auxiliary buffers.
- Request flags and available flags remain separate concepts.

### Block 8: RHI Interface Facets

**Area:** RHI / Renderer boundary

**Files:**

- `Engine/RHI/Public/Interop/RenderHardwareInterface.h`
- `Engine/RHI/Public/Interop/RendererBackendServices.h`
- `Engine/RHI/Public/Resources/TextureTypes.h`
- `Engine/RHI/Public/D3D12/Textures/TextureLoadResult.h`
- `Engine/RHI/Public/D3D12/Textures/CookedTextureAsset.h`

**Where we are:**

`RenderHardwareInterface` is a broad service interface. It exposes diagnostics, descriptor allocation, buffer creation, texture loading, timing, messages, present helpers, binding layout creation, and command-list concerns. RHI public texture types still include DXGI-oriented details in some places.

**Why it is noisy:**

Renderer sees many backend services through one large surface. Texture format concepts are partly backend-specific.

**Denoise target:**

Split over time into narrower facets:

- resource creation/upload
- descriptor allocation
- shader/binding layout services
- diagnostics
- presentation

For texture formats, prefer engine-level format codes at cooked/runtime boundaries and map to backend formats inside the backend.

**First move:**

Do not start here. First remove tool-side texture request noise, then revisit texture format abstraction.

**Validation:**

- Renderer public headers remain free of concrete D3D12 backend objects.
- Backend-specific headers stay in backend-specific folders.

### Block 9: Core Path And Math Utilities

**Area:** Core

**Files:**

- `Engine/Core/Public/Paths/DirectoryPaths.h`
- `Engine/Core/Public/FileSystemUtils.h`
- `Engine/Core/Public/Paths/PathUtils.h`
- `Engine/Core/Public/Math/MathUtils.h`
- `Engine/Core/Public/Strings/StringUtils.h`

**Where we are:**

Core owns broad path helpers for workspace, build output, cooked output, shader symbols, and assets. Core math/string helpers include DirectX-flavored convenience APIs.

**Why it is noisy:**

Core risks becoming the place where every subsystem puts path policy. Some path helpers are runtime concerns, some are tool concerns, and some are build/artifact layout concerns.

**Denoise target:**

- Keep generic path normalization in Core.
- Move cook/tool layout policy to tool-side helpers.
- Keep runtime asset path resolution separate from build/cook output layout.
- Avoid adding more subsystem-specific policy to Core path headers.

**First move:**

When Block 1 or Block 3 needs path helpers, create the narrow helper in the owning tool module rather than expanding `DirectoryPaths.h`.

**Validation:**

- Core helpers stay generic.
- Tool-only paths do not become required by runtime modules.

### Block 10: Editor UI Public Surface

**Area:** Editor / Application / Renderer / Platform boundary

**Files:**

- `Engine/Editor/Public/UI.h`
- `Engine/Application/Public/EditorApp.h`
- `Engine/Application/Public/ProjectApp.h`

**Where we are:**

`UI.h` publicly includes `Windows.h`, RHI handles, and diagnostics provider function types. It exposes `ProcessWindowMessage(HWND, UINT, WPARAM, LPARAM)` and `Render(NativeGraphicsCommandListHandle)`.

**Why it is noisy:**

The public editor UI surface is platform/backend-shaped. That makes UI composition depend on Win32 and RHI details.

**Denoise target:**

Introduce a smaller editor host interface:

- window events as engine/platform events, not raw Win32 in public UI
- viewport render products as renderer contracts
- diagnostics as a grouped provider object
- backend render entry hidden in private UI implementation

**First move:**

Defer until renderer/tool cleanup is stable. This is a real boundary issue, but it is not the first productivity blocker.

**Validation:**

- Editor public headers no longer include `Windows.h`.
- Editor UI public surface no longer exposes native command-list handles.

### Block 11: AssetCooker Bridge API

**Area:** Tools external/API boundary

**Files:**

- `Tools/AssetCooker/Public/AssetCookRequest.h`
- `Tools/AssetCooker/Public/AssetCookResult.h`
- `Tools/AssetCooker/Public/AssetCookerTypes.h`
- `Tools/AssetCooker/Private/Api/AssetCookerService.*`
- `Tools/AssetCooker/Private/Cli/AssetCookerCli.cpp`

**Where we are:**

AssetCooker public API uses C-style pointer structs and exposes categories, diagnostic records, output paths, and reload hints.

**Why it is noisy:**

This is fine as a narrow ABI bridge, but not ideal as the main internal model. Pointer lifetime is unclear and tool-specific details are exposed early.

**Denoise target:**

Keep bridge structs at the API edge. Add owning internal C++ request/result records and convert at the boundary.

**First move:**

After Blocks 1-3, isolate C-style API use to `Private/Api` and CLI boundaries.

**Validation:**

- Internal cooker code uses owning request/result records.
- Public C bridge remains small and documented.

## Recommended Order

### Wave 1: Tool Exchange Cleanup

1. Block 1: Texture cook request contract.
2. Block 2: Mesh/material cooker outputs.
3. Block 3: Cooked scene build record.

This wave removes immediate duplicated plumbing and makes the cook pipeline easier to reason about.

### Wave 2: Import And Runtime Data Separation

1. Block 4: Source import contract.
2. Block 5: GameFramework runtime scene data.

This wave separates authoring/import data from runtime/cooked data.

### Wave 3: Renderer And RHI Surfaces

1. Block 6: Renderer shader parameter binding.
2. Block 8: RHI interface facets.
3. Block 7: Viewport products, if the product list grows.

This wave shrinks runtime exchange surfaces without mixing them into cook-tool churn.

### Wave 4: Core And Editor Hygiene

1. Block 9: Core path/math utility ownership.
2. Block 10: Editor UI public surface.
3. Block 11: AssetCooker bridge API.

These are real, but they are best handled after the major data-flow contracts are cleaner.

## Suggested First Slice

Start with Block 1 only.

Why:

- It is concrete and small.
- It hits a duplicated real bug-risk area.
- It does not require a cooked binary format change.
- It makes later cooker-output grouping easier.

Definition of done:

- `TextureCookPolicy` or equivalent exists.
- `TextureCookRequestSet` or equivalent owns dedup/conflict checks.
- AssetCooker and MaterialCooker use the same dedup path.
- No local full-field texture request comparison remains in AssetCooker.

## Validation Checklist

Use this after every block:

1. `git diff --check`
2. targeted stale-symbol searches
3. dependency review for new public links/includes
4. boundary grep for editor/source metadata leaking into RHI, Renderer, or GameFramework diagnostics
5. shape review for related systems that should share request/output/status structure
6. targeted build/cook only when requested or when static checks cannot prove the change

## Non-Goals For The First Pass

1. Do not introduce a global `Result<T>` conversion yet.
2. Do not move texture request contracts into `CookCommon`.
3. Do not combine editor metadata catalog work with denoisification.
4. Do not change cooked binary formats as a side effect.
5. Do not start with RHI or Editor UI; they are important, but not the current highest-leverage first slice.
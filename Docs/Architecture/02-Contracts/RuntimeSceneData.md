# Runtime Scene Data Contract

## Purpose

This document defines the runtime scene-data boundary between `SparkleGameFramework` and `SparkleRenderer`. The goal is to keep gameplay/runtime ownership clear while allowing the renderer to consume stable frame snapshots without coupling GameFramework to RHI, backend-native APIs, or renderer-private caches.

This is a source-backed contract. If a behavior is not fully proven by the reviewed source, it is marked `Needs source confirmation`.

## Non-Goals

- This document does not change GameFramework or Renderer code.
- This document does not add new runtime scene features.
- This document does not create Renderer or RHI dependencies in GameFramework.
- This document does not redefine renderer frame-graph or RHI contracts.

## GameFramework Responsibilities

`SparkleGameFramework` owns the mutable runtime world.

Current source-backed GameFramework responsibilities include:

- level loading and active level state through `GameScene`, `LevelDesc`, and `LevelAsset`
- mutable scene subsystems:
  - cameras
  - animations
  - lighting
  - materials
  - material variants
  - meshes
  - skeletons
  - textures
- scene controllers and update policy through `GameSceneController`
- scene asset and cooked asset loading through `SceneAssetManager` and `SceneAssetRegistry`
- cooked asset identifiers and manifests
- runtime animation playback, pose generation, and morph-weight evaluation

Current source-backed module boundary:

- `SparkleGameFramework` links only `SparkleCore` and `SparklePlatform`
- it does not depend on `SparkleRenderer`
- it does not depend on `SparkleRHI`

Contract rule:

- GameFramework owns runtime scene truth
- GameFramework must remain renderer-agnostic and RHI-agnostic

## Renderer Responsibilities

`SparkleRenderer` owns render-time translation, caching, and GPU-facing scene preparation.

Current source-backed renderer responsibilities in this boundary include:

- capturing a renderer-owned `RenderSceneSnapshot` from `GameSceneSnapshot`
- building `RenderSceneData` from snapshot data
- translating material descriptions into renderer-side `MaterialData`
- translating mesh snapshots into renderer-side `MeshDraw` and `MeshInstanceBatch`
- uploading or resolving GPU mesh state through `GPUMeshCache`
- maintaining previous-frame world and skinning data for temporal rendering
- building renderer-side lighting arrays
- maintaining renderer-owned material and texture cache state
- coordinating scene-scoped renderer invalidation through `SceneRenderStateCoordinator`

Contract rule:

- Renderer owns GPU-facing scene derivation
- Renderer does not own gameplay scene mutation

## Asset Ownership

GameFramework owns runtime asset identity and asset payload composition.

Current source-backed asset ownership surfaces:

- `SceneAssetManager`
- `SceneAssetRegistry`
- `SceneAssetPayload`
- `SceneAssetId`
- `AssetId`
- cooked asset ids through `Assets::CookedAssetId`

Current source-backed responsibilities:

- registry load/save/resolve
- loading one or more scene assets into a `SceneAssetPayload`
- appending payload data into `GameScene`

Contract rule:

- GameFramework owns asset discovery, load, translation to runtime scene descriptions, and append semantics
- Renderer may consume the resulting scene snapshots, not raw asset-loader internals

## Cooked Asset Ownership

GameFramework also owns cooked runtime asset formats for scene content.

Current source-backed cooked asset ownership surfaces include:

- `CookedMeshAsset`
- `CookedMaterialAsset`
- `CookedTextureReference`
- `CookedSkeletonAsset`
- `CookedAnimationAsset`
- `CookedSceneManifest`
- `CookedSceneCameraRecord`
- `CookedSceneLightRecord`

Current source-backed ID model:

- `Assets::CookedAssetId` is a `std::uint64_t`
- `Assets::InvalidCookedAssetId` is `0`

Contract rule:

- GameFramework owns cooked scene/content asset decoding and runtime representation
- Renderer may rely on stable cooked asset ids and snapshot references, but it must not own cooked asset loading policy

## Scene Mutation Model

`GameScene` is the mutable runtime scene root.

Current source-backed mutation entry points include:

- `LoadLevel(...)`
- `Update(float deltaSeconds)`
- `AppendSceneAssetPayload(...)`
- `Clear()`
- mutable accessors to cameras, lighting, materials, variants, meshes, textures, skeletons, and animations
- scene controllers via `GameSceneController`

Current update sequence in `GameScene::Update(...)`:

1. run controllers in `PreAnimation`
2. update animations against skeletons
3. apply morph weights to meshes
4. run controllers in `PostAnimation`

Contract rule:

- mutation belongs to GameFramework-owned systems and controllers
- renderer code must not mutate `GameScene` as part of normal rendering

## Frame Snapshot Model

The key scene-to-render seam is snapshot capture.

Current source-backed flow:

1. `GameScene::CaptureSnapshot()` produces a `GameSceneSnapshot`
2. `RenderSceneSnapshot::Capture(GameSceneSnapshot&&)` moves snapshot data into renderer-owned storage
3. `RenderSceneDataBuilder::Build(const RenderSceneSnapshot&)` derives renderer-facing scene data

Current `GameSceneSnapshot` fields:

- `CameraSnapshot camera`
- `SceneAnimationSnapshot animations`
- `LightingSnapshot lighting`
- `TextureSnapshot textures`
- `MeshSnapshot meshes`
- `MaterialSnapshot materials`

Current `RenderSceneSnapshot` fields mirror those same value types.

Contract rule:

- GameFramework exports frame-readable snapshot values
- Renderer consumes snapshots and derives render-only structures from them
- the snapshot is the intended boundary between mutable runtime data and renderer-consumed frame data

## Materials, Meshes, Textures, Cameras, Lighting, Skeletons

### Materials

GameFramework ownership:

- `SceneMaterials` stores `std::vector<MaterialDesc>`
- `MaterialHandle` is a GameFramework-facing material slot handle
- `MaterialSnapshot` contains `std::vector<MaterialDesc>`

Renderer ownership:

- `MaterialCacheManager` and `MaterialCacheUtils` derive renderer-side cache state
- `MaterialData` stores renderer-facing values plus `const RenderBindingSet* textureBindingSet`

Contract rule:

- GameFramework owns authored/runtime material descriptions
- Renderer owns material cache realization and binding-set pointers

### Meshes

GameFramework ownership:

- `SceneMeshes` stores `std::vector<std::unique_ptr<MeshComponent>>`
- `MeshSnapshot` contains `MeshInstanceSnapshot` and `MeshInstanceGroupSnapshot`
- mesh snapshots carry:
  - `const Mesh*`
  - world transform
  - inverse transpose
  - material handle
  - cooked mesh asset id
  - cooked skeleton asset id
  - mesh kind
  - asset/group indices

Renderer ownership:

- `RenderSceneDataBuilder` converts snapshots into `MeshDraw`
- renderer adds:
  - previous world matrix
  - GPU mesh pointer
  - resolved material slot
  - joint-matrix offsets
  - batching data

Contract rule:

- GameFramework owns mesh component lifetime and authored/runtime mesh identity
- Renderer owns GPU mesh upload/cache state and render batching

### Textures

GameFramework ownership:

- `SceneTextures` stores texture paths
- `TextureSnapshot` contains `std::vector<std::filesystem::path> texturePaths`
- texture references are gathered from material descriptions and explicit texture paths

Renderer ownership:

- `TextureManager` loads scene textures for rendering
- renderer unloads scene-scoped textures through `SceneRenderStateCoordinator`

Contract rule:

- GameFramework owns texture references as runtime content references
- Renderer owns texture residency and render-time texture bindings

### Cameras

GameFramework ownership:

- `SceneCameras`
- `CameraSnapshot` with position, direction, FOV, aspect, near/far

Renderer ownership:

- `RenderCamera`
- render-view constant buffer data and per-view matrices

Contract rule:

- GameFramework owns camera state as gameplay/runtime data
- Renderer derives matrices and render-time camera constants from snapshots

### Lighting

GameFramework ownership:

- `SceneLighting`
- `LightingSnapshot`
- authored/runtime light descriptors

Renderer ownership:

- `RenderLightingBuilder`
- renderer-facing directional/point/spot light arrays

Contract rule:

- GameFramework owns light descriptions
- Renderer owns render-time packed light arrays

### Skeletons And Animation

GameFramework ownership:

- `SceneSkeletons`
- `SceneAnimations`
- animation clips, channels, keyframes, playback state, active poses, and active morph weights
- `SceneAnimationSnapshot`

Renderer ownership:

- joint matrix packing into renderer scene data
- previous joint-matrix history for temporal rendering
- per-draw skinning offsets

Contract rule:

- GameFramework owns animation evaluation and skeleton data
- Renderer owns frame-local GPU-facing skinning data derived from snapshot poses

## Threading And Update Assumptions

Current reviewed source suggests a straightforward host-thread update model.

Source-backed observations:

- `GameScene::Update(...)` mutates scene state directly
- `GameScene::CaptureSnapshot()` returns snapshot values synchronously
- renderer captures a snapshot during frame setup and then builds render scene data from it
- `SceneRenderStateCoordinator` reacts to level lifecycle events and flushes renderer caches synchronously

`Needs source confirmation`:

- formal thread ownership guarantees for scene mutation versus render snapshot capture
- whether any scene subsystem is intended to be safely mutated concurrently with snapshot capture

Current contract statement:

- assume scene mutation and snapshot capture are coordinated on the host thread unless future source proves a stronger threading contract

## IDs, Handles, And Lifetime Expectations

Current source-backed identifier and handle surfaces include:

- `AssetId`
- `Assets::CookedAssetId`
- `MaterialHandle`
- scene mesh asset/group/instance indices
- `const Mesh*` in `MeshInstanceSnapshot`

Contract rules:

1. GameFramework owns content ids and runtime handles.
2. Snapshot handles and pointers are only valid for snapshot consumption rules visible in current code paths.
3. Renderer may translate GameFramework ids and handles into renderer-local slots, caches, and GPU pointers.
4. Renderer-local pointers such as `const GPUMesh*` and `const RenderBindingSet*` must never leak back into GameFramework contracts.

`Needs source confirmation`:

- any stronger lifetime guarantee for `const Mesh*` beyond the normal snapshot-to-render build flow

## What Renderer May Read

Renderer may read:

- `GameSceneSnapshot`
- `RenderSceneSnapshot`
- camera snapshot data
- animation pose and morph-weight snapshot data
- lighting snapshot data
- texture paths
- mesh instance and mesh instance group snapshots
- material snapshot data
- cooked asset ids and runtime handles embedded in snapshot types

Renderer may also read `GameScene` indirectly where a renderer-owned coordinator is explicitly wired to the scene, for example:

- `SceneRenderStateCoordinator`

But even there, the renderer should treat GameFramework state as source data, not as renderer-owned mutable storage.

## What Renderer Must Not Mutate

Renderer must not mutate:

- `GameScene` runtime ownership structures as part of render preparation
- GameFramework-owned material descriptions
- mesh component ownership/lists
- texture reference lists
- skeleton descriptors
- animation clip ownership
- level descriptions
- asset registries and asset managers

Renderer-owned alternatives already visible in source:

- `RenderSceneSnapshot`
- `RenderSceneData`
- `MaterialData`
- `MeshDraw`
- `MeshInstanceBatch`
- renderer caches and GPU resources

## What GameFramework Must Not Know About RHI

GameFramework must not know about:

- `RenderHardwareInterface`
- `RenderDeviceServices`
- native graphics device, queue, or command-list handles
- GPU mesh cache objects
- renderer binding sets
- resource states or barriers
- backend-specific APIs such as D3D12, Vulkan, NVAPI, or Streamline

Current source-backed evidence:

- `SparkleGameFramework` CMake links only `SparkleCore` and `SparklePlatform`
- renderer/RHI-facing types appear only on the renderer side of the boundary

Contract rule:

- any new runtime data type that would require RHI knowledge is in the wrong layer unless it is first translated into a renderer-owned structure

## New Runtime Data Type Checklist

Use this checklist before adding a new GameFramework runtime data type that the renderer may eventually consume.

1. Put ownership in the correct module.
   - gameplay/runtime mutable state in GameFramework
   - render/GPU/cache state in Renderer

2. Keep GameFramework free of Renderer/RHI types.
   - no GPU pointers
   - no binding sets
   - no resource states
   - no backend-native handles

3. Decide the mutable owner.
   - scene subsystem
   - controller
   - asset payload append path
   - level-load path

4. Define snapshot form explicitly.
   - what fields move from mutable runtime state into frame-readable snapshot data

5. Define renderer translation explicitly.
   - what renderer-owned data is derived from the snapshot

6. Use stable ids/handles from GameFramework.
   - asset ids
   - material handles
   - mesh/skeleton ids
   - authored/runtime indices

7. Keep temporal or previous-frame state on the renderer side unless gameplay truly owns it.

8. Define mutation timing.
   - load-time
   - update-time
   - append-payload-time
   - reset-time

9. Define invalidation rules.
   - level unload
   - level change
   - asset append
   - scene reset

10. Mark uncertain lifetime or threading behavior as `Needs source confirmation` until proven in source.

## Known Gaps

- The snapshot seam is clear, but a formal thread-safety contract for scene mutation versus snapshot capture is not yet visible in reviewed source.
- `GameSceneSnapshot` currently does not include explicit skeleton snapshot payloads; renderer skinning data is derived through animation poses and mesh snapshot references instead.
- Exact lifetime guarantees around snapshot-held `const Mesh*` pointers are not documented outside source.
- Asset versioning and cooked compatibility policy across runtime scene data types still need a dedicated follow-up note.
- The reviewed source shows scene-scoped renderer invalidation on level lifecycle events, but a broader mutation-to-render invalidation matrix beyond those events still needs documentation.

## Source Anchors

Primary reviewed files for this contract:

- `Docs/Architecture/00-Review/B_EngineArchitectureScorecard.md`
- `Engine/GameFramework/CMakeLists.txt`
- `Engine/Application/CMakeLists.txt`
- `Engine/GameFramework/Public/Scene/GameScene.h`
- `Engine/GameFramework/Public/Scene/GameSceneSnapshot.h`
- `Engine/GameFramework/Private/Scene/GameScene.cpp`
- `Engine/GameFramework/Public/Scene/GameSceneController.h`
- `Engine/GameFramework/Public/Scene/Materials/SceneMaterials.h`
- `Engine/GameFramework/Public/Scene/Materials/MaterialSnapshot.h`
- `Engine/GameFramework/Public/Scene/Materials/MaterialHandle.h`
- `Engine/GameFramework/Public/Scene/Meshes/SceneMeshes.h`
- `Engine/GameFramework/Public/Scene/Meshes/MeshSnapshot.h`
- `Engine/GameFramework/Public/Scene/Textures/SceneTextures.h`
- `Engine/GameFramework/Public/Scene/Textures/TextureSnapshot.h`
- `Engine/GameFramework/Public/Scene/Camera/CameraSnapshot.h`
- `Engine/GameFramework/Public/Scene/Lighting/LightingSnapshot.h`
- `Engine/GameFramework/Public/Scene/Skeletons/SceneSkeletons.h`
- `Engine/GameFramework/Public/Scene/Animations/SceneAnimations.h`
- `Engine/GameFramework/Public/Scene/Animations/SceneAnimation.h`
- `Engine/GameFramework/Public/Assets/SceneAssetManager.h`
- `Engine/GameFramework/Public/Assets/SceneAssetRegistry.h`
- `Engine/GameFramework/Public/Assets/AssetId.h`
- `Engine/GameFramework/Public/Assets/Cooked/CookedAssetCommon.h`
- `Engine/Renderer/Private/SceneData/Lifecycle/RenderSceneSnapshot.h`
- `Engine/Renderer/Private/SceneData/Lifecycle/RenderSceneSnapshot.cpp`
- `Engine/Renderer/Private/SceneData/Lifecycle/SceneRenderStateCoordinator.h`
- `Engine/Renderer/Private/SceneData/Lifecycle/SceneRenderStateCoordinator.cpp`
- `Engine/Renderer/Private/SceneData/Builders/RenderSceneDataBuilder.h`
- `Engine/Renderer/Private/SceneData/Builders/RenderSceneDataBuilder.cpp`
- `Engine/Renderer/Private/SceneData/RenderSceneData.h`
- `Engine/Renderer/Public/SceneData/MeshDraw.h`
- `Engine/Renderer/Private/SceneData/MaterialData.h`

# Renderer Scene, View, And Frame Architecture

Status: target architecture, Unreal Engine reference analysis, and atomic implementation cutover plan; not implemented behavior
Date: 2026-08-18
Scope: GameFramework-to-Renderer publication, persistent render-scene ownership, GPU-scene ownership, one-frame scene and view products, temporal view state, deferred frame orchestration, frame-graph pass inputs, coherent cross-module naming, complete legacy-path removal, atomic landing, D3D12/Vulkan validation, and cleanup of the current frame path

## Decision

Sparkle should adopt the lifetime and responsibility split used by Unreal's deferred renderer without copying Unreal's class size, inheritance, naming prefixes, or feature breadth:

1. `RenderScene` is the persistent render-coordinator-owned mirror of gameplay scene state.
2. `RenderGpuScene` and the ray-tracing scene are scene capabilities owned beneath `RenderScene`, not frame-pipeline state.
3. `PreparedRenderScene` is an immutable, frame-slot-owned projection of the scene used by one submitted frame.
4. `RenderView` is an immutable one-frame view: camera, matrices, frustum, rectangles, resolved view policy, temporal shader values, and view-derived visibility/draw products.
5. `RenderViewState` is the persistent continuity for one stable viewport/view identity: previous camera state, jitter sequence, semantic history validity, exposure continuity, and view-history invalidation state.
6. `RenderFrame` owns only one frame's identity, time, frame-in-flight slot, `PreparedRenderScene`, and current `RenderView`. It is a lifetime boundary, not a service locator.
7. `FramePipeline` sequences the render-thread frame and owns the cached frame-graph execution. It delegates scene preparation, view preparation, history, providers, presentation, and pass-specific work to their existing or newly clarified owners.
8. Frame-graph passes declare narrow pass parameters. `FrameContext` and `PassRuntimeContext` are removed rather than renamed into new catch-all bags.
9. The migration lands as one complete architectural cutover. The work may be organized into reviewable workstreams, but no intermediate mixture of current and target ownership, naming, packet, shader ABI, pass context, or scene/view paths may merge or ship.

The target flow is:

```text
GameWorld / EditorViewportSession
       |
       | owned immutable RenderFrameSubmission
       |   FrameId
       |   RenderSceneUpdate
       |   RenderViewInput
       v
RenderCoordinator
       |
       +--> RenderScene.Apply(update)
       |        persistent RenderPrimitive records
       |        materials / textures / lights / sky / revisions
       |        RenderGpuScene / RenderRayTracingScene
       |
       +--> RenderViewBuilder.Build(input, viewport request, view state)
       |        matrices / frustum / extents / policy / temporal values
       |
       +--> RenderScene.PrepareFrame(frame slot)
       |        immutable PreparedRenderScene
       |
       +--> PrepareView(scene frame, view)
       |        visible indices / batches / view workload
       |
       `--> FramePipeline
                graph setup from explicit scene/view/pass parameters
                compile -> record -> submit -> present -> retire
```

This is a target design. Current code and executable build configuration remain the authority for implemented behavior. Mainline must remain wholly on the current architecture until the final cutover gate passes; after that landing, it must contain only the target architecture.

## Outcome

The completed refactor should make every renderer value answer four questions without tracing a broad context object:

- Who owns it?
- How long does it live?
- Is it scene-wide, view-specific, frame-wide, or pass-local?
- Which producer is allowed to mutate it?

The practical result is a frame path where:

- gameplay publishes renderer input but never owns renderer state;
- scene updates are applied once to a persistent render-thread mirror;
- camera and viewport policy live in the view path, not scene metadata;
- view-dependent culling and draw ordering do not contaminate scene-wide products;
- temporal history outlives `RenderView`, but current-frame view data does not;
- persistent GPU buffers are updated from scene data and exposed as narrow bindings;
- graph resources remain graph resources rather than fields on scene or view objects;
- each pass can see only its declared data and execution infrastructure;
- `FramePipeline` reads as a short sequence of owned stages rather than the owner of every renderer concern.

The refactor does not itself add multi-view rendering, stereo, a new rendering feature, a second scene representation, or a new scheduling system.

## Authority Boundary

This document owns the target Scene/View/Frame split and its migration order. It does not replace:

- [Whole Repository Architecture Map](WholeRepositoryMap.md) for the implemented repository map;
- [Renderer and RHI Architecture Boundary](RendererRhiBoundary.md) for RHI, frame-graph, submission, and backend ownership;
- [Multithreaded Engine Architecture](Multithreading/MultithreadedEngineArchitecture.md) for render-coordinator and worker publication rules;
- [Editor Viewport Camera Architecture](EditorViewportCamera.md) for authored-camera, free-view, and viewport exposure ownership;
- [Engineering Standards](../Engineering/Standards/README.md) for binding implementation and validation rules.

Unreal is precedent, not local authority. The local model is selected according to Sparkle's current scale and existing owners.

## Unreal Deferred Renderer: What Lives Where

The research below uses current Epic Games Unreal Engine 5.8 documentation. Epic publicly documents the stable interfaces and concepts, but important implementation classes such as private `FScene`, renderer-private `FViewInfo`, and parts of `FGPUScene` change across releases and are not exhaustively documented as member lists. The table therefore distinguishes documented facts from architectural inference. Sparkle should copy the durable lifetime split, not assume private member-for-member equivalence.

| Unreal concept | Lifetime and owner | What it carries | What it does not mean for Sparkle |
| --- | --- | --- | --- |
| `UWorld` / `UPrimitiveComponent` | Gameplay-side mutable authority | Authored/runtime component state and gameplay behavior | Renderer code should not dereference the live gameplay object graph. |
| `FPrimitiveSceneProxy` | Persistent render-thread mirror for one primitive | Renderer-facing primitive behavior and copied state required for rendering | Sparkle does not need Unreal's subclass hierarchy. A plain renderer-owned record remains preferable. |
| `FPrimitiveSceneInfo` | Persistent renderer-private record, one-to-one with the proxy | Scene membership, persistent and packed indices, static meshes, cached mesh draw-command information, light interactions, GPU-scene offsets, and dirty state | Sparkle does not need a second object beside `RenderProxy` unless two genuinely different lifetimes appear. |
| `FSceneInterface` / private `FScene` | Persistent scene manager and render-scene authority | Add/remove/update entry points for primitives, lights, decals, fog, sky, and other scene proxies; persistent arrays, indices, caches, scene uniform state, and GPU-scene integration | It is not a per-frame snapshot and does not contain one camera's culling result as scene truth. |
| `FGPUScene` | Persistent scene-wide GPU representation with per-update work | Primitive, instance, payload, and lightmap data addressed by persistent primitive/instance identities; dirty updates; documented dynamic-primitive writes may carry a view | It is not merely a temporary upload helper owned by the deferred frame function. |
| `FSceneViewFamily` / renderer-private family info | One scene-render invocation | Scene pointer, render target, time and frame counters, show flags, view mode, exposure/output/resolution policy, and the set of views sharing those values | Sparkle currently renders one view. A family abstraction is not justified until multiple simultaneous views share one invocation. |
| `FSceneView` | Recreated for a view each frame | Camera/view matrices, projection, view frustum, view rectangles, camera-cut and jitter flags, hidden/show-only sets, view feature allowances, final post-process settings, a view uniform buffer, family link, and optional persistent view-state link | It is not the persistent temporal-history owner. |
| Renderer-private `FViewInfo` | One renderer invocation, derived from `FSceneView` | Renderer-only per-view visibility, dynamic mesh elements, visible mesh draw commands, pass preparation, and other view-derived transient work | Exact private fields are version-sensitive. The durable lesson is that renderer-private view products remain view-specific. |
| `FSceneViewStateInterface` / private view state | Persistent across frames for one stable view identity | Temporal-AA sampling, eye adaptation, occlusion continuity, previous-view information, and other history required across frames | Persistent view state should not be embedded in the one-frame view value. |
| `FSceneRenderer` / `FDeferredShadingSceneRenderer` | One scene-render invocation and renderer orchestration scope | Consumes scene plus view family, initializes views and visibility, then schedules depth, base/GBuffer, lighting, and post-processing work | It is not the owner of gameplay truth or the persistent render scene. Sparkle's cached graph means its orchestration lifetime need not match Unreal's allocation pattern. |
| `FSceneTexturesConfig` / scene textures | One view-family/render-graph configuration and graph lifetime | Extent, formats, sample count, GBuffer configuration, feature/shading path, creation flags, and requested extracts | These are topology/resource facts, not persistent scene-domain state. Epic itself documents moving away from a global config toward family-local ownership. |
| RDG pass parameter structs | One graph/pass execution | Exactly the resources and shader/pass values used by a pass; declarations derive dependencies and transient lifetimes | A graph execution should not expose every scene, cache, setting, provider, and history through one shared bag. |

### The important Unreal boundaries

#### Gameplay state is mirrored, not borrowed

Epic's threaded-rendering documentation describes `FPrimitiveSceneProxy` as render-thread state corresponding to a game-thread component and explicitly warns against the renderer reading live gameplay-owned objects. Sparkle already follows the same fundamental direction through immutable render input, `RenderInputConsumer`, and `RenderWorld`; the refactor should strengthen that boundary rather than replace it.

#### The scene is persistent and retained

`FSceneInterface` exposes add, remove, update, and batched mutation operations around a private render scene. `FPrimitiveSceneInfo` stores persistent identity, scene membership, GPU-scene offsets, static meshes, cached draw-command information, and dirty flags. Epic's mesh-drawing guide describes retained static batches and cached draw commands that survive until a primitive leaves the scene.

The durable model is:

```text
game component change
        |
        v
render-scene mutation
        |
        +--> persistent primitive identity and cached scene state
        +--> dirty GPU-scene ranges
        `--> per-view selection of visible work
```

Scene-wide cached data must be constructible without a view. Epic's cached mesh-command path explicitly cannot rely on `FSceneView` during scene insertion. That is a useful placement test for Sparkle: if a value can be prepared and retained without knowing a camera, it is a scene candidate; if it changes with camera/frustum/viewport, it is a view product.

#### View, view family, and view state are different lifetimes

Epic documents `FSceneView` as a projection into a 2D region and constructs a new view each frame. It stores the current camera, frustum, rectangles, view policy, view uniform data, and a pointer to optional persistent state. `FSceneViewFamily` stores the shared scene-render invocation policy and list of views. `FSceneViewStateInterface` is explicitly persistent.

This is the core rule Sparkle should adopt:

```text
RenderViewState (persistent)
        |
        | previous values and history validity
        v
RenderView (one frame)
        |
        | current matrices, frustum, policy, visibility
        v
pass parameters (one pass)
```

#### GPU Scene is scene storage with update epochs

Epic's mesh-drawing guide describes GPU Scene as a scene-wide primitive-data buffer indexed by primitive identity. `FPrimitiveSceneInfo` exposes persistent indices, instance/payload/lightmap offsets, allocation/free operations, and GPU-scene dirty requests. Current writer parameters also expose a frame number and allow a view pointer for dynamic primitives.

The transferable point is not that every byte is permanent. It is that the owner is the render scene, while updates and dynamic slices have bounded frame/view epochs. Sparkle's existing persistent buffers plus frame-indexed dynamic storage already match this idea closely.

#### The graph owns graph resources; passes own their parameters

Epic's RDG derives dependencies and transient lifetimes from pass parameter structs, limits native resource access to a declaring execution lambda, and separates graph setup from side-effect-free command recording. Sparkle's frame graph already owns order, resources, barriers, aliasing, and recording groups. The missing cleanup is at the data-access surface: current passes can still reach broad frame and runtime bags after the graph has already declared their resource dependencies.

### Primary Unreal sources

- Epic Games, [Graphics Programming Overview](https://dev.epicgames.com/documentation/en-us/unreal-engine/graphics-programming-overview-for-unreal-engine)
- Epic Games, [Threaded Rendering](https://dev.epicgames.com/documentation/en-us/unreal-engine/threaded-rendering-in-unreal-engine)
- Epic Games, [`FSceneInterface`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/FSceneInterface)
- Epic Games, [`FPrimitiveSceneInfo`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Renderer/FPrimitiveSceneInfo)
- Epic Games, [`FSceneView`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/FSceneView)
- Epic Games, [`FSceneViewFamily`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/FSceneViewFamily)
- Epic Games, [`FSceneViewStateInterface`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/FSceneViewStateInterface)
- Epic Games, [Mesh Drawing Pipeline](https://dev.epicgames.com/documentation/en-us/unreal-engine/mesh-drawing-pipeline-in-unreal-engine)
- Epic Games, [`FSceneUniformBuffer`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Renderer/FSceneUniformBuffer)
- Epic Games, [`FGPUSceneWriteDelegateParams`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Renderer/FGPUSceneWriteDelegateParams)
- Epic Games, [`FSceneTexturesConfig`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/FSceneTexturesConfig)
- Epic Games, [Render Dependency Graph](https://dev.epicgames.com/documentation/en-us/unreal-engine/render-dependency-graph-in-unreal-engine)
- Epic Games, [`FScreenPassViewInfo`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Renderer/FScreenPassViewInfo)

## Current Sparkle Mapping

Sparkle already has most of the necessary concepts. The problem is classification and ownership, not absence.

| Current Sparkle concept | Closest Unreal role | Assessment |
| --- | --- | --- |
| `GameWorld` plus world extraction | `UWorld` / component-side publication | Good boundary: gameplay is authoritative and publishes owned renderer values. |
| `RenderWorldDelta` plus `RenderFrameDynamicData` | Render-scene mutation stream | Good structural/dynamic split, but camera and renderer-local metadata are mixed into scene dynamic data. |
| `RenderInputConsumer` | Render-thread scene-update acceptance | Good place for sequence/generation validation; it should not own provider/view history policy. |
| `RenderWorld` | Private `FScene` | This is already Sparkle's persistent render scene. Its name obscures that distinction from `GameWorld`. |
| `RenderProxy` | Collapsed proxy plus primitive-scene-info record | Appropriate for Sparkle's scale. Do not split it merely to match Unreal. |
| `PersistentRenderGpuScene` | `FGPUScene` | Correct persistent-buffer and frame-slot shape, but incorrectly owned by `FramePipeline` instead of the scene. |
| `RenderRayTracingScene` | Scene-owned ray-tracing acceleration capability | Persistent scene capability currently owned by `RendererHost`; its planning is partly frame/view dependent. |
| `RenderSceneData` | Mixed scene-frame plus renderer-private view info | The main ownership fault: scene-wide records, frame-dynamic records, view culling, camera-distance ordering, raster batches, and ray work coexist. |
| `RenderViewData` | Small subset of `FSceneView` | Good seed, but camera construction, frustum, policy, temporal state, and view-derived work are scattered elsewhere. |
| `TemporalDataBuilder` | A single implicit view state | Persistent previous-view state exists, but has no stable viewport/view identity and is named as a stateless builder. |
| `RenderCamera` plus `PerViewDataBuilder` | View construction | These are fragments of one view builder; `RenderCamera` is a mutable intermediate mirror with no independent authority. |
| `FrameContext` | One render invocation's scene/view aggregate | Owns useful one-frame values, but encourages all passes to reach all frame data. |
| `PassRuntimeContext` | No desirable direct equivalent | A broad service bag containing RHI, pass runtimes, frame values, display settings, history, caches, ray tracing, and providers. It defeats narrow pass ownership. |
| `FrameAssemblyResourceLayout` | Scene textures, history, external bindings, and output handles | Legitimate graph topology, but the name hides distinct transient, external, history, and product categories. |
| `BuildFrame` | Deferred render-graph topology construction | It builds a deferred graph; it does not build a complete frame value. |
| `FramePipeline` | Persistent frame lifecycle plus parts of scene renderer, view family, presentation, and provider orchestration | Too many responsibilities currently converge here. |
| `RendererHost` | Composition root | Correct long-lived role, but its many getters make it easy for downstream code to treat it as a service locator. |

### Current evidence of mixed responsibilities

As inspected on 2026-08-18:

- [`FramePipeline.cpp`](../../Engine/Renderer/Private/FramePipeline/FramePipeline.cpp) is 698 lines and coordinates input consumption, resize, graph rebuilds, uploads, camera mutation, per-frame constants, scene and view preparation, history invalidation, providers, ray tracing, scene resource binding, graph execution, submission, capture, UI, and presentation.
- [`RenderSceneData.h`](../../Engine/Renderer/Private/SceneData/RenderSceneData.h) contains scene revisions, lights, sky, all mesh/deformation records, materials, view-frustum-selected raster indices, camera-distance-sorted batches, workload values, and ray-tracing work.
- [`RenderPreparationInputResolver.cpp`](../../Engine/Renderer/Private/SceneData/Preparation/RenderPreparationInputResolver.cpp) receives both `RenderWorld` and a view frustum/camera position. Its result cannot be accurately described as scene-only.
- [`FrameContextBuilder.cpp`](../../Engine/Renderer/Private/Frame/Builders/FrameContextBuilder.cpp) prepares scene data, plans ray tracing from camera position, updates GPU Scene, constructs the view, and advances temporal history in one function.
- [`PassRuntimeContext.h`](../../Engine/Renderer/Private/FrameGraph/PassRuntimeContext.h) is referenced by 45 Renderer private files. `mainView` is referenced by 24 files. This is coupling evidence, not a reason to create a larger replacement context.
- [`PerFrameConstantBufferData.h`](../../Engine/Renderer/Private/ShaderData/PerFrameConstantBufferData.h) mixes true frame time/index values with view mode and viewport size.
- [`FramePipeline::FinalizeRenderInputMetadata`](../../Engine/Renderer/Private/FramePipeline/FramePipeline.cpp) writes render/output dimensions into a GameFramework-owned input packet after publication. Those dimensions are renderer/view configuration, not GameFramework scene metadata.
- `RenderFrameMetadata::Exposure` has no current consumer. Motion-vector and depth conventions are stable renderer/shader contracts rather than per-frame input. `ProviderGeneration` is populated from shader-package generation and then participates in input/history/capture behavior under a misleading name.
- `ViewportRenderRequest::ViewKind`, `ViewSelection`, and `FeatureFlags` currently have no `FramePipeline` consumer. The request contains the right intent categories, but they do not yet reach a first-class render view.

These are dated observations. Re-run the searches at the start of implementation because the frame path is actively changing.

## Atomic Migration Contract

This refactor has one landable state: the complete target architecture in this document. The work breakdown later in the document is an execution and review aid, not a sequence of supported repository states.

```text
mainline before cutover                  migration branch                       mainline after cutover

current architecture          ->        all workstreams completed       ->     target architecture only
no target aliases                       final head validated                    no current paths or names
```

The following rules are binding:

- Mainline does not receive a scene-only, view-only, packet-only, GPU-only, pass-context-only, naming-only, or shader-ABI-only subset.
- If review requires multiple commits or stacked pull requests, they target one private integration branch and are not independently mergeable, releasable, or described as implemented architecture.
- The final integration unit updates GameFramework, Application, Editor, Renderer, shaders, CMake membership, tests, generated/cooked artifacts, and documentation together.
- No feature flag, build option, CVar, runtime branch, typedef, adapter, overload, reader, writer, or fallback selects between current and target architectures.
- No target owner reads a current representation, and no current owner reads a target representation. There is one publication packet, one scene owner, one view path, one shader ABI, and one pass-input model at final head.
- Temporary migration scripts may mechanically rewrite source or regenerate artifacts, but they are not runtime code and are removed before landing unless they remain the canonical generator.
- Failed final validation is fixed on the migration branch. It is not bypassed by restoring an old path alongside the new one.
- Rollback means reverting the complete migration. There is no runtime compatibility fallback and no partial rollback that resurrects selected current owners.

At no point may documentation mark an individual workstream as implemented architecture. Only the complete cutover changes this document and the implemented repository map from target to implemented status.

### No-halfway review rule

A reviewer must reject the migration if any of these conditions exists at final head:

- both `RenderWorld` and `RenderScene` participate in runtime code;
- both `RenderInputFrame`/`RenderFrameMetadata` and `RenderFrameSubmission` are produced or consumed;
- both `RenderProxy` and `RenderPrimitive` represent scene primitives;
- both `RenderSceneData` and `PreparedRenderScene` carry current frame scene data;
- both `RenderViewData`/`RenderCamera`/`TemporalDataBuilder` and `RenderView`/`RenderViewState` are active;
- both `FrameContext`/`PassRuntimeContext` and explicit pass parameters can execute passes;
- both old and new constant-buffer layouts are cooked or accepted;
- both frame-pipeline-owned and scene-owned GPU/RT scene instances can exist;
- an adapter or alias makes an old call site compile without adopting the new owner;
- old files remain in CMake, generated manifests, cooked shader metadata, tests, examples, or current architecture documentation.

The correct response to any item above is to finish the migration or withhold the entire landing.

## Cross-Module Naming Contract

Coherence means that the same semantic concept uses the same root name across its producer, publication boundary, renderer owner, GPU binding, and shader data. It does not mean giving different lifetimes the same type.

| Concept | GameFramework / cross-thread name | Renderer CPU name | GPU / shader name | Naming rule |
| --- | --- | --- | --- | --- |
| Gameplay authority | `GameWorld` | none | none | `World` is reserved for gameplay-owned world state. |
| Renderer-bound scene change | `RenderSceneDelta` inside `RenderSceneUpdate` | `RenderScene::Apply` | dirty scene ranges | Delete `RenderWorldDelta`; the packet describes the render scene it updates. |
| Frame-varying scene values | `RenderSceneDynamicData` | input to `RenderScene` / `PreparedRenderScene` | scene instance/light/deformation records | Camera and viewport values are forbidden here. |
| Stable render object identity | `RenderObjectId` | `RenderPrimitive` | primitive/instance id or GPU-scene slot | Delete `RenderProxy`; one persistent record has one name. |
| View camera publication | `RenderViewCameraData` inside `RenderViewInput` | `RenderView` | `ViewCameraUniformData` | Delete ambiguous `RenderCameraData` and the mutable `RenderCamera` mirror. |
| Submitted render work | `RenderFrameSubmission` | accepted into `RenderFrame` | `FrameUniformData` for true frame values | `Submission` is cross-thread ownership; `Frame` is renderer frame-slot lifetime. |
| Persistent renderer scene | none | `RenderScene` | `RenderGpuScene` | `Gpu` uses repository casing; persistence is expressed by ownership, not a prefix. |
| One-frame scene projection | none | `PreparedRenderScene` | `RenderSceneGpuBindings` | `Prepared` distinguishes the immutable derived product from the mutable scene authority. |
| Current view | `RenderViewInput` / `ViewportRenderRequest` | `RenderView` | `ViewUniformData`, `ViewTemporalUniformData` | `PerView*` and `PerTemporal*` names are removed in the clean break. |
| Persistent view continuity | none | `RenderViewState` | graph/provider histories remain with their resource owners | State owns semantic continuity, not history resources. |
| Deferred graph construction | none | `BuildDeferredFrameGraph` | frame-graph resource handles | Delete generic `BuildFrame`; the function builds graph topology. |
| Deferred graph resource namespace | none | `DeferredFrameGraphResources` | transient, imported-scene, history, and viewport-product handles | Delete the ambiguous `FrameAssembly*` vocabulary; these are graph handles, not a frame value or scene owner. |
| Pass recording surface | none | `PassCommandContext` | declared pass parameters | `Context` is allowed only for transient command/resource/diagnostic infrastructure. |

The final implementation must freeze exact spellings before the first mechanical rename. Once frozen, all headers, filenames, forward declarations, member names, diagnostics labels, tests, CMake entries, shader structs, comments, and current documentation use the selected vocabulary. Synonyms such as world/scene, proxy/primitive, camera/view, snapshot/prepared scene, and frame/context must not survive as competing architectural terms.

## Target Ownership Model

### `RenderFrameSubmission`: one cross-thread publication

Keep one bounded, owned publication through the existing render queue, but separate its domains:

```text
RenderFrameSubmission
  FrameId
  Scene : RenderSceneUpdate
    Structural : RenderSceneDelta
    Dynamic    : RenderSceneDynamicData
  View : RenderViewInput
    Camera : RenderViewCameraData
    CameraCut
    CameraTeleported
```

`RenderSceneDynamicData` contains objects, lights, joint ranges/matrices, and morph ranges/weights. It contains no camera, output extent, exposure, provider/shader generation, viewport mode, or renderer conventions.

`RenderFrameMetadata` is deleted. Its current fields move as follows:

| Current field | Target owner |
| --- | --- |
| `FrameId` | Top-level `RenderFrameSubmission`, then renderer-local `RenderFrameIdentity` |
| `SceneGeneration` | `RenderSceneDelta`; do not duplicate it in metadata |
| `ProviderGeneration` | Renderer-local frame identity captured from the actual shader/provider owners |
| `RenderWidth`, `RenderHeight` | Resolved `RenderView` render extent and graph topology key |
| `OutputWidth`, `OutputHeight` | Resolved `RenderView` output extent/presentation configuration |
| `Exposure` | Delete; resolved viewport display/exposure settings already own this policy |
| Motion/depth conventions | Canonical shader/renderer contracts, not packet fields |
| `CameraCut`, `CameraTeleported` | `RenderViewInput`; consumed by `RenderViewState` invalidation |
| `ResetHistory` | Derived by the renderer from scene reset, view discontinuity, topology/provider changes, and explicit owner events |

The application/editor continues to resolve the effective view camera according to [Editor Viewport Camera Architecture](EditorViewportCamera.md) and publishes it as `RenderViewCameraData`. The renderer receives the resolved value and does not query `GameWorld` or Editor state.

### `RenderScene`: persistent renderer scene authority

`RenderWorld` is clean-break renamed to `RenderScene` because it is not gameplay world state. `RenderProxy` is clean-break renamed to `RenderPrimitive`. The final code contains neither old name. `RenderScene` owns:

- render primitives and stable `RenderObjectId` lookup;
- static and latest dynamic primitive values required by rendering;
- material, texture, sky, light, and instance-group scene tables;
- scene generation, accepted sequence, structural/material revisions, and dirty state;
- stable GPU-scene slot allocation;
- previous object transforms and deformation continuity, because those follow scene objects rather than one camera;
- `RenderGpuScene` and `RenderRayTracingScene` as focused subordinate capabilities.

It must not own:

- camera matrices or frusta;
- viewport/scissor/output extents;
- exposure, view mode, selection, or per-view feature flags;
- visible-instance lists or camera-distance draw order;
- temporal AA, eye adaptation, or per-view history validity;
- frame-graph handles or pass parameters;
- presentation, UI, capture, or image-provider policy.

Keep one `RenderPrimitive` record unless evidence establishes two different owners or lifetimes. Unreal's separate proxy/info objects solve module and polymorphism constraints Sparkle does not currently have.

### `PreparedRenderScene`: immutable frame-slot scene product

The render scene produces one immutable value for the selected frame-in-flight slot. This copy/materialization boundary is justified by producer/render pipelining, parallel preparation/recording, and GPU retirement. Storage is reused per slot.

It owns or views:

- structural/material revision values used for invalidation;
- prepared mesh instances and world bounds for all renderable primitives;
- current and previous joint matrices and morph weights;
- prepared scene lights and sky;
- resolved material values and the scene material texture-table binding;
- view-independent ray-tracing BLAS/TLAS source records;
- a narrow `RenderSceneGpuBindings` view after scene GPU update.

It does not contain raster visibility, camera distance, view sorting, raster batches, viewport data, view history, or graph handles.

Use moves, frame-slot vector reuse, spans, and stable indices. Do not copy material/mesh/cache objects simply to make this type self-contained. Every retained copy needs a frame-lifetime or concurrency reason under the repository copy budget.

### `RenderView`: immutable current-frame view

`RenderViewData`, `RenderCamera`, and `PerViewDataBuilder` are deleted and replaced by one view-construction path using `RenderViewCameraData`, `RenderViewBuilder`, and `RenderView`. The resulting `RenderView` owns:

- stable viewport id, current request/product generation, and resolved view-selection identity;
- view kind, view mode, enabled view features, and requested output policy;
- current camera value, view/projection/inverse matrices, frustum, near/far values, and camera position/direction;
- render extent, output extent, viewport, and scissor;
- resolved display/exposure settings relevant to this view;
- current jitter, previous matrices/jitter copied from state, and current history-valid flags;
- per-view and temporal shader constants;
- visible scene-instance indices, view-sorted/batched raster work, and view workload summary;
- view-dependent ray-tracing planning values when camera position or view policy changes the plan.

It references scene primitives through stable indices into `PreparedRenderScene`. It does not own scene-wide material tables, mesh cache objects, GPU resource owners, history textures, provider objects, or the mutable `RenderViewState`.

One-frame view data is immutable after preparation starts. Recording tasks may borrow it only while the owning frame slot remains alive.

### `RenderViewState`: persistent view continuity

Replace the implicit process-wide history inside `TemporalDataBuilder` with an explicit state value owned for the active viewport identity. Initially Sparkle needs one active `RenderViewState`, not a registry or `RenderViewFamily` system.

It owns:

- the stable viewport/view-selection identity for which history is valid; request/product generation is tracked separately and does not invalidate history by itself;
- previous view/projection/world-to-clip matrices and camera pose;
- previous jitter and temporal sample index;
- semantic validity and invalidation reasons for TAA, exposure, lighting reservoirs, reference accumulation, and provider history;
- lighting-history invalidation hashes that depend on scene plus view;
- last accepted scene/view/provider/shader generations required to invalidate continuity.

The frame graph still owns persistent history resource handles and lifetime. Image providers still own their private histories. `RenderViewState` owns the semantic decision that those histories are valid or must reset; it does not absorb their resources or implementations.

When viewport id, selection identity, projection, graph topology, image-provider generation, shader package generation, scene generation, camera cut, or explicit reset changes continuity, the owning stage invalidates state once with a reason. Do not scatter independent reset booleans across the input consumer, pipeline, temporal builder, histories, and providers.

If simultaneous stereo, split-screen, multiple editor views, or scene captures become a real requirement, introduce `RenderViewFamily` then. It should own the shared scene, target, frame time, output/resolution policy, and list of views. Do not add it now as an empty future abstraction.

### `RenderFrame`: a lifetime owner, not a context

Each frame-in-flight slot owns:

```text
RenderFrame
  Identity
    FrameId
    ShaderPackageGeneration
    ImageProviderGeneration
  Time
  FrameInFlightIndex
  PreparedRenderScene
  RenderView
```

`RenderFrame` is not passed indiscriminately to every pass. Functions request `const PreparedRenderScene&`, `const RenderView&`, `RenderFrameIdentity`, or a smaller projection according to actual need. Epic's `FScreenPassViewInfo`, which carries only the subset needed from a view, is useful precedent for narrow projections.

Do not add RHI, caches, providers, mutable histories, graph builders, diagnostics services, or presentation state to `RenderFrame`.

### GPU scene and ray-tracing scene

Clean-break rename `PersistentRenderGpuScene` to `RenderGpuScene`; persistence is implied by scene ownership. Move its lifetime beneath `RenderScene`. Preserve its current persistent buffers, revision checks, and frame-indexed dynamic slices.

`RenderSceneGpuData` is clean-break replaced by the narrow non-owning `RenderSceneGpuBindings` view. Frame-graph import handles remain in graph resource layout types and are bound from those scene-owned resources at frame setup.

Move logical ownership of `RenderRayTracingScene` beneath `RenderScene`, while keeping BLAS cache, TLAS strategy, and capability logic as focused subordinate types. Separate:

- view-independent traceable-instance and BLAS source preparation, which belongs to `PreparedRenderScene` / scene capability;
- camera- or view-policy-dependent TLAS planning, which belongs to the current render invocation;
- persistent acceleration resources and cache invalidation, which belong to `RenderRayTracingScene`.

Do not merge GPU Scene and ray tracing into one class. They share a scene owner but have different resource and update contracts.

### Frame constants and view constants

Split the shader ABI in one paired C++/HLSL clean break:

- `FrameUniformData`: frame index/id and scaled/unscaled time values;
- `ViewUniformData`: viewport size/inverse size, view mode, view rect, and resolved view feature values;
- `ViewCameraUniformData`: camera transforms, camera position/direction, and projection values;
- `ViewTemporalUniformData`: current/previous jitter, previous matrices, and history validity;
- `RenderSceneGpuBindings` and focused scene uniform values: scene-wide light/material/GPU-scene data.

`PerFrameConstantBufferData`, `PerViewConstantBufferData`, `PerViewCameraConstantBufferData`, and `PerTemporalConstantBufferData` are deleted. Viewport size and view mode move out of the frame ABI. No compatibility constant buffer, duplicate field, shader macro alias, duplicate registration name, or old cooked layout remains after the paired shader migration.

## Pass And Frame-Graph Contract

The target graph execution surface contains only infrastructure required to record compiled work:

```text
PassCommandContext
  Commands
  Resources
  Diagnostics
```

If global descriptor binding requires an additional primitive, expose that narrow operation through command/resource infrastructure rather than the whole `RenderHardwareInterface`.

All semantic inputs are pass-specific:

- shader/pass parameter structs receive copied frame, scene, and view uniform values during setup;
- mesh passes receive the prepared scene and current view explicitly while producing their pass parameters/draw work;
- a pass runtime/pipeline is captured by its pass object when the graph is built;
- history validity is copied only into passes that consume the corresponding history;
- ray-tracing settings and capabilities are supplied only to ray-tracing passes;
- image providers are captured by their provider passes for the provider generation that caused graph construction;
- mesh cache access is injected into the mesh-drawing owner, not exposed to all passes;
- display settings are resolved into exposure/tone-mapping pass parameters, not exposed globally.

Delete `PassRuntimeContext` and remove `FrameContext` after their last consumer migrates. Do not replace either with `RenderContext`, `RendererServices`, `FrameResources`, or another struct whose purpose is "everything passes may need."

`FrameAssemblyResourceLayout` and its `FrameAssembly*` nested names are clean-break replaced by graph-specific vocabulary:

- `DeferredFrameGraphResources`: the top-level handle namespace passed while constructing deferred topology;
- `DeferredTransientResources`: transient scene, GBuffer, lighting, exposure, and reconstruction handles;
- `DeferredImportedSceneResources`: graph imports for GPU Scene, sky, and TLAS;
- `FrameHistoryResourceLayout`: graph-owned persistent history handles;
- `ViewportFrameProducts`: requested exported viewport-product handles.

`FrameBuildSettings` is replaced by `DeferredFrameGraphSettings`. `BuildDeferredFrameGraph` returns `DeferredFrameGraphResources` directly, so the one-field `FrameBuildResult` wrapper is deleted. `FinalSceneColorProduced` is replaced by one initially invalid `ResolvedSceneColor` graph handle: reconstruction or upscaling publishes the handle it produced, and presentation consumes that handle. `ViewportFrameProducts::FinalSceneColor` remains the distinct encoded viewport product. `FrameGraphBuildResult` remains the distinct factory result that owns the built graph plus its exported resource handles.

`BuildFrame` becomes `BuildDeferredFrameGraph` because it creates graph topology. Graph handles never move into `RenderScene`, `PreparedRenderScene`, or `RenderView`.

## Frame Pipeline After The Refactor

`FramePipeline` remains the render-coordinator-owned frame sequencer. It should read at one level of abstraction:

```text
BeginFrame
  poll retirement and captures
  accept scene/view input
  resolve resize and graph topology key
  begin backend frame

PrepareFrame
  apply scene update
  build current RenderView seed from input, request, and view state
  prepare immutable scene frame
  prepare view visibility and draw work
  update scene GPU resources and ray-tracing plan
  resolve history/provider state

ExecuteFrame
  bind imported graph resources
  setup pass parameters from scene and view
  compile and execute graph

SubmitAndPresent
  submit
  publish products/captures/UI
  advance frame slot and retire completed work
```

Presentation, editor texture publication, UI playback, and capture may remain subordinate capabilities called by the pipeline, but their implementation should not be interleaved with scene/view preparation.

Keep the name `FramePipeline` for the thin lifecycle sequencer. Do not add a parallel `DeferredRenderer` wrapper or retain a forwarding facade merely to copy Unreal's name; the deferred-specific topology already has the explicit `BuildDeferredFrameGraph` name.

## Target Code Shape

These names are the committed target vocabulary. A rename discovered to be necessary during implementation requires updating this contract first and then applying the replacement everywhere in the same migration branch; it does not permit local synonyms.

```text
Engine/GameFramework
  Public/Rendering/RenderFrameSubmission.h
  Public/Rendering/RenderSceneUpdate.h
  Public/Rendering/RenderSceneDelta.h
  Public/Rendering/RenderSceneDynamicData.h
  Public/Rendering/RenderViewInput.h
  Public/Rendering/RenderViewCameraData.h
  Private/World/Extraction/...                 scene data only

Engine/Renderer/Private
  Scene/
    RenderScene.*                              persistent CPU scene owner
    RenderPrimitive.*                          one persistent primitive record
    RenderGpuScene.*                           persistent GPU scene capability
    RenderRayTracingScene.*                    persistent RT scene capability
    Preparation/RenderScenePreparation.*       view-independent frame product
    PreparedRenderScene.h
    RenderSceneGpuBindings.h                   narrow GPU binding projection
  View/
    RenderView.h                               immutable current view
    RenderViewBuilder.*
    RenderViewState.*                          persistent temporal semantics
    RenderViewPreparation.*                    visibility and view draw work
  Frame/
    RenderFrame.h                              frame-slot lifetime owner
    RenderFrameIdentity.h
    Deferred/BuildDeferredFrameGraph.*
    Deferred/DeferredFrameGraphResources.h
  FrameGraph/
    Execution/PassCommandContext.h             infrastructure only
  ShaderData/
    FrameUniformData.h
    ViewUniformData.h
    ViewCameraUniformData.h
    ViewTemporalUniformData.h
```

Do not create both old and new directory trees. Move files and update CMake membership in the same migration workstream that changes ownership; delete emptied builders, adapters, aliases, and replaced names before the final cutover.

## Complexity Budget And Explicit Non-Goals

The refactor is accepted only if it removes more ambiguity than structure it adds.

- Add `PreparedRenderScene`, `RenderViewState`, and the separated scene/view input values because they express real lifetimes.
- Delete `RenderInputFrame`, `RenderFrameMetadata`, `RenderWorldDelta`, `RenderFrameDynamicData`, `RenderWorld`, `RenderProxy`, `RenderSceneData`, `RenderViewData`, `RenderCamera`, `PerFrameDataBuilder`, `PerViewDataBuilder`, `TemporalDataBuilder`, `FrameContextBuilder`, `FrameContext`, `PassRuntimeContext`, `PersistentRenderGpuScene`, `RenderSceneGpuData`, the `FrameAssembly*` types, `FrameBuildSettings`, `FrameBuildResult`, and the four old constant-buffer data types as their responsibilities move.
- Do not add `RenderViewFamily` until one render invocation genuinely contains multiple views.
- Do not split `RenderPrimitive` into proxy/info/interface hierarchies without a demonstrated owner boundary.
- Do not add a generic renderer service registry, event bus, visitor, or manager.
- Do not introduce an Unreal-style global scene-texture singleton.
- Do not copy Unreal feature flags, post-process settings, or scene subsystems that Sparkle does not implement.
- Do not retain compatibility aliases, legacy packet readers, dual shader constant layouts, or old/new context paths.
- Do not add permanent diagnostics or dashboards for the refactor. Use existing timings, captures, tests, and one-time comparison evidence.
- Do not claim a performance improvement from moving fields. Measure preparation, graph setup, upload, recording, and frame time on the accepted workloads.

## Migration Work Breakdown - Not Independently Landable

These workstreams execute on one migration branch. Their checkpoints exist for review, diagnosis, and keeping the final integration understandable; none is an approved mainline architecture, merge boundary, release state, or reason to preserve compatibility. Every producer and consumer moves before the final cutover, and every replaced path is deleted before the migration branch becomes landable.

### Phase 0 - Freeze inventory, invariants, and baseline

- re-run the symbol/consumer inventory for every current type named in this document;
- record current D3D12 and Vulkan build/smoke status, frame-graph topology, selected cameras, CPU preparation timings, GPU frame timings, upload bytes, and frame-in-flight count;
- record the exact graph rebuild keys and every temporal reset producer/consumer;
- classify every `RenderSceneData`, `FrameContext`, `RenderViewData`, and `PassRuntimeContext` field as persistent scene, scene frame, view, view state, frame identity/time, graph topology, pass parameter, or deletion;
- freeze the exact target vocabulary, dependency direction, and current one-view scope;
- record the current legacy symbol, header, CMake, shader-registration, generated-metadata, and cooked-artifact inventory so the final eradication scan has a known starting set;
- identify overlapping dirty work before edits and preserve it.

Internal checkpoint only - not landable: every current field has one target owner, every old type and file has a deletion workstream, and baseline evidence can detect functional or performance regressions.

### Phase 1 - Correct the publication boundary

- replace `RenderInputFrame`/`RenderFrameMetadata`, `RenderWorldDelta`, `RenderFrameDynamicData`, and `RenderCameraData` with `RenderFrameSubmission`, `RenderSceneUpdate`, `RenderSceneDelta`, `RenderSceneDynamicData`, `RenderViewInput`, and `RenderViewCameraData`;
- remove camera values from scene dynamic data;
- remove render/output dimensions, exposure, conventions, provider generation, and reset policy from GameFramework metadata;
- make scene generation single-source in the structural scene update;
- capture shader/provider generation from renderer owners when constructing `RenderFrameIdentity`;
- derive renderer history resets from explicit scene/view/topology events;
- update GameFramework extraction, Application/editor camera publication, render queue, input consumer, capture metadata, and provider frame inputs together;
- update every include, filename, queue contract, serialization/capture label, test, and diagnostic using the current vocabulary;
- delete the old packet, delta, dynamic-data, camera-data, metadata types, fields, headers, and CMake entries without aliases or conversion adapters.

Internal checkpoint only - not landable: Renderer never mutates a GameFramework input packet, scene input contains no viewport policy, every accepted submission has one frame identity and one scene sequence, and no current packet name remains in executable code.

### Phase 2 - Establish persistent `RenderScene`

- clean-break rename `RenderWorld` to `RenderScene` and update diagnostics and documentation language;
- clean-break rename `RenderProxy` to `RenderPrimitive` and keep that one persistent record with its stable GPU-scene slot;
- move previous object transforms and deformation continuity into scene-owned preparation state;
- split current preparation into a view-independent `RenderScenePreparation` that publishes `PreparedRenderScene` in a reusable frame slot;
- move scene revisions, materials, texture table, sky, lights, all mesh records/bounds, deformation arrays, and view-independent ray inputs into that product;
- remove view frustum and camera position from scene preparation;
- preserve current task-graph concurrency with immutable/exclusive frame-slot outputs;
- delete `RenderWorld`, `RenderProxy`, `RenderSceneData`, their replaced paths, and any forwarding builders after all consumers use the target owners.

Internal checkpoint only - not landable: `RenderScene` and scene preparation compile without including view/frustum types, `PreparedRenderScene` contains no visible list, camera-distance order, viewport, or graph handle, and no second scene or primitive representation exists.

### Phase 3 - Establish `RenderView` and `RenderViewState`

- replace `RenderCamera`, `PerViewDataBuilder`, `TemporalDataBuilder`, and `RenderViewData` with `RenderViewBuilder`, `RenderView`, and `RenderViewState`;
- build matrices/frustum from immutable `RenderViewInput` and the actual requested render extent;
- resolve `ViewportRenderRequest` identity, kind, feature flags, output flags, and exposure into the view or graph topology key; wire intentional fields and delete unsupported promises;
- move raster visibility, camera-distance calculation, sorting, batching, and view workload from scene preparation into `RenderViewPreparation`;
- move semantic history validity and lighting invalidation hashes into view state;
- preserve one active view; reset/rebind its state on stable identity changes;
- replace the C++ and HLSL ABI with `FrameUniformData`, `ViewUniformData`, `ViewCameraUniformData`, and `ViewTemporalUniformData` in one paired change;
- update shader declarations, registrations, generated metadata, layout assertions, cooked artifacts, C++ producers, and every shader consumer together;
- delete `RenderViewData`, `RenderCamera`, `PerFrameDataBuilder`, `PerViewDataBuilder`, `TemporalDataBuilder`, the four old constant-buffer data types, their headers, registration names, and shader layouts.

Internal checkpoint only - not landable: all pass-visible camera, viewport, temporal, culling, and raster batch data is reachable through `RenderView`; no process-global implicit view history, `mainView` special-case name, old view builder, or old shader ABI remains.

### Phase 4 - Move scene GPU capabilities under the scene

- rename and move `PersistentRenderGpuScene` beneath `RenderScene` as `RenderGpuScene`;
- update it from `PreparedRenderScene` using the current frame slot and preserve dirty-range/revision behavior;
- replace `RenderSceneGpuData` with `RenderSceneGpuBindings` and keep graph import handles in graph types;
- move logical ownership of `RenderRayTracingScene` beneath `RenderScene`;
- separate view-independent traceable inputs, view-dependent planning, and persistent acceleration resources;
- keep GPU Scene and ray tracing as separate subordinate capabilities;
- update reset, device-loss, resize, and retirement behavior without a device-idle shortcut;
- delete the old GPU-scene type, scene-GPU-data type, paths, ownership members, and reset route after all consumers use the scene-owned capabilities.

Internal checkpoint only - not landable: `FramePipeline` owns no persistent scene/GPU-scene/RT-scene data, scene resources survive across frames under one owner, frame-slot bindings remain valid through submission retirement, and no second capability instance or ownership route exists.

### Phase 5 - Remove broad pass contexts

- inventory each `FrameContext` and `PassRuntimeContext` consumer and move it to explicit pass setup or constructor injection;
- copy only used frame/view/scene uniform values into pass parameters;
- inject mesh drawing access into the GBuffer mesh owner;
- capture provider objects in provider passes for the graph generation that owns them;
- give ray-tracing passes only the RT scene/capabilities/settings they consume;
- move history validity and display settings into their specific pass parameters;
- narrow execution infrastructure to commands, declared resource resolution, and diagnostics;
- delete `FrameContext`, `PassRuntimeContext`, related pointers/null checks, and unused `TextureCache` exposure.

Internal checkpoint only - not landable: repository search finds no broad context types, aliases, adapter overloads, or context-shaped replacements, and a pass cannot access a cache, provider, history, scene, or view it did not declare.

### Phase 6 - Make frame orchestration read as stages

- replace `BuildFrame`/`FrameBuildSettings`/`FrameBuildResult` with `BuildDeferredFrameGraph`/`DeferredFrameGraphSettings`, returning `DeferredFrameGraphResources` directly;
- replace `FrameAssemblyResourceLayout` and its nested `FrameAssembly*` types with `DeferredFrameGraphResources`, `DeferredTransientResources`, `DeferredImportedSceneResources`, and `ViewportFrameProducts`;
- delete `FinalSceneColorProduced`; reconstruction or upscaling publishes one `ResolvedSceneColor` handle, whose validity is the single pre-presentation graph-product truth;
- keep graph topology keys local to the frame pipeline: render/output extent, presentation target, render modes, exposure metering topology, requested outputs, and provider graph key;
- isolate imported scene-resource binding, view-history binding, provider setup, capture, UI, and presentation into their focused existing owners;
- reduce `FramePipeline` to the stage sequence documented above;
- narrow `RendererHost` construction/getter surface so capabilities receive dependencies directly and passes never use it as a service locator;
- delete helpers and members that became forwarding-only or unused.

Internal checkpoint only - not landable: `FramePipeline` has one sentence of responsibility, graph topology is visibly separate from scene/view values, and no new orchestration wrapper, service locator, or forwarding helper duplicates the old owner.

### Phase 7 - Final atomic cutover and paired-backend acceptance

- run the final legacy-eradication gate below before broad validation and again against the exact candidate head;
- run focused scene-input, view construction, culling/batching, temporal invalidation, GPU-scene, ray-scene, frame-graph, and shader-ABI checks;
- run `architecture_boundary_check` if any Renderer/RHI boundary or include direction changed;
- build the affected DevelopmentEditor configuration with both D3D12 and Vulkan enabled;
- run the accepted Showcase smoke from `Projects/Showcase` on both backends;
- capture fixed-camera raster and ray-traced GBuffer results before/after and compare frame-graph products, history behavior, and material/light output;
- compare CPU scene preparation, view preparation, graph setup, upload bytes, GPU frame time, and memory high-water against Phase 0;
- run `git diff --check` and record unavailable checks honestly;
- inspect the complete diff and final repository state as one integration unit, including renames, deletions, CMake membership, generated/cooked output, tests, and documentation;
- update the implemented architecture map and change this document's status only after the exact candidate code and evidence prove every gate;
- merge, revert, or release only the complete migration unit; never split the accepted candidate into independently landed workstreams.

Final cutover candidate: one scene owner, one view owner, one explicit view-state owner, one frame-slot product, one shader ABI, and narrow pass parameters serve both backends; the final eradication gate has zero unexpected matches and no legacy path can compile, cook, execute, or be selected.

## Final Atomic Cutover Gate

The candidate is not landable until every gate below passes against the exact integration head. A passing workstream checkpoint cannot waive or defer a final gate.

| Gate | Required final state | Required proof |
| --- | --- | --- |
| Legacy runtime symbols | No definition, declaration, include, construction, parameter, member, call site, test fixture, shader declaration, or registration remains for `RenderInputFrame`, `RenderFrameMetadata`, `RenderWorldDelta`, `RenderFrameDynamicData`, `RenderCameraData`, `RenderWorld`, `RenderProxy`, `RenderSceneData`, `RenderViewData`, `RenderCamera`, `PerFrameDataBuilder`, `PerViewDataBuilder`, `TemporalDataBuilder`, `FrameContextBuilder`, `FrameContext`, `PassRuntimeContext`, `PersistentRenderGpuScene`, `RenderSceneGpuData`, `FrameAssemblyResourceLayout`, the nested `FrameAssembly*` types, `FrameBuildSettings`, `FrameBuildResult`, `FinalSceneColorProduced`, `PerFrameConstantBufferData`, `PerViewConstantBufferData`, `PerViewCameraConstantBufferData`, or `PerTemporalConstantBufferData`. The deferred graph has no old `BuildFrame` declaration or call site. | Whole-repository exact-symbol search over `Engine`, `Projects`, executable build files, shader sources, tests, and generators returns zero matches. This target document may retain old names only to define their deletion. |
| Legacy files and build membership | No old header/source/shader filename, abandoned directory, CMake source entry, install list, generated manifest entry, or test-data path remains. | `rg --files` inventory plus CMake/manifest searches; configure/build cannot discover an old file through a stale explicit or globbed entry. |
| Transition machinery | No feature flag, build switch, CVar, environment variable, runtime conditional, typedef, alias, conversion constructor, compatibility overload, adapter, legacy reader/writer, deprecated wrapper, or fallback can select or reconstruct the current architecture. | Diff review plus targeted search for migration vocabulary and old-to-new conversions; every temporary migration helper is absent from the candidate unless it is the canonical generator. |
| Single runtime authority | Exactly one `RenderScene`, one scene-owned `RenderGpuScene`, one scene-owned `RenderRayTracingScene`, one active `RenderViewState`, and one target frame/view preparation path exist for the current one-view renderer. `FramePipeline` owns none of those persistent capabilities. | Construction/destruction/reset/device-loss/resize/retirement trace; ownership tests; repository search for all allocations, members, getters, and reset paths. |
| Publication and naming | GameFramework/Application/Editor publish only `RenderFrameSubmission` containing the committed scene and view values. Renderer names, diagnostic labels, captures, tests, and current docs use the target vocabulary consistently. | Trace every producer and consumer; public-header and include graph inspection; no old packet or synonym can enter the render queue. |
| Scene/view separation | `PreparedRenderScene` contains no camera, frustum, viewport, exposure, view visibility, camera-distance ordering, raster batches, history state, or graph handles. `RenderView` contains the resolved current-view values and view-derived work but no persistent scene/GPU/history-resource owner. | Field-by-field owner audit, focused include-boundary checks, and scene/view construction tests. |
| Shader ABI | C++ and HLSL use only `FrameUniformData`, `ViewUniformData`, `ViewCameraUniformData`, `ViewTemporalUniformData`, and focused scene bindings. No old registration name, duplicated field, layout alias, shader macro bridge, or old cooked metadata is accepted. | Layout assertions, registration inventory, shader validation/cook, clean generated metadata, and D3D12/Vulkan builds against the same source revision. |
| Pass surface | Pass setup supplies narrow semantic parameters and recording receives only `PassCommandContext` infrastructure. No pass can recover a service locator through `RendererHost`, `RenderFrame`, an owner pointer, or a replacement context bag. | Consumer-by-consumer audit, explicit resource/use comparison, parallel recording checks, and zero broad-context matches. |
| Target wiring | Every new request field, generation, invalidation cause, state value, binding, and graph key has an intentional producer and consumer, or is deleted. No target field exists only to preserve the shape of the current packet. | Field-level producer/consumer inventory and focused behavior tests for each accepted intent. |
| Generated and cached output | Generated shader metadata, cooked shader packages, local generated manifests, and checked-in generated artifacts are regenerated from the target ABI and contain no current names or layouts. Disposable stale caches are excluded from evidence and cannot be packaged. | Regeneration from the candidate, artifact search, shader cook/validation, and package/build manifest inspection. |
| Documentation truth | Current-state maps and reviewer routes describe only the implemented target after the code passes. Historical analysis is clearly marked as history or target rationale. | Documentation link/status check and comparison with executable owners and build membership. |
| Atomic repository state | The exact candidate contains all code, shader, build, test, generated-artifact, and documentation changes. Mainline observes either the pre-migration head or this complete candidate, never an internal checkpoint. | Final full diff/state review and one merge transaction; required checks run on the exact candidate or resulting merge commit. Rollback reverts that complete unit. |

The legacy-symbol set is a floor, not a fixed allowlist. The Phase 0 inventory must add any current synonym, wrapper, file, registration label, or ownership route discovered before implementation. A final search match is resolved by deletion or an explicit correction to this target document before landing; it is never silently exempted because replacing it is inconvenient.

The final review also traces each target value from producer to last consumer and records its owner, mutator, lifetime, frame-slot behavior, and retirement rule. This catches an in-between state that happens to use new names while retaining old ownership or duplicate data.

## Validation Matrix

| Claim | Required evidence |
| --- | --- |
| Cross-thread ownership is unchanged or stronger | Render queue/input tests; no Renderer read of live `GameWorld`/Editor state; scene generation and sequence rejection tests |
| Scene and view are semantically separated | focused type/include checks; camera/frustum absent from scene prep; culling/batches absent from scene frame |
| Frame-slot lifetime is safe | maximum-frames-in-flight stress; resize/provider/scene reset; no borrowed epoch beyond retirement |
| Temporal state follows the correct view | viewport identity switch, scene generation change, camera cut/teleport, projection change, provider/shader generation change, and stable-view continuation tests |
| Shader ABI remains paired | C++ layout assertions, shader cook/validation, both backend builds |
| GPU Scene retains behavior | create/update/destroy, stable slot, deformation previous/current, material revision, dirty/no-change upload evidence |
| Ray tracing retains behavior | BLAS/TLAS plan/build tests, descriptor/device-address capability paths where currently supported, fixed-camera output comparison |
| Pass ownership is narrower | zero `FrameContext`/`PassRuntimeContext` references; declared graph resources match execute usage; parallel recording checks |
| No performance regression is hidden | same-workload CPU/GPU timings, upload bytes, memory high-water, graph rebuild frequency, and frames-in-flight evidence |
| Backend parity remains real | D3D12 and Vulkan build plus runtime/capture evidence; unsupported provider differences reported through capabilities |

The refactor should reuse existing tests and evidence surfaces. Add a permanent test only for a durable contract that lacks coverage; do not add refactor-only dashboards, periodic logs, or duplicated diagnostics.

## Risks And Failure Rules

- A `PreparedRenderScene` or `RenderView` reference escaping its frame slot is a lifetime failure, not a reason to add shared ownership everywhere.
- A cached frame-graph pass capturing stack or previous-slot data is invalid. Pass-specific payloads must live through graph execution and frame retirement.
- Moving raster culling out of scene preparation must preserve task dependencies, deterministic batch ordering, and current automatic-batching behavior.
- Previous object/deformation values are scene temporal state. Moving them to `RenderViewState` would break multi-view correctness.
- Exposure/TAA/reservoir/reference histories are view continuity. Leaving them as one process-global temporal builder would break viewport identity changes.
- TLAS resources are scene resources, but a camera-sensitive planning heuristic is render-invocation state. Do not force both into one lifetime.
- Graph resource handles are invalid outside their graph generation. Do not store them in persistent scene/view owners to simplify call signatures.
- Shader constant reclassification is an ABI change and must update every C++ producer, shader declaration, and consumer together.
- A temporary compatibility adapter or dual context path is more dangerous than a larger clean-break changelist because it creates two authorities.
- If a workstream cannot preserve behavior with a single owner, stop work on the migration branch and revise this target rather than adding a fallback or landing a partial state.

## What Sparkle Should And Should Not Copy

Copy from Unreal:

- the gameplay-to-renderer mirror boundary;
- a persistent retained render scene;
- stable primitive identity and dirty GPU-scene updates;
- explicit distinction between current view and persistent view state;
- per-view visibility/draw preparation;
- a short-lived render invocation that consumes scene plus view;
- family/local graph resource configuration rather than global topology state;
- narrow pass/resource declarations.

Do not copy from Unreal:

- class size accumulated for decades of features;
- `F` prefixes, inheritance trees, macros, or pointer-heavy APIs;
- a separate proxy/info object when one Sparkle record has one owner and lifetime;
- multi-view family infrastructure before a current workload needs it;
- global scene texture configuration;
- renderer-private fields inferred from one engine revision;
- per-render allocation patterns that conflict with Sparkle's cached graph and frame-slot reuse;
- feature flags or subsystems with no Sparkle producer, consumer, or acceptance workload.

## Completion Definition

The refactor is complete only when:

- it lands as one complete integration unit, and the final atomic cutover gate passes on the exact candidate with no deferred cleanup;
- `RenderScene` is the sole persistent renderer scene authority;
- GPU Scene and ray-tracing scene live under that authority as focused capabilities;
- scene publication contains no camera, viewport, exposure, provider, or renderer-convention metadata;
- `PreparedRenderScene` is immutable for its frame slot and contains no view-derived raster work;
- `RenderView` contains current camera/view policy and view-derived visibility/draw products;
- `RenderViewState` is the sole semantic owner of persistent per-view continuity;
- `RenderFrame` is a small lifetime owner and not a service bag;
- every current symbol, file, build entry, generated registration, cooked layout, alias, adapter, and fallback enumerated by the final gate is deleted;
- the committed GameFramework/Renderer/GPU naming contract is used across types, files, fields, diagnostics, tests, CMake, C++, and HLSL with no competing synonym;
- `FrameUniformData`, `ViewUniformData`, `ViewCameraUniformData`, `ViewTemporalUniformData`, and focused scene bindings are the only frame/view/scene ABI;
- graph topology/resources remain owned by the frame graph;
- `BuildDeferredFrameGraph` is the only deferred graph-construction entry point;
- passes use explicit semantic parameters plus narrow `PassCommandContext` infrastructure and cannot reach a broad context or service locator;
- `FramePipeline` sequences owned stages without owning scene/view capabilities;
- every target value has one producer, intentional consumers, one lifetime, and a tested invalidation/retirement rule; unsupported request fields are deleted rather than left inert;
- D3D12 and Vulkan builds, focused tests, runtime smokes, captures, and performance comparisons support the claims;
- the implemented architecture map and document status are reconciled with the proven code.

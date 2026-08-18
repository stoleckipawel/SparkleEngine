# Renderer Scene, View, And Frame Architecture

Status: target architecture, Unreal Engine reference analysis, and atomic implementation cutover plan; not implemented behavior
Date: 2026-08-18
Scope: GameFramework-to-Renderer publication, persistent render-scene ownership, GPU-scene ownership, one-frame scene and view products, temporal view state, deferred frame orchestration, frame-graph pass inputs, Unreal-familiar concept translation, coherent cross-module naming and directory navigation, complete legacy-path removal, atomic landing, D3D12/Vulkan validation, and cleanup of the current frame path

## Decision

Sparkle should adopt the lifetime and responsibility split used by Unreal's deferred renderer without copying Unreal's class size, inheritance, naming prefixes, or feature breadth:

1. `RenderScene` is the persistent render-coordinator-owned mirror of gameplay scene state.
2. `RenderGpuScene` and the ray-tracing scene are scene capabilities owned beneath `RenderScene`, not frame-pipeline state.
3. `PreparedRenderScene` is an immutable, frame-slot-owned projection of the scene used by one submitted frame.
4. `RenderView` is an immutable one-frame view: camera, matrices, frustum, rectangles, resolved view policy, temporal shader values, and view-derived visibility/draw products.
5. `RenderViewState` is the persistent continuity for one stable viewport/view identity: previous camera state, jitter sequence, semantic history validity, exposure continuity, and view-history invalidation state.
6. `RenderFrame` owns only one frame's identity, time, frame-in-flight slot, `PreparedRenderScene`, and current `RenderView`. It is a lifetime boundary, not a service locator.
7. `FramePipeline` sequences the render-thread frame and owns the cached frame-graph execution. It delegates scene preparation, view preparation, history, providers, presentation, and pass-specific work to their existing or newly clarified owners.
8. Frame-graph passes declare narrow pass parameters. `FrameContext`, `FrameContextBuilder`, `PassExecutionContext`, `PassRuntimeContext`, `RayTracingPassContext`, and `ImageProviderPassContext` are removed rather than renamed into new catch-all bags; `PassCommandContext` retains only recording infrastructure.
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
- Epic Games, [`IRendererModule`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/RenderCore/IRendererModule)
- Epic Games, [Renderer module API](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Renderer)
- Epic Games, [`FSceneInterface`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/FSceneInterface)
- Epic Games, [`FPrimitiveSceneInfo`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Renderer/FPrimitiveSceneInfo)
- Epic Games, [`FSceneViewInitOptions`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/FSceneViewInitOptions)
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
- any of `FrameContext`/`PassExecutionContext`/`PassRuntimeContext`/`RayTracingPassContext`/`ImageProviderPassContext` coexist with the target explicit pass parameters and `PassCommandContext` execution surface;
- both old and new constant-buffer layouts are cooked or accepted;
- both frame-pipeline-owned and scene-owned GPU/RT scene instances can exist;
- an adapter or alias makes an old call site compile without adopting the new owner;
- target owners are split between the old `SceneData/`, `Camera/`, `FramePipeline/`, or generic `Frame/Core/` roots and the target `Scene/`, `View/`, or `Frame/` roots;
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

## Unreal-Familiar Translation And Navigation Contract

An engineer who knows Unreal should recognize the major nouns and lifetimes, then be able to find Sparkle's implementation without learning an unrelated vocabulary. Familiarity is a navigation aid, not a requirement to reproduce Unreal's type graph.

### Concept translation

| Unreal mental-model anchor | Sparkle target | Where to start | Deliberate Sparkle difference |
| --- | --- | --- | --- |
| `UWorld` and renderable components | `GameWorld` and world extraction | `Engine/GameFramework/Private/World/Extraction` | Gameplay remains the mutable authority and publishes owned render values. Renderer code never borrows the live gameplay object graph. |
| `FSceneInterface` mutation boundary | `RenderFrameSubmission` containing `RenderSceneUpdate` | `Engine/GameFramework/Public/Rendering`, then Renderer input acceptance | The owned cross-thread packet is already Sparkle's module boundary, so a second scene interface would only forward calls. |
| private `FScene` | `RenderScene` | `Engine/Renderer/Private/Scene/RenderScene.*` | Same retained render-scene role, without Unreal's feature breadth or public/private interface pair. |
| `FPrimitiveSceneProxy` plus `FPrimitiveSceneInfo` | `RenderPrimitive` | `Engine/Renderer/Private/Scene/RenderPrimitive.*` | One plain persistent record combines the mirror and scene-membership roles because Sparkle has no separate polymorphic proxy lifetime. |
| `FGPUScene` | `RenderGpuScene` | `Engine/Renderer/Private/Scene/GpuScene` | Same scene-owned persistent GPU representation and dirty-update concept, using Sparkle frame slots and RHI abstractions. |
| `FSceneViewInitOptions` | `RenderViewInput` plus `ViewportRenderRequest` | GameFramework `Public/Rendering`, then `Renderer/Private/View/RenderViewBuilder.*` | Authored camera publication and renderer-owned viewport/output policy remain separate until view construction. |
| `FSceneView` plus renderer-private `FViewInfo` | `RenderView` plus `RenderViewPreparation` | `Engine/Renderer/Private/View` | One immutable view value carries the current view and renderer-private view products; there is no inheritance layer or duplicate base/derived storage. |
| `FSceneViewStateInterface` and private view state | `RenderViewState` | `Engine/Renderer/Private/View/RenderViewState.*` | One concrete owner is enough for the current single-view renderer. History resources remain with the graph/providers. |
| `FSceneViewFamily` | no target type yet | `RenderFrame` for frame-slot lifetime; deferred graph settings for shared topology policy | `RenderFrame` is not renamed to view family. Add `RenderViewFamily` only when one invocation truly owns multiple simultaneous views. |
| `FSceneRenderer` / `FDeferredShadingSceneRenderer` | `FramePipeline` plus `BuildDeferredFrameGraph` | `Engine/Renderer/Private/Frame/FramePipeline.*`, then `Frame/Deferred` | Sparkle separates the persistent lifecycle sequencer from deferred graph topology instead of creating one large renderer-invocation class. |
| scene textures and RDG resource collections | `DeferredFrameGraphResources` | `Engine/Renderer/Private/Frame/Deferred` | These are graph handles grouped by graph role, never persistent `RenderScene` or `RenderView` fields. |
| RDG builder/compiler/executor | `FrameGraphBuilder` and `FrameGraph` infrastructure | `Engine/Renderer/Private/FrameGraph` | This folder is rendering-feature-agnostic graph machinery; deferred pass order does not live here. |
| RDG pass parameter structs | pass-specific parameter structs plus `PassCommandContext` | `Engine/Renderer/Private/Passes` | Semantic inputs are explicit; the execution context contains only commands, declared resources, and diagnostics. |

The most important non-equivalences are intentional: `RenderFrame` is not `FSceneViewFamily`, `FramePipeline` is not a retained scene, `PreparedRenderScene` is not a second scene, and `PassCommandContext` is not an Unreal-style renderer context. Naming one of these after a superficially similar Unreal type would make navigation worse by promising the wrong lifetime.

### Local naming grammar

| Word or suffix | Reserved meaning |
| --- | --- |
| `GameWorld` | gameplay-owned mutable world authority |
| `RenderScene` / `Scene` | persistent renderer mirror or view-independent scene concern |
| `View` | one camera/viewport projection and work derived from it |
| `Frame` | one publication, frame-in-flight lifetime, or whole-frame sequencing concern |
| `Input` | immutable values crossing into an owner; never retained mutable state |
| `Update` / `Delta` | accepted mutation intent; `Delta` is the changed payload inside the update envelope |
| `Prepared` | immutable derived CPU product for a bounded frame slot, not another authority |
| `State` | persistent semantic continuity mutated only by its named owner |
| `Resources` | owned resources or graph handles grouped by a clearly named lifetime/domain |
| `Bindings` | narrow non-owning projection used to bind owner-held resources |
| `UniformData` | CPU/HLSL ABI value with paired layout validation |
| `Builder` | short-lived construction logic with no persistent semantic ownership |
| `Pipeline` | ordered lifecycle sequencing, not a service registry or data owner |
| `Context` | allowed only for narrow operation infrastructure; it must name that operation, as in `PassCommandContext` |

Sparkle keeps its normal C++ style and does not import Unreal's prefix scheme: do not add `F`, `U`, `A`, `T`, or `E` prefixes, a blanket `I` rule, a `b` boolean rule, or Unreal macro vocabulary merely for familiarity. Existing intentional Sparkle interface names remain governed by their own contracts. Adopt domain nouns only where the semantic contract matches. Generic names such as `Manager`, `Data`, `Info`, `Context`, `Core`, or `Common` are not architectural destinations; they require a domain-qualified value role or are removed.

### Directory navigation rule

The path tells the same ownership story as the type name:

| Directory | Owns | Must not become |
| --- | --- | --- |
| `Scene/` | retained `RenderScene`, `RenderPrimitive`, scene-owned GPU/ray-tracing capabilities, materials, and view-independent preparation | a home for camera, viewport, visibility, or graph topology |
| `View/` | `RenderView` construction, `RenderViewState`, visibility, sorting, batching, and other view-derived work | a second scene store or owner of history textures |
| `Frame/` | `FramePipeline`, `RenderFrame`, frame identity/retirement, and deferred graph assembly | a broad bag of scene, view, pass, and service implementations |
| `Passes/` | feature pass setup, pass-specific parameters, and recording behavior grouped by rendering feature | persistent scene/view state or generic graph machinery |
| `FrameGraph/` | graph declaration, compilation, resource lifetime/barriers, recording plans, and execution infrastructure | deferred-renderer policy or feature-specific pass order |
| `ShaderData/` | C++ values that mirror shader ABI layouts | shader reflection/binding machinery or renderer state owners |
| `ShaderParameters/` | generic parameter-field binding and reflection mechanics | semantic frame, scene, or view storage |
| `Host/` | composition and lifetime wiring at the renderer boundary | a service locator used by passes or preparation code |

New code goes to the narrowest existing owner above. The migration moves the touched `SceneData/`, `Camera/`, `FramePipeline/`, and generic `Frame/Core/` responsibilities into this target layout and deletes the emptied roots. It does not opportunistically rearrange unrelated renderer subsystems merely to make the tree resemble Unreal.

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

Delete `FrameContext`, `FrameContextBuilder`, `PassExecutionContext`, `PassRuntimeContext`, `RayTracingPassContext`, and `ImageProviderPassContext` after every consumer is assigned to explicit setup, a pass-specific value, a captured focused capability, or deletion. Replace only the recording surface with `PassCommandContext`. Do not introduce `RenderContext`, `RendererServices`, `FrameResources`, or another struct whose purpose is "everything passes may need."

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
    Materials/...                              scene-owned material tables/caches
    GpuScene/RenderGpuScene.*                  persistent GPU scene capability
    RayTracing/RenderRayTracingScene.*         persistent RT scene capability
    Preparation/RenderScenePreparation.*       view-independent frame product
    Preparation/PreparedRenderScene.h
    GpuScene/RenderSceneGpuBindings.h          narrow GPU binding projection
  View/
    RenderView.h                               immutable current view
    RenderViewBuilder.*
    RenderViewState.*                          persistent temporal semantics
    RenderViewPreparation.*                    visibility and view draw work
  Frame/
    FramePipeline.*                            thin lifecycle sequencer
    RenderFrame.h                              frame-slot lifetime owner
    RenderFrameIdentity.h
    Retirement/FrameExecutionRetirementQueue.*
    Deferred/BuildDeferredFrameGraph.*
    Deferred/DeferredFrameGraphResources.h
  Passes/                                      feature-specific setup/recording
    Deferred/...
    RayTracing/...
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
- Delete `RenderInputFrame`, `RenderFrameMetadata`, `RenderWorldDelta`, `RenderFrameDynamicData`, `RenderWorld`, `RenderProxy`, `RenderSceneData`, `RenderViewData`, `RenderCamera`, `PerFrameDataBuilder`, `PerViewDataBuilder`, `TemporalDataBuilder`, `FrameContextBuilder`, `FrameContext`, `PassExecutionContext`, `PassRuntimeContext`, `RayTracingPassContext`, `ImageProviderPassContext`, `PersistentRenderGpuScene`, `RenderSceneGpuData`, the `FrameAssembly*` types, `FrameBuildSettings`, `FrameBuildResult`, and the four old constant-buffer data types as their responsibilities move.
- Do not add `RenderViewFamily` until one render invocation genuinely contains multiple views.
- Do not split `RenderPrimitive` into proxy/info/interface hierarchies without a demonstrated owner boundary.
- Do not add a generic renderer service registry, event bus, visitor, or manager.
- Do not introduce an Unreal-style global scene-texture singleton.
- Do not copy Unreal feature flags, post-process settings, or scene subsystems that Sparkle does not implement.
- Do not retain compatibility aliases, legacy packet readers, dual shader constant layouts, or old/new context paths.
- Do not add permanent diagnostics or dashboards for the refactor. Use existing timings, captures, tests, and one-time comparison evidence.
- Do not claim a performance improvement from moving fields. Measure preparation, graph setup, upload, recording, and frame time on the accepted workloads.

## Implementation Authority And Phase Contract

### Authority and conflict resolution

This document owns the newer Scene/View/Frame target decision and exact renderer-specific vocabulary within its stated scope. The engineering standards continue to own repository-wide implementation, safety, lifetime, formatting, validation, and evidence rules.

Apply conflicts as follows:

1. Preserve the invariant from the current owning standard: single authority, explicit lifetime, clean break, supported behavior, deterministic publication, backend parity, and proportional evidence are never weakened by this plan.
2. When an older renderer-specific example, type name, directory, or context spelling conflicts with this target, the target spelling and ownership win for this migration.
3. Update the owning standard, architecture route, executable configuration, and affected code in the same phase CL that makes the conflicting old rule obsolete. Do not add a local exception, suppress a check, or retain both spellings.
4. Executable policy such as `.clang-format`, `.clang-tidy`, compiler settings, CMake membership, and boundary checks is reconciled rather than bypassed. If the target truly requires a policy change, update the owning policy and its evidence in that phase CL.
5. Unreal remains external precedent. It cannot override a Sparkle owner, standard, naming rule, backend boundary, or measured result.

The current standards reconciliation keeps the useful `Context` rule while changing its application: `RenderCommandContext` remains the command-recording wrapper, `PassCommandContext` becomes the narrow frame-graph recording surface, and broad semantic context bags are deleted. The target also changes the Renderer module vocabulary from render world to persistent render-scene mirror without changing the GameFramework/Renderer dependency boundary.

### Required standards for every phase

Every phase implementation prompt requires these local authorities:

- [Integration Style Guide](../Engineering/Standards/IntegrationStyleGuide.md), especially the current clean-break policy;
- [Change Process](../Engineering/Standards/ChangeProcess.md), including ownership-path search, touched-neighborhood reconciliation, and completion reporting;
- [Repository Structure and Ownership](../Engineering/Standards/RepositoryStructureAndOwnership.md) for module direction, orchestration/capability separation, lifetime, and folder cohesion;
- [Coding Style](../Engineering/Standards/CodingStyle.md) and executable `.clang-format`/`.clang-tidy` policy;
- [Naming and Vocabulary](../Engineering/Standards/NamingAndVocabulary.md) plus this document's target naming contract;
- [Data-Oriented Design](../Engineering/Standards/DataOrientedDesign.md) for single truth, copy budget, identity, frame slots, and projections;
- [Validation, Performance, and Evidence](../Engineering/Standards/ValidationPerformanceAndEvidence.md) for claim-driven checks, performance classification, and removal of temporary proof machinery;
- this entire target document, not only the selected phase.

Phase-specific references below add to this set. If any link, standard, current owner, producer, consumer, build entry, or target assumption has changed, reconcile it before editing and update the prompt/plan rather than improvising a second architecture.

### Phase CL and no-intermediate-build rules

- Each phase is one logical, reviewable CL/commit on the private migration branch. Do not commit a phase until all of its scoped producers, consumers, moves, build entries, documentation, and legacy deletions meet that phase's acceptance criteria.
- A phase CL is a source-control and review boundary, not a supported architecture. It is not independently mergeable, releasable, cherry-pickable to mainline, or proof that the engine builds or runs.
- Mainline receives the complete phase series through one ref update and one reviewed integration transaction, using a merge or squash strategy that never lands an individual phase by itself.
- Phases 0 through 6 do not configure, compile, link, compile shaders, cook, launch, capture, or run performance workloads. Use exact searches, scoped diff inspection, CMake/include audits, documentation checks, `git diff --check`, and no-write formatting only. Executable acceptance occurs once in Phase 7 against the complete candidate.
- If a valid before-change executable/performance baseline does not already exist, acquire it before Phase 0 edits begin. Record its source revision and environment. Do not build a new baseline between migration phases.
- No-build phase acceptance makes no compilation or runtime claim. It proves source-level ownership closure, cleanup, and reviewability only.
- Update existing durable tests when they are consumers of a changed contract. Do not add submitted test fixtures, executables, probes, or CTest registrations without explicit user authorization; temporary local validation code is removed before its phase commit.
- If a phase cannot delete its assigned legacy concept because an unplanned consumer remains, move that consumer into the same phase or revise the phase boundaries before committing. Do not bridge the gap with an alias, adapter, overload, feature flag, fallback, or duplicate directory.
- Inspect and preserve unrelated dirty work before every phase. A phase CL contains only its owned migration scope and the directly required standards/documentation reconciliation.
- Phase 7 failures are fixed in the owning phase and folded into that phase CL before final review. Do not accumulate a miscellaneous final "make it build" commit that obscures ownership.

## Migration Phase Implementation Prompts

### Phase 0 - Freeze authority, inventory, invariants, and baseline

#### Implementation prompt

> Implement Phase 0 of the Renderer Scene/View/Frame migration as one documentation and inventory CL on the private migration branch. Apply every required standard above. Reconcile current code and executable policy, freeze the exact target vocabulary and owner map, assign every legacy definition and consumer to one later phase, and remove stale/conflicting documentation in scope. Do not change runtime source and do not configure, build, compile shaders, cook, launch, or capture. This CL is not independently landable.

#### Phase-specific references

- [Documentation root and authority map](../README.md)
- [Engineering Standards map](../Engineering/Standards/README.md)
- [Whole Repository Architecture Map](WholeRepositoryMap.md)
- [Renderer and RHI Architecture Boundary](RendererRhiBoundary.md)
- [SparkleEngine Code Review](../Engineering/CodeReview.md)

#### Required work

- re-run exact symbol, file, producer/consumer, include, CMake, shader-registration, generated-metadata, cooked-artifact, diagnostic-label, and current-document searches for every current and target name in this document;
- classify every field in `RenderSceneData`, `FrameContext`, `RenderViewData`, `PassRuntimeContext`, `RayTracingPassContext`, and `ImageProviderPassContext` as persistent scene, scene frame, current view, view state, frame identity/time, graph topology, pass parameter, focused owner dependency, or deletion;
- record the mutable owner, lifetime owner, producer, consumers, frame-slot/retirement behavior, copy justification, and target phase for every retained value;
- freeze exact target names and paths, the current one-view scope, dependency direction, and the legacy-eradication search set;
- reconcile renderer-specific examples in standards and architecture routes with the newer target while retaining their general ownership, lifetime, clean-break, and evidence invariants;
- record the exact revision/provenance of existing D3D12/Vulkan, shader, runtime, capture, and performance baseline evidence, or mark the corresponding final claim blocked if no valid baseline was acquired before edits;
- identify unrelated dirty work and define the path-level exclusion list for subsequent CLs.

#### Positive guardrails

- use exact `rg`/`rg --files` evidence and bounded owner/consumer reads;
- keep the inventory in the CL description or this existing implementation plan, not a new runtime reporting system;
- assign each old symbol, file, field, and directory to exactly one deletion phase;
- retain a standards invariant when replacing an obsolete example or spelling.

#### Negative guardrails

- do not edit runtime code, add scaffolding, pre-create target types, rename files, or introduce migration utilities;
- do not mark the architecture implemented or claim current build/runtime evidence;
- do not use familiarity with Unreal as the sole owner/lifetime justification;
- do not defer an unowned legacy item to generic "cleanup later."

#### Acceptance criteria (AC)

- every current field and material consumer has one target owner or explicit deletion decision;
- every rejected symbol/path is assigned to exactly one later phase and the final search floor includes it;
- every standards conflict is resolved in its owning document or assigned to the same phase as the corresponding code removal;
- baseline evidence has exact provenance or the affected Phase 7 claim is explicitly marked blocked;
- local documentation links resolve, the scoped diff contains no runtime code, and `git diff --check` passes;
- stale renderer-specific documentation in scope is removed or clearly marked historical/target rather than current implementation.

#### CL boundary

Suggested title: `Renderer: freeze atomic scene-view-frame migration contract`.

Commit only the reconciled plan, standards/routes, and inventory evidence. This phase cleans obsolete documentation and vocabulary; it does not preserve conflicting old rules for a later documentation-only cleanup.

### Phase 1 - Clean-break the publication boundary

#### Implementation prompt

> Implement Phase 1 as one source-consistent publication-boundary CL on the private migration branch. Replace the old GameFramework-to-Renderer frame packet, update every producer and consumer, and delete all old packet types and names owned by this phase. Apply the common standards and phase-specific references. Do not add compatibility and do not build, compile shaders, cook, launch, or run tests. Do not commit until the Phase 1 legacy search is clean. This CL is not independently landable.

#### Phase-specific references

- [GameFramework and ECS](../Engineering/Standards/GameFrameworkAndEcs.md)
- [Concurrency](../Engineering/Standards/Concurrency.md)
- [Multithreaded Engine Architecture](Multithreading/MultithreadedEngineArchitecture.md)
- [Editor Viewport Camera Architecture](EditorViewportCamera.md)
- [World Coordinate, Units, and Transform Contract](WorldCoordinateAndUnits.md)

#### Required work

- replace `RenderInputFrame`, `RenderFrameMetadata`, `RenderWorldDelta`, `RenderFrameDynamicData`, and `RenderCameraData` with `RenderFrameSubmission`, `RenderSceneUpdate`, `RenderSceneDelta`, `RenderSceneDynamicData`, `RenderViewInput`, and `RenderViewCameraData`;
- update GameFramework extraction, Application/editor view publication, the render queue, Renderer input acceptance, capture metadata, provider frame inputs, diagnostics labels, existing test consumers, includes, filenames, and CMake membership together;
- keep camera/cut/teleport in `RenderViewInput`; keep objects, lights, joints, and morph values in `RenderSceneDynamicData`;
- make scene generation single-source in `RenderSceneDelta`; keep `FrameId` at the submission root;
- capture shader/provider generations from their Renderer owners when constructing `RenderFrameIdentity`;
- derive renderer history invalidation from explicit scene, view, topology, shader, and provider events rather than an input reset flag;
- delete render/output extent, exposure, renderer convention, provider/shader generation, and reset-policy fields from the GameFramework packet.

#### Positive guardrails

- preserve immutable, owned, one-way publication and sequence/generation rejection;
- keep GameFramework independent of Renderer/RHI and keep Editor/Application publication value-based;
- use `RenderObjectId`, resource handles, moves, spans, and bounded packet storage according to the copy budget;
- update every source/build/document consumer in this CL even though executable validation waits for Phase 7.

#### Negative guardrails

- no old/new packet conversion, alias, overload, reader/writer, feature flag, or dual queue payload;
- no Renderer mutation of a published GameFramework value and no Renderer read of `GameWorld` or Editor state;
- no camera, viewport, exposure, graph key, shader/provider generation, or renderer convention in scene dynamic data;
- no speculative multi-view family, serialization version, or packet compatibility layer.

#### Acceptance criteria (AC)

- exact runtime/build searches return zero definitions or uses of the five old packet/data type names assigned to this phase;
- `RenderFrameQueue` has one payload type and all publication/acceptance sites use it directly;
- every target packet field has one producer and intentional consumer, with unsupported old fields deleted;
- scene generation and `FrameId` each have one authoritative publication location;
- includes, filenames, CMake entries, diagnostics, captures, existing test consumers, and current documentation use only target vocabulary;
- scoped diff inspection, exact stale-name searches, no-write formatting, and `git diff --check` pass; no executable check is claimed.

#### CL boundary

Suggested title: `Renderer: replace frame publication with scene and view inputs`.

The CL must delete the old packet headers/sources and their build entries. If any consumer still needs an old type, the phase is incomplete and must not be committed.

### Phase 2 - Establish the persistent `RenderScene` and prepared scene frame

#### Implementation prompt

> Implement Phase 2 as one persistent-scene ownership CL on the private migration branch. Replace `RenderWorld`, `RenderProxy`, and mixed `RenderSceneData` with the committed `RenderScene`, `RenderPrimitive`, `RenderScenePreparation`, and immutable frame-slot `PreparedRenderScene` model. Update all direct consumers and delete the phase-owned legacy representation and paths. Do not build or create a compatibility bridge. This CL is not independently landable.

#### Phase-specific references

- [Repository Structure and Ownership](../Engineering/Standards/RepositoryStructureAndOwnership.md)
- [Data-Oriented Design](../Engineering/Standards/DataOrientedDesign.md)
- [Concurrency](../Engineering/Standards/Concurrency.md)
- [Graphics Engineering](../Engineering/Standards/GraphicsEngineering.md)
- [Renderer and RHI Architecture Boundary](RendererRhiBoundary.md)

#### Required work

- clean-break rename `RenderWorld` to `RenderScene` and `RenderProxy` to `RenderPrimitive` across source, files, members, diagnostics, documentation, and build membership;
- move retained scene code from `Renderer/Private/SceneData` to `Renderer/Private/Scene` and keep one stable `RenderObjectId`/GPU-scene-slot relationship;
- make `RenderScene` the sole mutable renderer scene authority for primitives, materials, texture table, sky, lights, revisions, accepted sequence, dirty state, and scene-owned continuity;
- split view-independent preparation into `RenderScenePreparation`, publishing immutable `PreparedRenderScene` storage reused by frame-in-flight slot;
- move scene revisions, all primitive records/bounds, material values/bindings, lights, sky, deformation values, previous object/deformation values, and view-independent ray inputs into the prepared product or scene owner according to lifetime;
- remove frustum, camera position, visibility, camera-distance ordering, raster batching, viewport, history, and graph handles from scene preparation;
- preserve existing task-graph dependency, deterministic merge, exclusive output, cancellation, and frame-slot lifetime contracts while relocating their owner.

#### Positive guardrails

- one mutable `RenderScene`; all prepared values derive one-way from it;
- justify every copied/owned field by immutable publication, task isolation, frame-slot lifetime, or GPU ABI; prefer moves, spans, handles, and stable indices;
- keep one plain `RenderPrimitive` unless two real owners/lifetimes are proven;
- make view independence visible in includes, function signatures, fields, and folder placement.

#### Negative guardrails

- no `RenderSceneInterface`, proxy/info pair, subclass hierarchy, second scene snapshot, or Unreal-sized feature framework;
- no camera/frustum/view policy in `RenderScene` or `PreparedRenderScene`;
- no frame-graph handles, RHI service locator, presentation, provider, UI, or capture ownership in the scene;
- no `SceneData2`, alias, forwarding facade, duplicated material/mesh/cache objects, or parallel old/new directory tree.

#### Acceptance criteria (AC)

- runtime/build searches return zero `RenderWorld`, `RenderProxy`, and `RenderSceneData` definitions/uses outside historical target rationale;
- there is exactly one `RenderScene` construction, mutation, reset, and destruction ownership route;
- `PreparedRenderScene` has a documented producer, frame-slot owner, seal point, consumers, and retirement/reuse condition;
- scene preparation signatures and includes do not require view/frustum/camera types, and prepared fields contain no view-derived work;
- every moved file has reconciled includes/CMake membership and the old defining files/empty paths are deleted;
- scoped diff, owner/consumer searches, no-write formatting, and `git diff --check` pass without claiming compilation.

#### CL boundary

Suggested title: `Renderer: establish persistent render scene and prepared scene frame`.

This CL owns complete deletion of the old scene/primitive/mixed-scene representation. GPU-scene lifetime relocation remains Phase 4, but Phase 2 may not retain a second CPU scene authority to ease that later move.

### Phase 3 - Establish `RenderView`, `RenderViewState`, and the paired shader ABI

#### Implementation prompt

> Implement Phase 3 as one current-view, persistent-view-state, visibility, and paired C++/HLSL ABI CL on the private migration branch. Replace every old camera/view/temporal builder and constant-buffer layout, move view-derived work under `View`, update all source consumers, and delete stale generated/cooked representations. Do not build, compile shaders, cook, or retain old layouts. This CL is not independently landable.

#### Phase-specific references

- [Editor Viewport Camera Architecture](EditorViewportCamera.md)
- [World Coordinate, Units, and Transform Contract](WorldCoordinateAndUnits.md)
- [Graphics Engineering](../Engineering/Standards/GraphicsEngineering.md)
- [Shader Authoring and Cooked Program Architecture](Shaders/ShaderAuthoringAndCookedPrograms.md)
- [Naming and Vocabulary](../Engineering/Standards/NamingAndVocabulary.md)

#### Required work

- replace `RenderCamera`, `RenderViewData`, `PerFrameDataBuilder`, `PerViewDataBuilder`, and `TemporalDataBuilder` with `RenderViewBuilder`, immutable `RenderView`, and persistent `RenderViewState`;
- move camera construction and view-owned work from `Camera`, `Frame/Builders`, and mixed scene preparation into `Renderer/Private/View`;
- construct matrices, frustum, rectangles, extents, and current camera values from immutable `RenderViewInput` plus the actual `ViewportRenderRequest`;
- wire request identity, view kind/mode, feature flags, output flags, and exposure to the view or graph topology key; delete any unsupported field instead of leaving it inert;
- move visibility, camera distance, sorting, batching, raster work, workload summary, and view-sensitive ray planning into `RenderViewPreparation`;
- key `RenderViewState` by stable view identity and make it the sole semantic owner of previous matrices/jitter, temporal sample, continuity, and invalidation reasons;
- replace C++ and HLSL layouts and registrations with `FrameUniformData`, `ViewUniformData`, `ViewCameraUniformData`, and `ViewTemporalUniformData`, using identical ABI field names;
- delete stale generated metadata and cooked output derived from old layouts; Phase 7 regenerates canonical output once against the complete source candidate.

#### Positive guardrails

- one immutable current `RenderView` per current renderer invocation and one active persistent `RenderViewState` for the supported scope;
- keep current values in `RenderView`, previous/validity semantics in `RenderViewState`, and history resource ownership in graph/providers;
- preserve matrix spaces, units, `FovY`, `NearZ`, `FarZ`, jitter NDC naming, motion/depth conventions, and layout assertions;
- update C++, HLSL/HLSLI, shader registrations, parameter names, includes, files, diagnostics, existing test consumers, and build/cook manifests together at source level.

#### Negative guardrails

- no `RenderViewFamily`, stereo/multi-view framework, mutable camera mirror, global temporal builder, `mainView` special case, or duplicate base/derived view storage;
- no old/new constant buffers, shader macro bridge, registration alias, compatibility layout, or cooked package accepting the old ABI;
- no scene-wide data copied into the view and no history textures/provider objects owned by `RenderViewState`;
- no source-only placeholder field without an intentional producer and consumer.

#### Acceptance criteria (AC)

- exact searches return zero old camera/view/builder symbols, `mainView`, and the four old constant-buffer type/registration names in runtime, shader, build, test-consumer, generated, or cooked inputs;
- all view-dependent fields formerly in scene preparation have one owner under `RenderView`/`RenderViewPreparation`;
- all continuity/reset producers converge on `RenderViewState` with explicit stable identity and invalidation reasons;
- target C++ and HLSL ABI fields match by name/order/type and source layout assertions reflect only the new layouts;
- unsupported viewport request promises and old generated/cooked representations are deleted;
- scoped diff, source ABI inventory, no-write formatting, and `git diff --check` pass; shader compilation and runtime claims are deferred to Phase 7.

#### CL boundary

Suggested title: `Renderer: establish render view state and unified shader ABI`.

The CL is not committable while any old view builder/layout remains. Do not split the C++ and HLSL rename into separate commits.

### Phase 4 - Move GPU Scene and ray-tracing scene capabilities under `RenderScene`

#### Implementation prompt

> Implement Phase 4 as one persistent scene-GPU capability ownership CL on the private migration branch. Move GPU Scene and ray-tracing scene lifetime under `RenderScene`, preserve frame-slot and GPU-retirement correctness, update every allocation/reset/import consumer, and delete the old FramePipeline/RendererHost ownership routes and types. Do not build, run a backend, or add a fallback. This CL is not independently landable.

#### Phase-specific references

- [Graphics Engineering](../Engineering/Standards/GraphicsEngineering.md)
- [Renderer and RHI Architecture Boundary](RendererRhiBoundary.md)
- [Data-Oriented Design](../Engineering/Standards/DataOrientedDesign.md)
- [Concurrency](../Engineering/Standards/Concurrency.md)
- [Ray-Tracing Pipeline and Dual-Execution Delivery Plan](Shaders/RayTracingPipelineImplementationPlan.md)

#### Required work

- rename/move `PersistentRenderGpuScene` to scene-owned `Scene/GpuScene/RenderGpuScene` and replace `RenderSceneGpuData` with narrow `RenderSceneGpuBindings`;
- move logical ownership of `RenderRayTracingScene` beneath `RenderScene` in `Scene/RayTracing`, while keeping rendering passes in `Passes/RayTracing` and backend mechanics behind RHI;
- update GPU Scene from `PreparedRenderScene` for the selected frame slot, preserving stable slots, dirty ranges, revisions, capacity growth, frame boundaries, and token-based retirement;
- separate view-independent traceable inputs, view-dependent TLAS planning, and persistent BLAS/TLAS resources according to their lifetimes;
- keep graph import handles in `DeferredImportedSceneResources`, never in persistent scene capabilities;
- reconcile construction, reset, resize, device loss, shutdown, generation invalidation, frame-slot reuse, and delayed GPU completion;
- remove all old ownership members, getters, reset paths, allocation sites, files, includes, and CMake entries.

#### Positive guardrails

- one `RenderScene` owner with separate focused `RenderGpuScene` and `RenderRayTracingScene` capabilities;
- persistent indexed buffers plus deliberate dirty updates and bounded frame-indexed dynamic storage;
- explicit CPU owner, GPU token, replacement publication, and retirement rules;
- neutral Renderer policy and D3D12/Vulkan parity preserved for final validation.

#### Negative guardrails

- no merged GPU/RT god capability, duplicate capability instance, FramePipeline-owned persistent scene resources, or RendererHost service lookup;
- no frame-graph handle stored beyond graph generation and no view-dependent value promoted to persistent scene truth;
- no device-idle shortcut, unconditional full-scene rebuild/upload, vendor branch in neutral policy, or old/new reset fallback;
- no alias from old type names and no forwarding getter retaining old ownership.

#### Acceptance criteria (AC)

- searches show exactly one owner/allocation route for `RenderGpuScene` and `RenderRayTracingScene` and zero old `PersistentRenderGpuScene`/`RenderSceneGpuData` symbols;
- `FramePipeline` and `RendererHost` contain no persistent GPU-scene/ray-scene ownership member or lifecycle policy;
- dirty update, capacity replacement, frame-slot binding, device-loss/reset, and retirement paths have one source-level route and explicit lifetime comments only where non-obvious;
- graph imports consume narrow scene bindings without storing graph handles on persistent owners;
- paths, includes, CMake membership, diagnostics, and current documentation use the target names;
- scoped diff, ownership/allocation/reset searches, no-write formatting, and `git diff --check` pass without backend execution claims.

#### CL boundary

Suggested title: `Renderer: move GPU and ray-tracing scene lifetime under render scene`.

This phase cleans every old capability owner and name. Failure to prove one construction/reset/retirement route blocks the CL; it is not solved with a fallback instance.

### Phase 5 - Remove broad frame/pass contexts and make pass inputs explicit

#### Implementation prompt

> Implement Phase 5 as one pass-input and recording-surface CL on the private migration branch. Replace broad semantic contexts with pass-specific parameters and the narrow `PassCommandContext`, update every authored pass and frame-graph execution consumer, reconcile the naming standard, and delete all old context bags and forwarding access. Do not build or add context-shaped replacements. This CL is not independently landable.

#### Phase-specific references

- [Naming and Vocabulary](../Engineering/Standards/NamingAndVocabulary.md)
- [Repository Structure and Ownership](../Engineering/Standards/RepositoryStructureAndOwnership.md)
- [Validation, Performance, and Evidence](../Engineering/Standards/ValidationPerformanceAndEvidence.md)
- [Renderer and RHI Architecture Boundary](RendererRhiBoundary.md)
- [Render Dependency Graph](https://dev.epicgames.com/documentation/en-us/unreal-engine/render-dependency-graph-in-unreal-engine)

#### Required work

- inventory every field access through `FrameContext`, `PassExecutionContext`, `PassRuntimeContext`, `RayTracingPassContext`, and `ImageProviderPassContext` and assign it to pass setup, a pass-specific parameter record, constructor-captured capability, `RenderView`, `PreparedRenderScene`, or deletion;
- retain `RenderCommandContext` as the backend-neutral command wrapper and replace `PassExecutionContext` with `PassCommandContext` containing only command, declared-resource, and diagnostic infrastructure;
- copy only actually consumed frame/view/scene uniform values into pass parameters at setup;
- inject mesh drawing/cache access into its owning GBuffer mesh collaborator, capture provider objects in their graph-generation passes, and give ray-tracing passes only the exact scene/capability/settings they consume;
- move history validity, display/exposure values, and feature policy into the specific pass parameters that consume them;
- update graph executor, recording chunks, pass base operations, authored passes, includes, forwards, tests-as-consumers, naming standard examples, and CMake membership;
- delete all five old context types, builders, nullable pointer checks, generic getters, unused texture-cache exposure, and comments that endorse broad context growth.

#### Positive guardrails

- graph dependencies remain declared through graph/pass parameters and execution infrastructure remains semantic-free;
- pass objects capture long-lived pipeline/provider capabilities only for the graph generation that owns them;
- use references for required bounded access and pointers only for genuinely optional capabilities with an explicit reason;
- keep validation at the narrow owner rather than duplicating checks across passes.

#### Negative guardrails

- no `RendererServices`, `RenderContext`, `FrameResources`, `PassServices`, owner pointer, global registry, generic template accessor, or renamed service bag;
- no pass reach-through to `RendererHost`, `FramePipeline`, mutable `RenderScene`, provider stack, cache collection, or whole `RenderFrame` when it needs a smaller value;
- no compatibility overload accepting an old context and no staged old/new execute path;
- no permanent migration diagnostics, context field-use logging, or submitted test scaffold.

#### Acceptance criteria (AC)

- exact runtime/build searches return zero `FrameContext`, `FrameContextBuilder`, `PassExecutionContext`, `PassRuntimeContext`, `RayTracingPassContext`, and `ImageProviderPassContext` definitions/uses;
- `PassCommandContext` has only command recording, declared resource resolution, and diagnostics; it has no scene, view, frame semantics, cache/provider collection, RHI owner, or history policy;
- every former context field has one explicit destination or deletion record and every pass sees only declared semantic inputs;
- pass source contains no `RendererHost`/`FramePipeline` service lookup or generic context accessor;
- naming standards, files, includes, CMake, comments, and current docs use the target pass vocabulary;
- scoped diff, exact consumer searches, no-write formatting, and `git diff --check` pass without claiming compilation or recording execution.

#### CL boundary

Suggested title: `Renderer: replace broad pass contexts with explicit inputs`.

All context consumers and deletions belong in this one CL. A context retained for one difficult pass means the phase is incomplete.

### Phase 6 - Make frame orchestration and directory ownership match the target

#### Implementation prompt

> Implement Phase 6 as one frame-orchestration, graph-vocabulary, and physical-layout CL on the private migration branch. Make `FramePipeline` a thin lifecycle sequencer, rename deferred graph construction/resources exactly as specified, move files to the canonical Scene/View/Frame/Passes/FrameGraph paths, reconcile CMake/includes, and delete emptied roots and forwarding helpers. Do not build and do not add a `DeferredRenderer` facade. This CL is not independently landable.

#### Phase-specific references

- [Repository Structure and Ownership](../Engineering/Standards/RepositoryStructureAndOwnership.md)
- [Coding Style](../Engineering/Standards/CodingStyle.md)
- [Naming and Vocabulary](../Engineering/Standards/NamingAndVocabulary.md)
- [Renderer and RHI Architecture Boundary](RendererRhiBoundary.md)
- [Whole Repository Architecture Map](WholeRepositoryMap.md)

#### Required work

- replace `BuildFrame`, `FrameBuildSettings`, and `FrameBuildResult` with `BuildDeferredFrameGraph` and `DeferredFrameGraphSettings`, returning `DeferredFrameGraphResources` directly;
- replace `FrameAssemblyResourceLayout` and nested `FrameAssembly*` types with `DeferredFrameGraphResources`, `DeferredTransientResources`, `DeferredImportedSceneResources`, and `ViewportFrameProducts`;
- replace `FinalSceneColorProduced` with one initially invalid `ResolvedSceneColor` handle published by reconstruction/upscaling and consumed by presentation;
- keep render/output extent, presentation target, render modes, exposure metering topology, requested outputs, and provider graph key local to deferred graph/pipeline topology;
- make `FramePipeline` read as `BeginFrame`, `PrepareFrame`, `ExecuteFrame`, and `SubmitAndPresent`, delegating capability mechanics to their owners;
- move `FramePipeline.*` under `Frame`, dissolve touched generic `Frame/Core` files into `Frame`, `Frame/Deferred`, `View`, or deletion, and enforce the target directory navigation map;
- keep feature passes under `Passes`, generic graph compilation/execution under `FrameGraph`, shader ABI values under `ShaderData`, and binding/reflection mechanics under `ShaderParameters`;
- narrow `RendererHost` getters/constructor wiring, delete forwarding-only helpers/members/files, and reconcile all includes, filenames, CMake/source groups, exports, diagnostics, and documentation.

#### Positive guardrails

- one sentence of responsibility for `FramePipeline`: sequence the render-thread frame lifecycle and cached graph execution;
- one exact name/path per owner, with high-level workflow visible and mechanism in focused capabilities;
- graph handles remain graph-owned and product presence is represented by the authoritative handle;
- follow pinned formatting, header/source placement, include, namespace, and file/folder cohesion rules without unrelated reformatting.

#### Negative guardrails

- no parallel `DeferredRenderer`, scene renderer facade, generic `Core/Common/Helpers/Utilities/Manager` bucket, or one-method forwarding wrapper;
- no feature-pass order in generic `FrameGraph`, no pass implementations in `Frame`, and no semantic scene/view state in graph infrastructure;
- no old/new filenames or directories kept for include compatibility and no CMake entry for a moved/deleted file;
- no opportunistic reorganization of unrelated Renderer subsystems.

#### Acceptance criteria (AC)

- exact searches return zero old graph-construction/resource type names and `FinalSceneColorProduced` uses;
- `FramePipeline` owns no persistent scene/view/GPU/RT capability and its primary path visibly sequences the four documented stages;
- target owners exist only under canonical `Scene`, `View`, `Frame`, `Passes`, `FrameGraph`, `ShaderData`, and `ShaderParameters` locations;
- the old `SceneData`, `Camera`, `FramePipeline`, and touched generic `Frame/Core` roots are empty and removed, with no stale includes/CMake/source-group entries;
- `RendererHost` is composition-only and no downstream pass/preparation code uses it as a service locator;
- scoped diff, file/CMake/include inventory, no-write formatting, documentation links, and `git diff --check` pass without claiming a build.

#### CL boundary

Suggested title: `Renderer: finalize scene-view-frame ownership and navigation`.

The phase commits the physical architecture, not a cosmetic file shuffle. Every move must correspond to the owner map, and every emptied legacy path is deleted in this CL.

### Phase 7 - Regenerate, validate, reconcile, and atomically land

#### Implementation prompt

> Implement Phase 7 against the complete Phase 0-6 candidate. Run the final legacy-eradication gate first, regenerate canonical artifacts once, then perform claim-driven focused checks, shader validation, the required DevelopmentEditor D3D12/Vulkan build and Showcase runtime/capture evidence. Fix failures at their real owner and fold source fixes back into the owning phase CL. Update current architecture documentation only after all acceptance criteria pass. Land the complete phase series in one mainline integration transaction; never land an individual phase or add compatibility to make validation pass.

#### Phase-specific references

- [SparkleEngine Code Review](../Engineering/CodeReview.md)
- [Validation, Performance, and Evidence](../Engineering/Standards/ValidationPerformanceAndEvidence.md)
- [Graphics Engineering](../Engineering/Standards/GraphicsEngineering.md)
- [Renderer and RHI Architecture Boundary](RendererRhiBoundary.md)
- [Bistro and San Miguel Acceptance Workloads](../Engineering/BistroAndSanMiguelWorkloads.md), only where their gates are actually affected

#### Required work

- run the final legacy-eradication and navigation gates below before any build, fix every unexpected match in its owner, and repeat the gate on the rewritten exact candidate;
- regenerate shader metadata, cooked shader packages, manifests, and other canonical local/tracked artifacts from target sources; delete disposable stale caches rather than migrating them;
- run no-write format checks with the pinned formatter, `git diff --check`, applicable static/schema/documentation checks, and `architecture_boundary_check` when Renderer/RHI direction changed;
- compile/validate the smallest focused owning targets and shader layouts first, then build the affected `DevelopmentEditor` configuration with D3D12 and Vulkan enabled because the shared renderer/shader contract spans both;
- run focused scene-input, view construction, culling/batching, temporal invalidation, GPU-scene, ray-scene, frame-graph, retirement, and shader-ABI evidence using existing tests/harnesses without submitting new test scaffolding;
- run the accepted Showcase smoke from `Projects/Showcase` on D3D12 and Vulkan, then capture fixed-camera raster and ray-traced GBuffer results and compare products/history/material/light behavior;
- compare scene preparation, view preparation, graph setup, upload bytes, GPU frame time, memory high-water, graph rebuild frequency, and frames in flight against the provenance-matched Phase 0 baseline;
- inspect the complete diff/state, update the implemented repository map and this document's status only after code/evidence prove the target, and write the exact completion report required by Change Process.

#### Positive guardrails

- escalate validation from exact/source checks to focused compilation, then paired-backend runtime only because the final shared contract requires it;
- classify performance honestly as improves, preserves, or blocked with exact workload/hardware/build evidence;
- fold code fixes into the phase that owns the invariant, then rerun affected and final evidence on the exact candidate;
- preserve unsupported/unavailable evidence as an explicit blocked claim rather than simulated success.

#### Negative guardrails

- no compatibility alias, legacy reader, fallback path, disabled assertion, skipped backend, device-idle workaround, or widened context to make the candidate pass;
- no broad clean rebuild/all-content cook unless a specific final claim requires it or stale state makes narrower evidence inconclusive;
- no miscellaneous Phase 7 source-fix commit, permanent migration validator, new submitted test harness, or documentation claim ahead of executable proof;
- no partial merge, cherry-pick, fast-forward sequence, release, or status update for an individual phase.

#### Acceptance criteria (AC)

- every row of the Final Atomic Cutover Gate and applicable Validation Matrix passes on the exact final candidate;
- regenerated/tracked artifacts contain only target names/layouts and disposable old output is absent from package/build inputs;
- focused checks, shader validation, `DevelopmentEditor` D3D12/Vulkan build, both Showcase smokes, required captures, lifetime/retirement evidence, and applicable performance comparison have exact recorded results;
- the final diff contains no unrelated changes, temporary proof code, compatibility machinery, stale CMake/include/document references, or deferred cleanup;
- any unavailable required evidence makes the candidate `BLOCKED`; no mainline integration occurs;
- mainline receives the complete phase series in one reviewed integration transaction and rollback reverts that complete unit.

#### CL boundary

Suggested title: `Renderer: validate and accept scene-view-frame architecture`.

The Phase 7 CL contains canonical regenerated artifacts and final truthful documentation/evidence reconciliation only. Source fixes discovered here are folded into their owning Phase 1-6 CL before final review. The final candidate has one scene owner, one view owner, one view-state owner, one frame-slot product, one shader ABI, and narrow pass inputs on both backends.

## Final Atomic Cutover Gate

The candidate is not landable until every gate below passes against the exact integration head. A passing workstream checkpoint cannot waive or defer a final gate.

| Gate | Required final state | Required proof |
| --- | --- | --- |
| Legacy runtime symbols | No definition, declaration, include, construction, parameter, member, call site, test fixture, shader declaration, or registration remains for `RenderInputFrame`, `RenderFrameMetadata`, `RenderWorldDelta`, `RenderFrameDynamicData`, `RenderCameraData`, `RenderWorld`, `RenderProxy`, `RenderSceneData`, `RenderViewData`, `RenderCamera`, `PerFrameDataBuilder`, `PerViewDataBuilder`, `TemporalDataBuilder`, `FrameContextBuilder`, `FrameContext`, `PassExecutionContext`, `PassRuntimeContext`, `RayTracingPassContext`, `ImageProviderPassContext`, `PersistentRenderGpuScene`, `RenderSceneGpuData`, `FrameAssemblyResourceLayout`, the nested `FrameAssembly*` types, `FrameBuildSettings`, `FrameBuildResult`, `FinalSceneColorProduced`, `PerFrameConstantBufferData`, `PerViewConstantBufferData`, `PerViewCameraConstantBufferData`, or `PerTemporalConstantBufferData`. The deferred graph has no old `BuildFrame` declaration or call site. | Whole-repository exact-symbol search over `Engine`, `Projects`, executable build files, shader sources, tests, and generators returns zero matches. This target document may retain old names only to define their deletion. |
| Legacy files and build membership | No old header/source/shader filename, abandoned directory, CMake source entry, install list, generated manifest entry, or test-data path remains. | `rg --files` inventory plus CMake/manifest searches; configure/build cannot discover an old file through a stale explicit or globbed entry. |
| Transition machinery | No feature flag, build switch, CVar, environment variable, runtime conditional, typedef, alias, conversion constructor, compatibility overload, adapter, legacy reader/writer, deprecated wrapper, or fallback can select or reconstruct the current architecture. | Diff review plus targeted search for migration vocabulary and old-to-new conversions; every temporary migration helper is absent from the candidate unless it is the canonical generator. |
| Single runtime authority | Exactly one `RenderScene`, one scene-owned `RenderGpuScene`, one scene-owned `RenderRayTracingScene`, one active `RenderViewState`, and one target frame/view preparation path exist for the current one-view renderer. `FramePipeline` owns none of those persistent capabilities. | Construction/destruction/reset/device-loss/resize/retirement trace; ownership tests; repository search for all allocations, members, getters, and reset paths. |
| Publication and naming | GameFramework/Application/Editor publish only `RenderFrameSubmission` containing the committed scene and view values. Renderer names, diagnostic labels, captures, tests, and current docs use the target vocabulary consistently. | Trace every producer and consumer; public-header and include graph inspection; no old packet or synonym can enter the render queue. |
| Unreal-familiar navigation | The concept translation and directory table match executable ownership. Retained scene code is under `Scene`, current/persistent view code under `View`, lifecycle/deferred assembly under `Frame`, feature passes under `Passes`, and generic graph machinery under `FrameGraph`. The old `SceneData`, `Camera`, `FramePipeline`, and touched generic `Frame/Core` roots are empty and removed. | `rg --files`, include/CMake inspection, and owner-to-path review show one canonical location per concept and no old/new directory split. An Unreal anchor in the translation table reaches the target owner without a forwarding layer. |
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
| Unreal-familiar navigation is truthful | concept-translation review; owner-to-directory audit; zero touched old-root files/CMake entries; no forwarding layer created only to preserve an Unreal name |
| Frame-slot lifetime is safe | maximum-frames-in-flight stress; resize/provider/scene reset; no borrowed epoch beyond retirement |
| Temporal state follows the correct view | viewport identity switch, scene generation change, camera cut/teleport, projection change, provider/shader generation change, and stable-view continuation tests |
| Shader ABI remains paired | C++ layout assertions, shader cook/validation, both backend builds |
| GPU Scene retains behavior | create/update/destroy, stable slot, deformation previous/current, material revision, dirty/no-change upload evidence |
| Ray tracing retains behavior | BLAS/TLAS plan/build tests, descriptor/device-address capability paths where currently supported, fixed-camera output comparison |
| Pass ownership is narrower | zero old frame/pass/provider/ray context references; declared graph resources match execute usage; `PassCommandContext` contains infrastructure only; parallel recording checks |
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

- recognizable `Scene`, `Primitive`, `View`, `ViewState`, `GPU Scene`, and render-graph vocabulary where the lifetime and ownership match;
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
- a familiar Unreal class name when Sparkle's corresponding owner has a different lifetime or responsibility;
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
- the Unreal-to-Sparkle translation table and directory navigation rule match the executable owner graph, with every touched concept in one canonical target path and all emptied old roots removed;
- `FrameUniformData`, `ViewUniformData`, `ViewCameraUniformData`, `ViewTemporalUniformData`, and focused scene bindings are the only frame/view/scene ABI;
- graph topology/resources remain owned by the frame graph;
- `BuildDeferredFrameGraph` is the only deferred graph-construction entry point;
- passes use explicit semantic parameters plus narrow `PassCommandContext` infrastructure and cannot reach a broad context or service locator;
- `FramePipeline` sequences owned stages without owning scene/view capabilities;
- every target value has one producer, intentional consumers, one lifetime, and a tested invalidation/retirement rule; unsupported request fields are deleted rather than left inert;
- D3D12 and Vulkan builds, focused tests, runtime smokes, captures, and performance comparisons support the claims;
- the implemented architecture map and document status are reconciled with the proven code.

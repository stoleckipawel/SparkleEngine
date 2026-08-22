# Renderer Scene, View, And Frame Architecture

Status: target architecture, Unreal Engine reference analysis, and atomic implementation cutover plan; not implemented behavior
Date: 2026-08-18
Scope: GameFramework-to-Renderer publication, persistent render-scene ownership, GPU-scene ownership, one-frame scene and view products, temporal view state, render-frame orchestration, frame-graph pass inputs, Unreal-familiar concept translation, coherent cross-module naming and directory navigation, complete legacy-path removal, atomic landing, D3D12/Vulkan validation, and cleanup of the current frame path

Implementation checkpoint: Phases 1 through 5 established the target publication boundary, persistent scene authority, prepared scene/current view split, focused frame/view ABI, scene-owned GPU/ray-tracing capabilities, and explicit pass inputs on `master`. The render-frame graph and physical-layout phase remains target architecture, and no executable validation or release-readiness claim is made before Phase 7.

## Decision

Sparkle should adopt the lifetime and responsibility split used by Unreal's deferred renderer without copying Unreal's class size, inheritance, naming prefixes, or feature breadth:

1. `RenderScene` is the persistent render-coordinator-owned mirror of gameplay scene state.
2. `RenderGpuScene` and the ray-tracing scene are scene capabilities owned beneath `RenderScene`, not frame-pipeline state.
3. `PreparedRenderScene` is an immutable, frame-slot-owned projection of the scene used by one submitted frame.
4. `RenderView` is an immutable one-frame view: camera, matrices, frustum, rectangles, resolved view policy, temporal shader values, and view-derived visibility/draw products.
5. `RenderViewState` is the persistent continuity for one stable viewport/view identity: previous camera state, jitter sequence, semantic history validity, and view-history invalidation state. Exposure remains resolved viewport display policy, and graph/provider history remains with its existing owner.
6. `RenderFrame` owns only one frame's identity, time, frame-in-flight slot, `PreparedRenderScene`, and current `RenderView`. It is a lifetime boundary, not a service locator.
7. `FramePipeline` sequences the render-thread frame and owns the cached frame-graph execution. It delegates scene preparation, view preparation, history, providers, presentation, and pass-specific work to their existing or newly clarified owners.
8. Frame-graph passes declare narrow pass parameters. `FrameContext`, `FrameContextBuilder`, `PassExecutionContext`, `PassRuntimeContext`, `RayTracingPassContext`, and `ImageProviderPassContext` are removed rather than renamed into new catch-all bags; `PassCommandContext` retains only recording infrastructure.
9. The migration completes as one architectural cutover series on `master`. Each phase must clean-break its owned scope, and no intermediate mixture of current and target ownership, naming, packet, shader ABI, pass context, or scene/view paths may ship.

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

This is a target design. Current code and executable build configuration remain the authority for implemented behavior. `master` may contain the user-reviewed phase checkpoints, but those checkpoints are private migration states: they are not releasable, downstream-integration-ready, or evidence that the complete architecture works. After Phase 7, `master` must contain only the target architecture.

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

## Pre-migration Sparkle Mapping

The Phase 0 baseline already had most of the necessary concepts. The problem was classification and ownership, not absence.

| Phase 0 Sparkle concept | Closest Unreal role | Assessment |
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
| `BuildFrame` | Render-frame graph topology construction | It builds renderer graph topology; it does not build a complete frame value. |
| `FramePipeline` | Persistent frame lifecycle plus parts of scene renderer, view family, presentation, and provider orchestration | Too many responsibilities currently converge here. |
| `RendererHost` | Composition root | Correct long-lived role, but its many getters make it easy for downstream code to treat it as a service locator. |

### Phase 0 evidence of mixed responsibilities

As inspected on 2026-08-18:

- [`FramePipeline.cpp`](../../Engine/Renderer/Private/FramePipeline/FramePipeline.cpp) is 698 lines and coordinates input consumption, resize, graph rebuilds, uploads, camera mutation, per-frame constants, scene and view preparation, history invalidation, providers, ray tracing, scene resource binding, graph execution, submission, capture, UI, and presentation.
- Historical `Renderer/Private/SceneData/RenderSceneData.h` contained scene revisions, lights, sky, all mesh/deformation records, materials, view-frustum-selected raster indices, camera-distance-sorted batches, workload values, and ray-tracing work.
- Historical `Renderer/Private/SceneData/Preparation/RenderPreparationInputResolver.cpp` received both `RenderWorld` and a view frustum/camera position. Its result could not be accurately described as scene-only.
- At the Phase 0 baseline, `FrameContextBuilder.cpp` prepared scene data, planned ray tracing from camera position, updated GPU Scene, constructed the view, and advanced temporal history in one function.
- At the Phase 0 baseline, `PassRuntimeContext.h` was referenced by 45 Renderer private files and `mainView` by 24 files. This was coupling evidence, not a reason to create a larger replacement context.
- Historical `Renderer/Private/ShaderData/PerFrameConstantBufferData.h` mixed true frame time/index values with view mode and viewport size.
- [`FramePipeline::FinalizeRenderInputMetadata`](../../Engine/Renderer/Private/FramePipeline/FramePipeline.cpp) writes render/output dimensions into a GameFramework-owned input packet after publication. Those dimensions are renderer/view configuration, not GameFramework scene metadata.
- At the Phase 0 baseline, `RenderFrameMetadata::Exposure` had no consumer. Motion-vector and depth conventions were stable renderer/shader contracts rather than per-frame input, while `ProviderGeneration` was populated from shader-package generation and participated in input/history/capture behavior under a misleading name.
- At the Phase 0 baseline, `ViewportRenderRequest::ViewKind`, `ViewSelection`, and `FeatureFlags` had no `FramePipeline` consumer. Phase 3 routes stable view kind and selection into view identity and deletes the unsupported generic feature-flag promise instead of carrying it inertly.

These are dated observations. Re-run the searches at the start of implementation because the frame path is actively changing.

## Atomic Migration Contract

This refactor has one supported release state: the complete target architecture in this document. The phase breakdown is an execution and review aid on `master`, not a sequence of supported engine architectures.

```text
master before migration                 master phase series                   validated master

current architecture          ->        clean-break phase checkpoints   ->     target architecture only
no target aliases                       never release a checkpoint              no current paths or names
```

The following rules are binding:

- All phase work occurs directly in the checked-out `master` worktree. Do not create, switch, merge, rebase, or otherwise use a migration branch for any phase.
- The implementation agent leaves each phase unstaged and uncommitted for user review. Only the user may stage, commit, push, or submit a phase.
- A user-created phase commit on `master` is a private checkpoint only. It is not independently releasable, downstream-integratable, cherry-pickable as a supported feature, or described as the complete implemented architecture.
- The final integration unit updates GameFramework, Application, Editor, Renderer, shaders, CMake membership, tests, generated/cooked artifacts, and documentation together.
- No feature flag, build option, CVar, runtime branch, typedef, adapter, overload, reader, writer, or fallback selects between current and target architectures.
- No target owner reads a current representation, and no current owner reads a target representation. There is one publication packet, one scene owner, one view path, one shader ABI, and one pass-input model at final head.
- Temporary migration scripts may mechanically rewrite source or regenerate artifacts, but they are not runtime code and are removed before landing unless they remain the canonical generator.
- Failed final validation is fixed at the real owner directly on `master`. It is not bypassed by restoring an old path alongside the new one.
- Rollback means reverting the complete migration. There is no runtime compatibility fallback and no partial rollback that resurrects selected current owners.

At no point may documentation mark an individual phase as the complete implemented architecture. Only the validated Phase 7 state changes this document and the implemented repository map from target to implemented status.

### No-halfway review rule

A reviewer must reject the migration if any of these conditions exists at final head:

- both `RenderWorld` and `RenderScene` participate in runtime code;
- both `RenderInputFrame`/`RenderFrameMetadata` and `RenderFrameSubmission` are produced or consumed;
- both `RenderFramePacket` and `RenderExecutionRequest` can be queued or executed;
- both `RenderProxy` and `RenderPrimitive` represent scene primitives;
- both `RenderSceneData` and `PreparedRenderScene` carry current frame scene data;
- both `RenderViewData`/`RenderCamera`/`TemporalDataBuilder` and `RenderView`/`RenderViewState` are active;
- any of `FrameContext`/`PassExecutionContext`/`PassRuntimeContext`/`RayTracingPassContext`/`ImageProviderPassContext` coexist with the target explicit pass parameters and `PassCommandContext` execution surface;
- both old and new constant-buffer layouts are cooked or accepted;
- both `ViewLighting`/light `*ConstantBufferData` records and `SceneLighting`/light `*GpuData` records are registered or bound;
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
| Submitted render work | `RenderFrameSubmission` | accepted through `RenderExecutionRequest` into `RenderFrame` | `FrameUniformData` for true frame values | `Submission` is cross-module ownership, `ExecutionRequest` is the private queue envelope, and `Frame` is renderer frame-slot lifetime. |
| Persistent renderer scene | none | `RenderScene` | `RenderGpuScene` | `Gpu` uses repository casing; persistence is expressed by ownership, not a prefix. |
| One-frame scene projection | none | `PreparedRenderScene` | `RenderSceneGpuBindings` | `Prepared` distinguishes the immutable derived product from the mutable scene authority. |
| Scene lighting shader data | scene lights in `RenderSceneDynamicData` | prepared lights plus scene GPU capability | `SceneLightingUniformData` and `DirectionalLightGpuData`/`PointLightGpuData`/`SpotLightGpuData`/`RectLightGpuData` | Counts and structured-buffer records are scene GPU data, never a `ViewLighting*` value or `*ConstantBufferData` record. |
| Current view | `RenderViewInput` / `ViewportRenderRequest` | `RenderView` | `ViewUniformData`, `ViewTemporalUniformData` | `PerView*` and `PerTemporal*` names are removed in the clean break. |
| Persistent view continuity | none | `RenderViewState` | graph/provider histories remain with their resource owners | State owns semantic continuity, not history resources. |
| Render-frame graph construction | none | `BuildRenderFrameGraph` | frame-graph resource handles | Delete generic `BuildFrame`; the function builds renderer graph topology without naming a shading technique. |
| Render-frame graph resource namespace | none | `RenderFrameGraphResources` | transient, imported-scene, history, and viewport-product handles | Delete the ambiguous `FrameAssembly*` vocabulary; these are graph handles, not a frame value or scene owner. |
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
| `FSceneViewFamily` | no target type yet | `RenderFrame` for frame-slot lifetime; render-frame graph settings for shared topology policy | `RenderFrame` is not renamed to view family. Add `RenderViewFamily` only when one invocation truly owns multiple simultaneous views. |
| `FSceneRenderer` / `FDeferredShadingSceneRenderer` | `FramePipeline` plus `BuildRenderFrameGraph` | `Engine/Renderer/Private/Frame/FramePipeline.*`, then `Frame/Graph` | Sparkle separates the persistent lifecycle sequencer from technique-neutral graph topology instead of creating one large renderer-invocation class. |
| scene textures and RDG resource collections | `RenderFrameGraphResources` | `Engine/Renderer/Private/Frame/Graph` | These are graph handles grouped by graph role, never persistent `RenderScene` or `RenderView` fields. |
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
| `Frame/` | `FramePipeline`, `RenderFrame`, frame identity/retirement, and render-frame graph assembly | a broad bag of scene, view, pass, and service implementations |
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

The Renderer-private queue combines that submission with renderer execution inputs as one `RenderExecutionRequest { Submission, Time, Ui }`. `Time` is a `RenderFrameTime` value containing scaled/unscaled total and delta time but no frame id. `RenderFrameSubmission::FrameId` is the single frame identity; when adapting current `TimeInfo`, its `frameIndex` must agree or be discarded rather than retained as a second authority. This clean-break replaces the ambiguous current `RenderFramePacket`: `RenderFrameSubmission` is the cross-module scene/view publication, `RenderExecutionRequest` is the bounded RenderCoordinator queue payload, and `RenderFrame` is the renderer frame-slot lifetime owner. None is an alias for another.

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

`RenderSceneGpuData` is clean-break replaced by the narrow non-owning `RenderSceneGpuBindings` view. `RayTracingSceneFrameData` becomes the focused `RenderRayTracingFrameBindings` setup value rather than a field on a broad frame context. Frame-graph import handles remain in graph resource layout types and are bound from those scene-owned resources at frame setup.

Move logical ownership of `RenderRayTracingScene` beneath `RenderScene`, while keeping BLAS cache, TLAS strategy, and capability logic as focused subordinate types. Separate:

- view-independent traceable-instance and BLAS source preparation, which belongs to `PreparedRenderScene` / scene capability;
- camera- or view-policy-dependent TLAS planning, which belongs to the current render invocation;
- persistent acceleration resources and cache invalidation, which belong to `RenderRayTracingScene`.

Do not merge GPU Scene and ray tracing into one class. They share a scene owner but have different resource and update contracts.

### Frame, view, and scene-lighting shader data

Split the shader ABI in one paired C++/HLSL clean break:

- `FrameUniformData`: frame index/id and scaled/unscaled time values;
- `ViewUniformData`: viewport size/inverse size, view mode, view rect, and resolved view feature values;
- `ViewCameraUniformData`: camera transforms, camera position/direction, and projection values;
- `ViewTemporalUniformData`: current/previous jitter, previous matrices, and history validity;
- `SceneLightingUniformData`: scene-wide light counts;
- `DirectionalLightGpuData`, `PointLightGpuData`, `SpotLightGpuData`, and `RectLightGpuData`: structured-buffer light records;
- `RenderSceneGpuBindings` and other focused scene values: material/GPU-scene bindings.

`PerFrameConstantBufferData`, `PerViewConstantBufferData`, `PerViewCameraConstantBufferData`, `PerTemporalConstantBufferData`, `ViewLightingData`, and the four light `*ConstantBufferData` records are deleted. The broad `RenderConstantBufferData.h` include is deleted rather than renamed. C++ registrations and HLSL use focused frame/view uniform headers and scene-lighting shader-data headers directly. Viewport size and view mode move out of the frame ABI; light counts stop pretending to be view state. No compatibility constant buffer, duplicate field, shader macro alias, duplicate registration name, or old cooked layout remains after the paired shader migrations.

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

`FrameAssemblyResourceLayout` and its `FrameAssembly*` nested names are clean-break replaced by technique-neutral graph vocabulary:

- `RenderFrameGraphResources`: the top-level handle namespace passed while constructing renderer frame topology;
- `RenderFrameGraphTransientResources`: transient scene, GBuffer, lighting, exposure, and reconstruction handles;
- `RenderFrameGraphImportedSceneResources`: graph imports for GPU Scene, sky, and TLAS;
- `FrameHistoryResourceLayout`: graph-owned persistent history handles;
- `ViewportFrameProducts`: requested exported viewport-product handles.

`FrameBuildSettings` is replaced by `RenderFrameGraphSettings`. `BuildRenderFrameGraph` returns `RenderFrameGraphResources` directly, so the one-field `FrameBuildResult` wrapper is deleted. `FinalSceneColorProduced` is replaced by one initially invalid `ResolvedSceneColor` graph handle: reconstruction or upscaling publishes the handle it produced, and presentation consumes that handle. `ViewportFrameProducts::FinalSceneColor` remains the distinct encoded viewport product. `RenderFrameGraphBuildResult` remains the distinct factory result that owns the built graph plus its exported resource handles.

`BuildFrame` becomes `BuildRenderFrameGraph` because it creates renderer frame-graph topology. Graph handles never move into `RenderScene`, `PreparedRenderScene`, or `RenderView`.

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

Keep the name `FramePipeline` for the thin lifecycle sequencer. Do not add a parallel `DeferredRenderer` wrapper or retain a forwarding facade merely to copy Unreal's name; technique selection remains explicit pass/topology policy beneath the neutral `BuildRenderFrameGraph` entry point.

## Target Code Shape

These names are the committed target vocabulary. A rename discovered to be necessary during implementation requires updating this contract first and then applying the replacement everywhere in the same master-worktree CL; it does not permit local synonyms.

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
  Concurrency/FrameQueue/RenderExecutionRequest.h renderer-private queue payload
  Scene/
    RenderScene.*                              persistent CPU scene owner
    RenderPrimitive.*                          one persistent primitive record
    Materials/...                              scene-owned material tables/caches
    GpuScene/RenderGpuScene.*                  persistent GPU scene capability
    RayTracing/RenderRayTracingScene.*         persistent RT scene capability
    RayTracing/RenderRayTracingFrameBindings.h focused per-frame RT bindings
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
    RenderFrameTime.h
    Retirement/FrameExecutionRetirementQueue.*
    Graph/BuildRenderFrameGraph.*
    Graph/RenderFrameGraphResources.h
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
    SceneLightingUniformData.h
    LightGpuData.h
  Providers/ImageProviderFrameInput.h         focused provider setup value

Engine/Assets/Shaders/Resources
  FrameUniformData.hlsli
  ViewUniformData.hlsli
  ViewCameraUniformData.hlsli
  ViewTemporalUniformData.hlsli
  SceneLightingUniformData.hlsli
  LightGpuData.hlsli
```

Do not create both old and new directory trees. Move files and update CMake membership in the same migration workstream that changes ownership; delete emptied builders, adapters, aliases, and replaced names before the final cutover.

## Complexity Budget And Explicit Non-Goals

The refactor is accepted only if it removes more ambiguity than structure it adds.

- Add `PreparedRenderScene`, `RenderViewState`, and the separated scene/view input values because they express real lifetimes.
- Delete `RenderInputFrame`, `RenderFrameMetadata`, `RenderWorldDelta`, `RenderFrameDynamicData`, `RenderCameraData`, `RenderFramePacket`, `RenderInputConsumer`, `RenderInputConsumeResult`, `RenderWorld`, `RenderProxy`, `RenderSceneData`, `RenderViewData`, `RenderCamera`, `PerFrameDataBuilder`, `PerViewDataBuilder`, `TemporalDataBuilder`, `RenderTemporalFrameState`, `ImageProviderFrameContext`, `FrameContextBuilder`, `FrameContext`, `PassExecutionContext`, `PassRuntimeContext`, `RayTracingPassContext`, `ImageProviderPassContext`, `PersistentRenderGpuScene`, `RenderSceneGpuData` and its nested `*Data` binding groups, `RayTracingSceneFrameData`, `RenderViewLightingData`/`ViewLightingData`/`ViewLighting`, the four light `*ConstantBufferData` records, the `FrameAssembly*` types, `FrameBuildSettings`, `FrameBuildResult`, `FrameResolutionExtents`, `RenderConstantBufferData`, and the four old frame/view constant-buffer data types as their responsibilities move.
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

- Each phase is one logical, reviewable CL directly in the unstaged `master` worktree. No phase creates, switches, merges, rebases, or depends on another branch.
- The implementation agent never stages, commits, pushes, submits, or rewrites user commits. It leaves the complete phase diff on `master`; the user owns review and every source-control action.
- A user-created phase commit is a private checkpoint and not a supported architecture, release boundary, downstream integration unit, or proof that the engine builds or runs. Continue forward on `master` until Phase 7 validates the complete series.
- Phases 0 through 6 do not configure, compile, link, compile shaders, cook, launch, capture, or run performance workloads. Use exact searches, scoped diff inspection, CMake/include audits, documentation checks, `git diff --check`, and no-write formatting only. Executable acceptance occurs once in Phase 7 against the complete candidate.
- If a valid before-change executable/performance baseline does not already exist, acquire it before Phase 0 edits begin. Record its source revision and environment. Do not build a new baseline between migration phases.
- No-build phase acceptance makes no compilation or runtime claim. It proves source-level ownership closure, cleanup, and reviewability only.
- Update existing durable tests when they are consumers of a changed contract. Do not add submitted test fixtures, executables, probes, or CTest registrations without explicit user authorization; temporary local validation code is removed before phase handoff.
- If a phase cannot delete its assigned legacy concept because an unplanned consumer remains, move that consumer into the same phase or revise the phase boundaries before handoff. Do not bridge the gap with an alias, adapter, overload, feature flag, fallback, or duplicate directory.
- Inspect and preserve unrelated dirty work before every phase. A phase CL contains only its owned migration scope and the directly required standards/documentation reconciliation.
- Phase 7 failures are fixed at the owning responsibility and kept attributable in the final review. Do not accumulate a miscellaneous final "make it build" change that obscures ownership.

## Migration Phase Implementation Prompts

### Phase 0 - Freeze authority, inventory, invariants, and baseline

#### Implementation prompt

> Implement Phase 0 of the Renderer Scene/View/Frame migration as one documentation and inventory CL directly in the unstaged master worktree. Do not create or switch branches, and do not stage, commit, push, or submit; the user owns manual review and any source-control action. Apply every required standard above. Reconcile current code and executable policy, freeze the exact target vocabulary and owner map, assign every legacy definition and consumer to one later phase, and remove stale/conflicting documentation in scope. Do not change runtime source and do not configure, build, compile shaders, cook, launch, or capture. This CL is not independently landable.

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

The handoff contains only the reconciled plan, standards/routes, and inventory evidence. This phase cleans obsolete documentation and vocabulary; it does not preserve conflicting old rules for a later documentation-only cleanup.

#### Phase 0 completion record

This inventory was frozen before runtime edits on 2026-08-18 from `7a0833d5762f01b96e927e7a355d7b8e18a583f8` (`rendere refactor plan`, committed `2026-08-18T22:37:18+02:00`). The original Phase 0 provenance recorded a temporary branch and a clean worktree; that is historical evidence only. The migration is now recovered on `master`, and the binding master-only rule above applies to every current and future phase. The path-level exclusion list for unrelated tracked or untracked work remains empty.

The following ignored local products are not source, are not part of any Phase 0 claim, and must not enter a Phase 1-6 CL: `artifacts/`, `build/`, `logs/`, `Saved/`, ignored source-content drops under `Projects/Showcase/Assets/Meshes/`, `Projects/Showcase/Cooked/`, and `Projects/Showcase/imgui.ini`. Phase 3 may remove stale shader ABI products from disposable cache/cooked paths, and Phase 7 may regenerate canonical outputs, but neither exception authorizes committing or deleting unrelated user content. Re-inspect `git status --short --untracked-files=all` before every phase; any later dirty path is added to that phase's explicit exclusion list before editing.

##### Reproducible search protocol

The inventory used word-bounded exact names over runtime source, project consumers, executable CMake policy, shader source/registrations, documentation, and ignored local products. These commands are the minimum search floor; later phases add discovered names rather than narrowing it.

```powershell
$legacyNames = 'RenderInputFrame|RenderFrameMetadata|RenderWorldDelta|RenderFrameDynamicData|RenderCameraData|RenderFramePacket|RenderWorld|RenderProxy|RenderInputConsumer|RenderInputConsumeResult|RenderSceneData|RenderViewData|RenderCamera|PerFrameDataBuilder|PerViewDataBuilder|TemporalDataBuilder|FrameContextBuilder|FrameContext|PassExecutionContext|PassRuntimeContext|RayTracingPassContext|ImageProviderPassContext|PersistentRenderGpuScene|RenderSceneGpuData|RenderSceneGpuBuffer|RenderSceneGpuLightingData|RenderSceneGpuGeometryData|RenderSceneGpuRayTracingData|RayTracingSceneFrameData|FrameAssemblyResourceLayout|FrameAssemblyTransientResources|FrameAssemblyExternalResources|FrameAssemblyViewportProducts|FrameBuildSettings|FrameBuildResult|FrameResolutionExtents|FinalSceneColorProduced|PerFrameConstantBufferData|PerViewConstantBufferData|PerViewCameraConstantBufferData|PerTemporalConstantBufferData|BuildFrame|RenderTemporalFrameState|ImageProviderFrameContext|RenderConstantBufferData|mainView|RenderViewLightingData|ViewLightingData|ViewLighting|DirectionalLightConstantBufferData|PointLightConstantBufferData|SpotLightConstantBufferData|RectLightConstantBufferData'
$legacyPathNames = $legacyNames + '|ConstantBuffers|CameraConstantBufferData|TemporalConstantBuffer|LightConstantBufferData'
$targetNames = 'RenderFrameSubmission|RenderSceneUpdate|RenderSceneDelta|RenderSceneDynamicData|RenderViewInput|RenderViewCameraData|RenderExecutionRequest|RenderFrameIdentity|RenderFrameTime|RenderScene|RenderPrimitive|PreparedRenderScene|RenderScenePreparation|RenderView|RenderViewBuilder|RenderViewState|RenderViewPreparation|RenderGpuScene|RenderSceneGpuBufferBinding|RenderSceneGpuLightingBindings|RenderSceneGpuGeometryBindings|RenderSceneGpuRayTracingBindings|RenderSceneGpuBindings|RenderRayTracingFrameBindings|FrameUniformData|ViewUniformData|ViewCameraUniformData|ViewTemporalUniformData|SceneLightingUniformData|SceneLighting|DirectionalLightGpuData|PointLightGpuData|SpotLightGpuData|RectLightGpuData|PassCommandContext|ImageProviderFrameInput|BuildRenderFrameGraph|RenderFrameGraphSettings|RenderFrameGraphResources|RenderFrameGraphTransientResources|RenderFrameGraphImportedSceneResources|ViewportFrameProducts|ResolvedSceneColor'
$targetPathNames = $targetNames + '|LightGpuData'
$legacy = '\b(' + $legacyNames + ')\b'
$legacyPath = '\b(' + $legacyPathNames + ')\b'
$target = '\b(' + $targetNames + ')\b'
$diagnostic = '"[^"\r\n]*(' + $legacyNames + ')[^"\r\n]*"'
$targetDiagnostic = '"[^"\r\n]*(' + $targetNames + ')[^"\r\n]*"'

rg -n --glob '!Docs/**' $legacy Engine Projects CMake CMakeLists.txt
rg -n --glob '!Docs/**' $target Engine Projects CMake CMakeLists.txt
rg --files Engine Projects CMake | rg $legacyPathNames
rg --files Engine Projects CMake | rg $targetPathNames
rg -n --glob 'CMakeLists.txt' --glob '*.cmake' $legacy .
rg -n --glob '*.{h,cpp,hlsl,hlsli}' 'PerFrameConstantBufferData|PerViewConstantBufferData|PerViewCameraConstantBufferData|PerTemporalConstantBufferData' Engine/Assets/Shaders Engine/Renderer/ShaderRegistrations Engine/Renderer/Private CMake
rg -n --glob '*.{h,cpp,hlsl,hlsli,cmake,json}' $diagnostic Engine Projects CMake CMakeLists.txt
rg -n --glob '*.{h,cpp,hlsl,hlsli,cmake,json}' $targetDiagnostic Engine Projects CMake CMakeLists.txt
rg -n --glob '*.md' $legacy Docs
rg -n --glob '*.md' $target Docs
rg -l -uuu $legacyPath Saved artifacts build logs Projects/Showcase/Cooked
rg -l -uuu $target Saved artifacts build logs Projects/Showcase/Cooked
```

The last ignored-product legacy search found zero matching files under `Saved/` and `logs/`, 64 under `artifacts/`, 868 under `build/`, and 17 under `Projects/Showcase/Cooked/`. Those matches are stale binaries, symbols, CMake replies, shader-cache entries, or cooked shader packages; they are not baseline evidence. They remain an explicit Phase 3/4 deletion and Phase 7 regeneration search floor. The mirrored target search found zero under `Saved/`, `logs/`, and cooked output, 19 under `artifacts/`, and 347 under `build/`: 19 artifact and 345 build files carry the colliding current `RenderViewCameraData.h` path, while two Assimp dependency source copies independently use `RenderScene`. Neither is a Sparkle target implementation or baseline. Final source gates remain owner/path scoped and do not rename third-party vocabulary.

##### Exact current-symbol and deletion ledger

Counts are `exact matches / files` outside `Docs`. Every definition, declaration, include, forward declaration, construction, member, call site, test-as-consumer, and executable-policy match for a row belongs to the single phase shown. A phase cannot exempt a hard consumer by calling it unrelated cleanup.

| Deletion phase | Rejected current name | Matches / files | Defining or special surface |
| --- | --- | ---: | --- |
| 1 | `RenderInputFrame` | 35 / 21 | `GameFramework/Public/Rendering/RenderInputFrame.h`; queue, Application, Editor, Renderer API/input consumers |
| 1 | `RenderFrameMetadata` | 15 / 10 | `GameFramework/Public/Rendering/RenderFrameMetadata.h`; capture and pipeline consumers |
| 1 | `RenderWorldDelta` | 48 / 8 | `GameFramework/Public/Rendering/RenderWorldDelta.h`; structural/resource extractors |
| 1 | `RenderFrameDynamicData` | 51 / 21 | `GameFramework/Public/Rendering/RenderFrameDynamicData.h`; dynamic extractor and renderer consumers |
| 1 | `RenderCameraData` | 24 / 12 | Nested in `RenderFrameDynamicData.h`; Application/Editor camera publication |
| 1 | `RenderFramePacket` | 14 / 7 | `Renderer/Private/Concurrency/FrameQueue/RenderFramePacket.h`; queue/coordinator/execution context |
| 2 | `RenderWorld` | 74 / 16 | `Renderer/Private/SceneData/RenderWorld.*`; Host, input, preparation, texture upload, history |
| 2 | `RenderProxy` | 24 / 5 | Defined in `RenderWorld.h`; scene mutation and preparation |
| 3 | `RenderInputConsumer` | 12 / 5 | `SceneData/Input/RenderInputConsumer.*`; pipeline and viewport capture |
| 3 | `RenderInputConsumeResult` | 5 / 3 | Same definition; pipeline acceptance result |
| 3 | `RenderSceneData` | 160 / 44 | `Renderer/Private/SceneData/RenderSceneData.*`; preparation, GPU scene, passes, ray tracing, invalidation |
| 3 | `RenderViewData` | 54 / 31 | `Renderer/Private/Frame/Core/RenderViewData.h`; builders and authored passes |
| 3 | `RenderCamera` | 38 / 9 | `Renderer/Private/Camera/RenderCamera.*`; Host, frame setup, temporal builder |
| 3 | `PerFrameDataBuilder` | 19 / 3 | `Renderer/Private/Frame/Builders/PerFrameDataBuilder.*`; pipeline setup |
| 3 | `PerViewDataBuilder` | 25 / 7 | `Renderer/Private/Frame/Builders/PerViewDataBuilder.*`; frame builder/Host |
| 3 | `TemporalDataBuilder` | 20 / 7 | `Renderer/Private/Frame/Builders/TemporalDataBuilder.*`; Host, pipeline, frame builder |
| 3 | `PerFrameConstantBufferData` | 63 / 37 | C++ ABI, passes, HLSL and registrations |
| 3 | `PerViewConstantBufferData` | 63 / 35 | C++ ABI, passes, HLSL and registrations |
| 3 | `PerViewCameraConstantBufferData` | 21 / 15 | C++ ABI, camera/provider/Streamline, HLSL |
| 3 | `PerTemporalConstantBufferData` | 67 / 41 | C++ ABI, passes/provider/Streamline, HLSL and registrations |
| 3 | `RenderTemporalFrameState` | 7 / 5 | Derived duplicate in `Frame/Temporal/TemporalFrameState.*` and provider setup |
| 3 | `ImageProviderFrameContext` | 31 / 18 | Semantic provider input in `Providers/ImageProviderFrameContext.h`; providers/Streamline |
| 3 | `RenderConstantBufferData` | 37 / 37 | Broad include `ShaderData/RenderConstantBufferData.h` and its include consumers |
| 3 | `mainView` | 76 / 24 | Single-view member/access spelling across pipeline and passes |
| 4 | `PersistentRenderGpuScene` | 22 / 6 | `SceneData/GpuScene/PersistentRenderGpuScene.*`; Host/pipeline/frame builder |
| 4 | `RenderSceneGpuData` | 17 / 10 | `SceneData/RenderSceneGpuData.*`; frame context, bindings, GPU Scene |
| 4 | `RenderSceneGpuBuffer` | 23 / 4 | Old GPU binding leaf in `RenderSceneGpuData.h` and GPU Scene storage |
| 4 | `RenderSceneGpuLightingData` | 3 / 2 | Old lighting binding group |
| 4 | `RenderSceneGpuGeometryData` | 6 / 3 | Old geometry binding group |
| 4 | `RenderSceneGpuRayTracingData` | 4 / 3 | Old ray-tracing binding group |
| 4 | `RayTracingSceneFrameData` | 16 / 9 | `RayTracing/Scene/RayTracingSceneFrameData.h`; frame/passes/RT strategies |
| 4 | `RenderViewLightingData` | 21 / 21 | Misnamed C++ shader-data include path; scene GPU payload/registrations/passes |
| 4 | `ViewLightingData` | 35 / 25 | Scene-wide light-count uniform in C++, HLSL, registrations, bindings, and passes |
| 4 | `ViewLighting` | 79 / 32 | Scene-wide C++/HLSL uniform member, registration, and binding label |
| 4 | `DirectionalLightConstantBufferData` | 29 / 18 | Structured-buffer record misnamed as constant-buffer data |
| 4 | `PointLightConstantBufferData` | 34 / 19 | Structured-buffer record misnamed as constant-buffer data |
| 4 | `SpotLightConstantBufferData` | 40 / 19 | Structured-buffer record misnamed as constant-buffer data |
| 4 | `RectLightConstantBufferData` | 37 / 19 | Structured-buffer record misnamed as constant-buffer data |
| 5 | `FrameContextBuilder` | 11 / 4 | `Frame/Builders/FrameContextBuilder.*`; pipeline/Host |
| 5 | `FrameContext` | 124 / 54 | `Frame/Core/FrameContext.h`; graph setup/execution, retirement, passes |
| 5 | `PassExecutionContext` | 153 / 70 | `FrameGraph/Execution/PassExecutionContext.h`; all authored pass recording paths |
| 5 | `PassRuntimeContext` | 79 / 45 | `FrameGraph/PassRuntimeContext.h`; graph builder and passes |
| 5 | `RayTracingPassContext` | 9 / 5 | `RayTracing/Scene/RayTracingPassContext.h`; shadow/ray pass setup |
| 5 | `ImageProviderPassContext` | 6 / 4 | Defined in `Providers/RendererImageProviderStack.h`; provider passes/pipeline |
| 6 | `FrameAssemblyResourceLayout` | 59 / 48 | `Frame/Core/FrameAssembly.h`; deferred topology/factory/pipeline |
| 6 | `FrameAssemblyTransientResources` | 2 / 1 | `Frame/Core/FrameAssembly.h` |
| 6 | `FrameAssemblyExternalResources` | 17 / 11 | `Frame/Core/FrameAssembly.h`; scene/lighting/GBuffer setup |
| 6 | `FrameAssemblyViewportProducts` | 2 / 1 | `Frame/Core/FrameAssembly.h` |
| 6 | `FrameBuildSettings` | 14 / 11 | `Frame/Core/Frame.h`; graph factory and topology builders |
| 6 | `FrameBuildResult` | 5 / 3 | `Frame/Core/Frame.h`; `Frame.cpp` and graph factory |
| 6 | `FrameResolutionExtents` | 14 / 2 | `FramePipeline.h/.cpp`; render/output topology pair |
| 6 | `FinalSceneColorProduced` | 4 / 4 | `FrameAssembly.h`; post-processing/reconstruction/upscaling |
| 6 | `BuildFrame` | 3 / 3 | `Frame/Core/Frame.*`; `FrameGraphFactory.cpp` |

The target search returned zero runtime/build matches for every frozen target name except `RenderViewCameraData`, which had five include-path matches in five Renderer files. Those five refer only to the current Renderer-private shader header `Engine/Renderer/Private/ShaderData/RenderViewCameraData.h`, whose defined type is `PerViewCameraConstantBufferData`; they are not the target GameFramework publication value. Phase 1 introduces `Engine/GameFramework/Public/Rendering/RenderViewCameraData.h`. Phase 3 deletes the colliding Renderer-private header and replaces its ABI role with `Engine/Renderer/Private/ShaderData/ViewCameraUniformData.h`. The final path-scoped gate permits only the GameFramework type.

The filename search for target roots returned 12 current paths. Nine are rejected paths already owned by Phases 3/4: `RenderSceneData.*`, `RenderSceneGpuData.*`, `PersistentRenderGpuScene.*`, `RenderViewData.h`, Renderer-private `RenderViewCameraData.h`, and `RenderViewLightingData.h`. `SceneData/GpuScene/RenderGpuScenePayloads.h` keeps its target-congruent filename but moves beneath `Scene/GpuScene` in Phase 4. Public `RenderViewMode.h` and shader `RenderViewModeConstants.hlsli` are retained: they name an actual current-view display mode and do not compete with `RenderView` ownership. A target-root filename match is therefore never accepted as proof that a target type exists; its definition and owner must match the frozen ledger.

##### File, directory, build, shader, and artifact ownership

The Renderer and GameFramework CMake targets use recursive `CONFIGURE_DEPENDS` globs. There are no explicit per-file source entries to preserve, but every move/deletion must still be reflected in glob discovery, source groups, installed/public headers, and includes. `CMake/ArchitectureBoundaryCheck.cmake` line 12 contains mixed old shader-data alternatives: its frame/view constant-buffer alternatives belong to Phase 3 and its `RenderViewLightingData`/`ViewLighting` alternatives belong to Phase 4; line 128's `RenderWorld` token belongs to Phase 2. The frame/view ABI currently has 5 exact old-name matches in 3 HLSL/HLSLI files, 96 in 18 shader-registration files, 112 in 40 Renderer-private files, and 1 in the boundary check. Phase 3 replaces all of them together. Including the old binding label, the separate misnamed scene-lighting ABI has 42 matches in 6 HLSL/HLSLI files, 88 in 11 shader-registration files, 144 in 21 Renderer-private files, and 1 in the boundary check; Phase 4 replaces those paired C++/HLSL names with its GPU-scene owner.

| Rejected file/path set | Sole path-deletion phase | Required disposition |
| --- | ---: | --- |
| `GameFramework/Public/Rendering/RenderInputFrame.h`, `RenderFrameMetadata.h`, `RenderWorldDelta.h`, `RenderFrameDynamicData.h`; `World/Extraction/Dynamic/RenderFrameDynamicDataExtractor.*`; `Renderer/Private/Concurrency/FrameQueue/RenderFramePacket.h` | 1 | Replace with the six frozen publication headers, target extractor vocabulary, and `RenderExecutionRequest`; update all public/API/queue consumers. |
| `Renderer/Private/SceneData/RenderWorld.*`, `SceneData/Caching/**`, and `SceneData/Material*` | 2 | Move retained CPU scene/material owners under `Renderer/Private/Scene`; delete old defining paths. |
| `Renderer/Private/SceneData/Input/**`, `SceneData/Preparation/**`, `RenderSceneData.*`, `MeshInstanceBatch.h`, `RenderLightCollection.h`, `RenderMeshClassificationConversion.*`, `RenderMeshWorkloadSummary.h`, `RenderMeshWorldBounds.h`, `RenderRayTracingWorkPlan.h`, `RenderSkyData.h` | 3 | Fold mixed input holding into scene/view/frame preparation, split/move retained scene-frame and view work to `Scene/Preparation` or `View`, and delete obsolete records/paths. |
| `Renderer/Private/Camera/**`, `Frame/Core/RenderViewData.h`, `Frame/Builders/PerFrameDataBuilder.*`, `PerViewDataBuilder.*`, `TemporalDataBuilder.*`, `Frame/Temporal/TemporalFrameState.*` | 3 | Replace with `View/RenderView*`, `RenderViewState`, and focused preparation; remove the emptied `Camera` and old builder/temporal paths. |
| `Renderer/Private/ShaderData/PerFrameConstantBufferData.h`, `PerViewConstantBufferData.h`, `RenderViewCameraData.h`, `PerTemporalConstantBufferData.h`, `RenderConstantBufferData.h`; `Assets/Shaders/Resources/ConstantBuffers.hlsli`, `CameraConstantBufferData.hlsli`, `TemporalConstantBuffer.hlsli` | 3 | Replace with the four exact `*UniformData` C++/HLSL files; use focused includes and no broad compatibility aggregate. |
| Old ABI shader registrations, generated metadata, `build/Cache/Shaders/**`, and `Projects/Showcase/Cooked/Shaders/**` | 3 | Change all source registrations and remove disposable old generated/cooked products; Phase 7 regenerates once from the complete candidate. |
| `Renderer/Private/SceneData/GpuScene/**`, `SceneData/RenderSceneGpuData.*`, `RayTracing/Scene/RayTracingSceneFrameData.h`, current `RayTracing/Scene/RenderRayTracingScene.*` ownership path | 4 | Move/rename to scene-owned `Scene/GpuScene` and `Scene/RayTracing`; replace every old `*Data` binding group with the frozen `*Bindings` vocabulary and remove old owners/getters. The old `SceneData` root is removed when its last Phase 4 child is gone. |
| `Renderer/Private/ShaderData/RenderViewLightingData.h`, `Assets/Shaders/Resources/LightConstantBufferData.hlsli`, and their registrations | 4 | Split scene light counts from structured-buffer records into the paired `SceneLightingUniformData` and `LightGpuData` C++/HLSL headers; replace the seven rejected names including every `ViewLighting` binding label. |
| `Frame/Builders/FrameContextBuilder.*`, `Frame/Core/FrameContext.h`, `FrameGraph/Execution/PassExecutionContext.h`, `FrameGraph/PassRuntimeContext.h`, `RayTracing/Scene/RayTracingPassContext.h`, and the `ImageProviderPassContext` definition in `RendererImageProviderStack.h` | 5 | Replace consumers atomically with `RenderFrame`, explicit semantic inputs, captured focused dependencies, and `PassCommandContext`; delete all old definitions. |
| Remaining `Frame/Core/**`, `FramePipeline/**`, current feature-assembly directories under `Frame/**`, and old `FrameAssembly*`/`FrameBuild*` paths | 6 | Move retained owners to the frozen `Frame`, `Frame/Graph`, `Passes`, and `FrameGraph` paths; delete emptied roots and forwarding helpers. |

The dated [Whole Repository Architecture Map](WholeRepositoryMap.md) intentionally retains current names as a source-backed historical snapshot until Phase 7. The binding naming standard already marks broad contexts as target cleanup debt. The target-only geometry-cache, deferred-decal, and editor-camera documents are reconciled in Phase 0 to target vocabulary; none may present an old renderer name as future architecture.

##### Field ownership ledger: `RenderSceneData`

The current mutable value is assembled through `RenderPreparationGraph`, `RenderPreparationInputResolver`, preparation tasks, `RenderPreparationMerger`, `RenderLightPreparation`, and `MaterialCache`. Its vectors are moved back into the selected `FrameContext` slot and reused. The final mutable authority is `RenderScene`; the final one-frame lifetime owner is `RenderFrame::PreparedScene`. A prepared product is sealed after scene preparation/GPU-binding publication and remains immutable until its frame slot, or an old graph generation containing that slot, is token-safe to reuse or retire.

| Current field | Classification | Mutable owner and producer | Material consumers | Copy/lifetime decision | Phase 3 destination |
| --- | --- | --- | --- | --- | --- |
| `structuralRevision` | scene frame | `RenderScene` revision, historically copied from `RenderWorld` by `RenderPreparationInputResolver` | `PersistentRenderGpuScene` topology invalidation | Copy one scalar into the immutable slot product. | `PreparedRenderScene::structuralRevision` |
| `materialRevision` | scene frame | `RenderScene` material revision; resolver/`MaterialCache` publish it | `PersistentRenderGpuScene` material invalidation | Copy one scalar into the slot product. | `PreparedRenderScene::materialRevision` |
| `directionalLights` | scene frame | latest scene light table; `RenderLightPreparation` | `RenderGpuLightingPayloadBuilder`, lighting invalidation hash | Move/copy compact prepared records into slot storage for parallel recording. | prepared scene light collection |
| `pointLights` | scene frame | same | same | same | prepared scene light collection |
| `spotLights` | scene frame | same | same | same | prepared scene light collection |
| `rectLights` | scene frame | same | same | same | prepared scene light collection |
| `sky` | scene frame | `RenderScene` sky; currently `RenderPreparationInputResolver::ResolveSky` | frame-graph sky import, sky/ray-lighting passes, lighting invalidation | Copy the small value/handles; scene/texture owners retain referenced resources through slot retirement. | prepared scene sky |
| `meshInstances` | scene frame | `RenderScenePreparation`; currently `RenderPreparationMerger::PublishObjects` | GPU geometry/ray payload builders, GBuffer drawing, BLAS/TLAS builders/planners, lighting invalidation | Move all prepared primitives into slot storage; asset/resource references must remain generation-pinned until retirement. | prepared scene primitive records |
| `meshWorldBounds` | scene frame | scene preparation; currently merger | partitioned-TLAS planning | Move parallel-to-primitive bounds into the same slot and preserve stable indices. | prepared scene bounds |
| `rasterMeshInstanceIndices` | current view | `RenderViewPreparation`; currently batch builder/merger after frustum selection | GPU raster-instance upload and `GBufferMeshBatchDrawer` | Move into the immutable current view; never copy into scene truth. | `RenderView` visible/raster indices |
| `meshInstanceBatches` | current view | `RenderViewPreparation`; currently `MeshInstanceBatchBuilder`/merger | `GBufferMeshBatchDrawer` | Move into view-slot work for recording. | `RenderView` raster batches |
| `jointMatrices` | scene frame | scene deformation continuity plus current publication; currently preparation graph/tasks | GPU geometry upload, BLAS deformation, lighting invalidation | Move into prepared slot storage because recording/upload can overlap later scene mutation. | prepared current joint matrices |
| `previousJointMatrices` | scene frame | `RenderScene` object/deformation continuity; currently preparation graph | GPU motion/deformation upload | Move the selected previous values into the same slot as current values. | prepared previous joint matrices |
| `morphWeights` | scene frame | scene deformation continuity plus current publication; currently preparation graph/tasks | GPU geometry upload, BLAS deformation, lighting invalidation | Move into prepared slot storage. | prepared current morph weights |
| `previousMorphWeights` | scene frame | `RenderScene` continuity; currently preparation graph | GPU motion/deformation upload | Move into prepared slot storage. | prepared previous morph weights |
| `meshWorkload` | deletion | currently recomputed by `RenderPreparationMerger::PublishWorkload` | no runtime consumer found | Delete rather than preserve dead mixed scene/view counters. New diagnostics must derive from real owners if separately justified. | none |
| `rayTracingWork` | scene frame | view-independent ray input planning in `RenderScenePreparation`; currently merger | GPU ray payload builder and classic/partitioned TLAS strategies/planners | Move view-independent BLAS/input indices into slot storage; any camera-sensitive planning belongs to `RenderViewPreparation`. | prepared ray input records |
| `materials` | scene frame | immutable scene material generation; currently a span into mutable `MaterialCache` storage | input resolution, GBuffer drawing, GPU ray payload, TLAS builders, lighting invalidation | Do not copy per frame. Pin an immutable scene-owned material generation and expose a span whose owner survives every borrowing slot; retire replaced generations by submission token. | prepared scene material span |
| `materialTextureTable` | scene frame | scene material-binding generation; currently `MaterialCache::PublishMaterialTextureTable` | GPU Scene invalidation, frame validation, ray/deferred passes, lighting invalidation | Copy only `ResolvedMaterialTextureTable::{Binding, DescriptorCount, Generation}`; the scene capability owns and retires descriptor storage. | prepared material binding projection |

`ResolvedMaterialTextureTable::Binding`, `DescriptorCount`, and `Generation` are all scene-frame binding metadata with the same producer, scene capability owner, frame-slot borrow, and Phase 3 destination as `materialTextureTable`; none becomes a graph handle. `RenderSceneData::ResetForReuse` is deleted with the type in Phase 3 and replaced by explicit slot reset/seal behavior on the owning preparation/storage path.

##### Field ownership ledger: `FrameContext` and `RenderViewData`

Today `FramePipeline` owns `m_frameContexts[frameIndex]`, where `frameIndex` is the RHI frame-in-flight index. Graph replacement moves the graph and the full context vector into `FrameExecutionRetirementQueue`, which keeps them until the last submitted token on every queue completes. The target preserves that safety with `RenderFrame` slots; it does not preserve broad access.

| Carrier and field | Classification | Producer and material consumers | Final owner/lifetime and copy decision | Target phase/destination |
| --- | --- | --- | --- | --- |
| `FrameContext::sceneData` | scene frame | `FrameContextBuilder` via preparation; GPU update, graph binding, GBuffer/ray/lighting passes and invalidation | `RenderFrame` slot owns immutable `PreparedRenderScene`; move/reuse storage. | 3; replace field value, then delete carrier in Phase 5 |
| `FrameContext::sceneGpuData` | focused owner dependency | `PersistentRenderGpuScene::Update`; graph imports and GPU-backed passes | Scene-owned `RenderGpuScene` owns resources; prepared scene borrows narrow `RenderSceneGpuBindings` pinned to the slot. | 4; `PreparedRenderScene::GpuBindings` |
| `FrameContext::rayTracingScene` | focused owner dependency | `RenderRayTracingScene::Prepare`; TLAS graph import and ray/deferred shadow passes | Scene RT capability owns resources; `RenderRayTracingFrameBindings` is a bounded setup value used to bind graph/pass inputs, not retained as a broad frame field. | 4; focused bindings then carrier deletion |
| `FrameContext::mainView` | current view | view/temporal builders; provider setup and all raster/ray/deferred pass consumers | `RenderFrame` slot owns one immutable `RenderView`; rename removes the one-off `mainView` convention. | 3; `RenderFrame::View` |
| `RenderViewData::perViewData` | current view | `PerViewDataBuilder`; copied by scene/deferred/ray passes | `RenderView` owns CPU values; only consumed `ViewUniformData`/`ViewCameraUniformData` values copy into pass parameters. | 3 |
| `RenderViewData::perTemporalData` | current view | `TemporalDataBuilder`; providers and temporal/deferred/ray passes | `RenderViewState` mutates continuity, then `RenderView` owns current `ViewTemporalUniformData`; pass copies are ABI values. | 3 |
| `RenderViewData::temporalState` | deletion | derived from `perTemporalData` by `BuildRenderTemporalFrameState`; provider setup only | Delete the duplicate representation; providers receive explicit current view/state values through `ImageProviderFrameInput`. | 3 |
| `RenderViewData::viewport` | current view | `PerViewDataBuilder` from scene extent; raster commands and dispatch sizing | Store once on immutable `RenderView`; pass setup copies only dimensions/rectangles it uses. | 3 |
| `RenderViewData::scissorRect` | current view | `PerViewDataBuilder`; raster command setup | Store once on immutable `RenderView`; no separate owner. | 3 |

##### Field ownership ledger: pass and provider contexts

At the Phase 0 baseline, the three broad ray/provider/pass contexts were stack values built in `FramePipeline::ExecuteFrameGraph` and borrowed only during the synchronous graph execution/recording call. They had no frame-slot or GPU-retirement ownership. Their referents were owned by Host, scene, graph, provider stack, or pipeline-runtime generations. Phase 5 removes the bags and gives each consumer the narrow lifetime it actually requires.

| Current field | Classification | Current producer and material consumers | Phase 5 destination or deletion |
| --- | --- | --- | --- |
| `PassRuntimeContext::HardwareInterface` | deletion | pipeline supplies RHI; raster/compute binding and GBuffer use it | Expose only required binding/resource operations through `PassCommandContext::Commands`/`Resources`; no RHI owner reference survives. |
| `PassRuntimeContext::PassRuntimes` | focused owner dependency | Host runtime cache; templated graph builder constructs passes through `GetPassRuntime` | Capture each pass's concrete pipeline runtime when building the graph generation; delete generic accessor. |
| `PassRuntimeContext::PerFrame` | pass parameter | pipeline's current old frame ABI; GBuffer, lighting, sky, debug, exposure and ray passes | Copy only consumed `FrameUniformData` and view-owned values into specific pass parameters during setup. |
| `PassRuntimeContext::DisplaySettings` | pass parameter | pipeline-resolved display settings; exposure and tone mapping | Resolve/copy focused exposure and tone-mapping values into those passes only. |
| `PassRuntimeContext::History` | pass parameter | graph `ResolveFrameHistoryValidity`; exposure, reservoir, and reference accumulation | Copy each history-valid bit only to its consuming pass; graph remains resource owner. |
| `PassRuntimeContext::Meshes` | focused owner dependency | Host `GpuMeshCache`; GBuffer mesh drawer only | Inject a required cache reference into the GBuffer mesh-drawing collaborator at its owner boundary. |
| `PassRuntimeContext::Textures` | deletion | Host `TextureCache`; no context-field consumer found | Delete without replacement. |
| `PassRuntimeContext::RayTracing` | focused owner dependency | pipeline stack context; ray scene recording and ray/deferred shadow parameters | Give each ray pass exact scene bindings/settings/flags; capture stable capabilities at the owning pass/graph generation. |
| `PassRuntimeContext::ImageProviders` | focused owner dependency | provider stack; upscaling and ray-reconstruction passes | Capture the selected provider in its graph-generation pass; provider generation governs graph rebuild/retirement. |
| `RayTracingPassContext::Scene` | focused owner dependency | active scene from Host; ray-scene build and shadow uniform construction | Inject the exact scene operation/binding required; no nullable scene pointer in unrelated passes. |
| `RayTracingPassContext::CapabilityReport` | deletion | populated from the active scene; no field consumer found | Delete from the pass surface. Persistent capability reporting remains with `RenderRayTracingScene`. |
| `RayTracingPassContext::ShadowSettings` | pass parameter | pipeline CVar resolution; shadow uniform construction | Copy `RayTracedShadowSettings` into shadow-specific setup/parameters. |
| `RayTracingPassContext::ShadowsEnabled` | pass parameter | pipeline CVar resolution; shadow uniform construction | Copy one shadow feature value into shadow-specific setup/parameters. |
| `ImageProviderPassContext::Upscaling` | focused owner dependency | `RendererImageProviderStack::BuildPassContext`; `UpscalerPass` | Capture the selected upscaler in the graph-generation pass; stack owns it until that generation retires. |
| `ImageProviderPassContext::RayReconstruction` | focused owner dependency | same; `RayReconstructionPass` | Capture the selected reconstruction provider in its graph-generation pass with the same retirement rule. |

None of the six audited carriers is the final mutable owner of persistent scene state, persistent view state, or graph topology. Apparent ownership in them is projection or reach-through and is removed. The referenced old `PerFrameConstantBufferData` is itself mixed: `FrameIndex`, total/delta time, and scaled total/delta time classify as frame identity/time and become `FrameUniformData`; target `FrameIndex` is the deliberate 32-bit shader projection of authoritative `RenderFrameIdentity::FrameId`, while time comes from identity-free `RenderFrameTime`. `ViewModeIndex`, `ViewportSize`, and `ViewportSizeInv` classify as current view and become `ViewUniformData`. No topology handle moves through `PassRuntimeContext` or `RenderFrame`; graph settings/resources remain with the graph-generation owner.

##### Frozen target owners, names, paths, and scope

| Final owner/value | Mutable or lifetime owner | Exact canonical path | Phase first established |
| --- | --- | --- | ---: |
| `RenderFrameSubmission`, `RenderSceneUpdate`, `RenderSceneDelta`, `RenderSceneDynamicData`, `RenderViewInput`, `RenderViewCameraData` | Application/Editor/GameFramework construct owned publication values | `Engine/GameFramework/Public/Rendering/<ExactType>.h` | 1 |
| `RenderExecutionRequest`, `RenderFrameTime` | RenderCoordinator combines one submission with identity-free time and `UiRenderPacket`; `RenderFrameQueue` owns it in transit, then `RenderFrame` owns the same time value | `Engine/Renderer/Private/Concurrency/FrameQueue/RenderExecutionRequest.h`, `Engine/Renderer/Private/Frame/RenderFrameTime.h` | 1 |
| `RenderScene`, `RenderPrimitive` | `RendererHost` owns lifetime on the renderer owner thread; `RenderScene` alone mutates accepted scene state | `Engine/Renderer/Private/Scene/RenderScene.*`, `RenderPrimitive.*` | 2 |
| `RenderScenePreparation`, `PreparedRenderScene` | scene preparation mutates a selected slot until seal; `RenderFrame` owns the sealed product | `Engine/Renderer/Private/Scene/Preparation/RenderScenePreparation.*`, `PreparedRenderScene.h` | 3 |
| `RenderViewBuilder`, `RenderViewPreparation`, `RenderView` | builders/preparation produce once; `RenderFrame` owns the immutable value | `Engine/Renderer/Private/View/RenderViewBuilder.*`, `RenderViewPreparation.*`, `RenderView.h` | 3 |
| `RenderViewState` | `RendererHost` owns one active state; only view construction/invalidation mutates it | `Engine/Renderer/Private/View/RenderViewState.*` | 3 |
| `FrameUniformData`, `ViewUniformData`, `ViewCameraUniformData`, `ViewTemporalUniformData` | producing frame/view owners; copied into pass ABI values | C++: `Engine/Renderer/Private/ShaderData/<ExactType>.h`; HLSL: `Engine/Assets/Shaders/Resources/<ExactType>.hlsli` | 3 |
| `ImageProviderFrameInput` | provider setup call value; provider stack/provider owns any retained private history | `Engine/Renderer/Private/Providers/ImageProviderFrameInput.h` | 3 |
| `RenderGpuScene`, `RenderSceneGpuBufferBinding`, `RenderSceneGpuLightingBindings`, `RenderSceneGpuGeometryBindings`, `RenderSceneGpuRayTracingBindings`, `RenderSceneGpuBindings` | `RenderScene` owns GPU capability/resources; frame product borrows the focused binding projection | `Engine/Renderer/Private/Scene/GpuScene/RenderGpuScene.*`, `RenderSceneGpuBindings.h` | 4 |
| `SceneLightingUniformData`; `DirectionalLightGpuData`, `PointLightGpuData`, `SpotLightGpuData`, `RectLightGpuData` | scene GPU light payload producer owns CPU staging; focused values copy into pass bindings | C++: `Engine/Renderer/Private/ShaderData/SceneLightingUniformData.h`, `LightGpuData.h`; HLSL: `Engine/Assets/Shaders/Resources/SceneLightingUniformData.hlsli`, `LightGpuData.hlsli` | 4 |
| `RenderRayTracingScene`, `RenderRayTracingFrameBindings` | `RenderScene` owns capability/resources; graph/pass setup borrows focused frame bindings | `Engine/Renderer/Private/Scene/RayTracing/RenderRayTracingScene.*`, `RenderRayTracingFrameBindings.h` | 4 |
| `RenderFrame`, `RenderFrameIdentity` | `FramePipeline` owns active frame slots; retirement queue owns old graph-generation slots until tokens complete | `Engine/Renderer/Private/Frame/RenderFrame.h`, `RenderFrameIdentity.h` | 5 |
| `PassCommandContext` | frame-graph recording executor lends command/resource/diagnostic infrastructure for one recording call | `Engine/Renderer/Private/FrameGraph/Execution/PassCommandContext.h` | 5 |
| `BuildRenderFrameGraph`, `RenderFrameGraphSettings`, `RenderFrameGraphResources`, `RenderFrameGraphTransientResources`, `RenderFrameGraphImportedSceneResources`, `ViewportFrameProducts`, `ResolvedSceneColor` | graph factory/setup owns topology values and handles for one graph generation | `Engine/Renderer/Private/Frame/Graph/BuildRenderFrameGraph.*`, `RenderFrameGraphResources.h` | 6 |

`SceneLighting` is the one exact C++ pass member, shader-registration binding, and HLSL global name for `SceneLightingUniformData`; no `ViewLighting` alias remains.

The supported scope is exactly one `RenderViewInput`, one `ViewportRenderRequest`, one `RenderView`, and one active `RenderViewState` per render invocation. Stable continuity is keyed by `ViewportRenderRequest::ViewportId` plus `ViewSelection`; request/product generation is tracked separately. `RenderFrameSubmission::View` and `RenderFrame::View` are singular values, not vectors. There is no `RenderViewFamily`, view registry, stereo abstraction, or `mainView` spelling.

Dependency direction remains `GameFramework/Application/Editor publication -> RenderCoordinator queue -> Renderer private scene/view/frame owners -> FrameGraph/pass setup -> RHI/Tasks`. Renderer never reads live `GameWorld` or Editor state; GameFramework never depends on Renderer/RHI; `RenderScene` and `RenderView` never own graph handles; FrameGraph and passes cannot reach `RendererHost`, `FramePipeline`, mutable `RenderScene`, or a service collection. `RendererHost` is the composition/lifetime owner that injects focused dependencies, not a downstream lookup surface.

##### Baseline provenance and blocked claims

No D3D12 build, Vulkan build, shader validation/cook, Showcase runtime, fixed-camera capture, or performance record tied to exact source revision `7a0833d5762f01b96e927e7a355d7b8e18a583f8` and an exact configuration, adapter/driver, workload, resolution, warm-up, and sample policy was present in repository evidence before Phase 0 edits. Repository searches found plans and older dated observations, not a provenance-matched baseline manifest. Per the no-intermediate-build rule, no baseline may now be manufactured between Phases 0 and 6.

Therefore these Phase 7 comparative claims are `BLOCKED` unless an already-existing external record with exact matching provenance is supplied: before/after fixed-camera image parity and before/after scene preparation, view preparation, graph setup, upload bytes, GPU frame time, memory high-water, graph rebuild frequency, and frames-in-flight comparison. Phase 7 still must build and validate the complete candidate, compile/validate its shaders, run D3D12 and Vulkan Showcase smokes, capture the target outputs, and report standalone final measurements. Those final checks can prove the target candidate works; without a valid before record they cannot prove preservation or improvement relative to this starting revision, and the blocked comparison cannot be silently waived.

### Phase 1 - Clean-break the publication boundary

#### Implementation prompt

> Implement Phase 1 as one source-consistent publication-boundary CL directly in the unstaged master worktree. Do not create or switch branches, and do not stage, commit, push, or submit; the user owns manual review and any source-control action. Replace the old GameFramework-to-Renderer frame packet, update every producer and consumer, and delete all old packet types and names owned by this phase. Apply the common standards and phase-specific references. Do not add compatibility and do not build, compile shaders, cook, launch, or run tests. Do not hand off until the Phase 1 legacy search is clean. This CL is not independently landable.

#### Phase-specific references

- [GameFramework and ECS](../Engineering/Standards/GameFrameworkAndEcs.md)
- [Concurrency](../Engineering/Standards/Concurrency.md)
- [Multithreaded Engine Architecture](Multithreading/MultithreadedEngineArchitecture.md)
- [Editor Viewport Camera Architecture](EditorViewportCamera.md)
- [World Coordinate, Units, and Transform Contract](WorldCoordinateAndUnits.md)

#### Required work

- replace `RenderInputFrame`, `RenderFrameMetadata`, `RenderWorldDelta`, `RenderFrameDynamicData`, and `RenderCameraData` with `RenderFrameSubmission`, `RenderSceneUpdate`, `RenderSceneDelta`, `RenderSceneDynamicData`, `RenderViewInput`, and `RenderViewCameraData`;
- replace the Renderer-private `RenderFramePacket` queue envelope with `RenderExecutionRequest { Submission, Time, Ui }` and identity-free `RenderFrameTime` so publication, queue work, time, and frame-slot lifetime have distinct names;
- update GameFramework extraction, Application/editor view publication, the render queue, Renderer input acceptance, capture metadata, provider frame inputs, diagnostics labels, existing test consumers, includes, filenames, and CMake membership together;
- keep camera/cut/teleport in `RenderViewInput`; keep objects, lights, joints, and morph values in `RenderSceneDynamicData`;
- make scene generation single-source in `RenderSceneDelta`; keep `FrameId` at the submission root;
- remove `TimeInfo::frameIndex` from retained renderer time state after validating/adapting it; `RenderFrameSubmission::FrameId` is the only accepted frame identity and `FrameUniformData::FrameIndex` is its intentional 32-bit shader projection;
- capture shader/provider generations from their Renderer owners at frame execution, keep them out of the publication packet, and feed the same focused values into `RenderFrameIdentity` when Phase 5 establishes the frame-slot owner;
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

- exact runtime/build searches return zero definitions or uses of the six old packet/data/envelope type names assigned to this phase;
- `RenderFrameQueue` has exactly one `RenderExecutionRequest` payload type and all publication/acceptance sites use the target submission directly without reconstructing an old input packet;
- every target packet field has one producer and intentional consumer, with unsupported old fields deleted;
- scene generation and `FrameId` each have one authoritative publication location;
- includes, filenames, CMake entries, diagnostics, captures, existing test consumers, and current documentation use only target vocabulary;
- scoped diff inspection, exact stale-name searches, no-write formatting, and `git diff --check` pass; no executable check is claimed.

#### CL boundary

Suggested title: `Renderer: replace frame publication with scene and view inputs`.

The CL must delete the old publication and queue-envelope headers/sources and their build entries. If any consumer still needs an old type, the phase is incomplete and must not be handed off.

### Phase 2 - Establish the persistent `RenderScene` authority

#### Implementation prompt

> Implement Phase 2 as one persistent-scene authority CL directly in the unstaged master worktree. Do not create or switch branches, and do not stage, commit, push, or submit; the user owns manual review and any source-control action. Replace `RenderWorld` and `RenderProxy` with the committed `RenderScene` and `RenderPrimitive` model, move persistent CPU scene state and scene-owned continuity to that authority, update every direct consumer, and delete the two phase-owned legacy names and paths. Keep `RenderSceneData` as the sole current frame carrier until Phase 3; do not introduce `PreparedRenderScene`, `RenderView`, an adapter, or a second representation in this CL. Do not build. This CL is not independently landable.

#### Phase-specific references

- [Repository Structure and Ownership](../Engineering/Standards/RepositoryStructureAndOwnership.md)
- [Data-Oriented Design](../Engineering/Standards/DataOrientedDesign.md)
- [Concurrency](../Engineering/Standards/Concurrency.md)
- [Graphics Engineering](../Engineering/Standards/GraphicsEngineering.md)
- [Renderer and RHI Architecture Boundary](RendererRhiBoundary.md)

#### Required work

- clean-break rename `RenderWorld` to `RenderScene` and `RenderProxy` to `RenderPrimitive` across source, files, members, diagnostics, documentation, and build membership;
- move the persistent CPU scene and material owners assigned to this phase from `Renderer/Private/SceneData` to `Renderer/Private/Scene`, leaving input acceptance, preparation, and GPU-scene paths to their already assigned phases, and keep one stable `RenderObjectId`/GPU-scene-slot relationship;
- make `RenderScene` the sole mutable renderer scene authority for primitives, materials, texture table, sky, lights, revisions, accepted sequence, dirty state, and scene-owned continuity;
- move previous object transforms and deformation continuity out of the preparation helper and beneath `RenderScene`, while preserving their current behavior until the Phase 3 split;
- keep one existing `RenderSceneData` production/consumption route for this master-worktree checkpoint; update it to consume `RenderScene` directly without creating a target frame product beside it;
- preserve accepted generation/sequence rejection, stable slot allocation, reset behavior, task-graph dependencies, deterministic mutation, and existing frame-slot reuse while changing the persistent owner;
- update all includes, filenames, diagnostics, existing tests-as-consumers, and recursive-glob build membership for the two renamed types in this CL.

#### Positive guardrails

- one mutable `RenderScene`; all prepared values derive one-way from it;
- keep one plain `RenderPrimitive` unless two real owners/lifetimes are proven;
- keep the still-current `RenderSceneData` route single and unchanged in representation until its atomic Phase 3 replacement;
- make persistent-scene ownership visible in constructors, mutation entry points, reset paths, includes, and folder placement.

#### Negative guardrails

- no `RenderSceneInterface`, proxy/info pair, subclass hierarchy, second scene snapshot, or Unreal-sized feature framework;
- no `PreparedRenderScene`, `RenderView`, temporary scene/view split record, or target-to-current conversion in this phase;
- no camera/frustum/view policy promoted into persistent `RenderScene`;
- no frame-graph handles, RHI service locator, presentation, provider, UI, or capture ownership in the scene;
- no `SceneData2`, alias, forwarding facade, duplicated material/mesh/cache objects, or parallel old/new directory tree.

#### Acceptance criteria (AC)

- runtime/build searches return zero `RenderWorld` and `RenderProxy` definitions/uses outside historical target rationale;
- there is exactly one `RenderScene` construction, mutation, reset, and destruction ownership route;
- `RenderSceneData` remains the one current frame carrier and no `PreparedRenderScene` or `RenderView` definition exists yet;
- previous object/deformation continuity has one scene-owned mutable route and no duplicate remains in a helper or pipeline;
- every moved file has reconciled includes/CMake membership and the old defining files/empty paths are deleted;
- scoped diff, owner/consumer searches, no-write formatting, and `git diff --check` pass without claiming compilation.

#### CL boundary

Suggested title: `Renderer: establish persistent render scene authority`.

This CL owns complete deletion of `RenderWorld` and `RenderProxy`, not the mixed frame product. Phase 3 owns the one-shot `RenderSceneData`/view split. Keeping the current carrier until then avoids both a temporary bridge and two simultaneous scene-frame representations.

Phase 2 checkpoint: runtime source now has one `RenderScene` owned by `RendererHost`, one `RenderPrimitive` record, and scene-owned material, light, dynamic-deformation, stable-slot, revision, accepted-sequence, dirty, and continuity state under `Renderer/Private/Scene`. `RenderSceneData` deliberately remains the only current frame carrier; input acceptance and preparation remain on their Phase 3 paths until their atomic replacement.

### Phase 3 - Atomically split the prepared scene, view, view state, and shader ABI

#### Implementation prompt

> Implement Phase 3 as one atomic scene-frame/current-view/persistent-view-state and paired C++/HLSL ABI CL directly in the unstaged master worktree. Do not create or switch branches, and do not stage, commit, push, or submit; the user owns manual review and any source-control action. Replace mixed `RenderSceneData` and every old camera/view/temporal builder with immutable frame-slot `PreparedRenderScene`, immutable current `RenderView`, persistent `RenderViewState`, and their focused preparation paths. Update every source and shader consumer, move view-derived work under `View`, and delete all old representations plus stale generated/cooked output. Do not build, compile shaders, cook, or retain old layouts. This CL is not independently landable.

#### Phase-specific references

- [Editor Viewport Camera Architecture](EditorViewportCamera.md)
- [World Coordinate, Units, and Transform Contract](WorldCoordinateAndUnits.md)
- [Graphics Engineering](../Engineering/Standards/GraphicsEngineering.md)
- [Shader Authoring and Cooked Program Architecture](Shaders/ShaderAuthoringAndCookedPrograms.md)
- [Naming and Vocabulary](../Engineering/Standards/NamingAndVocabulary.md)

#### Required work

- replace `RenderSceneData` with `RenderScenePreparation` and immutable, frame-slot-owned `PreparedRenderScene`;
- delete `RenderInputConsumer`/`RenderInputConsumeResult`: `RenderScene::Apply` owns scene generation/sequence acceptance, while frame/view preparation consumes the accepted submission directly; do not retain a second pending packet or mixed dynamic snapshot;
- replace `RenderCamera`, `RenderViewData`, `PerFrameDataBuilder`, `PerViewDataBuilder`, and `TemporalDataBuilder` with `RenderViewBuilder`, immutable `RenderView`, and persistent `RenderViewState`;
- replace semantic `ImageProviderFrameContext` with focused `ImageProviderFrameInput`, and delete the duplicate `RenderTemporalFrameState` projection;
- publish scene revisions, all primitive records/bounds, material values/bindings, prepared lights/sky, current/previous deformation, and view-independent ray inputs into `PreparedRenderScene` using moves, stable indices, and justified per-slot storage;
- move camera construction and view-owned work from `Camera`, `Frame/Builders`, and mixed scene preparation into `Renderer/Private/View`;
- construct matrices, frustum, rectangles, extents, and current camera values from immutable `RenderViewInput` plus the actual `ViewportRenderRequest`;
- wire request identity and view kind into `RenderViewState`, view mode into `RenderView`, and output flags/exposure into existing graph and presentation policy; delete the unsupported generic feature-flag field instead of leaving it inert;
- move visibility, camera distance, sorting, batching, raster work, workload summary, and view-sensitive ray planning into `RenderViewPreparation`;
- remove frustum, camera position, visibility, camera-distance ordering, raster batching, viewport, history, and graph handles from `RenderScenePreparation` and `PreparedRenderScene`;
- key `RenderViewState` by stable view identity and make it the sole semantic owner of previous matrices/jitter, temporal sample, continuity, and invalidation reasons;
- replace C++ and HLSL layouts and registrations with `FrameUniformData`, `ViewUniformData`, `ViewCameraUniformData`, and `ViewTemporalUniformData`, using identical ABI field names;
- delete the broad `RenderConstantBufferData.h` include and make C++/HLSL consumers include only the uniform layouts they use;
- delete stale generated metadata and cooked output derived from old layouts; Phase 7 regenerates canonical output once against the complete source candidate.

#### Positive guardrails

- one immutable current `RenderView` per current renderer invocation and one active persistent `RenderViewState` for the supported scope;
- one immutable `PreparedRenderScene` per selected frame slot, sealed before view preparation/recording and retained with that slot until token-safe retirement/reuse;
- justify every copied/owned field by immutable publication, task isolation, frame-slot lifetime, or GPU ABI; prefer moves, spans whose owner is proven stable, handles, and stable indices;
- keep current values in `RenderView`, previous/validity semantics in `RenderViewState`, and history resource ownership in graph/providers;
- preserve matrix spaces, units, `FovY`, `NearZ`, `FarZ`, jitter NDC naming, motion/depth conventions, and layout assertions;
- update C++, HLSL/HLSLI, shader registrations, parameter names, includes, files, diagnostics, existing test consumers, and build/cook manifests together at source level.

#### Negative guardrails

- no `RenderViewFamily`, stereo/multi-view framework, mutable camera mirror, global temporal builder, `mainView` special case, or duplicate base/derived view storage;
- no `RenderSceneData`, target/current adapter, temporary scene-view transfer bag, or second scene-frame product;
- no camera/frustum/view policy in `RenderScene` or `PreparedRenderScene`;
- no old/new constant buffers, shader macro bridge, registration alias, compatibility layout, or cooked package accepting the old ABI;
- no scene-wide data copied into the view and no history textures/provider objects owned by `RenderViewState`;
- no source-only placeholder field without an intentional producer and consumer.

#### Acceptance criteria (AC)

- exact searches return zero `RenderInputConsumer`, `RenderInputConsumeResult`, `RenderSceneData`, old camera/view/builder symbols, `RenderTemporalFrameState`, `ImageProviderFrameContext`, `RenderConstantBufferData`, `mainView`, and the four old constant-buffer type/registration names in runtime, shader, build, test-consumer, generated, or cooked inputs;
- `PreparedRenderScene` has one producer, frame-slot owner, seal point, consumer set, and retirement/reuse condition, and its signatures/includes contain no view/frustum/camera types;
- all view-dependent fields formerly in scene preparation have one owner under `RenderView`/`RenderViewPreparation`;
- all continuity/reset producers converge on `RenderViewState` with explicit stable identity and invalidation reasons;
- target C++ and HLSL ABI fields match by name/order/type and source layout assertions reflect only the new layouts;
- unsupported viewport request promises and old generated/cooked representations are deleted;
- scoped diff, source ABI inventory, no-write formatting, and `git diff --check` pass; shader compilation and runtime claims are deferred to Phase 7.

#### CL boundary

Suggested title: `Renderer: split prepared scene and render view with unified shader ABI`.

The CL is not ready for handoff while any old view builder/layout remains. Keep the C++ and HLSL rename in this one review unit.

### Phase 4 - Move GPU Scene and ray-tracing scene capabilities under `RenderScene`

#### Implementation prompt

> Implement Phase 4 as one persistent scene-GPU capability ownership and paired scene-lighting ABI CL directly in the unstaged master worktree. Do not create or switch branches, and do not stage, commit, push, or submit; the user owns manual review and any source-control action. Move GPU Scene and ray-tracing scene lifetime under `RenderScene`, preserve frame-slot and GPU-retirement correctness, update every allocation/reset/import/shader consumer, and delete the old FramePipeline/RendererHost ownership routes and types. Do not build, compile shaders, cook, run a backend, or add a fallback. This CL is not independently landable.

#### Phase-specific references

- [Graphics Engineering](../Engineering/Standards/GraphicsEngineering.md)
- [Renderer and RHI Architecture Boundary](RendererRhiBoundary.md)
- [Data-Oriented Design](../Engineering/Standards/DataOrientedDesign.md)
- [Concurrency](../Engineering/Standards/Concurrency.md)
- [Shader Authoring and Cooked Program Architecture](Shaders/ShaderAuthoringAndCookedPrograms.md)
- [Ray-Tracing Pipeline and Dual-Execution Delivery Plan](Shaders/RayTracingPipelineImplementationPlan.md)

#### Required work

- rename/move `PersistentRenderGpuScene` to scene-owned `Scene/GpuScene/RenderGpuScene`; replace `RenderSceneGpuData`, `RenderSceneGpuBuffer`, and the three old nested `*Data` groups with `RenderSceneGpuBindings`, `RenderSceneGpuBufferBinding`, and focused lighting/geometry/ray-tracing binding groups;
- replace `RayTracingSceneFrameData` with focused `RenderRayTracingFrameBindings`, used only while binding graph/pass inputs;
- replace the scene-wide `ViewLightingData` uniform and `ViewLighting` binding with `SceneLightingUniformData`/`SceneLighting`; replace the four structured-buffer light `*ConstantBufferData` records with the exact `*LightGpuData` names in paired C++/HLSL headers and registrations;
- move logical ownership of `RenderRayTracingScene` beneath `RenderScene` in `Scene/RayTracing`, while keeping rendering passes in `Passes/RayTracing` and backend mechanics behind RHI;
- update GPU Scene from `PreparedRenderScene` for the selected frame slot, preserving stable slots, dirty ranges, revisions, capacity growth, frame boundaries, and token-based retirement;
- separate view-independent traceable inputs, view-dependent TLAS planning, and persistent BLAS/TLAS resources according to their lifetimes;
- keep graph import handles in `RenderFrameGraphImportedSceneResources`, never in persistent scene capabilities;
- reconcile construction, reset, resize, device loss, shutdown, generation invalidation, frame-slot reuse, and delayed GPU completion;
- remove all old ownership members, getters, reset paths, allocation sites, files, includes, and CMake entries.

#### Positive guardrails

- one `RenderScene` owner with separate focused `RenderGpuScene` and `RenderRayTracingScene` capabilities;
- persistent indexed buffers plus deliberate dirty updates and bounded frame-indexed dynamic storage;
- explicit CPU owner, GPU token, replacement publication, and retirement rules;
- paired C++/HLSL scene-lighting layouts with identical field order/type/name and no view-owned spelling for scene-wide counts;
- neutral Renderer policy and D3D12/Vulkan parity preserved for final validation.

#### Negative guardrails

- no merged GPU/RT god capability, duplicate capability instance, FramePipeline-owned persistent scene resources, or RendererHost service lookup;
- no frame-graph handle stored beyond graph generation and no view-dependent value promoted to persistent scene truth;
- no `ViewLighting*` alias, light `*ConstantBufferData` record, duplicate registration, or compatibility shader binding;
- no device-idle shortcut, unconditional full-scene rebuild/upload, vendor branch in neutral policy, or old/new reset fallback;
- no alias from old type names and no forwarding getter retaining old ownership.

#### Acceptance criteria (AC)

- searches show exactly one owner/allocation route for `RenderGpuScene` and `RenderRayTracingScene` and zero old `PersistentRenderGpuScene`, `RenderSceneGpuData`, nested old GPU `*Data` binding groups, `RayTracingSceneFrameData`, `RenderViewLightingData`, `ViewLightingData`, `ViewLighting`, or light `*ConstantBufferData` record symbols;
- `FramePipeline` and `RendererHost` contain no persistent GPU-scene/ray-scene ownership member or lifecycle policy;
- dirty update, capacity replacement, frame-slot binding, device-loss/reset, and retirement paths have one source-level route and explicit lifetime comments only where non-obvious;
- graph imports consume narrow scene bindings without storing graph handles on persistent owners;
- target scene-lighting C++/HLSL layouts and registration/binding labels match exactly, and no old scene-lighting generated/cooked representation remains;
- paths, includes, CMake membership, diagnostics, and current documentation use the target names;
- scoped diff, ownership/allocation/reset searches, no-write formatting, and `git diff --check` pass without backend execution claims.

#### CL boundary

Suggested title: `Renderer: move GPU and ray-tracing scene lifetime under render scene`.

This phase cleans every old capability owner and name. Failure to prove one construction/reset/retirement route blocks the CL; it is not solved with a fallback instance.

Phase 4 checkpoint: `RenderScene` is the sole lifetime owner of separate `RenderGpuScene` and `RenderRayTracingScene` capabilities. The selected prepared-scene slot borrows one `RenderSceneGpuBindings` projection, while `RenderRayTracingFrameBindings` exists only during graph import binding. Scene lighting now uses paired `SceneLightingUniformData` and `LightGpuData` C++/HLSL headers with the exact `SceneLighting` binding. Graph handles remain owned by the existing frame-graph resource layout until its Phase 6 clean-break rename; no graph handle is stored by either persistent scene capability. Backend compilation and execution evidence remains deferred to Phase 7.

### Phase 5 - Remove broad frame/pass contexts and make pass inputs explicit

#### Implementation prompt

> Implement Phase 5 as one pass-input and recording-surface CL directly in the unstaged master worktree. Do not create or switch branches, and do not stage, commit, push, or submit; the user owns manual review and any source-control action. Replace broad semantic contexts with pass-specific parameters and the narrow `PassCommandContext`, update every authored pass and frame-graph execution consumer, reconcile the naming standard, and delete all old context bags and forwarding access. Do not build or add context-shaped replacements. This CL is not independently landable.

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

Phase 5 source checkpoint: `RenderFrame` is the frame-slot owner for identity/time, one `PreparedRenderScene`, and one `RenderView`. Frame-graph setup copies only consumed semantic values into pass parameters; concrete pass runtimes, image providers, ray-tracing scene capability, and the GBuffer mesh cache are captured or injected at graph-generation owner boundaries. Recording receives only `PassCommandContext` command, declared-resource, and diagnostic infrastructure. The six legacy carrier/builder types and their forwarding access are absent from runtime source. This checkpoint is source-only and not independently landable; compilation and recording evidence remains deferred to Phase 7.

### Phase 6 - Make frame orchestration and directory ownership match the target

#### Implementation prompt

> Implement Phase 6 as one frame-orchestration, graph-vocabulary, and physical-layout CL directly in the unstaged master worktree. Do not create or switch branches, and do not stage, commit, push, or submit; the user owns manual review and any source-control action. Make `FramePipeline` a thin lifecycle sequencer, replace technique-specific or ambiguous graph construction/resource names with the exact neutral `RenderFrameGraph` vocabulary, move files to the canonical Scene/View/Frame/Passes/FrameGraph paths, reconcile CMake/includes, and delete emptied roots and forwarding helpers. Do not build and do not add a renderer facade. This CL is not independently landable.

#### Phase-specific references

- [Repository Structure and Ownership](../Engineering/Standards/RepositoryStructureAndOwnership.md)
- [Coding Style](../Engineering/Standards/CodingStyle.md)
- [Naming and Vocabulary](../Engineering/Standards/NamingAndVocabulary.md)
- [Renderer and RHI Architecture Boundary](RendererRhiBoundary.md)
- [Whole Repository Architecture Map](WholeRepositoryMap.md)

#### Required work

- replace `BuildFrame`, `FrameBuildSettings`, and `FrameBuildResult` with `BuildRenderFrameGraph` and `RenderFrameGraphSettings`, returning `RenderFrameGraphResources` directly;
- replace `FrameAssemblyResourceLayout` and nested `FrameAssembly*` types with `RenderFrameGraphResources`, `RenderFrameGraphTransientResources`, `RenderFrameGraphImportedSceneResources`, and `ViewportFrameProducts`;
- replace `FinalSceneColorProduced` with one initially invalid `ResolvedSceneColor` handle published by reconstruction/upscaling and consumed by presentation;
- delete `FrameResolutionExtents`; render/output extents have one current-view value and are copied into `RenderFrameGraphSettings` only as topology inputs;
- keep render/output extent, presentation target, render modes, exposure metering topology, requested outputs, and provider graph key local to render-frame graph/pipeline topology;
- make `FramePipeline` read as `BeginFrame`, `PrepareFrame`, `ExecuteFrame`, and `SubmitAndPresent`, delegating capability mechanics to their owners;
- move `FramePipeline.*` under `Frame`, dissolve touched generic `Frame/Core` files into `Frame`, `Frame/Graph`, `View`, or deletion, and enforce the target directory navigation map;
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

- exact searches return zero old graph-construction/resource type names, `FrameResolutionExtents`, and `FinalSceneColorProduced` uses;
- `FramePipeline` owns no persistent scene/view/GPU/RT capability and its primary path visibly sequences the four documented stages;
- target owners exist only under canonical `Scene`, `View`, `Frame`, `Passes`, `FrameGraph`, `ShaderData`, and `ShaderParameters` locations;
- the old `SceneData`, `Camera`, `FramePipeline`, and touched generic `Frame/Core` roots are empty and removed, with no stale includes/CMake/source-group entries;
- `RendererHost` is composition-only and no downstream pass/preparation code uses it as a service locator;
- scoped diff, file/CMake/include inventory, no-write formatting, documentation links, and `git diff --check` pass without claiming a build.

#### CL boundary

Suggested title: `Renderer: finalize scene-view-frame ownership and navigation`.

The phase delivers the physical architecture, not a cosmetic file shuffle. Every move must correspond to the owner map, and every emptied legacy path is deleted in this CL.

### Phase 7 - Regenerate, validate, reconcile, and atomically land

#### Implementation prompt

> Implement Phase 7 directly in the unstaged `master` worktree against the complete Phase 0-6 candidate. Do not create or switch branches, and do not stage, commit, push, or submit; the user owns manual review and every source-control action. Run the final legacy-eradication gate first, regenerate canonical artifacts once, then perform claim-driven focused checks, shader validation, the required DevelopmentEditor D3D12/Vulkan build and Showcase runtime/capture evidence. Fix failures at their real owner and keep them attributable in final review. Update current architecture documentation only after all acceptance criteria pass; never add compatibility to make validation pass.

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
- fix code at the responsibility that owns the invariant, then rerun affected and final evidence on the exact `master` candidate;
- preserve unsupported/unavailable evidence as an explicit blocked claim rather than simulated success.

#### Negative guardrails

- no compatibility alias, legacy reader, fallback path, disabled assertion, skipped backend, device-idle workaround, or widened context to make the candidate pass;
- no broad clean rebuild/all-content cook unless a specific final claim requires it or stale state makes narrower evidence inconclusive;
- no miscellaneous Phase 7 source-fix bucket, permanent migration validator, new submitted test harness, or documentation claim ahead of executable proof;
- no branch, merge, cherry-pick, rebase, release, or status update that treats an individual phase as the completed architecture.

#### Acceptance criteria (AC)

- every row of the Final Atomic Cutover Gate and applicable Validation Matrix passes on the exact final candidate;
- regenerated/tracked artifacts contain only target names/layouts and disposable old output is absent from package/build inputs;
- focused checks, shader validation, `DevelopmentEditor` D3D12/Vulkan build, both Showcase smokes, required captures, lifetime/retirement evidence, and applicable performance comparison have exact recorded results;
- the final diff contains no unrelated changes, temporary proof code, compatibility machinery, stale CMake/include/document references, or deferred cleanup;
- any unavailable required evidence makes the candidate `BLOCKED`; `master` remains a private, non-releasable migration state;
- the user reviews the exact validated `master` candidate and alone decides whether to commit, push, submit, release, or roll it back.

#### CL boundary

Suggested title: `Renderer: validate and accept scene-view-frame architecture`.

The Phase 7 CL contains canonical regenerated artifacts and final truthful documentation/evidence reconciliation. Source fixes discovered here remain attributed to their owning responsibility in final review. The final candidate has one scene owner, one view owner, one view-state owner, one frame-slot product, one shader ABI, and narrow pass inputs on both backends.

## Final Atomic Cutover Gate

The candidate is not landable until every gate below passes against the exact integration head. A passing workstream checkpoint cannot waive or defer a final gate.

| Gate | Required final state | Required proof |
| --- | --- | --- |
| Legacy runtime symbols | No definition, declaration, include, construction, parameter, member, call site, test fixture, shader declaration, or registration remains for `RenderInputFrame`, `RenderFrameMetadata`, `RenderWorldDelta`, `RenderFrameDynamicData`, `RenderCameraData`, `RenderFramePacket`, `RenderInputConsumer`, `RenderInputConsumeResult`, `RenderWorld`, `RenderProxy`, `RenderSceneData`, `RenderViewData`, `RenderCamera`, `PerFrameDataBuilder`, `PerViewDataBuilder`, `TemporalDataBuilder`, `RenderTemporalFrameState`, `ImageProviderFrameContext`, `FrameContextBuilder`, `FrameContext`, `PassExecutionContext`, `PassRuntimeContext`, `RayTracingPassContext`, `ImageProviderPassContext`, `PersistentRenderGpuScene`, `RenderSceneGpuData`, `RenderSceneGpuBuffer`, `RenderSceneGpuLightingData`, `RenderSceneGpuGeometryData`, `RenderSceneGpuRayTracingData`, `RayTracingSceneFrameData`, `RenderViewLightingData`, `ViewLightingData`, `ViewLighting`, the four light `*ConstantBufferData` records, `FrameAssemblyResourceLayout`, the nested `FrameAssembly*` types, `FrameBuildSettings`, `FrameBuildResult`, `FrameResolutionExtents`, `FinalSceneColorProduced`, `RenderConstantBufferData`, `PerFrameConstantBufferData`, `PerViewConstantBufferData`, `PerViewCameraConstantBufferData`, or `PerTemporalConstantBufferData`. The render-frame graph has no old `BuildFrame` declaration/call site or `mainView` member/access. | Whole-repository exact-symbol search over `Engine`, `Projects`, executable build files, shader sources, tests, and generators returns zero matches. The old `ConstantBuffers.hlsli`, `CameraConstantBufferData.hlsli`, `TemporalConstantBuffer.hlsli`, `LightConstantBufferData.hlsli`, Renderer-private `ShaderData/RenderViewCameraData.h`, and `ShaderData/RenderViewLightingData.h` paths are absent; the target GameFramework `RenderViewCameraData` is the only permitted exact type. This target document may retain old names only to define their deletion. |
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
| Atomic repository state | The exact `master` candidate contains all code, shader, build, test, generated-artifact, and documentation changes. Earlier phase commits remain private checkpoints and are never presented as supported releases. | Final full diff/state review on `master`; required checks run on the exact candidate. The user alone performs any commit, push, submit, release, or rollback. |

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
- If a workstream cannot preserve behavior with a single owner, stop editing the `master` worktree and revise this target rather than adding a fallback or presenting a partial state as complete.

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

- the complete `master` phase series passes the final atomic cutover gate on the exact candidate with no deferred cleanup;
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
- `BuildRenderFrameGraph` is the only renderer frame-graph construction entry point;
- passes use explicit semantic parameters plus narrow `PassCommandContext` infrastructure and cannot reach a broad context or service locator;
- `FramePipeline` sequences owned stages without owning scene/view capabilities;
- every target value has one producer, intentional consumers, one lifetime, and a tested invalidation/retirement rule; unsupported request fields are deleted rather than left inert;
- D3D12 and Vulkan builds, focused tests, runtime smokes, captures, and performance comparisons support the claims;
- the implemented architecture map and document status are reconciled with the proven code.

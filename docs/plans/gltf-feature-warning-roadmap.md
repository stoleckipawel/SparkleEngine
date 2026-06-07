# glTF Feature Warning Roadmap

This plan turns glTF importer warnings into staged feature work. The order is intentionally easiest to hardest: first make importer capabilities explicit, then wire features that already have engine runtime concepts, then build the animation and skinning foundation that needs new cooked formats and renderer support.

## Current Architecture Baseline

- Source import creates normalized source data in `SourceImportResult`; it does not create runtime objects and runtime code must not depend on source importer internals.
- Cooked scene manifests are the durable handoff. Camera and light records now live in the cooked scene manifest alongside mesh/material/instance data.
- GameFramework owns ordinary scene objects through `SceneCameras`, `SceneLighting`, `SceneMaterials`, `SceneMeshes`, and `SceneTextures`. Imported cameras and lights are not special at runtime; import is just one authoring path that creates the same scene camera/light data.
- Mesh identity should distinguish static mesh assets from skeletal mesh assets. A skeletal mesh is not just a static mesh with optional skin fields; it is a separate asset/runtime concept with skeleton binding, skin influence streams, and later animation/pose requirements.
- `GameSceneAssetPayloadAppender` is the assembly boundary that applies cooked scene payloads to GameFramework owners.
- `SceneLighting` is the runtime light collection. Loading helpers live under `Engine/GameFramework/Private/Scene/Lighting/Loading`, and frame/export snapshot helpers live under `Engine/GameFramework/Private/Scene/Lighting/Snapshots`.
- Renderer and RHI consume snapshots and cooked/render data only. They must not learn about glTF, source import data, imported-vs-authored flags, or editor selection state.

## Implementation Status

| Stage | Status | Notes |
| --- | --- | --- |
| Stage 0: Import Feature Diagnostics | Implemented | `SourceImportResult` exposes diagnostics and counts; cooker/converter print summaries. |
| Stage 1: glTF Camera Import | Implemented | glTF cameras import into cooked scene camera records and become ordinary `SceneCameraEntry` data in `SceneCameras`. |
| Stage 2: glTF Light Import | Implemented through GameFramework | glTF punctual lights import into cooked scene light records and ordinary `SceneLightDesc` data in `SceneLighting`; renderer shading currently consumes directional light snapshots. |
| Stage 3: Scene Metadata Manifest Versioning | Implemented as metadata foundation | Manifest version is `9` and includes camera records, light records, skeleton refs, optional animation refs, morph weights, material variant records, material variant mappings, scene feature flags, per-instance skeleton bindings, and per-instance source node identity. |
| Stage 4: Skeleton and Skin Data Import | Implemented through bind-pose data path | glTF skins import into cooked skeleton assets, separate mesh skin influence streams, cooked scene skeleton refs, GameFramework `SceneSkeletons`, and mesh skeleton bindings. Animation playback and GPU skinning remain later stages. |
| Stage 4.5: Static/Skeletal Mesh Asset Split | Implemented | Static Mesh and Skeletal Mesh are explicit cooked/runtime concepts; Stage 5+ animation work must keep using this split instead of reintroducing optional skin fields into the static mesh path. |
| Stage 5: Animation Clip Import and Cooking | Implemented for TRS and weight channels | glTF animations import into cooked animation assets and manifest animation refs; runtime can enumerate clips from cooked data. |
| Stage 6: Runtime Animation Playback | Implemented for skeletal TRS and morph-weight playback | GameFramework owns playback/evaluation; Renderer consumes immutable pose and morph-weight snapshots. |
| Stage 7: Renderer Skinning Path | Implemented baseline, scalability pending | Renderer has explicit skeletal draw classification, skin influence buffers, and per-frame joint palette upload. Compute/batched skinning remains future scalability work. |
| Stage 8: Renderer Point and Spot Light Shading | Implemented with D3D12 and Vulkan validation | GameFramework and renderer snapshots carry directional, point, and spot lights; Vulkan default texture upload, sampler binding, and descriptor-set submission now validate the path. |
| Stage 9: Morph Targets and Weighted Nodes | Implemented for skeletal/deformable meshes | Skeletal morph target streams import/cook/load, weighted nodes initialize morph state, weight animation channels drive runtime skeletal morph weights, and static mesh data remains clean. |
| Stage 10: Material Variants and Mesh Instancing | Implemented with D3D12 and Vulkan validation | Material variants import/cook/load into scene metadata and are selectable through GameFramework/editor. Authored mesh GPU instancing preserves instance groups through renderer batching diagnostics. |

## Current Warning Map

| Warning | Current owner | Missing feature | First functional owner |
| --- | --- | --- | --- |
| `animations are present and will be ignored` | Former glTF warning for supported TRS and weight clips | Unsupported/unknown channels only | GameFramework animation system evaluates TRS clips and skeletal morph-weight channels, with Renderer consuming evaluated skinning/deformable data |
| `nodes contain cameras and they will be ignored` | Former glTF warning | Implemented as camera import and cooked scene camera records | SourceImportAdapters -> SceneCooker -> GameFramework `SceneCameras` |
| `nodes contain lights and they will be ignored` | Former glTF warning | Implemented as punctual light import and cooked scene light records | SourceImportAdapters -> SceneCooker -> GameFramework `SceneLighting`; Renderer directional snapshot path consumes supported render data |
| `skinned nodes are present and will be imported as static data only` | Conditional glTF fallback warning | Only malformed or incomplete skin data should still hit this fallback | SourceImportAdapters -> cooked skeleton/mesh data -> GameFramework `SceneSkeletons` and mesh bindings -> Renderer skinning |

Remaining diagnostics to keep explicit when source assets actually contain them: unsupported glTF material extensions, unsupported embedded/encoded texture sources, Draco-compressed primitives, non-triangle primitives, and malformed skin/morph data that cannot satisfy the skeletal/deformable path.

## Shared Rules

- Remove or downgrade a warning only when the feature is functionally consumed by the next engine stage. Importing unused metadata is not enough.
- Keep SourceImportAdapters responsible only for source-format extraction and normalization into `SourceImportResult` data.
- Keep SceneCooker responsible for stable cooked scene manifests and asset references.
- Keep GameFramework responsible for runtime scene, level, camera, light, animation, and skeleton state.
- Keep Renderer responsible for render-side snapshots, GPU resources, skinning dispatch/draw setup, and shader-facing layouts.
- Preserve runtime cooked-only loading. Do not add runtime glTF parsing.
- Prefer versioned cooked asset/manifest changes over sidecar compatibility shims.
- Do not introduce imported-vs-created behavior branches in GameFramework, Editor, Renderer, or RHI. Imported cameras/lights must become the same scene data as authored/default cameras/lights.
- Do not add camera overrides for imported cameras. A scene has a set of cameras with deterministic active/default selection.
- Do not make import code directional-light-specific. Import and cooked scene metadata should carry directional, point, and spot light data; renderer shading support can land independently through snapshot/render updates.
- Do not model skeletal meshes as a permanent optional extension of static meshes. Static and skeletal mesh paths may share lower-level buffer helpers, but asset identity, cooked validation, runtime component semantics, editor labeling, and renderer draw classification should be explicit.
- Keep orchestrators thin. Feature extraction, cooked record translation, scene payload assembly, loading conversion, and snapshot building should live in dedicated files/folders with names that match their responsibility.

## Stage 0: Import Feature Diagnostics

Goal: Make unsupported glTF content visible as structured import diagnostics so later stages can prove when a warning is still intentional and when it is resolved.

Why first: This is the smallest stage and gives every later stage a precise acceptance gate.

Implementation prompt:

```text
Add structured glTF feature diagnostics for cameras, lights, animations, skins, morph targets, material variants, and mesh instancing.

Keep the extraction in Tools/Import/SourceImportAdapters. Extend SourceImportResult with a compact diagnostics/capability summary that records feature counts and whether each feature is imported, partially imported, or unsupported. GltfSceneReader should populate this summary while preserving current log messages. AssetCooker and AssetConverter should print the summary after import. Do not change runtime behavior yet.
```

Acceptance criteria:

- `SourceImportResult` exposes per-feature counts for animations, camera nodes, light nodes, skinned nodes, weighted nodes, morph targets, material variants, and mesh GPU instancing.
- Existing warning logs still appear for unsupported features.
- AssetCooker and AssetConverter logs include one concise import feature summary per scene.
- No runtime code depends on source importer internals.
- Validation command: `cmake --build build --config DevelopmentEditor --target AssetCooker AssetConverter -- /nologo /v:minimal /m:1 /p:UseMultiToolTask=false /p:TrackFileAccess=false /nodeReuse:false`.

## Stage 1: glTF Camera Import

Status: Implemented. Keep this section as the design contract for maintenance and regression checks.

Goal: Convert glTF camera nodes into ordinary scene cameras that can be selected and used by the editor/runtime camera system.

Why this is easiest: Sparkle already has `CameraDesc`, `SceneCamera`, `SceneCameras`, level parsing, and runtime camera application. The source-to-cooked bridge now creates the same camera data as authored/default cameras.

Implementation prompt:

```text
Implement glTF camera import as a functional feature.

Add ImportedCamera data to SourceImportResult with name, world transform, projection kind, vertical FOV, near/far planes when available, and source node index. GltfCameraImporter should extract camera nodes from cgltf. SceneCooker should persist cameras into cooked scene camera records. SceneManifestLoader should read cooked camera records, SceneAssetManager should include them in SceneAssetPayload, and GameSceneAssetPayloadAppender should append them to SceneCameras as ordinary SceneCameraEntry values.

Keep SourceImportAdapters format-focused. Keep GameFramework responsible for turning cooked camera records into CameraDesc/SceneCameraEntry. Do not make Renderer or RHI aware of glTF cameras. Do not add imported-camera override behavior; imported cameras and default/authored cameras are the same runtime concept.
```

Acceptance criteria:

- glTF camera nodes no longer produce an ignored-camera warning when camera data is imported, cooked, loaded, and consumed.
- Imported cameras are appended to `SceneCameras` as ordinary selectable scene cameras.
- Clicking a camera in the scene outliner can make it the viewport camera using the same behavior as any other scene camera.
- Perspective cameras map FOV and transform correctly enough to frame known sample content.
- Orthographic cameras are imported as scene camera data and either selected only when runtime support exists or diagnosed without changing renderer/RHI ownership.
- The feature handles multiple cameras deterministically, using a documented primary-camera policy.
- A recooked sample scene with glTF cameras launches with the imported camera on D3D12 and Vulkan.

Validation prompt:

```text
Force recook Showcase, launch runtime smoke with a glTF-camera scene on D3D12 and Vulkan, and verify logs show imported camera consumption with no ignored-camera warning for that scene.
```

## Stage 2: glTF Light Import

Status: Implemented through import, cook, load, editor, and GameFramework scene data. Renderer shading currently consumes the directional subset through `LightingSnapshot`.

Goal: Convert glTF lights into ordinary scene lights where Sparkle has matching runtime light concepts, without making imported lights special.

Why second: Lighting already has GameFramework and Renderer concepts. Import/cook/load should carry all glTF punctual light types up front; renderer support can consume additional light types later through snapshots and shaders.

Implementation prompt:

```text
Implement glTF KHR_lights_punctual import.

Add ImportedLight data to SourceImportResult with type, name, world transform, direction, color, intensity, range, cone angles, visibility/default enabled state, and source node index. GltfLightImporter should parse glTF node lights from cgltf. SceneCooker should persist all punctual light types into cooked scene light records. SceneManifestLoader should read cooked light records, SceneAssetManager should include them in SceneAssetPayload, and GameSceneAssetPayloadAppender should append them to SceneLighting as ordinary SceneLightDesc values.

Do not make import code directional-specific. Directional, point, and spot payloads should share common light data and specialize only the fields unique to that light kind. Keep renderer support separate: the current snapshot/shader path may consume directional lights first, but that must not leak back into import, cooking, GameFramework ownership, or editor presentation.
```

Acceptance criteria:

- Directional, point, and spot glTF lights are imported, cooked, loaded, and represented as ordinary `SceneLightDesc` values.
- Direction, color, intensity, range, cone angles, world transform, source node index, and enabled/default visibility survive import, cook, and load.
- Editor scene outliner and inspector show distinct light kinds and do not use directional-light stand-ins for point or spot lights.
- `nodes contain lights and they will be ignored` is removed for scenes whose light records are imported and loaded into GameFramework.
- Renderer-facing directional snapshots obey `RenderConfig::Lights::MaxDirectionalLights` with deterministic truncation behavior until broader punctual light shading lands.
- D3D12 and Vulkan smoke renders show the same imported lighting path.

Validation prompt:

```text
Recook a scene with KHR_lights_punctual directional, point, and spot lights. Launch runtime/editor smoke on D3D12 and Vulkan. Verify all light types appear in GameFramework scene lighting and editor presentation, and verify directional lights appear in renderer lighting input.
```

## Stage 3: Scene Metadata Manifest Versioning

Status: Implemented as the metadata foundation. Cooked scene manifest version `8` carries camera records, light records, skeleton refs, optional animation refs, morph weights, material variant records, material variant mappings, scene feature flags, and per-instance skeleton-ref bindings.

Goal: Maintain the durable cooked scene metadata path needed by cameras, lights, skeletons, and animations while preserving the GameFramework cooked payload boundary.

Why here: Cameras and lights already proved the cooked scene metadata path. Animation and skinning should build on the same manifest model rather than adding ad hoc sidecars or source-runtime shortcuts.

Implementation prompt:

```text
Extend the versioned cooked scene manifest metadata path.

Preserve existing camera and light records in CookedSceneManifest. Add optional skeleton refs, animation refs, and feature flags using the same manifest writer/reader pattern. Update SceneCooker writer and SceneManifestLoader reader together. Keep the runtime cooked-only. Add validation that mesh/material/instance ordering still holds, and make every new metadata block optional for scenes without those features.

Do not replace the current GameFramework handoff. SceneAssetPayload remains the cooked payload boundary, and GameSceneAssetPayloadAppender remains responsible for applying loaded payload data to GameFramework owners.
```

Acceptance criteria:

- Cooked scene manifest version is `8`, and load validation rejects mismatched versions with a clear recook error.
- SceneManifestLoader reads camera, light, skeleton-ref, animation-ref, morph-weight, material-variant, material-variant-mapping, and feature-flag metadata without touching source glTF files.
- SceneAssetManager carries available scene metadata through `SceneAssetPayload`; GameSceneAssetPayloadAppender remains the GameFramework application boundary for concrete scene owners.
- Scenes without metadata remain valid after recook under manifest version `6`.
- Source validation or focused cooker validation proves camera/light/skeleton/animation/material-variant metadata counts in the manifest match imported counts.
- Existing camera/light metadata behavior remains unchanged after the version update.

Validation prompt:

```text
Force recook Showcase, verify all .sscn manifests use the new version, then launch D3D12 and Vulkan runtime smoke to prove old mesh/material/instance loading still works.
```

## Stage 4: Skeleton and Skin Data Import

Status: Implemented through import, cook, load, and runtime bind-pose data ownership. This stage does not implement animation playback or GPU skinning; those remain Stage 5 through Stage 7.

Goal: Stop treating skinned nodes as ordinary static meshes by importing skeletons, joints, inverse bind matrices, and vertex joint weights.

Why this is harder: It changes mesh vertex payloads, cooked mesh format, asset IDs, runtime scene state, and renderer input layouts.

Implementation prompt:

```text
Implement glTF skeleton and skin data import without animation playback yet.

Extend SourceImportResult with ImportedSkeleton, ImportedJoint, ImportedSkinBinding, and skinned mesh vertex influences. GltfGeometryImporter should read JOINTS_0 and WEIGHTS_0, normalize weights, and associate mesh instances with a skin. Cook skeletons as versioned `.sskel` assets referenced by manifest skeleton refs. Update MeshCooker and cooked mesh loading to preserve skin influences as a separate optional stream, not as permanent fields on static mesh vertices. Runtime should load skeleton bind pose into `SceneSkeletons`, attach skeleton asset IDs to ordinary mesh components, and expose a neutral pose for skinned meshes.

Do not implement animation playback in this stage. The acceptance target is correct bind-pose/neutral-pose data handoff, not moving characters. Do not make Renderer or RHI aware of glTF or source import data; renderer-side work should only see ordinary mesh data, opaque skeleton IDs, and later pose snapshots.
```

Acceptance criteria:

- Skinned glTF nodes import skeleton and skin binding data instead of only static mesh data.
- Vertex joint indices and weights are validated, normalized, cooked, loaded, and visible to renderer-side mesh setup as a separate skinned mesh stream.
- Inverse bind matrices are preserved and associated with the correct joints.
- The runtime loads skeleton bind pose/neutral pose data and attaches mesh instances to skeleton asset IDs through ordinary GameFramework scene owners.
- The old `skinned nodes are present and will be imported as static data only` warning is removed only for assets that successfully load through the skinned path.
- Static mesh import remains unchanged for non-skinned assets.
- Static mesh cooked/runtime vertex records do not contain joint indices or weights.

Validation prompt:

```text
Recook a known skinned glTF sample, inspect cooked skeleton/mesh metadata counts, launch D3D12 and Vulkan runtime smoke, and verify the asset renders through the skinned path without falling back to static-only import.
```

## Stage 4.5: Static/Skeletal Mesh Asset Split

Status: Implemented. Cooked mesh assets carry an explicit static/skeletal kind, scene manifests preserve that kind on mesh references, `SceneAssetPayload` separates static and skeletal mesh assets/instances, GameFramework creates static vs skeletal mesh components, and renderer draw classification receives an explicit scene mesh kind.

Goal: Make Static Mesh and Skeletal Mesh separate asset/runtime concepts, similar to Unreal Engine's high-level distinction, while preserving Sparkle's existing cook/load boundaries.

Why before animation: Animation playback and renderer skinning become much easier to reason about when the engine can ask "is this a skeletal mesh?" from the asset/component type instead of inferring it from optional skin fields. This also keeps static mesh memory layout, editor presentation, diagnostics, batching, and renderer paths clean for non-skinned assets.

Implementation prompt:

```text
Split static and skeletal mesh concepts without leaking source import details.

Introduce explicit cooked skeletal mesh records/assets or a versioned mesh asset kind that cleanly separates static mesh payloads from skeletal mesh payloads. Add GameFramework runtime types/components that distinguish static mesh instances from skeletal mesh instances while sharing only reusable low-level geometry/buffer helpers. SceneAssetPayload should carry static mesh and skeletal mesh entries explicitly. SceneManifestLoader and SceneAssetManager should bind skeletal mesh instances to skeleton refs through normal cooked metadata, not importer internals.

Keep SourceImportAdapters responsible only for detecting whether imported geometry should become a static mesh or skeletal mesh. Keep MeshCooker or a dedicated SkeletalMeshCooker responsible for skeletal mesh cooked payloads. Keep Renderer classification explicit: static draws and skeletal draws may share common submission helpers, but they should use the cooked/runtime mesh kind rather than optional skin fields in a generic mesh to decide the whole path.
```

Acceptance criteria:

- Static meshes and skeletal meshes have explicit asset identity in cooked data, runtime payloads, editor labels, and diagnostics.
- Non-skinned assets use the static mesh path and do not carry skin influence streams, skeleton refs, or skeletal mesh flags.
- Skinned assets use the skeletal mesh path and require a valid skeleton binding, validated joint indices/weights, and cooked skeleton metadata.
- GameFramework scene ownership distinguishes static mesh instances from skeletal mesh instances without introducing imported-vs-authored behavior branches.
- Renderer draw classification can identify static vs skeletal work without inspecting source importer data.
- Existing Stage 4 CesiumMan validation still passes after the split, and existing static samples recook/load without skeletal metadata.

Validation prompt:

```text
Recook Showcase, inspect one static sample and one skeletal sample. Verify static mesh cooked assets contain no skin/skeleton payload, CesiumMan is classified as a skeletal mesh with one skeleton binding, and D3D12 runtime smoke loads both paths. Run Vulkan smoke as far as the existing texture path allows and verify scene payload classification happens before any renderer texture failure.
```

Current validation evidence:

- CesiumMan cooked mesh `A5E88B024417D647.smsh` uses mesh asset version `4`, `AssetKind=1`, `SkinInfluenceCount=3273`, and `Flags=1`.
- DamagedHelmet cooked mesh `DAFD3D6CF4F664E7.smsh` uses mesh asset version `4`, `AssetKind=0`, `SkinInfluenceCount=0`, and `Flags=0`.
- CesiumMan scene manifest version `9` has `FirstMeshRefKind=1`, `SkeletonRefs=1`, `AnimationRefs=1`, and `FirstInstanceSkeletonRefIndex=0`.
- DamagedHelmet scene manifest version `9` has `FirstMeshRefKind=0`, `SkeletonRefs=0`, and no instance skeleton ref.
- D3D12 smoke passes for CesiumMan and DamagedHelmet with scene payload classification before renderer submission.
- Vulkan smoke passes for CesiumMan after the Vulkan default texture/material path fix.

## Stage 5: Animation Clip Import and Cooking

Status: Implemented for cooked TRS and morph-weight animation clips.

Goal: Import glTF animation clips into cooked animation assets while keeping playback disabled until the runtime pose system exists.

Why before playback: It isolates source parsing, interpolation, channel targeting, and cooked asset format from runtime update behavior.

Implementation prompt:

```text
Implement glTF animation clip import and cooked animation assets.

Add ImportedAnimationClip, ImportedAnimationSampler, and ImportedAnimationChannel data for translation, rotation, scale, and morph weights. Parse cgltf animations into normalized clip data. Cook clips into versioned animation assets referenced by cooked scene metadata. Runtime should load animation assets and expose clip metadata, duration, channel count, target skeleton, and sampling mode without applying poses yet.
```

Acceptance criteria:

- glTF animations are parsed into named clips with stable durations and channel counts.
- Translation, rotation, and scale channels preserve keyframe times and interpolation modes required for playback.
- Cooked animation assets are versioned, loadable, and referenced from the cooked scene metadata.
- Runtime can enumerate loaded clips for a scene without source glTF access.
- The old `animations are present and will be ignored` warning is replaced by an imported-but-not-played diagnostic until Stage 6 lands.

Validation prompt:

```text
Recook a scene with at least one glTF animation, verify cooked animation assets and manifest references are generated, then run a small runtime/editor diagnostic that lists clip names, durations, and channel counts.
```

Current validation evidence:

- `CesiumMan/CesiumMan.gltf` recook reports `animations=1/imported`.
- `SkeletalMorphTriangle/SkeletalMorphTriangle.gltf` recook reports `animations=1/imported` with a weight channel targeting the skeletal mesh node.
- CesiumMan and SkeletalMorphTriangle cooked scene manifests use version `9` and carry animation refs.
- GameFramework scene load logs enumerate one loaded animation clip from cooked scene payload data.

## Stage 6: Runtime Animation Playback

Status: Implemented for cooked TRS skeletal animation playback and skeletal morph-weight playback. GameFramework owns playback state, pose evaluation, and mutable skeletal morph weights; Renderer consumes immutable pose/deformable mesh data and uploads per-frame joint palettes for skeletal mesh draws.

Goal: Evaluate cooked animation clips into skeleton poses at runtime.

Why this is harder: It introduces time state, clip selection, pose blending policy, skeleton ownership, level switching behavior, and editor/runtime controls.

Implementation prompt:

```text
Implement runtime skeletal animation playback using cooked animation clips.

Add GameFramework animation state owned by scene entities or mesh instances, with explicit clip selection, playback time, looping, speed, and pause state. Evaluate TRS channels into local joint poses, combine with hierarchy into model-space joint transforms, and expose immutable/frame-local animation snapshots to Renderer. Keep Renderer from owning clip playback state. Add editor/runtime diagnostics to show active clip and pose counts.
```

Acceptance criteria:

- A skinned sample animates from cooked clips without source glTF access.
- Animation state survives level load and resets cleanly on level switch.
- Renderer receives pose snapshots, not mutable GameFramework objects.
- D3D12 and Vulkan render the same animated pose sequence within expected tolerance.
- `animations are present and will be ignored` is removed for scenes with supported TRS animation playback.
- Logs clearly identify unsupported animation channels rather than claiming the whole animation is ignored.

Validation prompt:

```text
Launch runtime smoke on D3D12 and Vulkan with level switching enabled and an animated skinned scene in the switch list. Verify animation starts after load, advances over frames, and resets cleanly when switching away and back.
```

## Stage 7: Renderer Skinning Path

Status: Baseline implemented by Stage 6. The renderer now has explicit skin influence and joint matrix shader bindings, a per-frame skinning buffer, and skeletal draw classification. Remaining Stage 7 work is scalability/backend polish, not missing import support and not moving animation state into Renderer.

Goal: Move from the current per-frame joint-palette upload path to a scalable renderer-owned skinning submission path.

Why hardest in the core chain: It crosses Renderer, RHI, shader layouts, mesh buffers, FrameGraph scheduling, and backend parity.

Implementation prompt:

```text
Extend the renderer skinning path for animated skeletal meshes.

Preserve the Stage 6 boundary: animation evaluation stays in GameFramework and Renderer receives immutable pose data through scene snapshots. Build on the existing skinned mesh GPU buffers, pose palette upload, shader layout metadata, D3D12/Vulkan binding metadata, and skeletal draw classification. Add batching/scalability improvements or compute skinning through FrameGraph if profiling justifies it. Do not add backend-specific shortcuts in Renderer.
```

Acceptance criteria:

- Renderer has an explicit skinned mesh path separate from static mesh drawing.
- Pose data is uploaded or consumed through RHI-neutral resource descriptions.
- D3D12 and Vulkan use equivalent shader layouts and binding metadata.
- FrameGraph owns compute skinning barriers/resources if compute skinning is used.
- Runtime smoke renders an animated skinned mesh on both backends with no backend-only fallback.
- Validation includes source gates for renderer/RHI boundary ownership.

Validation prompt:

```text
Build ShowcaseRuntime and ShowcaseEditor, force recook animated assets, run D3D12 and Vulkan smoke with animated skinned content, then inspect logs for missing shader layouts, RHI binding errors, and backend divergence.
```

## Stage 8: Renderer Point and Spot Light Shading

Status: Implemented in the renderer lighting path with D3D12 and Vulkan validation. Vulkan default texture upload, immutable sampler binding, and descriptor-set submission now validate the lighting path beyond cooked payload classification.

Goal: Consume already-imported point and spot lights in Renderer so all punctual light types affect shading.

Why after the scene metadata path is proven: Import, cook, load, GameFramework ownership, and editor presentation already carry point and spot lights. The remaining work crosses Renderer, RHI-facing constant/storage layouts, light culling policy, shading changes, and backend parity.

Implementation prompt:

```text
Implement renderer shading support for scene point and spot lights.

Extend GameFramework lighting snapshots to include point and spot lights from existing SceneLightDesc data. Update renderer scene data, lighting passes, shaders, and RHI-visible layouts to consume point and spot light data. Preserve imported glTF range and spot cone angles already stored in cooked scene light records. Keep light ownership in GameFramework and render data extraction in snapshot/build steps; do not move playback or ownership state into Renderer.
```

Acceptance criteria:

- Imported point and spot lights affect rendering on D3D12 and Vulkan.
- Range, color, intensity, world transform/direction, and spot cone angles are preserved through import, cook, load, snapshot, and renderer consumption.
- Existing directional light behavior is unchanged.
- Logs no longer report loaded punctual lights as ignored.
- Editor scene inspector continues to display distinct light kinds without renderer-specific stand-ins.

Validation prompt:

```text
Recook a scene with directional, point, and spot glTF lights. Launch editor and runtime smoke on D3D12 and Vulkan and verify each light type contributes visibly or through deterministic renderer diagnostics.
```

Current validation evidence:

- `CameraCube.gltf` carries imported directional, point, and spot lights through `KHR_lights_punctual`; recook reports `lightNodes=3/imported` and `cookedLights=3`.
- GameFramework lighting snapshots carry directional, point, and spot records without importer-specific runtime behavior.
- Renderer scene data and per-view lighting constants carry point and spot lights through RHI-neutral layouts shared by D3D12 and Vulkan shader packages.
- DirectLighting HLSL consumes directional, point, and spot arrays from the same per-view lighting block.
- D3D12 runtime smoke on `CameraCubeImportedCamera` reaches frame 40 and logs `RenderLightingBuilder: prepared scene lighting (directionalLights=2, pointLights=1, spotLights=1)`.
- Vulkan runtime smoke now validates the same cooked lighting path after the Vulkan default texture/material null path fix.

## Stage 9: Morph Targets and Weighted Nodes

Status: Implemented for skeletal/deformable meshes. Static mesh data remains plain geometry and static mesh instances do not carry morph streams or morph weights. Skeletal glTF morph target deltas import into cooked skeletal mesh morph streams, weighted nodes initialize runtime morph state, and animation channels targeting weights drive mutable skeletal morph weights.

Goal: Address morph target and weighted-node warnings after the animation/skinning data model exists.

Why late: Morph targets overlap animation, mesh vertex streams, runtime pose/evaluation state, shader layouts, and memory pressure.

Implementation prompt:

```text
Extend glTF morph target import into full runtime weight playback.

Preserve the static/skeletal split. SourceImportAdapters may parse morph target deltas and node/default weights into imported deformation data, but MeshCooker writes morph streams only for skeletal mesh assets. SceneCooker stores per-instance morph weights only for skeletal instances. GameFramework applies default skeletal morph weights before Renderer sees mesh data. Static meshes must not carry morph target streams, morph weights, deformation state, or renderer classification flags. Add runtime mutable morph weight state and animation channel evaluation for skeletal weights. If a GPU or compute morph path replaces CPU default application, route it through RHI-neutral skeletal mesh/FrameGraph data rather than backend-specific renderer branches.
```

Acceptance criteria:

- Skeletal morph target streams are imported, cooked, loaded, and bound to the correct skeletal mesh primitives.
- Skeletal weighted nodes initialize runtime morph weights from glTF defaults.
- Animation channels targeting weights can drive morph weights if Stage 6 animation playback exists.
- Renderer path works on D3D12 and Vulkan without backend-specific renderer branches.
- Static meshes stay on the static path and do not cook/load morph streams.
- Static glTF primitives with morph targets remain outside the active supported Showcase baseline; supported morph playback is intentionally owned by the skeletal/deformable path.

Validation prompt:

```text
Use a glTF sample with visible morph targets, recook, run D3D12 and Vulkan smoke, and verify default and animated weights produce expected mesh deformation without warnings about ignored weighted nodes.
```

Current validation evidence:

- `Projects/Showcase/Assets/Meshes/SkeletalMorphTriangle/SkeletalMorphTriangle.gltf` is the Stage 9 validation fixture: one skeletal triangle, one joint, one morph target, default node weight state, and one animated weight channel.
- `SkeletalMorphTriangle.level` loads that fixture through the normal cooked scene asset path.
- Showcase recook reports `SkeletalMorphTriangle/SkeletalMorphTriangle.gltf` with `animations=1/imported`, `skinnedNodes=1/imported`, `weightedNodes=1/imported`, `morphTargets=1/imported`, and `warnings=0`; no ignored morph/weighted-node warnings appear.
- Showcase recook writes scene manifest version `9`, preserving source node identity for mesh instances so animation weight channels can target skeletal/deformable mesh components deterministically.
- Runtime has mutable skeletal morph weight state on `SkeletalCookedMesh`; animation weight channels produce node-targeted morph weight snapshots and `SceneMeshes` applies them only to skeletal components.
- D3D12 runtime smoke on `SkeletalMorphTriangle` reaches frame limit 45 and logs active playback with `poses=1`, `morphWeights=1`, and animated `firstMorphWeight` values.
- Vulkan runtime smoke on `SkeletalMorphTriangle` reaches frame limit 45 and logs the same skeletal morph playback path.
- Renderer remains importer-agnostic: changed CPU mesh geometry is detected through mesh geometry revisions and `GPUMeshCache` reuploads revised meshes without adding morph-specific renderer branches.
- Static meshes stay on the static path and do not cook/load morph streams.

## Stage 10: Material Variants and Mesh Instancing

Status: Implemented with D3D12 and Vulkan validation. Material variants import/cook/load as scene metadata and are selectable through `SceneMaterialVariants` and the editor Inspector `Variants` tab. Authored mesh GPU instancing preserves imported instance groups through cook/load and renderer batching. Vulkan default texture upload, sampler binding, and descriptor-set submission now validate these scenes beyond cooked payload classification.

Goal: Finish the remaining common glTF feature warnings after core scene, lighting, and animation content are functional.

Why last: These are useful but less foundational than cameras, lights, skeletons, and animation. Variants require material selection policy; GPU instancing requires renderer batching policy.

Implementation prompt:

```text
Implement glTF material variants and mesh GPU instancing as opt-in scene features.

For material variants, import variant sets and primitive mappings, cook stable variant metadata, and add runtime/editor selection. For mesh GPU instancing, preserve instance transforms and shared mesh/material references instead of flattening every instance into independent mesh instances. Keep batching decisions in Renderer and keep source importer output backend-neutral.
```

Acceptance criteria:

- Material variants can be enumerated and selected without recooking.
- Variant selection changes material bindings deterministically at runtime/editor level.
- GPU-instanced glTF nodes preserve shared geometry identity and instance transforms through cook/load.
- Renderer can batch or diagnose imported instance groups without changing source importer ownership.
- Existing scenes without variants or instancing remain unchanged.

Validation prompt:

```text
Recook variant and instancing samples, launch editor/runtime smoke on D3D12 and Vulkan, switch variants at runtime, and verify instance counts and material bindings match imported metadata.
```

Current validation evidence:

- `VariantTriangle/VariantTriangle.gltf` recook reports `materialVariants=2/imported`, `materialVariants=1`, `materialVariantMappings=1`, `cookedMaterialVariants=1`, and `cookedVariantMappings=1`.
- `VariantTriangle.sscn` uses manifest version `9`, `variants=1`, `mappings=1`, and feature flags `0x00000040`.
- Vulkan runtime smoke on `VariantTriangle` exits successfully after loading `materialVariants=1`, `variantMappings=1`, feature flags `0x00000040`, and default textures.
- D3D12 editor smoke on `VariantTriangle` loads and renders the scene with viewport evidence, reaches frame limit 30, and exits cleanly after `RuntimeApplication` shutdown.
- `Instancing/GpuInstancedCube.gltf` recook reports `meshGpuInstancing=1/imported`, `importedMeshInstanceGroups=1`, and `cookedInstanceGroups=1`.
- `GpuInstancedCube.sscn` uses manifest version `9`, `instances=3`, `groups=1`, and feature flags `0x00000080`.
- Vulkan runtime smoke on `GpuInstancedCube` exits successfully after loading `meshInstances=3`, `instanceGroups=1`, feature flags `0x00000080`, and default textures.

## Remaining Import-System Gaps

Stage 0 through Stage 10 are now implemented except where explicitly called out below. Future work should preserve the static/skeletal mesh split, keep import/cook/runtime/renderer boundaries separate, and avoid routing feature-specific state through generic static mesh data.

- Decide which advanced glTF material extensions Sparkle should care about next. Current importer diagnostics still report unsupported approximation for extensions such as clearcoat, transmission, volume, sheen, specular, iridescence, anisotropy, dispersion, unlit, and specular-glossiness.
- Keep Draco-compressed primitives, non-triangle primitive modes, unsupported embedded/encoded texture sources, and FBX-only feature gaps explicit as unsupported diagnostics until they receive dedicated feature work.

Suggested next implementation prompt:

```text
Triage advanced glTF material extension support.

Choose the next material extension family Sparkle should support beyond the current PBR baseline. Keep unsupported material extensions explicit as approximation diagnostics until import, cook, GameFramework material ownership, renderer shading, and D3D12/Vulkan validation all exist for the selected feature.
```


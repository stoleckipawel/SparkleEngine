# glTF Feature Warning Roadmap

This plan turns the current glTF importer warnings into staged feature work. The order is intentionally easiest to hardest: first make importer capabilities explicit, then wire features that already have engine runtime concepts, then build the animation and skinning foundation that needs new cooked formats and renderer support.

## Current Warning Map

| Warning | Current owner | Missing feature | First functional owner |
| --- | --- | --- | --- |
| `animations are present and will be ignored` | `Tools/SourceImportAdapters/Private/Gltf/GltfSceneReader.cpp` | Animation clip import, cooking, runtime playback | GameFramework animation system, with Renderer consuming evaluated skinning data |
| `nodes contain cameras and they will be ignored` | `Tools/SourceImportAdapters/Private/Gltf/GltfSceneReader.cpp` | Camera metadata import and level camera handoff | SourceImportAdapters -> SceneCooker -> GameFramework level camera |
| `nodes contain lights and they will be ignored` | `Tools/SourceImportAdapters/Private/Gltf/GltfSceneReader.cpp` | glTF light import and scene/level lighting handoff | SourceImportAdapters -> SceneCooker -> GameFramework lighting |
| `skinned nodes are present and will be imported as static data only` | `Tools/SourceImportAdapters/Private/Gltf/GltfSceneReader.cpp` and `GltfGeometryImporter.cpp` | Skeletons, joint weights, inverse bind matrices, animation pose evaluation, skinning path | SourceImportAdapters -> cooked skeleton/mesh data -> GameFramework animation -> Renderer skinning |

Related warnings to keep in the same backlog, but not let block the four warnings above: material variants, morph targets, weighted nodes, and mesh GPU instancing.

## Shared Rules

- Remove or downgrade a warning only when the feature is functionally consumed by the next engine stage. Importing unused metadata is not enough.
- Keep SourceImportAdapters responsible only for source-format extraction and normalization into `SourceImportResult` data.
- Keep SceneCooker responsible for stable cooked scene manifests and asset references.
- Keep GameFramework responsible for runtime scene, level, camera, light, animation, and skeleton state.
- Keep Renderer responsible for render-side snapshots, GPU resources, skinning dispatch/draw setup, and shader-facing layouts.
- Preserve runtime cooked-only loading. Do not add runtime glTF parsing.
- Prefer versioned cooked asset/manifest changes over sidecar compatibility shims.

## Stage 0: Import Feature Diagnostics

Goal: Make unsupported glTF content visible as structured import diagnostics so later stages can prove when a warning is still intentional and when it is resolved.

Why first: This is the smallest stage and gives every later stage a precise acceptance gate.

Implementation prompt:

```text
Add structured glTF feature diagnostics for cameras, lights, animations, skins, morph targets, material variants, and mesh instancing.

Keep the extraction in Tools/SourceImportAdapters. Extend SourceImportResult with a compact diagnostics/capability summary that records feature counts and whether each feature is imported, partially imported, or unsupported. GltfSceneReader should populate this summary while preserving current log messages. AssetCooker and AssetConverter should print the summary after import. Do not change runtime behavior yet.
```

Acceptance criteria:

- `SourceImportResult` exposes per-feature counts for animations, camera nodes, light nodes, skinned nodes, weighted nodes, morph targets, material variants, and mesh GPU instancing.
- Existing warning logs still appear for unsupported features.
- AssetCooker and AssetConverter logs include one concise import feature summary per scene.
- No runtime code depends on source importer internals.
- Validation command: `cmake --build build --config DevelopmentEditor --target AssetCooker AssetConverter -- /nologo /v:minimal /m:1 /p:UseMultiToolTask=false /p:TrackFileAccess=false /nodeReuse:false`.

## Stage 1: glTF Camera Import

Goal: Convert glTF camera nodes into a usable initial level camera or selectable imported camera metadata.

Why this is easiest: Sparkle already has `CameraDesc`, `SceneCamera`, `LevelDesc::cameraDesc`, level parsing, and runtime camera application. The missing piece is the source-to-cooked bridge.

Implementation prompt:

```text
Implement glTF camera import as a functional feature.

Add ImportedCamera data to SourceImportResult with name, world transform, projection kind, vertical FOV, near/far planes when available, and source node index. GltfSceneReader or a small GltfCameraImporter should extract camera nodes from cgltf. SceneCooker should persist the primary imported camera into the cooked scene or a scene metadata record. Level loading should be able to use that camera as the default view when a level references the cooked scene and does not override camera settings.

Keep SourceImportAdapters format-focused. Keep GameFramework responsible for turning imported camera metadata into CameraDesc. Do not make Renderer or RHI aware of glTF cameras.
```

Acceptance criteria:

- glTF camera nodes no longer produce an ignored-camera warning when camera metadata is imported and consumed.
- At least one imported camera can seed `LevelDesc::cameraDesc` or an equivalent scene camera handoff.
- Perspective cameras map FOV and transform correctly enough to frame known sample content.
- Orthographic cameras are either supported or explicitly reported as imported metadata but not selected for runtime view.
- The feature handles multiple cameras deterministically, using a documented primary-camera policy.
- A recooked sample scene with glTF cameras launches with the imported camera on D3D12 and Vulkan.

Validation prompt:

```text
Force recook Showcase, launch runtime smoke with a glTF-camera scene on D3D12 and Vulkan, and verify logs show imported camera consumption with no ignored-camera warning for that scene.
```

## Stage 2: glTF Light Import

Goal: Convert glTF lights into functional scene lighting where Sparkle already has matching runtime light concepts.

Why second: Directional lighting already exists in GameFramework and Renderer. Point and spot lights may require a renderer feature decision, so directional lights should land first while unsupported light types remain explicit.

Implementation prompt:

```text
Implement glTF KHR_lights_punctual import with functional directional light support first.

Add ImportedLight data to SourceImportResult with type, name, world transform, direction, color, intensity, range, cone angles, and source node index. Parse glTF node lights from cgltf. Map directional lights to DirectionalLightDesc and feed them into LevelLightingDesc or cooked scene lighting metadata. Preserve point and spot lights as imported metadata if the renderer cannot consume them yet, but keep warnings specific: unsupported point/spot runtime lighting rather than all lights ignored.
```

Acceptance criteria:

- Directional glTF lights are imported and affect the rendered scene.
- `nodes contain lights and they will be ignored` is replaced by type-specific diagnostics only for light types not yet consumed.
- Direction, color, intensity, and enabled/default visibility survive import, cook, load, and renderer snapshot capture.
- Multiple directional lights obey `LevelLightingDesc::MaxDirectionalLights` with deterministic truncation diagnostics.
- D3D12 and Vulkan smoke renders show the same imported lighting path.

Validation prompt:

```text
Recook a scene with KHR_lights_punctual directional lights, launch runtime/editor smoke on D3D12 and Vulkan, and verify imported directional lights appear in GameFramework lighting snapshots and renderer lighting input.
```

## Stage 3: Scene Metadata Manifest Versioning

Goal: Create the durable cooked scene metadata path needed by cameras, lights, skeletons, and animations.

Why here: Stage 1 and 2 can start narrowly, but animation and skinning should not be built on ad hoc sidecars. The cooked scene manifest currently stores mesh refs, material refs, and instances only.

Implementation prompt:

```text
Version the cooked scene manifest to carry optional scene metadata blocks.

Extend CookedSceneManifest with a versioned metadata section for cameras, lights, skeleton refs, animation refs, and feature flags. Update SceneCooker writer and SceneManifestLoader reader together. Keep the runtime cooked-only. Add validation that old assumptions about mesh/material/instance ordering still hold, and make new metadata optional for scenes without these features.
```

Acceptance criteria:

- Cooked scene manifest version increments and load validation rejects mismatched versions with a clear error.
- SceneManifestLoader can read camera and light metadata without touching source glTF files.
- SceneAssetManager applies available scene metadata through GameFramework owners.
- Scenes without metadata remain valid after recook under the new manifest version.
- Source validation or a focused cooker test proves metadata counts in the manifest match imported counts.

Validation prompt:

```text
Force recook Showcase, verify all .sscn manifests use the new version, then launch D3D12 and Vulkan runtime smoke to prove old mesh/material/instance loading still works.
```

## Stage 4: Skeleton and Skin Data Import

Goal: Stop treating skinned nodes as ordinary static meshes by importing skeletons, joints, inverse bind matrices, and vertex joint weights.

Why this is harder: It changes mesh vertex payloads, cooked mesh format, asset IDs, runtime scene state, and renderer input layouts.

Implementation prompt:

```text
Implement glTF skeleton and skin data import without animation playback yet.

Extend SourceImportResult with ImportedSkeleton, ImportedJoint, ImportedSkinBinding, and skinned mesh vertex influences. GltfGeometryImporter should read JOINTS_0 and WEIGHTS_0, normalize weights, and associate mesh instances with a skin. Add cooked skeleton assets or scene metadata references. Update MeshCooker and cooked mesh loading to preserve skin influence streams. Runtime should load skeleton bind pose and expose a neutral pose for skinned meshes.

Do not implement animation playback in this stage. The acceptance target is correct bind-pose skinned rendering or a clearly equivalent neutral-pose path, not moving characters.
```

Acceptance criteria:

- Skinned glTF nodes import skeleton and skin binding data instead of only static mesh data.
- Vertex joint indices and weights are validated, normalized, cooked, loaded, and visible to renderer-side mesh setup.
- Inverse bind matrices are preserved and associated with the correct joints.
- The runtime can render skinned assets in bind pose or neutral pose through a distinct skinned mesh path.
- The old `skinned nodes are present and will be imported as static data only` warning is removed only for assets that successfully load through the skinned path.
- Static mesh import remains unchanged for non-skinned assets.

Validation prompt:

```text
Recook a known skinned glTF sample, inspect cooked skeleton/mesh metadata counts, launch D3D12 and Vulkan runtime smoke, and verify the asset renders through the skinned path without falling back to static-only import.
```

## Stage 5: Animation Clip Import and Cooking

Goal: Import glTF animation clips into cooked animation assets while keeping playback disabled until the runtime pose system exists.

Why before playback: It isolates source parsing, interpolation, channel targeting, and cooked asset format from runtime update behavior.

Implementation prompt:

```text
Implement glTF animation clip import and cooked animation assets.

Add ImportedAnimationClip, ImportedAnimationSampler, and ImportedAnimationChannel data for translation, rotation, scale, and weights if morph targets are still deferred. Parse cgltf animations into normalized clip data. Cook clips into versioned animation assets referenced by cooked scene metadata. Runtime should load animation assets and expose clip metadata, duration, channel count, target skeleton, and sampling mode without applying poses yet.
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

## Stage 6: Runtime Animation Playback

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

Goal: Move from CPU/neutral-pose readiness to a scalable renderer-owned skinning path.

Why hardest in the core chain: It crosses Renderer, RHI, shader layouts, mesh buffers, FrameGraph scheduling, and backend parity.

Implementation prompt:

```text
Implement the renderer skinning path for animated skeletal meshes.

Add skinned mesh GPU buffers and pose palette upload or compute skinning according to the renderer architecture. Keep animation evaluation in GameFramework and pass immutable pose data through renderer snapshots. Add shader layout metadata for skinning resources, update D3D12 and Vulkan binding paths, and schedule any compute skinning pass through FrameGraph. Do not add backend-specific shortcuts in Renderer.
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

## Stage 8: Point and Spot Lights

Goal: Complete remaining glTF punctual light types after the scene metadata path is proven.

Why after skinning foundation starts: Directional lights are the easy lighting win. Point and spot lights likely require new renderer light lists, culling, shading changes, and editor controls.

Implementation prompt:

```text
Implement point and spot light support for imported glTF lights.

Extend GameFramework lighting descriptors beyond directional lights, add renderer snapshots for punctual lights, and update lighting passes/shaders to consume point and spot light data. Import glTF range and spot cone angles. Keep light ownership in GameFramework and render data extraction in Renderer.
```

Acceptance criteria:

- Imported point and spot lights affect rendering on D3D12 and Vulkan.
- Range, color, intensity, and spot cone angles are preserved through import, cook, load, snapshot, and renderer consumption.
- Existing directional light behavior is unchanged.
- Logs no longer report supported punctual lights as ignored.
- Editor scene inspector can display or at least diagnose imported punctual light counts.

Validation prompt:

```text
Recook a scene with directional, point, and spot glTF lights. Launch editor and runtime smoke on D3D12 and Vulkan and verify each light type contributes visibly or through deterministic renderer diagnostics.
```

## Stage 9: Morph Targets and Weighted Nodes

Goal: Address morph target and weighted-node warnings after the animation/skinning data model exists.

Why late: Morph targets overlap animation, mesh vertex streams, runtime pose/evaluation state, shader layouts, and memory pressure.

Implementation prompt:

```text
Implement glTF morph target import and runtime weights.

Import morph target deltas for positions, normals, and tangents where available. Cook morph target streams as versioned mesh data. Add runtime morph weight state and animation channel evaluation for weights. Update Renderer to blend morph targets either CPU-side, vertex-shader-side, or compute-side through FrameGraph according to the selected architecture.
```

Acceptance criteria:

- Morph target streams are imported, cooked, loaded, and bound to the correct mesh primitives.
- Weighted nodes initialize runtime morph weights from glTF defaults.
- Animation channels targeting weights can drive morph weights if Stage 6 animation playback exists.
- Renderer path works on D3D12 and Vulkan without backend-specific renderer branches.
- Existing static and skinned meshes without morph targets keep their old memory/layout path.

Validation prompt:

```text
Use a glTF sample with visible morph targets, recook, run D3D12 and Vulkan smoke, and verify default and animated weights produce expected mesh deformation without warnings about ignored weighted nodes.
```

## Stage 10: Material Variants and Mesh Instancing

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

## Recommended First Slice

Start with Stage 0, then Stage 1. That gives immediate value without committing to animation architecture too early. Stage 1 also exercises the full source-import -> cook -> level-load -> runtime-consume chain that later stages need.

Suggested first implementation prompt:

```text
Implement Stage 0 from docs/plans/gltf-feature-warning-roadmap.md.

Add structured glTF feature diagnostics to SourceImportResult and populate them from GltfSceneReader/GltfGeometryImporter. Print a concise summary from AssetCooker and AssetConverter. Preserve existing warnings. Do not change runtime behavior. Acceptance criteria are the Stage 0 checklist in the document, and validation is the DevelopmentEditor AssetCooker/AssetConverter build plus a Showcase force recook log showing the summary.
```

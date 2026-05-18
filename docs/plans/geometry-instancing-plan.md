# Geometry Instancing Plan

Date: 2026-05-18

## Goal

Introduce geometry instancing through the practical engine tiers we want to ship now: renderer auto-batching, import-preserved shared geometry, authored glTF instance groups, and a structured-buffer shader path. The implementation should preserve instancing information from source import through cook, load, GameFramework snapshot, Renderer scene data, and finally D3D12/Vulkan draw submission.

The primary user-facing acceptance signal is editor diagnostics that show instance batches. The visual scene should remain unchanged while repeated geometry is represented and drawn as batches instead of only as independent mesh draws. Later tiers such as multi-pass expansion, GPU-driven culling, indirect draws, and meshlet-level systems are intentionally out of scope for this plan.

## Current State

- `Tools/SourceImportAdapters/Public/SourceImportResult.h` now separates imported mesh primitives from imported mesh instances. Geometry identity is source mesh/primitive based, while placements carry primitive index, material index, world transform, and source node identity.
- `Tools/SourceImportAdapters/Private/Gltf/GltfGeometryImporter.cpp` now reuses one imported primitive for repeated references to the same glTF mesh primitive and emits separate imported instances for each placement.
- `Tools/SceneCooker/Private/SceneCooker.cpp` writes one `CookedSceneInstanceRecord` for each imported mesh instance, using the imported primitive index as the cooked mesh asset reference.
- `Engine/GameFramework/Public/Assets/SceneAssetPayload.h` loads scene payloads as a flat list of mesh instances, each carrying `MeshData` directly.
- `Engine/GameFramework/Public/Scene/Meshes/MeshSnapshot.h` exposes a flat `meshInstances` vector to Renderer.
- `Engine/Renderer/Public/SceneData/MeshDraw.h` and `RenderSceneDataBuilder` keep the flat model and resolve each snapshot entry into one `MeshDraw`.
- `Engine/Renderer/Private/Passes/GBufferPass.cpp` already calls `DrawIndexedInstanced`, but every draw currently uses `instanceCount = 1` and binds per-object constants per draw.
- `Engine/RHI/Public/Commands/RenderCommandList.h` already exposes instanced draw calls. D3D12 forwards to `DrawIndexedInstanced`, and Vulkan forwards to `vkCmdDrawIndexed`, so the immediate RHI draw API is not the main blocker.

## Active Scope

- Tier 1: Renderer auto-batching for compatible flat mesh instances.
- Tier 2: Import-preserved shared geometry, so repeated source primitives do not become duplicated mesh assets.
- Tier 3: Authored instance groups, especially glTF `EXT_mesh_gpu_instancing`.
- Tier 4: Renderer instance-buffer contract, shader binding, and `DrawIndexedInstanced` submission with `instanceCount > 1`.

Out of scope for this plan:

- Full multi-pass instancing beyond opaque GBuffer validation.
- GPU-driven culling, indirect draw compaction, meshlet/cluster rendering, or bindless material systems.
- Manual editor placement workflows.
- Material variants, skinning, and morph target batching beyond conservative batch-key exclusions.

## Product Decisions From Planning

- Prioritize import pipeline data over manual editor placement.
- Preserve authored instance information where available, especially glTF instancing metadata.
- Use a structured buffer indexed by `SV_InstanceID` or equivalent shader input, not per-instance vertex input as the first shader contract.
- Keep D3D12 and Vulkan in the first milestone design. Implementation can still be sliced, but the data contract should be backend-neutral from the start.
- Add editor diagnostics for instance batches as the first visible success signal.
- Add renderer-side duplicate detection as Tier 1, but treat it as a fallback. The primary source of truth for authored instancing is the import/cook pipeline.

## Production Architecture Principles

- Follow the NVRHI/NVIDIA Donut and AMD Cauldron pattern of separating mesh geometry, material state, instance data, draw batches, and backend command recording.
- Importers preserve facts from the source asset; they do not decide GPU batching policy.
- Cooked scene data preserves stable mesh primitive identity and instance placement separately.
- GameFramework owns loaded scene objects and snapshots; Renderer owns batching, GPU buffer lifetime, descriptor/binding setup, and draw submission.
- RHI stays backend-neutral. Do not introduce D3D12/Vulkan-specific instancing concepts into Renderer public data.
- Keep instance buffers transient or frame-owned, with deterministic lifetime and no raw backend resource ownership leaking into GameFramework or tools.
- Keep batch building deterministic: stable input order, stable group ids, explicit sort/group keys, and no hash-order-dependent draw ordering unless the renderer already accepts it.
- Keep shader-facing data tightly specified and reflected through the existing shader package pipeline. Do not hand-bind backend registers outside the parameter layout system.
- Existing non-instanced scenes should keep loading and rendering unchanged.
- Future features such as material variants, skinning, morph targets, render layers, and visibility should be explicit batch keys, not accidental reasons for incorrect batching.

## Reference Architecture Translation

Use NVRHI/Donut and Cauldron as architectural references, not as names to copy wholesale. The important shape is a layered data flow: source scene facts -> loaded scene resources -> renderer render items -> renderer draw batches -> pass-local bindings -> backend command list. Sparkle should keep its existing module names and public/private boundaries while adopting that separation.

Recommended Sparkle naming and placement:

- Source import layer: `Tools/SourceImportAdapters/Public/SourceImportResult.h` owns source-facing records such as `ImportedMeshPrimitive`, `ImportedMeshInstance`, `ImportedMeshInstanceGroup`, and `ImportedInstanceSourceFeature`. These are tool/import concepts only.
- Source import diagnostics: `Tools/SourceImportAdapters/Public/SourceImportDiagnostics.h` owns the unified import diagnostics root. Specialized import diagnostics, such as geometry instancing counts, live as subcomponents under `SourceImportDiagnostics`; importer-specific collectors, recorders, and diagnostic log reporters live under `Tools/SourceImportAdapters/Private/Diagnostics` and are called from the importer boundary.
- glTF extraction layer: `Tools/SourceImportAdapters/Private/Gltf/GltfGeometryImporter.*` owns cgltf-specific parsing helpers such as primitive identity discovery, node primitive instance creation, and `EXT_mesh_gpu_instancing` transform extraction. Do not expose cgltf types outside the private glTF adapter.
- Cooked scene contract: `Engine/GameFramework/Public/Assets/Cooked/CookedSceneManifest.h` owns POD records such as `CookedSceneInstanceRecord` and `CookedSceneInstanceGroupRecord`. These records describe asset indices, transform ranges, group ids, and source feature tags, not renderer batches.
- Scene cooker: `Tools/SceneCooker/Private/SceneCooker.cpp` maps imported primitives and instances to cooked manifest records. It should not know about `GPUMesh`, frame resources, shader parameters, or RHI command lists.
- Runtime payload: `Engine/GameFramework/Public/Assets/SceneAssetPayload.h` separates loaded mesh assets from mesh placements. Use names such as `SceneMeshAsset`, `MeshInstance`, and `MeshInstanceGroup` rather than renderer names like draw, batch, or GPU instance.
- Runtime snapshot: `Engine/GameFramework/Public/Scene/Meshes/MeshSnapshot.h` exposes immutable frame-local placement data. Use `MeshInstanceSnapshot` and `MeshInstanceGroupSnapshot` for renderer input. Keep material references as `MaterialHandle` and mesh references as engine mesh pointers or handles, not GPU resources.
- Renderer scene data: `Engine/Renderer/Private/SceneData/RenderSceneData.h` owns renderer-ready CPU data. Use `MeshRenderItem` for a resolved draw candidate, `MeshInstanceBatch` for a CPU batch, and `MeshInstanceData` for shader-facing per-instance data. Keep `MeshDraw` only as the singleton fallback or migrate it to the same render-item model.
- Renderer batch building: add focused private helpers under `Engine/Renderer/Private/SceneData/Builders`, for example `MeshInstanceBatchBuilder.h/.cpp`, because batching is part of converting snapshots into renderer scene data. This matches the existing `RenderSceneDataBuilder` placement.
- Renderer frame GPU data: frame-owned upload/allocation helpers belong under `Engine/Renderer/Private/Frame` or an existing renderer frame-resource owner, not in GameFramework or tools. Use names like `MeshInstanceFrameData` or `MeshInstanceBuffer` for the uploaded structured-buffer view and counts.
- Pass execution: `Engine/Renderer/Private/Passes/GBufferPass.cpp` consumes already-built batches and already-uploaded frame instance data. It binds pass/draw parameters and issues draw calls; it should not discover source instance groups or mutate scene payloads.
- RHI backend layer: D3D12 and Vulkan command-list implementations remain thin translations of `RenderCommandList::DrawIndexedInstanced` and resource binding calls. They should not receive source feature tags, material handles, or scene instance group ids.

Shape rules for implementation:

- Use `Primitive`, `Instance`, and `Group` names in import/cook/runtime data. Use `RenderItem`, `Batch`, and `InstanceData` names in renderer data. Use `Buffer` or `FrameData` names only where GPU upload/lifetime is owned.
- Keep `BatchKey` as a renderer-private value type. It should be comparable/sortable and should not leak into GameFramework or cooked assets.
- Keep grouping functions close to the owning abstraction: source grouping in `GltfGeometryImporter`, cooked record mapping in `SceneCooker`, runtime payload expansion in `SceneAssetManager`, renderer batch construction in `MeshInstanceBatchBuilder`, and draw submission in `GBufferPass`.
- Prefer small owning classes with explicit inputs/outputs over file-local anonymous helper clusters for batching. For example, `MeshInstanceBatchBuilder::Build(...)` is clearer than scattering grouping helpers inside `RenderSceneDataBuilder.cpp`.
- Do not create a generic global instancing manager. Instancing is a data path across import, scene, renderer, and pass layers; each layer owns its local representation.
- Integrate diagnostics with the existing snapshot/provider surfaces instead of adding a parallel diagnostics framework. Texture, mesh, memory, shader generation, and profiler diagnostics already use editor-facing snapshots captured on demand; geometry instancing diagnostics should extend the mesh diagnostics snapshot or a future renderer stats snapshot, not pass through `RenderSceneData` or backend command lists as ad hoc counters.
- Keep import diagnostics unified. `SourceImportResult` should expose one `diagnostics` member whose subcomponents describe geometry instancing, material import, texture source usage, animation/skinning readiness, or importer warnings as those needs appear. Do not add new one-off top-level diagnostics fields for each feature, and keep diagnostic log text/formatting out of importer, reader, material, and geometry implementation bodies.

## Proposed Data Model

### Source Import

Replace the current `ImportedMesh`-only scene model with separate concepts:

- `ImportedMeshPrimitive`: geometry, display name, source mesh index, primitive index, optional stable source identity.
- `ImportedMeshInstance`: primitive index, material index, world transform, optional source node index/name.
- `ImportedMeshInstanceGroup`: primitive index, material index, first instance/count or explicit instance indices, source feature tag such as `GltfExtMeshGpuInstancing`.

For glTF, `GltfGeometryImporter` should preserve source mesh/primitive identity. If multiple nodes reference the same glTF mesh primitive, they should point at the same imported primitive instead of duplicating geometry. If `EXT_mesh_gpu_instancing` is present, import its transform attributes into an instance group tied to the shared primitive and material binding.

### Cooked Scene Manifest

Bump `kCookedSceneManifestVersion` and split manifest sections:

- Mesh asset references: one per unique cooked mesh primitive.
- Material asset references: unchanged conceptually.
- Instance records: primitive asset index, material asset index, world transform, group index or invalid group.
- Instance group records: primitive asset index, material asset index, first instance, instance count, source feature tag, optional flags.

Keep fixed-size trivially copyable records and write sections in deterministic order. Older manifests can be rejected with a clear recook message while the engine is still small.

### Runtime Scene Payload

Change `SceneAssetPayload` so mesh data is not embedded in every placement. A useful shape is:

- loaded mesh assets: cooked mesh data plus cooked asset id.
- mesh instances: mesh asset index, transform, material handle, optional instance group id.
- mesh instance groups: mesh asset index, material handle, first instance/count, source feature tag.

`GameScene` can still create `MeshComponent` objects initially if that keeps the current scene model stable, but the payload should preserve group identity so the snapshot can expose it later. A cleaner follow-up is a scene mesh collection that owns mesh assets separately from component placements.

### Renderer Scene Data

Extend `RenderSceneData` with instanced draw data while retaining single draws for fallback:

- `MeshDraw` remains the single-instance path.
- `MeshInstanceData` stores world matrix, inverse transpose, material slot or packed material index.
- `MeshInstanceBatch` stores `GPUMesh*`, material slot, first instance, instance count, and batch flags.

`RenderSceneDataBuilder` should build batches from preserved source groups first. It may optionally add an auto-batching pass for compatible flat instances that share the same `GPUMesh`, material slot, pipeline-relevant state, and future-safe feature flags.

### GPU Data And Shader Contract

Add a per-frame structured instance buffer owned by Renderer or the pass runtime services. Each instance entry should contain:

- world matrix.
- world inverse transpose matrix.
- material slot or per-instance material data index if/when material indexing moves GPU-side.
- optional flags/reserved fields for future visibility/debug/skinning constraints.

The first shader contract should bind this buffer to the GBuffer vertex shader and index it by `SV_InstanceID + batchStartInstance` or by a push constant/root constant that carries `firstInstance`. This avoids changing vertex layouts and works across D3D12 and Vulkan with the existing shader reflection/package system.

## Batch Key

First milestone batch compatibility should require:

- same `GPUMesh`.
- same material slot and texture binding set.
- same raster pipeline and shader pass.
- same render layer/visibility state once those are represented.
- no skinning, morph targets, or per-object feature state unless that state is explicitly represented in the instance buffer and shader path.

This gives a conservative default: batch only when the renderer can prove the instances are identical except for per-instance data.

## Phased Implementation

Each phase below is intended to be usable as an implementation prompt. Keep phases source-compatible within the phase when possible, but prefer clean format/version bumps over compatibility shims once a new cooked contract is chosen.

### Phase 0: Diagnostics And Baseline

Tier coverage: prepares Tier 1, Tier 2, Tier 3, and Tier 4.

Implementation prompt:

```text
Add baseline geometry-instancing diagnostics without changing runtime behavior.

Inspect the source import, scene cook, GameFramework load/snapshot, Renderer scene-data build, and GBuffer draw paths. Add diagnostics that report: imported unique mesh primitive candidates, imported mesh placements, imported authored instance groups if detected, cooked mesh asset reference count, cooked instance count, cooked instance group count, runtime mesh asset count, runtime mesh instance count, renderer mesh draw count, renderer batch count, renderer instances-in-batches count, and estimated GBuffer draw calls saved.

Keep diagnostics module-appropriate: source import/cook diagnostics stay in tools, loaded scene diagnostics stay in GameFramework or editor-facing payload inspection, and renderer draw diagnostics stay in Renderer/editor diagnostics. Do not add backend-specific types or command-list inspection to tool or GameFramework code.

Add one small repeated-geometry validation asset or identify an existing one. Prefer a glTF where multiple nodes reference the same mesh primitive. If an `EXT_mesh_gpu_instancing` sample is available, include it as a second validation asset, but do not block this phase on it.
```

Production notes:

- Diagnostics must be deterministic and cheap enough to keep enabled in editor builds.
- Count authored instance groups separately from renderer-discovered batches.
- Do not infer correctness from draw-call count alone; keep original instance count visible.
- Keep renderer diagnostics out of the hot render payloads. Use the existing mesh diagnostics provider path for Phase 0 baselines and keep collection in a private renderer diagnostics helper, matching the current texture diagnostics shape where public rows are plain snapshot data and private owners build them on demand.
- Shader diagnostics for this phase should remain the existing shader package generation/reload status. Do not add shader-specific instancing diagnostics until the structured-buffer shader contract exists.

Validation:

- Recook the repeated-geometry sample and confirm current flattening is visible in diagnostics.
- Launch editor/runtime and confirm the rendered scene is unchanged.
- Confirm diagnostics make it obvious whether duplication was lost during import, cook, load, snapshot, or render build.

### Phase 1: Tier 2 Import-Preserved Shared Geometry

Tier coverage: Tier 2 foundation, prerequisite for Tier 3.

Implementation prompt:

```text
Split source import scene data so mesh geometry identity is separate from scene placement.

Replace the current `ImportedMesh`-as-geometry-plus-transform model with imported mesh primitives and imported mesh instances. Add `ImportedMeshPrimitive` for geometry, display/source identity, source mesh index, and primitive index. Add `ImportedMeshInstance` for primitive index, material index, world transform, and optional source node identity. Keep the public `SourceImportResult` backend-neutral and free of renderer/RHI concepts.

Update the glTF importer so repeated references to the same glTF mesh primitive produce one imported primitive and multiple imported instances. Preserve material binding per primitive instance. Keep existing triangle-only, Draco, morph-target, material-variant warning behavior intact. Update mesh/material cookers and converter command surfaces so they consume the new primitive/instance model instead of assuming one imported mesh equals one cooked mesh asset and one placement.

Use deterministic primitive identity. A suitable initial key is source mesh index plus primitive index. Do not key by display name or floating-point transform. Preserve original ordering so existing scenes remain stable when they do not share geometry.
```

Production notes:

- This follows the Donut/Cauldron-style separation of geometry resources from scene graph instances.
- Do not put renderer batching decisions in `SourceImportResult`.
- Keep import data explicit enough for later features: material variants, skinning, and morph targets should have a clear place to add feature state later.
- Avoid duplicate cooked mesh assets when two imported instances reference the same source primitive.

Validation:

- Source import diagnostics show unique primitive count lower than instance count for a repeated-geometry glTF.
- Mesh cooker emits one cooked mesh asset per unique primitive, not per placement.
- Existing non-repeated scenes produce equivalent mesh and instance counts to the old behavior.
- Source-only validation should compile the touched tools and engine headers before moving to the cooked manifest phase.

### Phase 2: Tier 3 Authored Instance Groups In Import And Cook

Tier coverage: Tier 3 data preservation.

Implementation prompt:

```text
Preserve authored mesh instance groups through source import and cooked scene manifests.

Add imported instance group data that references imported primitive index, material index, first instance or explicit instance range, instance count, generic group kind, and flags. Add an importer-neutral group-kind enum with at least `None`, `SharedMeshReference`, and `AuthoredInstanceGroup`. For glTF, detect `EXT_mesh_gpu_instancing` inside the private glTF importer and map it to the generic authored group kind while importing its TRANSLATION, ROTATION, SCALE, and MATRIX data into world transforms. Compose authored instance transforms with the node/world transform according to glTF semantics. Validate accessor counts and skip malformed groups with clear warnings instead of crashing.

Bump `kCookedSceneManifestVersion`. Add fixed-size trivially copyable cooked records for instance groups. The cooked manifest should contain mesh asset references, material asset references, instance records, and instance group records in deterministic order. Instance records should reference mesh asset index, material asset index, world transform, and group index or invalid group. Instance group records should reference mesh asset index, material asset index, first instance, instance count, importer-neutral group kind, and flags.

Update `SceneCooker` writer and `SceneManifestLoader` reader together. Reject old scene manifests with a clear recook-required error rather than carrying a long-term compatibility path. Scenes without authored instancing should emit zero instance groups and remain valid.
```

Production notes:

- Keep cooked records POD/trivially copyable, matching the existing cooked asset format style.
- Keep group kinds importer-neutral and backend-neutral. They are diagnostics and batching hints, not source-format or RHI commands.
- Do not make the renderer depend on cgltf or importer-specific structs.
- Validate all record indices at load time before building runtime payloads.

Validation:

- Recook existing scenes and verify old manifests are replaced by the new version.
- Inspect an instancing sample and confirm cooked primitive, instance, and group counts match import diagnostics.
- Confirm malformed or unsupported glTF instancing data logs a useful warning and does not poison unrelated meshes.
- Launch D3D12 and Vulkan runtime smoke with scenes that have zero instance groups and confirm unchanged rendering.

### Phase 3: Runtime Payload And Snapshot Contract

Tier coverage: carries Tier 2 and Tier 3 into Renderer; prepares Tier 1 and Tier 4.

Implementation prompt:

```text
Update GameFramework runtime scene payloads and snapshots so mesh asset identity and instance placement remain separate after load.

Change `SceneAssetPayload` so it owns loaded mesh assets separately from mesh instances. A mesh instance should reference a mesh asset index, transform, material handle, and optional instance group id instead of embedding `MeshData` directly. Add runtime instance group records that reference mesh asset index, material handle, first instance, instance count, source feature tag, and flags.

Update `SceneAssetManager` to load each cooked mesh asset once, then create placement records from cooked instance records. Update `GameScene` and `SceneMeshes` so existing component behavior continues to work, while the snapshot exposes enough data for Renderer to build batches from preserved groups. Keep Renderer-facing snapshot data immutable and frame-local.

Do not introduce Renderer or RHI dependencies into GameFramework. The snapshot may expose mesh pointers/handles and material handles as it does today, but GPU mesh upload, batch construction, and instance-buffer ownership remain Renderer responsibilities.
```

Production notes:

- This mirrors common production renderer architecture: the gameplay scene owns logical instances; the renderer builds render items and batches.
- Preserve flat instance access during transition so existing editor panels and selection behavior do not break.
- Keep group ids stable within a loaded scene for diagnostics.
- Validate material handle remapping carefully when scene payloads contain multiple material assets.

Validation:

- Loaded scene diagnostics match cooked manifest mesh asset, instance, and group counts.
- Existing editor mesh selection/used-mesh diagnostics still work for non-instanced scenes.
- A repeated-geometry scene loads one mesh asset with multiple placements where appropriate.
- Renderer receives snapshot data with group identity preserved but no backend-specific data.

### Phase 4: Tier 1 Renderer Auto-Batching

Tier coverage: Tier 1 fallback batching.

Implementation prompt:

```text
Add renderer-side auto-batching for compatible flat mesh instances, independent of authored instance groups.

Extend `RenderSceneDataBuilder` to resolve snapshot instances into intermediate render items, then group compatible items into `MeshInstanceBatch` records. The batch key must include `GPUMesh*`, material slot, texture binding set identity or material data identity, raster pass/pipeline relevant state, render layer/visibility flags when available, and explicit feature flags for unsupported per-object state. Keep the first implementation conservative: do not batch skinned meshes, morph targets, material variants, or anything with unknown per-object shader state.

Build batches deterministically. Preserve source-authored groups when present, then run auto-batching over remaining compatible flat instances. Keep singleton draws available as fallback. Add renderer diagnostics for candidate items, authored batches, auto batches, rejected candidates by reason, singleton draws, and estimated draw calls saved.
```

Production notes:

- This follows the NVRHI/Donut/Cauldron pattern of building backend-neutral draw batches before command recording.
- The batch builder should not allocate backend resources; it only builds CPU render scene data.
- Avoid unordered-map-dependent draw ordering unless keys are sorted before emission.
- Keep a debug path that can disable auto-batching while preserving authored groups.

Validation:

- A non-extension repeated-geometry scene produces auto batches and fewer planned GBuffer draws.
- Rejected candidates are explainable through diagnostics.
- Disabling auto-batching returns to singleton draw behavior without changing loaded scene data.
- Existing scenes render unchanged.

### Phase 5: Tier 4 Instance Buffer And GBuffer Shader Path

Tier coverage: Tier 4 GPU data contract and draw submission.

Implementation prompt:

```text
Implement the renderer instance-buffer path for opaque GBuffer draws.

Add renderer-owned frame-local instance data storage. Each `MeshInstanceData` entry should contain world matrix, world inverse transpose, material slot or reserved material index field, and reserved flags. Upload this data through the existing RHI/resource path used for dynamic or frame resources. Do not expose native D3D12/Vulkan buffers outside RHI/Renderer ownership.

Extend GBuffer draw parameters and shader parameter metadata to bind a structured instance buffer plus a first-instance offset or equivalent small constant. Update the GBuffer vertex shader to read instance data using `SV_InstanceID` plus the batch offset. Keep the vertex layout unchanged. Keep per-material pixel data and texture table binding behavior unchanged for the first implementation.

Update `GBufferPass::DrawOpaqueMeshes` so singleton draws still work, authored/auto batches bind the instance buffer, and batched draws submit `DrawIndexedInstanced(indexCount, instanceCount, 0, 0, 0)` or the equivalent start-instance path if the chosen shader contract uses it. Validate the shader package reflection for DXIL and SPIR-V so both D3D12 and Vulkan see the same logical binding layout.
```

Production notes:

- Prefer a structured buffer over per-instance vertex input to avoid duplicating pipeline input layouts.
- Keep the data layout explicitly padded/aligned for HLSL/SPIR-V reflection.
- Treat first-instance offset as a renderer draw parameter, not a global mutable shader state.
- The instance buffer is a renderer frame resource; GameFramework and tools never own or map it.
- Keep backend command recording thin: bind prepared resources and issue draw calls, matching NVRHI-style command list usage.

Validation:

- Recook shaders and verify the cooked shader package includes the instance-buffer binding for DXIL and SPIR-V.
- Run a D3D12 smoke scene with authored groups and auto batches; verify the image is unchanged and diagnostics show `instanceCount > 1` batches.
- Run the Vulkan smoke path with the same scene and confirm matching diagnostics.
- Verify singleton draws still render when no batches are present.

### Phase 6: Editor Diagnostics And Production Hardening

Tier coverage: closes Tier 1-4 for production readiness.

Implementation prompt:

```text
Expose instance batching diagnostics in the editor and harden failure behavior.

Add editor-facing diagnostics that show imported instance groups, runtime instance groups, renderer authored batches, renderer auto batches, singleton draws, instances per batch, rejected batch candidates by reason, and GBuffer draw calls saved. Prefer integrating with the existing mesh diagnostics surface unless a renderer stats panel already owns draw-call diagnostics more cleanly.

Add validation and defensive behavior around empty batches, invalid mesh/material indices, missing texture binding sets, missing instance buffer uploads, shader binding layout mismatches, and unsupported feature combinations. Fail closed: render singleton draws or skip only the invalid draw with a useful warning rather than corrupting batch state.

Add focused source-only checks or validation gates for scene manifest versioning, source import grouping, renderer batch-key construction, and shader package reflection parity. Keep checks narrow and tied to the touched modules.
```

Production notes:

- Diagnostics should let us distinguish authored groups from auto-discovered groups.
- Warnings should identify the asset, primitive, material, group id, or draw where possible.
- Production-ready means deterministic data, clear ownership, useful failure messages, backend parity, and no hidden compatibility shims.

Validation:

- Editor diagnostics show non-zero instance batch counts for the instancing sample.
- Invalid data tests or manual corrupt-manifest checks produce clear errors.
- D3D12 and Vulkan smoke paths continue to agree on batch counts.
- Documentation and prompts remain accurate after implementation.

## Open Questions

- Should the canonical validation asset be a small custom glTF committed under project assets, or an external sample referenced by the cook scripts?
- Should editor diagnostics live in the existing used-meshes panel, a renderer stats panel, or both?
- Should Phase 5 use an explicit first-instance draw parameter or the RHI `startInstanceLocation` value as the primary shader offset? The structured-buffer contract should choose one and keep it consistent across D3D12/Vulkan.

## Recommended First Slice

Start with Phase 0, then Phase 1. That gives immediate visibility into current duplication and fixes the root architectural problem: the source import model currently cannot represent shared geometry separately from placement.

First implementation prompt:

```text
Implement Phase 0 and the minimal Phase 1 source import split for geometry instancing.

Add diagnostics for imported unique mesh primitive candidates and mesh placements. Then introduce separate imported primitive and imported instance data in `SourceImportResult`, update the glTF importer to reuse one primitive for repeated source mesh/primitive references, and update the mesh cooker enough to continue emitting cooked mesh assets from unique primitives. Do not implement GPU instance buffers yet. Validate by recooking a repeated-geometry glTF and showing primitive count lower than instance count while rendering remains unchanged.
```

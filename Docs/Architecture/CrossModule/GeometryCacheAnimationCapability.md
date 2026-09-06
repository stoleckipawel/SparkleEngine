# Geometry Cache Animation Capability Snapshot

Status: capability snapshot; not release approval or executable evidence

Snapshot: 2026-08-28 at committed `master` revision `20814381`; source and executable build configuration were unchanged from implementation revision `99af6d5b`

Scope: current import, cook, animation, render-scene, deformation, residency, and Modern Sponza source surfaces relevant to geometry-cache delivery

Architecture authority: [Geometry Cache Animation Pipeline](GeometryCacheAnimation.md)

Delivery authority: [Geometry Cache Animation Delivery Plan](../../Plans/CrossModule/GeometryCacheAnimation.md)

Acceptance authority: [Geometry Cache Animation Acceptance Contract](../../Acceptance/CrossModule/GeometryCacheAnimation.md)

Code and executable build configuration remain authoritative. Reinspect every listed owner and absence before using this dated snapshot for implementation or release claims.

## Source-Backed Snapshot

The target architecture extends existing owners instead of adding a parallel animation or render scene:

- [`SourceSceneImporter.cpp`](../../../Tools/Import/SourceImporters/Private/SourceSceneImporter.cpp) selects glTF or FBX from a hard-coded importer list. [`ImportedScene.h`](../../../Tools/Import/SourceImporters/Public/Types/ImportedScene.h) carries meshes, instances, materials, skeletons, and clips but no geometry-cache tracks. No Alembic dependency exists in the current source dependency manifest.
- [`ImportedMeshDeformation.h`](../../../Tools/Import/SourceImporters/Public/Types/ImportedMeshDeformation.h) contains skin influences and morph targets. Expanding one cache sample per frame into morph targets would misuse that contract and scale memory with `vertex count * sample count`.
- [`ImportedSceneCooker.cpp`](../../../Tools/Cooking/AssetCooker/Private/Cooking/ImportedSceneCooker.cpp) is the scene-cook orchestrator. [`CookedSceneBuild.h`](../../../Tools/Cooking/SceneCooker/Public/CookedSceneBuild.h) and [`CookedSceneGenerationWriter.cpp`](../../../Tools/Cooking/SceneCooker/Private/CookedSceneGenerationWriter.cpp) already stage one atomic generation of meshes, materials, skeletons, animations, and manifests.
- [`SceneAssetFileReader.cpp`](../../../Engine/GameFramework/Private/Assets/Loading/SceneAssetFileReader.cpp) currently reads complete mesh, material, skeleton, and animation products for scene activation. A geometry cache cannot join that full-file retention path; activation should read only its header and chunk directory.
- [`AnimationComponents.h`](../../../Engine/GameFramework/Private/World/ECS/Components/AnimationComponents.h) and the animation systems own skeletal and morph playback/output. Geometry-cache playback needs its own small component because its output is sampled vertex data, not a pose or weight vector.
- [`RenderSceneDelta.h`](../../../Engine/GameFramework/Public/Rendering/RenderSceneDelta.h) carries structural mesh/material state, while [`RenderSceneDynamicData.h`](../../../Engine/GameFramework/Public/Rendering/RenderSceneDynamicData.h) carries per-frame transforms, joint matrices, and morph weights. Those are the existing structural and dynamic scene-publication seams to extend.
- Historical `RenderDeformationPreparation` lived under `Renderer/Private/SceneData/Preparation`. The committed renderer migration moved scene deformation continuity beneath `RenderScene` and view-independent frame materialization beneath `Scene/Preparation/RenderScenePreparation`; geometry-cache preparation belongs in that owner or a focused collaborator owned by it, never another scene-preparation graph.
- [`GBufferVS.hlsl`](../../../Engine/Assets/Shaders/Passes/GBuffer/GBufferVS.hlsl) evaluates morphing and skinning in the raster vertex shader. [`RayTracingMaterialHit.hlsli`](../../../Engine/Assets/Shaders/RayTracing/RayTracingMaterialHit.hlsli) repeats their attribute reconstruction at ray hits.
- [`RayTracingBlasGeometryBuilder.cpp`](../../../Engine/Renderer/Private/RayTracing/Acceleration/RayTracingBlasGeometryBuilder.cpp) currently reconstructs skinned positions on the CPU, and [`RayTracingBlasCache.cpp`](../../../Engine/Renderer/Private/RayTracing/Acceleration/RayTracingBlasCache.cpp) uploads a replacement vertex buffer and rebuilds its BLAS. Geometry-cache delivery must not copy this CPU-versus-shader split.
- [`BuildRenderFrameGraph.cpp`](../../../Engine/Renderer/Private/Frame/Graph/BuildRenderFrameGraph.cpp) adds the ray-tracing scene before GBuffer feature setup. Shared deformed geometry must therefore be ready before both consumers rather than inserted inside either one.
- [`AssetResidency`](../../../Engine/Renderer/Private/Resources/Residency/AssetResidency.h) already owns read/decode/upload/resident/retirement state and byte budgets. [`SparkleTasks`](../../../Engine/Tasks/Public/TaskTypes.h) already provides a bounded blocking-I/O lane. Geometry-cache streaming should reuse both instead of creating a cache-specific thread pool or second lifetime protocol.

The Modern Sponza Animated Knight archive is the motivating workload, not a reason for a scene-specific fix:

- `Knight_USD_002.fbx` contains renderable meshes and controller animation channels but no bones or skin weights. It can prove static FBX import and unit normalization; it cannot produce visible skeletal deformation.
- `Knight_Animation_Data_Only_002.fbx` also has no deform skeleton or skin binding.
- the Maya source contains skin clusters, bind-pose data, joints, and animation curves;
- `Exports/alembic/knight_ANIM_001.rnd.abc` and the published USD contain baked vertex deformation. The Alembic is the source intended for a geometry-cache path.

The knight remains unaccepted as an animated workload until the [Geometry Cache Animation Acceptance Contract](../../Acceptance/CrossModule/GeometryCacheAnimation.md) passes. Loading the static FBX, baking one pose, remapping controller names, or scaling only the knight entity does not close that gate.

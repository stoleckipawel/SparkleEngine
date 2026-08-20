#include "PCH.h"

#include "SceneData/Preparation/RenderPreparationInputResolver.h"

#include "Core/Public/Diagnostics/Verify.h"
#include "Meshes/GpuMesh.h"
#include "Meshes/GpuMeshCache.h"
#include "Renderer/Public/Debug/RendererCVars.h"
#include "Scene/Materials/MaterialHandleResolver.h"
#include "SceneData/Preparation/RenderDeformationPreparation.h"
#include "SceneData/Preparation/RenderPreparationRun.h"
#include "SceneData/RenderMeshClassificationConversion.h"
#include "Scene/RenderScene.h"
#include "ShaderData/MeshInstanceShaderData.h"
#include "Textures/RendererTexture.h"
#include "Textures/TextureCache.h"

#include <algorithm>
#include <span>
#include <utility>

static const auto g_renderPreparationInputResolverLogger = Logging::GetOrCreateLogger("Renderer.RenderPreparationInputResolver");

RenderPreparationInputResolver::RenderPreparationInputResolver(GpuMeshCache& gpuMeshCache, TextureCache& textureCache) noexcept :
    m_gpuMeshCache(&gpuMeshCache),
    m_textureCache(&textureCache)
{
}

void RenderPreparationInputResolver::Resolve(
    RenderScene& scene,
    const Frustum& frustum,
    const DirectX::XMFLOAT3& cameraPosition,
    RenderDeformationPreparation& deformationPreparation,
    RenderPreparationRun& run)
{
	run.ViewFrustum = frustum;
	run.CameraPosition = cameraPosition;
	run.Lights = scene.GetLights();
	run.EnableAutoBatching = CVarRendererMeshAutoBatching.Get();
	run.SceneData.structuralRevision = scene.GetStructuralRevision();
	run.SceneData.materialRevision = scene.GetMaterialRevision();
	scene.BuildMaterials(run.SceneData);

	ResolveSky(scene, run.SceneData);
	ResolveObjects(scene, run);
	ResolveInstanceGroups(scene, run);

	run.PreparedObjects.resize(run.ResolvedObjects.size());
	run.PreparedLights.resize(scene.GetLights().size());
	deformationPreparation.Prepare(scene, run.ResolvedObjects, run.Deformation);
}

void RenderPreparationInputResolver::ResolveObjects(const RenderScene& scene, RenderPreparationRun& run)
{
	run.ResolvedObjects.clear();
	run.ResolvedObjects.reserve(scene.GetPrimitives().size());
	for (const RenderPrimitive& primitive : scene.GetPrimitives())
	{
		if (!primitive.Dynamic.Visible || !primitive.GpuMeshResident)
			continue;
		run.ResolvedObjects.push_back(ResolveObject(scene, primitive, scene.GetMaterials().Generation, run.SceneData));
	}
}

ResolvedRenderObject RenderPreparationInputResolver::ResolveObject(
    const RenderScene& scene,
    const RenderPrimitive& primitive,
    std::uint32_t materialGeneration,
    RenderSceneData& sceneData)
{
	if (!primitive.Dynamic.Object.IsValid() || !primitive.Static.Mesh.IsValid())
		Diagnostics::Fatal(
		    g_renderPreparationInputResolverLogger,
		    __FILE__,
		    __LINE__,
		    "Render scene primitive contains an invalid object or mesh identity.");

	const GpuMesh* gpuMesh = m_gpuMeshCache->Resolve(primitive.GpuMesh);
	if (gpuMesh == nullptr || !gpuMesh->IsValid())
		Diagnostics::Fatal(g_renderPreparationInputResolverLogger, __FILE__, __LINE__, "Resident render scene primitive has no GPU mesh.");

	const std::uint32_t materialSlot =
	    MaterialHandleResolver::ResolveSlot(primitive.Static.Material, materialGeneration, sceneData.materials.size());
	const MaterialData& material = sceneData.materials[materialSlot];

	MeshDraw draw;
	draw.Material.Slot = materialSlot;
	draw.Source.GpuSceneSlot = primitive.GpuSceneSlot;
	draw.Source.MeshAssetId = primitive.Static.Mesh.GetAssetId();
	draw.Source.MeshGeneration = primitive.Static.Mesh.GetGeneration();
	draw.Skinning.SkeletonAssetId = primitive.Static.Skeleton.GetAssetId();
	draw.Skinning.JointMatrixOffset = kInvalidMeshInstanceJointMatrixOffset;
	draw.Geometry.MeshKind = RenderMeshClassificationConversion::ToRenderMeshKind(primitive.Static.MeshKind);
	draw.Geometry.Mesh = gpuMesh->GetHandle();
	draw.Geometry.LocalBoundsMin = gpuMesh->GetLocalBounds().Min;
	draw.Geometry.LocalBoundsMax = gpuMesh->GetLocalBounds().Max;
	draw.Geometry.HasLocalBounds = gpuMesh->GetLocalBounds().Valid;

	return ResolvedRenderObject{
	    .Object = primitive.Object,
	    .Draw = draw,
	    .WorldMatrix = primitive.Dynamic.WorldMatrix,
	    .PreviousWorldMatrix = scene.ResolvePreviousWorldMatrix(primitive),
	    .WorldInverseTranspose = primitive.Dynamic.WorldInverseTranspose,
	    .Material = material.gpuHandle,
	    .InstanceGroupIndex = RenderMeshClassificationConversion::ToRenderMeshInstanceGroupIndex(primitive.Static.InstanceGroupIndex),
	    .MaterialAlphaMode = material.alphaMode,
	    .MorphTargetCount = gpuMesh->GetMorphTargetCount(),
	    .MorphTargetVertexCount = gpuMesh->GetVertexCount()};
}

void RenderPreparationInputResolver::ResolveInstanceGroups(const RenderScene& scene, RenderPreparationRun& run) const
{
	run.InstanceGroups.clear();
	run.InstanceGroups.reserve(scene.GetInstanceGroups().size());
	for (const RenderMeshInstanceGroupData& group : scene.GetInstanceGroups())
	{
		run.InstanceGroups.push_back(
		    RenderMeshInstanceGroup{
		        .groupKind = RenderMeshClassificationConversion::ToRenderMeshInstanceGroupKind(group.Kind),
		        .instanceCount = group.InstanceCount});
	}
}

void RenderPreparationInputResolver::ResolveSky(const RenderScene& scene, RenderSceneData& sceneData) const
{
	const RendererTexture* skyTexture = nullptr;
	const SceneSkyDesc* sky = scene.GetSky() ? &*scene.GetSky() : nullptr;
	if (sky == nullptr)
	{
		skyTexture = m_textureCache->ResolveDefaultSkyTexture();
	}
	else
	{
		sceneData.sky.enabled = sky->enabled;
		sceneData.sky.color = sky->color;
		sceneData.sky.brightness = sky->brightness;
		if (!sky->skyTexture.IsValid())
		{
			skyTexture = m_textureCache->ResolveDefaultSkyTexture();
		}
		else
		{
			skyTexture = m_textureCache->GetSceneTexture(sky->skyTexture.texturePath);
		}
	}
	if (skyTexture == nullptr || !*skyTexture)
		Diagnostics::Fatal(g_renderPreparationInputResolverLogger, __FILE__, __LINE__, "Scene sky texture is unavailable.");
	sceneData.sky.texture = skyTexture;
}

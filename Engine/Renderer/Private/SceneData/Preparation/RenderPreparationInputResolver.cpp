#include "PCH.h"

#include "SceneData/Preparation/RenderPreparationInputResolver.h"

#include "Meshes/GPUMesh.h"
#include "Meshes/GPUMeshCache.h"
#include "Renderer/Public/Debug/RendererCVars.h"
#include "SceneData/Caching/MaterialCacheManager.h"
#include "SceneData/Caching/MaterialCacheUtils.h"
#include "SceneData/Preparation/RenderDeformationPreparation.h"
#include "SceneData/Preparation/RenderPreparationRun.h"
#include "SceneData/RenderMeshClassificationConversion.h"
#include "SceneData/RenderWorld.h"
#include "ShaderData/MeshInstanceShaderData.h"
#include "Textures/RendererTexture.h"
#include "Textures/TextureManager.h"

#include <algorithm>
#include <span>
#include <utility>

RenderPreparationInputResolver::RenderPreparationInputResolver(
    MaterialCacheManager& materialCache,
    GPUMeshCache& gpuMeshCache,
    TextureManager& textureManager) noexcept :
	m_materialCache(&materialCache),
	m_gpuMeshCache(&gpuMeshCache),
	m_textureManager(&textureManager)
{
}

void RenderPreparationInputResolver::Resolve(
    const RenderWorld& world,
    const RenderFrameDynamicData& dynamic,
    const Frustum& frustum,
    std::span<const RenderPreviousWorldTransform> previousWorldTransforms,
    RenderDeformationPreparation& deformationPreparation,
    RenderPreparationRun& run)
{
	run.ViewFrustum = frustum;
	run.CameraPosition = dynamic.Camera.Position;
	run.Lights =
	    std::span<const RenderLightData>{dynamic.Lights};
	run.EnableAutoBatching = CVarRendererMeshAutoBatching.Get();
	run.SceneData.structuralRevision = world.GetStructuralRevision();
	run.SceneData.materialRevision = world.GetMaterialRevision();
	m_materialCache->BuildMaterials(
	    world.GetMaterials(),
	    run.SceneData.materialRevision,
	    run.SceneData);

	ResolveSky(world, run.SceneData);
	ResolveObjects(world, previousWorldTransforms, run);
	ResolveInstanceGroups(world, run);

	run.PreparedObjects.resize(run.ResolvedObjects.size());
	run.PreparedLights.resize(dynamic.Lights.size());
	deformationPreparation.Prepare(
	    dynamic,
	    run.ResolvedObjects,
	    run.Deformation);
}

void RenderPreparationInputResolver::ResolveObjects(
    const RenderWorld& world,
    std::span<const RenderPreviousWorldTransform> previousWorldTransforms,
    RenderPreparationRun& run)
{
	run.ResolvedObjects.clear();
	run.ResolvedObjects.reserve(world.GetProxies().size());
	for (const RenderProxy& proxy : world.GetProxies())
	{
		ResolvedRenderObject object;
		if (TryResolveObject(proxy, previousWorldTransforms, run.SceneData, object))
		{
			run.ResolvedObjects.push_back(std::move(object));
		}
	}
}

bool RenderPreparationInputResolver::TryResolveObject(
    const RenderProxy& proxy,
    std::span<const RenderPreviousWorldTransform> previousWorldTransforms,
    RenderSceneData& sceneData,
    ResolvedRenderObject& output)
{
	if (!proxy.Dynamic.Object.IsValid() || !proxy.Dynamic.Visible)
	{
		return false;
	}

	if (!proxy.Static.Mesh.IsValid())
	{
		return false;
	}

	const GPUMesh* gpuMesh = m_gpuMeshCache->Resolve(proxy.GpuMesh);
	if (gpuMesh == nullptr || !gpuMesh->IsValid())
	{
		return false;
	}

	const std::uint32_t materialSlot =
	    MaterialCacheUtils::ResolveMaterialSlot(proxy.Static.Material, sceneData.materials.size());
	const MaterialData* material =
	    materialSlot < sceneData.materials.size() ? &sceneData.materials[materialSlot] : nullptr;

	MeshDraw draw;
	draw.Material.Slot = materialSlot;
	draw.Source.GpuSceneSlot = proxy.GpuSceneSlot;
	draw.Source.MeshAssetId = proxy.Static.Mesh.GetAssetId();
	draw.Source.MeshGeneration = proxy.Static.Mesh.GetGeneration();
	draw.Skinning.SkeletonAssetId = proxy.Static.Skeleton.GetAssetId();
	draw.Skinning.JointMatrixOffset = kInvalidMeshInstanceJointMatrixOffset;
	draw.Geometry.MeshKind = RenderMeshClassificationConversion::ToRenderMeshKind(proxy.Static.MeshKind);
	draw.Geometry.Mesh = gpuMesh->GetHandle();
	draw.Geometry.LocalBoundsMin = gpuMesh->GetLocalBounds().Min;
	draw.Geometry.LocalBoundsMax = gpuMesh->GetLocalBounds().Max;
	draw.Geometry.HasLocalBounds = gpuMesh->GetLocalBounds().Valid;

	DirectX::XMFLOAT4X4 previousWorldMatrix = proxy.Dynamic.WorldMatrix;
	if (proxy.GpuSceneSlot < previousWorldTransforms.size() &&
	    previousWorldTransforms[proxy.GpuSceneSlot].Object == proxy.Object)
	{
		previousWorldMatrix = previousWorldTransforms[proxy.GpuSceneSlot].WorldMatrix;
	}

	output = ResolvedRenderObject{
	    .Object = proxy.Object,
	    .Draw = draw,
	    .WorldMatrix = proxy.Dynamic.WorldMatrix,
	    .PreviousWorldMatrix = previousWorldMatrix,
	    .WorldInverseTranspose = proxy.Dynamic.WorldInverseTranspose,
	    .Material = material != nullptr ? material->gpuHandle : MaterialGpuHandle{},
	    .InstanceGroupIndex =
	        RenderMeshClassificationConversion::ToRenderMeshInstanceGroupIndex(proxy.Static.InstanceGroupIndex),
	    .MaterialAlphaMode = material != nullptr ? material->alphaMode : 0u,
	    .MorphTargetCount = gpuMesh->GetMorphTargetCount(),
	    .MorphTargetVertexCount = gpuMesh->GetVertexCount()};
	return true;
}

void RenderPreparationInputResolver::ResolveInstanceGroups(
    const RenderWorld& world,
    RenderPreparationRun& run) const
{
	run.InstanceGroups.clear();
	run.InstanceGroups.reserve(world.GetInstanceGroups().size());
	for (const RenderMeshInstanceGroupData& group :
	     world.GetInstanceGroups())
	{
		run.InstanceGroups.push_back(
		    RenderMeshInstanceGroup{
		        .groupKind =
		            RenderMeshClassificationConversion::
		                ToRenderMeshInstanceGroupKind(group.Kind),
		        .instanceCount = group.InstanceCount});
	}
}

void RenderPreparationInputResolver::ResolveSky(
    const RenderWorld& world,
    RenderSceneData& sceneData) const
{
	const RendererTexture* skyTexture = nullptr;
	const SceneSkyDesc* sky =
	    world.GetSky() ? &*world.GetSky() : nullptr;
	if (sky == nullptr)
	{
		skyTexture = m_textureManager->ResolveDefaultSkyTexture();
	}
	else
	{
		sceneData.sky.enabled = sky->enabled;
		sceneData.sky.color = sky->color;
		sceneData.sky.intensity = sky->intensity;
		if (!sky->skyTexture.IsValid())
		{
			skyTexture =
			    m_textureManager->ResolveDefaultSkyTexture();
		}
		else
		{
			skyTexture = m_textureManager->GetSceneTexture(
			    sky->skyTexture.texturePath);
			if (skyTexture == nullptr)
			{
				skyTexture =
				    m_textureManager->GetTexture(
				        TextureId::Checker);
			}
		}
	}
	sceneData.sky.texture =
	    skyTexture != nullptr && *skyTexture
	        ? skyTexture
	        : nullptr;
}

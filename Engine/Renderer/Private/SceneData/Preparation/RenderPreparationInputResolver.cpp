#include "PCH.h"

#include "SceneData/Preparation/RenderPreparationInputResolver.h"

#include "Core/Public/Diagnostics/Verify.h"
#include "Meshes/GpuMesh.h"
#include "Meshes/GpuMeshCache.h"
#include "Renderer/Public/Debug/RendererCVars.h"
#include "SceneData/Caching/MaterialCache.h"
#include "SceneData/Caching/MaterialHandleResolver.h"
#include "SceneData/Preparation/RenderDeformationPreparation.h"
#include "SceneData/Preparation/RenderPreparationRun.h"
#include "SceneData/RenderMeshClassificationConversion.h"
#include "SceneData/RenderWorld.h"
#include "ShaderData/MeshInstanceShaderData.h"
#include "Textures/RendererTexture.h"
#include "Textures/TextureCache.h"

#include <algorithm>
#include <span>
#include <utility>

static const auto g_renderPreparationInputResolverLogger = Logging::GetOrCreateLogger("Renderer.RenderPreparationInputResolver");

RenderPreparationInputResolver::RenderPreparationInputResolver(
    MaterialCache& materialCache,
    GpuMeshCache& gpuMeshCache,
    TextureCache& textureCache) noexcept :
    m_materialCache(&materialCache), m_gpuMeshCache(&gpuMeshCache), m_textureCache(&textureCache)
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
	run.Lights = std::span<const RenderLightData>{dynamic.Lights};
	run.EnableAutoBatching = CVarRendererMeshAutoBatching.Get();
	run.SceneData.structuralRevision = world.GetStructuralRevision();
	run.SceneData.materialRevision = world.GetMaterialRevision();
	m_materialCache->BuildMaterials(world.GetMaterials(), run.SceneData.materialRevision, run.SceneData);

	ResolveSky(world, run.SceneData);
	ResolveObjects(world, previousWorldTransforms, run);
	ResolveInstanceGroups(world, run);

	run.PreparedObjects.resize(run.ResolvedObjects.size());
	run.PreparedLights.resize(dynamic.Lights.size());
	deformationPreparation.Prepare(dynamic, run.ResolvedObjects, run.Deformation);
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
		if (!proxy.Dynamic.Visible || !proxy.GpuMeshResident)
			continue;
		run.ResolvedObjects.push_back(
		    ResolveObject(proxy, world.GetMaterials().Generation, previousWorldTransforms, run.SceneData));
	}
}

ResolvedRenderObject RenderPreparationInputResolver::ResolveObject(
    const RenderProxy& proxy,
    std::uint32_t materialGeneration,
    std::span<const RenderPreviousWorldTransform> previousWorldTransforms,
    RenderSceneData& sceneData)
{
	if (!proxy.Dynamic.Object.IsValid() || !proxy.Static.Mesh.IsValid())
		Diagnostics::Fatal(
		    g_renderPreparationInputResolverLogger,
		    __FILE__,
		    __LINE__,
		    "Render-world proxy contains an invalid object or mesh identity.");

	const GpuMesh* gpuMesh = m_gpuMeshCache->Resolve(proxy.GpuMesh);
	if (gpuMesh == nullptr || !gpuMesh->IsValid())
		Diagnostics::Fatal(
		    g_renderPreparationInputResolverLogger,
		    __FILE__,
		    __LINE__,
		    "Resident render-world proxy has no GPU mesh.");

	const std::uint32_t materialSlot =
	    MaterialHandleResolver::ResolveSlot(proxy.Static.Material, materialGeneration, sceneData.materials.size());
	const MaterialData& material = sceneData.materials[materialSlot];

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
	if (proxy.GpuSceneSlot < previousWorldTransforms.size() && previousWorldTransforms[proxy.GpuSceneSlot].Object == proxy.Object)
	{
		previousWorldMatrix = previousWorldTransforms[proxy.GpuSceneSlot].WorldMatrix;
	}

	return ResolvedRenderObject{
	    .Object = proxy.Object,
	    .Draw = draw,
	    .WorldMatrix = proxy.Dynamic.WorldMatrix,
	    .PreviousWorldMatrix = previousWorldMatrix,
	    .WorldInverseTranspose = proxy.Dynamic.WorldInverseTranspose,
	    .Material = material.gpuHandle,
	    .InstanceGroupIndex = RenderMeshClassificationConversion::ToRenderMeshInstanceGroupIndex(proxy.Static.InstanceGroupIndex),
	    .MaterialAlphaMode = material.alphaMode,
	    .MorphTargetCount = gpuMesh->GetMorphTargetCount(),
	    .MorphTargetVertexCount = gpuMesh->GetVertexCount()};
}

void RenderPreparationInputResolver::ResolveInstanceGroups(const RenderWorld& world, RenderPreparationRun& run) const
{
	run.InstanceGroups.clear();
	run.InstanceGroups.reserve(world.GetInstanceGroups().size());
	for (const RenderMeshInstanceGroupData& group : world.GetInstanceGroups())
	{
		run.InstanceGroups.push_back(
		    RenderMeshInstanceGroup{
		        .groupKind = RenderMeshClassificationConversion::ToRenderMeshInstanceGroupKind(group.Kind),
		        .instanceCount = group.InstanceCount});
	}
}

void RenderPreparationInputResolver::ResolveSky(const RenderWorld& world, RenderSceneData& sceneData) const
{
	const RendererTexture* skyTexture = nullptr;
	const SceneSkyDesc* sky = world.GetSky() ? &*world.GetSky() : nullptr;
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
		Diagnostics::Fatal(
		    g_renderPreparationInputResolverLogger,
		    __FILE__,
		    __LINE__,
		    "Scene sky texture is unavailable.");
	sceneData.sky.texture = skyTexture;
}

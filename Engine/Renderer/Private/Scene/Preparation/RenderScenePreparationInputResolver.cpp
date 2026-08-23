#include "PCH.h"

#include "Scene/Preparation/RenderScenePreparationInputResolver.h"

#include "Core/Public/Diagnostics/Verify.h"
#include "Meshes/GpuMesh.h"
#include "Meshes/GpuMeshCache.h"
#include "Scene/Materials/MaterialHandleResolver.h"
#include "Scene/Preparation/RenderDeformationPreparation.h"
#include "Scene/Preparation/RenderScenePreparationRun.h"
#include "Scene/Preparation/RenderMeshClassificationConversion.h"
#include "Scene/RenderScene.h"
#include "ShaderData/MeshInstanceShaderData.h"
#include "Textures/RendererTexture.h"
#include "Textures/TextureCache.h"

#include <algorithm>
#include <span>
#include <utility>

static const auto g_renderPreparationInputResolverLogger = Logging::GetOrCreateLogger("Renderer.RenderScenePreparationInputResolver");

RenderScenePreparationInputResolver::RenderScenePreparationInputResolver(GpuMeshCache& gpuMeshCache, TextureCache& textureCache) noexcept :
    m_gpuMeshCache(&gpuMeshCache),
    m_textureCache(&textureCache)
{
}

void RenderScenePreparationInputResolver::Resolve(
    RenderScene& scene,
    RenderDeformationPreparation& deformationPreparation,
    RenderScenePreparationRun& run)
{
	run.Lights = scene.GetLights();
	run.PreparedScene.structuralRevision = scene.GetStructuralRevision();
	run.PreparedScene.materialRevision = scene.GetMaterialRevision();
	scene.BuildMaterials(run.PreparedScene);

	ResolveSky(scene, run.PreparedScene);
	ResolvePrimitives(scene, run);
	ResolveInstanceGroups(scene, run);

	run.PreparedPrimitives.resize(run.ResolvedPrimitives.size());
	run.PreparedLights.resize(scene.GetLights().size());
	deformationPreparation.Prepare(scene, run.ResolvedPrimitives, run.Deformation);
}

void RenderScenePreparationInputResolver::ResolvePrimitives(const RenderScene& scene, RenderScenePreparationRun& run)
{
	run.ResolvedPrimitives.clear();
	run.ResolvedPrimitives.reserve(scene.GetPrimitives().size());
	for (const RenderPrimitive& primitive : scene.GetPrimitives())
	{
		if (!primitive.Dynamic.Visible || !primitive.GpuMeshResident)
			continue;
		run.ResolvedPrimitives.push_back(ResolvePrimitive(scene, primitive, scene.GetMaterials().Generation, run.PreparedScene));
	}
}

ResolvedRenderPrimitive RenderScenePreparationInputResolver::ResolvePrimitive(
    const RenderScene& scene,
    const RenderPrimitive& primitive,
    std::uint32_t materialGeneration,
    PreparedRenderScene& preparedScene)
{
	if (!primitive.Dynamic.Object.IsValid() || !primitive.Static.Mesh.IsValid())
		Diagnostics::Fatal(
		    g_renderPreparationInputResolverLogger,
		    __FILE__,
		    __LINE__,
		    "Render scene primitive contains an invalid primitive or mesh identity.");

	const GpuMesh* gpuMesh = m_gpuMeshCache->Resolve(primitive.GpuMesh);
	if (gpuMesh == nullptr || !gpuMesh->IsValid())
		Diagnostics::Fatal(g_renderPreparationInputResolverLogger, __FILE__, __LINE__, "Resident render scene primitive has no GPU mesh.");

	const std::uint32_t materialSlot =
	    MaterialHandleResolver::ResolveSlot(primitive.Static.Material, materialGeneration, preparedScene.materials.size());
	const MaterialData& material = preparedScene.materials[materialSlot];

	MeshDraw draw;
	draw.MaterialSlot = materialSlot;
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

	return ResolvedRenderPrimitive{
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

void RenderScenePreparationInputResolver::ResolveInstanceGroups(const RenderScene& scene, RenderScenePreparationRun& run) const
{
	run.PreparedScene.instanceGroups.clear();
	run.PreparedScene.instanceGroups.reserve(scene.GetInstanceGroups().size());
	for (const RenderMeshInstanceGroupData& group : scene.GetInstanceGroups())
	{
		run.PreparedScene.instanceGroups.push_back(
		    RenderMeshInstanceGroup{
		        .groupKind = RenderMeshClassificationConversion::ToRenderMeshInstanceGroupKind(group.Kind),
		        .instanceCount = group.InstanceCount});
	}
}

void RenderScenePreparationInputResolver::ResolveSky(const RenderScene& scene, PreparedRenderScene& preparedScene) const
{
	const RendererTexture* skyTexture = nullptr;
	const SceneSkyDesc* sky = scene.GetSky() ? &*scene.GetSky() : nullptr;
	if (sky == nullptr)
	{
		skyTexture = m_textureCache->ResolveDefaultSkyTexture();
	}
	else
	{
		preparedScene.sky.enabled = sky->enabled;
		preparedScene.sky.color = sky->color;
		preparedScene.sky.brightness = sky->brightness;
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
	preparedScene.sky.texture = skyTexture;
}

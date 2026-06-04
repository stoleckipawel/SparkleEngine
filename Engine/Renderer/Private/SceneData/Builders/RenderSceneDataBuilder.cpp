#include "PCH.h"

#include "RenderSceneDataBuilder.h"

#include "Meshes/GPUMeshCache.h"
#include "Renderer/Public/Debug/RendererCVars.h"
#include "Renderer/Public/SceneData/DirectionalLight.h"
#include "SceneData/RenderSceneData.h"
#include "SceneData/Builders/MeshInstanceBatchBuilder.h"
#include "Scene/Meshes/Mesh.h"
#include "SceneData/Caching/MaterialCacheManager.h"
#include "SceneData/Caching/MaterialCacheUtils.h"

#include <cstddef>
#include <utility>

static const auto g_renderSceneDataBuilderLogger = Logging::GetOrCreateLogger("Renderer.SceneData");

RenderSceneDataBuilder::RenderSceneDataBuilder(MaterialCacheManager& materialCache, GPUMeshCache& gpuMeshCache) noexcept :
    m_materialCache(&materialCache), m_gpuMeshCache(&gpuMeshCache)
{
}

RenderSceneData RenderSceneDataBuilder::Build(const RenderSceneSnapshot& sceneSnapshot)
{
	RenderSceneData sceneData = {};

	if (!m_materialCache)
	{
		Diagnostics::Fail(
		    g_renderSceneDataBuilderLogger,
		    __FILE__,
		    __LINE__,
		    "RenderSceneDataBuilder::Build: material cache manager is unavailable.");
		return sceneData;
	}

	BuildMaterials(sceneSnapshot, sceneData);
	BuildMeshInstanceBatches(sceneSnapshot, sceneData);
	BuildLighting(sceneSnapshot, sceneData);
	return sceneData;
}

void RenderSceneDataBuilder::BuildMaterials(const RenderSceneSnapshot& sceneSnapshot, RenderSceneData& sceneData) const
{
	if (m_materialCache == nullptr)
	{
		return;
	}

	m_materialCache->BuildMaterials(sceneSnapshot.materials, sceneData);
}

void RenderSceneDataBuilder::BuildMeshInstanceBatches(const RenderSceneSnapshot& sceneSnapshot, RenderSceneData& sceneData) const
{
	if (!sceneSnapshot.meshes.HasMeshes() || m_gpuMeshCache == nullptr)
	{
		return;
	}

	std::vector<MeshRenderItem> renderItems;
	renderItems.reserve(sceneSnapshot.meshes.meshInstances.size());

	for (const MeshInstanceSnapshot& meshInstance : sceneSnapshot.meshes.meshInstances)
	{
		const Mesh* mesh = meshInstance.mesh;
		if (mesh == nullptr)
		{
			continue;
		}

		GPUMesh* gpuMesh = m_gpuMeshCache->GetOrUpload(*mesh);
		if (gpuMesh == nullptr || !gpuMesh->IsValid())
		{
			continue;
		}

		MeshDraw draw = {};
		draw.worldMatrix = meshInstance.worldMatrix;
		draw.worldInvTranspose = meshInstance.worldInvTranspose;
		draw.materialSlot = MaterialCacheUtils::ResolveMaterialSlot(meshInstance.materialHandle, sceneData.materials.size());
		draw.gpuMesh = gpuMesh;

		SceneMeshInstanceGroupKind instanceGroupKind = SceneMeshInstanceGroupKind::None;
		if (meshInstance.instanceGroupIndex < sceneSnapshot.meshes.meshInstanceGroups.size())
		{
			instanceGroupKind = sceneSnapshot.meshes.meshInstanceGroups[meshInstance.instanceGroupIndex].groupKind;
		}

		renderItems.push_back(
		    MeshRenderItem{
		        .draw = draw,
		        .materialBindingSet = draw.materialSlot < sceneData.materials.size() ? sceneData.materials[draw.materialSlot].textureBindingSet
		                                                                           : nullptr,
		        .instanceGroupIndex = meshInstance.instanceGroupIndex,
		        .instanceGroupKind = instanceGroupKind,
		        .sourceInstanceIndex = static_cast<std::uint32_t>(renderItems.size())});
	}

	MeshInstanceBatchBuilder batchBuilder;
	MeshInstanceBatchBuildResult batchBuildResult = batchBuilder.Build(
	    renderItems,
	    sceneSnapshot.meshes.meshInstanceGroups,
	    MeshInstanceBatchBuildOptions{
	        .enableAutoBatching = CVarRendererMeshAutoBatching.Get(),
	        .requireMaterialBindingSet = true,
	        .collectDiagnostics = true});
	sceneData.meshInstances = std::move(batchBuildResult.batchInstances);
	sceneData.meshInstanceBatches = std::move(batchBuildResult.batches);

	static bool loggedFirstMeshBatchSummary = false;
	static bool loggedMissingMeshBatchWarning = false;
	if (!loggedFirstMeshBatchSummary && !sceneData.meshInstances.empty())
	{
		loggedFirstMeshBatchSummary = true;
		SPDLOG_LOGGER_INFO(
		    g_renderSceneDataBuilderLogger,
		    "RenderSceneDataBuilder: prepared {} renderable mesh instances in {} batches from {} scene mesh instances (rejected={}, missingGpuMesh={}, invalidMaterial={}).",
		    sceneData.meshInstances.size(),
		    sceneData.meshInstanceBatches.size(),
		    sceneSnapshot.meshes.meshInstances.size(),
		    batchBuildResult.diagnostics.RejectedCandidateCount,
		    batchBuildResult.diagnostics.RejectedMissingGpuMeshCount,
		    batchBuildResult.diagnostics.RejectedInvalidMaterialCount);
	}
	else if (!loggedMissingMeshBatchWarning && !sceneSnapshot.meshes.meshInstances.empty() && sceneData.meshInstanceBatches.empty())
	{
		loggedMissingMeshBatchWarning = true;
		SPDLOG_LOGGER_WARN(
		    g_renderSceneDataBuilderLogger,
		    "RenderSceneDataBuilder: scene has {} mesh instances but produced no render batches (candidates={}, rejected={}, missingGpuMesh={}, invalidGroup={}, invalidMaterial={}).",
		    sceneSnapshot.meshes.meshInstances.size(),
		    batchBuildResult.diagnostics.CandidateItemCount,
		    batchBuildResult.diagnostics.RejectedCandidateCount,
		    batchBuildResult.diagnostics.RejectedMissingGpuMeshCount,
		    batchBuildResult.diagnostics.RejectedInvalidInstanceGroupCount,
		    batchBuildResult.diagnostics.RejectedInvalidMaterialCount);
	}
}

void RenderSceneDataBuilder::BuildLighting(const RenderSceneSnapshot& sceneSnapshot, RenderSceneData& sceneData) const noexcept
{
	const LightingSnapshot& lightingSnapshot = sceneSnapshot.lighting;
	sceneData.directionalLights.clear();
	sceneData.directionalLights.reserve(lightingSnapshot.directionalLightCount);

	for (std::size_t lightIndex = 0; lightIndex < lightingSnapshot.directionalLightCount; ++lightIndex)
	{
		const DirectionalLightDesc& light = lightingSnapshot.directionalLights[lightIndex];
		DirectionalLight renderLight = {};
		renderLight.direction = light.direction;
		renderLight.intensity = light.intensity;
		renderLight.color = light.color;
		renderLight.castShadow = light.castShadow;
		sceneData.directionalLights.push_back(renderLight);
	}
}

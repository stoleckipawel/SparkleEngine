#include "PCH.h"

#include "RenderSceneDataBuilder.h"

#include "Meshes/GPUMeshCache.h"
#include "ShaderData/MeshInstanceShaderData.h"
#include "Renderer/Public/Debug/RendererCVars.h"
#include "SceneData/RenderSceneData.h"
#include "SceneData/Builders/MeshInstanceBatchBuilder.h"
#include "SceneData/Builders/RenderLightingBuilder.h"
#include "Scene/Meshes/Mesh.h"
#include "SceneData/Caching/MaterialCacheManager.h"
#include "SceneData/Caching/MaterialCacheUtils.h"
#include "SceneData/RenderMeshSnapshotAdapter.h"
#include "Scene/Meshes/MeshSnapshot.h"
#include "Textures/TextureManager.h"
#include "Textures/RendererTexture.h"

#include <cstddef>
#include <unordered_map>
#include <utility>

static const auto g_renderSceneDataBuilderLogger = Logging::GetOrCreateLogger("Renderer.SceneData");

namespace
{
	void CountMeshInstanceWorkload(const std::vector<MeshDraw>& meshInstances, RenderMeshWorkloadSummary& outWorkload) noexcept
	{
		for (const MeshDraw& meshInstance : meshInstances)
		{
			if (meshInstance.Geometry.MeshKind == RenderMeshKind::Skeletal)
			{
				++outWorkload.skinnedInstanceCount;
			}
			else
			{
				++outWorkload.staticInstanceCount;
			}
		}
	}

	void CountMeshBatchWorkload(const std::vector<MeshInstanceBatch>& meshBatches, RenderMeshWorkloadSummary& outWorkload) noexcept
	{
		for (const MeshInstanceBatch& batch : meshBatches)
		{
			if (batch.meshKind == RenderMeshKind::Skeletal)
			{
				++outWorkload.skinnedBatchCount;
			}
			else
			{
				++outWorkload.staticBatchCount;
			}
		}
	}
}

RenderSceneDataBuilder::RenderSceneDataBuilder(
    MaterialCacheManager& materialCache,
    GPUMeshCache& gpuMeshCache,
    TextureManager& textureManager) noexcept :
    m_materialCache(&materialCache), m_gpuMeshCache(&gpuMeshCache), m_textureManager(&textureManager)
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
	BuildSky(sceneSnapshot, sceneData);
	RenderLightingBuilder::Build(sceneSnapshot.lighting, sceneData);
	return sceneData;
}

void RenderSceneDataBuilder::BuildSky(const RenderSceneSnapshot& sceneSnapshot, RenderSceneData& sceneData) const
{
	if (m_textureManager == nullptr)
	{
		return;
	}

	const RendererTexture* skyTexture = nullptr;
	const SceneSkyDesc* sky = sceneSnapshot.sky.sky ? &*sceneSnapshot.sky.sky : nullptr;
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
			skyTexture = m_textureManager->ResolveDefaultSkyTexture();
		}
		else
		{
			skyTexture = m_textureManager->GetSceneTexture(sky->skyTexture.texturePath);
			if (skyTexture == nullptr)
			{
				skyTexture = m_textureManager->GetTexture(TextureId::Checker);
				SPDLOG_LOGGER_ERROR(
				    g_renderSceneDataBuilderLogger,
				    "RenderSceneDataBuilder: level sky texture '{}' is unavailable; using the diagnostic checker texture.",
				    sky->skyTexture.texturePath);
			}
		}
	}

	sceneData.sky.texture = skyTexture != nullptr && *skyTexture ? skyTexture : nullptr;
}

void RenderSceneDataBuilder::BuildMaterials(const RenderSceneSnapshot& sceneSnapshot, RenderSceneData& sceneData) const
{
	if (m_materialCache == nullptr)
	{
		return;
	}

	m_materialCache->BuildMaterials(sceneSnapshot.materials, sceneData);
}

void RenderSceneDataBuilder::BuildMeshInstanceBatches(const RenderSceneSnapshot& sceneSnapshot, RenderSceneData& sceneData)
{
	if (!sceneSnapshot.meshes.HasMeshes() || m_gpuMeshCache == nullptr)
	{
		m_previousMeshWorldMatrices.clear();
		m_previousSkinningMatricesBySkeletonAsset.clear();
		return;
	}

	std::vector<MeshRenderItem> renderItems;
	renderItems.reserve(sceneSnapshot.meshes.meshInstances.size());
	std::vector<DirectX::XMFLOAT4X4> currentMeshWorldMatrices;
	currentMeshWorldMatrices.reserve(sceneSnapshot.meshes.meshInstances.size());

	std::unordered_map<Assets::CookedAssetId, std::uint32_t> jointMatrixOffsets;
	std::unordered_map<Assets::CookedAssetId, std::vector<DirectX::XMFLOAT4X4>> currentSkinningMatricesBySkeletonAsset;
	for (const SceneAnimationPoseSnapshot& pose : sceneSnapshot.animations.poses)
	{
		if (pose.skeletonAssetId == Assets::InvalidCookedAssetId || pose.skinningMatrices.empty())
		{
			continue;
		}

		const auto offset = static_cast<std::uint32_t>(sceneData.jointMatrices.size());
		jointMatrixOffsets.emplace(pose.skeletonAssetId, offset);
		sceneData.jointMatrices.insert(sceneData.jointMatrices.end(), pose.skinningMatrices.begin(), pose.skinningMatrices.end());
		const auto previousIt = m_previousSkinningMatricesBySkeletonAsset.find(pose.skeletonAssetId);
		const std::vector<DirectX::XMFLOAT4X4>& previousSkinningMatrices =
		    previousIt != m_previousSkinningMatricesBySkeletonAsset.end() && previousIt->second.size() == pose.skinningMatrices.size()
		        ? previousIt->second
		        : pose.skinningMatrices;
		sceneData.previousJointMatrices.insert(
		    sceneData.previousJointMatrices.end(),
		    previousSkinningMatrices.begin(),
		    previousSkinningMatrices.end());
		currentSkinningMatricesBySkeletonAsset.emplace(pose.skeletonAssetId, pose.skinningMatrices);
	}

	for (std::uint32_t sourceInstanceIndex = 0; sourceInstanceIndex < static_cast<std::uint32_t>(sceneSnapshot.meshes.meshInstances.size());
	     ++sourceInstanceIndex)
	{
		const MeshInstanceSnapshot& meshInstance = sceneSnapshot.meshes.meshInstances[sourceInstanceIndex];
		currentMeshWorldMatrices.push_back(meshInstance.worldMatrix);
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
		draw.Transform.WorldMatrix = meshInstance.worldMatrix;
		draw.Transform.PreviousWorldMatrix = sourceInstanceIndex < m_previousMeshWorldMatrices.size()
		                                         ? m_previousMeshWorldMatrices[sourceInstanceIndex]
		                                         : meshInstance.worldMatrix;
		draw.Transform.WorldInvTranspose = meshInstance.worldInvTranspose;
		draw.Material.Slot = MaterialCacheUtils::ResolveMaterialSlot(meshInstance.materialHandle, sceneData.materials.size());
		draw.Source.SourceInstanceIndex = sourceInstanceIndex;
		draw.Source.MeshAssetId = meshInstance.meshAssetId;
		draw.Skinning.SkeletonAssetId = meshInstance.skeletonAssetId;
		draw.Skinning.JointMatrixOffset = kInvalidMeshInstanceJointMatrixOffset;
		if (meshInstance.meshKind == SceneMeshKind::Skeletal)
		{
			if (const auto it = jointMatrixOffsets.find(meshInstance.skeletonAssetId); it != jointMatrixOffsets.end())
			{
				draw.Skinning.JointMatrixOffset = it->second;
			}
		}
		draw.Geometry.MeshKind = RenderMeshSnapshotAdapter::ToRenderMeshKind(meshInstance.meshKind);
		draw.Geometry.GpuMesh = gpuMesh;

		renderItems.push_back(
		    MeshRenderItem{
		        .draw = draw,
		        .materialGpuHandle =
		            draw.Material.Slot < sceneData.materials.size() ? sceneData.materials[draw.Material.Slot].gpuHandle : MaterialGpuHandle{},
		        .instanceGroupIndex = RenderMeshSnapshotAdapter::ToRenderMeshInstanceGroupIndex(meshInstance.instanceGroupIndex)});
	}

	const std::vector<RenderMeshInstanceGroup> renderInstanceGroups =
	    RenderMeshSnapshotAdapter::BuildRenderMeshInstanceGroups(sceneSnapshot.meshes);
	MeshInstanceBatchBuilder batchBuilder;
	MeshInstanceBatchBuildResult batchBuildResult = batchBuilder.Build(
	    renderItems,
	    renderInstanceGroups,
	    MeshInstanceBatchBuildOptions{
	        .enableAutoBatching = CVarRendererMeshAutoBatching.Get(),
	        .requireMaterialBindingSet = true,
	        .collectDiagnostics = true});
	sceneData.meshInstances = std::move(batchBuildResult.batchInstances);
	sceneData.meshInstanceBatches = std::move(batchBuildResult.batches);
	m_previousMeshWorldMatrices = std::move(currentMeshWorldMatrices);
	m_previousSkinningMatricesBySkeletonAsset = std::move(currentSkinningMatricesBySkeletonAsset);
	sceneData.meshWorkload = {};
	sceneData.meshWorkload.jointMatrixCount = static_cast<std::uint32_t>(sceneData.jointMatrices.size());
	CountMeshInstanceWorkload(sceneData.meshInstances, sceneData.meshWorkload);
	CountMeshBatchWorkload(sceneData.meshInstanceBatches, sceneData.meshWorkload);

	static bool loggedMissingMeshBatchWarning = false;
	if (!loggedMissingMeshBatchWarning && !sceneSnapshot.meshes.meshInstances.empty() && sceneData.meshInstanceBatches.empty())
	{
		loggedMissingMeshBatchWarning = true;
		SPDLOG_LOGGER_WARN(
		    g_renderSceneDataBuilderLogger,
		    "RenderSceneDataBuilder: scene has {} mesh instances but produced no render batches (candidates={}, rejected={}, "
		    "missingGpuMesh={}, invalidGroup={}, invalidMaterial={}).",
		    sceneSnapshot.meshes.meshInstances.size(),
		    batchBuildResult.diagnostics.CandidateItemCount,
		    batchBuildResult.diagnostics.RejectedCandidateCount,
		    batchBuildResult.diagnostics.RejectedMissingGpuMeshCount,
		    batchBuildResult.diagnostics.RejectedInvalidInstanceGroupCount,
		    batchBuildResult.diagnostics.RejectedInvalidMaterialCount);
	}
}

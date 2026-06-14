#include "PCH.h"

#include "RenderSceneDataBuilder.h"

#include "Meshes/GPUMeshCache.h"
#include "RHI/Public/Resources/MeshInstanceShaderData.h"
#include "Renderer/Public/Debug/RendererCVars.h"
#include "SceneData/RenderSceneData.h"
#include "SceneData/Builders/MeshInstanceBatchBuilder.h"
#include "SceneData/Builders/RenderLightingBuilder.h"
#include "Scene/Meshes/Mesh.h"
#include "SceneData/Caching/MaterialCacheManager.h"
#include "SceneData/Caching/MaterialCacheUtils.h"
#include "SceneData/RenderMeshSnapshotAdapter.h"
#include "Scene/Meshes/MeshSnapshot.h"

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
	RenderLightingBuilder::Build(sceneSnapshot.lighting, sceneData);
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

	std::unordered_map<Assets::CookedAssetId, std::uint32_t> jointMatrixOffsets;
	for (const SceneAnimationPoseSnapshot& pose : sceneSnapshot.animations.poses)
	{
		if (pose.skeletonAssetId == Assets::InvalidCookedAssetId || pose.skinningMatrices.empty())
		{
			continue;
		}

		const auto offset = static_cast<std::uint32_t>(sceneData.jointMatrices.size());
		jointMatrixOffsets.emplace(pose.skeletonAssetId, offset);
		sceneData.jointMatrices.insert(sceneData.jointMatrices.end(), pose.skinningMatrices.begin(), pose.skinningMatrices.end());
	}

	for (std::uint32_t sourceInstanceIndex = 0; sourceInstanceIndex < static_cast<std::uint32_t>(sceneSnapshot.meshes.meshInstances.size());
	     ++sourceInstanceIndex)
	{
		const MeshInstanceSnapshot& meshInstance = sceneSnapshot.meshes.meshInstances[sourceInstanceIndex];
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
		draw.Transform.WorldInvTranspose = meshInstance.worldInvTranspose;
		draw.Material.Slot = MaterialCacheUtils::ResolveMaterialSlot(meshInstance.materialHandle, sceneData.materials.size());
		draw.Source.SourceInstanceIndex = sourceInstanceIndex;
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
		        .materialBindingSet = draw.Material.Slot < sceneData.materials.size() ? sceneData.materials[draw.Material.Slot].textureBindingSet
		                                                                           : nullptr,
		        .instanceGroupIndex = RenderMeshSnapshotAdapter::ToRenderMeshInstanceGroupIndex(meshInstance.instanceGroupIndex)});
	}

	const std::vector<RenderMeshInstanceGroup> renderInstanceGroups = RenderMeshSnapshotAdapter::BuildRenderMeshInstanceGroups(sceneSnapshot.meshes);
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
	sceneData.meshWorkload = {};
	sceneData.meshWorkload.jointMatrixCount = static_cast<std::uint32_t>(sceneData.jointMatrices.size());
	CountMeshInstanceWorkload(sceneData.meshInstances, sceneData.meshWorkload);
	CountMeshBatchWorkload(sceneData.meshInstanceBatches, sceneData.meshWorkload);

	static bool loggedFirstMeshBatchSummary = false;
	static bool loggedMissingMeshBatchWarning = false;
	if (!loggedFirstMeshBatchSummary && !sceneData.meshInstances.empty())
	{
		loggedFirstMeshBatchSummary = true;
		SPDLOG_LOGGER_INFO(
		    g_renderSceneDataBuilderLogger,
		    "RenderSceneDataBuilder: prepared {} renderable mesh instances in {} batches from {} scene mesh instances (staticInstances={}, skinnedInstances={}, staticBatches={}, skinnedBatches={}, jointMatrices={}, rejected={}, missingGpuMesh={}, invalidMaterial={}).",
		    sceneData.meshInstances.size(),
		    sceneData.meshInstanceBatches.size(),
		    sceneSnapshot.meshes.meshInstances.size(),
		    sceneData.meshWorkload.staticInstanceCount,
		    sceneData.meshWorkload.skinnedInstanceCount,
		    sceneData.meshWorkload.staticBatchCount,
		    sceneData.meshWorkload.skinnedBatchCount,
		    sceneData.meshWorkload.jointMatrixCount,
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

#include "PCH.h"

#include "RenderMeshDrawBuilder.h"

#include "Meshes/GPUMeshCache.h"
#include "Renderer/Public/Debug/RendererCVars.h"
#include "Scene/Meshes/Mesh.h"
#include "SceneData/Builders/MeshInstanceBatchBuilder.h"
#include "SceneData/Caching/MaterialCacheUtils.h"
#include "SceneData/RenderMeshClassificationConversion.h"
#include "SceneData/RenderSceneData.h"
#include "SceneData/RenderWorld.h"
#include "ShaderData/MeshInstanceShaderData.h"

#include <utility>

class RenderMeshDrawBuilderOperations final
{
  public:
	static void CountMeshInstanceWorkload(const std::vector<MeshDraw>& meshInstances, RenderMeshWorkloadSummary& workload) noexcept
	{
		for (const MeshDraw& meshInstance : meshInstances)
			meshInstance.Geometry.MeshKind == RenderMeshKind::Skeletal ? ++workload.skinnedInstanceCount
			                                                         : ++workload.staticInstanceCount;
	}

	static void CountMeshBatchWorkload(const std::vector<MeshInstanceBatch>& meshBatches, RenderMeshWorkloadSummary& workload) noexcept
	{
		for (const MeshInstanceBatch& batch : meshBatches)
			batch.meshKind == RenderMeshKind::Skeletal ? ++workload.skinnedBatchCount : ++workload.staticBatchCount;
	}
};

RenderMeshDrawBuilder::RenderMeshDrawBuilder(
    GPUMeshCache& gpuMeshCache) noexcept :
	m_gpuMeshCache(gpuMeshCache)
{
}

void RenderMeshDrawBuilder::ResetHistory() noexcept
{
	m_previousWorldMatrices.clear();
	m_previousSkinningMatrices.clear();
	m_currentSkinningMatrices.clear();
	m_morphBuilder.Reset();
}

void RenderMeshDrawBuilder::Build(const RenderWorld& world, const RenderFrameDynamicData& dynamic, RenderSceneData& sceneData)
{
	if (world.GetProxies().empty())
	{
		ResetHistory();
		return;
	}

	std::map<RenderObjectId, std::uint32_t> jointMatrixOffsets;
	AppendSkinningData(dynamic, sceneData, jointMatrixOffsets);
	m_morphBuilder.Prepare(dynamic, sceneData);

	std::vector<MeshRenderItem> renderItems;
	renderItems.reserve(dynamic.Objects.size());
	std::map<RenderObjectId, DirectX::XMFLOAT4X4> currentWorldMatrices;
	AppendVisibleMeshItems(world, dynamic, jointMatrixOffsets, sceneData, renderItems, currentWorldMatrices);
	BuildBatches(world, std::move(renderItems), sceneData);

	m_previousWorldMatrices = std::move(currentWorldMatrices);
	m_previousSkinningMatrices = std::move(m_currentSkinningMatrices);
	m_currentSkinningMatrices.clear();
	m_morphBuilder.Commit();
}

void RenderMeshDrawBuilder::AppendSkinningData(
	const RenderFrameDynamicData& dynamic,
	RenderSceneData& sceneData,
	std::map<RenderObjectId, std::uint32_t>& outJointMatrixOffsets)
{
	m_currentSkinningMatrices.clear();
	for (const RenderSkinningData& pose : dynamic.Skinning)
	{
		if (!pose.Object.IsValid() || !pose.Skeleton.IsValid() || !pose.Animation.IsValid() || pose.Matrices.empty()) continue;

		const auto offset = static_cast<std::uint32_t>(sceneData.jointMatrices.size());
		outJointMatrixOffsets.emplace(pose.Object, offset);
		sceneData.jointMatrices.insert(sceneData.jointMatrices.end(), pose.Matrices.begin(), pose.Matrices.end());
		const auto previous = m_previousSkinningMatrices.find(pose.Object);
		const auto& previousMatrices = previous != m_previousSkinningMatrices.end() && previous->second.size() == pose.Matrices.size()
		                                   ? previous->second : pose.Matrices;
		sceneData.previousJointMatrices.insert(
		    sceneData.previousJointMatrices.end(), previousMatrices.begin(), previousMatrices.end());
		m_currentSkinningMatrices.emplace(pose.Object, pose.Matrices);
	}
}

void RenderMeshDrawBuilder::AppendVisibleMeshItems(
	const RenderWorld& world,
	const RenderFrameDynamicData& dynamic,
	const std::map<RenderObjectId, std::uint32_t>& jointMatrixOffsets,
	const RenderSceneData& sceneData,
	std::vector<MeshRenderItem>& outItems,
	std::map<RenderObjectId, DirectX::XMFLOAT4X4>& outCurrentWorldMatrices)
{
	for (std::uint32_t sourceIndex = 0; sourceIndex < static_cast<std::uint32_t>(dynamic.Objects.size()); ++sourceIndex)
	{
		const RenderObjectDynamicData& object = dynamic.Objects[sourceIndex];
		if (!object.Visible) continue;
		const RenderProxy* proxy = world.Find(object.Object);
		if (proxy == nullptr || !proxy->Static.Mesh.IsValid()) continue;

		GPUMesh* gpuMesh =
		    m_gpuMeshCache.GetOrUpload(proxy->Static.Mesh);
		if (gpuMesh == nullptr || !gpuMesh->IsValid()) continue;
		outCurrentWorldMatrices.emplace(object.Object, object.WorldMatrix);

		MeshDraw draw = {};
		draw.Transform.WorldMatrix = object.WorldMatrix;
		const auto previousMatrix = m_previousWorldMatrices.find(object.Object);
		draw.Transform.PreviousWorldMatrix = previousMatrix != m_previousWorldMatrices.end()
		                                         ? previousMatrix->second : object.WorldMatrix;
		draw.Transform.WorldInvTranspose = object.WorldInverseTranspose;
		draw.Material.Slot = MaterialCacheUtils::ResolveMaterialSlot(proxy->Static.Material, sceneData.materials.size());
		draw.Source.GpuSceneSlot = proxy->GpuSceneSlot;
		draw.Source.MeshAssetId = proxy->Static.Mesh.GetAssetId();
		draw.Source.MeshGeneration = proxy->Static.Mesh.GetGeneration();
		draw.Skinning.SkeletonAssetId = proxy->Static.Skeleton.GetAssetId();
		draw.Skinning.JointMatrixOffset = kInvalidMeshInstanceJointMatrixOffset;
		if (proxy->Static.MeshKind == SceneMeshKind::Skeletal)
			if (const auto jointOffset = jointMatrixOffsets.find(object.Object); jointOffset != jointMatrixOffsets.end())
				draw.Skinning.JointMatrixOffset = jointOffset->second;
		draw.Geometry.MeshKind = RenderMeshClassificationConversion::ToRenderMeshKind(proxy->Static.MeshKind);
		draw.Geometry.Mesh = gpuMesh->GetHandle();
		draw.Geometry.LocalBoundsMin = gpuMesh->GetLocalBounds().Min;
		draw.Geometry.LocalBoundsMax = gpuMesh->GetLocalBounds().Max;
		draw.Geometry.HasLocalBounds = gpuMesh->GetLocalBounds().Valid;
		m_morphBuilder.Append(
		    object.Object,
		    *gpuMesh,
		    sceneData,
		    draw);

		outItems.push_back({.draw = draw,
		                    .materialGpuHandle = draw.Material.Slot < sceneData.materials.size()
		                                             ? sceneData.materials[draw.Material.Slot].gpuHandle : MaterialGpuHandle{},
		                    .instanceGroupIndex = RenderMeshClassificationConversion::ToRenderMeshInstanceGroupIndex(
		                        proxy->Static.InstanceGroupIndex)});
	}
}

void RenderMeshDrawBuilder::BuildBatches(
	const RenderWorld& world, std::vector<MeshRenderItem> items, RenderSceneData& sceneData) const
{
	std::vector<RenderMeshInstanceGroup> groups;
	groups.reserve(world.GetInstanceGroups().size());
	for (const RenderMeshInstanceGroupData& group : world.GetInstanceGroups())
		groups.push_back({.groupKind = RenderMeshClassificationConversion::ToRenderMeshInstanceGroupKind(group.Kind),
		                  .instanceCount = group.InstanceCount});

	MeshInstanceBatchBuilder builder;
	MeshInstanceBatchBuildResult result = builder.Build(
	    items, groups, {.enableAutoBatching = CVarRendererMeshAutoBatching.Get(),
	                    .requireMaterialBindingSet = true});
	sceneData.meshInstances = std::move(result.batchInstances);
	sceneData.meshInstanceBatches = std::move(result.batches);
	PublishWorkload(sceneData);
}

void RenderMeshDrawBuilder::PublishWorkload(
    RenderSceneData& sceneData) const
{
	sceneData.meshWorkload = {};
	sceneData.meshWorkload.jointMatrixCount = static_cast<std::uint32_t>(sceneData.jointMatrices.size());
	RenderMeshDrawBuilderOperations::CountMeshInstanceWorkload(sceneData.meshInstances, sceneData.meshWorkload);
	RenderMeshDrawBuilderOperations::CountMeshBatchWorkload(sceneData.meshInstanceBatches, sceneData.meshWorkload);
}

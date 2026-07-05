#include "PCH.h"

#include "RayTracing/Acceleration/RayTracingPtlasLogicalUpdateStream.h"

#include "RayTracing/Acceleration/RayTracingPtlasPartitionPlanner.h"
#include "SceneData/RenderSceneData.h"

bool RayTracingPtlasLogicalUpdateStream::ShouldEmitLogicalUpdate(const RayTracingPtlasPartitionEntry& entry) noexcept
{
	return entry.Validation.Valid && (entry.Update.DirtyTransform || entry.Update.MovedPartition);
}

std::array<float, 12> RayTracingPtlasLogicalUpdateStream::BuildInstanceTransform(
    const DirectX::XMFLOAT4X4& worldMatrix) noexcept
{
	return {
	    worldMatrix._11,
	    worldMatrix._12,
	    worldMatrix._13,
	    worldMatrix._14,
	    worldMatrix._21,
	    worldMatrix._22,
	    worldMatrix._23,
	    worldMatrix._24,
	    worldMatrix._31,
	    worldMatrix._32,
	    worldMatrix._33,
	    worldMatrix._34};
}

namespace
{
	RhiPartitionedTlasInstanceFlags ResolveInstanceFlags(const RenderSceneData& sceneData, const MeshDraw& draw) noexcept
	{
		RhiPartitionedTlasInstanceFlags flags = RhiPartitionedTlasInstanceFlags::TriangleFacingCullDisable;
		if (draw.Material.Slot < sceneData.materials.size() && sceneData.materials[draw.Material.Slot].alphaMode == 1u)
		{
			flags = flags | RhiPartitionedTlasInstanceFlags::ForceNoOpaque;
		}
		return flags;
	}
}

RayTracingPtlasLogicalUpdateStreamResult RayTracingPtlasLogicalUpdateStream::Build(
    const RenderSceneData& sceneData,
    const RayTracingPtlasPartitionPlan& partitionPlan) const
{
	RayTracingPtlasLogicalUpdateStreamResult result{};
	result.Records.reserve(partitionPlan.Indices.Entries.size());

	for (const RayTracingPtlasPartitionEntry& entry : partitionPlan.Indices.Entries)
	{
		if (entry.Identity.RenderInstanceIndex >= sceneData.meshInstances.size())
		{
			++result.SkippedInvalidInstanceCount;
			continue;
		}
		if (!ShouldEmitLogicalUpdate(entry))
		{
			continue;
		}

		const MeshDraw& draw = sceneData.meshInstances[entry.Identity.RenderInstanceIndex];
		result.Records.push_back(
		    RayTracingPtlasLogicalUpdateRecord{
		        .Transform = BuildInstanceTransform(draw.Transform.WorldMatrix),
		        .RenderInstanceIndex = entry.Identity.RenderInstanceIndex,
		        .InstanceIndex = entry.Identity.StableInstanceIndex,
		        .PartitionIndex = entry.Assignment.PartitionId,
		        .InstanceID = entry.Identity.RenderInstanceIndex,
		        .InstanceMask = 0xFFu,
		        .InstanceContributionToHitGroupIndex = 0u,
		        .InstanceFlags = ResolveInstanceFlags(sceneData, draw)});
	}

	result.LogicalUpdateCount = static_cast<std::uint32_t>(result.Records.size());
	return result;
}

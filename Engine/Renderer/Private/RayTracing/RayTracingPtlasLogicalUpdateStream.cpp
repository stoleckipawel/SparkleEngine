#include "PCH.h"

#include "RayTracing/RayTracingPtlasLogicalUpdateStream.h"

#include "RayTracing/RayTracingPtlasPartitionPlanner.h"
#include "SceneData/RenderSceneData.h"

bool RayTracingPtlasLogicalUpdateStream::ShouldEmitLogicalUpdate(const RayTracingPtlasPartitionEntry& entry) noexcept
{
	return entry.Validation.Valid && (entry.Update.DirtyTransform || entry.Update.MovedPartition);
}

RhiPartitionedTlasLogicalUpdateFlags RayTracingPtlasLogicalUpdateStream::BuildUpdateFlags(
    const RayTracingPtlasPartitionEntry& entry) noexcept
{
	RhiPartitionedTlasLogicalUpdateFlags flags = RhiPartitionedTlasLogicalUpdateFlags::ValidInstance;
	if (entry.Update.DirtyTransform)
	{
		flags = flags | RhiPartitionedTlasLogicalUpdateFlags::DirtyTransform;
	}
	if (entry.Update.MovedPartition)
	{
		flags = flags | RhiPartitionedTlasLogicalUpdateFlags::MovedPartition;
	}
	if (entry.Update.UsesGlobalPartition)
	{
		flags = flags | RhiPartitionedTlasLogicalUpdateFlags::UsesGlobalPartition;
	}
	return flags;
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
		    RhiPartitionedTlasLogicalUpdateRecord{
		        .Transform = BuildInstanceTransform(draw.Transform.WorldMatrix),
		        .StableInstanceIndex = entry.Identity.StableInstanceIndex,
		        .RenderInstanceIndex = entry.Identity.RenderInstanceIndex,
		        .InstanceIndex = entry.Identity.StableInstanceIndex,
		        .PartitionIndex = entry.Assignment.PartitionId,
		        .PreviousPartitionIndex = entry.Assignment.PreviousPartitionId,
		        .InstanceID = entry.Identity.RenderInstanceIndex,
		        .InstanceMask = 0xFFu,
		        .InstanceContributionToHitGroupIndex = 0u,
		        .AccelerationStructure = 0,
		        .InstanceFlags = ResolveInstanceFlags(sceneData, draw),
		        .UpdateFlags = BuildUpdateFlags(entry)});
	}

	result.LogicalUpdateCount = static_cast<std::uint32_t>(result.Records.size());
	return result;
}

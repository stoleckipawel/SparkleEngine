#include "PCH.h"

#include "RayTracing/Acceleration/RayTracingPtlasPartitionPlanner.h"

#include "Meshes/GPUMesh.h"
#include "Renderer/Public/SceneData/MeshDraw.h"
#include "SceneData/RenderSceneData.h"

#include <algorithm>
#include <cmath>
#include <unordered_set>
#include <utility>
#include <vector>

namespace
{
	struct SceneBounds final
	{
		DirectX::XMFLOAT3 Min = {};
		DirectX::XMFLOAT3 Max = {};
		bool Valid = false;
	};

	struct InstanceBounds final
	{
		DirectX::XMFLOAT3 Min = {};
		DirectX::XMFLOAT3 Max = {};
		bool Valid = false;
	};

	void ExpandSceneBounds(SceneBounds& bounds, const DirectX::XMFLOAT3& point) noexcept
	{
		if (!bounds.Valid)
		{
			bounds.Min = point;
			bounds.Max = point;
			bounds.Valid = true;
			return;
		}

		bounds.Min.x = (std::min)(bounds.Min.x, point.x);
		bounds.Min.y = (std::min)(bounds.Min.y, point.y);
		bounds.Min.z = (std::min)(bounds.Min.z, point.z);
		bounds.Max.x = (std::max)(bounds.Max.x, point.x);
		bounds.Max.y = (std::max)(bounds.Max.y, point.y);
		bounds.Max.z = (std::max)(bounds.Max.z, point.z);
	}

	DirectX::XMFLOAT3 TransformPoint(
	    const DirectX::XMFLOAT3& point,
	    const DirectX::XMFLOAT4X4& worldMatrix) noexcept
	{
		const DirectX::XMVECTOR localPoint = DirectX::XMLoadFloat3(&point);
		const DirectX::XMMATRIX world = DirectX::XMLoadFloat4x4(&worldMatrix);
		DirectX::XMFLOAT3 transformed{};
		DirectX::XMStoreFloat3(&transformed, DirectX::XMVector3TransformCoord(localPoint, world));
		return transformed;
	}

	InstanceBounds ComputeInstanceWorldBounds(const MeshDraw& draw) noexcept
	{
		if (draw.Geometry.GpuMesh == nullptr || !draw.Geometry.GpuMesh->GetLocalBounds().Valid)
		{
			const DirectX::XMFLOAT3 position{draw.Transform.WorldMatrix._41, draw.Transform.WorldMatrix._42, draw.Transform.WorldMatrix._43};
			return InstanceBounds{.Min = position, .Max = position, .Valid = true};
		}

		const GPUMeshBounds& localBounds = draw.Geometry.GpuMesh->GetLocalBounds();
		const DirectX::XMFLOAT3 corners[] = {
		    {localBounds.Min.x, localBounds.Min.y, localBounds.Min.z},
		    {localBounds.Max.x, localBounds.Min.y, localBounds.Min.z},
		    {localBounds.Min.x, localBounds.Max.y, localBounds.Min.z},
		    {localBounds.Max.x, localBounds.Max.y, localBounds.Min.z},
		    {localBounds.Min.x, localBounds.Min.y, localBounds.Max.z},
		    {localBounds.Max.x, localBounds.Min.y, localBounds.Max.z},
		    {localBounds.Min.x, localBounds.Max.y, localBounds.Max.z},
		    {localBounds.Max.x, localBounds.Max.y, localBounds.Max.z}};

		InstanceBounds worldBounds{};
		for (const DirectX::XMFLOAT3& corner : corners)
		{
			const DirectX::XMFLOAT3 transformedCorner = TransformPoint(corner, draw.Transform.WorldMatrix);
			if (!worldBounds.Valid)
			{
				worldBounds.Min = transformedCorner;
				worldBounds.Max = transformedCorner;
				worldBounds.Valid = true;
				continue;
			}

			worldBounds.Min.x = (std::min)(worldBounds.Min.x, transformedCorner.x);
			worldBounds.Min.y = (std::min)(worldBounds.Min.y, transformedCorner.y);
			worldBounds.Min.z = (std::min)(worldBounds.Min.z, transformedCorner.z);
			worldBounds.Max.x = (std::max)(worldBounds.Max.x, transformedCorner.x);
			worldBounds.Max.y = (std::max)(worldBounds.Max.y, transformedCorner.y);
			worldBounds.Max.z = (std::max)(worldBounds.Max.z, transformedCorner.z);
		}
		return worldBounds;
	}

	DirectX::XMFLOAT3 ComputeInstancePartitionPosition(const MeshDraw& draw) noexcept
	{
		const InstanceBounds bounds = ComputeInstanceWorldBounds(draw);
		return DirectX::XMFLOAT3{
		    0.5f * (bounds.Min.x + bounds.Max.x),
		    0.5f * (bounds.Min.y + bounds.Max.y),
		    0.5f * (bounds.Min.z + bounds.Max.z)};
	}

	SceneBounds ComputeSceneBounds(const RenderSceneData& sceneData) noexcept
	{
		SceneBounds bounds{};
		for (const MeshDraw& draw : sceneData.meshInstances)
		{
			const InstanceBounds instanceBounds = ComputeInstanceWorldBounds(draw);
			if (instanceBounds.Valid)
			{
				ExpandSceneBounds(bounds, instanceBounds.Min);
				ExpandSceneBounds(bounds, instanceBounds.Max);
			}
		}
		return bounds;
	}

	std::uint32_t QuantizeAxis(float value, float minValue, float maxValue, std::uint32_t partitionsPerAxis) noexcept
	{
		const float extent = maxValue - minValue;
		if (extent <= 0.0001f || partitionsPerAxis <= 1)
		{
			return 0;
		}

		const float normalized = (value - minValue) / extent;
		const float scaled = std::floor(std::clamp(normalized, 0.0f, 0.999999f) * static_cast<float>(partitionsPerAxis));
		return static_cast<std::uint32_t>(scaled);
	}

	std::uint32_t ComputeGridPartitionId(
	    const DirectX::XMFLOAT3& position,
	    const SceneBounds& bounds,
	    std::uint32_t partitionsPerAxis) noexcept
	{
		const std::uint32_t x = QuantizeAxis(position.x, bounds.Min.x, bounds.Max.x, partitionsPerAxis);
		const std::uint32_t z = QuantizeAxis(position.z, bounds.Min.z, bounds.Max.z, partitionsPerAxis);
		return x + z * partitionsPerAxis;
	}

	DirectX::XMFLOAT3 ComputeGridPartitionCenter(
	    std::uint32_t partitionId,
	    const SceneBounds& bounds,
	    std::uint32_t partitionsPerAxis) noexcept
	{
		if (partitionsPerAxis == 0)
		{
			return bounds.Min;
		}

		const std::uint32_t x = partitionId % partitionsPerAxis;
		const std::uint32_t z = partitionId / partitionsPerAxis;

		const DirectX::XMFLOAT3 extent{
		    bounds.Max.x - bounds.Min.x,
		    bounds.Max.y - bounds.Min.y,
		    bounds.Max.z - bounds.Min.z};
		const float invPartitions = 1.0f / static_cast<float>(partitionsPerAxis);
		return DirectX::XMFLOAT3{
		    bounds.Min.x + (static_cast<float>(x) + 0.5f) * extent.x * invPartitions,
		    0.5f * (bounds.Min.y + bounds.Max.y),
		    bounds.Min.z + (static_cast<float>(z) + 0.5f) * extent.z * invPartitions};
	}

	std::uint64_t ComputeGridPartitionCount(std::uint32_t partitionsPerAxis) noexcept
	{
		const std::uint64_t partitions = partitionsPerAxis;
		return partitions * partitions;
	}

	bool RequiresGlobalPartition(RayTracingPtlasPartitionUpdateMode updateMode) noexcept
	{
		return updateMode == RayTracingPtlasPartitionUpdateMode::AlwaysMoveDynamicToGlobal ||
		       updateMode == RayTracingPtlasPartitionUpdateMode::UpdatePartitionNearbyMoveToGlobalOtherwise;
	}

	float DistanceSquared(const DirectX::XMFLOAT3& lhs, const DirectX::XMFLOAT3& rhs) noexcept
	{
		const float dx = lhs.x - rhs.x;
		const float dy = lhs.y - rhs.y;
		const float dz = lhs.z - rhs.z;
		return dx * dx + dy * dy + dz * dz;
	}
}

RayTracingPtlasPartitionPlan RayTracingPtlasPartitionPlanner::Build(
    const RenderSceneData& sceneData,
    const RayTracingPtlasPartitionPlannerConfig& inputConfig) noexcept
{
	const RayTracingPtlasPartitionPlannerConfig config = SanitizeConfig(inputConfig);
	RayTracingPtlasPartitionPlan plan{};
	plan.Counts.PartitionsPerAxis = config.PartitionsPerAxis;
	plan.Indices.RenderInstanceToEntry.assign(sceneData.meshInstances.size(), kRayTracingPtlasInvalidEntryIndex);

	const std::uint64_t gridPartitionCount64 = ComputeGridPartitionCount(config.PartitionsPerAxis);
	const bool requiresGlobalPartition = RequiresGlobalPartition(config.PartitionUpdateMode);
	const std::uint64_t maxPartitionCount = static_cast<std::uint64_t>((std::numeric_limits<std::uint32_t>::max)());
	plan.Validation.HasPartitionOverflow =
	    gridPartitionCount64 > maxPartitionCount || (requiresGlobalPartition && gridPartitionCount64 == maxPartitionCount);
	const bool hasGlobalPartition = requiresGlobalPartition && !plan.Validation.HasPartitionOverflow;
	plan.Counts.GridPartitionCount =
	    plan.Validation.HasPartitionOverflow ? 0u : static_cast<std::uint32_t>(gridPartitionCount64);
	plan.Counts.PartitionCount = plan.Counts.GridPartitionCount + (hasGlobalPartition ? 1u : 0u);
	plan.Counts.GlobalPartitionIndex = hasGlobalPartition ? plan.Counts.GridPartitionCount : kRayTracingPtlasInvalidEntryIndex;

	if (sceneData.meshInstances.empty())
	{
		m_previousInstances.clear();
		m_partitionStates.clear();
		return plan;
	}

	const SceneBounds bounds = ComputeSceneBounds(sceneData);

	if (!plan.Validation.HasPartitionOverflow && m_partitionStates.size() != plan.Counts.GridPartitionCount)
	{
		m_partitionStates.assign(plan.Counts.GridPartitionCount, PartitionRuntimeState{});
	}
	for (PartitionRuntimeState& partitionState : m_partitionStates)
	{
		partitionState.TouchedThisFrame = false;
		partitionState.FarFromCamera = false;
	}
	const float modeChangeDistanceSquared = config.ModeChangeDistance * config.ModeChangeDistance;
	for (std::uint32_t partitionId = 0; partitionId < plan.Counts.GridPartitionCount && partitionId < m_partitionStates.size(); ++partitionId)
	{
		const DirectX::XMFLOAT3 partitionCenter =
		    ComputeGridPartitionCenter(partitionId, bounds, config.PartitionsPerAxis);
		m_partitionStates[partitionId].FarFromCamera =
		    DistanceSquared(partitionCenter, config.CameraPosition) >= modeChangeDistanceSquared;
	}

	struct ObservedInstance final
	{
		const MeshDraw* Draw = nullptr;
		std::uint32_t RenderInstanceIndex = 0;
		std::uint32_t StableIndex = 0;
		std::uint32_t LocalPartitionId = 0;
		std::uint32_t PreviousPartitionId = 0;
		bool DirtyTransform = false;
		bool GlobalEligible = false;
	};

	std::unordered_set<std::uint32_t> seenStableIndices;
	seenStableIndices.reserve(sceneData.meshInstances.size());
	std::uint32_t maxStableIndex = 0;
	for (const MeshDraw& draw : sceneData.meshInstances)
	{
		maxStableIndex = (std::max)(maxStableIndex, draw.Source.SourceInstanceIndex);
	}
	std::vector<PreviousInstanceState> nextPrevious(static_cast<std::size_t>(maxStableIndex) + 1u);
	std::vector<ObservedInstance> observedInstances;
	observedInstances.reserve(sceneData.meshInstances.size());

	for (std::uint32_t renderInstanceIndex = 0; renderInstanceIndex < static_cast<std::uint32_t>(sceneData.meshInstances.size());
	     ++renderInstanceIndex)
	{
		const MeshDraw& draw = sceneData.meshInstances[renderInstanceIndex];
		const std::uint32_t stableIndex = draw.Source.SourceInstanceIndex;
		const DirectX::XMFLOAT3 position = ComputeInstancePartitionPosition(draw);
		const std::uint32_t localPartitionId =
		    plan.Validation.HasPartitionOverflow
		        ? 0u
		        : ComputeGridPartitionId(position, bounds, config.PartitionsPerAxis);
		const PreviousInstanceState* previous =
		    stableIndex < m_previousInstances.size() && m_previousInstances[stableIndex].Valid ? &m_previousInstances[stableIndex] : nullptr;
		const bool dirtyTransform =
		    previous == nullptr || IsTransformDirty(draw.Transform.WorldMatrix, previous->WorldMatrix, config.TransformDirtyEpsilon);
		const bool globalEligible = hasGlobalPartition && IsGlobalPartitionEligible(draw);
		const bool duplicateStableIndex = seenStableIndices.contains(stableIndex);
		if (!duplicateStableIndex)
		{
			seenStableIndices.insert(stableIndex);
		}
		else
		{
			plan.Validation.HasDuplicateStableIndices = true;
		}
		if (globalEligible && dirtyTransform && localPartitionId < m_partitionStates.size())
		{
			PartitionRuntimeState& partitionState = m_partitionStates[localPartitionId];
			partitionState.TouchedThisFrame = true;
		}
		if (globalEligible && dirtyTransform && previous != nullptr && previous->PartitionId == plan.Counts.GlobalPartitionIndex &&
		    previous->LocalPartitionId < m_partitionStates.size())
		{
			PartitionRuntimeState& originalPartitionState = m_partitionStates[previous->LocalPartitionId];
			originalPartitionState.TouchedThisFrame = true;
		}

		observedInstances.push_back(
		    ObservedInstance{
		        .Draw = &draw,
		        .RenderInstanceIndex = renderInstanceIndex,
		        .StableIndex = stableIndex,
		        .LocalPartitionId = localPartitionId,
		        .PreviousPartitionId = previous != nullptr ? previous->PartitionId : localPartitionId,
		        .DirtyTransform = dirtyTransform,
		        .GlobalEligible = globalEligible});
	}

	if (plan.Validation.HasDuplicateStableIndices)
	{
		m_previousInstances.clear();
		return plan;
	}

	plan.Indices.Entries.reserve(sceneData.meshInstances.size());
	std::vector<std::uint32_t> partitionInstanceCounts(plan.Counts.PartitionCount, 0u);

	for (const ObservedInstance& observed : observedInstances)
	{
		const MeshDraw& draw = *observed.Draw;

		bool shouldUseGlobalPartition = false;
		if (observed.GlobalEligible && !plan.Validation.HasPartitionOverflow)
		{
			const PartitionRuntimeState* partitionState =
			    observed.LocalPartitionId < m_partitionStates.size() ? &m_partitionStates[observed.LocalPartitionId] : nullptr;
			const bool partitionTouched = partitionState != nullptr && partitionState->TouchedThisFrame;
			const bool forceWholePartition = config.MarkAllDynamicInPartition && partitionTouched;
			const bool instanceOrPartitionUpdated = observed.DirtyTransform || forceWholePartition;
			if (config.PartitionUpdateMode == RayTracingPtlasPartitionUpdateMode::AlwaysMoveDynamicToGlobal)
			{
				shouldUseGlobalPartition = instanceOrPartitionUpdated;
			}
			else if (config.PartitionUpdateMode == RayTracingPtlasPartitionUpdateMode::UpdatePartitionNearbyMoveToGlobalOtherwise)
			{
				const bool farFromCamera = partitionState != nullptr && partitionState->FarFromCamera;
				shouldUseGlobalPartition = farFromCamera && instanceOrPartitionUpdated;
			}
		}

		const std::uint32_t partitionId = shouldUseGlobalPartition ? plan.Counts.GlobalPartitionIndex : observed.LocalPartitionId;
		const std::uint32_t previousPartitionId = observed.PreviousPartitionId;
		const bool movedPartition = previousPartitionId != partitionId;
		const bool dirtyTransform = observed.DirtyTransform;

		RayTracingPtlasPartitionEntry entry{
		    .Identity =
		        RayTracingPtlasPartitionEntryIdentity{
		            .StableInstanceIndex = observed.StableIndex,
		            .RenderInstanceIndex = observed.RenderInstanceIndex,
		            .SourceInstanceIndex = draw.Source.SourceInstanceIndex},
		    .Assignment =
		        RayTracingPtlasPartitionAssignment{
		            .PartitionId = partitionId,
		            .PreviousPartitionId = previousPartitionId},
		    .Update =
		        RayTracingPtlasPartitionUpdateState{
		            .DirtyTransform = dirtyTransform,
		            .MovedPartition = movedPartition,
		            .GlobalPartitionEligible = observed.GlobalEligible,
		            .UsesGlobalPartition = shouldUseGlobalPartition},
		    .Valid = !plan.Validation.HasPartitionOverflow};

		plan.Indices.RenderInstanceToEntry[observed.RenderInstanceIndex] = static_cast<std::uint32_t>(plan.Indices.Entries.size());
		plan.Indices.Entries.push_back(entry);
		if (partitionId < partitionInstanceCounts.size())
		{
			const std::uint32_t instanceCount = ++partitionInstanceCounts[partitionId];
			if (partitionId == plan.Counts.GlobalPartitionIndex)
			{
				plan.Counts.MaxInstancesInGlobalPartition =
				    (std::max)(plan.Counts.MaxInstancesInGlobalPartition, instanceCount);
			}
			else
			{
				plan.Counts.MaxInstancesPerPartition = (std::max)(plan.Counts.MaxInstancesPerPartition, instanceCount);
			}
		}
		nextPrevious[observed.StableIndex] = PreviousInstanceState{
		    .WorldMatrix = draw.Transform.WorldMatrix,
		    .LocalPartitionId = observed.LocalPartitionId,
		    .PartitionId = partitionId,
		    .Valid = entry.Valid};
	}

	m_previousInstances = std::move(nextPrevious);
	return plan;
}

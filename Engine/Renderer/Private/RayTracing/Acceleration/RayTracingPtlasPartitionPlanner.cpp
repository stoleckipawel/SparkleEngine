#include "PCH.h"

#include "RayTracing/Acceleration/RayTracingPtlasPartitionPlanner.h"

#include "Meshes/GpuMesh.h"
#include "Renderer/Public/SceneData/MeshDraw.h"
#include "Scene/Preparation/PreparedRenderScene.h"

#include <algorithm>
#include <cmath>
#include <unordered_set>
#include <utility>
#include <vector>

static const auto g_rayTracingPtlasPartitionPlannerLogger = Logging::GetOrCreateLogger("Renderer.RayTracing.PartitionPlanner");

RayTracingPtlasPartitionPlanner::RayTracingPtlasPartitionPlanner() noexcept = default;

struct RayTracingPtlasPartitionPlanner::SceneBounds final
{
	DirectX::XMFLOAT3 Min = {};
	DirectX::XMFLOAT3 Max = {};
	bool Valid = false;
};

struct RayTracingPtlasPartitionPlanner::InstanceBounds final
{
	DirectX::XMFLOAT3 Min = {};
	DirectX::XMFLOAT3 Max = {};
	bool Valid = false;
};

struct RayTracingPtlasPartitionPlanner::ObservedInstance final
{
	const MeshDraw* Draw = nullptr;
	std::uint32_t PrimitiveIndex = 0;
	std::uint32_t GpuSceneSlot = 0;
	std::uint32_t LocalPartitionId = 0;
	std::uint32_t PreviousPartitionId = 0;
	bool DirtyTransform = false;
	bool GlobalEligible = false;
};

struct RayTracingPtlasPartitionPlanner::BuildState final
{
	std::vector<PreviousInstanceState> NextPrevious;
	std::vector<ObservedInstance> ObservedInstances;
};

void RayTracingPtlasPartitionPlanner::ExpandSceneBounds(SceneBounds& bounds, const DirectX::XMFLOAT3& point) noexcept
{
	if (!bounds.Valid)
	{
		bounds.Min = point;
		bounds.Max = point;
		bounds.Valid = true;
		return;
	}

	bounds.Min.x = (std::min) (bounds.Min.x, point.x);
	bounds.Min.y = (std::min) (bounds.Min.y, point.y);
	bounds.Min.z = (std::min) (bounds.Min.z, point.z);
	bounds.Max.x = (std::max) (bounds.Max.x, point.x);
	bounds.Max.y = (std::max) (bounds.Max.y, point.y);
	bounds.Max.z = (std::max) (bounds.Max.z, point.z);
}

DirectX::XMFLOAT3 RayTracingPtlasPartitionPlanner::TransformPoint(
    const DirectX::XMFLOAT3& point,
    const DirectX::XMFLOAT4X4& worldMatrix) noexcept
{
	const DirectX::XMVECTOR localPoint = DirectX::XMLoadFloat3(&point);
	const DirectX::XMMATRIX world = DirectX::XMLoadFloat4x4(&worldMatrix);
	DirectX::XMFLOAT3 transformed{};
	DirectX::XMStoreFloat3(&transformed, DirectX::XMVector3TransformCoord(localPoint, world));
	return transformed;
}

RayTracingPtlasPartitionPlanner::InstanceBounds RayTracingPtlasPartitionPlanner::ComputeInstanceWorldBounds(
    const PreparedRenderScene& preparedScene,
    std::uint32_t primitiveIndex) noexcept
{
	if (primitiveIndex < preparedScene.primitives.size())
	{
		const RenderMeshWorldBounds& prepared = preparedScene.primitives[primitiveIndex].WorldBounds;
		if (prepared.Valid)
		{
			return InstanceBounds{.Min = prepared.Min, .Max = prepared.Max, .Valid = true};
		}
	}
	if (primitiveIndex >= preparedScene.primitives.size())
	{
		return {};
	}
	const MeshDraw& draw = preparedScene.primitives[primitiveIndex].Draw;
	if (!draw.Geometry.Mesh || !draw.Geometry.HasLocalBounds)
	{
		const DirectX::XMFLOAT3 position{draw.Transform.WorldMatrix._41, draw.Transform.WorldMatrix._42, draw.Transform.WorldMatrix._43};
		return InstanceBounds{.Min = position, .Max = position, .Valid = true};
	}

	const DirectX::XMFLOAT3 corners[] = {
	    {draw.Geometry.LocalBoundsMin.x, draw.Geometry.LocalBoundsMin.y, draw.Geometry.LocalBoundsMin.z},
	    {draw.Geometry.LocalBoundsMax.x, draw.Geometry.LocalBoundsMin.y, draw.Geometry.LocalBoundsMin.z},
	    {draw.Geometry.LocalBoundsMin.x, draw.Geometry.LocalBoundsMax.y, draw.Geometry.LocalBoundsMin.z},
	    {draw.Geometry.LocalBoundsMax.x, draw.Geometry.LocalBoundsMax.y, draw.Geometry.LocalBoundsMin.z},
	    {draw.Geometry.LocalBoundsMin.x, draw.Geometry.LocalBoundsMin.y, draw.Geometry.LocalBoundsMax.z},
	    {draw.Geometry.LocalBoundsMax.x, draw.Geometry.LocalBoundsMin.y, draw.Geometry.LocalBoundsMax.z},
	    {draw.Geometry.LocalBoundsMin.x, draw.Geometry.LocalBoundsMax.y, draw.Geometry.LocalBoundsMax.z},
	    {draw.Geometry.LocalBoundsMax.x, draw.Geometry.LocalBoundsMax.y, draw.Geometry.LocalBoundsMax.z}};

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

		worldBounds.Min.x = (std::min) (worldBounds.Min.x, transformedCorner.x);
		worldBounds.Min.y = (std::min) (worldBounds.Min.y, transformedCorner.y);
		worldBounds.Min.z = (std::min) (worldBounds.Min.z, transformedCorner.z);
		worldBounds.Max.x = (std::max) (worldBounds.Max.x, transformedCorner.x);
		worldBounds.Max.y = (std::max) (worldBounds.Max.y, transformedCorner.y);
		worldBounds.Max.z = (std::max) (worldBounds.Max.z, transformedCorner.z);
	}
	return worldBounds;
}

DirectX::XMFLOAT3 RayTracingPtlasPartitionPlanner::ComputeInstancePartitionPosition(
    const PreparedRenderScene& preparedScene,
    std::uint32_t primitiveIndex) noexcept
{
	const InstanceBounds bounds = ComputeInstanceWorldBounds(preparedScene, primitiveIndex);
	return DirectX::XMFLOAT3{
	    0.5f * (bounds.Min.x + bounds.Max.x),
	    0.5f * (bounds.Min.y + bounds.Max.y),
	    0.5f * (bounds.Min.z + bounds.Max.z)};
}

RayTracingPtlasPartitionPlanner::SceneBounds RayTracingPtlasPartitionPlanner::ComputeSceneBounds(
    const PreparedRenderScene& preparedScene) noexcept
{
	SceneBounds bounds{};
	for (const std::uint32_t blasInputIndex : preparedScene.rayTracingWork.PartitionedTlasBlasInputIndices)
	{
		if (blasInputIndex >= preparedScene.rayTracingWork.BlasInputs.size())
		{
			Diagnostics::Fatal(
			    g_rayTracingPtlasPartitionPlannerLogger,
			    __FILE__,
			    __LINE__,
			    "Partition planning references a BLAS input outside the prepared work plan.");
		}
		const std::uint32_t primitiveIndex = preparedScene.rayTracingWork.BlasInputs[blasInputIndex].PrimitiveIndex;
		const InstanceBounds instanceBounds = ComputeInstanceWorldBounds(preparedScene, primitiveIndex);
		if (instanceBounds.Valid)
		{
			ExpandSceneBounds(bounds, instanceBounds.Min);
			ExpandSceneBounds(bounds, instanceBounds.Max);
		}
	}
	return bounds;
}

std::uint32_t RayTracingPtlasPartitionPlanner::QuantizeAxis(
    float value,
    float minValue,
    float maxValue,
    std::uint32_t partitionsPerAxis) noexcept
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

std::uint32_t RayTracingPtlasPartitionPlanner::ComputeGridPartitionId(
    const DirectX::XMFLOAT3& position,
    const SceneBounds& bounds,
    std::uint32_t partitionsPerAxis) noexcept
{
	const std::uint32_t x = QuantizeAxis(position.x, bounds.Min.x, bounds.Max.x, partitionsPerAxis);
	const std::uint32_t z = QuantizeAxis(position.z, bounds.Min.z, bounds.Max.z, partitionsPerAxis);
	return x + z * partitionsPerAxis;
}

DirectX::XMFLOAT3 RayTracingPtlasPartitionPlanner::ComputeGridPartitionCenter(
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

	const DirectX::XMFLOAT3 extent{bounds.Max.x - bounds.Min.x, bounds.Max.y - bounds.Min.y, bounds.Max.z - bounds.Min.z};
	const float invPartitions = 1.0f / static_cast<float>(partitionsPerAxis);
	return DirectX::XMFLOAT3{
	    bounds.Min.x + (static_cast<float>(x) + 0.5f) * extent.x * invPartitions,
	    0.5f * (bounds.Min.y + bounds.Max.y),
	    bounds.Min.z + (static_cast<float>(z) + 0.5f) * extent.z * invPartitions};
}

std::uint64_t RayTracingPtlasPartitionPlanner::ComputeGridPartitionCount(std::uint32_t partitionsPerAxis) noexcept
{
	const std::uint64_t partitions = partitionsPerAxis;
	return partitions * partitions;
}

bool RayTracingPtlasPartitionPlanner::RequiresGlobalPartition(RayTracingPtlasPartitionUpdateMode updateMode) noexcept
{
	return updateMode == RayTracingPtlasPartitionUpdateMode::AlwaysMoveDynamicToGlobal
	    || updateMode == RayTracingPtlasPartitionUpdateMode::UpdatePartitionNearbyMoveToGlobalOtherwise;
}

RayTracingPtlasPartitionPlan RayTracingPtlasPartitionPlanner::InitializePlan(
    const PreparedRenderScene& preparedScene,
    const RayTracingPtlasPartitionPlannerConfig& config) noexcept
{
	RayTracingPtlasPartitionPlan plan{};
	plan.Counts.PartitionsPerAxis = config.PartitionsPerAxis;
	plan.Indices.PrimitiveToEntry.assign(preparedScene.primitives.size(), kRayTracingPtlasInvalidEntryIndex);

	const std::uint64_t gridPartitionCount = ComputeGridPartitionCount(config.PartitionsPerAxis);
	const bool requiresGlobalPartition = RequiresGlobalPartition(config.PartitionUpdateMode);
	const std::uint64_t maximumPartitionCount = (std::numeric_limits<std::uint32_t>::max)();
	plan.Validation.HasPartitionOverflow =
	    gridPartitionCount > maximumPartitionCount || (requiresGlobalPartition && gridPartitionCount == maximumPartitionCount);
	if (plan.Validation.HasPartitionOverflow)
	{
		return plan;
	}

	plan.Counts.GridPartitionCount = static_cast<std::uint32_t>(gridPartitionCount);
	plan.Counts.PartitionCount = plan.Counts.GridPartitionCount + (requiresGlobalPartition ? 1u : 0u);
	plan.Counts.GlobalPartitionIndex = requiresGlobalPartition ? plan.Counts.GridPartitionCount : kRayTracingPtlasInvalidEntryIndex;
	return plan;
}

void RayTracingPtlasPartitionPlanner::PreparePartitionStates(
    const SceneBounds& bounds,
    const RayTracingPtlasPartitionPlannerConfig& config,
    const RayTracingPtlasPartitionPlan& plan)
{
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
	for (std::uint32_t partitionId = 0; partitionId < plan.Counts.GridPartitionCount; ++partitionId)
	{
		const DirectX::XMFLOAT3 partitionCenter = ComputeGridPartitionCenter(partitionId, bounds, config.PartitionsPerAxis);
		m_partitionStates[partitionId].FarFromCamera = DistanceSquared(partitionCenter, config.CameraPosition) >= modeChangeDistanceSquared;
	}
}

void RayTracingPtlasPartitionPlanner::CollectObservedInstances(
    const PreparedRenderScene& preparedScene,
    const SceneBounds& bounds,
    const RayTracingPtlasPartitionPlannerConfig& config,
    RayTracingPtlasPartitionPlan& plan,
    BuildState& state)
{
	const RenderRayTracingWorkPlan& work = preparedScene.rayTracingWork;
	const bool hasGlobalPartition = plan.Counts.GlobalPartitionIndex != kRayTracingPtlasInvalidEntryIndex;
	std::unordered_set<std::uint32_t> seenStableIndices;
	seenStableIndices.reserve(work.PartitionedTlasBlasInputIndices.size());
	state.ObservedInstances.reserve(work.PartitionedTlasBlasInputIndices.size());
	std::uint32_t maximumStableIndex = 0;

	for (const std::uint32_t blasInputIndex : work.PartitionedTlasBlasInputIndices)
	{
		if (blasInputIndex >= work.BlasInputs.size())
		{
			Diagnostics::Fatal(
			    g_rayTracingPtlasPartitionPlannerLogger,
			    __FILE__,
			    __LINE__,
			    "Partition planning references a BLAS input outside the prepared work plan.");
		}
		const RenderRayTracingBlasInput& input = work.BlasInputs[blasInputIndex];
		if (input.PrimitiveIndex >= preparedScene.primitives.size())
		{
			Diagnostics::Fatal(
			    g_rayTracingPtlasPartitionPlannerLogger,
			    __FILE__,
			    __LINE__,
			    "Partition planning references a mesh instance outside the render scene.");
		}
		if (input.GpuSceneSlot == (std::numeric_limits<std::uint32_t>::max)())
		{
			Diagnostics::Fatal(
			    g_rayTracingPtlasPartitionPlannerLogger,
			    __FILE__,
			    __LINE__,
			    "Partition planning cannot index the maximum GPU-scene slot.");
		}

		const MeshDraw& draw = preparedScene.primitives[input.PrimitiveIndex].Draw;
		const DirectX::XMFLOAT3 position = ComputeInstancePartitionPosition(preparedScene, input.PrimitiveIndex);
		const std::uint32_t localPartitionId =
		    plan.Validation.HasPartitionOverflow ? 0u : ComputeGridPartitionId(position, bounds, config.PartitionsPerAxis);
		const PreviousInstanceState* previous =
		    input.GpuSceneSlot < m_previousInstances.size() && m_previousInstances[input.GpuSceneSlot].Valid
		    ? &m_previousInstances[input.GpuSceneSlot]
		    : nullptr;
		const bool dirtyTransform =
		    previous == nullptr || IsTransformDirty(draw.Transform.WorldMatrix, previous->WorldMatrix, config.TransformDirtyEpsilon);
		const bool globalEligible = hasGlobalPartition && IsGlobalPartitionEligible(draw);
		if (!seenStableIndices.insert(input.GpuSceneSlot).second)
		{
			plan.Validation.HasDuplicateStableIndices = true;
		}
		if (globalEligible && dirtyTransform && localPartitionId < m_partitionStates.size())
		{
			m_partitionStates[localPartitionId].TouchedThisFrame = true;
		}
		if (globalEligible && dirtyTransform && previous != nullptr && previous->PartitionId == plan.Counts.GlobalPartitionIndex
		    && previous->LocalPartitionId < m_partitionStates.size())
		{
			m_partitionStates[previous->LocalPartitionId].TouchedThisFrame = true;
		}

		state.ObservedInstances.push_back(
		    ObservedInstance{
		        .Draw = &draw,
		        .PrimitiveIndex = input.PrimitiveIndex,
		        .GpuSceneSlot = input.GpuSceneSlot,
		        .LocalPartitionId = localPartitionId,
		        .PreviousPartitionId = previous != nullptr ? previous->PartitionId : localPartitionId,
		        .DirtyTransform = dirtyTransform,
		        .GlobalEligible = globalEligible});
		maximumStableIndex = (std::max) (maximumStableIndex, input.GpuSceneSlot);
	}

	state.NextPrevious.resize(static_cast<std::size_t>(maximumStableIndex) + 1u);
}

void RayTracingPtlasPartitionPlanner::AppendPlanEntries(
    const RayTracingPtlasPartitionPlannerConfig& config,
    RayTracingPtlasPartitionPlan& plan,
    BuildState& state)
{
	plan.Indices.Entries.reserve(state.ObservedInstances.size());
	std::vector<std::uint32_t> partitionInstanceCounts(plan.Counts.PartitionCount, 0u);

	for (const ObservedInstance& observed : state.ObservedInstances)
	{
		const PartitionRuntimeState* partitionState =
		    observed.LocalPartitionId < m_partitionStates.size() ? &m_partitionStates[observed.LocalPartitionId] : nullptr;
		const bool partitionTouched = partitionState != nullptr && partitionState->TouchedThisFrame;
		const bool instanceOrPartitionUpdated = observed.DirtyTransform || (config.MarkAllDynamicInPartition && partitionTouched);
		const bool moveDynamicToGlobal = config.PartitionUpdateMode == RayTracingPtlasPartitionUpdateMode::AlwaysMoveDynamicToGlobal;
		const bool moveFarDynamicToGlobal =
		    config.PartitionUpdateMode == RayTracingPtlasPartitionUpdateMode::UpdatePartitionNearbyMoveToGlobalOtherwise
		    && partitionState != nullptr && partitionState->FarFromCamera;
		const bool useGlobalPartition = observed.GlobalEligible && !plan.Validation.HasPartitionOverflow && instanceOrPartitionUpdated
		    && (moveDynamicToGlobal || moveFarDynamicToGlobal);
		const std::uint32_t partitionId = useGlobalPartition ? plan.Counts.GlobalPartitionIndex : observed.LocalPartitionId;

		const RayTracingPtlasPartitionEntry entry{
		    .Identity =
		        RayTracingPtlasPartitionEntryIdentity{.PrimitiveIndex = observed.PrimitiveIndex, .GpuSceneSlot = observed.GpuSceneSlot},
		    .Assignment =
		        RayTracingPtlasPartitionAssignment{.PartitionId = partitionId, .PreviousPartitionId = observed.PreviousPartitionId},
		    .Update =
		        RayTracingPtlasPartitionUpdateState{
		            .DirtyTransform = observed.DirtyTransform,
		            .MovedPartition = observed.PreviousPartitionId != partitionId,
		            .GlobalPartitionEligible = observed.GlobalEligible,
		            .UsesGlobalPartition = useGlobalPartition},
		    .Valid = !plan.Validation.HasPartitionOverflow};

		plan.Indices.PrimitiveToEntry[observed.PrimitiveIndex] = static_cast<std::uint32_t>(plan.Indices.Entries.size());
		plan.Indices.Entries.push_back(entry);
		if (partitionId < partitionInstanceCounts.size())
		{
			const std::uint32_t instanceCount = ++partitionInstanceCounts[partitionId];
			std::uint32_t& maximumInstanceCount = partitionId == plan.Counts.GlobalPartitionIndex
			    ? plan.Counts.MaxInstancesInGlobalPartition
			    : plan.Counts.MaxInstancesPerPartition;
			maximumInstanceCount = (std::max) (maximumInstanceCount, instanceCount);
		}
		state.NextPrevious[observed.GpuSceneSlot] = PreviousInstanceState{
		    .WorldMatrix = observed.Draw->Transform.WorldMatrix,
		    .LocalPartitionId = observed.LocalPartitionId,
		    .PartitionId = partitionId,
		    .Valid = entry.Valid};
	}
}

float RayTracingPtlasPartitionPlanner::DistanceSquared(const DirectX::XMFLOAT3& lhs, const DirectX::XMFLOAT3& rhs) noexcept
{
	const float dx = lhs.x - rhs.x;
	const float dy = lhs.y - rhs.y;
	const float dz = lhs.z - rhs.z;
	return dx * dx + dy * dy + dz * dz;
}

RayTracingPtlasPartitionPlan RayTracingPtlasPartitionPlanner::Build(
    const PreparedRenderScene& preparedScene,
    const RayTracingPtlasPartitionPlannerConfig& inputConfig) noexcept
{
	ValidateConfig(inputConfig);
	const RayTracingPtlasPartitionPlannerConfig& config = inputConfig;
	RayTracingPtlasPartitionPlan plan = InitializePlan(preparedScene, config);
	const RenderRayTracingWorkPlan& work = preparedScene.rayTracingWork;
	if (work.PartitionedTlasBlasInputIndices.empty())
	{
		m_previousInstances.clear();
		m_partitionStates.clear();
		return plan;
	}

	const SceneBounds bounds = ComputeSceneBounds(preparedScene);
	PreparePartitionStates(bounds, config, plan);
	BuildState state;
	CollectObservedInstances(preparedScene, bounds, config, plan, state);

	if (plan.Validation.HasDuplicateStableIndices)
	{
		m_previousInstances.clear();
		return plan;
	}

	AppendPlanEntries(config, plan, state);
	m_previousInstances = std::move(state.NextPrevious);
	return plan;
}

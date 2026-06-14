#include "PCH.h"

#include "RayTracing/RayTracingPtlasPartitionPlanner.h"

#include "Renderer/Public/SceneData/MeshDraw.h"
#include "SceneData/RenderSceneData.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_set>
#include <utility>

namespace
{
	constexpr std::uint32_t kInvalidEntryIndex = (std::numeric_limits<std::uint32_t>::max)();
	constexpr std::uint32_t kMaxPlannerPartitionsPerAxis = 64;

	struct SceneBounds final
	{
		DirectX::XMFLOAT3 Min = {};
		DirectX::XMFLOAT3 Max = {};
		bool Valid = false;
	};

	SceneBounds ComputeSceneBounds(const RenderSceneData& sceneData) noexcept
	{
		SceneBounds bounds{};
		for (const MeshDraw& draw : sceneData.meshInstances)
		{
			const DirectX::XMFLOAT3 position{draw.Transform.WorldMatrix._41, draw.Transform.WorldMatrix._42, draw.Transform.WorldMatrix._43};
			if (!bounds.Valid)
			{
				bounds.Min = position;
				bounds.Max = position;
				bounds.Valid = true;
				continue;
			}

			bounds.Min.x = (std::min)(bounds.Min.x, position.x);
			bounds.Min.y = (std::min)(bounds.Min.y, position.y);
			bounds.Min.z = (std::min)(bounds.Min.z, position.z);
			bounds.Max.x = (std::max)(bounds.Max.x, position.x);
			bounds.Max.y = (std::max)(bounds.Max.y, position.y);
			bounds.Max.z = (std::max)(bounds.Max.z, position.z);
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
		const std::uint32_t y = QuantizeAxis(position.y, bounds.Min.y, bounds.Max.y, partitionsPerAxis);
		const std::uint32_t z = QuantizeAxis(position.z, bounds.Min.z, bounds.Max.z, partitionsPerAxis);
		return x + y * partitionsPerAxis + z * partitionsPerAxis * partitionsPerAxis;
	}
}

const RayTracingPtlasPartitionEntry* RayTracingPtlasPartitionPlan::FindByRenderInstance(std::uint32_t renderInstanceIndex) const noexcept
{
	if (renderInstanceIndex >= Indices.RenderInstanceToEntry.size())
	{
		return nullptr;
	}

	const std::uint32_t entryIndex = Indices.RenderInstanceToEntry[renderInstanceIndex];
	return entryIndex != kInvalidEntryIndex && entryIndex < Indices.Entries.size() ? &Indices.Entries[entryIndex] : nullptr;
}

std::uint32_t RayTracingPtlasPartitionPlan::GetPackedDebugVisualizationDataForRenderInstance(
    std::uint32_t renderInstanceIndex) const noexcept
{
	if (const RayTracingPtlasPartitionEntry* entry = FindByRenderInstance(renderInstanceIndex))
	{
		return entry->DebugVisualization.PackedData;
	}
	return kRayTracingPtlasPartitionDebugInvalid;
}

RayTracingPtlasPartitionPlan RayTracingPtlasPartitionPlanner::Build(
    const RenderSceneData& sceneData,
    const RayTracingPtlasPartitionPlannerConfig& inputConfig) noexcept
{
	const RayTracingPtlasPartitionPlannerConfig config = SanitizeConfig(inputConfig);
	RayTracingPtlasPartitionPlan plan{};
	plan.Counts.CandidateInstanceCount = static_cast<std::uint32_t>(sceneData.meshInstances.size());
	plan.Indices.RenderInstanceToEntry.assign(sceneData.meshInstances.size(), kInvalidEntryIndex);

	const std::uint64_t gridPartitionCount64 =
	    static_cast<std::uint64_t>(config.PartitionsPerAxis) * config.PartitionsPerAxis * config.PartitionsPerAxis;
	plan.Validation.HasPartitionOverflow = gridPartitionCount64 > kRayTracingPtlasPartitionDebugPartitionMask;
	plan.Counts.GridPartitionCount =
	    plan.Validation.HasPartitionOverflow ? 0u : static_cast<std::uint32_t>(gridPartitionCount64);
	plan.Counts.PartitionCount =
	    plan.Counts.GridPartitionCount + (config.EnableGlobalPartition && !plan.Validation.HasPartitionOverflow ? 1u : 0u);

	if (sceneData.meshInstances.empty())
	{
		m_previousInstances.clear();
		return plan;
	}

	const SceneBounds bounds = ComputeSceneBounds(sceneData);
	std::unordered_set<std::uint32_t> seenStableIndices;
	seenStableIndices.reserve(sceneData.meshInstances.size());
	std::uint32_t maxStableIndex = 0;
	for (const MeshDraw& draw : sceneData.meshInstances)
	{
		maxStableIndex = (std::max)(maxStableIndex, draw.Source.SourceInstanceIndex);
	}
	std::vector<PreviousInstanceState> nextPrevious(static_cast<std::size_t>(maxStableIndex) + 1u);
	plan.Indices.Entries.reserve(sceneData.meshInstances.size());

	for (std::uint32_t renderInstanceIndex = 0; renderInstanceIndex < static_cast<std::uint32_t>(sceneData.meshInstances.size());
	     ++renderInstanceIndex)
	{
		const MeshDraw& draw = sceneData.meshInstances[renderInstanceIndex];
		const std::uint32_t stableIndex = draw.Source.SourceInstanceIndex;
		const DirectX::XMFLOAT3 position = ExtractTranslation(draw.Transform.WorldMatrix);
		const std::uint32_t gridPartitionId =
		    plan.Validation.HasPartitionOverflow ? 0u : ComputeGridPartitionId(position, bounds, config.PartitionsPerAxis);
		const bool globalEligible = config.EnableGlobalPartition && IsGlobalPartitionEligible(draw);
		const PreviousInstanceState* previous =
		    stableIndex < m_previousInstances.size() && m_previousInstances[stableIndex].Valid ? &m_previousInstances[stableIndex] : nullptr;
		const bool dirtyTransform =
		    previous == nullptr || IsTransformDirty(draw.Transform.WorldMatrix, previous->WorldMatrix, config.TransformDirtyEpsilon);
		const bool usesGlobalPartition = globalEligible && dirtyTransform && !plan.Validation.HasPartitionOverflow;
		const std::uint32_t partitionId = usesGlobalPartition ? plan.Counts.GridPartitionCount : gridPartitionId;
		const std::uint32_t previousPartitionId = previous != nullptr ? previous->PartitionId : partitionId;
		const bool movedPartition = previous != nullptr && previousPartitionId != partitionId;

		RayTracingPtlasPartitionEntry entry{
		    .Identity =
		        RayTracingPtlasPartitionEntryIdentity{
		            .StableInstanceIndex = stableIndex,
		            .RenderInstanceIndex = renderInstanceIndex,
		            .SourceInstanceIndex = draw.Source.SourceInstanceIndex},
		    .Assignment =
		        RayTracingPtlasPartitionAssignment{
		            .PartitionId = partitionId,
		            .PreviousPartitionId = previousPartitionId},
		    .Update =
		        RayTracingPtlasPartitionUpdateState{
		            .DirtyTransform = dirtyTransform,
		            .MovedPartition = movedPartition,
		            .GlobalPartitionEligible = globalEligible,
		            .UsesGlobalPartition = usesGlobalPartition},
		    .Validation =
		        RayTracingPtlasPartitionValidation{
		            .DuplicateStableIndex = seenStableIndices.contains(stableIndex),
		            .Valid = !plan.Validation.HasPartitionOverflow}};

		if (entry.Validation.DuplicateStableIndex)
		{
			++plan.Counts.DuplicateStableIndexCount;
			plan.Validation.HasDuplicateStableIndices = true;
			entry.Validation.Valid = false;
		}
		else
		{
			seenStableIndices.insert(stableIndex);
		}

		if (entry.Assignment.PartitionId > kRayTracingPtlasPartitionDebugPartitionMask)
		{
			plan.Validation.HasInvalidPartition = true;
			entry.Validation.Valid = false;
		}
		if (entry.Update.DirtyTransform)
		{
			++plan.Counts.DirtyTransformCount;
		}
		if (entry.Update.MovedPartition)
		{
			++plan.Counts.MovedPartitionCount;
		}
		if (entry.Update.GlobalPartitionEligible)
		{
			++plan.Counts.GlobalPartitionEligibleCount;
		}
		if (entry.Update.UsesGlobalPartition)
		{
			++plan.Counts.GlobalPartitionInstanceCount;
		}

		entry.DebugVisualization.PackedData = PackDebugVisualizationData(entry);
		plan.Indices.RenderInstanceToEntry[renderInstanceIndex] = static_cast<std::uint32_t>(plan.Indices.Entries.size());
		plan.Indices.Entries.push_back(entry);
		nextPrevious[stableIndex] = PreviousInstanceState{
		    .WorldMatrix = draw.Transform.WorldMatrix,
		    .PartitionId = partitionId,
		    .Valid = entry.Validation.Valid};
	}

	m_previousInstances = std::move(nextPrevious);
	return plan;
}

void RayTracingPtlasPartitionPlanner::Clear() noexcept
{
	m_previousInstances.clear();
}

RayTracingPtlasPartitionPlannerConfig RayTracingPtlasPartitionPlanner::SanitizeConfig(RayTracingPtlasPartitionPlannerConfig config) noexcept
{
	config.PartitionsPerAxis = std::clamp(config.PartitionsPerAxis, 1u, kMaxPlannerPartitionsPerAxis);
	config.TransformDirtyEpsilon = (std::max)(config.TransformDirtyEpsilon, 0.0f);
	return config;
}

DirectX::XMFLOAT3 RayTracingPtlasPartitionPlanner::ExtractTranslation(const DirectX::XMFLOAT4X4& worldMatrix) noexcept
{
	return DirectX::XMFLOAT3{worldMatrix._41, worldMatrix._42, worldMatrix._43};
}

bool RayTracingPtlasPartitionPlanner::IsTransformDirty(
    const DirectX::XMFLOAT4X4& current,
    const DirectX::XMFLOAT4X4& previous,
    float epsilon) noexcept
{
	const float* currentValues = &current._11;
	const float* previousValues = &previous._11;
	for (std::size_t index = 0; index < 16; ++index)
	{
		if (std::abs(currentValues[index] - previousValues[index]) > epsilon)
		{
			return true;
		}
	}
	return false;
}

bool RayTracingPtlasPartitionPlanner::IsGlobalPartitionEligible(const MeshDraw& draw) noexcept
{
	return draw.Geometry.MeshKind != RenderMeshKind::Static;
}

std::uint32_t RayTracingPtlasPartitionPlanner::PackDebugVisualizationData(const RayTracingPtlasPartitionEntry& entry) noexcept
{
	std::uint32_t packedDebugVisualizationData = entry.Assignment.PartitionId & kRayTracingPtlasPartitionDebugPartitionMask;
	if (entry.Update.DirtyTransform)
	{
		packedDebugVisualizationData |= kRayTracingPtlasPartitionDebugDirtyTransform;
	}
	if (entry.Update.MovedPartition)
	{
		packedDebugVisualizationData |= kRayTracingPtlasPartitionDebugMovedPartition;
	}
	if (entry.Update.UsesGlobalPartition)
	{
		packedDebugVisualizationData |= kRayTracingPtlasPartitionDebugGlobalPartition;
	}
	if (!entry.Validation.Valid)
	{
		packedDebugVisualizationData |= kRayTracingPtlasPartitionDebugInvalid;
	}
	return packedDebugVisualizationData;
}

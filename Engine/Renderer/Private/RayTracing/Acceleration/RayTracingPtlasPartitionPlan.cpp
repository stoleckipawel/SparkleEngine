#include "PCH.h"

#include "RayTracing/Acceleration/RayTracingPtlasPartitionPlanner.h"

const RayTracingPtlasPartitionEntry* RayTracingPtlasPartitionPlan::FindByRenderInstance(std::uint32_t renderInstanceIndex) const noexcept
{
	if (renderInstanceIndex >= Indices.RenderInstanceToEntry.size())
	{
		return nullptr;
	}

	const std::uint32_t entryIndex = Indices.RenderInstanceToEntry[renderInstanceIndex];
	return entryIndex != kRayTracingPtlasInvalidEntryIndex && entryIndex < Indices.Entries.size() ? &Indices.Entries[entryIndex] : nullptr;
}

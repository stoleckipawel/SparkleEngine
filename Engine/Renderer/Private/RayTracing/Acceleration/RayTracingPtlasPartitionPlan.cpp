#include "PCH.h"

#include "RayTracing/Acceleration/RayTracingPtlasPartitionPlanner.h"

const RayTracingPtlasPartitionEntry* RayTracingPtlasPartitionPlan::FindByPrimitive(std::uint32_t primitiveIndex) const noexcept
{
	if (primitiveIndex >= Indices.PrimitiveToEntry.size())
	{
		return nullptr;
	}

	const std::uint32_t entryIndex = Indices.PrimitiveToEntry[primitiveIndex];
	return entryIndex != kRayTracingPtlasInvalidEntryIndex && entryIndex < Indices.Entries.size() ? &Indices.Entries[entryIndex] : nullptr;
}

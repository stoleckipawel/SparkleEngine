#include "PCH.h"

#include "RayTracing/RhiRayTracingDesc.h"

void PopulateStandardRayTracingCapabilityGroups(RhiRayTracingCapabilities& capabilities) noexcept
{
	capabilities.Groups.ClassicTlas = RhiClassicTlasCapabilities{
	    .SupportsClassicTlasBuild = capabilities.SupportsAccelerationStructure,
	    .SupportsClassicTlasUpdate = capabilities.SupportsAccelerationStructure,
	    .SupportsGpuReadableInstanceBuffer = capabilities.SupportsAccelerationStructure,
	    .InstanceDescSizeInBytes = capabilities.InstanceDescSizeInBytes};
}

const char* RhiRayTracingTopLevelProviderToString(ERhiRayTracingTopLevelProvider provider) noexcept
{
	switch (provider)
	{
		case ERhiRayTracingTopLevelProvider::ClassicTlas:
			return "ClassicTlas";
		case ERhiRayTracingTopLevelProvider::PartitionedTlas:
			return "PartitionedTlas";
		case ERhiRayTracingTopLevelProvider::None:
		default:
			return "None";
	}
}

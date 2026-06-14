#include "PCH.h"

#include "RayTracing/RhiRayTracingDesc.h"

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

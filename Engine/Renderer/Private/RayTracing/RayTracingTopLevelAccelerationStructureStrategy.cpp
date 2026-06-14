#include "PCH.h"

#include "RayTracing/RayTracingTopLevelAccelerationStructureStrategy.h"

#include "RHI/Public/CVars/RHICVars.h"
#include "RayTracing/RayTracingCapabilityReport.h"
#include "RayTracing/RayTracingClassicTlasStrategy.h"
#include "RayTracing/RayTracingPartitionedTlasStrategy.h"

RayTracingTopLevelAccelerationStructureStrategy::RayTracingTopLevelAccelerationStructureStrategy() noexcept = default;

RayTracingTopLevelAccelerationStructureStrategy::~RayTracingTopLevelAccelerationStructureStrategy() noexcept = default;

std::unique_ptr<RayTracingTopLevelAccelerationStructureStrategy> CreateRayTracingTopLevelAccelerationStructureStrategy(
    RenderHardwareInterface& renderHardwareInterface,
    const RayTracingCapabilityReport& capabilityReport) noexcept
{
	if (CVarRhiRayTracingPreferPartitionedTlas.Get() && capabilityReport.PartitionedTlas.Supported)
	{
		return std::make_unique<RayTracingPartitionedTlasStrategy>(renderHardwareInterface, capabilityReport);
	}

	return std::make_unique<RayTracingClassicTlasStrategy>(renderHardwareInterface);
}

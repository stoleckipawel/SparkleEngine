#include "PCH.h"

#include "RayTracing/Acceleration/RayTracingTopLevelAccelerationStructureStrategy.h"

#include "RayTracing/RayTracingCapabilityReport.h"
#include "RayTracing/Acceleration/RayTracingClassicTlasStrategy.h"
#include "RayTracing/Acceleration/RayTracingPartitionedTlasStrategy.h"

RayTracingTopLevelAccelerationStructureStrategy::RayTracingTopLevelAccelerationStructureStrategy() noexcept = default;

RayTracingTopLevelAccelerationStructureStrategy::~RayTracingTopLevelAccelerationStructureStrategy() noexcept = default;

std::unique_ptr<RayTracingTopLevelAccelerationStructureStrategy> CreateRayTracingTopLevelAccelerationStructureStrategy(
    RenderHardwareInterface& renderHardwareInterface,
    const RayTracingCapabilityReport& capabilityReport) noexcept
{
	if (capabilityReport.TopLevelProvider.SelectedProvider == ERhiRayTracingTopLevelProvider::PartitionedTlas)
	{
		return std::make_unique<RayTracingPartitionedTlasStrategy>(renderHardwareInterface, capabilityReport);
	}

	return std::make_unique<RayTracingClassicTlasStrategy>(renderHardwareInterface);
}

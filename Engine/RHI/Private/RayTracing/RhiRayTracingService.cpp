#include "PCH.h"

#include "RayTracing/RhiRayTracingService.h"

RhiRayTracingAccelerationStructurePrebuildInfo RhiRayTracingService::GetTopLevelAccelerationStructurePrebuildInfo(
    std::uint32_t instanceCount,
    ERhiClassicTlasBuildFlags buildFlags) const noexcept
{
	return GetClassicTlasService().GetClassicTopLevelAccelerationStructurePrebuildInfo(instanceCount, buildFlags);
}

RhiPartitionedTlasBuildSizes RhiRayTracingService::GetPartitionedTopLevelAccelerationStructureBuildSizes(
    const RhiPartitionedTlasDesc& desc) const noexcept
{
	return GetPartitionedTlasService().GetPartitionedTopLevelAccelerationStructureBuildSizes(desc);
}

RhiOwnedResourceHandle RhiRayTracingService::CreatePartitionedTopLevelAccelerationStructureBuffer(
    const RhiPartitionedTlasBuildSizes& sizes,
    std::wstring_view debugName)
{
	return GetPartitionedTlasService().CreatePartitionedTopLevelAccelerationStructureBuffer(sizes, debugName);
}

RhiOwnedResourceHandle RhiRayTracingService::CreatePartitionedTopLevelAccelerationStructureOperationBuffer(
    const RhiPartitionedTlasOperationPackDesc& operationPack,
    std::wstring_view debugName)
{
	return GetPartitionedTlasService().CreatePartitionedTopLevelAccelerationStructureOperationBuffer(operationPack, debugName);
}

RhiPartitionedTlasOperationBufferLayout RhiRayTracingService::GetPartitionedTopLevelAccelerationStructureOperationBufferLayout(
    const RhiPartitionedTlasDesc& desc) const noexcept
{
	return GetPartitionedTlasService().GetPartitionedTopLevelAccelerationStructureOperationBufferLayout(desc);
}

RhiOwnedResourceHandle RhiRayTracingService::CreateRayTracingInstanceBuffer(
    const RhiRayTracingInstanceDesc* instances,
    std::uint32_t instanceCount,
    std::wstring_view debugName)
{
	return GetClassicTlasService().CreateClassicTopLevelAccelerationStructureInstanceBuffer(instances, instanceCount, debugName);
}

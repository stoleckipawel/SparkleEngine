#include "PCH.h"

#include "RayTracing/RhiPartitionedTlasService.h"

RhiPartitionedTlasBuildSizes RhiPartitionedTlasService::GetPartitionedTopLevelAccelerationStructureBuildSizes(
    const RhiPartitionedTlasDesc&) const noexcept
{
	return {};
}

RhiOwnedResourceHandle RhiPartitionedTlasService::CreatePartitionedTopLevelAccelerationStructureBuffer(
    const RhiPartitionedTlasBuildSizes&,
    std::wstring_view)
{
	return {};
}

RhiOwnedResourceHandle RhiPartitionedTlasService::CreatePartitionedTopLevelAccelerationStructureOperationBuffer(
    const RhiPartitionedTlasOperationPackDesc&,
    std::wstring_view)
{
	return {};
}

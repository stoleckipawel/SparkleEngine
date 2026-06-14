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

RhiOwnedResourceHandle RhiPartitionedTlasService::CreatePartitionedTopLevelAccelerationStructureLogicalUpdateBuffer(
    const RhiPartitionedTlasLogicalUpdateBufferDesc&,
    const RhiPartitionedTlasLogicalUpdateRecord*,
    std::uint32_t,
    std::wstring_view)
{
	return {};
}

RhiPartitionedTlasGpuOperationBufferLayout
RhiPartitionedTlasService::GetPartitionedTopLevelAccelerationStructureGpuOperationBufferLayout(
    const RhiPartitionedTlasDesc&) const noexcept
{
	return {};
}

RhiOwnedResourceHandle RhiPartitionedTlasService::CreatePartitionedTopLevelAccelerationStructureGpuOperationBuffer(
    const RhiPartitionedTlasGpuOperationBufferDesc&,
    std::wstring_view)
{
	return {};
}

bool RhiPartitionedTlasService::PackPartitionedTopLevelAccelerationStructureGpuOperations(
    RenderCommandList&,
    const RhiPartitionedTlasGpuOperationPackDesc&) noexcept
{
	return false;
}

#pragma once

#include "../Interop/RhiNativeHandles.h"
#include "../RHIAPI.h"
#include "RhiPartitionedTlasDesc.h"

#include <string_view>

class RenderCommandList;

class SPARKLE_RHI_API RhiPartitionedTlasService
{
  public:
	virtual ~RhiPartitionedTlasService() noexcept = default;

	virtual RhiPartitionedTlasBuildSizes GetPartitionedTopLevelAccelerationStructureBuildSizes(
	    const RhiPartitionedTlasDesc&) const noexcept;
	virtual RhiOwnedResourceHandle CreatePartitionedTopLevelAccelerationStructureBuffer(
	    const RhiPartitionedTlasBuildSizes&,
	    std::wstring_view);
	virtual RhiOwnedResourceHandle CreatePartitionedTopLevelAccelerationStructureOperationBuffer(
	    const RhiPartitionedTlasOperationPackDesc&,
	    std::wstring_view);
	virtual RhiOwnedResourceHandle CreatePartitionedTopLevelAccelerationStructureLogicalUpdateBuffer(
	    const RhiPartitionedTlasLogicalUpdateBufferDesc&,
	    const RhiPartitionedTlasLogicalUpdateRecord*,
	    std::uint32_t,
	    std::wstring_view);
	virtual RhiPartitionedTlasGpuOperationBufferLayout GetPartitionedTopLevelAccelerationStructureGpuOperationBufferLayout(
	    const RhiPartitionedTlasDesc&) const noexcept;
	virtual RhiOwnedResourceHandle CreatePartitionedTopLevelAccelerationStructureGpuOperationBuffer(
	    const RhiPartitionedTlasGpuOperationBufferDesc&,
	    std::wstring_view);
	virtual bool PackPartitionedTopLevelAccelerationStructureGpuOperations(
	    RenderCommandList&,
	    const RhiPartitionedTlasGpuOperationPackDesc&) noexcept;
};

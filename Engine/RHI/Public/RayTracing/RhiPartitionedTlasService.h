#pragma once

#include "../Resources/RhiResourceHandles.h"
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
	virtual RhiPartitionedTlasOperationBufferLayout GetPartitionedTopLevelAccelerationStructureOperationBufferLayout(
	    const RhiPartitionedTlasDesc&) const noexcept;
};

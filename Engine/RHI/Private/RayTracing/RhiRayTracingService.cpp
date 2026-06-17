#include "PCH.h"

#include "RayTracing/RhiRayTracingService.h"

#include <cstdint>
#include <string_view>

namespace RhiRayTracingServiceFallback
{
	class NullClassicTlasService final : public RhiClassicTlasService
	{
	  public:
		RhiRayTracingAccelerationStructurePrebuildInfo GetClassicTopLevelAccelerationStructurePrebuildInfo(
		    std::uint32_t,
		    ERhiClassicTlasBuildFlags) const noexcept override
		{
			return {};
		}

		RhiOwnedResourceHandle CreateClassicTopLevelAccelerationStructureInstanceBuffer(
		    const RhiRayTracingInstanceDesc*,
		    std::uint32_t,
		    std::wstring_view) override
		{
			return {};
		}
	};

	class NullPartitionedTlasService final : public RhiPartitionedTlasService
	{
	};

	NullClassicTlasService& GetNullClassicTlasService() noexcept
	{
		static NullClassicTlasService service;
		return service;
	}

	NullPartitionedTlasService& GetNullPartitionedTlasService() noexcept
	{
		static NullPartitionedTlasService service;
		return service;
	}
}  // namespace RhiRayTracingServiceFallback

RhiClassicTlasService& RhiRayTracingService::GetClassicTlasService() noexcept
{
	return RhiRayTracingServiceFallback::GetNullClassicTlasService();
}

const RhiClassicTlasService& RhiRayTracingService::GetClassicTlasService() const noexcept
{
	return RhiRayTracingServiceFallback::GetNullClassicTlasService();
}

RhiPartitionedTlasService& RhiRayTracingService::GetPartitionedTlasService() noexcept
{
	return RhiRayTracingServiceFallback::GetNullPartitionedTlasService();
}

const RhiPartitionedTlasService& RhiRayTracingService::GetPartitionedTlasService() const noexcept
{
	return RhiRayTracingServiceFallback::GetNullPartitionedTlasService();
}

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

RhiOwnedResourceHandle RhiRayTracingService::CreatePartitionedTopLevelAccelerationStructureLogicalUpdateBuffer(
    const RhiPartitionedTlasLogicalUpdateBufferDesc& desc,
    const RhiPartitionedTlasLogicalUpdateRecord* records,
    std::uint32_t recordCount,
    std::wstring_view debugName)
{
	return GetPartitionedTlasService().CreatePartitionedTopLevelAccelerationStructureLogicalUpdateBuffer(
	    desc,
	    records,
	    recordCount,
	    debugName);
}

RhiPartitionedTlasGpuOperationBufferLayout
RhiRayTracingService::GetPartitionedTopLevelAccelerationStructureGpuOperationBufferLayout(
    const RhiPartitionedTlasDesc& desc) const noexcept
{
	return GetPartitionedTlasService().GetPartitionedTopLevelAccelerationStructureGpuOperationBufferLayout(desc);
}

RhiOwnedResourceHandle RhiRayTracingService::CreatePartitionedTopLevelAccelerationStructureGpuOperationBuffer(
    const RhiPartitionedTlasGpuOperationBufferDesc& desc,
    std::wstring_view debugName)
{
	return GetPartitionedTlasService().CreatePartitionedTopLevelAccelerationStructureGpuOperationBuffer(desc, debugName);
}

bool RhiRayTracingService::PackPartitionedTopLevelAccelerationStructureGpuOperations(
    RenderCommandList& commandList,
    const RhiPartitionedTlasGpuOperationPackDesc& desc) noexcept
{
	return GetPartitionedTlasService().PackPartitionedTopLevelAccelerationStructureGpuOperations(commandList, desc);
}

RhiOwnedResourceHandle RhiRayTracingService::CreateRayTracingInstanceBuffer(
    const RhiRayTracingInstanceDesc* instances,
    std::uint32_t instanceCount,
    std::wstring_view debugName)
{
	return GetClassicTlasService().CreateClassicTopLevelAccelerationStructureInstanceBuffer(instances, instanceCount, debugName);
}

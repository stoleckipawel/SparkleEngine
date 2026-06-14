#pragma once

#include "../Interop/RhiNativeHandles.h"
#include "../Memory/RhiMemoryTypes.h"
#include "../RayTracing/RhiClassicTlasService.h"
#include "../RayTracing/RhiPartitionedTlasService.h"
#include "../RayTracing/RhiRayTracingDesc.h"
#include "../Resources/RhiResourceDesc.h"
#include "../RHIAPI.h"

#include <cstdint>
#include <string_view>

class RenderCommandList;

class SPARKLE_RHI_API RhiRayTracingService
{
  public:
	virtual ~RhiRayTracingService() noexcept = default;

	virtual RhiClassicTlasService& GetClassicTlasService() noexcept;
	virtual const RhiClassicTlasService& GetClassicTlasService() const noexcept;
	virtual RhiPartitionedTlasService& GetPartitionedTlasService() noexcept;
	virtual const RhiPartitionedTlasService& GetPartitionedTlasService() const noexcept;
	virtual RhiRayTracingCapabilities GetRayTracingCapabilities() const noexcept = 0;
	virtual RhiRayTracingAccelerationStructurePrebuildInfo GetBottomLevelAccelerationStructurePrebuildInfo(
	    const RhiRayTracingGeometryDesc& geometry) const noexcept = 0;
	virtual RhiRayTracingAccelerationStructurePrebuildInfo GetTopLevelAccelerationStructurePrebuildInfo(
	    std::uint32_t instanceCount) const noexcept;
	virtual RhiPartitionedTlasBuildSizes GetPartitionedTopLevelAccelerationStructureBuildSizes(
	    const RhiPartitionedTlasDesc& desc) const noexcept;
	virtual RhiOwnedResourceHandle CreatePartitionedTopLevelAccelerationStructureBuffer(
	    const RhiPartitionedTlasBuildSizes& sizes,
	    std::wstring_view debugName);
	virtual RhiOwnedResourceHandle CreatePartitionedTopLevelAccelerationStructureOperationBuffer(
	    const RhiPartitionedTlasOperationPackDesc& operationPack,
	    std::wstring_view debugName);
	virtual RhiOwnedResourceHandle CreatePartitionedTopLevelAccelerationStructureLogicalUpdateBuffer(
	    const RhiPartitionedTlasLogicalUpdateBufferDesc& desc,
	    const RhiPartitionedTlasLogicalUpdateRecord* records,
	    std::uint32_t recordCount,
	    std::wstring_view debugName);
	virtual RhiPartitionedTlasGpuOperationBufferLayout GetPartitionedTopLevelAccelerationStructureGpuOperationBufferLayout(
	    const RhiPartitionedTlasDesc& desc) const noexcept;
	virtual RhiOwnedResourceHandle CreatePartitionedTopLevelAccelerationStructureGpuOperationBuffer(
	    const RhiPartitionedTlasGpuOperationBufferDesc& desc,
	    std::wstring_view debugName);
	virtual bool PackPartitionedTopLevelAccelerationStructureGpuOperations(
	    RenderCommandList& commandList,
	    const RhiPartitionedTlasGpuOperationPackDesc& desc) noexcept;
	virtual RhiOwnedResourceHandle CreateRayTracingScratchBuffer(std::uint64_t sizeInBytes, std::wstring_view debugName) = 0;
	virtual RhiOwnedResourceHandle CreateRayTracingAccelerationStructureBuffer(
	    std::uint64_t sizeInBytes,
	    ERhiRayTracingAccelerationStructureType type,
	    std::wstring_view debugName) = 0;
	virtual RhiOwnedResourceHandle CreateRayTracingInstanceBuffer(
	    const RhiRayTracingInstanceDesc* instances,
	    std::uint32_t instanceCount,
	    std::wstring_view debugName);
};

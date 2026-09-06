#pragma once

#include "../Memory/RhiMemoryTypes.h"
#include "../RayTracing/RhiClassicTlasService.h"
#include "../RayTracing/RhiPartitionedTlasService.h"
#include "../RayTracing/RhiRayTracingDesc.h"
#include "../RayTracing/RhiRayTracingPipelineDesc.h"
#include "../Resources/RhiResourceDesc.h"
#include "../Resources/RhiResourceHandles.h"
#include "../RHIAPI.h"

#include <cstdint>
#include <memory>
#include <string_view>

class RenderCommandList;

class SPARKLE_RHI_API RhiRayTracingService
{
public:
	virtual ~RhiRayTracingService() noexcept = default;
	RhiRayTracingService(const RhiRayTracingService&) = delete;
	RhiRayTracingService& operator=(const RhiRayTracingService&) = delete;
	RhiRayTracingService(RhiRayTracingService&&) = delete;
	RhiRayTracingService& operator=(RhiRayTracingService&&) = delete;

	virtual RhiClassicTlasService& GetClassicTlasService() noexcept = 0;
	virtual const RhiClassicTlasService& GetClassicTlasService() const noexcept = 0;
	virtual RhiPartitionedTlasService& GetPartitionedTlasService() noexcept = 0;
	virtual const RhiPartitionedTlasService& GetPartitionedTlasService() const noexcept = 0;
	virtual RhiRayTracingAccelerationStructurePrebuildInfo GetBottomLevelAccelerationStructurePrebuildInfo(
	    const RhiRayTracingGeometryDesc& geometry) const noexcept = 0;
	virtual RhiRayTracingAccelerationStructurePrebuildInfo GetTopLevelAccelerationStructurePrebuildInfo(
	    std::uint32_t instanceCount,
	    ERhiClassicTlasBuildFlags buildFlags = ERhiClassicTlasBuildFlags::None) const noexcept;
	virtual RhiPartitionedTlasBuildSizes GetPartitionedTopLevelAccelerationStructureBuildSizes(
	    const RhiPartitionedTlasDesc& desc) const noexcept;
	virtual RhiOwnedResourceHandle CreatePartitionedTopLevelAccelerationStructureBuffer(
	    const RhiPartitionedTlasBuildSizes& sizes,
	    std::wstring_view debugName);
	virtual RhiOwnedResourceHandle CreatePartitionedTopLevelAccelerationStructureOperationBuffer(
	    const RhiPartitionedTlasOperationPackDesc& operationPack,
	    std::wstring_view debugName);
	virtual RhiPartitionedTlasOperationBufferLayout GetPartitionedTopLevelAccelerationStructureOperationBufferLayout(
	    const RhiPartitionedTlasDesc& desc) const noexcept;
	virtual RhiOwnedResourceHandle CreateRayTracingScratchBuffer(std::uint64_t sizeInBytes, std::wstring_view debugName) = 0;
	virtual RhiOwnedResourceHandle CreateRayTracingAccelerationStructureBuffer(
	    std::uint64_t sizeInBytes,
	    ERhiRayTracingAccelerationStructureType type,
	    std::wstring_view debugName) = 0;
	virtual RhiOwnedResourceHandle CreateRayTracingInstanceBuffer(
	    const RhiRayTracingInstanceDesc* instances,
	    std::uint32_t instanceCount,
	    std::wstring_view debugName);
	virtual std::unique_ptr<RayTracingShaderTable> CreateRayTracingShaderTable(const RayTracingShaderTableDesc& desc) = 0;

protected:
	RhiRayTracingService() noexcept = default;
};

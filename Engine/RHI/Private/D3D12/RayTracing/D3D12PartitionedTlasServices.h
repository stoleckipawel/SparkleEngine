#pragma once

#include "RayTracing/RhiPartitionedTlasService.h"

#include <cstdint>
#include <string_view>

class D3D12GpuMemoryAllocator;
class D3D12NvapiRayTracingProvider;
class D3D12Rhi;

class D3D12PartitionedTlasServices final : public RhiPartitionedTlasService
{
  public:
	D3D12PartitionedTlasServices(
	    D3D12Rhi& rhi,
	    D3D12GpuMemoryAllocator& memoryAllocator,
	    D3D12NvapiRayTracingProvider& nvapiProvider) noexcept;

	RhiPartitionedTlasBuildSizes GetPartitionedTopLevelAccelerationStructureBuildSizes(
	    const RhiPartitionedTlasDesc& desc) const noexcept override;
	RhiOwnedResourceHandle CreatePartitionedTopLevelAccelerationStructureBuffer(
	    const RhiPartitionedTlasBuildSizes& sizes,
	    std::wstring_view debugName) override;
	RhiOwnedResourceHandle CreatePartitionedTopLevelAccelerationStructureOperationBuffer(
	    const RhiPartitionedTlasOperationPackDesc& operationPack,
	    std::wstring_view debugName) override;
	RhiOwnedResourceHandle CreatePartitionedTopLevelAccelerationStructureLogicalUpdateBuffer(
	    const RhiPartitionedTlasLogicalUpdateBufferDesc& desc,
	    const RhiPartitionedTlasLogicalUpdateRecord* records,
	    std::uint32_t recordCount,
	    std::wstring_view debugName) override;
	RhiPartitionedTlasGpuOperationBufferLayout GetPartitionedTopLevelAccelerationStructureGpuOperationBufferLayout(
	    const RhiPartitionedTlasDesc& desc) const noexcept override;
	RhiOwnedResourceHandle CreatePartitionedTopLevelAccelerationStructureGpuOperationBuffer(
	    const RhiPartitionedTlasGpuOperationBufferDesc& desc,
	    std::wstring_view debugName) override;
	bool PackPartitionedTopLevelAccelerationStructureGpuOperations(
	    RenderCommandList& commandList,
	    const RhiPartitionedTlasGpuOperationPackDesc& desc) noexcept override;

  private:
	static std::uint64_t AlignUp(std::uint64_t value, std::uint64_t alignment) noexcept;
	static std::uint32_t ToNvapiPartitionedInstanceFlags(RhiPartitionedTlasInstanceFlags flags) noexcept;
	static std::uint32_t ToNvapiPartitionedOperationType(ERhiPartitionedTlasOperationType type) noexcept;
	static std::uint64_t ResolveOperationArgumentGpuAddress(
	    const RhiPartitionedTlasOperationHeader& operation,
	    RhiGpuVirtualAddress instanceWriteAddress,
	    RhiGpuVirtualAddress instanceUpdateAddress,
	    RhiGpuVirtualAddress partitionTranslationAddress) noexcept;
	static std::uint64_t ResolveOperationArgumentStride(const RhiPartitionedTlasOperationHeader& operation) noexcept;

	D3D12Rhi* m_rhi = nullptr;
	D3D12GpuMemoryAllocator* m_memoryAllocator = nullptr;
	D3D12NvapiRayTracingProvider* m_nvapiProvider = nullptr;
};

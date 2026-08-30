#pragma once

#include "RayTracing/RhiPartitionedTlasOperationLayout.h"
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
	RhiPartitionedTlasOperationBufferLayout GetPartitionedTopLevelAccelerationStructureOperationBufferLayout(
	    const RhiPartitionedTlasDesc& desc) const noexcept override;

private:
	static std::uint32_t ToNvapiPartitionedInstanceFlags(RhiPartitionedTlasInstanceFlags flags) noexcept;
	static std::uint32_t ToNvapiPartitionedOperationType(ERhiPartitionedTlasOperationType type) noexcept;
	static RhiPartitionedTlasNativeOperationLayout GetNativeOperationLayout() noexcept;

	D3D12Rhi* m_rhi = nullptr;
	D3D12GpuMemoryAllocator* m_memoryAllocator = nullptr;
	D3D12NvapiRayTracingProvider* m_nvapiProvider = nullptr;
};

#pragma once

#include "D3D12/RayTracing/D3D12ClassicTlasServices.h"
#include "D3D12/RayTracing/D3D12NvapiRayTracingProvider.h"
#include "D3D12/RayTracing/D3D12PartitionedTlasServices.h"
#include "RayTracing/RhiRayTracingService.h"

#include <cstdint>
#include <string_view>

class D3D12GpuMemoryAllocator;
class D3D12Rhi;
struct ID3D12GraphicsCommandList7;

class D3D12RayTracingServices final : public RhiRayTracingService
{
  public:
	D3D12RayTracingServices(
	    D3D12Rhi& rhi,
	    D3D12GpuMemoryAllocator& memoryAllocator,
	    D3D12NvapiRayTracingProvider& nvapiProvider) noexcept;

	RhiClassicTlasService& GetClassicTlasService() noexcept override;
	const RhiClassicTlasService& GetClassicTlasService() const noexcept override;
	RhiPartitionedTlasService& GetPartitionedTlasService() noexcept override;
	const RhiPartitionedTlasService& GetPartitionedTlasService() const noexcept override;
	RhiRayTracingCapabilities GetCapabilities() const noexcept;
	RhiRayTracingCapabilities GetRayTracingCapabilities() const noexcept override;
	RhiRayTracingAccelerationStructurePrebuildInfo GetBottomLevelAccelerationStructurePrebuildInfo(
	    const RhiRayTracingGeometryDesc& geometry) const noexcept override;
	RhiRayTracingAccelerationStructurePrebuildInfo GetTopLevelAccelerationStructurePrebuildInfo(std::uint32_t instanceCount)
	    const noexcept override;
	RhiPartitionedTlasBuildSizes GetPartitionedTopLevelAccelerationStructureBuildSizes(
	    const RhiPartitionedTlasDesc& desc) const noexcept override;
	RhiOwnedResourceHandle CreatePartitionedTopLevelAccelerationStructureBuffer(
	    const RhiPartitionedTlasBuildSizes& sizes,
	    std::wstring_view debugName) override;
	RhiOwnedResourceHandle CreatePartitionedTopLevelAccelerationStructureOperationBuffer(
	    const RhiPartitionedTlasOperationPackDesc& operationPack,
	    std::wstring_view debugName) override;
	RhiOwnedResourceHandle CreateScratchBuffer(std::uint64_t sizeInBytes, std::wstring_view debugName);
	RhiOwnedResourceHandle CreateRayTracingScratchBuffer(std::uint64_t sizeInBytes, std::wstring_view debugName) override;
	RhiOwnedResourceHandle CreateAccelerationStructureBuffer(
	    std::uint64_t sizeInBytes,
	    ERhiRayTracingAccelerationStructureType type,
	    std::wstring_view debugName);
	RhiOwnedResourceHandle CreateRayTracingAccelerationStructureBuffer(
	    std::uint64_t sizeInBytes,
	    ERhiRayTracingAccelerationStructureType type,
	    std::wstring_view debugName) override;
	RhiOwnedResourceHandle CreateInstanceBuffer(
	    const RhiRayTracingInstanceDesc* instances,
	    std::uint32_t instanceCount,
	    std::wstring_view debugName);
	RhiOwnedResourceHandle CreateRayTracingInstanceBuffer(
	    const RhiRayTracingInstanceDesc* instances,
	    std::uint32_t instanceCount,
	    std::wstring_view debugName) override;
	bool BuildPartitionedTopLevelAccelerationStructure(
	    ID3D12GraphicsCommandList7* commandList,
	    const RhiPartitionedTlasBuildCommandDesc& desc) const noexcept;

  private:
	D3D12Rhi* m_rhi = nullptr;
	D3D12GpuMemoryAllocator* m_memoryAllocator = nullptr;
	D3D12NvapiRayTracingProvider* m_nvapiProvider = nullptr;
	D3D12ClassicTlasServices m_classicTlasServices;
	D3D12PartitionedTlasServices m_partitionedTlasServices;
};

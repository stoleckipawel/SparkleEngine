#pragma once

#include "RayTracing/RhiRayTracingService.h"

#include <cstdint>
#include <string_view>

class D3D12GpuMemoryAllocator;
class D3D12Rhi;

class D3D12RayTracingServices final : public RhiRayTracingService
{
  public:
	D3D12RayTracingServices(D3D12Rhi& rhi, D3D12GpuMemoryAllocator& memoryAllocator) noexcept;

	RhiRayTracingCapabilities GetCapabilities() const noexcept;
	RhiRayTracingCapabilities GetRayTracingCapabilities() const noexcept override { return GetCapabilities(); }
	RhiRayTracingAccelerationStructurePrebuildInfo GetBottomLevelAccelerationStructurePrebuildInfo(
	    const RhiRayTracingGeometryDesc& geometry) const noexcept override;
	RhiRayTracingAccelerationStructurePrebuildInfo GetTopLevelAccelerationStructurePrebuildInfo(std::uint32_t instanceCount)
	    const noexcept override;
	RhiOwnedResourceHandle CreateScratchBuffer(std::uint64_t sizeInBytes, std::wstring_view debugName);
	RhiOwnedResourceHandle CreateRayTracingScratchBuffer(std::uint64_t sizeInBytes, std::wstring_view debugName) override
	{
		return CreateScratchBuffer(sizeInBytes, debugName);
	}
	RhiOwnedResourceHandle CreateAccelerationStructureBuffer(
	    std::uint64_t sizeInBytes,
	    ERhiRayTracingAccelerationStructureType type,
	    std::wstring_view debugName);
	RhiOwnedResourceHandle CreateRayTracingAccelerationStructureBuffer(
	    std::uint64_t sizeInBytes,
	    ERhiRayTracingAccelerationStructureType type,
	    std::wstring_view debugName) override
	{
		return CreateAccelerationStructureBuffer(sizeInBytes, type, debugName);
	}
	RhiOwnedResourceHandle CreateInstanceBuffer(
	    const RhiRayTracingInstanceDesc* instances,
	    std::uint32_t instanceCount,
	    std::wstring_view debugName);
	RhiOwnedResourceHandle CreateRayTracingInstanceBuffer(
	    const RhiRayTracingInstanceDesc* instances,
	    std::uint32_t instanceCount,
	    std::wstring_view debugName) override
	{
		return CreateInstanceBuffer(instances, instanceCount, debugName);
	}

  private:
	D3D12Rhi* m_rhi = nullptr;
	D3D12GpuMemoryAllocator* m_memoryAllocator = nullptr;
};

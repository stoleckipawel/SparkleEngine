#pragma once

#include "Interop/RhiNativeHandles.h"
#include "RayTracing/RhiRayTracingDesc.h"

#include <cstdint>
#include <string_view>

class D3D12GpuMemoryAllocator;
class D3D12Rhi;

class D3D12RayTracingServices final
{
  public:
	D3D12RayTracingServices(D3D12Rhi& rhi, D3D12GpuMemoryAllocator& memoryAllocator) noexcept;

	RhiRayTracingCapabilities GetCapabilities() const noexcept;
	RhiRayTracingAccelerationStructurePrebuildInfo GetBottomLevelAccelerationStructurePrebuildInfo(
	    const RhiRayTracingGeometryDesc& geometry) const noexcept;
	RhiRayTracingAccelerationStructurePrebuildInfo GetTopLevelAccelerationStructurePrebuildInfo(std::uint32_t instanceCount)
	    const noexcept;
	RhiOwnedResourceHandle CreateScratchBuffer(std::uint64_t sizeInBytes, std::wstring_view debugName);
	RhiOwnedResourceHandle CreateAccelerationStructureBuffer(
	    std::uint64_t sizeInBytes,
	    ERhiRayTracingAccelerationStructureType type,
	    std::wstring_view debugName);
	RhiOwnedResourceHandle CreateInstanceBuffer(
	    const RhiRayTracingInstanceDesc* instances,
	    std::uint32_t instanceCount,
	    std::wstring_view debugName);

  private:
	D3D12Rhi* m_rhi = nullptr;
	D3D12GpuMemoryAllocator* m_memoryAllocator = nullptr;
};

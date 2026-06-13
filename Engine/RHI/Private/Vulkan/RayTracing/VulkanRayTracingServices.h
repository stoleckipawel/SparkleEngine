#pragma once

#include "Interop/RhiNativeHandles.h"
#include "RayTracing/RhiRayTracingDesc.h"

#include <cstdint>
#include <string_view>

class VulkanGpuMemoryAllocator;
class VulkanRhi;

class VulkanRayTracingServices final
{
  public:
	VulkanRayTracingServices(VulkanRhi& rhi, VulkanGpuMemoryAllocator& memoryAllocator) noexcept;

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
	VulkanRhi* m_rhi = nullptr;
	VulkanGpuMemoryAllocator* m_memoryAllocator = nullptr;
};

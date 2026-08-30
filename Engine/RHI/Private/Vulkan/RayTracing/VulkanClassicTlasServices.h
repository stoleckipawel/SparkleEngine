#pragma once

#include "Resources/RhiResourceHandles.h"
#include "RayTracing/RhiClassicTlasService.h"

#include <cstdint>
#include <string_view>

class VulkanGpuMemoryAllocator;
class VulkanRhi;

class VulkanClassicTlasServices final : public RhiClassicTlasService
{
public:
	VulkanClassicTlasServices(VulkanRhi& rhi, VulkanGpuMemoryAllocator& memoryAllocator) noexcept;

	RhiRayTracingAccelerationStructurePrebuildInfo GetClassicTopLevelAccelerationStructurePrebuildInfo(
	    std::uint32_t instanceCount,
	    ERhiClassicTlasBuildFlags buildFlags = ERhiClassicTlasBuildFlags::None) const noexcept override;
	RhiOwnedResourceHandle CreateClassicTopLevelAccelerationStructureInstanceBuffer(
	    const RhiRayTracingInstanceDesc* instances,
	    std::uint32_t instanceCount,
	    std::wstring_view debugName) override;

private:
	VulkanRhi* m_rhi = nullptr;
	VulkanGpuMemoryAllocator* m_memoryAllocator = nullptr;
};

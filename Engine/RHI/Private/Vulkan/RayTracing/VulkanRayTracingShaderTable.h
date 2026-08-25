#pragma once

#include "RayTracing/RhiRayTracingPipelineDesc.h"
#include "Vulkan/Memory/VulkanGpuAllocation.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

class VulkanGpuMemoryAllocator;
class VulkanRayTracingPipeline;
class VulkanRhi;

class VulkanRayTracingShaderTable final : public RayTracingShaderTable
{
public:
	VulkanRayTracingShaderTable(VulkanRhi& rhi, VulkanGpuMemoryAllocator& memoryAllocator, const RayTracingShaderTableDesc& desc);

	RhiResourceHandle GetResource() const noexcept override { return RhiResourceHandle{m_allocation.get()}; }
	RhiRayTracingShaderTableRegion GetRayGenerationRegion() const noexcept override { return m_rayGeneration; }
	RhiRayTracingShaderTableRegion GetMissRegion() const noexcept override { return m_miss; }
	RhiRayTracingShaderTableRegion GetHitGroupRegion() const noexcept override { return m_hitGroup; }
	RhiRayTracingShaderTableRegion GetCallableRegion() const noexcept override { return m_callable; }
	VkDeviceAddress GetDeviceAddress() const noexcept { return m_allocation != nullptr ? m_allocation->BufferDeviceAddress : 0; }

private:
	static std::vector<std::byte> CollectShaderIdentifiers(
	    const VulkanRayTracingPipeline& pipeline,
	    std::span<const RhiRayTracingShaderRecord> records,
	    std::span<const std::byte> groupHandles,
	    std::uint32_t handleSize);

	std::unique_ptr<VulkanGpuAllocationRecord> m_allocation;
	RhiRayTracingShaderTableRegion m_rayGeneration;
	RhiRayTracingShaderTableRegion m_miss;
	RhiRayTracingShaderTableRegion m_hitGroup;
	RhiRayTracingShaderTableRegion m_callable;
};

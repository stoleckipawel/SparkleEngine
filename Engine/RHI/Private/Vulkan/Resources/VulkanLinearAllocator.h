#pragma once

#include "Vulkan/Memory/VulkanGpuAllocation.h"
#include "Resources/RhiResourceDesc.h"

#include <cstdint>
#include <memory>
#include <vector>

class VulkanGpuMemoryAllocator;

class VulkanLinearAllocator final
{
  public:
	explicit VulkanLinearAllocator(VulkanGpuMemoryAllocator& memoryAllocator);
	~VulkanLinearAllocator() noexcept = default;

	VulkanLinearAllocator(const VulkanLinearAllocator&) = delete;
	VulkanLinearAllocator& operator=(const VulkanLinearAllocator&) = delete;
	VulkanLinearAllocator(VulkanLinearAllocator&&) = delete;
	VulkanLinearAllocator& operator=(VulkanLinearAllocator&&) = delete;

	void BeginFrame(std::uint32_t frameIndex) noexcept;
	RhiGpuVirtualAddress AllocateAndCopy(const void* data, std::uint32_t sizeInBytes);

  private:
	VulkanGpuMemoryAllocator& m_memoryAllocator;
	std::uint32_t m_currentFrameIndex = 0;
	std::vector<std::vector<std::unique_ptr<VulkanGpuAllocationRecord>>> m_uploadRecordsByFrame;
};
#pragma once

#include "Resources/RhiResourceDesc.h"
#include "Vulkan/VulkanIncludes.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>

class VulkanGpuMemoryAllocator;
class VulkanRhi;
struct VulkanGpuAllocationRecord;

class VulkanRecordingUploadPage final
{
  public:
	VulkanRecordingUploadPage(
	    VulkanRhi& rhi,
	    VulkanGpuMemoryAllocator& memoryAllocator,
	    std::uint64_t capacityInBytes) noexcept;
	~VulkanRecordingUploadPage() noexcept;

	VulkanRecordingUploadPage(const VulkanRecordingUploadPage&) = delete;
	VulkanRecordingUploadPage& operator=(const VulkanRecordingUploadPage&) = delete;
	VulkanRecordingUploadPage(VulkanRecordingUploadPage&&) = delete;
	VulkanRecordingUploadPage& operator=(VulkanRecordingUploadPage&&) = delete;

	void Reset() noexcept;
	RhiGpuVirtualAddress AllocateAndCopy(const void* data, std::uint32_t sizeInBytes) noexcept;
	bool Resolve(
	    RhiGpuVirtualAddress address,
	    VkBuffer& buffer,
	    VkDeviceSize& offset,
	    VkDeviceSize& range) const noexcept;

	std::uint64_t GetCapacityInBytes() const noexcept { return m_capacityInBytes; }

  private:
	struct Allocation final
	{
		VkDeviceSize Offset = 0;
		VkDeviceSize Size = 0;
	};

	static constexpr std::size_t MaximumAllocations = 4096;

	std::unique_ptr<VulkanGpuAllocationRecord> m_buffer;
	VulkanGpuMemoryAllocator* m_memoryAllocator = nullptr;
	std::array<Allocation, MaximumAllocations> m_allocations;
	std::byte* m_mappedData = nullptr;
	std::size_t m_allocationCount = 0;
	std::uint64_t m_capacityInBytes = 0;
	VkDeviceSize m_alignment = 1;
	VkDeviceSize m_offset = 0;
};

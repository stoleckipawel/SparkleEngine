#pragma once

#include "Vulkan/VulkanIncludes.h"

#include <cstdint>

class VulkanRhi;

class VulkanRecordingDescriptorPool final
{
  public:
	static constexpr std::uint32_t DescriptorSetCapacity = 256;

	explicit VulkanRecordingDescriptorPool(VulkanRhi& rhi) noexcept;
	~VulkanRecordingDescriptorPool() noexcept;

	VulkanRecordingDescriptorPool(const VulkanRecordingDescriptorPool&) = delete;
	VulkanRecordingDescriptorPool& operator=(const VulkanRecordingDescriptorPool&) = delete;
	VulkanRecordingDescriptorPool(VulkanRecordingDescriptorPool&&) = delete;
	VulkanRecordingDescriptorPool& operator=(VulkanRecordingDescriptorPool&&) = delete;

	void Reset() noexcept;
	VkDescriptorSet AllocateSet(VkDescriptorSetLayout layout) noexcept;

	std::uint32_t GetCapacity() const noexcept { return DescriptorSetCapacity; }
	VkDescriptorPool GetNativePool() const noexcept { return m_pool; }

  private:
	void CreatePool() noexcept;

	VulkanRhi* m_rhi = nullptr;
	VkDescriptorPool m_pool = VK_NULL_HANDLE;
	std::uint32_t m_allocatedSetCount = 0;
};

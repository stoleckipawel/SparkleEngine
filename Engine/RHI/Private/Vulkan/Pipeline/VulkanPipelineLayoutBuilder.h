#pragma once

#include "Vulkan/VulkanIncludes.h"

#include <memory>
#include <span>
#include <string_view>

class RenderBindingLayout;
class VulkanRhi;

class VulkanPipelineLayout final
{
  public:
	VulkanPipelineLayout() noexcept = default;
	VulkanPipelineLayout(VkDevice device, VkPipelineLayout layout) noexcept;
	~VulkanPipelineLayout() noexcept;

	VulkanPipelineLayout(const VulkanPipelineLayout&) = delete;
	VulkanPipelineLayout& operator=(const VulkanPipelineLayout&) = delete;
	VulkanPipelineLayout(VulkanPipelineLayout&& other) noexcept;
	VulkanPipelineLayout& operator=(VulkanPipelineLayout&& other) noexcept;

	VkPipelineLayout Get() const noexcept { return m_layout; }
	explicit operator bool() const noexcept { return m_layout != VK_NULL_HANDLE; }

  private:
	void Reset() noexcept;

	VkDevice m_device = VK_NULL_HANDLE;
	VkPipelineLayout m_layout = VK_NULL_HANDLE;
};

class VulkanPipelineLayoutBuilder final
{
  public:
	void SetBindingLayout(const RenderBindingLayout* bindingLayout) noexcept;
	std::unique_ptr<VulkanPipelineLayout> Build(VulkanRhi& rhi, std::string_view debugName) const;

  private:
	std::span<const VkDescriptorSetLayout> m_descriptorSetLayouts;
	std::span<const VkPushConstantRange> m_pushConstantRanges;
};
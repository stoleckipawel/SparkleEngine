#pragma once

#include "Device/RenderHardwareInterface.h"
#include "Vulkan/VulkanIncludes.h"

#include <memory>
#include <string>
#include <vector>

class VulkanRhi;

class VulkanBindingLayout final : public RenderBindingLayout
{
  public:
	VulkanBindingLayout(
	    VkDevice device,
	    const PassParameterLayout& parameterLayout,
	    std::vector<VkDescriptorSetLayout> descriptorSetLayouts,
	    std::vector<VkSampler> immutableSamplers,
	    std::vector<VkPushConstantRange> pushConstantRanges,
	    std::vector<CompiledBinding> bindings,
	    std::vector<std::string> bindingNames) noexcept;
	~VulkanBindingLayout() noexcept override;

	VulkanBindingLayout(const VulkanBindingLayout&) = delete;
	VulkanBindingLayout& operator=(const VulkanBindingLayout&) = delete;
	VulkanBindingLayout(VulkanBindingLayout&&) = delete;
	VulkanBindingLayout& operator=(VulkanBindingLayout&&) = delete;

	const std::vector<VkDescriptorSetLayout>& GetDescriptorSetLayouts() const noexcept { return m_descriptorSetLayouts; }
	const std::vector<VkPushConstantRange>& GetPushConstantRanges() const noexcept { return m_pushConstantRanges; }

  private:
	VkDevice m_device = VK_NULL_HANDLE;
	std::vector<VkDescriptorSetLayout> m_descriptorSetLayouts;
	std::vector<VkSampler> m_immutableSamplers;
	std::vector<VkPushConstantRange> m_pushConstantRanges;
};

class VulkanBindingLayoutCompiler final
{
  public:
	static std::unique_ptr<VulkanBindingLayout> Compile(VulkanRhi& rhi, const RenderBindingLayoutCompileDesc& desc);
};

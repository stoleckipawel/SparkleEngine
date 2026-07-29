#pragma once

#include "Pipeline/RhiPipelineDesc.h"
#include "Vulkan/VulkanIncludes.h"

#include <string>
#include <string_view>

class VulkanRhi;

class VulkanShaderModule final
{
  public:
	VulkanShaderModule() = default;
	VulkanShaderModule(VulkanRhi& rhi, const RhiShaderStageDesc& desc, std::string_view pipelineName, bool required);
	~VulkanShaderModule() noexcept;

	VulkanShaderModule(const VulkanShaderModule&) = delete;
	VulkanShaderModule& operator=(const VulkanShaderModule&) = delete;
	VulkanShaderModule(VulkanShaderModule&& other) noexcept;
	VulkanShaderModule& operator=(VulkanShaderModule&& other) noexcept;

	bool IsValid() const noexcept { return m_module != VK_NULL_HANDLE; }
	explicit operator bool() const noexcept { return IsValid(); }

	VkPipelineShaderStageCreateInfo BuildStageCreateInfo() const noexcept;

  private:
	static VkShaderStageFlagBits ToVkShaderStage(ShaderStage stage) noexcept;
	void Reset() noexcept;

	VkDevice m_device = VK_NULL_HANDLE;
	VkShaderModule m_module = VK_NULL_HANDLE;
	ShaderStage m_stage = ShaderStage::Count;
	std::string m_entryPoint = "main";
};

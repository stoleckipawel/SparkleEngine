#pragma once

#include "Pipeline/RhiPipelineStateDesc.h"
#include "Vulkan/VulkanIncludes.h"

#include <memory>
#include <string>

class VulkanRhi;
class VulkanPipelineLayout;

class VulkanPipelineState final : public RenderPipelineState
{
  public:
	VulkanPipelineState(VulkanRhi& rhi, const GraphicsPipelineStateDesc& desc);
	VulkanPipelineState(VulkanRhi& rhi, const ComputePipelineStateDesc& desc);
	~VulkanPipelineState() noexcept override;

	VulkanPipelineState(const VulkanPipelineState&) = delete;
	VulkanPipelineState& operator=(const VulkanPipelineState&) = delete;
	VulkanPipelineState(VulkanPipelineState&&) = delete;
	VulkanPipelineState& operator=(VulkanPipelineState&&) = delete;

	VkPipeline GetPipeline() const noexcept { return m_pipeline; }
	VkPipelineLayout GetPipelineLayout() const noexcept;
	VkPipelineBindPoint GetBindPoint() const noexcept { return m_bindPoint; }

  private:
	void Reset() noexcept;

	VkDevice m_device = VK_NULL_HANDLE;
	VkPipeline m_pipeline = VK_NULL_HANDLE;
	std::unique_ptr<VulkanPipelineLayout> m_pipelineLayout;
	VkPipelineBindPoint m_bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
};
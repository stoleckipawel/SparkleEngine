#pragma once

#include "Pipeline/RhiPipelineDesc.h"
#include "Vulkan/VulkanIncludes.h"

#include <memory>
#include <string>

class VulkanRhi;
class VulkanPipelineLayout;

class VulkanPipeline final : public RenderPipeline
{
public:
	VulkanPipeline(VulkanRhi& rhi, const GraphicsPipelineDesc& desc);
	VulkanPipeline(VulkanRhi& rhi, const ComputePipelineDesc& desc);
	~VulkanPipeline() noexcept override;

	VulkanPipeline(const VulkanPipeline&) = delete;
	VulkanPipeline& operator=(const VulkanPipeline&) = delete;
	VulkanPipeline(VulkanPipeline&&) = delete;
	VulkanPipeline& operator=(VulkanPipeline&&) = delete;

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

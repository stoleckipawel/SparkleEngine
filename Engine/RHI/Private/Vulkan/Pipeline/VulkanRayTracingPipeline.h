#pragma once

#include "RayTracing/RhiRayTracingPipelineDesc.h"
#include "Vulkan/VulkanIncludes.h"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

class VulkanPipelineLayout;
class VulkanRhi;

class VulkanRayTracingPipeline final : public RayTracingPipeline
{
public:
	VulkanRayTracingPipeline(VulkanRhi& rhi, const RayTracingPipelineDesc& desc);
	~VulkanRayTracingPipeline() noexcept override;

	VkPipeline GetPipeline() const noexcept { return m_pipeline; }
	VkPipelineLayout GetPipelineLayout() const noexcept;
	std::uint32_t FindShaderGroup(std::string_view exportName) const noexcept;
	std::uint32_t GetShaderGroupCount() const noexcept { return static_cast<std::uint32_t>(m_groupNames.size()); }

private:
	VkDevice m_device = VK_NULL_HANDLE;
	VkPipeline m_pipeline = VK_NULL_HANDLE;
	std::unique_ptr<VulkanPipelineLayout> m_pipelineLayout;
	std::vector<std::string> m_groupNames;
};

#include "Vulkan/Pipeline/VulkanPipelineService.h"

#include "Vulkan/Device/VulkanRhi.h"
#include "Vulkan/Pipeline/VulkanBindingLayout.h"
#include "Vulkan/Pipeline/VulkanPipeline.h"

VulkanPipelineService::VulkanPipelineService(VulkanRhi& rhi) noexcept : m_rhi(&rhi) {}

std::unique_ptr<RenderBindingLayout> VulkanPipelineService::CreateBindingLayout(const RenderBindingLayoutCompileDesc& desc)
{
	if (m_rhi == nullptr || desc.ParameterLayout == nullptr || desc.ShaderPackage == nullptr)
	{
		return {};
	}

	return VulkanBindingLayoutCompiler::Compile(*m_rhi, desc);
}

std::unique_ptr<RenderPipeline> VulkanPipelineService::CreateGraphicsPipeline(const GraphicsPipelineDesc& desc)
{
	if (m_rhi == nullptr || desc.BindingLayout == nullptr || !desc.VertexShader.IsValid())
	{
		return {};
	}

	return std::make_unique<VulkanPipeline>(*m_rhi, desc);
}

std::unique_ptr<RenderPipeline> VulkanPipelineService::CreateComputePipeline(const ComputePipelineDesc& desc)
{
	if (m_rhi == nullptr || desc.BindingLayout == nullptr || !desc.ComputeShader.IsValid())
	{
		return {};
	}

	return std::make_unique<VulkanPipeline>(*m_rhi, desc);
}

#include "Vulkan/VulkanPCH.h"

#include "Vulkan/Pipeline/VulkanRayTracingPipeline.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Strings/StringUtils.h"
#include "Validation/RhiContract.h"
#include "Vulkan/Core/VulkanResult.h"
#include "Vulkan/Device/VulkanRhi.h"
#include "Vulkan/Diagnostics/VulkanDebugNames.h"
#include "Vulkan/Pipeline/VulkanPipelineLayoutBuilder.h"
#include "Vulkan/Pipeline/VulkanShaderModule.h"

#include <limits>
#include <unordered_map>

VulkanRayTracingPipeline::VulkanRayTracingPipeline(VulkanRhi& rhi, const RayTracingPipelineDesc& desc) :
    RayTracingPipeline(desc),
    m_device(rhi.GetDevice())
{
	RhiContract::ValidateRayTracingPipelineDesc(desc);
	const RhiRayTracingCapabilities capabilities = rhi.GetRayTracingCapabilities();
	if (!capabilities.SupportsRayTracingPipeline || rhi.GetCreateRayTracingPipelines() == nullptr
	    || desc.MaxPayloadSizeInBytes > capabilities.MaxRayPayloadSizeInBytes
	    || desc.MaxAttributeSizeInBytes > capabilities.MaxRayAttributeSizeInBytes
	    || desc.MaxRecursionDepth > capabilities.MaxTraceRecursionDepth)
	{
		throw Diagnostics::Error("Vulkan ray-tracing pipeline is not fully ready.");
	}
	const std::string debugName = desc.DebugName != nullptr ? Strings::ToNarrow(desc.DebugName) : "VulkanRayTracingPipeline";
	VulkanPipelineLayoutBuilder layoutBuilder;
	layoutBuilder.SetBindingLayout(desc.GlobalBindingLayout);
	m_pipelineLayout = layoutBuilder.Build(rhi, debugName);

	std::vector<VulkanShaderModule> modules;
	std::vector<VkPipelineShaderStageCreateInfo> stages;
	std::unordered_map<std::string_view, std::uint32_t> stageIndices;
	modules.reserve(desc.ShaderExports.size());
	stages.reserve(desc.ShaderExports.size());
	for (const RhiRayTracingShaderExportDesc& shaderExport : desc.ShaderExports)
	{
		modules.emplace_back(rhi, RhiShaderStageDesc{shaderExport.Shader}, debugName);
		stages.push_back(modules.back().BuildStageCreateInfo());
		stageIndices.emplace(shaderExport.ExportName, static_cast<std::uint32_t>(stages.size() - 1));
	}

	std::vector<VkRayTracingShaderGroupCreateInfoKHR> groups;
	groups.reserve(desc.ShaderExports.size() + desc.HitGroups.size());
	for (std::uint32_t index = 0; index < desc.ShaderExports.size(); ++index)
	{
		const RhiRayTracingShaderExportDesc& shaderExport = desc.ShaderExports[index];
		const ShaderStage stage = shaderExport.Shader->Entry->Stage;
		if (stage != ShaderStage::RayGeneration && stage != ShaderStage::Miss && stage != ShaderStage::Callable)
		{
			continue;
		}
		groups.push_back(
		    VkRayTracingShaderGroupCreateInfoKHR{
		        .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
		        .pNext = nullptr,
		        .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
		        .generalShader = index,
		        .closestHitShader = VK_SHADER_UNUSED_KHR,
		        .anyHitShader = VK_SHADER_UNUSED_KHR,
		        .intersectionShader = VK_SHADER_UNUSED_KHR,
		        .pShaderGroupCaptureReplayHandle = nullptr});
		m_groupNames.emplace_back(shaderExport.ExportName);
	}
	const auto stageIndex = [&stageIndices](std::string_view name)
	{
		const auto found = stageIndices.find(name);
		return found != stageIndices.end() ? found->second : VK_SHADER_UNUSED_KHR;
	};
	for (const RhiRayTracingHitGroupDesc& hitGroup : desc.HitGroups)
	{
		groups.push_back(
		    VkRayTracingShaderGroupCreateInfoKHR{
		        .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
		        .pNext = nullptr,
		        .type = hitGroup.Kind == ERhiRayTracingHitGroupKind::Triangles ? VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR
		                                                                       : VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR,
		        .generalShader = VK_SHADER_UNUSED_KHR,
		        .closestHitShader = stageIndex(hitGroup.ClosestHitExport),
		        .anyHitShader = stageIndex(hitGroup.AnyHitExport),
		        .intersectionShader = stageIndex(hitGroup.IntersectionExport),
		        .pShaderGroupCaptureReplayHandle = nullptr});
		m_groupNames.emplace_back(hitGroup.ExportName);
	}

	const VkRayTracingPipelineCreateInfoKHR createInfo{
	    .sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR,
	    .pNext = nullptr,
	    .flags = 0,
	    .stageCount = static_cast<std::uint32_t>(stages.size()),
	    .pStages = stages.data(),
	    .groupCount = static_cast<std::uint32_t>(groups.size()),
	    .pGroups = groups.data(),
	    .maxPipelineRayRecursionDepth = desc.MaxRecursionDepth,
	    .pLibraryInfo = nullptr,
	    .pLibraryInterface = nullptr,
	    .pDynamicState = nullptr,
	    .layout = GetPipelineLayout(),
	    .basePipelineHandle = VK_NULL_HANDLE,
	    .basePipelineIndex = -1};
	const VkResult result =
	    rhi.GetCreateRayTracingPipelines()(m_device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_pipeline);
	if (!VulkanResult::Succeeded(result))
	{
		throw Diagnostics::Error(VulkanResult::FormatFailure("vkCreateRayTracingPipelinesKHR", result));
	}
	VulkanDebugNames::SetObjectName(
	    rhi.GetSetDebugUtilsObjectName(),
	    m_device,
	    VK_OBJECT_TYPE_PIPELINE,
	    reinterpret_cast<std::uint64_t>(m_pipeline),
	    debugName);
}

VulkanRayTracingPipeline::~VulkanRayTracingPipeline() noexcept
{
	if (m_device != VK_NULL_HANDLE && m_pipeline != VK_NULL_HANDLE)
	{
		vkDestroyPipeline(m_device, m_pipeline, nullptr);
	}
}

VkPipelineLayout VulkanRayTracingPipeline::GetPipelineLayout() const noexcept
{
	return m_pipelineLayout != nullptr ? m_pipelineLayout->Get() : VK_NULL_HANDLE;
}

std::uint32_t VulkanRayTracingPipeline::FindShaderGroup(std::string_view exportName) const noexcept
{
	for (std::uint32_t index = 0; index < m_groupNames.size(); ++index)
	{
		if (m_groupNames[index] == exportName)
		{
			return index;
		}
	}
	return std::numeric_limits<std::uint32_t>::max();
}

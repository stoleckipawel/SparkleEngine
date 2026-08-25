#include "Vulkan/VulkanPCH.h"

#include "Vulkan/Pipeline/VulkanShaderModule.h"

#include "Strings/StringUtils.h"
#include "Vulkan/Core/VulkanResult.h"
#include "Vulkan/Device/VulkanRhi.h"
#include "Vulkan/Diagnostics/VulkanDebugNames.h"

#include <format>

static const auto g_vulkanShaderModuleLogger = Logging::GetOrCreateLogger("RHI.Vulkan.ShaderModule");

VulkanShaderModule::VulkanShaderModule(VulkanRhi& rhi, const RhiShaderStageDesc& desc, std::string_view pipelineName) :
    m_device(rhi.GetDevice())
{
	if (!desc.IsValid())
	{
		return;
	}

	const ResolvedShader& shader = *desc.Shader;
	if (shader.Entry->BinaryFormat != ShaderBinaryFormat::SpirV)
	{
		Diagnostics::Fatal(
		    g_vulkanShaderModuleLogger,
		    __FILE__,
		    __LINE__,
		    std::format(
		        "Pipeline '{}' received non-SPIR-V code for shader stage '{}'",
		        pipelineName,
		        GetShaderStagePrefix(shader.Entry->Stage)));
	}

	const ShaderBytecode bytecode = shader.GetBytecode();
	if (!bytecode.IsValid() || (bytecode.Size % sizeof(std::uint32_t)) != 0)
	{
		Diagnostics::Fatal(
		    g_vulkanShaderModuleLogger,
		    __FILE__,
		    __LINE__,
		    std::format(
		        "Pipeline '{}' has invalid cooked SPIR-V bytecode for stage '{}'",
		        pipelineName,
		        GetShaderStagePrefix(shader.Entry->Stage)));
	}

	const std::string_view entryPoint = shader.Map->ResolveString(shader.Entry->EntryPoint);
	if (!entryPoint.empty())
	{
		m_entryPoint.assign(entryPoint.begin(), entryPoint.end());
	}

	const VkShaderModuleCreateInfo createInfo{
	    .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
	    .pNext = nullptr,
	    .flags = 0,
	    .codeSize = bytecode.Size,
	    .pCode = static_cast<const std::uint32_t*>(bytecode.Data)};
	const VkResult result = vkCreateShaderModule(m_device, &createInfo, nullptr, &m_module);
	if (!VulkanResult::Succeeded(result))
	{
		Diagnostics::Fatal(g_vulkanShaderModuleLogger, __FILE__, __LINE__, VulkanResult::FormatFailure("vkCreateShaderModule", result));
	}

	m_stage = shader.Entry->Stage;
	VulkanDebugNames::SetObjectName(
	    rhi.GetSetDebugUtilsObjectName(),
	    m_device,
	    VK_OBJECT_TYPE_SHADER_MODULE,
	    reinterpret_cast<std::uint64_t>(m_module),
	    std::format("{}:{}", pipelineName, GetShaderStagePrefix(shader.Entry->Stage)));
}

VulkanShaderModule::~VulkanShaderModule() noexcept
{
	Reset();
}

VulkanShaderModule::VulkanShaderModule(VulkanShaderModule&& other) noexcept
{
	*this = std::move(other);
}

VulkanShaderModule& VulkanShaderModule::operator=(VulkanShaderModule&& other) noexcept
{
	if (this != &other)
	{
		Reset();
		m_device = other.m_device;
		m_module = other.m_module;
		m_stage = other.m_stage;
		m_entryPoint = std::move(other.m_entryPoint);
		other.m_device = VK_NULL_HANDLE;
		other.m_module = VK_NULL_HANDLE;
		other.m_stage = ShaderStage::Count;
	}
	return *this;
}

VkPipelineShaderStageCreateInfo VulkanShaderModule::BuildStageCreateInfo() const noexcept
{
	return VkPipelineShaderStageCreateInfo{
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
	    .pNext = nullptr,
	    .flags = 0,
	    .stage = ToVkShaderStage(m_stage),
	    .module = m_module,
	    .pName = m_entryPoint.c_str(),
	    .pSpecializationInfo = nullptr};
}

VkShaderStageFlagBits VulkanShaderModule::ToVkShaderStage(ShaderStage stage) noexcept
{
	switch (stage)
	{
		case ShaderStage::Vertex:
			return VK_SHADER_STAGE_VERTEX_BIT;
		case ShaderStage::Pixel:
			return VK_SHADER_STAGE_FRAGMENT_BIT;
		case ShaderStage::Compute:
			return VK_SHADER_STAGE_COMPUTE_BIT;
		case ShaderStage::Geometry:
			return VK_SHADER_STAGE_GEOMETRY_BIT;
		case ShaderStage::Hull:
			return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
		case ShaderStage::Domain:
			return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
		case ShaderStage::RayGeneration:
			return VK_SHADER_STAGE_RAYGEN_BIT_KHR;
		case ShaderStage::Miss:
			return VK_SHADER_STAGE_MISS_BIT_KHR;
		case ShaderStage::ClosestHit:
			return VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
		case ShaderStage::AnyHit:
			return VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
		case ShaderStage::Intersection:
			return VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
		case ShaderStage::Callable:
			return VK_SHADER_STAGE_CALLABLE_BIT_KHR;
		case ShaderStage::Count:
		default:
			return VK_SHADER_STAGE_ALL;
	}
}

void VulkanShaderModule::Reset() noexcept
{
	if (m_device != VK_NULL_HANDLE && m_module != VK_NULL_HANDLE)
	{
		vkDestroyShaderModule(m_device, m_module, nullptr);
	}
	m_module = VK_NULL_HANDLE;
}

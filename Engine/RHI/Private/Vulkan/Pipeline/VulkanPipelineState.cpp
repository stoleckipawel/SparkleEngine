#include "Vulkan/VulkanPCH.h"

#include "Vulkan/Pipeline/VulkanPipelineState.h"

#include "Strings/StringUtils.h"
#include "Vulkan/Core/VulkanResult.h"
#include "Vulkan/Device/VulkanRhi.h"
#include "Vulkan/Diagnostics/VulkanDebugNames.h"
#include "Vulkan/Pipeline/VulkanBindingLayout.h"
#include "Vulkan/Pipeline/VulkanPipelineLayoutBuilder.h"
#include "Vulkan/Pipeline/VulkanShaderModule.h"
#include "Vulkan/Pipeline/VulkanVertexLayout.h"
#include "Vulkan/VulkanTypeConversions.h"

#include <array>
#include <format>
#include <span>
#include <vector>

static const auto g_vulkanPipelineStateLogger = Logging::GetOrCreateLogger("RHI.Vulkan.PipelineState");

namespace
{
	struct VulkanPipelineCacheKey final
	{
		std::uint64_t Layout = 0;
		std::uint32_t RenderTargetCount = 0;
		std::array<VkFormat, 8> RenderTargetFormats = {};
		VkFormat DepthStencilFormat = VK_FORMAT_UNDEFINED;
		VkPrimitiveTopology PrimitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		VkCullModeFlags CullMode = VK_CULL_MODE_BACK_BIT;
		VkFrontFace FrontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		VkCompareOp DepthCompareOp = VK_COMPARE_OP_LESS;
		bool DepthEnable = false;
		bool RenderWireframe = false;
	};

	std::string ToDebugName(const wchar_t* debugName)
	{
		return debugName != nullptr ? Strings::ToNarrow(debugName) : std::string{"VulkanPipelineState"};
	}

	VkStencilOpState BuildStencilFaceState(
	    CompareOp compareOp,
	    RhiStencilOp failOp,
	    RhiStencilOp depthFailOp,
	    RhiStencilOp passOp,
	    const RhiStencilTestDesc& desc) noexcept
	{
		return VkStencilOpState{
		    .failOp = VulkanTypeConversions::ToVkStencilOp(failOp),
		    .passOp = VulkanTypeConversions::ToVkStencilOp(passOp),
		    .depthFailOp = VulkanTypeConversions::ToVkStencilOp(depthFailOp),
		    .compareOp = VulkanTypeConversions::ToVkCompareOp(compareOp),
		    .compareMask = desc.StencilReadMask,
		    .writeMask = desc.StencilWriteMask,
		    .reference = 0};
	}

	bool HasStencilAspect(PixelFormat format) noexcept
	{
		switch (format)
		{
			case PixelFormat::D24_UNorm_S8_UInt:
				return true;
			default:
				return false;
		}
	}

	void HandlePipelineCreateFailure(std::string_view debugName, const char* functionName, VkResult result)
	{
		Diagnostics::Fail(
		    g_vulkanPipelineStateLogger,
		    __FILE__,
		    __LINE__,
		    std::format("Failed to create Vulkan pipeline '{}': {}", debugName, VulkanResult::FormatFailure(functionName, result)));
	}

	VkFrontFace ToVkFrontFace(ERhiFrontFaceWinding winding) noexcept
	{
		return winding == ERhiFrontFaceWinding::Clockwise ? VK_FRONT_FACE_CLOCKWISE : VK_FRONT_FACE_COUNTER_CLOCKWISE;
	}
}

VulkanPipelineState::VulkanPipelineState(VulkanRhi& rhi, const GraphicsPipelineStateDesc& desc) :
	m_device(rhi.GetDevice()), m_bindPoint(VK_PIPELINE_BIND_POINT_GRAPHICS)
{
	const std::string debugName = ToDebugName(desc.DebugName);
	VulkanPipelineLayoutBuilder layoutBuilder;
	layoutBuilder.SetBindingLayout(desc.BindingLayout);
	m_pipelineLayout = layoutBuilder.Build(rhi, debugName);

	VulkanShaderModule vertexShader(rhi, desc.VertexShader, debugName, true);
	VulkanShaderModule pixelShader(rhi, desc.PixelShader, debugName, false);

	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
	shaderStages.push_back(vertexShader.BuildStageCreateInfo());
	if (pixelShader)
	{
		shaderStages.push_back(pixelShader.BuildStageCreateInfo());
	}

	const std::span<const VkVertexInputBindingDescription> vertexBindings = VulkanVertexLayout::GetStaticMeshBindings();
	const std::span<const VkVertexInputAttributeDescription> vertexAttributes = VulkanVertexLayout::GetStaticMeshAttributes();
	const VkPipelineVertexInputStateCreateInfo vertexInputState{
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
	    .pNext = nullptr,
	    .flags = 0,
	    .vertexBindingDescriptionCount = desc.VertexLayout == RhiVertexLayoutKind::StaticMesh ? 1u : 0u,
	    .pVertexBindingDescriptions = desc.VertexLayout == RhiVertexLayoutKind::StaticMesh ? vertexBindings.data() : nullptr,
	    .vertexAttributeDescriptionCount = desc.VertexLayout == RhiVertexLayoutKind::StaticMesh ? static_cast<std::uint32_t>(vertexAttributes.size()) : 0u,
	    .pVertexAttributeDescriptions = desc.VertexLayout == RhiVertexLayoutKind::StaticMesh ? vertexAttributes.data() : nullptr};

	const VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
	    .pNext = nullptr,
	    .flags = 0,
	    .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
	    .primitiveRestartEnable = VK_FALSE};

	const VkPipelineViewportStateCreateInfo viewportState{
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
	    .pNext = nullptr,
	    .flags = 0,
	    .viewportCount = 1,
	    .pViewports = nullptr,
	    .scissorCount = 1,
	    .pScissors = nullptr};

	const VkPipelineRasterizationStateCreateInfo rasterizationState{
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
	    .pNext = nullptr,
	    .flags = 0,
	    .depthClampEnable = VK_FALSE,
	    .rasterizerDiscardEnable = VK_FALSE,
	    .polygonMode = desc.RenderWireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL,
	    .cullMode = VulkanTypeConversions::ToVkCullModeFlags(desc.CullMode),
	    .frontFace = ToVkFrontFace(desc.FrontFaceWinding),
	    .depthBiasEnable = VK_FALSE,
	    .depthBiasConstantFactor = 0.0f,
	    .depthBiasClamp = 0.0f,
	    .depthBiasSlopeFactor = 0.0f,
	    .lineWidth = 1.0f};

	const VkPipelineMultisampleStateCreateInfo multisampleState{
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
	    .pNext = nullptr,
	    .flags = 0,
	    .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
	    .sampleShadingEnable = VK_FALSE,
	    .minSampleShading = 0.0f,
	    .pSampleMask = nullptr,
	    .alphaToCoverageEnable = VK_FALSE,
	    .alphaToOneEnable = VK_FALSE};

	std::array<VkPipelineColorBlendAttachmentState, 8> blendAttachments = {};
	for (std::uint32_t renderTargetIndex = 0; renderTargetIndex < desc.RenderTargetCount && renderTargetIndex < blendAttachments.size(); ++renderTargetIndex)
	{
		blendAttachments[renderTargetIndex] = VkPipelineColorBlendAttachmentState{
		    .blendEnable = VK_FALSE,
		    .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
		    .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
		    .colorBlendOp = VK_BLEND_OP_ADD,
		    .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
		    .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
		    .alphaBlendOp = VK_BLEND_OP_ADD,
		    .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT};
	}
	const VkPipelineColorBlendStateCreateInfo colorBlendState{
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
	    .pNext = nullptr,
	    .flags = 0,
	    .logicOpEnable = VK_FALSE,
	    .logicOp = VK_LOGIC_OP_COPY,
	    .attachmentCount = desc.RenderTargetCount,
	    .pAttachments = blendAttachments.data(),
	    .blendConstants = {0.0f, 0.0f, 0.0f, 0.0f}};

	const bool hasDepthStencilFormat = desc.DepthStencilFormat != PixelFormat::Unknown;
	const bool hasStencilFormat = HasStencilAspect(desc.DepthStencilFormat);
	const VkPipelineDepthStencilStateCreateInfo depthStencilState{
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
	    .pNext = nullptr,
	    .flags = 0,
	    .depthTestEnable = hasDepthStencilFormat && desc.DepthTest.DepthEnable ? VK_TRUE : VK_FALSE,
	    .depthWriteEnable = hasDepthStencilFormat && desc.DepthTest.DepthWriteEnable ? VK_TRUE : VK_FALSE,
	    .depthCompareOp = VulkanTypeConversions::ToVkCompareOp(desc.DepthTest.DepthFunc),
	    .depthBoundsTestEnable = VK_FALSE,
	    .stencilTestEnable = hasStencilFormat && desc.StencilTest.StencilEnable ? VK_TRUE : VK_FALSE,
	    .front = BuildStencilFaceState(
	        desc.StencilTest.FrontFaceStencilFunc,
	        desc.StencilTest.FrontFaceStencilFailOp,
	        desc.StencilTest.FrontFaceStencilDepthFailOp,
	        desc.StencilTest.FrontFaceStencilPassOp,
	        desc.StencilTest),
	    .back = BuildStencilFaceState(
	        desc.StencilTest.BackFaceStencilFunc,
	        desc.StencilTest.BackFaceStencilFailOp,
	        desc.StencilTest.BackFaceStencilDepthFailOp,
	        desc.StencilTest.BackFaceStencilPassOp,
	        desc.StencilTest),
	    .minDepthBounds = 0.0f,
	    .maxDepthBounds = 1.0f};

	const std::array<VkDynamicState, 2> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
	const VkPipelineDynamicStateCreateInfo dynamicState{
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
	    .pNext = nullptr,
	    .flags = 0,
	    .dynamicStateCount = static_cast<std::uint32_t>(dynamicStates.size()),
	    .pDynamicStates = dynamicStates.data()};

	std::array<VkFormat, 8> renderTargetFormats = {};
	for (std::uint32_t renderTargetIndex = 0; renderTargetIndex < desc.RenderTargetCount && renderTargetIndex < renderTargetFormats.size(); ++renderTargetIndex)
	{
		renderTargetFormats[renderTargetIndex] = VulkanTypeConversions::ToVkFormat(desc.RenderTargetFormats[renderTargetIndex]);
	}
	const VkPipelineRenderingCreateInfo renderingCreateInfo{
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
	    .pNext = nullptr,
	    .viewMask = 0,
	    .colorAttachmentCount = desc.RenderTargetCount,
	    .pColorAttachmentFormats = renderTargetFormats.data(),
	    .depthAttachmentFormat = VulkanTypeConversions::ToVkFormat(desc.DepthStencilFormat),
	    .stencilAttachmentFormat =
	        hasStencilFormat ? VulkanTypeConversions::ToVkFormat(desc.DepthStencilFormat) : VK_FORMAT_UNDEFINED};

	const VulkanPipelineCacheKey cacheKey{
	    .Layout = reinterpret_cast<std::uint64_t>(GetPipelineLayout()),
	    .RenderTargetCount = desc.RenderTargetCount,
	    .RenderTargetFormats = renderTargetFormats,
	    .DepthStencilFormat = renderingCreateInfo.depthAttachmentFormat,
	    .PrimitiveTopology = inputAssemblyState.topology,
	    .CullMode = rasterizationState.cullMode,
	    .FrontFace = rasterizationState.frontFace,
	    .DepthCompareOp = depthStencilState.depthCompareOp,
	    .DepthEnable = depthStencilState.depthTestEnable == VK_TRUE,
	    .RenderWireframe = desc.RenderWireframe};
	(void)cacheKey;

	const VkGraphicsPipelineCreateInfo createInfo{
	    .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
	    .pNext = &renderingCreateInfo,
	    .flags = 0,
	    .stageCount = static_cast<std::uint32_t>(shaderStages.size()),
	    .pStages = shaderStages.data(),
	    .pVertexInputState = &vertexInputState,
	    .pInputAssemblyState = &inputAssemblyState,
	    .pTessellationState = nullptr,
	    .pViewportState = &viewportState,
	    .pRasterizationState = &rasterizationState,
	    .pMultisampleState = &multisampleState,
	    .pDepthStencilState = &depthStencilState,
	    .pColorBlendState = &colorBlendState,
	    .pDynamicState = &dynamicState,
	    .layout = GetPipelineLayout(),
	    .renderPass = VK_NULL_HANDLE,
	    .subpass = 0,
	    .basePipelineHandle = VK_NULL_HANDLE,
	    .basePipelineIndex = -1};
	const VkResult result = vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_pipeline);
	if (!VulkanResult::Succeeded(result))
	{
		HandlePipelineCreateFailure(debugName, "vkCreateGraphicsPipelines", result);
	}

	VulkanDebugNames::SetObjectName(
	    rhi.GetSetDebugUtilsObjectName(),
	    m_device,
	    VK_OBJECT_TYPE_PIPELINE,
	    reinterpret_cast<std::uint64_t>(m_pipeline),
	    debugName);
}

VulkanPipelineState::VulkanPipelineState(VulkanRhi& rhi, const ComputePipelineStateDesc& desc) :
	m_device(rhi.GetDevice()), m_bindPoint(VK_PIPELINE_BIND_POINT_COMPUTE)
{
	const std::string debugName = ToDebugName(desc.DebugName);
	VulkanPipelineLayoutBuilder layoutBuilder;
	layoutBuilder.SetBindingLayout(desc.BindingLayout);
	m_pipelineLayout = layoutBuilder.Build(rhi, debugName);

	VulkanShaderModule computeShader(rhi, desc.ComputeShader, debugName, true);
	const VkPipelineShaderStageCreateInfo shaderStage = computeShader.BuildStageCreateInfo();
	const VkComputePipelineCreateInfo createInfo{
	    .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
	    .pNext = nullptr,
	    .flags = 0,
	    .stage = shaderStage,
	    .layout = GetPipelineLayout(),
	    .basePipelineHandle = VK_NULL_HANDLE,
	    .basePipelineIndex = -1};
	const VkResult result = vkCreateComputePipelines(m_device, VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_pipeline);
	if (!VulkanResult::Succeeded(result))
	{
		HandlePipelineCreateFailure(debugName, "vkCreateComputePipelines", result);
	}

	VulkanDebugNames::SetObjectName(
	    rhi.GetSetDebugUtilsObjectName(),
	    m_device,
	    VK_OBJECT_TYPE_PIPELINE,
	    reinterpret_cast<std::uint64_t>(m_pipeline),
	    debugName);
}

VulkanPipelineState::~VulkanPipelineState() noexcept
{
	Reset();
}

VkPipelineLayout VulkanPipelineState::GetPipelineLayout() const noexcept
{
	return m_pipelineLayout != nullptr ? m_pipelineLayout->Get() : VK_NULL_HANDLE;
}

void VulkanPipelineState::Reset() noexcept
{
	if (m_device == VK_NULL_HANDLE)
	{
		return;
	}

	if (m_pipeline != VK_NULL_HANDLE)
	{
		vkDestroyPipeline(m_device, m_pipeline, nullptr);
		m_pipeline = VK_NULL_HANDLE;
	}

	m_pipelineLayout.reset();
}

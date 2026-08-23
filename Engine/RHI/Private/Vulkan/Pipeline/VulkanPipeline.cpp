#include "Vulkan/VulkanPCH.h"

#include "Vulkan/Pipeline/VulkanPipeline.h"

#include "Strings/StringUtils.h"
#include "Vulkan/Core/VulkanResult.h"
#include "Vulkan/Device/VulkanRhi.h"
#include "Vulkan/Diagnostics/VulkanDebugNames.h"
#include "Vulkan/Pipeline/VulkanBindingLayout.h"
#include "Vulkan/Pipeline/VulkanPipelineLayoutBuilder.h"
#include "Vulkan/Pipeline/VulkanShaderModule.h"
#include "Vulkan/VulkanTypeConversions.h"

#include <array>
#include <format>
#include <span>
#include <vector>

static const auto g_vulkanPipelineLogger = Logging::GetOrCreateLogger("RHI.Vulkan.Pipeline");

class VulkanPipelineImplementation final
{
public:
	static bool IsColorAttachmentFormat(PixelFormat format) noexcept
	{
		switch (format)
		{
			case PixelFormat::R32G32B32A32_Float:
			case PixelFormat::R16G16B16A16_Float:
			case PixelFormat::R8G8B8A8_UNorm:
			case PixelFormat::R8G8B8A8_UNorm_Srgb:
			case PixelFormat::R16G16_Float:
			case PixelFormat::R32_Float:
			case PixelFormat::B8G8R8A8_UNorm:
			case PixelFormat::B8G8R8A8_UNorm_Srgb:
				return true;
			default:
				return false;
		}
	}

	static bool IsDepthStencilAttachmentFormat(PixelFormat format) noexcept
	{
		return format == PixelFormat::D32_Float || format == PixelFormat::D24_UNorm_S8_UInt;
	}

	static std::string ToDebugName(const wchar_t* debugName)
	{
		return debugName != nullptr ? Strings::ToNarrow(debugName) : std::string{"VulkanPipeline"};
	}

	static VkStencilOpState BuildStencilFaceState(
	    CompareOp compareOp,
	    RhiStencilOp failOp,
	    RhiStencilOp depthFailOp,
	    RhiStencilOp passOp,
	    const RhiStencilState& desc) noexcept
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

	static bool HasStencilAspect(PixelFormat format) noexcept
	{
		switch (format)
		{
			case PixelFormat::D24_UNorm_S8_UInt:
				return true;
			default:
				return false;
		}
	}

	static void HandlePipelineCreateFailure(std::string_view debugName, const char* functionName, VkResult result)
	{
		Diagnostics::Fatal(
		    g_vulkanPipelineLogger,
		    __FILE__,
		    __LINE__,
		    std::format("Failed to create Vulkan pipeline '{}': {}", debugName, VulkanResult::FormatFailure(functionName, result)));
	}

	static VkFrontFace ToVkFrontFace(ERhiFrontFaceWinding winding) noexcept
	{
		switch (winding)
		{
			case ERhiFrontFaceWinding::Clockwise:
				return VK_FRONT_FACE_CLOCKWISE;
			case ERhiFrontFaceWinding::CounterClockwise:
				return VK_FRONT_FACE_COUNTER_CLOCKWISE;
		}
		Diagnostics::Fatal(g_vulkanPipelineLogger, __FILE__, __LINE__, "Vulkan received an unsupported front-face winding.");
	}

	static VkPolygonMode ToVkPolygonMode(RhiFillMode fillMode) noexcept
	{
		switch (fillMode)
		{
			case RhiFillMode::Solid:
				return VK_POLYGON_MODE_FILL;
			case RhiFillMode::Wireframe:
				return VK_POLYGON_MODE_LINE;
		}
		Diagnostics::Fatal(g_vulkanPipelineLogger, __FILE__, __LINE__, "Vulkan received an unsupported fill mode.");
	}

	static VkFormat ToVkVertexFormat(RhiVertexElementFormat format) noexcept
	{
		switch (format)
		{
			case RhiVertexElementFormat::Float2:
				return VK_FORMAT_R32G32_SFLOAT;
			case RhiVertexElementFormat::Float4:
				return VK_FORMAT_R32G32B32A32_SFLOAT;
			case RhiVertexElementFormat::Float3:
				return VK_FORMAT_R32G32B32_SFLOAT;
		}
		Diagnostics::Fatal(g_vulkanPipelineLogger, __FILE__, __LINE__, "Vulkan received an unsupported vertex element format.");
	}

	static VkBlendFactor ToVkBlendFactor(RhiBlendFactor factor) noexcept
	{
		switch (factor)
		{
			case RhiBlendFactor::Zero:
				return VK_BLEND_FACTOR_ZERO;
			case RhiBlendFactor::SourceAlpha:
				return VK_BLEND_FACTOR_SRC_ALPHA;
			case RhiBlendFactor::InverseSourceAlpha:
				return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			case RhiBlendFactor::One:
				return VK_BLEND_FACTOR_ONE;
		}
		Diagnostics::Fatal(g_vulkanPipelineLogger, __FILE__, __LINE__, "Vulkan received an unsupported blend factor.");
	}

	static VkBlendOp ToVkBlendOperation(RhiBlendOperation operation) noexcept
	{
		switch (operation)
		{
			case RhiBlendOperation::Add:
				return VK_BLEND_OP_ADD;
		}
		Diagnostics::Fatal(g_vulkanPipelineLogger, __FILE__, __LINE__, "Vulkan received an unsupported blend operation.");
	}
};

VulkanPipeline::VulkanPipeline(VulkanRhi& rhi, const GraphicsPipelineDesc& desc) :
    m_device(rhi.GetDevice()),
    m_bindPoint(VK_PIPELINE_BIND_POINT_GRAPHICS)
{
	if (desc.BindingLayout == nullptr || (desc.ColorAttachmentCount == 0 && desc.DepthStencilAttachmentFormat == PixelFormat::Unknown)
	    || desc.ColorAttachmentCount > desc.ColorAttachmentFormats.size()
	    || desc.VertexInput.BindingCount > desc.VertexInput.Bindings.size()
	    || desc.VertexInput.ElementCount > desc.VertexInput.Elements.size()
	    || (desc.SampleCount != 1 && desc.SampleCount != 2 && desc.SampleCount != 4 && desc.SampleCount != 8))
	{
		Diagnostics::Fatal(g_vulkanPipelineLogger, __FILE__, __LINE__, "Vulkan received an unsupported graphics pipeline description.");
	}
	for (std::uint32_t elementIndex = 0; elementIndex < desc.VertexInput.ElementCount; ++elementIndex)
	{
		const std::uint8_t binding = desc.VertexInput.Elements[elementIndex].Binding;
		bool bindingExists = false;
		for (std::uint32_t bindingIndex = 0; bindingIndex < desc.VertexInput.BindingCount; ++bindingIndex)
		{
			bindingExists = bindingExists || desc.VertexInput.Bindings[bindingIndex].Binding == binding;
		}
		if (!bindingExists)
		{
			Diagnostics::Fatal(g_vulkanPipelineLogger, __FILE__, __LINE__, "Vulkan vertex input references a missing binding.");
		}
	}
	const bool hasDepthStencilFormat = desc.DepthStencilAttachmentFormat != PixelFormat::Unknown;
	const bool hasStencilFormat = VulkanPipelineImplementation::HasStencilAspect(desc.DepthStencilAttachmentFormat);
	if ((desc.Depth.DepthEnable || desc.Depth.DepthWriteEnable || desc.Stencil.StencilEnable) && !hasDepthStencilFormat)
	{
		Diagnostics::Fatal(g_vulkanPipelineLogger, __FILE__, __LINE__, "Vulkan depth-stencil state requires an attachment format.");
	}
	if (desc.Stencil.StencilEnable && !hasStencilFormat)
	{
		Diagnostics::Fatal(
		    g_vulkanPipelineLogger,
		    __FILE__,
		    __LINE__,
		    "Vulkan stencil state requires a stencil-capable attachment format.");
	}
	for (std::uint32_t index = 0; index < desc.ColorAttachmentCount; ++index)
	{
		if (!VulkanPipelineImplementation::IsColorAttachmentFormat(desc.ColorAttachmentFormats[index]))
		{
			Diagnostics::Fatal(g_vulkanPipelineLogger, __FILE__, __LINE__, "Vulkan color attachment format must be explicit.");
		}
	}
	if (desc.DepthStencilAttachmentFormat != PixelFormat::Unknown
	    && !VulkanPipelineImplementation::IsDepthStencilAttachmentFormat(desc.DepthStencilAttachmentFormat))
	{
		Diagnostics::Fatal(g_vulkanPipelineLogger, __FILE__, __LINE__, "Vulkan received an unsupported depth-stencil attachment format.");
	}
	const std::uint32_t blendTargetCount = desc.Blend.IndependentBlendEnable ? desc.ColorAttachmentCount : 1;
	for (std::uint32_t index = 0; index < blendTargetCount; ++index)
	{
		if ((desc.Blend.Targets[index].ColorWriteMask & 0xF0u) != 0)
		{
			Diagnostics::Fatal(g_vulkanPipelineLogger, __FILE__, __LINE__, "Vulkan received an unsupported color write mask.");
		}
	}
	const std::string debugName = VulkanPipelineImplementation::ToDebugName(desc.DebugName);
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

	std::array<VkVertexInputBindingDescription, 4> vertexBindings = {};
	for (std::uint32_t index = 0; index < desc.VertexInput.BindingCount; ++index)
	{
		const RhiVertexInputBinding& binding = desc.VertexInput.Bindings[index];
		vertexBindings[index] = VkVertexInputBindingDescription{
		    .binding = binding.Binding,
		    .stride = binding.StrideInBytes,
		    .inputRate = binding.PerInstance ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX};
	}
	std::array<VkVertexInputAttributeDescription, 16> vertexAttributes = {};
	for (std::uint32_t index = 0; index < desc.VertexInput.ElementCount; ++index)
	{
		const RhiVertexInputElement& element = desc.VertexInput.Elements[index];
		vertexAttributes[index] = VkVertexInputAttributeDescription{
		    .location = element.Location,
		    .binding = element.Binding,
		    .format = VulkanPipelineImplementation::ToVkVertexFormat(element.Format),
		    .offset = element.OffsetInBytes};
	}
	const VkPipelineVertexInputStateCreateInfo vertexInputState{
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
	    .pNext = nullptr,
	    .flags = 0,
	    .vertexBindingDescriptionCount = desc.VertexInput.BindingCount,
	    .pVertexBindingDescriptions = desc.VertexInput.BindingCount != 0 ? vertexBindings.data() : nullptr,
	    .vertexAttributeDescriptionCount = desc.VertexInput.ElementCount,
	    .pVertexAttributeDescriptions = desc.VertexInput.ElementCount != 0 ? vertexAttributes.data() : nullptr};

	const VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
	    .pNext = nullptr,
	    .flags = 0,
	    .topology = VulkanTypeConversions::ToVkPrimitiveTopology(desc.PrimitiveTopology),
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
	    .depthClampEnable = desc.Rasterizer.DepthClipEnable ? VK_FALSE : VK_TRUE,
	    .rasterizerDiscardEnable = VK_FALSE,
	    .polygonMode = VulkanPipelineImplementation::ToVkPolygonMode(desc.Rasterizer.FillMode),
	    .cullMode = VulkanTypeConversions::ToVkCullModeFlags(desc.Rasterizer.CullMode),
	    .frontFace = VulkanPipelineImplementation::ToVkFrontFace(desc.Rasterizer.FrontFaceWinding),
	    .depthBiasEnable = VK_FALSE,
	    .depthBiasConstantFactor = 0.0f,
	    .depthBiasClamp = 0.0f,
	    .depthBiasSlopeFactor = 0.0f,
	    .lineWidth = 1.0f};

	const VkPipelineMultisampleStateCreateInfo multisampleState{
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
	    .pNext = nullptr,
	    .flags = 0,
	    .rasterizationSamples = static_cast<VkSampleCountFlagBits>(desc.SampleCount),
	    .sampleShadingEnable = VK_FALSE,
	    .minSampleShading = 0.0f,
	    .pSampleMask = nullptr,
	    .alphaToCoverageEnable = desc.Blend.AlphaToCoverageEnable ? VK_TRUE : VK_FALSE,
	    .alphaToOneEnable = VK_FALSE};

	std::array<VkPipelineColorBlendAttachmentState, 8> blendAttachments = {};
	for (std::uint32_t renderTargetIndex = 0; renderTargetIndex < desc.ColorAttachmentCount; ++renderTargetIndex)
	{
		const RhiBlendTargetState& blend = desc.Blend.Targets[desc.Blend.IndependentBlendEnable ? renderTargetIndex : 0];
		blendAttachments[renderTargetIndex] = VkPipelineColorBlendAttachmentState{
		    .blendEnable = blend.BlendEnable ? VK_TRUE : VK_FALSE,
		    .srcColorBlendFactor = VulkanPipelineImplementation::ToVkBlendFactor(blend.SourceColor),
		    .dstColorBlendFactor = VulkanPipelineImplementation::ToVkBlendFactor(blend.DestinationColor),
		    .colorBlendOp = VulkanPipelineImplementation::ToVkBlendOperation(blend.ColorOperation),
		    .srcAlphaBlendFactor = VulkanPipelineImplementation::ToVkBlendFactor(blend.SourceAlpha),
		    .dstAlphaBlendFactor = VulkanPipelineImplementation::ToVkBlendFactor(blend.DestinationAlpha),
		    .alphaBlendOp = VulkanPipelineImplementation::ToVkBlendOperation(blend.AlphaOperation),
		    .colorWriteMask = blend.ColorWriteMask};
	}
	const VkPipelineColorBlendStateCreateInfo colorBlendState{
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
	    .pNext = nullptr,
	    .flags = 0,
	    .logicOpEnable = VK_FALSE,
	    .logicOp = VK_LOGIC_OP_COPY,
	    .attachmentCount = desc.ColorAttachmentCount,
	    .pAttachments = blendAttachments.data(),
	    .blendConstants = {0.0f, 0.0f, 0.0f, 0.0f}};

	const VkPipelineDepthStencilStateCreateInfo depthStencilState{
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
	    .pNext = nullptr,
	    .flags = 0,
	    .depthTestEnable = hasDepthStencilFormat && desc.Depth.DepthEnable ? VK_TRUE : VK_FALSE,
	    .depthWriteEnable = hasDepthStencilFormat && desc.Depth.DepthWriteEnable ? VK_TRUE : VK_FALSE,
	    .depthCompareOp = VulkanTypeConversions::ToVkCompareOp(desc.Depth.DepthFunc),
	    .depthBoundsTestEnable = VK_FALSE,
	    .stencilTestEnable = hasStencilFormat && desc.Stencil.StencilEnable ? VK_TRUE : VK_FALSE,
	    .front = VulkanPipelineImplementation::BuildStencilFaceState(
	        desc.Stencil.FrontFaceStencilFunc,
	        desc.Stencil.FrontFaceStencilFailOp,
	        desc.Stencil.FrontFaceStencilDepthFailOp,
	        desc.Stencil.FrontFaceStencilPassOp,
	        desc.Stencil),
	    .back = VulkanPipelineImplementation::BuildStencilFaceState(
	        desc.Stencil.BackFaceStencilFunc,
	        desc.Stencil.BackFaceStencilFailOp,
	        desc.Stencil.BackFaceStencilDepthFailOp,
	        desc.Stencil.BackFaceStencilPassOp,
	        desc.Stencil),
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
	for (std::uint32_t renderTargetIndex = 0; renderTargetIndex < desc.ColorAttachmentCount; ++renderTargetIndex)
	{
		renderTargetFormats[renderTargetIndex] = VulkanTypeConversions::ToVkFormat(desc.ColorAttachmentFormats[renderTargetIndex]);
	}
	const VkPipelineRenderingCreateInfo renderingCreateInfo{
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
	    .pNext = nullptr,
	    .viewMask = 0,
	    .colorAttachmentCount = desc.ColorAttachmentCount,
	    .pColorAttachmentFormats = renderTargetFormats.data(),
	    .depthAttachmentFormat = VulkanTypeConversions::ToVkFormat(desc.DepthStencilAttachmentFormat),
	    .stencilAttachmentFormat =
	        hasStencilFormat ? VulkanTypeConversions::ToVkFormat(desc.DepthStencilAttachmentFormat) : VK_FORMAT_UNDEFINED};

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
		VulkanPipelineImplementation::HandlePipelineCreateFailure(debugName, "vkCreateGraphicsPipelines", result);
	}

	VulkanDebugNames::SetObjectName(
	    rhi.GetSetDebugUtilsObjectName(),
	    m_device,
	    VK_OBJECT_TYPE_PIPELINE,
	    reinterpret_cast<std::uint64_t>(m_pipeline),
	    debugName);
}

VulkanPipeline::VulkanPipeline(VulkanRhi& rhi, const ComputePipelineDesc& desc) :
    m_device(rhi.GetDevice()),
    m_bindPoint(VK_PIPELINE_BIND_POINT_COMPUTE)
{
	const std::string debugName = VulkanPipelineImplementation::ToDebugName(desc.DebugName);
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
		VulkanPipelineImplementation::HandlePipelineCreateFailure(debugName, "vkCreateComputePipelines", result);
	}

	VulkanDebugNames::SetObjectName(
	    rhi.GetSetDebugUtilsObjectName(),
	    m_device,
	    VK_OBJECT_TYPE_PIPELINE,
	    reinterpret_cast<std::uint64_t>(m_pipeline),
	    debugName);
}

VulkanPipeline::~VulkanPipeline() noexcept
{
	Reset();
}

VkPipelineLayout VulkanPipeline::GetPipelineLayout() const noexcept
{
	return m_pipelineLayout != nullptr ? m_pipelineLayout->Get() : VK_NULL_HANDLE;
}

void VulkanPipeline::Reset() noexcept
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

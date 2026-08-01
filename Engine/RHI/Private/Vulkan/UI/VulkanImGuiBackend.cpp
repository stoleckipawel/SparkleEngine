#include "Vulkan/VulkanPCH.h"

#include "Vulkan/UI/VulkanImGuiBackend.h"

#include "Vulkan/Core/VulkanResult.h"
#include "Vulkan/Commands/VulkanRenderCommandList.h"
#include "Vulkan/Descriptors/VulkanDescriptorService.h"
#include "Vulkan/VulkanRenderHardwareInterface.h"

#include <backends/imgui_impl_vulkan.h>

#include <limits>

static const auto g_vulkanImGuiBackendLogger = Logging::GetOrCreateLogger("RHI.Vulkan.ImGui");

VulkanImGuiBackend::VulkanImGuiBackend(
    VulkanRenderHardwareInterface& renderHardwareInterface,
    VulkanDescriptorService& descriptorService) noexcept :
    m_renderHardwareInterface(renderHardwareInterface), m_descriptorService(descriptorService)
{
}

void VulkanImGuiBackend::Initialize()
{
	if (m_imguiContext != nullptr)
	{
		return;
	}

	ImGuiContext* previousContext = ImGui::GetCurrentContext();
	m_imguiContext = previousContext;
	if (m_imguiContext == nullptr)
	{
		m_imguiContext = ImGui::CreateContext();
		m_ownsContext = true;
	}
	ImGui::SetCurrentContext(m_imguiContext);

	const std::uint32_t backBufferCount = m_renderHardwareInterface.GetSwapChainBackBufferCount();
	const VkFormat colorFormat = m_renderHardwareInterface.GetNativeBackBufferFormat();
	if (backBufferCount < 2 || colorFormat == VK_FORMAT_UNDEFINED)
	{
		RestoreContext(previousContext);
		Diagnostics::Fatal(g_vulkanImGuiBackendLogger, __FILE__, __LINE__, "Cannot initialize ImGui without a valid Vulkan swap chain.");
	}

	VkPipelineRenderingCreateInfo pipelineRenderingInfo{
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
	    .pNext = nullptr,
	    .viewMask = 0,
	    .colorAttachmentCount = 1,
	    .pColorAttachmentFormats = &colorFormat,
	    .depthAttachmentFormat = VK_FORMAT_UNDEFINED,
	    .stencilAttachmentFormat = VK_FORMAT_UNDEFINED};

	ImGui_ImplVulkan_InitInfo initInfo = {};
	initInfo.ApiVersion = m_renderHardwareInterface.GetVulkanApiVersion();
	initInfo.Instance = m_renderHardwareInterface.GetVulkanInstance();
	initInfo.PhysicalDevice = m_renderHardwareInterface.GetVulkanPhysicalDevice();
	initInfo.Device = m_renderHardwareInterface.GetVulkanDevice();
	initInfo.QueueFamily = m_renderHardwareInterface.GetVulkanGraphicsQueueFamilyIndex();
	initInfo.Queue = m_renderHardwareInterface.GetVulkanGraphicsQueue();
	initInfo.DescriptorPoolSize = 1024;
	initInfo.MinImageCount = backBufferCount;
	initInfo.ImageCount = backBufferCount;
	initInfo.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	initInfo.PipelineInfoMain.PipelineRenderingCreateInfo = pipelineRenderingInfo;
	initInfo.UseDynamicRendering = true;
	initInfo.CheckVkResultFn = [](VkResult result)
	{
		if (result != VK_SUCCESS)
		{
			SPDLOG_LOGGER_ERROR(g_vulkanImGuiBackendLogger, "{}", VulkanResult::FormatFailure("ImGui Vulkan backend", result));
		}
	};

	if (initInfo.Instance == VK_NULL_HANDLE || initInfo.PhysicalDevice == VK_NULL_HANDLE || initInfo.Device == VK_NULL_HANDLE ||
	    initInfo.Queue == VK_NULL_HANDLE || initInfo.QueueFamily == UINT32_MAX)
	{
		RestoreContext(previousContext);
		Diagnostics::Fatal(g_vulkanImGuiBackendLogger, __FILE__, __LINE__, "Cannot initialize ImGui with incomplete Vulkan device state.");
	}

	if (!ImGui_ImplVulkan_Init(&initInfo))
	{
		RestoreContext(previousContext);
		Diagnostics::Fatal(g_vulkanImGuiBackendLogger, __FILE__, __LINE__, "ImGui Vulkan renderer initialization failed.");
	}

	const VkSamplerCreateInfo samplerInfo{
	    .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
	    .pNext = nullptr,
	    .flags = 0,
	    .magFilter = VK_FILTER_LINEAR,
	    .minFilter = VK_FILTER_LINEAR,
	    .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
	    .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
	    .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
	    .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
	    .mipLodBias = 0.0f,
	    .anisotropyEnable = VK_FALSE,
	    .maxAnisotropy = 1.0f,
	    .compareEnable = VK_FALSE,
	    .compareOp = VK_COMPARE_OP_ALWAYS,
	    .minLod = 0.0f,
	    .maxLod = VK_LOD_CLAMP_NONE,
	    .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
	    .unnormalizedCoordinates = VK_FALSE};
	const VkResult samplerResult = vkCreateSampler(m_renderHardwareInterface.GetVulkanDevice(), &samplerInfo, nullptr, &m_imguiSampler);
	if (!VulkanResult::Succeeded(samplerResult) || m_imguiSampler == VK_NULL_HANDLE)
	{
		SPDLOG_LOGGER_ERROR(g_vulkanImGuiBackendLogger, "{}", VulkanResult::FormatFailure("vkCreateSampler", samplerResult));
		ImGui_ImplVulkan_Shutdown();
		RestoreContext(previousContext);
		Diagnostics::Fatal(g_vulkanImGuiBackendLogger, __FILE__, __LINE__, VulkanResult::FormatFailure("vkCreateSampler", samplerResult));
	}

	RestoreContext(previousContext);
}

void VulkanImGuiBackend::BeginFrame() noexcept
{
	ImGuiContext* previousContext = ActivateContext();
	ImGui_ImplVulkan_NewFrame();
	RestoreContext(previousContext);
}

std::uint64_t VulkanImGuiBackend::ResolveTextureId(RhiGpuDescriptorHandle shaderResourceView) noexcept
{
	ImGuiContext* previousContext = ActivateContext();
	const std::uint64_t textureId = GetTextureId(m_descriptorService.GetRegisteredImageView(shaderResourceView));
	RestoreContext(previousContext);
	return textureId;
}

void VulkanImGuiBackend::RenderDrawData(ImDrawData* drawData) noexcept
{
	if (drawData == nullptr)
	{
		return;
	}

	ImGuiContext* previousContext = ActivateContext();
	RenderCommandList& commandList = m_renderHardwareInterface.GetGraphicsCommandList(m_renderHardwareInterface.GetCurrentFrameIndex());
	VkCommandBuffer commandBuffer = static_cast<VkCommandBuffer>(
	    commandList.GetNativeHandle(
	        RhiNativeInteropRequest{
	            .Consumer = ERhiNativeInteropConsumer::Presentation,
	            .Reason = "Render ImGui draw data through Vulkan backend"})
	        .Value);
	if (commandBuffer == VK_NULL_HANDLE)
	{
		RestoreContext(previousContext);
		return;
	}

	ImGui_ImplVulkan_RenderDrawData(drawData, commandBuffer);
	RestoreContext(previousContext);
}

void VulkanImGuiBackend::ReleaseTexture(
    ImTextureData& texture) noexcept
{
	ImGuiContext* previousContext = ActivateContext();
	texture.UnusedFrames = (std::numeric_limits<int>::max)();
	texture.WantDestroyNextFrame = true;
	texture.SetStatus(ImTextureStatus_WantDestroy);
	ImGui_ImplVulkan_UpdateTexture(&texture);
	RestoreContext(previousContext);
}

std::uint64_t VulkanImGuiBackend::GetTextureId(VkImageView imageView) noexcept
{
	if (imageView == VK_NULL_HANDLE || m_imguiSampler == VK_NULL_HANDLE)
	{
		return 0;
	}

	for (const TextureBinding& binding : m_textureBindings)
	{
		if (binding.ImageView == imageView)
		{
			return reinterpret_cast<std::uint64_t>(binding.DescriptorSet);
		}
	}

	const VkDescriptorSet descriptorSet =
	    ImGui_ImplVulkan_AddTexture(m_imguiSampler, imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	if (descriptorSet == VK_NULL_HANDLE)
	{
		return 0;
	}

	m_textureBindings.push_back(TextureBinding{.ImageView = imageView, .DescriptorSet = descriptorSet});
	return reinterpret_cast<std::uint64_t>(descriptorSet);
}

void VulkanImGuiBackend::Shutdown() noexcept
{
	if (m_imguiContext == nullptr)
	{
		return;
	}
	ImGuiContext* previousContext = ActivateContext();
	for (const TextureBinding& binding : m_textureBindings)
	{
		if (binding.DescriptorSet != VK_NULL_HANDLE)
		{
			ImGui_ImplVulkan_RemoveTexture(binding.DescriptorSet);
		}
	}
	m_textureBindings.clear();

	if (m_imguiSampler != VK_NULL_HANDLE)
	{
		vkDestroySampler(m_renderHardwareInterface.GetVulkanDevice(), m_imguiSampler, nullptr);
		m_imguiSampler = VK_NULL_HANDLE;
	}

	ImGui_ImplVulkan_Shutdown();
	RestoreContext(previousContext);
	if (m_ownsContext)
	{
		ImGui::DestroyContext(m_imguiContext);
	}
	m_imguiContext = nullptr;
	m_ownsContext = false;
}

ImGuiContext* VulkanImGuiBackend::ActivateContext() const noexcept
{
	ImGuiContext* previousContext = ImGui::GetCurrentContext();
	ImGui::SetCurrentContext(m_imguiContext);
	return previousContext;
}

void VulkanImGuiBackend::RestoreContext(ImGuiContext* context) noexcept
{
	ImGui::SetCurrentContext(context);
}

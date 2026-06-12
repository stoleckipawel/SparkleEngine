#include "Vulkan/VulkanPCH.h"

#include "Vulkan/UI/VulkanImGuiBackend.h"

#include "Config/RenderConfig.h"
#include "Vulkan/Core/VulkanResult.h"
#include "Vulkan/Commands/VulkanRenderCommandList.h"
#include "Vulkan/VulkanRenderHardwareInterface.h"

#include <backends/imgui_impl_vulkan.h>

static const auto g_vulkanImGuiBackendLogger = Logging::GetOrCreateLogger("RHI.Vulkan.ImGui");

VulkanImGuiBackend::VulkanImGuiBackend(VulkanRenderHardwareInterface& renderHardware) noexcept : m_renderHardware(&renderHardware) {}

bool VulkanImGuiBackend::Initialize()
{
	if (m_renderHardware == nullptr)
	{
		return false;
	}

	const std::uint32_t backBufferCount = m_renderHardware->GetSwapChainBackBufferCount();
	const VkFormat colorFormat = m_renderHardware->GetNativeBackBufferFormat();
	if (backBufferCount < 2 || colorFormat == VK_FORMAT_UNDEFINED)
	{
		return false;
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
	initInfo.ApiVersion = m_renderHardware->GetVulkanApiVersion();
	initInfo.Instance = m_renderHardware->GetVulkanInstance();
	initInfo.PhysicalDevice = m_renderHardware->GetVulkanPhysicalDevice();
	initInfo.Device = m_renderHardware->GetVulkanDevice();
	initInfo.QueueFamily = m_renderHardware->GetVulkanGraphicsQueueFamilyIndex();
	initInfo.Queue = m_renderHardware->GetVulkanGraphicsQueue();
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
		return false;
	}

	if (!ImGui_ImplVulkan_Init(&initInfo))
	{
		return false;
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
	const VkResult samplerResult = vkCreateSampler(m_renderHardware->GetVulkanDevice(), &samplerInfo, nullptr, &m_imguiSampler);
	if (!VulkanResult::Succeeded(samplerResult) || m_imguiSampler == VK_NULL_HANDLE)
	{
		SPDLOG_LOGGER_ERROR(g_vulkanImGuiBackendLogger, "{}", VulkanResult::FormatFailure("vkCreateSampler", samplerResult));
		ImGui_ImplVulkan_Shutdown();
		return false;
	}

	return true;
}

void VulkanImGuiBackend::BeginFrame() noexcept
{
	ImGui_ImplVulkan_NewFrame();
}

void VulkanImGuiBackend::RenderDrawData(ImDrawData* drawData) noexcept
{
	if (m_renderHardware == nullptr || drawData == nullptr)
	{
		return;
	}

	RenderCommandList& commandList = m_renderHardware->GetGraphicsCommandList(m_renderHardware->GetCurrentFrameIndex());
	VkCommandBuffer commandBuffer = static_cast<VkCommandBuffer>(commandList.GetNativeHandle().Value);
	if (commandBuffer == VK_NULL_HANDLE)
	{
		return;
	}

	ImGui_ImplVulkan_RenderDrawData(drawData, commandBuffer);
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
	if (m_renderHardware != nullptr)
	{
		m_renderHardware->WaitForIdle();
	}

	for (const TextureBinding& binding : m_textureBindings)
	{
		if (binding.DescriptorSet != VK_NULL_HANDLE)
		{
			ImGui_ImplVulkan_RemoveTexture(binding.DescriptorSet);
		}
	}
	m_textureBindings.clear();

	if (m_renderHardware != nullptr && m_imguiSampler != VK_NULL_HANDLE)
	{
		vkDestroySampler(m_renderHardware->GetVulkanDevice(), m_imguiSampler, nullptr);
		m_imguiSampler = VK_NULL_HANDLE;
	}

	ImGui_ImplVulkan_Shutdown();
}

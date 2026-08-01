#include "Vulkan/VulkanPCH.h"

#include "Vulkan/SwapChain/VulkanSwapChain.h"

#include "CVars/RHICVars.h"
#include "Frame/RhiFrameConstants.h"
#include "Presentation/RhiPresentationDefaults.h"
#include "Vulkan/Commands/VulkanCommandQueue.h"
#include "Vulkan/Core/VulkanResult.h"
#include "Vulkan/Device/VulkanRhi.h"
#include "Vulkan/VulkanTypeConversions.h"
#include "Window/Window.h"


#include <algorithm>
#include <format>
#include <limits>
#include <utility>

static const auto g_vulkanSwapChainLogger = Logging::GetOrCreateLogger("RHI.Vulkan.SwapChain");

VulkanSwapChain::VulkanSwapChain(
    VulkanRhi& rhi,
    Window& window,
    PixelFormat backBufferFormat,
    const RhiPresentationConfiguration& presentationConfiguration) :
    m_rhi(rhi),
    m_window(&window),
    m_backBufferFormat(backBufferFormat),
    m_configuredBackBufferCount(presentationConfiguration.BackBufferCount),
    m_maximumFramesInFlight(presentationConfiguration.MaximumFramesInFlight),
    m_imageAvailableSemaphores(m_maximumFramesInFlight, VK_NULL_HANDLE)
{
	CreateSurface();
	CreatePresentationSemaphores();
	CreateSwapChain();
	CreateBackBufferImageViews();
}

VulkanSwapChain::~VulkanSwapChain() noexcept
{
	ReleaseSwapChain();
	ReleasePresentationSemaphores();
	if (m_surface != VK_NULL_HANDLE)
	{
		vkDestroySurfaceKHR(m_rhi.GetInstance(), m_surface, nullptr);
		m_surface = VK_NULL_HANDLE;
	}
}

bool VulkanSwapChain::AcquireNextImage(std::uint32_t frameIndex) noexcept
{
	if (m_swapChain == VK_NULL_HANDLE || !HasValidWindowSize())
	{
		return false;
	}

	const VkResult result = vkAcquireNextImageKHR(
	    m_rhi.GetDevice(),
	    m_swapChain,
	    std::numeric_limits<std::uint64_t>::max(),
	    GetImageAvailableSemaphore(frameIndex),
	    VK_NULL_HANDLE,
	    &m_currentBackBufferIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		m_resizeRequested = true;
		return false;
	}
	if (result == VK_SUBOPTIMAL_KHR || VulkanResult::Succeeded(result))
	{
		return true;
	}

	Diagnostics::Fatal(
	    g_vulkanSwapChainLogger,
	    __FILE__,
	    __LINE__,
	    VulkanResult::FormatFailure("vkAcquireNextImageKHR", result));
	return false;
}

bool VulkanSwapChain::Present(VkSemaphore renderFinishedSemaphore) noexcept
{
	if (m_swapChain == VK_NULL_HANDLE)
	{
		return false;
	}

	const VkSemaphore waitSemaphores[] = {renderFinishedSemaphore};
	const VkPresentInfoKHR presentInfo{
	    .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
	    .pNext = nullptr,
	    .waitSemaphoreCount = renderFinishedSemaphore != VK_NULL_HANDLE ? 1u : 0u,
	    .pWaitSemaphores = renderFinishedSemaphore != VK_NULL_HANDLE ? waitSemaphores : nullptr,
	    .swapchainCount = 1,
	    .pSwapchains = &m_swapChain,
	    .pImageIndices = &m_currentBackBufferIndex,
	    .pResults = nullptr};

	const VkResult result = m_rhi.GetCommandQueue(ERhiQueueType::Graphics).Present(presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		m_resizeRequested = true;
		return true;
	}
	if (!VulkanResult::Succeeded(result))
	{
		Diagnostics::Fatal(g_vulkanSwapChainLogger, __FILE__, __LINE__, VulkanResult::FormatFailure("vkQueuePresentKHR", result));
	}
	if (m_vsyncEnabled != CVarVSync.Get())
	{
		m_resizeRequested = true;
		return true;
	}
	return false;
}

void VulkanSwapChain::Resize() noexcept
{
	if (!HasValidWindowSize())
	{
		return;
	}

	VkSwapchainKHR oldSwapChain = m_swapChain;
	ReleaseBackBufferImageViews();
	CreateSwapChain(oldSwapChain);
	if (oldSwapChain != VK_NULL_HANDLE)
	{
		vkDestroySwapchainKHR(m_rhi.GetDevice(), oldSwapChain, nullptr);
	}
	CreateBackBufferImageViews();
}

RhiResourceHandle VulkanSwapChain::GetCurrentBackBufferResource() const noexcept
{
	return RhiResourceHandle{GetCurrentBackBufferImage()};
}

VkImage VulkanSwapChain::GetCurrentBackBufferImage() const noexcept
{
	return GetBackBufferImage(m_currentBackBufferIndex);
}

VkImageView VulkanSwapChain::GetCurrentBackBufferImageView() const noexcept
{
	return GetBackBufferImageView(m_currentBackBufferIndex);
}

bool VulkanSwapChain::ConsumeResizeRequest() noexcept
{
	return std::exchange(m_resizeRequested, false);
}

VkSemaphore VulkanSwapChain::GetImageAvailableSemaphore(std::uint32_t frameIndex) const noexcept
{
	return m_imageAvailableSemaphores[frameIndex % m_imageAvailableSemaphores.size()];
}

VkSemaphore VulkanSwapChain::GetCurrentRenderFinishedSemaphore() const noexcept
{
	return m_currentBackBufferIndex < m_backBuffers.size() ? m_backBuffers[m_currentBackBufferIndex].RenderFinishedSemaphore : VK_NULL_HANDLE;
}

VkImage VulkanSwapChain::GetBackBufferImage(std::uint32_t index) const noexcept
{
	return index < m_backBuffers.size() ? m_backBuffers[index].Image : VK_NULL_HANDLE;
}

VkImageView VulkanSwapChain::GetBackBufferImageView(std::uint32_t index) const noexcept
{
	return index < m_backBuffers.size() ? m_backBuffers[index].ImageView : VK_NULL_HANDLE;
}

RhiViewport VulkanSwapChain::GetDefaultViewport() const noexcept
{
	return RhiViewport{
	    .X = 0.0f,
	    .Y = 0.0f,
	    .Width = static_cast<float>(m_extent.width),
	    .Height = static_cast<float>(m_extent.height),
	    .MinDepth = 0.0f,
	    .MaxDepth = 1.0f};
}

RhiRect VulkanSwapChain::GetDefaultScissorRect() const noexcept
{
	return RhiRect{.Left = 0, .Top = 0, .Right = static_cast<std::int32_t>(m_extent.width), .Bottom = static_cast<std::int32_t>(m_extent.height)};
}

void VulkanSwapChain::CreateSurface()
{
	const VkWin32SurfaceCreateInfoKHR createInfo{
	    .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
	    .pNext = nullptr,
	    .flags = 0,
	    .hinstance = GetModuleHandleW(nullptr),
	    .hwnd = m_window != nullptr ? m_window->GetHWND() : nullptr};

	const VkResult result = vkCreateWin32SurfaceKHR(m_rhi.GetInstance(), &createInfo, nullptr, &m_surface);
	if (!VulkanResult::Succeeded(result))
	{
		Diagnostics::Fatal(g_vulkanSwapChainLogger, __FILE__, __LINE__, VulkanResult::FormatFailure("vkCreateWin32SurfaceKHR", result));
	}

	VkBool32 supportsPresent = VK_FALSE;
	(void)vkGetPhysicalDeviceSurfaceSupportKHR(m_rhi.GetPhysicalDevice(), m_rhi.GetGraphicsQueueFamilyIndex(), m_surface, &supportsPresent);
	if (supportsPresent != VK_TRUE)
	{
		Diagnostics::Fatal(g_vulkanSwapChainLogger, __FILE__, __LINE__, "Selected Vulkan graphics queue family does not support the Win32 surface.");
	}
}

void VulkanSwapChain::CreateSwapChain(VkSwapchainKHR oldSwapChain)
{
	VkSurfaceCapabilitiesKHR capabilities{};
	VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_rhi.GetPhysicalDevice(), m_surface, &capabilities);
	if (!VulkanResult::Succeeded(result))
	{
		Diagnostics::Fatal(g_vulkanSwapChainLogger, __FILE__, __LINE__, VulkanResult::FormatFailure("vkGetPhysicalDeviceSurfaceCapabilitiesKHR", result));
	}

	m_surfaceFormat = SelectSurfaceFormat();
	m_vsyncEnabled = CVarVSync.Get();
	m_presentMode = SelectPresentMode(m_vsyncEnabled);
	m_extent = SelectExtent(capabilities);

	VkSwapchainCreateInfoKHR createInfo{
	    .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
	    .pNext = nullptr,
	    .flags = 0,
	    .surface = m_surface,
	    .minImageCount = SelectImageCount(capabilities),
	    .imageFormat = m_surfaceFormat.format,
	    .imageColorSpace = m_surfaceFormat.colorSpace,
	    .imageExtent = m_extent,
	    .imageArrayLayers = 1,
	    .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
	    .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
	    .queueFamilyIndexCount = 0,
	    .pQueueFamilyIndices = nullptr,
	    .preTransform = capabilities.currentTransform,
	    .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
	    .presentMode = m_presentMode,
	    .clipped = VK_TRUE,
	    .oldSwapchain = oldSwapChain};
	m_rhi.ConfigureResourceQueueSharing(createInfo);

	result = vkCreateSwapchainKHR(m_rhi.GetDevice(), &createInfo, nullptr, &m_swapChain);
	if (!VulkanResult::Succeeded(result))
	{
		Diagnostics::Fatal(g_vulkanSwapChainLogger, __FILE__, __LINE__, VulkanResult::FormatFailure("vkCreateSwapchainKHR", result));
	}

}

void VulkanSwapChain::CreateBackBufferImageViews()
{
	std::uint32_t imageCount = 0;
	VkResult result = vkGetSwapchainImagesKHR(m_rhi.GetDevice(), m_swapChain, &imageCount, nullptr);
	if (!VulkanResult::Succeeded(result))
	{
		Diagnostics::Fatal(g_vulkanSwapChainLogger, __FILE__, __LINE__, VulkanResult::FormatFailure("vkGetSwapchainImagesKHR", result));
	}

	std::vector<VkImage> images(imageCount);
	result = vkGetSwapchainImagesKHR(m_rhi.GetDevice(), m_swapChain, &imageCount, images.data());
	if (!VulkanResult::Succeeded(result))
	{
		Diagnostics::Fatal(g_vulkanSwapChainLogger, __FILE__, __LINE__, VulkanResult::FormatFailure("vkGetSwapchainImagesKHR", result));
	}

	m_backBuffers.clear();
	m_backBuffers.reserve(images.size());
	for (VkImage image : images)
	{
		BackBufferRecord record{.Image = image, .ImageView = CreateImageView(image, m_surfaceFormat.format)};
		const VkSemaphoreCreateInfo semaphoreInfo{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, .pNext = nullptr, .flags = 0};
		const VkResult semaphoreResult = vkCreateSemaphore(m_rhi.GetDevice(), &semaphoreInfo, nullptr, &record.RenderFinishedSemaphore);
		if (!VulkanResult::Succeeded(semaphoreResult))
		{
			Diagnostics::Fatal(
			    g_vulkanSwapChainLogger,
			    __FILE__,
			    __LINE__,
			    VulkanResult::FormatFailure("vkCreateSemaphore", semaphoreResult));
		}
		m_backBuffers.push_back(record);
	}
	m_currentBackBufferIndex = 0;
}

void VulkanSwapChain::CreatePresentationSemaphores()
{
	const VkSemaphoreCreateInfo createInfo{
	    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
	    .pNext = nullptr,
	    .flags = 0};
	for (VkSemaphore& semaphore : m_imageAvailableSemaphores)
	{
		const VkResult result = vkCreateSemaphore(m_rhi.GetDevice(), &createInfo, nullptr, &semaphore);
		if (!VulkanResult::Succeeded(result))
		{
			Diagnostics::Fatal(
			    g_vulkanSwapChainLogger,
			    __FILE__,
			    __LINE__,
			    VulkanResult::FormatFailure("vkCreateSemaphore(imageAvailable)", result));
		}
	}
}

void VulkanSwapChain::ReleasePresentationSemaphores() noexcept
{
	for (VkSemaphore& semaphore : m_imageAvailableSemaphores)
	{
		if (semaphore != VK_NULL_HANDLE)
		{
			vkDestroySemaphore(m_rhi.GetDevice(), semaphore, nullptr);
			semaphore = VK_NULL_HANDLE;
		}
	}
}

void VulkanSwapChain::ReleaseBackBufferImageViews() noexcept
{
	for (BackBufferRecord& backBuffer : m_backBuffers)
	{
		if (backBuffer.ImageView != VK_NULL_HANDLE)
		{
			vkDestroyImageView(m_rhi.GetDevice(), backBuffer.ImageView, nullptr);
			backBuffer.ImageView = VK_NULL_HANDLE;
		}
		if (backBuffer.RenderFinishedSemaphore != VK_NULL_HANDLE)
		{
			vkDestroySemaphore(m_rhi.GetDevice(), backBuffer.RenderFinishedSemaphore, nullptr);
			backBuffer.RenderFinishedSemaphore = VK_NULL_HANDLE;
		}
		backBuffer.Image = VK_NULL_HANDLE;
	}
	m_backBuffers.clear();
}

void VulkanSwapChain::ReleaseSwapChain() noexcept
{
	ReleaseBackBufferImageViews();
	if (m_swapChain != VK_NULL_HANDLE)
	{
		vkDestroySwapchainKHR(m_rhi.GetDevice(), m_swapChain, nullptr);
		m_swapChain = VK_NULL_HANDLE;
	}
}

bool VulkanSwapChain::HasValidWindowSize() const noexcept
{
	return m_window != nullptr && m_window->HasValidSize();
}

VkSurfaceFormatKHR VulkanSwapChain::SelectSurfaceFormat() const
{
	std::uint32_t formatCount = 0;
	VkResult result = vkGetPhysicalDeviceSurfaceFormatsKHR(m_rhi.GetPhysicalDevice(), m_surface, &formatCount, nullptr);
	if (!VulkanResult::Succeeded(result) || formatCount == 0)
	{
		Diagnostics::Fatal(g_vulkanSwapChainLogger, __FILE__, __LINE__, "No Vulkan surface formats are available.");
	}

	std::vector<VkSurfaceFormatKHR> formats(formatCount);
	result = vkGetPhysicalDeviceSurfaceFormatsKHR(m_rhi.GetPhysicalDevice(), m_surface, &formatCount, formats.data());
	if (!VulkanResult::Succeeded(result))
	{
		Diagnostics::Fatal(g_vulkanSwapChainLogger, __FILE__, __LINE__, VulkanResult::FormatFailure("vkGetPhysicalDeviceSurfaceFormatsKHR", result));
	}

	const VkFormat requestedFormat = VulkanTypeConversions::ToVkFormat(m_backBufferFormat);
	const auto requestedIt = std::find_if(formats.begin(), formats.end(), [requestedFormat](const VkSurfaceFormatKHR& format) noexcept {
		return format.format == requestedFormat && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	});
	if (requestedIt != formats.end())
	{
		return *requestedIt;
	}

	Diagnostics::Fatal(
	    g_vulkanSwapChainLogger,
	    __FILE__,
	    __LINE__,
	    std::format("Requested Vulkan present format '{}' is not supported by the surface.", PixelFormatName(m_backBufferFormat)));
}

VkPresentModeKHR VulkanSwapChain::SelectPresentMode(bool vsyncEnabled) const
{
	if (vsyncEnabled)
	{
		return VK_PRESENT_MODE_FIFO_KHR;
	}

	std::uint32_t presentModeCount = 0;
	VkResult result = vkGetPhysicalDeviceSurfacePresentModesKHR(m_rhi.GetPhysicalDevice(), m_surface, &presentModeCount, nullptr);
	if (!VulkanResult::Succeeded(result) || presentModeCount == 0)
	{
		Diagnostics::Fatal(
		    g_vulkanSwapChainLogger,
		    __FILE__,
		    __LINE__,
		    "VSync is disabled, but Vulkan present modes could not be queried.");
	}

	std::vector<VkPresentModeKHR> presentModes(presentModeCount);
	result = vkGetPhysicalDeviceSurfacePresentModesKHR(m_rhi.GetPhysicalDevice(), m_surface, &presentModeCount, presentModes.data());
	if (!VulkanResult::Succeeded(result))
	{
		Diagnostics::Fatal(
		    g_vulkanSwapChainLogger,
		    __FILE__,
		    __LINE__,
		    VulkanResult::FormatFailure("vkGetPhysicalDeviceSurfacePresentModesKHR", result));
	}

	if (std::find(presentModes.begin(), presentModes.end(), VK_PRESENT_MODE_IMMEDIATE_KHR) != presentModes.end())
	{
		return VK_PRESENT_MODE_IMMEDIATE_KHR;
	}
	Diagnostics::Fatal(
	    g_vulkanSwapChainLogger,
	    __FILE__,
	    __LINE__,
	    "VSync is disabled, but the Vulkan surface does not expose immediate presentation.");
}

VkExtent2D VulkanSwapChain::SelectExtent(const VkSurfaceCapabilitiesKHR& capabilities) const noexcept
{
	if (capabilities.currentExtent.width != std::numeric_limits<std::uint32_t>::max())
	{
		return capabilities.currentExtent;
	}

	VkExtent2D extent{
	    .width = m_window != nullptr ? m_window->GetWidth() : 1u,
	    .height = m_window != nullptr ? m_window->GetHeight() : 1u};
	extent.width = std::clamp(extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
	extent.height = std::clamp(extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
	return extent;
}

std::uint32_t VulkanSwapChain::SelectImageCount(const VkSurfaceCapabilitiesKHR& capabilities) const noexcept
{
	const std::uint32_t imageCount = m_configuredBackBufferCount;
	if (imageCount < capabilities.minImageCount ||
	    (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount))
	{
		Diagnostics::Fatal(
		    g_vulkanSwapChainLogger,
		    __FILE__,
		    __LINE__,
		    std::format(
		        "Vulkan surface image-count range [{}, {}] does not include the configured count {}.",
		        capabilities.minImageCount,
		        capabilities.maxImageCount == 0 ? std::numeric_limits<std::uint32_t>::max() : capabilities.maxImageCount,
		        imageCount));
	}
	return imageCount;
}

VkImageView VulkanSwapChain::CreateImageView(VkImage image, VkFormat format) const
{
	const VkImageViewCreateInfo createInfo{
	    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
	    .pNext = nullptr,
	    .flags = 0,
	    .image = image,
	    .viewType = VK_IMAGE_VIEW_TYPE_2D,
	    .format = format,
	    .components = VkComponentMapping{
	        .r = VK_COMPONENT_SWIZZLE_IDENTITY,
	        .g = VK_COMPONENT_SWIZZLE_IDENTITY,
	        .b = VK_COMPONENT_SWIZZLE_IDENTITY,
	        .a = VK_COMPONENT_SWIZZLE_IDENTITY},
	    .subresourceRange = VkImageSubresourceRange{
	        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
	        .baseMipLevel = 0,
	        .levelCount = 1,
	        .baseArrayLayer = 0,
	        .layerCount = 1}};

	VkImageView imageView = VK_NULL_HANDLE;
	const VkResult result = vkCreateImageView(m_rhi.GetDevice(), &createInfo, nullptr, &imageView);
	if (!VulkanResult::Succeeded(result))
	{
		Diagnostics::Fatal(g_vulkanSwapChainLogger, __FILE__, __LINE__, VulkanResult::FormatFailure("vkCreateImageView", result));
	}
	return imageView;
}

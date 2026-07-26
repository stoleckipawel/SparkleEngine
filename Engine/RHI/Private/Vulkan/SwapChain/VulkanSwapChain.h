#pragma once

#include "Device/RenderHardwareInterface.h"
#include "Frame/RhiFrameConstants.h"
#include "Formats/PixelFormat.h"
#include "Vulkan/VulkanIncludes.h"

#include <array>
#include <cstdint>
#include <vector>

class VulkanRhi;
class Window;

class VulkanSwapChain final
{
  public:
	VulkanSwapChain(VulkanRhi& rhi, Window& window, PixelFormat backBufferFormat);
	~VulkanSwapChain() noexcept;

	VulkanSwapChain(const VulkanSwapChain&) = delete;
	VulkanSwapChain& operator=(const VulkanSwapChain&) = delete;
	VulkanSwapChain(VulkanSwapChain&&) = delete;
	VulkanSwapChain& operator=(VulkanSwapChain&&) = delete;

	bool AcquireNextImage(std::uint32_t frameIndex) noexcept;
	bool Present(VkSemaphore renderFinishedSemaphore) noexcept;
	void ResizeAfterDeviceIdle() noexcept;
	bool ConsumeResizeRequest() noexcept;

	std::uint32_t GetCurrentBackBufferIndex() const noexcept { return m_currentBackBufferIndex; }
	RhiResourceHandle GetCurrentBackBufferResource() const noexcept;
	VkImage GetCurrentBackBufferImage() const noexcept;
	VkImageView GetCurrentBackBufferImageView() const noexcept;
	VkSemaphore GetImageAvailableSemaphore(std::uint32_t frameIndex) const noexcept;
	VkSemaphore GetCurrentRenderFinishedSemaphore() const noexcept;
	VkImage GetBackBufferImage(std::uint32_t index) const noexcept;
	VkImageView GetBackBufferImageView(std::uint32_t index) const noexcept;
	std::uint32_t GetBackBufferCount() const noexcept { return static_cast<std::uint32_t>(m_backBuffers.size()); }
	RhiViewport GetDefaultViewport() const noexcept;
	RhiRect GetDefaultScissorRect() const noexcept;
	PixelFormat GetBackBufferFormat() const noexcept { return m_backBufferFormat; }
	VkFormat GetNativeBackBufferFormat() const noexcept { return m_surfaceFormat.format; }

  private:
	struct BackBufferRecord final
	{
		VkImage Image = VK_NULL_HANDLE;
		VkImageView ImageView = VK_NULL_HANDLE;
		VkSemaphore RenderFinishedSemaphore = VK_NULL_HANDLE;
	};

	void CreateSurface();
	void CreatePresentationSemaphores();
	void ReleasePresentationSemaphores() noexcept;
	void CreateSwapChain(VkSwapchainKHR oldSwapChain = VK_NULL_HANDLE);
	void CreateBackBufferImageViews();
	void ReleaseBackBufferImageViews() noexcept;
	void ReleaseSwapChain() noexcept;
	bool HasValidWindowSize() const noexcept;
	VkSurfaceFormatKHR SelectSurfaceFormat() const;
	VkPresentModeKHR SelectPresentMode() const;
	VkExtent2D SelectExtent(const VkSurfaceCapabilitiesKHR& capabilities) const noexcept;
	std::uint32_t SelectImageCount(const VkSurfaceCapabilitiesKHR& capabilities) const noexcept;
	VkImageView CreateImageView(VkImage image, VkFormat format) const;

	VulkanRhi& m_rhi;
	Window* m_window = nullptr;
	VkSurfaceKHR m_surface = VK_NULL_HANDLE;
	VkSwapchainKHR m_swapChain = VK_NULL_HANDLE;
	VkSurfaceFormatKHR m_surfaceFormat = {};
	VkPresentModeKHR m_presentMode = VK_PRESENT_MODE_FIFO_KHR;
	VkExtent2D m_extent = {};
	PixelFormat m_backBufferFormat = PixelFormat::Unknown;
	std::vector<BackBufferRecord> m_backBuffers;
	std::array<VkSemaphore, RhiFrameConstants::FramesInFlight> m_imageAvailableSemaphores{};
	std::uint32_t m_currentBackBufferIndex = 0;
	bool m_resizeRequested = false;
};

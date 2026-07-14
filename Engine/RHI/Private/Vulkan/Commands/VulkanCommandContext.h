#pragma once

#include "Frame/RhiFrameConstants.h"
#include "Vulkan/VulkanIncludes.h"

#include <array>
#include <cstdint>
#include <memory>

class VulkanRenderCommandList;
class VulkanRhi;

class VulkanCommandContext final
{
  public:
	explicit VulkanCommandContext(VulkanRhi& rhi);
	~VulkanCommandContext() noexcept;

	VulkanCommandContext(const VulkanCommandContext&) = delete;
	VulkanCommandContext& operator=(const VulkanCommandContext&) = delete;
	VulkanCommandContext(VulkanCommandContext&&) = delete;
	VulkanCommandContext& operator=(VulkanCommandContext&&) = delete;

	void BeginFrame(std::uint32_t frameIndex) noexcept;
	void SubmitFrame(std::uint32_t frameIndex, VkSemaphore waitSemaphore, VkSemaphore signalSemaphore) noexcept;
	void CancelFrame(std::uint32_t frameIndex) noexcept;
	void WaitForIdle() noexcept;

	VulkanRenderCommandList& GetCommandList(std::uint32_t frameIndex) noexcept;
	VkCommandBuffer GetCommandBuffer(std::uint32_t frameIndex) const noexcept;
	bool IsCommandBufferRecording(VkCommandBuffer commandBuffer) const noexcept;
	VkSemaphore GetImageAvailableSemaphore(std::uint32_t frameIndex) const noexcept;
	VkSemaphore GetRenderFinishedSemaphore(std::uint32_t frameIndex) const noexcept;
	std::uint64_t GetNextRetireFenceValue() const noexcept { return m_nextRetireFenceValue; }
	std::uint64_t GetCompletedRetireFenceValue() const noexcept { return m_completedRetireFenceValue; }

  private:
	struct FrameState final
	{
		VkCommandPool CommandPool = VK_NULL_HANDLE;
		VkCommandBuffer CommandBuffer = VK_NULL_HANDLE;
		VkFence Fence = VK_NULL_HANDLE;
		VkSemaphore ImageAvailableSemaphore = VK_NULL_HANDLE;
		VkSemaphore RenderFinishedSemaphore = VK_NULL_HANDLE;
		std::uint64_t RetireFenceValue = 0;
		std::unique_ptr<VulkanRenderCommandList> CommandList;
		bool IsRecording = false;
	};

	FrameState& GetFrameState(std::uint32_t frameIndex) noexcept;
	const FrameState& GetFrameState(std::uint32_t frameIndex) const noexcept;
	void CreateFrameState(std::uint32_t frameIndex);
	void DestroyFrameState(FrameState& frameState) noexcept;
	void NameFrameState(std::uint32_t frameIndex, FrameState& frameState) noexcept;

	VulkanRhi& m_rhi;
	std::array<FrameState, RhiFrameConstants::FramesInFlight> m_frames;
	std::uint64_t m_nextRetireFenceValue = 1;
	std::uint64_t m_completedRetireFenceValue = 0;
};

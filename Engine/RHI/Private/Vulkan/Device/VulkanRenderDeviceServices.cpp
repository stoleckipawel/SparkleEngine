#include "Vulkan/VulkanPCH.h"

#include "Device/RenderDeviceBackendFactory.h"

#include "Frame/RhiFrameConstants.h"
#include "Vulkan/Commands/VulkanCommandContext.h"
#include "Vulkan/Commands/VulkanCommandQueue.h"
#include "Vulkan/Commands/VulkanRenderCommandList.h"
#include "Vulkan/Device/VulkanRhi.h"
#include "Vulkan/Memory/VulkanGpuMemoryAllocator.h"
#include "Vulkan/SwapChain/VulkanSwapChain.h"
#include "Vulkan/VulkanRenderHardwareInterface.h"

#include "Window/Window.h"

#include <array>
#include <span>

class VulkanRenderDeviceServices final : public RenderDeviceBackendServices
{
 public:
	static std::unique_ptr<VulkanRenderDeviceServices> Create(
	    Window& window,
	    PixelFormat backBufferFormat) noexcept;
	~VulkanRenderDeviceServices() noexcept override;

	VulkanRenderDeviceServices(const VulkanRenderDeviceServices&) = delete;
	VulkanRenderDeviceServices& operator=(const VulkanRenderDeviceServices&) = delete;
	VulkanRenderDeviceServices(VulkanRenderDeviceServices&&) = delete;
	VulkanRenderDeviceServices& operator=(VulkanRenderDeviceServices&&) = delete;

	RenderHardwareInterface& GetRenderHardwareInterface() noexcept override;
	const RenderHardwareInterface& GetRenderHardwareInterface() const noexcept override;
	RhiImGuiRenderer& GetImGuiRenderer() noexcept override;
	void WaitForIdle() noexcept override;
	void ResizeSwapChain() noexcept override;
	void BeginFrame() noexcept override;
	RenderCommandList& GetCurrentGraphicsCommandList() noexcept override;
	RenderCommandList& GetGraphicsCommandList(std::uint32_t frameIndex) noexcept override;
	RenderCommandList& BeginCommandList(ERhiQueueType queueType) noexcept override;
	RhiSubmissionToken SubmitCommandList(
	    RenderCommandList& commandList,
	    std::span<const RhiSubmissionToken> waitTokens) noexcept override;
	void QueueWait(ERhiQueueType waitQueue, RhiSubmissionToken executionToken) noexcept override;
	void WaitForSubmission(RhiSubmissionToken token) noexcept override;
	bool IsSubmissionComplete(RhiSubmissionToken token) const noexcept override;
	RhiSubmissionToken GetLastSubmittedToken(ERhiQueueType queueType) const noexcept override;
	void SubmitFrame() noexcept override;
	void AdvanceFrameInFlight() noexcept override;
	void CloseExecuteAndFlushCurrentFrame() noexcept override;

  private:
	VulkanRenderDeviceServices() noexcept = default;

	std::unique_ptr<VulkanRhi> m_rhi;
	std::unique_ptr<VulkanGpuMemoryAllocator> m_memoryAllocator;
	std::unique_ptr<VulkanSwapChain> m_swapChain;
	std::unique_ptr<VulkanCommandContext> m_commandContext;
	std::unique_ptr<VulkanRenderHardwareInterface> m_renderHardwareInterface;
	std::uint32_t m_currentFrameIndex = 0;
	std::array<RhiSubmissionState, RhiQueueTypeCount> m_pendingQueueWaits;
	bool m_hasAcquiredBackBuffer = false;
	bool m_hasConsumedAcquireSemaphore = false;
};

std::unique_ptr<RenderDeviceBackendServices> CreateVulkanRenderDeviceServices(
    Window& window,
    PixelFormat backBufferFormat,
    RhiExternalFeatureHooks externalFeatureHooks) noexcept
{
	(void) externalFeatureHooks;
	return VulkanRenderDeviceServices::Create(window, backBufferFormat);
}

std::unique_ptr<VulkanRenderDeviceServices> VulkanRenderDeviceServices::Create(
    Window& window,
    PixelFormat backBufferFormat) noexcept
{
	auto services = std::unique_ptr<VulkanRenderDeviceServices>(new VulkanRenderDeviceServices());
	{
		services->m_rhi = std::make_unique<VulkanRhi>();
	}
	{
		services->m_memoryAllocator = std::make_unique<VulkanGpuMemoryAllocator>(*services->m_rhi);
	}
	{
		services->m_swapChain = std::make_unique<VulkanSwapChain>(*services->m_rhi, window, backBufferFormat);
	}
	{
		services->m_commandContext = std::make_unique<VulkanCommandContext>(*services->m_rhi);
	}
	{
		services->m_renderHardwareInterface = std::make_unique<VulkanRenderHardwareInterface>(
		    *services->m_rhi,
		    *services->m_swapChain,
		    *services->m_commandContext,
		    *services->m_memoryAllocator);
	}
	return services;
}

VulkanRenderDeviceServices::~VulkanRenderDeviceServices() noexcept
{
	m_renderHardwareInterface.reset();
	m_commandContext.reset();
	m_swapChain.reset();
	m_memoryAllocator.reset();
	m_rhi.reset();
}

RenderHardwareInterface& VulkanRenderDeviceServices::GetRenderHardwareInterface() noexcept
{
	return *m_renderHardwareInterface;
}

const RenderHardwareInterface& VulkanRenderDeviceServices::GetRenderHardwareInterface() const noexcept
{
	return *m_renderHardwareInterface;
}

RhiImGuiRenderer& VulkanRenderDeviceServices::GetImGuiRenderer() noexcept
{
	return m_renderHardwareInterface->GetImGuiRenderer();
}

void VulkanRenderDeviceServices::WaitForIdle() noexcept
{
	m_hasAcquiredBackBuffer = false;
	m_hasConsumedAcquireSemaphore = false;
	m_renderHardwareInterface->WaitForIdle();
}

void VulkanRenderDeviceServices::ResizeSwapChain() noexcept
{
	m_renderHardwareInterface->WaitForIdle();
	m_swapChain->ResizeAfterDeviceIdle();
	m_renderHardwareInterface->RebuildSwapChainBackBufferViews();
}

void VulkanRenderDeviceServices::BeginFrame() noexcept
{
	m_renderHardwareInterface->SetCurrentFrameIndex(m_currentFrameIndex);
	m_hasConsumedAcquireSemaphore = false;
	for (RhiSubmissionState& waits : m_pendingQueueWaits)
	{
		waits.Clear();
	}
	m_commandContext->BeginFrame(m_currentFrameIndex);
	(void)BeginCommandList(ERhiQueueType::Graphics);
	m_renderHardwareInterface->ResetTransientFrameResources();
	m_hasAcquiredBackBuffer = m_swapChain->AcquireNextImage(m_currentFrameIndex);
	if (!m_hasAcquiredBackBuffer)
	{
		m_commandContext->CancelFrame(m_currentFrameIndex);
		m_renderHardwareInterface->RebuildSwapChainBackBufferViews();
	}
}

RenderCommandList& VulkanRenderDeviceServices::GetCurrentGraphicsCommandList() noexcept
{
	return m_renderHardwareInterface->GetGraphicsCommandList(m_currentFrameIndex);
}

RenderCommandList& VulkanRenderDeviceServices::GetGraphicsCommandList(std::uint32_t frameIndex) noexcept
{
	return m_renderHardwareInterface->GetGraphicsCommandList(frameIndex);
}

RenderCommandList& VulkanRenderDeviceServices::BeginCommandList(ERhiQueueType queueType) noexcept
{
	return m_commandContext->BeginCommandList(queueType, m_currentFrameIndex);
}

RhiSubmissionToken VulkanRenderDeviceServices::SubmitCommandList(
	RenderCommandList& commandList,
	std::span<const RhiSubmissionToken> waitTokens) noexcept
{
	const ERhiQueueType queueType = commandList.GetQueueType();
	const std::size_t queueIndex = RhiQueueTypeToIndex(queueType);
	RhiSubmissionState resolvedWaits = m_pendingQueueWaits[queueIndex];
	for (const RhiSubmissionToken waitToken : waitTokens)
	{
		resolvedWaits.MarkUsed(waitToken);
	}
	m_pendingQueueWaits[queueIndex].Clear();
	std::array<RhiSubmissionToken, RhiQueueTypeCount> resolvedWaitTokens{};
	const std::size_t resolvedWaitCount = resolvedWaits.CopyTokens(resolvedWaitTokens);
	VkSemaphore binaryWaitSemaphore = VK_NULL_HANDLE;
	if (queueType == ERhiQueueType::Graphics && m_hasAcquiredBackBuffer && !m_hasConsumedAcquireSemaphore)
	{
		binaryWaitSemaphore = m_swapChain->GetImageAvailableSemaphore(m_currentFrameIndex);
		m_hasConsumedAcquireSemaphore = true;
	}
	auto& vulkanCommandList = static_cast<VulkanRenderCommandList&>(commandList);
	const RhiSubmissionToken token = m_commandContext->SubmitCommandList(
	    vulkanCommandList,
	    m_currentFrameIndex,
	    std::span<const RhiSubmissionToken>(resolvedWaitTokens.data(), resolvedWaitCount),
	    binaryWaitSemaphore,
	    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
	commandList.ResolveTrackedResources(token);
	return token;
}

void VulkanRenderDeviceServices::QueueWait(
	ERhiQueueType waitQueue,
	RhiSubmissionToken executionToken) noexcept
{
	if (!IsRhiQueueTypeValid(waitQueue) || !executionToken.IsValid() || waitQueue == executionToken.Queue)
	{
		return;
	}
	m_pendingQueueWaits[RhiQueueTypeToIndex(waitQueue)].MarkUsed(executionToken);
}

void VulkanRenderDeviceServices::WaitForSubmission(RhiSubmissionToken token) noexcept
{
	if (token.IsValid())
	{
		m_rhi->GetCommandQueue(token.Queue).WaitForSubmission(token.Value);
	}
}

bool VulkanRenderDeviceServices::IsSubmissionComplete(RhiSubmissionToken token) const noexcept
{
	return !token.IsValid() || m_rhi->GetCommandQueue(token.Queue).IsSubmissionComplete(token.Value);
}

RhiSubmissionToken VulkanRenderDeviceServices::GetLastSubmittedToken(ERhiQueueType queueType) const noexcept
{
	return IsRhiQueueTypeValid(queueType) ? m_rhi->GetCommandQueue(queueType).GetLastSubmittedToken() : RhiSubmissionToken{};
}

void VulkanRenderDeviceServices::SubmitFrame() noexcept
{
	if (!m_hasAcquiredBackBuffer)
	{
		return;
	}

	const VkSemaphore renderFinishedSemaphore = m_swapChain->GetCurrentRenderFinishedSemaphore();
	RenderCommandList& graphicsCommandList = GetCurrentGraphicsCommandList();
	auto& vulkanCommandList = static_cast<VulkanRenderCommandList&>(graphicsCommandList);
	m_renderHardwareInterface->PrepareCurrentBackBufferForPresentation(vulkanCommandList);
	RhiSubmissionState waits = m_pendingQueueWaits[RhiQueueTypeToIndex(ERhiQueueType::Graphics)];
	m_pendingQueueWaits[RhiQueueTypeToIndex(ERhiQueueType::Graphics)].Clear();
	for (const ERhiQueueType queueType : {ERhiQueueType::Compute, ERhiQueueType::Copy})
	{
		waits.MarkUsed(m_rhi->GetCommandQueue(queueType).GetLastSubmittedToken());
	}
	std::array<RhiSubmissionToken, RhiQueueTypeCount> waitTokens{};
	const std::size_t waitTokenCount = waits.CopyTokens(waitTokens);
	const VkSemaphore acquireSemaphore = m_hasConsumedAcquireSemaphore
	                                         ? VK_NULL_HANDLE
	                                         : m_swapChain->GetImageAvailableSemaphore(m_currentFrameIndex);
	const RhiSubmissionToken frameToken = m_commandContext->SubmitCommandList(
	    vulkanCommandList,
	    m_currentFrameIndex,
	    std::span<const RhiSubmissionToken>(waitTokens.data(), waitTokenCount),
	    acquireSemaphore,
	    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
	    renderFinishedSemaphore);
	graphicsCommandList.ResolveTrackedResources(frameToken);
	m_hasConsumedAcquireSemaphore = true;
	if (!frameToken.IsValid())
	{
		m_hasAcquiredBackBuffer = false;
		return;
	}
	if (m_swapChain->Present(renderFinishedSemaphore))
	{
		m_renderHardwareInterface->RebuildSwapChainBackBufferViews();
	}
	m_hasAcquiredBackBuffer = false;
	m_hasConsumedAcquireSemaphore = false;
}

void VulkanRenderDeviceServices::AdvanceFrameInFlight() noexcept
{
	m_currentFrameIndex = (m_currentFrameIndex + 1u) % RhiFrameConstants::FramesInFlight;
	m_renderHardwareInterface->SetCurrentFrameIndex(m_currentFrameIndex);
}

void VulkanRenderDeviceServices::CloseExecuteAndFlushCurrentFrame() noexcept
{
	if (m_hasAcquiredBackBuffer)
	{
		SubmitFrame();
	}
	m_renderHardwareInterface->WaitForIdle();
}

#include "Vulkan/VulkanPCH.h"

#include "Device/RenderDeviceBackendFactory.h"

#include "Frame/RhiFrameConstants.h"
#include "Vulkan/Commands/VulkanCommandContext.h"
#include "Vulkan/Device/VulkanRhi.h"
#include "Vulkan/Memory/VulkanGpuMemoryAllocator.h"
#include "Vulkan/SwapChain/VulkanSwapChain.h"
#include "Vulkan/VulkanRenderHardwareInterface.h"

#include "Window/Window.h"

#include <array>
#include <algorithm>
#include <span>
#include <vector>

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
	void Flush() noexcept override;
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
	std::array<bool, RhiQueueTypeCount> m_queueRecording{};
	std::array<std::vector<RhiSubmissionToken>, RhiQueueTypeCount> m_pendingQueueWaits;
	bool m_hasAcquiredBackBuffer = false;
	bool m_graphicsReadyForPresent = false;
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
	if (m_renderHardwareInterface != nullptr)
	{
		m_renderHardwareInterface->WaitForIdle();
	}

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

void VulkanRenderDeviceServices::Flush() noexcept
{
	m_hasAcquiredBackBuffer = false;
	m_graphicsReadyForPresent = false;
	m_renderHardwareInterface->WaitForIdle();
}

void VulkanRenderDeviceServices::ResizeSwapChain() noexcept
{
	m_renderHardwareInterface->WaitForIdle();
	m_swapChain->Resize();
	m_renderHardwareInterface->RebuildSwapChainBackBufferViews();
}

void VulkanRenderDeviceServices::BeginFrame() noexcept
{
	m_renderHardwareInterface->SetCurrentFrameIndex(m_currentFrameIndex);
	m_queueRecording.fill(false);
	m_graphicsReadyForPresent = false;
	m_commandContext->BeginFrame(m_currentFrameIndex);
	m_queueRecording[RhiQueueTypeToIndex(ERhiQueueType::Graphics)] = true;
	m_renderHardwareInterface->GetCommandList(ERhiQueueType::Graphics, m_currentFrameIndex).ResetTrackedResources();
	m_renderHardwareInterface->ResetTransientFrameResources();
	m_hasAcquiredBackBuffer = m_swapChain->AcquireNextImage(m_commandContext->GetImageAvailableSemaphore(m_currentFrameIndex));
	if (!m_hasAcquiredBackBuffer)
	{
		m_commandContext->CancelFrame(m_currentFrameIndex);
		m_queueRecording[RhiQueueTypeToIndex(ERhiQueueType::Graphics)] = false;
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
	const std::size_t queueIndex = RhiQueueTypeToIndex(queueType);
	if (!m_queueRecording[queueIndex])
	{
		m_commandContext->BeginCommandList(queueType, m_currentFrameIndex);
		m_queueRecording[queueIndex] = true;
		m_renderHardwareInterface->GetCommandList(queueType, m_currentFrameIndex).ResetTrackedResources();
	}
	return m_renderHardwareInterface->GetCommandList(queueType, m_currentFrameIndex);
}

RhiSubmissionToken VulkanRenderDeviceServices::SubmitCommandList(
	RenderCommandList& commandList,
	std::span<const RhiSubmissionToken> waitTokens) noexcept
{
	const ERhiQueueType queueType = commandList.GetQueueType();
	const std::size_t queueIndex = RhiQueueTypeToIndex(queueType);
	if (!m_queueRecording[queueIndex])
	{
		return m_commandContext->GetLastSubmittedToken(queueType);
	}

	RhiSubmissionToken token{};
	std::vector<RhiSubmissionToken> resolvedWaitTokens = m_pendingQueueWaits[queueIndex];
	resolvedWaitTokens.insert(resolvedWaitTokens.end(), waitTokens.begin(), waitTokens.end());
	m_pendingQueueWaits[queueIndex].clear();
	if (queueType == ERhiQueueType::Graphics && m_hasAcquiredBackBuffer)
	{
		const VkSemaphore imageAvailableSemaphore = m_commandContext->GetImageAvailableSemaphore(m_currentFrameIndex);
		const VkSemaphore renderFinishedSemaphore = m_swapChain->GetCurrentRenderFinishedSemaphore();
		token = m_commandContext->SubmitCommandList(
		    queueType,
		    m_currentFrameIndex,
		    resolvedWaitTokens,
		    imageAvailableSemaphore,
		    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		    renderFinishedSemaphore);
		m_graphicsReadyForPresent = token.IsValid();
	}
	else
	{
		token = m_commandContext->SubmitCommandList(queueType, m_currentFrameIndex, resolvedWaitTokens);
	}
	commandList.ResolveTrackedResources(token);
	m_queueRecording[queueIndex] = false;
	return token;
}

void VulkanRenderDeviceServices::QueueWait(
	ERhiQueueType waitQueue,
	RhiSubmissionToken executionToken) noexcept
{
	if (!executionToken.IsValid() || waitQueue == executionToken.Queue)
	{
		return;
	}
	auto& waits = m_pendingQueueWaits[RhiQueueTypeToIndex(waitQueue)];
	const auto existing = std::find(waits.begin(), waits.end(), executionToken);
	if (existing == waits.end())
	{
		waits.push_back(executionToken);
	}
}

void VulkanRenderDeviceServices::WaitForSubmission(RhiSubmissionToken token) noexcept
{
	m_commandContext->WaitForSubmission(token);
}

bool VulkanRenderDeviceServices::IsSubmissionComplete(RhiSubmissionToken token) const noexcept
{
	return m_commandContext->IsSubmissionComplete(token);
}

RhiSubmissionToken VulkanRenderDeviceServices::GetLastSubmittedToken(ERhiQueueType queueType) const noexcept
{
	return m_commandContext->GetLastSubmittedToken(queueType);
}

void VulkanRenderDeviceServices::SubmitFrame() noexcept
{
	if (!m_hasAcquiredBackBuffer)
	{
		return;
	}

	const VkSemaphore renderFinishedSemaphore = m_swapChain->GetCurrentRenderFinishedSemaphore();
	if (m_queueRecording[RhiQueueTypeToIndex(ERhiQueueType::Graphics)])
	{
		RenderCommandList& graphicsCommandList = GetCurrentGraphicsCommandList();
		(void)SubmitCommandList(graphicsCommandList, {});
	}
	if (!m_graphicsReadyForPresent)
	{
		m_hasAcquiredBackBuffer = false;
		return;
	}
	if (m_swapChain->Present(renderFinishedSemaphore))
	{
		m_renderHardwareInterface->RebuildSwapChainBackBufferViews();
	}
	m_hasAcquiredBackBuffer = false;
	m_graphicsReadyForPresent = false;
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

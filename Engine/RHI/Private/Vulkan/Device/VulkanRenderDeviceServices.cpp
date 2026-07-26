#include "Vulkan/VulkanPCH.h"

#include "Device/RenderDeviceBackendFactory.h"

#include "Commands/RhiCommandRecordingLeaseAccess.h"
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
	RenderCommandList& BeginCurrentGraphicsCommandList() noexcept override;
	RhiCommandRecordingLease AcquireCommandRecordingLease(
	    ERhiQueueType queueType,
	    RhiCommandRecordingOwner owner) noexcept override;
	RhiSubmissionToken SubmitCommandRecordingLease(
	    RhiCommandRecordingLease&& lease,
	    std::span<const RhiSubmissionToken> waitTokens) noexcept override;
	RhiSubmissionToken SubmitCurrentGraphicsCommandList(
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

	void Initialize(Window& window, PixelFormat backBufferFormat);
	void InitializeDevice();
	void InitializePresentation(Window& window, PixelFormat backBufferFormat);
	void InitializeHardwareInterface();
	void BeginFrameRecording();
	void AcquireFrameBackBuffer();
	RhiSubmissionState ConsumeQueueWaits(
	    ERhiQueueType queueType,
	    std::span<const RhiSubmissionToken> waitTokens) noexcept;
	VkSemaphore ConsumeAcquireSemaphore(ERhiQueueType queueType) noexcept;
	RhiSubmissionState ConsumePresentationWaits() noexcept;
	void CompletePresentation(
	    RhiSubmissionToken frameToken,
	    VkSemaphore renderFinishedSemaphore) noexcept;
	RhiSubmissionToken SubmitCommandList(
	    RenderCommandList& commandList,
	    std::span<const RhiSubmissionToken> waitTokens) noexcept;

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
	services->Initialize(window, backBufferFormat);
	return services;
}

void VulkanRenderDeviceServices::Initialize(
    Window& window,
    PixelFormat backBufferFormat)
{
	InitializeDevice();
	InitializePresentation(window, backBufferFormat);
	InitializeHardwareInterface();
}

void VulkanRenderDeviceServices::InitializeDevice()
{
	m_rhi = std::make_unique<VulkanRhi>();
	m_memoryAllocator = std::make_unique<VulkanGpuMemoryAllocator>(*m_rhi);
	m_commandContext = std::make_unique<VulkanCommandContext>(*m_rhi);
}

void VulkanRenderDeviceServices::InitializePresentation(
    Window& window,
    PixelFormat backBufferFormat)
{
	m_swapChain = std::make_unique<VulkanSwapChain>(*m_rhi, window, backBufferFormat);
}

void VulkanRenderDeviceServices::InitializeHardwareInterface()
{
	m_renderHardwareInterface = std::make_unique<VulkanRenderHardwareInterface>(
	    *m_rhi,
	    *m_swapChain,
	    *m_commandContext,
	    *m_memoryAllocator);
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
	BeginFrameRecording();
	AcquireFrameBackBuffer();
}

void VulkanRenderDeviceServices::BeginFrameRecording()
{
	m_hasConsumedAcquireSemaphore = false;
	for (RhiSubmissionState& waits : m_pendingQueueWaits)
	{
		waits.Clear();
	}

	m_commandContext->BeginFrame(m_currentFrameIndex);
	(void)BeginCurrentGraphicsCommandList();
	m_renderHardwareInterface->ResetTransientFrameResources();
}

void VulkanRenderDeviceServices::AcquireFrameBackBuffer()
{
	m_hasAcquiredBackBuffer = m_swapChain->AcquireNextImage(m_currentFrameIndex);
	if (!m_hasAcquiredBackBuffer && m_swapChain->ConsumeResizeRequest())
	{
		ResizeSwapChain();
		m_hasAcquiredBackBuffer =
		    m_swapChain->AcquireNextImage(m_currentFrameIndex);
	}
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

RenderCommandList& VulkanRenderDeviceServices::BeginCurrentGraphicsCommandList() noexcept
{
	return m_commandContext->BeginCommandList(ERhiQueueType::Graphics, m_currentFrameIndex);
}

RhiCommandRecordingLease VulkanRenderDeviceServices::AcquireCommandRecordingLease(
    ERhiQueueType queueType,
    RhiCommandRecordingOwner owner) noexcept
{
	RenderCommandList& commandList = m_commandContext->BeginCommandList(queueType, m_currentFrameIndex);
	return RhiCommandRecordingLeaseAccess::Create(
	    RhiCommandRecordingLeaseInitialization{
	        .BackendState = &commandList,
	        .CommandList = &commandList,
	        .QueueType = queueType,
	        .FrameSlot = m_currentFrameIndex,
	        .ContextId = RhiCommandRecordingContextId{.Value = 0},
	        .Owner = owner});
}

RhiSubmissionToken VulkanRenderDeviceServices::SubmitCommandRecordingLease(
    RhiCommandRecordingLease&& lease,
    std::span<const RhiSubmissionToken> waitTokens) noexcept
{
	if (!lease.IsClosed())
	{
		lease.Close();
	}

	const RhiCommandRecordingLeaseBackendState state =
	    RhiCommandRecordingLeaseAccess::Consume(std::move(lease));
	if (state.CommandList == nullptr || !state.Closed || state.FrameSlot != m_currentFrameIndex ||
	    state.QueueType != state.CommandList->GetQueueType())
	{
		return {};
	}

	return SubmitCommandList(*state.CommandList, waitTokens);
}

RhiSubmissionToken VulkanRenderDeviceServices::SubmitCurrentGraphicsCommandList(
    std::span<const RhiSubmissionToken> waitTokens) noexcept
{
	return SubmitCommandList(GetCurrentGraphicsCommandList(), waitTokens);
}

RhiSubmissionToken VulkanRenderDeviceServices::SubmitCommandList(
    RenderCommandList& commandList,
    std::span<const RhiSubmissionToken> waitTokens) noexcept
{
	const ERhiQueueType queueType = commandList.GetQueueType();
	const RhiSubmissionState resolvedWaits = ConsumeQueueWaits(queueType, waitTokens);
	std::array<RhiSubmissionToken, RhiQueueTypeCount> resolvedWaitTokens{};
	const std::size_t resolvedWaitCount = resolvedWaits.CopyTokens(resolvedWaitTokens);
	const VkSemaphore binaryWaitSemaphore = ConsumeAcquireSemaphore(queueType);

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

RhiSubmissionState VulkanRenderDeviceServices::ConsumeQueueWaits(
    ERhiQueueType queueType,
    std::span<const RhiSubmissionToken> waitTokens) noexcept
{
	const std::size_t queueIndex = RhiQueueTypeToIndex(queueType);
	RhiSubmissionState resolvedWaits = m_pendingQueueWaits[queueIndex];
	for (const RhiSubmissionToken waitToken : waitTokens)
	{
		resolvedWaits.MarkUsed(waitToken);
	}

	m_pendingQueueWaits[queueIndex].Clear();
	return resolvedWaits;
}

VkSemaphore VulkanRenderDeviceServices::ConsumeAcquireSemaphore(ERhiQueueType queueType) noexcept
{
	if (queueType == ERhiQueueType::Graphics && m_hasAcquiredBackBuffer && !m_hasConsumedAcquireSemaphore)
	{
		m_hasConsumedAcquireSemaphore = true;
		return m_swapChain->GetImageAvailableSemaphore(m_currentFrameIndex);
	}

	return VK_NULL_HANDLE;
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

	const RhiSubmissionState waits = ConsumePresentationWaits();
	std::array<RhiSubmissionToken, RhiQueueTypeCount> waitTokens{};
	const std::size_t waitTokenCount = waits.CopyTokens(waitTokens);
	const VkSemaphore acquireSemaphore = ConsumeAcquireSemaphore(ERhiQueueType::Graphics);
	const RhiSubmissionToken frameToken = m_commandContext->SubmitCommandList(
	    vulkanCommandList,
	    m_currentFrameIndex,
	    std::span<const RhiSubmissionToken>(waitTokens.data(), waitTokenCount),
	    acquireSemaphore,
	    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
	    renderFinishedSemaphore);

	graphicsCommandList.ResolveTrackedResources(frameToken);
	CompletePresentation(frameToken, renderFinishedSemaphore);
}

RhiSubmissionState VulkanRenderDeviceServices::ConsumePresentationWaits() noexcept
{
	RhiSubmissionState waits = m_pendingQueueWaits[RhiQueueTypeToIndex(ERhiQueueType::Graphics)];
	m_pendingQueueWaits[RhiQueueTypeToIndex(ERhiQueueType::Graphics)].Clear();
	for (const ERhiQueueType queueType : {ERhiQueueType::Compute, ERhiQueueType::Copy})
	{
		waits.MarkUsed(m_rhi->GetCommandQueue(queueType).GetLastSubmittedToken());
	}

	return waits;
}

void VulkanRenderDeviceServices::CompletePresentation(
    RhiSubmissionToken frameToken,
    VkSemaphore renderFinishedSemaphore) noexcept
{
	if (!frameToken.IsValid())
	{
		m_hasAcquiredBackBuffer = false;
		return;
	}
	if (m_swapChain->Present(renderFinishedSemaphore))
	{
		(void)m_swapChain->ConsumeResizeRequest();
		ResizeSwapChain();
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

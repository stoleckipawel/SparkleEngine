#include "Vulkan/VulkanPCH.h"

#include "Device/RenderDeviceBackendFactory.h"

#include "Frame/RhiFrameConstants.h"
#include "Vulkan/Commands/VulkanCommandRecordingContext.h"
#include "Vulkan/Commands/VulkanCommandQueue.h"
#include "Vulkan/Commands/VulkanRenderCommandList.h"
#include "Vulkan/Descriptors/VulkanDescriptorService.h"
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
	void SettleForShutdown() noexcept override;
	void ResizeSwapChain() noexcept override;
	void BeginFrame() noexcept override;
	void PrepareCommandRecording() noexcept override;
	RenderCommandList& GetCurrentGraphicsCommandList() noexcept override;
	RenderCommandList& GetGraphicsCommandList(std::uint32_t frameIndex) noexcept override;
	RenderCommandList& BeginCurrentGraphicsCommandList() noexcept override;
	RhiCommandRecordingLease AcquireCommandRecordingLease(
	    ERhiQueueType queueType,
	    RhiCommandRecordingOwner owner) noexcept override;
	RhiCommandRecordingLease TakeCurrentGraphicsCommandRecordingLease() noexcept override;
	RhiSubmissionToken SubmitCommandRecordingLease(
	    RhiCommandRecordingLease&& lease,
	    std::span<const RhiSubmissionToken> waitTokens) noexcept override;
	RhiSubmissionToken SubmitCommandRecordingBatch(
	    std::span<RhiCommandRecordingLease> leases,
	    std::span<const RhiSubmissionToken> waitTokens) noexcept override;
	RhiSubmissionToken SubmitCurrentGraphicsCommandList(
	    std::span<const RhiSubmissionToken> waitTokens) noexcept override;
	void QueueWait(ERhiQueueType waitQueue, RhiSubmissionToken executionToken) noexcept override;
	void WaitForSubmission(RhiSubmissionToken token) noexcept override;
	bool IsSubmissionComplete(RhiSubmissionToken token) const noexcept override;
	RhiSubmissionToken GetLastSubmittedToken(ERhiQueueType queueType) const noexcept override;
	void SubmitFrame() noexcept override;
	void AdvanceFrameInFlight() noexcept override;

  private:
	VulkanRenderDeviceServices() noexcept = default;

	void Initialize(Window& window, PixelFormat backBufferFormat);
	void InitializeDevice();
	void InitializePresentation(Window& window, PixelFormat backBufferFormat);
	void InitializeHardwareInterface();
	void BeginFrameRecording();
	void AcquireFrameBackBuffer();
	void DrainSwapChainQueue() noexcept;
	RhiSubmissionState ConsumeQueueWaits(
	    ERhiQueueType queueType,
	    std::span<const RhiSubmissionToken> waitTokens) noexcept;
	VkSemaphore ConsumeAcquireSemaphore(ERhiQueueType queueType) noexcept;
	RhiSubmissionState ConsumePresentationWaits() noexcept;
	void CompletePresentation(
	    RhiSubmissionToken frameToken,
	    VkSemaphore renderFinishedSemaphore) noexcept;
	RhiSubmissionToken SubmitLease(
	    RhiCommandRecordingLease&& lease,
	    std::span<const RhiSubmissionToken> waitTokens,
	    VkSemaphore binarySignalSemaphore = VK_NULL_HANDLE) noexcept;
	RhiSubmissionToken SubmitLeaseBatch(
	    std::span<RhiCommandRecordingLease> leases,
	    std::span<const RhiSubmissionToken> waitTokens,
	    VkSemaphore binarySignalSemaphore = VK_NULL_HANDLE) noexcept;

	std::unique_ptr<VulkanRhi> m_rhi;
	std::unique_ptr<VulkanGpuMemoryAllocator> m_memoryAllocator;
	std::unique_ptr<VulkanSwapChain> m_swapChain;
	std::unique_ptr<VulkanCommandRecordingContext> m_commandRecordingContext;
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
	    *m_memoryAllocator);

	m_commandRecordingContext =
	    std::make_unique<VulkanCommandRecordingContext>(
	        *m_rhi,
	        *m_memoryAllocator,
	        *m_renderHardwareInterface->m_descriptorService);
	m_renderHardwareInterface->SetCommandRecordingContext(
	    *m_commandRecordingContext);
}

VulkanRenderDeviceServices::~VulkanRenderDeviceServices() noexcept
{
	m_commandRecordingContext.reset();
	m_renderHardwareInterface.reset();
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

void VulkanRenderDeviceServices::SettleForShutdown() noexcept
{
	m_hasAcquiredBackBuffer = false;
	m_hasConsumedAcquireSemaphore = false;
	m_renderHardwareInterface->WaitForIdle();
}

void VulkanRenderDeviceServices::ResizeSwapChain() noexcept
{
	DrainSwapChainQueue();
	m_swapChain->Resize();
	m_renderHardwareInterface->RebuildSwapChainBackBufferViews();
}

void VulkanRenderDeviceServices::BeginFrame() noexcept
{
	m_renderHardwareInterface->SetCurrentFrameIndex(m_currentFrameIndex);
	BeginFrameRecording();
	AcquireFrameBackBuffer();
}

void VulkanRenderDeviceServices::PrepareCommandRecording() noexcept
{
	m_renderHardwareInterface->m_descriptorService->PublishRecordingReadView();
	m_memoryAllocator->PublishRecordingReadView();
}

void VulkanRenderDeviceServices::BeginFrameRecording()
{
	m_hasConsumedAcquireSemaphore = false;
	for (RhiSubmissionState& waits : m_pendingQueueWaits)
	{
		waits.Clear();
	}

	m_commandRecordingContext->BeginFrame(m_currentFrameIndex);
	m_renderHardwareInterface->ResetTransientFrameResources();
	(void)BeginCurrentGraphicsCommandList();
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
		m_commandRecordingContext->CancelFrame(m_currentFrameIndex);
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
	return m_commandRecordingContext->BeginCurrentGraphicsCommandList(
	    m_currentFrameIndex);
}

RhiCommandRecordingLease VulkanRenderDeviceServices::AcquireCommandRecordingLease(
    ERhiQueueType queueType,
    RhiCommandRecordingOwner owner) noexcept
{
	return m_commandRecordingContext->Acquire(
	    queueType,
	    m_currentFrameIndex,
	    owner);
}

RhiCommandRecordingLease
VulkanRenderDeviceServices::TakeCurrentGraphicsCommandRecordingLease() noexcept
{
	return m_commandRecordingContext
	    ->TakeCurrentGraphicsCommandRecordingLease(
	        m_currentFrameIndex);
}

RhiSubmissionToken VulkanRenderDeviceServices::SubmitCommandRecordingLease(
    RhiCommandRecordingLease&& lease,
    std::span<const RhiSubmissionToken> waitTokens) noexcept
{
	return SubmitLease(std::move(lease), waitTokens);
}

RhiSubmissionToken VulkanRenderDeviceServices::SubmitCommandRecordingBatch(
    std::span<RhiCommandRecordingLease> leases,
    std::span<const RhiSubmissionToken> waitTokens) noexcept
{
	return SubmitLeaseBatch(leases, waitTokens);
}

RhiSubmissionToken VulkanRenderDeviceServices::SubmitCurrentGraphicsCommandList(
    std::span<const RhiSubmissionToken> waitTokens) noexcept
{
	const RhiSubmissionState resolvedWaits =
	    ConsumeQueueWaits(ERhiQueueType::Graphics, waitTokens);
	std::array<RhiSubmissionToken, RhiQueueTypeCount> resolvedWaitTokens{};
	const std::size_t resolvedWaitCount =
	    resolvedWaits.CopyTokens(resolvedWaitTokens);

	return m_commandRecordingContext->SubmitCurrentGraphicsCommandList(
	    m_currentFrameIndex,
	    std::span<const RhiSubmissionToken>(
	        resolvedWaitTokens.data(),
	        resolvedWaitCount),
	    ConsumeAcquireSemaphore(ERhiQueueType::Graphics));
}

RhiSubmissionToken VulkanRenderDeviceServices::SubmitLease(
    RhiCommandRecordingLease&& lease,
    std::span<const RhiSubmissionToken> waitTokens,
    VkSemaphore binarySignalSemaphore) noexcept
{
	const ERhiQueueType queueType = lease.GetQueueType();
	const RhiSubmissionState resolvedWaits = ConsumeQueueWaits(queueType, waitTokens);
	std::array<RhiSubmissionToken, RhiQueueTypeCount> resolvedWaitTokens{};
	const std::size_t resolvedWaitCount = resolvedWaits.CopyTokens(resolvedWaitTokens);

	return m_commandRecordingContext->Submit(
	    std::move(lease),
	    std::span<const RhiSubmissionToken>(resolvedWaitTokens.data(), resolvedWaitCount),
	    ConsumeAcquireSemaphore(queueType),
	    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
	    binarySignalSemaphore);
}

RhiSubmissionToken VulkanRenderDeviceServices::SubmitLeaseBatch(
    std::span<RhiCommandRecordingLease> leases,
    std::span<const RhiSubmissionToken> waitTokens,
    VkSemaphore binarySignalSemaphore) noexcept
{
	if (leases.empty())
	{
		return {};
	}

	const ERhiQueueType queueType = leases.front().GetQueueType();
	const RhiSubmissionState resolvedWaits =
	    ConsumeQueueWaits(queueType, waitTokens);
	std::array<RhiSubmissionToken, RhiQueueTypeCount> resolvedWaitTokens{};
	const std::size_t resolvedWaitCount =
	    resolvedWaits.CopyTokens(resolvedWaitTokens);

	return m_commandRecordingContext->SubmitBatch(
	    leases,
	    std::span<const RhiSubmissionToken>(
	        resolvedWaitTokens.data(),
	        resolvedWaitCount),
	    ConsumeAcquireSemaphore(queueType),
	    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
	    binarySignalSemaphore);
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

void VulkanRenderDeviceServices::DrainSwapChainQueue() noexcept
{
	VulkanCommandQueue& graphicsQueue =
	    m_rhi->GetCommandQueue(ERhiQueueType::Graphics);
	const RhiSubmissionToken lastSubmission =
	    graphicsQueue.GetLastSubmittedToken();
	if (lastSubmission.IsValid())
	{
		graphicsQueue.WaitForSubmission(lastSubmission.Value);
	}

	graphicsQueue.DrainForSwapChainRecreation();
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
	const RhiSubmissionToken frameToken =
	    m_commandRecordingContext->SubmitCurrentGraphicsCommandList(
	    m_currentFrameIndex,
	    std::span<const RhiSubmissionToken>(waitTokens.data(), waitTokenCount),
	    acquireSemaphore,
	    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
	    renderFinishedSemaphore);

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

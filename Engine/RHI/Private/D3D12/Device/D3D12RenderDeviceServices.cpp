#include "PCH.h"

#include "Device/RenderDeviceBackendFactory.h"

#include "D3D12/D3D12RenderHardwareInterface.h"
#include "D3D12/Commands/D3D12CommandRecordingContext.h"
#include "D3D12/Device/D3D12Rhi.h"
#include "D3D12/SwapChain/D3D12SwapChain.h"
#include "D3D12/Descriptors/D3D12DescriptorHeapManager.h"
#include "D3D12/Memory/D3D12GpuMemoryAllocator.h"
#include "D3D12/Resources/D3D12UploadService.h"
#include "D3D12/Samplers/D3D12SamplerLibrary.h"

#include "Window/Window.h"

#include <array>
#include <span>

class D3D12RenderDeviceServices final : public RenderDeviceBackendServices
{
public:
	static std::unique_ptr<D3D12RenderDeviceServices> Create(
	    Window& window,
	    PixelFormat backBufferFormat,
	    const RhiPresentationConfiguration& presentationConfiguration,
	    RhiInterposerHooks interposerHooks) noexcept;
	~D3D12RenderDeviceServices() noexcept override;

	D3D12RenderDeviceServices(const D3D12RenderDeviceServices&) = delete;
	D3D12RenderDeviceServices& operator=(const D3D12RenderDeviceServices&) = delete;
	D3D12RenderDeviceServices(D3D12RenderDeviceServices&&) = delete;
	D3D12RenderDeviceServices& operator=(D3D12RenderDeviceServices&&) = delete;

	RenderHardwareInterface& GetRenderHardwareInterface() noexcept override;
	const RenderHardwareInterface& GetRenderHardwareInterface() const noexcept override;
	RhiImGuiRenderer& GetImGuiRenderer() noexcept override;
	void SettleForShutdown() noexcept override;
	void ResizeSwapChain() noexcept override;
	void BeginFrame(std::uint64_t frameId) noexcept override;
	void PrepareCommandRecording() noexcept override;
	RenderCommandList& GetCurrentGraphicsCommandList() noexcept override;
	RenderCommandList& GetGraphicsCommandList(std::uint32_t frameIndex) noexcept override;
	RenderCommandList& BeginCurrentGraphicsCommandList() noexcept override;
	RhiCommandRecordingLease AcquireCommandRecordingLease(ERhiQueueType queueType, RhiCommandRecordingOwner owner) noexcept override;
	RhiCommandRecordingLease TakeCurrentGraphicsCommandRecordingLease() noexcept override;
	RhiSubmissionToken SubmitCommandRecordingLease(
	    RhiCommandRecordingLease&& lease,
	    std::span<const RhiSubmissionToken> waitTokens) noexcept override;
	RhiSubmissionToken SubmitCommandRecordingBatch(
	    std::span<RhiCommandRecordingLease> leases,
	    std::span<const RhiSubmissionToken> waitTokens) noexcept override;
	RhiSubmissionToken SubmitCurrentGraphicsCommandList(std::span<const RhiSubmissionToken> waitTokens) noexcept override;
	void QueueWait(ERhiQueueType waitQueue, RhiSubmissionToken executionToken) noexcept override;
	void WaitForSubmission(RhiSubmissionToken token) noexcept override;
	bool IsSubmissionComplete(RhiSubmissionToken token) const noexcept override;
	RhiSubmissionToken GetLastSubmittedToken(ERhiQueueType queueType) const noexcept override;
	void SubmitFrame(std::uint64_t frameId) noexcept override;
	void AdvanceFrameInFlight() noexcept override;

private:
	D3D12RenderDeviceServices() noexcept = default;

	void Initialize(
	    Window& window,
	    PixelFormat backBufferFormat,
	    const RhiPresentationConfiguration& presentationConfiguration,
	    RhiInterposerHooks interposerHooks);
	void InitializeDevice(RhiInterposerHooks interposerHooks);
	void InitializePresentation(
	    Window& window,
	    PixelFormat backBufferFormat,
	    const RhiPresentationConfiguration& presentationConfiguration);
	void InitializeHardwareInterface();
	void InitializeCommandRecording();
	void InitializeSamplers();
	void DrainPresentationQueue() noexcept;

	std::unique_ptr<D3D12Rhi> m_rhi;
	std::unique_ptr<D3D12DescriptorHeapManager> m_descriptorHeapManager;
	std::unique_ptr<D3D12SwapChain> m_swapChain;
	std::unique_ptr<D3D12UploadService> m_uploadService;
	std::unique_ptr<D3D12RenderHardwareInterface> m_renderHardwareInterface;
	std::unique_ptr<D3D12CommandRecordingContext> m_commandRecordingContext;
	std::unique_ptr<D3D12SamplerLibrary> m_samplerLibrary;
	std::uint32_t m_currentFrameIndex = 0;
};

std::unique_ptr<RenderDeviceBackendServices> CreateD3D12RenderDeviceServices(
    Window& window,
    PixelFormat backBufferFormat,
    const RhiPresentationConfiguration& presentationConfiguration,
    RhiInterposerHooks interposerHooks) noexcept
{
	return D3D12RenderDeviceServices::Create(window, backBufferFormat, presentationConfiguration, interposerHooks);
}

std::unique_ptr<D3D12RenderDeviceServices> D3D12RenderDeviceServices::Create(
    Window& window,
    PixelFormat backBufferFormat,
    const RhiPresentationConfiguration& presentationConfiguration,
    RhiInterposerHooks interposerHooks) noexcept
{
	auto services = std::unique_ptr<D3D12RenderDeviceServices>(new D3D12RenderDeviceServices());
	services->Initialize(window, backBufferFormat, presentationConfiguration, interposerHooks);
	return services;
}

void D3D12RenderDeviceServices::Initialize(
    Window& window,
    PixelFormat backBufferFormat,
    const RhiPresentationConfiguration& presentationConfiguration,
    RhiInterposerHooks interposerHooks)
{
	InitializeDevice(interposerHooks);
	InitializePresentation(window, backBufferFormat, presentationConfiguration);
	InitializeHardwareInterface();
	InitializeCommandRecording();
	InitializeSamplers();
}

void D3D12RenderDeviceServices::InitializeDevice(RhiInterposerHooks interposerHooks)
{
	m_rhi = std::make_unique<D3D12Rhi>(interposerHooks);
	m_descriptorHeapManager = std::make_unique<D3D12DescriptorHeapManager>(*m_rhi);
}

void D3D12RenderDeviceServices::InitializePresentation(
    Window& window,
    PixelFormat backBufferFormat,
    const RhiPresentationConfiguration& presentationConfiguration)
{
	m_swapChain = std::make_unique<D3D12SwapChain>(*m_rhi, window, *m_descriptorHeapManager, backBufferFormat, presentationConfiguration);
}

void D3D12RenderDeviceServices::InitializeHardwareInterface()
{
	m_uploadService = std::make_unique<D3D12UploadService>(*m_rhi, m_rhi->GetMemoryAllocator());
	m_renderHardwareInterface = std::make_unique<D3D12RenderHardwareInterface>(
	    *m_rhi,
	    m_rhi->GetMemoryAllocator(),
	    *m_descriptorHeapManager,
	    *m_swapChain,
	    *m_uploadService);
}

void D3D12RenderDeviceServices::InitializeCommandRecording()
{
	m_commandRecordingContext = std::make_unique<D3D12CommandRecordingContext>(
	    *m_rhi,
	    *m_renderHardwareInterface,
	    *m_descriptorHeapManager,
	    m_swapChain->GetMaximumFramesInFlight());
	m_renderHardwareInterface->SetCommandRecordingContext(*m_commandRecordingContext);
}

void D3D12RenderDeviceServices::InitializeSamplers()
{
	m_samplerLibrary = std::make_unique<D3D12SamplerLibrary>(*m_rhi, m_renderHardwareInterface->GetDescriptorService());
	m_renderHardwareInterface->SetSamplerTableHandle(m_samplerLibrary->GetTableHandle());
}

D3D12RenderDeviceServices::~D3D12RenderDeviceServices() noexcept
{
	m_samplerLibrary.reset();
	m_commandRecordingContext.reset();
	m_renderHardwareInterface.reset();
	m_uploadService.reset();
	m_swapChain.reset();
	m_descriptorHeapManager.reset();

	m_rhi.reset();
}

RenderHardwareInterface& D3D12RenderDeviceServices::GetRenderHardwareInterface() noexcept
{
	return *m_renderHardwareInterface;
}

const RenderHardwareInterface& D3D12RenderDeviceServices::GetRenderHardwareInterface() const noexcept
{
	return *m_renderHardwareInterface;
}

RhiImGuiRenderer& D3D12RenderDeviceServices::GetImGuiRenderer() noexcept
{
	return m_renderHardwareInterface->GetImGuiRenderer();
}

void D3D12RenderDeviceServices::SettleForShutdown() noexcept
{
	m_renderHardwareInterface->WaitForIdle();
	m_rhi->GetMemoryAllocator().ClearRecordingReadView();
}

void D3D12RenderDeviceServices::ResizeSwapChain() noexcept
{
	DrainPresentationQueue();
	m_swapChain->Resize();
}

void D3D12RenderDeviceServices::DrainPresentationQueue() noexcept
{
	// Frame submission is signaled before Present. This boundary also retires presentation and interposer work
	// before ResizeBuffers releases the current back buffers.
	const RhiSubmissionToken presentationToken = m_rhi->SignalQueue(ERhiQueueType::Graphics);
	m_rhi->WaitForSubmission(presentationToken);
}

void D3D12RenderDeviceServices::BeginFrame(std::uint64_t frameId) noexcept
{
	m_swapChain->WaitForPresentationSlot();
	m_rhi->NotifyFrameLatencyMarker(ERhiFrameLatencyMarker::RenderSubmitStart, frameId);
	m_rhi->SetCurrentFrameIndex(m_currentFrameIndex);
	m_commandRecordingContext->BeginFrame(m_currentFrameIndex);
	m_uploadService->BeginFrame();
	(void) BeginCurrentGraphicsCommandList();
}

void D3D12RenderDeviceServices::PrepareCommandRecording() noexcept
{
	m_rhi->GetMemoryAllocator().PublishRecordingReadView();
}

RenderCommandList& D3D12RenderDeviceServices::GetCurrentGraphicsCommandList() noexcept
{
	return m_renderHardwareInterface->GetGraphicsCommandList(m_rhi->GetCurrentFrameIndex());
}

RenderCommandList& D3D12RenderDeviceServices::GetGraphicsCommandList(std::uint32_t frameIndex) noexcept
{
	return m_renderHardwareInterface->GetGraphicsCommandList(frameIndex);
}

RenderCommandList& D3D12RenderDeviceServices::BeginCurrentGraphicsCommandList() noexcept
{
	return m_commandRecordingContext->BeginCurrentGraphicsCommandList(m_rhi->GetCurrentFrameIndex());
}

RhiCommandRecordingLease D3D12RenderDeviceServices::AcquireCommandRecordingLease(
    ERhiQueueType queueType,
    RhiCommandRecordingOwner owner) noexcept
{
	return m_commandRecordingContext->Acquire(queueType, m_rhi->GetCurrentFrameIndex(), owner);
}

RhiCommandRecordingLease D3D12RenderDeviceServices::TakeCurrentGraphicsCommandRecordingLease() noexcept
{
	return m_commandRecordingContext->TakeCurrentGraphicsCommandRecordingLease(m_rhi->GetCurrentFrameIndex());
}

RhiSubmissionToken D3D12RenderDeviceServices::SubmitCommandRecordingLease(
    RhiCommandRecordingLease&& lease,
    std::span<const RhiSubmissionToken> waitTokens) noexcept
{
	return m_commandRecordingContext->Submit(std::move(lease), waitTokens);
}

RhiSubmissionToken D3D12RenderDeviceServices::SubmitCommandRecordingBatch(
    std::span<RhiCommandRecordingLease> leases,
    std::span<const RhiSubmissionToken> waitTokens) noexcept
{
	return m_commandRecordingContext->SubmitBatch(leases, waitTokens);
}

RhiSubmissionToken D3D12RenderDeviceServices::SubmitCurrentGraphicsCommandList(std::span<const RhiSubmissionToken> waitTokens) noexcept
{
	return m_commandRecordingContext->SubmitCurrentGraphicsCommandList(m_rhi->GetCurrentFrameIndex(), waitTokens);
}

void D3D12RenderDeviceServices::QueueWait(ERhiQueueType waitQueue, RhiSubmissionToken executionToken) noexcept
{
	m_rhi->QueueWait(waitQueue, executionToken);
}

void D3D12RenderDeviceServices::WaitForSubmission(RhiSubmissionToken token) noexcept
{
	m_rhi->WaitForSubmission(token);
}

bool D3D12RenderDeviceServices::IsSubmissionComplete(RhiSubmissionToken token) const noexcept
{
	return m_rhi->IsSubmissionComplete(token);
}

RhiSubmissionToken D3D12RenderDeviceServices::GetLastSubmittedToken(ERhiQueueType queueType) const noexcept
{
	return m_rhi->GetLastSubmittedToken(queueType);
}

void D3D12RenderDeviceServices::SubmitFrame(std::uint64_t frameId) noexcept
{
	for (const ERhiQueueType queueType : {ERhiQueueType::Compute, ERhiQueueType::Copy})
	{
		m_rhi->QueueWait(ERhiQueueType::Graphics, m_rhi->GetLastSubmittedToken(queueType));
	}

	(void) SubmitCurrentGraphicsCommandList({});
	m_rhi->NotifyFrameLatencyMarker(ERhiFrameLatencyMarker::RenderSubmitEnd, frameId);
	m_rhi->NotifyFrameLatencyMarker(ERhiFrameLatencyMarker::PresentStart, frameId);
	m_swapChain->Present();
	m_rhi->NotifyFrameLatencyMarker(ERhiFrameLatencyMarker::PresentEnd, frameId);
}

void D3D12RenderDeviceServices::AdvanceFrameInFlight() noexcept
{
	m_swapChain->UpdateCurrentBackBufferIndex();
	m_currentFrameIndex = (m_currentFrameIndex + 1u) % m_swapChain->GetMaximumFramesInFlight();
}

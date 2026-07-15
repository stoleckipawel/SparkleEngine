#include "PCH.h"

#include "Device/RenderDeviceBackendFactory.h"

#include "D3D12/D3D12RenderHardwareInterface.h"
#include "D3D12/Device/D3D12Rhi.h"
#include "D3D12/SwapChain/D3D12SwapChain.h"
#include "D3D12/Descriptors/D3D12DescriptorHeapManager.h"
#include "D3D12/Resources/D3D12FrameResource.h"
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
	    RhiExternalFeatureHooks externalFeatureHooks) noexcept;
	~D3D12RenderDeviceServices() noexcept override;

	D3D12RenderDeviceServices(const D3D12RenderDeviceServices&) = delete;
	D3D12RenderDeviceServices& operator=(const D3D12RenderDeviceServices&) = delete;
	D3D12RenderDeviceServices(D3D12RenderDeviceServices&&) = delete;
	D3D12RenderDeviceServices& operator=(D3D12RenderDeviceServices&&) = delete;

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
	D3D12RenderDeviceServices() noexcept = default;

	std::unique_ptr<D3D12Rhi> m_rhi;
	std::unique_ptr<D3D12DescriptorHeapManager> m_descriptorHeapManager;
	std::unique_ptr<D3D12SwapChain> m_swapChain;
	std::unique_ptr<D3D12FrameResourceManager> m_frameResourceManager;
	std::unique_ptr<D3D12UploadService> m_uploadService;
	std::unique_ptr<D3D12RenderHardwareInterface> m_renderHardwareInterface;
	std::unique_ptr<D3D12SamplerLibrary> m_samplerLibrary;
	std::array<bool, RhiQueueTypeCount> m_queueRecording{};
};

std::unique_ptr<RenderDeviceBackendServices> CreateD3D12RenderDeviceServices(
    Window& window,
    PixelFormat backBufferFormat,
    RhiExternalFeatureHooks externalFeatureHooks) noexcept
{
	return D3D12RenderDeviceServices::Create(window, backBufferFormat, externalFeatureHooks);
}

std::unique_ptr<D3D12RenderDeviceServices> D3D12RenderDeviceServices::Create(
    Window& window,
    PixelFormat backBufferFormat,
    RhiExternalFeatureHooks externalFeatureHooks) noexcept
{
	auto services = std::unique_ptr<D3D12RenderDeviceServices>(new D3D12RenderDeviceServices());

	{
		services->m_rhi = std::make_unique<D3D12Rhi>(externalFeatureHooks);
	}
	{
		services->m_descriptorHeapManager = std::make_unique<D3D12DescriptorHeapManager>(*services->m_rhi);
	}
	{
		services->m_swapChain = std::make_unique<D3D12SwapChain>(
		    *services->m_rhi,
		    window,
		    *services->m_descriptorHeapManager,
		    backBufferFormat);
	}
	{
		services->m_frameResourceManager =
		    std::make_unique<D3D12FrameResourceManager>(*services->m_rhi, D3D12FrameResourceManager::DefaultCapacityPerFrame);
	}
	{
		services->m_uploadService = std::make_unique<D3D12UploadService>(
		    *services->m_rhi,
		    *services->m_frameResourceManager,
		    services->m_rhi->GetMemoryAllocator());
	}
	{
		services->m_renderHardwareInterface = std::make_unique<D3D12RenderHardwareInterface>(
		    *services->m_rhi,
		    services->m_rhi->GetMemoryAllocator(),
		    *services->m_descriptorHeapManager,
		    *services->m_swapChain,
		    *services->m_uploadService);
	}
	{
		services->m_samplerLibrary =
		    std::make_unique<D3D12SamplerLibrary>(*services->m_rhi, services->m_renderHardwareInterface->GetDescriptorService());
	}
	services->m_renderHardwareInterface->SetSamplerTableHandle(services->m_samplerLibrary->GetTableHandle());
	return services;
}

D3D12RenderDeviceServices::~D3D12RenderDeviceServices() noexcept
{
	if (m_rhi != nullptr)
	{
		m_rhi->Flush();
	}

	m_samplerLibrary.reset();
	m_renderHardwareInterface.reset();
	m_uploadService.reset();
	m_frameResourceManager.reset();
	m_swapChain.reset();
	m_descriptorHeapManager.reset();

	if (m_rhi != nullptr && IsDebuggerPresent())
	{
		m_rhi->ReportLiveObjects();
	}

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

void D3D12RenderDeviceServices::Flush() noexcept
{
	m_renderHardwareInterface->WaitForIdle();
}

void D3D12RenderDeviceServices::ResizeSwapChain() noexcept
{
	m_swapChain->Resize();
}

void D3D12RenderDeviceServices::BeginFrame() noexcept
{
	const UINT frameIndex = m_swapChain->GetFrameInFlightIndex();
	m_rhi->SetCurrentFrameIndex(frameIndex);
	m_frameResourceManager->BeginFrame(m_rhi->GetFence().Get(), m_rhi->GetFenceEvent(), frameIndex);
	for (std::size_t queueIndex = 0; queueIndex < RhiQueueTypeCount; ++queueIndex)
	{
		m_rhi->WaitForGPU(static_cast<ERhiQueueType>(queueIndex), frameIndex);
		m_queueRecording[queueIndex] = false;
	}
	m_uploadService->BeginFrame();
	m_renderHardwareInterface->GetResourceService().DrainCompletedResourceReleases();
	(void)BeginCommandList(ERhiQueueType::Graphics);
}

RenderCommandList& D3D12RenderDeviceServices::GetCurrentGraphicsCommandList() noexcept
{
	return m_renderHardwareInterface->GetGraphicsCommandList(m_rhi->GetCurrentFrameIndex());
}

RenderCommandList& D3D12RenderDeviceServices::GetGraphicsCommandList(std::uint32_t frameIndex) noexcept
{
	return m_renderHardwareInterface->GetGraphicsCommandList(frameIndex);
}

RenderCommandList& D3D12RenderDeviceServices::BeginCommandList(ERhiQueueType queueType) noexcept
{
	const std::size_t queueIndex = RhiQueueTypeToIndex(queueType);
	const UINT frameIndex = m_rhi->GetCurrentFrameIndex();
	if (!m_queueRecording[queueIndex])
	{
		m_rhi->ResetCommandAllocator(queueType, frameIndex);
		m_rhi->ResetCommandList(queueType, frameIndex);
		m_queueRecording[queueIndex] = true;
		m_renderHardwareInterface->GetCommandList(queueType, frameIndex).ResetTrackedResources();
	}
	return m_renderHardwareInterface->GetCommandList(queueType, frameIndex);
}

RhiSubmissionToken D3D12RenderDeviceServices::SubmitCommandList(
	RenderCommandList& commandList,
	std::span<const RhiSubmissionToken> waitTokens) noexcept
{
	const ERhiQueueType queueType = commandList.GetQueueType();
	const std::size_t queueIndex = RhiQueueTypeToIndex(queueType);
	if (!m_queueRecording[queueIndex])
	{
		return m_rhi->GetLastSubmittedToken(queueType);
	}

	for (const RhiSubmissionToken waitToken : waitTokens)
	{
		m_rhi->QueueWait(queueType, waitToken);
	}
	const UINT frameIndex = m_rhi->GetCurrentFrameIndex();
	m_rhi->CloseCommandList(queueType, frameIndex);
	m_rhi->ExecuteCommandList(queueType, frameIndex);
	const RhiSubmissionToken submissionToken = m_rhi->Signal(queueType, frameIndex);
	commandList.ResolveTrackedResources(submissionToken);
	m_queueRecording[queueIndex] = false;
	return submissionToken;
}

void D3D12RenderDeviceServices::QueueWait(
	ERhiQueueType waitQueue,
	RhiSubmissionToken executionToken) noexcept
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

void D3D12RenderDeviceServices::SubmitFrame() noexcept
{
	const UINT frameIndex = m_rhi->GetCurrentFrameIndex();
	RenderCommandList& graphicsCommandList = m_renderHardwareInterface->GetGraphicsCommandList(frameIndex);
	const RhiSubmissionToken submissionToken = SubmitCommandList(graphicsCommandList, {});
	m_frameResourceManager->EndFrame(submissionToken.Value);
	m_swapChain->Present();
}

void D3D12RenderDeviceServices::AdvanceFrameInFlight() noexcept
{
	m_swapChain->UpdateFrameInFlightIndex();
}

void D3D12RenderDeviceServices::CloseExecuteAndFlushCurrentFrame() noexcept
{
	if (m_queueRecording[RhiQueueTypeToIndex(ERhiQueueType::Graphics)])
	{
		RenderCommandList& graphicsCommandList = GetCurrentGraphicsCommandList();
		(void)SubmitCommandList(graphicsCommandList, {});
	}
	m_renderHardwareInterface->WaitForIdle();
}

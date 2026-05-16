#include "PCH.h"

#include "Device/RenderDeviceServices.h"

#include "D3D12/D3D12RenderHardwareInterface.h"
#include "D3D12/D3D12Rhi.h"
#include "D3D12/D3D12SwapChain.h"
#include "D3D12/Descriptors/D3D12DescriptorHeapManager.h"
#include "D3D12/Resources/D3D12ConstantBufferManager.h"
#include "D3D12/Resources/D3D12FrameResource.h"
#include "D3D12/Samplers/D3D12SamplerLibrary.h"

#include "Time/Timer.h"
#include "Window/Window.h"

#include "Core/Public/Diagnostics/Logger.h"
#include "Core/Public/Diagnostics/Trace.h"
#include "Core/Public/Diagnostics/Verify.h"
#include <string>

static std::shared_ptr<spdlog::logger> g_rhiServicesLogger = Logging::GetOrCreateLogger("RHI.Services");

class RenderDeviceBackendServices
{
  public:
	virtual ~RenderDeviceBackendServices() noexcept = default;

	virtual RenderHardwareInterface& GetRenderHardwareInterface() noexcept = 0;
	virtual const RenderHardwareInterface& GetRenderHardwareInterface() const noexcept = 0;
	virtual RenderDiagnostics& GetDiagnostics() noexcept = 0;
	virtual const RenderDiagnostics& GetDiagnostics() const noexcept = 0;
	virtual void Flush() noexcept = 0;
	virtual void ResizeSwapChain() noexcept = 0;
	virtual void BeginFrame() noexcept = 0;
	virtual RenderCommandList& GetCurrentGraphicsCommandList() noexcept = 0;
	virtual void SubmitFrame() noexcept = 0;
	virtual void AdvanceFrameInFlight() noexcept = 0;
	virtual void UpdatePerFrameConstants(std::uint32_t renderViewMode) noexcept = 0;
	virtual void CloseExecuteAndFlushCurrentFrame() noexcept = 0;
};

class D3D12RenderDeviceServices final : public RenderDeviceBackendServices
{
  public:
	static std::unique_ptr<D3D12RenderDeviceServices> Create(Timer& timer, Window& window) noexcept;
	~D3D12RenderDeviceServices() noexcept override;

	D3D12RenderDeviceServices(const D3D12RenderDeviceServices&) = delete;
	D3D12RenderDeviceServices& operator=(const D3D12RenderDeviceServices&) = delete;
	D3D12RenderDeviceServices(D3D12RenderDeviceServices&&) = delete;
	D3D12RenderDeviceServices& operator=(D3D12RenderDeviceServices&&) = delete;

	RenderHardwareInterface& GetRenderHardwareInterface() noexcept override;
	const RenderHardwareInterface& GetRenderHardwareInterface() const noexcept override;
	RenderDiagnostics& GetDiagnostics() noexcept override;
	const RenderDiagnostics& GetDiagnostics() const noexcept override;
	void Flush() noexcept override;
	void ResizeSwapChain() noexcept override;
	void BeginFrame() noexcept override;
	RenderCommandList& GetCurrentGraphicsCommandList() noexcept override;
	void SubmitFrame() noexcept override;
	void AdvanceFrameInFlight() noexcept override;
	void UpdatePerFrameConstants(std::uint32_t renderViewMode) noexcept override;
	void CloseExecuteAndFlushCurrentFrame() noexcept override;

  private:
	D3D12RenderDeviceServices() noexcept = default;

	std::unique_ptr<D3D12Rhi> m_rhi;
	std::unique_ptr<D3D12DescriptorHeapManager> m_descriptorHeapManager;
	std::unique_ptr<D3D12SwapChain> m_swapChain;
	std::unique_ptr<D3D12FrameResourceManager> m_frameResourceManager;
	std::unique_ptr<D3D12ConstantBufferManager> m_constantBufferManager;
	std::unique_ptr<D3D12RenderHardwareInterface> m_renderHardwareInterface;
	std::unique_ptr<D3D12SamplerLibrary> m_samplerLibrary;
};

static void FailUnsupportedRhiBackend(ERhiBackendApi api) noexcept
{
	const std::string message = std::string("RHI backend '") + RhiBackendApiToString(api) + "' is not implemented by RenderDeviceServices yet.";
	Diagnostics::Fail(g_rhiServicesLogger, __FILE__, __LINE__, message);
}

struct RenderDeviceServices::Impl
{
	std::unique_ptr<RenderDeviceBackendServices> backend;
};

RenderDeviceServices::RenderDeviceServices() noexcept = default;

RenderDeviceServices::~RenderDeviceServices() noexcept = default;

std::unique_ptr<RenderDeviceServices> RenderDeviceServices::Create(Timer& timer, Window& window) noexcept
{
	return Create(timer, window, ResolveDefaultRhiBackendSelection());
}

std::unique_ptr<RenderDeviceServices> RenderDeviceServices::Create(Timer& timer, Window& window, RhiBackendSelection selection) noexcept
{
	SPARKLE_CPU_SCOPE("RHI.CreateBackend");
	SPDLOG_LOGGER_INFO(g_rhiServicesLogger, "Creating RHI backend: {}", RhiBackendApiToString(selection.Api));

	auto services = std::unique_ptr<RenderDeviceServices>(new RenderDeviceServices());
	services->m_impl = std::make_unique<Impl>();
	switch (selection.Api)
	{
		case ERhiBackendApi::D3D12:
			services->m_impl->backend = D3D12RenderDeviceServices::Create(timer, window);
			break;
		case ERhiBackendApi::Vulkan:
		case ERhiBackendApi::Unknown:
		default:
			FailUnsupportedRhiBackend(selection.Api);
	}
	return services;
}

std::unique_ptr<D3D12RenderDeviceServices> D3D12RenderDeviceServices::Create(Timer& timer, Window& window) noexcept
{
	auto services = std::unique_ptr<D3D12RenderDeviceServices>(new D3D12RenderDeviceServices());

	{
		SPARKLE_CPU_SCOPE("RHI.CreateDevice");
		services->m_rhi = std::make_unique<D3D12Rhi>();
	}
	{
		SPARKLE_CPU_SCOPE("RHI.CreateDescriptorHeaps");
		services->m_descriptorHeapManager = std::make_unique<D3D12DescriptorHeapManager>(*services->m_rhi);
	}
	{
		SPARKLE_CPU_SCOPE("RHI.CreateSwapChain");
		services->m_swapChain = std::make_unique<D3D12SwapChain>(*services->m_rhi, window, *services->m_descriptorHeapManager);
	}
	{
		SPARKLE_CPU_SCOPE("RHI.CreateFrameResources");
		services->m_frameResourceManager =
		    std::make_unique<D3D12FrameResourceManager>(*services->m_rhi, D3D12FrameResourceManager::DefaultCapacityPerFrame);
	}
	{
		SPARKLE_CPU_SCOPE("RHI.CreateConstantBuffers");
		services->m_constantBufferManager = std::make_unique<D3D12ConstantBufferManager>(
		    timer,
		    *services->m_rhi,
		    window,
		    *services->m_descriptorHeapManager,
		    *services->m_frameResourceManager,
		    *services->m_swapChain);
	}
	{
		SPARKLE_CPU_SCOPE("RHI.CreateHardwareInterface");
		services->m_renderHardwareInterface = std::make_unique<D3D12RenderHardwareInterface>(
		    *services->m_rhi,
		    services->m_rhi->GetMemoryAllocator(),
		    *services->m_descriptorHeapManager,
		    *services->m_swapChain,
		    *services->m_constantBufferManager);
	}
	{
		SPARKLE_CPU_SCOPE("RHI.CreateSamplerLibrary");
		services->m_samplerLibrary = std::make_unique<D3D12SamplerLibrary>(*services->m_rhi, *services->m_renderHardwareInterface);
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
	m_constantBufferManager.reset();
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

RenderDiagnostics& D3D12RenderDeviceServices::GetDiagnostics() noexcept
{
	return m_renderHardwareInterface->GetDiagnostics();
}

const RenderDiagnostics& D3D12RenderDeviceServices::GetDiagnostics() const noexcept
{
	return m_renderHardwareInterface->GetDiagnostics();
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
	m_rhi->WaitForGPU(frameIndex);
	m_rhi->ResetCommandAllocator(frameIndex);
	m_rhi->ResetCommandList(frameIndex);
}

RenderCommandList& D3D12RenderDeviceServices::GetCurrentGraphicsCommandList() noexcept
{
	return m_renderHardwareInterface->GetGraphicsCommandList(m_rhi->GetCurrentFrameIndex());
}

void D3D12RenderDeviceServices::SubmitFrame() noexcept
{
	const UINT frameIndex = m_rhi->GetCurrentFrameIndex();
	m_rhi->CloseCommandList(frameIndex);
	m_rhi->ExecuteCommandList(frameIndex);
	m_rhi->Signal(frameIndex);
	m_frameResourceManager->EndFrame(m_rhi->GetNextFenceValue() - 1);
	m_swapChain->Present();
}

void D3D12RenderDeviceServices::AdvanceFrameInFlight() noexcept
{
	m_swapChain->UpdateFrameInFlightIndex();
}

void D3D12RenderDeviceServices::UpdatePerFrameConstants(std::uint32_t renderViewMode) noexcept
{
	m_constantBufferManager->UpdatePerFrame(renderViewMode);
}

void D3D12RenderDeviceServices::CloseExecuteAndFlushCurrentFrame() noexcept
{
	const UINT frameIndex = m_rhi->GetCurrentFrameIndex();
	m_rhi->CloseCommandList(frameIndex);
	m_rhi->ExecuteCommandList(frameIndex);
	m_renderHardwareInterface->WaitForIdle();
}

RenderHardwareInterface& RenderDeviceServices::GetRenderHardwareInterface() noexcept
{
	return m_impl->backend->GetRenderHardwareInterface();
}

const RenderHardwareInterface& RenderDeviceServices::GetRenderHardwareInterface() const noexcept
{
	const RenderDeviceBackendServices& backend = *m_impl->backend;
	return backend.GetRenderHardwareInterface();
}

RenderDiagnostics& RenderDeviceServices::GetDiagnostics() noexcept
{
	return m_impl->backend->GetDiagnostics();
}

const RenderDiagnostics& RenderDeviceServices::GetDiagnostics() const noexcept
{
	const RenderDeviceBackendServices& backend = *m_impl->backend;
	return backend.GetDiagnostics();
}

void RenderDeviceServices::Flush() noexcept
{
	m_impl->backend->Flush();
}

void RenderDeviceServices::ResizeSwapChain() noexcept
{
	m_impl->backend->ResizeSwapChain();
}

void RenderDeviceServices::BeginFrame() noexcept
{
	m_impl->backend->BeginFrame();
}

RenderCommandList& RenderDeviceServices::GetCurrentGraphicsCommandList() noexcept
{
	return m_impl->backend->GetCurrentGraphicsCommandList();
}

void RenderDeviceServices::SubmitFrame() noexcept
{
	m_impl->backend->SubmitFrame();
}

void RenderDeviceServices::AdvanceFrameInFlight() noexcept
{
	m_impl->backend->AdvanceFrameInFlight();
}

void RenderDeviceServices::UpdatePerFrameConstants(std::uint32_t renderViewMode) noexcept
{
	m_impl->backend->UpdatePerFrameConstants(renderViewMode);
}

void RenderDeviceServices::CloseExecuteAndFlushCurrentFrame() noexcept
{
	m_impl->backend->CloseExecuteAndFlushCurrentFrame();
}

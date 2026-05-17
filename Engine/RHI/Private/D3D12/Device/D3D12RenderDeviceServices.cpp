#include "PCH.h"

#include "Device/RenderDeviceBackendFactory.h"

#include "D3D12/D3D12RenderHardwareInterface.h"
#include "D3D12/Device/D3D12Rhi.h"
#include "D3D12/SwapChain/D3D12SwapChain.h"
#include "D3D12/Descriptors/D3D12DescriptorHeapManager.h"
#include "D3D12/Resources/D3D12ConstantBufferManager.h"
#include "D3D12/Resources/D3D12FrameResource.h"
#include "D3D12/Samplers/D3D12SamplerLibrary.h"

#include "Time/Timer.h"
#include "Window/Window.h"

#include "Core/Public/Diagnostics/Trace.h"

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

std::unique_ptr<RenderDeviceBackendServices> CreateD3D12RenderDeviceServices(Timer& timer, Window& window) noexcept
{
	return D3D12RenderDeviceServices::Create(timer, window);
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
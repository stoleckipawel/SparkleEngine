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

#include "Core/Public/Diagnostics/Trace.h"

struct RenderDeviceServices::Impl
{
	std::unique_ptr<D3D12Rhi> rhi;
	std::unique_ptr<D3D12DescriptorHeapManager> descriptorHeapManager;
	std::unique_ptr<D3D12SwapChain> swapChain;
	std::unique_ptr<D3D12FrameResourceManager> frameResourceManager;
	std::unique_ptr<D3D12ConstantBufferManager> constantBufferManager;
	std::unique_ptr<D3D12RenderHardwareInterface> renderHardwareInterface;
	std::unique_ptr<D3D12SamplerLibrary> samplerLibrary;
};

RenderDeviceServices::RenderDeviceServices() noexcept = default;

RenderDeviceServices::~RenderDeviceServices() noexcept
{
	if (m_impl != nullptr)
	{
		if (m_impl->rhi != nullptr)
		{
			m_impl->rhi->Flush();
		}

		m_impl->samplerLibrary.reset();
		m_impl->renderHardwareInterface.reset();
		m_impl->constantBufferManager.reset();
		m_impl->frameResourceManager.reset();
		m_impl->swapChain.reset();
		m_impl->descriptorHeapManager.reset();

		if (m_impl->rhi != nullptr && IsDebuggerPresent())
		{
			m_impl->rhi->ReportLiveObjects();
		}

		m_impl->rhi.reset();
	}
}

std::unique_ptr<RenderDeviceServices> RenderDeviceServices::Create(Timer& timer, Window& window) noexcept
{
	SPARKLE_CPU_SCOPE("RHI.CreateBackend");
	auto services = std::unique_ptr<RenderDeviceServices>(new RenderDeviceServices());
	services->m_impl = std::make_unique<Impl>();

	{
		SPARKLE_CPU_SCOPE("RHI.CreateDevice");
		services->m_impl->rhi = std::make_unique<D3D12Rhi>();
	}
	{
		SPARKLE_CPU_SCOPE("RHI.CreateDescriptorHeaps");
		services->m_impl->descriptorHeapManager = std::make_unique<D3D12DescriptorHeapManager>(*services->m_impl->rhi);
	}
	{
		SPARKLE_CPU_SCOPE("RHI.CreateSwapChain");
		services->m_impl->swapChain =
		    std::make_unique<D3D12SwapChain>(*services->m_impl->rhi, window, *services->m_impl->descriptorHeapManager);
	}
	{
		SPARKLE_CPU_SCOPE("RHI.CreateFrameResources");
		services->m_impl->frameResourceManager =
		    std::make_unique<D3D12FrameResourceManager>(*services->m_impl->rhi, D3D12FrameResourceManager::DefaultCapacityPerFrame);
	}
	{
		SPARKLE_CPU_SCOPE("RHI.CreateConstantBuffers");
		services->m_impl->constantBufferManager = std::make_unique<D3D12ConstantBufferManager>(
		    timer,
		    *services->m_impl->rhi,
		    window,
		    *services->m_impl->descriptorHeapManager,
		    *services->m_impl->frameResourceManager,
		    *services->m_impl->swapChain);
	}
	{
		SPARKLE_CPU_SCOPE("RHI.CreateHardwareInterface");
		services->m_impl->renderHardwareInterface = std::make_unique<D3D12RenderHardwareInterface>(
		    *services->m_impl->rhi,
		    *services->m_impl->descriptorHeapManager,
		    *services->m_impl->swapChain,
		    *services->m_impl->constantBufferManager);
	}
	{
		SPARKLE_CPU_SCOPE("RHI.CreateSamplerLibrary");
		services->m_impl->samplerLibrary =
		    std::make_unique<D3D12SamplerLibrary>(*services->m_impl->rhi, *services->m_impl->renderHardwareInterface);
	}
	services->m_impl->renderHardwareInterface->SetSamplerTableHandle(services->m_impl->samplerLibrary->GetTableHandle());
	return services;
}

RenderHardwareInterface& RenderDeviceServices::GetRenderHardwareInterface() noexcept
{
	return *m_impl->renderHardwareInterface;
}

const RenderHardwareInterface& RenderDeviceServices::GetRenderHardwareInterface() const noexcept
{
	return *m_impl->renderHardwareInterface;
}

RenderDiagnostics& RenderDeviceServices::GetDiagnostics() noexcept
{
	return m_impl->renderHardwareInterface->GetDiagnostics();
}

const RenderDiagnostics& RenderDeviceServices::GetDiagnostics() const noexcept
{
	return m_impl->renderHardwareInterface->GetDiagnostics();
}

void RenderDeviceServices::Flush() noexcept
{
	m_impl->renderHardwareInterface->WaitForIdle();
}

void RenderDeviceServices::ResizeSwapChain() noexcept
{
	m_impl->swapChain->Resize();
}

void RenderDeviceServices::BeginFrame() noexcept
{
	const UINT frameIndex = m_impl->swapChain->GetFrameInFlightIndex();
	m_impl->rhi->SetCurrentFrameIndex(frameIndex);
	m_impl->frameResourceManager->BeginFrame(m_impl->rhi->GetFence().Get(), m_impl->rhi->GetFenceEvent(), frameIndex);
	m_impl->rhi->WaitForGPU(frameIndex);
	m_impl->rhi->ResetCommandAllocator(frameIndex);
	m_impl->rhi->ResetCommandList(frameIndex);
}

RenderCommandList& RenderDeviceServices::GetCurrentGraphicsCommandList() noexcept
{
	return m_impl->renderHardwareInterface->GetGraphicsCommandList(m_impl->rhi->GetCurrentFrameIndex());
}

void RenderDeviceServices::SubmitFrame() noexcept
{
	const UINT frameIndex = m_impl->rhi->GetCurrentFrameIndex();
	m_impl->rhi->CloseCommandList(frameIndex);
	m_impl->rhi->ExecuteCommandList(frameIndex);
	m_impl->rhi->Signal(frameIndex);
	m_impl->frameResourceManager->EndFrame(m_impl->rhi->GetNextFenceValue() - 1);
	m_impl->swapChain->Present();
}

void RenderDeviceServices::AdvanceFrameInFlight() noexcept
{
	m_impl->swapChain->UpdateFrameInFlightIndex();
}

void RenderDeviceServices::UpdatePerFrameConstants(std::uint32_t renderViewMode) noexcept
{
	m_impl->constantBufferManager->UpdatePerFrame(renderViewMode);
}

void RenderDeviceServices::CloseExecuteAndFlushCurrentFrame() noexcept
{
	const UINT frameIndex = m_impl->rhi->GetCurrentFrameIndex();
	m_impl->rhi->CloseCommandList(frameIndex);
	m_impl->rhi->ExecuteCommandList(frameIndex);
	m_impl->renderHardwareInterface->WaitForIdle();
}

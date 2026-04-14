#include "PCH.h"

#include "Interop/RendererBackendServices.h"
#include "Interop/Internal/RendererBackendServicesAccess.h"

#include "D3D12/D3D12RenderHardwareInterface.h"
#include "D3D12/D3D12Rhi.h"
#include "D3D12/D3D12SwapChain.h"
#include "D3D12/Descriptors/D3D12DescriptorHeapManager.h"
#include "D3D12/Resources/D3D12ConstantBufferManager.h"
#include "D3D12/Resources/D3D12FrameResource.h"
#include "D3D12/Samplers/D3D12SamplerLibrary.h"

#include "Time/Timer.h"
#include "Window/Window.h"

struct RendererBackendServices::Impl
{
	std::unique_ptr<D3D12Rhi> rhi;
	std::unique_ptr<D3D12DescriptorHeapManager> descriptorHeapManager;
	std::unique_ptr<D3D12SwapChain> swapChain;
	std::unique_ptr<D3D12FrameResourceManager> frameResourceManager;
	std::unique_ptr<D3D12ConstantBufferManager> constantBufferManager;
	std::unique_ptr<D3D12SamplerLibrary> samplerLibrary;
	std::unique_ptr<D3D12RenderHardwareInterface> renderHardwareInterface;
};

RendererBackendServices::RendererBackendServices() noexcept = default;

RendererBackendServices::~RendererBackendServices() noexcept = default;

std::unique_ptr<RendererBackendServices> RendererBackendServices::Create(Timer& timer, Window& window) noexcept
{
	auto services = std::unique_ptr<RendererBackendServices>(new RendererBackendServices());
	services->m_impl = std::make_unique<Impl>();
	services->m_impl->rhi = std::make_unique<D3D12Rhi>();
	services->m_impl->descriptorHeapManager = std::make_unique<D3D12DescriptorHeapManager>(*services->m_impl->rhi);
	services->m_impl->swapChain =
	    std::make_unique<D3D12SwapChain>(*services->m_impl->rhi, window, *services->m_impl->descriptorHeapManager);
	services->m_impl->frameResourceManager =
	    std::make_unique<D3D12FrameResourceManager>(*services->m_impl->rhi, D3D12FrameResourceManager::DefaultCapacityPerFrame);
	services->m_impl->constantBufferManager = std::make_unique<D3D12ConstantBufferManager>(
	    timer,
	    *services->m_impl->rhi,
	    window,
	    *services->m_impl->descriptorHeapManager,
	    *services->m_impl->frameResourceManager,
	    *services->m_impl->swapChain);
	services->m_impl->samplerLibrary =
	    std::make_unique<D3D12SamplerLibrary>(*services->m_impl->rhi, *services->m_impl->descriptorHeapManager);
	services->m_impl->renderHardwareInterface = std::make_unique<D3D12RenderHardwareInterface>(
	    *services->m_impl->rhi,
	    *services->m_impl->descriptorHeapManager,
	    *services->m_impl->swapChain);
	return services;
}

RenderHardwareInterface& RendererBackendServices::GetRenderHardwareInterface() noexcept
{
	return *m_impl->renderHardwareInterface;
}

const RenderHardwareInterface& RendererBackendServices::GetRenderHardwareInterface() const noexcept
{
	return *m_impl->renderHardwareInterface;
}

void RendererBackendServices::Flush() noexcept
{
	m_impl->rhi->Flush();
}

void RendererBackendServices::ResizeSwapChain() noexcept
{
	m_impl->swapChain->Resize();
}

void RendererBackendServices::BeginFrame() noexcept
{
	const UINT frameIndex = m_impl->swapChain->GetFrameInFlightIndex();
	m_impl->rhi->SetCurrentFrameIndex(frameIndex);
	m_impl->frameResourceManager->BeginFrame(m_impl->rhi->GetFence().Get(), m_impl->rhi->GetFenceEvent(), frameIndex);
	m_impl->rhi->WaitForGPU(frameIndex);
	m_impl->rhi->ResetCommandAllocator(frameIndex);
	m_impl->rhi->ResetCommandList(frameIndex);
}

NativeGraphicsCommandListHandle RendererBackendServices::GetCurrentGraphicsCommandListHandle() const noexcept
{
	return m_impl->renderHardwareInterface->GetGraphicsCommandListHandle(m_impl->rhi->GetCurrentFrameIndex());
}

void RendererBackendServices::SubmitFrame() noexcept
{
	const UINT frameIndex = m_impl->rhi->GetCurrentFrameIndex();
	m_impl->rhi->CloseCommandList(frameIndex);
	m_impl->rhi->ExecuteCommandList(frameIndex);
	m_impl->rhi->Signal(frameIndex);
	m_impl->frameResourceManager->EndFrame(m_impl->rhi->GetNextFenceValue() - 1);
	m_impl->swapChain->Present();
}

void RendererBackendServices::AdvanceFrameInFlight() noexcept
{
	m_impl->swapChain->UpdateFrameInFlightIndex();
}

void RendererBackendServices::UpdatePerFrameConstants(std::uint32_t renderViewMode) noexcept
{
	m_impl->constantBufferManager->UpdatePerFrame(renderViewMode);
}

void RendererBackendServices::CloseExecuteAndFlushCurrentFrame() noexcept
{
	const UINT frameIndex = m_impl->rhi->GetCurrentFrameIndex();
	m_impl->rhi->CloseCommandList(frameIndex);
	m_impl->rhi->ExecuteCommandList(frameIndex);
	m_impl->rhi->Flush();
}

namespace Rhi::Internal
{
	D3D12Rhi& RendererBackendServicesAccess::GetRhi(RendererBackendServices& services) noexcept
	{
		return *services.m_impl->rhi;
	}

	D3D12DescriptorHeapManager& RendererBackendServicesAccess::GetDescriptorHeapManager(RendererBackendServices& services) noexcept
	{
		return *services.m_impl->descriptorHeapManager;
	}

	D3D12SwapChain& RendererBackendServicesAccess::GetSwapChain(RendererBackendServices& services) noexcept
	{
		return *services.m_impl->swapChain;
	}

	D3D12ConstantBufferManager& RendererBackendServicesAccess::GetConstantBufferManager(RendererBackendServices& services) noexcept
	{
		return *services.m_impl->constantBufferManager;
	}

	D3D12SamplerLibrary& RendererBackendServicesAccess::GetSamplerLibrary(RendererBackendServices& services) noexcept
	{
		return *services.m_impl->samplerLibrary;
	}
}
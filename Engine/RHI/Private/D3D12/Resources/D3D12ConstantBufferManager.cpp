#include "PCH.h"
#include "D3D12ConstantBufferManager.h"
#include "D3D12FrameResource.h"
#include "Timer.h"
#include "Window.h"
#include "UI.h"
#include "D3D12SwapChain.h"
#include <cmath>

D3D12ConstantBufferManager::D3D12ConstantBufferManager(
    Timer& timer,
    D3D12Rhi& rhi,
    Window& window,
    D3D12DescriptorHeapManager& descriptorHeapManager,
    D3D12FrameResourceManager& frameResourceManager,
    D3D12SwapChain& swapChain,
    UI& ui) :
    m_timer(&timer), m_window(&window), m_frameResourceManager(&frameResourceManager), m_swapChain(&swapChain), m_ui(&ui)
{
	for (uint32_t i = 0; i < RHISettings::FramesInFlight; ++i)
	{
		m_perFrameCB[i] = std::make_unique<D3D12ConstantBuffer<PerFrameConstantBufferData>>(rhi, descriptorHeapManager);
		m_perViewCB[i] = std::make_unique<D3D12ConstantBuffer<PerViewConstantBufferData>>(rhi, descriptorHeapManager);
	}
}

D3D12ConstantBufferManager::~D3D12ConstantBufferManager() noexcept
{
	for (uint32_t i = 0; i < RHISettings::FramesInFlight; ++i)
	{
		m_perFrameCB[i].reset();
		m_perViewCB[i].reset();
	}
}

D3D12_GPU_VIRTUAL_ADDRESS D3D12ConstantBufferManager::GetPerFrameGpuAddress() const
{
	return m_perFrameCB[m_swapChain->GetFrameInFlightIndex()]->GetGPUVirtualAddress();
}

D3D12_GPU_VIRTUAL_ADDRESS D3D12ConstantBufferManager::GetPerViewGpuAddress() const
{
	return m_perViewCB[m_swapChain->GetFrameInFlightIndex()]->GetGPUVirtualAddress();
}

void D3D12ConstantBufferManager::UpdatePerFrame()
{
	PerFrameConstantBufferData data = {};
	data.FrameIndex = m_timer->GetFrameCount();
	data.TotalTime = static_cast<float>(m_timer->GetTotalTime(TimeDomain::Unscaled, TimeUnit::Seconds));
	data.DeltaTime = static_cast<float>(m_timer->GetDelta(TimeDomain::Unscaled, TimeUnit::Seconds));
	data.ScaledTotalTime = static_cast<float>(m_timer->GetTotalTime(TimeDomain::Scaled, TimeUnit::Seconds));
	data.ScaledDeltaTime = static_cast<float>(m_timer->GetDelta(TimeDomain::Scaled, TimeUnit::Seconds));
	const float width = static_cast<float>(m_window->GetWidth());
	const float height = static_cast<float>(m_window->GetHeight());
	data.ViewportSize = DirectX::XMFLOAT2(width, height);
	data.ViewportSizeInv = DirectX::XMFLOAT2(width != 0.0f ? 1.0f / width : 0.0f, height != 0.0f ? 1.0f / height : 0.0f);
	data.ViewModeIndex = static_cast<uint32_t>(m_ui->GetViewMode());

	const uint32_t frameInFlightIndex = m_swapChain->GetFrameInFlightIndex();
	m_perFrameCB[frameInFlightIndex]->Update(data);
}

void D3D12ConstantBufferManager::UpdatePerView(const PerViewConstantBufferData& data)
{
	const uint32_t frameInFlightIndex = m_swapChain->GetFrameInFlightIndex();
	m_perViewCB[frameInFlightIndex]->Update(data);
}

D3D12_GPU_VIRTUAL_ADDRESS D3D12ConstantBufferManager::UpdatePerObjectVS(const PerObjectVSConstantBufferData& data)
{
	return m_frameResourceManager->AllocateConstantBuffer(data);
}

D3D12_GPU_VIRTUAL_ADDRESS D3D12ConstantBufferManager::UpdatePerObjectPS(const PerObjectPSConstantBufferData& data)
{
	return m_frameResourceManager->AllocateConstantBuffer(data);
}

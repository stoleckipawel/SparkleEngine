#include "PCH.h"
#include "D3D12/Resources/D3D12ConstantBufferManager.h"
#include "D3D12/Resources/D3D12FrameResource.h"
#include "Timer.h"
#include "Window/Window.h"
#include "D3D12/D3D12SwapChain.h"

D3D12ConstantBufferManager::D3D12ConstantBufferManager(
    Timer& timer,
    D3D12Rhi& rhi,
    Window& window,
    D3D12DescriptorHeapManager& descriptorHeapManager,
    D3D12FrameResourceManager& frameResourceManager,
    D3D12SwapChain& swapChain) :
    m_timer(&timer), m_window(&window), m_frameResourceManager(&frameResourceManager), m_swapChain(&swapChain)
{
	for (uint32_t i = 0; i < RenderConfig::FramesInFlight; ++i)
	{
		m_perFrameCB[i] = std::make_unique<D3D12ConstantBuffer<PerFrameConstantBufferData>>(rhi, descriptorHeapManager);
	}
}

D3D12ConstantBufferManager::~D3D12ConstantBufferManager() noexcept
{
	for (uint32_t i = 0; i < RenderConfig::FramesInFlight; ++i)
	{
		m_perFrameCB[i].reset();
	}
}

D3D12_GPU_VIRTUAL_ADDRESS D3D12ConstantBufferManager::GetPerFrameGpuAddress() const
{
	return m_perFrameCB[m_swapChain->GetFrameInFlightIndex()]->GetGPUVirtualAddress();
}

const PerFrameConstantBufferData& D3D12ConstantBufferManager::GetPerFrameData() const noexcept
{
	return m_perFrameData[m_swapChain->GetFrameInFlightIndex()];
}

void D3D12ConstantBufferManager::UpdatePerFrame(std::uint32_t viewModeIndex)
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
	data.ViewModeIndex = viewModeIndex;

	const uint32_t frameInFlightIndex = m_swapChain->GetFrameInFlightIndex();
	m_perFrameData[frameInFlightIndex] = data;
	m_perFrameCB[frameInFlightIndex]->Update(data);
}
D3D12_GPU_VIRTUAL_ADDRESS D3D12ConstantBufferManager::AllocatePerView(const PerViewConstantBufferData& data)
{
	return m_frameResourceManager->AllocateConstantBuffer(data);
}

D3D12_GPU_VIRTUAL_ADDRESS D3D12ConstantBufferManager::UpdatePerObjectVS(const PerObjectVSConstantBufferData& data)
{
	return m_frameResourceManager->AllocateConstantBuffer(data);
}

D3D12_GPU_VIRTUAL_ADDRESS D3D12ConstantBufferManager::UpdatePerObjectPS(const PerObjectPSConstantBufferData& data)
{
	return m_frameResourceManager->AllocateConstantBuffer(data);
}

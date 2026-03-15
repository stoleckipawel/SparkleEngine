#include "PCH.h"
#include "D3D12SwapChain.h"
#include "Window.h"
#include "D3D12Rhi.h"
#include "D3D12DescriptorHeapManager.h"

D3D12SwapChain::D3D12SwapChain(D3D12Rhi& rhi, Window& window, D3D12DescriptorHeapManager& descriptorHeapManager) :
    m_rhi(rhi), m_window(&window), m_descriptorHeapManager(&descriptorHeapManager)
{
	AllocateHandles();
	Create();

	m_swapChain->SetMaximumFrameLatency(RHISettings::FramesInFlight);
	m_waitableObject = m_swapChain->GetFrameLatencyWaitableObject();

	UpdateFrameInFlightIndex();

	CreateRenderTargetViews();
}

D3D12SwapChain::~D3D12SwapChain() noexcept
{
	ReleaseBuffers();
	m_swapChain.Reset();
}

void D3D12SwapChain::Create()
{
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.Width = m_window->GetWidth();
	swapChainDesc.Height = m_window->GetHeight();
	swapChainDesc.Format = RHISettings::BackBufferFormat;
	swapChainDesc.Stereo = false;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = RHISettings::FramesInFlight;
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
	swapChainDesc.Flags = ComputeSwapChainFlags();

	DXGI_SWAP_CHAIN_FULLSCREEN_DESC swapChainFullsceenDesc{};
	swapChainFullsceenDesc.Windowed = true;

	ComPtr<IDXGISwapChain1> swapChain;
	CHECK(m_rhi.GetDxgiFactory()->CreateSwapChainForHwnd(
	    m_rhi.GetCommandQueue().Get(),
	    m_window->GetHWND(),
	    &swapChainDesc,
	    &swapChainFullsceenDesc,
	    nullptr,
	    &swapChain));

	CHECK(swapChain.As(&m_swapChain));
}

void D3D12SwapChain::Resize()
{
	ReleaseBuffers();

	m_swapChain->ResizeBuffers(
	    RHISettings::FramesInFlight,
	    m_window->GetWidth(),
	    m_window->GetHeight(),
	    RHISettings::BackBufferFormat,
	    ComputeSwapChainFlags());

	CreateRenderTargetViews();

	UpdateFrameInFlightIndex();
}
void D3D12SwapChain::AllocateHandles()
{
	for (UINT i = 0; i < RHISettings::FramesInFlight; i++)
	{
		m_rtvHandles[i] = m_descriptorHeapManager->AllocateHandle(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}
}
void D3D12SwapChain::CreateRenderTargetViews()
{
	for (UINT i = 0; i < RHISettings::FramesInFlight; i++)
	{
		CHECK(m_swapChain->GetBuffer(i, IID_PPV_ARGS(m_buffers[i].ReleaseAndGetAddressOf())));
		m_buffers[i]->SetName(L"RHI_BackBuffer");

		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
		rtvDesc.Format = RHISettings::BackBufferFormat;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice = 0;
		rtvDesc.Texture2D.PlaneSlice = 0;

		m_rhi.GetDevice()->CreateRenderTargetView(m_buffers[i].Get(), &rtvDesc, GetCPUHandle(i));
	}
}

UINT D3D12SwapChain::GetAllowTearingFlag() const
{
	BOOL allowTearing = FALSE;

	m_rhi.GetDxgiFactory()->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));

	return (allowTearing == TRUE) ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0u;
}

UINT D3D12SwapChain::ComputeSwapChainFlags() const
{
	UINT flags = 0u;
	flags |= GetFrameLatencyWaitableFlag();
	flags |= GetAllowTearingFlag();
	return flags;
}
D3D12_VIEWPORT D3D12SwapChain::GetDefaultViewport() const
{
	D3D12_VIEWPORT vp;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	vp.Width = float(m_window->GetWidth());
	vp.Height = float(m_window->GetHeight());
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	return vp;
}
D3D12_RECT D3D12SwapChain::GetDefaultScissorRect() const
{
	D3D12_RECT scissorRect;
	scissorRect.left = 0;
	scissorRect.top = 0;
	scissorRect.right = m_window->GetWidth();
	scissorRect.bottom = m_window->GetHeight();
	return scissorRect;
}
void D3D12SwapChain::Present()
{
	UINT presentInterval = RHISettings::VSync ? 1u : 0u;
	UINT presentFlags = 0u;
	if (!RHISettings::VSync)
	{
		BOOL allowTearing = FALSE;
		m_rhi.GetDxgiFactory()->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
		presentFlags = (allowTearing == TRUE) ? DXGI_PRESENT_ALLOW_TEARING : 0u;
	}
	CHECK(m_swapChain->Present(presentInterval, presentFlags));
}

void D3D12SwapChain::ReleaseBuffers()
{
	for (UINT i = 0; i < RHISettings::FramesInFlight; i++)
	{
		m_buffers[i].Reset();
		if (m_rtvHandles[i].IsValid())
		{
			m_descriptorHeapManager->FreeHandle(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, m_rtvHandles[i]);
		}
	}
}
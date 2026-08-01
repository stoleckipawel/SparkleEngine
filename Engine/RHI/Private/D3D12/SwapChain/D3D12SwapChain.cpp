#include "PCH.h"
#include "D3D12/SwapChain/D3D12SwapChain.h"
#include "CVars/RHICVars.h"
#include "Window/Window.h"
#include "D3D12/Device/D3D12Rhi.h"
#include "D3D12/D3D12TypeConversions.h"
#include "D3D12/Descriptors/D3D12DescriptorHeapManager.h"
#include "Presentation/RhiPresentationDefaults.h"

#include <format>

static const auto g_d3d12SwapChainLogger = Logging::GetOrCreateLogger("RHI.D3D12.SwapChain");

D3D12SwapChain::D3D12SwapChain(
    D3D12Rhi& rhi,
    Window& window,
    D3D12DescriptorHeapManager& descriptorHeapManager,
    PixelFormat backBufferFormat,
    const RhiPresentationConfiguration& presentationConfiguration) :
    m_rhi(rhi),
    m_backBufferCount(presentationConfiguration.BackBufferCount),
    m_maximumFramesInFlight(presentationConfiguration.MaximumFramesInFlight),
    m_backBufferFormat(backBufferFormat),
    m_window(&window),
    m_descriptorHeapManager(&descriptorHeapManager)
{
	m_buffers.resize(m_backBufferCount);
	m_rtvHandles.resize(m_backBufferCount);
	AllocateHandles();
	Create();

	CHECK(GetPresentationInterface()->SetMaximumFrameLatency(m_maximumFramesInFlight));
	m_waitableObject = GetPresentationInterface()->GetFrameLatencyWaitableObject();
	if (m_waitableObject == nullptr)
	{
		Diagnostics::Fatal(g_d3d12SwapChainLogger, __FILE__, __LINE__, "D3D12 swap chain did not expose its frame-latency waitable object.");
	}

	UpdateCurrentBackBufferIndex();

	CreateRenderTargetViews();
}

D3D12SwapChain::~D3D12SwapChain() noexcept
{
	ReleaseBuffers();
	ReleaseRenderTargetHandles();
	if (m_waitableObject != nullptr)
	{
		CloseHandle(m_waitableObject);
		m_waitableObject = nullptr;
	}
	m_externalSwapChain.Reset();
	m_swapChain.Reset();
}

UINT D3D12SwapChain::GetWindowWidth() const noexcept
{
	return m_window->GetWidth();
}

UINT D3D12SwapChain::GetWindowHeight() const noexcept
{
	return m_window->GetHeight();
}

bool D3D12SwapChain::HasValidWindowSize() const noexcept
{
	return m_window->HasValidSize();
}

void D3D12SwapChain::Create()
{
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.Width = GetWindowWidth();
	swapChainDesc.Height = GetWindowHeight();
	swapChainDesc.Format = D3D12TypeConversions::ToDxgiFormat(m_backBufferFormat);
	swapChainDesc.Stereo = false;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = m_backBufferCount;
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
	swapChainDesc.Flags = ComputeSwapChainFlags();

	DXGI_SWAP_CHAIN_FULLSCREEN_DESC swapChainFullsceenDesc{};
	swapChainFullsceenDesc.Windowed = true;

	ComPtr<IDXGIFactory7> externalFactory;
	if (m_rhi.TryUpgradeInterposerInterface(
	        ERhiInterposerInterfaceKind::PresentationFactory,
	        m_rhi.GetDxgiFactory().Get(),
	        IID_PPV_ARGS(externalFactory.ReleaseAndGetAddressOf())))
	{
		ComPtr<IDXGISwapChain1> externalSwapChain;
		const HRESULT createResult = externalFactory->CreateSwapChainForHwnd(
		    m_rhi.GetPresentationCommandQueue(),
		    m_window->GetHWND(),
		    &swapChainDesc,
		    &swapChainFullsceenDesc,
		    nullptr,
		    externalSwapChain.ReleaseAndGetAddressOf());
		if (SUCCEEDED(createResult) &&
		    SUCCEEDED(externalSwapChain.As(&m_externalSwapChain)) &&
		    m_rhi.TryResolveNativeInterface(
		        ERhiInterposerInterfaceKind::PresentationSurface,
		        m_externalSwapChain.Get(),
		        IID_PPV_ARGS(m_swapChain.ReleaseAndGetAddressOf())))
		{
			m_rhi.NotifyInterposerPresentationReady(true);
			return;
		}

		m_externalSwapChain.Reset();
	}

	ComPtr<IDXGISwapChain1> swapChain;
	CHECK(m_rhi.GetDxgiFactory()->CreateSwapChainForHwnd(
	    m_rhi.GetCommandQueue().Get(),
	    m_window->GetHWND(),
	    &swapChainDesc,
	    &swapChainFullsceenDesc,
	    nullptr,
	    swapChain.ReleaseAndGetAddressOf()));
	CHECK(swapChain.As(&m_swapChain));
	m_rhi.NotifyInterposerPresentationReady(false);
}

void D3D12SwapChain::Resize()
{
	if (!HasValidWindowSize())
	{
		return;
	}

	ReleaseBuffers();
	ResizeBuffersToWindow();
	CreateRenderTargetViews();

	UpdateCurrentBackBufferIndex();
}

void D3D12SwapChain::ResizeBuffersToWindow()
{
	CHECK(GetPresentationInterface()->ResizeBuffers(
	    m_backBufferCount,
	    GetWindowWidth(),
	    GetWindowHeight(),
	    D3D12TypeConversions::ToDxgiFormat(m_backBufferFormat),
	    ComputeSwapChainFlags()));
}

void D3D12SwapChain::AllocateHandles()
{
	for (UINT i = 0; i < m_backBufferCount; i++)
	{
		m_rtvHandles[i] = m_descriptorHeapManager->GetAllocator(D3D12_DESCRIPTOR_HEAP_TYPE_RTV)->Allocate();
	}
}
void D3D12SwapChain::CreateRenderTargetViews()
{
	for (UINT i = 0; i < m_backBufferCount; i++)
	{
		CHECK(GetPresentationInterface()->GetBuffer(i, IID_PPV_ARGS(m_buffers[i].ReleaseAndGetAddressOf())));
		m_buffers[i]->SetName(L"RHI_BackBuffer");

		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
		rtvDesc.Format = D3D12TypeConversions::ToDxgiFormat(m_backBufferFormat);
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

UINT D3D12SwapChain::GetFrameLatencyWaitableFlag() const
{
	return DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
}

UINT D3D12SwapChain::ComputeSwapChainFlags() const
{
	UINT flags = 0u;
	flags |= GetFrameLatencyWaitableFlag();
	flags |= GetAllowTearingFlag();
	return flags;
}

RhiViewport D3D12SwapChain::GetDefaultViewport() const
{
	return RhiViewport{
	    .X = 0.0f,
	    .Y = 0.0f,
	    .Width = static_cast<float>(GetWindowWidth()),
	    .Height = static_cast<float>(GetWindowHeight()),
	    .MinDepth = 0.0f,
	    .MaxDepth = 1.0f};
}

RhiRect D3D12SwapChain::GetDefaultScissorRect() const
{
	return RhiRect{
	    .Left = 0,
	    .Top = 0,
	    .Right = static_cast<std::int32_t>(GetWindowWidth()),
	    .Bottom = static_cast<std::int32_t>(GetWindowHeight())};
}

void D3D12SwapChain::Present()
{
	const bool bVSync = CVarVSync.Get();
	UINT presentInterval = bVSync ? 1u : 0u;
	UINT presentFlags = 0u;
	if (!bVSync)
	{
		BOOL allowTearing = FALSE;
		m_rhi.GetDxgiFactory()->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
		presentFlags = (allowTearing == TRUE) ? DXGI_PRESENT_ALLOW_TEARING : 0u;
	}
	CHECK(GetPresentationInterface()->Present(presentInterval, presentFlags));
}

void D3D12SwapChain::WaitForPresentationSlot() const noexcept
{
	const DWORD waitResult = WaitForSingleObject(m_waitableObject, INFINITE);
	if (waitResult != WAIT_OBJECT_0)
	{
		Diagnostics::Fatal(
		    g_d3d12SwapChainLogger,
		    __FILE__,
		    __LINE__,
		    std::format("D3D12 frame-latency wait failed with result 0x{:08X}.", waitResult));
	}
}

IDXGISwapChain3* D3D12SwapChain::GetPresentationInterface() const noexcept
{
	return m_externalSwapChain != nullptr ? m_externalSwapChain.Get() : m_swapChain.Get();
}

void D3D12SwapChain::ReleaseBuffers()
{
	for (UINT i = 0; i < m_backBufferCount; i++)
	{
		m_buffers[i].Reset();
	}
}

void D3D12SwapChain::ReleaseRenderTargetHandles() noexcept
{
	for (UINT i = 0; i < m_backBufferCount; i++)
	{
		if (m_rtvHandles[i].IsValid())
		{
			m_descriptorHeapManager->GetAllocator(D3D12_DESCRIPTOR_HEAP_TYPE_RTV)->Free(m_rtvHandles[i]);
		}
	}
}

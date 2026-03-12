#include "PCH.h"
#include "D3D12DebugLayer.h"

#if ENGINE_GPU_VALIDATION

D3D12DebugLayer::D3D12DebugLayer()
{
	InitD3D12Debug();
	InitDXGIDebug();
}

D3D12DebugLayer::~D3D12DebugLayer() noexcept
{
	ReportLiveDXGIObjects();
	m_dxgiDebug.Reset();
	m_d3d12Debug.Reset();
}

void D3D12DebugLayer::InitializeInfoQueue(ID3D12Device* device)
{
	if (!device)
		return;

	ConfigureInfoQueue(device);
	ApplyInfoQueueFilters(device);
}

void D3D12DebugLayer::InitD3D12Debug()
{
	CHECK(D3D12GetDebugInterface(IID_PPV_ARGS(m_d3d12Debug.ReleaseAndGetAddressOf())));
	m_d3d12Debug->EnableDebugLayer();
}

void D3D12DebugLayer::InitDXGIDebug()
{
	CHECK(DXGIGetDebugInterface1(0, IID_PPV_ARGS(m_dxgiDebug.ReleaseAndGetAddressOf())));
	m_dxgiDebug->EnableLeakTrackingForThread();
}

void D3D12DebugLayer::ConfigureInfoQueue(ID3D12Device* device)
{
	ComPtr<ID3D12InfoQueue> infoQueue;
	if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(infoQueue.ReleaseAndGetAddressOf()))))
	{
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
	}
}

void D3D12DebugLayer::ApplyInfoQueueFilters(ID3D12Device* device)
{
	ComPtr<ID3D12InfoQueue> infoQueue;
	if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(infoQueue.ReleaseAndGetAddressOf()))))
	{
		D3D12_MESSAGE_ID disabledMessages[] = {
		    static_cast<D3D12_MESSAGE_ID>(1424)
		};
		D3D12_INFO_QUEUE_FILTER filter = {};
		filter.DenyList.NumIDs = static_cast<UINT>(std::size(disabledMessages));
		filter.DenyList.pIDList = disabledMessages;
		infoQueue->AddStorageFilterEntries(&filter);
	}
}

void D3D12DebugLayer::ReportLiveDeviceObjects(ID3D12Device* device)
{
	#if ENGINE_REPORT_LIVE_OBJECTS
	ComPtr<ID3D12DebugDevice> debugDevice;
	if (device && SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(debugDevice.ReleaseAndGetAddressOf()))))
	{
		OutputDebugStringW(L"D3D12 Live Device Objects (detail + summary):\n");
		debugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL | D3D12_RLDO_SUMMARY);
	}
	#endif
}

void D3D12DebugLayer::ReportLiveDXGIObjects()
{
	#if ENGINE_REPORT_LIVE_OBJECTS
	if (m_dxgiDebug)
	{
		OutputDebugStringW(L"DXGI Live Objects (all flags):\n");
		m_dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_ALL));
	}
	#endif
}
#endif

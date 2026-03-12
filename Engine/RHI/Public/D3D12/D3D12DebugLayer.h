// ============================================================================
// D3D12DebugLayer.h
// ----------------------------------------------------------------------------
// Manages D3D12 and DXGI debug/validation layers.
//
#pragma once

#ifdef ENGINE_GPU_VALIDATION
	#include <wrl/client.h>
	#include <d3d12.h>
	#include <dxgidebug.h>

using Microsoft::WRL::ComPtr;

class D3D12DebugLayer final
{
  public:
	// Constructs and enables debug layers. Call before device creation.
	D3D12DebugLayer();

	// Shuts down debug layers and reports live objects.
	~D3D12DebugLayer() noexcept;

	D3D12DebugLayer(const D3D12DebugLayer&) = delete;
	D3D12DebugLayer& operator=(const D3D12DebugLayer&) = delete;
	D3D12DebugLayer(D3D12DebugLayer&&) = delete;
	D3D12DebugLayer& operator=(D3D12DebugLayer&&) = delete;

	// After device creation, initialize InfoQueue filters for the created device.
	void InitializeInfoQueue(ID3D12Device* device);

	// Reports live D3D12 device objects (call before device Reset).
	void ReportLiveDeviceObjects(ID3D12Device* device);

	// Reports DXGI live objects (factory, adapters, swapchains).
	void ReportLiveDXGIObjects();

  private:
	void InitD3D12Debug();
	void InitDXGIDebug();
	void ConfigureInfoQueue(ID3D12Device* device);
	void ApplyInfoQueueFilters(ID3D12Device* device);

	ComPtr<ID3D12Debug> m_d3d12Debug;  // D3D12 debug interface
	ComPtr<IDXGIDebug1> m_dxgiDebug;   // DXGI debug interface
};

#endif

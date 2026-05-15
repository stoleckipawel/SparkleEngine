#pragma once

#if ENGINE_GPU_VALIDATION
	#include "Device/RenderHardwareInterface.h"

	#include <deque>
	#include <wrl/client.h>
	#include <d3d12.h>
	#include <dxgidebug.h>

using Microsoft::WRL::ComPtr;

class D3D12DebugLayer final
{
  public:
	D3D12DebugLayer();

	~D3D12DebugLayer() noexcept;

	D3D12DebugLayer(const D3D12DebugLayer&) = delete;
	D3D12DebugLayer& operator=(const D3D12DebugLayer&) = delete;
	D3D12DebugLayer(D3D12DebugLayer&&) = delete;
	D3D12DebugLayer& operator=(D3D12DebugLayer&&) = delete;

	void InitializeInfoQueue(ID3D12Device* device);
	bool SupportsDebugMessages() const noexcept;
	bool TryPopMessage(RhiDiagnosticMessage& outMessage) noexcept;
	void ClearMessages() noexcept;
	bool SupportsLiveObjectReports() const noexcept;
	bool SupportsCrashDiagnostics() const noexcept;
	void ReportLiveObjects(ID3D12Device* device);
	void CollectCrashDiagnostics(ID3D12Device* device) noexcept;

	void ReportLiveDeviceObjects(ID3D12Device* device);

	void ReportLiveDXGIObjects();

  private:
	void DrainStoredMessages() noexcept;
	void InitDredSettings() noexcept;
	void InitD3D12Debug();
	void InitDXGIDebug();
	void ConfigureInfoQueue(ID3D12Device* device);
	void ApplyInfoQueueFilters(ID3D12Device* device);

	ComPtr<ID3D12Debug> m_d3d12Debug;
	ComPtr<IDXGIDebug1> m_dxgiDebug;
	ComPtr<ID3D12InfoQueue> m_infoQueue;
	std::deque<RhiDiagnosticMessage> m_messages;
	bool m_supportsCrashDiagnostics = false;
};

#endif

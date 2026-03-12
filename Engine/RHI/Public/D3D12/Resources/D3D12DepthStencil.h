// ============================================================================
// D3D12DepthStencil.h
// ----------------------------------------------------------------------------
// Manages a GPU depth-stencil resource and its DSV descriptor.
//
#pragma once

#include "D3D12DescriptorHandle.h"
#include <d3d12.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class Window;
class D3D12DescriptorHeapManager;
class D3D12Rhi;

class D3D12DepthStencil
{
  public:
	D3D12DepthStencil(D3D12Rhi& rhi, Window& window, D3D12DescriptorHeapManager& descriptorHeapManager);

	~D3D12DepthStencil() noexcept;

	D3D12DepthStencil(const D3D12DepthStencil&) = delete;
	D3D12DepthStencil& operator=(const D3D12DepthStencil&) = delete;
	D3D12DepthStencil(D3D12DepthStencil&&) = delete;
	D3D12DepthStencil& operator=(D3D12DepthStencil&&) = delete;

	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle() const noexcept { return m_dsvHandle.GetGPU(); }
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle() const noexcept { return m_dsvHandle.GetCPU(); }

	void Clear() noexcept;
	void SetWriteState() noexcept;
	void SetReadState() noexcept;

	const ComPtr<ID3D12Resource2>& GetResource() const noexcept { return m_resource; }

  private:
	void CreateResource();
	void CreateDepthStencilView();

  private:
	D3D12Rhi& m_rhi;
	ComPtr<ID3D12Resource2> m_resource;
	D3D12_DEPTH_STENCIL_VIEW_DESC m_depthStencilDesc = {};
	D3D12DescriptorHandle m_dsvHandle;
	Window* m_window = nullptr;
	D3D12DescriptorHeapManager* m_descriptorHeapManager = nullptr;
};

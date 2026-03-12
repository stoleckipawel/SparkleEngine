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
	// Constructs and initializes the depth stencil resource and view
	D3D12DepthStencil(D3D12Rhi& rhi, Window& window, D3D12DescriptorHeapManager& descriptorHeapManager);

	// Destructor releases resources and descriptor handle
	~D3D12DepthStencil() noexcept;

	// Deleted copy/move to enforce unique ownership
	D3D12DepthStencil(const D3D12DepthStencil&) = delete;
	D3D12DepthStencil& operator=(const D3D12DepthStencil&) = delete;
	D3D12DepthStencil(D3D12DepthStencil&&) = delete;
	D3D12DepthStencil& operator=(D3D12DepthStencil&&) = delete;

	// Returns the GPU descriptor handle for shader binding (non-owning)
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle() const noexcept { return m_dsvHandle.GetGPU(); }
	// Returns the CPU descriptor handle for descriptor heap management (non-owning)
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle() const noexcept { return m_dsvHandle.GetCPU(); }

	// Clears the depth stencil view
	void Clear() noexcept;
	// Transition depth buffer to writable state before rendering
	void SetWriteState() noexcept;
	// Transition depth buffer to readable state after rendering
	void SetReadState() noexcept;

	// Internal helper: returns underlying resource. Returns const reference to avoid copies.
	const ComPtr<ID3D12Resource2>& GetResource() const noexcept { return m_resource; }

  private:
	// Creates the depth stencil resource on the GPU
	void CreateResource();
	// Creates the depth stencil view in the descriptor heap
	void CreateDepthStencilView();

  private:
	D3D12Rhi& m_rhi;                                                // RHI reference
	ComPtr<ID3D12Resource2> m_resource;                             // Depth stencil resource
	D3D12_DEPTH_STENCIL_VIEW_DESC m_depthStencilDesc = {};          // DSV description
	D3D12DescriptorHandle m_dsvHandle;                              // Allocated DSV descriptor handle
	Window* m_window = nullptr;                                     // Window reference for dimensions
	D3D12DescriptorHeapManager* m_descriptorHeapManager = nullptr;  // Descriptor heap manager reference
};

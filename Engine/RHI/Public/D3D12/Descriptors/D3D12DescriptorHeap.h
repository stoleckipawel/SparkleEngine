// ============================================================================
// D3D12DescriptorHeap.h
// ----------------------------------------------------------------------------
// RAII wrapper for a single D3D12 descriptor heap.
//
#pragma once

#include "D3D12Rhi.h"
#include "D3D12DescriptorHandle.h"

using Microsoft::WRL::ComPtr;

class D3D12DescriptorHeap
{
  public:
	// ========================================================================
	// Lifecycle
	// ========================================================================

	/// Name is set on the COM object.
	explicit D3D12DescriptorHeap(D3D12Rhi& rhi, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, LPCWSTR name);

	D3D12DescriptorHeap(const D3D12DescriptorHeap&) = delete;
	D3D12DescriptorHeap& operator=(const D3D12DescriptorHeap&) = delete;
	D3D12DescriptorHeap(D3D12DescriptorHeap&&) = delete;
	D3D12DescriptorHeap& operator=(D3D12DescriptorHeap&&) = delete;

	~D3D12DescriptorHeap() noexcept;

	// ========================================================================
	// Accessors
	// ========================================================================

	/// Depends on heap type and tier policy.
	/// - Sampler: D3D12_MAX_SHADER_VISIBLE_SAMPLER_HEAP_SIZE
	/// - Shader-visible CBV/SRV/UAV: Tier 2 max size
	/// - Non-visible (RTV/DSV): same as shader-visible (configurable)
	UINT GetNumDescriptors() const;

	ID3D12DescriptorHeap* GetRaw() const noexcept { return m_heap.Get(); }

	/// Performs bounds check and returns invalid handle on error.
	D3D12DescriptorHandle GetHandleAt(UINT index) const;

  private:
	// ------------------------------------------------------------------------
	// State
	// ------------------------------------------------------------------------

	D3D12Rhi* m_rhi = nullptr;               ///< Reference to RHI for device access
	D3D12_DESCRIPTOR_HEAP_DESC m_desc = {};  ///< Heap description
	ComPtr<ID3D12DescriptorHeap> m_heap;     ///< Underlying D3D12 heap
};

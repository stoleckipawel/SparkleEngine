#pragma once

#include "D3D12Rhi.h"
#include "D3D12DescriptorHandle.h"

using Microsoft::WRL::ComPtr;

class D3D12DescriptorHeap
{
  public:
	explicit D3D12DescriptorHeap(D3D12Rhi& rhi, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, LPCWSTR name);

	D3D12DescriptorHeap(const D3D12DescriptorHeap&) = delete;
	D3D12DescriptorHeap& operator=(const D3D12DescriptorHeap&) = delete;
	D3D12DescriptorHeap(D3D12DescriptorHeap&&) = delete;
	D3D12DescriptorHeap& operator=(D3D12DescriptorHeap&&) = delete;

	~D3D12DescriptorHeap() noexcept;

	UINT GetNumDescriptors() const;

	ID3D12DescriptorHeap* GetRaw() const noexcept { return m_heap.Get(); }

	D3D12DescriptorHandle GetHandleAt(UINT index) const;

  private:
	D3D12Rhi* m_rhi = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC m_desc = {};
	ComPtr<ID3D12DescriptorHeap> m_heap;
};

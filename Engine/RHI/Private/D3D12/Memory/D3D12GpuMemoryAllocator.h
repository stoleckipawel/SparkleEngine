#pragma once

#include <memory>

struct ID3D12Device;
struct IDXGIAdapter;

class D3D12GpuMemoryAllocator final
{
  public:
	explicit D3D12GpuMemoryAllocator(IDXGIAdapter* adapter, ID3D12Device* device) noexcept;
	~D3D12GpuMemoryAllocator() noexcept;

	D3D12GpuMemoryAllocator(const D3D12GpuMemoryAllocator&) = delete;
	D3D12GpuMemoryAllocator& operator=(const D3D12GpuMemoryAllocator&) = delete;
	D3D12GpuMemoryAllocator(D3D12GpuMemoryAllocator&&) = delete;
	D3D12GpuMemoryAllocator& operator=(D3D12GpuMemoryAllocator&&) = delete;

	bool IsInitialized() const noexcept;

  private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

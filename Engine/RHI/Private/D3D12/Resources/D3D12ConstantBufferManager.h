#pragma once
#include <cstdint>

#include "Resources/RhiUploadService.h"

class D3D12FrameResourceManager;

class D3D12ConstantBufferManager final : public RhiUploadService
{
  public:
	explicit D3D12ConstantBufferManager(D3D12FrameResourceManager& frameResourceManager) noexcept;
	~D3D12ConstantBufferManager() noexcept;

	D3D12ConstantBufferManager(const D3D12ConstantBufferManager&) = delete;
	D3D12ConstantBufferManager& operator=(const D3D12ConstantBufferManager&) = delete;
	D3D12ConstantBufferManager(D3D12ConstantBufferManager&&) = delete;
	D3D12ConstantBufferManager& operator=(D3D12ConstantBufferManager&&) = delete;

	RhiGpuVirtualAddress AllocateUniform(const void* data, std::uint32_t sizeInBytes);
	RhiGpuVirtualAddress AllocateUniformConstantBuffer(const void* data, std::uint32_t sizeInBytes) override
	{
		return AllocateUniform(data, sizeInBytes);
	}

  private:
	D3D12FrameResourceManager* m_frameResourceManager = nullptr;
};

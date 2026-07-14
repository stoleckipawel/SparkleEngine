#pragma once

#include "Resources/RhiUploadService.h"

#include <cstdint>
#include <memory>
#include <vector>

class D3D12FrameResourceManager;
class D3D12GpuMemoryAllocator;
class D3D12Rhi;
struct D3D12GpuAllocationRecord;

class D3D12UploadService final : public RhiUploadService
{
  public:
	D3D12UploadService(
	    D3D12Rhi& rhi,
	    D3D12FrameResourceManager& frameResourceManager,
	    D3D12GpuMemoryAllocator& memoryAllocator) noexcept;
	~D3D12UploadService() noexcept;

	D3D12UploadService(const D3D12UploadService&) = delete;
	D3D12UploadService& operator=(const D3D12UploadService&) = delete;
	D3D12UploadService(D3D12UploadService&&) = delete;
	D3D12UploadService& operator=(D3D12UploadService&&) = delete;

	void BeginFrame() noexcept;
	RhiGpuVirtualAddress AllocateUniformConstantBuffer(const void* data, std::uint32_t sizeInBytes) override;
	bool UploadTexture(
	    RenderCommandList& commandList,
	    RhiOwnedResourceHandle destination,
	    const RhiTextureUploadDesc& textureUpload,
	    ResourceState finalState,
	    std::wstring_view debugName) override;

  private:
	struct PendingTextureUpload final
	{
		std::unique_ptr<D3D12GpuAllocationRecord> StagingResource;
		std::uint64_t RetireFenceValue = 0;
	};

	void DrainCompletedTextureUploads() noexcept;

	D3D12Rhi* m_rhi = nullptr;
	D3D12FrameResourceManager* m_frameResourceManager = nullptr;
	D3D12GpuMemoryAllocator* m_memoryAllocator = nullptr;
	std::vector<PendingTextureUpload> m_pendingTextureUploads;
};

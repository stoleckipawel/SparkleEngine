#pragma once

#include "Resources/RhiUploadService.h"

#include <memory>
#include <span>
#include <vector>

class VulkanGpuMemoryAllocator;
class VulkanRenderCommandList;
struct VulkanGpuAllocationRecord;

class VulkanUploadService final : public RhiUploadService
{
public:
	explicit VulkanUploadService(VulkanGpuMemoryAllocator& memoryAllocator);
	~VulkanUploadService() noexcept;

	VulkanUploadService(const VulkanUploadService&) = delete;
	VulkanUploadService& operator=(const VulkanUploadService&) = delete;
	VulkanUploadService(VulkanUploadService&&) = delete;
	VulkanUploadService& operator=(VulkanUploadService&&) = delete;

	RhiGpuVirtualAddress AllocateUniformConstantBuffer(
	    RenderCommandList& commandList,
	    const void* data,
	    std::uint32_t sizeInBytes) override;
	bool UploadBuffer(
	    RenderCommandList& commandList,
	    RhiOwnedResourceHandle destination,
	    std::span<const std::byte> data,
	    ResourceState finalState,
	    std::wstring_view debugName) override;
	bool UploadTexture(
	    RenderCommandList& commandList,
	    RhiOwnedResourceHandle destination,
	    const RhiTextureUploadDesc& textureUpload,
	    ResourceState finalState,
	    std::wstring_view debugName) override;

private:
	static constexpr VkDeviceSize TextureUploadAlignment = 4;

	static VkDeviceSize AlignTextureUploadOffset(VkDeviceSize offset) noexcept;
	static std::uint64_t CalculateTextureUploadBytes(const RhiTextureUploadDesc& textureUpload) noexcept;
	static bool CopyTextureUploadData(
	    const RhiTextureUploadDesc& textureUpload,
	    std::span<std::uint8_t> destination,
	    std::vector<VkBufferImageCopy>& regions) noexcept;
	bool ValidateBufferUploadRequest(
	    const RenderCommandList& commandList,
	    const VulkanGpuAllocationRecord* destination,
	    std::span<const std::byte> data) const noexcept;
	std::unique_ptr<VulkanGpuAllocationRecord> CreateBufferStagingResource(std::span<const std::byte> data, std::wstring_view debugName);
	static void RecordBufferUpload(
	    VulkanRenderCommandList& commandList,
	    const VulkanGpuAllocationRecord& destination,
	    const VulkanGpuAllocationRecord& stagingResource,
	    std::uint64_t sizeInBytes,
	    ResourceState finalState) noexcept;
	bool ValidateTextureUploadRequest(
	    const RenderCommandList& commandList,
	    const VulkanGpuAllocationRecord* destination,
	    const RhiTextureUploadDesc& textureUpload) const noexcept;
	std::unique_ptr<VulkanGpuAllocationRecord> CreateTextureStagingResource(
	    const RhiTextureUploadDesc& textureUpload,
	    std::wstring_view debugName,
	    std::vector<VkBufferImageCopy>& copyRegions);
	static void RecordTextureUpload(
	    VulkanRenderCommandList& commandList,
	    const VulkanGpuAllocationRecord& destination,
	    const VulkanGpuAllocationRecord& stagingResource,
	    const RhiTextureUploadDesc& textureUpload,
	    std::span<const VkBufferImageCopy> copyRegions,
	    ResourceState finalState) noexcept;

	VulkanGpuMemoryAllocator* m_memoryAllocator = nullptr;
};

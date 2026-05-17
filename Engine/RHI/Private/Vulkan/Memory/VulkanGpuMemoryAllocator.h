#pragma once

#include "Memory/RhiMemoryDiagnostics.h"
#include "Memory/RhiMemoryTypes.h"
#include "Vulkan/Memory/VulkanGpuAllocation.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string_view>

class VulkanRhi;

struct VmaAllocator_T;

class VulkanGpuMemoryAllocator final
{
  public:
	explicit VulkanGpuMemoryAllocator(VulkanRhi& rhi) noexcept;
	~VulkanGpuMemoryAllocator() noexcept;

	VulkanGpuMemoryAllocator(const VulkanGpuMemoryAllocator&) = delete;
	VulkanGpuMemoryAllocator& operator=(const VulkanGpuMemoryAllocator&) = delete;
	VulkanGpuMemoryAllocator(VulkanGpuMemoryAllocator&&) = delete;
	VulkanGpuMemoryAllocator& operator=(VulkanGpuMemoryAllocator&&) = delete;

	bool IsInitialized() const noexcept;
	bool SupportsBudgetQueries() const noexcept;
	bool SupportsJsonDump() const noexcept;
	RhiMemoryUsageSnapshot CreateMemoryUsageSnapshot() const;
	bool WriteAllocatorJsonDump(const std::filesystem::path& outputPath, bool includeDetailedMap = true) const noexcept;

	std::unique_ptr<VulkanGpuAllocationRecord> CreateBuffer(
	    const VkBufferCreateInfo& bufferCreateInfo,
	    RhiMemoryCategory category,
	    RhiMemoryResidencyClass residencyClass,
	    std::wstring_view debugName) noexcept;
	std::unique_ptr<VulkanGpuAllocationRecord> CreateImage(
	    const VkImageCreateInfo& imageCreateInfo,
	    RhiMemoryCategory category,
	    RhiMemoryResidencyClass residencyClass,
	    std::wstring_view debugName) noexcept;
	bool WriteAllocation(VulkanGpuAllocationRecord& record, const void* data, std::size_t sizeInBytes) noexcept;
	VulkanGpuAllocationRecord* FindAllocationRecord(NativeResourceHandle resource) const noexcept;

	void QueueDestroyResource(std::unique_ptr<VulkanGpuAllocationRecord> record, std::uint64_t retireFenceValue) noexcept;
	void DrainCompletedReleases(std::uint64_t completedFenceValue) noexcept;
	void FlushPendingReleases() noexcept;

  private:
	struct Impl;
	struct PendingAllocationRelease;

	void DestroyAllocation(VulkanGpuAllocationRecord& record) noexcept;
	void UnmapAllocation(VulkanGpuAllocationRecord& record) noexcept;
	void SetAllocationDebugName(VulkanGpuAllocationRecord& record, std::wstring_view debugName) noexcept;
	void RegisterAllocationRecord(VulkanGpuAllocationRecord& record) noexcept;
	void UnregisterAllocationRecord(VulkanGpuAllocationRecord& record) noexcept;

	std::unique_ptr<VulkanGpuAllocationRecord> CreateAllocationRecord(
	    VulkanGpuAllocationResourceKind resourceKind,
	    VkBuffer buffer,
	    VkImage image,
	    VmaAllocation_T* allocation,
	    RhiMemoryCategory category,
	    RhiMemoryResidencyClass residencyClass,
	    std::wstring_view debugName) noexcept;
	static std::uint32_t ResolveMemoryHeapIndex(const VkPhysicalDeviceMemoryProperties& memoryProperties, std::uint32_t memoryTypeIndex) noexcept;
	static VkImageAspectFlags ResolveImageAspectMask(VkFormat format) noexcept;

	friend struct VulkanGpuAllocationRecord;

	VulkanRhi& m_rhi;
	std::unique_ptr<Impl> m_impl;
};
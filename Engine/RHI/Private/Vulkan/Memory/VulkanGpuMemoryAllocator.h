#pragma once

#include "Core/Public/Threading/ThreadOwnership.h"
#include "Memory/RhiMemoryDiagnostics.h"
#include "Memory/RhiMemoryTypes.h"
#include "Resources/RhiResourceDesc.h"
#include "Vulkan/Memory/VulkanGpuAllocation.h"

#include <cstddef>
#include <array>
#include <cstdint>
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
	RhiMemoryUsageSnapshot CreateMemoryUsageSnapshot() const;

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
	std::unique_ptr<VulkanGpuMemoryBlockRecord> CreateTransientMemoryBlock(
	    RhiTransientAllocationPool pool,
	    std::uint64_t sizeInBytes,
	    std::uint64_t alignment,
	    std::wstring_view debugName) noexcept;
	std::unique_ptr<VulkanGpuAllocationRecord> CreateAliasingImage(
	    VulkanGpuMemoryBlockRecord& memoryBlock,
	    std::uint64_t memoryBlockOffset,
	    const VkImageCreateInfo& imageCreateInfo,
	    std::wstring_view debugName) noexcept;
	std::unique_ptr<VulkanGpuAllocationRecord> CreateAliasingBuffer(
	    VulkanGpuMemoryBlockRecord& memoryBlock,
	    std::uint64_t memoryBlockOffset,
	    const VkBufferCreateInfo& bufferCreateInfo,
	    std::wstring_view debugName) noexcept;
	bool WriteAllocation(
	    VulkanGpuAllocationRecord& record,
	    const void* data,
	    std::size_t sizeInBytes,
	    std::size_t destinationOffsetInBytes = 0) noexcept;
	VulkanGpuAllocationRecord* FindAllocationRecord(RhiResourceHandle resource) const noexcept;
	VulkanGpuAllocationRecord* FindAllocationRecordByDeviceAddress(VkDeviceAddress deviceAddress) const noexcept;

	void QueueDestroyResource(std::unique_ptr<VulkanGpuAllocationRecord> record) noexcept;
	void QueueDestroyMemoryBlock(std::unique_ptr<VulkanGpuMemoryBlockRecord> record) noexcept;
	void DrainCompletedReleases(const std::array<std::uint64_t, RhiQueueTypeCount>& completedValues) noexcept;
	void FlushPendingReleases() noexcept;

  private:
	struct Impl;
	struct PendingAllocationRelease;
	struct PendingMemoryBlockRelease;

	void DestroyAllocation(VulkanGpuAllocationRecord& record) noexcept;
	void DestroyMemoryBlock(VulkanGpuMemoryBlockRecord& record) noexcept;
	void UnmapAllocation(VulkanGpuAllocationRecord& record) noexcept;
	void SetAllocationDebugName(VulkanGpuAllocationRecord& record, std::wstring_view debugName) noexcept;
	void SetMemoryBlockDebugName(VulkanGpuMemoryBlockRecord& record, std::wstring_view debugName) noexcept;
	void RegisterAllocationRecord(VulkanGpuAllocationRecord& record) noexcept;
	void UnregisterAllocationRecord(VulkanGpuAllocationRecord& record) noexcept;
	void RegisterMemoryBlockRecord(VulkanGpuMemoryBlockRecord& record) noexcept;
	void UnregisterMemoryBlockRecord(VulkanGpuMemoryBlockRecord& record) noexcept;
	bool EnsureMemoryBlockAllocationForImage(VulkanGpuMemoryBlockRecord& memoryBlock, const VkImageCreateInfo& imageCreateInfo) noexcept;
	bool EnsureMemoryBlockAllocationForBuffer(VulkanGpuMemoryBlockRecord& memoryBlock, const VkBufferCreateInfo& bufferCreateInfo) noexcept;

	std::unique_ptr<VulkanGpuAllocationRecord> CreateAllocationRecord(
	    VulkanGpuAllocationResourceKind resourceKind,
	    VkBuffer buffer,
	    VkImage image,
	    VmaAllocation_T* allocation,
	    RhiMemoryCategory category,
	    RhiMemoryResidencyClass residencyClass,
	    std::wstring_view debugName) noexcept;
	std::unique_ptr<VulkanGpuAllocationRecord> CreateAliasingAllocationRecord(
	    VulkanGpuAllocationResourceKind resourceKind,
	    VkBuffer buffer,
	    VkImage image,
	    VulkanGpuMemoryBlockRecord& memoryBlock,
	    std::wstring_view debugName) noexcept;
	static std::uint32_t ResolveMemoryHeapIndex(const VkPhysicalDeviceMemoryProperties& memoryProperties, std::uint32_t memoryTypeIndex) noexcept;
	static VkImageAspectFlags ResolveImageAspectMask(VkFormat format) noexcept;

	friend struct VulkanGpuAllocationRecord;
	friend struct VulkanGpuMemoryBlockRecord;
	friend void SetVulkanAllocationRecordDebugName(VulkanGpuAllocationRecord& record, std::wstring_view debugName) noexcept;
	friend void SetVulkanMemoryBlockRecordDebugName(VulkanGpuMemoryBlockRecord& record, std::wstring_view debugName) noexcept;

	VulkanRhi& m_rhi;
	Threading::OwnerThread m_owner{"Vulkan GPU memory allocator"};
	std::unique_ptr<Impl> m_impl;
};

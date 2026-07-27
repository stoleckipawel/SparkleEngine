#pragma once

#include "Vulkan/Memory/VulkanRecordingResource.h"

#include <atomic>
#include <memory>
#include <span>

struct VulkanGpuAllocationRecord;

class VulkanRecordingResourceTable final
{
  public:
	VulkanRecordingResourceTable() noexcept;
	~VulkanRecordingResourceTable() noexcept;

	VulkanRecordingResourceTable(const VulkanRecordingResourceTable&) = delete;
	VulkanRecordingResourceTable& operator=(const VulkanRecordingResourceTable&) = delete;
	VulkanRecordingResourceTable(VulkanRecordingResourceTable&&) = delete;
	VulkanRecordingResourceTable& operator=(VulkanRecordingResourceTable&&) = delete;

  private:
	friend class VulkanGpuMemoryAllocator;

	void Publish(std::span<VulkanGpuAllocationRecord* const> records) noexcept;
	bool Resolve(
	    RhiResourceHandle resource,
	    VulkanRecordingResource& outResource) const noexcept;
	bool Resolve(
	    RhiGpuVirtualAddress address,
	    VulkanRecordingResource& outResource) const noexcept;
	VulkanRecordingResourceUseToken Retain(RhiResourceHandle resource) const noexcept;
	VulkanRecordingResourceUseToken Retain(VulkanGpuAllocationRecord& record) const noexcept;
	void Release(
	    VulkanRecordingResourceUseToken use,
	    RhiSubmissionToken submissionToken) const noexcept;

	struct ResourceEntry;
	struct AddressEntry;
	struct ReadView;

	static std::shared_ptr<ReadView> BuildReadView(
	    std::span<VulkanGpuAllocationRecord* const> records);
	static void CollectPublishedResources(
	    std::span<VulkanGpuAllocationRecord* const> records,
	    ReadView& readView);
	static void BuildAddressProjections(ReadView& readView);
	static const ResourceEntry* FindResource(
	    const ReadView& readView,
	    RhiResourceHandle resource) noexcept;
	static const ResourceEntry* FindExactAddress(
	    const ReadView& readView,
	    RhiGpuVirtualAddress address) noexcept;
	static const ResourceEntry* FindBufferAddress(
	    const ReadView& readView,
	    RhiGpuVirtualAddress address) noexcept;
	static bool AddressEntryPrecedes(
	    const AddressEntry& left,
	    const AddressEntry& right) noexcept;
	static VulkanRecordingResource BuildResource(
	    const VulkanGpuAllocationRecord& record) noexcept;
	static void RetainReference(VulkanGpuAllocationRecord& record) noexcept;
	static void ReleaseReference(VulkanGpuAllocationRecord& record) noexcept;
	static void ReleaseReference(
	    VulkanGpuAllocationRecord& record,
	    RhiSubmissionToken submissionToken) noexcept;

	std::atomic<std::shared_ptr<const ReadView>> m_readView;
};

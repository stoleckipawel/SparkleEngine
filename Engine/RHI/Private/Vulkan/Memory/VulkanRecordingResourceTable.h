#pragma once

#include "Vulkan/Memory/VulkanRecordingResource.h"

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
	void Resolve(
	    const VulkanGpuAllocationRecord& record,
	    VulkanRecordingResource& outResource) const noexcept;
	VulkanRecordingResourceUseToken Retain(RhiResourceHandle resource) const noexcept;
	VulkanRecordingResourceUseToken Retain(VulkanGpuAllocationRecord& record) const noexcept;
	void Release(
	    VulkanRecordingResourceUseToken use,
	    RhiSubmissionToken submissionToken) const noexcept;

	struct ResourceEntry;
	struct ReadView;

	static const ResourceEntry* FindResource(
	    const ReadView& readView,
	    RhiResourceHandle resource) noexcept;
	static VulkanRecordingResource BuildResource(
	    const VulkanGpuAllocationRecord& record) noexcept;
	static void RetainReference(VulkanGpuAllocationRecord& record) noexcept;
	static void ReleaseReference(VulkanGpuAllocationRecord& record) noexcept;
	static void ReleaseReference(
	    VulkanGpuAllocationRecord& record,
	    RhiSubmissionToken submissionToken) noexcept;

	std::shared_ptr<const ReadView> m_readView;
};

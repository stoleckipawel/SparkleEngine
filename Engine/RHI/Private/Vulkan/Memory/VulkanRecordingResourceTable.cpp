#include "Vulkan/VulkanPCH.h"

#include "Vulkan/Memory/VulkanRecordingResourceTable.h"

#include "Vulkan/Memory/VulkanGpuAllocation.h"

#include <algorithm>
#include <cassert>
#include <iterator>
#include <vector>

struct VulkanRecordingResourceTable::ResourceEntry final
{
	VulkanRecordingResource Resource;
	VulkanGpuAllocationRecord* Record = nullptr;
	VkDeviceAddress LookupAddress = 0;
	bool CoversAddressRange = false;
};

struct VulkanRecordingResourceTable::ReadView final
{
	~ReadView() noexcept;

	std::vector<ResourceEntry> ResourcesByHandle;
	std::vector<ResourceEntry> ResourcesByAddress;
};

VulkanRecordingResourceTable::ReadView::~ReadView() noexcept
{
	for (const ResourceEntry& entry : ResourcesByHandle)
	{
		if (entry.Record != nullptr)
		{
			VulkanRecordingResourceTable::ReleaseReference(*entry.Record);
		}
	}
}

VulkanRecordingResourceTable::VulkanRecordingResourceTable() noexcept = default;
VulkanRecordingResourceTable::~VulkanRecordingResourceTable() noexcept = default;

void VulkanRecordingResourceTable::Publish(
    std::span<VulkanGpuAllocationRecord* const> records) noexcept
{
	auto readView = std::make_shared<ReadView>();
	readView->ResourcesByHandle.reserve(records.size());
	readView->ResourcesByAddress.reserve(records.size() * 2);

	for (VulkanGpuAllocationRecord* record : records)
	{
		if (record == nullptr || record->PendingRelease)
		{
			continue;
		}

		const ResourceEntry entry{
		    .Resource = BuildResource(*record),
		    .Record = record};

		RetainReference(*record);
		readView->ResourcesByHandle.push_back(entry);

		if (entry.Resource.BufferDeviceAddress != 0)
		{
			ResourceEntry addressEntry = entry;
			addressEntry.LookupAddress = entry.Resource.BufferDeviceAddress;
			addressEntry.CoversAddressRange = true;
			readView->ResourcesByAddress.push_back(addressEntry);
		}
		if (entry.Resource.DeviceAddress != 0)
		{
			ResourceEntry addressEntry = entry;
			addressEntry.LookupAddress = entry.Resource.DeviceAddress;
			readView->ResourcesByAddress.push_back(addressEntry);
		}
	}

	std::ranges::sort(
	    readView->ResourcesByHandle,
	    {},
	    [](const ResourceEntry& entry) noexcept
	    {
		    return entry.Resource.ResourceHandleValue;
	    });
	std::ranges::sort(
	    readView->ResourcesByAddress,
	    {},
	    &ResourceEntry::LookupAddress);

	std::atomic_store(
	    &m_readView,
	    std::shared_ptr<const ReadView>(std::move(readView)));
}

bool VulkanRecordingResourceTable::Resolve(
    RhiResourceHandle resource,
    VulkanRecordingResource& outResource) const noexcept
{
	const std::shared_ptr<const ReadView> readView = std::atomic_load(&m_readView);
	if (readView == nullptr)
	{
		return false;
	}

	const ResourceEntry* const entry = FindResource(*readView, resource);
	if (entry == nullptr)
	{
		return false;
	}

	outResource = entry->Resource;
	return true;
}

bool VulkanRecordingResourceTable::Resolve(
    RhiGpuVirtualAddress address,
    VulkanRecordingResource& outResource) const noexcept
{
	if (address == 0)
	{
		return false;
	}
	if (Resolve(RhiResourceHandle{.Value = reinterpret_cast<void*>(address)}, outResource))
	{
		return true;
	}

	const std::shared_ptr<const ReadView> readView = std::atomic_load(&m_readView);
	if (readView == nullptr || readView->ResourcesByAddress.empty())
	{
		return false;
	}

	const auto after = std::ranges::upper_bound(
	    readView->ResourcesByAddress,
	    address,
	    {},
	    &ResourceEntry::LookupAddress);
	if (after == readView->ResourcesByAddress.begin())
	{
		return false;
	}

	const ResourceEntry& candidate = *std::prev(after);
	const bool addressMatches =
	    candidate.CoversAddressRange
	        ? address - candidate.LookupAddress < candidate.Resource.ResourceSizeInBytes
	        : address == candidate.LookupAddress;
	if (!addressMatches)
	{
		return false;
	}

	outResource = candidate.Resource;
	return true;
}

void VulkanRecordingResourceTable::Resolve(
    const VulkanGpuAllocationRecord& record,
    VulkanRecordingResource& outResource) const noexcept
{
	outResource = BuildResource(record);
}

VulkanRecordingResourceUseToken VulkanRecordingResourceTable::Retain(
    RhiResourceHandle resource) const noexcept
{
	const std::shared_ptr<const ReadView> readView = std::atomic_load(&m_readView);
	if (readView == nullptr)
	{
		return {};
	}

	const ResourceEntry* const entry = FindResource(*readView, resource);
	return entry != nullptr && entry->Record != nullptr
	           ? Retain(*entry->Record)
	           : VulkanRecordingResourceUseToken{};
}

VulkanRecordingResourceUseToken VulkanRecordingResourceTable::Retain(
    VulkanGpuAllocationRecord& record) const noexcept
{
	RetainReference(record);

	VulkanRecordingResourceUseToken use;
	use.m_value = reinterpret_cast<std::uintptr_t>(&record);
	return use;
}

void VulkanRecordingResourceTable::Release(
    VulkanRecordingResourceUseToken use,
    RhiSubmissionToken submissionToken) const noexcept
{
	if (!use)
	{
		return;
	}

	auto* const record =
	    reinterpret_cast<VulkanGpuAllocationRecord*>(use.m_value);
	ReleaseReference(*record, submissionToken);
}

const VulkanRecordingResourceTable::ResourceEntry*
VulkanRecordingResourceTable::FindResource(
    const ReadView& readView,
    RhiResourceHandle resource) noexcept
{
	if (!resource)
	{
		return nullptr;
	}

	const std::uintptr_t resourceValue =
	    reinterpret_cast<std::uintptr_t>(resource.Value);
	const auto found = std::ranges::lower_bound(
	    readView.ResourcesByHandle,
	    resourceValue,
	    {},
	    [](const ResourceEntry& entry) noexcept
	    {
		    return entry.Resource.ResourceHandleValue;
	    });
	return found != readView.ResourcesByHandle.end() &&
	               found->Resource.ResourceHandleValue == resourceValue
	           ? &*found
	           : nullptr;
}

VulkanRecordingResource VulkanRecordingResourceTable::BuildResource(
    const VulkanGpuAllocationRecord& record) noexcept
{
	return VulkanRecordingResource{
	    .Buffer = record.Buffer,
	    .Image = record.Image,
	    .AccelerationStructure = record.AccelerationStructure,
	    .DeviceAddress = record.DeviceAddress,
	    .BufferDeviceAddress = record.BufferDeviceAddress,
	    .ResourceHandleValue = reinterpret_cast<std::uintptr_t>(GetVulkanResourceHandle(record).Value),
	    .ResourceSizeInBytes = record.ResourceSizeInBytes,
	    .Format = record.Format,
	    .Extent = record.Extent,
	    .AspectMask = record.AspectMask,
	    .Usage = record.Usage,
	    .AccelerationStructureType = record.AccelerationStructureType,
	    .ResourceKind = record.ResourceKind,
	    .IsPartitionedAccelerationStructure =
	        record.IsPartitionedAccelerationStructure};
}

void VulkanRecordingResourceTable::RetainReference(
    VulkanGpuAllocationRecord& record) noexcept
{
	record.RecordingReferenceCount.fetch_add(1, std::memory_order_relaxed);
	if (record.ParentMemoryBlock != nullptr)
	{
		record.ParentMemoryBlock->RecordingReferenceCount.fetch_add(
		    1,
		    std::memory_order_relaxed);
	}
}

void VulkanRecordingResourceTable::ReleaseReference(
    VulkanGpuAllocationRecord& record) noexcept
{
	const std::uint32_t previousReferences =
	    record.RecordingReferenceCount.fetch_sub(
	        1,
	        std::memory_order_relaxed);
	assert(previousReferences != 0);

	if (record.ParentMemoryBlock != nullptr)
	{
		const std::uint32_t previousBlockReferences =
		    record.ParentMemoryBlock->RecordingReferenceCount.fetch_sub(
		        1,
		        std::memory_order_relaxed);
		assert(previousBlockReferences != 0);
	}
}

void VulkanRecordingResourceTable::ReleaseReference(
    VulkanGpuAllocationRecord& record,
    RhiSubmissionToken submissionToken) noexcept
{
	ReleaseReference(record);

	record.LastUse.MarkUsed(submissionToken);
	if (record.ParentMemoryBlock != nullptr)
	{
		record.ParentMemoryBlock->LastUse.MarkUsed(submissionToken);
	}
}

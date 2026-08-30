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
	std::size_t PublicationOrder = 0;
};

struct VulkanRecordingResourceTable::AddressEntry final
{
	VkDeviceAddress LookupAddress = 0;
	std::size_t ResourceIndex = 0;
	std::size_t PublicationOrder = 0;
};

struct VulkanRecordingResourceTable::ReadView final
{
	~ReadView() noexcept;

	std::vector<ResourceEntry> ResourcesByHandle;
	std::vector<AddressEntry> ResourcesByExactAddress;
	std::vector<AddressEntry> ResourcesByBufferAddress;
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

void VulkanRecordingResourceTable::Publish(std::span<VulkanGpuAllocationRecord* const> records) noexcept
{
	std::shared_ptr<ReadView> readView = BuildReadView(records);
	m_readView.store(std::shared_ptr<const ReadView>(std::move(readView)), std::memory_order_release);
}

bool VulkanRecordingResourceTable::Resolve(RhiResourceHandle resource, VulkanRecordingResource& outResource) const noexcept
{
	const std::shared_ptr<const ReadView> readView = m_readView.load(std::memory_order_acquire);
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

bool VulkanRecordingResourceTable::Resolve(RhiGpuVirtualAddress address, VulkanRecordingResource& outResource) const noexcept
{
	if (address == 0)
	{
		return false;
	}
	if (Resolve(RhiResourceHandle{.Value = reinterpret_cast<void*>(address)}, outResource))
	{
		return true;
	}

	const std::shared_ptr<const ReadView> readView = m_readView.load(std::memory_order_acquire);
	if (readView == nullptr)
	{
		return false;
	}

	const ResourceEntry* entry = FindExactAddress(*readView, address);
	if (entry == nullptr)
	{
		entry = FindBufferAddress(*readView, address);
	}
	if (entry == nullptr)
	{
		return false;
	}

	outResource = entry->Resource;
	return true;
}

VulkanRecordingResourceUseToken VulkanRecordingResourceTable::Retain(RhiResourceHandle resource) const noexcept
{
	const std::shared_ptr<const ReadView> readView = m_readView.load(std::memory_order_acquire);
	if (readView == nullptr)
	{
		return {};
	}

	const ResourceEntry* const entry = FindResource(*readView, resource);
	return entry != nullptr && entry->Record != nullptr ? Retain(*entry->Record) : VulkanRecordingResourceUseToken{};
}

VulkanRecordingResourceUseToken VulkanRecordingResourceTable::Retain(VulkanGpuAllocationRecord& record) const noexcept
{
	RetainReference(record);

	VulkanRecordingResourceUseToken use;
	use.m_value = reinterpret_cast<std::uintptr_t>(&record);
	return use;
}

void VulkanRecordingResourceTable::Release(VulkanRecordingResourceUseToken use, RhiSubmissionToken submissionToken) const noexcept
{
	if (!use)
	{
		return;
	}

	auto* const record = reinterpret_cast<VulkanGpuAllocationRecord*>(use.m_value);
	ReleaseReference(*record, submissionToken);
}

std::shared_ptr<VulkanRecordingResourceTable::ReadView> VulkanRecordingResourceTable::BuildReadView(
    std::span<VulkanGpuAllocationRecord* const> records)
{
	auto readView = std::make_shared<ReadView>();
	readView->ResourcesByHandle.reserve(records.size());
	readView->ResourcesByExactAddress.reserve(records.size());
	readView->ResourcesByBufferAddress.reserve(records.size());

	CollectPublishedResources(records, *readView);
	std::ranges::sort(
	    readView->ResourcesByHandle,
	    {},
	    [](const ResourceEntry& entry) noexcept { return entry.Resource.ResourceHandleValue; });

	BuildAddressProjections(*readView);
	std::ranges::sort(readView->ResourcesByExactAddress, &AddressEntryPrecedes);
	std::ranges::sort(readView->ResourcesByBufferAddress, &AddressEntryPrecedes);
	return readView;
}

void VulkanRecordingResourceTable::CollectPublishedResources(std::span<VulkanGpuAllocationRecord* const> records, ReadView& readView)
{
	std::size_t publicationOrder = 0;
	for (VulkanGpuAllocationRecord* record : records)
	{
		if (record == nullptr || record->PendingRelease)
		{
			continue;
		}

		RetainReference(*record);
		readView.ResourcesByHandle.push_back(
		    ResourceEntry{.Resource = BuildResource(*record), .Record = record, .PublicationOrder = publicationOrder++});
	}
}

void VulkanRecordingResourceTable::BuildAddressProjections(ReadView& readView)
{
	for (std::size_t resourceIndex = 0; resourceIndex < readView.ResourcesByHandle.size(); ++resourceIndex)
	{
		const ResourceEntry& resourceEntry = readView.ResourcesByHandle[resourceIndex];
		const VulkanRecordingResource& resource = resourceEntry.Resource;
		if (resource.BufferDeviceAddress != 0)
		{
			readView.ResourcesByBufferAddress.push_back(
			    AddressEntry{
			        .LookupAddress = resource.BufferDeviceAddress,
			        .ResourceIndex = resourceIndex,
			        .PublicationOrder = resourceEntry.PublicationOrder});
		}
		if (resource.DeviceAddress != 0)
		{
			readView.ResourcesByExactAddress.push_back(
			    AddressEntry{
			        .LookupAddress = resource.DeviceAddress,
			        .ResourceIndex = resourceIndex,
			        .PublicationOrder = resourceEntry.PublicationOrder});
		}
	}
}

const VulkanRecordingResourceTable::ResourceEntry* VulkanRecordingResourceTable::FindResource(
    const ReadView& readView,
    RhiResourceHandle resource) noexcept
{
	if (!resource)
	{
		return nullptr;
	}

	const std::uintptr_t resourceValue = reinterpret_cast<std::uintptr_t>(resource.Value);
	const auto found = std::ranges::lower_bound(
	    readView.ResourcesByHandle,
	    resourceValue,
	    {},
	    [](const ResourceEntry& entry) noexcept { return entry.Resource.ResourceHandleValue; });
	return found != readView.ResourcesByHandle.end() && found->Resource.ResourceHandleValue == resourceValue ? &*found : nullptr;
}

const VulkanRecordingResourceTable::ResourceEntry* VulkanRecordingResourceTable::FindExactAddress(
    const ReadView& readView,
    RhiGpuVirtualAddress address) noexcept
{
	const auto found = std::ranges::lower_bound(readView.ResourcesByExactAddress, address, {}, &AddressEntry::LookupAddress);
	return found != readView.ResourcesByExactAddress.end() && found->LookupAddress == address
	        && found->ResourceIndex < readView.ResourcesByHandle.size()
	    ? &readView.ResourcesByHandle[found->ResourceIndex]
	    : nullptr;
}

const VulkanRecordingResourceTable::ResourceEntry* VulkanRecordingResourceTable::FindBufferAddress(
    const ReadView& readView,
    RhiGpuVirtualAddress address) noexcept
{
	const auto after = std::ranges::upper_bound(readView.ResourcesByBufferAddress, address, {}, &AddressEntry::LookupAddress);
	if (after == readView.ResourcesByBufferAddress.begin())
	{
		return nullptr;
	}

	const VkDeviceAddress baseAddress = std::prev(after)->LookupAddress;
	const auto firstAtBase = std::ranges::lower_bound(readView.ResourcesByBufferAddress, baseAddress, {}, &AddressEntry::LookupAddress);
	for (auto candidate = firstAtBase; candidate != after && candidate->LookupAddress == baseAddress; ++candidate)
	{
		if (candidate->ResourceIndex >= readView.ResourcesByHandle.size())
		{
			continue;
		}

		const ResourceEntry& resource = readView.ResourcesByHandle[candidate->ResourceIndex];
		if (address - baseAddress < resource.Resource.ResourceSizeInBytes)
		{
			return &resource;
		}
	}

	return nullptr;
}

bool VulkanRecordingResourceTable::AddressEntryPrecedes(const AddressEntry& left, const AddressEntry& right) noexcept
{
	if (left.LookupAddress != right.LookupAddress)
	{
		return left.LookupAddress < right.LookupAddress;
	}

	return left.PublicationOrder < right.PublicationOrder;
}

VulkanRecordingResource VulkanRecordingResourceTable::BuildResource(const VulkanGpuAllocationRecord& record) noexcept
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
	    .IsPartitionedAccelerationStructure = record.IsPartitionedAccelerationStructure};
}

void VulkanRecordingResourceTable::RetainReference(VulkanGpuAllocationRecord& record) noexcept
{
	record.RecordingReferenceCount.fetch_add(1, std::memory_order_relaxed);
	if (record.ParentMemoryBlock != nullptr)
	{
		record.ParentMemoryBlock->RecordingReferenceCount.fetch_add(1, std::memory_order_relaxed);
	}
}

void VulkanRecordingResourceTable::ReleaseReference(VulkanGpuAllocationRecord& record) noexcept
{
	const std::uint32_t previousReferences = record.RecordingReferenceCount.fetch_sub(1, std::memory_order_relaxed);
	assert(previousReferences != 0);

	if (record.ParentMemoryBlock != nullptr)
	{
		const std::uint32_t previousBlockReferences =
		    record.ParentMemoryBlock->RecordingReferenceCount.fetch_sub(1, std::memory_order_relaxed);
		assert(previousBlockReferences != 0);
	}
}

void VulkanRecordingResourceTable::ReleaseReference(VulkanGpuAllocationRecord& record, RhiSubmissionToken submissionToken) noexcept
{
	record.LastUse.MarkUsed(submissionToken);
	if (record.ParentMemoryBlock != nullptr)
	{
		record.ParentMemoryBlock->LastUse.MarkUsed(submissionToken);
	}

	ReleaseReference(record);
}

#include "Vulkan/VulkanPCH.h"

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include "Vulkan/Memory/VulkanGpuMemoryAllocator.h"

#include "Core/Public/Strings/StringUtils.h"
#include "Vulkan/Core/VulkanResult.h"
#include "Vulkan/Device/VulkanRhi.h"
#include "Vulkan/Diagnostics/VulkanDebugNames.h"
#include "Vulkan/Memory/VulkanRecordingResourceTable.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <mutex>
#include <string>
#include <vector>

static const auto g_vulkanMemoryLogger = Logging::GetOrCreateLogger("RHI.Vulkan.Memory");

struct VulkanGpuMemoryAllocator::PendingAllocationRelease final
{
	std::unique_ptr<VulkanGpuAllocationRecord> Record;
};

struct VulkanGpuMemoryAllocator::PendingMemoryBlockRelease final
{
	std::unique_ptr<VulkanGpuMemoryBlockRecord> Record;
};

struct VulkanGpuMemoryAllocator::CategoryAggregation final
{
	RhiMemoryCategoryStats Stats;
	std::vector<std::uint32_t> UniqueHeapIndices;
};

struct VulkanGpuMemoryAllocator::Impl final
{
	VmaAllocator Allocator = nullptr;
	VkPhysicalDeviceMemoryProperties MemoryProperties = {};
	mutable std::mutex RecordsMutex;
	std::vector<VulkanGpuAllocationRecord*> LiveRecords;
	std::vector<VulkanGpuMemoryBlockRecord*> LiveMemoryBlockRecords;
	std::vector<PendingAllocationRelease> PendingReleases;
	std::vector<PendingMemoryBlockRelease> PendingMemoryBlockReleases;

	~Impl() noexcept
	{
		PendingReleases.clear();
		PendingMemoryBlockReleases.clear();
		if (Allocator != nullptr)
		{
			vmaDestroyAllocator(Allocator);
			Allocator = nullptr;
		}
	}
};

std::uint32_t VulkanGpuMemoryAllocator::ResolveVmaMemoryUsage(RhiMemoryResidencyClass residencyClass) noexcept
{
	switch (residencyClass)
	{
		case RhiMemoryResidencyClass::HostUpload:
		case RhiMemoryResidencyClass::HostReadback:
			return VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
		case RhiMemoryResidencyClass::DeviceLocal:
		case RhiMemoryResidencyClass::Transient:
		default:
			return VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
	}
}

std::uint32_t VulkanGpuMemoryAllocator::ResolveVmaAllocationFlags(RhiMemoryResidencyClass residencyClass) noexcept
{
	switch (residencyClass)
	{
		case RhiMemoryResidencyClass::HostUpload:
			return VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
		case RhiMemoryResidencyClass::HostReadback:
			return VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
		case RhiMemoryResidencyClass::DeviceLocal:
		case RhiMemoryResidencyClass::Transient:
		default:
			return 0;
	}
}

VulkanGpuMemoryAllocator::CategoryAggregation& VulkanGpuMemoryAllocator::FindOrCreateCategoryAggregation(
    std::vector<CategoryAggregation>& aggregations,
    RhiMemoryCategory category,
    RhiMemoryResidencyClass residencyClass)
{
	auto existing = std::find_if(
	    aggregations.begin(),
	    aggregations.end(),
	    [category, residencyClass](const CategoryAggregation& aggregation)
	    {
		    return aggregation.Stats.Category == category && aggregation.Stats.ResidencyClass == residencyClass;
	    });
	if (existing != aggregations.end())
	{
		return *existing;
	}

	CategoryAggregation aggregation;
	aggregation.Stats.Category = category;
	aggregation.Stats.ResidencyClass = residencyClass;
	aggregations.push_back(std::move(aggregation));
	return aggregations.back();
}

void VulkanGpuMemoryAllocator::AddHeapReference(
    CategoryAggregation& aggregation,
    std::uint32_t heapIndex,
    std::uint64_t heapBudgetBytes) noexcept
{
	if (heapIndex == UINT32_MAX)
	{
		return;
	}

	const auto existing = std::find(aggregation.UniqueHeapIndices.begin(), aggregation.UniqueHeapIndices.end(), heapIndex);
	if (existing != aggregation.UniqueHeapIndices.end())
	{
		return;
	}

	aggregation.UniqueHeapIndices.push_back(heapIndex);
	++aggregation.Stats.BlockCount;
	aggregation.Stats.BudgetBytes += heapBudgetBytes;
}

VulkanGpuMemoryAllocator::VulkanGpuMemoryAllocator(VulkanRhi& rhi) noexcept :
	m_rhi(rhi),
	m_impl(std::make_unique<Impl>()),
	m_recordingResources(std::make_unique<VulkanRecordingResourceTable>())
{
	if (m_rhi.GetInstance() == VK_NULL_HANDLE || m_rhi.GetPhysicalDevice() == VK_NULL_HANDLE || m_rhi.GetDevice() == VK_NULL_HANDLE)
	{
		Diagnostics::Fail(g_vulkanMemoryLogger, __FILE__, __LINE__, "VulkanGpuMemoryAllocator requires a valid instance, physical device, and device");
	}

	vkGetPhysicalDeviceMemoryProperties(m_rhi.GetPhysicalDevice(), &m_impl->MemoryProperties);

	const bool memoryBudgetExtensionEnabled = std::any_of(
	    m_rhi.GetEnabledDeviceExtensions().begin(),
	    m_rhi.GetEnabledDeviceExtensions().end(),
	    [](const std::string& extension) noexcept { return extension == VK_EXT_MEMORY_BUDGET_EXTENSION_NAME; });
	VmaAllocatorCreateFlags allocatorFlags = memoryBudgetExtensionEnabled ? VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT : 0;
	if (m_rhi.GetRayTracingCapabilities().SupportsRayTracing)
	{
		allocatorFlags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
	}
	const std::uint32_t vulkanApiVersion = std::min(m_rhi.GetAdapterInfo().ApiVersion, static_cast<std::uint32_t>(VK_API_VERSION_1_3));
	const VmaAllocatorCreateInfo allocatorCreateInfo{
	    .flags = allocatorFlags,
	    .physicalDevice = m_rhi.GetPhysicalDevice(),
	    .device = m_rhi.GetDevice(),
	    .preferredLargeHeapBlockSize = 0,
	    .pAllocationCallbacks = nullptr,
	    .pDeviceMemoryCallbacks = nullptr,
	    .pHeapSizeLimit = nullptr,
	    .pVulkanFunctions = nullptr,
	    .instance = m_rhi.GetInstance(),
	    .vulkanApiVersion = vulkanApiVersion,
	    .pTypeExternalMemoryHandleTypes = nullptr};
	const VkResult result = vmaCreateAllocator(&allocatorCreateInfo, &m_impl->Allocator);
	if (!VulkanResult::Succeeded(result))
	{
		Diagnostics::Fail(g_vulkanMemoryLogger, __FILE__, __LINE__, VulkanResult::FormatFailure("vmaCreateAllocator", result));
	}
}

VulkanGpuMemoryAllocator::~VulkanGpuMemoryAllocator() noexcept = default;

bool VulkanGpuMemoryAllocator::IsInitialized() const noexcept
{
	return m_impl != nullptr && m_impl->Allocator != nullptr;
}

bool VulkanGpuMemoryAllocator::SupportsBudgetQueries() const noexcept
{
	return IsInitialized();
}

RhiMemoryUsageSnapshot VulkanGpuMemoryAllocator::CreateMemoryUsageSnapshot() const
{
	m_owner.AssertAccess();
	RhiMemoryUsageSnapshot snapshot;
	if (m_impl == nullptr || m_impl->Allocator == nullptr)
	{
		return snapshot;
	}

	std::array<VmaBudget, VK_MAX_MEMORY_HEAPS> heapBudgets = {};
	vmaGetHeapBudgets(m_impl->Allocator, heapBudgets.data());

	VmaTotalStatistics totalStats = {};
	vmaCalculateStatistics(m_impl->Allocator, &totalStats);
	snapshot.AllocatorBackend = ERhiMemoryAllocatorBackend::VulkanManaged;
	snapshot.HasBudgetData = true;
	snapshot.HasDelayedDestructionTracking = true;
	snapshot.TotalUsedBytes = totalStats.total.statistics.allocationBytes;
	snapshot.TotalAllocatedBytes = totalStats.total.statistics.blockBytes;

	for (std::uint32_t heapIndex = 0; heapIndex < m_impl->MemoryProperties.memoryHeapCount; ++heapIndex)
	{
		snapshot.TotalBudgetBytes += heapBudgets[heapIndex].budget;
		snapshot.ApiUsageBytes += heapBudgets[heapIndex].usage;
	}

	std::vector<CategoryAggregation> aggregations;
	{
		std::scoped_lock lock(m_impl->RecordsMutex);
		aggregations.reserve(m_impl->LiveRecords.size());

		for (const VulkanGpuAllocationRecord* record : m_impl->LiveRecords)
		{
			if (record == nullptr || record->Allocation == nullptr || record->ParentMemoryBlock != nullptr)
			{
				continue;
			}

			CategoryAggregation& aggregation =
			    FindOrCreateCategoryAggregation(aggregations, record->Category, record->ResidencyClass);
			++aggregation.Stats.AllocationCount;
			++aggregation.Stats.ResourceCount;
			aggregation.Stats.UsedBytes += record->UsedBytes;
			aggregation.Stats.AllocatedBytes += record->AllocatedBytes;
			snapshot.CommittedUsageBytes += record->AllocatedBytes;
			if (record->ResidencyClass == RhiMemoryResidencyClass::Transient || record->Category == RhiMemoryCategory::TransientResource)
			{
				snapshot.TransientUsageBytes += record->AllocatedBytes;
			}
			if (record->MemoryHeapIndex < heapBudgets.size())
			{
				AddHeapReference(aggregation, record->MemoryHeapIndex, heapBudgets[record->MemoryHeapIndex].budget);
			}
		}

		for (const VulkanGpuMemoryBlockRecord* record : m_impl->LiveMemoryBlockRecords)
		{
			if (record == nullptr || record->Allocation == nullptr)
			{
				continue;
			}

			CategoryAggregation& aggregation =
			    FindOrCreateCategoryAggregation(aggregations, record->Category, record->ResidencyClass);
			++aggregation.Stats.AllocationCount;
			aggregation.Stats.ResourceCount += record->AliasingResourceCount;
			aggregation.Stats.UsedBytes += record->UsedBytes;
			aggregation.Stats.AllocatedBytes += record->AllocatedBytes;
			snapshot.PlacedUsageBytes += record->AllocatedBytes;
			if (record->ResidencyClass == RhiMemoryResidencyClass::Transient || record->Category == RhiMemoryCategory::TransientResource)
			{
				snapshot.TransientUsageBytes += record->AllocatedBytes;
			}
			if (record->MemoryHeapIndex < heapBudgets.size())
			{
				AddHeapReference(aggregation, record->MemoryHeapIndex, heapBudgets[record->MemoryHeapIndex].budget);
			}
		}

		for (const PendingAllocationRelease& pendingRelease : m_impl->PendingReleases)
		{
			if (pendingRelease.Record != nullptr)
			{
				snapshot.DelayedDestructionBytes += pendingRelease.Record->AllocatedBytes;
				++snapshot.DelayedDestructionAllocationCount;
			}
		}

		for (const PendingMemoryBlockRelease& pendingRelease : m_impl->PendingMemoryBlockReleases)
		{
			if (pendingRelease.Record != nullptr)
			{
				snapshot.DelayedDestructionBytes += pendingRelease.Record->AllocatedBytes;
				++snapshot.DelayedDestructionAllocationCount;
			}
		}
	}

	snapshot.CategoryStats.reserve(aggregations.size());
	for (const CategoryAggregation& aggregation : aggregations)
	{
		snapshot.CategoryStats.push_back(aggregation.Stats);
	}

	return snapshot;
}

std::unique_ptr<VulkanGpuAllocationRecord> VulkanGpuMemoryAllocator::CreateBuffer(
    const VkBufferCreateInfo& bufferCreateInfo,
    RhiMemoryCategory category,
    RhiMemoryResidencyClass residencyClass,
    std::wstring_view debugName) noexcept
{
	if (m_impl == nullptr || m_impl->Allocator == nullptr)
	{
		return {};
	}

	const VmaAllocationCreateInfo allocationCreateInfo{
	    .flags = static_cast<VmaAllocationCreateFlags>(ResolveVmaAllocationFlags(residencyClass)),
	    .usage = static_cast<VmaMemoryUsage>(ResolveVmaMemoryUsage(residencyClass))};
	VkBufferCreateInfo nativeCreateInfo = bufferCreateInfo;
	m_rhi.ConfigureResourceQueueSharing(nativeCreateInfo);
	VkBuffer buffer = VK_NULL_HANDLE;
	VmaAllocation allocation = nullptr;
	const VkResult result = vmaCreateBuffer(m_impl->Allocator, &nativeCreateInfo, &allocationCreateInfo, &buffer, &allocation, nullptr);
	if (!VulkanResult::Succeeded(result) || buffer == VK_NULL_HANDLE || allocation == nullptr)
	{
		return {};
	}

	std::unique_ptr<VulkanGpuAllocationRecord> record =
	    CreateAllocationRecord(VulkanGpuAllocationResourceKind::Buffer, buffer, VK_NULL_HANDLE, allocation, category, residencyClass, debugName);
	if (record != nullptr)
	{
		record->ResourceSizeInBytes = bufferCreateInfo.size;
		record->Usage = bufferCreateInfo.usage;
		if ((bufferCreateInfo.usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) != 0 && m_rhi.GetGetBufferDeviceAddress() != nullptr)
		{
			const VkBufferDeviceAddressInfo addressInfo{.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .pNext = nullptr, .buffer = buffer};
			record->BufferDeviceAddress = m_rhi.GetGetBufferDeviceAddress()(m_rhi.GetDevice(), &addressInfo);
			record->DeviceAddress = record->BufferDeviceAddress;
		}
	}
	return record;
}

std::unique_ptr<VulkanGpuAllocationRecord> VulkanGpuMemoryAllocator::CreateImage(
    const VkImageCreateInfo& imageCreateInfo,
    RhiMemoryCategory category,
    RhiMemoryResidencyClass residencyClass,
    std::wstring_view debugName) noexcept
{
	if (m_impl == nullptr || m_impl->Allocator == nullptr)
	{
		return {};
	}

	const VmaAllocationCreateInfo allocationCreateInfo{
	    .flags = static_cast<VmaAllocationCreateFlags>(ResolveVmaAllocationFlags(residencyClass)),
	    .usage = static_cast<VmaMemoryUsage>(ResolveVmaMemoryUsage(residencyClass))};
	VkImageCreateInfo nativeCreateInfo = imageCreateInfo;
	m_rhi.ConfigureResourceQueueSharing(nativeCreateInfo);
	VkImage image = VK_NULL_HANDLE;
	VmaAllocation allocation = nullptr;
	const VkResult result = vmaCreateImage(m_impl->Allocator, &nativeCreateInfo, &allocationCreateInfo, &image, &allocation, nullptr);
	if (!VulkanResult::Succeeded(result) || image == VK_NULL_HANDLE || allocation == nullptr)
	{
		return {};
	}

	std::unique_ptr<VulkanGpuAllocationRecord> record =
	    CreateAllocationRecord(VulkanGpuAllocationResourceKind::Image, VK_NULL_HANDLE, image, allocation, category, residencyClass, debugName);
	if (record != nullptr)
	{
		record->Format = imageCreateInfo.format;
		record->Extent = imageCreateInfo.extent;
		record->AspectMask = ResolveImageAspectMask(imageCreateInfo.format);
		record->ImageFlags = imageCreateInfo.flags;
		record->Usage = imageCreateInfo.usage;
	}
	return record;
}

std::unique_ptr<VulkanGpuMemoryBlockRecord> VulkanGpuMemoryAllocator::CreateTransientMemoryBlock(
    RhiTransientAllocationPool,
    std::uint64_t sizeInBytes,
    std::uint64_t alignment,
    std::wstring_view debugName) noexcept
{
	if (m_impl == nullptr || m_impl->Allocator == nullptr || sizeInBytes == 0)
	{
		return {};
	}

	auto record = std::make_unique<VulkanGpuMemoryBlockRecord>();
	record->Category = RhiMemoryCategory::TransientResource;
	record->ResidencyClass = RhiMemoryResidencyClass::Transient;
	record->RequestedSizeInBytes = sizeInBytes;
	record->UsedBytes = sizeInBytes;
	record->Alignment = alignment;
	record->Owner = this;
	record->DebugName = std::wstring(debugName);
	RegisterMemoryBlockRecord(*record);
	return record;
}

std::unique_ptr<VulkanGpuAllocationRecord> VulkanGpuMemoryAllocator::CreateAliasingImage(
    VulkanGpuMemoryBlockRecord& memoryBlock,
    std::uint64_t memoryBlockOffset,
    const VkImageCreateInfo& imageCreateInfo,
    std::wstring_view debugName) noexcept
{
	VkImageCreateInfo nativeCreateInfo = imageCreateInfo;
	m_rhi.ConfigureResourceQueueSharing(nativeCreateInfo);
	if (m_impl == nullptr || m_impl->Allocator == nullptr || !EnsureMemoryBlockAllocationForImage(memoryBlock, nativeCreateInfo) ||
	    memoryBlock.Allocation == nullptr)
	{
		return {};
	}

	VkImage image = VK_NULL_HANDLE;
	const VkResult result = vmaCreateAliasingImage2(
	    m_impl->Allocator,
	    memoryBlock.Allocation,
	    static_cast<VkDeviceSize>(memoryBlockOffset),
	    &nativeCreateInfo,
	    &image);
	if (!VulkanResult::Succeeded(result) || image == VK_NULL_HANDLE)
	{
		return {};
	}

	std::unique_ptr<VulkanGpuAllocationRecord> record =
	    CreateAliasingAllocationRecord(VulkanGpuAllocationResourceKind::Image, VK_NULL_HANDLE, image, memoryBlock, debugName);
	if (record != nullptr)
	{
		record->Format = imageCreateInfo.format;
		record->Extent = imageCreateInfo.extent;
		record->AspectMask = ResolveImageAspectMask(imageCreateInfo.format);
		record->ImageFlags = imageCreateInfo.flags;
		record->Usage = imageCreateInfo.usage;
	}
	return record;
}

std::unique_ptr<VulkanGpuAllocationRecord> VulkanGpuMemoryAllocator::CreateAliasingBuffer(
    VulkanGpuMemoryBlockRecord& memoryBlock,
    std::uint64_t memoryBlockOffset,
    const VkBufferCreateInfo& bufferCreateInfo,
    std::wstring_view debugName) noexcept
{
	VkBufferCreateInfo nativeCreateInfo = bufferCreateInfo;
	m_rhi.ConfigureResourceQueueSharing(nativeCreateInfo);
	if (m_impl == nullptr || m_impl->Allocator == nullptr || !EnsureMemoryBlockAllocationForBuffer(memoryBlock, nativeCreateInfo) ||
	    memoryBlock.Allocation == nullptr)
	{
		return {};
	}

	VkBuffer buffer = VK_NULL_HANDLE;
	const VkResult result = vmaCreateAliasingBuffer2(
	    m_impl->Allocator,
	    memoryBlock.Allocation,
	    static_cast<VkDeviceSize>(memoryBlockOffset),
	    &nativeCreateInfo,
	    &buffer);
	if (!VulkanResult::Succeeded(result) || buffer == VK_NULL_HANDLE)
	{
		return {};
	}

	std::unique_ptr<VulkanGpuAllocationRecord> record =
	    CreateAliasingAllocationRecord(VulkanGpuAllocationResourceKind::Buffer, buffer, VK_NULL_HANDLE, memoryBlock, debugName);
	if (record != nullptr)
	{
		record->Usage = bufferCreateInfo.usage;
		record->ResourceSizeInBytes = bufferCreateInfo.size;
	}
	return record;
}

bool VulkanGpuMemoryAllocator::WriteAllocation(
    VulkanGpuAllocationRecord& record,
    const void* data,
    std::size_t sizeInBytes,
    std::size_t destinationOffsetInBytes) noexcept
{
	if (m_impl == nullptr || m_impl->Allocator == nullptr ||
	    record.Allocation == nullptr || data == nullptr || sizeInBytes == 0 ||
	    destinationOffsetInBytes > record.ResourceSizeInBytes ||
	    sizeInBytes > record.ResourceSizeInBytes - destinationOffsetInBytes)
	{
		return false;
	}

	void* mappedData = nullptr;
	const VkResult mapResult = vmaMapMemory(m_impl->Allocator, record.Allocation, &mappedData);
	if (!VulkanResult::Succeeded(mapResult) || mappedData == nullptr)
	{
		return false;
	}

	record.IsMapped = true;
	record.CpuMappedAddress = mappedData;
	std::memcpy(
	    static_cast<std::byte*>(mappedData) + destinationOffsetInBytes,
	    data,
	    sizeInBytes);
	(void)vmaFlushAllocation(
	    m_impl->Allocator,
	    record.Allocation,
	    destinationOffsetInBytes,
	    sizeInBytes);
	vmaUnmapMemory(m_impl->Allocator, record.Allocation);
	record.IsMapped = false;
	record.CpuMappedAddress = nullptr;
	return true;
}

void* VulkanGpuMemoryAllocator::MapUploadPage(
    VulkanGpuAllocationRecord& record) noexcept
{
	if (m_impl == nullptr ||
	    m_impl->Allocator == nullptr ||
	    record.Allocation == nullptr ||
	    record.ResidencyClass != RhiMemoryResidencyClass::HostUpload)
	{
		return nullptr;
	}
	if (record.IsMapped)
	{
		return record.CpuMappedAddress;
	}

	void* mappedData = nullptr;
	const VkResult result =
	    vmaMapMemory(m_impl->Allocator, record.Allocation, &mappedData);
	if (!VulkanResult::Succeeded(result) || mappedData == nullptr)
	{
		return nullptr;
	}

	record.IsMapped = true;
	record.CpuMappedAddress = mappedData;
	return mappedData;
}

bool VulkanGpuMemoryAllocator::FlushUploadPage(
    VulkanGpuAllocationRecord& record,
    std::size_t offsetInBytes,
    std::size_t sizeInBytes) noexcept
{
	if (m_impl == nullptr ||
	    m_impl->Allocator == nullptr ||
	    record.Allocation == nullptr ||
	    !record.IsMapped ||
	    offsetInBytes > record.ResourceSizeInBytes ||
	    sizeInBytes > record.ResourceSizeInBytes - offsetInBytes)
	{
		return false;
	}

	const VkResult result =
	    vmaFlushAllocation(
	        m_impl->Allocator,
	        record.Allocation,
	        offsetInBytes,
	        sizeInBytes);
	return VulkanResult::Succeeded(result);
}

void VulkanGpuMemoryAllocator::PublishRecordingReadView() noexcept
{
	m_owner.AssertAccess();
	if (m_impl == nullptr)
	{
		return;
	}

	m_recordingResources->Publish(m_impl->LiveRecords);
}

bool VulkanGpuMemoryAllocator::ResolveRecordingResource(
    RhiResourceHandle resource,
    VulkanRecordingResource& outResource) const noexcept
{
	return m_recordingResources->Resolve(resource, outResource);
}

bool VulkanGpuMemoryAllocator::ResolveRecordingAddress(
    RhiGpuVirtualAddress address,
    VulkanRecordingResource& outResource) const noexcept
{
	return m_recordingResources->Resolve(address, outResource);
}

bool VulkanGpuMemoryAllocator::ResolveCoordinatorRecordingResource(
    RhiResourceHandle resource,
    VulkanRecordingResource& outResource) const noexcept
{
	VulkanGpuAllocationRecord* const record = FindAllocationRecord(resource);
	if (record == nullptr)
	{
		return false;
	}

	m_recordingResources->Resolve(*record, outResource);
	return true;
}

bool VulkanGpuMemoryAllocator::ResolveCoordinatorRecordingAddress(
    RhiGpuVirtualAddress address,
    VulkanRecordingResource& outResource) const noexcept
{
	VulkanGpuAllocationRecord* const record =
	    FindAllocationRecordByDeviceAddress(address);
	if (record == nullptr)
	{
		return false;
	}

	m_recordingResources->Resolve(*record, outResource);
	return true;
}

VulkanRecordingResourceUseToken VulkanGpuMemoryAllocator::RetainRecordingResource(RhiResourceHandle resource) const noexcept
{
	return m_recordingResources->Retain(resource);
}

VulkanRecordingResourceUseToken VulkanGpuMemoryAllocator::RetainCoordinatorRecordingResource(
    RhiResourceHandle resource) const noexcept
{
	VulkanGpuAllocationRecord* const record = FindAllocationRecord(resource);
	return record != nullptr
	           ? m_recordingResources->Retain(*record)
	           : VulkanRecordingResourceUseToken{};
}

void VulkanGpuMemoryAllocator::ReleaseRecordingResource(
    VulkanRecordingResourceUseToken use,
    RhiSubmissionToken submissionToken) const noexcept
{
	m_recordingResources->Release(use, submissionToken);
}

VulkanGpuAllocationRecord* VulkanGpuMemoryAllocator::FindAllocationRecord(RhiResourceHandle resource) const noexcept
{
	m_owner.AssertAccess();
	if (m_impl == nullptr || !resource)
	{
		return nullptr;
	}

	std::scoped_lock lock(m_impl->RecordsMutex);
	for (VulkanGpuAllocationRecord* record : m_impl->LiveRecords)
	{
		if (record == nullptr || record->PendingRelease)
		{
			continue;
		}

		if (GetVulkanResourceHandle(*record).Value == resource.Value)
		{
			return record;
		}
	}

	return nullptr;
}

VulkanGpuAllocationRecord* VulkanGpuMemoryAllocator::FindAllocationRecordByDeviceAddress(VkDeviceAddress deviceAddress) const noexcept
{
	m_owner.AssertAccess();
	if (m_impl == nullptr || deviceAddress == 0)
	{
		return nullptr;
	}

	std::scoped_lock lock(m_impl->RecordsMutex);
	for (VulkanGpuAllocationRecord* record : m_impl->LiveRecords)
	{
		if (record == nullptr || record->PendingRelease)
		{
			continue;
		}

		const bool matchesDeviceAddress = record->DeviceAddress == deviceAddress;
		const bool fallsWithinBuffer =
		    record->BufferDeviceAddress != 0 && deviceAddress >= record->BufferDeviceAddress &&
		    deviceAddress - record->BufferDeviceAddress < record->ResourceSizeInBytes;
		if (matchesDeviceAddress || fallsWithinBuffer)
		{
			return record;
		}
	}

	return nullptr;
}

void VulkanGpuMemoryAllocator::QueueDestroyResource(std::unique_ptr<VulkanGpuAllocationRecord> record) noexcept
{
	m_owner.AssertAccess();
	if (m_impl == nullptr || record == nullptr)
	{
		return;
	}

	std::scoped_lock lock(m_impl->RecordsMutex);
	record->PendingRelease = true;
	m_impl->PendingReleases.push_back(PendingAllocationRelease{.Record = std::move(record)});
}

void VulkanGpuMemoryAllocator::QueueDestroyMemoryBlock(std::unique_ptr<VulkanGpuMemoryBlockRecord> record) noexcept
{
	m_owner.AssertAccess();
	if (m_impl == nullptr || record == nullptr)
	{
		return;
	}

	std::scoped_lock lock(m_impl->RecordsMutex);
	m_impl->PendingMemoryBlockReleases.push_back(PendingMemoryBlockRelease{.Record = std::move(record)});
}

void VulkanGpuMemoryAllocator::DrainCompletedReleases(
	const std::array<std::uint64_t, RhiQueueTypeCount>& completedValues) noexcept
{
	m_owner.AssertAccess();
	if (m_impl == nullptr)
	{
		return;
	}

	std::vector<std::unique_ptr<VulkanGpuMemoryBlockRecord>> readyMemoryBlockReleases;
	std::vector<std::unique_ptr<VulkanGpuAllocationRecord>> readyReleases;
	{
		std::scoped_lock lock(m_impl->RecordsMutex);
		auto pending = m_impl->PendingReleases.begin();
		while (pending != m_impl->PendingReleases.end())
		{
			if (pending->Record == nullptr ||
			    (pending->Record->RecordingReferenceCount.load(
			         std::memory_order_relaxed) == 0 &&
			     pending->Record->LastUse.IsComplete(completedValues)))
			{
				readyReleases.push_back(std::move(pending->Record));
				pending = m_impl->PendingReleases.erase(pending);
			}
			else
			{
				++pending;
			}
		}

		auto pendingMemoryBlock = m_impl->PendingMemoryBlockReleases.begin();
		while (pendingMemoryBlock != m_impl->PendingMemoryBlockReleases.end())
		{
			if (pendingMemoryBlock->Record == nullptr ||
			    (pendingMemoryBlock->Record->RecordingReferenceCount.load(
			         std::memory_order_relaxed) == 0 &&
			     pendingMemoryBlock->Record->LastUse.IsComplete(completedValues)))
			{
				readyMemoryBlockReleases.push_back(std::move(pendingMemoryBlock->Record));
				pendingMemoryBlock = m_impl->PendingMemoryBlockReleases.erase(pendingMemoryBlock);
			}
			else
			{
				++pendingMemoryBlock;
			}
		}
	}

	readyReleases.clear();
	readyMemoryBlockReleases.clear();
}

void VulkanGpuMemoryAllocator::FlushPendingReleases() noexcept
{
	std::array<std::uint64_t, RhiQueueTypeCount> completedValues{};
	completedValues.fill(UINT64_MAX);
	DrainCompletedReleases(completedValues);
}

void VulkanGpuMemoryAllocator::DestroyAllocation(VulkanGpuAllocationRecord& record) noexcept
{
	if (m_impl == nullptr || m_impl->Allocator == nullptr)
	{
		return;
	}

	if (!record.OwnsAllocation || record.ParentMemoryBlock != nullptr)
	{
		if (record.AccelerationStructure != VK_NULL_HANDLE && m_rhi.GetDestroyAccelerationStructure() != nullptr)
		{
			m_rhi.GetDestroyAccelerationStructure()(m_rhi.GetDevice(), record.AccelerationStructure, nullptr);
		}
		if (record.Buffer != VK_NULL_HANDLE)
		{
			vkDestroyBuffer(m_rhi.GetDevice(), record.Buffer, nullptr);
		}
		if (record.Image != VK_NULL_HANDLE)
		{
			vkDestroyImage(m_rhi.GetDevice(), record.Image, nullptr);
		}

		record.Buffer = VK_NULL_HANDLE;
		record.Image = VK_NULL_HANDLE;
		record.AccelerationStructure = VK_NULL_HANDLE;
		record.DeviceAddress = 0;
		record.BufferDeviceAddress = 0;
		record.AccelerationStructureType = VK_ACCELERATION_STRUCTURE_TYPE_MAX_ENUM_KHR;
		record.IsPartitionedAccelerationStructure = false;
		record.Allocation = nullptr;
		record.ResourceKind = VulkanGpuAllocationResourceKind::Unknown;
		return;
	}

	if (record.Allocation == nullptr)
	{
		return;
	}

	switch (record.ResourceKind)
	{
		case VulkanGpuAllocationResourceKind::Buffer:
			if (record.AccelerationStructure != VK_NULL_HANDLE && m_rhi.GetDestroyAccelerationStructure() != nullptr)
			{
				m_rhi.GetDestroyAccelerationStructure()(m_rhi.GetDevice(), record.AccelerationStructure, nullptr);
				record.AccelerationStructure = VK_NULL_HANDLE;
			}
			if (record.Buffer != VK_NULL_HANDLE)
			{
				vmaDestroyBuffer(m_impl->Allocator, record.Buffer, record.Allocation);
			}
			break;
		case VulkanGpuAllocationResourceKind::Image:
			if (record.Image != VK_NULL_HANDLE)
			{
				vmaDestroyImage(m_impl->Allocator, record.Image, record.Allocation);
			}
			break;
		case VulkanGpuAllocationResourceKind::Unknown:
		default:
			vmaFreeMemory(m_impl->Allocator, record.Allocation);
			break;
	}

	record.Buffer = VK_NULL_HANDLE;
	record.Image = VK_NULL_HANDLE;
	record.AccelerationStructure = VK_NULL_HANDLE;
	record.DeviceAddress = 0;
	record.BufferDeviceAddress = 0;
	record.AccelerationStructureType = VK_ACCELERATION_STRUCTURE_TYPE_MAX_ENUM_KHR;
	record.IsPartitionedAccelerationStructure = false;
	record.Allocation = nullptr;
	record.ResourceKind = VulkanGpuAllocationResourceKind::Unknown;
}

void VulkanGpuMemoryAllocator::DestroyMemoryBlock(VulkanGpuMemoryBlockRecord& record) noexcept
{
	if (m_impl == nullptr || m_impl->Allocator == nullptr || record.Allocation == nullptr)
	{
		return;
	}

	vmaFreeMemory(m_impl->Allocator, record.Allocation);
	record.Allocation = nullptr;
	record.AllocatedBytes = 0;
	record.MemoryTypeIndex = UINT32_MAX;
	record.MemoryHeapIndex = UINT32_MAX;
}

void VulkanGpuMemoryAllocator::UnmapAllocation(VulkanGpuAllocationRecord& record) noexcept
{
	if (m_impl == nullptr || m_impl->Allocator == nullptr || record.Allocation == nullptr)
	{
		return;
	}

	vmaUnmapMemory(m_impl->Allocator, record.Allocation);
	record.IsMapped = false;
	record.CpuMappedAddress = nullptr;
}

void VulkanGpuMemoryAllocator::SetAllocationDebugName(VulkanGpuAllocationRecord& record, std::wstring_view debugName) noexcept
{
	if (m_impl == nullptr || m_impl->Allocator == nullptr)
	{
		return;
	}

	const std::string narrowName = Strings::ToNarrow(debugName);
	if (record.OwnsAllocation && record.Allocation != nullptr)
	{
		vmaSetAllocationName(m_impl->Allocator, record.Allocation, narrowName.empty() ? nullptr : narrowName.c_str());
	}

	PFN_vkSetDebugUtilsObjectNameEXT setObjectName = m_rhi.GetSetDebugUtilsObjectName();
	if (setObjectName == nullptr || narrowName.empty())
	{
		return;
	}

	if (record.Buffer != VK_NULL_HANDLE)
	{
		(void)VulkanDebugNames::SetObjectName(setObjectName, m_rhi.GetDevice(), VK_OBJECT_TYPE_BUFFER, reinterpret_cast<std::uint64_t>(record.Buffer), narrowName);
	}
	if (record.Image != VK_NULL_HANDLE)
	{
		(void)VulkanDebugNames::SetObjectName(setObjectName, m_rhi.GetDevice(), VK_OBJECT_TYPE_IMAGE, reinterpret_cast<std::uint64_t>(record.Image), narrowName);
	}
	if (record.AccelerationStructure != VK_NULL_HANDLE)
	{
		(void)VulkanDebugNames::SetObjectName(
		    setObjectName,
		    m_rhi.GetDevice(),
		    VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR,
		    reinterpret_cast<std::uint64_t>(record.AccelerationStructure),
		    narrowName);
	}
}

void VulkanGpuMemoryAllocator::SetMemoryBlockDebugName(VulkanGpuMemoryBlockRecord& record, std::wstring_view debugName) noexcept
{
	if (m_impl == nullptr || m_impl->Allocator == nullptr || record.Allocation == nullptr)
	{
		return;
	}

	const std::string narrowName = Strings::ToNarrow(debugName);
	vmaSetAllocationName(m_impl->Allocator, record.Allocation, narrowName.empty() ? nullptr : narrowName.c_str());
}

void VulkanGpuMemoryAllocator::RegisterAllocationRecord(VulkanGpuAllocationRecord& record) noexcept
{
	m_owner.AssertAccess();
	if (m_impl == nullptr)
	{
		return;
	}

	std::scoped_lock lock(m_impl->RecordsMutex);
	if (std::find(m_impl->LiveRecords.begin(), m_impl->LiveRecords.end(), &record) == m_impl->LiveRecords.end())
	{
		m_impl->LiveRecords.push_back(&record);
	}
}

void VulkanGpuMemoryAllocator::UnregisterAllocationRecord(VulkanGpuAllocationRecord& record) noexcept
{
	m_owner.AssertAccess();
	if (m_impl == nullptr)
	{
		return;
	}

	std::scoped_lock lock(m_impl->RecordsMutex);
	auto eraseBegin = std::remove(m_impl->LiveRecords.begin(), m_impl->LiveRecords.end(), &record);
	m_impl->LiveRecords.erase(eraseBegin, m_impl->LiveRecords.end());
}

void VulkanGpuMemoryAllocator::RegisterMemoryBlockRecord(VulkanGpuMemoryBlockRecord& record) noexcept
{
	m_owner.AssertAccess();
	if (m_impl == nullptr)
	{
		return;
	}

	std::scoped_lock lock(m_impl->RecordsMutex);
	if (std::find(m_impl->LiveMemoryBlockRecords.begin(), m_impl->LiveMemoryBlockRecords.end(), &record) == m_impl->LiveMemoryBlockRecords.end())
	{
		m_impl->LiveMemoryBlockRecords.push_back(&record);
	}
}

void VulkanGpuMemoryAllocator::UnregisterMemoryBlockRecord(VulkanGpuMemoryBlockRecord& record) noexcept
{
	m_owner.AssertAccess();
	if (m_impl == nullptr)
	{
		return;
	}

	std::scoped_lock lock(m_impl->RecordsMutex);
	auto eraseBegin = std::remove(m_impl->LiveMemoryBlockRecords.begin(), m_impl->LiveMemoryBlockRecords.end(), &record);
	m_impl->LiveMemoryBlockRecords.erase(eraseBegin, m_impl->LiveMemoryBlockRecords.end());
}

bool VulkanGpuMemoryAllocator::EnsureMemoryBlockAllocationForImage(
    VulkanGpuMemoryBlockRecord& memoryBlock,
    const VkImageCreateInfo& imageCreateInfo) noexcept
{
	if (m_impl == nullptr || m_impl->Allocator == nullptr)
	{
		return false;
	}
	if (memoryBlock.Allocation != nullptr)
	{
		return true;
	}

	VkImage image = VK_NULL_HANDLE;
	const VkResult imageResult = vkCreateImage(m_rhi.GetDevice(), &imageCreateInfo, nullptr, &image);
	if (!VulkanResult::Succeeded(imageResult) || image == VK_NULL_HANDLE)
	{
		return false;
	}

	VkMemoryRequirements memoryRequirements = {};
	vkGetImageMemoryRequirements(m_rhi.GetDevice(), image, &memoryRequirements);
	memoryRequirements.size = std::max(memoryRequirements.size, static_cast<VkDeviceSize>(memoryBlock.RequestedSizeInBytes));
	memoryRequirements.alignment = std::max(memoryRequirements.alignment, static_cast<VkDeviceSize>(memoryBlock.Alignment));
	const VmaAllocationCreateInfo allocationCreateInfo{
	    .flags = VMA_ALLOCATION_CREATE_CAN_ALIAS_BIT,
	    .preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT};
	const VkResult allocationResult =
	    vmaAllocateMemory(m_impl->Allocator, &memoryRequirements, &allocationCreateInfo, &memoryBlock.Allocation, nullptr);
	vkDestroyImage(m_rhi.GetDevice(), image, nullptr);
	if (!VulkanResult::Succeeded(allocationResult) || memoryBlock.Allocation == nullptr)
	{
		memoryBlock.Allocation = nullptr;
		return false;
	}

	VmaAllocationInfo allocationInfo = {};
	vmaGetAllocationInfo(m_impl->Allocator, memoryBlock.Allocation, &allocationInfo);
	memoryBlock.AllocatedBytes = allocationInfo.size;
	memoryBlock.MemoryTypeIndex = allocationInfo.memoryType;
	memoryBlock.MemoryHeapIndex = ResolveMemoryHeapIndex(m_impl->MemoryProperties, allocationInfo.memoryType);
	SetMemoryBlockDebugName(memoryBlock, memoryBlock.DebugName);
	return true;
}

bool VulkanGpuMemoryAllocator::EnsureMemoryBlockAllocationForBuffer(
    VulkanGpuMemoryBlockRecord& memoryBlock,
    const VkBufferCreateInfo& bufferCreateInfo) noexcept
{
	if (m_impl == nullptr || m_impl->Allocator == nullptr)
	{
		return false;
	}
	if (memoryBlock.Allocation != nullptr)
	{
		return true;
	}

	VkBuffer buffer = VK_NULL_HANDLE;
	const VkResult bufferResult = vkCreateBuffer(m_rhi.GetDevice(), &bufferCreateInfo, nullptr, &buffer);
	if (!VulkanResult::Succeeded(bufferResult) || buffer == VK_NULL_HANDLE)
	{
		return false;
	}

	VkMemoryRequirements memoryRequirements = {};
	vkGetBufferMemoryRequirements(m_rhi.GetDevice(), buffer, &memoryRequirements);
	memoryRequirements.size = std::max(memoryRequirements.size, static_cast<VkDeviceSize>(memoryBlock.RequestedSizeInBytes));
	memoryRequirements.alignment = std::max(memoryRequirements.alignment, static_cast<VkDeviceSize>(memoryBlock.Alignment));
	const VmaAllocationCreateInfo allocationCreateInfo{
	    .flags = VMA_ALLOCATION_CREATE_CAN_ALIAS_BIT,
	    .preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT};
	const VkResult allocationResult =
	    vmaAllocateMemory(m_impl->Allocator, &memoryRequirements, &allocationCreateInfo, &memoryBlock.Allocation, nullptr);
	vkDestroyBuffer(m_rhi.GetDevice(), buffer, nullptr);
	if (!VulkanResult::Succeeded(allocationResult) || memoryBlock.Allocation == nullptr)
	{
		memoryBlock.Allocation = nullptr;
		return false;
	}

	VmaAllocationInfo allocationInfo = {};
	vmaGetAllocationInfo(m_impl->Allocator, memoryBlock.Allocation, &allocationInfo);
	memoryBlock.AllocatedBytes = allocationInfo.size;
	memoryBlock.MemoryTypeIndex = allocationInfo.memoryType;
	memoryBlock.MemoryHeapIndex = ResolveMemoryHeapIndex(m_impl->MemoryProperties, allocationInfo.memoryType);
	SetMemoryBlockDebugName(memoryBlock, memoryBlock.DebugName);
	return true;
}

std::unique_ptr<VulkanGpuAllocationRecord> VulkanGpuMemoryAllocator::CreateAllocationRecord(
    VulkanGpuAllocationResourceKind resourceKind,
    VkBuffer buffer,
    VkImage image,
    VmaAllocation allocation,
    RhiMemoryCategory category,
    RhiMemoryResidencyClass residencyClass,
    std::wstring_view debugName) noexcept
{
	VmaAllocationInfo allocationInfo = {};
	vmaGetAllocationInfo(m_impl->Allocator, allocation, &allocationInfo);

	auto record = std::make_unique<VulkanGpuAllocationRecord>();
	record->Buffer = buffer;
	record->Image = image;
	record->Allocation = allocation;
	record->ResourceKind = resourceKind;
	record->Category = category;
	record->ResidencyClass = residencyClass;
	record->UsedBytes = allocationInfo.size;
	record->AllocatedBytes = allocationInfo.size;
	if (resourceKind == VulkanGpuAllocationResourceKind::Buffer)
	{
		VkBufferMemoryRequirementsInfo2 requirementsInfo{.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2, .pNext = nullptr, .buffer = buffer};
		VkMemoryRequirements2 memoryRequirements{.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2};
		vkGetBufferMemoryRequirements2(m_rhi.GetDevice(), &requirementsInfo, &memoryRequirements);
		record->ResourceSizeInBytes = memoryRequirements.memoryRequirements.size;
	}
	if (resourceKind == VulkanGpuAllocationResourceKind::Image)
	{
		VkImageMemoryRequirementsInfo2 requirementsInfo{.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2, .pNext = nullptr, .image = image};
		VkMemoryRequirements2 memoryRequirements{.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2};
		vkGetImageMemoryRequirements2(m_rhi.GetDevice(), &requirementsInfo, &memoryRequirements);
		record->ResourceSizeInBytes = memoryRequirements.memoryRequirements.size;
	}
	record->MemoryTypeIndex = allocationInfo.memoryType;
	record->MemoryHeapIndex = ResolveMemoryHeapIndex(m_impl->MemoryProperties, allocationInfo.memoryType);
	if (resourceKind == VulkanGpuAllocationResourceKind::Image)
	{
		record->AspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}
	record->Owner = this;
	record->DebugName = std::wstring(debugName);
	SetAllocationDebugName(*record, debugName);
	RegisterAllocationRecord(*record);
	return record;
}

std::unique_ptr<VulkanGpuAllocationRecord> VulkanGpuMemoryAllocator::CreateAliasingAllocationRecord(
    VulkanGpuAllocationResourceKind resourceKind,
    VkBuffer buffer,
    VkImage image,
    VulkanGpuMemoryBlockRecord& memoryBlock,
    std::wstring_view debugName) noexcept
{
	if (memoryBlock.Allocation == nullptr)
	{
		return {};
	}

	auto record = std::make_unique<VulkanGpuAllocationRecord>();
	record->Buffer = buffer;
	record->Image = image;
	record->Allocation = memoryBlock.Allocation;
	record->ParentMemoryBlock = &memoryBlock;
	record->ResourceKind = resourceKind;
	record->Category = memoryBlock.Category;
	record->ResidencyClass = memoryBlock.ResidencyClass;
	record->AllocatedBytes = 0;
	record->MemoryTypeIndex = memoryBlock.MemoryTypeIndex;
	record->MemoryHeapIndex = memoryBlock.MemoryHeapIndex;
	record->OwnsAllocation = false;

	if (resourceKind == VulkanGpuAllocationResourceKind::Buffer && buffer != VK_NULL_HANDLE)
	{
		VkBufferMemoryRequirementsInfo2 requirementsInfo{.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2, .pNext = nullptr, .buffer = buffer};
		VkMemoryRequirements2 memoryRequirements{.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2};
		vkGetBufferMemoryRequirements2(m_rhi.GetDevice(), &requirementsInfo, &memoryRequirements);
		record->ResourceSizeInBytes = memoryRequirements.memoryRequirements.size;
		record->UsedBytes = memoryRequirements.memoryRequirements.size;
	}
	if (resourceKind == VulkanGpuAllocationResourceKind::Image && image != VK_NULL_HANDLE)
	{
		VkImageMemoryRequirementsInfo2 requirementsInfo{.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2, .pNext = nullptr, .image = image};
		VkMemoryRequirements2 memoryRequirements{.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2};
		vkGetImageMemoryRequirements2(m_rhi.GetDevice(), &requirementsInfo, &memoryRequirements);
		record->ResourceSizeInBytes = memoryRequirements.memoryRequirements.size;
		record->UsedBytes = memoryRequirements.memoryRequirements.size;
		record->AspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	record->Owner = this;
	record->DebugName = std::wstring(debugName);
	++memoryBlock.AliasingResourceCount;
	SetAllocationDebugName(*record, debugName);
	RegisterAllocationRecord(*record);
	return record;
}

std::uint32_t VulkanGpuMemoryAllocator::ResolveMemoryHeapIndex(
    const VkPhysicalDeviceMemoryProperties& memoryProperties,
    std::uint32_t memoryTypeIndex) noexcept
{
	return memoryTypeIndex < memoryProperties.memoryTypeCount ? memoryProperties.memoryTypes[memoryTypeIndex].heapIndex : UINT32_MAX;
}

VkImageAspectFlags VulkanGpuMemoryAllocator::ResolveImageAspectMask(VkFormat format) noexcept
{
	switch (format)
	{
		case VK_FORMAT_D16_UNORM:
		case VK_FORMAT_D32_SFLOAT:
			return VK_IMAGE_ASPECT_DEPTH_BIT;
		case VK_FORMAT_D24_UNORM_S8_UINT:
		case VK_FORMAT_D32_SFLOAT_S8_UINT:
			return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		default:
			return VK_IMAGE_ASPECT_COLOR_BIT;
	}
}

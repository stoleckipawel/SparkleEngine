#include "Vulkan/VulkanPCH.h"

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include "Vulkan/Memory/VulkanGpuMemoryAllocator.h"

#include "Core/Public/Environment/EnvironmentVariables.h"
#include "Core/Public/Strings/StringUtils.h"
#include "Vulkan/Core/VulkanResult.h"
#include "Vulkan/Device/VulkanRhi.h"
#include "Vulkan/Diagnostics/VulkanDebugNames.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <vector>

static const auto g_vulkanMemoryLogger = Logging::GetOrCreateLogger("RHI.Vulkan.Memory");

struct VulkanGpuMemoryAllocator::PendingAllocationRelease final
{
	std::unique_ptr<VulkanGpuAllocationRecord> Record;
	std::uint64_t RetireFenceValue = 0;
};

struct VulkanGpuMemoryAllocator::Impl final
{
	VmaAllocator Allocator = nullptr;
	VkPhysicalDeviceMemoryProperties MemoryProperties = {};
	mutable std::mutex RecordsMutex;
	std::vector<VulkanGpuAllocationRecord*> LiveRecords;
	std::vector<PendingAllocationRelease> PendingReleases;

	~Impl() noexcept
	{
		PendingReleases.clear();
		if (Allocator != nullptr)
		{
			vmaDestroyAllocator(Allocator);
			Allocator = nullptr;
		}
	}
};

namespace
{
	struct CategoryAggregation final
	{
		RhiMemoryCategoryStats Stats;
		std::vector<std::uint32_t> UniqueHeapIndices;
	};

	VmaMemoryUsage ToVmaMemoryUsage(RhiMemoryResidencyClass residencyClass) noexcept
	{
		switch (residencyClass)
		{
			case RhiMemoryResidencyClass::HostUpload:
				return VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
			case RhiMemoryResidencyClass::HostReadback:
				return VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
			case RhiMemoryResidencyClass::DeviceLocal:
			case RhiMemoryResidencyClass::Transient:
			default:
				return VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
		}
	}

	VmaAllocationCreateFlags ToVmaAllocationFlags(RhiMemoryResidencyClass residencyClass) noexcept
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

	CategoryAggregation& FindOrCreateAggregation(
	    std::vector<CategoryAggregation>& aggregations,
	    const VulkanGpuAllocationRecord& record,
	    const std::array<VmaBudget, VK_MAX_MEMORY_HEAPS>& heapBudgets)
	{
		auto existing = std::find_if(
		    aggregations.begin(),
		    aggregations.end(),
		    [&record](const CategoryAggregation& aggregation)
		    {
			    return aggregation.Stats.Category == record.Category && aggregation.Stats.ResidencyClass == record.ResidencyClass;
		    });
		if (existing != aggregations.end())
		{
			return *existing;
		}

		CategoryAggregation aggregation;
		aggregation.Stats.Category = record.Category;
		aggregation.Stats.ResidencyClass = record.ResidencyClass;
		aggregations.push_back(std::move(aggregation));
		return aggregations.back();
	}

	void AddHeapReference(
	    CategoryAggregation& aggregation,
	    std::uint32_t heapIndex,
	    const std::array<VmaBudget, VK_MAX_MEMORY_HEAPS>& heapBudgets) noexcept
	{
		if (heapIndex == UINT32_MAX || heapIndex >= heapBudgets.size())
		{
			return;
		}

		if (std::find(aggregation.UniqueHeapIndices.begin(), aggregation.UniqueHeapIndices.end(), heapIndex) == aggregation.UniqueHeapIndices.end())
		{
			aggregation.UniqueHeapIndices.push_back(heapIndex);
			++aggregation.Stats.BlockCount;
			aggregation.Stats.BudgetBytes += heapBudgets[heapIndex].budget;
		}
	}
}

VulkanGpuMemoryAllocator::VulkanGpuMemoryAllocator(VulkanRhi& rhi) noexcept : m_rhi(rhi), m_impl(std::make_unique<Impl>())
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
	const VmaAllocatorCreateFlags allocatorFlags = memoryBudgetExtensionEnabled ? VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT : 0;
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

	if (Environment::GetFlag("SPARKLE_RHI_MEMORY_DIAGNOSTICS"))
	{
		SPDLOG_LOGGER_INFO(g_vulkanMemoryLogger, "VMA allocator initialized");
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

bool VulkanGpuMemoryAllocator::SupportsJsonDump() const noexcept
{
	return IsInitialized();
}

RhiMemoryUsageSnapshot VulkanGpuMemoryAllocator::CreateMemoryUsageSnapshot() const
{
	RhiMemoryUsageSnapshot snapshot;
	if (m_impl == nullptr || m_impl->Allocator == nullptr)
	{
		return snapshot;
	}

	std::array<VmaBudget, VK_MAX_MEMORY_HEAPS> heapBudgets = {};
	vmaGetHeapBudgets(m_impl->Allocator, heapBudgets.data());

	VmaTotalStatistics totalStats = {};
	vmaCalculateStatistics(m_impl->Allocator, &totalStats);
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
		snapshot.Allocations.reserve(m_impl->LiveRecords.size());
		aggregations.reserve(m_impl->LiveRecords.size());

		for (const VulkanGpuAllocationRecord* record : m_impl->LiveRecords)
		{
			if (record == nullptr || record->Allocation == nullptr)
			{
				continue;
			}

			CategoryAggregation& aggregation = FindOrCreateAggregation(aggregations, *record, heapBudgets);
			++aggregation.Stats.AllocationCount;
			++aggregation.Stats.ResourceCount;
			aggregation.Stats.UsedBytes += record->UsedBytes;
			aggregation.Stats.AllocatedBytes += record->AllocatedBytes;
			AddHeapReference(aggregation, record->MemoryHeapIndex, heapBudgets);

			snapshot.Allocations.push_back(RhiMemoryAllocationInfo{
			    .Category = record->Category,
			    .ResidencyClass = record->ResidencyClass,
			    .UsedBytes = record->UsedBytes,
			    .AllocatedBytes = record->AllocatedBytes,
			    .DebugName = record->DebugName});
		}
	}

	snapshot.CategoryStats.reserve(aggregations.size());
	for (const CategoryAggregation& aggregation : aggregations)
	{
		snapshot.CategoryStats.push_back(aggregation.Stats);
	}

	return snapshot;
}

bool VulkanGpuMemoryAllocator::WriteAllocatorJsonDump(const std::filesystem::path& outputPath, bool includeDetailedMap) const noexcept
{
	if (m_impl == nullptr || m_impl->Allocator == nullptr || outputPath.empty())
	{
		return false;
	}

	char* statsString = nullptr;
	vmaBuildStatsString(m_impl->Allocator, &statsString, includeDetailedMap ? VK_TRUE : VK_FALSE);
	if (statsString == nullptr)
	{
		return false;
	}

	std::error_code directoryError;
	const std::filesystem::path parentPath = outputPath.parent_path();
	if (!parentPath.empty())
	{
		std::filesystem::create_directories(parentPath, directoryError);
		if (directoryError)
		{
			vmaFreeStatsString(m_impl->Allocator, statsString);
			return false;
		}
	}

	std::ofstream output(outputPath, std::ios::binary | std::ios::trunc);
	if (!output.is_open())
	{
		vmaFreeStatsString(m_impl->Allocator, statsString);
		return false;
	}

	output << statsString;
	const bool succeeded = output.good();
	vmaFreeStatsString(m_impl->Allocator, statsString);
	return succeeded;
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

	const VmaAllocationCreateInfo allocationCreateInfo{.flags = ToVmaAllocationFlags(residencyClass), .usage = ToVmaMemoryUsage(residencyClass)};
	VkBuffer buffer = VK_NULL_HANDLE;
	VmaAllocation allocation = nullptr;
	const VkResult result = vmaCreateBuffer(m_impl->Allocator, &bufferCreateInfo, &allocationCreateInfo, &buffer, &allocation, nullptr);
	if (!VulkanResult::Succeeded(result) || buffer == VK_NULL_HANDLE || allocation == nullptr)
	{
		return {};
	}

	std::unique_ptr<VulkanGpuAllocationRecord> record =
	    CreateAllocationRecord(VulkanGpuAllocationResourceKind::Buffer, buffer, VK_NULL_HANDLE, allocation, category, residencyClass, debugName);
	if (record != nullptr)
	{
		record->ResourceSizeInBytes = bufferCreateInfo.size;
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

	const VmaAllocationCreateInfo allocationCreateInfo{.flags = ToVmaAllocationFlags(residencyClass), .usage = ToVmaMemoryUsage(residencyClass)};
	VkImage image = VK_NULL_HANDLE;
	VmaAllocation allocation = nullptr;
	const VkResult result = vmaCreateImage(m_impl->Allocator, &imageCreateInfo, &allocationCreateInfo, &image, &allocation, nullptr);
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
	}
	return record;
}

bool VulkanGpuMemoryAllocator::WriteAllocation(VulkanGpuAllocationRecord& record, const void* data, std::size_t sizeInBytes) noexcept
{
	if (m_impl == nullptr || m_impl->Allocator == nullptr || record.Allocation == nullptr || data == nullptr || sizeInBytes == 0)
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
	std::memcpy(mappedData, data, sizeInBytes);
	(void)vmaFlushAllocation(m_impl->Allocator, record.Allocation, 0, sizeInBytes);
	vmaUnmapMemory(m_impl->Allocator, record.Allocation);
	record.IsMapped = false;
	record.CpuMappedAddress = nullptr;
	return true;
}

VulkanGpuAllocationRecord* VulkanGpuMemoryAllocator::FindAllocationRecord(NativeResourceHandle resource) const noexcept
{
	if (m_impl == nullptr || !resource)
	{
		return nullptr;
	}

	std::scoped_lock lock(m_impl->RecordsMutex);
	for (VulkanGpuAllocationRecord* record : m_impl->LiveRecords)
	{
		if (record == nullptr)
		{
			continue;
		}

		if (GetVulkanNativeResource(*record).Value == resource.Value)
		{
			return record;
		}
	}

	return nullptr;
}

void VulkanGpuMemoryAllocator::QueueDestroyResource(std::unique_ptr<VulkanGpuAllocationRecord> record, std::uint64_t retireFenceValue) noexcept
{
	if (m_impl == nullptr || record == nullptr)
	{
		return;
	}

	std::scoped_lock lock(m_impl->RecordsMutex);
	m_impl->PendingReleases.push_back(PendingAllocationRelease{.Record = std::move(record), .RetireFenceValue = retireFenceValue});
}

void VulkanGpuMemoryAllocator::DrainCompletedReleases(std::uint64_t completedFenceValue) noexcept
{
	if (m_impl == nullptr)
	{
		return;
	}

	std::vector<std::unique_ptr<VulkanGpuAllocationRecord>> readyReleases;
	{
		std::scoped_lock lock(m_impl->RecordsMutex);
		auto pending = m_impl->PendingReleases.begin();
		while (pending != m_impl->PendingReleases.end())
		{
			if (pending->Record == nullptr || pending->RetireFenceValue <= completedFenceValue)
			{
				readyReleases.push_back(std::move(pending->Record));
				pending = m_impl->PendingReleases.erase(pending);
			}
			else
			{
				++pending;
			}
		}
	}
}

void VulkanGpuMemoryAllocator::FlushPendingReleases() noexcept
{
	DrainCompletedReleases(UINT64_MAX);
}

void VulkanGpuMemoryAllocator::DestroyAllocation(VulkanGpuAllocationRecord& record) noexcept
{
	if (m_impl == nullptr || m_impl->Allocator == nullptr || record.Allocation == nullptr)
	{
		return;
	}

	switch (record.ResourceKind)
	{
		case VulkanGpuAllocationResourceKind::Buffer:
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
	record.Allocation = nullptr;
	record.ResourceKind = VulkanGpuAllocationResourceKind::Unknown;
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
	vmaSetAllocationName(m_impl->Allocator, record.Allocation, narrowName.empty() ? nullptr : narrowName.c_str());

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
}

void VulkanGpuMemoryAllocator::RegisterAllocationRecord(VulkanGpuAllocationRecord& record) noexcept
{
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
	if (m_impl == nullptr)
	{
		return;
	}

	std::scoped_lock lock(m_impl->RecordsMutex);
	auto eraseBegin = std::remove(m_impl->LiveRecords.begin(), m_impl->LiveRecords.end(), &record);
	m_impl->LiveRecords.erase(eraseBegin, m_impl->LiveRecords.end());
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
#pragma once

#include "Interop/RhiNativeHandles.h"
#include "Memory/RhiMemoryTypes.h"
#include "Commands/RhiQueue.h"
#include "Vulkan/VulkanIncludes.h"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

struct VmaAllocation_T;

class VulkanGpuMemoryAllocator;
struct VulkanGpuMemoryBlockRecord;

enum class VulkanGpuAllocationResourceKind : std::uint8_t
{
	Unknown,
	Buffer,
	Image,
};

struct VulkanGpuAllocationRecord final
{
	VkBuffer Buffer = VK_NULL_HANDLE;
	VkImage Image = VK_NULL_HANDLE;
	VkAccelerationStructureKHR AccelerationStructure = VK_NULL_HANDLE;
	VkDeviceAddress DeviceAddress = 0;
	VkDeviceAddress BufferDeviceAddress = 0;
	VkAccelerationStructureTypeKHR AccelerationStructureType = VK_ACCELERATION_STRUCTURE_TYPE_MAX_ENUM_KHR;
	bool IsPartitionedAccelerationStructure = false;
	VmaAllocation_T* Allocation = nullptr;
	VulkanGpuMemoryBlockRecord* ParentMemoryBlock = nullptr;
	VulkanGpuAllocationResourceKind ResourceKind = VulkanGpuAllocationResourceKind::Unknown;
	RhiMemoryCategory Category = RhiMemoryCategory::Other;
	RhiMemoryResidencyClass ResidencyClass = RhiMemoryResidencyClass::DeviceLocal;
	std::uint64_t UsedBytes = 0;
	std::uint64_t AllocatedBytes = 0;
	std::uint64_t ResourceSizeInBytes = 0;
	std::uint32_t MemoryTypeIndex = UINT32_MAX;
	std::uint32_t MemoryHeapIndex = UINT32_MAX;
	VkFormat Format = VK_FORMAT_UNDEFINED;
	VkExtent3D Extent = {};
	VkImageAspectFlags AspectMask = 0;
	VkImageCreateFlags ImageFlags = 0;
	VkFlags Usage = 0;
	std::wstring DebugName;
	VulkanGpuMemoryAllocator* Owner = nullptr;
	bool IsMapped = false;
	void* CpuMappedAddress = nullptr;
	bool OwnsAllocation = true;
	RhiSubmissionState LastUse;
	std::uint32_t RecordingReferenceCount = 0;

	VulkanGpuAllocationRecord() noexcept = default;
	~VulkanGpuAllocationRecord() noexcept;

	VulkanGpuAllocationRecord(const VulkanGpuAllocationRecord&) = delete;
	VulkanGpuAllocationRecord& operator=(const VulkanGpuAllocationRecord&) = delete;
	VulkanGpuAllocationRecord(VulkanGpuAllocationRecord&&) = delete;
	VulkanGpuAllocationRecord& operator=(VulkanGpuAllocationRecord&&) = delete;
};

struct VulkanGpuMemoryBlockRecord final
{
	VmaAllocation_T* Allocation = nullptr;
	RhiMemoryCategory Category = RhiMemoryCategory::TransientResource;
	RhiMemoryResidencyClass ResidencyClass = RhiMemoryResidencyClass::Transient;
	std::uint64_t UsedBytes = 0;
	std::uint64_t AllocatedBytes = 0;
	std::uint64_t RequestedSizeInBytes = 0;
	std::uint64_t Alignment = 0;
	std::uint32_t MemoryTypeIndex = UINT32_MAX;
	std::uint32_t MemoryHeapIndex = UINT32_MAX;
	std::wstring DebugName;
	VulkanGpuMemoryAllocator* Owner = nullptr;
	std::uint32_t AliasingResourceCount = 0;
	RhiSubmissionState LastUse;
	std::uint32_t RecordingReferenceCount = 0;

	VulkanGpuMemoryBlockRecord() noexcept = default;
	~VulkanGpuMemoryBlockRecord() noexcept;

	VulkanGpuMemoryBlockRecord(const VulkanGpuMemoryBlockRecord&) = delete;
	VulkanGpuMemoryBlockRecord& operator=(const VulkanGpuMemoryBlockRecord&) = delete;
	VulkanGpuMemoryBlockRecord(VulkanGpuMemoryBlockRecord&&) = delete;
	VulkanGpuMemoryBlockRecord& operator=(VulkanGpuMemoryBlockRecord&&) = delete;
};

RhiOwnedResourceHandle MakeVulkanOwnedResourceHandle(std::unique_ptr<VulkanGpuAllocationRecord> record) noexcept;
std::unique_ptr<VulkanGpuAllocationRecord> TakeVulkanOwnedResourceHandle(RhiOwnedResourceHandle handle) noexcept;
VulkanGpuAllocationRecord* GetVulkanGpuAllocationRecord(RhiOwnedResourceHandle handle) noexcept;
NativeResourceHandle GetVulkanNativeResource(VulkanGpuAllocationRecord& record) noexcept;
void SetVulkanAllocationRecordDebugName(VulkanGpuAllocationRecord& record, std::wstring_view debugName) noexcept;

RhiOwnedMemoryBlockHandle MakeVulkanOwnedMemoryBlockHandle(std::unique_ptr<VulkanGpuMemoryBlockRecord> record) noexcept;
std::unique_ptr<VulkanGpuMemoryBlockRecord> TakeVulkanOwnedMemoryBlockHandle(RhiOwnedMemoryBlockHandle handle) noexcept;
VulkanGpuMemoryBlockRecord* GetVulkanGpuMemoryBlockRecord(RhiOwnedMemoryBlockHandle handle) noexcept;
void SetVulkanMemoryBlockRecordDebugName(VulkanGpuMemoryBlockRecord& record, std::wstring_view debugName) noexcept;

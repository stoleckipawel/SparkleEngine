#pragma once

#include "Interop/RhiNativeHandles.h"
#include "Memory/RhiMemoryTypes.h"
#include "Vulkan/VulkanIncludes.h"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

struct VmaAllocation_T;

class VulkanGpuMemoryAllocator;

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
	VmaAllocation_T* Allocation = nullptr;
	VulkanGpuAllocationResourceKind ResourceKind = VulkanGpuAllocationResourceKind::Unknown;
	RhiMemoryCategory Category = RhiMemoryCategory::Other;
	RhiMemoryResidencyClass ResidencyClass = RhiMemoryResidencyClass::DeviceLocal;
	std::uint64_t UsedBytes = 0;
	std::uint64_t AllocatedBytes = 0;
	std::uint32_t MemoryTypeIndex = UINT32_MAX;
	std::uint32_t MemoryHeapIndex = UINT32_MAX;
	std::wstring DebugName;
	VulkanGpuMemoryAllocator* Owner = nullptr;
	bool IsMapped = false;
	void* CpuMappedAddress = nullptr;

	VulkanGpuAllocationRecord() noexcept = default;
	~VulkanGpuAllocationRecord() noexcept;

	VulkanGpuAllocationRecord(const VulkanGpuAllocationRecord&) = delete;
	VulkanGpuAllocationRecord& operator=(const VulkanGpuAllocationRecord&) = delete;
	VulkanGpuAllocationRecord(VulkanGpuAllocationRecord&&) = delete;
	VulkanGpuAllocationRecord& operator=(VulkanGpuAllocationRecord&&) = delete;
};

RhiOwnedResourceHandle MakeVulkanOwnedResourceHandle(std::unique_ptr<VulkanGpuAllocationRecord> record) noexcept;
std::unique_ptr<VulkanGpuAllocationRecord> TakeVulkanOwnedResourceHandle(RhiOwnedResourceHandle handle) noexcept;
VulkanGpuAllocationRecord* GetVulkanGpuAllocationRecord(RhiOwnedResourceHandle handle) noexcept;
NativeResourceHandle GetVulkanNativeResource(VulkanGpuAllocationRecord& record) noexcept;
void SetVulkanAllocationRecordDebugName(VulkanGpuAllocationRecord& record, std::wstring_view debugName) noexcept;
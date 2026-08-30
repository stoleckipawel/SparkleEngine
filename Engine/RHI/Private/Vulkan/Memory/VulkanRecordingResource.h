#pragma once

#include "Resources/RhiResourceDesc.h"
#include "Vulkan/Memory/VulkanGpuAllocation.h"
#include "Vulkan/VulkanIncludes.h"

#include <cstdint>

struct VulkanRecordingResource final
{
	VkBuffer Buffer = VK_NULL_HANDLE;
	VkImage Image = VK_NULL_HANDLE;
	VkAccelerationStructureKHR AccelerationStructure = VK_NULL_HANDLE;
	VkDeviceAddress DeviceAddress = 0;
	VkDeviceAddress BufferDeviceAddress = 0;
	std::uintptr_t ResourceHandleValue = 0;
	std::uint64_t ResourceSizeInBytes = 0;
	VkFormat Format = VK_FORMAT_UNDEFINED;
	VkExtent3D Extent = {};
	VkImageAspectFlags AspectMask = 0;
	VkFlags Usage = 0;
	VkAccelerationStructureTypeKHR AccelerationStructureType = VK_ACCELERATION_STRUCTURE_TYPE_MAX_ENUM_KHR;
	VulkanGpuAllocationResourceKind ResourceKind = VulkanGpuAllocationResourceKind::Unknown;
	bool IsPartitionedAccelerationStructure = false;
};

class VulkanRecordingResourceUseToken final
{
public:
	constexpr explicit operator bool() const noexcept { return m_value != 0; }

private:
	friend class VulkanRecordingResourceTable;

	std::uintptr_t m_value = 0;
};

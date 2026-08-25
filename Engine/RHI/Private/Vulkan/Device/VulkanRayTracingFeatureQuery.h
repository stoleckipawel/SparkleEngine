#pragma once

#include "Vulkan/VulkanIncludes.h"

struct VulkanRayTracingFeatureStatus final
{
	bool SupportsAccelerationStructureExtension = false;
	bool SupportsRayTracingPipelineExtension = false;
	bool SupportsRayQueryExtension = false;
	bool SupportsDeferredHostOperationsExtension = false;
	bool SupportsBufferDeviceAddressExtension = false;
	bool SupportsPartitionedAccelerationStructureExtension = false;
	bool SupportsBufferDeviceAddressFeature = false;
	bool SupportsAccelerationStructureFeature = false;
	bool SupportsRayTracingPipelineFeature = false;
	bool SupportsRayQueryFeature = false;
	bool SupportsPartitionedAccelerationStructureFeature = false;
	bool EnabledAccelerationStructure = false;
	bool EnabledInlineRayQuery = false;
	bool EnabledRayTracingPipeline = false;
	bool EnabledPartitionedAccelerationStructure = false;
};

namespace VulkanRayTracingFeatureQuery
{
	VulkanRayTracingFeatureStatus Query(VkPhysicalDevice physicalDevice) noexcept;
}

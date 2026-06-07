#pragma once

#include "Vulkan/VulkanIncludes.h"

struct VulkanRayTracingFeatureStatus final
{
	bool SupportsAccelerationStructureExtension = false;
	bool SupportsRayTracingPipelineExtension = false;
	bool SupportsRayQueryExtension = false;
	bool SupportsDeferredHostOperationsExtension = false;
	bool SupportsAccelerationStructureFeature = false;
	bool SupportsRayTracingPipelineFeature = false;
	bool SupportsRayQueryFeature = false;
	bool EnabledBackend = false;
};

class VulkanRayTracingFeatureQuery final
{
  public:
	static VulkanRayTracingFeatureStatus Query(VkPhysicalDevice physicalDevice) noexcept;
};

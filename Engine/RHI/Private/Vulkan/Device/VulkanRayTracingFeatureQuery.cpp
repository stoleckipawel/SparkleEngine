#include "Vulkan/VulkanPCH.h"

#include "Vulkan/Device/VulkanRayTracingFeatureQuery.h"
#include "Vulkan/Core/VulkanResult.h"

#include <algorithm>
#include <string_view>
#include <vector>

namespace
{
	bool IsDeviceExtensionAvailable(VkPhysicalDevice device, const char* extensionName) noexcept
	{
		std::uint32_t extensionCount = 0;
		if (!VulkanResult::Succeeded(vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr)))
		{
			return false;
		}

		std::vector<VkExtensionProperties> extensions(extensionCount);
		if (!VulkanResult::Succeeded(vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, extensions.data())))
		{
			return false;
		}

		return std::any_of(extensions.begin(), extensions.end(), [extensionName](const VkExtensionProperties& extension) noexcept {
			return std::string_view(extension.extensionName) == extensionName;
		});
	}
}  // namespace

VulkanRayTracingFeatureStatus VulkanRayTracingFeatureQuery::Query(VkPhysicalDevice physicalDevice) noexcept
{
	VulkanRayTracingFeatureStatus status{};
	if (physicalDevice == VK_NULL_HANDLE)
	{
		return status;
	}

	status.SupportsAccelerationStructureExtension =
	    IsDeviceExtensionAvailable(physicalDevice, VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
	status.SupportsRayTracingPipelineExtension =
	    IsDeviceExtensionAvailable(physicalDevice, VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
	status.SupportsRayQueryExtension = IsDeviceExtensionAvailable(physicalDevice, VK_KHR_RAY_QUERY_EXTENSION_NAME);
	status.SupportsDeferredHostOperationsExtension =
	    IsDeviceExtensionAvailable(physicalDevice, VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);

	VkPhysicalDeviceFeatures2 features{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
	VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{
	    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR};
	VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelineFeatures{
	    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR};
	VkPhysicalDeviceRayQueryFeaturesKHR rayQueryFeatures{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR};

	void** next = &features.pNext;
	if (status.SupportsAccelerationStructureExtension)
	{
		*next = &accelerationStructureFeatures;
		next = &accelerationStructureFeatures.pNext;
	}
	if (status.SupportsRayTracingPipelineExtension)
	{
		*next = &rayTracingPipelineFeatures;
		next = &rayTracingPipelineFeatures.pNext;
	}
	if (status.SupportsRayQueryExtension)
	{
		*next = &rayQueryFeatures;
	}

	vkGetPhysicalDeviceFeatures2(physicalDevice, &features);
	status.SupportsAccelerationStructureFeature = accelerationStructureFeatures.accelerationStructure == VK_TRUE;
	status.SupportsRayTracingPipelineFeature = rayTracingPipelineFeatures.rayTracingPipeline == VK_TRUE;
	status.SupportsRayQueryFeature = rayQueryFeatures.rayQuery == VK_TRUE;
	status.EnabledBackend = false;
	return status;
}

#include "Vulkan/VulkanPCH.h"

#include "Vulkan/Diagnostics/VulkanDebugNames.h"

bool VulkanDebugNames::SetObjectName(
    PFN_vkSetDebugUtilsObjectNameEXT setObjectName,
    VkDevice device,
    VkObjectType objectType,
    std::uint64_t objectHandle,
    std::string_view name) noexcept
{
	if (setObjectName == nullptr || device == VK_NULL_HANDLE || objectHandle == 0 || name.empty())
	{
		return false;
	}

	const std::string objectName{name};
	VkDebugUtilsObjectNameInfoEXT nameInfo{};
	nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
	nameInfo.objectType = objectType;
	nameInfo.objectHandle = objectHandle;
	nameInfo.pObjectName = objectName.c_str();
	return setObjectName(device, &nameInfo) == VK_SUCCESS;
}

const char* VulkanDebugNames::ObjectTypeToString(VkObjectType objectType) noexcept
{
	switch (objectType)
	{
		case VK_OBJECT_TYPE_INSTANCE:
			return "VkInstance";
		case VK_OBJECT_TYPE_PHYSICAL_DEVICE:
			return "VkPhysicalDevice";
		case VK_OBJECT_TYPE_DEVICE:
			return "VkDevice";
		case VK_OBJECT_TYPE_QUEUE:
			return "VkQueue";
		case VK_OBJECT_TYPE_COMMAND_BUFFER:
			return "VkCommandBuffer";
		case VK_OBJECT_TYPE_DEVICE_MEMORY:
			return "VkDeviceMemory";
		case VK_OBJECT_TYPE_BUFFER:
			return "VkBuffer";
		case VK_OBJECT_TYPE_IMAGE:
			return "VkImage";
		case VK_OBJECT_TYPE_IMAGE_VIEW:
			return "VkImageView";
		case VK_OBJECT_TYPE_SHADER_MODULE:
			return "VkShaderModule";
		case VK_OBJECT_TYPE_PIPELINE_LAYOUT:
			return "VkPipelineLayout";
		case VK_OBJECT_TYPE_RENDER_PASS:
			return "VkRenderPass";
		case VK_OBJECT_TYPE_PIPELINE:
			return "VkPipeline";
		case VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT:
			return "VkDescriptorSetLayout";
		case VK_OBJECT_TYPE_SAMPLER:
			return "VkSampler";
		case VK_OBJECT_TYPE_DESCRIPTOR_POOL:
			return "VkDescriptorPool";
		case VK_OBJECT_TYPE_DESCRIPTOR_SET:
			return "VkDescriptorSet";
		case VK_OBJECT_TYPE_FRAMEBUFFER:
			return "VkFramebuffer";
		case VK_OBJECT_TYPE_COMMAND_POOL:
			return "VkCommandPool";
		case VK_OBJECT_TYPE_SURFACE_KHR:
			return "VkSurfaceKHR";
		case VK_OBJECT_TYPE_SWAPCHAIN_KHR:
			return "VkSwapchainKHR";
		default:
			return "VkObject";
	}
}
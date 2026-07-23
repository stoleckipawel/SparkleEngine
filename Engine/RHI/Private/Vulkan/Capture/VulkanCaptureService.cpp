#include "Vulkan/Capture/VulkanCaptureService.h"

#include "Capture/RhiBmpWriter.h"
#include "Vulkan/Commands/VulkanCommandQueue.h"
#include "Vulkan/Device/VulkanRhi.h"
#include "Vulkan/VulkanTypeConversions.h"

class VulkanCaptureServiceOperations final
{
  public:
	static std::uint32_t FindVulkanMemoryType(
	    VkPhysicalDevice physicalDevice,
	    std::uint32_t typeBits,
	    VkMemoryPropertyFlags requiredFlags) noexcept
	{
		VkPhysicalDeviceMemoryProperties memoryProperties{};
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
		for (std::uint32_t index = 0; index < memoryProperties.memoryTypeCount; ++index)
		{
			const bool typeAvailable = (typeBits & (1u << index)) != 0;
			const bool flagsMatch = (memoryProperties.memoryTypes[index].propertyFlags & requiredFlags) == requiredFlags;
			if (typeAvailable && flagsMatch)
			{
				return index;
			}
		}
		return UINT32_MAX;
	}

	static void RecordCaptureTransition(
	    VkCommandBuffer commandBuffer,
	    VkImage image,
	    ResourceState before,
	    ResourceState after) noexcept
	{
		if (commandBuffer == VK_NULL_HANDLE || image == VK_NULL_HANDLE || before == after)
		{
			return;
		}

		const VulkanResourceStateMapping sourceState = VulkanTypeConversions::ToResourceStateMapping(before);
		const VulkanResourceStateMapping destinationState = VulkanTypeConversions::ToResourceStateMapping(after);
		const VkImageMemoryBarrier2 barrier{
		    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		    .pNext = nullptr,
		    .srcStageMask = sourceState.StageMask,
		    .srcAccessMask = sourceState.AccessMask,
		    .dstStageMask = destinationState.StageMask,
		    .dstAccessMask = destinationState.AccessMask,
		    .oldLayout = sourceState.ImageLayout,
		    .newLayout = destinationState.ImageLayout,
		    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		    .image = image,
		    .subresourceRange = VkImageSubresourceRange{
		        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		        .baseMipLevel = 0,
		        .levelCount = 1,
		        .baseArrayLayer = 0,
		        .layerCount = 1}};
		const VkDependencyInfo dependency{
		    .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		    .pNext = nullptr,
		    .dependencyFlags = 0,
		    .memoryBarrierCount = 0,
		    .pMemoryBarriers = nullptr,
		    .bufferMemoryBarrierCount = 0,
		    .pBufferMemoryBarriers = nullptr,
		    .imageMemoryBarrierCount = 1,
		    .pImageMemoryBarriers = &barrier};
		vkCmdPipelineBarrier2(commandBuffer, &dependency);
	}
};

VulkanCaptureService::VulkanCaptureService(VulkanRhi& rhi) noexcept : m_rhi(&rhi) {}

RhiCaptureResult VulkanCaptureService::CaptureTextureToBmp(const RhiTextureCaptureRequest& request) noexcept
{
	const bool captured = CaptureNativeTextureToBmp(request.Resource, request.Width, request.Height, request.SourceState, request.OutputPath);
	return RhiCaptureResult{
	    .Status = captured ? ERhiCaptureStatus::Succeeded : ERhiCaptureStatus::Failed,
	    .BackendApi = ERhiBackendApi::Vulkan,
	    .FrameId = request.FrameId,
	    .ViewMode = request.ViewMode,
	    .ViewModeName = request.ViewModeName,
	    .ArtifactPath = captured ? request.OutputPath : std::filesystem::path{},
	    .FailureReason = captured ? "" : "Vulkan texture capture failed; verify the resource is a valid VkImage and the output path is writable."};
}

bool VulkanCaptureService::CaptureNativeTextureToBmp(
    RhiResourceHandle resource,
    std::uint32_t width,
    std::uint32_t height,
    ResourceState sourceState,
    const std::filesystem::path& outputPath) noexcept
{
	if (m_rhi == nullptr || resource.Value == nullptr || width == 0 || height == 0)
	{
		return false;
	}

	const VkDevice device = m_rhi->GetDevice();
	const VkPhysicalDevice physicalDevice = m_rhi->GetPhysicalDevice();
	const std::uint32_t queueFamilyIndex = m_rhi->GetGraphicsQueueFamilyIndex();
	const VkImage sourceImage = static_cast<VkImage>(resource.Value);
	if (device == VK_NULL_HANDLE || physicalDevice == VK_NULL_HANDLE || sourceImage == VK_NULL_HANDLE)
	{
		return false;
	}

	const VkDeviceSize bytesPerPixel = sizeof(float) * 4u;
	const VkDeviceSize readbackSize = static_cast<VkDeviceSize>(width) * height * bytesPerPixel;
	const VkBufferCreateInfo bufferInfo{
	    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
	    .pNext = nullptr,
	    .flags = 0,
	    .size = readbackSize,
	    .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
	    .queueFamilyIndexCount = 0,
	    .pQueueFamilyIndices = nullptr};

	VkBuffer readbackBuffer = VK_NULL_HANDLE;
	if (vkCreateBuffer(device, &bufferInfo, nullptr, &readbackBuffer) != VK_SUCCESS || readbackBuffer == VK_NULL_HANDLE)
	{
		return false;
	}

	VkMemoryRequirements memoryRequirements{};
	vkGetBufferMemoryRequirements(device, readbackBuffer, &memoryRequirements);
	const std::uint32_t memoryTypeIndex = VulkanCaptureServiceOperations::FindVulkanMemoryType(
	    physicalDevice,
	    memoryRequirements.memoryTypeBits,
	    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	if (memoryTypeIndex == UINT32_MAX)
	{
		vkDestroyBuffer(device, readbackBuffer, nullptr);
		return false;
	}

	const VkMemoryAllocateInfo allocateInfo{
	    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
	    .pNext = nullptr,
	    .allocationSize = memoryRequirements.size,
	    .memoryTypeIndex = memoryTypeIndex};
	VkDeviceMemory readbackMemory = VK_NULL_HANDLE;
	if (vkAllocateMemory(device, &allocateInfo, nullptr, &readbackMemory) != VK_SUCCESS || readbackMemory == VK_NULL_HANDLE ||
	    vkBindBufferMemory(device, readbackBuffer, readbackMemory, 0) != VK_SUCCESS)
	{
		if (readbackMemory != VK_NULL_HANDLE)
		{
			vkFreeMemory(device, readbackMemory, nullptr);
		}
		vkDestroyBuffer(device, readbackBuffer, nullptr);
		return false;
	}

	const VkCommandPoolCreateInfo poolInfo{
	    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
	    .pNext = nullptr,
	    .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
	    .queueFamilyIndex = queueFamilyIndex};
	VkCommandPool commandPool = VK_NULL_HANDLE;
	if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS || commandPool == VK_NULL_HANDLE)
	{
		vkFreeMemory(device, readbackMemory, nullptr);
		vkDestroyBuffer(device, readbackBuffer, nullptr);
		return false;
	}

	const VkCommandBufferAllocateInfo commandBufferInfo{
	    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
	    .pNext = nullptr,
	    .commandPool = commandPool,
	    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
	    .commandBufferCount = 1};
	VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
	if (vkAllocateCommandBuffers(device, &commandBufferInfo, &commandBuffer) != VK_SUCCESS || commandBuffer == VK_NULL_HANDLE)
	{
		vkDestroyCommandPool(device, commandPool, nullptr);
		vkFreeMemory(device, readbackMemory, nullptr);
		vkDestroyBuffer(device, readbackBuffer, nullptr);
		return false;
	}

	const VkCommandBufferBeginInfo beginInfo{
	    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
	    .pNext = nullptr,
	    .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	    .pInheritanceInfo = nullptr};
	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
	{
		vkDestroyCommandPool(device, commandPool, nullptr);
		vkFreeMemory(device, readbackMemory, nullptr);
		vkDestroyBuffer(device, readbackBuffer, nullptr);
		return false;
	}

	VulkanCaptureServiceOperations::RecordCaptureTransition(commandBuffer, sourceImage, sourceState, ResourceState::CopySource);

	const VkBufferImageCopy copyRegion{
	    .bufferOffset = 0,
	    .bufferRowLength = 0,
	    .bufferImageHeight = 0,
	    .imageSubresource = VkImageSubresourceLayers{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = 0, .baseArrayLayer = 0, .layerCount = 1},
	    .imageOffset = VkOffset3D{.x = 0, .y = 0, .z = 0},
	    .imageExtent = VkExtent3D{.width = width, .height = height, .depth = 1}};
	vkCmdCopyImageToBuffer(commandBuffer, sourceImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, readbackBuffer, 1, &copyRegion);

	VulkanCaptureServiceOperations::RecordCaptureTransition(commandBuffer, sourceImage, ResourceState::CopySource, sourceState);

	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
	{
		vkDestroyCommandPool(device, commandPool, nullptr);
		vkFreeMemory(device, readbackMemory, nullptr);
		vkDestroyBuffer(device, readbackBuffer, nullptr);
		return false;
	}

	VulkanCommandQueue& graphicsQueue = m_rhi->GetCommandQueue(ERhiQueueType::Graphics);
	const RhiSubmissionToken captureToken =
	    graphicsQueue.Submit(VulkanQueueSubmission{.CommandBuffer = commandBuffer});
	if (!captureToken.IsValid())
	{
		vkDestroyCommandPool(device, commandPool, nullptr);
		vkFreeMemory(device, readbackMemory, nullptr);
		vkDestroyBuffer(device, readbackBuffer, nullptr);
		return false;
	}
	graphicsQueue.WaitForSubmission(captureToken.Value);

	void* mappedData = nullptr;
	const bool mapped = vkMapMemory(device, readbackMemory, 0, readbackSize, 0, &mappedData) == VK_SUCCESS && mappedData != nullptr;
	const bool wroteCapture = mapped
	                            ? WriteRhiBmp(
	                                  outputPath,
	                                  static_cast<const std::byte*>(mappedData),
	                                  width,
	                                  height,
	                                  width * static_cast<std::uint32_t>(bytesPerPixel),
	                                  RhiBmpSourceFormat::Rgba32Float)
	                            : false;
	if (mapped)
	{
		vkUnmapMemory(device, readbackMemory);
	}

	vkDestroyCommandPool(device, commandPool, nullptr);
	vkFreeMemory(device, readbackMemory, nullptr);
	vkDestroyBuffer(device, readbackBuffer, nullptr);
	return wroteCapture;
}

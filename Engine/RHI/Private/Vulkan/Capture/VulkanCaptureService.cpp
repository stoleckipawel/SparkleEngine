#include "Vulkan/VulkanPCH.h"

#include "Vulkan/Capture/VulkanCaptureService.h"

#include "Capture/RhiCaptureFormat.h"
#include "Vulkan/Commands/VulkanCommandQueue.h"
#include "Vulkan/Device/VulkanRhi.h"
#include "Vulkan/VulkanTypeConversions.h"

#include <algorithm>
#include <cstring>

class VulkanCaptureCommands final
{
public:
	static std::uint32_t FindMemoryType(
	    VkPhysicalDevice physicalDevice,
	    std::uint32_t typeBits,
	    VkMemoryPropertyFlags requiredFlags) noexcept
	{
		VkPhysicalDeviceMemoryProperties properties{};
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &properties);
		for (std::uint32_t index = 0; index < properties.memoryTypeCount; ++index)
		{
			const bool typeAvailable = (typeBits & (1u << index)) != 0;
			const bool flagsMatch = (properties.memoryTypes[index].propertyFlags & requiredFlags) == requiredFlags;
			if (typeAvailable && flagsMatch)
			{
				return index;
			}
		}
		return UINT32_MAX;
	}

	static void RecordTransition(VkCommandBuffer commandBuffer, VkImage image, ResourceState before, ResourceState after) noexcept
	{
		if (commandBuffer == VK_NULL_HANDLE || image == VK_NULL_HANDLE || before == after)
		{
			return;
		}
		const VulkanResourceStateMapping source = VulkanTypeConversions::ToResourceStateMapping(before);
		const VulkanResourceStateMapping destination = VulkanTypeConversions::ToResourceStateMapping(after);
		const VkImageMemoryBarrier2 barrier{
		    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		    .srcStageMask = source.StageMask,
		    .srcAccessMask = source.AccessMask,
		    .dstStageMask = destination.StageMask,
		    .dstAccessMask = destination.AccessMask,
		    .oldLayout = source.ImageLayout,
		    .newLayout = destination.ImageLayout,
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
		    .imageMemoryBarrierCount = 1,
		    .pImageMemoryBarriers = &barrier};
		vkCmdPipelineBarrier2(commandBuffer, &dependency);
	}
};

struct VulkanCaptureService::PendingReadback final
{
	RhiCaptureTicket Ticket;
	RhiTextureCaptureRequest Request;
	RhiSubmissionToken Submission;
	VkBuffer Buffer = VK_NULL_HANDLE;
	VkDeviceMemory Memory = VK_NULL_HANDLE;
	VkCommandPool CommandPool = VK_NULL_HANDLE;
	VkCommandBuffer CommandBuffer = VK_NULL_HANDLE;
	std::uint64_t ByteCount = 0;
	std::uint32_t RowPitch = 0;
	PixelFormat Format = PixelFormat::Unknown;
	bool Cancelled = false;
};

VulkanCaptureService::VulkanCaptureService(VulkanRhi& rhi) noexcept :
    m_rhi(&rhi)
{
}

VulkanCaptureService::~VulkanCaptureService() noexcept
{
	if (m_rhi != nullptr)
	{
		for (const std::unique_ptr<PendingReadback>& pending : m_pendingReadbacks)
		{
			if (pending && pending->Submission.IsValid())
			{
				m_rhi->GetCommandQueue(ERhiQueueType::Graphics).WaitForSubmission(pending->Submission.Value);
			}
		}
	}
	while (!m_pendingReadbacks.empty())
	{
		ReleasePending(m_pendingReadbacks.size() - 1);
	}
}

RhiCaptureTicket VulkanCaptureService::BeginTextureReadback(const RhiTextureCaptureRequest& request) noexcept
{
	DrainCancelledReadbacks();
	if (m_rhi == nullptr || !request.Resource || request.Width == 0 || request.Height == 0)
	{
		return {};
	}

	const VkDevice device = m_rhi->GetDevice();
	const VkPhysicalDevice physicalDevice = m_rhi->GetPhysicalDevice();
	const VkImage sourceImage = static_cast<VkImage>(request.Resource.Value);
	RhiCaptureFormat captureFormat;
	if (device == VK_NULL_HANDLE || physicalDevice == VK_NULL_HANDLE || sourceImage == VK_NULL_HANDLE
	    || !TryResolveRhiCaptureFormat(request.SourceFormat, captureFormat))
	{
		return {};
	}

	auto pending = std::make_unique<PendingReadback>();
	pending->Ticket = RhiCaptureTicket{m_nextTicket++};
	pending->Request = request;
	pending->Format = request.SourceFormat;
	pending->RowPitch = request.Width * captureFormat.BytesPerPixel;
	pending->ByteCount = static_cast<std::uint64_t>(pending->RowPitch) * request.Height;
	const VkBufferCreateInfo bufferInfo{
	    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
	    .size = pending->ByteCount,
	    .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	    .sharingMode = VK_SHARING_MODE_EXCLUSIVE};
	if (vkCreateBuffer(device, &bufferInfo, nullptr, &pending->Buffer) != VK_SUCCESS || pending->Buffer == VK_NULL_HANDLE)
	{
		return {};
	}

	VkMemoryRequirements requirements{};
	vkGetBufferMemoryRequirements(device, pending->Buffer, &requirements);
	const std::uint32_t memoryType = VulkanCaptureCommands::FindMemoryType(
	    physicalDevice,
	    requirements.memoryTypeBits,
	    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	if (memoryType == UINT32_MAX)
	{
		vkDestroyBuffer(device, pending->Buffer, nullptr);
		return {};
	}
	const VkMemoryAllocateInfo allocation{
	    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
	    .allocationSize = requirements.size,
	    .memoryTypeIndex = memoryType};
	if (vkAllocateMemory(device, &allocation, nullptr, &pending->Memory) != VK_SUCCESS || pending->Memory == VK_NULL_HANDLE
	    || vkBindBufferMemory(device, pending->Buffer, pending->Memory, 0) != VK_SUCCESS)
	{
		if (pending->Memory != VK_NULL_HANDLE)
		{
			vkFreeMemory(device, pending->Memory, nullptr);
		}
		vkDestroyBuffer(device, pending->Buffer, nullptr);
		return {};
	}

	const VkCommandPoolCreateInfo poolInfo{
	    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
	    .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
	    .queueFamilyIndex = m_rhi->GetGraphicsQueueFamilyIndex()};
	if (vkCreateCommandPool(device, &poolInfo, nullptr, &pending->CommandPool) != VK_SUCCESS)
	{
		vkFreeMemory(device, pending->Memory, nullptr);
		vkDestroyBuffer(device, pending->Buffer, nullptr);
		return {};
	}
	const VkCommandBufferAllocateInfo commandBufferInfo{
	    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
	    .commandPool = pending->CommandPool,
	    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
	    .commandBufferCount = 1};
	if (vkAllocateCommandBuffers(device, &commandBufferInfo, &pending->CommandBuffer) != VK_SUCCESS)
	{
		vkDestroyCommandPool(device, pending->CommandPool, nullptr);
		vkFreeMemory(device, pending->Memory, nullptr);
		vkDestroyBuffer(device, pending->Buffer, nullptr);
		return {};
	}
	const VkCommandBufferBeginInfo beginInfo{
	    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
	    .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};
	if (vkBeginCommandBuffer(pending->CommandBuffer, &beginInfo) != VK_SUCCESS)
	{
		vkDestroyCommandPool(device, pending->CommandPool, nullptr);
		vkFreeMemory(device, pending->Memory, nullptr);
		vkDestroyBuffer(device, pending->Buffer, nullptr);
		return {};
	}

	VulkanCaptureCommands::RecordTransition(pending->CommandBuffer, sourceImage, request.SourceState, ResourceState::CopySource);
	const VkBufferImageCopy copyRegion{
	    .imageSubresource =
	        VkImageSubresourceLayers{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = 0, .baseArrayLayer = 0, .layerCount = 1},
	    .imageExtent = VkExtent3D{.width = request.Width, .height = request.Height, .depth = 1}};
	vkCmdCopyImageToBuffer(pending->CommandBuffer, sourceImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, pending->Buffer, 1, &copyRegion);
	VulkanCaptureCommands::RecordTransition(pending->CommandBuffer, sourceImage, ResourceState::CopySource, request.SourceState);
	if (vkEndCommandBuffer(pending->CommandBuffer) != VK_SUCCESS)
	{
		vkDestroyCommandPool(device, pending->CommandPool, nullptr);
		vkFreeMemory(device, pending->Memory, nullptr);
		vkDestroyBuffer(device, pending->Buffer, nullptr);
		return {};
	}
	pending->Submission =
	    m_rhi->GetCommandQueue(ERhiQueueType::Graphics)
	        .Submit(VulkanQueueSubmission{.CommandBuffers = std::span<const VkCommandBuffer>(&pending->CommandBuffer, 1)});
	if (!pending->Submission.IsValid())
	{
		vkDestroyCommandPool(device, pending->CommandPool, nullptr);
		vkFreeMemory(device, pending->Memory, nullptr);
		vkDestroyBuffer(device, pending->Buffer, nullptr);
		return {};
	}

	const RhiCaptureTicket ticket = pending->Ticket;
	m_pendingReadbacks.push_back(std::move(pending));
	return ticket;
}

bool VulkanCaptureService::TryTakeTextureReadback(RhiCaptureTicket ticket, RhiCaptureReadback& readback) noexcept
{
	DrainCancelledReadbacks();
	PendingReadback* pending = FindPending(ticket);
	if (pending == nullptr || m_rhi == nullptr
	    || !m_rhi->GetCommandQueue(ERhiQueueType::Graphics).IsSubmissionComplete(pending->Submission.Value))
	{
		return false;
	}

	void* mappedData = nullptr;
	if (vkMapMemory(m_rhi->GetDevice(), pending->Memory, 0, pending->ByteCount, 0, &mappedData) != VK_SUCCESS || mappedData == nullptr)
	{
		return false;
	}
	readback.Result = RhiCaptureResult{
	    .Status = ERhiCaptureStatus::Succeeded,
	    .BackendApi = ERhiBackendApi::Vulkan,
	    .FrameId = pending->Request.FrameId,
	    .ViewMode = pending->Request.ViewMode,
	    .ViewModeName = pending->Request.ViewModeName,
	    .ArtifactPath = pending->Request.OutputPath};
	readback.Width = pending->Request.Width;
	readback.Height = pending->Request.Height;
	readback.RowPitch = pending->RowPitch;
	readback.Format = pending->Format;
	readback.Pixels.resize(static_cast<std::size_t>(pending->ByteCount));
	std::memcpy(readback.Pixels.data(), mappedData, readback.Pixels.size());
	vkUnmapMemory(m_rhi->GetDevice(), pending->Memory);

	for (std::size_t index = 0; index < m_pendingReadbacks.size(); ++index)
	{
		if (m_pendingReadbacks[index].get() == pending)
		{
			ReleasePending(index);
			break;
		}
	}
	return true;
}

void VulkanCaptureService::CancelTextureReadback(RhiCaptureTicket ticket) noexcept
{
	for (std::size_t index = 0; index < m_pendingReadbacks.size(); ++index)
	{
		const std::unique_ptr<PendingReadback>& pending = m_pendingReadbacks[index];
		if (!pending || pending->Ticket.Value != ticket.Value)
		{
			continue;
		}
		pending->Cancelled = true;
		DrainCancelledReadbacks();
		return;
	}
}

VulkanCaptureService::PendingReadback* VulkanCaptureService::FindPending(RhiCaptureTicket ticket) noexcept
{
	for (const std::unique_ptr<PendingReadback>& pending : m_pendingReadbacks)
	{
		if (pending && pending->Ticket.Value == ticket.Value)
		{
			return pending.get();
		}
	}
	return nullptr;
}

void VulkanCaptureService::DrainCancelledReadbacks() noexcept
{
	if (m_rhi == nullptr)
	{
		return;
	}
	for (std::size_t index = 0; index < m_pendingReadbacks.size();)
	{
		const std::unique_ptr<PendingReadback>& pending = m_pendingReadbacks[index];
		if (!pending || !pending->Cancelled
		    || !m_rhi->GetCommandQueue(ERhiQueueType::Graphics).IsSubmissionComplete(pending->Submission.Value))
		{
			++index;
			continue;
		}
		ReleasePending(index);
	}
}

void VulkanCaptureService::ReleasePending(std::size_t index) noexcept
{
	if (index >= m_pendingReadbacks.size())
	{
		return;
	}
	const VkDevice device = m_rhi != nullptr ? m_rhi->GetDevice() : VK_NULL_HANDLE;
	const std::unique_ptr<PendingReadback>& pending = m_pendingReadbacks[index];
	if (device != VK_NULL_HANDLE && pending)
	{
		if (pending->CommandPool != VK_NULL_HANDLE)
		{
			vkDestroyCommandPool(device, pending->CommandPool, nullptr);
		}
		if (pending->Memory != VK_NULL_HANDLE)
		{
			vkFreeMemory(device, pending->Memory, nullptr);
		}
		if (pending->Buffer != VK_NULL_HANDLE)
		{
			vkDestroyBuffer(device, pending->Buffer, nullptr);
		}
	}
	m_pendingReadbacks.erase(m_pendingReadbacks.begin() + index);
}

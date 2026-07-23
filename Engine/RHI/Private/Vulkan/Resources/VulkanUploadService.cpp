#include "Vulkan/VulkanPCH.h"

#include "Vulkan/Resources/VulkanUploadService.h"

#include "Commands/RenderCommandList.h"
#include "Vulkan/Commands/VulkanCommandContext.h"
#include "Vulkan/Commands/VulkanRenderCommandList.h"
#include "Vulkan/Memory/VulkanGpuAllocation.h"
#include "Vulkan/Memory/VulkanGpuMemoryAllocator.h"
#include "Vulkan/VulkanTypeConversions.h"

#include <cstring>
#include <span>
#include <vector>

class VulkanUploadServiceImplementation final
{
  public:
	static constexpr VkDeviceSize TextureUploadAlignment = 4;

	struct TextureUploadRegion final
	{
		VkDeviceSize Offset = 0;
		std::uint32_t Width = 0;
		std::uint32_t Height = 0;
		std::uint32_t MipLevel = 0;
		std::uint32_t ArrayLayer = 0;
	};

	static VkDeviceSize AlignTextureUploadOffset(VkDeviceSize offset) noexcept
	{
		return (offset + TextureUploadAlignment - 1u) & ~(TextureUploadAlignment - 1u);
	}

	static std::uint64_t CalculateTextureUploadBytes(const RhiTextureUploadDesc& textureUpload) noexcept
	{
		VkDeviceSize offset = 0;
		for (const RhiTextureArraySliceUploadData& arraySlice : textureUpload.ArraySlices)
		{
			for (const RhiTextureMipUploadData& mipLevel : arraySlice.MipLevels)
			{
				offset = AlignTextureUploadOffset(offset);
				offset += mipLevel.Data.size();
			}
		}
		return offset;
	}

	static bool CopyTextureUploadData(
	    const RhiTextureUploadDesc& textureUpload,
	    std::span<std::uint8_t> destination,
	    std::vector<TextureUploadRegion>& regions) noexcept
	{
		VkDeviceSize offset = 0;
		for (std::uint32_t arrayLayer = 0; arrayLayer < textureUpload.ArraySlices.size(); ++arrayLayer)
		{
			const RhiTextureArraySliceUploadData& arraySlice = textureUpload.ArraySlices[arrayLayer];
			for (std::uint32_t mipIndex = 0; mipIndex < arraySlice.MipLevels.size(); ++mipIndex)
			{
				const RhiTextureMipUploadData& mipLevel = arraySlice.MipLevels[mipIndex];
				offset = AlignTextureUploadOffset(offset);
				if (offset + mipLevel.Data.size() > destination.size())
				{
					return false;
				}

				std::memcpy(destination.data() + offset, mipLevel.Data.data(), mipLevel.Data.size());
				regions.push_back(
				    TextureUploadRegion{
				        .Offset = offset,
				        .Width = mipLevel.Width,
				        .Height = mipLevel.Height,
				        .MipLevel = mipIndex,
				        .ArrayLayer = arrayLayer});
				offset += mipLevel.Data.size();
			}
		}
		return regions.size() == textureUpload.GetSubresourceCount();
	}
};

VulkanUploadService::VulkanUploadService(
    VulkanCommandContext& commandContext,
    VulkanGpuMemoryAllocator& memoryAllocator) :
	m_commandContext(&commandContext), m_memoryAllocator(&memoryAllocator), m_uniformAllocator(memoryAllocator)
{
}

void VulkanUploadService::BeginFrame(std::uint32_t frameIndex) noexcept
{
	m_uniformAllocator.BeginFrame(frameIndex);
}

RhiGpuVirtualAddress VulkanUploadService::AllocateUniformConstantBuffer(const void* data, std::uint32_t sizeInBytes)
{
	return m_uniformAllocator.AllocateAndCopy(data, sizeInBytes);
}

bool VulkanUploadService::UploadTexture(
    RenderCommandList& commandList,
    RhiOwnedResourceHandle destination,
    const RhiTextureUploadDesc& textureUpload,
    ResourceState finalState,
    std::wstring_view debugName)
{
	VulkanGpuAllocationRecord* const destinationRecord = GetVulkanGpuAllocationRecord(destination);
	if (m_commandContext == nullptr || m_memoryAllocator == nullptr || destinationRecord == nullptr ||
	    destinationRecord->Image == VK_NULL_HANDLE || !textureUpload.IsValid() || commandList.GetBackendApi() != ERhiBackendApi::Vulkan)
	{
		return false;
	}

	const VkCommandBuffer commandBuffer = static_cast<VulkanRenderCommandList&>(commandList).GetVulkanCommandBuffer();
	if (commandBuffer == VK_NULL_HANDLE || !m_commandContext->IsCommandBufferRecording(commandBuffer))
	{
		return false;
	}

	const std::uint64_t uploadBufferBytes = VulkanUploadServiceImplementation::CalculateTextureUploadBytes(textureUpload);
	const VkBufferCreateInfo uploadBufferCreateInfo{
	    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
	    .pNext = nullptr,
	    .flags = 0,
	    .size = uploadBufferBytes,
	    .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
	    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
	    .queueFamilyIndexCount = 0,
	    .pQueueFamilyIndices = nullptr};
	std::unique_ptr<VulkanGpuAllocationRecord> stagingResource = m_memoryAllocator->CreateBuffer(
	    uploadBufferCreateInfo,
	    RhiMemoryCategory::Upload,
	    RhiMemoryResidencyClass::HostUpload,
	    debugName.empty() ? L"TextureUpload" : debugName);
	if (stagingResource == nullptr || stagingResource->Buffer == VK_NULL_HANDLE)
	{
		return false;
	}

	std::vector<std::uint8_t> uploadBytes(static_cast<std::size_t>(uploadBufferBytes));
	std::vector<VulkanUploadServiceImplementation::TextureUploadRegion> regions;
	regions.reserve(textureUpload.GetSubresourceCount());
	if (!VulkanUploadServiceImplementation::CopyTextureUploadData(textureUpload, uploadBytes, regions) ||
	    !m_memoryAllocator->WriteAllocation(*stagingResource, uploadBytes.data(), uploadBytes.size()))
	{
		return false;
	}

	const VkImageSubresourceRange subresourceRange{
	    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
	    .baseMipLevel = 0,
	    .levelCount = textureUpload.GetMipCount(),
	    .baseArrayLayer = 0,
	    .layerCount = textureUpload.GetArraySize()};
	const VkImageMemoryBarrier2 toTransfer{
	    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
	    .pNext = nullptr,
	    .srcStageMask = VK_PIPELINE_STAGE_2_NONE,
	    .srcAccessMask = 0,
	    .dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
	    .dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
	    .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
	    .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
	    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
	    .image = destinationRecord->Image,
	    .subresourceRange = subresourceRange};
	const VkDependencyInfo transferDependency{
	    .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
	    .pNext = nullptr,
	    .imageMemoryBarrierCount = 1,
	    .pImageMemoryBarriers = &toTransfer};
	vkCmdPipelineBarrier2(commandBuffer, &transferDependency);

	std::vector<VkBufferImageCopy> copyRegions;
	copyRegions.reserve(regions.size());
	for (const VulkanUploadServiceImplementation::TextureUploadRegion& region : regions)
	{
		copyRegions.push_back(
		    VkBufferImageCopy{
		        .bufferOffset = region.Offset,
		        .bufferRowLength = 0,
		        .bufferImageHeight = 0,
		        .imageSubresource = VkImageSubresourceLayers{
		            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		            .mipLevel = region.MipLevel,
		            .baseArrayLayer = region.ArrayLayer,
		            .layerCount = 1},
		        .imageOffset = {},
		        .imageExtent = VkExtent3D{.width = region.Width, .height = region.Height, .depth = 1}});
	}
	vkCmdCopyBufferToImage(
	    commandBuffer,
	    stagingResource->Buffer,
	    destinationRecord->Image,
	    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	    static_cast<std::uint32_t>(copyRegions.size()),
	    copyRegions.data());

	const ResourceState submittedFinalState =
	    commandList.GetQueueType() == ERhiQueueType::Copy ? ResourceState::Common : finalState;
	const VulkanResourceStateMapping finalStateMapping = VulkanTypeConversions::ToResourceStateMapping(submittedFinalState);
	const VkImageMemoryBarrier2 toFinalState{
	    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
	    .pNext = nullptr,
	    .srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
	    .srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
	    .dstStageMask = finalStateMapping.StageMask,
	    .dstAccessMask = finalStateMapping.AccessMask,
	    .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	    .newLayout = finalStateMapping.ImageLayout,
	    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
	    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
	    .image = destinationRecord->Image,
	    .subresourceRange = subresourceRange};
	const VkDependencyInfo finalStateDependency{
	    .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
	    .pNext = nullptr,
	    .imageMemoryBarrierCount = 1,
	    .pImageMemoryBarriers = &toFinalState};
	vkCmdPipelineBarrier2(commandBuffer, &finalStateDependency);

	commandList.TrackResource(RhiResourceHandle{destinationRecord->Image});
	commandList.TrackResource(RhiResourceHandle{stagingResource->Buffer});
	m_memoryAllocator->QueueDestroyResource(std::move(stagingResource));
	return true;
}

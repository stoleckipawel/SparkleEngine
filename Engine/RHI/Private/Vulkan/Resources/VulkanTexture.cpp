#include "Vulkan/VulkanPCH.h"

#include "Vulkan/Resources/VulkanTexture.h"

#include "Vulkan/Core/VulkanResult.h"
#include "Vulkan/Descriptors/VulkanDescriptorAllocator.h"
#include "Vulkan/Descriptors/VulkanDescriptorManager.h"
#include "Vulkan/Device/VulkanRhi.h"
#include "Vulkan/Memory/VulkanGpuAllocation.h"
#include "Vulkan/Memory/VulkanGpuMemoryAllocator.h"
#include "Vulkan/VulkanTypeConversions.h"

#include <algorithm>
#include <cstring>
#include <numeric>
#include <span>
#include <vector>

static const auto g_vulkanTextureLogger = Logging::GetOrCreateLogger("RHI.Vulkan.Texture");

namespace
{
	constexpr VkDeviceSize kVulkanTextureUploadAlignment = 4;

	struct VulkanTextureUploadRegion final
	{
		VkDeviceSize Offset = 0;
		std::uint32_t Width = 0;
		std::uint32_t Height = 0;
		std::uint32_t MipLevel = 0;
		std::uint32_t ArrayLayer = 0;
	};

	VkDeviceSize AlignUploadOffset(VkDeviceSize offset) noexcept
	{
		return (offset + kVulkanTextureUploadAlignment - 1u) & ~(kVulkanTextureUploadAlignment - 1u);
	}

	std::uint64_t CalculatePayloadBytes(const RhiTextureUploadDesc& textureUpload) noexcept
	{
		std::uint64_t byteCount = 0;
		for (const RhiTextureArraySliceUploadData& arraySlice : textureUpload.ArraySlices)
		{
			for (const RhiTextureMipUploadData& mipLevel : arraySlice.MipLevels)
			{
				byteCount += static_cast<std::uint64_t>(mipLevel.Data.size());
			}
		}
		return byteCount;
	}

	std::uint64_t CalculateUploadBufferBytes(const RhiTextureUploadDesc& textureUpload) noexcept
	{
		VkDeviceSize offset = 0;
		for (const RhiTextureArraySliceUploadData& arraySlice : textureUpload.ArraySlices)
		{
			for (const RhiTextureMipUploadData& mipLevel : arraySlice.MipLevels)
			{
				offset = AlignUploadOffset(offset);
				offset += mipLevel.Data.size();
			}
		}
		return offset;
	}

	void CopyUploadPayload(
	    const RhiTextureUploadDesc& textureUpload,
	    std::span<std::uint8_t> destination,
	    std::vector<VulkanTextureUploadRegion>& outRegions) noexcept
	{
		VkDeviceSize offset = 0;
		for (std::uint32_t arrayLayer = 0; arrayLayer < static_cast<std::uint32_t>(textureUpload.ArraySlices.size()); ++arrayLayer)
		{
			const RhiTextureArraySliceUploadData& arraySlice = textureUpload.ArraySlices[arrayLayer];
			for (std::uint32_t mipLevelIndex = 0; mipLevelIndex < static_cast<std::uint32_t>(arraySlice.MipLevels.size()); ++mipLevelIndex)
			{
				const RhiTextureMipUploadData& mipLevel = arraySlice.MipLevels[mipLevelIndex];
				offset = AlignUploadOffset(offset);
				if (offset + mipLevel.Data.size() > destination.size())
				{
					return;
				}

				std::memcpy(destination.data() + offset, mipLevel.Data.data(), mipLevel.Data.size());
				outRegions.push_back(
				    VulkanTextureUploadRegion{
				        .Offset = offset,
				        .Width = mipLevel.Width,
				        .Height = mipLevel.Height,
				        .MipLevel = mipLevelIndex,
				        .ArrayLayer = arrayLayer});
				offset += mipLevel.Data.size();
			}
		}
	}

	VkImageCreateFlags ResolveImageCreateFlags(const RhiTextureUploadDesc& textureUpload) noexcept
	{
		return textureUpload.IsCube() ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;
	}

	VkImageViewType ResolveImageViewType(const RhiTextureUploadDesc& textureUpload) noexcept
	{
		return textureUpload.IsCube() ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D;
	}

	void RecordUploadCommands(
	    VkCommandBuffer commandBuffer,
	    VkImage image,
	    VkBuffer uploadBuffer,
	    const RhiTextureUploadDesc& textureUpload,
	    std::span<const VulkanTextureUploadRegion> regions) noexcept
	{
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
		    .image = image,
		    .subresourceRange = VkImageSubresourceRange{
		        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		        .baseMipLevel = 0,
		        .levelCount = textureUpload.GetMipCount(),
		        .baseArrayLayer = 0,
		        .layerCount = textureUpload.GetArraySize()}};
		const VkDependencyInfo transferDependency{
		    .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		    .pNext = nullptr,
		    .dependencyFlags = 0,
		    .memoryBarrierCount = 0,
		    .pMemoryBarriers = nullptr,
		    .bufferMemoryBarrierCount = 0,
		    .pBufferMemoryBarriers = nullptr,
		    .imageMemoryBarrierCount = 1,
		    .pImageMemoryBarriers = &toTransfer};
		vkCmdPipelineBarrier2(commandBuffer, &transferDependency);

		std::vector<VkBufferImageCopy> copyRegions;
		copyRegions.reserve(regions.size());
		for (const VulkanTextureUploadRegion& region : regions)
		{
			copyRegions.push_back(
			    VkBufferImageCopy{
			        .bufferOffset = region.Offset,
			        .bufferRowLength = 0,
			        .bufferImageHeight = 0,
			        .imageSubresource =
			            VkImageSubresourceLayers{
			                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			                .mipLevel = region.MipLevel,
			                .baseArrayLayer = region.ArrayLayer,
			                .layerCount = 1},
			        .imageOffset = VkOffset3D{},
			        .imageExtent = VkExtent3D{.width = region.Width, .height = region.Height, .depth = 1}});
		}
		vkCmdCopyBufferToImage(
		    commandBuffer,
		    uploadBuffer,
		    image,
		    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		    static_cast<std::uint32_t>(copyRegions.size()),
		    copyRegions.data());

		const VkImageMemoryBarrier2 toShaderRead{
		    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		    .pNext = nullptr,
		    .srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
		    .srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
		    .dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
		    .dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
		    .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		    .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		    .image = image,
		    .subresourceRange = VkImageSubresourceRange{
		        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		        .baseMipLevel = 0,
		        .levelCount = textureUpload.GetMipCount(),
		        .baseArrayLayer = 0,
		        .layerCount = textureUpload.GetArraySize()}};
		const VkDependencyInfo shaderReadDependency{
		    .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		    .pNext = nullptr,
		    .dependencyFlags = 0,
		    .memoryBarrierCount = 0,
		    .pMemoryBarriers = nullptr,
		    .bufferMemoryBarrierCount = 0,
		    .pBufferMemoryBarriers = nullptr,
		    .imageMemoryBarrierCount = 1,
		    .pImageMemoryBarriers = &toShaderRead};
		vkCmdPipelineBarrier2(commandBuffer, &shaderReadDependency);
	}
}

VulkanTexture::VulkanTexture(
    VulkanRhi& rhi,
    VulkanGpuMemoryAllocator& memoryAllocator,
    VulkanDescriptorManager& descriptorManager,
    RhiTextureUploadDesc textureUpload,
    std::wstring_view debugName) :
    m_rhi(rhi), m_memoryAllocator(memoryAllocator), m_descriptorManager(descriptorManager), m_textureUpload(std::move(textureUpload))
{
	if (!m_textureUpload.IsValid())
	{
		Diagnostics::Fail(g_vulkanTextureLogger, __FILE__, __LINE__, "VulkanTexture: runtime texture upload payload is invalid.");
		return;
	}

	CreateImage(debugName);
	UploadImage(debugName);
	CreateShaderResourceView();
}

VulkanTexture::~VulkanTexture() noexcept
{
	if (m_shaderResourceDescriptor)
	{
		m_descriptorManager.GetAllocator().ReleaseRegisteredDescriptor(m_shaderResourceDescriptor);
		m_shaderResourceDescriptor = {};
	}
	if (m_imageView != VK_NULL_HANDLE)
	{
		vkDestroyImageView(m_rhi.GetDevice(), m_imageView, nullptr);
		m_imageView = VK_NULL_HANDLE;
	}
	m_uploadAllocation.reset();
	m_imageAllocation.reset();
}

void VulkanTexture::CreateImage(std::wstring_view debugName)
{
	const VkImageCreateInfo imageCreateInfo{
	    .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
	    .pNext = nullptr,
	    .flags = ResolveImageCreateFlags(m_textureUpload),
	    .imageType = VK_IMAGE_TYPE_2D,
	    .format = VulkanTypeConversions::ToVkFormat(m_textureUpload.Format),
	    .extent = VkExtent3D{.width = m_textureUpload.Width, .height = m_textureUpload.Height, .depth = 1},
	    .mipLevels = m_textureUpload.GetMipCount(),
	    .arrayLayers = m_textureUpload.GetArraySize(),
	    .samples = VK_SAMPLE_COUNT_1_BIT,
	    .tiling = VK_IMAGE_TILING_OPTIMAL,
	    .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
	    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
	    .queueFamilyIndexCount = 0,
	    .pQueueFamilyIndices = nullptr,
	    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED};
	m_imageAllocation = m_memoryAllocator.CreateImage(
	    imageCreateInfo,
	    RhiMemoryCategory::Texture,
	    RhiMemoryResidencyClass::DeviceLocal,
	    debugName.empty() ? L"RHI_VulkanTexture" : debugName);
	if (m_imageAllocation == nullptr || m_imageAllocation->Image == VK_NULL_HANDLE)
	{
		Diagnostics::Fail(g_vulkanTextureLogger, __FILE__, __LINE__, "VulkanTexture: failed to allocate image resource.");
	}
}

void VulkanTexture::UploadImage(std::wstring_view debugName)
{
	if (m_imageAllocation == nullptr || m_imageAllocation->Image == VK_NULL_HANDLE)
	{
		return;
	}

	const std::uint64_t uploadBufferBytes = CalculateUploadBufferBytes(m_textureUpload);
	const VkBufferCreateInfo uploadBufferCreateInfo{
	    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
	    .pNext = nullptr,
	    .flags = 0,
	    .size = uploadBufferBytes,
	    .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
	    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
	    .queueFamilyIndexCount = 0,
	    .pQueueFamilyIndices = nullptr};
	m_uploadAllocation = m_memoryAllocator.CreateBuffer(
	    uploadBufferCreateInfo,
	    RhiMemoryCategory::Upload,
	    RhiMemoryResidencyClass::HostUpload,
	    debugName.empty() ? L"RHI_VulkanTextureUpload" : debugName);
	if (m_uploadAllocation == nullptr || m_uploadAllocation->Buffer == VK_NULL_HANDLE)
	{
		Diagnostics::Fail(g_vulkanTextureLogger, __FILE__, __LINE__, "VulkanTexture: failed to allocate upload buffer.");
		return;
	}

	std::vector<std::uint8_t> uploadBytes(static_cast<std::size_t>(uploadBufferBytes));
	std::vector<VulkanTextureUploadRegion> regions;
	regions.reserve(m_textureUpload.GetSubresourceCount());
	CopyUploadPayload(m_textureUpload, uploadBytes, regions);
	if (regions.size() != m_textureUpload.GetSubresourceCount() ||
	    !m_memoryAllocator.WriteAllocation(*m_uploadAllocation, uploadBytes.data(), uploadBytes.size()))
	{
		Diagnostics::Fail(g_vulkanTextureLogger, __FILE__, __LINE__, "VulkanTexture: failed to populate upload buffer.");
		return;
	}

	const VkCommandPoolCreateInfo poolCreateInfo{
	    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
	    .pNext = nullptr,
	    .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
	    .queueFamilyIndex = m_rhi.GetGraphicsQueueFamilyIndex()};
	VkCommandPool commandPool = VK_NULL_HANDLE;
	if (!VulkanResult::Succeeded(vkCreateCommandPool(m_rhi.GetDevice(), &poolCreateInfo, nullptr, &commandPool)))
	{
		Diagnostics::Fail(g_vulkanTextureLogger, __FILE__, __LINE__, "VulkanTexture: failed to create upload command pool.");
		return;
	}

	const VkCommandBufferAllocateInfo allocateInfo{
	    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
	    .pNext = nullptr,
	    .commandPool = commandPool,
	    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
	    .commandBufferCount = 1};
	VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
	const VkResult allocateResult = vkAllocateCommandBuffers(m_rhi.GetDevice(), &allocateInfo, &commandBuffer);
	if (!VulkanResult::Succeeded(allocateResult))
	{
		vkDestroyCommandPool(m_rhi.GetDevice(), commandPool, nullptr);
		Diagnostics::Fail(
		    g_vulkanTextureLogger,
		    __FILE__,
		    __LINE__,
		    VulkanResult::FormatFailure("vkAllocateCommandBuffers", allocateResult));
		return;
	}

	const VkCommandBufferBeginInfo beginInfo{
	    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
	    .pNext = nullptr,
	    .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	    .pInheritanceInfo = nullptr};
	const VkResult beginResult = vkBeginCommandBuffer(commandBuffer, &beginInfo);
	if (VulkanResult::Succeeded(beginResult))
	{
		RecordUploadCommands(commandBuffer, m_imageAllocation->Image, m_uploadAllocation->Buffer, m_textureUpload, regions);
		const VkResult endResult = vkEndCommandBuffer(commandBuffer);
		if (VulkanResult::Succeeded(endResult))
		{
			const VkSubmitInfo submitInfo{
			    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			    .pNext = nullptr,
			    .waitSemaphoreCount = 0,
			    .pWaitSemaphores = nullptr,
			    .pWaitDstStageMask = nullptr,
			    .commandBufferCount = 1,
			    .pCommandBuffers = &commandBuffer,
			    .signalSemaphoreCount = 0,
			    .pSignalSemaphores = nullptr};
			const VkResult submitResult = vkQueueSubmit(m_rhi.GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
			if (VulkanResult::Succeeded(submitResult))
			{
				(void) vkQueueWaitIdle(m_rhi.GetGraphicsQueue());
			}
			else
			{
				Diagnostics::Fail(g_vulkanTextureLogger, __FILE__, __LINE__, VulkanResult::FormatFailure("vkQueueSubmit", submitResult));
			}
		}
		else
		{
			Diagnostics::Fail(g_vulkanTextureLogger, __FILE__, __LINE__, VulkanResult::FormatFailure("vkEndCommandBuffer", endResult));
		}
	}
	else
	{
		Diagnostics::Fail(g_vulkanTextureLogger, __FILE__, __LINE__, VulkanResult::FormatFailure("vkBeginCommandBuffer", beginResult));
	}

	vkFreeCommandBuffers(m_rhi.GetDevice(), commandPool, 1, &commandBuffer);
	vkDestroyCommandPool(m_rhi.GetDevice(), commandPool, nullptr);
}

void VulkanTexture::CreateShaderResourceView()
{
	if (m_imageAllocation == nullptr || m_imageAllocation->Image == VK_NULL_HANDLE)
	{
		return;
	}

	const VkImageViewCreateInfo createInfo = BuildImageViewCreateInfo();
	const VkResult result = vkCreateImageView(m_rhi.GetDevice(), &createInfo, nullptr, &m_imageView);
	if (!VulkanResult::Succeeded(result) || m_imageView == VK_NULL_HANDLE)
	{
		Diagnostics::Fail(g_vulkanTextureLogger, __FILE__, __LINE__, VulkanResult::FormatFailure("vkCreateImageView", result));
		return;
	}

	m_shaderResourceDescriptor =
	    m_descriptorManager.GetAllocator().RegisterImageDescriptor(ERhiResourceViewKind::TextureShaderResource, m_imageView);
}

VkImageViewCreateInfo VulkanTexture::BuildImageViewCreateInfo() const noexcept
{
	return VkImageViewCreateInfo{
	    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
	    .pNext = nullptr,
	    .flags = 0,
	    .image = m_imageAllocation != nullptr ? m_imageAllocation->Image : VK_NULL_HANDLE,
	    .viewType = ResolveImageViewType(m_textureUpload),
	    .format = VulkanTypeConversions::ToVkFormat(m_textureUpload.Format),
	    .components =
	        VkComponentMapping{
	            .r = VK_COMPONENT_SWIZZLE_IDENTITY,
	            .g = VK_COMPONENT_SWIZZLE_IDENTITY,
	            .b = VK_COMPONENT_SWIZZLE_IDENTITY,
	            .a = VK_COMPONENT_SWIZZLE_IDENTITY},
	    .subresourceRange = VkImageSubresourceRange{
	        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
	        .baseMipLevel = 0,
	        .levelCount = m_textureUpload.GetMipCount(),
	        .baseArrayLayer = 0,
	        .layerCount = m_textureUpload.GetArraySize()}};
}

void VulkanTexture::WriteShaderResourceView(RhiCpuDescriptorHandle destination) const
{
	m_descriptorManager.GetAllocator().WriteImageDescriptor(destination, ERhiResourceViewKind::TextureShaderResource, m_imageView);
}

NativeResourceHandle VulkanTexture::GetNativeResource() const noexcept
{
	return NativeResourceHandle{m_imageAllocation != nullptr ? m_imageAllocation->Image : VK_NULL_HANDLE};
}

TextureRuntimeInfo VulkanTexture::GetRuntimeInfo() const noexcept
{
	TextureRuntimeInfo info;
	info.Width = m_textureUpload.Width;
	info.Height = m_textureUpload.Height;
	info.ArraySize = m_textureUpload.GetArraySize();
	info.Dimension = m_textureUpload.Dimension;
	info.Format = m_textureUpload.Format;
	info.FormatName = PixelFormatName(m_textureUpload.Format);
	info.FormatIntent = m_textureUpload.FormatIntent;
	info.MipCount = m_textureUpload.GetMipCount();
	info.EstimatedByteSize = CalculatePayloadBytes(m_textureUpload);
	info.GpuShaderResourceViewId = m_shaderResourceDescriptor.Value;
	info.IsValid = m_textureUpload.IsValid() && m_imageAllocation != nullptr && m_imageAllocation->Image != VK_NULL_HANDLE &&
	               m_imageView != VK_NULL_HANDLE && m_shaderResourceDescriptor;
	return info;
}

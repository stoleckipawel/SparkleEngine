#include "Vulkan/VulkanPCH.h"

#include "Vulkan/VulkanTypeConversions.h"

VkFormat VulkanTypeConversions::ToVkFormat(PixelFormat format) noexcept
{
	switch (format)
	{
		case PixelFormat::R8G8B8A8_UNorm:
			return VK_FORMAT_R8G8B8A8_UNORM;
		case PixelFormat::B8G8R8A8_UNorm:
			return VK_FORMAT_B8G8R8A8_UNORM;
		case PixelFormat::R16G16B16A16_Float:
			return VK_FORMAT_R16G16B16A16_SFLOAT;
		case PixelFormat::D24_UNorm_S8_UInt:
			return VK_FORMAT_D24_UNORM_S8_UINT;
		case PixelFormat::R32_Float:
			return VK_FORMAT_R32_SFLOAT;
		case PixelFormat::Unknown:
		default:
			return VK_FORMAT_UNDEFINED;
	}
}

PixelFormat VulkanTypeConversions::ToPixelFormat(VkFormat format) noexcept
{
	switch (format)
	{
		case VK_FORMAT_R8G8B8A8_UNORM:
			return PixelFormat::R8G8B8A8_UNorm;
		case VK_FORMAT_B8G8R8A8_UNORM:
			return PixelFormat::B8G8R8A8_UNorm;
		case VK_FORMAT_R16G16B16A16_SFLOAT:
			return PixelFormat::R16G16B16A16_Float;
		case VK_FORMAT_D24_UNORM_S8_UINT:
			return PixelFormat::D24_UNorm_S8_UInt;
		case VK_FORMAT_R32_SFLOAT:
			return PixelFormat::R32_Float;
		default:
			return PixelFormat::Unknown;
	}
}

VkIndexType VulkanTypeConversions::ToVkIndexType(RhiIndexFormat format) noexcept
{
	switch (format)
	{
		case RhiIndexFormat::UInt16:
			return VK_INDEX_TYPE_UINT16;
		case RhiIndexFormat::UInt32:
		default:
			return VK_INDEX_TYPE_UINT32;
	}
}

bool VulkanTypeConversions::IsBufferResourceStateSupported(ResourceState state) noexcept
{
	switch (state)
	{
		case ResourceState::Common:
		case ResourceState::ShaderResource:
		case ResourceState::UnorderedAccess:
		case ResourceState::RayTracingAccelerationStructure:
		case ResourceState::CopySource:
		case ResourceState::CopyDest:
			return true;
		case ResourceState::RenderTarget:
		case ResourceState::DepthWrite:
		case ResourceState::DepthRead:
		case ResourceState::Present:
		case ResourceState::Count:
		default:
			return false;
	}
}

bool VulkanTypeConversions::IsImageResourceStateSupported(ResourceState state) noexcept
{
	switch (state)
	{
		case ResourceState::Common:
		case ResourceState::RenderTarget:
		case ResourceState::DepthWrite:
		case ResourceState::DepthRead:
		case ResourceState::ShaderResource:
		case ResourceState::UnorderedAccess:
		case ResourceState::CopySource:
		case ResourceState::CopyDest:
		case ResourceState::Present:
			return true;
		case ResourceState::RayTracingAccelerationStructure:
		case ResourceState::Count:
		default:
			return false;
	}
}

VulkanResourceStateMapping VulkanTypeConversions::ToResourceStateMapping(ResourceState state) noexcept
{
	switch (state)
	{
		case ResourceState::Common:
			return VulkanResourceStateMapping{
			    .StageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
			    .AccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT,
			    .ImageLayout = VK_IMAGE_LAYOUT_GENERAL};
		case ResourceState::RenderTarget:
			return VulkanResourceStateMapping{
			    .StageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
			    .AccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
			    .ImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
		case ResourceState::DepthWrite:
			return VulkanResourceStateMapping{
			    .StageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
			    .AccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			    .ImageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
		case ResourceState::DepthRead:
			return VulkanResourceStateMapping{
			    .StageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT |
			                 VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
			    .AccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
			    .ImageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL};
		case ResourceState::ShaderResource:
			return VulkanResourceStateMapping{
			    .StageMask = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT |
			                 VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR,
			    .AccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_READ_BIT,
			    .ImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
		case ResourceState::UnorderedAccess:
			return VulkanResourceStateMapping{
			    .StageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT |
			                 VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR,
			    .AccessMask = VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
			    .ImageLayout = VK_IMAGE_LAYOUT_GENERAL};
		case ResourceState::RayTracingAccelerationStructure:
			return VulkanResourceStateMapping{
			    .StageMask = VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR | VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR,
			    .AccessMask = VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR,
			    .ImageLayout = VK_IMAGE_LAYOUT_GENERAL};
		case ResourceState::CopySource:
			return VulkanResourceStateMapping{
			    .StageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
			    .AccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
			    .ImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL};
		case ResourceState::CopyDest:
			return VulkanResourceStateMapping{
			    .StageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
			    .AccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
			    .ImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL};
		case ResourceState::Present:
			return VulkanResourceStateMapping{
			    .StageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
			    .AccessMask = VK_ACCESS_2_MEMORY_READ_BIT,
			    .ImageLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR};
		default:
			return ToResourceStateMapping(ResourceState::Common);
	}
}

VkBufferCreateInfo VulkanTypeConversions::BuildBufferCreateInfo(const RhiBufferResourceDesc& desc, VkBufferUsageFlags extraUsage) noexcept
{
	VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | extraUsage;
	if (desc.AllowUnorderedAccess)
	{
		usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	}
	if (desc.StrideInBytes > 0)
	{
		usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	}

	return VkBufferCreateInfo{
	    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
	    .pNext = nullptr,
	    .flags = 0,
	    .size = desc.SizeInBytes,
	    .usage = usage,
	    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
	    .queueFamilyIndexCount = 0,
	    .pQueueFamilyIndices = nullptr};
}

VkImageCreateInfo VulkanTypeConversions::BuildTextureCreateInfo(const RhiTextureResourceDesc& desc, VkImageUsageFlags extraUsage) noexcept
{
	VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | extraUsage;
	if (desc.AllowRenderTarget)
	{
		usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	}
	if (desc.AllowDepthStencil)
	{
		usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	}
	if (desc.AllowUnorderedAccess)
	{
		usage |= VK_IMAGE_USAGE_STORAGE_BIT;
	}

	return VkImageCreateInfo{
	    .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
	    .pNext = nullptr,
	    .flags = 0,
	    .imageType = VK_IMAGE_TYPE_2D,
	    .format = ToVkFormat(desc.Format),
	    .extent = VkExtent3D{.width = desc.Width, .height = desc.Height, .depth = 1},
	    .mipLevels = desc.MipLevels,
	    .arrayLayers = 1,
	    .samples = VK_SAMPLE_COUNT_1_BIT,
	    .tiling = VK_IMAGE_TILING_OPTIMAL,
	    .usage = usage,
	    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
	    .queueFamilyIndexCount = 0,
	    .pQueueFamilyIndices = nullptr,
	    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED};
}

VkImageAspectFlags VulkanTypeConversions::ResolveAspectMask(PixelFormat format) noexcept
{
	switch (format)
	{
		case PixelFormat::D24_UNorm_S8_UInt:
			return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		default:
			return VK_IMAGE_ASPECT_COLOR_BIT;
	}
}
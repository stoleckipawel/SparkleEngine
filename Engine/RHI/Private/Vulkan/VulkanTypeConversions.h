#pragma once

#include "Formats/PixelFormat.h"
#include "Interop/ResourceState.h"
#include "Resources/RhiResourceDesc.h"
#include "Vulkan/VulkanIncludes.h"

struct VulkanResourceStateMapping final
{
	VkPipelineStageFlags2 StageMask = VK_PIPELINE_STAGE_2_NONE;
	VkAccessFlags2 AccessMask = VK_ACCESS_2_NONE;
	VkImageLayout ImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
};

class VulkanTypeConversions final
{
  public:
	VulkanTypeConversions() = delete;

	static VkFormat ToVkFormat(PixelFormat format) noexcept;
	static PixelFormat ToPixelFormat(VkFormat format) noexcept;
	static VkIndexType ToVkIndexType(RhiIndexFormat format) noexcept;
	static bool IsBufferResourceStateSupported(ResourceState state) noexcept;
	static bool IsImageResourceStateSupported(ResourceState state) noexcept;
	static VulkanResourceStateMapping ToResourceStateMapping(ResourceState state) noexcept;
	static VkBufferCreateInfo BuildBufferCreateInfo(const RhiBufferResourceDesc& desc, VkBufferUsageFlags extraUsage = 0) noexcept;
	static VkImageCreateInfo BuildTextureCreateInfo(const RhiTextureResourceDesc& desc, VkImageUsageFlags extraUsage = 0) noexcept;
	static VkImageAspectFlags ResolveAspectMask(PixelFormat format) noexcept;
};
#pragma once

#include "Vulkan/VulkanIncludes.h"

#include <array>
#include <span>

namespace VulkanVertexLayout
{
	inline constexpr VkVertexInputBindingDescription kStaticMeshBinding{
	    .binding = 0,
	    .stride = 64,
	    .inputRate = VK_VERTEX_INPUT_RATE_VERTEX};

	inline constexpr std::array<VkVertexInputAttributeDescription, 5> kStaticMeshAttributes{
	    VkVertexInputAttributeDescription{.location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = 0},
	    VkVertexInputAttributeDescription{.location = 1, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = 12},
	    VkVertexInputAttributeDescription{.location = 2, .binding = 0, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = 20},
	    VkVertexInputAttributeDescription{.location = 3, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = 36},
	    VkVertexInputAttributeDescription{.location = 4, .binding = 0, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = 48}};

	inline std::span<const VkVertexInputBindingDescription> GetStaticMeshBindings() noexcept
	{
		return std::span<const VkVertexInputBindingDescription>(&kStaticMeshBinding, 1);
	}

	inline std::span<const VkVertexInputAttributeDescription> GetStaticMeshAttributes() noexcept
	{
		return kStaticMeshAttributes;
	}
}  // namespace VulkanVertexLayout
#include "Vulkan/VulkanPCH.h"

#include "Vulkan/Device/VulkanQueueTopology.h"

#include <bit>
#include <limits>

class VulkanQueueSelection final
{
  public:
	static bool SupportsQueueWork(VkQueueFlags flags, ERhiQueueType queueType) noexcept
	{
		switch (queueType)
		{
			case ERhiQueueType::Graphics:
				return (flags & VK_QUEUE_GRAPHICS_BIT) != 0;
			case ERhiQueueType::Compute:
				return (flags & VK_QUEUE_COMPUTE_BIT) != 0;
			case ERhiQueueType::Copy:
				return (flags & (VK_QUEUE_TRANSFER_BIT | VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) != 0;
			default:
				return false;
		}
	}

	static std::uint32_t QueueSpecializationScore(VkQueueFlags flags, ERhiQueueType queueType) noexcept
	{
		if (queueType == ERhiQueueType::Graphics)
		{
			return 0;
		}

		std::uint32_t score = std::popcount(static_cast<std::uint32_t>(flags));
		if (queueType == ERhiQueueType::Compute && (flags & VK_QUEUE_GRAPHICS_BIT) != 0)
		{
			score += 100;
		}
		else if (queueType == ERhiQueueType::Copy)
		{
			if ((flags & VK_QUEUE_GRAPHICS_BIT) != 0)
			{
				score += 100;
			}
			else if ((flags & VK_QUEUE_COMPUTE_BIT) != 0)
			{
				score += 50;
			}
		}
		return score;
	}

	static VulkanQueueLocation SelectUnusedQueue(
		std::span<const VkQueueFamilyProperties> families,
		std::vector<std::uint32_t>& usedQueueCounts,
		ERhiQueueType queueType) noexcept
	{
		std::uint32_t selectedFamily = UINT32_MAX;
		std::uint32_t selectedScore = std::numeric_limits<std::uint32_t>::max();
		for (std::uint32_t familyIndex = 0; familyIndex < families.size(); ++familyIndex)
		{
			const VkQueueFamilyProperties& family = families[familyIndex];
			if (!SupportsQueueWork(family.queueFlags, queueType) || usedQueueCounts[familyIndex] >= family.queueCount)
			{
				continue;
			}

			const std::uint32_t score = QueueSpecializationScore(family.queueFlags, queueType);
			if (score < selectedScore)
			{
				selectedFamily = familyIndex;
				selectedScore = score;
			}
		}

		if (selectedFamily == UINT32_MAX)
		{
			return {};
		}

		return VulkanQueueLocation{
		    .FamilyIndex = selectedFamily,
		    .QueueIndex = usedQueueCounts[selectedFamily]++};
	}

	static VulkanQueueLocation SelectFallbackQueue(
		std::span<const VkQueueFamilyProperties> families,
		ERhiQueueType queueType,
		std::span<const VulkanQueueLocation> selectedLocations) noexcept
	{
		for (auto location = selectedLocations.rbegin(); location != selectedLocations.rend(); ++location)
		{
			if (location->IsValid() && SupportsQueueWork(families[location->FamilyIndex].queueFlags, queueType))
			{
				return *location;
			}
		}
		return {};
	}
};

VulkanQueueTopology VulkanQueueTopology::Select(VkPhysicalDevice physicalDevice)
{
	VulkanQueueTopology topology;
	if (physicalDevice == VK_NULL_HANDLE)
	{
		return topology;
	}

	std::uint32_t familyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &familyCount, nullptr);
	std::vector<VkQueueFamilyProperties> families(familyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &familyCount, families.data());
	std::vector<std::uint32_t> usedQueueCounts(familyCount, 0);

	topology.m_locations[RhiQueueTypeToIndex(ERhiQueueType::Graphics)] =
	    VulkanQueueSelection::SelectUnusedQueue(families, usedQueueCounts, ERhiQueueType::Graphics);
	for (const ERhiQueueType queueType : {ERhiQueueType::Compute, ERhiQueueType::Copy})
	{
		VulkanQueueLocation& location = topology.m_locations[RhiQueueTypeToIndex(queueType)];
		location = VulkanQueueSelection::SelectUnusedQueue(families, usedQueueCounts, queueType);
		if (!location.IsValid())
		{
			location = VulkanQueueSelection::SelectFallbackQueue(families, queueType, topology.m_locations);
		}
	}

	for (std::uint32_t familyIndex = 0; familyIndex < usedQueueCounts.size(); ++familyIndex)
	{
		if (usedQueueCounts[familyIndex] != 0)
		{
			topology.m_familyRequests.push_back(
			    VulkanQueueFamilyRequest{.FamilyIndex = familyIndex, .QueueCount = usedQueueCounts[familyIndex]});
			topology.m_familyIndices.push_back(familyIndex);
		}
	}
	return topology;
}

const VulkanQueueLocation& VulkanQueueTopology::Get(ERhiQueueType queueType) const noexcept
{
	return m_locations[RhiQueueTypeToIndex(queueType)];
}

bool VulkanQueueTopology::Supports(ERhiQueueType queueType) const noexcept
{
	return Get(queueType).IsValid();
}

bool VulkanQueueTopology::HasIndependentQueue(ERhiQueueType queueType) const noexcept
{
	return Supports(queueType) &&
	       (queueType == ERhiQueueType::Graphics || Get(queueType) != Get(ERhiQueueType::Graphics));
}

#pragma once

#include "Commands/RhiQueue.h"
#include "Vulkan/VulkanIncludes.h"

#include <array>
#include <cstdint>
#include <span>
#include <vector>

struct VulkanQueueLocation final
{
	std::uint32_t FamilyIndex = UINT32_MAX;
	std::uint32_t QueueIndex = UINT32_MAX;

	bool IsValid() const noexcept { return FamilyIndex != UINT32_MAX && QueueIndex != UINT32_MAX; }
	bool operator==(const VulkanQueueLocation&) const noexcept = default;
};

struct VulkanQueueFamilyRequest final
{
	std::uint32_t FamilyIndex = UINT32_MAX;
	std::uint32_t QueueCount = 0;
};

class VulkanQueueTopology final
{
public:
	static VulkanQueueTopology Select(VkPhysicalDevice physicalDevice);

	const VulkanQueueLocation& Get(ERhiQueueType queueType) const noexcept;
	bool Supports(ERhiQueueType queueType) const noexcept;
	bool HasIndependentQueue(ERhiQueueType queueType) const noexcept;
	std::span<const VulkanQueueFamilyRequest> GetFamilyRequests() const noexcept { return m_familyRequests; }
	std::span<const std::uint32_t> GetFamilyIndices() const noexcept { return m_familyIndices; }

private:
	std::array<VulkanQueueLocation, RhiQueueTypeCount> m_locations{};
	std::vector<VulkanQueueFamilyRequest> m_familyRequests;
	std::vector<std::uint32_t> m_familyIndices;
};

#include "Vulkan/VulkanPCH.h"

#include "Vulkan/Descriptors/VulkanRecordingDescriptorPool.h"

#include "Vulkan/Core/VulkanResult.h"
#include "Vulkan/Device/VulkanRhi.h"

#include <algorithm>
#include <utility>

SPARKLE_DEFINE_LOG_CATEGORY_STATIC(g_vulkanRecordingDescriptorPoolLogger, "RHI.Vulkan.DescriptorPool");

VulkanRecordingDescriptorPool::VulkanRecordingDescriptorPool(VulkanRhi& rhi) noexcept :
    m_rhi(&rhi)
{
	CreatePool({});
}

VulkanRecordingDescriptorPool::~VulkanRecordingDescriptorPool() noexcept
{
	for (const PoolPage& page : m_pages)
	{
		vkDestroyDescriptorPool(m_rhi->GetDevice(), page.Pool, nullptr);
	}
}

void VulkanRecordingDescriptorPool::Reset() noexcept
{
	// The recording context waits for its submission token before resetting any page.
	for (PoolPage& page : m_pages)
	{
		const VkResult result = vkResetDescriptorPool(m_rhi->GetDevice(), page.Pool, 0);
		if (!VulkanResult::Succeeded(result))
		{
			Diagnostics::Fatal(
			    g_vulkanRecordingDescriptorPoolLogger,
			    __FILE__,
			    __LINE__,
			    VulkanResult::FormatFailure("vkResetDescriptorPool", result));
		}
		page.Remaining = page.Capacity;
		page.AllocatedSets = 0;
	}
	m_allocatedSetCount = 0;
}

bool VulkanRecordingDescriptorPool::CanAllocate(const PoolPage& page, std::span<const VkDescriptorPoolSize> requirements) noexcept
{
	if (page.AllocatedSets >= DescriptorSetCapacity)
	{
		return false;
	}
	for (const VkDescriptorPoolSize& requirement : requirements)
	{
		const auto available = std::ranges::find(page.Remaining, requirement.type, &VkDescriptorPoolSize::type);
		if (available == page.Remaining.end() || available->descriptorCount < requirement.descriptorCount)
		{
			return false;
		}
	}
	return true;
}

VkDescriptorSet VulkanRecordingDescriptorPool::AllocateSet(
    VkDescriptorSetLayout layout,
    std::span<const VkDescriptorPoolSize> requirements) noexcept
{
	if (layout == VK_NULL_HANDLE || m_allocatedSetCount >= DescriptorSetCapacity)
	{
		return VK_NULL_HANDLE;
	}

	for (PoolPage& page : m_pages)
	{
		if (CanAllocate(page, requirements))
		{
			return AllocateSet(page, layout, requirements);
		}
	}

	// Size a new page from the actual layout, never from a shader- or scene-specific array count.
	// An unused page has no set referenced by this recording; prior recordings retired before Reset.
	const auto unusedPage = std::ranges::find(m_pages, 0u, &PoolPage::AllocatedSets);
	if (unusedPage != m_pages.end())
	{
		vkDestroyDescriptorPool(m_rhi->GetDevice(), unusedPage->Pool, nullptr);
		m_pages.erase(unusedPage);
	}
	CreatePool(requirements);
	return AllocateSet(m_pages.back(), layout, requirements);
}

VkDescriptorSet VulkanRecordingDescriptorPool::AllocateSet(
    PoolPage& page,
    VkDescriptorSetLayout layout,
    std::span<const VkDescriptorPoolSize> requirements) noexcept
{
	const VkDescriptorSetAllocateInfo allocateInfo{
	    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
	    .pNext = nullptr,
	    .descriptorPool = page.Pool,
	    .descriptorSetCount = 1,
	    .pSetLayouts = &layout};

	VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
	const VkResult result = vkAllocateDescriptorSets(m_rhi->GetDevice(), &allocateInfo, &descriptorSet);
	if (!VulkanResult::Succeeded(result))
	{
		Diagnostics::Fatal(
		    g_vulkanRecordingDescriptorPoolLogger,
		    __FILE__,
		    __LINE__,
		    VulkanResult::FormatFailure("vkAllocateDescriptorSets", result));
	}
	for (const VkDescriptorPoolSize& requirement : requirements)
	{
		auto available = std::ranges::find(page.Remaining, requirement.type, &VkDescriptorPoolSize::type);
		available->descriptorCount -= requirement.descriptorCount;
	}
	++page.AllocatedSets;
	++m_allocatedSetCount;
	return descriptorSet;
}

void VulkanRecordingDescriptorPool::CreatePool(std::span<const VkDescriptorPoolSize> requirements) noexcept
{
	PoolPage page;
	page.Capacity = {
	    {.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = 1024},
	    {.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = 1024},
	    {.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, .descriptorCount = 1024},
	    {.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .descriptorCount = 512},
	    {.type = VK_DESCRIPTOR_TYPE_SAMPLER, .descriptorCount = 256}};

	if (m_rhi->GetRayTracingCapabilities().SupportsAccelerationStructure)
	{
		page.Capacity.push_back({.type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, .descriptorCount = 128});
	}
	if (m_rhi->GetFeatureStatus().RayTracing.EnabledPartitionedAccelerationStructure)
	{
		page.Capacity.push_back({.type = VK_DESCRIPTOR_TYPE_PARTITIONED_ACCELERATION_STRUCTURE_NV, .descriptorCount = 128});
	}
	for (const VkDescriptorPoolSize& requirement : requirements)
	{
		auto capacity = std::ranges::find(page.Capacity, requirement.type, &VkDescriptorPoolSize::type);
		if (capacity == page.Capacity.end())
		{
			page.Capacity.push_back(requirement);
		}
		else
		{
			capacity->descriptorCount = std::max(capacity->descriptorCount, requirement.descriptorCount);
		}
	}
	page.Remaining = page.Capacity;

	const VkDescriptorPoolCreateInfo createInfo{
	    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
	    .pNext = nullptr,
	    .flags = 0,
	    .maxSets = DescriptorSetCapacity,
	    .poolSizeCount = static_cast<std::uint32_t>(page.Capacity.size()),
	    .pPoolSizes = page.Capacity.data()};

	const VkResult result = vkCreateDescriptorPool(m_rhi->GetDevice(), &createInfo, nullptr, &page.Pool);
	if (!VulkanResult::Succeeded(result))
	{
		Diagnostics::Fatal(
		    g_vulkanRecordingDescriptorPoolLogger,
		    __FILE__,
		    __LINE__,
		    VulkanResult::FormatFailure("vkCreateDescriptorPool", result));
	}
	m_pages.push_back(std::move(page));
}

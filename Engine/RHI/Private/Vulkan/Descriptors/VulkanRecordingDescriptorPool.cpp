#include "Vulkan/VulkanPCH.h"

#include "Vulkan/Descriptors/VulkanRecordingDescriptorPool.h"

#include "Vulkan/Core/VulkanResult.h"
#include "Vulkan/Device/VulkanRhi.h"

#include <array>

VulkanRecordingDescriptorPool::VulkanRecordingDescriptorPool(VulkanRhi& rhi) noexcept : m_rhi(&rhi)
{
	CreatePool();
}

VulkanRecordingDescriptorPool::~VulkanRecordingDescriptorPool() noexcept
{
	if (m_pool != VK_NULL_HANDLE)
	{
		vkDestroyDescriptorPool(m_rhi->GetDevice(), m_pool, nullptr);
		m_pool = VK_NULL_HANDLE;
	}
}

void VulkanRecordingDescriptorPool::Reset() noexcept
{
	if (m_pool == VK_NULL_HANDLE)
	{
		return;
	}

	const VkResult result = vkResetDescriptorPool(m_rhi->GetDevice(), m_pool, 0);
	if (!VulkanResult::Succeeded(result))
	{
		return;
	}

	m_allocatedSetCount = 0;
}

VkDescriptorSet VulkanRecordingDescriptorPool::AllocateSet(VkDescriptorSetLayout layout) noexcept
{
	if (m_pool == VK_NULL_HANDLE || layout == VK_NULL_HANDLE || m_allocatedSetCount >= DescriptorSetCapacity)
	{
		return VK_NULL_HANDLE;
	}

	const VkDescriptorSetAllocateInfo allocateInfo{
	    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
	    .pNext = nullptr,
	    .descriptorPool = m_pool,
	    .descriptorSetCount = 1,
	    .pSetLayouts = &layout};

	VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
	const VkResult result = vkAllocateDescriptorSets(m_rhi->GetDevice(), &allocateInfo, &descriptorSet);
	if (!VulkanResult::Succeeded(result))
	{
		return VK_NULL_HANDLE;
	}

	++m_allocatedSetCount;
	return descriptorSet;
}

void VulkanRecordingDescriptorPool::CreatePool() noexcept
{
	std::array<VkDescriptorPoolSize, 7> poolSizes{
	    VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = 1024},
	    VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = 1024},
	    VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, .descriptorCount = 1024},
	    VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .descriptorCount = 512},
	    VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_SAMPLER, .descriptorCount = 256}};
	std::uint32_t poolSizeCount = 5;

	if (m_rhi->GetRayTracingCapabilities().SupportsRayTracing)
	{
		poolSizes[poolSizeCount++] =
		    VkDescriptorPoolSize{
		        .type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
		        .descriptorCount = 128};
	}
	if (m_rhi->GetFeatureStatus().RayTracing.EnabledPartitionedAccelerationStructure)
	{
		poolSizes[poolSizeCount++] =
		    VkDescriptorPoolSize{
		        .type = VK_DESCRIPTOR_TYPE_PARTITIONED_ACCELERATION_STRUCTURE_NV,
		        .descriptorCount = 128};
	}

	const VkDescriptorPoolCreateInfo createInfo{
	    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
	    .pNext = nullptr,
	    .flags = 0,
	    .maxSets = DescriptorSetCapacity,
	    .poolSizeCount = poolSizeCount,
	    .pPoolSizes = poolSizes.data()};

	const VkResult result = vkCreateDescriptorPool(m_rhi->GetDevice(), &createInfo, nullptr, &m_pool);
	if (!VulkanResult::Succeeded(result))
	{
		m_pool = VK_NULL_HANDLE;
	}
}

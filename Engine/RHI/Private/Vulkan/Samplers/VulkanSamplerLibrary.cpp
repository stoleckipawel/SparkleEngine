#include "Vulkan/VulkanPCH.h"

#include "Vulkan/Samplers/VulkanSamplerLibrary.h"

#include "Vulkan/Core/VulkanResult.h"
#include "Vulkan/Descriptors/VulkanDescriptorManager.h"
#include "Vulkan/Device/VulkanRhi.h"

#include <algorithm>

static const auto g_vulkanSamplerLibraryLogger = Logging::GetOrCreateLogger("RHI.Vulkan.Samplers");

VulkanSamplerLibrary::VulkanSamplerLibrary(VulkanRhi& rhi, VulkanDescriptorManager& descriptorManager) noexcept :
    m_rhi(rhi), m_descriptorManager(descriptorManager)
{
}

VulkanSamplerLibrary::~VulkanSamplerLibrary() noexcept
{
	for (SamplerRecord& samplerRecord : m_samplerRecords)
	{
		if (samplerRecord.Table)
		{
			m_descriptorManager.ReleaseDescriptorTable(samplerRecord.Table);
		}
		if (samplerRecord.Sampler != VK_NULL_HANDLE)
		{
			vkDestroySampler(m_rhi.GetDevice(), samplerRecord.Sampler, nullptr);
		}
	}
	m_samplerRecords.clear();
}

RhiDescriptorTableBinding VulkanSamplerLibrary::GetSharedSamplerBinding(const RhiSamplerDesc& samplerDesc) noexcept
{
	for (const SamplerRecord& samplerRecord : m_samplerRecords)
	{
		if (SamplerDescEquals(samplerRecord.Desc, samplerDesc))
		{
			return RhiDescriptorTableBinding{.Table = samplerRecord.Table, .DescriptorIndex = 0};
		}
	}

	const VkSampler sampler = CreateSampler(samplerDesc);
	if (sampler == VK_NULL_HANDLE)
	{
		return {};
	}

	const RhiDescriptorTableHandle table = m_descriptorManager.AllocateDescriptorTable(ERhiDescriptorAllocatorType::Sampler, 1);
	m_descriptorManager.GetAllocator().WriteSamplerDescriptor(m_descriptorManager.GetDescriptorTableCpuHandle(table), sampler);
	m_samplerRecords.push_back(SamplerRecord{.Desc = samplerDesc, .Sampler = sampler, .Table = table});
	return RhiDescriptorTableBinding{.Table = table, .DescriptorIndex = 0};
}

bool VulkanSamplerLibrary::SamplerDescEquals(const RhiSamplerDesc& lhs, const RhiSamplerDesc& rhs) noexcept
{
	return lhs.MinMagFilter == rhs.MinMagFilter && lhs.MipFilter == rhs.MipFilter && lhs.Address.U == rhs.Address.U &&
	       lhs.Address.V == rhs.Address.V && lhs.Address.W == rhs.Address.W && lhs.MaxAnisotropy == rhs.MaxAnisotropy;
}

VkFilter VulkanSamplerLibrary::ToVkFilter(RhiSamplerMinMagFilter filter) noexcept
{
	switch (filter)
	{
		case RhiSamplerMinMagFilter::Point:
			return VK_FILTER_NEAREST;
		case RhiSamplerMinMagFilter::Linear:
		default:
			return VK_FILTER_LINEAR;
	}
}

VkSamplerMipmapMode VulkanSamplerLibrary::ToVkMipmapMode(RhiSamplerMipFilter filter) noexcept
{
	switch (filter)
	{
		case RhiSamplerMipFilter::Point:
		case RhiSamplerMipFilter::None:
			return VK_SAMPLER_MIPMAP_MODE_NEAREST;
		case RhiSamplerMipFilter::Linear:
		default:
			return VK_SAMPLER_MIPMAP_MODE_LINEAR;
	}
}

VkSamplerAddressMode VulkanSamplerLibrary::ToVkAddressMode(RhiSamplerAddressMode mode) noexcept
{
	switch (mode)
	{
		case RhiSamplerAddressMode::Clamp:
			return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		case RhiSamplerAddressMode::Mirror:
			return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		case RhiSamplerAddressMode::Wrap:
		default:
			return VK_SAMPLER_ADDRESS_MODE_REPEAT;
	}
}

VkSampler VulkanSamplerLibrary::CreateSampler(const RhiSamplerDesc& desc) const
{
	const std::uint32_t maxAnisotropy = static_cast<std::uint32_t>(desc.MaxAnisotropy);
	const bool anisotropyEnabled = maxAnisotropy > 1 && m_rhi.GetFeatureStatus().EnabledSamplerAnisotropy;
	const VkSamplerCreateInfo createInfo{
	    .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
	    .pNext = nullptr,
	    .flags = 0,
	    .magFilter = ToVkFilter(desc.MinMagFilter),
	    .minFilter = ToVkFilter(desc.MinMagFilter),
	    .mipmapMode = ToVkMipmapMode(desc.MipFilter),
	    .addressModeU = ToVkAddressMode(desc.Address.U),
	    .addressModeV = ToVkAddressMode(desc.Address.V),
	    .addressModeW = ToVkAddressMode(desc.Address.W),
	    .mipLodBias = 0.0f,
	    .anisotropyEnable = anisotropyEnabled ? VK_TRUE : VK_FALSE,
	    .maxAnisotropy = static_cast<float>(std::max(1u, maxAnisotropy)),
	    .compareEnable = VK_FALSE,
	    .compareOp = VK_COMPARE_OP_ALWAYS,
	    .minLod = 0.0f,
	    .maxLod = desc.MipFilter == RhiSamplerMipFilter::None ? 0.0f : VK_LOD_CLAMP_NONE,
	    .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
	    .unnormalizedCoordinates = VK_FALSE};

	VkSampler sampler = VK_NULL_HANDLE;
	const VkResult result = vkCreateSampler(m_rhi.GetDevice(), &createInfo, nullptr, &sampler);
	if (!VulkanResult::Succeeded(result))
	{
		Diagnostics::Fail(
		    g_vulkanSamplerLibraryLogger,
		    __FILE__,
		    __LINE__,
		    VulkanResult::FormatFailure("vkCreateSampler", result));
	}
	return sampler;
}

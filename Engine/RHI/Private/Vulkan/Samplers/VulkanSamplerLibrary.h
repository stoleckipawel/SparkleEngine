#pragma once

#include "Device/RenderHardwareInterface.h"
#include "Vulkan/VulkanIncludes.h"

#include <vector>

class VulkanDescriptorManager;
class VulkanRhi;

class VulkanSamplerLibrary final
{
  public:
	VulkanSamplerLibrary(VulkanRhi& rhi, VulkanDescriptorManager& descriptorManager) noexcept;
	~VulkanSamplerLibrary() noexcept;

	VulkanSamplerLibrary(const VulkanSamplerLibrary&) = delete;
	VulkanSamplerLibrary& operator=(const VulkanSamplerLibrary&) = delete;
	VulkanSamplerLibrary(VulkanSamplerLibrary&&) = delete;
	VulkanSamplerLibrary& operator=(VulkanSamplerLibrary&&) = delete;

	RhiDescriptorTableBinding GetSharedSamplerBinding(const RhiSamplerDesc& samplerDesc) noexcept;

  private:
	struct SamplerRecord final
	{
		RhiSamplerDesc Desc = {};
		VkSampler Sampler = VK_NULL_HANDLE;
		RhiDescriptorTableHandle Table = {};
	};

	static bool SamplerDescEquals(const RhiSamplerDesc& lhs, const RhiSamplerDesc& rhs) noexcept;
	static VkFilter ToVkFilter(RhiSamplerMinMagFilter filter) noexcept;
	static VkSamplerMipmapMode ToVkMipmapMode(RhiSamplerMipFilter filter) noexcept;
	static VkSamplerAddressMode ToVkAddressMode(RhiSamplerAddressMode mode) noexcept;

	VkSampler CreateSampler(const RhiSamplerDesc& desc) const;

	VulkanRhi& m_rhi;
	VulkanDescriptorManager& m_descriptorManager;
	std::vector<SamplerRecord> m_samplerRecords;
};
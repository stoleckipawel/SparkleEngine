#pragma once

#include "Memory/RhiMemoryTypes.h"
#include "Resources/RhiResourceDesc.h"

#include <memory>
#include <string_view>

class VulkanGpuMemoryAllocator;

class VulkanTextureFactory final
{
  public:
	explicit VulkanTextureFactory(VulkanGpuMemoryAllocator& memoryAllocator) noexcept;
	~VulkanTextureFactory() noexcept = default;

	VulkanTextureFactory(const VulkanTextureFactory&) = delete;
	VulkanTextureFactory& operator=(const VulkanTextureFactory&) = delete;
	VulkanTextureFactory(VulkanTextureFactory&&) = delete;
	VulkanTextureFactory& operator=(VulkanTextureFactory&&) = delete;

	RhiOwnedResourceHandle CreateTextureResource(
	    const RhiTextureResourceDesc& desc,
	    RhiMemoryCategory category,
	    RhiMemoryResidencyClass residencyClass,
	    std::wstring_view debugName);

  private:
	VulkanGpuMemoryAllocator& m_memoryAllocator;
};
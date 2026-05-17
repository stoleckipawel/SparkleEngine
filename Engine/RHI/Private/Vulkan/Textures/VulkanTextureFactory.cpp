#include "Vulkan/VulkanPCH.h"

#include "Vulkan/Textures/VulkanTextureFactory.h"

#include "Formats/PixelFormat.h"
#include "Vulkan/Memory/VulkanGpuAllocation.h"
#include "Vulkan/Memory/VulkanGpuMemoryAllocator.h"
#include "Vulkan/VulkanTypeConversions.h"

VulkanTextureFactory::VulkanTextureFactory(VulkanGpuMemoryAllocator& memoryAllocator) noexcept : m_memoryAllocator(memoryAllocator) {}

RhiOwnedResourceHandle VulkanTextureFactory::CreateTextureResource(
    const RhiTextureResourceDesc& desc,
    RhiMemoryCategory category,
    RhiMemoryResidencyClass residencyClass,
    std::wstring_view debugName)
{
	if (desc.Width == 0 || desc.Height == 0 || desc.Format == PixelFormat::Unknown)
	{
		return {};
	}

	const VkImageCreateInfo imageCreateInfo = VulkanTypeConversions::BuildTextureCreateInfo(desc);
	std::unique_ptr<VulkanGpuAllocationRecord> record =
	    m_memoryAllocator.CreateImage(imageCreateInfo, category, residencyClass, debugName);
	return record != nullptr ? MakeVulkanOwnedResourceHandle(std::move(record)) : RhiOwnedResourceHandle{};
}
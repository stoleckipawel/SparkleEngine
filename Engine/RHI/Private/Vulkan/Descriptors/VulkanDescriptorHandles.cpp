#include "Vulkan/VulkanPCH.h"

#include "Vulkan/Descriptors/VulkanDescriptorHandles.h"

RhiDescriptorTableHandle VulkanDescriptorHandles::MakeTableHandle(std::uint32_t index) noexcept
{
	return RhiDescriptorTableHandle{index + 1u};
}

RhiGpuDescriptorHandle VulkanDescriptorHandles::MakeGpuDescriptorHandle(std::uint32_t index) noexcept
{
	return RhiGpuDescriptorHandle{GpuDescriptorMagic | static_cast<std::uint64_t>(index + 1u)};
}

RhiCpuDescriptorHandle VulkanDescriptorHandles::MakeCpuDescriptorHandle(std::uint32_t tableIndex, std::uint32_t descriptorIndex) noexcept
{
	return RhiCpuDescriptorHandle{
	    CpuDescriptorMagic | (static_cast<std::uintptr_t>(tableIndex + 1u) << CpuDescriptorTableShift) |
	    static_cast<std::uintptr_t>(descriptorIndex + 1u)};
}

bool VulkanDescriptorHandles::DecodeGpuDescriptorHandle(RhiGpuDescriptorHandle handle, std::uint32_t& outIndex) noexcept
{
	if ((handle.Value & 0xFFFFFFFF00000000ull) != GpuDescriptorMagic)
	{
		return false;
	}
	const std::uint32_t encodedIndex = static_cast<std::uint32_t>(handle.Value & 0xFFFFFFFFull);
	if (encodedIndex == 0)
	{
		return false;
	}
	outIndex = encodedIndex - 1u;
	return true;
}

bool VulkanDescriptorHandles::DecodeCpuDescriptorHandle(
    RhiCpuDescriptorHandle handle,
    std::uint32_t& outTableIndex,
    std::uint32_t& outDescriptorIndex) noexcept
{
	if ((handle.Value & CpuDescriptorMagicMask) != CpuDescriptorMagic)
	{
		return false;
	}
	const std::uint32_t encodedTableIndex =
	    static_cast<std::uint32_t>((handle.Value >> CpuDescriptorTableShift) & CpuDescriptorIndexMask);
	const std::uint32_t encodedDescriptorIndex = static_cast<std::uint32_t>(handle.Value & CpuDescriptorIndexMask);
	if (encodedTableIndex == 0 || encodedDescriptorIndex == 0)
	{
		return false;
	}
	outTableIndex = encodedTableIndex - 1u;
	outDescriptorIndex = encodedDescriptorIndex - 1u;
	return true;
}

RhiCpuDescriptorHandle VulkanDescriptorHandles::MakeImageViewCpuHandle(VkImageView imageView) noexcept
{
	return RhiCpuDescriptorHandle{reinterpret_cast<std::uintptr_t>(imageView)};
}

VkImageView VulkanDescriptorHandles::DecodeImageViewCpuHandle(RhiCpuDescriptorHandle handle) noexcept
{
	return reinterpret_cast<VkImageView>(handle.Value);
}

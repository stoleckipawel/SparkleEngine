#include "Vulkan/VulkanPCH.h"

#include "Vulkan/Descriptors/VulkanDescriptorHandles.h"

RhiDescriptorTableHandle VulkanDescriptorHandles::MakeTableHandle(std::uint32_t index, std::uint16_t generation) noexcept
{
	return RhiDescriptorTableHandle::Make(index, generation);
}

RhiGpuDescriptorHandle VulkanDescriptorHandles::MakeGpuDescriptorHandle(std::uint32_t index) noexcept
{
	return RhiGpuDescriptorHandle{GpuDescriptorMagic | static_cast<std::uint64_t>(index + 1u)};
}

RhiCpuDescriptorHandle VulkanDescriptorHandles::MakeCpuDescriptorHandle(
    RhiDescriptorTableHandle tableHandle,
    std::uint32_t descriptorIndex) noexcept
{
	if (!tableHandle || descriptorIndex >= CpuDescriptorIndexMask)
	{
		return {};
	}

	return RhiCpuDescriptorHandle{
	    CpuDescriptorMagic | (static_cast<std::uintptr_t>(tableHandle.Value) << CpuDescriptorTableShift)
	    | static_cast<std::uintptr_t>(descriptorIndex + 1u)};
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
    RhiDescriptorTableHandle& outTableHandle,
    std::uint32_t& outDescriptorIndex) noexcept
{
	if ((handle.Value & CpuDescriptorMagicMask) != CpuDescriptorMagic)
	{
		return false;
	}
	const std::uint32_t encodedTableHandle = static_cast<std::uint32_t>((handle.Value >> CpuDescriptorTableShift) & CpuDescriptorTableMask);
	const std::uint32_t encodedDescriptorIndex = static_cast<std::uint32_t>(handle.Value & CpuDescriptorIndexMask);
	if (encodedTableHandle == 0 || encodedDescriptorIndex == 0)
	{
		return false;
	}
	outTableHandle.Value = encodedTableHandle;
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

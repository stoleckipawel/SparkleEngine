#pragma once

#include "Descriptors/RhiDescriptorHandles.h"
#include "Vulkan/VulkanIncludes.h"

#include <cstdint>

class VulkanDescriptorHandles final
{
  public:
	static constexpr std::uint32_t MaximumCpuDescriptorCount = 0xFFFFu;

	static RhiDescriptorTableHandle MakeTableHandle(std::uint32_t index, std::uint16_t generation) noexcept;
	static RhiGpuDescriptorHandle MakeGpuDescriptorHandle(std::uint32_t index) noexcept;
	static RhiCpuDescriptorHandle MakeCpuDescriptorHandle(RhiDescriptorTableHandle tableHandle, std::uint32_t descriptorIndex) noexcept;
	static bool DecodeGpuDescriptorHandle(RhiGpuDescriptorHandle handle, std::uint32_t& outIndex) noexcept;
	static bool DecodeCpuDescriptorHandle(
	    RhiCpuDescriptorHandle handle,
	    RhiDescriptorTableHandle& outTableHandle,
	    std::uint32_t& outDescriptorIndex) noexcept;
	static RhiCpuDescriptorHandle MakeImageViewCpuHandle(VkImageView imageView) noexcept;
	static VkImageView DecodeImageViewCpuHandle(RhiCpuDescriptorHandle handle) noexcept;

	VulkanDescriptorHandles() = delete;
	~VulkanDescriptorHandles() = delete;

  private:
	static constexpr std::uint64_t GpuDescriptorMagic = 0x5350564B00000000ull;
	static constexpr std::uintptr_t CpuDescriptorMagic = static_cast<std::uintptr_t>(0x4350000000000000ull);
	static constexpr std::uintptr_t CpuDescriptorMagicMask = static_cast<std::uintptr_t>(0xFFFF000000000000ull);
	static constexpr std::uintptr_t CpuDescriptorIndexMask = static_cast<std::uintptr_t>(0xFFFFull);
	static constexpr std::uintptr_t CpuDescriptorTableMask = static_cast<std::uintptr_t>(0xFFFFFFFFull);
	static constexpr std::uint32_t CpuDescriptorTableShift = 16u;
};

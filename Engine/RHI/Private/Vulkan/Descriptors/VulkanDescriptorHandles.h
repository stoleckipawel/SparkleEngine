#pragma once

#include "Descriptors/RhiDescriptorHandles.h"
#include "Vulkan/VulkanIncludes.h"

#include <cstdint>

class VulkanDescriptorHandles final
{
  public:
	static RhiDescriptorTableHandle MakeTableHandle(std::uint32_t index) noexcept;
	static RhiGpuDescriptorHandle MakeGpuDescriptorHandle(std::uint32_t index) noexcept;
	static RhiCpuDescriptorHandle MakeCpuDescriptorHandle(std::uint32_t tableIndex, std::uint32_t descriptorIndex) noexcept;
	static bool DecodeGpuDescriptorHandle(RhiGpuDescriptorHandle handle, std::uint32_t& outIndex) noexcept;
	static bool DecodeCpuDescriptorHandle(
	    RhiCpuDescriptorHandle handle,
	    std::uint32_t& outTableIndex,
	    std::uint32_t& outDescriptorIndex) noexcept;
	static RhiCpuDescriptorHandle MakeImageViewCpuHandle(VkImageView imageView) noexcept;
	static VkImageView DecodeImageViewCpuHandle(RhiCpuDescriptorHandle handle) noexcept;

	VulkanDescriptorHandles() = delete;
	~VulkanDescriptorHandles() = delete;

  private:
	static constexpr std::uint64_t GpuDescriptorMagic = 0x5350564B00000000ull;
	static constexpr std::uintptr_t CpuDescriptorMagic = static_cast<std::uintptr_t>(0x4350564B00000000ull);
};
#pragma once

#include "../RHIAPI.h"

#include <cstdint>
#include <string_view>

enum class RhiMemoryCategory : std::uint8_t
{
	Texture,
	Mesh,
	RayTracing,
	TransientResource,
	Upload,
	Readback,
	ConstantBuffer,
	Other,
};

enum class RhiMemoryResidencyClass : std::uint8_t
{
	DeviceLocal,
	HostUpload,
	HostReadback,
	Transient,
};

SPARKLE_RHI_API std::string_view RhiMemoryCategoryName(RhiMemoryCategory category) noexcept;
SPARKLE_RHI_API std::string_view RhiMemoryResidencyClassName(RhiMemoryResidencyClass residencyClass) noexcept;

#pragma once

#include <cstdint>
#include <string_view>

enum class RhiMemoryCategory : std::uint8_t
{
	Texture,
	Mesh,
	RayTracing,
	FrameGraphTransient,
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

constexpr std::string_view RhiMemoryCategoryName(RhiMemoryCategory category) noexcept
{
	switch (category)
	{
		case RhiMemoryCategory::Texture:
			return "Texture";
		case RhiMemoryCategory::Mesh:
			return "Mesh";
		case RhiMemoryCategory::RayTracing:
			return "RayTracing";
		case RhiMemoryCategory::FrameGraphTransient:
			return "FrameGraphTransient";
		case RhiMemoryCategory::Upload:
			return "Upload";
		case RhiMemoryCategory::Readback:
			return "Readback";
		case RhiMemoryCategory::ConstantBuffer:
			return "ConstantBuffer";
		case RhiMemoryCategory::Other:
		default:
			return "Other";
	}
}

constexpr std::string_view RhiMemoryResidencyClassName(RhiMemoryResidencyClass residencyClass) noexcept
{
	switch (residencyClass)
	{
		case RhiMemoryResidencyClass::DeviceLocal:
			return "DeviceLocal";
		case RhiMemoryResidencyClass::HostUpload:
			return "HostUpload";
		case RhiMemoryResidencyClass::HostReadback:
			return "HostReadback";
		case RhiMemoryResidencyClass::Transient:
			return "Transient";
		default:
			return "Unknown";
	}
}

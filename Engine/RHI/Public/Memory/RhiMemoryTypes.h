#pragma once

#include <cstdint>

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

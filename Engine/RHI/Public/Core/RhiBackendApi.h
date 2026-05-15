#pragma once

#include <cstdint>

enum class ERhiBackendApi : std::uint8_t
{
	Unknown = 0,
	D3D12 = 1,
	Vulkan = 2,
};

#pragma once

#include <cstdint>

struct ImageProviderGraphKey final
{
	std::uint32_t UpscalerProvider = 0;
	std::uint32_t RayReconstructionMode = 0;

	bool operator==(const ImageProviderGraphKey&) const noexcept = default;
};

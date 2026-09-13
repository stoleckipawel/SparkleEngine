#pragma once

#include <cstdint>

enum class ImageProviderPipeline : std::uint8_t
{
	PresentationUpscaling = 0,
	RayReconstruction = 1,
	NativeResolution = 2,
};

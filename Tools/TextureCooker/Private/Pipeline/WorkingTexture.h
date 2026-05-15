#pragma once

#include "RHI/Private/D3D12/Textures/TextureLoadResult.h"

#include <vector>

namespace TextureCookPipeline
{
	struct WorkingMipLevel
	{
		std::uint32_t width = 1;
		std::uint32_t height = 1;
		std::vector<float> pixels;
	};

	struct WorkingTexture
	{
		TextureResourceDimension dimension = TextureResourceDimension::Texture2D;
		std::uint32_t arraySize = 1;
		bool sourceWasFloat = false;
		std::vector<std::vector<WorkingMipLevel>> arraySlices;
	};
}

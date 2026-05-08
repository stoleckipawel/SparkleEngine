#pragma once

#include "RHI/Public/D3D12/Textures/TextureLoadResult.h"

#include <cstdint>
#include <vector>

namespace TextureCookPipeline
{
	enum class CompressionTarget : std::uint8_t
	{
		None,
		BC1,
		BC4,
		BC5,
		BC6H,
		BC7,
	};

	struct WorkingMipLevel
	{
		std::uint32_t width = 1;
		std::uint32_t height = 1;
		std::vector<float> pixels;
	};

	struct WorkingImage
	{
		TextureResourceDimension dimension = TextureResourceDimension::Texture2D;
		std::uint32_t arraySize = 1;
		bool sourceWasFloat = false;
		std::vector<std::vector<WorkingMipLevel>> arraySlices;
	};
}

#include "PCH.h"

#include "Pipeline/Stages/MipStage.h"

#include "Pipeline/ImageOps.h"

#include "Core/Public/Diagnostics/Error.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image_resize2.h>

#include <algorithm>
#include <cmath>

namespace TextureCookPipeline
{
	static void DownsampleNormalMipLevel(const WorkingMipLevel& sourceMip, WorkingMipLevel& outMip)
	{
		outMip.pixels.resize(static_cast<std::size_t>(outMip.width) * static_cast<std::size_t>(outMip.height) * 4u);

		for (std::uint32_t y = 0; y < outMip.height; ++y)
		{
			for (std::uint32_t x = 0; x < outMip.width; ++x)
			{
				float sumX = 0.0f;
				float sumY = 0.0f;
				float sumZ = 0.0f;
				float sumAlpha = 0.0f;
				std::uint32_t sampleCount = 0;

				for (std::uint32_t sourceY = y * 2u; sourceY < (std::min)(sourceMip.height, (y * 2u) + 2u); ++sourceY)
				{
					for (std::uint32_t sourceX = x * 2u; sourceX < (std::min)(sourceMip.width, (x * 2u) + 2u); ++sourceX)
					{
						const std::size_t sourceOffset = (static_cast<std::size_t>(sourceY) * sourceMip.width + sourceX) * 4u;
						const float normalX = (sourceMip.pixels[sourceOffset + 0u] * 2.0f) - 1.0f;
						const float normalY = (sourceMip.pixels[sourceOffset + 1u] * 2.0f) - 1.0f;
						const float normalZ = (sourceMip.pixels[sourceOffset + 2u] * 2.0f) - 1.0f;
						sumX += normalX;
						sumY += normalY;
						sumZ += normalZ;
						sumAlpha += sourceMip.pixels[sourceOffset + 3u];
						++sampleCount;
					}
				}

				const float length = std::sqrt(sumX * sumX + sumY * sumY + sumZ * sumZ);
				const float invLength = length > 1e-6f ? (1.0f / length) : 0.0f;
				const std::size_t outputOffset = (static_cast<std::size_t>(y) * outMip.width + x) * 4u;
				outMip.pixels[outputOffset + 0u] = ((sumX * invLength) * 0.5f) + 0.5f;
				outMip.pixels[outputOffset + 1u] = ((sumY * invLength) * 0.5f) + 0.5f;
				outMip.pixels[outputOffset + 2u] = ((sumZ * invLength) * 0.5f) + 0.5f;
				outMip.pixels[outputOffset + 3u] = sampleCount > 0 ? (sumAlpha / static_cast<float>(sampleCount)) : 1.0f;
			}
		}
	}

	static void ResizeFloatMipLevel(
	    TextureMipFilter mipFilter,
	    const WorkingMipLevel& sourceMip,
	    WorkingMipLevel& outMip)
	{
		outMip.pixels.resize(static_cast<std::size_t>(outMip.width) * static_cast<std::size_t>(outMip.height) * 4u);

		STBIR_RESIZE resize{};
		stbir_resize_init(
		    &resize,
		    sourceMip.pixels.data(),
		    static_cast<int>(sourceMip.width),
		    static_cast<int>(sourceMip.height),
		    static_cast<int>(sourceMip.width * 4u * sizeof(float)),
		    outMip.pixels.data(),
		    static_cast<int>(outMip.width),
		    static_cast<int>(outMip.height),
		    static_cast<int>(outMip.width * 4u * sizeof(float)),
		    STBIR_4CHANNEL,
		    STBIR_TYPE_FLOAT);
		stbir_set_edgemodes(&resize, STBIR_EDGE_CLAMP, STBIR_EDGE_CLAMP);

		if (mipFilter == TextureMipFilter::Kaiser)
		{
			stbir_set_filter_callbacks(&resize, &KaiserKernel, &KaiserSupport, &KaiserKernel, &KaiserSupport);
		}
		else
		{
			stbir_set_filters(&resize, STBIR_FILTER_BOX, STBIR_FILTER_BOX);
		}

		if (!stbir_resize_extended(&resize))
		{
			throw Diagnostics::Error("stb_image_resize2 failed to generate the next mip level.");
		}
	}

	static WorkingMipLevel GenerateNextMip(
	    TextureMipFilter mipFilter,
	    const WorkingMipLevel& sourceMip)
	{
		WorkingMipLevel mip;
		mip.width = (std::max)(1u, sourceMip.width >> 1u);
		mip.height = (std::max)(1u, sourceMip.height >> 1u);

		if (mipFilter == TextureMipFilter::NormalAware)
		{
			DownsampleNormalMipLevel(sourceMip, mip);
			return mip;
		}

		ResizeFloatMipLevel(mipFilter, sourceMip, mip);
		return mip;
	}

	static void GenerateMipChain(const TextureCookRequest& request, WorkingTexture& workingTexture)
	{
		for (auto& arraySlice : workingTexture.arraySlices)
		{
			if (arraySlice.empty())
			{
				throw Diagnostics::Error("Working texture slice is empty during mip generation.");
			}

			WorkingMipLevel currentMip = arraySlice.front();
			arraySlice.clear();
			arraySlice.push_back(currentMip);

			while (currentMip.width > 1u || currentMip.height > 1u)
			{
				WorkingMipLevel nextMip = GenerateNextMip(request.policy.mipFilter, currentMip);
				arraySlice.push_back(nextMip);
				currentMip = arraySlice.back();
			}
		}
	}

	void ApplyMipPolicy(const TextureCookRequest& request, WorkingTexture& workingTexture)
	{
		switch (request.policy.mipPolicy)
		{
			case TextureMipPolicy::NoMips:
				for (auto& arraySlice : workingTexture.arraySlices)
				{
					if (arraySlice.size() > 1)
					{
						arraySlice.resize(1);
					}
				}
				return;
			case TextureMipPolicy::PreserveExisting:
				return;
			case TextureMipPolicy::Generate:
				GenerateMipChain(request, workingTexture);
				return;
		}

		throw Diagnostics::Error("Unknown texture mip policy.");
	}
}

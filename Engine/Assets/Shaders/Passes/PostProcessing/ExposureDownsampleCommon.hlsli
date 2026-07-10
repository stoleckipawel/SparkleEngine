#ifndef SPARKLE_EXPOSURE_DOWNSAMPLE_COMMON_HLSLI
#define SPARKLE_EXPOSURE_DOWNSAMPLE_COMMON_HLSLI

#include "Display/Exposure.hlsli"

RWTexture2D<float4> LuminanceMomentsOutput;

namespace ExposureDownsample
{
	float2 LoadMoments(Texture2D inputTexture, uint2 pixel, bool sceneColorInput)
	{
		const float4 sampleValue = inputTexture.Load(int3(pixel, 0));
		return sceneColorInput ? Exposure::BuildLogLuminanceMoment(sampleValue.rgb) : sampleValue.xy;
	}

	float2 SumMoments2x2(Texture2D inputTexture, uint2 outputPixel, bool sceneColorInput)
	{
		uint inputWidth = 0u;
		uint inputHeight = 0u;
		inputTexture.GetDimensions(inputWidth, inputHeight);

		float2 moments = 0.0f.xx;
		[unroll] for (uint y = 0u; y < 2u; ++y)
		{
			[unroll] for (uint x = 0u; x < 2u; ++x)
			{
				const uint2 pixel = outputPixel * 2u + uint2(x, y);
				if (pixel.x < inputWidth && pixel.y < inputHeight)
				{
					moments += LoadMoments(inputTexture, pixel, sceneColorInput);
				}
			}
		}
		return moments;
	}

	bool IsOutsideOutput(uint2 outputPixel)
	{
		uint outputWidth = 0u;
		uint outputHeight = 0u;
		LuminanceMomentsOutput.GetDimensions(outputWidth, outputHeight);
		return outputPixel.x >= outputWidth || outputPixel.y >= outputHeight;
	}

	void Store(uint2 outputPixel, float2 moments)
	{
		LuminanceMomentsOutput[outputPixel] = float4(moments, 0.0f, 0.0f);
	}
}

#endif

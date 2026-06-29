#include "Display/Exposure.hlsli"

Texture2D SceneColor;
Texture2D LuminanceMomentsInput;
RWTexture2D<float4> LuminanceMomentsOutput;

float2 SumSceneMoments2x2(uint2 outputPixel)
{
	uint inputWidth = 0;
	uint inputHeight = 0;
	SceneColor.GetDimensions(inputWidth, inputHeight);

	float2 moments = 0.0f.xx;
	[unroll] for (uint y = 0u; y < 2u; ++y)
	{
		[unroll] for (uint x = 0u; x < 2u; ++x)
		{
			const uint2 pixel = outputPixel * 2u + uint2(x, y);
			if (pixel.x < inputWidth && pixel.y < inputHeight)
			{
				moments += Exposure::BuildLogLuminanceMoment(SceneColor.Load(int3(pixel, 0)).rgb);
			}
		}
	}
	return moments;
}

float2 SumTextureMoments2x2(uint2 outputPixel)
{
	uint inputWidth = 0;
	uint inputHeight = 0;
	LuminanceMomentsInput.GetDimensions(inputWidth, inputHeight);

	float2 moments = 0.0f.xx;
	[unroll] for (uint y = 0u; y < 2u; ++y)
	{
		[unroll] for (uint x = 0u; x < 2u; ++x)
		{
			const uint2 pixel = outputPixel * 2u + uint2(x, y);
			if (pixel.x < inputWidth && pixel.y < inputHeight)
			{
				moments += LuminanceMomentsInput.Load(int3(pixel, 0)).xy;
			}
		}
	}
	return moments;
}

bool IsOutsideOutput(uint2 outputPixel)
{
	uint outputWidth = 0;
	uint outputHeight = 0;
	LuminanceMomentsOutput.GetDimensions(outputWidth, outputHeight);
	return outputPixel.x >= outputWidth || outputPixel.y >= outputHeight;
}

[numthreads(8, 8, 1)] void DownsampleSceneMain(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	if (IsOutsideOutput(dispatchThreadId.xy))
	{
		return;
	}

	LuminanceMomentsOutput[dispatchThreadId.xy] = float4(SumSceneMoments2x2(dispatchThreadId.xy), 0.0f, 0.0f);
}

[numthreads(8, 8, 1)] void DownsampleTextureMain(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	if (IsOutsideOutput(dispatchThreadId.xy))
	{
		return;
	}

	LuminanceMomentsOutput[dispatchThreadId.xy] = float4(SumTextureMoments2x2(dispatchThreadId.xy), 0.0f, 0.0f);
}

#include "Passes/PostProcessing/ExposureDownsampleCommon.hlsli"

Texture2D SceneColor;

[numthreads(8, 8, 1)] void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	const uint2 outputPixel = dispatchThreadId.xy;
	if (ExposureDownsample::IsOutsideOutput(outputPixel))
	{
		return;
	}

	ExposureDownsample::Store(outputPixel, ExposureDownsample::SumMoments2x2(SceneColor, outputPixel, true));
}

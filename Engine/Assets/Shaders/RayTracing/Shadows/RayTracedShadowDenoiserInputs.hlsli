#ifndef SPARKLE_RAY_TRACED_SHADOW_DENOISER_INPUTS_HLSLI
#define SPARKLE_RAY_TRACED_SHADOW_DENOISER_INPUTS_HLSLI

#include "RayTracing/Shadows/RayTracedShadowSignals.hlsli"

namespace RayTracedShadowDenoiserInputs
{
	float4 PackShadowSignal(ShadowVisibilitySignal signal)
	{
		return float4(signal.Visibility, signal.HitDistance, signal.Confidence, signal.MaxDistance);
	}

	ShadowVisibilitySignal UnpackShadowSignal(float4 packedSignal)
	{
		ShadowVisibilitySignal signal;
		signal.Visibility = packedSignal.x;
		signal.HitDistance = packedSignal.y;
		signal.Confidence = packedSignal.z;
		signal.MaxDistance = packedSignal.w;
		return signal;
	}
}

#endif

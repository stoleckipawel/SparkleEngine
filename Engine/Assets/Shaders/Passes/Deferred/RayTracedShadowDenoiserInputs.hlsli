#ifndef SPARKLE_RAY_TRACED_SHADOW_DENOISER_INPUTS_HLSLI
#define SPARKLE_RAY_TRACED_SHADOW_DENOISER_INPUTS_HLSLI

#include "Passes/Deferred/RayTracedShadowSignals.hlsli"

namespace RayTracedShadowDenoiserInputs
{
	float4 PackShadowSignal(ShadowVisibilitySignal signal)
	{
		return float4(signal.Visibility, signal.HitDistance, signal.Confidence, signal.MaxDistance);
	}
}

#endif

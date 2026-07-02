#ifndef SPARKLE_RAY_TRACED_SHADOW_SIGNAL_PACKING_HLSLI
#define SPARKLE_RAY_TRACED_SHADOW_SIGNAL_PACKING_HLSLI

#include "RayTracing/Shadows/RayTracedShadowSignals.hlsli"

namespace RayTracedShadowSignalPacking
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

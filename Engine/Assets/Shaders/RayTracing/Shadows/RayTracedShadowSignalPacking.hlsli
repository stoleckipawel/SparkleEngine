#ifndef SPARKLE_RAY_TRACED_SHADOW_SIGNAL_PACKING_HLSLI
#define SPARKLE_RAY_TRACED_SHADOW_SIGNAL_PACKING_HLSLI

#include "/Engine/RayTracing/Shadows/RayTracedShadowSignals.hlsli"

namespace RayTracedShadowSignalPacking
{
	float4 PackShadowSignal(ShadowVisibilitySample signal)
	{
		return float4(signal.Visibility, signal.HitDistance, signal.Confidence, signal.MaxDistance);
	}

	ShadowVisibilitySample UnpackShadowSignal(float4 packedSignal)
	{
		ShadowVisibilitySample signal;
		signal.Visibility = packedSignal.x;
		signal.HitDistance = packedSignal.y;
		signal.Confidence = packedSignal.z;
		signal.MaxDistance = packedSignal.w;
		return signal;
	}
}

#endif

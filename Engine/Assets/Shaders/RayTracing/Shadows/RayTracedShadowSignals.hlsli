#ifndef SPARKLE_RAY_TRACED_SHADOW_SIGNALS_HLSLI
#define SPARKLE_RAY_TRACED_SHADOW_SIGNALS_HLSLI

struct ShadowVisibilitySample
{
	float Visibility;
	float HitDistance;
	float Confidence;
	float MaxDistance;
};

namespace RayTracedShadowSignals
{
	ShadowVisibilitySample BuildUnshadowedSignal(float maxDistance)
	{
		ShadowVisibilitySample signal;
		signal.Visibility = 1.0f;
		signal.HitDistance = maxDistance;
		signal.Confidence = 1.0f;
		signal.MaxDistance = maxDistance;
		return signal;
	}

	ShadowVisibilitySample BuildOccludedSignal(float hitDistance, float maxDistance)
	{
		ShadowVisibilitySample signal;
		signal.Visibility = 0.0f;
		signal.HitDistance = hitDistance;
		signal.Confidence = 1.0f;
		signal.MaxDistance = maxDistance;
		return signal;
	}
}

#endif

#ifndef SPARKLE_RAY_TRACING_PATH_TRACER_HLSLI
#define SPARKLE_RAY_TRACING_PATH_TRACER_HLSLI

#include "/Engine/RayTracing/PathSurface.hlsli"
#include "/Engine/RayTracing/RayTracingPathSample.hlsli"

namespace PathTracer
{
	struct PathState
	{
		float3 OriginWorld;
		float3 DirectionWorld;
		float3 Throughput;
		uint SurfaceDepth;
	};

	float4 InvalidRadiance()
	{
		const float quietNan = asfloat(0x7FC00000u);
		return float4(quietNan, quietNan, quietNan, 0.0f);
	}

	void AddRadiance(inout float3 contribution, float3 throughput, float3 radiance)
	{
		contribution += throughput * radiance;
	}

	float PowerHeuristic(float selectedProbability, float competingProbability)
	{
		const float scale = max(selectedProbability, competingProbability);
		if (selectedProbability <= 0.0f || scale <= 0.0f)
		{
			return 0.0f;
		}
		const float selected = selectedProbability / scale;
		const float competing = competingProbability / scale;
		const float selected2 = selected * selected;
		const float competing2 = competing * competing;
		const float denominator = selected2 + competing2;
		return denominator > 0.0f ? selected2 / denominator : 0.0f;
	}

	void ApplySurvivalCompensation(inout float3 throughput, float survivalProbability)
	{
		throughput /= survivalProbability;
	}

	void ApplyDirectionSample(inout PathState path, RayTracingPathSample::DirectionSample sample)
	{
		path.Throughput *= sample.Throughput;
		path.DirectionWorld = sample.DirectionWorld;
		++path.SurfaceDepth;
	}
}

#endif

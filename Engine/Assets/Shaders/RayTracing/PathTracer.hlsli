#ifndef SPARKLE_RAY_TRACING_PATH_TRACER_HLSLI
#define SPARKLE_RAY_TRACING_PATH_TRACER_HLSLI

#include "/Engine/BRDF/Diffuse.hlsli"
#include "/Engine/Common/Sampling.hlsli"
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

	bool IsFiniteNonNegative(float3 value)
	{
		return all(isfinite(value)) && all(value >= 0.0f);
	}

	float4 InvalidRadiance()
	{
		const float quietNan = asfloat(0x7FC00000u);
		return float4(quietNan, quietNan, quietNan, 0.0f);
	}

	bool TryAddRadiance(inout float3 contribution, float3 throughput, float3 radiance)
	{
		const float3 value = throughput * radiance;
		if (!IsFiniteNonNegative(value) || !IsFiniteNonNegative(contribution + value))
		{
			return false;
		}
		contribution += value;
		return true;
	}

	RayTracingPathSample::DirectionSample InvalidDirectionSample(uint lobe)
	{
		RayTracingPathSample::DirectionSample result = (RayTracingPathSample::DirectionSample)0;
		result.Lobe = lobe;
		result.RejectionReason = RayTracingPathSample::RejectionReasonInvalidSample;
		return result;
	}

	RayTracingPathSample::DirectionSample SampleLambertian(RayTracingPathSurface surface, float2 sample, float selectionMass)
	{
		RayTracingPathSample::DirectionSample result = InvalidDirectionSample(RayTracingPathSample::LobeDiffuse);
		if (!surface.Valid || selectionMass <= 0.0f)
		{
			return result;
		}

		const CommonSampling::CosineHemisphereSample directionSample = CommonSampling::SampleCosineHemisphere(surface.NormalWorld, sample);
		result.DirectionWorld = directionSample.DirectionWorld;
		result.Pdf = selectionMass * directionSample.Pdf;
		result.CosineTerm = directionSample.Cosine;
		result.CompleteContinuousF = BRDF::Diffuse::Lambert(surface.BaseColor);
		result.Throughput = result.CompleteContinuousF * result.CosineTerm / result.Pdf;
		result.HasSupport = any(surface.BaseColor > 0.0f);
		result.RejectionReason = RayTracingPathSample::RejectionReasonNone;
		return result;
	}

	bool ApplyDirectionSample(inout PathState path, RayTracingPathSample::DirectionSample sample)
	{
		if (sample.RejectionReason != RayTracingPathSample::RejectionReasonNone || !sample.HasSupport)
		{
			return false;
		}
		const float3 throughput = path.Throughput * sample.Throughput;
		if (!IsFiniteNonNegative(throughput))
		{
			return false;
		}
		path.Throughput = throughput;
		path.DirectionWorld = sample.DirectionWorld;
		++path.SurfaceDepth;
		return true;
	}
}

#endif

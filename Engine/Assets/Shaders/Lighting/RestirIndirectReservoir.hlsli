#ifndef SPARKLE_RESTIR_INDIRECT_RESERVOIR_HLSLI
#define SPARKLE_RESTIR_INDIRECT_RESERVOIR_HLSLI

#include "/Engine/Resources/FrameUniformData.hlsli"

#include "/Engine/Common/Color.hlsli"
#include "/Engine/Common/Random.hlsli"
#include "/Engine/Lighting/RestirReservoirCommon.hlsli"
#include "/Engine/RayTracing/Shadows/RayTracedShadowVisibility.hlsli"
#include "/Engine/RayTracing/GBufferPathSurface.hlsli"
#include "/Engine/RayTracing/PathLighting.hlsli"

namespace RestirIndirectReservoir
{
	static const float MinTarget = 1.0e-6f;

	typedef RayTracingGBufferPathSurface::Surface Surface;

	struct Candidate
	{
		uint2 RandomPixel;
		uint SampleIndex;
		uint RandomFrameIndex;
	};

	struct Reservoir
	{
		Candidate Selected;
		float WeightSum;
		float Target;
		float M;
		float Valid;
	};

	Surface LoadSurface(uint2 pixelCoord)
	{
		return RayTracingGBufferPathSurface::Load(pixelCoord);
	}

	float4 PackSurface(Surface surface)
	{
		return RestirReservoirCommon::PackSurface(surface.Valid, surface.GBuffer.NormalWorld, surface.ViewDistance);
	}

	bool AreSurfacesCompatible(Surface surface, float4 packedSurface)
	{
		return RestirReservoirCommon::AreSurfacesCompatible(surface.Valid,
		                                                    surface.GBuffer.NormalWorld,
		                                                    surface.ViewDistance,
		                                                    packedSurface);
	}

	Reservoir EmptyReservoir()
	{
		Reservoir reservoir = (Reservoir)0;
		return reservoir;
	}

	bool IsValid(Reservoir reservoir)
	{
		return reservoir.Valid > 0.5f && reservoir.WeightSum > 0.0f && reservoir.Target > MinTarget && reservoir.M > 0.0f;
	}

	float4 PackSample(Reservoir reservoir)
	{
		return IsValid(reservoir) ? float4(float(reservoir.Selected.RandomPixel.x),
		                                   float(reservoir.Selected.RandomPixel.y),
		                                   float(reservoir.Selected.SampleIndex),
		                                   float(reservoir.Selected.RandomFrameIndex))
		                          : 0.0f.xxxx;
	}

	float4 PackWeight(Reservoir reservoir)
	{
		return reservoir.M > 0.0f ? float4(reservoir.WeightSum, reservoir.Target, reservoir.M, reservoir.Valid) : 0.0f.xxxx;
	}

	Reservoir UnpackReservoir(float4 samplePayload, float4 weightPayload)
	{
		Reservoir reservoir;
		reservoir.Selected.RandomPixel = uint2(samplePayload.xy + 0.5f);
		reservoir.Selected.SampleIndex = uint(samplePayload.z + 0.5f);
		reservoir.Selected.RandomFrameIndex = uint(samplePayload.w + 0.5f);
		reservoir.WeightSum = weightPayload.x;
		reservoir.Target = weightPayload.y;
		reservoir.M = weightPayload.z;
		reservoir.Valid = weightPayload.w;
		if (reservoir.M <= 0.0f)
		{
			return EmptyReservoir();
		}
		if (!IsValid(reservoir))
		{
			reservoir.WeightSum = 0.0f;
			reservoir.Target = 0.0f;
			reservoir.Valid = 0.0f;
		}
		return reservoir;
	}

	RayTracingPathTrace::TraceSettings BuildTraceSettings()
	{
		return RayTracingPathTrace::BuildSurfaceTraceSettings(RestirIndirectNormalBias, RestirIndirectMaxDistance);
	}

	RayTracingPathLighting::Result EvaluateCandidate(Surface surface, Candidate candidate, Texture2D skyTexture, SamplerState skySampler)
	{
		return RayTracingPathLighting::TraceSurfacePathWithRandomFrame(skyTexture,
		                                                               skySampler,
		                                                               surface.PathSurface,
		                                                               candidate.RandomPixel,
		                                                               candidate.SampleIndex,
		                                                               RayTracingPathSampling::SpecularSampleModeStochasticGGX,
		                                                               RestirIndirectBounceCount,
		                                                               candidate.RandomFrameIndex,
		                                                               BuildTraceSettings());
	}

	float EvaluateTarget(RayTracingPathLighting::Result path)
	{
		return max(CommonColor::LuminanceRec709(max(path.FinalContribution, 0.0f.xxx)), 0.0f);
	}

	bool StreamCandidate(inout Reservoir reservoir, Candidate candidate, float target, float random)
	{
		reservoir.M += 1.0f;
		if (target <= MinTarget)
		{
			return false;
		}
		const float newWeightSum = reservoir.WeightSum + target;
		const bool selected = random * newWeightSum <= target;
		reservoir.WeightSum = newWeightSum;
		if (selected)
		{
			reservoir.Selected = candidate;
			reservoir.Target = target;
			reservoir.Valid = 1.0f;
		}
		return selected;
	}

	bool CombineReservoir(inout Reservoir reservoir,
	                      Reservoir candidateReservoir,
	                      Surface surface,
	                      Texture2D skyTexture,
	                      SamplerState skySampler,
	                      float maxM,
	                      float random)
	{
		if (!surface.Valid || candidateReservoir.M <= 0.0f)
		{
			return false;
		}
		const float candidateM = candidateReservoir.M;
		const float acceptedM = min(candidateM, maxM);
		reservoir.M += acceptedM;
		if (!IsValid(candidateReservoir))
		{
			return false;
		}
		const RayTracingPathLighting::Result replay = EvaluateCandidate(surface, candidateReservoir.Selected, skyTexture, skySampler);
		const float target = EvaluateTarget(replay);
		if (target <= MinTarget)
		{
			return false;
		}
		const float sampleWeight =
		    candidateReservoir.WeightSum * (acceptedM / candidateM) * target / max(candidateReservoir.Target, MinTarget);
		const float newWeightSum = reservoir.WeightSum + sampleWeight;
		const bool selected = random * newWeightSum <= sampleWeight;
		reservoir.WeightSum = newWeightSum;
		if (selected)
		{
			reservoir.Selected = candidateReservoir.Selected;
			reservoir.Target = target;
			reservoir.Valid = 1.0f;
		}
		return selected;
	}

	Reservoir BuildInitialReservoir(Surface surface, uint2 pixelCoord, Texture2D skyTexture, SamplerState skySampler)
	{
		Reservoir reservoir = EmptyReservoir();
		if (!surface.Valid)
		{
			return reservoir;
		}
		uint rng = RestirReservoirCommon::BuildSeed(pixelCoord, 0x1D1EEC7u);
		[unroll]
		for (uint candidateIndex = 0u; candidateIndex < RestirReservoirCommon::InitialCandidateCount; ++candidateIndex)
		{
			Candidate candidate;
			candidate.RandomPixel = pixelCoord;
			candidate.SampleIndex = candidateIndex;
			candidate.RandomFrameIndex = FrameIndex;
			const float target = EvaluateTarget(EvaluateCandidate(surface, candidate, skyTexture, skySampler));
			StreamCandidate(reservoir, candidate, target, CommonRandom::Random01(rng));
		}
		return reservoir;
	}

	float GetFinalWeight(Reservoir reservoir)
	{
		return IsValid(reservoir) ? reservoir.WeightSum / max(reservoir.M * reservoir.Target, MinTarget) : 0.0f;
	}
}

#endif

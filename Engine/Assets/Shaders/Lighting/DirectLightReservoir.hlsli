#ifndef SPARKLE_DIRECT_LIGHT_RESERVOIR_HLSLI
#define SPARKLE_DIRECT_LIGHT_RESERVOIR_HLSLI

#include "/Engine/Resources/ViewCameraUniformData.hlsli"

#include "/Engine/Common/Color.hlsli"
#include "/Engine/Common/Random.hlsli"
#include "/Engine/Lighting/DirectLightSampling.hlsli"
#include "/Engine/Lighting/RestirReservoirCommon.hlsli"
#include "/Engine/Lighting/SurfaceLighting.hlsli"
#include "/Engine/Passes/GBuffer/GBufferUtils.hlsli"

namespace DirectLightReservoir
{
	static const float MinPdf = 1.0e-6f;

	struct Surface
	{
		bool Valid;
		GBufferData GBuffer;
		float3 PositionWorld;
		float3 ViewDirWorld;
		float ViewDistance;
		bool EvaluateSubsurface;
	};

	struct Reservoir
	{
		DirectLightSampling::LightCandidate Candidate;
		float2 ShapeSample;
		float WeightSum;
		float TargetPdf;
		float M;
		float Valid;
	};

	Surface LoadSurface(uint2 pixelCoord)
	{
		Surface surface;
		surface.Valid = false;
		surface.GBuffer = LoadGBuffer(pixelCoord);
		surface.PositionWorld = 0.0f.xxx;
		surface.ViewDirWorld = 0.0f.xxx;
		surface.ViewDistance = 0.0f;
		surface.EvaluateSubsurface = false;

		if (IsSkyPixel(surface.GBuffer.SceneDepth))
		{
			return surface;
		}

		surface.PositionWorld = ReconstructGBufferWorldPosition(pixelCoord, surface.GBuffer.SceneDepth, InvViewMTX, InvProjectionMTX);
		const float3 cameraToSurface = surface.PositionWorld - Position;
		surface.ViewDistance = length(cameraToSurface);
		surface.ViewDirWorld = surface.ViewDistance > 1.0e-5f ? -cameraToSurface / surface.ViewDistance : 0.0f.xxx;
		surface.EvaluateSubsurface = any(surface.GBuffer.SubsurfaceColor > 0.0f.xxx) && surface.GBuffer.SubsurfaceStrength > 0.0f;
		surface.Valid = true;
		return surface;
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
		Reservoir reservoir;
		reservoir.Candidate = DirectLightSampling::InvalidLightCandidate();
		reservoir.ShapeSample = 0.0f.xx;
		reservoir.WeightSum = 0.0f;
		reservoir.TargetPdf = 0.0f;
		reservoir.M = 0.0f;
		reservoir.Valid = 0.0f;
		return reservoir;
	}

	bool IsValid(Reservoir reservoir)
	{
		return reservoir.Valid > 0.5f && DirectLightSampling::IsValid(reservoir.Candidate) && reservoir.WeightSum > 0.0f
		    && reservoir.TargetPdf > 0.0f && reservoir.M > 0.0f;
	}

	float4 PackReservoirSample(Reservoir reservoir)
	{
		return IsValid(reservoir) ? float4((float)reservoir.Candidate.Light.Type,
		                                   (float)reservoir.Candidate.Light.Index,
		                                   reservoir.ShapeSample.x,
		                                   reservoir.ShapeSample.y)
		                          : 0.0f.xxxx;
	}

	float4 PackReservoirWeight(Reservoir reservoir)
	{
		return reservoir.M > 0.0f ? float4(reservoir.WeightSum, reservoir.TargetPdf, reservoir.M, reservoir.Valid) : 0.0f.xxxx;
	}

	Reservoir UnpackReservoir(float4 samplePayload, float4 weightPayload)
	{
		Reservoir reservoir;
		reservoir.Candidate.Light.Type = (uint)(samplePayload.x + 0.5f);
		reservoir.Candidate.Light.Index = (uint)(samplePayload.y + 0.5f);
		reservoir.Candidate.SelectionPdf = 1.0f;
		reservoir.Candidate.Valid = 1.0f;
		reservoir.ShapeSample = saturate(samplePayload.zw);
		reservoir.WeightSum = weightPayload.x;
		reservoir.TargetPdf = weightPayload.y;
		reservoir.M = weightPayload.z;
		reservoir.Valid = weightPayload.w;

		if (reservoir.M <= 0.0f)
		{
			return EmptyReservoir();
		}
		if (!IsValid(reservoir) || !DirectLightSampling::IsLightIdInRange(reservoir.Candidate.Light))
		{
			reservoir.Candidate = DirectLightSampling::InvalidLightCandidate();
			reservoir.ShapeSample = 0.0f.xx;
			reservoir.WeightSum = 0.0f;
			reservoir.TargetPdf = 0.0f;
			reservoir.Valid = 0.0f;
		}

		return reservoir;
	}

	LightSampling::DirectLightSample ReplayLightSample(Reservoir reservoir, float3 positionWorld)
	{
		if (!IsValid(reservoir))
		{
			return LightSampling::InvalidDirectLightSample();
		}

		DirectLightSampling::LightCandidate candidate = reservoir.Candidate;
		candidate.SelectionPdf = 1.0f;
		LightSampling::DirectLightSample sample = DirectLightSampling::SampleDirectLight(candidate, positionWorld, reservoir.ShapeSample);
		sample.LightSelectionPdf = 1.0f;
		sample.PdfW = 1.0f;
		return sample;
	}

	float3 EvaluateRawContribution(Surface surface, Reservoir reservoir)
	{
		if (!surface.Valid || !IsValid(reservoir))
		{
			return 0.0f.xxx;
		}

		const LightSampling::DirectLightSample lightSample = ReplayLightSample(reservoir, surface.PositionWorld);
		float3 diffuse = 0.0f;
		float3 specular = 0.0f;
		float3 subsurface = 0.0f;
		SurfaceLighting::AccumulateDirectLightSample(surface.ViewDirWorld,
		                                             surface.GBuffer.NormalWorld,
		                                             surface.GBuffer.BaseColor,
		                                             surface.GBuffer.Roughness,
		                                             surface.GBuffer.Metallic,
		                                             surface.GBuffer.DielectricF0,
		                                             surface.GBuffer.SubsurfaceColor,
		                                             surface.GBuffer.SubsurfaceStrength,
		                                             surface.EvaluateSubsurface,
		                                             lightSample,
		                                             1.0f,
		                                             diffuse,
		                                             specular,
		                                             subsurface);
		return max(diffuse + specular + subsurface, 0.0f.xxx);
	}

	float EvaluateTargetPdf(Surface surface, Reservoir reservoir)
	{
		return CommonColor::LuminanceRec709(EvaluateRawContribution(surface, reservoir));
	}

	float ComputeSourcePdf(DirectLightSampling::LightCandidate candidate, float3 positionWorld, float2 shapeSample)
	{
		if (!DirectLightSampling::IsValid(candidate))
		{
			return 0.0f;
		}

		const LightSampling::DirectLightSample sample = DirectLightSampling::SampleDirectLight(candidate, positionWorld, shapeSample);
		return max(candidate.SelectionPdf * sample.PdfW, 0.0f);
	}

	bool StreamWeightedSample(inout Reservoir reservoir,
	                          DirectLightSampling::LightCandidate candidate,
	                          float2 shapeSample,
	                          float targetPdf,
	                          float sourcePdf,
	                          float random)
	{
		reservoir.M += 1.0f;
		if (!DirectLightSampling::IsValid(candidate) || targetPdf <= MinPdf || sourcePdf <= MinPdf)
		{
			return false;
		}

		const float sampleWeight = targetPdf / sourcePdf;
		const float newWeightSum = reservoir.WeightSum + sampleWeight;
		const bool selected = random * newWeightSum <= sampleWeight;
		reservoir.WeightSum = newWeightSum;
		if (selected)
		{
			reservoir.Candidate = candidate;
			reservoir.Candidate.SelectionPdf = 1.0f;
			reservoir.ShapeSample = shapeSample;
			reservoir.TargetPdf = targetPdf;
			reservoir.Valid = 1.0f;
		}
		return selected;
	}

	bool CombineReservoir(inout Reservoir reservoir, Reservoir candidateReservoir, Surface surface, float maxM, float random)
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

		const float targetPdf = EvaluateTargetPdf(surface, candidateReservoir);
		if (targetPdf <= MinPdf)
		{
			return false;
		}

		const float historyScale = acceptedM / candidateM;
		const float sampleWeight = candidateReservoir.WeightSum * historyScale * targetPdf / max(candidateReservoir.TargetPdf, MinPdf);
		if (sampleWeight <= MinPdf)
		{
			return false;
		}

		const float newWeightSum = reservoir.WeightSum + sampleWeight;
		const bool selected = random * newWeightSum <= sampleWeight;
		reservoir.WeightSum = newWeightSum;
		if (selected)
		{
			reservoir.Candidate = candidateReservoir.Candidate;
			reservoir.ShapeSample = candidateReservoir.ShapeSample;
			reservoir.TargetPdf = targetPdf;
			reservoir.Valid = 1.0f;
		}
		return selected;
	}

	Reservoir BuildInitialReservoir(Surface surface, uint2 pixelCoord)
	{
		if (!surface.Valid)
		{
			return EmptyReservoir();
		}

		Reservoir reservoir = EmptyReservoir();
		uint rng = RestirReservoirCommon::BuildSeed(pixelCoord, 0xC0FFEEu);
		[unroll] for (uint candidateIndex = 0u; candidateIndex < RestirReservoirCommon::InitialCandidateCount; ++candidateIndex)
		{
			const DirectLightSampling::LightCandidate candidate =
			    DirectLightSampling::SampleUniformLightCandidate(CommonRandom::Random01(rng));
			const float2 shapeSample = CommonRandom::Random02(rng);
			Reservoir candidateReservoir = EmptyReservoir();
			candidateReservoir.Candidate = candidate;
			candidateReservoir.ShapeSample = shapeSample;
			candidateReservoir.WeightSum = 1.0f;
			candidateReservoir.TargetPdf = 1.0f;
			candidateReservoir.M = 1.0f;
			candidateReservoir.Valid = DirectLightSampling::IsValid(candidate) ? 1.0f : 0.0f;

			const float targetPdf = EvaluateTargetPdf(surface, candidateReservoir);
			const float sourcePdf = ComputeSourcePdf(candidate, surface.PositionWorld, shapeSample);
			StreamWeightedSample(reservoir, candidate, shapeSample, targetPdf, sourcePdf, CommonRandom::Random01(rng));
		}

		return reservoir;
	}

	float GetFinalWeight(Reservoir reservoir)
	{
		return IsValid(reservoir) ? reservoir.WeightSum / max(reservoir.M * reservoir.TargetPdf, MinPdf) : 0.0f;
	}
}

#endif

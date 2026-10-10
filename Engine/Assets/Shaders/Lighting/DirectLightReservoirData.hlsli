#ifndef SPARKLE_DIRECT_LIGHT_RESERVOIR_DATA_HLSLI
#define SPARKLE_DIRECT_LIGHT_RESERVOIR_DATA_HLSLI

#include "/Engine/Lighting/DirectLightSampling.hlsli"

namespace DirectLightReservoir
{
	static const float MinPdf = 1.0e-6f;

	struct Reservoir
	{
		DirectLightSampling::LightCandidate Candidate;
		float2 ShapeSample;
		float WeightSum;
		float TargetPdf;
		float M;
		float Valid;
	};

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
		return reservoir.Valid > 0.5f && DirectLightSampling::IsValid(reservoir.Candidate) && reservoir.WeightSum > 0.0f && reservoir.TargetPdf > 0.0f && reservoir.M > 0.0f;
	}

	float4 PackReservoirSample(Reservoir reservoir)
	{
		return IsValid(reservoir) ? float4((float)reservoir.Candidate.Light.Type, (float)reservoir.Candidate.Light.Index, reservoir.ShapeSample.x, reservoir.ShapeSample.y) : 0.0f.xxxx;
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

	float GetFinalWeight(Reservoir reservoir)
	{
		return IsValid(reservoir) ? reservoir.WeightSum / max(reservoir.M * reservoir.TargetPdf, MinPdf) : 0.0f;
	}
}

#endif

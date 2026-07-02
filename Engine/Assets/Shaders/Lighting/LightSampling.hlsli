#ifndef SPARKLE_LIGHT_SAMPLING_HLSLI
#define SPARKLE_LIGHT_SAMPLING_HLSLI

#include "Common/Math.hlsli"
#include "Common/Random.hlsli"

namespace LightSampling
{
	static const uint LightTypeDirectional = 0u;
	static const uint LightTypePoint = 1u;
	static const uint LightTypeSpot = 2u;
	static const uint LightTypeRect = 3u;
	static const uint LightTypeInvalid = 0xFFFFFFFFu;
	static const uint LightIndexInvalid = 0xFFFFFFFFu;

	struct DirectLightSample
	{
		bool Valid;
		float3 DirectionWorld;
		float Distance;
		float3 IncidentRadiance;
		float PdfW;
		float LightSelectionPdf;
		float3 EmitterNormalWorld;
		float3 SamplePositionWorld;
		float VisibilityDistance;
		bool IsDirectional;
	};

	DirectLightSample InvalidDirectLightSample()
	{
		DirectLightSample result;
		result.Valid = false;
		result.DirectionWorld = 0.0f.xxx;
		result.Distance = 0.0f;
		result.IncidentRadiance = 0.0f.xxx;
		result.PdfW = 0.0f;
		result.LightSelectionPdf = 1.0f;
		result.EmitterNormalWorld = 0.0f.xxx;
		result.SamplePositionWorld = 0.0f.xxx;
		result.VisibilityDistance = 0.0f;
		result.IsDirectional = false;
		return result;
	}

	DirectLightSample PunctualDirectLightSample(float3 directionWorld, float3 incidentRadiance, float distanceToLight, bool isDirectional)
	{
		DirectLightSample result;
		result.Valid = any(incidentRadiance > 0.0f.xxx);
		result.DirectionWorld = SafeNormalize(directionWorld);
		result.Distance = distanceToLight;
		result.IncidentRadiance = max(incidentRadiance, 0.0f.xxx);
		result.PdfW = 1.0f;
		result.LightSelectionPdf = 1.0f;
		result.EmitterNormalWorld = 0.0f.xxx;
		result.SamplePositionWorld = 0.0f.xxx;
		result.VisibilityDistance = distanceToLight;
		result.IsDirectional = isDirectional;
		return result;
	}

	float AreaPdfToSolidAnglePdf(float pdfA, float distanceToLight, float cosLight)
	{
		return pdfA * distanceToLight * distanceToLight / max(abs(cosLight), 1.0e-4f);
	}

	DirectLightSample AreaDirectLightSample(
	    float3 positionWorld,
	    float3 samplePositionWorld,
	    float3 emitterNormalWorld,
	    float3 emittedRadiance,
	    float pdfA,
	    float rangeCutoff)
	{
		const float3 surfaceToLight = samplePositionWorld - positionWorld;
		const float distanceToLight = length(surfaceToLight);
		if (distanceToLight <= 1.0e-4f || pdfA <= 0.0f || rangeCutoff <= 0.0f)
		{
			return InvalidDirectLightSample();
		}

		const float3 directionWorld = surfaceToLight / distanceToLight;
		const float3 normalWorld = SafeNormalize(emitterNormalWorld);
		const float cosLight = dot(normalWorld, -directionWorld);
		if (cosLight <= 1.0e-4f)
		{
			return InvalidDirectLightSample();
		}

		const float pdfW = AreaPdfToSolidAnglePdf(pdfA, distanceToLight, cosLight);
		if (pdfW <= 1.0e-4f)
		{
			return InvalidDirectLightSample();
		}

		DirectLightSample result;
		result.Valid = any(emittedRadiance > 0.0f.xxx);
		result.DirectionWorld = directionWorld;
		result.Distance = distanceToLight;
		result.IncidentRadiance = max(emittedRadiance, 0.0f.xxx) * rangeCutoff;
		result.PdfW = pdfW;
		result.LightSelectionPdf = 1.0f;
		result.EmitterNormalWorld = normalWorld;
		result.SamplePositionWorld = samplePositionWorld;
		result.VisibilityDistance = distanceToLight;
		result.IsDirectional = false;
		return result;
	}

	float2 StableLightSample2D(float3 positionWorld, uint lightIndex, uint dimensionTag, uint frameIndex)
	{
		uint state = CommonRandom::Hash(asuint(positionWorld.x));
		state = CommonRandom::Hash(state ^ asuint(positionWorld.y));
		state = CommonRandom::Hash(state ^ asuint(positionWorld.z));
		state = CommonRandom::Hash(state ^ (lightIndex * 1664525u + dimensionTag * 1013904223u + frameIndex));
		return CommonRandom::Random02(state);
	}
}

#endif

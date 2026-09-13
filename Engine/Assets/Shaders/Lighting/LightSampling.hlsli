#ifndef SPARKLE_LIGHT_SAMPLING_HLSLI
#define SPARKLE_LIGHT_SAMPLING_HLSLI

#include "/Engine/Resources/FrameUniformData.hlsli"

#include "/Engine/Common/Math.hlsli"
#include "/Engine/Common/Random.hlsli"

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
		float EmitterEndpointBaseOffset;
		float4 EmitterEndpointTraversalSensitivity;
		float VisibilityDistance;
		bool IsDirectional;
		bool Delta;
		uint TargetInstanceId;
		uint TargetPrimitiveIndex;
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
		result.EmitterEndpointBaseOffset = 0.0f;
		result.EmitterEndpointTraversalSensitivity = 0.0f.xxxx;
		result.VisibilityDistance = 0.0f;
		result.IsDirectional = false;
		result.Delta = false;
		result.TargetInstanceId = 0xFFFFFFFFu;
		result.TargetPrimitiveIndex = 0xFFFFFFFFu;
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
		result.EmitterEndpointBaseOffset = 0.0f;
		result.EmitterEndpointTraversalSensitivity = 0.0f.xxxx;
		result.VisibilityDistance = distanceToLight;
		result.IsDirectional = isDirectional;
		result.Delta = true;
		result.TargetInstanceId = 0xFFFFFFFFu;
		result.TargetPrimitiveIndex = 0xFFFFFFFFu;
		return result;
	}

	float AreaPdfToSolidAnglePdf(float pdfA, float distanceToLight, float cosLight)
	{
		return pdfA * distanceToLight * distanceToLight / max(abs(cosLight), 1.0e-4f);
	}

	DirectLightSample AreaDirectLightSample(float3 positionWorld,
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
		result.EmitterEndpointBaseOffset = 0.0f;
		result.EmitterEndpointTraversalSensitivity = 0.0f.xxxx;
		result.VisibilityDistance = distanceToLight;
		result.IsDirectional = false;
		result.Delta = false;
		result.TargetInstanceId = 0xFFFFFFFFu;
		result.TargetPrimitiveIndex = 0xFFFFFFFFu;
		return result;
	}

	float3 PhotometricRgbToRadiometric(float3 color, float quantity)
	{
		const float luminance = dot(color, float3(0.2126f, 0.7152f, 0.0722f));
		return luminance > 0.0f ? quantity * color / (683.0f * luminance) : 0.0f.xxx;
	}

	DirectLightSample RadiometricDirectionalLightSample(float3 directionWorld, float3 irradiance)
	{
		DirectLightSample result = (DirectLightSample)0;
		result.Valid = true;
		result.DirectionWorld = normalize(directionWorld);
		result.Distance = FLT_MAX;
		result.IncidentRadiance = irradiance;
		result.PdfW = 0.0f;
		result.LightSelectionPdf = 1.0f;
		result.VisibilityDistance = FLT_MAX;
		result.IsDirectional = true;
		result.Delta = true;
		result.TargetInstanceId = 0xFFFFFFFFu;
		result.TargetPrimitiveIndex = 0xFFFFFFFFu;
		return result;
	}

	DirectLightSample RadiometricPointLightSample(float3 positionWorld, float3 lightPositionWorld, float3 radiantIntensity)
	{
		DirectLightSample result = (DirectLightSample)0;
		const float3 toLight = lightPositionWorld - positionWorld;
		const float distance2 = dot(toLight, toLight);
		const float distance = sqrt(distance2);
		result.Valid = true;
		result.DirectionWorld = toLight / distance;
		result.Distance = distance;
		result.IncidentRadiance = radiantIntensity / distance2;
		result.PdfW = 0.0f;
		result.LightSelectionPdf = 1.0f;
		result.SamplePositionWorld = lightPositionWorld;
		result.EmitterEndpointBaseOffset = 0.0f;
		result.EmitterEndpointTraversalSensitivity = 0.0f.xxxx;
		result.VisibilityDistance = distance;
		result.IsDirectional = false;
		result.Delta = true;
		result.TargetInstanceId = 0xFFFFFFFFu;
		result.TargetPrimitiveIndex = 0xFFFFFFFFu;
		return result;
	}

	DirectLightSample RadiometricAreaLightSample(float3 positionWorld,
	                                             float3 samplePositionWorld,
	                                             float3 emitterNormalWorld,
	                                             float3 emittedRadiance,
	                                             float pdfA,
	                                             bool twoSided)
	{
		DirectLightSample result = (DirectLightSample)0;
		const float3 toLight = samplePositionWorld - positionWorld;
		const float distance2 = dot(toLight, toLight);
		const float distance = sqrt(distance2);
		const float3 directionWorld = toLight / distance;
		const float3 normalWorld = normalize(emitterNormalWorld);
		const float emitterCosine = dot(normalWorld, -directionWorld);
		const float absoluteEmitterCosine = twoSided ? abs(emitterCosine) : emitterCosine;
		if (absoluteEmitterCosine <= 0.0f)
		{
			return (DirectLightSample)0;
		}
		result.Valid = true;
		result.DirectionWorld = directionWorld;
		result.Distance = distance;
		result.IncidentRadiance = emittedRadiance;
		result.PdfW = pdfA * distance2 / absoluteEmitterCosine;
		result.LightSelectionPdf = 1.0f;
		result.EmitterNormalWorld = normalWorld;
		result.SamplePositionWorld = samplePositionWorld;
		result.EmitterEndpointBaseOffset = 0.0f;
		result.EmitterEndpointTraversalSensitivity = 0.0f.xxxx;
		result.VisibilityDistance = distance;
		result.IsDirectional = false;
		result.Delta = false;
		result.TargetInstanceId = 0xFFFFFFFFu;
		result.TargetPrimitiveIndex = 0xFFFFFFFFu;
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

#ifndef SPARKLE_LIGHTING_ENVIRONMENT_SAMPLING_HLSLI
#define SPARKLE_LIGHTING_ENVIRONMENT_SAMPLING_HLSLI

#include "/Engine/Lighting/LightSampling.hlsli"
#include "/Engine/Lighting/Sky.hlsli"

LightSampling::DirectLightSample SampleUniformEnvironmentRadiance(Texture2D skyTexture,
	                                                                SamplerState skySampler,
	                                                                float2 sample)
{
	const float cosineTheta = 1.0f - 2.0f * sample.y;
	const float sineTheta = sqrt(max(0.0f, 1.0f - cosineTheta * cosineTheta));
	const float phi = 6.28318530717958647692f * sample.x;
	const float3 directionWorld = float3(sineTheta * cos(phi), cosineTheta, sineTheta * sin(phi));
	LightSampling::DirectLightSample result = (LightSampling::DirectLightSample)0;
	result.Valid = true;
	result.DirectionWorld = directionWorld;
	result.Distance = FLT_MAX;
	result.IncidentRadiance = SampleSkyRadiance(skyTexture, skySampler, directionWorld);
	result.PdfW = 0.07957747154594766788f;
	result.LightSelectionPdf = 1.0f;
	result.VisibilityDistance = FLT_MAX;
	result.IsDirectional = true;
	result.TargetInstanceId = 0xFFFFFFFFu;
	result.TargetPrimitiveIndex = 0xFFFFFFFFu;
	return result;
}

#endif

#ifndef SPARKLE_AREA_LIGHTS_HLSLI
#define SPARKLE_AREA_LIGHTS_HLSLI

#include "Common/Constants.hlsli"
#include "Common/Math.hlsli"
#include "Common/Sampling.hlsli"
#include "Lighting/LightSampling.hlsli"
#include "Lighting/PunctualLights.hlsli"
#include "Resources/ConstantBuffers.hlsli"

namespace AreaLights
{
	LightSampling::DirectLightSample SampleDirectionalLight(uint lightIndex, float2 sample)
	{
		const DirectionalLightConstantBufferData light = DirectionalLights[lightIndex];
		const float3 centerDirectionWorld = PunctualLights::GetDirectionalLightDirection(lightIndex);
		const float3 illuminance = max(light.Color * light.Intensity, 0.0f.xxx);
		const float coneHalfAngle = max(light.AngularDiameter, 0.0f) * 0.5f;
		if (coneHalfAngle <= 1.0e-5f)
		{
			return LightSampling::PunctualDirectLightSample(centerDirectionWorld, illuminance, FLT_MAX, true);
		}

		const float cosHalfAngle = cos(coneHalfAngle);
		const float solidAngle = TWO_PI * max(1.0f - cosHalfAngle, 0.0f);
		const float projectedSolidAngle = PI * max(1.0f - cosHalfAngle * cosHalfAngle, 0.0f);
		if (solidAngle <= 1.0e-6f || projectedSolidAngle <= 1.0e-6f)
		{
			return LightSampling::PunctualDirectLightSample(centerDirectionWorld, illuminance, FLT_MAX, true);
		}

		LightSampling::DirectLightSample result = LightSampling::PunctualDirectLightSample(
		    CommonSampling::SampleConeDirection(centerDirectionWorld, coneHalfAngle, sample),
		    illuminance / projectedSolidAngle,
		    FLT_MAX,
		    true);
		result.PdfW = rcp(solidAngle);
		return result;
	}

	LightSampling::DirectLightSample SamplePointLight(float3 positionWorld, uint lightIndex, float2 sample)
	{
		const PointLightConstantBufferData light = PointLights[lightIndex];
		float distanceToLight = 0.0f;
		const float3 centerDirectionWorld = PunctualLights::GetPointLightDirection(positionWorld, lightIndex, distanceToLight);
		const float radius = max(light.SourceRadius, 0.0f);
		if (radius <= 1.0e-4f)
		{
			const float attenuation = PunctualLights::ComputeDistanceAttenuation(distanceToLight, light.Range);
			return LightSampling::PunctualDirectLightSample(centerDirectionWorld, light.Color * light.Intensity * attenuation, distanceToLight, false);
		}

		const float3 samplePositionWorld = CommonSampling::SampleSpherePoint(
		    light.Position,
		    radius,
		    positionWorld - light.Position,
		    sample);
		const float3 normalWorld = SafeNormalize(samplePositionWorld - light.Position);
		const float sphereArea = 4.0f * PI * radius * radius;
		const float3 emittedRadiance = light.Color * light.Intensity / max(PI * radius * radius, 1.0e-4f);
		return LightSampling::AreaDirectLightSample(
		    positionWorld,
		    samplePositionWorld,
		    normalWorld,
		    emittedRadiance,
		    rcp(max(sphereArea, 1.0e-4f)),
		    PunctualLights::ComputeRangeCutoff(length(samplePositionWorld - positionWorld), light.Range));
	}

	LightSampling::DirectLightSample SampleSpotLight(float3 positionWorld, uint lightIndex, float2 sample)
	{
		const SpotLightConstantBufferData light = SpotLights[lightIndex];
		float distanceToLight = 0.0f;
		const float3 centerDirectionWorld = PunctualLights::GetSpotLightDirection(positionWorld, lightIndex, distanceToLight);
		const float3 lightDirectionWorld = SafeNormalize(light.Direction, float3(0.0f, -1.0f, 0.0f));
		const float radius = max(light.SourceRadius, 0.0f);
		if (radius <= 1.0e-4f)
		{
			const float3 lightToSurfaceDirection = -centerDirectionWorld;
			const float attenuation = PunctualLights::ComputeDistanceAttenuation(distanceToLight, light.Range);
			const float coneAttenuation =
			    PunctualLights::ComputeSpotConeAttenuation(lightToSurfaceDirection, lightDirectionWorld, light.InnerConeCosine, light.OuterConeCosine);
			return LightSampling::PunctualDirectLightSample(centerDirectionWorld, light.Color * light.Intensity * attenuation * coneAttenuation, distanceToLight, false);
		}

		const float3 samplePositionWorld = CommonSampling::SampleDiskPoint(light.Position, lightDirectionWorld, radius, sample);
		const float3 surfaceToSample = samplePositionWorld - positionWorld;
		const float distanceToSample = length(surfaceToSample);
		const float3 sampledDirectionWorld = distanceToSample > 1.0e-4f ? surfaceToSample / distanceToSample : centerDirectionWorld;
		const float coneAttenuation =
		    PunctualLights::ComputeSpotConeAttenuation(-sampledDirectionWorld, lightDirectionWorld, light.InnerConeCosine, light.OuterConeCosine);
		const float diskArea = PI * radius * radius;
		const float3 emittedRadiance = light.Color * light.Intensity * coneAttenuation / max(diskArea, 1.0e-4f);
		return LightSampling::AreaDirectLightSample(
		    positionWorld,
		    samplePositionWorld,
		    lightDirectionWorld,
		    emittedRadiance,
		    rcp(max(diskArea, 1.0e-4f)),
		    PunctualLights::ComputeRangeCutoff(distanceToSample, light.Range));
	}

	void BuildRectFrame(RectLightConstantBufferData light, out float3 normalWorld, out float3 tangentWorld, out float3 bitangentWorld)
	{
		normalWorld = SafeNormalize(light.Direction, float3(0.0f, -1.0f, 0.0f));
		tangentWorld = light.Tangent - normalWorld * dot(light.Tangent, normalWorld);
		if (dot(tangentWorld, tangentWorld) <= 1.0e-8f)
		{
			CommonSampling::BuildOrthonormalBasis(normalWorld, tangentWorld, bitangentWorld);
			return;
		}

		tangentWorld = normalize(tangentWorld);
		bitangentWorld = normalize(cross(normalWorld, tangentWorld));
	}

	LightSampling::DirectLightSample SampleRectLight(float3 positionWorld, uint lightIndex, float2 sample)
	{
		const RectLightConstantBufferData light = RectLights[lightIndex];
		const float width = max(light.Width, 0.0f);
		const float height = max(light.Height, 0.0f);
		const float area = width * height;
		if (area <= 1.0e-4f)
		{
			return LightSampling::InvalidDirectLightSample();
		}

		float3 normalWorld;
		float3 tangentWorld;
		float3 bitangentWorld;
		BuildRectFrame(light, normalWorld, tangentWorld, bitangentWorld);
		const float3 samplePositionWorld =
		    light.Position +
		    tangentWorld * ((sample.x - 0.5f) * width) +
		    bitangentWorld * ((sample.y - 0.5f) * height);
		return LightSampling::AreaDirectLightSample(
		    positionWorld,
		    samplePositionWorld,
		    normalWorld,
		    light.Color * light.Luminance,
		    rcp(area),
		    1.0f);
	}
}

#endif

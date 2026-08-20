#ifndef SPARKLE_AREA_LIGHTS_HLSLI
#define SPARKLE_AREA_LIGHTS_HLSLI

#include "Resources/LightGpuData.hlsli"

#include "Common/Constants.hlsli"
#include "Common/Math.hlsli"
#include "Common/Sampling.hlsli"
#include "Lighting/LightSampling.hlsli"
#include "Lighting/PunctualLights.hlsli"
namespace AreaLights
{
	LightSampling::DirectLightSample SampleDirectionalLight(uint lightIndex, float2 sample)
	{
		const DirectionalLightGpuData light = DirectionalLights[lightIndex];
		const float3 centerDirectionWorld = PunctualLights::GetDirectionalLightDirection(lightIndex);
		const float3 illuminance = light.Color * light.Illuminance;
		const float coneHalfAngle = light.AngularSizeRadians * 0.5f;
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

		LightSampling::DirectLightSample result =
		    LightSampling::PunctualDirectLightSample(CommonSampling::SampleConeDirection(centerDirectionWorld, coneHalfAngle, sample),
		                                             illuminance / projectedSolidAngle,
		                                             FLT_MAX,
		                                             true);
		result.PdfW = rcp(solidAngle);
		return result;
	}

	LightSampling::DirectLightSample SamplePointLight(float3 positionWorld, uint lightIndex, float2 sample)
	{
		const PointLightGpuData light = PointLights[lightIndex];
		float distanceToLight = 0.0f;
		const float3 centerDirectionWorld = PunctualLights::GetPointLightDirection(positionWorld, lightIndex, distanceToLight);
		const float radius = light.Radius;
		if (radius <= 1.0e-4f)
		{
			const float distanceAttenuation =
			    PunctualLights::ComputePunctualDistanceAttenuation(distanceToLight, light.Range, light.DistanceAttenuationCoefficients);
			return LightSampling::PunctualDirectLightSample(centerDirectionWorld,
			                                                light.Color * light.LuminousIntensity * distanceAttenuation,
			                                                distanceToLight,
			                                                false);
		}

		const float3 samplePositionWorld =
		    CommonSampling::SampleSpherePoint(light.Position, radius, positionWorld - light.Position, sample);
		const float3 normalWorld = SafeNormalize(samplePositionWorld - light.Position);
		const float sphereArea = 4.0f * PI * radius * radius;
		const float3 emittedRadiance = light.Color * light.LuminousIntensity / max(PI * radius * radius, 1.0e-4f);
		const float distanceToSample = length(samplePositionWorld - positionWorld);
		return LightSampling::AreaDirectLightSample(
		    positionWorld,
		    samplePositionWorld,
		    normalWorld,
		    emittedRadiance,
		    rcp(max(sphereArea, 1.0e-4f)),
		    PunctualLights::ComputeRangeAttenuation(distanceToSample, light.Range)
		        * PunctualLights::ComputeAreaDistanceAttenuationCorrection(distanceToSample, light.DistanceAttenuationCoefficients));
	}

	LightSampling::DirectLightSample SampleSpotLight(float3 positionWorld, uint lightIndex, float2 sample)
	{
		const SpotLightGpuData light = SpotLights[lightIndex];
		float distanceToLight = 0.0f;
		const float3 centerDirectionWorld = PunctualLights::GetSpotLightDirection(positionWorld, lightIndex, distanceToLight);
		const float3 lightDirectionWorld = normalize(light.Direction);
		const float radius = light.Radius;
		if (radius <= 1.0e-4f)
		{
			const float3 lightToSurfaceDirection = -centerDirectionWorld;
			const float distanceAttenuation =
			    PunctualLights::ComputePunctualDistanceAttenuation(distanceToLight, light.Range, light.DistanceAttenuationCoefficients);
			const float angularAttenuation = PunctualLights::ComputeSpotAngularAttenuation(lightToSurfaceDirection,
			                                                                               lightDirectionWorld,
			                                                                               light.InnerAngleCosine,
			                                                                               light.OuterAngleCosine);
			return LightSampling::PunctualDirectLightSample(centerDirectionWorld,
			                                                light.Color * light.LuminousIntensity * distanceAttenuation
			                                                    * angularAttenuation,
			                                                distanceToLight,
			                                                false);
		}

		const float3 samplePositionWorld = CommonSampling::SampleDiskPoint(light.Position, lightDirectionWorld, radius, sample);
		const float3 surfaceToSample = samplePositionWorld - positionWorld;
		const float distanceToSample = length(surfaceToSample);
		const float3 sampledDirectionWorld = distanceToSample > 1.0e-4f ? surfaceToSample / distanceToSample : centerDirectionWorld;
		const float angularAttenuation = PunctualLights::ComputeSpotAngularAttenuation(-sampledDirectionWorld,
		                                                                               lightDirectionWorld,
		                                                                               light.InnerAngleCosine,
		                                                                               light.OuterAngleCosine);
		const float diskArea = PI * radius * radius;
		const float3 emittedRadiance = light.Color * light.LuminousIntensity * angularAttenuation / max(diskArea, 1.0e-4f);
		return LightSampling::AreaDirectLightSample(
		    positionWorld,
		    samplePositionWorld,
		    lightDirectionWorld,
		    emittedRadiance,
		    rcp(max(diskArea, 1.0e-4f)),
		    PunctualLights::ComputeRangeAttenuation(distanceToSample, light.Range)
		        * PunctualLights::ComputeAreaDistanceAttenuationCorrection(distanceToSample, light.DistanceAttenuationCoefficients));
	}

	void BuildRectFrame(RectLightGpuData light, out float3 normalWorld, out float3 tangentWorld, out float3 bitangentWorld)
	{
		normalWorld = normalize(light.Direction);
		tangentWorld = light.Tangent - normalWorld * dot(light.Tangent, normalWorld);
		tangentWorld = normalize(tangentWorld);
		bitangentWorld = normalize(cross(normalWorld, tangentWorld));
	}

	LightSampling::DirectLightSample SampleRectLight(float3 positionWorld, uint lightIndex, float2 sample)
	{
		const RectLightGpuData light = RectLights[lightIndex];
		const float width = light.Width;
		const float height = light.Height;
		const float area = width * height;

		float3 normalWorld;
		float3 tangentWorld;
		float3 bitangentWorld;
		BuildRectFrame(light, normalWorld, tangentWorld, bitangentWorld);
		const float3 samplePositionWorld =
		    light.Position + tangentWorld * ((sample.x - 0.5f) * width) + bitangentWorld * ((sample.y - 0.5f) * height);
		return LightSampling::AreaDirectLightSample(positionWorld,
		                                            samplePositionWorld,
		                                            normalWorld,
		                                            light.Color * light.Luminance,
		                                            rcp(area),
		                                            1.0f);
	}
}

#endif

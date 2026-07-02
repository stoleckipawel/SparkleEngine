#ifndef SPARKLE_DIRECT_LIGHT_SAMPLING_HLSLI
#define SPARKLE_DIRECT_LIGHT_SAMPLING_HLSLI

#include "Common/Color.hlsli"
#include "Lighting/AreaLights.hlsli"
#include "Resources/ConstantBuffers.hlsli"

namespace DirectLightSampling
{
	struct LightId
	{
		uint Type;
		uint Index;
	};

	struct LightCandidate
	{
		LightId Light;
		float SelectionPdf;
		float Valid;
	};

	LightCandidate InvalidLightCandidate()
	{
		LightCandidate candidate;
		candidate.Light.Type = LightSampling::LightTypeInvalid;
		candidate.Light.Index = LightSampling::LightIndexInvalid;
		candidate.SelectionPdf = 0.0f;
		candidate.Valid = 0.0f;
		return candidate;
	}

	bool IsValid(LightCandidate candidate)
	{
		return candidate.Valid > 0.5f && candidate.SelectionPdf > 0.0f;
	}

	uint GetDirectLightCount()
	{
		return ViewLighting.DirectionalLightCount +
		       ViewLighting.PointLightCount +
		       ViewLighting.SpotLightCount +
		       ViewLighting.RectLightCount;
	}

	LightId GetDirectLightId(uint linearIndex)
	{
		LightId light;
		if (linearIndex < ViewLighting.DirectionalLightCount)
		{
			light.Type = LightSampling::LightTypeDirectional;
			light.Index = linearIndex;
			return light;
		}

		linearIndex -= ViewLighting.DirectionalLightCount;
		if (linearIndex < ViewLighting.PointLightCount)
		{
			light.Type = LightSampling::LightTypePoint;
			light.Index = linearIndex;
			return light;
		}

		linearIndex -= ViewLighting.PointLightCount;
		if (linearIndex < ViewLighting.SpotLightCount)
		{
			light.Type = LightSampling::LightTypeSpot;
			light.Index = linearIndex;
			return light;
		}

		linearIndex -= ViewLighting.SpotLightCount;
		if (linearIndex < ViewLighting.RectLightCount)
		{
			light.Type = LightSampling::LightTypeRect;
			light.Index = linearIndex;
			return light;
		}

		light.Type = LightSampling::LightTypeInvalid;
		light.Index = LightSampling::LightIndexInvalid;
		return light;
	}

	bool CastsShadow(LightId light)
	{
		if (light.Type == LightSampling::LightTypeDirectional)
		{
			return DirectionalLights[light.Index].CastShadow != 0u;
		}

		if (light.Type == LightSampling::LightTypePoint)
		{
			return PointLights[light.Index].CastShadow != 0u;
		}

		if (light.Type == LightSampling::LightTypeSpot)
		{
			return SpotLights[light.Index].CastShadow != 0u;
		}

		if (light.Type == LightSampling::LightTypeRect)
		{
			return RectLights[light.Index].CastShadow != 0u;
		}

		return false;
	}

	float EstimateDirectionalLightWeight(uint lightIndex, float3 normalWorld)
	{
		const float3 directionWorld = PunctualLights::GetDirectionalLightDirection(lightIndex);
		const float noL = max(dot(normalWorld, directionWorld), 0.0f);
		return CommonColor::LuminanceRec709(max(DirectionalLights[lightIndex].Color * DirectionalLights[lightIndex].Intensity, 0.0f.xxx)) * noL;
	}

	float EstimatePointLightWeight(uint lightIndex, float3 positionWorld, float3 normalWorld)
	{
		float distanceToLight = 0.0f;
		const float3 directionWorld = PunctualLights::GetPointLightDirection(positionWorld, lightIndex, distanceToLight);
		const float noL = max(dot(normalWorld, directionWorld), 0.0f);
		const PointLightConstantBufferData light = PointLights[lightIndex];
		const float attenuation = PunctualLights::ComputeDistanceAttenuation(distanceToLight, light.Range);
		return CommonColor::LuminanceRec709(max(light.Color * light.Intensity * attenuation, 0.0f.xxx)) * noL;
	}

	float EstimateSpotLightWeight(uint lightIndex, float3 positionWorld, float3 normalWorld)
	{
		float distanceToLight = 0.0f;
		const float3 directionWorld = PunctualLights::GetSpotLightDirection(positionWorld, lightIndex, distanceToLight);
		const float noL = max(dot(normalWorld, directionWorld), 0.0f);
		const SpotLightConstantBufferData light = SpotLights[lightIndex];
		const float attenuation = PunctualLights::ComputeDistanceAttenuation(distanceToLight, light.Range);
		const float coneAttenuation = PunctualLights::ComputeSpotConeAttenuation(-directionWorld, light.Direction, light.InnerConeCosine, light.OuterConeCosine);
		return CommonColor::LuminanceRec709(max(light.Color * light.Intensity * attenuation * coneAttenuation, 0.0f.xxx)) * noL;
	}

	float EstimateRectLightWeight(uint lightIndex, float3 positionWorld, float3 normalWorld)
	{
		const RectLightConstantBufferData light = RectLights[lightIndex];
		const float width = max(light.Width, 0.0f);
		const float height = max(light.Height, 0.0f);
		const float area = width * height;
		if (area <= 1.0e-4f)
		{
			return 0.0f;
		}

		const float3 surfaceToLight = light.Position - positionWorld;
		const float distanceSquared = max(dot(surfaceToLight, surfaceToLight), 1.0e-4f);
		const float3 directionWorld = surfaceToLight * rsqrt(distanceSquared);
		const float noL = max(dot(normalWorld, directionWorld), 0.0f);
		const float cosLight = max(dot(SafeNormalize(light.Direction, float3(0.0f, -1.0f, 0.0f)), -directionWorld), 0.0f);
		return CommonColor::LuminanceRec709(max(light.Color * light.Luminance, 0.0f.xxx)) * area * noL * cosLight / distanceSquared;
	}

	float EstimateLightWeight(LightId light, float3 positionWorld, float3 normalWorld)
	{
		if (light.Type == LightSampling::LightTypeDirectional)
		{
			return EstimateDirectionalLightWeight(light.Index, normalWorld);
		}

		if (light.Type == LightSampling::LightTypePoint)
		{
			return EstimatePointLightWeight(light.Index, positionWorld, normalWorld);
		}

		if (light.Type == LightSampling::LightTypeSpot)
		{
			return EstimateSpotLightWeight(light.Index, positionWorld, normalWorld);
		}

		if (light.Type == LightSampling::LightTypeRect)
		{
			return EstimateRectLightWeight(light.Index, positionWorld, normalWorld);
		}

		return 0.0f;
	}

	LightCandidate SampleLightCandidate(float3 positionWorld, float3 normalWorld, float random)
	{
		const uint lightCount = GetDirectLightCount();
		if (lightCount == 0u)
		{
			return InvalidLightCandidate();
		}

		float totalWeight = 0.0f;
		[loop] for (uint linearIndex = 0u; linearIndex < lightCount; ++linearIndex)
		{
			totalWeight += EstimateLightWeight(GetDirectLightId(linearIndex), positionWorld, normalWorld);
		}

		if (totalWeight <= 1.0e-6f)
		{
			return InvalidLightCandidate();
		}

		const float target = min(saturate(random), 0.999999f) * totalWeight;
		float accumulatedWeight = 0.0f;
		LightCandidate fallback = InvalidLightCandidate();
		[loop] for (uint linearIndex = 0u; linearIndex < lightCount; ++linearIndex)
		{
			const LightId light = GetDirectLightId(linearIndex);
			const float weight = EstimateLightWeight(light, positionWorld, normalWorld);
			if (weight <= 0.0f)
			{
				continue;
			}

			LightCandidate candidate;
			candidate.Light = light;
			candidate.SelectionPdf = weight / totalWeight;
			candidate.Valid = 1.0f;
			fallback = candidate;

			accumulatedWeight += weight;
			if (target <= accumulatedWeight)
			{
				return candidate;
			}
		}

		return fallback;
	}

	LightSampling::DirectLightSample SampleDirectLight(LightCandidate candidate, float3 positionWorld, float2 shapeSample)
	{
		if (!IsValid(candidate))
		{
			return LightSampling::InvalidDirectLightSample();
		}

		LightSampling::DirectLightSample sample = LightSampling::InvalidDirectLightSample();
		if (candidate.Light.Type == LightSampling::LightTypeDirectional)
		{
			sample = AreaLights::SampleDirectionalLight(candidate.Light.Index, shapeSample);
		}
		else if (candidate.Light.Type == LightSampling::LightTypePoint)
		{
			sample = AreaLights::SamplePointLight(positionWorld, candidate.Light.Index, shapeSample);
		}
		else if (candidate.Light.Type == LightSampling::LightTypeSpot)
		{
			sample = AreaLights::SampleSpotLight(positionWorld, candidate.Light.Index, shapeSample);
		}
		else if (candidate.Light.Type == LightSampling::LightTypeRect)
		{
			sample = AreaLights::SampleRectLight(positionWorld, candidate.Light.Index, shapeSample);
		}

		sample.LightSelectionPdf = max(candidate.SelectionPdf, 1.0e-6f);
		return sample;
	}

	float4 PackLightCandidate(LightCandidate candidate)
	{
		return IsValid(candidate)
		           ? float4(float(candidate.Light.Type), float(candidate.Light.Index), candidate.SelectionPdf, candidate.Valid)
		           : 0.0f.xxxx;
	}

	LightCandidate UnpackLightCandidate(float4 packedCandidate)
	{
		LightCandidate candidate;
		candidate.Light.Type = (uint)(packedCandidate.x + 0.5f);
		candidate.Light.Index = (uint)(packedCandidate.y + 0.5f);
		candidate.SelectionPdf = packedCandidate.z;
		candidate.Valid = packedCandidate.w;
		if (IsValid(candidate))
		{
			return candidate;
		}

		return InvalidLightCandidate();
	}
}

#endif

#pragma once

#include "Common/Constants.hlsli"
#include "Common/Math.hlsli"
#include "Common/Sampling.hlsli"
#include "BRDF/Distribution.hlsli"

namespace BRDF
{
	namespace SpecularSampling
	{
		struct LobeSample
		{
			float3 DirectionWorld;
			float Pdf;
			float3 Throughput;
			bool Mirror;
			bool Valid;
		};

		float3 SampleGGXHalfVector(float3 normalWorld, float roughness, float2 sample)
		{
			float3 tangentWorld;
			float3 bitangentWorld;
			CommonSampling::BuildOrthonormalBasis(normalWorld, tangentWorld, bitangentWorld);

			const float alpha = roughness * roughness;
			const float alphaSquared = alpha * alpha;
			const float phi = TWO_PI * sample.x;
			const float cosTheta =
			    sqrt(saturate((1.0f - sample.y) / max(1.0f + (alphaSquared - 1.0f) * sample.y, 1.0e-4f)));
			const float sinTheta = sqrt(saturate(1.0f - cosTheta * cosTheta));
			const float3 localHalfVector = float3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
			return SafeNormalize(
			    tangentWorld * localHalfVector.x + bitangentWorld * localHalfVector.y + normalWorld * localHalfVector.z,
			    normalWorld);
		}

		LobeSample SampleReflectionLobe(
		    float3 normalWorld,
		    float3 viewDirWorld,
		    float roughness,
		    uint sampleMode,
		    float2 randomSample)
		{
			const float3 mirrorDirection = SafeNormalize(reflect(-viewDirWorld, normalWorld), normalWorld);
			if (sampleMode == 0u || roughness <= 0.0f)
			{
				LobeSample result;
				result.DirectionWorld = mirrorDirection;
				result.Pdf = 1.0f;
				result.Throughput = 1.0f.xxx;
				result.Mirror = true;
				result.Valid = true;
				return result;
			}

			const float3 halfVectorWorld = SampleGGXHalfVector(normalWorld, roughness, randomSample);
			const float3 sampleDirectionWorld = SafeNormalize(reflect(-viewDirWorld, halfVectorWorld), mirrorDirection);
			const float NoV = saturate(dot(normalWorld, viewDirWorld));
			const float NoL = saturate(dot(normalWorld, sampleDirectionWorld));
			const float NoH = saturate(dot(normalWorld, halfVectorWorld));
			const float VoH = saturate(dot(viewDirWorld, halfVectorWorld));

			if (NoV <= 0.0f || NoL <= 0.0f || NoH <= 0.0f || VoH <= 0.0f)
			{
				LobeSample result;
				result.DirectionWorld = mirrorDirection;
				result.Pdf = 1.0f;
				result.Throughput = 0.0f.xxx;
				result.Mirror = false;
				result.Valid = false;
				return result;
			}

			const float alpha = roughness * roughness;
			const float D = Distribution::Evaluate(NoH, alpha);
			const float pdf = max(D * NoH / max(4.0f * VoH, 1.0e-4f), 1.0e-4f);

			LobeSample result;
			result.DirectionWorld = sampleDirectionWorld;
			result.Pdf = pdf;
			result.Throughput = 1.0f.xxx;
			result.Mirror = false;
			result.Valid = true;
			return result;
		}
	}
}

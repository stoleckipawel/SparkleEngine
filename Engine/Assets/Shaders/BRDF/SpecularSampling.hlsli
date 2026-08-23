#pragma once

#include "/Engine/Common/Constants.hlsli"
#include "/Engine/Common/Math.hlsli"
#include "/Engine/Common/Sampling.hlsli"
#include "/Engine/BRDF/Distribution.hlsli"
#include "/Engine/BRDF/Geometry.hlsli"

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

		float3 SampleVisibleGGXHalfVector(float3 normalWorld, float3 viewDirWorld, float roughness, float2 sample)
		{
			float3 tangentWorld;
			float3 bitangentWorld;
			CommonSampling::BuildOrthonormalBasis(normalWorld, tangentWorld, bitangentWorld);

			const float alpha = roughness * roughness;
			const float3 localView = float3(
			    dot(viewDirWorld, tangentWorld),
			    dot(viewDirWorld, bitangentWorld),
			    max(dot(viewDirWorld, normalWorld), 1.0e-5f));
			const float3 stretchedView = normalize(float3(alpha * localView.xy, localView.z));

			const float lensq = dot(stretchedView.xy, stretchedView.xy);
			const float3 basis1 = lensq > 1.0e-8f
			                          ? float3(-stretchedView.y, stretchedView.x, 0.0f) * rsqrt(lensq)
			                          : float3(1.0f, 0.0f, 0.0f);
			const float3 basis2 = cross(stretchedView, basis1);
			const float radius = sqrt(saturate(sample.x));
			const float phi = TWO_PI * sample.y;
			const float diskX = radius * cos(phi);
			float diskY = radius * sin(phi);
			const float viewBlend = 0.5f * (1.0f + stretchedView.z);
			diskY = lerp(sqrt(saturate(1.0f - diskX * diskX)), diskY, viewBlend);
			const float projectedZ = sqrt(saturate(1.0f - diskX * diskX - diskY * diskY));
			const float3 visibleNormal = diskX * basis1 + diskY * basis2 + projectedZ * stretchedView;
			const float3 localHalfVector = normalize(float3(alpha * visibleNormal.xy, max(visibleNormal.z, 0.0f)));
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

			const float3 halfVectorWorld = SampleVisibleGGXHalfVector(normalWorld, viewDirWorld, roughness, randomSample);
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
			const float visibleMask = Geometry::SmithG1_GGX(NoV, alpha);
			const float pdf = D * visibleMask / max(4.0f * NoV, 1.0e-6f);
			if (pdf <= 1.0e-8f)
			{
				LobeSample result;
				result.DirectionWorld = mirrorDirection;
				result.Pdf = 0.0f;
				result.Throughput = 0.0f.xxx;
				result.Mirror = false;
				result.Valid = false;
				return result;
			}

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

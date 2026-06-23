#pragma once

#include "Common/Constants.hlsli"
#include "Common/Math.hlsli"

namespace CommonSampling
{
	static const float TwoPi = 6.28318530717958647692f;

	void BuildOrthonormalBasis(float3 normal, out float3 tangent, out float3 bitangent)
	{
		const float3 axis = SafeNormalize(normal);
		const float signValue = axis.z >= 0.0f ? 1.0f : -1.0f;
		const float basisA = -1.0f / (signValue + axis.z);
		const float basisB = axis.x * axis.y * basisA;
		tangent = float3(1.0f + signValue * axis.x * axis.x * basisA, signValue * basisB, -signValue * axis.x);
		bitangent = float3(basisB, signValue + axis.y * axis.y * basisA, -axis.y);
	}

	float3 SampleConeDirection(float3 axis, float coneHalfAngleRadians, float2 sample)
	{
		const float3 normalizedAxis = SafeNormalize(axis);
		const float clampedHalfAngle = max(coneHalfAngleRadians, 0.0f);
		if (clampedHalfAngle <= 0.0f)
		{
			return normalizedAxis;
		}

		const float cosThetaMin = cos(clampedHalfAngle);
		const float cosTheta = lerp(1.0f, cosThetaMin, sample.x);
		const float sinTheta = sqrt(saturate(1.0f - cosTheta * cosTheta));
		const float phi = TwoPi * sample.y;

		float3 tangent;
		float3 bitangent;
		BuildOrthonormalBasis(normalizedAxis, tangent, bitangent);
		return SafeNormalize(
		    tangent * (cos(phi) * sinTheta) +
		    bitangent * (sin(phi) * sinTheta) +
		    normalizedAxis * cosTheta);
	}

	float3 SampleSpherePoint(float3 center, float radius, float3 referenceDirection, float2 sample)
	{
		if (radius <= 0.0f)
		{
			return center;
		}

		const float z = 1.0f - 2.0f * sample.x;
		const float radial = sqrt(saturate(1.0f - z * z));
		const float phi = TwoPi * sample.y;
		float3 tangent;
		float3 bitangent;
		const float3 axis = length(referenceDirection) > EPSILON ? SafeNormalize(referenceDirection) : float3(0.0f, 1.0f, 0.0f);
		BuildOrthonormalBasis(axis, tangent, bitangent);
		const float3 offset =
		    tangent * (radial * cos(phi)) +
		    bitangent * (radial * sin(phi)) +
		    axis * z;
		return center + offset * radius;
	}

	float3 SampleDiskPoint(float3 center, float3 normal, float radius, float2 sample)
	{
		if (radius <= 0.0f)
		{
			return center;
		}

		const float sampleRadius = radius * sqrt(sample.x);
		const float phi = TwoPi * sample.y;
		float3 tangent;
		float3 bitangent;
		BuildOrthonormalBasis(normal, tangent, bitangent);
		return center + tangent * (sampleRadius * cos(phi)) + bitangent * (sampleRadius * sin(phi));
	}

	struct CosineHemisphereSample
	{
		float3 DirectionWorld;
		float Pdf;
		float Cosine;
	};

	CosineHemisphereSample SampleCosineHemisphere(float3 normal, float2 sample)
	{
		const float phi = TwoPi * sample.x;
		const float cosTheta = sqrt(saturate(1.0f - sample.y));
		const float sinTheta = sqrt(saturate(sample.y));

		float3 tangent;
		float3 bitangent;
		BuildOrthonormalBasis(normal, tangent, bitangent);

		CosineHemisphereSample result;
		result.DirectionWorld = SafeNormalize(
		    tangent * (cos(phi) * sinTheta) +
		    bitangent * (sin(phi) * sinTheta) +
		    SafeNormalize(normal) * cosTheta,
		    SafeNormalize(normal));
		result.Cosine = saturate(dot(SafeNormalize(normal), result.DirectionWorld));
		result.Pdf = max(result.Cosine * INV_PI, 1.0e-4f);
		return result;
	}
}

#ifndef SPARKLE_RAY_ENDPOINTS_HLSLI
#define SPARKLE_RAY_ENDPOINTS_HLSLI

#include "/Engine/Common/Constants.hlsli"
#include "/Engine/RayTracing/RayTracingEvaluatedTriangle.hlsli"

namespace RayEndpoints
{
	static const float ReconstructionError = 0x1.0p-24f;
	static const float TriangleIntersectionError = 0x1.8p-23f;
	static const float TransformError = 0x1.0p-23f;

	struct Ray
	{
		float3 Origin;
		float3 Direction;
		float TMin;
		float TMax;
	};

	Ray Primary(float3 origin, float3 direction, float tMin, float tMax)
	{
		Ray ray;
		ray.Origin = origin;
		ray.Direction = direction;
		ray.TMin = tMin;
		ray.TMax = tMax;
		return ray;
	}

	float NextFloat(float value, float direction)
	{
		if (value == 0.0f)
		{
			return direction > 0.0f ? asfloat(1u) : asfloat(0x80000001u);
		}
		const uint increment = (value > 0.0f) == (direction > 0.0f) ? 1u : 0xFFFFFFFFu;
		return asfloat(asuint(value) + increment);
	}

	float3 RoundOutward(float3 position, float3 displacement)
	{
		return float3(displacement.x == 0.0f ? position.x : NextFloat(position.x, displacement.x),
		              displacement.y == 0.0f ? position.y : NextFloat(position.y, displacement.y),
		              displacement.z == 0.0f ? position.z : NextFloat(position.z, displacement.z));
	}

	float SurfaceErrorBound(
	    RayTracingEvaluatedTriangle triangle,
	    MeshInstanceData mesh,
	    float3 positionObject,
	    float3 positionWorld,
	    float3 normalObject,
	    float3 normalWorld)
	{
		const float3 edge1 = triangle.V1.Position - triangle.V0.Position;
		const float3 edge2 = triangle.V2.Position - triangle.V0.Position;
		const float3 extent = abs(edge1) + abs(edge2) + abs(abs(edge1) - abs(edge2));
		float3 objectError = ReconstructionError * abs(triangle.V0.Position) + TriangleIntersectionError * extent;
		const float3x3 worldLinear = (float3x3)mesh.WorldMatrix;
		const float3 worldTranslation = float3(mesh.WorldMatrix._41, mesh.WorldMatrix._42, mesh.WorldMatrix._43);
		const float3 worldError = mul(objectError, abs(worldLinear))
		                        + TransformError * (abs(mul(positionObject, worldLinear)) + abs(worldTranslation));
		objectError += TransformError * mul(float4(abs(positionWorld), 1.0f), abs(mesh.WorldInverseMatrix)).xyz;
		const float3 transformedNormal = mul(normalObject, (float3x3)mesh.WorldInverseTranspose);
		const float inverseNormalLength = rsqrt(dot(transformedNormal, transformedNormal));
		return dot(abs(normalWorld), worldError) + dot(abs(normalObject), objectError) * inverseNormalLength;
	}

	float3 OffsetSurface(float3 position, float3 geometricNormal, float errorBound, float3 direction)
	{
		const float side = dot(direction, geometricNormal) >= 0.0f ? 1.0f : -1.0f;
		const float3 displacement = side * errorBound * geometricNormal;
		return RoundOutward(position + displacement, displacement);
	}

	float AnalyticPositionError(float3 position, float3 normal)
	{
		return TransformError * dot(abs(normal), abs(position));
	}

	Ray Continuation(float3 position, float3 geometricNormal, float errorBound, float3 direction)
	{
		Ray ray;
		ray.Origin = OffsetSurface(position, geometricNormal, errorBound, direction);
		ray.Direction = direction;
		ray.TMin = 0.0f;
		ray.TMax = FLT_MAX;
		return ray;
	}

	Ray Connection(float3 position,
	               float3 geometricNormal,
	               float errorBound,
	               float3 targetPosition,
	               float3 targetGeometricNormal,
	               float targetErrorBound)
	{
		const float3 semanticDirection = normalize(targetPosition - position);
		const float3 source = OffsetSurface(position, geometricNormal, errorBound, semanticDirection);
		const float3 target = targetErrorBound == 0.0f
		    ? RoundOutward(targetPosition, -semanticDirection)
		    : OffsetSurface(targetPosition, targetGeometricNormal, targetErrorBound, -semanticDirection);
		Ray ray;
		ray.Origin = source;
		ray.Direction = target - source;
		ray.TMin = 0.0f;
		ray.TMax = asfloat(asuint(1.0f) - 1u);
		return ray;
	}
}

#endif

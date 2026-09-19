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

	struct SurfaceEndpointError
	{
		float BaseOffset;
		float4 TraversalSensitivity;
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

	float3 TransformPosition(float3 position, row_major float4x4 transform)
	{
		precise float3 result;
		result.x = transform._41 + mad(position.x, transform._11, mad(position.y, transform._21, position.z * transform._31));
		result.y = transform._42 + mad(position.x, transform._12, mad(position.y, transform._22, position.z * transform._32));
		result.z = transform._43 + mad(position.x, transform._13, mad(position.y, transform._23, position.z * transform._33));
		return result;
	}

	float3 TransformGeometricNormal(float3 normalObject, MeshInstanceData mesh)
	{
		const float3 transformedNormal = mul(normalObject, (float3x3)mesh.WorldInverseTranspose);
		return transformedNormal * rsqrt(dot(transformedNormal, transformedNormal));
	}

	SurfaceEndpointError BuildSurfaceEndpointError(RayTracingEvaluatedTriangle triangle,
	                                               MeshInstanceData mesh,
	                                               float3 positionObject,
	                                               float3 normalObject)
	{
		const float3 edge1 = triangle.V1.Position - triangle.V0.Position;
		const float3 edge2 = triangle.V2.Position - triangle.V0.Position;
		const float3 extent3 = abs(edge1) + abs(edge2) + abs(abs(edge1) - abs(edge2));
		const float extent = max(extent3.x, max(extent3.y, extent3.z));
		const float3 objectError = ReconstructionError * abs(triangle.V0.Position) + TriangleIntersectionError * extent;
		const float3x3 worldLinear = (float3x3)mesh.WorldMatrix;
		const float3 worldTranslation = float3(mesh.WorldMatrix._41, mesh.WorldMatrix._42, mesh.WorldMatrix._43);

		const float3 worldError =
		    TriangleIntersectionError * mul(abs(positionObject), abs(worldLinear)) + TransformError * abs(worldTranslation);

		const float3 transformedNormal = mul(normalObject, (float3x3)mesh.WorldInverseTranspose);
		const float inverseNormalLength = rsqrt(dot(transformedNormal, transformedNormal));
		const float3 normalWorld = transformedNormal * inverseNormalLength;

		SurfaceEndpointError result;
		result.BaseOffset = dot(abs(normalWorld), worldError) + inverseNormalLength * dot(abs(normalObject), objectError);
		result.TraversalSensitivity =
		    TransformError * inverseNormalLength * mul(abs(mesh.WorldInverseMatrix), float4(abs(normalObject), 0.0f));
		return result;
	}

	float SurfaceErrorBound(RayTracingEvaluatedTriangle triangle,
	                        MeshInstanceData mesh,
	                        float3 positionObject,
	                        float3 positionWorld,
	                        float3 normalObject)
	{
		const SurfaceEndpointError error = BuildSurfaceEndpointError(triangle, mesh, positionObject, normalObject);
		return error.BaseOffset + dot(float4(abs(positionWorld), 1.0f), error.TraversalSensitivity);
	}

	float3 OffsetSurface(float3 position, float3 geometricNormal, float errorBound, float3 direction)
	{
		const float side = dot(direction, geometricNormal) >= 0.0f ? 1.0f : -1.0f;
		precise float3 result = mad(side * errorBound, geometricNormal, position);
		return result;
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
	               float targetBaseOffset,
	               float4 targetTraversalSensitivity)
	{
		const float3 semanticDirection = normalize(targetPosition - position);
		const float3 source = OffsetSurface(position, geometricNormal, errorBound, semanticDirection);
		precise float3 direction = targetPosition - source;
		const float targetError = targetBaseOffset + dot(float4(abs(source) + abs(direction), 1.0f), targetTraversalSensitivity);
		const float targetSide = dot(-direction, targetGeometricNormal) >= 0.0f ? 1.0f : -1.0f;
		direction = mad(targetSide * targetError, targetGeometricNormal, direction);
		Ray ray;
		ray.Origin = source;
		ray.Direction = direction;
		ray.TMin = 0.0f;
		ray.TMax = asfloat(asuint(1.0f) - 1u);
		return ray;
	}
}

#endif

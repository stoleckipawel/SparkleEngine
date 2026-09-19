#ifndef SPARKLE_RAY_TRACING_EVALUATED_TRIANGLE_HLSLI
#define SPARKLE_RAY_TRACING_EVALUATED_TRIANGLE_HLSLI

#include "/Engine/Resources/MeshInstanceShaderData.hlsli"

#include "/Engine/Geometry/Morphing.hlsli"
#include "/Engine/Geometry/Skinning.hlsli"
#include "/Engine/RayTracing/RayTracingHitData.hlsli"

struct RayTracingEvaluatedVertex
{
	float3 Position;
	float3 Normal;
	float3 Tangent;
	float TangentSign;
	float2 TexCoord0;
	float4 Color;
};

struct RayTracingEvaluatedTriangle
{
	RayTracingHitInstance Instance;
	RayTracingHitMaterial Material;
	float3 BarycentricWeights;
	RayTracingEvaluatedVertex V0;
	RayTracingEvaluatedVertex V1;
	RayTracingEvaluatedVertex V2;
};

RayTracingEvaluatedVertex EvaluateRayTracingVertex(RayTracingHitVertex vertex,
                                                   MeshInstanceData mesh,
                                                   RayTracingHitInstance instance,
                                                   uint vertexIndex)
{
	const uint localVertexIndex = vertexIndex - instance.FirstVertex;
	const MorphedVertexAttributes morphed =
	    ApplyMorphing(mesh, instance.MorphTargetDeltaOffset, localVertexIndex, vertex.Position, vertex.Normal, vertex.Tangent.xyz);
	const SkinnedVertexAttributes skinned = ApplySkinning(mesh, vertexIndex, morphed.Position, morphed.Normal, morphed.Tangent);

	RayTracingEvaluatedVertex result;
	result.Position = skinned.Position;
	result.Normal = skinned.Normal;
	result.Tangent = skinned.Tangent;
	result.TangentSign = vertex.Tangent.w;
	result.TexCoord0 = vertex.TexCoord0;
	result.Color = vertex.Color;
	return result;
}

RayTracingEvaluatedTriangle EvaluateRayTracingTriangle(uint instanceId, uint primitiveIndex, float2 barycentrics)
{
	const RayTracingHitTriangle source = LoadRayTracingHitTriangle(instanceId, primitiveIndex, barycentrics);
	const MeshInstanceData mesh = MeshInstances[instanceId];
	RayTracingEvaluatedTriangle result;
	result.Instance = source.Instance;
	result.Material = source.Material;
	result.BarycentricWeights = source.BarycentricWeights;
	result.V0 = EvaluateRayTracingVertex(source.V0, mesh, source.Instance, source.VertexIndices.x);
	result.V1 = EvaluateRayTracingVertex(source.V1, mesh, source.Instance, source.VertexIndices.y);
	result.V2 = EvaluateRayTracingVertex(source.V2, mesh, source.Instance, source.VertexIndices.z);
	return result;
}

float3 InterpolateRayTracingPosition(RayTracingEvaluatedTriangle evaluatedTriangle)
{
	precise float3 edge1 = evaluatedTriangle.V1.Position - evaluatedTriangle.V0.Position;
	precise float3 edge2 = evaluatedTriangle.V2.Position - evaluatedTriangle.V0.Position;
	precise float3 position =
	    evaluatedTriangle.V0.Position + mad(evaluatedTriangle.BarycentricWeights.y, edge1, evaluatedTriangle.BarycentricWeights.z * edge2);
	return position;
}

float3 EvaluatePreviousRayTracingPosition(RayTracingHitTriangle hitTriangle, MeshInstanceData mesh)
{
	const uint localVertex0 = hitTriangle.VertexIndices.x - hitTriangle.Instance.FirstVertex;
	const uint localVertex1 = hitTriangle.VertexIndices.y - hitTriangle.Instance.FirstVertex;
	const uint localVertex2 = hitTriangle.VertexIndices.z - hitTriangle.Instance.FirstVertex;
	const MorphedVertexAttributes morphed0 = ApplyPreviousMorphing(mesh,
	                                                               hitTriangle.Instance.MorphTargetDeltaOffset,
	                                                               localVertex0,
	                                                               hitTriangle.V0.Position,
	                                                               hitTriangle.V0.Normal,
	                                                               hitTriangle.V0.Tangent.xyz);

	const MorphedVertexAttributes morphed1 = ApplyPreviousMorphing(mesh,
	                                                               hitTriangle.Instance.MorphTargetDeltaOffset,
	                                                               localVertex1,
	                                                               hitTriangle.V1.Position,
	                                                               hitTriangle.V1.Normal,
	                                                               hitTriangle.V1.Tangent.xyz);

	const MorphedVertexAttributes morphed2 = ApplyPreviousMorphing(mesh,
	                                                               hitTriangle.Instance.MorphTargetDeltaOffset,
	                                                               localVertex2,
	                                                               hitTriangle.V2.Position,
	                                                               hitTriangle.V2.Normal,
	                                                               hitTriangle.V2.Tangent.xyz);

	const float3 p0 =
	    ApplyPreviousSkinning(mesh, hitTriangle.VertexIndices.x, morphed0.Position, morphed0.Normal, morphed0.Tangent).Position;
	const float3 p1 =
	    ApplyPreviousSkinning(mesh, hitTriangle.VertexIndices.y, morphed1.Position, morphed1.Normal, morphed1.Tangent).Position;
	const float3 p2 =
	    ApplyPreviousSkinning(mesh, hitTriangle.VertexIndices.z, morphed2.Position, morphed2.Normal, morphed2.Tangent).Position;
	precise float3 edge1 = p1 - p0;
	precise float3 edge2 = p2 - p0;
	precise float3 position = p0 + mad(hitTriangle.BarycentricWeights.y, edge1, hitTriangle.BarycentricWeights.z * edge2);
	return position;
}

float2 InterpolateRayTracingTexCoord0(RayTracingEvaluatedTriangle evaluatedTriangle)
{
	return evaluatedTriangle.V0.TexCoord0 * evaluatedTriangle.BarycentricWeights.x
	    + evaluatedTriangle.V1.TexCoord0 * evaluatedTriangle.BarycentricWeights.y
	    + evaluatedTriangle.V2.TexCoord0 * evaluatedTriangle.BarycentricWeights.z;
}

float4 InterpolateRayTracingColor(RayTracingEvaluatedTriangle evaluatedTriangle)
{
	return evaluatedTriangle.V0.Color * evaluatedTriangle.BarycentricWeights.x
	    + evaluatedTriangle.V1.Color * evaluatedTriangle.BarycentricWeights.y
	    + evaluatedTriangle.V2.Color * evaluatedTriangle.BarycentricWeights.z;
}

#endif

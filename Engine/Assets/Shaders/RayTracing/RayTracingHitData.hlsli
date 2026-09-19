#ifndef SPARKLE_RAY_TRACING_HIT_DATA_HLSLI
#define SPARKLE_RAY_TRACING_HIT_DATA_HLSLI

#include "/Engine/RayTracing/RayTracingHitSurface.hlsli"
#include "/Engine/Resources/MaterialTextureMappingData.hlsli"

struct RayTracingHitVertex
{
	float3 Position;
	float3 Normal;
	float4 Tangent;
	float2 TexCoord0;
	float4 Color;
};

struct RayTracingHitInstance
{
	uint FirstVertex;
	uint FirstIndex;
	uint VertexCount;
	uint IndexCount;
	uint MaterialSlot;
	uint Flags;
	uint GeometryFlags;
	uint RejectionReason;
	uint AlphaMode;
	uint MaterialTextureFlags;
	uint AbiVersion;
	uint MorphTargetDeltaOffset;
};

struct RayTracingHitMaterial
{
	float4 BaseColor;
	float3 EmissiveColor;
	float Metallic;
	float Roughness;
	float F0;
	float AlphaCutoff;
	uint AlphaMode;
	uint TextureFlags;
	float3 SubsurfaceColor;
	float SubsurfaceStrength;
	uint Flags;
	uint4 TextureIndices0;
	uint4 TextureIndices1;
	MaterialTextureMappingData TextureMappings[8];
};

StructuredBuffer<RayTracingHitVertex> RayTracingHitVertices;
StructuredBuffer<uint> RayTracingHitIndices;
StructuredBuffer<RayTracingHitInstance> RayTracingHitInstances;
StructuredBuffer<RayTracingHitMaterial> RayTracingHitMaterials;

struct RayTracingHitTriangle
{
	RayTracingHitInstance Instance;
	RayTracingHitMaterial Material;
	float3 BarycentricWeights;
	uint3 VertexIndices;
	RayTracingHitVertex V0;
	RayTracingHitVertex V1;
	RayTracingHitVertex V2;
};

RayTracingHitTriangle LoadRayTracingHitTriangle(uint instanceId, uint primitiveIndex, float2 barycentrics)
{
	RayTracingHitTriangle triangle;
	triangle.Instance = RayTracingHitInstances[instanceId];
	triangle.Material = RayTracingHitMaterials[triangle.Instance.MaterialSlot];
	triangle.BarycentricWeights = float3(1.0f - barycentrics.x - barycentrics.y, barycentrics.x, barycentrics.y);
	const uint primitiveFirstLocalIndex = primitiveIndex * 3u;
	triangle.VertexIndices = triangle.Instance.FirstVertex
	    + uint3(RayTracingHitIndices[triangle.Instance.FirstIndex + primitiveFirstLocalIndex + 0u],
	            RayTracingHitIndices[triangle.Instance.FirstIndex + primitiveFirstLocalIndex + 1u],
	            RayTracingHitIndices[triangle.Instance.FirstIndex + primitiveFirstLocalIndex + 2u]);
	triangle.V0 = RayTracingHitVertices[triangle.VertexIndices.x];
	triangle.V1 = RayTracingHitVertices[triangle.VertexIndices.y];
	triangle.V2 = RayTracingHitVertices[triangle.VertexIndices.z];
	return triangle;
}

float2 InterpolateRayTracingHitTexCoord0(RayTracingHitTriangle triangle)
{
	return triangle.V0.TexCoord0 * triangle.BarycentricWeights.x + triangle.V1.TexCoord0 * triangle.BarycentricWeights.y
	    + triangle.V2.TexCoord0 * triangle.BarycentricWeights.z;
}

float4 InterpolateRayTracingHitColor(RayTracingHitTriangle triangle)
{
	return triangle.V0.Color * triangle.BarycentricWeights.x + triangle.V1.Color * triangle.BarycentricWeights.y
	    + triangle.V2.Color * triangle.BarycentricWeights.z;
}

#endif

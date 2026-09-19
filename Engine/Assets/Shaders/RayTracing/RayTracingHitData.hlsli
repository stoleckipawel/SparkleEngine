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

StructuredBuffer<RayTracingHitVertex> RayTracingHitVertices : register(t4096, space1);
StructuredBuffer<uint> RayTracingHitIndices : register(t4097, space1);
StructuredBuffer<RayTracingHitInstance> RayTracingHitInstances : register(t4098, space1);
StructuredBuffer<RayTracingHitMaterial> RayTracingHitMaterials : register(t4099, space1);

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
	RayTracingHitTriangle hitTriangle;
	hitTriangle.Instance = RayTracingHitInstances[instanceId];
	hitTriangle.Material = RayTracingHitMaterials[hitTriangle.Instance.MaterialSlot];
	hitTriangle.BarycentricWeights = float3(1.0f - barycentrics.x - barycentrics.y, barycentrics.x, barycentrics.y);
	const uint primitiveFirstLocalIndex = primitiveIndex * 3u;
	hitTriangle.VertexIndices = hitTriangle.Instance.FirstVertex
	    + uint3(RayTracingHitIndices[hitTriangle.Instance.FirstIndex + primitiveFirstLocalIndex + 0u],
	            RayTracingHitIndices[hitTriangle.Instance.FirstIndex + primitiveFirstLocalIndex + 1u],
	            RayTracingHitIndices[hitTriangle.Instance.FirstIndex + primitiveFirstLocalIndex + 2u]);
	hitTriangle.V0 = RayTracingHitVertices[hitTriangle.VertexIndices.x];
	hitTriangle.V1 = RayTracingHitVertices[hitTriangle.VertexIndices.y];
	hitTriangle.V2 = RayTracingHitVertices[hitTriangle.VertexIndices.z];
	return hitTriangle;
}

float2 InterpolateRayTracingHitTexCoord0(RayTracingHitTriangle hitTriangle)
{
	return hitTriangle.V0.TexCoord0 * hitTriangle.BarycentricWeights.x + hitTriangle.V1.TexCoord0 * hitTriangle.BarycentricWeights.y
	    + hitTriangle.V2.TexCoord0 * hitTriangle.BarycentricWeights.z;
}

float4 InterpolateRayTracingHitColor(RayTracingHitTriangle hitTriangle)
{
	return hitTriangle.V0.Color * hitTriangle.BarycentricWeights.x + hitTriangle.V1.Color * hitTriangle.BarycentricWeights.y
	    + hitTriangle.V2.Color * hitTriangle.BarycentricWeights.z;
}

#endif

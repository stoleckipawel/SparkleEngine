#ifndef SPARKLE_RAY_TRACING_HIT_DATA_HLSLI
#define SPARKLE_RAY_TRACING_HIT_DATA_HLSLI

#include "/Engine/RayTracing/RayTracingHitSurface.hlsli"

struct RayTracingHitVertex
{
	float3 Position;
	float3 Normal;
	float4 Tangent;
	float2 TexCoord0;
	float2 Padding0;
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
};

StructuredBuffer<RayTracingHitVertex> RayTracingHitVertices;
StructuredBuffer<uint> RayTracingHitIndices;
StructuredBuffer<RayTracingHitInstance> RayTracingHitInstances;
StructuredBuffer<RayTracingHitMaterial> RayTracingHitMaterials;

bool TryLoadRayTracingHitTriangle(uint instanceId,
                                  uint primitiveIndex,
                                  float2 barycentrics,
                                  out RayTracingHitInstance hitInstance,
                                  out RayTracingHitMaterial material,
                                  out float3 barycentricWeights,
                                  out uint3 vertexIndices,
                                  out RayTracingHitVertex v0,
                                  out RayTracingHitVertex v1,
                                  out RayTracingHitVertex v2,
                                  out uint rejectionReason)
{
	hitInstance = (RayTracingHitInstance)0;
	material = (RayTracingHitMaterial)0;
	barycentricWeights = 0.0f.xxx;
	vertexIndices = 0u.xxx;
	v0 = (RayTracingHitVertex)0;
	v1 = (RayTracingHitVertex)0;
	v2 = (RayTracingHitVertex)0;
	rejectionReason = RayTracingHitSurface::ReasonNone;

	if (instanceId >= RayTracingHitInstanceCount)
	{
		rejectionReason = RayTracingHitSurface::ReasonInstanceOutOfRange;
		return false;
	}

	hitInstance = RayTracingHitInstances[instanceId];
	if ((hitInstance.Flags & RayTracingHitSurface::InstanceFlagValid) == 0u)
	{
		rejectionReason = hitInstance.RejectionReason;
		return false;
	}
	if (hitInstance.MaterialSlot >= RayTracingHitMaterialCount)
	{
		rejectionReason = RayTracingHitSurface::ReasonInvalidMaterial;
		return false;
	}

	const uint primitiveFirstLocalIndex = primitiveIndex * 3u;
	if (primitiveFirstLocalIndex + 2u >= hitInstance.IndexCount)
	{
		rejectionReason = RayTracingHitSurface::ReasonInvalidPrimitive;
		return false;
	}

	const uint i0 = hitInstance.FirstVertex + RayTracingHitIndices[hitInstance.FirstIndex + primitiveFirstLocalIndex + 0u];
	const uint i1 = hitInstance.FirstVertex + RayTracingHitIndices[hitInstance.FirstIndex + primitiveFirstLocalIndex + 1u];
	const uint i2 = hitInstance.FirstVertex + RayTracingHitIndices[hitInstance.FirstIndex + primitiveFirstLocalIndex + 2u];
	const uint vertexEnd = hitInstance.FirstVertex + hitInstance.VertexCount;
	if (i0 >= vertexEnd || i1 >= vertexEnd || i2 >= vertexEnd)
	{
		rejectionReason = RayTracingHitSurface::ReasonInvalidVertexIndex;
		return false;
	}

	barycentricWeights = float3(1.0f - barycentrics.x - barycentrics.y, barycentrics.x, barycentrics.y);
	vertexIndices = uint3(i0, i1, i2);
	v0 = RayTracingHitVertices[i0];
	v1 = RayTracingHitVertices[i1];
	v2 = RayTracingHitVertices[i2];
	material = RayTracingHitMaterials[hitInstance.MaterialSlot];
	return true;
}

#endif

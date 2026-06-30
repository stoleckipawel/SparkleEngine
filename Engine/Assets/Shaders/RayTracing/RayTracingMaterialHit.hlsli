#pragma once

#include "Common/Math.hlsli"
#include "Geometry/Basis.hlsli"
#include "Material/MaterialNormal.hlsli"
#include "Material/MaterialTextureTable.hlsli"
#include "RayTracing/RayTracingHitSurface.hlsli"

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
	uint Padding0;
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

struct RayTracingTraceResult
{
	bool Hit;
	float RayT;
	uint InstanceId;
	uint PrimitiveIndex;
	float2 Barycentrics;
	bool AlphaCandidateSeen;
	bool AlphaCandidateAccepted;
	bool AlphaCandidateRejected;
	float AlphaCandidateValue;
	float AlphaCandidateCutoff;
};

struct RayTracingHitSurfaceData
{
	bool Valid;
	float3 PositionWorld;
	float3 NormalWorld;
	float3 TangentWorld;
	float3 BitangentWorld;
	float3 NormalTangent;
	float TangentSign;
	float2 TexCoord0;
	uint MaterialSlot;
	uint GeometryFlags;
	uint RejectionReason;
	float3 BaseColor;
	float3 EmissiveColor;
	float3 SubsurfaceColor;
	float Roughness;
	float Metallic;
	float DielectricF0;
	float Alpha;
	float SubsurfaceStrength;
};

StructuredBuffer<RayTracingHitVertex> RayTracingHitVertices;
StructuredBuffer<uint> RayTracingHitIndices;
StructuredBuffer<RayTracingHitInstance> RayTracingHitInstances;
StructuredBuffer<RayTracingHitMaterial> RayTracingHitMaterials;
Texture2D MaterialTextureTable[4096];
SamplerState MaterialTextureSampler;

float4 SampleRayTracingMaterialTexture(RayTracingHitMaterial material, uint textureSlot, float2 uv)
{
	const uint textureIndex = MaterialTextureTableSampling::ResolveTextureIndex(material.TextureIndices0, material.TextureIndices1, textureSlot);
	return MaterialTextureTableSampling::SampleLevel(MaterialTextureTable, MaterialTextureSampler, textureIndex, uv);
}

void ResolveRayTracingHitMaterialTextures(
    RayTracingHitMaterial material,
    float2 uv,
    out float4 baseColor,
    out float roughness,
    out float metallic,
    out float3 emissive,
    out float3 normalTangent)
{
	baseColor = material.BaseColor;
	roughness = material.Roughness;
	metallic = material.Metallic;
	emissive = material.EmissiveColor;
	normalTangent = float3(0.0f, 0.0f, 1.0f);

	if (MaterialTextureTableSampling::HasTexture(material.TextureFlags, MaterialTextureTableSampling::TextureSlotNormal))
	{
		normalTangent =
		    UnpackMaterialNormal(SampleRayTracingMaterialTexture(material, MaterialTextureTableSampling::TextureSlotNormal, uv).xy);
	}

	if (MaterialTextureTableSampling::HasTexture(material.TextureFlags, MaterialTextureTableSampling::TextureSlotBaseColor))
	{
		baseColor =
		    SampleRayTracingMaterialTexture(material, MaterialTextureTableSampling::TextureSlotBaseColor, uv) * material.BaseColor;
	}

	if (MaterialTextureTableSampling::HasTexture(material.TextureFlags, MaterialTextureTableSampling::TextureSlotRoughness))
	{
		roughness =
		    SampleRayTracingMaterialTexture(material, MaterialTextureTableSampling::TextureSlotRoughness, uv).r *
		    material.Roughness;
	}

	if (MaterialTextureTableSampling::HasTexture(material.TextureFlags, MaterialTextureTableSampling::TextureSlotMetallic))
	{
		metallic =
		    SampleRayTracingMaterialTexture(material, MaterialTextureTableSampling::TextureSlotMetallic, uv).r *
		    material.Metallic;
	}

	if (MaterialTextureTableSampling::HasTexture(material.TextureFlags, MaterialTextureTableSampling::TextureSlotEmissive))
	{
		emissive =
		    SampleRayTracingMaterialTexture(material, MaterialTextureTableSampling::TextureSlotEmissive, uv).rgb *
		    material.EmissiveColor;
	}
}

bool TryLoadRayTracingHitTriangle(
    uint instanceId,
    uint primitiveIndex,
    float2 barycentrics,
    out RayTracingHitInstance hitInstance,
    out RayTracingHitMaterial material,
    out float3 barycentricWeights,
    out RayTracingHitVertex v0,
    out RayTracingHitVertex v1,
    out RayTracingHitVertex v2,
    out uint rejectionReason)
{
	hitInstance = (RayTracingHitInstance) 0;
	material = (RayTracingHitMaterial) 0;
	barycentricWeights = 0.0f.xxx;
	v0 = (RayTracingHitVertex) 0;
	v1 = (RayTracingHitVertex) 0;
	v2 = (RayTracingHitVertex) 0;
	rejectionReason = RayTracingHitSurface::ReasonNone;

	if (RayTracingHitDataAvailable == 0u)
	{
		rejectionReason = RayTracingHitSurface::ReasonHitDataUnavailable;
		return false;
	}
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
	v0 = RayTracingHitVertices[i0];
	v1 = RayTracingHitVertices[i1];
	v2 = RayTracingHitVertices[i2];
	material = RayTracingHitMaterials[hitInstance.MaterialSlot];
	return true;
}

bool ResolveRayTracingCandidateAlpha(
    uint instanceId,
    uint primitiveIndex,
    float2 barycentrics,
    out float sampledAlpha,
    out float alphaCutoff)
{
	sampledAlpha = 1.0f;
	alphaCutoff = 0.5f;

	RayTracingHitInstance hitInstance = (RayTracingHitInstance) 0;
	RayTracingHitMaterial material = (RayTracingHitMaterial) 0;
	float3 barycentricWeights = 0.0f.xxx;
	RayTracingHitVertex v0 = (RayTracingHitVertex) 0;
	RayTracingHitVertex v1 = (RayTracingHitVertex) 0;
	RayTracingHitVertex v2 = (RayTracingHitVertex) 0;
	uint rejectionReason = RayTracingHitSurface::ReasonNone;
	if (!TryLoadRayTracingHitTriangle(instanceId, primitiveIndex, barycentrics, hitInstance, material, barycentricWeights, v0, v1, v2, rejectionReason))
	{
		return true;
	}

	if (material.AlphaMode == RayTracingHitSurface::AlphaModeBlended)
	{
		return true;
	}
	if (material.AlphaMode != RayTracingHitSurface::AlphaModeTested)
	{
		return true;
	}

	const float2 uv = v0.TexCoord0 * barycentricWeights.x + v1.TexCoord0 * barycentricWeights.y + v2.TexCoord0 * barycentricWeights.z;
	float4 baseColor = material.BaseColor;
	if (MaterialTextureTableSampling::HasTexture(material.TextureFlags, MaterialTextureTableSampling::TextureSlotBaseColor))
	{
		baseColor =
		    SampleRayTracingMaterialTexture(material, MaterialTextureTableSampling::TextureSlotBaseColor, uv) * material.BaseColor;
	}

	sampledAlpha = saturate(baseColor.a);
	alphaCutoff = saturate(material.AlphaCutoff);
	return sampledAlpha >= alphaCutoff;
}

RayTracingHitSurfaceData ReconstructRayTracingHitSurface(RayTracingTraceResult trace, float3 rayOriginWorld, float3 rayDirectionWorld)
{
	RayTracingHitSurfaceData surface;
	surface.Valid = false;
	surface.PositionWorld = rayOriginWorld + rayDirectionWorld * trace.RayT;
	surface.NormalWorld = 0.0f.xxx;
	surface.TangentWorld = 0.0f.xxx;
	surface.BitangentWorld = 0.0f.xxx;
	surface.NormalTangent = float3(0.0f, 0.0f, 1.0f);
	surface.TangentSign = 1.0f;
	surface.TexCoord0 = 0.0f.xx;
	surface.MaterialSlot = 0u;
	surface.GeometryFlags = 0u;
	surface.RejectionReason = trace.Hit ? RayTracingHitSurface::ReasonHitDataUnavailable : RayTracingHitSurface::ReasonNoHit;
	surface.BaseColor = 0.0f.xxx;
	surface.EmissiveColor = 0.0f.xxx;
	surface.SubsurfaceColor = 0.0f.xxx;
	surface.Roughness = 1.0f;
	surface.Metallic = 0.0f;
	surface.DielectricF0 = 0.04f;
	surface.Alpha = 1.0f;
	surface.SubsurfaceStrength = 0.0f;

	if (!trace.Hit)
	{
		return surface;
	}

	RayTracingHitInstance hitInstance = (RayTracingHitInstance) 0;
	RayTracingHitMaterial material = (RayTracingHitMaterial) 0;
	float3 barycentricWeights = 0.0f.xxx;
	RayTracingHitVertex v0 = (RayTracingHitVertex) 0;
	RayTracingHitVertex v1 = (RayTracingHitVertex) 0;
	RayTracingHitVertex v2 = (RayTracingHitVertex) 0;
	uint rejectionReason = RayTracingHitSurface::ReasonNone;
	if (!TryLoadRayTracingHitTriangle(
	        trace.InstanceId,
	        trace.PrimitiveIndex,
	        trace.Barycentrics,
	        hitInstance,
	        material,
	        barycentricWeights,
	        v0,
	        v1,
	        v2,
	        rejectionReason))
	{
		surface.MaterialSlot = hitInstance.MaterialSlot;
		surface.GeometryFlags = hitInstance.GeometryFlags;
		surface.RejectionReason = rejectionReason;
		return surface;
	}
	surface.MaterialSlot = hitInstance.MaterialSlot;
	surface.GeometryFlags = hitInstance.GeometryFlags;
	surface.RejectionReason = hitInstance.RejectionReason;

	const float3 localNormal = v0.Normal * barycentricWeights.x + v1.Normal * barycentricWeights.y + v2.Normal * barycentricWeights.z;
	const float4 localTangent =
	    v0.Tangent * barycentricWeights.x + v1.Tangent * barycentricWeights.y + v2.Tangent * barycentricWeights.z;
	const MeshInstanceData meshInstance = MeshInstances[trace.InstanceId];
	const float3x3 worldInvTransposeMatrix = (float3x3) meshInstance.WorldInvTransposeMTX;
	const float3x3 worldMatrix = (float3x3) meshInstance.WorldMTX;
	float3 normalWorld = SafeNormalize(mul(localNormal, worldInvTransposeMatrix), -rayDirectionWorld);
	float3 tangentWorld = SafeNormalize(mul(localTangent.xyz, worldMatrix), 0.0f.xxx);
	const float tangentSign = localTangent.w >= 0.0f ? 1.0f : -1.0f;
	const bool twoSided = (hitInstance.Flags & RayTracingHitSurface::InstanceFlagTwoSided) != 0u;
	const bool frontFacing = dot(normalWorld, -rayDirectionWorld) >= 0.0f;
	if (!frontFacing && !twoSided)
	{
		surface.RejectionReason = RayTracingHitSurface::ReasonOneSidedBackface;
		return surface;
	}
	tangentWorld = OrthonormalizeTangent(tangentWorld, normalWorld);
	const float3 bitangentWorld = ComputeBitangentFromSign(normalWorld, tangentWorld, tangentSign);

	surface.Valid = true;
	surface.PositionWorld =
	    mul(float4(v0.Position * barycentricWeights.x + v1.Position * barycentricWeights.y + v2.Position * barycentricWeights.z, 1.0f),
	        meshInstance.WorldMTX)
	        .xyz;
	surface.NormalWorld = normalWorld;
	surface.TangentWorld = tangentWorld;
	surface.BitangentWorld = bitangentWorld;
	surface.TangentSign = tangentSign;
	surface.TexCoord0 = v0.TexCoord0 * barycentricWeights.x + v1.TexCoord0 * barycentricWeights.y + v2.TexCoord0 * barycentricWeights.z;
	surface.MaterialSlot = hitInstance.MaterialSlot;
	surface.GeometryFlags = hitInstance.GeometryFlags;
	surface.RejectionReason = RayTracingHitSurface::ReasonNone;

	float4 resolvedBaseColor = material.BaseColor;
	float resolvedRoughness = material.Roughness;
	float resolvedMetallic = material.Metallic;
	float3 resolvedEmissive = material.EmissiveColor;
	float3 resolvedNormalTangent = float3(0.0f, 0.0f, 1.0f);
	ResolveRayTracingHitMaterialTextures(
	    material,
	    surface.TexCoord0,
	    resolvedBaseColor,
	    resolvedRoughness,
	    resolvedMetallic,
	    resolvedEmissive,
	    resolvedNormalTangent);

	surface.NormalTangent = resolvedNormalTangent;
	surface.NormalWorld = TransformTangentNormalToWorld(resolvedNormalTangent, normalWorld, tangentWorld, bitangentWorld);
	if (!frontFacing)
	{
		surface.NormalWorld = -surface.NormalWorld;
		surface.TangentWorld = -surface.TangentWorld;
		surface.BitangentWorld = -surface.BitangentWorld;
	}
	surface.BaseColor = saturate(resolvedBaseColor.rgb);
	surface.EmissiveColor = max(resolvedEmissive, 0.0f.xxx);
	surface.SubsurfaceColor = saturate(material.SubsurfaceColor);
	surface.Roughness = saturate(resolvedRoughness);
	surface.Metallic = saturate(resolvedMetallic);
	surface.DielectricF0 = saturate(material.F0);
	surface.Alpha = saturate(resolvedBaseColor.a);
	surface.SubsurfaceStrength = saturate(material.SubsurfaceStrength);
	return surface;
}

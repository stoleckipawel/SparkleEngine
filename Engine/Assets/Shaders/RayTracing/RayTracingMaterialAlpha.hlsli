#pragma once

#include "/Engine/Material/MaterialTextureTable.hlsli"
#include "/Engine/RayTracing/RayTracingHitData.hlsli"

Texture2D MaterialTextureTable[4096];

float4 SampleRayTracingMaterialTexture(RayTracingHitMaterial material, uint textureSlot, float2 uv)
{
	const uint textureIndex =
	    MaterialTextureTableSampling::ResolveTextureIndex(material.TextureIndices0, material.TextureIndices1, textureSlot);
	const MaterialTextureMappingData mapping = material.TextureMappings[textureSlot];
	const float2 mappedUv = MaterialTextureTableSampling::TransformUv(uv, mapping.UvLinear, mapping.UvOffset);
	return MaterialTextureTableSampling::SampleBaseLevelBilinear(MaterialTextureTable, textureIndex, mappedUv, mapping.AddressModes);
}

bool ResolveRayTracingCandidateAlpha(uint instanceId,
                                     uint primitiveIndex,
                                     float2 barycentrics,
                                     bool frontFace,
                                     out float sampledAlpha,
                                     out float alphaCutoff)
{
	const RayTracingHitTriangle triangle = LoadRayTracingHitTriangle(instanceId, primitiveIndex, barycentrics);
	if (!frontFace && (triangle.Instance.Flags & RayTracingHitSurface::InstanceFlagTwoSided) == 0u)
	{
		sampledAlpha = 1.0f;
		alphaCutoff = triangle.Material.AlphaCutoff;
		return false;
	}
	if (triangle.Material.AlphaMode != RayTracingHitSurface::AlphaModeTested)
	{
		sampledAlpha = 1.0f;
		alphaCutoff = triangle.Material.AlphaCutoff;
		return true;
	}

	float4 baseColor = triangle.Material.BaseColor;
	if (MaterialTextureTableSampling::HasTexture(
	        triangle.Material.TextureFlags, MaterialTextureTableSampling::TextureSlotBaseColor))
	{
		baseColor = SampleRayTracingMaterialTexture(
		                triangle.Material,
		                MaterialTextureTableSampling::TextureSlotBaseColor,
		                InterpolateRayTracingHitTexCoord0(triangle))
		          * triangle.Material.BaseColor;
	}
	baseColor *= InterpolateRayTracingHitColor(triangle);
	sampledAlpha = baseColor.a;
	alphaCutoff = triangle.Material.AlphaCutoff;
	return sampledAlpha >= alphaCutoff;
}

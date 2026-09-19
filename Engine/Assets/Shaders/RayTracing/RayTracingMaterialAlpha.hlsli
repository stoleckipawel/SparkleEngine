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

bool PassesRayTracingMaterialAlpha(RayTracingHitMaterial material, float2 texCoord0, float4 vertexColor)
{
	if (material.AlphaMode != RayTracingHitSurface::AlphaModeTested)
	{
		return true;
	}

	float alpha = material.BaseColor.a;
	if (MaterialTextureTableSampling::HasTexture(material.TextureFlags, MaterialTextureTableSampling::TextureSlotBaseColor))
	{
		alpha *= SampleRayTracingMaterialTexture(material, MaterialTextureTableSampling::TextureSlotBaseColor, texCoord0).a;
	}
	return alpha * vertexColor.a >= material.AlphaCutoff;
}

bool ResolveRayTracingCandidateAlpha(uint instanceId, uint primitiveIndex, float2 barycentrics, bool frontFace)
{
	const RayTracingHitTriangle triangle = LoadRayTracingHitTriangle(instanceId, primitiveIndex, barycentrics);
	if (!frontFace && (triangle.Instance.Flags & RayTracingHitSurface::InstanceFlagTwoSided) == 0u)
	{
		return false;
	}
	return PassesRayTracingMaterialAlpha(triangle.Material,
	                                     InterpolateRayTracingHitTexCoord0(triangle),
	                                     InterpolateRayTracingHitColor(triangle));
}

#pragma once

namespace MaterialTextureTableSampling
{
	static const uint TextureSlotBaseColor = 0u;
	static const uint TextureSlotNormal = 1u;
	static const uint TextureSlotRoughness = 2u;
	static const uint TextureSlotMetallic = 3u;
	static const uint TextureSlotOcclusion = 4u;
	static const uint TextureSlotEmissive = 5u;
	static const uint TextureSlotSubsurfaceColor = 6u;
	static const uint TextureSlotSubsurfaceStrength = 7u;

	static const float ExplicitLod = 0.0f;

	uint TextureFlag(uint textureSlot)
	{
		return 1u << (textureSlot + 1u);
	}

	bool HasTexture(uint textureFlags, uint textureSlot)
	{
		return (textureFlags & TextureFlag(textureSlot)) != 0u;
	}

	uint ResolveTextureIndex(uint4 textureIndices0, uint4 textureIndices1, uint textureSlot)
	{
		return textureSlot < 4u ? textureIndices0[textureSlot] : textureIndices1[textureSlot - 4u];
	}

	float4 SampleLevel(Texture2D table[4096], SamplerState tableSampler, uint textureIndex, float2 uv)
	{
		return table[NonUniformResourceIndex(textureIndex)].SampleLevel(tableSampler, uv, ExplicitLod);
	}
}

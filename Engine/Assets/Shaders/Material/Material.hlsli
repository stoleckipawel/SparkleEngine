#pragma once

#include "Resources/ConstantBuffers.hlsli"
#include "Resources/Samplers.hlsli"
#include "Geometry/PixelInput.hlsli"
#include "Geometry/Transforms.hlsli"
#include "Material/MaterialNormal.hlsli"

Texture2D TextureBaseColor;
Texture2D TextureNormal;
Texture2D TextureRoughness;
Texture2D TextureMetallic;
Texture2D TextureOcclusion;
Texture2D TextureEmissive;
Texture2D TextureSubsurfaceColor;
Texture2D TextureSubsurfaceStrength;

namespace Material
{
	static const uint TextureGroupDiffuse = 1u;
	static const uint TextureGroupNormalMap = 2u;
	static const uint TextureGroupRoughness = 3u;
	static const uint TextureGroupMetallic = 4u;
	static const uint TextureGroupAmbientOcclusion = 5u;
	static const uint TextureGroupEmissive = 6u;
	static const uint TextureGroupSubsurfaceColor = 7u;
	static const uint TextureGroupSubsurfaceStrength = 8u;

	static const uint AlphaModeOpaque = 0u;
	static const uint AlphaModeMask = 1u;
	static const uint AlphaModeBlend = 2u;


	uint TextureGroupFlag(uint textureGroup)
	{
		return 1u << textureGroup;
	}

	bool HasTexture(uint textureGroup)
	{
		return (TextureFlags & TextureGroupFlag(textureGroup)) != 0u;
	}

	void ApplyAlphaMode(float alpha)
	{
		if (AlphaMode == AlphaModeMask)
		{
			clip(alpha - AlphaCutoff);
		}
	}

	struct Properties
	{
		float3 BaseColor;
		float3 NormalTangent;
		float3 NormalWorld;
		float Roughness;
		float Metallic;
		float DielectricF0;
		float AmbientOcclusion;


		float3 SubsurfaceColor;
		float SubsurfaceStrength;


		float3 Emissive;
		float Alpha;
		uint AlphaMode;
	};

	Properties MakeDefault()
	{
		Properties props;
		props.BaseColor = float3(1.0f, 1.0f, 1.0f);
		props.NormalTangent = float3(0.0f, 0.0f, 1.0f);
		props.NormalWorld = float3(0.0f, 0.0f, 1.0f);
		props.Roughness = 1.0f;
		props.Metallic = 0.0f;
		props.DielectricF0 = saturate(F0);
		props.AmbientOcclusion = 1.0f;
		props.SubsurfaceColor = float3(0.0f, 0.0f, 0.0f);
		props.SubsurfaceStrength = 0.0f;
		props.Emissive = float3(0.0f, 0.0f, 0.0f);
		props.Alpha = BaseColor.a;
		props.AlphaMode = AlphaModeOpaque;
		return props;
	}

	float4 SampleBaseColor(float2 UV)
	{
		return HasTexture(TextureGroupDiffuse) ? TextureBaseColor.Sample(SamplerAniso16xWrap, UV) * BaseColor : BaseColor;
	}

	float3 SampleNormalTangent(float2 UV)
	{
		if (!HasTexture(TextureGroupNormalMap))
		{
			return float3(0.0f, 0.0f, 1.0f);
		}

		return UnpackMaterialNormal(TextureNormal.Sample(SamplerAniso16xWrap, UV).xy);
	}

	float SampleRoughness(float2 UV)
	{
		if (!HasTexture(TextureGroupRoughness))
		{
			return Roughness;
		}

		return TextureRoughness.Sample(SamplerAniso16xWrap, UV).r * Roughness;
	}

	float3 SampleEmissive(float2 UV)
	{
		if (!HasTexture(TextureGroupEmissive))
		{
			return EmissiveColor;
		}

		return TextureEmissive.Sample(SamplerAniso16xWrap, UV).rgb * EmissiveColor;
	}

	float SampleMetallic(float2 UV)
	{
		if (!HasTexture(TextureGroupMetallic))
		{
			return Metallic;
		}

		return TextureMetallic.Sample(SamplerAniso16xWrap, UV).r * Metallic;
	}

	float SampleAmbientOcclusion(float2 UV)
	{
		if (!HasTexture(TextureGroupAmbientOcclusion))
		{
			return 1.0f;
		}

		return TextureOcclusion.Sample(SamplerAniso16xWrap, UV).r;
	}

	float3 SampleSubsurfaceColor(float2 UV)
	{
		if (!HasTexture(TextureGroupSubsurfaceColor))
		{
			return SubsurfaceColor;
		}

		return TextureSubsurfaceColor.Sample(SamplerAniso16xWrap, UV).rgb * SubsurfaceColor;
	}

	float SampleSubsurfaceStrength(float2 UV)
	{
		if (!HasTexture(TextureGroupSubsurfaceStrength))
		{
			return SubsurfaceStrength;
		}

		return TextureSubsurfaceStrength.Sample(SamplerAniso16xWrap, UV).r * SubsurfaceStrength;
	}

	Properties Sample(PS::Input Input)
	{
		Properties props = MakeDefault();
		const float4 baseColor = SampleBaseColor(Input.TexCoord);
		props.BaseColor = baseColor.rgb;
		props.Alpha = baseColor.a;
		props.AlphaMode = AlphaMode;
		ApplyAlphaMode(props.Alpha);
		props.NormalTangent = SampleNormalTangent(Input.TexCoord);
		props.NormalWorld = TransformNormalToWorld(props.NormalTangent, Input.NormalWorld, Input.TangentWorld.xyz, Input.BitangentWorld);
		if (!Input.IsFrontFace)
		{
			props.NormalWorld = -props.NormalWorld;
		}
		props.Roughness = SampleRoughness(Input.TexCoord);
		props.Metallic = SampleMetallic(Input.TexCoord);
		props.DielectricF0 = saturate(F0);
		props.AmbientOcclusion = SampleAmbientOcclusion(Input.TexCoord);
		props.SubsurfaceColor = SampleSubsurfaceColor(Input.TexCoord);
		props.SubsurfaceStrength = SampleSubsurfaceStrength(Input.TexCoord);
		props.Emissive = SampleEmissive(Input.TexCoord);
		return props;
	}
}  // namespace Material

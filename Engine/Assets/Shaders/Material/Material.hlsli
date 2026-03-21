#pragma once

#include "Resources/ConstantBuffers.hlsli"
#include "Resources/Samplers.hlsli"
#include "Geometry/PixelInput.hlsli"
#include "Geometry/Transforms.hlsli"









Texture2D TextureBaseColor : register(t0);
Texture2D TextureNormal : register(t1);
Texture2D TextureMetallicRoughness : register(t2);
Texture2D TextureOcclusion : register(t3);
Texture2D TextureEmissive : register(t4);





namespace Material
{
	static const uint TextureFlagAlbedo = 0x01u;
	static const uint TextureFlagNormal = 0x02u;
	static const uint TextureFlagMetallicRoughness = 0x04u;
	static const uint TextureFlagOcclusion = 0x08u;
	static const uint TextureFlagEmissive = 0x10u;

	static const uint AlphaModeOpaque = 0u;
	static const uint AlphaModeMask = 1u;
	static const uint AlphaModeBlend = 2u;


	float RemapDielectricF0(float EncodedF0)
	{

		return saturate(EncodedF0) * 0.08f;
	}

	bool HasTexture(uint textureFlag)
	{
		return (TextureFlags & textureFlag) != 0u;
	}

	float4 SampleBaseColorTexture(float2 uv)
	{
		return TextureBaseColor.Sample(SamplerAniso16xWrap, uv);
	}

	float3 UnpackNormal(float3 encodedNormal)
	{
		return normalize(encodedNormal * 2.0f - 1.0f);
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
		props.DielectricF0 = 0.5f;
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
		return HasTexture(TextureFlagAlbedo) ? SampleBaseColorTexture(UV) : BaseColor;
	}

	float3 SampleNormalTangent(float2 UV)
	{
		if (!HasTexture(TextureFlagNormal))
		{
			return float3(0.0f, 0.0f, 1.0f);
		}

		return UnpackNormal(TextureNormal.Sample(SamplerAniso16xWrap, UV).xyz);
	}

	float SampleRoughness(float2 UV)
	{
		if (!HasTexture(TextureFlagMetallicRoughness))
		{
			return Roughness;
		}

		return TextureMetallicRoughness.Sample(SamplerAniso16xWrap, UV).g;
	}

	float3 SampleEmissive(float2 UV)
	{
		if (!HasTexture(TextureFlagEmissive))
		{
			return EmissiveColor;
		}

		return TextureEmissive.Sample(SamplerAniso16xWrap, UV).rgb * EmissiveColor;
	}

	float SampleMetallic(float2 UV)
	{
		if (!HasTexture(TextureFlagMetallicRoughness))
		{
			return Metallic;
		}

		return TextureMetallicRoughness.Sample(SamplerAniso16xWrap, UV).b;
	}

	float SampleDielectricF0(float2 UV)
	{
		return RemapDielectricF0(F0);
	}

	float SampleAmbientOcclusion(float2 UV)
	{
		if (!HasTexture(TextureFlagOcclusion))
		{
			return 1.0f;
		}

		return TextureOcclusion.Sample(SamplerAniso16xWrap, UV).r;
	}

	float3 SampleSubsurfaceColor(float2 UV)
	{
		return float3(0.0f, 0.0f, 0.0f);
	}

	float SampleSubsurfaceStrength(float2 UV)
	{
		return 0.0f;
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
		props.NormalWorld = TransformNormalToWorld(props.NormalTangent, Input.NormalWorld, Input.TangentWorld, Input.BitangentWorld);
		props.Roughness = SampleRoughness(Input.TexCoord);
		props.Metallic = SampleMetallic(Input.TexCoord);
		props.DielectricF0 = SampleDielectricF0(Input.TexCoord);
		props.AmbientOcclusion = SampleAmbientOcclusion(Input.TexCoord);
		props.SubsurfaceColor = SampleSubsurfaceColor(Input.TexCoord);
		props.SubsurfaceStrength = SampleSubsurfaceStrength(Input.TexCoord);
		props.Emissive = SampleEmissive(Input.TexCoord);
		return props;
	}
}

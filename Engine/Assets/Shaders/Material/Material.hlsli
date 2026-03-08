#pragma once

#include "Resources/ConstantBuffers.hlsli"
#include "Resources/Samplers.hlsli"
#include "Geometry/PixelInput.hlsli"
#include "Geometry/Transforms.hlsli"

// =============================================================================
// Material System
// =============================================================================

// -----------------------------------------------------------------------------
// Texture Bindings
// -----------------------------------------------------------------------------

Texture2D TextureBaseColor : register(t0);
Texture2D TextureNormal : register(t1);
Texture2D TextureMetallicRoughness : register(t2);
Texture2D TextureOcclusion : register(t3);
Texture2D TextureEmissive : register(t4);

// -----------------------------------------------------------------------------
// Material Namespace
// -----------------------------------------------------------------------------

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

	// We store Dielectric F0 for precision and remap to actual desired F0 dielectric range
	float RemapDielectricF0(float EncodedF0)
	{
		// Remap from [0.0, 1.0] to [0.00, 0.08]
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

	// -------------------------------------------------------------------------
	// Material Properties Structure
	// -------------------------------------------------------------------------
	struct Properties
	{
		// Core PBR parameters
		float3 BaseColor;        // Albedo for dielectrics, reflectance for metals
		float3 NormalTangent;    // Normal in tangent space (from normal map)
		float3 NormalWorld;      // Normal in world space (after normal mapping)
		float Roughness;         // Perceptual roughness [0=smooth, 1=rough]
		float Metallic;          // Metalness [0=dielectric, 1=metal]
		float DielectricF0;      // F0 reflectance for dielectrics 0.0 - 1.0 -> remapped[0.00, 0.08]
		float AmbientOcclusion;  // Baked occlusion [0=occluded, 1=exposed]

		// Subsurface scattering
		float3 SubsurfaceColor;    // Color tint for subsurface scattering
		float SubsurfaceStrength;  // Intensity of SSS effect [0=none, 1=full]

		// Emission
		float3 Emissive;  // Self-illumination (HDR, can exceed 1.0)
		float Alpha;
		uint AlphaMode;
	};

	// -------------------------------------------------------------------------
	// Default Material
	// -------------------------------------------------------------------------
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

	// -------------------------------------------------------------------------
	// Individual Texture Samplers
	// -------------------------------------------------------------------------

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
		return 0.0f;  // Disabled by default
	}

	// -------------------------------------------------------------------------
	// Full Material Sampling
	// -------------------------------------------------------------------------

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
}  // namespace Material

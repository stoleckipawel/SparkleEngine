#pragma once

// =============================================================================
// Debug View Modes
// =============================================================================
// Keep these indices in sync with Engine/UI/Public/Sections/ViewMode.h.

namespace ViewMode
{
	static const uint Lit = 0u;
	static const uint GBufferDiffuse = 1u;
	static const uint GBufferNormal = 2u;
	static const uint GBufferRoughness = 3u;
	static const uint GBufferMetallic = 4u;
	static const uint GBufferEmissive = 5u;
	static const uint GBufferAmbientOcclusion = 6u;
	static const uint GBufferSubsurfaceColor = 7u;
	static const uint GBufferSubsurfaceStrength = 8u;
	static const uint DirectDiffuse = 9u;
	static const uint DirectSpecular = 10u;
	static const uint DirectSubsurface = 11u;
	static const uint IndirectDiffuse = 12u;
	static const uint IndirectSpecular = 13u;

	float3 PreviewScalar(float v)
	{
		return saturate(v).xxx;
	}

	float3 PreviewNormal(float3 n)
	{
		// Map [-1, 1] to [0, 1] for visualization.
		return normalize(n) * 0.5f + 0.5f;
	}

	float3 PreviewHdr(float3 c)
	{
		// Simple Reinhard tone-map for inspecting HDR lighting terms.
		const float3 safe = max(c, 0.0f);
		return safe / (1.0f + safe);
	}

	float3 Resolve(
	    float3 lit,
	    Material::Properties matProps,
	    float3 directDiffuse,
	    float3 directSpecular,
	    float3 directSubsurface,
	    float3 indirectDiffuse,
	    float3 indirectSpecular)
	{
		switch (ViewModeIndex)
		{
			case Lit:
				return lit;

			case GBufferDiffuse:
				return saturate(matProps.BaseColor);

			case GBufferNormal:
				return PreviewNormal(matProps.NormalWorld);

			case GBufferRoughness:
				return PreviewScalar(matProps.Roughness);

			case GBufferMetallic:
				return PreviewScalar(matProps.Metallic);

			case GBufferEmissive:
				return PreviewHdr(matProps.Emissive);

			case GBufferAmbientOcclusion:
				return PreviewScalar(matProps.AmbientOcclusion);

			case GBufferSubsurfaceColor:
				return saturate(matProps.SubsurfaceColor);

			case GBufferSubsurfaceStrength:
				return PreviewScalar(matProps.SubsurfaceStrength);

			case DirectDiffuse:
				return PreviewHdr(directDiffuse);

			case DirectSpecular:
				return PreviewHdr(directSpecular);

			case DirectSubsurface:
				return PreviewHdr(directSubsurface);

			case IndirectDiffuse:
				return PreviewHdr(indirectDiffuse);

			case IndirectSpecular:
				return PreviewHdr(indirectSpecular);

			default:
				return lit;
		}
	}
}  // namespace ViewMode

#pragma once

#include "Debug/RenderViewModeConstants.hlsli"

namespace ViewMode
{
	float3 PreviewScalar(float v)
	{
		return saturate(v).xxx;
	}

	float3 PreviewNormal(float3 n)
	{
		return normalize(n) * 0.5f + 0.5f;
	}

	float3 PreviewHdr(float3 c)
	{
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
			case Wireframe:
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

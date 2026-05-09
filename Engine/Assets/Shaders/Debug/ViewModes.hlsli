#pragma once

namespace ViewMode
{
	static const uint Lit = 0u;
	static const uint Wireframe = 1u;
	static const uint GBufferDiffuse = 2u;
	static const uint GBufferNormal = 3u;
	static const uint GBufferRoughness = 4u;
	static const uint GBufferMetallic = 5u;
	static const uint GBufferEmissive = 6u;
	static const uint GBufferAmbientOcclusion = 7u;
	static const uint GBufferSubsurfaceColor = 8u;
	static const uint GBufferSubsurfaceStrength = 9u;
	static const uint DirectDiffuse = 10u;
	static const uint DirectSpecular = 11u;
	static const uint DirectSubsurface = 12u;
	static const uint IndirectDiffuse = 13u;
	static const uint IndirectSpecular = 14u;
	static const uint IndirectSubsurface = 15u;

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
	    float3 indirectSpecular,
	    float3 indirectSubsurface)
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

			case IndirectSubsurface:
				return PreviewHdr(indirectSubsurface);
			default:
				return lit;
		}
	}
}  // namespace ViewMode

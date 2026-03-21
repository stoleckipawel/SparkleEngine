#pragma once

#include "Common/Constants.hlsli"
#include "Common/Math.hlsli"
#include "BRDF/Config.hlsli"
#include "BRDF/ShadingData.hlsli"









namespace BRDF
{
	namespace Subsurface
	{








		float3 WrapLighting(float3 albedo, float3 subsurfaceColor, float NoL, float wrap)
		{
			const float wrappedNoL = (NoL + wrap) / (1.0f + wrap);
			const float scatterWidth = saturate(wrappedNoL);
			return albedo * subsurfaceColor * scatterWidth * INV_PI;
		}










		float3 Disney(float3 albedo, float3 subsurfaceColor, float roughness, float NoV, float NoL, float LoH)
		{
			const float FL = Pow5(1.0f - NoL);
			const float FV = Pow5(1.0f - NoV);

			const float Fss90 = roughness * LoH * LoH;
			const float Fss = lerp(1.0f, Fss90, FL) * lerp(1.0f, Fss90, FV);
			const float ss = 1.25f * (Fss * (1.0f / (NoL + NoV + EPSILON) - 0.5f) + 0.5f);

			return albedo * subsurfaceColor * INV_PI * ss;
		}





		float3 EvaluateDirect(float3 albedo, float3 subsurfaceColor, float roughness, float subsurfaceStrength, ShadingData sd)
		{
#if BRDF_SUBSURFACE_MODEL == BRDF_SUBSURFACE_NONE
			return float3(0.0f, 0.0f, 0.0f);
#elif BRDF_SUBSURFACE_MODEL == BRDF_SUBSURFACE_WRAP
			return WrapLighting(albedo, subsurfaceColor, sd.NoL, subsurfaceStrength);
#elif BRDF_SUBSURFACE_MODEL == BRDF_SUBSURFACE_DISNEY
			return Disney(albedo, subsurfaceColor, roughness, sd.NoV, sd.NoL, sd.LoH) * subsurfaceStrength;
#else
			return float3(0.0f, 0.0f, 0.0f);
#endif
		}

	}
}

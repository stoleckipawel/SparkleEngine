#pragma once

#include "BRDF/Config.hlsli"







namespace BRDF
{
	namespace Occlusion
	{















		float3 MultibounceAO_Jimenez(float ao, float3 albedo)
		{
			const float3 a = 2.0404f * albedo - 0.3324f;
			const float3 b = -4.7951f * albedo + 0.6417f;
			const float3 c = 2.7552f * albedo + 0.6903f;
			return max(ao.xxx, ((ao * a + b) * ao + c) * ao);
		}


		float3 MultibounceAO(float ao, float3 albedo)
		{
#if BRDF_MULTIBOUNCE_AO_MODEL == BRDF_MULTIBOUNCE_AO_JIMENEZ
			return MultibounceAO_Jimenez(ao, albedo);
#else
			return ao.xxx;
#endif
		}
















		float SpecularOcclusion_Lagarde(float NoV, float ao)
		{
			return saturate(pow(NoV + ao, 2.0f) - 1.0f + ao);
		}







		float SpecularOcclusion_LagardeApprox(float NoV, float ao, float roughness)
		{
			return saturate(pow(NoV + ao, exp2(-16.0f * roughness - 1.0f)) - 1.0f + ao);
		}







		float SpecularOcclusion_GTAO(float NoV, float ao, float roughness)
		{
			return saturate(pow(NoV + ao, roughness * roughness) - 1.0f + ao);
		}


		float SpecularOcclusion(float NoV, float ao, float roughness)
		{
#if BRDF_SO_MODEL == BRDF_SO_NONE
			return 1.0f;
#elif BRDF_SO_MODEL == BRDF_SO_LAGARDE
			return SpecularOcclusion_Lagarde(NoV, ao);
#elif BRDF_SO_MODEL == BRDF_SO_LAGARDE_APPROX
			return SpecularOcclusion_LagardeApprox(NoV, ao, roughness);
#else
			return SpecularOcclusion_GTAO(NoV, ao, roughness);
#endif
		}

	}
}

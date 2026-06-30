#pragma once

#include "Common/Constants.hlsli"
#include "BRDF/Config.hlsli"

namespace BRDF
{
	namespace Geometry
	{
		float Lambda_GGX(float NdotX, float alpha)
		{
			const float a2 = alpha * alpha;
			const float NdotX2 = max(NdotX * NdotX, EPSILON);
			return (-1.0f + sqrt(1.0f + a2 * (1.0f - NdotX2) / NdotX2)) * 0.5f;
		}

		float SmithG1_SchlickGGX(float NdotX, float k)
		{
			return NdotX / (NdotX * (1.0f - k) + k);
		}

		float SmithG1_GGX(float NdotX, float alpha)
		{
			return 1.0f / (1.0f + Lambda_GGX(NdotX, alpha));
		}

		float Smith_Uncorrelated_SchlickGGX(float NoV, float NoL, float roughness)
		{
			const float r = roughness + 1.0f;
			const float k = (r * r) / 8.0f;
			return SmithG1_SchlickGGX(NoV, k) * SmithG1_SchlickGGX(NoL, k);
		}

		float Smith_Uncorrelated_GGX(float NoV, float NoL, float alpha)
		{
			return SmithG1_GGX(NoV, alpha) * SmithG1_GGX(NoL, alpha);
		}

		float Smith_HeightCorrelated_GGX(float NoV, float NoL, float alpha)
		{
			const float a2 = alpha * alpha;
			const float GGXV = NoL * sqrt(NoV * NoV * (1.0f - a2) + a2);
			const float GGXL = NoV * sqrt(NoL * NoL * (1.0f - a2) + a2);
			return 0.5f / max(GGXV + GGXL, EPSILON);
		}

		float Smith_HeightCorrelated_GGX_Fast(float NoV, float NoL, float roughness)
		{
			const float a = roughness;
			const float GGXV = NoL * (NoV * (1.0f - a) + a);
			const float GGXL = NoV * (NoL * (1.0f - a) + a);
			return 0.5f / max(GGXV + GGXL, EPSILON);
		}

		float Kelemen(float VoH)
		{
			return 1.0f / max(4.0f * VoH * VoH, EPSILON);
		}

		float Neumann(float NoV, float NoL)
		{
			return (NoV * NoL) / max(max(NoV, NoL), EPSILON);
		}

		float EvaluateDirect(float NoV, float NoL, float VoH, float roughness, float alpha)
		{
#if BRDF_GEOMETRY_MODEL == BRDF_GEOMETRY_SMITH_GGX
			return Smith_Uncorrelated_SchlickGGX(NoV, NoL, roughness);
#elif BRDF_GEOMETRY_MODEL == BRDF_GEOMETRY_SMITH_GGX_CORRELATED
			return Smith_HeightCorrelated_GGX(NoV, NoL, alpha);
#elif BRDF_GEOMETRY_MODEL == BRDF_GEOMETRY_SMITH_GGX_CORRELATED_FAST
			return Smith_HeightCorrelated_GGX_Fast(NoV, NoL, roughness);
#elif BRDF_GEOMETRY_MODEL == BRDF_GEOMETRY_KELEMEN
			return Kelemen(VoH);
#elif BRDF_GEOMETRY_MODEL == BRDF_GEOMETRY_NEUMANN
			return Neumann(NoV, NoL);
#else
			return Smith_HeightCorrelated_GGX(NoV, NoL, alpha);
#endif
		}

		float EvaluateIndirect(float NoV, float NoL, float alpha)
		{
#if BRDF_GEOMETRY_MODEL == BRDF_GEOMETRY_SMITH_GGX
			const float k = (alpha * alpha) / 2.0f;
			return SmithG1_SchlickGGX(NoV, k) * SmithG1_SchlickGGX(NoL, k);
#elif BRDF_GEOMETRY_MODEL == BRDF_GEOMETRY_SMITH_GGX_CORRELATED
			return Smith_HeightCorrelated_GGX(NoV, NoL, alpha);
#elif BRDF_GEOMETRY_MODEL == BRDF_GEOMETRY_SMITH_GGX_CORRELATED_FAST
			const float roughness = sqrt(alpha);
			return Smith_HeightCorrelated_GGX_Fast(NoV, NoL, roughness);
#else
			return Smith_HeightCorrelated_GGX(NoV, NoL, alpha);
#endif
		}

	}  // namespace Geometry
}  // namespace BRDF

#pragma once

#include "/Engine/Common/Constants.hlsli"
#include "/Engine/BRDF/Config.hlsli"
#include "/Engine/BRDF/ShadingData.hlsli"
#include "/Engine/BRDF/Fresnel.hlsli"
#include "/Engine/BRDF/Distribution.hlsli"
#include "/Engine/BRDF/Geometry.hlsli"

namespace BRDF
{
	namespace Specular
	{
		float3 CookTorrance(float D, float3 F, float G, float NoV, float NoL)
		{
#if BRDF_GEOMETRY_MODEL == BRDF_GEOMETRY_SMITH_GGX_CORRELATED || BRDF_GEOMETRY_MODEL == BRDF_GEOMETRY_SMITH_GGX_CORRELATED_FAST

			return D * F * G;
#else
			const float denom = max(4.0f * NoV * NoL, EPSILON);
			return (D * F * G) / denom;
#endif
		}

		float3 EvaluateDirect(ShadingData sd, float roughness, float3 F)
		{
			const float alpha = roughness * roughness;

			const float D = Distribution::Evaluate(sd.NoH, alpha);
			const float G = Geometry::EvaluateDirect(sd.NoV, sd.NoL, sd.VoH, roughness, alpha);

			return CookTorrance(D, F, G, sd.NoV, sd.NoL);
		}

		float2 ApproximateBRDFIntegration(float NoV, float roughness)
		{
			const float4 c0 = float4(-1.0f, -0.0275f, -0.572f, 0.022f);
			const float4 c1 = float4(1.0f, 0.0425f, 1.04f, -0.04f);
			float4 r = roughness * c0 + c1;
			float a004 = min(r.x * r.x, exp2(-9.28f * NoV)) * r.x + r.y;
			return float2(-1.04f, 1.04f) * a004 + r.zw;
		}

		float3 EvaluateIndirectWithLUT(float NoV, float3 F0, float roughness, float3 prefilteredEnv, float2 brdfLUT)
		{
			const float3 F = Fresnel::EvaluateIndirect(NoV, F0, roughness);
			return prefilteredEnv * (F * brdfLUT.x + brdfLUT.y);
		}

		float3 EvaluateIndirect(float NoV, float3 F0, float roughness, float3 prefilteredEnv)
		{
			const float2 brdf = ApproximateBRDFIntegration(NoV, roughness);
			const float3 F = Fresnel::EvaluateIndirect(NoV, F0, roughness);
			return prefilteredEnv * (F * brdf.x + brdf.y);
		}

	}
}

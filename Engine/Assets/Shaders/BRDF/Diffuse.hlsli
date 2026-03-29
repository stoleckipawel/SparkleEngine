#pragma once

#include "Common/Constants.hlsli"
#include "Common/Math.hlsli"
#include "BRDF/Config.hlsli"
#include "BRDF/ShadingData.hlsli"

namespace BRDF
{
	namespace Diffuse
	{
		float3 Lambert(float3 albedo)
		{
			return albedo * INV_PI;
		}

		float3 Burley(float3 albedo, float roughness, float NoV, float NoL, float LoH)
		{
			const float f90 = 0.5f + 2.0f * roughness * LoH * LoH;
			const float lightScatter = 1.0f + (f90 - 1.0f) * Pow5(1.0f - NoL);
			const float viewScatter = 1.0f + (f90 - 1.0f) * Pow5(1.0f - NoV);
			return albedo * INV_PI * lightScatter * viewScatter;
		}

		float3 OrenNayar(float3 albedo, float roughness, float NoV, float NoL, float3 N, float3 V, float3 L)
		{
			const float sigma2 = roughness * roughness;
			const float A = 1.0f - 0.5f * sigma2 / (sigma2 + 0.33f);
			const float B = 0.45f * sigma2 / (sigma2 + 0.09f);

			const float3 Vtan = normalize(V - N * NoV);
			const float3 Ltan = normalize(L - N * NoL);
			const float cosPhiDiff = max(0.0f, dot(Vtan, Ltan));

			const float sinThetaV = sqrt(max(0.0f, 1.0f - NoV * NoV));
			const float sinThetaL = sqrt(max(0.0f, 1.0f - NoL * NoL));
			const float sinAlpha = max(sinThetaV, sinThetaL);
			const float tanBeta = min(sinThetaV / max(NoV, EPSILON), sinThetaL / max(NoL, EPSILON));

			return albedo * INV_PI * (A + B * cosPhiDiff * sinAlpha * tanBeta);
		}

		float3 Chan(float3 albedo, float roughness, float NoV, float NoL, float LoH, float NoH)
		{
			const float f90 = 0.5f + 2.0f * roughness * LoH * LoH;

			const float FV = 1.0f + (f90 - 1.0f) * Pow5(1.0f - NoV);
			const float FL = 1.0f + (f90 - 1.0f) * Pow5(1.0f - NoL);

			const float Fd90 = f90;
			const float Fd = lerp(1.0f, Fd90, FL) * lerp(1.0f, Fd90, FV);

			const float Fss90 = roughness * LoH * LoH;
			const float Fss = lerp(1.0f, Fss90, FL) * lerp(1.0f, Fss90, FV);
			const float ss = 1.25f * (Fss * (1.0f / (NoL + NoV) - 0.5f) + 0.5f);

			return albedo * INV_PI * lerp(Fd, ss, saturate(roughness));
		}

		float3 EvaluateDirect(float3 albedo, float roughness, ShadingData sd)
		{
#if BRDF_DIFFUSE_MODEL == BRDF_DIFFUSE_LAMBERT
			return Lambert(albedo);
#elif BRDF_DIFFUSE_MODEL == BRDF_DIFFUSE_BURLEY
			return Burley(albedo, roughness, sd.NoV, sd.NoL, sd.LoH);
#elif BRDF_DIFFUSE_MODEL == BRDF_DIFFUSE_OREN_NAYAR
			return OrenNayar(albedo, roughness, sd.NoV, sd.NoL, sd.N, sd.V, sd.L);
#elif BRDF_DIFFUSE_MODEL == BRDF_DIFFUSE_CHAN
			return Chan(albedo, roughness, sd.NoV, sd.NoL, sd.LoH, sd.NoH);
#else
			return Lambert(albedo);
#endif
		}

		float3 EvaluateIndirect(float3 albedo)
		{
			return Lambert(albedo);
		}
	}  // namespace Diffuse
}  // namespace BRDF

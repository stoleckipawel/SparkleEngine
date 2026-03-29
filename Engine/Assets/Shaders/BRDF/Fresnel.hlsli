#pragma once

#include "Common/Math.hlsli"
#include "BRDF/Config.hlsli"

namespace BRDF
{
	namespace Fresnel
	{
		float3 Schlick(float VoH, float3 F0)
		{
			return F0 + (1.0f - F0) * Pow5(1.0f - VoH);
		}

		float3 SchlickRoughness(float NoV, float3 F0, float roughness)
		{
			const float3 Fr = max((1.0f - roughness).xxx, F0) - F0;
			return F0 + Fr * Pow5(1.0f - NoV);
		}

		float3 EvaluateDirect(float VoH, float3 F0)
		{
			return Schlick(VoH, F0);
		}

		float3 EvaluateIndirect(float NoV, float3 F0, float roughness)
		{
			return SchlickRoughness(NoV, F0, roughness);
		}
	}  // namespace Fresnel
}  // namespace BRDF

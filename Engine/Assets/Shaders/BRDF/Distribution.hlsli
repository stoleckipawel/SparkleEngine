#pragma once

#include "Common/Constants.hlsli"
#include "BRDF/Config.hlsli"

namespace BRDF
{
	namespace Distribution
	{
		float GGX(float NoH, float alpha)
		{
			const float a2 = alpha * alpha;
			const float f = (NoH * NoH) * (a2 - 1.0f) + 1.0f;
			return a2 / (PI * f * f);
		}

		float Beckmann(float NoH, float alpha)
		{
			const float a2 = alpha * alpha;
			const float NoH2 = NoH * NoH;
			const float exponent = (NoH2 - 1.0f) / (a2 * NoH2);
			return exp(exponent) / (PI * a2 * NoH2 * NoH2);
		}

		float BlinnPhong(float NoH, float alpha)
		{
			const float a2 = max(alpha * alpha, EPSILON);
			const float n = 2.0f / a2 - 2.0f;
			return (n + 2.0f) / (2.0f * PI) * pow(max(NoH, 0.0f), n);
		}

		float Evaluate(float NoH, float alpha)
		{
#if BRDF_DISTRIBUTION_MODEL == BRDF_DISTRIBUTION_GGX
			return GGX(NoH, alpha);
#elif BRDF_DISTRIBUTION_MODEL == BRDF_DISTRIBUTION_BECKMANN
			return Beckmann(NoH, alpha);
#elif BRDF_DISTRIBUTION_MODEL == BRDF_DISTRIBUTION_BLINN_PHONG
			return BlinnPhong(NoH, alpha);
#else
			return GGX(NoH, alpha);
#endif
		}
	}  // namespace Distribution
}  // namespace BRDF
